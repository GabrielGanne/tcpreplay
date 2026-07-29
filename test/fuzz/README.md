# Fuzz targets

Coverage-guided fuzzing for the interfaces that read untrusted input (#1092).

## Why

15 security advisories were published against this project in 2026. Every one
was reported from outside, and the bug classes are what a fuzzer finds in
minutes: off-by-one writes, unchecked attacker-supplied lengths, sign
confusion, unbounded copies. The fragroute rules parser alone accounted for
five.

These tools read pcap files and rules files that somebody else produced. That
is textbook fuzz surface, and it was being covered by volunteers.

## The targets

| Target | Surface | Advisories it would have covered |
|---|---|---|
| `fuzz_pcap` | libpcap read path plus the shared header parsers, DLT chosen by the input | GHSA-jj65-mrgg-f5fx, GHSA-5q26-7fxx-v8fh, GHSA-m6w7-8497-g9c9, GHSA-ww62-mxv7-pg55 |
| `fuzz_fragroute` | rules-file parser and the module chain it builds | GHSA-777w-9599-w8g4, GHSA-p7xp-4gj2-x56c, GHSA-27v4-xhfx-g2rx, GHSA-m655-53p4-6qm8, GHSA-v8c4-9w98-9v6v |
| `fuzz_services` | `tcpprep --services` parser | GHSA-fwcr-mqg6-hqmx |

`fuzz_fragroute` found a double-free the first time it ran (#1102).

## Running them

**Fuzzing** needs clang:

```console
$ CC=clang ./configure --enable-fuzzer
$ make
$ ASAN_OPTIONS=detect_leaks=0 ./test/fuzz/fuzz_pcap test/fuzz/corpus/pcap -max_total_time=60
```

`detect_leaks=0` is deliberate: the tools exit without freeing everything, and
leak reports would bury the memory-corruption findings that matter here.

**Replaying the corpus** needs nothing — no clang, no libFuzzer. This is the
default build, and `make check` does it:

```console
$ ./configure && make check
PASS: run-corpus.sh
```

That split is the point. When OSS-Fuzz finds a crashing input, commit it under
`corpus/<target>/` and it becomes a regression test that runs everywhere the
suite builds, for everyone, forever — not just for whoever has a fuzzing
toolchain installed.

Build with `--enable-asan` for the corpus replay to catch anything subtler
than an outright crash.

## Adding a crashing input

```console
$ cp crash-3f9a... test/fuzz/corpus/fragroute/
$ git add test/fuzz/corpus/fragroute/crash-3f9a...
```

Name it after what it demonstrates where practical.

## OSS-Fuzz

`oss-fuzz/` holds the build script and project metadata. Submitting means
opening a PR against [google/oss-fuzz](https://github.com/google/oss-fuzz)
adding `projects/tcpreplay/` with a `Dockerfile` that clones this repo, plus
the `build.sh` and `project.yaml` from here. Keeping them in-tree means they
stay in step with the targets instead of drifting in a repo nobody here
watches.

`build.sh` deliberately does not pick a compiler or sanitizer flags —
OSS-Fuzz supplies those, and linking against `$LIB_FUZZING_ENGINE` means the
same harnesses work under libFuzzer, AFL++ and Honggfuzz unchanged.
