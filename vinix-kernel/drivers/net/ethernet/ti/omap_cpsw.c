/*
 * drivers/net/ethernet/ti/omap_cpsw.c — AM335x CPSW Ethernet driver
 *
 * Single-port Ethernet via CPSW_3G Port 1 (LAN8710A, MII).
 * ALE bypass mode — tất cả frame forwarded về host port 0.
 * BDs và buffers đặt trong CPPI_RAM (0x4A102000, strongly ordered).
 */

#include "types.h"
#include "mmio.h"
#include "uart.h"
#include "irq.h"
#include "platform_device.h"
#include "vinix/init.h"
#include "vinix/errno.h"
#include "vinix/netdevice.h"
#include "vinix/skbuff.h"
#include "vinix/mdio.h"
#include "vinix/phy.h"
#include "mach/memmap.h"
#include "mach/prcm.h"
#include "mach/control.h"
#include "mach/irqs.h"
#include "string.h"

/*
 * MII1 pad mux — TRM Ch.9 §9.3.1.50 + Table 9-10.
 * RXACTIVE=1, PUDEN=1, MMODE=0 → 0x28 (input pads + clocks).
 * RXACTIVE=0, PUDEN=1, MMODE=0 → 0x08 (output-only pads).
 */
#define CONF_MII1_COL       (CTRL_MODULE_BASE + 0x908)
#define CONF_MII1_CRS       (CTRL_MODULE_BASE + 0x90C)
#define CONF_MII1_RX_ER     (CTRL_MODULE_BASE + 0x910)
#define CONF_MII1_TX_EN     (CTRL_MODULE_BASE + 0x914)
#define CONF_MII1_RX_DV     (CTRL_MODULE_BASE + 0x918)
#define CONF_MII1_TXD3      (CTRL_MODULE_BASE + 0x91C)
#define CONF_MII1_TXD2      (CTRL_MODULE_BASE + 0x920)
#define CONF_MII1_TXD1      (CTRL_MODULE_BASE + 0x924)
#define CONF_MII1_TXD0      (CTRL_MODULE_BASE + 0x928)
#define CONF_MII1_TX_CLK    (CTRL_MODULE_BASE + 0x92C)
#define CONF_MII1_RX_CLK    (CTRL_MODULE_BASE + 0x930)
#define CONF_MII1_RXD3      (CTRL_MODULE_BASE + 0x934)
#define CONF_MII1_RXD2      (CTRL_MODULE_BASE + 0x938)
#define CONF_MII1_RXD1      (CTRL_MODULE_BASE + 0x93C)
#define CONF_MII1_RXD0      (CTRL_MODULE_BASE + 0x940)

#define MII1_PAD_INPUT      0x28
#define MII1_PAD_OUTPUT     0x08

/* CPSW_SS registers */
#define SS_ID_VER            0x00
#define SS_SOFT_RESET        0x08
#define SS_STAT_PORT_EN      0x0C

/* CPSW_PORT registers */
#define PORT_P1_SA_LO        0x120
#define PORT_P1_SA_HI        0x124

/* CPDMA registers */
#define CPDMA_TX_CONTROL     0x04
#define CPDMA_RX_CONTROL     0x14
#define CPDMA_SOFT_RESET     0x1C
#define CPDMA_RX_BUF_OFFSET  0x28
#define CPDMA_TX_INTMASK_SET 0x88
#define CPDMA_RX_INTMASK_SET 0xA8
#define CPDMA_EOI_VECTOR     0x94
#define CPDMA_EOI_RX         1
#define CPDMA_EOI_TX         2

/* CPDMA StateRAM (HDP/CP) */
#define SR_TX0_HDP           0x00
#define SR_RX0_HDP           0x20
#define SR_TX0_CP            0x40
#define SR_RX0_CP            0x60

/* ALE registers */
#define ALE_CONTROL          0x08
#define ALE_PORTCTL0         0x40
#define ALE_PORTCTL1         0x44
#define ALE_PORTCTL2         0x48

#define ALE_CTL_ENABLE       (1u << 31)
#define ALE_CTL_CLEAR_TBL    (1 << 30)
#define ALE_CTL_BYPASS       (1 << 4)
#define ALE_PORT_FORWARD     3

/* CPSW_SL1 registers */
#define SL_MACCONTROL        0x04
#define SL_SOFT_RESET        0x0C

#define MAC_GMII_EN          (1 << 5)
#define MAC_FULLDUPLEX       (1 << 0)

/* CPSW_WR registers */
#define WR_SOFT_RESET        0x04
#define WR_C0_RX_EN          0x14
#define WR_C0_TX_EN          0x18
#define WR_C0_RX_STAT        0x44
#define WR_C0_TX_STAT        0x48

/* Buffer Descriptor flags */
#define BD_SOP               (1u << 31)
#define BD_EOP               (1 << 30)
#define BD_OWNER             (1 << 29)
#define BD_EOQ               (1 << 28)
#define BD_TO_PORT_EN        (1 << 20)
#define BD_TO_PORT1          (1 << 16)
#define BD_PKT_LEN_MASK      0x7FF

/* CPPI_RAM layout (8KB = 0x4A102000..0x4A103FFF):
 *   0x0000: rx_bd[4]  — 4 × 16 bytes = 64 bytes
 *   0x0040: tx_bd     — 16 bytes
 *   0x0100: rx_buf[4] — 4 × 1536 = 6144 bytes
 *   0x1900: tx_buf    — 1536 bytes        (total 7824 < 8192)
 */
#define CPSW_RX_COUNT        4
#define CPSW_BUF_SIZE        1536
#define CPSW_TIMEOUT         10000
#define CPPI_RX_BD(i)        (OMAP_CPPI_RAM_BASE + 0x0000 + (i) * 16)
#define CPPI_TX_BD           (OMAP_CPPI_RAM_BASE + 0x0040)
#define CPPI_RX_BUF(i)       (OMAP_CPPI_RAM_BASE + 0x0100 + (i) * CPSW_BUF_SIZE)
#define CPPI_TX_BUF          (OMAP_CPPI_RAM_BASE + 0x1900)

/* BD word accessors — CPPI_RAM là strongly ordered, bắt buộc dùng mmio. */
#define BD_NEXT(a)           mmio_read32((a) + 0)
#define BD_BUF(a)            mmio_read32((a) + 4)
#define BD_FLAGS(a)          mmio_read32((a) + 12)

struct cpsw_priv {
    uint32_t    ss_base;
    uint32_t    cpdma_base;
    uint32_t    cpdma_sr_base;
    uint32_t    ale_base;
    uint32_t    sl1_base;
    uint32_t    wr_base;
    uint32_t    port_base;
    struct phy_device  *phy;
    struct net_device  *ndev;
    int         rx_idx;
};

static struct cpsw_priv  omap_cpsw_priv;
static struct net_device omap_cpsw_ndev;

static void cpsw_bd_init(uint32_t addr, uint32_t next,
                         uint32_t buf, uint32_t buf_len, uint32_t flags)
{
    mmio_write32(addr + 0,  next);
    mmio_write32(addr + 4,  buf);
    mmio_write32(addr + 8,  buf_len);
    mmio_write32(addr + 12, flags);
}

/* CPPI_RAM is strongly ordered — copy data word by word via mmio */
static void cpsw_buf_write(uint32_t addr, const unsigned char *src, unsigned int len)
{
    unsigned int i;
    for (i = 0; i + 4 <= len; i += 4) {
        uint32_t w = ((uint32_t)src[i])
                   | ((uint32_t)src[i+1] << 8)
                   | ((uint32_t)src[i+2] << 16)
                   | ((uint32_t)src[i+3] << 24);
        mmio_write32(addr + i, w);
    }
    if (i < len) {
        uint32_t w = 0;
        unsigned int j;
        for (j = 0; i + j < len; j++)
            w |= ((uint32_t)src[i + j]) << (j * 8);
        mmio_write32(addr + i, w);
    }
}

static void cpsw_buf_read(unsigned char *dst, uint32_t addr, unsigned int len)
{
    unsigned int i;
    for (i = 0; i + 4 <= len; i += 4) {
        uint32_t w = mmio_read32(addr + i);
        dst[i]   = (unsigned char)(w);
        dst[i+1] = (unsigned char)(w >> 8);
        dst[i+2] = (unsigned char)(w >> 16);
        dst[i+3] = (unsigned char)(w >> 24);
    }
    if (i < len) {
        uint32_t w = mmio_read32(addr + i);
        unsigned int j;
        for (j = 0; i + j < len; j++)
            dst[i + j] = (unsigned char)(w >> (j * 8));
    }
}

static void cpsw_pinmux_setup(void)
{
    /* RX data + status + both clocks: input enabled */
    mmio_write32(CONF_MII1_COL,    MII1_PAD_INPUT);
    mmio_write32(CONF_MII1_CRS,    MII1_PAD_INPUT);
    mmio_write32(CONF_MII1_RX_ER,  MII1_PAD_INPUT);
    mmio_write32(CONF_MII1_RX_DV,  MII1_PAD_INPUT);
    mmio_write32(CONF_MII1_TX_CLK, MII1_PAD_INPUT);  /* PHY drives TX_CLK in MII */
    mmio_write32(CONF_MII1_RX_CLK, MII1_PAD_INPUT);
    mmio_write32(CONF_MII1_RXD3,   MII1_PAD_INPUT);
    mmio_write32(CONF_MII1_RXD2,   MII1_PAD_INPUT);
    mmio_write32(CONF_MII1_RXD1,   MII1_PAD_INPUT);
    mmio_write32(CONF_MII1_RXD0,   MII1_PAD_INPUT);

    /* TX data + enable: output only */
    mmio_write32(CONF_MII1_TX_EN, MII1_PAD_OUTPUT);
    mmio_write32(CONF_MII1_TXD3,  MII1_PAD_OUTPUT);
    mmio_write32(CONF_MII1_TXD2,  MII1_PAD_OUTPUT);
    mmio_write32(CONF_MII1_TXD1,  MII1_PAD_OUTPUT);
    mmio_write32(CONF_MII1_TXD0,  MII1_PAD_OUTPUT);

    pr_info("[CPSW] MII1 pinmux configured (15 pads, mode 0)\n");
}

static int cpsw_soft_reset(uint32_t reg)
{
    int timeout = CPSW_TIMEOUT;
    mmio_write32(reg, 1);
    while (mmio_read32(reg) && timeout--)
        ;
    return timeout ? 0 : -EIO;
}

static int cpsw_hw_reset(struct cpsw_priv *priv)
{
    if (cpsw_soft_reset(priv->sl1_base   + SL_SOFT_RESET))   return -EIO;
    if (cpsw_soft_reset(priv->cpdma_base + CPDMA_SOFT_RESET)) return -EIO;
    if (cpsw_soft_reset(priv->wr_base    + WR_SOFT_RESET))    return -EIO;
    if (cpsw_soft_reset(priv->ss_base    + SS_SOFT_RESET))    return -EIO;
    pr_info("[CPSW] hw reset done\n");
    return 0;
}

static void cpsw_ale_init(struct cpsw_priv *priv)
{
    mmio_write32(priv->ale_base + ALE_CONTROL,
                 ALE_CTL_ENABLE | ALE_CTL_CLEAR_TBL | ALE_CTL_BYPASS);
    mmio_write32(priv->ale_base + ALE_PORTCTL0, ALE_PORT_FORWARD);
    mmio_write32(priv->ale_base + ALE_PORTCTL1, ALE_PORT_FORWARD);
    mmio_write32(priv->ale_base + ALE_PORTCTL2, ALE_PORT_FORWARD);
    pr_info("[CPSW] ALE bypass, control=0x%08x\n",
            mmio_read32(priv->ale_base + ALE_CONTROL));
}

static void cpsw_sl_init(struct cpsw_priv *priv)
{
    const uint8_t *mac = priv->ndev->dev_addr;
    uint32_t sa_lo, sa_hi;

    sa_lo = ((uint32_t)mac[0] << 8)  | mac[1];
    sa_hi = ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16)
          | ((uint32_t)mac[4] << 8)  | mac[5];

    mmio_write32(priv->port_base + PORT_P1_SA_LO, sa_lo);
    mmio_write32(priv->port_base + PORT_P1_SA_HI, sa_hi);

    mmio_write32(priv->sl1_base + SL_MACCONTROL, MAC_GMII_EN | MAC_FULLDUPLEX);
    pr_info("[CPSW] SL1 maccontrol=0x%08x mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
            mmio_read32(priv->sl1_base + SL_MACCONTROL),
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static int cpsw_cpdma_init(struct cpsw_priv *priv)
{
    int i;

    /* Zero all HDP/CP — required before enabling channels */
    for (i = 0; i < 8; i++) {
        mmio_write32(priv->cpdma_sr_base + SR_TX0_HDP + i * 4, 0);
        mmio_write32(priv->cpdma_sr_base + SR_RX0_HDP + i * 4, 0);
        mmio_write32(priv->cpdma_sr_base + SR_TX0_CP  + i * 4, 0);
        mmio_write32(priv->cpdma_sr_base + SR_RX0_CP  + i * 4, 0);
    }

    mmio_write32(priv->cpdma_base + CPDMA_RX_BUF_OFFSET, 0);
    mmio_write32(priv->cpdma_base + CPDMA_TX_CONTROL, 1);
    mmio_write32(priv->cpdma_base + CPDMA_RX_CONTROL, 1);

    mmio_write32(priv->cpdma_base + CPDMA_TX_INTMASK_SET, 1);
    mmio_write32(priv->cpdma_base + CPDMA_RX_INTMASK_SET, 1);

    mmio_write32(priv->wr_base + WR_C0_TX_EN, 1);
    mmio_write32(priv->wr_base + WR_C0_RX_EN, 1);

    pr_info("[CPSW] CPDMA enabled, tx_ctrl=0x%08x rx_ctrl=0x%08x\n",
            mmio_read32(priv->cpdma_base + CPDMA_TX_CONTROL),
            mmio_read32(priv->cpdma_base + CPDMA_RX_CONTROL));
    return 0;
}

static void cpsw_rx_ring_init(struct cpsw_priv *priv)
{
    int i;

    for (i = 0; i < CPSW_RX_COUNT; i++) {
        uint32_t next = (i < CPSW_RX_COUNT - 1) ? CPPI_RX_BD(i + 1) : 0;
        cpsw_bd_init(CPPI_RX_BD(i), next, CPPI_RX_BUF(i), CPSW_BUF_SIZE, BD_OWNER);
    }

    priv->rx_idx = 0;
    mmio_write32(priv->cpdma_sr_base + SR_RX0_HDP, CPPI_RX_BD(0));
    pr_info("[CPSW] RX ring ready, HDP=0x%08x\n", CPPI_RX_BD(0));
}

static int cpsw_ndo_open(struct net_device *ndev)
{
    ndev->flags |= IFF_UP | IFF_RUNNING;
    netif_carrier_on(ndev);
    netif_wake_queue(ndev);
    pr_info("[CPSW] %s up\n", ndev->name);
    return 0;
}

static int cpsw_ndo_stop(struct net_device *ndev)
{
    ndev->flags &= ~(IFF_UP | IFF_RUNNING);
    netif_carrier_off(ndev);
    netif_stop_queue(ndev);
    pr_info("[CPSW] %s down\n", ndev->name);
    return 0;
}

static int cpsw_ndo_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    struct cpsw_priv *priv = ndev->priv;
    uint32_t flags;

    flags = BD_FLAGS(CPPI_TX_BD);
    if (flags & BD_OWNER) {
        netif_stop_queue(ndev);
        return -EBUSY;
    }

    cpsw_buf_write(CPPI_TX_BUF, skb->data, skb->len);
    cpsw_bd_init(CPPI_TX_BD, 0, CPPI_TX_BUF, skb->len,
                 BD_SOP | BD_EOP | BD_OWNER | BD_TO_PORT_EN | BD_TO_PORT1
                 | (skb->len & BD_PKT_LEN_MASK));

    mmio_write32(priv->cpdma_sr_base + SR_TX0_HDP, CPPI_TX_BD);

    kfree_skb(skb);
    return 0;
}

static int cpsw_ndo_get_link(struct net_device *ndev)
{
    struct cpsw_priv *priv = ndev->priv;
    if (!priv->phy) return 0;
    phy_update_link(priv->phy);
    return priv->phy->link;
}

static const struct net_device_ops cpsw_netdev_ops = {
    .ndo_open            = cpsw_ndo_open,
    .ndo_stop            = cpsw_ndo_stop,
    .ndo_start_xmit      = cpsw_ndo_start_xmit,
    .ndo_set_mac_address = 0,
    .ndo_get_link        = cpsw_ndo_get_link,
};

static void cpsw_rx_irq(void *data)
{
    struct cpsw_priv *priv = data;
    int i = priv->rx_idx;
    uint32_t bd    = CPPI_RX_BD(i);
    uint32_t flags = BD_FLAGS(bd);

    if (flags & BD_OWNER) {
        mmio_write32(priv->cpdma_base + CPDMA_EOI_VECTOR, CPDMA_EOI_RX);
        return;
    }

    unsigned int pkt_len = flags & BD_PKT_LEN_MASK;
    if (pkt_len > 0 && pkt_len <= CPSW_BUF_SIZE) {
        struct sk_buff *skb = alloc_skb(pkt_len);
        if (skb) {
            cpsw_buf_read(skb_put(skb, pkt_len), CPPI_RX_BUF(i), pkt_len);
            skb->dev = priv->ndev;
            netif_rx(skb);
        }
    }

    cpsw_bd_init(bd, 0, CPPI_RX_BUF(i), CPSW_BUF_SIZE, BD_OWNER);

    mmio_write32(priv->cpdma_sr_base + SR_RX0_CP, bd);

    if (flags & BD_EOQ)
        mmio_write32(priv->cpdma_sr_base + SR_RX0_HDP, bd);
    else
        priv->rx_idx = (i + 1) % CPSW_RX_COUNT;

    mmio_write32(priv->cpdma_base + CPDMA_EOI_VECTOR, CPDMA_EOI_RX);
}

static void cpsw_tx_irq(void *data)
{
    struct cpsw_priv *priv = data;

    mmio_write32(priv->cpdma_sr_base + SR_TX0_CP, CPPI_TX_BD);
    mmio_write32(priv->cpdma_base + CPDMA_EOI_VECTOR, CPDMA_EOI_TX);
    netif_wake_queue(priv->ndev);
}

static void cpsw_read_mac(unsigned char *addr)
{
    uint32_t lo = mmio_read32(CTRL_MAC_ID0_LO);
    uint32_t hi = mmio_read32(CTRL_MAC_ID0_HI);

    addr[0] =  hi        & 0xFF;
    addr[1] = (hi >>  8) & 0xFF;
    addr[2] = (hi >> 16) & 0xFF;
    addr[3] = (hi >> 24) & 0xFF;
    addr[4] =  lo        & 0xFF;
    addr[5] = (lo >>  8) & 0xFF;
}

static int omap_cpsw_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int rx_irq = platform_get_irq(pdev, 0);
    uint32_t base;
    struct cpsw_priv *priv = &omap_cpsw_priv;
    struct net_device *ndev = &omap_cpsw_ndev;
    struct mdio_bus *mbus;

    pr_info("[CPSW] probing %s\n", pdev->name);

    cpsw_pinmux_setup();

    if (!mem) return -EINVAL;

    base = mem->start;
    priv->ss_base       = base;
    priv->port_base     = base + 0x100;
    priv->cpdma_base    = base + 0x800;
    priv->cpdma_sr_base = base + 0xA00;
    priv->ale_base      = base + 0xD00;
    priv->sl1_base      = base + 0xD80;
    priv->wr_base       = base + 0x1200;
    priv->ndev          = ndev;

    if (cpsw_hw_reset(priv)) {
        pr_err("[CPSW] hw reset failed\n");
        return -EIO;
    }

    cpsw_read_mac(ndev->dev_addr);
    pr_info("[CPSW] MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
            ndev->dev_addr[0], ndev->dev_addr[1], ndev->dev_addr[2],
            ndev->dev_addr[3], ndev->dev_addr[4], ndev->dev_addr[5]);

    mbus = mdio_get_bus(0);
    if (mbus) {
        mdio_enable(mbus);
        priv->phy = phy_probe(mbus, 0);
        if (priv->phy) {
            phy_init(priv->phy);
            phy_update_link(priv->phy);
            pr_info("[CPSW] PHY link=%d\n", priv->phy->link);
        }
    }

    cpsw_ale_init(priv);
    cpsw_sl_init(priv);

    if (cpsw_cpdma_init(priv)) return -EIO;
    cpsw_rx_ring_init(priv);
    cpsw_bd_init(CPPI_TX_BD, 0, CPPI_TX_BUF, 0, 0);

    /* Enable stats for CPU port and slave port 1 */
    mmio_write32(priv->ss_base + SS_STAT_PORT_EN, 0x3);

    /* Select MII mode for port 1 */
    uint32_t sel = mmio_read32(CTRL_GMII_SEL);
    sel = (sel & ~GMII1_SEL_MASK) | GMII1_SEL_MII;
    mmio_write32(CTRL_GMII_SEL, sel);

    ndev->mtu        = 1500;
    ndev->netdev_ops = &cpsw_netdev_ops;
    ndev->priv       = priv;

    if (register_netdev(ndev)) {
        pr_err("[CPSW] register_netdev failed\n");
        return -ENODEV;
    }

    /* Auto-up interface for immediate network functionality */
    if (cpsw_ndo_open(ndev)) {
        pr_err("[CPSW] auto-up interface failed\n");
        return -EIO;
    }

    if (request_irq(rx_irq, cpsw_rx_irq, 0, "cpsw-rx", priv) != 0) {
        pr_err("[CPSW] request_irq RX failed\n");
        return -EIO;
    }
    enable_irq(rx_irq);

    if (request_irq(PLATFORM_IRQ_CPSW_TX, cpsw_tx_irq, 0, "cpsw-tx", priv) != 0) {
        pr_err("[CPSW] request_irq TX failed\n");
        return -EIO;
    }
    enable_irq(PLATFORM_IRQ_CPSW_TX);

    pr_info("[CPSW] probe done, %s ready\n", ndev->name);
    return 0;
}

static int omap_cpsw_remove(struct platform_device *pdev)
{
    (void)pdev;
    return 0;
}

static struct platform_driver omap_cpsw_driver = {
    .drv    = { .name = "omap-cpsw" },
    .probe  = omap_cpsw_probe,
    .remove = omap_cpsw_remove,
};

static int __init omap_cpsw_driver_init(void)
{
    return platform_driver_register(&omap_cpsw_driver);
}
device_initcall(omap_cpsw_driver_init);
