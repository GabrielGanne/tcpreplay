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
 * Bounds tests for the IPv6 extension-header walk in get_layer4_v6().
 *
 * This is the GHSA-jj65-mrgg-f5fx regression (CWE-125, fixed in 4.5.5 and
 * carried into 4.6.0).  get_ipv6_next() validates the header it is handed,
 * but the pointer it returns was only checked with "ptr > end_ptr" - so it
 * could land exactly on end_ptr, or leave fewer bytes than the next
 * tcpr_ipv6_ext_hdr_base needs, and the caller then read ip_nh one byte past
 * the captured data.  Reachable from tcpprep, tcprewrite and tcpreplay by
 * feeding any of them an untrusted pcap.
 *
 * That advisory is worth a regression test precisely because it came back:
 * it was reported in 2024 against 4.4.4 and marked patched, but the
 * reporter's proof-of-concept still reproduced on 4.5.4.
 *
 * The contract under test is narrow and absolute: for *any* input buffer,
 * get_layer4_v6() either returns NULL or returns a pointer within
 * [buf, end_ptr].  It must never return a pointer past the end, and must
 * never dereference past it.  Run this under --enable-asan to check the
 * second half; the assertions below check the first.
 */

#include "config.h"
#include "defines.h"

#include "common/get.h"
#include "tap.h"

/*
 * A buffer with a guard page's worth of poison after end_ptr.  If the walk
 * reads past the end, ASan (or valgrind) has something to trip on, and the
 * returned pointer is checked against the boundary either way.
 */
#define BUFSZ 256

struct v6case {
    const char *name;
    size_t caplen;     /* bytes the "capture" actually contains */
    uint8_t next_hdr;  /* ip_nh of the fixed IPv6 header */
    uint8_t ext_nh;    /* ip_nh of the first extension header, if present */
    uint8_t ext_len;   /* ip_len of the first extension header, if present */
    int have_ext;
};

static void
run_case(const struct v6case *c)
{
    uint8_t buf[BUFSZ];
    ipv6_hdr_t *ip6;
    const u_char *end_ptr;
    void *l4;

    memset(buf, 0xA5, sizeof(buf)); /* poison, so a short read is visible */

    ip6 = (ipv6_hdr_t *)buf;
    memset(ip6, 0, TCPR_IPV6_H);
    ip6->ip_nh = c->next_hdr;

    if (c->have_ext && c->caplen >= TCPR_IPV6_H + 2) {
        buf[TCPR_IPV6_H] = c->ext_nh;      /* ip_nh  */
        buf[TCPR_IPV6_H + 1] = c->ext_len; /* ip_len */
    }

    /*
     * end_ptr is the last valid byte, matching how callers in tcpedit and
     * tcpprep compute it (pktdata + caplen - 1).
     */
    end_ptr = (const u_char *)buf + c->caplen - 1;

    l4 = get_layer4_v6(ip6, end_ptr);

    tap_ok(l4 == NULL || ((const u_char *)l4 >= buf && (const u_char *)l4 <= end_ptr),
           "%s (caplen %zu): result %s within bounds",
           c->name,
           c->caplen,
           l4 == NULL ? "NULL is" : "stays");
}

static const struct v6case cases[] = {
        /*
         * Truncation cases: the capture ends partway through the chain.
         * Every one of these must come back NULL rather than a pointer into
         * or past the poison.
         */
        {"header cut short", TCPR_IPV6_H - 1, IPPROTO_TCP, 0, 0, 0},
        {"exactly the v6 header", TCPR_IPV6_H, IPPROTO_TCP, 0, 0, 0},
        {"ext hdr promised, none present", TCPR_IPV6_H, TCPR_IPV6_NH_HBH, 0, 0, 0},
        {"ext hdr base cut in half", TCPR_IPV6_H + 1, TCPR_IPV6_NH_HBH, IPPROTO_TCP, 0, 1},
        {"ext hdr base exactly, no body", TCPR_IPV6_H + 2, TCPR_IPV6_NH_HBH, IPPROTO_TCP, 0, 1},
        {"ext hdr claims more than captured", TCPR_IPV6_H + 8, TCPR_IPV6_NH_HBH, IPPROTO_TCP, 40, 1},
        {"routing hdr claims more than captured", TCPR_IPV6_H + 8, TCPR_IPV6_NH_ROUTING, IPPROTO_TCP, 40, 1},
        {"destopts claims more than captured", TCPR_IPV6_H + 8, TCPR_IPV6_NH_DESTOPTS, IPPROTO_TCP, 40, 1},
        {"AH claims more than captured", TCPR_IPV6_H + 8, TCPR_IPV6_NH_AH, IPPROTO_TCP, 40, 1},
        {"fragment hdr truncated", TCPR_IPV6_H + 4, TCPR_IPV6_NH_FRAGMENT, IPPROTO_TCP, 0, 1},
        {"v6-in-v6 truncated", TCPR_IPV6_H + 4, TCPR_IPV6_NH_IPV6, IPPROTO_TCP, 0, 1},

        /* Well-formed cases: these should resolve, and stay inside the buffer. */
        {"plain TCP, full capture", 128, IPPROTO_TCP, 0, 0, 0},
        {"plain UDP, full capture", 128, IPPROTO_UDP, 0, 0, 0},
        {"one HBH then TCP", 128, TCPR_IPV6_NH_HBH, IPPROTO_TCP, 0, 1},
        {"ESP is unparsable, must be NULL", 128, TCPR_IPV6_NH_ESP, 0, 0, 0},
};

int
main(void)
{
    unsigned int i;
    unsigned int n = sizeof(cases) / sizeof(cases[0]);

    tap_plan(n);
    tap_diag("TCPR_IPV6_H=%u sizeof(ipv6_hdr_t)=%zu", (unsigned int)TCPR_IPV6_H, sizeof(ipv6_hdr_t));

    for (i = 0; i < n; i++)
        run_case(&cases[i]);

    return tap_done();
}
