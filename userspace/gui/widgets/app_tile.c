/*
 * widgets/app_tile.c - Square app icon used in the home grid
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "app_tile.h"

#define TILE_SIZE   96
#define BADGE_SIZE  60

static void glow_anim_cb(void *obj, int32_t v)
{
	lv_obj_set_style_border_width((lv_obj_t *)obj, (lv_coord_t)v, 0);
}

lv_obj_t *app_tile_create(lv_obj_t *parent, const char *symbol,
			  const char *label, uint32_t color)
{
	lv_obj_t *tile = lv_obj_create(parent);
	lv_obj_set_size(tile, TILE_SIZE, TILE_SIZE);
	lv_obj_set_style_bg_opa(tile, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(tile, 0, 0);
	lv_obj_set_style_pad_all(tile, 0, 0);
	lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

	/* Colored badge that holds the symbol */
	lv_obj_t *badge = lv_obj_create(tile);
	lv_obj_set_size(badge, BADGE_SIZE, BADGE_SIZE);
	lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_style_bg_color(badge, lv_color_hex(color), 0);
	lv_obj_set_style_bg_grad_color(badge, lv_color_darken(lv_color_hex(color), 60), 0);
	lv_obj_set_style_bg_grad_dir(badge, LV_GRAD_DIR_VER, 0);
	lv_obj_set_style_radius(badge, 16, 0);
	lv_obj_set_style_border_width(badge, 0, 0);
	lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *sym = lv_label_create(badge);
	lv_label_set_text(sym, symbol);
	lv_obj_set_style_text_color(sym, lv_color_white(), 0);
	lv_obj_set_style_text_font(sym, &lv_font_montserrat_24, 0);
	lv_obj_center(sym);

	/* Label under badge */
	lv_obj_t *lbl = lv_label_create(tile);
	lv_label_set_text(lbl, label);
	lv_obj_set_style_text_color(lbl, lv_color_hex(0x333333), 0);
	lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
	lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -2);

	return tile;
}

void app_tile_glow(lv_obj_t *tile)
{
	lv_obj_t *badge = lv_obj_get_child(tile, 0);
	lv_obj_set_style_border_color(badge, lv_color_hex(0xFFD700), 0);
	lv_obj_set_style_border_opa(badge, LV_OPA_COVER, 0);

	lv_anim_t a;
	lv_anim_init(&a);
	lv_anim_set_var(&a, badge);
	lv_anim_set_values(&a, 0, 6);
	lv_anim_set_time(&a, 400);
	lv_anim_set_playback_time(&a, 400);
	lv_anim_set_repeat_count(&a, 3);
	lv_anim_set_exec_cb(&a, glow_anim_cb);
	lv_anim_start(&a);
}
