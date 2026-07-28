---
title: tcpbridge
description: Bridge two network segments, rewriting packets in flight.
---

# tcpbridge

**Bridges two network segments**, optionally rewriting packets as they cross.
Where `tcpreplay` sends a *saved* capture, `tcpbridge` forwards *live* traffic
between two interfaces — applying the same `libtcpedit` rewriting engine as
[`tcprewrite`](tcprewrite.md) to each packet in flight.

```
tcpbridge [OPTION]... --intf1=<ethA> --intf2=<ethB>
```

!!! danger "Forwards live traffic — needs root"
    `tcpbridge` moves real packets between real interfaces. Use it only on lab
    segments you control.

## What it's for

`tcpbridge` is useful when you want a device or host to see *modified* live
traffic without changing the source. Sitting between two segments, it can, for
example:

- rewrite addresses or VLAN tags so traffic from one lab network appears to come
  from another;
- normalize or mangle headers to test how a downstream device copes;
- convert between link-layer types across the bridge.

## Basic use

```console
$ sudo tcpbridge --intf1=eth0 --intf2=eth1
```

By default it forwards frames between `eth0` and `eth1`. Add any
[`tcprewrite`](tcprewrite.md) editing option to transform packets as they pass:

```console
$ sudo tcpbridge --intf1=eth0 --intf2=eth1 \
    --pnat=10.0.0.0/8:192.168.0.0/16 \
    --enet-vlan=del \
    --fixcsum
```

## Direction and filtering

Because a bridge inherently has two sides, the rewriting options that take
separate client/server values apply naturally: traffic entering `--intf1` is one
direction, `--intf2` the other. You can also limit which traffic is bridged with
the same include/exclude and filtering options the other tools use.

## When to reach for something else

- To send a **saved** capture rather than forward live traffic, use
  [`tcpreplay`](tcpreplay.md).
- To edit a capture **into a file** without any network, use
  [`tcprewrite`](tcprewrite.md).

## See also

- [`tcprewrite`](tcprewrite.md) — the same editing options, applied to a file.
- [DLT plugins](../concepts/dlt-plugins.md) — link-layer conversion across the bridge.
- [`tcpbridge` man page](../reference/man-pages.md) — every option.
