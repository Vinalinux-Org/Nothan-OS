/*
 * include/nothan/netdevice.h — Network device abstraction
 */

#ifndef NOTHAN_NETDEVICE_H
#define NOTHAN_NETDEVICE_H

#include "types.h"

#define IFNAMSIZ      16
#define ETH_ALEN      6

/* net_device.flags */
#define IFF_UP        0x0001
#define IFF_RUNNING   0x0040
#define IFF_NOARP     0x0080

struct net_device;
struct sk_buff;

struct net_device_ops {
    int  (*ndo_open)            (struct net_device *ndev);
    int  (*ndo_stop)            (struct net_device *ndev);
    int  (*ndo_start_xmit)      (struct sk_buff *skb, struct net_device *ndev);
    int  (*ndo_set_mac_address) (struct net_device *ndev, void *addr);
    int  (*ndo_get_link)        (struct net_device *ndev);
};

struct net_device {
    char            name[IFNAMSIZ];      /* "eth0" — assigned by core */
    unsigned char   dev_addr[ETH_ALEN];  /* MAC address */
    unsigned int    mtu;                  /* default 1500 */
    unsigned int    flags;                /* IFF_UP, IFF_RUNNING, ... */
    void           *priv;                 /* driver-private — netdev_priv() */

    const struct net_device_ops *netdev_ops;

    /* Flow control state — set by netif_*_queue helpers. */
    int             tx_queue_stopped;
    int             carrier_ok;

    /* Linked-list slot in core registry. */
    struct net_device *next;
};

/* Register/unregister with subsystem core. */
int  register_netdev  (struct net_device *ndev);
void unregister_netdev(struct net_device *ndev);

/* Driver-private state accessor (offset just past struct). */
void *netdev_priv(struct net_device *ndev);
void  free_netdev(struct net_device *ndev);

/* RX entry — driver IRQ handler calls this with frame. Core
 * either queues for IP stack or drops if no consumer. */
int  netif_rx(struct sk_buff *skb);

/* TX flow control — driver calls when ring full / drained. */
void netif_stop_queue   (struct net_device *ndev);
void netif_wake_queue   (struct net_device *ndev);
int  netif_queue_stopped(struct net_device *ndev);

/* Link status — driver calls when PHY link state changes. */
void netif_carrier_on (struct net_device *ndev);
void netif_carrier_off(struct net_device *ndev);

#endif /* NOTHAN_NETDEVICE_H */
