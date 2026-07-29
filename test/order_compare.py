#!/usr/bin/env python3
#
# Compare a captured pcap's packet order against the source pcap it was
# replayed from (#1098). Used by replay_order.sh; not a standalone test.
#
# A pure count or pure content check can't tell reordering apart from loss:
# #1078's actual defect was frames leaving out of order while every one of
# them still arrived, so this walks the capture and asks a different
# question - does each captured packet that matches something in the
# original show up no earlier, relative to the others, than it did in the
# source?
#
# Matching is by exact packet bytes, not identity, since the pcap can contain
# duplicate-looking packets; a greedy earliest-unconsumed-occurrence match
# handles duplicates while still catching a real swap. Anything captured
# that matches nothing in the original (background interface noise) is
# skipped rather than treated as a mismatch - loss and noise are not what
# this check is for; see mtu_matrix.sh's wirecheck.sh call for loss.
#
# usage: order_compare.py <original.pcap> <captured.pcap>

import struct, sys
from collections import defaultdict, deque

def read_packets(path):
    pkts = []
    with open(path, "rb") as f:
        f.read(24)  # global header
        while True:
            hdr = f.read(16)
            if len(hdr) < 16:
                break
            _, _, incl_len, _ = struct.unpack("<IIII", hdr)
            pkts.append(f.read(incl_len))
    return pkts

orig = read_packets(sys.argv[1])
cap = read_packets(sys.argv[2])

print("original packet count: %d" % len(orig))
print("captured packet count: %d" % len(cap))

if len(cap) == 0:
    # Same "capture never actually attached" signature as mtu_check_jumbo.py -
    # safe for the caller to retry the whole cycle without risking masking a
    # real reordering, since that reproduces deterministically once a capture
    # attaches at all.
    print("FAIL: captured 0 packets - capture likely wasn't attached in time")
    sys.exit(2)

# index all occurrences of each distinct packet's bytes, in original order
occurrences = defaultdict(deque)
for i, pkt in enumerate(orig):
    occurrences[pkt].append(i)

last_matched = -1
matched = 0
violations = []
for cap_i, pkt in enumerate(cap):
    candidates = occurrences.get(pkt)
    if not candidates:
        continue  # matches nothing in the source: unrelated noise, not ours

    # consume occurrences up to (not including) the first one still ahead of
    # what's already been matched, so a later duplicate can't be mistaken for
    # an earlier one that already went by
    while candidates and candidates[0] <= last_matched:
        candidates.popleft()

    if not candidates:
        violations.append((cap_i, pkt))
        continue

    last_matched = candidates.popleft()
    matched += 1

print("matched %d captured packets against the source, in order" % matched)

if violations:
    for cap_i, pkt in violations[:10]:
        print("FAIL: captured packet %d (%d bytes) arrived out of order - every one of its "
              "occurrences in the source was already accounted for by an earlier arrival "
              "(the #1078 signature)" % (cap_i, len(pkt)))
    if len(violations) > 10:
        print("... and %d more" % (len(violations) - 10))
    sys.exit(1)

if matched == 0:
    print("FAIL: none of the captured traffic matched the source pcap at all")
    sys.exit(2)

print("OK: every matched packet arrived in source order")
sys.exit(0)
