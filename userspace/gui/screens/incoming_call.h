#ifndef __GUI_INCOMING_CALL_H
#define __GUI_INCOMING_CALL_H

#include "lvgl/lvgl.h"

/*
 * incoming_call_create() - Full-screen incoming-call overlay: avatar,
 * peer, number, and Reject / Accept. A nav builder; @arg is the const
 * struct call_info *. Accept answers (pops to the call); Reject hangs up.
 */
void incoming_call_create(lv_obj_t *screen, void *arg);

#endif
