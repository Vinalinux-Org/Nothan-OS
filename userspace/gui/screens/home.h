#ifndef __GUI_HOME_H
#define __GUI_HOME_H

#include "lvgl/lvgl.h"

/*
 * home_create() - Build the home screen on the given parent (typically
 * the active screen). Lays out the 3x3 app grid with status bar above.
 *
 * Returns the "Messages" tile so the demo flow can glow it.
 */
lv_obj_t *home_create(lv_obj_t *parent);

#endif
