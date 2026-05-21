/*
 * drivers/ui/lv_port_disp.c — LVGL display driver for Nothan-OS
 *
 * 800×480 RGB565 framebuffer, full-screen back-buffer + full_refresh=1.
 * Back buffer = 800×480×2 = 768 KB in BSS.
 */

#include "lvgl/lvgl.h"
#include "fb.h"
#include "types.h"

#define DISP_W  800
#define DISP_H  480

static lv_color_t         back_buf[DISP_W * DISP_H] __attribute__((aligned(8)));
static lv_disp_draw_buf_t draw_buf;

static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area,
                       lv_color_t *color_p)
{
    (void)area;
    (void)color_p;
    uint16_t *fb = fb_get_buffer();
    lv_memcpy(fb, back_buf, sizeof(back_buf));
    lv_disp_flush_ready(drv);
}

void lv_port_disp_init(void)
{
    static lv_disp_drv_t drv;

    lv_disp_draw_buf_init(&draw_buf, back_buf, NULL, DISP_W * DISP_H);

    lv_disp_drv_init(&drv);
    drv.draw_buf     = &draw_buf;
    drv.flush_cb     = disp_flush;
    drv.hor_res      = DISP_W;
    drv.ver_res      = DISP_H;
    drv.full_refresh = 1;
    lv_disp_drv_register(&drv);
}
