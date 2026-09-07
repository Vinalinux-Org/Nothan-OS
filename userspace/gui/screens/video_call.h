#ifndef __GUI_VIDEO_CALL_H
#define __GUI_VIDEO_CALL_H

#include "lvgl/lvgl.h"

/* The in-call screen. @arg is the peer index, cast through (void *)(long). */
void video_call_create(lv_obj_t *screen, void *arg);

#endif
