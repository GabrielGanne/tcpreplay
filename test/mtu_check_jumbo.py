#!/usr/bin/env python3
#
# Compare frame lengths between an original pcap and one captured off the
# wire, to catch a TX_RING silently truncating a frame (#1079). Used by
# mtu_matrix.sh; not a standalone test.
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

print("original frames: %s" % orig)
print("captured frames: %s" % cap)

if len(cap) < len(orig):
    print("FAIL: captured %d frames, expected at least %d" % (len(cap), len(orig)))
    sys.exit(1)

# match by position: the dummy interface carries nothing else, so replay order
# should be preserved
bad = 0
for i, want in enumerate(orig):
    got = cap[i]
    if got < want:
        print("FAIL: frame %d: original was %d bytes, only %d reached the wire "
              "(truncated - the #1079 signature)" % (i, want, got))
        bad += 1

if bad:
    sys.exit(1)

print("OK: all %d frames reached the wire at full length" % len(orig))
sys.exit(0)
