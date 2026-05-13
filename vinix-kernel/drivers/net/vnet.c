/*
 * drivers/net/vnet.c — ARP responder + ICMP echo reply for 192.168.2.100
 */

#include "vinix/netdevice.h"
#include "vinix/skbuff.h"
#include "uart.h"
#include "string.h"
#include "ip.h"

static const unsigned char bbb_ip[4] = {192, 168, 2, 100};

#define ETH_HDR  14
#define ARP_LEN  28

static void arp_reply(struct sk_buff *rxskb)
{
    unsigned char *rx   = rxskb->data;
    unsigned char *rarp = rx + ETH_HDR;
    struct sk_buff *tx;
    unsigned char *p, *a;

    if (rarp[6] != 0x00 || rarp[7] != 0x01)         return; /* not request */
    if (memcmp(rarp + 24, bbb_ip, 4) != 0)           return; /* not our IP  */

    tx = alloc_skb(ETH_HDR + ARP_LEN);
    if (!tx) return;
    p = skb_put(tx, ETH_HDR + ARP_LEN);

    memcpy(p,     rx + 6,               6); /* eth dst = requester MAC */
    memcpy(p + 6, rxskb->dev->dev_addr, 6); /* eth src = our MAC       */
    p[12] = 0x08; p[13] = 0x06;             /* EtherType ARP           */

    a = p + ETH_HDR;
    a[0] = 0x00; a[1] = 0x01;               /* htype Ethernet          */
    a[2] = 0x08; a[3] = 0x00;               /* ptype IPv4              */
    a[4] = 6;    a[5] = 4;
    a[6] = 0x00; a[7] = 0x02;               /* oper reply              */
    memcpy(a +  8, rxskb->dev->dev_addr, 6); /* SHA = our MAC          */
    memcpy(a + 14, bbb_ip,               4); /* SPA = our IP           */
    memcpy(a + 18, rarp + 8,             6); /* THA = requester MAC    */
    memcpy(a + 22, rarp + 14,            4); /* TPA = requester IP     */

    tx->dev = rxskb->dev;
    pr_info("[VNET] ARP reply -> %02x:%02x:%02x:%02x:%02x:%02x\n",
            rx[6], rx[7], rx[8], rx[9], rx[10], rx[11]);
    rxskb->dev->netdev_ops->ndo_start_xmit(tx, rxskb->dev);
}

void vnet_rx(struct sk_buff *skb)
{
    unsigned char *p = skb->data;
    uint16_t etype   = ((uint16_t)p[12] << 8) | p[13];

    if (etype == 0x0806)
        arp_reply(skb);
    else if (etype == 0x0800)
        ip_rx(skb);

    kfree_skb(skb);
}
