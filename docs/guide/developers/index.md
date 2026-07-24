---
title: Developers
description: Embed the libraries, extend the plugins, and contribute.
---

# Developers

For building *on* Tcpreplay rather than just using it — embedding the engines in
your own program, adding link-layer support, or contributing to the project.

<div class="grid cards" markdown>

-   :material-play-box: **[Using libtcpreplay](libtcpreplay.md)**

    ---

    Drive the replay engine from your own C program and read live statistics —
    no forking the binary and scraping stdout.

-   :material-file-edit: **[Using libtcpedit](libtcpedit.md)**

    ---

    Embed the packet-rewriting engine that `tcprewrite`, `tcpbridge` and
    `tcpreplay-edit` share.

-   :material-puzzle-plus: **[Writing a DLT plugin](writing-dlt-plugins.md)**

    ---

    Add support for a new link-layer type by writing one self-contained plugin.

-   :material-sitemap: **[Plugin architecture](plugin-architecture.md)**

    ---

    The design rationale behind the DLT plugin system.

-   :material-source-pull: **[Contributing](../contributing.md)**

    ---

    Build, test, and submit changes to the project.

</div>
