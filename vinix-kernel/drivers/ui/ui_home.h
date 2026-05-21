#ifndef UI_HOME_H
#define UI_HOME_H

#include "lvgl/lvgl.h"

lv_obj_t *ui_home_create(void);

void ui_home_update_clock(uint8_t h, uint8_t m, uint8_t s,
                          const char *date_str, const char *day_str);
void ui_home_update_weather(const char *temp_str, const char *desc_str,
                            const char *location_str);
void ui_home_update_status(bool bt, bool wifi, bool battery_ok);
void ui_home_set_app_cb(uint8_t idx, lv_event_cb_t cb);
void ui_home_set_dock_cb(uint8_t idx, lv_event_cb_t cb);
void ui_home_set_dock_active(uint8_t idx);

#endif /* UI_HOME_H */
