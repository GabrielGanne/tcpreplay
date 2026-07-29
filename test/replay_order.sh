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
# Verify packets reach the wire in the order they appear in the pcap (#1098).
#
# Two 4.6 changes touched ordering directly and were both backed only by
# reasoning about the code, never measurement:
#
#   #1078  TX_RING filled ring frames out of order, reordering the wire.
#          Fixed by filling in order.
#   #1074  io_uring submissions were not chained, so a send that could not
#          complete inline was retried out of band and packets behind it
#          went out first. Fixed with IOSQE_IO_HARDLINK.
#
# The reason this was never verified before: at --topspeed, the 179-packet
# test capture is on the wire in under a millisecond, and no capture attaches
# fast enough to see it - this was attempted and failed three times while
# working on the two fixes above. --pps here paces the replay to roughly 1.8
# real seconds instead, which is what actually makes an ordering check
# feasible rather than a bigger version of the same losing race (see
# mtu_matrix.sh's replay_mtu_jumbo, which hit the identical problem: more
# packets sent instantaneously is still instantaneous).
#
# `ip link add ... type dummy` makes this cheap and hermetic - no hardware
# needed - which also makes it Linux-only; skips cleanly (exit 77) elsewhere.
#
# usage: replay_order.sh <logfile> <tcpreplay-binary> [args...]
#
# Exit 0 on success, 1 on failure, 77 (automake's "skip") where dummy
# interfaces or the tools to verify them aren't available.

set -u

if [ $# -lt 2 ]; then
    echo "usage: $0 <logfile> <tcpreplay-binary> [args...]" >&2
    exit 99
fi

logfile="$1"; shift
dir=$(dirname "$0")
iface="tcprorder0"
pcap="$dir/test.pcap"

if [ "$(uname -s)" != "Linux" ]; then
    echo "replay_order: dummy interfaces are Linux-only, skipping" >> "$logfile"
    exit 77
fi
if ! command -v ip >/dev/null 2>&1; then
    echo "replay_order: 'ip' not found, skipping" >> "$logfile"
    exit 77
fi
if ! command -v tcpdump >/dev/null 2>&1; then
    echo "replay_order: tcpdump not found, skipping" >> "$logfile"
    exit 77
fi
if ! command -v python3 >/dev/null 2>&1; then
    echo "replay_order: python3 not found, skipping" >> "$logfile"
    exit 77
fi

cleanup() {
    ip link delete "$iface" >/dev/null 2>&1 || true
}
trap cleanup EXIT INT TERM

# in case a previous run was interrupted before its own cleanup ran
ip link delete "$iface" >/dev/null 2>&1 || true

if ! ip link add "$iface" type dummy >> "$logfile" 2>&1; then
    echo "replay_order: could not create $iface (need root?), skipping" >> "$logfile"
    exit 77
fi
# test.pcap's largest frame is 1514 bytes - above a default 1500 MTU. Give it
# headroom so nothing here is incidentally exercising the MTU-truncation bugs
# replay_mtu_jumbo/replay_mtu_small already cover; this test is about order,
# not size.
ip link set "$iface" mtu 2000 >> "$logfile" 2>&1
ip link set "$iface" up >> "$logfile" 2>&1

echo "replay_order: $iface up" >> "$logfile"

# Same retry shape as replay_mtu_jumbo, and for the same reason: retrying on
# "captured nothing at all" only papers over a scheduling race on a loaded CI
# runner, never a real reordering, which reproduces deterministically once a
# capture attaches at all.
attempt=1
checkrc=2
while [ $attempt -le 3 ] && [ $checkrc -eq 2 ]; do
    if [ $attempt -gt 1 ]; then
        echo "replay_order: retrying capture (attempt $attempt/3)" >> "$logfile"
    fi

    capfile=$(mktemp) || exit 99
    tcpdump_out=$(mktemp) || { rm -f "$capfile"; exit 99; }

    tcpdump -i "$iface" -w "$capfile" -s 0 > "$tcpdump_out" 2>&1 &
    tcpdump_pid=$!

    i=0
    ready=0
    while [ $i -lt 50 ]; do
        if grep -q "listening on" "$tcpdump_out" 2>/dev/null; then
            ready=1
            break
        fi
        if ! kill -0 "$tcpdump_pid" 2>/dev/null; then
            echo "replay_order: tcpdump exited before it started capturing:" >> "$logfile"
            cat "$tcpdump_out" >> "$logfile"
            rm -f "$capfile" "$tcpdump_out"
            exit 1
        fi
        i=$((i + 1))
        sleep 0.1
    done
    if [ "$ready" -eq 0 ]; then
        echo "replay_order: tcpdump did not report 'listening on' within 5s, proceeding anyway" >> "$logfile"
    fi
    cat "$tcpdump_out" >> "$logfile"
    rm -f "$tcpdump_out"
    sleep 1

    # --pps=100 spreads the 179-packet capture over ~1.8 real seconds - the
    # actual fix for the timing problem, not a bigger burst (see the comment
    # at the top of this file and in replay_mtu_jumbo/mtu_matrix.sh).
    "$@" -i "$iface" --pps=100 "$pcap" >> "$logfile" 2>&1
    rc=$?

    sleep 0.5
    kill "$tcpdump_pid" >/dev/null 2>&1
    wait "$tcpdump_pid" 2>/dev/null

    if [ $rc -ne 0 ]; then
        echo "replay_order: tcpreplay exited $rc" >> "$logfile"
        rm -f "$capfile"
        exit $rc
    fi

    python3 "$dir/order_compare.py" "$pcap" "$capfile" >> "$logfile" 2>&1
    checkrc=$?
    rm -f "$capfile"
    attempt=$((attempt + 1))
done

exit $checkrc
