/* ============================================================
 * memmap.c
 * ------------------------------------------------------------
 * AM3358 peripheral region table.
 * ============================================================ */

#include "mach/memmap.h"

const struct platform_peripheral_region platform_peripheral_map[] = {
    { PLATFORM_PERIPH_L4_WKUP_PA, PLATFORM_PERIPH_L4_WKUP_SECTIONS, "L4_WKUP" },
    { PLATFORM_PERIPH_L4_PER_PA,  PLATFORM_PERIPH_L4_PER_SECTIONS,  "L4_PER"  },
};

const int platform_peripheral_map_count =
    sizeof(platform_peripheral_map) / sizeof(platform_peripheral_map[0]);
