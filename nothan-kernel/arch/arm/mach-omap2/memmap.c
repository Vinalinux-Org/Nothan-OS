/*
 * arch/arm/mach-omap2/memmap.c — AM335x peripheral region table
 *
 * Defines platform_peripheral_map[], which lists the physical address
 * ranges that the MMU maps as strongly-ordered identity sections
 * during boot.
 */

#include "mach/memmap.h"

const struct platform_peripheral_region platform_peripheral_map[] = {
    { PLATFORM_PERIPH_L4_WKUP_PA,  PLATFORM_PERIPH_L4_WKUP_SECTIONS,  "L4_WKUP" },
    { PLATFORM_PERIPH_USBSS_PA,    PLATFORM_PERIPH_USBSS_SECTIONS,    "USBSS"   },
    { PLATFORM_PERIPH_L4_PER_PA,   PLATFORM_PERIPH_L4_PER_SECTIONS,   "L4_PER"  },
    { PLATFORM_PERIPH_L4_FAST_PA,  PLATFORM_PERIPH_L4_FAST_SECTIONS,  "L4_FAST" },
};

const int platform_peripheral_map_count =
    sizeof(platform_peripheral_map) / sizeof(platform_peripheral_map[0]);
