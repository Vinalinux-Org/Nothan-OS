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

/* Programmatically scroll the app grid. to_end=1 scrolls to the last row,
 * to_end=0 scrolls back to the top. animated=0 snaps instantly. */
void home_scroll_to_end(int to_end, int animated);

#endif
