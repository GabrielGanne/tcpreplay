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
 * Minimal unit-test harness for the Tcpreplay suite.
 *
 * Why not a test framework?  The obvious candidate, Google Test, is C++.
 * Tcpreplay is pure C and portable to Linux, macOS, *BSD, Solaris, Haiku and
 * Cygwin, with libpcap as its only hard dependency (see docs/INSTALL).
 * Pulling in gtest would put a C++ toolchain on the critical path of every
 * build and CI job for the sake of a few dozen assertions over pure
 * functions, on platforms where gtest is not reliably packaged.  A C-native
 * framework (Unity, greatest, ...) would mean vendoring third-party code
 * under a second licence.
 *
 * So: no framework, no dependency.  Reporting is TAP-shaped because it reads
 * well and diffs well, and the exit status follows automake's standard test
 * protocol (0 = pass, 1 = fail, 77 = skip, 99 = hard error), which both
 * `make check` and CTest understand natively.  Nothing here is invented
 * where a standard already existed.
 */

#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* automake's standard test exit codes */
#define TAP_EXIT_PASS 0
#define TAP_EXIT_FAIL 1
#define TAP_EXIT_SKIP 77
#define TAP_EXIT_ERROR 99

static unsigned int tap_count;
static unsigned int tap_failed;

static inline void
tap_plan(unsigned int tests)
{
    printf("1..%u\n", tests);
    fflush(stdout);
}

/*
 * Record one assertion.  "ok" is the TAP verdict; everything after is a
 * printf-style description, which for a failure should say what was expected
 * versus what happened - a bare "not ok 7" is not worth much at 3am.
 */
static inline void
tap_ok(int ok, const char *fmt, ...)
{
    va_list ap;

    ++tap_count;
    printf("%sok %u - ", ok ? "" : "not ", tap_count);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    fflush(stdout);

    if (!ok)
        ++tap_failed;
}

/* A diagnostic line; TAP consumers ignore '#' comments, humans do not. */
static inline void
tap_diag(const char *fmt, ...)
{
    va_list ap;

    printf("# ");
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    fflush(stdout);
}

/*
 * Skip the whole file: the feature under test is not compiled in or not
 * supported here.  This is not a failure - TX_RING tests on a BSD box, say.
 */
static inline int
tap_skip_all(const char *reason)
{
    printf("1..0 # SKIP %s\n", reason);
    fflush(stdout);
    return TAP_EXIT_SKIP;
}

static inline int
tap_done(void)
{
    if (tap_failed) {
        tap_diag("%u of %u assertions failed", tap_failed, tap_count);
        return TAP_EXIT_FAIL;
    }

    tap_diag("all %u assertions passed", tap_count);
    return TAP_EXIT_PASS;
}

/* the common case: an assertion whose description is the expression itself */
#define TAP_ASSERT(expr) tap_ok((expr) ? 1 : 0, "%s", #expr)
