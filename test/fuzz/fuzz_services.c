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
 * Fuzz target: the tcpprep --services file parser.
 *
 * GHSA-fwcr-mqg6-hqmx (CWE-121) lived here. parse_services() matched each
 * line against "([0-9]+)/(tcp|udp)" and copied the port substring into a
 * 10-byte stack buffer using the *regex match length* as the count. The digit
 * group is unbounded, so a long enough run of digits before /tcp overflowed
 * the buffer - and at larger sizes gave strncpy overlapping source and
 * destination.
 *
 * A fuzzer reaches that in seconds: the input is a text file, and the
 * interesting shape is "lots of digits, then a slash". This target exists so
 * the next one of those is found here rather than in someone's inbox.
 */

#include "fuzz_common.h"

#include "defines.h" /* tcpr_services_t, before services.h uses it */
#include "common/services.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /*
     * tcpr_services_t is two 64KB arrays. Keep it static rather than putting
     * 128KB on the stack every iteration - libFuzzer runs this millions of
     * times, and a stack overflow here would be the harness's fault, not the
     * code's.
     */
    static tcpr_services_t services;
    char path[FUZZ_PATH_MAX];

    /* parse_services() takes a filename, so the input has to reach the disk */
    if (fuzz_write_tempfile(data, size, path, sizeof(path)) != 0)
        return 0;

    memset(&services, 0, sizeof(services));
    parse_services(path, &services);

    unlink(path);
    return 0;
}
