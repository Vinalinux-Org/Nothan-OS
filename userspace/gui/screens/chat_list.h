#ifndef __GUI_CHAT_LIST_H
#define __GUI_CHAT_LIST_H

#include "lvgl/lvgl.h"

/*
 * The Messages tab's content.
 *
 * Takes a container, not a screen: it lives inside the app shell alongside two
 * other tabs, and the shell owns the screen.  A tab that built its own screen
 * would have to tear down and rebuild the bar it sits under every time the
 * user switched.
 */
void chat_list_build(lv_obj_t *parent);

/* Rebuild the rows in place, when the store changed under an open tab. */
void chat_list_refresh(void);

#endif
