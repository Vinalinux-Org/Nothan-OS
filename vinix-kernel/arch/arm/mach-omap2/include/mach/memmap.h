/* arch/arm/mach-omap2/include/mach/memmap.h — AM3358 peripheral memory map. */

#ifndef PLATFORM_MEMMAP_H
#define PLATFORM_MEMMAP_H

#include "types.h"

#define PLATFORM_PERIPH_L4_WKUP_PA        0x44E00000
#define PLATFORM_PERIPH_L4_WKUP_SECTIONS  1

#define PLATFORM_PERIPH_L4_PER_PA         0x48000000
#define PLATFORM_PERIPH_L4_PER_SECTIONS   4

#define PLATFORM_PERIPH_USB0_PA           0x47400000
#define PLATFORM_PERIPH_USB0_SECTIONS     1

/* Individual peripheral base addresses.
 * Drivers obtain their base from platform_get_resource() in probe().
 * These constants are used only in early boot paths where the platform
 * bus is not yet available (e.g. the early UART console). */
#define OMAP_UART0_BASE     0x44E09000
#define OMAP_WDT1_BASE      0x44E35000
#define OMAP_MMC0_BASE      0x48060000
#define OMAP_I2C0_BASE      0x44E0B000
#define OMAP_DMTIMER2_BASE  0x48040000
#define OMAP_INTC_BASE      0x48200000
#define OMAP_LCDC_BASE      0x4830E000
#define OMAP_USBSS_BASE     0x47400000
#define OMAP_USB0_CTRL_BASE 0x47401000
#define OMAP_USB0_CORE_BASE 0x47401400
#define OMAP_USB0_PHY_BASE  0x47401300
#define OMAP_USB1_CTRL_BASE 0x47401800
#define OMAP_USB1_CORE_BASE 0x47401C00
#define OMAP_USB1_PHY_BASE  0x47401B00

struct platform_peripheral_region {
    uint32_t pa;
    uint32_t sections;
    const char *name;
};

extern const struct platform_peripheral_region platform_peripheral_map[];
extern const int platform_peripheral_map_count;

#endif
