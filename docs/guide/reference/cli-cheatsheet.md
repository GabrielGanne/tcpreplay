---
title: CLI cheat sheet
description: The most-used Tcpreplay commands on one page.
---

# CLI cheat sheet

A one-page reminder. See the [man pages](man-pages.md) for the full option list.

!!! danger "`tcpreplay`, `tcpbridge` and `tcpliveplay` send live traffic and need root."

## Replay

```console
# Original timing
$ sudo tcpreplay -i eth0 file.pcap

# As fast as possible, preloaded (line-rate test)
$ sudo tcpreplay -i eth0 --topspeed --preload-pcap --loop 1000 file.pcap

# Fixed bandwidth / packet rate
$ sudo tcpreplay -i eth0 --mbps 1000 file.pcap
$ sudo tcpreplay -i eth0 --pps 1000000 --pps-multi 32 file.pcap

# Faster/slower than recorded
$ sudo tcpreplay -i eth0 --multiplier 10 file.pcap

# Loop forever; unique IPs per loop; simulate 2% loss
$ sudo tcpreplay -i eth0 --loop 0 --unique-ip --loss 2 file.pcap

# Fast paths (must be compiled in)
$ sudo tcpreplay -i eth0 --io-uring --topspeed --preload-pcap file.pcap
$ sudo tcpreplay -i eth0 --xdp     --topspeed --preload-pcap file.pcap
$ sudo tcpreplay -i eth0 --netmap  --topspeed --preload-pcap file.pcap

# Dry run: write what would be sent to a file
$ tcpreplay -i eth0 -w out.pcap file.pcap
```

## Two-NIC in-line test

```console
$ tcpprep --auto=bridge --pcap=file.pcap --cachefile=file.cache
$ sudo tcpreplay -i eth0 -I eth1 -c file.cache file.pcap
```

## Rewrite

```console
# MAC / IP / port
$ tcprewrite -i in.pcap -o out.pcap --enet-dmac=00:11:22:33:44:55 --fixcsum
$ tcprewrite -i in.pcap -o out.pcap --pnat=10.0.0.0/8:192.168.0.0/16 --fixcsum
$ tcprewrite -i in.pcap -o out.pcap --portmap=80:8080 --fixcsum

# VLAN add / strip
$ tcprewrite -i in.pcap -o out.pcap --enet-vlan=add --enet-vlan-tag=100 --fixcsum
$ tcprewrite -i in.pcap -o out.pcap --enet-vlan=del --fixcsum

# Change link-layer type
$ tcprewrite -i eth.pcap -o raw.pcap --dlt=raw
$ tcprewrite -i in.pcap  -o eth.pcap --dlt=enet \
    --enet-smac=00:11:22:33:44:55 --enet-dmac=66:77:88:99:aa:bb

# Edit + replay in one pass (needs tcpreplay-edit)
$ sudo tcpreplay-edit -i eth0 --pnat=10.0.0.0/8:192.168.0.0/16 --fixcsum file.pcap
```

## Classify (tcpprep)

```console
$ tcpprep --auto=bridge  --pcap=in.pcap --cachefile=in.cache   # by MAC
$ tcpprep --auto=router  --pcap=in.pcap --cachefile=in.cache   # by subnet
$ tcpprep --cidr=10.0.0.0/8 --pcap=in.pcap --cachefile=in.cache
$ tcpprep --port         --pcap=in.pcap --cachefile=in.cache   # by service port
$ tcpprep --print-info   --cachefile=in.cache --pcap=in.pcap   # inspect
```

## Inspect / bridge / live

```console
# Decode a capture's structure
$ tcpcapinfo file.pcap

# Bridge two segments, rewriting in flight
$ sudo tcpbridge --intf1=eth0 --intf2=eth1 --pnat=10.0.0.0/8:192.168.0.0/16 --fixcsum

# Replay a TCP session to a live server
$ sudo tcpliveplay eth0 session.pcap 10.0.0.20 00:11:22:33:44:55 80
```

## Practise safely (Linux dummy interface)

```console
$ sudo ip link add dummy0 type dummy && sudo ip link set dummy0 up
$ sudo tcpreplay -i dummy0 --topspeed file.pcap
$ sudo ip link delete dummy0
```
