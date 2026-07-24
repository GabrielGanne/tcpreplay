---
title: Writing a DLT plugin
description: Add support for a new link-layer type to libtcpedit.
---

# Writing a DLT plugin
This is the developer guide for writing a new DLT (link-layer type) plugin for
**libtcpedit**, the packet-rewriting engine shared by `tcprewrite`, `tcpbridge`,
and `tcpreplay-edit`.

## Overview

Each supported link layer (Ethernet, Linux SLL, Cisco HDLC, raw IP, radiotap, ...)
is implemented as a **DLT plugin** under `src/tcpedit/plugins/`, translating that
link layer to/from Ethernet-equivalent processing. Existing plugins: `dlt_en10mb`
(Ethernet), `dlt_hdlc`, `dlt_ieee80211`, `dlt_jnpr_ether`, `dlt_linuxsll`,
`dlt_linuxsll2`, `dlt_loop`, `dlt_null`, `dlt_pppserial`, `dlt_radiotap`,
`dlt_raw`, `dlt_user`. Adding a new DLT means adding a plugin here — don't
branch on link-layer type in generic tcpedit code.

## 1. Generate the plugin template

From `src/tcpedit/plugins/`, run:

```
./dlt_template.sh <name>
```

`<name>` is your DLT's short name (e.g. `en10mb` for `DLT_EN10MB`/Ethernet). This
creates `dlt_<name>/` populated from `dlt_template/`, generating:

| File | Purpose |
|---|---|
| `Makefile.am` | automake sources list for this plugin |
| `<name>.c` / `<name>.h` | the plugin implementation and its public prototypes |
| `<name>_api.c` / `<name>_api.h` | optional config API, for encoders with user-settable options |
| `<name>_types.h` | plugin-private structs (decoded "extra" data, encoder config) |
| `<name>_opts.def` | AutoOpts option definitions for this plugin's `--<prefix>-*` flags |

The script prints the remaining steps (also listed below) when it finishes.

## 2. Wire it into the build

1. Add a line to the **end** of `src/tcpedit/plugins/Makefile.am`:
   ```
   include $(srcdir)/dlt_<name>/Makefile.am
   ```
2. Add a line to `src/tcpedit/plugins/dlt_stub.def`:
   ```
   #include dlt_<name>/<name>_opts.def
   ```
3. Register the plugin in `src/tcpedit/plugins/dlt_plugins.c`:
   ```c
   #include "dlt_<name>/<name>.h"
   ```
   and inside `tcpedit_dlt_register()`:
   ```c
   retcode += dlt_<name>_register(ctx);
   ```
4. If your plugin supports being selected as an *output* DLT
   (`tcprewrite --dlt`), add it to the table in
   `src/tcpedit/plugins/dlt_opts.def`. If it supports being an *input* pcap's DLT,
   it just needs to decode — no extra wiring beyond registration.

Rebuild from a clean tree so the generated option files pick up the new plugin
(`./autogen.sh && ./configure && make`, or `cmake -B build && cmake --build
build`). If a stale generated file causes weird build errors after adding a
plugin, deleting `src/tcpedit/tcpedit_stub.h` and reconfiguring is the traditional
fix.

## 3. Implement the plugin

In `<name>.c`, near the top:

```c
static char dlt_name[] = "<name>";      /* internal name -- leave as-is */
static char dlt_prefix[] = "???";       /* your CLI option prefix, e.g. "enet" */
static uint16_t dlt_value = 0xFFFF;     /* the DLT_* value from libpcap's bpf.h */
```

`dlt_name`, `dlt_prefix`, and `dlt_value` **must all be unique** across every
registered plugin — collisions will misroute packets between plugins at
runtime.

`dlt_prefix` becomes the prefix for every CLI flag your plugin exposes: if it's
`enet`, your options are `--enet-src`, `--enet-dst`, `--enet-vlan-mode`, etc.
Prefer descriptive multi-character prefixes over single-character short options.

The template pre-declares the full function set (see `<name>.h`); fill in the
ones marked `FIXME:`:

- `dlt_<name>_register` / `_init` / `_post_init` / `_cleanup`
- `dlt_<name>_parse_opts` — read your `--<prefix>-*` options
- `dlt_<name>_decode` / `_encode` — the actual header translation
- `dlt_<name>_proto`, `_get_layer3`, `_merge_layer3`, `_l2addr_type`, `_l2len`, `_get_mac`

Notes from experience:

- Not every plugin needs both `encode()` and `decode()`. A DLT that only makes
  sense as an *input* (e.g. one using a synthetic/fake L2 header) can omit
  `encode()` — but then it can never be selected as an *output* DLT either;
  some other plugin's encoder has to be chosen instead.
- Don't take shortcuts on variable-length headers — e.g. 802.1Q VLAN-tagged
  Ethernet frames are 18 bytes, not the usual 14.
- Options exist to let users control *encoder* behavior. Don't expose options for
  decoder-only concerns.
- Put real validation/parsing logic in `<name>_api.c`/`.h`, called from
  `dlt_<name>_parse_opts()` — keep the option-parsing function itself thin.
- `$(srcdir)` in these `Makefile.am` fragments refers to `src/tcpedit`, not the
  plugin's own directory.

## Reference

The cleanest real examples to read alongside this guide: `dlt_null` (simplest —
minimal decode/encode) and `dlt_en10mb` (most complete — VLAN handling, MAC
rewriting, the `enet_*` config API). Both live under `src/tcpedit/plugins/`.

See also: [Tcpedit Plugin Architecture](../developers/plugin-architecture.md) (the
design rationale behind this plugin system) and
[Using libtcpedit](../developers/libtcpedit.md) (using the library as a caller, rather
than extending it).

---
*Adapted from the [Tcpreplay developer wiki](https://github.com/appneta/tcpreplay/wiki/DLT-Plugin-Developer-Guide),
originally written by Aaron Turner in 2009.*
