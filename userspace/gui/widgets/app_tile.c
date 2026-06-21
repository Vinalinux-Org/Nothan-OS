/*
 * widgets/app_tile.c - Squircle app icon with optional label
 *
 * Pass label = NULL for a label-less variant used in the dock.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "app_tile.h"
#include "../theme/theme.h"

#define BADGE_GRID   54
#define BADGE_DOCK   58
#define TILE_W       80
#define LABEL_GAP    4		/* space between badge bottom and label */
#define TILE_H       (BADGE_GRID + LABEL_GAP + 18)	/* 18 ≈ 12px font line */

static void style_badge(lv_obj_t *badge, uint32_t color, int size)
{
	lv_color_t base = lv_color_hex(color);
	lv_color_t hi   = lv_color_lighten(base, 30);

	lv_obj_remove_style_all(badge);
	lv_obj_set_size(badge, size, size);
	/* Squircle: ~22% radius gives the MIUI/iOS rounded-square look. */
	lv_obj_set_style_radius(badge, (size * 22) / 100, 0);
	lv_obj_set_style_bg_color(badge, hi, 0);
	lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_grad_color(badge, base, 0);
	lv_obj_set_style_bg_grad_dir(badge, LV_GRAD_DIR_VER, 0);

	/* Soft drop shadow — gives icons depth against the dark surface. */
	lv_obj_set_style_shadow_color(badge, lv_color_black(), 0);
	lv_obj_set_style_shadow_opa(badge, LV_OPA_40, 0);
	lv_obj_set_style_shadow_width(badge, 10, 0);
	lv_obj_set_style_shadow_spread(badge, 0, 0);
	lv_obj_set_style_shadow_offset_y(badge, 3, 0);

	/* Pressed: dim slightly, instant color change, no animation. */
	lv_obj_set_style_bg_color(badge, lv_color_darken(base, 50), LV_STATE_PRESSED);
	lv_obj_set_style_bg_grad_color(badge, lv_color_darken(base, 80), LV_STATE_PRESSED);

	lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *app_tile_create(lv_obj_t *parent, const char *symbol,
			  const char *label, uint32_t color)
{
	int badge_size = label ? BADGE_GRID : BADGE_DOCK;

	lv_obj_t *tile = lv_obj_create(parent);
	lv_obj_remove_style_all(tile);
	/* Width is always TILE_W so grid and dock SPACE_EVENLY math match —
	 * column centers align between rows. Height differs to drop the
	 * label area on the dock variant. */
	lv_obj_set_size(tile, TILE_W, label ? TILE_H : badge_size);
	lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

	/* The badge IS the interactive widget — a real lv_button, so it
	 * carries CLICKABLE + the PRESSED state styling below for free.
	 * The tile around it is just a layout holder (badge + label). */
	lv_obj_t *badge = lv_button_create(tile);
	style_badge(badge, color, badge_size);
	/* Align AFTER styling: style_badge() does remove_style_all(), which
	 * wipes any alignment set before it — that left every badge pinned
	 * to the tile's top-left and the whole grid looked shifted left. */
	if (label)
		lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 0);
	else
		lv_obj_center(badge);

	lv_obj_t *sym = lv_label_create(badge);
	lv_label_set_text(sym, symbol);
	lv_obj_set_style_text_color(sym, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(sym, &lv_font_montserrat_24, 0);
	lv_obj_center(sym);

	if (label) {
		lv_obj_t *lbl = lv_label_create(tile);
		lv_label_set_text(lbl, label);
		lv_obj_set_style_text_color(lbl, theme_color(THEME_TEXT), 0);
		lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
		/* Sit just under the badge instead of the tile bottom, so the
		 * label hugs its icon. */
		lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, badge_size + LABEL_GAP);
	}

	return tile;
}
