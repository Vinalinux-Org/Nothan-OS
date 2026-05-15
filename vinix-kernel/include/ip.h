/*
 * include/ip.h — IPv4 layer public interface
 */
#ifndef IP_H
#define IP_H
#include "types.h"
#include "vinix/skbuff.h"

void ip_rx(struct sk_buff *skb);
void ip_tx(struct sk_buff *skb, const unsigned char *dst_mac,
           const unsigned char *dst_ip, uint8_t proto);
#endif
