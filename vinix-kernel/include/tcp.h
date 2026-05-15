/*
 * include/tcp.h — TCP layer public interface
 */
#ifndef TCP_H
#define TCP_H
#include "types.h"
#include "vinix/skbuff.h"

void tcp_rx(struct sk_buff *skb, unsigned char *ip);
void tcp_poll(void);
void tcp_sse_push(const unsigned char *frame, uint16_t frame_len);
#endif
