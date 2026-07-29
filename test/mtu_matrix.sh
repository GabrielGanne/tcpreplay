#!/bin/sh
#
#   Copyright (c) 2026 Fred Klassen <tcpreplay.dev at gmail dot com> - AppNeta by Broadcom
#
#   The Tcpreplay Suite of tools is free software: you can redistribute it
#   and/or modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation, either version 3 of the
#   License, or with the authors permission any later version.
#
#   The Tcpreplay Suite is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with the Tcpreplay Suite.  If not, see <http://www.gnu.org/licenses/>.
#
# Replay against dummy interfaces at MTUs the default test NIC never
# exercises (#1097).
#
# The suite otherwise runs at whatever MTU the configured test NIC happens to
# have - 1500 on every CI runner - and that is exactly why two MTU-dependent
# bugs shipped without either being caught here:
#
#   #1079  txring_mkreq() divided tp_frame_size back down to a single page
#          after growing the block to fit a jumbo MTU, so any frame that
#          needed more than a page was truncated to 4096 bytes on the wire.
#   #1090  tp_frame_size came out unaligned for small MTUs (1280, the IPv6
#          minimum, gave 4096/3 = 1365), and the kernel rejected the ring
#          outright with EINVAL - TX_RING was unusable on such an interface
#          at all.
#
# `ip link add ... type dummy` makes both cheap and hermetic: no hardware, no
# jumbo-capable NIC required, just `ip link set <dev> mtu N`. That also makes
# this Linux-only, and it needs to skip cleanly everywhere else.
#
# usage: mtu_matrix.sh <jumbo|small> <logfile> <tcpreplay-binary> [args...]
#
# Exit 0 on success, 1 on failure, 77 (automake's "skip") where dummy
# interfaces or the tools to verify them aren't available.

set -u

if [ $# -lt 3 ]; then
    echo "usage: $0 <jumbo|small> <logfile> <tcpreplay-binary> [args...]" >&2
    exit 99
fi

mode="$1"; shift
logfile="$1"; shift

dir=$(dirname "$0")

if [ "$(uname -s)" != "Linux" ]; then
    echo "mtu_matrix: dummy interfaces are Linux-only, skipping ($mode)" >> "$logfile"
    exit 77
fi

if ! command -v ip >/dev/null 2>&1; then
    echo "mtu_matrix: 'ip' not found, skipping ($mode)" >> "$logfile"
    exit 77
fi

case "$mode" in
    jumbo)
        iface="tcprjumbo0"
        mtu=9000
        pcap="$dir/test_jumbo.pcap"
        ;;
    small)
        iface="tcprsmall0"
        mtu=1280
        pcap="$dir/test.pcap"
        ;;
    *)
        echo "usage: $0 <jumbo|small> <logfile> <tcpreplay-binary> [args...]" >&2
        exit 99
        ;;
esac

cleanup() {
    ip link delete "$iface" >/dev/null 2>&1 || true
}
trap cleanup EXIT INT TERM

# in case a previous run was interrupted before its own cleanup ran
ip link delete "$iface" >/dev/null 2>&1 || true

if ! ip link add "$iface" type dummy >> "$logfile" 2>&1; then
    echo "mtu_matrix: could not create $iface (need root?), skipping ($mode)" >> "$logfile"
    exit 77
fi

if ! ip link set "$iface" mtu "$mtu" >> "$logfile" 2>&1; then
    echo "mtu_matrix: could not set mtu $mtu on $iface" >> "$logfile"
    exit 1
fi
ip link set "$iface" up >> "$logfile" 2>&1

echo "mtu_matrix: $mode - $iface at mtu $mtu" >> "$logfile"

if [ "$mode" = "small" ]; then
    # #1090 was a hard failure at ring setup (EINVAL) - the existing "did the
    # command even succeed" pattern already catches that. What it would not
    # catch is TX_RING silently reporting packets sent that never reached the
    # wire, which is exactly what wirecheck.sh guards against.
    "$dir/wirecheck.sh" "$iface" "$logfile" "$@" -i "$iface" -l 1 -t "$pcap"
    exit $?
fi

# jumbo: a wire-count check is not enough here - a truncated frame still
# counts as "sent" as far as the interface counter is concerned, #1079's
# frames went out, just short. Capture what actually crosses the wire and
# compare its length against the original.
if ! command -v tcpdump >/dev/null 2>&1; then
    echo "mtu_matrix: tcpdump not found, cannot verify jumbo frame content, skipping" >> "$logfile"
    exit 77
fi
if ! command -v python3 >/dev/null 2>&1; then
    echo "mtu_matrix: python3 not found, cannot verify jumbo frame content, skipping" >> "$logfile"
    exit 77
fi

capfile=$(mktemp) || exit 99
tcpdump_out=$(mktemp) || { rm -f "$capfile"; exit 99; }

tcpdump -i "$iface" -w "$capfile" -s 0 > "$tcpdump_out" 2>&1 &
tcpdump_pid=$!

# tcpdump prints "listening on ..." once its capture socket is actually
# attached to the interface - wait for that instead of guessing a fixed
# delay, which is racy on a loaded CI runner: a jumbo frame sent before
# tcpdump has attached is simply never seen, and looks identical to one
# that was truncated.
i=0
ready=0
while [ $i -lt 50 ]; do
    if grep -q "listening on" "$tcpdump_out" 2>/dev/null; then
        ready=1
        break
    fi
    if ! kill -0 "$tcpdump_pid" 2>/dev/null; then
        echo "mtu_matrix: tcpdump exited before it started capturing:" >> "$logfile"
        cat "$tcpdump_out" >> "$logfile"
        rm -f "$capfile" "$tcpdump_out"
        exit 1
    fi
    i=$((i + 1))
    sleep 0.1
done
if [ "$ready" -eq 0 ]; then
    echo "mtu_matrix: tcpdump did not report 'listening on' within 5s, proceeding anyway" >> "$logfile"
fi
cat "$tcpdump_out" >> "$logfile"
rm -f "$tcpdump_out"

# tcpdump prints "listening on" as soon as pcap_activate() returns, but the
# kernel-side PF_PACKET bind isn't guaranteed instantaneous relative to that
# - at -l 1 (3 packets, sent in under a millisecond) even a few milliseconds
# of residual attach latency is enough to lose every packet. Give it a beat.
sleep 0.3

"$@" -i "$iface" -l 1 -t "$pcap" >> "$logfile" 2>&1
rc=$?

# give the last frame a moment to actually reach the capture before it's torn down
sleep 0.5
kill "$tcpdump_pid" >/dev/null 2>&1
wait "$tcpdump_pid" 2>/dev/null

if [ $rc -ne 0 ]; then
    echo "mtu_matrix: tcpreplay exited $rc" >> "$logfile"
    rm -f "$capfile"
    exit $rc
fi

python3 "$dir/mtu_check_jumbo.py" "$pcap" "$capfile" >> "$logfile" 2>&1
checkrc=$?
rm -f "$capfile"
exit $checkrc
