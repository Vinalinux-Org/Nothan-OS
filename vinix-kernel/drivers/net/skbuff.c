/*
 * drivers/net/skbuff.c — socket buffer implementation
 *
 * alloc_skb allocates a contiguous buffer; head/data/tail/end pointers
 * track payload boundaries. Drivers use skb_put/skb_push/skb_pull/skb_reserve
 * to build and consume frames without pointer arithmetic at call sites.
 */

#include "vinix/skbuff.h"
#include "vinix/errno.h"
#include "slab.h"
#include "string.h"

struct sk_buff *alloc_skb(unsigned int size)
{
    struct sk_buff *skb = kmalloc(sizeof(*skb), GFP_ATOMIC);
    if (!skb) return 0;

    unsigned char *buf = kmalloc(size, GFP_ATOMIC);
    if (!buf) {
        kfree(skb);
        return 0;
    }

    skb->head = buf;
    skb->data = buf;
    skb->tail = buf;
    skb->end  = buf + size;
    skb->len  = 0;
    skb->dev  = 0;
    skb->next = 0;
    return skb;
}

void kfree_skb(struct sk_buff *skb)
{
    if (!skb) return;
    kfree(skb->head);
    kfree(skb);
}

void skb_reserve(struct sk_buff *skb, int len)
{
    skb->data += len;
    skb->tail += len;
}

unsigned char *skb_put(struct sk_buff *skb, unsigned int len)
{
    unsigned char *old_tail = skb->tail;
    skb->tail += len;
    skb->len  += len;
    return old_tail;
}

unsigned char *skb_push(struct sk_buff *skb, unsigned int len)
{
    skb->data -= len;
    skb->len  += len;
    return skb->data;
}

unsigned char *skb_pull(struct sk_buff *skb, unsigned int len)
{
    skb->data += len;
    skb->len  -= len;
    return skb->data;
}

int skb_headroom(const struct sk_buff *skb)
{
    return (int)(skb->data - skb->head);
}

int skb_tailroom(const struct sk_buff *skb)
{
    return (int)(skb->end - skb->tail);
}
