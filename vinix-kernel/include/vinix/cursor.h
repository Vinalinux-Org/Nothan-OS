/*
 * include/vinix/cursor.h — LVGL v8 canvas-based cursor (32x32, RGB565)
 *
 * Requires LVGL v8.x linked into the project.
 * Arrow: white fill, 2px black outline, hotspot (0,0).
 */

#ifndef VINIX_CURSOR_H
#define VINIX_CURSOR_H

#include "lvgl.h"

void cursor_init(lv_indev_t *mouse_indev);

#endif /* VINIX_CURSOR_H */
