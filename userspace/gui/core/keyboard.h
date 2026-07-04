#ifndef GUI_KEYBOARD_H
#define GUI_KEYBOARD_H

#include "lvgl/lvgl.h"

/* On-screen keyboard height (lv_layer_top, bottom-aligned). Screens that must
 * keep content visible above it use this to size/scroll when it opens. */
#define GUI_KEYBOARD_HEIGHT  260

/*
 * gui_keyboard_init() — create the singleton on-screen keyboard on
 * lv_layer_top(). Call once from nav_init().
 *
 * gui_keyboard_attach(ta, mode) — wire a textarea so the keyboard
 * appears when it is focused and hides when it loses focus. Also
 * applies cursor styling and (in the simulator) enables SDL keyboard.
 */
void gui_keyboard_init(void);
void gui_keyboard_attach(lv_obj_t *ta, lv_keyboard_mode_t mode);

/*
 * gui_keyboard_set_lift(obj, y_ofs_closed) — register a companion widget
 * that floats above the keyboard when it opens and drops back to y_ofs_closed
 * (BOTTOM_MID-relative) when the keyboard closes. Call after creating the
 * companion widget, before the textarea is first focused. Pass NULL to clear.
 */
void gui_keyboard_set_lift(lv_obj_t *obj, int32_t y_ofs_closed);

#endif
