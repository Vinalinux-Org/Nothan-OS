/*
 * drivers/ui/lv_port.h — LVGL port interface for Nothan-OS
 */

#ifndef LV_PORT_H
#define LV_PORT_H

#include "lvgl/lvgl.h"

void        lv_port_disp_init(void);
void        lv_port_indev_init(void);
lv_indev_t *lv_port_get_mouse_indev(void);  /* for cursor binding */

#endif /* LV_PORT_H */
