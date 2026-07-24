---
title: Replaying a TCP session to a live server
description: Exercise a real server up the full stack with tcpliveplay.
---

# Replaying a TCP session to a live server

Most of the suite pushes frames onto the wire without caring whether anything
listens. [`tcpliveplay`](../tools/tcpliveplay.md) is the exception: it holds a
*real* TCP conversation with a live server — handshake, sequence tracking,
adapting to the server's actual replies — so you test the server and its full
stack, not just Layer 2.

!!! danger "This connects to a real server"
    `tcpliveplay` opens a genuine connection to the destination you name. Only
    run it against servers you're authorized to test.

## When to use it

Reach for `tcpliveplay` when the *server's behaviour* is what you're testing —
replaying a captured client exchange (an HTTP request, a protocol handshake)
against a fresh instance of that server to observe how it responds or to
reproduce a bug.

It is built for **single-session client-side TCP**: the capture should contain
one TCP conversation, and you replay the client half against the server.

## Step 1 — capture a suitable session

On the client, capture just the conversation with the server, filtered tightly so
the file contains a single clean session:

```console
$ sudo tcpdump -i eth0 -w session.pcap host 10.0.0.20 and tcp port 80
```

Then generate the traffic once (make the request), and stop the capture. Confirm
it's a single session:

```console
$ tcpcapinfo session.pcap
```

## Step 2 — replay it against the server

```console
$ sudo tcpliveplay eth0 session.pcap 10.0.0.20 00:11:22:33:44:55 80
```

The arguments are the interface, the capture, and the destination **IP**, **MAC**
and **port**:

| Argument | Value |
| --- | --- |
| interface | `eth0` |
| capture | `session.pcap` |
| dest IP | `10.0.0.20` |
| dest MAC | `00:11:22:33:44:55` |
| dest port | `80` (or `random`) |

`tcpliveplay` rewrites the sequence/ack numbers, addresses and ports so the
recorded client exchange becomes a live one against the server you specify, then
reports how the exchange went.

## Why not just `tcpreplay`?

`tcpreplay` would blast the captured frames — including the *server's* packets and
the original sequence numbers — onto the wire, where a real server would reject
them as nonsense (wrong sequence numbers, unexpected addresses, a connection it
never opened). `tcpliveplay` instead *becomes* the client and negotiates a real
connection, which is the only way to meaningfully drive a live server from a
capture.

## See also

- [`tcpliveplay`](../tools/tcpliveplay.md) — the tool reference.
- [The replay model](../concepts/replay-model.md) — why the other tools are
  Layer 2 and this one isn't.
