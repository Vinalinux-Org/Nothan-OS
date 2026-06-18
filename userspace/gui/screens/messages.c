/*
 * screens/messages.c - Messages app: nav header + conversation list
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "messages.h"
#include "../widgets/status_bar.h"

struct convo {
	const char *initials;
	uint32_t    avatar_color;
	const char *name;
	const char *preview;
	const char *time;
};

static const struct convo convos[5] = {
	{ "MN", 0x3498DB, "Minh Nguyen",   "See you tomorrow at 9",     "09:32" },
	{ "LP", 0xE67E22, "Linh Pham",     "Got the files, thanks!",    "08:15" },
	{ "TT", 0x2ECC71, "Tuan Tran",     "Heading to the office now", "Mon"   },
	{ "HD", 0x9B59B6, "Hai Doan",      "Ship it 🚀",                "Sun"   },
	{ "AT", 0xE74C3C, "Anh Thao",      "Happy birthday!",           "Fri"   },
};

static void add_convo(lv_obj_t *list, const struct convo *c)
{
	lv_obj_t *row = lv_obj_create(list);
	lv_obj_set_size(row, lv_pct(100), 72);
	lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(row, 0, 0);
	lv_obj_set_style_border_color(row, lv_color_hex(0xE0E0E0), 0);
	lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
	lv_obj_set_style_border_width(row, 1, 0);
	lv_obj_set_style_pad_all(row, 8, 0);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

	/* Round avatar with initials */
	lv_obj_t *avatar = lv_obj_create(row);
	lv_obj_set_size(avatar, 48, 48);
	lv_obj_align(avatar, LV_ALIGN_LEFT_MID, 0, 0);
	lv_obj_set_style_bg_color(avatar, lv_color_hex(c->avatar_color), 0);
	lv_obj_set_style_radius(avatar, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_border_width(avatar, 0, 0);
	lv_obj_clear_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *ini = lv_label_create(avatar);
	lv_label_set_text(ini, c->initials);
	lv_obj_set_style_text_color(ini, lv_color_white(), 0);
	lv_obj_set_style_text_font(ini, &lv_font_montserrat_18, 0);
	lv_obj_center(ini);

	/* Name + preview, stacked to the right of the avatar */
	lv_obj_t *name = lv_label_create(row);
	lv_label_set_text(name, c->name);
	lv_obj_set_style_text_color(name, lv_color_hex(0x111111), 0);
	lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
	lv_obj_align(name, LV_ALIGN_TOP_LEFT, 60, 4);

	lv_obj_t *preview = lv_label_create(row);
	lv_label_set_text(preview, c->preview);
	lv_obj_set_style_text_color(preview, lv_color_hex(0x666666), 0);
	lv_obj_set_style_text_font(preview, &lv_font_montserrat_12, 0);
	lv_obj_align(preview, LV_ALIGN_BOTTOM_LEFT, 60, -4);

	/* Time on the right edge */
	lv_obj_t *time = lv_label_create(row);
	lv_label_set_text(time, c->time);
	lv_obj_set_style_text_color(time, lv_color_hex(0x999999), 0);
	lv_obj_set_style_text_font(time, &lv_font_montserrat_12, 0);
	lv_obj_align(time, LV_ALIGN_TOP_RIGHT, 0, 4);
}

lv_obj_t *messages_create(lv_obj_t *parent)
{
	lv_obj_set_style_bg_color(parent, lv_color_white(), 0);
	lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

	status_bar_create(parent);

	/* Nav header: back arrow + screen title */
	lv_obj_t *header = lv_obj_create(parent);
	lv_obj_set_size(header, lv_pct(100), 48);
	lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 32);
	lv_obj_set_style_bg_color(header, lv_color_hex(0x3498DB), 0);
	lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(header, 0, 0);
	lv_obj_set_style_radius(header, 0, 0);
	lv_obj_set_style_pad_all(header, 8, 0);
	lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *back = lv_label_create(header);
	lv_label_set_text(back, LV_SYMBOL_LEFT);
	lv_obj_set_style_text_color(back, lv_color_white(), 0);
	lv_obj_set_style_text_font(back, &lv_font_montserrat_18, 0);
	lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);

	lv_obj_t *title = lv_label_create(header);
	lv_label_set_text(title, "Messages");
	lv_obj_set_style_text_color(title, lv_color_white(), 0);
	lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
	lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

	/* Conversation list */
	lv_obj_t *list = lv_obj_create(parent);
	lv_obj_set_size(list, lv_pct(100), 640 - 32 - 48);
	lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 32 + 48);
	lv_obj_set_style_bg_color(list, lv_color_white(), 0);
	lv_obj_set_style_border_width(list, 0, 0);
	lv_obj_set_style_radius(list, 0, 0);
	lv_obj_set_style_pad_all(list, 0, 0);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

	for (int i = 0; i < 5; i++)
		add_convo(list, &convos[i]);

	return parent;
}
