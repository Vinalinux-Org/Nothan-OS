/*
 * include/nothan/leds.h — User LED API (BBB USR0-USR3)
 */
#ifndef NOTHAN_LEDS_H
#define NOTHAN_LEDS_H

void led_set(int n, int val);
int  led_get(int n);

#endif /* NOTHAN_LEDS_H */
