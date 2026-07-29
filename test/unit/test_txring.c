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
#include "common/utils.h"
#include <errno.h>
#include <string.h>
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

/*
 * txring_put()/txring_drain() (#1096) - the state machine over tp_status that
 * #1078 lived in: frames filled out of order left a hole the kernel's own
 * walk stopped at, stranding every frame past it, which txring_put() had
 * already reported as sent.
 *
 * Neither function does any I/O of its own beyond the ring memory - the
 * mmap(), setsockopt() and poll thread all live in txring_init()/
 * txring_close(), out of scope here. That makes a malloc'd buffer standing in
 * for the mmap'd ring a faithful stand-in: txring_put() only ever reads and
 * writes tx_head at tp_frame_size*i, exactly as it would through a real
 * mapping. txring_drain() does reach for txp->fd, but only once pending
 * frames remain after checking tp_status; fd is set to -1 below so that
 * sendto() fails immediately with EBADF - a "kernel" that never picks
 * anything up - rather than the tests waiting out the real 2-second drain
 * timeout.
 */

static struct tpacket_hdr *
frame_header(txring_t *txp, unsigned int i)
{
    return (struct tpacket_hdr *)((void *)txp->tx_head + ((size_t)txp->treq->tp_frame_size * i));
}

static char *
frame_data(txring_t *txp, unsigned int i)
{
    return ((char *)frame_header(txp, i)) + tdata_offset;
}

static void
make_fake_ring(txring_t *txp, struct tpacket_req *treq, unsigned int frame_size, unsigned int frame_nr)
{
    unsigned int i;

    memset(treq, 0, sizeof(*treq));
    treq->tp_frame_size = frame_size;
    treq->tp_frame_nr = frame_nr;
    /* block geometry itself is txring_mkreq()'s job, tested above; one frame
     * per "block" here is enough to give txring_put()/txring_drain() a ring
     * to walk */
    treq->tp_block_size = frame_size;
    treq->tp_block_nr = frame_nr;

    txp->treq = treq;
    txp->tx_head = (volatile struct tpacket_hdr *)safe_malloc((size_t)frame_size * frame_nr);
    memset((void *)txp->tx_head, 0, (size_t)frame_size * frame_nr);
    txp->tx_index = 0;
    txp->fd = -1; /* see comment above: txring_put() never touches fd at all */
    txp->shutdown_flag = 0;

    for (i = 0; i < frame_nr; i++)
        frame_header(txp, i)->tp_status = TP_STATUS_AVAILABLE;
}

static void
free_fake_ring(txring_t *txp)
{
    safe_free((void *)txp->tx_head);
}

static void
test_put_fills_in_order_and_wraps(void)
{
    txring_t txp;
    struct tpacket_req treq;
    const unsigned int frame_nr = 4;
    const unsigned int frame_size = (unsigned int)tdata_offset + 64;
    static const char *const payloads[] = {"first", "second", "third", "fourth"};
    unsigned int i;

    make_fake_ring(&txp, &treq, frame_size, frame_nr);

    for (i = 0; i < frame_nr; i++) {
        size_t len = strlen(payloads[i]) + 1;
        int ret = txring_put(&txp, payloads[i], len);

        tap_ok(ret == (int)len, "put %u: returns queued length %zu", i, len);
        tap_ok(frame_header(&txp, i)->tp_status == TP_STATUS_SEND_REQUEST,
               "put %u: frame marked TP_STATUS_SEND_REQUEST",
               i);
        tap_ok(frame_header(&txp, i)->tp_len == len, "put %u: tp_len set to %zu", i, len);
        tap_ok(memcmp(frame_data(&txp, i), payloads[i], len) == 0, "put %u: payload copied intact", i);
    }

    /* frames were claimed 0,1,2,3 in that order - never a hole (#1078) */
    tap_ok(txp.tx_index == 0, "tx_index wraps back to 0 after filling all %u frames", frame_nr);

    free_fake_ring(&txp);
}

static void
test_put_full_ring_returns_enobufs(void)
{
    txring_t txp;
    struct tpacket_req treq;
    const unsigned int frame_nr = 3;
    const unsigned int frame_size = (unsigned int)tdata_offset + 32;
    unsigned int i;
    int ret;

    make_fake_ring(&txp, &treq, frame_size, frame_nr);

    /* fill every frame; nothing ever hands one back, so the ring is genuinely full */
    for (i = 0; i < frame_nr; i++)
        tap_ok(txring_put(&txp, "x", 1) == 1, "prefill %u/%u: queued", i + 1, frame_nr);

    errno = 0;
    ret = txring_put(&txp, "x", 1);
    tap_ok(ret == -1, "put on a full ring returns -1 rather than overwriting a pending frame");
    tap_ok(errno == ENOBUFS, "put on a full ring sets errno to ENOBUFS");

    free_fake_ring(&txp);
}

static void
test_put_refuses_oversized_packet(void)
{
    txring_t txp;
    struct tpacket_req treq;
    const unsigned int frame_size = (unsigned int)tdata_offset + 8; /* payload area == 8 */
    char oversized[20];
    int ret;

    make_fake_ring(&txp, &treq, frame_size, 1);
    memset(oversized, 'A', sizeof(oversized));

    errno = 0;
    ret = txring_put(&txp, oversized, sizeof(oversized));

    /*
     * put() used to clamp length and queue the truncated result as if it
     * were the whole packet - the kernel would then genuinely transmit a
     * corrupted frame while the caller's accounting called it a failure
     * (#1108). Refusing outright means "failed" is no longer a lie: nothing
     * touches the ring, and the frame the kernel never gets to see can't
     * reach the wire.
     */
    tap_ok(ret == -1, "put refuses a packet bigger than the frame's payload area (8) rather than truncating it");
    tap_ok(errno == EMSGSIZE, "put sets errno to EMSGSIZE on refusal");
    tap_ok(frame_header(&txp, 0)->tp_status == TP_STATUS_AVAILABLE,
           "the frame is untouched - still available, not queued with truncated data");

    free_fake_ring(&txp);
}

static void
test_put_reclaims_wrong_format_frame(void)
{
    txring_t txp;
    struct tpacket_req treq;
    const unsigned int frame_size = (unsigned int)tdata_offset + 16;
    int ret;

    make_fake_ring(&txp, &treq, frame_size, 2);
    frame_header(&txp, 0)->tp_status = TP_STATUS_WRONG_FORMAT;

    ret = txring_put(&txp, "hi", 2);

    tap_ok(ret == 2, "put reclaims a TP_STATUS_WRONG_FORMAT frame instead of spinning on it forever");
    tap_ok(frame_header(&txp, 0)->tp_status == TP_STATUS_SEND_REQUEST,
           "the reclaimed frame ends up SEND_REQUEST like any other");

    free_fake_ring(&txp);
}

static void
test_drain_nothing_pending(void)
{
    txring_t txp;
    struct tpacket_req treq;
    COUNTER bytes = 999;
    unsigned int pending;

    make_fake_ring(&txp, &treq, (unsigned int)tdata_offset + 16, 4);

    pending = txring_drain(&txp, &bytes);

    tap_ok(pending == 0, "drain of an idle ring reports 0 frames pending");
    tap_ok(bytes == 0, "drain of an idle ring reports 0 bytes pending");

    free_fake_ring(&txp);
}

static void
test_drain_counts_pending_not_rejected(void)
{
    txring_t txp;
    struct tpacket_req treq;
    COUNTER bytes = 0;
    unsigned int pending;

    make_fake_ring(&txp, &treq, (unsigned int)tdata_offset + 16, 4);

    frame_header(&txp, 0)->tp_status = TP_STATUS_SEND_REQUEST; /* queued, not yet picked up */
    frame_header(&txp, 0)->tp_len = 40;
    frame_header(&txp, 1)->tp_status = TP_STATUS_SENDING; /* the driver has it */
    frame_header(&txp, 1)->tp_len = 60;
    frame_header(&txp, 2)->tp_status = TP_STATUS_WRONG_FORMAT; /* never going out - not pending */
    frame_header(&txp, 2)->tp_len = 9999;
    /* frame 3 stays TP_STATUS_AVAILABLE: already sent, or never used */

    pending = txring_drain(&txp, &bytes);

    tap_ok(pending == 2, "drain counts SEND_REQUEST and SENDING, skipping AVAILABLE and WRONG_FORMAT (got %u)", pending);
    tap_ok(bytes == 100, "drain sums tp_len across only the pending frames (got " COUNTER_SPEC ")", bytes);

    free_fake_ring(&txp);
}

int
main(void)
{
    unsigned int pagesize = (unsigned int)getpagesize();
    unsigned int i;
    unsigned int n = sizeof(mtus) / sizeof(mtus[0]);

    /* 7 assertions per MTU, plus the fixed counts in each txring_put()/
     * txring_drain() test below */
    tap_plan(n * 7 + 17 + 5 + 3 + 2 + 2 + 2);
    tap_diag("pagesize=%u TPACKET_HDRLEN=%u TPACKET_ALIGNMENT=%d tdata_offset=%d",
             pagesize,
             (unsigned int)TPACKET_HDRLEN,
             TPACKET_ALIGNMENT,
             tdata_offset);

    for (i = 0; i < n; i++)
        check_one_mtu(mtus[i], pagesize);

    test_put_fills_in_order_and_wraps();
    test_put_full_ring_returns_enobufs();
    test_put_refuses_oversized_packet();
    test_put_reclaims_wrong_format_frame();
    test_drain_nothing_pending();
    test_drain_counts_pending_not_rejected();

    return tap_done();
}

#endif /* HAVE_TX_RING */
