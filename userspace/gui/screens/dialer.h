#ifndef __GUI_DIALER_H
#define __GUI_DIALER_H

#include "lvgl/lvgl.h"

/*
 * dialer_create() - Phone dialer: number display, 12-key pad (digits +
 * letters), call and backspace buttons. A nav builder; @arg is unused.
 * The call button dials and pushes the active-call screen.
 */
void dialer_create(lv_obj_t *screen, void *arg);

#endif
