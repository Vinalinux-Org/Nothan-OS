/*
 * bootloader/src/clock.c - AM335x DPLL and peripheral-clock setup
 *
 * Crystal: 24 MHz
 *
 * DPLL targets (offsets verified against U-Boot clock_am33xx.c):
 *   MPU  : 24 * 125 / (2+1) / 1  = 1000 MHz
 *   CORE : 24 * 125 / (2+1)      = 1000 MHz  (M4/10=100, M5/8=125, M6/4=250)
 *   PER  : 24 * 400 / (9+1) / 5  =  192 MHz  (/4 -> 48 MHz to peripherals)
 *   DDR  : 24 * 400 / (23+1) / 1 =  400 MHz
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "am335x.h"
#include "boot.h"

#define CLKTRCTRL_SW_WKUP	0x2
#define IDLEST_MASK		0x00030000	/* CLKCTRL bits [17:16]: 0=functional */
#define ST_MN_BYPASS		(1 << 8)
#define ST_DPLL_CLK		(1 << 0)
#define DPLL_TIMEOUT		100000

static void dpll_bypass(uint32_t clkmode, uint32_t idlest)
{
	uint32_t val = (readl(clkmode) & ~0x7) | DPLL_MN_BYP_MODE;
	int timeout = DPLL_TIMEOUT;

	writel(val, clkmode);
	while (!(readl(idlest) & ST_MN_BYPASS) && --timeout)
		;
}

static void dpll_set_mn(uint32_t clksel, uint32_t m, uint32_t n)
{
	uint32_t val = readl(clksel) & ~0x7FFFF;

	val |= (m << 8) | n;
	writel(val, clksel);
}

static void dpll_set_div(uint32_t reg, uint32_t div)
{
	writel((readl(reg) & ~0x1F) | div, reg);
}

static void dpll_lock(uint32_t clkmode, uint32_t idlest)
{
	uint32_t val = (readl(clkmode) & ~0x7) | DPLL_LOCK_MODE;
	int timeout = DPLL_TIMEOUT;

	writel(val, clkmode);
	while (!(readl(idlest) & ST_DPLL_CLK) && --timeout)
		;
}

static void config_mpu_pll(void)
{
	dpll_bypass(CM_CLKMODE_DPLL_MPU, CM_IDLEST_DPLL_MPU);
	dpll_set_mn(CM_CLKSEL_DPLL_MPU, 125, 2);
	dpll_set_div(CM_DIV_M2_DPLL_MPU, 1);
	dpll_lock(CM_CLKMODE_DPLL_MPU, CM_IDLEST_DPLL_MPU);
}

static void config_core_pll(void)
{
	dpll_bypass(CM_CLKMODE_DPLL_CORE, CM_IDLEST_DPLL_CORE);
	dpll_set_mn(CM_CLKSEL_DPLL_CORE, 125, 2);
	dpll_set_div(CM_DIV_M4_DPLL_CORE, 10);
	dpll_set_div(CM_DIV_M5_DPLL_CORE, 8);
	dpll_set_div(CM_DIV_M6_DPLL_CORE, 4);
	dpll_lock(CM_CLKMODE_DPLL_CORE, CM_IDLEST_DPLL_CORE);
}

static void config_per_pll(void)
{
	dpll_bypass(CM_CLKMODE_DPLL_PER, CM_IDLEST_DPLL_PER);
	dpll_set_mn(CM_CLKSEL_DPLL_PER, 400, 9);
	dpll_set_div(CM_DIV_M2_DPLL_PER, 5);
	dpll_lock(CM_CLKMODE_DPLL_PER, CM_IDLEST_DPLL_PER);
}

static void config_ddr_pll(void)
{
	dpll_bypass(CM_CLKMODE_DPLL_DDR, CM_IDLEST_DPLL_DDR);
	dpll_set_mn(CM_CLKSEL_DPLL_DDR, 400, 23);
	dpll_set_div(CM_DIV_M2_DPLL_DDR, 1);
	dpll_lock(CM_CLKMODE_DPLL_DDR, CM_IDLEST_DPLL_DDR);
}

static void enable_clock_domains(void)
{
	writel(CLKTRCTRL_SW_WKUP, CM_WKUP_CLKSTCTRL);
	writel(CLKTRCTRL_SW_WKUP, CM_PER_L3_CLKSTCTRL);
	writel(CLKTRCTRL_SW_WKUP, CM_PER_L4LS_CLKSTCTRL);
	writel(CLKTRCTRL_SW_WKUP, CM_PER_L4FW_CLKSTCTRL);
}

static void enable_module(uint32_t reg)
{
	int timeout = DPLL_TIMEOUT;

	writel(MODULE_ENABLE, reg);
	while ((readl(reg) & IDLEST_MASK) != 0 && --timeout)
		;
}

void clock_domains_early_init(void)
{
	enable_clock_domains();
}

void clock_init(void)
{
	config_mpu_pll();
	config_core_pll();
	config_per_pll();
	config_ddr_pll();
	enable_module(CM_WKUP_UART0_CLKCTRL);
	enable_module(CM_PER_EMIF_CLKCTRL);
	enable_module(CM_PER_MMC0_CLKCTRL);
}
