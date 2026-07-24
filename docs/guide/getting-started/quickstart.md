---
title: Quickstart
description: Replay, rate-limit and rewrite your first capture in five minutes.
---

# Quickstart

This walks through the whole loop: **inspect** a capture, **replay** it, control
its **speed**, then **rewrite** it for a different target. Budget about five
minutes.

!!! danger "Replay puts real packets on a real network"
    `tcpreplay` injects live traffic onto the interface you give it. It needs
    root (raw sockets), and the packets it sends are the *actual* packets from
    the capture — with whatever MACs, IPs and ports they contain.

    **Do not replay onto a production network**, and never onto the interface
    you are connected through (for example, your SSH link). Use an isolated lab
    segment, a spare NIC, or a virtual network. To experiment with zero risk of
    disturbing anything, replay onto a dummy interface (see
    [below](#practising-safely-on-a-dummy-interface)).

Throughout, replace `eth0` with your test interface.

## 1. Get a capture

You can use any pcap file. To make your own from live traffic:

```console
$ sudo tcpdump -i eth0 -w mycapture.pcap -c 1000   # grab 1000 packets
```

Or grab one of the [sample captures](../reference/sample-captures.md). The rest
of this page assumes a file named `sample.pcap`.

## 2. Inspect it

Before sending anything, see what's in the file:

```console
$ tcpcapinfo sample.pcap
```

`tcpcapinfo` decodes the pcap's own structure — link-layer type, snap length,
byte order, per-packet lengths — which is exactly what you need to know before
replaying or rewriting. See [tcpcapinfo](../tools/tcpcapinfo.md).

## 3. Replay at the original speed

The default behaviour is to reproduce the capture's own inter-packet timing:

```console
$ sudo tcpreplay -i eth0 sample.pcap
```

When it finishes, you get a report:

```
Actual: 1000 packets (486400 bytes) sent in 9.89 seconds
Rated: 49180.0 Bps, 0.393 Mbps, 101.10 pps
Statistics for network device: eth0
    Successful packets:     1000
    Failed packets:         0
    Truncated packets:      0
    Retried packets (ENOBUFS): 0
```

## 4. Control the speed

The same packets, sent at whatever pace your test calls for:

=== "As fast as possible"

    ```console
    $ sudo tcpreplay -i eth0 --topspeed sample.pcap
    ```

    `--topspeed` (or `-t`) ignores the capture's timing and sends flat-out.

=== "A fixed bit rate"

    ```console
    $ sudo tcpreplay -i eth0 --mbps 100 sample.pcap
    ```

    Send at 100 Mbps regardless of the original timing.

=== "A fixed packet rate"

    ```console
    $ sudo tcpreplay -i eth0 --pps 5000 sample.pcap
    ```

    Send 5000 packets per second.

=== "Faster / slower than recorded"

    ```console
    $ sudo tcpreplay -i eth0 --multiplier 10 sample.pcap
    ```

    Replay at 10× the recorded speed (use a fraction like `0.5` to slow down).

Add `--loop 100` to send the file 100 times, or `--loop 0` to loop forever.
For the full timing model, see
[Timing & speed control](../concepts/timing-and-speed.md).

!!! tip "Warm the cache for high-rate tests"
    Add `--preload-pcap` (`-K`) to load the entire file into memory before
    sending. This removes disk I/O from the send loop and is essential for
    accurate line-rate benchmarks. See
    [Performance & line-rate testing](../guides/performance-testing.md).

## 5. Rewrite it for a different target

Say you want to replay this capture at a device that expects different
addresses. `tcprewrite` edits the file without sending anything:

```console
$ tcprewrite --infile=sample.pcap --outfile=retargeted.pcap \
    --enet-dmac=00:11:22:33:44:55 \
    --pnat=10.0.0.0/8:192.168.0.0/16 \
    --fixcsum
```

That rewrites the destination MAC, maps the `10.0.0.0/8` network onto
`192.168.0.0/16`, and recalculates checksums. Now replay the rewritten file:

```console
$ sudo tcpreplay -i eth0 retargeted.pcap
```

!!! note "Rewrite and replay in one pass"
    If your build includes `tcpreplay-edit`, you can skip the intermediate file
    and pass `tcprewrite`'s options straight to the replay command:

    ```console
    $ sudo tcpreplay-edit -i eth0 \
        --enet-dmac=00:11:22:33:44:55 \
        --pnat=10.0.0.0/8:192.168.0.0/16 sample.pcap
    ```

See [Rewriting packet headers](../guides/rewriting-headers.md) for the full set
of edits.

## Practising safely on a dummy interface

To run every command above with no chance of touching a real network, create a
Linux dummy interface and replay onto that:

```console
$ sudo ip link add dummy0 type dummy
$ sudo ip link set dummy0 up
$ sudo tcpreplay -i dummy0 --topspeed sample.pcap   # goes nowhere, but exercises the full path
```

Remove it when you're done:

```console
$ sudo ip link delete dummy0
```

## Where to next

<div class="grid cards" markdown>

-   :material-tools: **[The tools](../tools/index.md)** — what each command is for.
-   :material-book-open-variant: **[How-to guides](../guides/index.md)** — real testing recipes.
-   :material-lightbulb-on: **[Concepts](../concepts/index.md)** — how it all fits together.

</div>
