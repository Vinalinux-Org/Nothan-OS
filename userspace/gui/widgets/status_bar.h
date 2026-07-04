#ifndef __GUI_STATUS_BAR_H
#define __GUI_STATUS_BAR_H

#include "lvgl/lvgl.h"

/* Shared so screens that offset content below the bar (e.g. home) stay in
 * sync with the bar's actual height instead of duplicating the literal. */
#define STATUS_BAR_HEIGHT  34

/*
 * status_bar_create() - full-width bar across the top with clock left,
 * "MyNuong" title centered, signal + wifi + battery icons right.
 */
lv_obj_t *status_bar_create(lv_obj_t *parent);

/* Update signal indicator. registered=0 shows X; registered=1 shows bars
 * scaled to rssi (0-31 AT+CSQ scale, 99=unknown). Safe to call before
 * status_bar_create — the call is ignored until the widget exists. */
void status_bar_update_signal(int registered, int rssi);

#endif
