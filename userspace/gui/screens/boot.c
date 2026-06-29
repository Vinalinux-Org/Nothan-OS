/*
 * screens/boot.c - MyNuong splash: wordmark + tagline
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "boot.h"
#include "../theme/theme.h"
#include "../core/log.h"

#define BOOT_BAR_DELAY_MS 3000	/* hold the bar at 0 while the HDMI panel locks
				 * (it takes ~1-2s to sync), so by the time the
				 * screen actually shows, the bar is still near empty
				 * instead of already half-filled. */
#define BOOT_BAR_MS  5000	/* fill time; delay + fill (8.0s) sits under BOOT_MS=8700 */

/* lv_anim drives an int 0..100 into the bar value as it fills. */
static void boot_bar_anim_cb(void *bar, int32_t v)
{
	lv_bar_set_value((lv_obj_t *)bar, v, LV_ANIM_OFF);
}

void boot_create(lv_obj_t *parent, void *arg)
{
	(void)arg;
	gui_log("screen: boot\n");
	lv_obj_set_style_bg_color(parent, theme_color(THEME_BG), 0);
	lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
	lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *logo = lv_label_create(parent);
	lv_label_set_text(logo, "MyNuong");
	lv_obj_set_style_text_color(logo, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(logo, &lv_font_montserrat_48, 0);
	lv_obj_align(logo, LV_ALIGN_CENTER, 0, -90);

	lv_obj_t *tagline = lv_label_create(parent);
	lv_label_set_text(tagline, "Developed by Vinalinux");
	lv_obj_set_style_text_color(tagline, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(tagline, &lv_font_montserrat_18, 0);
	lv_obj_align(tagline, LV_ALIGN_CENTER, 0, -42);

	/* Progress bar that fills gradually while the splash holds. Track on the
	 * surface color, indicator in the accent; rounded to read as a pill. */
	lv_obj_t *bar = lv_bar_create(parent);
	lv_obj_set_size(bar, 220, 6);
	lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -80);
	lv_bar_set_range(bar, 0, 100);
	lv_bar_set_value(bar, 0, LV_ANIM_OFF);
	lv_obj_set_style_bg_color(bar, theme_color(THEME_SURFACE), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
	lv_obj_set_style_bg_color(bar, theme_color(THEME_ACCENT), LV_PART_INDICATOR);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
	lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);

	lv_anim_t a;
	lv_anim_init(&a);
	lv_anim_set_var(&a, bar);
	lv_anim_set_exec_cb(&a, boot_bar_anim_cb);
	lv_anim_set_values(&a, 0, 100);
	lv_anim_set_duration(&a, BOOT_BAR_MS);
	lv_anim_set_delay(&a, BOOT_BAR_DELAY_MS);
	lv_anim_start(&a);
}
