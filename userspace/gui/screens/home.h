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

#endif
