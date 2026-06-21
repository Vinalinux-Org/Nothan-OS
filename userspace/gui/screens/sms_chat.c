/*
 * screens/sms_chat.c - SMS: one conversation thread + composer
 *
 * Sending is visual for now — it lands on the SIM7600CE SMS path once
 * the telephony stack exists (mock-data phase).
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "sms_chat.h"
#include "dialer.h"
#include "../theme/theme.h"
#include "../core/log.h"
#include "../core/nav.h"
#include "../core/keyboard.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../services/messages.h"

#define INPUT_H     56
#define BUBBLE_MAXW 240

/* One composer exists at a time, so a file-scope handle is enough. */
static lv_obj_t *chat_input;

static void on_call(lv_event_t *e)
{
	const struct sms_conversation *c = lv_event_get_user_data(e);
	gui_logf("event: call %s\n", c ? c->peer : "?");
	nav_push(dialer_create, c ? (void *)c->peer : NULL);
}

static void on_send(lv_event_t *e)
{
	(void)e;
	const char *text = lv_textarea_get_text(chat_input);
	gui_logf("event: send msg '%s'\n", text ? text : "");
	/* Append + transmit once telephony lands; clear the field for now. */
	lv_textarea_set_text(chat_input, "");
}

static void add_bubble(lv_obj_t *list, const struct sms_message *m)
{
	/* Full-width row that pushes the bubble to one side. */
	lv_obj_t *row = lv_obj_create(list);
	lv_obj_remove_style_all(row);
	lv_obj_set_width(row, lv_pct(100));
	lv_obj_set_height(row, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row,
			      m->sent ? LV_FLEX_ALIGN_END : LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *bubble = lv_obj_create(row);
	lv_obj_remove_style_all(bubble);
	lv_obj_set_width(bubble, LV_SIZE_CONTENT);
	lv_obj_set_height(bubble, LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(bubble, 8, 0);
	lv_obj_set_style_radius(bubble, RADIUS_MD, 0);
	lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
	lv_obj_set_flex_flow(bubble, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(bubble,
			      LV_FLEX_ALIGN_START,
			      m->sent ? LV_FLEX_ALIGN_END : LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_row(bubble, 2, 0);
	lv_obj_clear_flag(bubble, LV_OBJ_FLAG_SCROLLABLE);
	if (m->sent) {
		lv_obj_set_style_bg_color(bubble, theme_color(THEME_ACCENT), 0);
		lv_obj_set_style_bg_grad_color(bubble, theme_color(THEME_ACCENT_2), 0);
		lv_obj_set_style_bg_grad_dir(bubble, LV_GRAD_DIR_HOR, 0);
	} else {
		lv_obj_set_style_bg_color(bubble, theme_color(THEME_SURFACE), 0);
	}

	lv_obj_t *txt = lv_label_create(bubble);
	lv_label_set_long_mode(txt, LV_LABEL_LONG_WRAP);
	lv_obj_set_width(txt, LV_SIZE_CONTENT);
	lv_obj_set_style_max_width(txt, BUBBLE_MAXW, 0);
	lv_label_set_text(txt, m->text);
	lv_obj_set_style_text_color(txt, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(txt, &lv_font_montserrat_14, 0);

}

static lv_obj_t *build_input_bar(lv_obj_t *parent)
{
	lv_obj_t *bar = lv_obj_create(parent);
	lv_obj_remove_style_all(bar);
	lv_obj_set_size(bar, lv_pct(100), INPUT_H);
	lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -NAV_BAR_HEIGHT);
	lv_obj_set_style_bg_color(bar, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_hor(bar, 10, 0);
	lv_obj_set_style_pad_column(bar, 8, 0);
	lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

	chat_input = lv_textarea_create(bar);
	lv_textarea_set_one_line(chat_input, true);
	lv_textarea_set_placeholder_text(chat_input, "Message");
	lv_obj_set_height(chat_input, 40);
	lv_obj_set_flex_grow(chat_input, 1);
	lv_obj_set_style_bg_color(chat_input, theme_color(THEME_BG), 0);
	lv_obj_set_style_bg_opa(chat_input, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(chat_input, 20, 0);
	lv_obj_set_style_border_width(chat_input, 0, 0);
	lv_obj_set_style_text_color(chat_input, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(chat_input, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(chat_input, theme_color(THEME_SUBTEXT),
				    LV_PART_TEXTAREA_PLACEHOLDER);

	lv_obj_t *send = lv_button_create(bar);
	lv_obj_remove_style_all(send);
	lv_obj_set_size(send, 40, 40);
	lv_obj_set_style_radius(send, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_opa(send, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(send, theme_color(THEME_ACCENT), 0);
	lv_obj_set_style_bg_grad_color(send, theme_color(THEME_ACCENT_2), 0);
	lv_obj_set_style_bg_grad_dir(send, LV_GRAD_DIR_VER, 0);
	lv_obj_add_event_cb(send, on_send, LV_EVENT_CLICKED, NULL);

	lv_obj_t *glyph = lv_label_create(send);
	lv_label_set_text(glyph, LV_SYMBOL_UPLOAD);
	lv_obj_set_style_text_color(glyph, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(glyph, &lv_font_montserrat_16, 0);
	lv_obj_center(glyph);

	gui_keyboard_attach(chat_input, LV_KEYBOARD_MODE_TEXT_LOWER);
	return bar;
}

void sms_chat_create(lv_obj_t *screen, void *arg)
{
	const struct sms_conversation *c = arg;
	gui_logf("screen: sms-chat (%s)\n", c ? c->peer : "?");

	lv_obj_t *call = app_header_create(screen, c ? c->peer : "Chat",
					   LV_SYMBOL_CALL);
	if (call)
		lv_obj_add_event_cb(call, on_call, LV_EVENT_CLICKED, (void *)c);

	lv_obj_t *input_bar = build_input_bar(screen);
	gui_keyboard_set_lift(input_bar, -(int32_t)NAV_BAR_HEIGHT);

	int list_top    = APP_HEADER_HEIGHT;
	int list_bottom = NAV_BAR_HEIGHT + INPUT_H;
	lv_obj_t *list = lv_obj_create(screen);
	lv_obj_remove_style_all(list);
	lv_obj_set_size(list, lv_pct(100), 640 - list_top - list_bottom);
	lv_obj_align(list, LV_ALIGN_TOP_MID, 0, list_top);
	lv_obj_set_style_pad_hor(list, 12, 0);
	lv_obj_set_style_pad_ver(list, 8, 0);
	lv_obj_set_style_pad_row(list, 10, 0);
	lv_obj_set_scroll_dir(list, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);

	for (int i = 0; c && i < c->message_count; i++)
		add_bubble(list, &c->messages[i]);
}
