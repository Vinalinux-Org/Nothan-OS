#ifndef __GUI_HOME_H
#define __GUI_HOME_H

#include "lvgl/lvgl.h"

/*
 * home_create() - Build the home launcher into @screen (a nav builder;
 * @arg is unused). Lays out the status bar, search bar, scrollable
 * 4-column app grid and the floating dock. The system nav bar is owned
 * by the navigation stack, not the home screen.
 */
void home_create(lv_obj_t *screen, void *arg);

/*
 * home_scroll_to_end() - Smoothly scroll the app grid fully to the
 * bottom (@to_bottom != 0) or back to the top (@to_bottom == 0) over
 * @duration_ms with an ease-in-out path. Used by the demo sweep.
 */
void home_scroll_to_end(int to_bottom, int duration_ms);

#endif
