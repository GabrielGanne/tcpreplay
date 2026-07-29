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
# Check that packets a replay *reported* as sent actually reached the wire.
#
# The replay tests otherwise assert nothing beyond an exit status, which is
# how #1078 shipped: TX_RING stranded frames in the ring and counted them as
# successful, so at -l 1 tcpreplay reported 179 packets sent while
# transmitting zero - and exited 0, so every test passed. The same blind spot
# hid #1082, where --xdp --loop delivered one pass of the pcap and then
# wedged, again exiting 0.
#
# The interface's own tx_packets counter is the independent second opinion.
#
# usage: wirecheck.sh <interface> <logfile> <command> [args...]
#
# Exits 0 if the delta covers what was reported, 1 if packets went missing,
# and 77 (automake's "skip") where the counter is not readable - non-Linux,
# or an interface without /sys statistics.

set -u

if [ $# -lt 3 ]; then
    echo "usage: $0 <interface> <logfile> <command> [args...]" >&2
    exit 99
fi

iface="$1"; shift
logfile="$1"; shift

counter="/sys/class/net/${iface}/statistics/tx_packets"

if [ ! -r "$counter" ]; then
    echo "wirecheck: $counter not readable, skipping wire verification" >> "$logfile"
    # Not a failure: run the command anyway so the test still exercises it,
    # and report its own verdict.
    "$@" >> "$logfile" 2>&1
    exit $?
fi

before=$(cat "$counter")

# Keep the command's own output so the reported count can be parsed out of it,
# and still fold everything into the shared test log.
out=$(mktemp) || exit 99
"$@" > "$out" 2>&1
rc=$?
cat "$out" >> "$logfile"

after=$(cat "$counter")
delta=$((after - before))

# "Successful packets:  <n>" is what sendpacket_getstat() prints.
reported=$(sed -n 's/.*Successful packets: *\([0-9][0-9]*\).*/\1/p' "$out" | tail -1)
rm -f "$out"

if [ $rc -ne 0 ]; then
    echo "wirecheck: command exited $rc" >> "$logfile"
    exit $rc
fi

if [ -z "$reported" ]; then
    echo "wirecheck: no 'Successful packets' line to compare against; wire delta was $delta" >> "$logfile"
    exit 0
fi

# The interface counter can legitimately run *ahead* of what tcpreplay sent -
# IPv6 router solicitations and similar background chatter share the link - so
# this is a floor, not an equality. Undershooting is the failure that matters:
# it means packets were counted as sent and never transmitted.
if [ "$delta" -lt "$reported" ]; then
    echo "wirecheck: FAILED on $iface - reported $reported packets sent, but the interface" >> "$logfile"
    echo "           transmitted only $delta. Packets were counted as sent and never" >> "$logfile"
    echo "           reached the wire (the #1078 signature)." >> "$logfile"
    exit 1
fi

echo "wirecheck: OK on $iface - reported $reported, interface transmitted $delta" >> "$logfile"
exit 0
