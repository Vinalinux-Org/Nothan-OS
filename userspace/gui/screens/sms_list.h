#ifndef __GUI_SMS_LIST_H
#define __GUI_SMS_LIST_H

#include "lvgl/lvgl.h"

/*
 * sms_list_create() - SMS app landing screen: header with a compose
 * action, a search box, and a scrollable list of conversations (avatar,
 * peer, time, last-message preview). A nav builder; @arg is unused.
 * Tapping a row pushes the conversation thread.
 */
void sms_list_create(lv_obj_t *screen, void *arg);

#endif
