/* $Id$ */

/* Copyright (c) 2010 Dmitriy Gerasimov <gesser@demlabs.ru>
 * Copyright (c) 2010 Aaron Turner.
 * Copyright (c) 2023 Fred Klassen - AppNet Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright owners nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "defines.h"
#include "config.h"

#ifdef HAVE_TX_RING

#include "err.h"
#include "txring.h"
#include "utils.h"
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

/* how long to wait for the TX ring to drain before giving up (#1078) */
#define TXRING_DRAIN_TIMEOUT_SEC 2

int tdata_offset = TPACKET_HDRLEN - sizeof(struct sockaddr_ll);

/**
 * This task will call send() procedure
 */
void *
txring_send(void *arg)
{
    int ec_send;
    static int total = 0;
    txring_t *txp = (txring_t *)arg;

    do {
        /* send all buffers with TP_STATUS_SEND_REQUEST */
        ec_send = sendto(txp->fd, NULL, 0, MSG_DONTWAIT, (struct sockaddr *)NULL, sizeof(struct sockaddr_ll));

        if (ec_send > 0) {
            total += ec_send;
            dbgx(2, "Sent %d bytes (+%d bytes)", total, ec_send);
        } else {
            /* nothing to do => schedule : useful if no SMP */
            usleep(100);
        }

    } while (!txp->shutdown_flag);

    // if(blocking) printf("end of task send()\n");
    // printf("end of task send(ec=%x)\n", ec_send);

    return (void *)(intptr_t)ec_send;
}

/**
 * Put data in the next TX ring frame, waiting for it if the kernel still owns it
 *
 * The ring is a strict FIFO in both directions: tpacket_snd() walks it in order
 * and stops at the first frame that is not TP_STATUS_SEND_REQUEST.  So we have
 * to fill it in order too.  This used to hunt forward for any frame that
 * happened to be free, which left a hole at every frame the kernel was still
 * working on - the kernel then stopped at that hole and every frame past it was
 * stranded, discarded unsent at teardown after txring_put() had already
 * reported it sent (#1078).  Filling out of order also put the packets on the
 * wire out of order.
 *
 * Returns the queued length, or -1 with errno set to ENOBUFS if the ring stayed
 * full, or EMSGSIZE if the packet doesn't fit this MTU's frame size - the
 * caller (sendpacket()) counts either as a failure.
 */
int
txring_put(txring_t *txp, const void *data, size_t length)
{
    /* the frame's payload starts after the header, so that's all it can hold */
    const size_t max_len = (size_t)txp->treq->tp_frame_size - tdata_offset;
    struct tpacket_hdr *ps_header;
    char *to_data;
    unsigned int spins;

    if (length > max_len) {
        /*
         * Refuse rather than truncate-and-send-anyway (#1108). This used to
         * copy in only the first max_len bytes and queue that as the whole
         * packet - the kernel doesn't know or care that it's short, so the
         * corrupted frame reached the wire, while the caller's own
         * accounting (send_packets.c) counted it a failure on the strength
         * of this function's truncated return value. "0 successful, N
         * failed" was true of the bookkeeping, not of what the interface
         * actually transmitted. Checked before touching the ring at all, so
         * a packet that can never fit doesn't cost it a slot.
         *
         * TODO: fragment instead of refusing outright, once something
         * upstream can reassemble it.
         */
        warnx("[!] %zu byte packet exceeds the %zu-byte frame this MTU allows - refusing to send it",
              length,
              max_len);
        errno = EMSGSIZE;
        return -1;
    }

    ps_header = ((struct tpacket_hdr *)((void *)txp->tx_head + (txp->treq->tp_frame_size * txp->tx_index)));
    to_data = ((void *)ps_header) + tdata_offset;

    /* wait for the kernel to hand this frame back */
    for (spins = 0; (volatile uint32_t)ps_header->tp_status != TP_STATUS_AVAILABLE; spins++) {
        if ((volatile uint32_t)ps_header->tp_status == TP_STATUS_WRONG_FORMAT) {
            /*
             * The kernel rejected this frame and, with PACKET_LOSS off, stopped
             * the ring on it.  Nobody else will ever clear it, so reclaim it
             * here rather than wedge every frame behind it.
             */
            warnx("TP_STATUS_WRONG_FORMAT on frame %u - reclaiming it", txp->tx_index);
            break;
        }

        if (spins >= txp->treq->tp_frame_nr) {
            dbgx(2, "TX ring full at frame %u", txp->tx_index);
            errno = ENOBUFS;
            return -1;
        }

        /* nothing to do => schedule : useful if no SMP */
        usleep(0);
    }

    memcpy(to_data, data, length);
    ps_header->tp_len = length;
    ps_header->tp_status = TP_STATUS_SEND_REQUEST;

    if (++txp->tx_index >= txp->treq->tp_frame_nr) {
        txp->tx_index = 0;
    }

    return (int)length;
}

/**
 * Count the frames the kernel still owes us: those userspace has asked to be
 * sent but that haven't been picked up yet (TP_STATUS_SEND_REQUEST), and those
 * the driver has taken but not yet completed (TP_STATUS_SENDING).  Frames the
 * kernel rejected (TP_STATUS_WRONG_FORMAT) are never going out, so they aren't
 * pending.  Their combined length is reported through "bytes" when non-NULL.
 */
static unsigned int
txring_pending(txring_t *txp, COUNTER *bytes)
{
    unsigned int i;
    unsigned int pending = 0;

    if (bytes != NULL) {
        *bytes = 0;
    }

    for (i = 0; i < txp->treq->tp_frame_nr; i++) {
        struct tpacket_hdr *ps_header =
                ((struct tpacket_hdr *)((void *)txp->tx_head + ((size_t)txp->treq->tp_frame_size * i)));
        uint32_t status = (volatile uint32_t)ps_header->tp_status;

        if (status == TP_STATUS_SEND_REQUEST || status == TP_STATUS_SENDING) {
            pending++;
            if (bytes != NULL) {
                *bytes += ps_header->tp_len;
            }
        }
    }

    return pending;
}

/**
 * \brief Wait for everything queued in the TX ring to reach the wire
 *
 * txring_put() reports a packet sent as soon as it is copied into the ring, so
 * without this the frames still queued when the ring is torn down are silently
 * discarded after having been counted as successful (#1078).  For short
 * replays that was the entire run.
 *
 * Returns the number of frames that could *not* be drained (0 on success), and
 * their combined length through "bytes" when non-NULL, so the caller can take
 * them back out of its statistics.
 */
unsigned int
txring_drain(txring_t *txp, COUNTER *bytes)
{
    struct timeval start, now;
    unsigned int pending;

    if (!txp) {
        return 0;
    }

    gettimeofday(&start, NULL);

    while ((pending = txring_pending(txp, bytes)) > 0) {
        /*
         * Kick the ring, handing every TP_STATUS_SEND_REQUEST frame to the
         * driver.  MSG_DONTWAIT rather than a blocking send so that the
         * timeout below stays enforceable: a blocking sendto() on a backed-up
         * device would hang the teardown rather than give up on it.
         */
        if (sendto(txp->fd, NULL, 0, MSG_DONTWAIT, (struct sockaddr *)NULL, sizeof(struct sockaddr_ll)) < 0 &&
            errno != EAGAIN && errno != ENOBUFS && errno != EINTR) {
            warnx("Unable to flush TX ring: %s", strerror(errno));
            break;
        }

        gettimeofday(&now, NULL);
        if ((now.tv_sec - start.tv_sec) * 1000000L + (now.tv_usec - start.tv_usec) >=
            TXRING_DRAIN_TIMEOUT_SEC * 1000000L) {
            warnx("TX ring still holding %u packets after %d seconds - giving up on them",
                  pending,
                  TXRING_DRAIN_TIMEOUT_SEC);
            break;
        }

        /* nothing to do => schedule : useful if no SMP */
        usleep(100);
    }

    return pending;
}

/**
 * \brief Build TX ring buffer request structure
 *
 * This builds a ring buffer request structure making sure
 * that we have buffers big enough so that a frame which
 * is the size of the MTU doesn't get truncated. We also
 * need to structure things with minimum memory wastage
 */
void
txring_mkreq(struct tpacket_req *treq, unsigned int mtu)
{
    unsigned int pg, bs;
    unsigned int s;
    unsigned int mult = 1;
    unsigned nr_blocks = 1000;

    bs = pg = getpagesize();
    s = mtu + TPACKET_HDRLEN;

    memset(treq, 0, sizeof(struct tpacket_req));
    if (bs <= s) {
        /*
         * One frame needs more than a page: grow the block by whole pages
         * until the frame fits, and give the whole block to that one frame.
         *
         * "mult" counts pages-per-frame here, not frames-per-block - it only
         * means the latter in the small-MTU branch below. Dividing by it set
         * tp_frame_size straight back to a single page, so a 9000-byte MTU
         * produced 4096-byte frames and every jumbo frame was truncated to
         * 4096 (#1079). Reported by @Steve-Tech, who also identified the cause.
         */
        while (bs < s) {
            bs += pg;
        }

        treq->tp_block_size = bs;
        treq->tp_frame_size = bs; /* one frame per block */
        treq->tp_block_nr = nr_blocks;
        treq->tp_frame_nr = nr_blocks;
    } else {
        /*
         * Several frames fit in a page.  The frame size has to be a multiple
         * of TPACKET_ALIGNMENT - the kernel rejects the ring outright
         * otherwise:
         *
         *     if (unlikely(req->tp_frame_size & (TPACKET_ALIGNMENT - 1)))
         *             goto out;              -- net/packet/af_packet.c
         *
         * This used to pack as many frames into the page as would fit and
         * then divide, which lands on an aligned size only by luck: a
         * 1280-byte MTU (the IPv6 minimum) gave 4096/3 = 1365, and
         * setsockopt(PACKET_TX_RING) failed with EINVAL, so TX_RING was
         * unusable on any small-MTU interface (#1090).
         *
         * Round the requirement up to the alignment first, then fit as many
         * of those as the page holds. Rounding the division's result *down*
         * instead would be wrong - it can fall back under the MTU (a 61-byte
         * MTU wants 113 bytes and would get 112).
         */
        /*
         * Pack as many frames into the page as will fit, then round the frame
         * size *down* to TPACKET_ALIGNMENT - the kernel rejects an unaligned
         * frame size outright:
         *
         *     if (unlikely(req->tp_frame_size & (TPACKET_ALIGNMENT - 1)))
         *             goto out;              -- net/packet/af_packet.c
         *
         * A 1280-byte MTU (the IPv6 minimum) used to give 4096/3 = 1365 and
         * setsockopt(PACKET_TX_RING) failed with EINVAL, so TX_RING could not
         * be used on a small-MTU interface at all (#1090).
         *
         * Round down, not up. Sizing the frame to exactly TPACKET_ALIGN(s)
         * looks tighter and is wrong: the kernel needs headroom beyond
         * mtu + TPACKET_HDRLEN for the link-layer reserve, and a frame that
         * merely satisfies that arithmetic silently fails to send - a 1500-MTU
         * interface delivered 2 packets out of 179 (#1094). Keeping the
         * original generous packing avoids that, so MTUs that already worked
         * keep exactly the geometry they had.
         *
         * Rounding down can drop below s on very small MTUs (61 bytes needs
         * 113 and would get 112), so give back a frame when it does.
         */
        while ((s * (mult + 1)) <= pg) {
            mult++;
        }

        treq->tp_frame_size = (pg / mult) & ~(TPACKET_ALIGNMENT - 1);
        while (treq->tp_frame_size < s && mult > 1) {
            mult--;
            treq->tp_frame_size = (pg / mult) & ~(TPACKET_ALIGNMENT - 1);
        }

        treq->tp_block_size = pg;
        treq->tp_block_nr = nr_blocks;
        /* must equal the kernel's own frames_per_block * tp_block_nr */
        treq->tp_frame_nr = (pg / treq->tp_frame_size) * nr_blocks;
    }
    dbgx(1,
         "txring: block_size=%d block_nr=%d frame_size=%d frame_nr=%d",
         treq->tp_block_size,
         treq->tp_block_nr,
         treq->tp_frame_size,
         treq->tp_frame_nr);
}

/**
 * \brief Create TX ring for socket and init indexes
 *
 * Creates our pthread for sending, currently hardcoded for priority = 20
 */
txring_t *
txring_init(int fd, unsigned int mtu)
{
    pthread_attr_t t_attr_send;
    struct sched_param para_send;
    int mode_loss = 0;
    txring_t *txp;

    /* allocate memory for structure and fill it with different stuff*/
    txp = (txring_t *)safe_malloc(sizeof(txring_t));
    txp->treq = (struct tpacket_req *)safe_malloc(sizeof(struct tpacket_req));
    txp->fd = fd;
    txp->shutdown_flag = 0;

    txring_mkreq(txp->treq, mtu);
    txp->tx_size = txp->treq->tp_block_size * txp->treq->tp_block_nr;
    txp->tx_index = 0; /* Set index on start*/

    /* Set PACKET_LOSS sockoption */
    if (setsockopt(fd, SOL_PACKET, PACKET_LOSS, (char *)&mode_loss, sizeof(mode_loss)) < 0) {
        perror("setsockopt: PACKET_LOSS");
        safe_free(txp->treq);
        safe_free(txp);
        return NULL;
    }

    /* Enable TX Ring */
    if (setsockopt(fd, SOL_PACKET, PACKET_TX_RING, (char *)txp->treq, sizeof(struct tpacket_req)) < 0) {
        perror("Can't setsockopt PACKET_TX_RING");
        safe_free(txp->treq);
        safe_free(txp);
        return NULL;
    }

    /* mmap unswapped memory with TX ring buffer*/
    txp->tx_head = mmap(0, txp->tx_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (txp->tx_head == MAP_FAILED) {
        perror("mmap() failed ");
        safe_free(txp->treq);
        safe_free(txp);
        return NULL;
    }

    /* Start poll thread*/
    pthread_attr_init(&t_attr_send);
    pthread_attr_setschedpolicy(&t_attr_send, SCHED_RR);
    para_send.sched_priority = 20;
    pthread_attr_setschedparam(&t_attr_send, &para_send);

    if (pthread_create(&txp->tx_send, &t_attr_send, txring_send, (void *)txp) != 0) {
        perror("pthread_create() failed\n");
        abort();
    }

    /* the attr object is only needed to seed the new thread; safe to
     * release immediately once pthread_create() has returned (#1061) */
    pthread_attr_destroy(&t_attr_send);

    return txp;
}

/**
 * \brief Tear down a TX ring: stop the poll thread, disable and unmap the
 * ring buffer, and free the txring_t itself (#leak found under
 * AddressSanitizer)
 */
void
txring_close(txring_t *txp)
{
    struct tpacket_req disable_req;

    if (!txp)
        return;

    txp->shutdown_flag = 1;
    pthread_join(txp->tx_send, NULL);

    /*
     * Frames still queued at this point have already been reported to the
     * caller as sent, and disabling the ring below throws them away (#1078).
     * tcpreplay drains earlier, before it reads its statistics; this covers
     * every other teardown path.
     */
    txring_drain(txp, NULL);

    /* explicitly disable the ring before unmapping it (#1061) */
    memset(&disable_req, 0, sizeof(disable_req));
    setsockopt(txp->fd, SOL_PACKET, PACKET_TX_RING, (char *)&disable_req, sizeof(disable_req));

    if (txp->tx_head != NULL && txp->tx_head != MAP_FAILED)
        munmap((void *)txp->tx_head, txp->tx_size);

    safe_free(txp->treq);
    safe_free(txp);
}

#endif /* HAVE_TX_RING */
