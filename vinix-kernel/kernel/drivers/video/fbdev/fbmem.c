/* ============================================================
 * fbmem.c — Framebuffer Graphics Primitives
 * RGB565, 800x600 @ 60Hz
 *
 * Owns the framebuffer pointer, dimensions, and pixel-level
 * primitives. Font data lives in lib/fonts/font_8x16.c; the
 * text console layered on top lives in fbcon.c.
 * ============================================================ */

#include "fb.h"
#include "lcdc.h"
#include "vinix/fb.h"
#include "vinix/printk.h"

static uint16_t *g_fb;
static uint32_t  g_width;
static uint32_t  g_height;
static uint32_t  g_pitch;    /* bytes per row */

/* Active fb_info — registered by the underlying display driver
 * (tilcdc) via register_framebuffer. fb_init copies geometry
 * out so the primitives below can run without dereferencing
 * fb_info on every pixel write. */
static struct fb_info *active_fb;

int register_framebuffer(struct fb_info *fb)
{
    if (!fb) return -1;
    active_fb = fb;
    pr_info("[FBDEV] %s registered (%ux%u, %u bpp)\n",
            fb->fix.id[0] ? fb->fix.id : "fb0",
            fb->var.xres, fb->var.yres, fb->var.bits_per_pixel);
    return 0;
}

int unregister_framebuffer(struct fb_info *fb)
{
    if (active_fb == fb) active_fb = 0;
    return 0;
}

void fb_init(void)
{
    /* Pull geometry from the registered fb_info if present, else
     * fall back to the legacy lcdc accessors during early bring-up. */
    if (active_fb) {
        g_fb     = (uint16_t *)active_fb->screen_base;
        g_width  = active_fb->var.xres;
        g_height = active_fb->var.yres;
        g_pitch  = active_fb->fix.line_length;
    } else {
        g_fb     = lcdc_get_framebuffer();
        g_width  = lcdc_get_width();
        g_height = lcdc_get_height();
        g_pitch  = lcdc_get_pitch();
    }
}

/* Geometry accessors used by fbcon and other layers that need
 * the screen size without depending on lcdc directly. */
uint32_t  fb_get_width(void)   { return g_width;  }
uint32_t  fb_get_height(void)  { return g_height; }
uint16_t *fb_get_buffer(void)  { return g_fb;     }

/* ============================================================
 * Primitives
 * ============================================================ */

void fb_putpixel(uint32_t x, uint32_t y, uint16_t color)
{
    if (x < g_width && y < g_height)
        g_fb[y * g_width + x] = color;
}

void fb_fillrect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color)
{
    if (x >= g_width || y >= g_height) return;
    if (x + w > g_width)  w = g_width - x;
    if (y + h > g_height) h = g_height - y;

    uint32_t packed = ((uint32_t)color << 16) | color;

    for (uint32_t row = y; row < y + h; row++) {
        uint16_t *line = &g_fb[row * g_width + x];
        uint32_t col = 0;

        /* Handle odd start (align to 32-bit) */
        if (((uintptr_t)line & 2) && col < w) {
            line[col++] = color;
        }

        /* Fast 32-bit fill */
        uint32_t *line32 = (uint32_t *)&line[col];
        uint32_t pairs = (w - col) / 2;
        for (uint32_t p = 0; p < pairs; p++) {
            line32[p] = packed;
        }
        col += pairs * 2;

        /* Handle odd remainder */
        if (col < w) {
            line[col] = color;
        }
    }
}

void fb_clear(uint16_t color)
{
    /* Pack two pixels into one 32-bit write for 2x speed
     * on non-cacheable framebuffer memory */
    uint32_t packed = ((uint32_t)color << 16) | color;
    uint32_t *fb32 = (uint32_t *)g_fb;
    uint32_t total = (g_width * g_height) / 2;
    for (uint32_t i = 0; i < total; i++) {
        fb32[i] = packed;
    }
}

/* ============================================================
 * Text rendering
 * ============================================================ */

void fb_draw_char(uint32_t x, uint32_t y, char c, uint16_t fg, uint16_t bg)
{
    const uint8_t *glyph = font_8x16_glyph(c);

    for (uint32_t row = 0; row < FB_FONT_H; row++) {
        uint32_t py = y + row;
        if (py >= g_height) break;

        uint8_t bits = glyph[row];
        for (uint32_t col = 0; col < FB_FONT_W; col++) {
            uint32_t px = x + col;
            if (px >= g_width) break;

            g_fb[py * g_width + px] = (bits & 0x80) ? fg : bg;
            bits <<= 1;
        }
    }
}

void fb_puts(uint32_t x, uint32_t y, const char *str, uint16_t fg, uint16_t bg)
{
    while (*str) {
        if (x + FB_FONT_W > g_width) break;
        fb_draw_char(x, y, *str, fg, bg);
        x += FB_FONT_W;
        str++;
    }
}

void fb_print(uint32_t col, uint32_t row, const char *str, uint16_t fg, uint16_t bg)
{
    fb_puts(col * FB_FONT_W, row * FB_FONT_H, str, fg, bg);
}

void fb_draw_char_scaled(uint32_t x, uint32_t y, char c, uint16_t fg, uint16_t bg,
                         uint32_t scale)
{
    const uint8_t *glyph = font_8x16_glyph(c);

    for (uint32_t row = 0; row < FB_FONT_H; row++) {
        uint8_t bits = glyph[row];
        for (uint32_t col = 0; col < FB_FONT_W; col++) {
            uint16_t color = (bits & 0x80) ? fg : bg;
            /* Fill a scale×scale block for each font pixel */
            fb_fillrect(x + col * scale, y + row * scale, scale, scale, color);
            bits <<= 1;
        }
    }
}

void fb_puts_scaled(uint32_t x, uint32_t y, const char *str, uint16_t fg, uint16_t bg,
                    uint32_t scale)
{
    while (*str) {
        fb_draw_char_scaled(x, y, *str, fg, bg, scale);
        x += FB_FONT_W * scale;
        str++;
    }
}

/* ============================================================
 * Circle drawing
 * ============================================================ */

/* Integer square root — scanline approach, no stdlib needed.
 * For r <= 55: max 55 iterations per call, negligible at 800MHz. */
static uint32_t fb_isqrt(uint32_t n)
{
    uint32_t x = 0;
    while ((x + 1) * (x + 1) <= n)
        x++;
    return x;
}

void fb_fillcircle(uint32_t cx, uint32_t cy, uint32_t r, uint16_t color)
{
    /* Scanline fill: for each dy, compute horizontal span dx.
     * fb_fillrect clips bounds, including uint32_t underflow cases
     * (e.g. cx < dx → cx-dx wraps to huge value → clipped out). */
    for (uint32_t dy = 0; dy <= r; dy++) {
        uint32_t dx = fb_isqrt(r * r - dy * dy);
        fb_fillrect(cx - dx, cy - dy, 2 * dx + 1, 1, color);
        if (dy > 0)
            fb_fillrect(cx - dx, cy + dy, 2 * dx + 1, 1, color);
    }
}
