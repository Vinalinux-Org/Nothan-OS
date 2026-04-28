/*
 * AM335x WDT1 watchdog driver — disables the on-reset-running watchdog.
 *
 * AM335x TRM Ch.20.
 */

#include "mmio.h"
#include "platform_device.h"
#include "vinix/init.h"
#include "vinix/printk.h"
#include "vinix/errno.h"
#include "mach/prcm.h"

#define WDT_WWPS                0x34
#define WDT_WSPR                0x48

#define WDT_DISABLE_SEQ1        0xAAAA
#define WDT_DISABLE_SEQ2        0x5555

/*
 * The AM335x WDT1 starts running out of reset; if the kernel never
 * services it the SoC reboots after ~83s. We disable it because
 * VinixOS does not yet implement a watchdog kicker.
 *
 * Sequence per TRM 20.4.3.8: enable clock → write SEQ1 → wait WWPS
 * idle → write SEQ2 → wait WWPS idle.
 */
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

/* arch_initcall — must disable WDT1 early in boot (TRM: ROM leaves it
 * running, ~83 s timeout). Runs after board file (core_initcall) so
 * the omap-wdt platform_device is already on the bus. */
static int __init omap_wdt_driver_init(void)
{
    return platform_driver_register(&omap_wdt_driver);
}
arch_initcall(omap_wdt_driver_init);
