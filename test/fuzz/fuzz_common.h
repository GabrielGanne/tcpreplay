/*
 *   Copyright (c) 2026 Fred Klassen <tcpreplay.dev at gmail dot com> - AppNeta by Broadcom
 *
 *   The Tcpreplay Suite of tools is free software: you can redistribute it
 *   and/or modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of the
 *   License, or with the authors permission any later version.
 *
 *   The Tcpreplay Suite is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Tcpreplay Suite.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Shared plumbing for the fuzz targets.
 *
 * Each target defines LLVMFuzzerTestOneInput() and nothing else. Two ways to
 * run them:
 *
 *   - under libFuzzer (clang -fsanitize=fuzzer,address), for actual fuzzing
 *     and for OSS-Fuzz;
 *   - as a plain program taking files on the command line, built by any C
 *     compiler, which is how the checked-in corpus gets replayed as a
 *     regression test with no fuzzing toolchain present.
 *
 * The second mode is the reason FUZZ_STANDALONE exists. It keeps the targets
 * useful on the platforms this project supports where clang and libFuzzer are
 * not a given, and it means a crashing input found by OSS-Fuzz can be checked
 * in and re-run by anyone.
 */

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define FUZZ_PATH_MAX 128

/*
 * Several code paths under test print to stdout - fragroute's "print" module
 * dumps every packet it sees. At fuzzing rates that is gigabytes of noise and
 * a hard throughput ceiling, so send stdout to /dev/null once, on first use.
 * stderr is left alone: that is where the sanitizers report.
 */
static inline void
fuzz_quiet_stdout(void)
{
    static int done;
    int devnull;

    if (done)
        return;
    done = 1;

    if ((devnull = open("/dev/null", O_WRONLY)) >= 0) {
        dup2(devnull, STDOUT_FILENO);
        if (devnull != STDOUT_FILENO)
            close(devnull);
    }
}

/* libFuzzer's entry point; provided by each target */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

/*
 * Several of the interfaces under test take a *filename* rather than a buffer
 * - parse_services() and fragroute_init() both read a file off disk. Writing
 * the input out is the only honest way to exercise them as the tools do.
 *
 * Returns 0 on success and fills "path".
 */
static inline int
fuzz_write_tempfile(const uint8_t *data, size_t size, char *path, size_t pathlen)
{
    int fd;
    ssize_t written;

    if (pathlen < sizeof("/tmp/tcpr-fuzz-XXXXXX"))
        return -1;

    memcpy(path, "/tmp/tcpr-fuzz-XXXXXX", sizeof("/tmp/tcpr-fuzz-XXXXXX"));

    if ((fd = mkstemp(path)) < 0)
        return -1;

    written = write(fd, data, size);
    close(fd);

    if (written < 0 || (size_t)written != size) {
        unlink(path);
        return -1;
    }

    return 0;
}

#ifdef FUZZ_STANDALONE
/*
 * Corpus replay driver. Each argument is a file fed to the target once, so
 * `fuzz_services corpus/services/*` re-runs the whole corpus. Under a
 * sanitizer build this is a regression test; without one it still catches
 * anything that crashes outright.
 */
int
main(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        FILE *fp = fopen(argv[i], "rb");
        long len;
        uint8_t *buf;

        if (fp == NULL) {
            fprintf(stderr, "%s: cannot open\n", argv[i]);
            return 1;
        }

        if (fseek(fp, 0, SEEK_END) != 0 || (len = ftell(fp)) < 0) {
            fclose(fp);
            fprintf(stderr, "%s: cannot size\n", argv[i]);
            return 1;
        }
        rewind(fp);

        if ((buf = malloc((size_t)len ? (size_t)len : 1)) == NULL) {
            fclose(fp);
            return 1;
        }

        if (len > 0 && fread(buf, 1, (size_t)len, fp) != (size_t)len) {
            free(buf);
            fclose(fp);
            fprintf(stderr, "%s: short read\n", argv[i]);
            return 1;
        }
        fclose(fp);

        LLVMFuzzerTestOneInput(buf, (size_t)len);
        free(buf);

        printf("ok - %s\n", argv[i]);
    }

    return 0;
}
#endif /* FUZZ_STANDALONE */
