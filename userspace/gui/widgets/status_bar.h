#ifndef __GUI_STATUS_BAR_H
#define __GUI_STATUS_BAR_H

#include "lvgl/lvgl.h"

/* Shared so screens that offset content below the bar (e.g. home) stay in
 * sync with the bar's actual height instead of duplicating the literal. */
#define STATUS_BAR_HEIGHT  34

/*
 * status_bar_create() - full-width bar across the top with clock left,
 * "MiNuong" title centered, signal + wifi + battery icons right.
 */
lv_obj_t *status_bar_create(lv_obj_t *parent);

#endif
