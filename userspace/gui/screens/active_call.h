#ifndef __GUI_ACTIVE_CALL_H
#define __GUI_ACTIVE_CALL_H

#include "lvgl/lvgl.h"

/*
 * active_call_create() - In-call screen: avatar, peer, number, call
 * duration, plus Mute and Hang-up. A nav builder; @arg is the const
 * struct call_info * for the call. Hang-up ends the call (pops).
 */
void active_call_create(lv_obj_t *screen, void *arg);

#endif
