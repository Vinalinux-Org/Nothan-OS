/*
 * widgets/avatar.c - Round gradient avatar with a single initial
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "avatar.h"
#include "../theme/theme.h"

lv_obj_t *avatar_create(lv_obj_t *parent, char initial, int size,
			const lv_font_t *font)
{
	lv_obj_t *av = lv_obj_create(parent);
	lv_obj_remove_style_all(av);
	lv_obj_set_size(av, size, size);
	lv_obj_set_style_radius(av, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(av, theme_color(THEME_ACCENT), 0);
	lv_obj_set_style_bg_grad_color(av, theme_color(THEME_ACCENT_2), 0);
	lv_obj_set_style_bg_grad_dir(av, LV_GRAD_DIR_VER, 0);
	lv_obj_set_style_bg_opa(av, LV_OPA_COVER, 0);
	lv_obj_clear_flag(av, LV_OBJ_FLAG_SCROLLABLE);

	char s[2] = { initial, '\0' };
	lv_obj_t *lbl = lv_label_create(av);
	lv_label_set_text(lbl, s);
	lv_obj_set_style_text_color(lbl, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(lbl, font, 0);
	lv_obj_center(lbl);
	return av;
}
