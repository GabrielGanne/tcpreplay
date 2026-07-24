---
title: tcpreplay-edit
description: Replay pcap files while rewriting their headers in a single pass.
---

# tcpreplay-edit

**Replays pcap files onto the network *and* rewrites their headers in a single
pass.** It is [`tcpreplay`](tcpreplay.md) with the entire
[`tcprewrite`](tcprewrite.md) editing engine folded in — every replay option
*plus* every rewrite option — so you can retarget a capture and send it without
an intermediate file.

```
tcpreplay-edit [OPTION]... <pcap_file(s)> | <pcap_dir(s)>
```

!!! danger "Sends live traffic — needs root"
    Like `tcpreplay`, this injects real packets and requires root. Never point it
    at a production network or the interface you're connected through.

## When to use it

| Use… | …when |
| --- | --- |
| **tcpreplay-edit** | you want to edit packets *on the way out* — rewrite MACs/IPs/ports, change VLANs or DLT, fix checksums — as part of the replay. |
| **[tcpreplay](tcpreplay.md)** | the capture is already correct for your target; it's the leaner, edit-free engine. |
| **[tcprewrite](tcprewrite.md)** | you only want to produce an edited pcap file and not send anything. |

!!! note "Present when built with editing support"
    `tcpreplay-edit` is produced alongside `tcpreplay` whenever the suite is
    built with the tcpedit engine — the case in a normal build.

## Basic use

Rewrite and replay in one command — no separate `tcprewrite` step:

```console
$ sudo tcpreplay-edit -i eth0 \
    --enet-dmac=00:11:22:33:44:55 \
    --pnat=10.0.0.0/8:192.168.0.0/16 \
    --fixcsum sample.pcap
```

That sets the destination MAC, remaps the `10.0.0.0/8` network onto
`192.168.0.0/16`, fixes the checksums, and sends.

## Options

`tcpreplay-edit` accepts the union of two option sets:

- **every [`tcpreplay`](tcpreplay.md) option** — interface, speed control,
  looping, the fast-path backends, two-NIC in-line testing; and
- **every [`tcprewrite`](tcprewrite.md) editing option** — address, port, VLAN
  and DLT rewriting, checksum fixing, and the rest.

Rather than duplicate them here, see those two pages for what each does — and the
[`tcpreplay-edit` man page](../reference/man/tcpreplay-edit.md) for the complete,
combined list.

## See also

- [tcpreplay](tcpreplay.md) — replay without editing.
- [tcprewrite](tcprewrite.md) — edit a capture into a file.
- [Rewriting packet headers](../guides/rewriting-headers.md) — the edits you can
  apply on the way out.
- [`tcpreplay-edit` man page](../reference/man/tcpreplay-edit.md) — every option.
