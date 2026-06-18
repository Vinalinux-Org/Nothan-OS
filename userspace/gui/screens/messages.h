#ifndef __GUI_MESSAGES_H
#define __GUI_MESSAGES_H

#include "lvgl/lvgl.h"

/*
 * messages_create() - Build the Messages app screen on @parent:
 * status bar + nav header (back + title) + list of conversations.
 */
lv_obj_t *messages_create(lv_obj_t *parent);

#endif
