---
title: DLT plugins
description: How the suite translates between link-layer types.
---

# DLT plugins

Every pcap declares a **data-link type (DLT)** — the framing of its packets:
Ethernet, raw IP, Linux "cooked" (SLL), 802.11, Cisco HDLC, and more. A capture
taken on one kind of link can't simply be replayed onto a different kind; the
framing has to be translated first. That translation is what the **DLT plugins**
do.

## One plugin per link type

Inside `libtcpedit`, each supported link type is a self-contained plugin that
knows how to decode that framing to a common internal representation and encode
back out to it. Converting a capture from DLT *A* to DLT *B* means: decode with
plugin *A*, then encode with plugin *B*.

Plugins that ship with the suite include:

| Plugin | Link type |
| --- | --- |
| `en10mb` | Ethernet (the common case) |
| `raw` | Raw IPv4/IPv6, no Layer 2 |
| `linuxsll` / `linuxsll2` | Linux "cooked" capture (e.g. from `any`) |
| `ieee80211` / `radiotap` | 802.11 wireless |
| `hdlc` | Cisco HDLC |
| `null` / `loop` | BSD/loopback null framing |
| `pppserial` | PPP over serial |
| `jnpr_ether` | Juniper Ethernet |
| `user` | User-supplied Layer 2 header |

## Using them

You don't invoke plugins directly — you ask [`tcprewrite`](../tools/tcprewrite.md)
(or `tcpbridge`, or `tcpreplay-edit`) for a target DLT and it selects the right
pair:

```console
# strip Ethernet framing → raw IP
$ tcprewrite --dlt=raw --infile=eth.pcap --outfile=raw.pcap

# raw IP or SLL → Ethernet (you supply the MACs it can't know)
$ tcprewrite --dlt=enet --infile=sll.pcap --outfile=eth.pcap \
    --enet-smac=00:11:22:33:44:55 --enet-dmac=66:77:88:99:aa:bb
```

Some conversions need information the source framing doesn't carry — most
commonly MAC addresses when encoding *to* Ethernet. The plugin surfaces those as
required options.

## Why a plugin architecture?

Adding support for a new link layer means writing one new plugin against a
shared interface — not threading special cases through the generic code. The
suite ships a `dlt_template` (and `dlt_template.sh`) to scaffold one. This keeps
each link type's quirks isolated, which matters because framing details are
exactly the kind of thing that's fiddly and easy to get subtly wrong.

## Relationship to `libtcpedit`

The plugins live in `libtcpedit`, the rewriting engine shared by `tcprewrite`,
`tcpbridge` and `tcpreplay-edit`. That's why all three can convert link types
with the same `--dlt` option: they're all driving the same plugin set.

## See also

- [`tcprewrite` link-layer conversion](../tools/tcprewrite.md#changing-the-link-layer-type).
- [`tcpcapinfo`](../tools/tcpcapinfo.md) — shows a capture's current DLT.
