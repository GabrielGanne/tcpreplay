---
title: tcpliveplay
description: Replay a captured TCP session against a live server.
---

# tcpliveplay

**Replays a captured TCP session against a live server.** Unlike the rest of the
suite, which pushes Layer 2 frames onto the wire regardless of what's listening,
`tcpliveplay` carries on a *real* TCP conversation: it establishes the
three-way handshake, tracks sequence and acknowledgement numbers, adapts to the
server's actual responses, and tears the connection down. That exercises the
full network stack, all the way to the application.

```
tcpliveplay [OPTION]... <ethX> <file.pcap> <dest IP> <dest MAC> <dport>
```

!!! danger "Talks to a real server — needs root"
    `tcpliveplay` opens a live connection to the destination you name. Point it
    only at servers you're authorized to test.

## What it's for

Use `tcpliveplay` when you need to test the *server*, not just the wire — for
example replaying a captured HTTP or protocol exchange against a real
implementation to see how it behaves, or reproducing a client interaction for
debugging.

Because it negotiates a genuine connection, it only works with **single-session
TCP** captures where the client side is what you're replaying: capture a
conversation between a client and the server under test, then replay the client
half against a fresh instance of that server.

## Basic use

```console
$ sudo tcpliveplay eth0 session.pcap 10.0.0.20 00:11:22:33:44:55 80
```

The positional arguments are:

| Position | Meaning |
| --- | --- |
| `eth0` | The interface to send from. |
| `session.pcap` | The captured TCP session to replay. |
| `10.0.0.20` | The destination server's IP. |
| `00:11:22:33:44:55` | The destination server's MAC. |
| `80` | The destination port — a number, or `random`. |

`tcpliveplay` rewrites the sequence numbers, addresses and ports as needed so
the captured client exchange becomes a live one against the server you specify.

## Contrast with `tcpreplay`

| | `tcpreplay` | `tcpliveplay` |
| --- | --- | --- |
| Sends | Every frame, as-is | A live, adapting TCP conversation |
| Cares if anyone answers? | No | Yes — tracks the server's replies |
| Stack depth | Layer 2 | Full stack, to the application |
| Traffic type | Any pcap | Single-session client-side TCP |

## See also

- [Replaying a TCP session to a live server](../guides/live-tcp-replay.md) — the
  full recipe, including how to capture a suitable session.
- [`tcpliveplay` man page](../reference/man-pages.md) — every option.
