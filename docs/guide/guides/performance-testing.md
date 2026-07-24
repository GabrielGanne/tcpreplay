---
title: Performance & line-rate testing
description: Push measured, repeatable load at a device and account for every packet.
---

# Performance & line-rate testing

Tcpreplay's original claim to fame is generating wire-rate traffic from
commodity hardware to benchmark network devices — throughput, packet rate,
flow-handling, loss. This guide covers doing that *accurately*: pushing a known
load and accounting for what arrives.

## The setup

A performance test has three parts:

```mermaid
flowchart LR
    sender["Sender<br/>tcpreplay"] -->|traffic under test| dut(["Device under test"])
    dut -->|forwarded / processed| receiver["Receiver<br/>counts what arrives"]
    mgmt["Management / SSH"] -.->|separate NIC| sender
```

1. **A sender** running `tcpreplay`, ideally on a dedicated NIC.
2. **The device under test (DUT)** — a firewall, IDS, NetFlow appliance, switch.
3. **A receiver or counter** on the far side that measures what actually made it
   through: a packet capture, an interface counter, or the DUT's own stats.

!!! warning "Keep management traffic off the test NIC"
    Run your SSH/management session on a *different* interface from the one under
    test. Replaying onto the interface you're connected through will disrupt your
    own session and pollute the measurement.

## Step 1 — prepare the capture

Pick a capture that represents the traffic you care about. To generate more load
than a small file contains, you'll loop it — and to keep the send loop from
waiting on disk, preload it into RAM.

```console
$ tcpcapinfo sample.pcap     # sanity-check DLT, snap length, truncation
```

If the capture's addresses don't match your test topology, retarget it once,
offline, with [`tcprewrite`](../tools/tcprewrite.md) — don't pay that cost per
packet at send time.

## Step 2 — establish the ceiling

Find the maximum rate this sender can push, with disk I/O removed:

```console
$ sudo tcpreplay -i eth0 --topspeed --preload-pcap --loop 1000 sample.pcap
```

- `--topspeed` — send flat-out.
- `--preload-pcap` (`-K`) — load the file into RAM first. **Essential** for
  accurate high-rate tests.
- `--loop 1000` — send the file 1000 times to sustain load long enough to
  measure.

Read the `Rated:` line for achieved Mbps/pps. If you can't reach line rate on
the normal path, that's your cue to add a [fast-path backend](fast-paths.md).

## Step 3 — hold a specific load

Once you know the ceiling, test the DUT at a controlled level:

=== "Fixed bandwidth"

    ```console
    $ sudo tcpreplay -i eth0 --mbps 1000 --preload-pcap --loop 0 sample.pcap
    ```

    Hold 1 Gbps indefinitely (`--loop 0`). Stop with ++ctrl+c++.

=== "Fixed packet rate"

    ```console
    $ sudo tcpreplay -i eth0 --pps 1000000 --pps-multi 32 \
        --preload-pcap --loop 0 sample.pcap
    ```

    One million packets/sec. `--pps-multi 32` sends in bursts of 32 so the
    scheduler isn't asked for impossibly short per-packet sleeps — important for
    accuracy at high pps. See
    [Timing & speed control](../concepts/timing-and-speed.md).

=== "For a fixed duration"

    ```console
    $ sudo tcpreplay -i eth0 --mbps 1000 --duration 60 \
        --preload-pcap --loop 0 sample.pcap
    ```

    Run for exactly 60 seconds.

## Step 4 — account for every packet

`tcpreplay`'s report tells you what the **sender** transmitted:

```
Actual: 71305000 packets (46082655000 bytes) sent in 38.03 seconds
Rated: 1201832266.1 Bps, 9614.65 Mbps, 1859629.17 pps
    Successful packets:     71305000
    Failed packets:         0
```

The test result is the comparison between that and what the **receiver** counted.
The gap is your loss. Watch the sender's own counters too:

- **Failed / Retried (ENOBUFS)** — the kernel's send buffer filled. Add
  `--preload-pcap` if you haven't, or move to a fast-path backend.
- **Truncated** — the capture had a short snap length; the packets are smaller
  than the original traffic. Fix the capture, not the test.

## Generating many flows from a small capture

Flow-processing devices (NetFlow/IPFIX appliances, stateful firewalls) care
about the *number of distinct flows*, not just bytes. A small capture has few
flows — but `--unique-ip` makes each loop iteration use different IP addresses,
so N loops of a K-flow capture yields up to N×K distinct-looking flows:

```console
$ sudo tcpreplay -i eth0 --topspeed --preload-pcap \
    --unique-ip --loop 5000 smallFlows.pcap
```

The statistics report includes flow counts and flows-per-second (fps) to help
you tune flow-timeout settings on the DUT.

## Simulating loss

To test how a device or protocol behaves on a lossy link, drop a percentage of
packets at random:

```console
$ sudo tcpreplay -i eth0 --loss 2 sample.pcap    # drop ~2%
```

## Checklist for a clean number

- [x] Management traffic on a separate NIC.
- [x] `--preload-pcap` for any high-rate run.
- [x] Capture retargeted offline, not per-packet.
- [x] Sender `Failed`/`Retried` counters at zero.
- [x] Receiver-side measurement in place — the sender only tells half the story.
- [x] A fast-path backend if you can't reach line rate on the normal path.

## See also

- [Fast-path backends](fast-paths.md) — reaching line rate.
- [Timing & speed control](../concepts/timing-and-speed.md) — the accuracy story.
- [`tcpreplay`](../tools/tcpreplay.md) — every option.
