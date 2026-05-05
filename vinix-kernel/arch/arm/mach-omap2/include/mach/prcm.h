/*
 * arch/arm/mach-omap2/include/mach/prcm.h
 *
 * AM335x PRCM register map — every driver must enable its module clock
 * here before accessing the peripheral's own registers.
 */

#ifndef _MACH_PRCM_H
#define _MACH_PRCM_H

/* CM_PER — Clock Module Peripheral domain (L4_PER) */
#define CM_PER_BASE                 0x44E00000UL

#define CM_PER_L4LS_CLKSTCTRL       (CM_PER_BASE + 0x000)
#define CM_PER_CPGMAC0_CLKCTRL      (CM_PER_BASE + 0x014)
#define CM_PER_LCDC_CLKCTRL         (CM_PER_BASE + 0x018)
#define CM_PER_MMC0_CLKCTRL         (CM_PER_BASE + 0x03C)
#define CM_PER_UART0_CLKCTRL        (CM_PER_BASE + 0x06C)
#define CM_PER_TIMER2_CLKCTRL       (CM_PER_BASE + 0x080)
#define CM_PER_CPSW_CLKSTCTRL       (CM_PER_BASE + 0x144)
#define CM_PER_LCDC_CLKSTCTRL       (CM_PER_BASE + 0x148)

/* CM_DPLL — Clock Module DPLL (PLL clock selection) */
#define CM_DPLL_BASE                0x44E00500UL

#define CM_DPLL_CLKSEL_TIMER2_CLK   (CM_DPLL_BASE + 0x008)

/* CM_WKUP — Clock Module Wakeup domain (L4_WKUP) */
#define CM_WKUP_BASE                0x44E00400UL

#define CM_IDLEST_DPLL_DISP         (CM_WKUP_BASE + 0x048)
#define CM_CLKSEL_DPLL_DISP         (CM_WKUP_BASE + 0x054)
#define CM_CLKMODE_DPLL_DISP        (CM_WKUP_BASE + 0x098)
#define CM_DIV_M2_DPLL_DISP         (CM_WKUP_BASE + 0x0A4)
#define CM_WKUP_I2C0_CLKCTRL        (CM_WKUP_BASE + 0x0B8)
#define CM_WKUP_WDT1_CLKCTRL        (CM_WKUP_BASE + 0x0D4)

/* CM_*_CLKCTRL — common bits */
#define MODULEMODE_DISABLED         0x0
#define MODULEMODE_ENABLE           0x2
#define MODULEMODE_MASK             0x3

#define IDLEST_FUNCTIONAL           (0x0 << 16)
#define IDLEST_TRANSITION           (0x1 << 16)
#define IDLEST_IDLE                 (0x2 << 16)
#define IDLEST_DISABLED             (0x3 << 16)
#define IDLEST_MASK                 (0x3 << 16)

#endif /* _MACH_PRCM_H */
