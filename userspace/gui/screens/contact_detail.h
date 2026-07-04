#ifndef __GUI_CONTACT_DETAIL_H
#define __GUI_CONTACT_DETAIL_H

#include "lvgl/lvgl.h"

/*
 * contact_detail_create() - Single-contact view: avatar, name, number,
 * and Call / SMS actions. A nav builder; @arg is the const struct
 * contact * to display (NULL renders a placeholder).
 */
void contact_detail_create(lv_obj_t *screen, void *arg);

#endif
