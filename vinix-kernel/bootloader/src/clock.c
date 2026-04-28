/* ============================================================
 * clock.c
 * ------------------------------------------------------------
 * AM335x PLL and peripheral-clock setup.
 * ============================================================ */

#include "am335x.h"
#include "boot.h"

/* DDR PLL: 24 MHz crystal × 400 / (23+1) / 1 = 400 MHz for DDR3. */
static void config_ddr_pll(void)
{
    uint32_t clkmode, clksel, div_m2;

    /* DPLL spec: must enter MN-bypass before reprogramming M/N. */
    clkmode = readl(CM_CLKMODE_DPLL_DDR);
    clkmode = (clkmode & ~0x7) | DPLL_MN_BYP_MODE;
    writel(clkmode, CM_CLKMODE_DPLL_DDR);

    for (int i = 0; i < 10000; i++) {
        if (readl(CM_IDLEST_DPLL_DDR) & 0x00000100) break; /* ST_MN_BYPASS */
    }

    clksel = readl(CM_CLKSEL_DPLL_DDR);
    clksel &= ~0x7FFFF;
    clksel |= (400 << 8) | 23;   /* M @ [18:8], N @ [6:0] */
    writel(clksel, CM_CLKSEL_DPLL_DDR);

    div_m2 = readl(CM_DIV_M2_DPLL_DDR);
    div_m2 = (div_m2 & ~0x1F) | 1;
    writel(div_m2, CM_DIV_M2_DPLL_DDR);

    clkmode = readl(CM_CLKMODE_DPLL_DDR);
    clkmode = (clkmode & ~0x7) | DPLL_LOCK_MODE;
    writel(clkmode, CM_CLKMODE_DPLL_DDR);

    for (int i = 0; i < 10000; i++) {
        if (readl(CM_IDLEST_DPLL_DDR) & 0x1) break;        /* ST_DPLL_CLK */
    }
}

static void enable_peripheral_clocks(void)
{
    writel(MODULE_ENABLE, CM_PER_EMIF_CLKCTRL);
    while ((readl(CM_PER_EMIF_CLKCTRL) & 0x30000) != 0);

    writel(MODULE_ENABLE, CM_PER_MMC0_CLKCTRL);
    while ((readl(CM_PER_MMC0_CLKCTRL) & 0x30000) != 0);

    writel(MODULE_ENABLE, CM_WKUP_UART0_CLKCTRL);
    while ((readl(CM_WKUP_UART0_CLKCTRL) & 0x30000) != 0);
}

/* CRITICAL: must not call UART here — caller owns UART logging.
 * BootROM already configured MPU/PER/CORE PLLs; only DDR is left. */
void clock_init(void)
{
    config_ddr_pll();
    enable_peripheral_clocks();
}
