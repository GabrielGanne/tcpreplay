#!/usr/bin/env python3
#
# Regenerate test_jumbo.pcap (#1097): usage: python3 gen_jumbo_pcap.py test_jumbo.pcap

import struct, sys

def eth_ip_udp_frame(total_len, seed):
    payload_len = total_len - 42
    assert payload_len >= 0
    eth = bytes.fromhex("02000000000102000000000208 00".replace(" ", ""))
    ip_total = 20 + 8 + payload_len
    ip = struct.pack("!BBHHHBBH4s4s",
                      0x45, 0, ip_total, 0, 0, 64, 17, 0,
                      bytes([10,0,0,1]), bytes([10,0,0,2]))
    udp = struct.pack("!HHHH", 1234, 5678, 8 + payload_len, 0)
    payload = bytes([(seed + i) & 0xFF for i in range(payload_len)])
    frame = eth + ip + udp + payload
    assert len(frame) == total_len, (len(frame), total_len)
    return frame

def pcap_bytes(frames):
    out = struct.pack("<IHHiIII", 0xa1b2c3d4, 2, 4, 0, 0, 65535, 1)
    ts = 0
    for f in frames:
        out += struct.pack("<IIII", 1700000000, ts, len(f), len(f))
        out += f
        ts += 1000
    return out

frames = [
    eth_ip_udp_frame(100, 1),      # ordinary small packet: sanity check at a jumbo MTU
    eth_ip_udp_frame(4200, 2),     # just over the old txring 4096-byte truncation point (#1079)
    eth_ip_udp_frame(8900, 3),     # near the 9000-byte MTU ceiling
]

with open(sys.argv[1], "wb") as fh:
    fh.write(pcap_bytes(frames))
print("wrote", sys.argv[1], [len(f) for f in frames])
