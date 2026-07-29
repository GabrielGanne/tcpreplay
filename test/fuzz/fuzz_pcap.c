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
 * Fuzz target: the packet header parsers, driven from a whole pcap file.
 *
 * This is the surface that reads untrusted input in every tool - tcpprep,
 * tcprewrite, tcpreplay and tcpcapinfo all start by opening a capture
 * somebody else produced - and it is where most of the 2026 advisories
 * landed:
 *
 *   GHSA-jj65-mrgg-f5fx  heap over-read walking IPv6 extension headers
 *                        (get_layer4_v6); reported against 4.4.4, marked
 *                        patched, still reproducing on 4.5.4
 *   GHSA-5q26-7fxx-v8fh  heap OOB in ARP rewriting, from trusting the
 *                        packet's own ar_hln/ar_pln
 *   GHSA-m6w7-8497-g9c9  heap over-read via --pktlen, where the on-the-wire
 *                        length exceeds what was captured
 *   GHSA-ww62-mxv7-pg55  SEGV in the DLT_JUNIPER_ETHER decoder
 *
 * The fuzzer drives the pcap through libpcap exactly as the tools do, then
 * runs each packet through the shared header parsers. The DLT comes from the
 * file, so the input controls which decode path is taken and the whole DLT
 * plugin matrix is reachable from one target.
 *
 * The capture is fed through fmemopen() rather than a temp file: this runs
 * millions of times and the filesystem would dominate.
 */

#include "fuzz_common.h"

#include "common/get.h"
#include "defines.h"

#include <pcap.h>

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FILE *fp;
    pcap_t *pcap;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr *pkthdr;
    const u_char *pktdata;
    int datalink;
    int rc;
    unsigned int packets = 0;

    /* a pcap file header is 24 bytes; below that there is nothing to open */
    if (size < 24)
        return 0;

    /*
     * fmemopen wants a writable pointer but libpcap only reads. Casting away
     * const on the fuzzer's buffer is not on - copy it.
     */
    void *copy = malloc(size);
    if (copy == NULL)
        return 0;
    memcpy(copy, data, size);

    fp = fmemopen(copy, size, "rb");
    if (fp == NULL) {
        free(copy);
        return 0;
    }

    pcap = pcap_fopen_offline(fp, errbuf);
    if (pcap == NULL) {
        /* malformed header: libpcap rejected it, which is the correct outcome */
        fclose(fp);
        free(copy);
        return 0;
    }
    /* from here on the pcap_t owns fp and closes it in pcap_close() - closing
     * it again here is a double free, and it is the harness that would be
     * wrong, not libpcap */

    datalink = pcap_datalink(pcap);

    while ((rc = pcap_next_ex(pcap, &pkthdr, &pktdata)) == 1) {
        uint16_t ethertype;
        uint32_t l2len, l2offset, vlan_offset;

        /*
         * Cap the work per input. A crafted pcap can claim an enormous packet
         * count, and the fuzzer's time is better spent on new shapes than on
         * one pathological file.
         */
        if (++packets > 512)
            break;

        if (pkthdr->caplen == 0)
            continue;

        /*
         * The parsers take an end pointer computed from caplen. Deliberately
         * use caplen, not len: the gap between them is what GHSA-m6w7-8497-g9c9
         * was about, and a harness that papered over it would hide the bug
         * class it exists to find.
         */
        if (get_l2len_protocol(pktdata,
                               pkthdr->caplen,
                               datalink,
                               &ethertype,
                               &l2len,
                               &l2offset,
                               &vlan_offset) < 0)
            continue;

        if (l2len > pkthdr->caplen)
            continue;

        switch (ethertype) {
        case ETHERTYPE_IP: {
            ipv4_hdr_t *ip_hdr = (ipv4_hdr_t *)(pktdata + l2len);

            if (pkthdr->caplen < l2len + sizeof(ipv4_hdr_t))
                break;

            (void)get_layer4_v4(ip_hdr, pktdata + pkthdr->caplen - 1);
            break;
        }

        case ETHERTYPE_IP6: {
            ipv6_hdr_t *ip6_hdr = (ipv6_hdr_t *)(pktdata + l2len);

            if (pkthdr->caplen < l2len + sizeof(ipv6_hdr_t))
                break;

            /* GHSA-jj65-mrgg-f5fx */
            (void)get_layer4_v6(ip6_hdr, pktdata + pkthdr->caplen - 1);
            (void)get_ipv6_l4proto(ip6_hdr, pktdata + pkthdr->caplen - 1);
            break;
        }

        default:
            break;
        }
    }

    pcap_close(pcap);
    free(copy);
    return 0;
}
