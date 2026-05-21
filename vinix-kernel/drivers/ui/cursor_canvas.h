/*
 * drivers/ui/cursor_canvas.h — LVGL v8 canvas cursor for Nothan-OS
 *
 * Two shapes: arrow (default) and hand (pointer).
 * Both use white fill + 2px black outline on a magenta chroma-key background.
 */

#ifndef CURSOR_CANVAS_H
#define CURSOR_CANVAS_H

#include "lvgl/lvgl.h"

typedef enum {
    CURSOR_SHAPE_ARROW = 0,
    CURSOR_SHAPE_HAND,
} cursor_shape_t;

void cursor_canvas_init(lv_indev_t *mouse_indev);
void cursor_canvas_set(cursor_shape_t shape);

#endif /* CURSOR_CANVAS_H */
