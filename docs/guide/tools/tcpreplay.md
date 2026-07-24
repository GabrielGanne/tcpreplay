---
title: tcpreplay
description: Replay pcap files onto the network at arbitrary speeds.
---

# tcpreplay

**Replays pcap files onto the network.** It reads one or more captures and sends
their packets out a network interface — at the original timing, a controlled
rate, or as fast as the hardware allows.

```
tcpreplay [OPTION]... <pcap_file(s)> | <pcap_dir(s)>
```

!!! danger "Sends live traffic — needs root"
    `tcpreplay` injects real packets and requires root. Never point it at a
    production network or the interface you're connected through. See the
    [Quickstart safety note](../getting-started/quickstart.md).

## Basic use

```console
$ sudo tcpreplay -i eth0 sample.pcap
```

`-i` / `--intf1` names the output interface. Give multiple files or a directory
to send them in sequence.

## Controlling speed

This is the heart of the tool. Pick exactly one speed model:

| Option | Behaviour |
| --- | --- |
| *(default)* | Reproduce the capture's original inter-packet timing. |
| `-t`, `--topspeed` | Send as fast as possible, ignoring timestamps. |
| `--mbps=N` | Send at a fixed bit rate of N Mbps. |
| `--pps=N` | Send at a fixed rate of N packets/sec. |
| `-x`, `--multiplier=N` | Multiply the recorded speed (e.g. `10` = 10×, `0.5` = half). |
| `--pps-multi=N` | Send packets in bursts of N (smooths high `--pps` rates). |

See [Timing & speed control](../concepts/timing-and-speed.md) for how these
interact with the scheduler and where accuracy comes from.

## Looping and repetition

| Option | Effect |
| --- | --- |
| `-l`, `--loop=N` | Send the file(s) N times (`0` = loop forever). |
| `--loopdelay-ms=N` | Wait N milliseconds between loops. |
| `--unique-ip` | On each loop, tweak IPs so every pass looks like distinct hosts — useful for generating many flows from one small capture. |

## Performance options

| Option | Effect |
| --- | --- |
| `-K`, `--preload-pcap` | Load the whole file into RAM before sending (removes disk I/O from the hot loop — use for line-rate tests). |
| `--netmap` | Use the netmap fast path. |
| `--xdp` | Use the AF_XDP fast path (Linux). |
| `--io-uring` | Use the io_uring fast path (Linux). |
| `--raw` | Inject via a PF_INET raw IP socket instead of PF_PACKET (Linux, IPv4). |

The fast-path backends are covered in [Fast-path backends](../guides/fast-paths.md).

## Two-NIC (in-line) testing

To drive a device that sits *in* the path (a firewall, IPS, proxy), send
client traffic out one NIC and server traffic out another:

```console
$ sudo tcpreplay -i eth0 -I eth1 -c sample.cache sample.pcap
```

- `-I` / `--intf2` is the second interface.
- `-c` / `--cachefile` is a [`tcpprep`](tcpprep.md) cache that says which packets
  are client-side and which are server-side.

See [Splitting client/server traffic](../guides/client-server-split.md).

## Other useful options

| Option | Effect |
| --- | --- |
| `--loss=N` | Randomly drop N% of packets to simulate a lossy link. |
| `-w`, `--write=FILE` | Write what *would* be sent to a file instead of a NIC (dry run / capture). |
| `--stats=N` | Print running statistics every N seconds. |
| `--duration=N` | Stop after N seconds regardless of file length. |
| `-v`, `--verbose` | Decode each packet as it's sent (via `tcpdump`). |

## Reading the output

```
Actual: 1000 packets (486400 bytes) sent in 9.89 seconds
Rated: 49180.0 Bps, 0.393 Mbps, 101.10 pps
Statistics for network device: eth0
    Successful packets:     1000
    Failed packets:         0
    Truncated packets:      0
    Retried packets (ENOBUFS): 0
```

- **Actual** — what was sent and how long it took.
- **Rated** — the achieved throughput.
- **Failed / Retried** — packets the kernel refused or made you retry; non-zero
  values here usually mean you're hitting a send-buffer limit and should
  consider `--preload-pcap` or a fast-path backend.

## See also

- [Quickstart](../getting-started/quickstart.md) — the tool in a five-minute loop.
- [Performance & line-rate testing](../guides/performance-testing.md).
- [`tcpreplay` man page](../reference/man-pages.md) — every option.
