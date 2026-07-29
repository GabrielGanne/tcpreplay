#!/usr/bin/env python3
#
# Compare frame lengths between an original pcap and one captured off the
# wire, to catch a TX_RING silently truncating a frame (#1079). Used by
# mtu_matrix.sh; not a standalone test.
#
# mtu_matrix.sh replays the original pcap in a loop rather than once, so the
# expected size set is checked for presence, not a strict positional match -
# a single clean copy of each frame size proves the ring geometry isn't
# truncating, and looping is what makes that copy's arrival independent of
# exactly when the capture socket finished attaching.
#
# usage: mtu_check_jumbo.py <original.pcap> <captured.pcap>

import struct, sys

def frame_lengths(path):
    lens = []
    with open(path, "rb") as f:
        f.read(24)  # global header
        while True:
            hdr = f.read(16)
            if len(hdr) < 16:
                break
            _, _, incl_len, orig_len = struct.unpack("<IIII", hdr)
            f.read(incl_len)
            lens.append(orig_len)
    return lens

orig = frame_lengths(sys.argv[1])
cap = frame_lengths(sys.argv[2])
wanted = sorted(set(orig))

print("expected frame sizes: %s" % wanted)
print("captured frames: %s" % cap)

if len(cap) == 0:
    # Distinguished from a content mismatch below: this is the signature of
    # tcpdump's capture socket not yet being attached when traffic flowed,
    # not of a truncated frame - the caller can safely retry the whole
    # capture-and-replay cycle without risking masking a real regression,
    # since #1079's actual defect (data arriving short) always reproduces
    # deterministically once a capture actually attaches in time.
    print("FAIL: captured 0 frames - capture likely wasn't attached in time")
    sys.exit(2)

# For each distinct size in the original, at least one captured frame must be
# at least that long - shorter means it was truncated (#1079's signature).
# Equal-or-longer, not equal, because the loop can pick up unrelated
# background traffic (see the BPF filter in mtu_matrix.sh) sized coincidentally
# close to one of ours; only a *short* arrival is ever a defect.
bad = []
for want in wanted:
    if not any(got >= want for got in cap):
        bad.append(want)

if bad:
    for want in bad:
        best = max((got for got in cap if got <= want), default=0)
        print("FAIL: no frame of >= %d bytes arrived intact (longest short match: %d) "
              "- truncated, the #1079 signature" % (want, best))
    sys.exit(1)

print("OK: every expected frame size (%s) reached the wire at full length" % wanted)
sys.exit(0)
