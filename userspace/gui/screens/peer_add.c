/*
 * screens/peer_add.c - typing in someone to talk to
 *
 * An address typed by hand, because on this network nothing announces itself.
 * Zalo has a directory server; a LAN has broadcast, and a box that answered
 * every announcement on the segment would collect whatever else is plugged in.
 * Discovery is a design of its own — until it exists, the honest interface is
 * the one that asks.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "peer_add.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../core/keyboard.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../services/chat.h"

static lv_obj_t *name_field;
static lv_obj_t *addr_field;
static lv_obj_t *hint;

static void on_deleted(lv_event_t *e)
{
	(void)e;
	name_field = NULL;
	addr_field = NULL;
	hint       = NULL;
}

static void on_save(lv_event_t *e)
{
	(void)e;

	if (!name_field || !addr_field)
		return;

	const char *name = lv_textarea_get_text(name_field);
	const char *addr = lv_textarea_get_text(addr_field);

	if (chat_peer_add(name, addr) < 0) {
		/*
		 * Refused, and the screen stays.  Popping back on failure would
		 * leave the user at a contact list with nothing new in it and
		 * no idea why — the two failures here, a bad address and a full
		 * list, are both things the person can act on if told.
		 */
		lv_label_set_text(hint,
				  "Cannot add — check the name and the address");
		lv_obj_set_style_text_color(hint, theme_color(THEME_DANGER), 0);
		return;
	}

	gui_logf("event: peer added %s\n", addr);
	nav_pop();
}

static lv_obj_t *field(lv_obj_t *parent, const char *placeholder, int y)
{
	lv_obj_t *ta = lv_textarea_create(parent);

	lv_textarea_set_one_line(ta, true);
	lv_textarea_set_placeholder_text(ta, placeholder);
	lv_obj_set_size(ta, SCREEN_W - 40, 56);
	lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, y);
	lv_obj_set_style_anim_duration(ta, 0, LV_PART_CURSOR);
	lv_obj_set_style_bg_color(ta, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(ta, RADIUS_MD, 0);
	lv_obj_set_style_border_width(ta, 0, 0);
	lv_obj_set_style_text_color(ta, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, 0);
	lv_obj_set_style_text_color(ta, theme_color(THEME_SUBTEXT),
				    LV_PART_TEXTAREA_PLACEHOLDER);
	return ta;
}

void peer_add_create(lv_obj_t *screen, void *arg)
{
	(void)arg;

	app_header_create(screen, "New contact", NULL);

	name_field = field(screen, "Name", APP_HEADER_HEIGHT + 24);
	addr_field = field(screen, "10.42.0.1", APP_HEADER_HEIGHT + 96);

	/*
	 * The address keyboard is the text one.  A digits-only mode would be
	 * the right answer for a phone number and the wrong one here: the dot
	 * is part of the address, and a keyboard that cannot type it turns a
	 * correct entry into an impossible one.
	 */
	gui_keyboard_attach(name_field, LV_KEYBOARD_MODE_TEXT_LOWER);
	gui_keyboard_attach(addr_field, LV_KEYBOARD_MODE_NUMBER);

	hint = lv_label_create(screen);
	lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
	lv_obj_set_width(hint, SCREEN_W - 40);
	lv_label_set_text(hint,
			  "The address of the other box, on this network.");
	lv_obj_set_style_text_color(hint, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
	lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, APP_HEADER_HEIGHT + 164);

	lv_obj_t *save = lv_button_create(screen);
	lv_obj_remove_style_all(save);
	lv_obj_set_size(save, SCREEN_W - 40, 56);
	lv_obj_align(save, LV_ALIGN_TOP_MID, 0, APP_HEADER_HEIGHT + 224);
	lv_obj_set_style_radius(save, RADIUS_MD, 0);
	lv_obj_set_style_bg_opa(save, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(save, theme_color(THEME_ACCENT), 0);
	lv_obj_set_style_bg_grad_color(save, theme_color(THEME_ACCENT_2), 0);
	lv_obj_set_style_bg_grad_dir(save, LV_GRAD_DIR_HOR, 0);
	lv_obj_add_event_cb(save, on_save, LV_EVENT_CLICKED, NULL);

	lv_obj_t *lbl = lv_label_create(save);
	lv_label_set_text(lbl, "Save");
	lv_obj_set_style_text_color(lbl, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
	lv_obj_center(lbl);

	lv_obj_add_event_cb(screen, on_deleted, LV_EVENT_DELETE, NULL);
}
