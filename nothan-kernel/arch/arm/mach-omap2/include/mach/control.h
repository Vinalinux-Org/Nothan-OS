/*
 * arch/arm/mach-omap2/include/mach/control.h
 *
 * AM335x Control Module — pinmux base + pad configuration bits.
 * Driver-specific CONF_<pad> offsets stay in each driver (peripheral-specific).
 */

#ifndef _MACH_CONTROL_H
#define _MACH_CONTROL_H

#define CTRL_MODULE_BASE            0x44E10000UL

/* MAC address fused at manufacturing — AM335x TRM §9.4.1.1 */
#define CTRL_MAC_ID0_LO             (CTRL_MODULE_BASE + 0x630)
#define CTRL_MAC_ID0_HI             (CTRL_MODULE_BASE + 0x634)

#define CTRL_GMII_SEL               (CTRL_MODULE_BASE + 0x650)

/* GMII1_SEL field [2:1] — BBB: R117 pulls RMIISEL GND = MII mode (TRM §9.10.7) */
#define GMII1_SEL_MII               (0x0 << 1)
#define GMII1_SEL_RMII              (0x1 << 1)
#define GMII1_SEL_RGMII             (0x2 << 1)
#define GMII1_SEL_MASK              (0x3 << 1)

/* Pad configuration bits */
#define PIN_MODE_MASK               0x7
#define PIN_PULL_DISABLE            (1 << 3)
#define PIN_PULLUP_EN               (1 << 4)
#define PIN_INPUT_EN                (1 << 5)
#define PIN_SLEW_SLOW               (1 << 6)

#endif /* _MACH_CONTROL_H */
