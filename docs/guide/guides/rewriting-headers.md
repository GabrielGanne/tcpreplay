---
title: Rewriting packet headers
description: Retarget a capture at your lab with tcprewrite.
---

# Rewriting packet headers

A capture carries the addresses of wherever it was taken. To replay it somewhere
else — your lab, a specific DUT — you rewrite those headers first. This guide
collects the common edits. All are done offline by
[`tcprewrite`](../tools/tcprewrite.md), which never touches the network.

The pattern is always:

```console
$ tcprewrite --infile=in.pcap --outfile=out.pcap [edits...] --fixcsum
```

!!! tip "Always `--fixcsum` after editing"
    Any change to addresses, ports or lengths invalidates the packet checksums.
    `--fixcsum` (`-C`) recalculates them so the DUT doesn't discard your traffic
    as corrupt.

## Point traffic at a specific host

Set the destination MAC (Layer 2 next hop) and, if needed, the source:

```console
$ tcprewrite --infile=in.pcap --outfile=out.pcap \
    --enet-dmac=00:11:22:33:44:55 \
    --enet-smac=aa:bb:cc:dd:ee:ff \
    --fixcsum
```

## Remap a whole network

Map one subnet onto another with a CIDR translation — every address in the
source range is rewritten into the target range, preserving host positions:

```console
$ tcprewrite --infile=in.pcap --outfile=out.pcap \
    --pnat=10.0.0.0/8:192.168.0.0/16 \
    --fixcsum
```

Rewrite only one side with `--srcipmap` or `--dstipmap`.

## Force a two-endpoint conversation

Make every packet look like traffic between exactly two IPs — handy for pointing
a mixed capture at a single client/server pair:

```console
$ tcprewrite -c in.cache --infile=in.pcap --outfile=out.pcap \
    --endpoints=10.1.1.10:10.1.1.20 \
    --fixcsum
```

The [cache file](../tools/tcpprep.md) tells `tcprewrite` which endpoint is the
client and which the server. Build it first:

```console
$ tcpprep --auto=bridge --pcap=in.pcap --cachefile=in.cache
```

## Remap ports

```console
$ tcprewrite --infile=in.pcap --outfile=out.pcap \
    --portmap=80:8080,443:8443 \
    --fixcsum
```

## Add or strip VLAN tags

=== "Add a tag"

    ```console
    $ tcprewrite --infile=in.pcap --outfile=out.pcap \
        --enet-vlan=add --enet-vlan-tag=100 --fixcsum
    ```

=== "Strip tags"

    ```console
    $ tcprewrite --infile=in.pcap --outfile=out.pcap \
        --enet-vlan=del --fixcsum
    ```

## Change the link-layer type

Convert between DLTs using the [DLT plugins](../concepts/dlt-plugins.md):

```console
# Ethernet → raw IP (strip Layer 2)
$ tcprewrite --dlt=raw --infile=eth.pcap --outfile=raw.pcap

# raw/SLL → Ethernet (supply the MACs)
$ tcprewrite --dlt=enet --infile=in.pcap --outfile=eth.pcap \
    --enet-smac=00:11:22:33:44:55 --enet-dmac=66:77:88:99:aa:bb
```

## Fix length mismatches

If a capture's on-wire length disagrees with its captured bytes, reconcile it:

```console
$ tcprewrite --fixlen=pad --infile=in.pcap --outfile=out.pcap    # pad up
$ tcprewrite --fixlen=trunc --infile=in.pcap --outfile=out.pcap  # trim down
```

## Do it all in one replay

If you have `tcpreplay-edit`, you can apply any of these edits *and* send in a
single command, no intermediate file:

```console
$ sudo tcpreplay-edit -i eth0 \
    --enet-dmac=00:11:22:33:44:55 \
    --pnat=10.0.0.0/8:192.168.0.0/16 \
    --fixcsum sample.pcap
```

## See also

- [`tcprewrite`](../tools/tcprewrite.md) — the full option set.
- [Splitting client/server traffic](client-server-split.md) — direction-aware edits.
- [DLT plugins](../concepts/dlt-plugins.md) — link-layer conversion.
