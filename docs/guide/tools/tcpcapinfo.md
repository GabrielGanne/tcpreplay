---
title: tcpcapinfo
description: Inspect and debug the raw structure of a pcap file.
---

# tcpcapinfo

**A raw pcap decoder and debugger.** It reports the low-level structure of a
capture file — the parts that determine whether the other tools can read and
replay it correctly. Reach for it first whenever a capture behaves unexpectedly.

```
tcpcapinfo [OPTION]... <pcap_file(s)>
```

`tcpcapinfo` only reads; it needs no privileges.

## What it tells you

For each file (and each packet, if you ask), `tcpcapinfo` reports things like:

- the **magic number** and **byte order** (was this written big- or
  little-endian, and at microsecond or nanosecond resolution?);
- the **pcap version**;
- the **link-layer type (DLT)** — Ethernet, raw IP, Linux SLL, 802.11, …;
- the **snap length** the capture was taken with;
- per-packet **captured vs original length**, flagging truncation.

## Basic use

```console
$ tcpcapinfo sample.pcap
```

Point it at several files at once to compare them:

```console
$ tcpcapinfo *.pcap
```

## Why it matters before replaying

Two of the most common replay surprises are visible here first:

- **Wrong link-layer type.** If a capture's DLT isn't what your interface
  expects, you'll want [`tcprewrite --dlt`](tcprewrite.md#changing-the-link-layer-type)
  to convert it. `tcpcapinfo` shows you the DLT.
- **Truncated packets.** If the capture was taken with a small snap length, the
  packets on disk are shorter than they were on the wire. `tcpcapinfo` shows the
  captured-vs-original lengths so you know before you send.

## See also

- [`tcprewrite`](tcprewrite.md) — fix DLT and length issues `tcpcapinfo` reveals.
- [Sample captures](../reference/sample-captures.md).
- [`tcpcapinfo` man page](../reference/man-pages.md) — every option.
