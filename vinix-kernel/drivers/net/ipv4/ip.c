/*
 * drivers/net/ipv4/ip.c — IP layer: receive dispatch + transmit framing
 *
 * ip_rx: EtherType 0x0800 frames vào đây. Validates IPv4, dispatch theo proto.
 * ip_tx: ICMP/TCP gọi để wrap payload vào IP+ETH rồi transmit.
 */

#include "vinix/netdevice.h"
#include "vinix/skbuff.h"
#include "uart.h"
#include "string.h"

#define ETH_HDR  14
#define IP_HDR   20

static const unsigned char bbb_ip[4] = {192, 168, 1, 100};

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

extern void tcp_rx(struct sk_buff *skb, unsigned char *ip);

void ip_tx(struct sk_buff *skb, const unsigned char *dst_mac,
           const unsigned char *dst_ip, uint8_t proto)
{
    uint16_t total = (uint16_t)(skb->len + IP_HDR);
    uint16_t csum;
    unsigned char *iph, *eth;

    iph     = skb_push(skb, IP_HDR);
    iph[0]  = 0x45;
    iph[1]  = 0;
    iph[2]  = total >> 8;  iph[3]  = total & 0xFF;
    iph[4]  = 0;           iph[5]  = 0;
    iph[6]  = 0;           iph[7]  = 0;
    iph[8]  = 64;
    iph[9]  = proto;
    iph[10] = 0;           iph[11] = 0;
    memcpy(iph + 12, bbb_ip, 4);
    memcpy(iph + 16, dst_ip, 4);
    csum    = net_checksum(iph, IP_HDR);
    iph[10] = csum >> 8;   iph[11] = csum & 0xFF;

    eth = skb_push(skb, ETH_HDR);
    memcpy(eth,     dst_mac,            6);
    memcpy(eth + 6, skb->dev->dev_addr, 6);
    eth[12] = 0x08; eth[13] = 0x00;

    skb->dev->netdev_ops->ndo_start_xmit(skb, skb->dev);
}

static void icmp_rx(struct sk_buff *skb, unsigned char *ip)
{
    unsigned char *rx       = skb->data;
    unsigned int   ihl      = (ip[0] & 0xF) * 4;
    unsigned char *icmp     = ip + ihl;
    unsigned int   icmp_len = skb->len - ETH_HDR - ihl;
    struct sk_buff *tx;
    unsigned char *tic;
    uint16_t csum;

    if (icmp[0] != 8) return;  /* not echo request */

    tx = alloc_skb(ETH_HDR + IP_HDR + icmp_len);
    if (!tx) return;
    skb_reserve(tx, ETH_HDR + IP_HDR);
    tic = skb_put(tx, icmp_len);

    memcpy(tic, icmp, icmp_len);
    tic[0] = 0;
    tic[2] = 0; tic[3] = 0;
    csum   = net_checksum(tic, icmp_len);
    tic[2] = csum >> 8; tic[3] = csum & 0xFF;

    tx->dev = skb->dev;
    ip_tx(tx, rx + 6, ip + 12, 1);
}

void ip_rx(struct sk_buff *skb)
{
    unsigned char *ip = skb->data + ETH_HDR;
    uint8_t proto;

    if ((ip[0] >> 4) != 4)               return;  /* not IPv4 */
    if (memcmp(ip + 16, bbb_ip, 4) != 0) return;  /* not our IP */

    proto = ip[9];
    if (proto == 1)
        icmp_rx(skb, ip);
    else if (proto == 6)
        tcp_rx(skb, ip);
}
