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
 * Unit tests for txring_mkreq() - the Linux TX_RING geometry calculation.
 *
 * This function decides tp_block_size/tp_frame_size/tp_block_nr/tp_frame_nr
 * from the interface MTU, and the result is handed straight to
 * setsockopt(PACKET_TX_RING).  It is pure arithmetic with no I/O, which makes
 * it exactly the kind of code worth unit testing - and it has been wrong
 * twice:
 *
 *   #1079  tp_frame_size was divided back down to a single page after the
 *          block had been grown to fit the MTU, so every frame larger than
 *          a page was truncated to 4096 bytes.  Jumbo replay was silently
 *          corrupt.
 *
 *   #1090  tp_frame_size was not rounded to TPACKET_ALIGNMENT, so the kernel
 *          rejected the ring outright with EINVAL for any MTU <= ~1365 -
 *          including 1280, the IPv6 minimum MTU.
 *
 * Neither was caught by the existing suite, because every test there needs a
 * built binary, root, and a live NIC, and none of them replay jumbo frames or
 * run against a small-MTU interface.
 *
 * The assertions below are not arbitrary - each mirrors a check the kernel
 * itself performs in packet_set_ring() (net/packet/af_packet.c).  If one
 * fails here, setsockopt() would have failed on a real socket.
 */

#include "config.h"
#include "defines.h"

#include "tap.h"

#ifndef HAVE_TX_RING

int
main(void)
{
    return tap_skip_all("TX_RING support not compiled in (Linux only)");
}

#else /* HAVE_TX_RING */

#include "common/txring.h"
#include <unistd.h>

void txring_mkreq(struct tpacket_req *treq, unsigned int mtu);

/* tdata_offset is where the frame's payload starts, past the tpacket header */
extern int tdata_offset;

/*
 * Every MTU worth worrying about: the IPv4 and IPv6 minimums, the usual
 * Ethernet 1500, tunnel-ish sizes, the page-boundary cases either side of
 * 4096, the common jumbo sizes, and the 16-bit ceiling.
 */
static const unsigned int mtus[] =
        {68, 128, 296, 576, 1280, 1300, 1400, 1492, 1500, 2000, 4000, 4095, 4096, 4097, 8192, 9000, 9216, 16000, 65535};

static void
check_one_mtu(unsigned int mtu, unsigned int pagesize)
{
    struct tpacket_req treq;
    unsigned int need = mtu + TPACKET_HDRLEN;

    txring_mkreq(&treq, mtu);

    /*
     * #1079: a frame has to be able to hold an MTU-sized packet plus the
     * tpacket header, or the packet is truncated on the way out.  This is
     * the assertion that fails on the pre-#1079 code for mtu >= ~4045.
     */
    tap_ok(treq.tp_frame_size >= need,
           "mtu %u: frame_size %u >= mtu + TPACKET_HDRLEN (%u)",
           mtu,
           treq.tp_frame_size,
           need);

    /*
     * #1090, and a hard kernel requirement:
     *     if (unlikely(req->tp_frame_size & (TPACKET_ALIGNMENT - 1)))
     *             goto out;
     * An unaligned frame_size is not a performance question, it is EINVAL.
     */
    tap_ok(treq.tp_frame_size % TPACKET_ALIGNMENT == 0,
           "mtu %u: frame_size %u is TPACKET_ALIGNMENT(%d)-aligned",
           mtu,
           treq.tp_frame_size,
           TPACKET_ALIGNMENT);

    /* kernel: block_size must be a multiple of the page size */
    tap_ok(treq.tp_block_size % pagesize == 0,
           "mtu %u: block_size %u is a multiple of pagesize %u",
           mtu,
           treq.tp_block_size,
           pagesize);

    /* kernel: a frame has to fit inside a block */
    tap_ok(treq.tp_block_size >= treq.tp_frame_size,
           "mtu %u: block_size %u >= frame_size %u",
           mtu,
           treq.tp_block_size,
           treq.tp_frame_size);

    /* kernel: the ring must hold at least one frame */
    tap_ok(treq.tp_frame_nr > 0 && treq.tp_block_nr > 0,
           "mtu %u: frame_nr %u and block_nr %u are both non-zero",
           mtu,
           treq.tp_frame_nr,
           treq.tp_block_nr);

    /*
     * The frames have to actually fit in the memory the blocks describe.
     * Over-promising here means txring_put() walks off the end of the
     * mapping.
     */
    tap_ok((uint64_t)treq.tp_frame_size * treq.tp_frame_nr <=
                   (uint64_t)treq.tp_block_size * treq.tp_block_nr,
           "mtu %u: frames (%u x %u) fit within blocks (%u x %u)",
           mtu,
           treq.tp_frame_size,
           treq.tp_frame_nr,
           treq.tp_block_size,
           treq.tp_block_nr);

    /*
     * txring_put() clamps the copy to tp_frame_size - tdata_offset.  If that
     * is under the MTU the packet is truncated - which is #1079 seen from the
     * other side, and the bound that used to be computed without tdata_offset
     * at all (it overran the following frame's header).
     */
    tap_ok((unsigned int)((int)treq.tp_frame_size - tdata_offset) >= mtu,
           "mtu %u: payload area %d >= mtu",
           mtu,
           (int)treq.tp_frame_size - tdata_offset);
}

int
main(void)
{
    unsigned int pagesize = (unsigned int)getpagesize();
    unsigned int i;
    unsigned int n = sizeof(mtus) / sizeof(mtus[0]);

    /* 7 assertions per MTU */
    tap_plan(n * 7);
    tap_diag("pagesize=%u TPACKET_HDRLEN=%u TPACKET_ALIGNMENT=%d tdata_offset=%d",
             pagesize,
             (unsigned int)TPACKET_HDRLEN,
             TPACKET_ALIGNMENT,
             tdata_offset);

    for (i = 0; i < n; i++)
        check_one_mtu(mtus[i], pagesize);

    return tap_done();
}

#endif /* HAVE_TX_RING */
