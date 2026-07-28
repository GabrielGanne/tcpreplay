
<!-- @import "[TOC]" {cmd="toc" depthFrom=1 depthTo=6 orderedList=false} -->

<!-- code_chunk_output -->

- [Security Policy](#security-policy)
  - [Supported Versions](#supported-versions)
  - [Scope](#scope)
  - [Reporting a Vulnerability](#reporting-a-vulnerability)
  - [Disclosure Policy](#disclosure-policy)
  - [Published Advisories](#published-advisories)

<!-- /code_chunk_output -->

# Security Policy

## Supported Versions

We release patches for security vulnerabilities. Which versions are eligible to
receive such patches depends on the CVSS v3.0 rating:

| CVSS v3.0 | Supported Versions                 |
| --------- | ---------------------------------- |
| 9.0-10.0  | Releases within the previous year  |
| 4.0-8.9   | Most recent release                |

Tcpreplay parses untrusted, attacker-controlled input (pcap files, cache
files, fragroute rules files, and, for `tcpreplay`/`tcpbridge`/`tcpliveplay`,
live traffic) as its normal mode of operation, so most reported issues are
memory-safety bugs in that parsing path rather than issues in a network-facing
service.

## Scope

In scope:

- Packet/header parsing and rewriting in `libtcpedit` (`src/tcpedit/`) and its
  DLT plugins (`src/tcpedit/plugins/`) — the code that decodes pcap-derived
  packet data.
- The `tcpprep` cache file format and reader (`src/common/cache.c`, `tree.c`).
- The `fragroute` rule parser and mangling modules (`src/fragroute/`).
- The packet-injection abstraction (`src/common/sendpacket.c`) and its
  backends (libpcap, BPF, `PF_PACKET`, TX_RING, netmap, XDP/AF_XDP, libdnet,
  libnet, tuntap, KHIAL).
- `tcpliveplay`'s live TCP session replay path, since it drives a real network
  stack rather than only emitting L2 frames.
- Denial-of-service and memory-corruption bugs (heap/stack buffer overflow,
  out-of-bounds read/write, use-after-free, double-free, NULL-pointer
  dereference, reachable assertion, infinite loop) reachable from a crafted
  pcap, cache file, or fragroute rules file.

Out of scope:

- Findings that require the operator to already have local root/administrator
  privileges (tcpreplay's raw-socket/injection paths already require elevated
  privileges to run at all).
- Denial-of-service claims that only describe transmitting a large or
  malformed volume of traffic onto the wire — that's tcpreplay's intended
  function.
- Issues in third-party dependencies (`libpcap`, `libpcre`) — report those
  upstream; we'll still take a report here if it's specifically exploitable
  through tcpreplay's use of the dependency.

## Reporting a Vulnerability

Preferred: use GitHub's private vulnerability reporting on this repository —
open the **Security** tab and click **Report a vulnerability**
(https://github.com/appneta/tcpreplay/security/advisories/new). This keeps
the report and any discussion private until a fix ships and lets us draft the
GHSA advisory directly from your report.

Alternatively, email [tcpreplay.dev@gmail.com](mailto:tcpreplay.dev@gmail.com).

Please do not open a public GitHub issue for a suspected vulnerability.

Include, where possible:

- A description of the vulnerability and its impact.
- The affected binary/binaries and, if known, the file/function involved.
- Steps to reproduce, ideally with a minimal pcap/cache/rules file attached.
- Any crash output (ASan/UBSan, `gdb` backtrace, etc.) you already have.

You will receive a response within 72 hours acknowledging receipt.

## Disclosure Policy

- We follow coordinated disclosure. Once a report is confirmed, we work on a
  fix and, historically, ship a patch release within a few days depending on
  complexity.
- We will request a CVE and/or publish a GitHub Security Advisory (GHSA) for
  the issue, and will credit the reporter in the advisory and in
  `docs/CHANGELOG`, unless the reporter asks to remain anonymous.
- We ask that reporters hold public disclosure (blog posts, talks, public
  issue/PR comments) until a patched release is available.

## Published Advisories

Past advisories, including CVE/GHSA IDs and affected versions, are published
at https://github.com/appneta/tcpreplay/security/advisories and summarized
with each fix in `docs/CHANGELOG`.
