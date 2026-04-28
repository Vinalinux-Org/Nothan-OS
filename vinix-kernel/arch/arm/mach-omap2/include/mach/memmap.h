/* ============================================================
 * platform/memmap.h
 * ------------------------------------------------------------
 * AM3358 peripheral memory map.
 * ============================================================ */

#ifndef PLATFORM_MEMMAP_H
#define PLATFORM_MEMMAP_H

#include "types.h"

/* AM335x TRM Ch.02 — peripheral bus base addresses. */
#define PLATFORM_PERIPH_L4_WKUP_PA        0x44E00000
#define PLATFORM_PERIPH_L4_WKUP_SECTIONS  1

#define PLATFORM_PERIPH_L4_PER_PA         0x48000000
#define PLATFORM_PERIPH_L4_PER_SECTIONS   4

/* Individual peripheral bases — TRM Ch.02 memory map.
 * Drivers use these ONLY when platform_get_resource() is not yet
 * available (early console, pre-probe paths). Normal drivers get
 * their base from platform_get_resource() in probe(). */
#define OMAP_UART0_BASE     0x44E09000
#define OMAP_WDT1_BASE      0x44E35000
#define OMAP_MMC0_BASE      0x48060000
#define OMAP_I2C0_BASE      0x44E0B000
#define OMAP_DMTIMER2_BASE  0x48040000
#define OMAP_INTC_BASE      0x48200000
#define OMAP_LCDC_BASE      0x4830E000

struct platform_peripheral_region {
    uint32_t pa;
    uint32_t sections;
    const char *name;
};

extern const struct platform_peripheral_region platform_peripheral_map[];
extern const int platform_peripheral_map_count;

#endif
