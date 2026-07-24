---
title: Support & community
description: How to get help, report bugs, and join the Tcpreplay community.
---

# Support & community

## Getting help

Before asking, please check the resources that most often already hold the
answer:

- This user guide and the [FAQ](../faq.md).
- The [man pages](../reference/man-pages.md) (`man tcpreplay`, `tcpreplay --help`).
- The [GitHub Discussions](https://github.com/appneta/tcpreplay/discussions) and
  [issue tracker](https://github.com/appneta/tcpreplay/issues) — someone may have
  hit the same thing.
- The `tcpreplay-users`
  [mailing list archives](http://sourceforge.net/mailarchive/forum.php?forum_name=tcpreplay-users).

Still stuck? Ask on
[GitHub Discussions](https://github.com/appneta/tcpreplay/discussions) or the
[tcpreplay-users mailing list](https://lists.sourceforge.net/lists/listinfo/tcpreplay-users).

## Reporting a good bug report

The quality of a bug report largely determines how fast it can be resolved.
Include enough for someone to reproduce it.

=== "Build / compile problems"

    - The Tcpreplay version you're compiling.
    - Your platform (OS, version, architecture).
    - The contents of `config.status`.
    - The output of `configure` and `make`.

=== "Runtime problems"

    - Version information (`tcpreplay --version`).
    - The exact command line — options and arguments.
    - Your platform (OS, version, architecture).
    - The make/model of the NIC(s) and driver version, if relevant.
    - The error message and a description of what happened vs. what you expected.

File it on the [issue tracker](https://github.com/appneta/tcpreplay/issues).

!!! warning "Security issues go through a private channel"
    Do **not** report a security vulnerability in a public issue. Use GitHub's
    private security-advisory process — see `docs/SECURITY.md` in the repository.

## Contributing

Contributions — bug fixes, features, docs — are welcome:

- **Discuss first?** Design questions and proposals fit
  [GitHub Discussions](https://github.com/appneta/tcpreplay/discussions).
- **Ready to code?** See [Contributing](../contributing.md) for the fork → branch
  → pull-request workflow and the build/test setup.
- **Digging into internals?** The [Developers](../developers/index.md) section has
  the library and plugin guides.

Developers can also follow the
[tcpreplay-devel](https://lists.sourceforge.net/lists/listinfo/tcpreplay-devel)
mailing list; how-to questions belong on `tcpreplay-users` instead.

## The mailing lists

| List | For |
| --- | --- |
| [tcpreplay-users](https://lists.sourceforge.net/lists/listinfo/tcpreplay-users) | Usage questions and help. |
| [tcpreplay-devel](https://lists.sourceforge.net/lists/listinfo/tcpreplay-devel) | Development discussion. |
