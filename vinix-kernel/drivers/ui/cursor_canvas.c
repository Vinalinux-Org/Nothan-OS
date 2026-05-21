/*
 * drivers/ui/cursor_canvas.c — LVGL v8 canvas cursor for Nothan-OS
 *
 * Draws two cursor shapes at runtime on a 32×32 lv_canvas:
 *
 *   ARROW — classic top-left arrow, hotspot (0,0)
 *     Black outer polygon → white inner polygon (2px inset)
 *
 *   HAND  — browser-style pointer (index finger up), hotspot (8,0)
 *     Black rectangles for finger/palm outline → white fill rects
 *
 * Background = magenta 0xFF00FF (= LV_COLOR_CHROMA_KEY → transparent).
 * Canvas lives on LV_LAYER_SYS (always rendered above all widgets).
 * Bound to the mouse indev via lv_indev_set_cursor().
 */

#include "cursor_canvas.h"

#define CSIZE   32      /* canvas width and height in pixels */

/* Two canvas buffers — one per shape, swapped on cursor_canvas_set() */
static uint8_t s_buf_arrow[CSIZE * CSIZE * sizeof(lv_color_t)];
static uint8_t s_buf_hand [CSIZE * CSIZE * sizeof(lv_color_t)];

static lv_obj_t      *s_canvas;
static cursor_shape_t s_current = CURSOR_SHAPE_ARROW;

/* ── Colour helpers ─────────────────────────────────────────────────── */

static inline lv_draw_rect_dsc_t solid_rect(lv_color_t col)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color     = col;
    d.bg_opa       = LV_OPA_COVER;
    d.border_width = 0;
    d.radius       = 0;
    return d;
}

/* ── Arrow cursor ───────────────────────────────────────────────────── */

static void draw_arrow(lv_obj_t *canvas)
{
    lv_canvas_fill_bg(canvas, lv_color_hex(0xFF00FF), LV_OPA_COVER);

    /* Black outer polygon — forms the visible 2px border */
    static const lv_point_t outer[7] = {
        {2,2},{2,24},{8,18},{12,30},{16,28},{12,16},{20,16}
    };
    lv_draw_rect_dsc_t blk = solid_rect(lv_color_black());
    lv_canvas_draw_polygon(canvas, outer, 7, &blk);

    /* White inner polygon — ~2px inset of outer shape */
    static const lv_point_t inner[7] = {
        {4,4},{4,21},{9,17},{13,27},{14,26},{11,15},{18,15}
    };
    lv_draw_rect_dsc_t wht = solid_rect(lv_color_white());
    lv_canvas_draw_polygon(canvas, inner, 7, &wht);
}

/* ── Hand / pointer cursor ──────────────────────────────────────────── */
/*
 * Layout (32×32, hotspot at (8,0) = index finger tip):
 *
 *   Index finger  : x=6..13,  y=0..18  (widest 8px, tall 18px)
 *   Middle finger : x=14..19, y=6..17  (short, curled)
 *   Ring finger   : x=20..24, y=8..17
 *   Palm          : x=4..27,  y=18..27 (wide base)
 *   Thumb         : x=2..7,   y=21..27 (left side)
 *
 * Each part = black outer rect → white inner rect (1px inset each side).
 */

static void draw_part(lv_obj_t *canvas,
                      lv_coord_t x, lv_coord_t y,
                      lv_coord_t w, lv_coord_t h)
{
    lv_draw_rect_dsc_t blk = solid_rect(lv_color_black());
    lv_canvas_draw_rect(canvas, x, y, w, h, &blk);

    if (w > 2 && h > 2) {
        lv_draw_rect_dsc_t wht = solid_rect(lv_color_white());
        lv_canvas_draw_rect(canvas, x + 1, y + 1, w - 2, h - 2, &wht);
    }
}

static void draw_hand(lv_obj_t *canvas)
{
    lv_canvas_fill_bg(canvas, lv_color_hex(0xFF00FF), LV_OPA_COVER);

    draw_part(canvas,  6,  0,  8, 18);   /* index finger (extended up)  */
    draw_part(canvas, 14,  6,  6, 12);   /* middle finger (curled)       */
    draw_part(canvas, 20,  8,  5, 10);   /* ring finger (curled)         */
    draw_part(canvas, 25, 10,  4,  8);   /* pinky finger (curled)        */
    draw_part(canvas,  4, 18, 24,  9);   /* palm base                    */
    draw_part(canvas,  2, 21,  5,  6);   /* thumb (left)                 */

    /* Fingernail accent on index tip — tiny white oval */
    lv_draw_rect_dsc_t nail = solid_rect(lv_color_hex(0xdddddd));
    nail.radius = LV_RADIUS_CIRCLE;
    lv_canvas_draw_rect(canvas, 8, 1, 4, 3, &nail);
}

/* ── Swap canvas buffer to the chosen shape ─────────────────────────── */

static void apply_shape(cursor_shape_t shape)
{
    void *buf = (shape == CURSOR_SHAPE_HAND) ? s_buf_hand : s_buf_arrow;

    lv_canvas_set_buffer(s_canvas, buf, CSIZE, CSIZE,
                         LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED);

    if (shape == CURSOR_SHAPE_HAND) {
        draw_hand(s_canvas);
        /* Hotspot for hand: tip of index finger is at pixel (8,0) */
        lv_obj_set_style_translate_x(s_canvas, -8, 0);
        lv_obj_set_style_translate_y(s_canvas,  0, 0);
    } else {
        draw_arrow(s_canvas);
        /* Hotspot for arrow: tip is at (0,0) — no offset needed */
        lv_obj_set_style_translate_x(s_canvas, 0, 0);
        lv_obj_set_style_translate_y(s_canvas, 0, 0);
    }

    s_current = shape;
}

/* ── Public API ─────────────────────────────────────────────────────── */

void cursor_canvas_init(lv_indev_t *mouse_indev)
{
    /* Create canvas on the system layer — above all other widgets */
    s_canvas = lv_canvas_create(lv_layer_sys());

    /* Draw the default arrow shape */
    apply_shape(CURSOR_SHAPE_ARROW);

    /* Bind the cursor image to the mouse input device */
    lv_indev_set_cursor(mouse_indev, s_canvas);
}

void cursor_canvas_set(cursor_shape_t shape)
{
    if (shape == s_current || !s_canvas)
        return;
    apply_shape(shape);
}
