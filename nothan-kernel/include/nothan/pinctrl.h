/*
 * include/nothan/pinctrl.h - Pin mux control subsystem
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#ifndef _NOTHAN_PINCTRL_H
#define _NOTHAN_PINCTRL_H

#include <nothan/types.h>

/* AM335x Control Module CONF register bit fields */
#define PIN_MUXMODE(n)		((n) & 0x7)
#define PIN_PUDEN		(1 << 3)	/* 1 = pull disabled */
#define PIN_PUTYPESEL		(1 << 4)	/* 1 = pull-up, 0 = pull-down */
#define PIN_RXACTIVE		(1 << 5)	/* 1 = input enabled */
#define PIN_SLEWCTRL		(1 << 6)	/* 1 = slow slew */

#define PIN_OUTPUT		(PIN_PUDEN)
#define PIN_OUTPUT_PULLDOWN	(0)
#define PIN_OUTPUT_PULLUP	(PIN_PUTYPESEL)
#define PIN_INPUT		(PIN_RXACTIVE | PIN_PUDEN)
#define PIN_INPUT_PULLDOWN	(PIN_RXACTIVE)
#define PIN_INPUT_PULLUP	(PIN_RXACTIVE | PIN_PUTYPESEL)

/**
 * struct pin_desc - single pad configuration
 * @reg: Control Module CONF register virtual address
 * @val: value to write (mode + pull + rx bits)
 */
struct pin_desc {
	u32	reg;
	u32	val;
};

/**
 * struct pin_group - named group of pads for one peripheral function
 * @name:  identifier used by drivers, e.g. "uart0"
 * @pins:  array of pad descriptors
 * @npins: number of entries in @pins
 */
struct pin_group {
	const char            *name;
	const struct pin_desc *pins;
	unsigned int           npins;
};

/**
 * pinctrl_register() - Register a board's pin group table
 * @groups: array of pin_group entries
 * @ngroups: number of entries
 */
void pinctrl_register(const struct pin_group *groups, unsigned int ngroups);

/**
 * pinctrl_select() - Configure all pads in a named group
 * @name: group name as registered via pinctrl_register()
 *
 * Return: 0 on success, -1 if group not found.
 */
int pinctrl_select(const char *name);

#endif /* _NOTHAN_PINCTRL_H */
