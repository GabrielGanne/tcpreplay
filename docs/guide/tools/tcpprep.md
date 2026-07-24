---
title: tcpprep
description: Classify a capture's packets as client or server into a cache file.
---

# tcpprep

**Pre-processes a pcap into a cache file.** It reads a capture once, decides for
each packet whether it's client-to-server or server-to-client, and writes a
compact *cache file* recording those decisions. `tcpreplay` and `tcprewrite`
then read that cache to make direction-aware choices — most importantly, which
NIC to send each packet out of during a two-NIC in-line test.

```
tcpprep [OPTION]... --pcap=<in.pcap> --cachefile=<out.cache>
```

`tcpprep` only reads and analyses; it needs no privileges and touches no
network.

## Why a separate step?

Splitting classification out of the replay has two benefits:

1. **Speed.** The (potentially expensive) analysis happens once, offline. The
   real-time replay just reads a lookup table.
2. **Control.** You can inspect, tweak, or hand-author the classification
   independently of the send.

See [The tcpprep cache pipeline](../concepts/prep-cache-pipeline.md) for the
design.

## Basic use

```console
$ tcpprep --auto=bridge --pcap=in.pcap --cachefile=in.cache
```

Then hand the cache to another tool with `-c` / `--cachefile`:

```console
$ sudo tcpreplay -i eth0 -I eth1 -c in.cache in.pcap
```

## Classification modes

You pick *how* packets get split into client and server:

=== "Automatic"

    ```console
    $ tcpprep --auto=MODE --pcap=in.pcap --cachefile=in.cache
    ```

    `tcpprep` builds a tree of the IPs it sees and infers roles. `MODE` is one
    of:

    | Mode | Heuristic |
    | --- | --- |
    | `bridge` | Split by MAC address (Layer 2). |
    | `router` | Split by IP subnet, learning which side is which. |
    | `client` | Ambiguous hosts are treated as clients. |
    | `server` | Ambiguous hosts are treated as servers. |
    | `first` | The first host seen in a conversation is the client. |

=== "By network (CIDR)"

    ```console
    $ tcpprep --cidr=10.0.0.0/8 --pcap=in.pcap --cachefile=in.cache
    ```

    Everything in the given network(s) is the client side; everything else is
    the server side. (`-c` is the short form — note this is *not* the same as
    the `--cachefile` short flag on the consuming tools.)

=== "By port"

    ```console
    $ tcpprep --port --pcap=in.pcap --cachefile=in.cache
    ```

    Classify by well-known server ports (from `/etc/services`, or a file given
    with `--services`).

=== "By regex / MAC"

    ```console
    $ tcpprep --regex='^10\.1\.' --pcap=in.pcap --cachefile=in.cache
    $ tcpprep --mac=00:11:22:33:44:55 --pcap=in.pcap --cachefile=in.cache
    ```

    Match source IPs against a regular expression, or split on a specific MAC.

Add `--reverse` to swap the client/server assignment.

## Inspecting a cache

To see how packets were classified without running a replay:

```console
$ tcpprep --print-info --cachefile=in.cache --pcap=in.pcap
```

You can also embed a note in the cache with `--comment` and read it back with
`--print-comment` — handy for recording how a cache was produced.

## See also

- [Splitting client/server traffic](../guides/client-server-split.md) — the
  end-to-end two-NIC recipe.
- [The tcpprep cache pipeline](../concepts/prep-cache-pipeline.md) — the concept.
- [`tcpprep` man page](../reference/man-pages.md) — every option.
