#!/bin/sh
#
# Replay every checked-in corpus input through its fuzz target.
#
# This is the regression half of the fuzzing work: OSS-Fuzz (or a local run)
# finds a crashing input, it gets committed under corpus/, and from then on
# every `make check` re-runs it. No clang and no libFuzzer needed - the
# targets are built with the standalone driver in this mode - so the check
# works on every platform the suite builds on, which is the point.
#
# Build it under --enable-asan for this to catch anything subtler than an
# outright segfault.

set -u
status=0

srcdir="${srcdir:-.}"

for target in fuzz_services fuzz_pcap fuzz_fragroute; do
    [ -x "./$target" ] || continue          # fragroute is optional
    name=$(echo "$target" | sed 's/^fuzz_//')
    dir="$srcdir/corpus/$name"

    [ -d "$dir" ] || continue

    # shellcheck disable=SC2086
    set -- "$dir"/*
    [ -e "$1" ] || continue                 # empty corpus

    if ./"$target" "$@" > /dev/null 2>&1; then
        echo "PASS: $target ($# input(s))"
    else
        echo "FAIL: $target - one of $# input(s) crashed; rerun ./$target $dir/* to see which"
        status=1
    fi
done

exit $status
