/*
 * screens/boot.c - MiNuong splash: wordmark + tagline + progress bar
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "boot.h"
#include "../theme/theme.h"
#include "../core/log.h"

#define BAR_WIDTH    160
#define BAR_HEIGHT   3
#define BAR_FILL_MS  6800   /* ~boot phase duration; ~200ms before swap */

static void bar_set_cb(void *obj, int32_t v)
{
	lv_bar_set_value((lv_obj_t *)obj, v, LV_ANIM_OFF);
}

void boot_create(lv_obj_t *parent, void *arg)
{
	(void)arg;
	gui_log("screen: boot\n");
	lv_obj_set_style_bg_color(parent, theme_color(THEME_BG), 0);
	lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
	lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *logo = lv_label_create(parent);
	lv_label_set_text(logo, "MiNuong");
	lv_obj_set_style_text_color(logo, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(logo, &lv_font_montserrat_42, 0);
	lv_obj_align(logo, LV_ALIGN_CENTER, 0, -24);

	lv_obj_t *tagline = lv_label_create(parent);
	lv_label_set_text(tagline, "Developed by Vinalinux");
	lv_obj_set_style_text_color(tagline, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(tagline, &lv_font_montserrat_14, 0);
	lv_obj_align(tagline, LV_ALIGN_CENTER, 0, 24);

	/* Thin loading bar with accent gradient, near the bottom. */
	lv_obj_t *bar = lv_bar_create(parent);
	lv_obj_set_size(bar, BAR_WIDTH, BAR_HEIGHT);
	lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -80);
	lv_obj_set_style_bg_color(bar, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_border_width(bar, 0, 0);
	lv_obj_set_style_radius(bar, BAR_HEIGHT, 0);

	lv_obj_set_style_bg_color(bar, theme_color(THEME_ACCENT), LV_PART_INDICATOR);
	lv_obj_set_style_bg_grad_color(bar, theme_color(THEME_ACCENT_2), LV_PART_INDICATOR);
	lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
	lv_obj_set_style_radius(bar, BAR_HEIGHT, LV_PART_INDICATOR);

	lv_bar_set_range(bar, 0, 100);
	lv_bar_set_value(bar, 0, LV_ANIM_OFF);

	/* Single linear fill 0→100 over boot duration — functional progress,
	 * not decorative animation. */
	lv_anim_t a;
	lv_anim_init(&a);
	lv_anim_set_var(&a, bar);
	lv_anim_set_values(&a, 0, 100);
	lv_anim_set_time(&a, BAR_FILL_MS);
	lv_anim_set_exec_cb(&a, bar_set_cb);
	lv_anim_start(&a);
}
