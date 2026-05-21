/*
 * drivers/input/cursor.c — LVGL v8 canvas-based arrow cursor
 *
 * Draws a 32x32 arrow at runtime using lv_canvas_draw_polygon():
 *   1. Fill canvas with magenta (chroma key = transparent)
 *   2. Black outer polygon  → gives 2px border
 *   3. White inner polygon  → gives white fill
 *
 * Canvas lives on LV_LAYER_SYS (always on top).
 * Hotspot is (0,0) — the tip of the arrow — so no translate offset needed.
 */

#include "vinix/cursor.h"

#define CURSOR_SIZE 32

/* RGB565: 2 bytes/px → 2048 bytes total, well within 8 KB RAM budget */
static uint8_t cursor_buf[CURSOR_SIZE * CURSOR_SIZE * sizeof(lv_color_t)];
static lv_obj_t *cursor_obj;

static void draw_arrow(lv_obj_t *canvas)
{
    /* Transparent background (magenta = chroma key in LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED) */
    lv_canvas_fill_bg(canvas, lv_color_hex(0xFF00FF), LV_OPA_COVER);

    /*
     * Outer black polygon — forms the 2px visible border.
     * Shape: classic top-left arrow with tail notch, fitted in 32x32.
     */
    static const lv_point_t outer[] = {
        {2, 2}, {2, 24}, {8, 18}, {12, 30}, {16, 28}, {12, 16}, {20, 16}
    };
    lv_draw_rect_dsc_t blk;
    lv_draw_rect_dsc_init(&blk);
    blk.bg_color     = lv_color_black();
    blk.bg_opa       = LV_OPA_COVER;
    blk.border_width = 0;
    blk.radius       = 0;
    lv_canvas_draw_polygon(canvas, outer, 7, &blk);

    /*
     * Inner white polygon — ~2px inset of the outer shape.
     * Computed by moving each edge inward 2px along its normal.
     */
    static const lv_point_t inner[] = {
        {4, 4}, {4, 21}, {9, 17}, {13, 27}, {14, 26}, {11, 15}, {18, 15}
    };
    lv_draw_rect_dsc_t wht;
    lv_draw_rect_dsc_init(&wht);
    wht.bg_color     = lv_color_white();
    wht.bg_opa       = LV_OPA_COVER;
    wht.border_width = 0;
    wht.radius       = 0;
    lv_canvas_draw_polygon(canvas, inner, 7, &wht);
}

void cursor_init(lv_indev_t *mouse_indev)
{
    /* Create canvas on the system layer — always rendered above all widgets */
    cursor_obj = lv_canvas_create(lv_layer_sys());
    lv_canvas_set_buffer(cursor_obj, cursor_buf,
                         CURSOR_SIZE, CURSOR_SIZE,
                         LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED);

    draw_arrow(cursor_obj);

    /* Bind cursor image to the mouse input device */
    lv_indev_set_cursor(mouse_indev, cursor_obj);
}
