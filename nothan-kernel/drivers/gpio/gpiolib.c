/*
 * drivers/gpio/gpiolib.c - GPIO descriptor library and consumer API
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/gpio.h>
#include <nothan/printk.h>

#define FLAG_REQUESTED	(1U << 0)
#define FLAG_IS_OUT	(1U << 1)

struct gpio_desc {
	struct gpio_chip	*chip;
	unsigned int		 flags;
};

static struct gpio_desc gpio_desc[ARCH_NR_GPIOS];

static inline int desc_is_valid(unsigned int gpio)
{
	return gpio < ARCH_NR_GPIOS && gpio_desc[gpio].chip != 0;
}

/**
 * gpiochip_add() - Register a GPIO chip and claim its GPIO number range
 * @chip: chip to register; chip->base and chip->ngpio must be set
 *
 * Return: 0 on success, -1 if range is out of bounds or overlaps existing chip.
 */
int gpiochip_add(struct gpio_chip *chip)
{
	unsigned int i;
	unsigned int end;

	if (!chip || chip->base < 0)
		return -1;

	end = (unsigned int)chip->base + chip->ngpio;
	if (end > ARCH_NR_GPIOS)
		return -1;

	for (i = (unsigned int)chip->base; i < end; i++) {
		if (gpio_desc[i].chip)
			return -1;
	}

	for (i = (unsigned int)chip->base; i < end; i++)
		gpio_desc[i].chip = chip;

	printk("[GPIO] chip '%s' registered: gpio%d..gpio%d\n",
	       chip->label, chip->base, chip->base + chip->ngpio - 1);
	return 0;
}

/**
 * gpiochip_remove() - Unregister a GPIO chip
 * @chip: chip to remove
 *
 * Return: 0 on success, -1 if any pin is still requested.
 */
int gpiochip_remove(struct gpio_chip *chip)
{
	unsigned int i;
	unsigned int end;

	if (!chip)
		return -1;

	end = (unsigned int)chip->base + chip->ngpio;

	for (i = (unsigned int)chip->base; i < end; i++) {
		if (gpio_desc[i].flags & FLAG_REQUESTED)
			return -1;
	}

	for (i = (unsigned int)chip->base; i < end; i++) {
		gpio_desc[i].chip  = 0;
		gpio_desc[i].flags = 0;
	}
	return 0;
}

/**
 * gpio_request() - Claim exclusive ownership of a GPIO pin
 * @gpio:  GPIO number
 * @label: descriptive label (for diagnostics)
 *
 * Return: 0 on success, -1 if invalid or already requested.
 */
int gpio_request(unsigned int gpio, const char *label)
{
	(void)label;
	if (!desc_is_valid(gpio))
		return -1;
	if (gpio_desc[gpio].flags & FLAG_REQUESTED)
		return -1;
	gpio_desc[gpio].flags |= FLAG_REQUESTED;
	return 0;
}

/**
 * gpio_free() - Release a previously requested GPIO pin
 * @gpio: GPIO number
 */
void gpio_free(unsigned int gpio)
{
	if (!desc_is_valid(gpio))
		return;
	gpio_desc[gpio].flags &= ~FLAG_REQUESTED;
}

/**
 * gpio_direction_input() - Configure a GPIO pin as input
 * @gpio: GPIO number
 *
 * Return: 0 on success, -1 on error.
 */
int gpio_direction_input(unsigned int gpio)
{
	struct gpio_chip *chip;

	if (!desc_is_valid(gpio))
		return -1;
	chip = gpio_desc[gpio].chip;
	if (!chip->direction_input)
		return -1;
	gpio_desc[gpio].flags &= ~FLAG_IS_OUT;
	return chip->direction_input(chip, gpio - (unsigned int)chip->base);
}

/**
 * gpio_direction_output() - Configure a GPIO pin as output with initial value
 * @gpio:  GPIO number
 * @value: initial output level (0=low, non-zero=high)
 *
 * Return: 0 on success, -1 on error.
 */
int gpio_direction_output(unsigned int gpio, int value)
{
	struct gpio_chip *chip;

	if (!desc_is_valid(gpio))
		return -1;
	chip = gpio_desc[gpio].chip;
	if (!chip->direction_output)
		return -1;
	gpio_desc[gpio].flags |= FLAG_IS_OUT;
	return chip->direction_output(chip, gpio - (unsigned int)chip->base, value);
}

/**
 * gpio_get_value() - Read the current logic level of a GPIO pin
 * @gpio: GPIO number
 *
 * Return: 0 or 1 on success, -1 on error.
 */
int gpio_get_value(unsigned int gpio)
{
	struct gpio_chip *chip;

	if (!desc_is_valid(gpio))
		return -1;
	chip = gpio_desc[gpio].chip;
	if (!chip->get)
		return -1;
	return chip->get(chip, gpio - (unsigned int)chip->base);
}

/**
 * gpio_set_value() - Drive a GPIO output pin high or low
 * @gpio:  GPIO number
 * @value: 0 for low, non-zero for high
 */
void gpio_set_value(unsigned int gpio, int value)
{
	struct gpio_chip *chip;

	if (!desc_is_valid(gpio))
		return;
	chip = gpio_desc[gpio].chip;
	if (!chip->set)
		return;
	chip->set(chip, gpio - (unsigned int)chip->base, value);
}

/**
 * gpio_to_irq() - Map a GPIO pin to its interrupt number
 * @gpio: GPIO number
 *
 * Return: IRQ number on success, -1 if not supported by this chip.
 */
int gpio_to_irq(unsigned int gpio)
{
	struct gpio_chip *chip;

	if (!desc_is_valid(gpio))
		return -1;
	chip = gpio_desc[gpio].chip;
	if (!chip->to_irq)
		return -1;
	return chip->to_irq(chip, gpio - (unsigned int)chip->base);
}
