/*
 * arch/arm/mach-omap2/board-bbb.c — BeagleBone Black platform device table
 *
 * Registers the following platform devices with the platform bus:
 *   omap-uart    @ 0x44E09000  (UART0 console)
 *   omap-intc    @ 0x48200000  (interrupt controller)
 *   omap-dmtimer @ 0x48040000  (DMTimer2)
 *   omap-hsmmc   @ 0x48060000  (MMC0)
 *   omap-wdt     @ 0x44E35000  (WDT1)
 *   omap-i2c     @ 0x44E0B000  (I2C0 — TDA19988 HDMI, PMIC, EEPROM)
 *   tilcdc       @ 0x4830E000  (LCDC + TDA19988 HDMI bridge)
 *   omap-mdio    @ 0x4A101000  (MDIO bus controller)
 *   omap-cpsw    @ 0x4A100000  (Ethernet switch)
 *   omap-gpio    @ 0x44E07000/4804C000/481AC000/481AE000  (GPIO banks 0-3)
 *
 * Each driver's probe() function is called by the platform bus when its
 * matching name is found in this table.
 */

#include "platform_device.h"
#include "mach/irqs.h"
#include "nothan/init.h"

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

static struct platform_device omap_wdt = {
    .name   = "omap-wdt",
    .base   = 0x44E35000,
    .irq    = 0,
    .clk_id = "wdt1",
};

static struct platform_device omap_i2c0 = {
    .name   = "omap-i2c",
    .base   = 0x44E0B000,
    .irq    = 0,                /* polling mode */
    .clk_id = "i2c0",
};

static struct platform_device omap_tilcdc = {
    .name   = "tilcdc",
    .base   = 0x4830E000,
    .irq    = 0,                /* polled, no IRQ used yet */
    .clk_id = "lcdc",
};

static struct platform_device omap_mdio0 = {
    .name   = "omap-mdio",
    .base   = 0x4A101000,
    .irq    = 0,
    .clk_id = "cpgmac0",
};

static struct platform_device omap_cpsw0 = {
    .name   = "omap-cpsw",
    .base   = 0x4A100000,
    .irq    = PLATFORM_IRQ_CPSW_RX,
    .clk_id = "cpgmac0",
};

static struct platform_device omap_gpio0 = {
    .name   = "omap-gpio",
    .base   = 0x44E07000,
    .irq    = 0,    /* not used */
    .clk_id = "gpio0",
};

static struct platform_device omap_gpio1 = {
    .name   = "omap-gpio",
    .base   = 0x4804C000,
    .irq    = 0,    /* not used */
    .clk_id = "gpio1",
};

static struct platform_device omap_gpio2 = {
    .name   = "omap-gpio",
    .base   = 0x481AC000,
    .irq    = 0,    /* not used */
    .clk_id = "gpio2",
};

static struct platform_device omap_gpio3 = {
    .name   = "omap-gpio",
    .base   = 0x481AE000,
    .irq    = 0,    /* not used */
    .clk_id = "gpio3",
};

static struct platform_device omap_musb = {
    .name   = "omap-musb",
    .base   = 0x47400000,
    .irq    = PLATFORM_IRQ_USB1,
    .clk_id = "usb",
};

static struct platform_device *bbb_devices[] = {
    &omap_intc,
    &omap_uart0,
    &omap_i2c0,
    &omap_dmtimer2,
    &omap_hsmmc0,
    &omap_wdt,
    &omap_tilcdc,
    &omap_mdio0,
    &omap_cpsw0,
    &omap_gpio0,
    &omap_gpio1,
    &omap_gpio2,
    &omap_gpio3,
    &omap_musb,
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
