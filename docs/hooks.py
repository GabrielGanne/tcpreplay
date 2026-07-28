"""MkDocs build hooks for the Tcpreplay guide.

Renders the man pages into the site from their AsciiDoc sources at build time,
so the guide hosts them directly (no dependence on the old Jekyll site) and they
never drift from the tools.

The man-page AsciiDoc lives in ../src/*.adoc, generated from the *_opts.def
option definitions by the main build. Here we render each to embeddable HTML
with `asciidoctor -s` and wrap it in a Markdown page under
reference/man/<tool>.md (git-ignored, regenerated every build).

If asciidoctor or the .adoc sources aren't present (e.g. a docs-only checkout),
the hook logs a warning and skips — a local `mkdocs serve` still works, it just
won't include the man pages. CI installs asciidoctor so the published site has
them.
"""

from __future__ import annotations

import logging
import shutil
import subprocess
from pathlib import Path

log = logging.getLogger("mkdocs.hooks.manpages")

# tool -> one-line description for the page front matter
TOOLS = {
    "tcpreplay": "Replay pcap files onto the network at arbitrary speeds.",
    "tcprewrite": "Rewrite Layer 2-4 headers in a pcap file.",
    "tcpprep": "Classify a capture's packets as client or server.",
    "tcpbridge": "Bridge two network segments, rewriting packets in flight.",
    "tcpliveplay": "Replay a captured TCP session against a live server.",
    "tcpcapinfo": "Inspect and debug the raw structure of a pcap file.",
    "tcpreplay-edit": "Replay with tcprewrite's editing options folded in.",
}


def on_pre_build(config, **kwargs) -> None:
    docs_dir = Path(config["docs_dir"])
    src_dir = docs_dir.parent.parent / "src"
    out_dir = docs_dir / "reference" / "man"
    out_dir.mkdir(parents=True, exist_ok=True)

    asciidoctor = shutil.which("asciidoctor")
    header = (
        "---\ntitle: {tool}(1)\ndescription: {desc}\nsearch:\n  boost: 0.5\n---\n\n"
        "# {tool}(1)\n\n"
    )
    intro = (
        '!!! info "Generated reference"\n'
        "    This page is rendered from the `{tool}` man page — the complete "
        "option list. For task-oriented help see the "
        "[tools](../../tools/index.md) and [guides](../../guides/index.md).\n\n"
    )
    placeholder = (
        '!!! note "Man page not rendered in this build"\n'
        "    The full `{tool}` man page renders on the published site (and in "
        "any build with the AsciiDoc sources + asciidoctor present). Meanwhile, "
        "run `man {tool}` or `{tool} --help`, or see "
        "[{tool}](../../tools/{tool_page}).\n"
    )

    rendered = 0
    for tool, desc in TOOLS.items():
        page = header.format(tool=tool, desc=desc)
        adoc = src_dir / f"{tool}.adoc"
        html = None
        if asciidoctor and adoc.exists():
            try:
                html = subprocess.run(
                    [asciidoctor, "-s", "-o", "-", str(adoc)],
                    check=True, capture_output=True, text=True,
                ).stdout
                rendered += 1
            except subprocess.CalledProcessError as exc:  # pragma: no cover
                log.warning("asciidoctor failed for %s: %s", tool, exc)

        if html is not None:
            page += intro.format(tool=tool) + html + "\n"
        else:
            tool_page = f"{tool}.md"
            page += placeholder.format(tool=tool, tool_page=tool_page)
        (out_dir / f"{tool}.md").write_text(page)

    if rendered:
        log.info("rendered %d man pages into %s", rendered, out_dir)
    else:
        log.info("no AsciiDoc sources/asciidoctor; wrote man-page placeholders")


# Old Jekyll wiki URL (relative to site root, exactly as served: /wiki/<x>.html)
# -> new page path (source .md, resolved to its output URL below). Generated as
# static redirect stubs in on_post_build so inbound links to the retired Jekyll
# site keep working.
REDIRECTS = {
    "wiki/overview.html": "index.md",
    "wiki/installation.html": "getting-started/installation.md",
    "wiki/howto.html": "guides/performance-testing.md",
    "wiki/usage-examples.html": "reference/cli-cheatsheet.md",
    "wiki/common-arguments.html": "tools/index.md",
    "wiki/captures.html": "reference/sample-captures.md",
    "wiki/faq.html": "faq.md",
    "wiki/history.html": "about/history.md",
    "wiki/support.html": "about/support.md",
    "wiki/developer-community.html": "about/support.md",
    "wiki/maillist.html": "about/support.md",
    "wiki/contributions.html": "contributing.md",
    "wiki/products.html": "tools/index.md",
    "wiki/tcpreplay.html": "tools/tcpreplay.md",
    "wiki/tcprewrite.html": "tools/tcprewrite.md",
    "wiki/tcpprep.html": "tools/tcpprep.md",
    "wiki/tcpbridge.html": "tools/tcpbridge.md",
    "wiki/tcpliveplay.html": "tools/tcpliveplay.md",
    "wiki/tcpcapinfo.html": "tools/tcpcapinfo.md",
    "wiki/tcpreplay-edit.html": "tools/tcpreplay-edit.md",
    "wiki/using-libtcpreplay.html": "developers/libtcpreplay.md",
    "wiki/using-libtcpedit.html": "developers/libtcpedit.md",
    "wiki/dlt-plugin-developer-guide.html": "developers/writing-dlt-plugins.md",
    "wiki/tcpedit-plugin-architecture.html": "developers/plugin-architecture.md",
    "wiki/tcpreplay-man.html": "reference/man/tcpreplay.md",
    "wiki/tcprewrite-man.html": "reference/man/tcprewrite.md",
    "wiki/tcpprep-man.html": "reference/man/tcpprep.md",
    "wiki/tcpbridge-man.html": "reference/man/tcpbridge.md",
    "wiki/tcpliveplay-man.html": "reference/man/tcpliveplay.md",
    "wiki/tcpcapinfo-man.html": "reference/man/tcpcapinfo.md",
    "wiki/tcpreplay-edit-man.html": "reference/man/tcpreplay-edit.md",
}

_STUB = (
    '<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
    '<meta http-equiv="refresh" content="0; url={url}">\n'
    '<link rel="canonical" href="{url}">\n<title>Redirecting…</title>\n'
    '</head>\n<body>\nThis page has moved. Redirecting to '
    '<a href="{url}">{url}</a>.\n</body>\n</html>\n'
)


def _target_url(md_path: str, use_directory_urls: bool) -> str:
    """Output URL of a docs .md page, relative to the /wiki/ stub location."""
    path = md_path[:-3]  # strip .md
    if use_directory_urls:
        url = "" if path == "index" else (
            path[: -len("/index")] + "/" if path.endswith("/index") else path + "/"
        )
    else:
        url = "index.html" if path == "index" else path + ".html"
    # stubs live under wiki/, so step up one level to reach site root
    return "../" + url


def on_post_build(config, **kwargs) -> None:
    site_dir = Path(config["site_dir"])
    use_dir_urls = config.get("use_directory_urls", True)
    written = 0
    for old, target in REDIRECTS.items():
        url = _target_url(target, use_dir_urls)
        dest = site_dir / old
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(_STUB.format(url=url))
        written += 1
    log.info("wrote %d legacy redirect stubs", written)
