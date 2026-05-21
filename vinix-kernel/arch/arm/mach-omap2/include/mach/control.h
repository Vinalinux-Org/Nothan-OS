/*
 * arch/arm/mach-omap2/include/mach/control.h
 *
 * AM335x Control Module — pinmux base + pad configuration bits.
 * Driver-specific CONF_<pad> offsets stay in each driver (peripheral-specific).
 */

#ifndef _MACH_CONTROL_H
#define _MACH_CONTROL_H

#define CTRL_MODULE_BASE            0x44E10000UL

/* Pad configuration bits */
#define PIN_MODE_MASK               0x7
#define PIN_PULL_DISABLE            (1 << 3)
#define PIN_PULLUP_EN               (1 << 4)
#define PIN_INPUT_EN                (1 << 5)
#define PIN_SLEW_SLOW               (1 << 6)

#define CTRL_USB_CTRL0              (CTRL_MODULE_BASE + 0x620)
#define CTRL_USB_CTRL1              (CTRL_MODULE_BASE + 0x628)
/* TRM Table 9-32: bit 0 = cm_pwrdn, bit 1 = otg_pwrdn (0 = PHY active) */
#define USB_CTRL_CM_PWRDN           (1 << 0)
#define USB_CTRL_OTG_PWRDN          (1 << 1)
/* bit 19: otgvdet_en — enables all OTG VBUS comparators (required for A_WAIT_VRISE) */
#define USB_CTRL_OTGVDET_EN         (1 << 19)
#define USB_CTRL0_CM_PWRDN          USB_CTRL_CM_PWRDN

#endif /* _MACH_CONTROL_H */
