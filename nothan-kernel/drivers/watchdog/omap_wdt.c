/*
 * drivers/watchdog/omap_wdt.c
 *
 * AM335x WDT1 disable driver — ROM leaves WDT1 running at reset; no kicker in v1.
 */

#include "mmio.h"
#include "platform_device.h"
#include "nothan/init.h"
#include "nothan/printk.h"
#include "nothan/errno.h"
#include "mach/prcm.h"

#define WDT_WWPS                0x34
#define WDT_WSPR                0x48

#define WDT_DISABLE_SEQ1        0xAAAA
#define WDT_DISABLE_SEQ2        0x5555

/* Two-write magic sequence; poll WWPS between writes or the second write is lost. */
static void omap_wdt_hw_disable(uint32_t base)
{
    mmio_write32(CM_WKUP_WDT1_CLKCTRL, MODULEMODE_ENABLE);
    while ((mmio_read32(CM_WKUP_WDT1_CLKCTRL) & MODULEMODE_MASK) != MODULEMODE_ENABLE)
        ;

    mmio_write32(base + WDT_WSPR, WDT_DISABLE_SEQ1);
    while (mmio_read32(base + WDT_WWPS))
        ;

    mmio_write32(base + WDT_WSPR, WDT_DISABLE_SEQ2);
    while (mmio_read32(base + WDT_WWPS))
        ;
}

static int omap_wdt_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!mem)
        return -ENODEV;

    omap_wdt_hw_disable(mem->start);
    pr_info("[WDT] disabled @ 0x%08x\n", mem->start);
    return 0;
}

static struct platform_driver omap_wdt_driver = {
    .drv.name = "omap-wdt",
    .probe    = omap_wdt_probe,
};

/* arch_initcall: must fire before ~83s ROM timeout. */
static int __init omap_wdt_driver_init(void)
{
    return platform_driver_register(&omap_wdt_driver);
}
arch_initcall(omap_wdt_driver_init);
