/*
 * drivers/net/vnet.c — ARP responder + ICMP echo reply for 192.168.1.100
 */

#include "vinix/netdevice.h"
#include "vinix/skbuff.h"
#include "uart.h"
#include "string.h"

static const unsigned char bbb_ip[4] = {192, 168, 1, 100};

#define ETH_HDR  14
#define ARP_LEN  28
#define IP_HDR   20

static uint16_t net_checksum(const unsigned char *buf, unsigned int len)
{
    uint32_t s = 0;
    for (; len > 1; len -= 2, buf += 2)
        s += ((uint32_t)buf[0] << 8) | buf[1];
    if (len)
        s += (uint32_t)buf[0] << 8;
    while (s >> 16)
        s = (s & 0xFFFF) + (s >> 16);
    return (uint16_t)(~s);
}

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

static void icmp_reply(struct sk_buff *rxskb)
{
    unsigned char *rx      = rxskb->data;
    unsigned char *ip      = rx + ETH_HDR;
    unsigned int   ihl     = (ip[0] & 0xF) * 4;
    unsigned char *icmp    = ip + ihl;
    unsigned int   icmp_len = rxskb->len - ETH_HDR - ihl;
    struct sk_buff *tx;
    unsigned char *p, *tip, *tic;
    uint16_t csum, total;

    if (ip[9] != 1)                       return; /* not ICMP    */
    if (memcmp(ip + 16, bbb_ip, 4) != 0) return; /* not our IP  */
    if (icmp[0] != 8)                     return; /* not request */

    tx = alloc_skb(ETH_HDR + IP_HDR + icmp_len);
    if (!tx) return;
    p = skb_put(tx, ETH_HDR + IP_HDR + icmp_len);

    memcpy(p,     rx + 6, 6); /* eth dst = laptop MAC */
    memcpy(p + 6, rx,     6); /* eth src = our MAC    */
    p[12] = 0x08; p[13] = 0x00;

    tip    = p + ETH_HDR;
    total  = (uint16_t)(IP_HDR + icmp_len);
    tip[0] = 0x45;
    tip[1] = 0;
    tip[2] = total >> 8;  tip[3]  = total & 0xFF;
    tip[4] = ip[4];       tip[5]  = ip[5];
    tip[6] = 0;           tip[7]  = 0;
    tip[8] = 64;
    tip[9] = 1;
    tip[10] = 0;          tip[11] = 0;
    memcpy(tip + 12, ip + 16, 4);           /* src = our IP    */
    memcpy(tip + 16, ip + 12, 4);           /* dst = laptop IP */
    csum    = net_checksum(tip, IP_HDR);
    tip[10] = csum >> 8;  tip[11] = csum & 0xFF;

    tic = tip + IP_HDR;
    memcpy(tic, icmp, icmp_len);
    tic[0] = 0;
    tic[2] = 0; tic[3] = 0;
    csum   = net_checksum(tic, icmp_len);
    tic[2] = csum >> 8;   tic[3] = csum & 0xFF;

    tx->dev = rxskb->dev;
    pr_info("[VNET] ICMP echo reply\n");
    rxskb->dev->netdev_ops->ndo_start_xmit(tx, rxskb->dev);
}

void vnet_rx(struct sk_buff *skb)
{
    unsigned char *p = skb->data;
    uint16_t etype   = ((uint16_t)p[12] << 8) | p[13];

    if (etype == 0x0806)
        arp_reply(skb);
    else if (etype == 0x0800)
        icmp_reply(skb);

    kfree_skb(skb);
}
