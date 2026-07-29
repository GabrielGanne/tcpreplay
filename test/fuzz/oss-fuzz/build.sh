#!/bin/bash -eu
#
# OSS-Fuzz build script for tcpreplay.
#
# OSS-Fuzz supplies the compiler, the sanitizer flags and $LIB_FUZZING_ENGINE,
# so this deliberately does not choose any of them - it configures the tree the
# normal way and links the targets against whatever the engine is. That keeps
# the same harnesses working under libFuzzer, AFL++ and Honggfuzz without
# per-engine variants.
#
# Referenced from an oss-fuzz/projects/tcpreplay/Dockerfile that clones this
# repo and runs this script. Kept in-tree so it stays in step with the targets.

cd "$SRC/tcpreplay"

./autogen.sh
# --disable-local-libopts: the tearoff is CLI plumbing, not attack surface, and
# building it wastes fuzzing budget.
./configure --disable-local-libopts
make -j"$(nproc)"

FUZZ_CFLAGS="-DHAVE_CONFIG_H -I. -Isrc -Itest/fuzz"
FUZZ_LIBS="src/fragroute/libfragroute.a src/common/libcommon.a -lpcap"

# libdnet is optional; fragroute needs it
if [ -f src/fragroute/libfragroute.a ]; then
    FUZZ_LIBS="$FUZZ_LIBS -ldnet"
fi

for target in fuzz_services fuzz_pcap fuzz_fragroute; do
    [ -f "test/fuzz/${target}.c" ] || continue

    $CC $CFLAGS $FUZZ_CFLAGS -c "test/fuzz/${target}.c" -o "/tmp/${target}.o"
    $CXX $CXXFLAGS "/tmp/${target}.o" $LIB_FUZZING_ENGINE $FUZZ_LIBS \
        -o "$OUT/${target}"

    # ship the seed corpus so the engine starts from valid inputs
    name="${target#fuzz_}"
    if [ -d "test/fuzz/corpus/$name" ]; then
        zip -j "$OUT/${target}_seed_corpus.zip" "test/fuzz/corpus/$name"/* > /dev/null
    fi
done
