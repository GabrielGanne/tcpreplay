---
title: Contributing
description: How to build, test, and contribute to Tcpreplay.
---

# Contributing

Tcpreplay is open source under the GPLv3 and welcomes contributions — bug
reports, fixes, features, and documentation.

## Where things live

The project is a single C codebase that builds all six tools:

- `src/` — the tool entry points plus the shared replay engine.
- `src/common/` — support code shared by every binary (packet injection, cache
  files, CIDR/MAC helpers, error reporting).
- `src/tcpedit/` — **`libtcpedit`**, the rewriting engine shared by `tcprewrite`,
  `tcpbridge` and `tcpreplay-edit`.
- `src/tcpedit/plugins/` — the [DLT plugins](concepts/dlt-plugins.md), one per
  link-layer type.
- `test/` — fixtures and the test suite.
- `docs/` — this user guide (`docs/guide/`) and project docs.

The `docs/HACKING` file in the repo has the detailed coding standards and
contribution mechanics.

## Build and test

```console
$ cmake -B build && cmake --build build      # build (see Installation)
$ cd test && sudo make test                  # run the test suite (autotools, root)
```

The test suite sends live traffic on the configured NICs and needs root, so run
it on a machine where that's safe — never over the interface you're connected
through. See [Installation](getting-started/installation.md) for build details.

## Improving these docs

This guide is Markdown under `docs/guide/`, built with
[MkDocs Material](https://squidfunk.github.io/mkdocs-material/). To preview your
changes live, build it in a virtual environment (modern Python installs refuse a
system-wide `pip install` — PEP 668):

```console
$ cd docs
$ python3 -m venv .venv && source .venv/bin/activate
$ pip install -r guide/requirements.txt
$ mkdocs serve       # http://127.0.0.1:8000
```

Edits are ordinary Markdown. Keep the task-oriented tone: show people how to do a
job, and link to the [man pages](reference/man-pages.md) for exhaustive option
lists rather than duplicating them here. There's an **edit** pencil on every page
that links straight to its source.

## Reporting bugs and security issues

- **Bugs and features:** open an issue on
  [GitHub](https://github.com/appneta/tcpreplay/issues).
- **Security vulnerabilities:** please report them privately via GitHub's
  security advisory process rather than a public issue — see `docs/SECURITY.md`
  in the repo.

## License and credit

Contributions are licensed under the same GPLv3 as the project, and contributors
are acknowledged in the `docs/CREDIT` file.
