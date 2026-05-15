/*
 * include/vinix/leds.h — User LED API (BBB USR0-USR3)
 */
#ifndef VINIX_LEDS_H
#define VINIX_LEDS_H

void led_set(int n, int val);
int  led_get(int n);

#endif /* VINIX_LEDS_H */
