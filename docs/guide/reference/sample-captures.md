---
title: Sample captures
description: Where to get pcap files to practise with.
---

# Sample captures

You need a `.pcap` to do anything with the suite. Here's where to get one.

## Make your own

The most representative capture is one of your own traffic:

```console
$ sudo tcpdump -i eth0 -w mycapture.pcap -c 1000
```

Capture with a **full snap length** if you plan to benchmark, so packets aren't
truncated:

```console
$ sudo tcpdump -i eth0 -s 0 -w mycapture.pcap
```

Check what you got:

```console
$ tcpcapinfo mycapture.pcap
```

## Captures shipped with the source

The source tree includes small captures under `test/` used by the test suite —
handy for quick experiments:

| File | Notes |
| --- | --- |
| `test/test.pcap` | General-purpose mixed capture. |
| `test/test_nano.pcap` | Nanosecond-timestamp capture. |

These are intentionally tiny (test fixtures), so loop them (`--loop`) if you need
sustained traffic.

## Well-known public sample sets

The Tcpreplay project has long referenced a couple of sample captures for
performance demos:

| File | Rough size | Good for |
| --- | --- | --- |
| `smallFlows.pcap` | small | Quick functional tests; many flows with `--unique-ip`. |
| `bigFlows.pcap` | larger | Sustained load and flow-heavy performance tests. |

Check the project [website](https://tcpreplay.appneta.com/) for current download
links.

## Other sources

- **Wireshark sample captures** — a large community collection of protocol
  examples.
- **Your capture tool of choice** — anything that writes standard pcap (or
  pcap-ng, with a recent libpcap) works.

!!! tip "Retarget before replaying"
    A downloaded capture carries someone else's addresses. Before sending it at a
    specific device, retarget it with
    [`tcprewrite`](../guides/rewriting-headers.md) so the MACs and IPs match your
    lab.
