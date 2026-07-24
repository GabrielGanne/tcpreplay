---
title: Tcpreplay
description: Edit and replay captured network traffic at controlled speeds.
hide:
  - navigation
---

# Tcpreplay

**Tcpreplay is a suite of GPLv3 utilities for editing and replaying network
traffic that was previously captured** by tools like `tcpdump` and Wireshark.
Take a `.pcap` file, optionally rewrite its Layer 2–4 headers, and push it back
onto the wire — at the original timing, a fixed rate, or as fast as the hardware
allows.

It is used by firewall, IDS/IPS, NetFlow and other networking vendors,
enterprises, universities, and open-source projects to test and tune devices
against realistic, repeatable traffic.

<div class="grid cards" markdown>

-   :material-rocket-launch: **New here?**

    ---

    Install the suite and replay your first capture in five minutes.

    [:octicons-arrow-right-24: Quickstart](getting-started/quickstart.md)

-   :material-tools: **The tools**

    ---

    Six focused command-line tools. Learn what each is for and how to drive it.

    [:octicons-arrow-right-24: Tools overview](tools/index.md)

-   :material-book-open-variant: **How-to guides**

    ---

    Task-oriented recipes: line-rate testing, header rewriting, live TCP replay.

    [:octicons-arrow-right-24: Guides](guides/index.md)

-   :material-lightbulb-on: **Concepts**

    ---

    The mental model behind the suite: the replay pipeline, timing, DLT plugins.

    [:octicons-arrow-right-24: Concepts](concepts/index.md)

</div>

## What can you do with it?

<div class="grid cards" markdown>

-   :material-play-speed: **Replay traffic**

    Send a capture back onto the network with `tcpreplay` — preserving the
    original inter-packet timing, or at a target rate in packets/sec or Mbps,
    or flat-out at line rate.

-   :material-file-replace: **Rewrite packets**

    Rewrite MAC/IP addresses, ports, VLAN tags, checksums and more with
    `tcprewrite`, without touching the payload semantics — ideal for retargeting
    a capture at your lab.

-   :material-account-switch: **Split client/server**

    Classify each packet as client- or server-side with `tcpprep`, so a single
    capture can drive a two-NIC in-line test through a device under test.

-   :material-server-network: **Replay to a live server**

    Replay a captured TCP session against a real server with `tcpliveplay`,
    exercising the full stack up to the application — not just Layer 2.

</div>

## The suite at a glance

| Tool | One-liner |
| --- | --- |
| [`tcpreplay`](tools/tcpreplay.md) | Replay pcap files onto the network at arbitrary speeds. |
| [`tcprewrite`](tools/tcprewrite.md) | Rewrite Layer 2–4 headers in a pcap (front end to `libtcpedit`). |
| [`tcpprep`](tools/tcpprep.md) | Pre-process a pcap into a cache file, classifying packets as client/server. |
| [`tcpbridge`](tools/tcpbridge.md) | Bridge two network segments, applying `tcprewrite` logic in flight. |
| [`tcpliveplay`](tools/tcpliveplay.md) | Replay a captured TCP session against a live server (full stack). |
| [`tcpcapinfo`](tools/tcpcapinfo.md) | Inspect and debug the raw structure of a pcap file. |

!!! note "`tcpreplay` vs `tcpreplay-edit`"
    When built with editing support, you also get **`tcpreplay-edit`**, which
    is `tcpreplay` with all of `tcprewrite`'s rewriting options folded in — so
    you can rewrite headers *and* replay in a single pass. Plain `tcpreplay` is
    the lean, edit-free replay engine.

## Why it exists

Real network traffic is messy, bursty, and hard to reproduce on demand.
Tcpreplay lets you capture a slice of it once and replay it deterministically as
often as you like — the same packets, the same timing, at whatever speed your
test calls for. That makes it possible to benchmark throughput, reproduce a bug,
regression-test a firewall rule, or drive a NetFlow appliance to its limits,
all from a saved `.pcap`.

[Get started :material-arrow-right:](getting-started/index.md){ .md-button .md-button--primary }
[Browse the tools :material-arrow-right:](tools/index.md){ .md-button }
