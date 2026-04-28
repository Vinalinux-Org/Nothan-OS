/* ============================================================
 * fbcon.c
 * ------------------------------------------------------------
 * Text console layered on top of fbmem's graphics primitives.
 * Tracks (col, row) cursor, handles \n, \r, \t, line wrap, and
 * scrolling by copying pixel rows up.
 * ============================================================ */

#include "fb.h"

static uint32_t con_col;
static uint32_t con_row;
static uint32_t con_max_col;
static uint32_t con_max_row;
static uint16_t con_fg;
static uint16_t con_bg;
static int      con_enabled;

void fb_console_init(uint16_t fg, uint16_t bg)
{
    con_fg      = fg;
    con_bg      = bg;
    con_col     = 0;
    con_row     = 0;
    con_max_col = fb_get_width()  / FB_FONT_W;
    con_max_row = fb_get_height() / FB_FONT_H;
    con_enabled = 1;

    fb_clear(bg);
}

static void con_scroll(void)
{
    uint32_t  width        = fb_get_width();
    uint16_t *fb           = fb_get_buffer();
    uint32_t  row_pixels   = width * FB_FONT_H;
    uint32_t  total_pixels = width * (con_max_row - 1) * FB_FONT_H;

    /* Shift content up by one row. */
    uint16_t *dst = fb;
    uint16_t *src = fb + row_pixels;
    for (uint32_t i = 0; i < total_pixels; i++)
        dst[i] = src[i];

    /* Clear the now-stale last row. */
    uint16_t *last_row = fb + total_pixels;
    for (uint32_t i = 0; i < row_pixels; i++)
        last_row[i] = con_bg;
}

void fb_console_putchar(char c)
{
    if (!con_enabled) return;

    if (c == '\n') {
        con_col = 0;
        con_row++;
    } else if (c == '\r') {
        con_col = 0;
    } else if (c == '\t') {
        con_col = (con_col + 4) & ~3;
    } else {
        fb_draw_char(con_col * FB_FONT_W, con_row * FB_FONT_H,
                     c, con_fg, con_bg);
        con_col++;
    }

    if (con_col >= con_max_col) {
        con_col = 0;
        con_row++;
    }

    if (con_row >= con_max_row) {
        con_scroll();
        con_row = con_max_row - 1;
    }
}

void fb_console_puts(const char *str)
{
    while (*str) fb_console_putchar(*str++);
}

void fb_console_enable(int enable)
{
    con_enabled = enable;
}

int fb_console_is_enabled(void)
{
    return con_enabled;
}

uint16_t fb_get_console_bg(void)
{
    return con_bg;
}
