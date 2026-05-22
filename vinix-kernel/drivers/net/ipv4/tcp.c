/*
 * drivers/net/ipv4/tcp.c — TCP state machine: connection pool, HTTP/1.1 keep-alive, SSE
 *
 * conn_table[MAX_CONN]: route packets by (remote_ip, remote_port).
 * No retransmit, no window scaling.
 */

#include "vinix/netdevice.h"
#include "vinix/skbuff.h"
#include "sleep.h"
#include "uart.h"
#include "string.h"
#include "ip.h"
#include "http.h"

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

#define HTTP_CHUNK      1400

#define MAX_CONN        32
#define CONN_TYPE_HTTP  0
#define CONN_TYPE_SSE   1

/* keep-alive idle timeout: 5 seconds (500 ticks at 100 Hz) */
#define KA_TIMEOUT_TICKS  500
#define SYN_TIMEOUT_TICKS 300

static const unsigned char bbb_ip[4] = {192, 168, 2, 100};

struct tcp_conn {
    uint32_t           remote_ip;
    uint16_t           remote_port;
    uint32_t           seq;
    uint32_t           ack;
    int                state;
    int                type;          /* CONN_TYPE_HTTP | CONN_TYPE_SSE */
    uint32_t           last_active;   /* jiffies — for keep-alive timeout */
    struct net_device *dev;
    unsigned char      remote_mac[6];
};

static struct tcp_conn conn_table[MAX_CONN];
static unsigned char   http_resp_buf[32768];

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

static void tcp_send(struct tcp_conn *conn, uint32_t seq, uint32_t ack_num,
                     uint8_t flags, const unsigned char *data, uint16_t data_len)
{
    unsigned char dst_ip[4];
    uint16_t      tcp_len = TCP_HDR + data_len;
    struct sk_buff *tx;
    unsigned char  *t, pseudo[12];
    uint16_t        csum;

    dst_ip[0] = (conn->remote_ip >> 24) & 0xFF;
    dst_ip[1] = (conn->remote_ip >> 16) & 0xFF;
    dst_ip[2] = (conn->remote_ip >>  8) & 0xFF;
    dst_ip[3] =  conn->remote_ip        & 0xFF;

    tx = alloc_skb(ETH_HDR + IP_HDR + tcp_len);
    if (!tx) return;
    skb_reserve(tx, ETH_HDR + IP_HDR);
    t = skb_put(tx, tcp_len);
    memset(t, 0, TCP_HDR);

    t[0] = 0;  t[1] = 80;
    t[2] = conn->remote_port >> 8;  t[3] = conn->remote_port & 0xFF;

    t[4] = seq >> 24;          t[5] = (seq >> 16) & 0xFF;
    t[6] = (seq >>  8) & 0xFF; t[7] =  seq        & 0xFF;

    t[8]  = ack_num >> 24;           t[9]  = (ack_num >> 16) & 0xFF;
    t[10] = (ack_num >>  8) & 0xFF;  t[11] =  ack_num        & 0xFF;

    t[12] = (TCP_HDR / 4) << 4;
    t[13] = flags;
    t[14] = 0xFF; t[15] = 0xFF;

    if (data && data_len) memcpy(t + TCP_HDR, data, data_len);

    memcpy(pseudo,     bbb_ip,  4);
    memcpy(pseudo + 4, dst_ip,  4);
    pseudo[8]  = 0;
    pseudo[9]  = 6;
    pseudo[10] = tcp_len >> 8;
    pseudo[11] = tcp_len & 0xFF;
    csum    = tcp_checksum(pseudo, t, tcp_len);
    t[16]   = csum >> 8; t[17] = csum & 0xFF;

    tx->dev = conn->dev;
    ip_tx(tx, conn->remote_mac, dst_ip, 6);
}

static struct tcp_conn *find_conn(uint32_t remote_ip, uint16_t remote_port)
{
    int i;
    for (i = 0; i < MAX_CONN; i++) {
        if (conn_table[i].state != TCP_LISTEN &&
            conn_table[i].remote_ip   == remote_ip &&
            conn_table[i].remote_port == remote_port)
            return &conn_table[i];
    }
    return NULL;
}

static struct tcp_conn *alloc_conn(void)
{
    int i;
    for (i = 0; i < MAX_CONN; i++)
        if (conn_table[i].state == TCP_LISTEN) return &conn_table[i];
    return NULL;
}

static void tcp_handle_data(struct tcp_conn *conn,
                             unsigned char *data, uint16_t data_len)
{
    int      ka = 0, ctype = CONN_TYPE_HTTP;
    uint16_t resp_len, offset, chunk;

    conn->ack += data_len;
    tcp_send(conn, conn->seq, conn->ack, TCP_FLAG_ACK, NULL, 0);

    resp_len = http_rx(data, data_len, http_resp_buf, sizeof(http_resp_buf),
                       &ka, &ctype);
    if (resp_len > 0) {
        offset = 0;
        while (offset < resp_len) {
            chunk = resp_len - offset;
            if (chunk > HTTP_CHUNK) chunk = HTTP_CHUNK;
            tcp_send(conn, conn->seq, conn->ack,
                     TCP_FLAG_ACK, http_resp_buf + offset, chunk);
            conn->seq += chunk;
            offset    += chunk;
        }
    }

    if (ctype == CONN_TYPE_SSE) {
        conn->type        = CONN_TYPE_SSE;
        conn->last_active = jiffies;
        /* SSE connection stays ESTABLISHED — net_task pushes frames */
    } else if (ka) {
        conn->last_active = jiffies;
        /* HTTP keep-alive: stay ESTABLISHED, await next request */
    } else {
        tcp_send(conn, conn->seq, conn->ack,
                 TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
        conn->seq++;
        conn->state = TCP_LAST_ACK;
    }
}

void tcp_rx(struct sk_buff *skb, unsigned char *ip)
{
    unsigned char *rx        = skb->data;
    unsigned char *tcp       = ip + (ip[0] & 0xF) * 4;
    uint8_t  flags           = tcp[13];
    uint32_t client_seq      = ((uint32_t)tcp[4] << 24) | ((uint32_t)tcp[5] << 16) |
                               ((uint32_t)tcp[6] <<  8) |  tcp[7];
    uint16_t tcp_hdr_len     = ((tcp[12] >> 4) & 0xF) * 4;
    uint16_t ip_total        = ((uint16_t)ip[2] << 8) | ip[3];
    uint16_t data_len        = ip_total - (uint16_t)(ip[0] & 0xF) * 4 - tcp_hdr_len;
    unsigned char *data      = tcp + tcp_hdr_len;
    uint32_t remote_ip       = ((uint32_t)ip[12] << 24) | ((uint32_t)ip[13] << 16) |
                               ((uint32_t)ip[14] <<  8) |  ip[15];
    uint16_t remote_port     = ((uint16_t)tcp[0] << 8) | tcp[1];
    struct tcp_conn *conn;

    if (flags & TCP_FLAG_RST) {
        conn = find_conn(remote_ip, remote_port);
        if (conn) conn->state = TCP_LISTEN;
        return;
    }

    conn = find_conn(remote_ip, remote_port);
    if (!conn) {
        if (!(flags & TCP_FLAG_SYN)) return;
        conn = alloc_conn();
        if (!conn) { pr_info("[TCP] table full, drop SYN\n"); return; }
    }

    switch (conn->state) {
    case TCP_LISTEN:
        if (!(flags & TCP_FLAG_SYN)) break;
        conn->remote_ip   = remote_ip;
        conn->remote_port = remote_port;
        conn->dev         = skb->dev;
        memcpy(conn->remote_mac, rx + 6, 6);
        conn->seq         = 0x12345678;
        conn->ack         = client_seq + 1;
        conn->type        = CONN_TYPE_HTTP;
        conn->last_active = jiffies;
        tcp_send(conn, conn->seq, conn->ack,
                 TCP_FLAG_SYN | TCP_FLAG_ACK, NULL, 0);
        conn->seq++;
        conn->state = TCP_SYN_RCVD;
        break;

    case TCP_SYN_RCVD:
        if (!(flags & TCP_FLAG_ACK)) break;
        conn->state = TCP_ESTABLISHED;
        conn->last_active = jiffies;
        if (data_len > 0) tcp_handle_data(conn, data, data_len);
        break;

    case TCP_ESTABLISHED:
        conn->last_active = jiffies;
        if (flags & TCP_FLAG_FIN) {
            conn->ack++;
            tcp_send(conn, conn->seq, conn->ack, TCP_FLAG_ACK, NULL, 0);
            conn->state = TCP_LISTEN;
            break;
        }
        if (data_len > 0) tcp_handle_data(conn, data, data_len);
        break;

    case TCP_LAST_ACK:
        if (flags & TCP_FLAG_ACK) {
            conn->state = TCP_LISTEN;
        }
        break;
    }
}

/* Called from net_task every second: close idle keep-alive connections */
void tcp_poll(void)
{
    int i;
    for (i = 0; i < MAX_CONN; i++) {
        struct tcp_conn *conn = &conn_table[i];
        
        if (conn->state == TCP_SYN_RCVD) {
            if (jiffies - conn->last_active > SYN_TIMEOUT_TICKS) {
                pr_info("[TCP] SYN timeout, freeing slot\n");
                conn->state = TCP_LISTEN;
            }
            continue;
        }

        if (conn->state != TCP_ESTABLISHED) continue;
        if (conn->type  == CONN_TYPE_HTTP &&
            jiffies - conn->last_active > KA_TIMEOUT_TICKS) {
            tcp_send(conn, conn->seq, conn->ack,
                     TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
            conn->seq++;
            conn->state = TCP_LAST_ACK;
        }
    }
}

/* Called from net_task every second: push SSE frame to all SSE connections */
void tcp_sse_push(const unsigned char *frame, uint16_t frame_len)
{
    int i;
    for (i = 0; i < MAX_CONN; i++) {
        struct tcp_conn *conn = &conn_table[i];
        if (conn->state == TCP_ESTABLISHED && conn->type == CONN_TYPE_SSE) {
            tcp_send(conn, conn->seq, conn->ack,
                     TCP_FLAG_ACK, frame, frame_len);
            conn->seq        += frame_len;
            conn->last_active = jiffies;
        }
    }
}
