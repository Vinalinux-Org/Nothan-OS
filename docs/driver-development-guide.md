# Driver Development Guide

**Đối tượng**: team viết driver mới cho VinixOS.

---

## 1. 3-layer model

```text
┌─────────────────────────────────────────────────────┐
│  KERNEL CORE — VFS, network stack, IRQ, kmalloc    │  (kernel team)
├─────────────────────────────────────────────────────┤
│  SUBSYSTEM CLASS — net_device, register_netdev,    │  (kernel team)
│  netif_rx, alloc_etherdev                          │
├─────────────────────────────────────────────────────┤
│  HW DRIVER — implement net_device_ops vtable       │  (BẠN viết)
│  drivers/net/ethernet/ti/cpsw.c                    │
└─────────────────────────────────────────────────────┘
```

Bạn chỉ chạm Layer 3. Hợp đồng giữa Layer 3 và Layer 2 là `struct net_device_ops`.

---

## 2. Interface contract (struct + helper)

Kernel core sẽ define ở `vinix-kernel/include/vinix/netdevice.h`. Signature dưới đây là CONTRACT.

### `struct net_device_ops` — vtable bạn implement

```c
struct net_device_ops {
    int (*ndo_open)        (struct net_device *ndev);
    int (*ndo_stop)        (struct net_device *ndev);
    int (*ndo_start_xmit)  (struct sk_buff *skb, struct net_device *ndev);
    int (*ndo_set_mac_address)(struct net_device *ndev, void *addr);
    int (*ndo_get_link)    (struct net_device *ndev);
};
```

### `struct net_device`

```c
struct net_device {
    char            name[IFNAMSIZ];      /* "eth0" — core gán */
    unsigned char   dev_addr[ETH_ALEN];  /* MAC address, 6 byte */
    unsigned int    mtu;                  /* default 1500 */
    unsigned int    flags;                /* IFF_UP, IFF_RUNNING */
    void           *priv;                 /* driver-private state */
    const struct net_device_ops *netdev_ops;
};
```

### `struct sk_buff` — gói tin

```c
struct sk_buff {
    unsigned char     *data;       /* payload pointer */
    unsigned int       len;        /* payload length */
    struct net_device *dev;
};
```

### Helpers core cung cấp

```c
struct net_device *alloc_etherdev(int sizeof_priv);
void               free_netdev(struct net_device *ndev);
void              *netdev_priv(struct net_device *ndev);

int                register_netdev(struct net_device *ndev);
void               unregister_netdev(struct net_device *ndev);

int                netif_rx(struct sk_buff *skb);
void               netif_stop_queue(struct net_device *ndev);
void               netif_wake_queue(struct net_device *ndev);
void               netif_carrier_on(struct net_device *ndev);
void               netif_carrier_off(struct net_device *ndev);

struct sk_buff    *alloc_skb(unsigned int size);
void               kfree_skb(struct sk_buff *skb);
unsigned char     *skb_put(struct sk_buff *skb, unsigned int len);
void               skb_reserve(struct sk_buff *skb, int len);
```

---

## 3. Khai báo platform_device

File: [vinix-kernel/arch/arm/mach-omap2/board-bbb.c]

```c
static struct platform_device bbb_cpsw0 = {
    .name = "ti-cpsw",
    .base = 0x4A100000,
    .irq  = 41,
    .clk_id = "cpsw_125mhz_gclk",
};
```

Append vào array `bbb_devices[]`. Kernel core tự register.

---

## 4. Skeleton driver file

File: `vinix-kernel/drivers/net/ethernet/ti/cpsw.c`

```c
/* ============================================================
 * cpsw.c — TI CPSW Ethernet driver for AM335x
 * ============================================================ */

#include "linux/init.h"
#include "linux/netdevice.h"
#include "linux/etherdevice.h"
#include "linux/skbuff.h"
#include "platform_device.h"
#include "mmio.h"
#include "irq.h"
#include "uart.h"

/* HW register offsets — định nghĩa theo TRM Ch.14 */
#define CPSW_SS_SOFT_RESET   0x008
/* ... */

/* Driver-private state */
struct cpsw_priv {
    void __iomem      *base;
    int                irq;
    struct net_device *ndev;
    /* TX/RX descriptor rings, PHY state, ... */
};

/* ──── Subsystem callbacks ──── */

static int cpsw_ndo_open(struct net_device *ndev)
{
    /* TODO */
    return 0;
}

static int cpsw_ndo_stop(struct net_device *ndev)
{
    /* TODO */
    return 0;
}

static int cpsw_ndo_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    /* TODO */
    return 0;
}

static const struct net_device_ops cpsw_netdev_ops = {
    .ndo_open       = cpsw_ndo_open,
    .ndo_stop       = cpsw_ndo_stop,
    .ndo_start_xmit = cpsw_ndo_start_xmit,
};

/* ──── IRQ handler ──── */

static void cpsw_irq(void *data)
{
    /* TODO: read IRQ status, dispatch RX/TX, ack */
}

/* ──── Probe / Remove ──── */

static int cpsw_probe(struct platform_device *pdev)
{
    struct net_device *ndev;
    struct cpsw_priv  *priv;
    struct resource   *mem;

    ndev = alloc_etherdev(sizeof(*priv));
    if (!ndev) return -E_NOMEM;

    priv = netdev_priv(ndev);
    mem  = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    priv->base = (void __iomem *)mem->start;
    priv->irq  = platform_get_irq(pdev, 0);
    priv->ndev = ndev;

    ndev->netdev_ops = &cpsw_netdev_ops;

    /* TODO: HW init (reset, ALE, MDIO scan PHY, MAC address) */

    request_irq(priv->irq, cpsw_irq, "ti-cpsw", priv);

    return register_netdev(ndev);
}

static int cpsw_remove(struct platform_device *pdev)
{
    /* TODO */
    return 0;
}

/* ──── Driver registration ──── */

static struct platform_driver cpsw_driver = {
    .drv    = { .name = "ti-cpsw" },
    .probe  = cpsw_probe,
    .remove = cpsw_remove,
};

module_platform_driver(cpsw_driver);
```
