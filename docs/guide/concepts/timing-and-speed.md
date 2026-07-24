---
title: Timing & speed control
description: How the scheduler reproduces or overrides a capture's timing.
---

# Timing & speed control

Every packet in a pcap has a timestamp. How `tcpreplay` uses those timestamps is
what the speed options control — and it's the single most important thing to
understand for accurate testing.

## The default: reproduce the recorded timing

With no speed option, `tcpreplay` treats the capture as a script to perform in
real time. It looks at the gap between each packet's timestamp and the previous
one, and waits that long before sending. A capture recorded over ten seconds
replays over ten seconds, with the same bursts and idle gaps.

This is the right choice when the *shape* of the traffic matters — reproducing a
bug that depends on timing, or testing behaviour under a realistic arrival
pattern.

## Overriding the timing

Often you don't want the original pace — you want a specific load. The speed
options replace the timestamp-derived delay with something you choose:

| Model | What it does | Use it to… |
| --- | --- | --- |
| `--topspeed` (`-t`) | Send with no delay at all | Find the maximum rate the host/NIC can push. |
| `--mbps=N` | Pace to N megabits/sec | Hold a device at a known bandwidth. |
| `--pps=N` | Pace to N packets/sec | Test packet-rate limits independent of size. |
| `--multiplier=N` (`-x`) | Scale the recorded gaps by 1/N | Speed up or slow down realistic traffic. |

Only one applies at a time — they're different answers to "how long do I wait
between packets?"

## Where accuracy comes from

Pacing to a rate means sleeping for very short, very precise intervals between
packets. At low rates this is easy. At high rates the per-packet delay shrinks
toward the resolution of the system's timers and the cost of the send call
itself, and two things start to limit accuracy:

- **Timer/scheduler granularity.** Sleeping for a few microseconds is not
  something a general-purpose OS does exactly. `tcpreplay` uses the most precise
  mechanism available and, for very high rates, can send in small **bursts**
  (`--pps-multi`) so it schedules fewer, larger waits rather than many
  impossibly short ones.
- **Per-packet overhead.** Reading from disk, system-call cost, and copying into
  kernel buffers all eat into the time budget. Removing that overhead is what the
  performance options are for.

## Getting out of your own way at high rates

For line-rate work, two changes matter more than any timing flag:

- **`--preload-pcap` (`-K`)** loads the whole capture into RAM up front, so the
  send loop never waits on disk.
- **A fast-path backend** (`--netmap`, `--xdp`, `--io-uring`) bypasses much of
  the normal kernel send path, dramatically cutting per-packet overhead.

With those in place, `--topspeed` on commodity hardware can approach the NIC's
line rate. See [Fast-path backends](../guides/fast-paths.md) and
[Performance & line-rate testing](../guides/performance-testing.md).

## Nanosecond captures

Modern captures may carry nanosecond-resolution timestamps. Tcpreplay preserves
that resolution end to end, so sub-microsecond inter-packet timing in a capture
is honoured rather than rounded — relevant when you replay high-rate traffic and
want the fine-grained arrival pattern intact.

## A mental checklist

- Reproducing behaviour that depends on *when* packets arrive? → default timing.
- Testing a device at a *known load*? → `--mbps` or `--pps`.
- Finding a *ceiling*? → `--topspeed` + `--preload-pcap` + a fast path.
- Realistic traffic, just faster/slower? → `--multiplier`.

## See also

- [`tcpreplay` speed options](../tools/tcpreplay.md#controlling-speed).
- [Performance & line-rate testing](../guides/performance-testing.md).
