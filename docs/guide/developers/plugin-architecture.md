---
title: Plugin architecture
description: The design rationale behind libtcpedit's DLT plugin system.
---

# Tcpedit plugin architecture
The design rationale behind libtcpedit's DLT plugin system — the "why" behind
the "how" in the [DLT Plugin Developer Guide](../developers/writing-dlt-plugins.md).

## Goals

- A real, thread-safe library other projects can embed (context variables, no
  globals) — see [Using libtcpedit](../developers/libtcpedit.md).
- A plugin architecture so people can add DLT support without touching generic
  code.
- Plugins are bidirectional: able to both decode (read) and encode (write) a link
  layer.
- A consistent API across plugins, with developer templates for creating new ones
  (`dlt_template.sh` — see the
  [DLT Plugin Developer Guide](../developers/writing-dlt-plugins.md)).

**Original plan vs. what actually shipped:** the original design proposed three
plugin groups — DLT/Layer 2, Layer 3, and Layer 4-7. Only the DLT/Layer 2 group
was ever built (`src/tcpedit/plugins/dlt_*`). Layer 3+ rewriting (TTL, TOS/traffic
class, checksums, MTU, IP/port remapping, ...) is instead handled directly by the
core library's `tcpedit_set_*()` API — there's no separate L3/L4-7 plugin
mechanism.

## Why DLT/Layer 2 needs plugins

Converting a pcap between link-layer types (e.g. Linux "any" / `DLT_LINUX_SLL`'s
pseudo-header to a real 802.3 Ethernet header) requires two things done together:
changing the DLT type, and rewriting the actual layer-2 header bytes to match. DLT
type and L2 header format are tightly coupled, and this project can't support every
DLT type that exists — so the design goal is making it *easy to add one*, not
building all of them up front.

Real constraints this creates:
- Different DLTs have different header sizes (some plugins need to make room for a
  larger header than the source had).
- Different DLTs have different (or no) L2 address concepts.
- Some DLTs carry less information than others, or have ambiguous/assumed L3
  typing.
- Supporting conversion between every pair of N DLTs directly would be O(n&sup2;)
  — hence each plugin only needs to convert to/from a common internal
  representation, not to every other DLT directly.

## What each plugin provides

- A **decoder**: parses an existing packet's L2 header into `tcpeditdlt_t`'s
  internal representation.
- An **encoder**: writes a new L2 header from that internal representation.
- Declared input fields (what it can extract when decoding — e.g. src/dst MAC,
  HDLC address, protocol type).
- Declared output fields (what it needs when encoding).
- AutoOpts option definitions for anything the user needs to be able to override
  (e.g. VLAN tag info) or must supply when a field is missing from the source.

Not every plugin needs both directions — e.g. a synthetic/fake-header DLT might
only ever make sense to decode, never to select as an output encoder.

## Runtime flow

1. Every plugin registers itself (`tcpedit_dlt_register()` in `dlt_plugins.c`).
2. The caller selects an output DLT (directly, or via `--dlt`/AutoOpts) plus any
   field overrides.
3. The source pcap's DLT is read from the input file.
4. tcpedit sanity-checks that (source DLT + user overrides) actually supply
   everything the chosen destination DLT's encoder requires.
5. Per packet: the source DLT's decoder fills in `tcpeditdlt_t`; that's handed to
   the destination DLT's encoder, which produces the new L2 header; the packet is
   reassembled and L3+ processing (via the core `tcpedit_set_*()` config, not a
   plugin) runs on top.

## Current plugins

| Plugin | Role |
|---|---|
| `dlt_en10mb` | Ethernet — VLAN add/delete/edit, src/dst MAC edit |
| `dlt_hdlc` | Cisco HDLC — destination address, control value |
| `dlt_user` | User-defined DLT type override |
| `dlt_raw` | Raw IP (no L2 header) |
| `dlt_null` / `dlt_loop` | BSD loopback / loopback |
| `dlt_linuxsll` / `dlt_linuxsll2` | Linux "any" (SLL/SLL2) — decode-oriented |
| `dlt_ieee80211` / `dlt_radiotap` | 802.11, optionally with Radiotap headers |
| `dlt_jnpr_ether` | Juniper Ethernet |
| `dlt_pppserial` | PPP over a serial link |

See `src/tcpedit/plugins/dlt_plugins.c` in the source tree for the authoritative,
current registration list.

---
*Adapted from the [Tcpreplay developer wiki](https://github.com/appneta/tcpreplay/wiki/Tcpedit-Plugin-Architecture).*
