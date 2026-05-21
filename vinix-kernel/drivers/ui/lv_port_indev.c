/*
 * drivers/ui/lv_port_indev.c — LVGL pointer input from USB HID mouse
 *
 * Reads position and button state from the existing mouse_cursor driver
 * (cursor_get_pos / cursor_get_buttons) and feeds LVGL.
 *
 * The raw framebuffer cursor is hidden on init — LVGL canvas cursor
 * (cursor_canvas.c) replaces it via lv_indev_set_cursor().
 */

#include "lvgl/lvgl.h"
#include "vinix/mouse_cursor.h"
#include "types.h"

static lv_indev_t *s_mouse_indev;

static void mouse_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    int32_t x, y;
    cursor_get_pos(&x, &y);
    data->point.x = (lv_coord_t)x;
    data->point.y = (lv_coord_t)y;
    data->state   = (cursor_get_buttons() & 0x01)
                    ? LV_INDEV_STATE_PRESSED
                    : LV_INDEV_STATE_RELEASED;
}

void lv_port_indev_init(void)
{
    /* Hide the raw XOR cursor — LVGL canvas cursor takes over visuals */
    cursor_hide();

    static lv_indev_drv_t drv;
    lv_indev_drv_init(&drv);
    drv.type    = LV_INDEV_TYPE_POINTER;
    drv.read_cb = mouse_read;
    s_mouse_indev = lv_indev_drv_register(&drv);
}

lv_indev_t *lv_port_get_mouse_indev(void)
{
    return s_mouse_indev;
}
