#ifndef __GUI_CHAT_THREAD_H
#define __GUI_CHAT_THREAD_H

#include "lvgl/lvgl.h"

/* One conversation. @arg is the peer index, cast through (void *)(long). */
void chat_thread_create(lv_obj_t *screen, void *arg);

#endif
