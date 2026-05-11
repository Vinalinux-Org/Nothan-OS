/*
 * drivers/net/ipv4/tcp.c — TCP state machine for HTTP/1.0
 *
 * Single static connection (g_conn). No retransmit, no window scaling.
 */

#include "vinix/netdevice.h"
#include "vinix/skbuff.h"
#include "uart.h"
#include "string.h"

#define ETH_HDR  14
#define IP_HDR   20
#define TCP_HDR  20

#define TCP_FLAG_FIN  0x01
#define TCP_FLAG_SYN  0x02
#define TCP_FLAG_RST  0x04
#define TCP_FLAG_ACK  0x10

#define TCP_LISTEN      0
#define TCP_SYN_RCVD    1
#define TCP_ESTABLISHED 2
#define TCP_LAST_ACK    3

static const unsigned char bbb_ip[4] = {192, 168, 1, 100};

struct tcp_conn {
    uint32_t remote_ip;
    uint16_t remote_port;
    uint32_t seq;
    uint32_t ack;
    int      state;
};

static struct tcp_conn g_conn;
static unsigned char   http_resp_buf[512];

extern void     ip_tx(struct sk_buff *skb, const unsigned char *dst_mac,
                      const unsigned char *dst_ip, uint8_t proto);
extern uint16_t http_rx(const unsigned char *req, uint16_t req_len,
                        unsigned char *resp, uint16_t resp_max);

static uint16_t tcp_checksum(const unsigned char *pseudo,
                              const unsigned char *seg, uint16_t len)
{
    uint32_t s = 0;
    int i;
    for (i = 0; i < 12; i += 2)
        s += ((uint32_t)pseudo[i] << 8) | pseudo[i+1];
    for (i = 0; i + 1 < len; i += 2)
        s += ((uint32_t)seg[i] << 8) | seg[i+1];
    if (len & 1)
        s += (uint32_t)seg[len-1] << 8;
    while (s >> 16)
        s = (s & 0xFFFF) + (s >> 16);
    return (uint16_t)(~s);
}

static void tcp_send(struct sk_buff *rxskb, unsigned char *ip,
                     uint32_t seq, uint32_t ack_num, uint8_t flags,
                     const unsigned char *data, uint16_t data_len)
{
    unsigned char *rx      = rxskb->data;
    unsigned char *rtcp    = ip + (ip[0] & 0xF) * 4;
    uint16_t       tcp_len = TCP_HDR + data_len;
    struct sk_buff *tx;
    unsigned char *t, pseudo[12];
    uint16_t csum;

    tx = alloc_skb(ETH_HDR + IP_HDR + tcp_len);
    if (!tx) return;
    skb_reserve(tx, ETH_HDR + IP_HDR);
    t = skb_put(tx, tcp_len);
    memset(t, 0, TCP_HDR);

    t[0] = 0;       t[1] = 80;
    t[2] = rtcp[0]; t[3] = rtcp[1];

    t[4] = seq >> 24;          t[5] = (seq >> 16) & 0xFF;
    t[6] = (seq >>  8) & 0xFF; t[7] =  seq        & 0xFF;

    t[8]  = ack_num >> 24;           t[9]  = (ack_num >> 16) & 0xFF;
    t[10] = (ack_num >>  8) & 0xFF;  t[11] =  ack_num        & 0xFF;

    t[12] = (TCP_HDR / 4) << 4;
    t[13] = flags;
    t[14] = 0xFF; t[15] = 0xFF;

    if (data && data_len)
        memcpy(t + TCP_HDR, data, data_len);

    memcpy(pseudo,     bbb_ip,  4);
    memcpy(pseudo + 4, ip + 12, 4);
    pseudo[8]  = 0;
    pseudo[9]  = 6;
    pseudo[10] = tcp_len >> 8;
    pseudo[11] = tcp_len & 0xFF;
    csum    = tcp_checksum(pseudo, t, tcp_len);
    t[16]   = csum >> 8; t[17] = csum & 0xFF;

    tx->dev = rxskb->dev;
    ip_tx(tx, rx + 6, ip + 12, 6);
}

static void tcp_handle_data(struct sk_buff *skb, unsigned char *ip,
                             unsigned char *data, uint16_t data_len)
{
    uint16_t resp_len;

    g_conn.ack += data_len;

    tcp_send(skb, ip, g_conn.seq, g_conn.ack, TCP_FLAG_ACK, NULL, 0);

    resp_len = http_rx(data, data_len, http_resp_buf, sizeof(http_resp_buf));
    if (resp_len > 0) {
        tcp_send(skb, ip, g_conn.seq, g_conn.ack,
                 TCP_FLAG_ACK, http_resp_buf, resp_len);
        g_conn.seq += resp_len;
    }

    tcp_send(skb, ip, g_conn.seq, g_conn.ack,
             TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
    g_conn.seq++;
    g_conn.state = TCP_LAST_ACK;
    pr_info("[TCP] FIN sent\n");
}

void tcp_rx(struct sk_buff *skb, unsigned char *ip)
{
    unsigned char *tcp   = ip + (ip[0] & 0xF) * 4;
    uint8_t  flags       = tcp[13];
    uint32_t client_seq  = ((uint32_t)tcp[4] << 24) | ((uint32_t)tcp[5] << 16) |
                           ((uint32_t)tcp[6] <<  8) |  tcp[7];
    uint16_t tcp_hdr_len = ((tcp[12] >> 4) & 0xF) * 4;
    uint16_t ip_total    = ((uint16_t)ip[2] << 8) | ip[3];
    uint16_t data_len    = ip_total - (uint16_t)(ip[0] & 0xF) * 4 - tcp_hdr_len;
    unsigned char *data  = tcp + tcp_hdr_len;

    if (flags & TCP_FLAG_RST) {
        g_conn.state = TCP_LISTEN;
        return;
    }

    switch (g_conn.state) {
    case TCP_LISTEN:
        if (!(flags & TCP_FLAG_SYN)) break;
        g_conn.seq         = 0x12345678;
        g_conn.ack         = client_seq + 1;
        g_conn.remote_port = ((uint16_t)tcp[0] << 8) | tcp[1];
        pr_info("[TCP] SYN from %d.%d.%d.%d:%d\n",
                ip[12], ip[13], ip[14], ip[15], g_conn.remote_port);
        tcp_send(skb, ip, g_conn.seq, g_conn.ack,
                 TCP_FLAG_SYN | TCP_FLAG_ACK, NULL, 0);
        g_conn.seq++;
        g_conn.state = TCP_SYN_RCVD;
        pr_info("[TCP] SYN-ACK sent\n");
        break;

    case TCP_SYN_RCVD:
        if (!(flags & TCP_FLAG_ACK)) break;
        g_conn.state = TCP_ESTABLISHED;
        pr_info("[TCP] ESTABLISHED\n");
        if (data_len > 0)
            tcp_handle_data(skb, ip, data, data_len);
        break;

    case TCP_ESTABLISHED:
        if (flags & TCP_FLAG_FIN) {
            g_conn.ack++;
            tcp_send(skb, ip, g_conn.seq, g_conn.ack, TCP_FLAG_ACK, NULL, 0);
            g_conn.state = TCP_LISTEN;
            break;
        }
        if (data_len > 0)
            tcp_handle_data(skb, ip, data, data_len);
        break;

    case TCP_LAST_ACK:
        if (flags & TCP_FLAG_ACK) {
            g_conn.state = TCP_LISTEN;
            pr_info("[TCP] CLOSED\n");
        }
        break;
    }
}
