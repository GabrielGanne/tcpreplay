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
 * Fuzz target: the fragroute rules-file parser and module chain.
 *
 * More advisories have come out of this one file than anywhere else in the
 * tree:
 *
 *   GHSA-777w-9599-w8g4  stack overflow in mod_open()'s success-path
 *                        diagnostic, from a rules file of a few hundred
 *                        perfectly valid one-word directives
 *   GHSA-p7xp-4gj2-x56c  OOB write on an *empty* rules file, from
 *                        buf[strlen(buf) - 4] underflowing size_t
 *   GHSA-27v4-xhfx-g2rx  heap overflow from a negative fragment size, which
 *                        passed the "multiple of 8" check because -8 % 8 == 0
 *   GHSA-m655-53p4-6qm8  off-by-one in ip_chaff
 *   GHSA-v8c4-9w98-9v6v  the same off-by-one in tcp_chaff
 *
 * Every one of those is reachable from an attacker-influenced rules file and
 * needs no crafted packet at all. The pattern is unmistakable: this parser
 * accepts text, and nobody had ever fed it anything but valid text.
 *
 * The target parses the rules and then runs a fixed, well-formed packet
 * through the resulting module chain, because several of the bugs above are
 * in the modules' apply functions rather than the parser - they need the
 * chain to actually execute.
 */

#include "fuzz_common.h"

#include "defines.h"
#include "fragroute/fragroute.h"

#include <pcap.h>

/* A minimal, valid IPv4/UDP frame. Deliberately fixed: the rules file is the
 * variable under test here, and the pcap target covers packet shapes. */
static const uint8_t template_packet[] = {
        /* Ethernet: broadcast dst, locally-administered src, IPv4 */
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00,
        /* IPv4: 20-byte header, total length 0x002e (46), UDP, TTL 1 */
        0x45, 0x00, 0x00, 0x2e, 0x00, 0x01, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x00, 0x02,
        /* UDP: sport 1234, dport 5678, length 26 */
        0x04, 0xd2, 0x16, 0x2e, 0x00, 0x1a, 0x00, 0x00,
        /* payload */
        0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41};

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char path[FUZZ_PATH_MAX];
    char errbuf[FRAGROUTE_ERRBUF_LEN];
    fragroute_t *ctx;
    uint8_t packet[sizeof(template_packet)];

    /*
     * Cap the input. The parser is line-oriented and the interesting
     * behaviour - directive count, argument shape - is all reachable well
     * under this; beyond it the fuzzer is just making bigger files.
     */
    if (size > 64 * 1024)
        return 0;

    fuzz_quiet_stdout(); /* the "print" module dumps every packet */

    /* fragroute_init() reads a path, as tcprewrite --fragroute does */
    if (fuzz_write_tempfile(data, size, path, sizeof(path)) != 0)
        return 0;

    ctx = fragroute_init(1500, DLT_EN10MB, path, errbuf);
    unlink(path);

    if (ctx == NULL)
        return 0; /* rejected the rules, which is a valid outcome */

    /*
     * Run a packet through the chain. Several of the advisories above are in
     * the modules' apply paths - ip_chaff's off-by-one fires on any ordinary
     * IP packet - so parsing alone would miss them.
     *
     * fragroute_process() writes through its buffer, so hand it a copy.
     */
    memcpy(packet, template_packet, sizeof(packet));
    if (fragroute_process(ctx, packet, sizeof(packet)) >= 0) {
        /*
         * fragroute_getfragment() memcpy's into a buffer the *caller* owns -
         * it takes char** but only reads the pointer, it does not allocate.
         * tcprewrite.c does the same with a MAXPACKET buffer; passing an
         * uninitialised pointer here segfaults inside the library, which is
         * the harness's fault rather than a finding.
         */
        char *frag = malloc(MAXPACKET);

        if (frag != NULL) {
            while (fragroute_getfragment(ctx, &frag) > 0)
                ;
            free(frag);
        }
    }

    fragroute_close(ctx);
    return 0;
}
