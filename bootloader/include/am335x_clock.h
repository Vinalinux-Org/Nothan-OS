#ifndef AM335X_CLOCK_H
#define AM335X_CLOCK_H

/* CM_PER */
#define CM_BASE                  0x44E00000
#define CM_PER_BASE              (CM_BASE + 0x000)
#define CM_PER_L4LS_CLKSTCTRL   (CM_PER_BASE + 0x00)
#define CM_PER_L3_CLKSTCTRL     (CM_PER_BASE + 0x04)
#define CM_PER_L4FW_CLKSTCTRL   (CM_PER_BASE + 0x08)
#define CM_PER_EMIF_CLKCTRL     (CM_PER_BASE + 0x28)
#define CM_PER_MMC0_CLKCTRL     (CM_PER_BASE + 0x3C)

/* CM_WKUP */
#define CM_WKUP_BASE             0x44E00400
#define CM_WKUP_CLKSTCTRL        (CM_WKUP_BASE + 0x00)
#define CM_WKUP_UART0_CLKCTRL    (CM_WKUP_BASE + 0xB4)

#define CM_PER_GPIO1_CLKCTRL     (CM_PER_BASE + 0xAC)

/*
 * DPLL registers live in CM_WKUP address space (0x44E00400), not at
 * a separate CM_DPLL_BASE. Offsets verified against U-Boot clock_am33xx.c.
 */
#define CM_CLKMODE_DPLL_MPU      (CM_WKUP_BASE + 0x88)
#define CM_IDLEST_DPLL_MPU       (CM_WKUP_BASE + 0x20)
#define CM_CLKSEL_DPLL_MPU       (CM_WKUP_BASE + 0x2C)
#define CM_DIV_M2_DPLL_MPU       (CM_WKUP_BASE + 0xA8)

#define CM_CLKMODE_DPLL_CORE     (CM_WKUP_BASE + 0x90)
#define CM_IDLEST_DPLL_CORE      (CM_WKUP_BASE + 0x5C)
#define CM_CLKSEL_DPLL_CORE      (CM_WKUP_BASE + 0x68)
#define CM_DIV_M4_DPLL_CORE      (CM_WKUP_BASE + 0x80)
#define CM_DIV_M5_DPLL_CORE      (CM_WKUP_BASE + 0x84)
#define CM_DIV_M6_DPLL_CORE      (CM_WKUP_BASE + 0xD8)

#define CM_CLKMODE_DPLL_PER      (CM_WKUP_BASE + 0x8C)
#define CM_IDLEST_DPLL_PER       (CM_WKUP_BASE + 0x70)
#define CM_CLKSEL_DPLL_PER       (CM_WKUP_BASE + 0x9C)
#define CM_DIV_M2_DPLL_PER       (CM_WKUP_BASE + 0xAC)

#define CM_CLKMODE_DPLL_DDR      (CM_WKUP_BASE + 0x94)
#define CM_IDLEST_DPLL_DDR       (CM_WKUP_BASE + 0x34)
#define CM_CLKSEL_DPLL_DDR       (CM_WKUP_BASE + 0x40)
#define CM_DIV_M2_DPLL_DDR       (CM_WKUP_BASE + 0xA0)

#define DPLL_MN_BYP_MODE         4
#define DPLL_LOCK_MODE           7

#define MODULE_DISABLE           0
#define MODULE_ENABLE            2

#endif /* AM335X_CLOCK_H */
