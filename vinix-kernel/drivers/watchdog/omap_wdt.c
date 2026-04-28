/*
 * AM335x WDT1 watchdog driver — disables the on-reset-running watchdog.
 *
 * AM335x TRM Ch.20.
 */

#include "watchdog.h"
#include "mmio.h"
#include "mach/prcm.h"

#define WDT1_BASE               0x44E35000

#define WDT_WWPS                (WDT1_BASE + 0x34)
#define WDT_WSPR                (WDT1_BASE + 0x48)

#define WDT_DISABLE_SEQ1        0xAAAA
#define WDT_DISABLE_SEQ2        0x5555

/*
 * The AM335x WDT1 starts running out of reset; if the kernel never
 * services it the SoC reboots after ~83s. We disable it here because
 * VinixOS does not yet implement a watchdog kicker.
 *
 * Sequence per TRM 20.4.3.8: enable clock → write SEQ1 → wait WWPS
 * idle → write SEQ2 → wait WWPS idle.
 */
void watchdog_disable(void)
{
    mmio_write32(CM_WKUP_WDT1_CLKCTRL, MODULEMODE_ENABLE);
    while ((mmio_read32(CM_WKUP_WDT1_CLKCTRL) & MODULEMODE_MASK) != MODULEMODE_ENABLE)
        ;

    mmio_write32(WDT_WSPR, WDT_DISABLE_SEQ1);
    while (mmio_read32(WDT_WWPS))
        ;

    mmio_write32(WDT_WSPR, WDT_DISABLE_SEQ2);
    while (mmio_read32(WDT_WWPS))
        ;
}