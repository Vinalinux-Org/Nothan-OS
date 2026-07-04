/*
 * drivers/pinctrl/pinctrl-am335x.c - AM335x pad mux control
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/pinctrl.h>
#include <nothan/mmio.h>
#include <nothan/printk.h>

static const struct pin_group *pin_groups;
static unsigned int             pin_ngroups;

/**
 * pinctrl_register() - Register the board pin group table
 * @groups:  array of struct pin_group
 * @ngroups: number of groups
 */
void pinctrl_register(const struct pin_group *groups, unsigned int ngroups)
{
	pin_groups  = groups;
	pin_ngroups = ngroups;
	printk("[PINCTRL] %u groups registered\n", ngroups);
}

static int streq(const char *a, const char *b)
{
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return *a == *b;
}

/**
 * pinctrl_select() - Write pad config registers for a named group
 * @name: group name
 *
 * Return: 0 on success, -1 if not found.
 */
int pinctrl_select(const char *name)
{
	unsigned int i, j;

	for (i = 0; i < pin_ngroups; i++) {
		if (!streq(pin_groups[i].name, name))
			continue;
		for (j = 0; j < pin_groups[i].npins; j++)
			mmio_write32(pin_groups[i].pins[j].reg,
				     pin_groups[i].pins[j].val);
		printk("[PINCTRL] '%s' configured (%u pins)\n",
		       name, pin_groups[i].npins);
		return 0;
	}

	printk("[PINCTRL] group '%s' not found\n", name);
	return -1;
}
