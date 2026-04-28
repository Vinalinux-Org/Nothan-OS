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

#endif /* _MACH_CONTROL_H */
