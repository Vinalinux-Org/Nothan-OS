/*
 * drivers/net/net_core.c — network device registry
 *
 * Drivers fill a net_device, then call register_netdev() to expose it.
 * Core assigns the interface name (eth0, eth1, ...) and holds a pointer
 * table — same static-pool pattern as mmc_core and i2c-core.
 */

#include "nothan/netdevice.h"
#include "nothan/skbuff.h"
#include "nothan/errno.h"
#include "uart.h"
#include "format.h"

#define MAX_NET_DEVS 4

static struct net_device *netdev_table[MAX_NET_DEVS];
static int                netdev_count = 0;

int register_netdev(struct net_device *ndev)
{
    if (!ndev) return -EINVAL;
    if (netdev_count >= MAX_NET_DEVS) return -ENOSPC;

    ksnprintf(ndev->name, IFNAMSIZ, "eth%d", netdev_count);
    netdev_table[netdev_count++] = ndev;
    pr_info("[NET] %s registered\n", ndev->name);
    return 0;
}

void unregister_netdev(struct net_device *ndev)
{
    (void)ndev;
}

void *netdev_priv(struct net_device *ndev)
{
    return ndev->priv;
}

void free_netdev(struct net_device *ndev)
{
    (void)ndev;
}

extern void vnet_rx(struct sk_buff *skb);

int netif_rx(struct sk_buff *skb)
{
    vnet_rx(skb);
    return 0;
}

void netif_stop_queue(struct net_device *ndev)  { ndev->tx_queue_stopped = 1; }
void netif_wake_queue(struct net_device *ndev)  { ndev->tx_queue_stopped = 0; }
int  netif_queue_stopped(struct net_device *ndev) { return ndev->tx_queue_stopped; }

void netif_carrier_on(struct net_device *ndev)  { ndev->carrier_ok = 1; }
void netif_carrier_off(struct net_device *ndev) { ndev->carrier_ok = 0; }
