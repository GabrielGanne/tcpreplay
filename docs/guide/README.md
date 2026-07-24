# Tcpreplay user guide (source)

This directory is the **source of truth** for the Tcpreplay user guide. It is
written in Markdown and built into a static site with
[MkDocs Material](https://squidfunk.github.io/mkdocs-material/).

The rendered guide is served at <https://tcpreplay.appneta.com/> and is copied
into the `appneta.github.io` site repo as needed (see below).

## Layout

```
docs/
  mkdocs.yml            # site configuration
  guide/                # <- this directory: the Markdown sources (docs_dir)
    index.md            # landing page
    getting-started/    # installation, quickstart
    tools/              # one page per binary
    guides/             # how-to recipes
    concepts/           # explanation / mental model
    reference/          # cheat sheet, sample captures, man-page links
    contributing.md
    assets/             # css and images
    requirements.txt    # build dependencies
  site/                 # build output (git-ignored)
```

The information architecture follows the [Diátaxis](https://diataxis.fr/)
framework — tutorials, how-to guides, reference, and explanation each have their
own home so every page has one clear job.

## Build and preview

```sh
cd docs
pip install -r guide/requirements.txt
mkdocs serve          # live preview at http://127.0.0.1:8000
mkdocs build          # static site into docs/site/
```

`mkdocs serve` reloads on save, so it's the fastest way to write.

## Editing guidelines

- **Task-oriented.** Show how to do a job. Link to the man pages
  (`reference/man-pages.md`) for exhaustive option lists rather than duplicating
  them — the man pages are generated from the same `*_opts.def` files the tools
  are built from, so they never drift.
- **Correctness first.** Flags and behaviour must match the current release.
  When a feature changes, update the guide in the same PR.
- **Portable Markdown.** Stick to standard Markdown plus the Material extensions
  already used (admonitions, content tabs, `grid cards`). That keeps pages easy
  to copy into the Jekyll site.

## Copying to appneta.github.io

The website (`appneta.github.io`) is a Jekyll site. Until the two are unified,
the workflow is:

1. Build here (`mkdocs build`) and review locally.
2. For content that should also live on the Jekyll site, copy the relevant
   Markdown into the site's `wiki/` tree and add the small Jekyll front matter
   (`layout`, `title`, `categories`, `description`) those pages use.
3. Alternatively, publish the full MkDocs site to its own path — see the docs CI
   workflow (`.github/workflows/docs.yml`), which builds the site on every push
   and can be extended to deploy it.

Keeping the source here means the guide versions with the code and is reviewed in
the same pull requests as the features it documents.
