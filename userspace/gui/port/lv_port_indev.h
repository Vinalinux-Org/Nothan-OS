#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#include "lvgl/lvgl.h"

void lv_port_indev_init(void);

/* Register a textarea so the simulator routes keyboard input to it when
 * focused. No-op on hardware (the keypad indev handles input there). */
void sim_register_ta(lv_obj_t *ta);

#endif
