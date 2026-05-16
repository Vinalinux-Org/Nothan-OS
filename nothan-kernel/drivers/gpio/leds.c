/*
 * drivers/gpio/leds.c — BBB user LED control (USR0-USR3)
 *
 * Maps LED index 0-3 to GPIO1_21-24 (gpio 53-56). Callers use
 * led_set()/led_get() without knowing the GPIO numbering.
 */

#include "nothan/gpio.h"
#include "nothan/leds.h"
#include "nothan/init.h"

#define LED_COUNT   4
#define LED_GPIO(n) (53 + (n))

static int led_state[LED_COUNT];

void led_set(int n, int val)
{
    if ((unsigned int)n >= LED_COUNT)
        return;
    led_state[n] = !!val;
    gpio_set_value(LED_GPIO(n), val);
}

int led_get(int n)
{
    if ((unsigned int)n >= LED_COUNT)
        return 0;
    return led_state[n];
}

static int __init bbb_leds_init(void)
{
    for (int i = 0; i < LED_COUNT; i++)
        gpio_direction_output(LED_GPIO(i), 0);
    return 0;
}
late_initcall(bbb_leds_init);
