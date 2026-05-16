/*
 * include/nothan/gpio.h — GPIO pin control API
 *
 * GPIO numbering: bank * 32 + pin.
 * GPIO1_21 = 53, GPIO1_22 = 54, GPIO1_23 = 55, GPIO1_24 = 56 (BBB USR LEDs).
 */

#ifndef NOTHAN_GPIO_H
#define NOTHAN_GPIO_H

int  gpio_request(unsigned int gpio, const char *label);
void gpio_free(unsigned int gpio);
int  gpio_direction_output(unsigned int gpio, int value);
int  gpio_direction_input(unsigned int gpio);
void gpio_set_value(unsigned int gpio, int value);
int  gpio_get_value(unsigned int gpio);

#endif /* NOTHAN_GPIO_H */
