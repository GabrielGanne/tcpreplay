---
title: tcprewrite
description: Rewrite Layer 2–4 packet headers in a pcap file.
---

# tcprewrite

**Rewrites the packets in a pcap file** and writes a new pcap. It edits headers
— MAC and IP addresses, ports, VLAN tags, checksums, even the link-layer type —
without putting anything on the wire. It is the command-line front end to the
`libtcpedit` engine, which `tcpbridge` and `tcpreplay-edit` share.

```
tcprewrite [OPTION]... --infile=<in.pcap> --outfile=<out.pcap>
```

`tcprewrite` never touches a network interface, so it needs no special
privileges.

## The essentials

```console
$ tcprewrite --infile=in.pcap --outfile=out.pcap [edits...] --fixcsum
```

- `-i`, `--infile` — the capture to read.
- `-o`, `--outfile` — the pcap to write.
- `-C`, `--fixcsum` — recalculate IP/TCP/UDP checksums after editing (do this
  whenever you change addresses, ports or lengths).

## Rewriting addresses

=== "MAC addresses (Layer 2)"

    | Option | Effect |
    | --- | --- |
    | `--enet-smac=MAC` | Set the source MAC. |
    | `--enet-dmac=MAC` | Set the destination MAC. |

    Provide two comma-separated MACs to rewrite client and server directions
    differently (requires a [cache file](tcpprep.md)).

=== "IP addresses (Layer 3)"

    | Option | Effect |
    | --- | --- |
    | `-N`, `--pnat=MAP` | Rewrite IPs by CIDR map, e.g. `10.0.0.0/8:192.168.0.0/16`. |
    | `-S`, `--srcipmap=MAP` | Rewrite only source IPs. |
    | `-D`, `--dstipmap=MAP` | Rewrite only destination IPs. |
    | `-e`, `--endpoints=IP1:IP2` | Force every packet to look like a conversation between two endpoints. |
    | `-s`, `--seed=N` | Pseudo-randomize IPs with a repeatable seed. |

=== "Ports (Layer 4)"

    | Option | Effect |
    | --- | --- |
    | `-r`, `--portmap=MAP` | Remap ports, e.g. `80:8080,443:8443`. |

## VLAN, MTU and link layer

| Option | Effect |
| --- | --- |
| `--enet-vlan=add\|del` | Add or strip 802.1Q VLAN tags. |
| `--enet-vlan-tag=N` | The VLAN ID to add. |
| `--mtu=N` | Set the assumed MTU. |
| `--mtu-trunc` | Truncate packets larger than the MTU. |
| `--fixlen=pad\|trunc\|del` | Reconcile a mismatched on-wire vs captured length. |
| `--dlt=DLT` | Convert the link-layer type (see below). |

## Changing the link-layer type

`tcprewrite` can translate between data-link types using the
[DLT plugins](../concepts/dlt-plugins.md) — for example, strip Ethernet framing
to produce a raw IP capture, or convert a Linux "cooked" capture to Ethernet:

```console
$ tcprewrite --dlt=raw   --infile=eth.pcap --outfile=raw.pcap
$ tcprewrite --dlt=enet  --infile=sll.pcap --outfile=eth.pcap \
    --enet-smac=00:11:22:33:44:55 --enet-dmac=66:77:88:99:aa:bb
```

Converting *to* Ethernet requires you to supply the MACs it can't infer.

## Direction-aware rewriting

Many options accept two values — one for client-to-server packets, one for
server-to-client — so a single pass can retarget both halves of a conversation
differently. That requires a [`tcpprep`](tcpprep.md) cache file:

```console
$ tcpprep --auto=bridge --pcap=in.pcap --cachefile=in.cache
$ tcprewrite -c in.cache \
    --endpoints=10.1.1.1:10.1.1.2 \
    --infile=in.pcap --outfile=out.pcap --fixcsum
```

## A worked example

Retarget a capture at a lab device — new destination MAC, remapped network,
remapped web port, checksums fixed:

```console
$ tcprewrite \
    --infile=capture.pcap \
    --outfile=lab.pcap \
    --enet-dmac=00:11:22:33:44:55 \
    --pnat=10.0.0.0/8:192.168.50.0/24 \
    --portmap=80:8080 \
    --fixcsum
```

See [Rewriting packet headers](../guides/rewriting-headers.md) for more recipes.

## See also

- [Rewriting packet headers](../guides/rewriting-headers.md) — task recipes.
- [DLT plugins](../concepts/dlt-plugins.md) — how link-layer conversion works.
- [`tcprewrite` man page](../reference/man-pages.md) — every option.
