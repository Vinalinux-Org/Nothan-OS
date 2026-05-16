/*
 * include/nothan/etherdevice.h — Ethernet device helpers
 */

#ifndef NOTHAN_ETHERDEVICE_H
#define NOTHAN_ETHERDEVICE_H

#include "nothan/netdevice.h"
#include "nothan/skbuff.h"

#define ETH_HLEN          14      /* sizeof(struct ethhdr) */
#define ETH_FRAME_LEN     1514    /* max frame including header */
#define ETH_DATA_LEN      1500    /* MTU */

#define ETH_P_IP          0x0800
#define ETH_P_ARP         0x0806
#define ETH_P_IPV6        0x86DD

struct ethhdr {
    unsigned char  h_dest[ETH_ALEN];
    unsigned char  h_source[ETH_ALEN];
    uint16_t       h_proto;
};

/* Allocate net_device + sizeof_priv bytes for driver state.
 * Pre-fills MTU=1500 and L2-related defaults. */
struct net_device *alloc_etherdev(int sizeof_priv);

/* Pull ethernet header off skb, set skb->protocol. Returns
 * the protocol (host byte order). Driver calls this in the
 * RX path before netif_rx. */
uint16_t eth_type_trans(struct sk_buff *skb, struct net_device *dev);

/* True if `addr` is a valid unicast (not multicast, not all-zero). */
int is_valid_ether_addr(const unsigned char *addr);

#endif /* NOTHAN_ETHERDEVICE_H */
