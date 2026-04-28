/*
 * <peripheral> driver — one-line description.
 *
 * Template for new platform drivers. Copy this file into
 * vinix-kernel/drivers/<subsystem>/<name>.c and replace every
 * "TEMPLATE:" tag with the real value.
 *
 * Reference: drivers/tty/serial/omap_serial.c (gold standard).
 * AM335x TRM Ch.<N>.
 */

#include "types.h"
#include "mmio.h"
#include "platform_device.h"
#include "vinix/init.h"
#include "vinix/printk.h"

/* TEMPLATE: subsystem header — cdev, blkdev, serial_core, mmc/host, ... */
/* #include "vinix/<class>.h" */

/* TEMPLATE: pull cross-cutting SoC defs from mach/, NOT from a local
 * #define. Drop the line if the driver does not touch PRCM / pinmux. */
#include "mach/prcm.h"
/* #include "mach/control.h" */

/* TEMPLATE: peripheral-internal register offsets — these describe the
 * peripheral itself, so they live in the driver. */
#define XXX_CTRL                0x00
#define XXX_STATUS              0x04
#define XXX_DATA                0x08

/* TEMPLATE: bit fields for the registers above. */
#define XXX_CTRL_ENABLE         (1 << 0)
#define XXX_STATUS_READY        (1 << 0)

struct xxx_priv {
    void   *base;          /* MMIO base from platform_get_resource */
    int     irq;
    /* TEMPLATE: driver-private state */
};

static int xxx_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq              = platform_get_irq(pdev, 0);

    if (!mem)
        return -ENODEV;

    /* TEMPLATE: enable module clock via mach/prcm.h. Skip if peripheral
     * is always-on (e.g. INTC). */
    /* mmio_write32(CM_PER_XXX_CLKCTRL, MODULEMODE_ENABLE); */

    /* TEMPLATE: HW init — reset + configure registers. Reference TRM
     * sequence; keep WHY-comments only. */

    /* TEMPLATE: register with subsystem class:
     *   cdev_register(...)         — character device
     *   add_disk(...)              — block device
     *   register_netdev(...)       — network device
     *   uart_add_one_port(...)     — serial port
     *   mmc_add_host(...)          — MMC host
     *   register_framebuffer(...)  — framebuffer
     *   irqchip_register(...)      — IRQ controller
     *   clockevents_register_device(...)
     *   watchdog_register_device(...)
     */

    pr_info("[XXX] probed at 0x%x, irq %d\n", mem->start, irq);
    (void)irq;
    return 0;
}

static int xxx_remove(struct platform_device *pdev)
{
    (void)pdev;
    /* TEMPLATE: unwind probe — unregister from subsystem, free private
     * state, disable module clock. Safe to leave empty for MVP since
     * VinixOS does not unload drivers. */
    return 0;
}

static struct platform_driver xxx_driver = {
    .drv.name = "omap-xxx",          /* TEMPLATE: must match
                                        platform_device.name in
                                        arch/arm/mach-omap2/board-bbb.c */
    .probe    = xxx_probe,
    .remove   = xxx_remove,
};

/*
 * TEMPLATE: pick the right initcall level.
 *   arch_initcall    — lvl 3, early HW that printk / panic depend on
 *                       (uart, watchdog disable)
 *   subsys_initcall  — lvl 4, bus / IRQ controller (intc, i2c host)
 *   fs_initcall      — lvl 5, block storage (mmc)
 *   device_initcall  — lvl 6, normal device (display, timer)
 *   late_initcall    — lvl 7, selftests / sanity probes
 *
 * Convenience macro module_platform_driver() == device_initcall.
 */
static int __init xxx_driver_init(void)
{
    return platform_driver_register(&xxx_driver);
}
device_initcall(xxx_driver_init);


/*
 * TEMPLATE: corresponding entry to add to
 * vinix-kernel/arch/arm/mach-omap2/board-bbb.c
 *
 *   static struct platform_device omap_xxx0 = {
 *       .name   = "omap-xxx",     // must match xxx_driver.drv.name
 *       .base   = 0x4XXXXXXX,     // peripheral base from TRM Ch.02
 *       .irq    = PLATFORM_IRQ_XXX,
 *       .clk_id = "xxx0",
 *   };
 *
 * then add &omap_xxx0 to bbb_devices[].
 */
