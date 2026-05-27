/* arch/arm/mach-omap2/include/mach/memmap.h — AM3358 peripheral memory map. */

#ifndef PLATFORM_MEMMAP_H
#define PLATFORM_MEMMAP_H

#include "types.h"

#define PLATFORM_PERIPH_L4_WKUP_PA        0x44E00000
#define PLATFORM_PERIPH_L4_WKUP_SECTIONS  1

#define PLATFORM_PERIPH_L4_PER_PA         0x48000000
#define PLATFORM_PERIPH_L4_PER_SECTIONS   4

#define PLATFORM_PERIPH_L4_FAST_PA        0x4A000000
#define PLATFORM_PERIPH_L4_FAST_SECTIONS  2

#define PLATFORM_PERIPH_USB_PA            0x47400000
#define PLATFORM_PERIPH_USB_SECTIONS      1

/* Individual peripheral base addresses.
 * Drivers obtain their base from platform_get_resource() in probe().
 * These constants are used only in early boot paths where the platform
 * bus is not yet available (e.g. the early UART console). */
#define OMAP_UART0_BASE           0x44E09000
#define OMAP_WDT1_BASE            0x44E35000
#define OMAP_MMC0_BASE            0x48060000
#define OMAP_I2C0_BASE            0x44E0B000
#define OMAP_DMTIMER2_BASE        0x48040000
#define OMAP_INTC_BASE            0x48200000
#define OMAP_LCDC_BASE            0x4830E000
#define OMAP_CTRL_MODULE_BASE     0x44E10000

/* L4_FAST */
#define OMAP_CPSW_SS_BASE         0x4A100000
#define OMAP_CPSW_MDIO_BASE       0x4A101000

/* GPIO banks — 4 independent hardware instances */
#define OMAP_GPIO0_BASE           0x44E07000
#define OMAP_GPIO1_BASE           0x4804C000
#define OMAP_GPIO2_BASE           0x481AC000
#define OMAP_GPIO3_BASE           0x481AE000

struct platform_peripheral_region {
    uint32_t pa;
    uint32_t sections;
    const char *name;
};

extern const struct platform_peripheral_region platform_peripheral_map[];
extern const int platform_peripheral_map_count;

#endif
