---
title: Man pages
description: The complete, generated option reference for every tool.
---

# Man pages

The man pages are the **authoritative, exhaustive** reference for every option.
This user guide is deliberately task-oriented and links out to them rather than
duplicating the full flag list — the man pages are generated from the same
option definitions the tools are built from, so they never drift.

## On your system

Once installed, each tool has a standard man page:

```console
$ man tcpreplay
$ man tcprewrite
$ man tcpprep
$ man tcpbridge
$ man tcpliveplay
$ man tcpcapinfo
$ man tcpreplay-edit
```

## From the tool itself

Every tool prints its full option list on demand:

```console
$ tcpreplay --help          # full usage
$ tcpreplay --version       # version and build info
```

## On this site

The rendered man pages are hosted here under **Manual reference**:

- [tcpreplay](man/tcpreplay.md)
- [tcprewrite](man/tcprewrite.md)
- [tcpprep](man/tcpprep.md)
- [tcpbridge](man/tcpbridge.md)
- [tcpliveplay](man/tcpliveplay.md)
- [tcpcapinfo](man/tcpcapinfo.md)
- [tcpreplay-edit](man/tcpreplay-edit.md)

!!! info "How the man pages are generated"
    Option definitions live in `*_opts.def` files. A build step turns those into
    both the compiled CLI parser and the man page source, then renders the man
    pages. Because both come from one source, the documented options always match
    the built binary.
