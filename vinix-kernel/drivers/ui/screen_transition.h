#ifndef SCREEN_TRANSITION_H
#define SCREEN_TRANSITION_H

#include "lvgl/lvgl.h"

extern int is_transitioning;

void transition_to(lv_obj_t *next_scr, int delete_current);

#endif /* SCREEN_TRANSITION_H */
