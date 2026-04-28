/* ============================================================
 * board-bbb.c
 * ------------------------------------------------------------
 * BeagleBone Black platform bus table — registers omap-uart,
 * omap-intc, omap-dmtimer, omap-hsmmc with the platform bus.
 * ============================================================ */

#include "platform_device.h"
#include "mach/irqs.h"
#include "vinix/init.h"

static struct platform_device omap_uart0 = {
    .name   = "omap-uart",
    .base   = 0x44E09000,
    .irq    = PLATFORM_IRQ_UART0,
    .clk_id = "uart0",
};

static struct platform_device omap_intc = {
    .name   = "omap-intc",
    .base   = 0x48200000,
    .irq    = 0,
    .clk_id = 0,
};

static struct platform_device omap_dmtimer2 = {
    .name   = "omap-dmtimer",
    .base   = 0x48040000,
    .irq    = PLATFORM_IRQ_DMTIMER2,
    .clk_id = "timer2",
};

static struct platform_device omap_hsmmc0 = {
    .name   = "omap-hsmmc",
    .base   = 0x48060000,
    .irq    = PLATFORM_IRQ_MMC0,
    .clk_id = "mmc0",
};

static struct platform_device *bbb_devices[] = {
    &omap_uart0,
    &omap_intc,
    &omap_dmtimer2,
    &omap_hsmmc0,
};

static int __init bbb_platform_init(void)
{
    bus_register(&platform_bus);
    for (unsigned i = 0; i < sizeof(bbb_devices) / sizeof(bbb_devices[0]); i++) {
        platform_device_register(bbb_devices[i]);
    }
    return 0;
}
core_initcall(bbb_platform_init);
