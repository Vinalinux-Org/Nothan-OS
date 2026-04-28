/* ============================================================
 * fb.h — Framebuffer Graphics API
 * Target: 800x600 @ 60Hz, RGB565
 * ============================================================ */

#ifndef FB_H
#define FB_H

#include "types.h"

/* RGB565 color from 8-bit components */
#define FB_RGB(r, g, b) ((uint16_t)( \
    (((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

/* Common colors */
#define FB_BLACK    0x0000
#define FB_WHITE    0xFFFF
#define FB_RED      0xF800
#define FB_GREEN    0x07E0
#define FB_BLUE     0x001F
#define FB_YELLOW   0xFFE0
#define FB_CYAN     0x07FF
#define FB_MAGENTA  0xF81F
#define FB_GRAY     0x7BEF

/* Font dimensions (built-in 8x16 bitmap font) */
#define FB_FONT_W   8
#define FB_FONT_H   16

/* Text grid dimensions (derived from display size) */
#define FB_COLS     (800 / FB_FONT_W)   /* 100 */
#define FB_ROWS     (600 / FB_FONT_H)   /* 37  */

/**
 * Initialize framebuffer subsystem.
 * Must be called after lcdc_init() + lcdc_start_raster().
 */
void fb_init(void);

/** Set a single pixel */
void fb_putpixel(uint32_t x, uint32_t y, uint16_t color);

/** Fill a rectangle */
void fb_fillrect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color);

/** Clear entire screen */
void fb_clear(uint16_t color);

/** Draw a single character at pixel position */
void fb_draw_char(uint32_t x, uint32_t y, char c, uint16_t fg, uint16_t bg);

/** Draw a null-terminated string at pixel position */
void fb_puts(uint32_t x, uint32_t y, const char *str, uint16_t fg, uint16_t bg);

/** Print a string at text grid position (col, row) */
void fb_print(uint32_t col, uint32_t row, const char *str, uint16_t fg, uint16_t bg);

/** Draw a scaled character (scale = integer multiplier, e.g. 4 = 32x64px) */
void fb_draw_char_scaled(uint32_t x, uint32_t y, char c, uint16_t fg, uint16_t bg, uint32_t scale);

/** Draw a scaled string */
void fb_puts_scaled(uint32_t x, uint32_t y, const char *str, uint16_t fg, uint16_t bg, uint32_t scale);

/** Fill a solid circle with center (cx, cy) and radius r */
void fb_fillcircle(uint32_t cx, uint32_t cy, uint32_t r, uint16_t color);

/** Framebuffer dimensions — exposed for fbcon and other consumers
 * that need the geometry without owning lcdc directly. */
uint32_t  fb_get_width(void);
uint32_t  fb_get_height(void);
uint16_t *fb_get_buffer(void);
uint16_t  fb_get_console_bg(void);

/** Returns 16 bytes of bitmap data for ASCII char (32-126); '?' fallback. */
const uint8_t *font_8x16_glyph(char c);

/* ============================================================
 * Console mode — auto cursor, newline, scroll
 * ============================================================ */

/** Start console mode (gray bg, white text) */
void fb_console_init(uint16_t fg, uint16_t bg);

/** Print one character with auto-advance and scroll */
void fb_console_putchar(char c);

/** Print string to console */
void fb_console_puts(const char *str);

/** Enable/disable console output (for splash transition) */
void fb_console_enable(int enable);

/** Check if console is active */
int fb_console_is_enabled(void);

#endif /* FB_H */
