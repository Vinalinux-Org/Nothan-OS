#ifndef __GUI_THEME_H
#define __GUI_THEME_H

#include "lvgl/lvgl.h"

/*
 * Logical screen size (portrait). The panel is native 800×480 landscape,
 * mounted rotated 90°; the kernel rotates this portrait output in lcdc_flush.
 * Use these instead of hardcoding pixel dimensions.
 */
#define SCREEN_W   480
#define SCREEN_H   800

/*
 * Deep Space color tokens — see Documentation/02-gui-design.md
 */
#define THEME_BG          0x0A0E1A   /* screen background */
#define THEME_SURFACE     0x141929   /* card, status bar, dock */
#define THEME_ACCENT      0x7C3AED   /* primary accent (gradient start) */
#define THEME_ACCENT_2    0x3B82F6   /* accent gradient end */
#define THEME_TEXT        0xF8FAFC   /* primary text */
#define THEME_SUBTEXT     0x64748B   /* secondary text, labels */
#define THEME_BORDER      0x1E2538   /* subtle dividers on dark bg */
#define THEME_DANGER      0xEF4444   /* hangup, reject */
#define THEME_SUCCESS     0x22C55E   /* accept call */

/*
 * Corner-radius scale — one consistent rounding language across the UI.
 * Use these instead of ad-hoc numbers so every surface rounds alike.
 */
#define RADIUS_SM   8    /* small controls: nav keys, chips */
#define RADIUS_MD   16   /* cards */
#define RADIUS_LG   26   /* floating panels: dock */

static inline lv_color_t theme_color(uint32_t hex)
{
	return lv_color_hex(hex);
}

/* Apply a visible blinking cursor to a textarea.
 * Without an active LVGL theme, LV_PART_CURSOR is transparent by default. */
static inline void theme_apply_cursor(lv_obj_t *ta)
{
	lv_obj_set_style_bg_opa(ta, LV_OPA_TRANSP, LV_PART_CURSOR);
	lv_obj_set_style_border_side(ta, LV_BORDER_SIDE_LEFT, LV_PART_CURSOR);
	lv_obj_set_style_border_width(ta, 2, LV_PART_CURSOR);
	lv_obj_set_style_border_color(ta, theme_color(THEME_TEXT), LV_PART_CURSOR);
	lv_obj_set_style_border_opa(ta, LV_OPA_COVER, LV_PART_CURSOR);
}

#endif
