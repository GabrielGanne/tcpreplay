---
title: How-to guides
description: Task-oriented recipes for common Tcpreplay jobs.
---

# How-to guides

Goal-oriented recipes. Each assumes you've done the
[Quickstart](../getting-started/quickstart.md) and know the basics; here we
tackle real jobs end to end.

<div class="grid cards" markdown>

-   :material-speedometer: **[Performance & line-rate testing](performance-testing.md)**

    Push measured, repeatable load at a device and account for every packet.

-   :material-rocket: **[Fast-path backends](fast-paths.md)**

    Wire-rate transmission on commodity NICs with io_uring, AF_XDP, or netmap.

-   :material-file-replace: **[Rewriting packet headers](rewriting-headers.md)**

    Retarget a capture at your lab: MACs, IPs, ports, VLANs, DLT.

-   :material-account-switch: **[Splitting client/server traffic](client-server-split.md)**

    Drive an in-line device with a two-NIC, direction-aware replay.

-   :material-server-network: **[Replaying a TCP session to a live server](live-tcp-replay.md)**

    Exercise a real server up the full stack with `tcpliveplay`.

</div>
