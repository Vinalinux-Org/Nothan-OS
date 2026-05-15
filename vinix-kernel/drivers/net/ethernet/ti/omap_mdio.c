/*
 * drivers/net/ethernet/ti/omap_mdio.c — AM335x MDIO bus driver
 *
 * MDIO bus controller for CPSW Ethernet subsystem. Provides mdio_read()
 * and mdio_write() for the omap_cpsw driver to manage the LAN8710A PHY.
 */

#include "types.h"
#include "mmio.h"
#include "uart.h"
#include "platform_device.h"
#include "vinix/init.h"
#include "vinix/errno.h"
#include "vinix/mdio.h"
#include "vinix/phy.h"
#include "mach/prcm.h"
#include "mach/control.h"

/* Pinmux — conf_mdio_data/clk: mode 0, input enable, pullup */
#define CONF_MDIO_DATA  (CTRL_MODULE_BASE + 0x948)
#define CONF_MDIO_CLK   (CTRL_MODULE_BASE + 0x94C)
#define MDIO_PIN_CFG    (PIN_INPUT_EN | PIN_PULLUP_EN)

/* Register offsets */
#define MDIO_VERSION     0x00
#define MDIO_CONTROL     0x04
#define MDIO_ALIVE       0x08
#define MDIO_USERACCESS0 0x80

/* MDIO_CONTROL bits */
#define CTRL_ENABLE      (1 << 30)

/* MDIO_USERACCESS0 bits */
#define UA_GO            (1 << 31)
#define UA_WRITE         (1 << 30)
#define UA_ACK           (1 << 29)
#define UA_DATA_MASK     0xFFFF

/* CPSW module clock = 125 MHz; target MDIO = 2.2 MHz → div = 55 */
#define MDIO_CLKDIV      55
#define MDIO_TIMEOUT     10000
#define MAX_MDIO_BUSES   1

static struct mdio_bus  omap_mdio_bus = { .name = "omap-mdio0" };
static struct mdio_bus *bus_table[MAX_MDIO_BUSES];
static int              bus_count = 0;

/* mdio-core */

int mdio_add_bus(struct mdio_bus *bus)
{
    if (!bus || bus_count >= MAX_MDIO_BUSES) return -EINVAL;
    bus_table[bus_count++] = bus;
    pr_info("[MDIO] bus %d (%s) registered\n", bus_count - 1, bus->name);
    return 0;
}

struct mdio_bus *mdio_get_bus(int nr)
{
    if (nr < 0 || nr >= bus_count) return 0;
    return bus_table[nr];
}

/* mdio read/write */

int mdio_read(struct mdio_bus *bus, uint8_t phy_addr, uint8_t reg)
{
    uint32_t base = bus->base;
    uint32_t val;
    int timeout;

    timeout = MDIO_TIMEOUT;
    while ((mmio_read32(base + MDIO_USERACCESS0) & UA_GO) && timeout--)
        ;
    if (!timeout) return -EIO;

    mmio_write32(base + MDIO_USERACCESS0,
                 UA_GO | ((uint32_t)reg << 21) | ((uint32_t)phy_addr << 16));

    timeout = MDIO_TIMEOUT;
    while ((mmio_read32(base + MDIO_USERACCESS0) & UA_GO) && timeout--)
        ;
    if (!timeout) return -EIO;

    val = mmio_read32(base + MDIO_USERACCESS0);
    if (!(val & UA_ACK)) return -EIO;

    return (int)(val & UA_DATA_MASK);
}

int mdio_write(struct mdio_bus *bus, uint8_t phy_addr, uint8_t reg, uint16_t data)
{
    uint32_t base = bus->base;
    int timeout;

    timeout = MDIO_TIMEOUT;
    while ((mmio_read32(base + MDIO_USERACCESS0) & UA_GO) && timeout--)
        ;
    if (!timeout) return -EIO;

    mmio_write32(base + MDIO_USERACCESS0,
                 UA_GO | UA_WRITE | ((uint32_t)reg << 21) |
                 ((uint32_t)phy_addr << 16) | (data & UA_DATA_MASK));

    timeout = MDIO_TIMEOUT;
    while ((mmio_read32(base + MDIO_USERACCESS0) & UA_GO) && timeout--)
        ;
    return timeout ? 0 : -EIO;
}

static struct phy_device g_phy;

struct phy_device *phy_probe(struct mdio_bus *bus, uint8_t addr)
{
    int id1 = mdio_read(bus, addr, MII_PHYSID1);
    int id2 = mdio_read(bus, addr, MII_PHYSID2);
    if (id1 < 0 || id2 < 0) {
        pr_err("[MDIO] phy_probe: no ACK at addr %d\n", addr);
        return NULL;
    }
    pr_info("[MDIO] PHY addr=%d id=%04x:%04x\n", addr, id1, id2);
    g_phy.bus    = bus;
    g_phy.addr   = addr;
    g_phy.link   = 0;
    g_phy.speed  = 0;
    g_phy.duplex = 0;
    return &g_phy;
}

int phy_init(struct phy_device *phy)
{
    int val;
    int timeout;

    mdio_write(phy->bus, phy->addr, MII_BMCR, BMCR_RESET);
    timeout = MDIO_TIMEOUT;
    while (timeout--) {
        val = mdio_read(phy->bus, phy->addr, MII_BMCR);
        if (val < 0)             return -EIO;
        if (!(val & BMCR_RESET)) break;
    }
    if (!timeout) {
        pr_err("[MDIO] PHY reset timeout\n");
        return -EIO;
    }
    mdio_write(phy->bus, phy->addr, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);
    pr_info("[MDIO] PHY reset OK, auto-neg started\n");
    return 0;
}

int phy_update_link(struct phy_device *phy)
{
    int bmsr;

    mdio_read(phy->bus, phy->addr, MII_BMSR);      /* clear latched bits */
    bmsr = mdio_read(phy->bus, phy->addr, MII_BMSR);
    if (bmsr < 0) return -EIO;
    phy->link = (bmsr & BMSR_LSTATUS) ? 1 : 0;
    return 0;
}


void mdio_enable(struct mdio_bus *bus)
{
    mmio_write32(bus->base + MDIO_CONTROL, MDIO_CLKDIV | CTRL_ENABLE);
    pr_info("[MDIO] re-enabled, CONTROL=0x%08x ALIVE=0x%08x\n",
            mmio_read32(bus->base + MDIO_CONTROL),
            mmio_read32(bus->base + MDIO_ALIVE));
}

static int omap_mdio_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    uint32_t base;
    uint32_t val;
    int timeout;

    pr_info("[MDIO] probing %s @ 0x%08x\n",
            pdev->name, mem ? mem->start : 0);

    if (!mem)
        return -EINVAL;

    base = mem->start;

    /* Clock enable — CM_PER_CPGMAC0_CLKCTRL */
    mmio_write32(CM_PER_CPGMAC0_CLKCTRL, MODULEMODE_ENABLE);
    timeout = MDIO_TIMEOUT;
    while (timeout--) {
        if ((mmio_read32(CM_PER_CPGMAC0_CLKCTRL) & IDLEST_MASK) == IDLEST_FUNCTIONAL)
            break;
    }
    if (!timeout) {
        pr_err("[MDIO] clock enable timeout\n");
        return -EIO;
    }
    pr_info("[MDIO] clock enabled, clkctrl=0x%08x\n",
            mmio_read32(CM_PER_CPGMAC0_CLKCTRL));

    /* Pinmux — mode 0 */
    mmio_write32(CONF_MDIO_DATA, MDIO_PIN_CFG);
    mmio_write32(CONF_MDIO_CLK,  MDIO_PIN_CFG);

    /* Enable MDIO controller with clock divisor */
    mmio_write32(base + MDIO_CONTROL, MDIO_CLKDIV | CTRL_ENABLE);
    pr_info("[MDIO] control=0x%08x version=0x%08x\n",
            mmio_read32(base + MDIO_CONTROL),
            mmio_read32(base + MDIO_VERSION));

    val = mmio_read32(base + MDIO_ALIVE);
    pr_info("[MDIO] ALIVE=0x%08x%s\n", val,
            (val & 1) ? " — PHY at addr 0x0 detected" : " — no PHY");

    omap_mdio_bus.base = base;
    mdio_add_bus(&omap_mdio_bus);

    return 0;
}

static int omap_mdio_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver omap_mdio_driver = {
    .drv    = { .name = "omap-mdio" },
    .probe  = omap_mdio_probe,
    .remove = omap_mdio_remove,
};

static int __init omap_mdio_driver_init(void)
{
    return platform_driver_register(&omap_mdio_driver);
}
subsys_initcall(omap_mdio_driver_init);
