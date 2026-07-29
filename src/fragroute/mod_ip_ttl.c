/*
 * mod_ip_ttl.c
 *
 * Copyright (c) 2001 Dug Song <dugsong@monkey.org>
 *
 * $Id$
 */

#include "config.h"
#include "argv.h"
#include "mod.h"
#include "pkt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ip_ttl_data {
    int ttl;
};

void *
ip_ttl_close(void *d)
{
    if (d != NULL)
        free(d);
    return (NULL);
}

void *
ip_ttl_open(int argc, char *argv[])
{
    struct ip_ttl_data *data;

    if (argc != 2)
        return (NULL);

    if ((data = calloc(1, sizeof(*data))) == NULL)
        return (NULL);

    if ((data->ttl = strtol(argv[1], NULL, 10)) <= 0 || data->ttl > 255)
        return (ip_ttl_close(data));

    return (data);
}

int
ip_ttl_apply(void *d, struct pktq *pktq)
{
    struct ip_ttl_data *data = (struct ip_ttl_data *)d;
    struct pkt *pkt;
    int ttldec;

    TAILQ_FOREACH(pkt, pktq, pkt_next)
    {
        uint16_t eth_type = htons(pkt->pkt_eth->eth_type);

        if (eth_type == ETH_TYPE_IP) {
            int ttlshift;

            ttldec = pkt->pkt_ip->ip_ttl - data->ttl;
            pkt->pkt_ip->ip_ttl = data->ttl;

            /*
             * ttldec is negative whenever the packet's TTL is below the
             * configured one, and shifting a negative value left is undefined
             * behaviour:
             *
             *   mod_ip_ttl.c:61: runtime error: left shift of negative value -4
             *
             * The existing "ip_ttl 5" test fires this on every run against a
             * capture whose packets have a lower TTL - it had simply never been
             * run under UBSan (#1100).
             *
             * Do the shift in unsigned, where it is defined, and convert back.
             * This is deliberately not a rewrite of the checksum adjustment:
             * the result is bit-identical to the old expression for every
             * ttldec in -255..255, so the golden fixtures that encode current
             * behaviour stay valid.
             */
            ttlshift = (int)((unsigned int)ttldec << 8);

            if (pkt->pkt_ip->ip_sum >= htons(0xffff - ttlshift))
                pkt->pkt_ip->ip_sum += htons(ttlshift) + 1;
            else
                pkt->pkt_ip->ip_sum += htons(ttlshift);
        } else if (eth_type == ETH_TYPE_IPV6) {
            pkt->pkt_ip6->ip6_hlim = data->ttl;
        }
    }
    return (0);
}

struct mod mod_ip_ttl = {
        "ip_ttl",       /* name */
        "ip_ttl <ttl>", /* usage */
        ip_ttl_open,    /* open */
        ip_ttl_apply,   /* apply */
        ip_ttl_close    /* close */
};
