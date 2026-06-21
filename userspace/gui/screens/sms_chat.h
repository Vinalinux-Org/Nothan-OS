#ifndef __GUI_SMS_CHAT_H
#define __GUI_SMS_CHAT_H

#include "lvgl/lvgl.h"

/*
 * sms_chat_create() - One conversation thread: date separator, message
 * bubbles (received left/surface, sent right/accent) and a bottom input
 * bar. A nav builder; @arg is the const struct sms_conversation * to
 * show (NULL renders an empty thread).
 */
void sms_chat_create(lv_obj_t *screen, void *arg);

#endif
