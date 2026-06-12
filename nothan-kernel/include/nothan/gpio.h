/*
 * include/nothan/gpio.h - GPIO subsystem API
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#ifndef _NOTHAN_GPIO_H
#define _NOTHAN_GPIO_H

#include <nothan/types.h>

/* AM335x: 4 banks × 32 pins */
#define ARCH_NR_GPIOS	128

/**
 * struct gpio_chip - abstract a GPIO controller (one per bank)
 * @label:            diagnostic name
 * @base:             first GPIO number handled by this chip
 * @ngpio:            number of GPIOs (32 for AM335x)
 * @direction_input:  configure pin @offset as input
 * @direction_output: configure pin @offset as output with initial @value
 * @get:              read current value of pin @offset
 * @set:              drive pin @offset to @value
 * @to_irq:           optional; map pin @offset to an IRQ number
 * @priv:             driver-private data
 */
struct gpio_chip {
	const char	*label;
	int		 base;
	u16		 ngpio;

	int  (*direction_input)(struct gpio_chip *chip, unsigned offset);
	int  (*direction_output)(struct gpio_chip *chip, unsigned offset, int value);
	int  (*get)(struct gpio_chip *chip, unsigned offset);
	void (*set)(struct gpio_chip *chip, unsigned offset, int value);
	int  (*to_irq)(struct gpio_chip *chip, unsigned offset);

	void	*priv;
};

static inline int gpio_is_valid(int gpio)
{
	return (unsigned int)gpio < ARCH_NR_GPIOS;
}

int gpiochip_add(struct gpio_chip *chip);
int gpiochip_remove(struct gpio_chip *chip);

int  gpio_request(unsigned int gpio, const char *label);
void gpio_free(unsigned int gpio);
int  gpio_direction_input(unsigned int gpio);
int  gpio_direction_output(unsigned int gpio, int value);
int  gpio_get_value(unsigned int gpio);
void gpio_set_value(unsigned int gpio, int value);
int  gpio_to_irq(unsigned int gpio);

#endif /* _NOTHAN_GPIO_H */
