# Tcpreplay documentation site (source)

This directory is the **source of truth for the entire Tcpreplay website**. It is
written in Markdown and built into a static site with
[MkDocs Material](https://squidfunk.github.io/mkdocs-material/).

It replaces the previous Jekyll site (`appneta.github.io`): the user guide, the
tool docs, the developer/library guides, the FAQ, the man pages, and the About
pages all live here now, so the docs version with the code and are reviewed in
the same pull requests as the features they describe.

## Layout

```
docs/
  mkdocs.yml            # site configuration
  hooks.py              # build hooks: render man pages, emit legacy redirects
  guide/                # <- the Markdown sources (docs_dir)
    index.md            # landing page
    getting-started/    # installation, quickstart
    tools/              # one page per binary
    guides/             # how-to recipes
    concepts/           # explanation / mental model
    developers/         # library + plugin guides, contributing
    reference/          # cheat sheet, sample captures, man-page links
      man/              # generated man pages (git-ignored, rendered from ../src/*.adoc)
    about/              # history, support & community
    faq.md
    assets/             # css
    requirements.txt    # build dependencies
  site/                 # build output (git-ignored)
```

The information architecture follows the [Diátaxis](https://diataxis.fr/)
framework — tutorial, how-to, reference, and explanation each have their own home.

## Build and preview

Use a virtual environment — modern Python installs (Homebrew, recent Debian/
Ubuntu) refuse a system-wide `pip install` (PEP 668, *externally-managed-
environment*):

```sh
cd docs
python3 -m venv .venv          # once
source .venv/bin/activate      # each shell (Windows: .venv\Scripts\activate)
pip install -r guide/requirements.txt
mkdocs serve                   # live preview at http://127.0.0.1:8000
mkdocs build                   # static site into docs/site/
```

Run `deactivate` to leave the environment. `.venv/` is git-ignored.

!!! tip "One-off without activating"
    `pipx run --spec mkdocs-material mkdocs serve` runs it in a throwaway
    environment — handy for a quick look, though a venv is better for repeated
    edits.

`mkdocs serve` reloads on save. The man pages render from `../src/*.adoc`
(produced by the main build); if those or `asciidoctor` are absent, the build
still succeeds and shows man-page placeholders — so a docs-only checkout previews
fine.

## Man pages

`hooks.py` renders each `src/*.adoc` (generated from the `*_opts.def` option
definitions) into `guide/reference/man/` at build time, so the hosted man pages
never drift from the tools. Those files are git-ignored and regenerated every
build.

## Legacy redirects

`hooks.py` also writes static redirect stubs at the old Jekyll URLs
(`/wiki/*.html`) pointing to their new locations, so existing inbound links keep
working after the Jekyll site is retired. The map lives in `hooks.py`
(`REDIRECTS`).

## Publishing and cutover

Two workflows drive this:

- **`.github/workflows/docs.yml`** builds the guide with `--strict` on every docs
  change, so a dead link or bad config fails CI.
- **`.github/workflows/docs-deploy.yml`** builds the full site (with rendered man
  pages) and publishes it to this repository's **GitHub Pages** — a reviewable
  preview at `https://appneta.github.io/tcpreplay/` that does **not** touch the
  live site or claim the `tcpreplay.appneta.com` domain.

**Cutover to production** (retiring the Jekyll site) is a deliberate, manual step:

1. Review the preview build.
2. Enable Pages on this repo (Settings → Pages → Source: *GitHub Actions*) if not
   already, and let `docs-deploy` publish.
3. Move the `tcpreplay.appneta.com` custom domain from the `appneta.github.io`
   repo to this repo's Pages (add the `CNAME`, update DNS as needed).
4. Archive the old `appneta.github.io` Jekyll content.

Keeping the domain move as the final manual step means nothing about the live
site changes until you decide to flip it.
