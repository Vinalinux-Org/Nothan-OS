/*
 * screens/contacts_add.c - Contacts: add-contact form
 *
 * Text entry needs an input device + keyboard, neither of which exists
 * yet (HDMI output). The widget tree and the Save path are complete, so
 * the form works the moment an input backend lands.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "contacts_add.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../core/keyboard.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../services/contacts.h"

/* Only one add/edit form exists at a time, so file-scope state is enough.
 * edit_index = -1 means "add new"; >= 0 means "edit that contact". */
static lv_obj_t *name_field;
static lv_obj_t *phone_field;
static int       edit_index = -1;

static void on_save(lv_event_t *e)
{
	(void)e;
	const char *name  = lv_textarea_get_text(name_field);
	const char *phone = lv_textarea_get_text(phone_field);

	gui_logf("event: save contact name='%s' phone='%s' (%s)\n",
		 name ? name : "", phone ? phone : "",
		 edit_index >= 0 ? "edit" : "add");
	if (name && name[0]) {
		if (edit_index >= 0)
			contacts_update(edit_index, name, phone);
		else
			contacts_add(name, phone);
	}
	nav_pop();
}

static lv_obj_t *field_label(lv_obj_t *parent, const char *text, int y)
{
	lv_obj_t *lbl = lv_label_create(parent);
	lv_label_set_text(lbl, text);
	lv_obj_set_style_text_color(lbl, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
	lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 16, y);
	return lbl;
}

static lv_obj_t *text_field(lv_obj_t *parent, int y, const char *accepted)
{
	lv_obj_t *ta = lv_textarea_create(parent);
	lv_textarea_set_one_line(ta, true);
	if (accepted)
		lv_textarea_set_accepted_chars(ta, accepted);
	lv_obj_set_size(ta, lv_pct(92), 44);
	lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, y);
	lv_obj_set_style_bg_color(ta, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(ta, RADIUS_MD, 0);
	lv_obj_set_style_border_width(ta, 0, 0);
	lv_obj_set_style_text_color(ta, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(ta, &lv_font_montserrat_16, 0);
	return ta;
}

void contacts_add_create(lv_obj_t *screen, void *arg)
{
	/* arg encodes the mode: 0 = add new, n = edit contact (n-1). */
	edit_index = (int)(long)arg - 1;
	const struct contact *cur =
		edit_index >= 0 ? contacts_get(edit_index) : NULL;
	gui_logf("screen: contacts-%s\n", cur ? "edit" : "add");

	app_header_create(screen, cur ? "Edit contact" : "Add contact", NULL);

	int y = APP_HEADER_HEIGHT + 24;
	field_label(screen, "Name", y);
	name_field = text_field(screen, y + 22, NULL);
	if (cur)
		lv_textarea_set_text(name_field, cur->name);
	gui_keyboard_attach(name_field, LV_KEYBOARD_MODE_TEXT_LOWER);

	y += 22 + 44 + 20;
	field_label(screen, "Phone", y);
	phone_field = text_field(screen, y + 22, "0123456789 +");
	if (cur)
		lv_textarea_set_text(phone_field, cur->phone);
	gui_keyboard_attach(phone_field, LV_KEYBOARD_MODE_NUMBER);

	lv_obj_t *save = lv_button_create(screen);
	lv_obj_remove_style_all(save);
	lv_obj_set_size(save, lv_pct(92), 48);
	lv_obj_align(save, LV_ALIGN_BOTTOM_MID, 0, -(NAV_BAR_HEIGHT + 16));
	lv_obj_set_style_radius(save, RADIUS_MD, 0);
	lv_obj_set_style_bg_opa(save, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(save, theme_color(THEME_ACCENT), 0);
	lv_obj_set_style_bg_grad_color(save, theme_color(THEME_ACCENT_2), 0);
	lv_obj_set_style_bg_grad_dir(save, LV_GRAD_DIR_HOR, 0);
	lv_obj_add_event_cb(save, on_save, LV_EVENT_CLICKED, NULL);

	lv_obj_t *lbl = lv_label_create(save);
	lv_label_set_text(lbl, "Save contact");
	lv_obj_set_style_text_color(lbl, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
	lv_obj_center(lbl);
}
