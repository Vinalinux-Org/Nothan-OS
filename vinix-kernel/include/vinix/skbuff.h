/*
 * include/vinix/skbuff.h — Socket buffer (skbuff) interface
 */

#ifndef VINIX_SKBUFF_H
#define VINIX_SKBUFF_H

#include "types.h"

struct net_device;

struct sk_buff {
    unsigned char     *head;        /* start of allocated buffer */
    unsigned char     *data;        /* current payload start */
    unsigned char     *tail;        /* current payload end */
    unsigned char     *end;         /* end of allocated buffer */

    unsigned int       len;         /* payload length (tail - data) */
    struct net_device *dev;         /* ingress / egress device */
    uint16_t           protocol;    /* L3 protocol — eth_type_trans sets */

    struct sk_buff    *next;        /* for queueing */
};

/* Allocate skb with `size` bytes payload capacity. */
struct sk_buff *alloc_skb(unsigned int size);

/* Free skb + payload buffer. */
void kfree_skb(struct sk_buff *skb);

/* Reserve headroom — advance data pointer by `len`. Used
 * before skb_put to leave space for headers prepended later
 * via skb_push. */
void skb_reserve(struct sk_buff *skb, int len);

/* Extend tail by `len`, returns OLD tail — caller fills bytes. */
unsigned char *skb_put(struct sk_buff *skb, unsigned int len);

/* Prepend `len` bytes to data — returns NEW data ptr. Used
 * to build headers from outermost in. */
unsigned char *skb_push(struct sk_buff *skb, unsigned int len);

/* Strip `len` bytes from data front. Returns NEW data ptr. */
unsigned char *skb_pull(struct sk_buff *skb, unsigned int len);

/* Available headroom / tailroom for callers building packets. */
int skb_headroom(const struct sk_buff *skb);
int skb_tailroom(const struct sk_buff *skb);

#endif /* VINIX_SKBUFF_H */
