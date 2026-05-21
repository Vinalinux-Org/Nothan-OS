/*
 * drivers/input/mouse_cursor.c — software mouse cursor renderer
 *
 * XOR-blended 16x16 arrow cursor on RGB565 framebuffer.
 * Consumes EV_REL/EV_KEY events from an input_dev.
 */

#include "types.h"
#include "fb.h"
#include "vinix/input.h"
#include "vinix/init.h"
#include "vinix/printk.h"

#define CURSOR_W 16
#define CURSOR_H 16

static const uint16_t cursor_shape[CURSOR_H] = {
    0x8000, 0xC000, 0xE000, 0xF000,
    0xF800, 0xFC00, 0xFE00, 0xFF00,
    0xF800, 0xCC00, 0x8600, 0x0600,
    0x0300, 0x0300, 0x0180, 0x0180,
};

static int32_t cursor_x = 400;
static int32_t cursor_y = 300;
static int32_t acc_dx;
static int32_t acc_dy;
static uint8_t cursor_visible;
static uint8_t btn_state;          /* bit 0=left, 1=right, 2=middle */
static struct input_dev *cursor_idev;

static uint16_t saved_bg[CURSOR_H][CURSOR_W];
static uint8_t bg_saved;

static uint16_t *fb_buf;
static uint32_t fb_w;
static uint32_t fb_h;

static void cursor_save_bg(int32_t x, int32_t y)
{
    for (int32_t r = 0; r < CURSOR_H; r++) {
        int32_t py = y + r;
        if (py < 0 || py >= (int32_t)fb_h) {
            for (int32_t c = 0; c < CURSOR_W; c++)
                saved_bg[r][c] = FB_BLACK;
            continue;
        }
        for (int32_t c = 0; c < CURSOR_W; c++) {
            int32_t px = x + c;
            if (px < 0 || px >= (int32_t)fb_w)
                saved_bg[r][c] = FB_BLACK;
            else
                saved_bg[r][c] = fb_buf[py * fb_w + px];
        }
    }
    bg_saved = 1;
}

static void cursor_restore_bg(int32_t x, int32_t y)
{
    if (!bg_saved) return;
    for (int32_t r = 0; r < CURSOR_H; r++) {
        int32_t py = y + r;
        if (py < 0 || py >= (int32_t)fb_h) continue;
        for (int32_t c = 0; c < CURSOR_W; c++) {
            int32_t px = x + c;
            if (px < 0 || px >= (int32_t)fb_w) continue;
            if (cursor_shape[r] & (0x8000 >> c))
                fb_buf[py * fb_w + px] = saved_bg[r][c];
        }
    }
    bg_saved = 0;
}

static void cursor_draw(int32_t x, int32_t y)
{
    cursor_save_bg(x, y);
    for (int32_t r = 0; r < CURSOR_H; r++) {
        int32_t py = y + r;
        if (py < 0 || py >= (int32_t)fb_h) continue;
        for (int32_t c = 0; c < CURSOR_W; c++) {
            int32_t px = x + c;
            if (px < 0 || px >= (int32_t)fb_w) continue;
            if (cursor_shape[r] & (0x8000 >> c))
                fb_buf[py * fb_w + px] ^= 0xFFFF;
        }
    }
}

static void cursor_move(int32_t dx, int32_t dy)
{
    if (!fb_buf) return;

    int32_t nx = cursor_x + dx;
    int32_t ny = cursor_y + dy;
    if (nx < 0) nx = 0;
    if (ny < 0) ny = 0;
    if (nx >= (int32_t)fb_w - 1) nx = (int32_t)fb_w - 1;
    if (ny >= (int32_t)fb_h - 1) ny = (int32_t)fb_h - 1;

    if (cursor_visible)
        cursor_restore_bg(cursor_x, cursor_y);

    cursor_x = nx;
    cursor_y = ny;

    if (cursor_visible)
        cursor_draw(cursor_x, cursor_y);
}

static void cursor_input_handler(struct input_dev *dev, struct input_event *ev)
{
    (void)dev;
    if (ev->type == EV_KEY) {
        uint8_t bit = 0;
        if      (ev->code == BTN_LEFT)   bit = (1 << 0);
        else if (ev->code == BTN_RIGHT)  bit = (1 << 1);
        else if (ev->code == BTN_MIDDLE) bit = (1 << 2);
        if (bit) {
            if (ev->value) btn_state |= bit;
            else           btn_state &= ~bit;
        }
    } else if (ev->type == EV_REL) {
        if      (ev->code == REL_X) acc_dx += ev->value;
        else if (ev->code == REL_Y) acc_dy += ev->value;
    } else if (ev->type == EV_SYN) {
        if (acc_dx || acc_dy) {
            cursor_move(acc_dx, acc_dy);
            acc_dx = 0;
            acc_dy = 0;
        }
    }
}

void cursor_set_input_dev(struct input_dev *idev)
{
    cursor_idev = idev;
    idev->event = cursor_input_handler;
    pr_info("[CURSOR] bound to input device '%s'\n", idev->name);
}

uint8_t cursor_get_buttons(void)
{
    return btn_state;
}

void cursor_get_pos(int32_t *x, int32_t *y)
{
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

void cursor_hide(void)
{
    if (!fb_buf) return;
    if (cursor_visible && bg_saved)
        cursor_restore_bg(cursor_x, cursor_y);
    cursor_visible = 0;
}

void cursor_show(void)
{
    if (!fb_buf) return;
    cursor_visible = 1;
    cursor_draw(cursor_x, cursor_y);
}

static int cursor_late_init(void)
{
    fb_buf = fb_get_buffer();
    fb_w   = fb_get_width();
    fb_h   = fb_get_height();

    if (!fb_buf || !fb_w || !fb_h) {
        pr_info("[CURSOR] no framebuffer yet, deferring\n");
        return -1;
    }

    if (cursor_x >= (int32_t)fb_w) cursor_x = (int32_t)fb_w / 2;
    if (cursor_y >= (int32_t)fb_h) cursor_y = (int32_t)fb_h / 2;

    cursor_visible = 1;
    cursor_draw(cursor_x, cursor_y);

    pr_info("[CURSOR] initialized at (%d,%d) on %ux%u fb\n",
            cursor_x, cursor_y, fb_w, fb_h);
    return 0;
}
late_initcall(cursor_late_init);
