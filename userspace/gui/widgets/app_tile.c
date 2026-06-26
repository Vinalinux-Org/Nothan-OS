/*
 * widgets/app_tile.c - Squircle app icon with optional label
 *
 * Pass label = NULL for a label-less variant used in the dock.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "app_tile.h"
#include "../theme/theme.h"

#define BADGE_GRID   64
#define BADGE_DOCK   60
#define TILE_W       104	/* grid tile: width budget so labels don't clip */
#define LABEL_GAP    6		/* space between badge bottom and label */
#define TILE_H       (BADGE_GRID + LABEL_GAP + 20)	/* 20 ≈ 14px font line */

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
	/* BISECT: vertical gradient OFF — test if the bg_grad render is what
	 * corrupts an object's style metadata during the home-grid scroll. */

	/* Drop shadow OFF — flat, simple look. (Shadows were ruled out as the
	 * heap-corruption cause; kept off for the clean style.) */

	/* Pressed: dim slightly, instant color change, no animation. */
	lv_obj_set_style_bg_color(badge, lv_color_darken(base, 50), LV_STATE_PRESSED);

	lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *app_tile_create(lv_obj_t *parent, const char *symbol,
			  const char *label, uint32_t color)
{
	int badge_size = label ? BADGE_GRID : BADGE_DOCK;

	lv_obj_t *tile = lv_obj_create(parent);
	lv_obj_remove_style_all(tile);
	/* Grid (labeled) tiles use TILE_W so the label has room; the grid
	 * layout centers each tile in its column. Dock (label-less) tiles
	 * shrink to the badge so the floating tray keeps wide even gaps. */
	if (label)
		lv_obj_set_size(tile, TILE_W, TILE_H);
	else
		lv_obj_set_size(tile, badge_size, badge_size);
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
	lv_obj_set_style_text_font(sym, &lv_font_montserrat_28, 0);
	lv_obj_center(sym);

	if (label) {
		lv_obj_t *lbl = lv_label_create(tile);
		lv_label_set_text(lbl, label);
		lv_obj_set_style_text_color(lbl, theme_color(THEME_TEXT), 0);
		lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
		/* Sit just under the badge instead of the tile bottom, so the
		 * label hugs its icon. */
		lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, badge_size + LABEL_GAP);
	}

	return tile;
}
