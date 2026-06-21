#ifndef __GUI_NAV_BAR_H
#define __GUI_NAV_BAR_H

#include "lvgl/lvgl.h"

#define NAV_BAR_HEIGHT  40

/*
 * nav_bar_create() - System-wide Android-style 3-button nav at the
 * bottom of the screen: back, home, recents. Each button fires its
 * CLICKED callback (pass NULL to leave a key inert). Owned by the
 * navigation stack, which mounts it on the display top layer.
 */
lv_obj_t *nav_bar_create(lv_obj_t *parent, lv_event_cb_t on_back,
			 lv_event_cb_t on_home, lv_event_cb_t on_recents);

#endif
