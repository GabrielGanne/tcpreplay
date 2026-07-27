---
title: Fast-path backends
description: Wire-rate transmission on commodity NICs with io_uring, AF_XDP, or netmap.
---

# Fast-path backends

The default send path hands each packet to the kernel through a normal socket.
That's portable and fine for moderate rates, but the per-packet overhead caps how
fast you can go. For line-rate work on commodity NICs, Tcpreplay can use a
**fast-path backend** that bypasses much of that overhead.

## The options

| Backend | Flag | Platform | Notes |
| --- | --- | --- | --- |
| **AF_XDP** | `--xdp` | Linux | Kernel-bypass via an XDP socket; the fastest option, and falls back automatically if the adapter can't support it. |
| **io_uring** | `--io-uring` | Linux | Batches sends through the io_uring interface. |
| **netmap** | `--netmap` | Linux/BSD | Long-established kernel-bypass framework. |
| **PF_INET raw** | `--raw` | Linux | *Not* a speed path — routes through the IP stack (see below). |

Each must be **compiled in** — the build has to find the relevant library/headers
(`liburing`, `libxdp`/`libbpf`, or a netmap checkout). Confirm support in the
`configure`/CMake summary, e.g. `LIBXDP for AF_XDP socket: yes`.

## Using one

The backend is just a flag on an otherwise normal replay. Combine it with
`--preload-pcap` for a genuine line-rate test:

=== "AF_XDP"

    ```console
    $ sudo tcpreplay -i eth0 --xdp --topspeed --preload-pcap \
        --loop 1000 sample.pcap
    ```

    If the adapter can't set up an AF_XDP socket — no driver support, or the
    default queue doesn't exist — tcpreplay warns and falls back to the default
    injection method rather than failing the run. Pass `--xdp-no-fallback` to
    turn that into a hard error, which you want when benchmarking: a silent
    change of injector changes what's being measured. `--xdp-batch-size`
    exists for very high-speed links (100GigE+, especially with small packets)
    but shouldn't normally need changing — sends already pipeline across the
    UMEM at the default batch of 1.

=== "io_uring"

    ```console
    $ sudo tcpreplay -i eth0 --io-uring --topspeed --preload-pcap \
        --loop 1000 sample.pcap
    ```

=== "netmap"

    ```console
    $ sudo tcpreplay -i eth0 --netmap --topspeed --preload-pcap \
        --loop 1000 sample.pcap
    ```

    netmap takes over the interface while running; the NIC won't carry normal
    traffic until `tcpreplay` exits.

## Choosing between them

- **AF_XDP** is the recommended first choice: it reaches the highest rates —
  line rate on a 1GigE NIC in testing — and deploys easily, since it falls back
  to the default injector on its own if the adapter or driver can't support it,
  rather than requiring you to know in advance whether it will work.
- **io_uring** is the easiest to have available on a modern Linux kernel and
  needs no special NIC handling, but has no path to zero-copy sends the way
  AF_XDP does — a solid fallback where AF_XDP isn't compiled in.
- **netmap** is mature and cross-platform but takes exclusive control of the NIC
  and needs its kernel module/driver support.

If none is compiled in, `--preload-pcap` on the normal path is still your best
lever — see [Performance testing](performance-testing.md).

## `--raw` is different

`--raw` is listed with the backends because it's also an alternate send path,
but it is **not** about speed. It injects through a PF_INET raw IP socket, so the
packet goes *through* the kernel's IP stack — routing, netfilter/iptables — like
any locally generated traffic. Use it when you want the stack to process the
replayed packets, accepting that:

- the kernel builds its own Ethernet framing (source/dest MACs from the capture
  are not reproduced), and
- it's IPv4-only.

## See also

- [Performance & line-rate testing](performance-testing.md).
- [Timing & speed control](../concepts/timing-and-speed.md) — why overhead caps rate.
- [Installation](../getting-started/installation.md) — enabling the backends at build time.
