/*
 * AM335x Control Module — pad control / pinmux register map.
 *
 * Pin mode and pull configuration. Driver picks its own CONF_<pad>
 * offsets from the TRM since pinmux is peripheral-specific (which pad
 * carries which signal). Only the base address is cross-cutting and
 * lives here.
 *
 * AM335x TRM Ch.09.
 */

#ifndef _MACH_CONTROL_H
#define _MACH_CONTROL_H

#define CTRL_MODULE_BASE            0x44E10000UL

/* conf_<module>_<pin> register bits (TRM 9.3.1.49 onwards) */
#define PIN_MODE_MASK               0x7
#define PIN_PULL_DISABLE            (1 << 3)
#define PIN_PULLUP_EN               (1 << 4)
#define PIN_INPUT_EN                (1 << 5)
#define PIN_SLEW_SLOW               (1 << 6)

#endif /* _MACH_CONTROL_H */
