#ifndef __GUI_STATUS_BAR_H
#define __GUI_STATUS_BAR_H

#include "lvgl/lvgl.h"

/*
 * status_bar_create() - 360x32 bar across the top with clock left,
 * "NothanOS" title centered, signal + battery icons right.
 */
lv_obj_t *status_bar_create(lv_obj_t *parent);

#endif
