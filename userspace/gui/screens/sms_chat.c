/*
 * screens/sms_chat.c - SMS: one conversation thread + composer
 *
 * Identified by conversation index. Sending appends to the store and the
 * thread rebuilds so the bubble appears immediately; opening (or returning
 * to) the thread marks it read and rebuilds, so messages received while away
 * show up. The thread auto-scrolls to the newest bubble.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "sms_chat.h"
#include "../theme/theme.h"
#include "../core/log.h"
#include "../services/modem_client.h"
#include "../core/nav.h"
#include "../core/keyboard.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../services/messages.h"

#define INPUT_H     68
#define BUBBLE_MAXW 240

static int        chat_idx;
static lv_obj_t  *chat_input;
static lv_obj_t  *chat_list;

/* An inbound message moves its thread to store index 0 (services/messages.c).
 * Remap chat_idx by the same rule so an open thread keeps pointing at its own
 * conversation. @moved_from = the pre-move index that jumped to the top.
 * Harmless when no thread is open — chat_idx is re-set on the next open. */
void sms_chat_reindex(int moved_from)
{
	if (chat_idx == moved_from) {
		chat_idx = 0;
	} else if (chat_idx < moved_from) {
		chat_idx += 1;
	}
	/* chat_idx > moved_from: its position is unchanged */
}


static void add_bubble(lv_obj_t *list, const struct sms_message *m)
{
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
	lv_obj_set_flex_align(bubble, LV_FLEX_ALIGN_START,
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
	lv_obj_set_style_text_font(txt, &lv_font_montserrat_20, 0);
}

/* Rebuild every bubble from the store and scroll to the newest. */
static void rebuild_thread(void)
{
	if (!chat_list) {
		return;
	}
	lv_obj_clean(chat_list);

	const struct sms_conversation *c = sms_conversation_get(chat_idx);
	lv_obj_t *last = NULL;
	for (int i = 0; c && i < c->message_count; i++) {
		add_bubble(chat_list, &c->messages[i]);
		last = lv_obj_get_child(chat_list, -1);
	}
	if (last) {
		lv_obj_update_layout(chat_list);
		lv_obj_scroll_to_view(last, LV_ANIM_OFF);
	}
}

static void on_send(lv_event_t *e)
{
	(void)e;
	const char *text = lv_textarea_get_text(chat_input);
	if (!text || !text[0]) return;
	if (!modem_net_registered()) {
		gui_toast("No network");
		return;
	}
	sms_send(chat_idx, text);
	lv_textarea_set_text(chat_input, "");

	/* Append only the new bubble — avoids the lv_obj_clean→scroll-reset→
	 * scroll_to_view jump that rebuild_thread() would cause mid-send. */
	const struct sms_conversation *c = sms_conversation_get(chat_idx);
	if (c && c->message_count > 0) {
		add_bubble(chat_list, &c->messages[c->message_count - 1]);
		lv_obj_t *last = lv_obj_get_child(chat_list, -1);
		if (last) {
			lv_obj_update_layout(chat_list);
			lv_obj_scroll_to_view(last, LV_ANIM_OFF);
		}
	}
}

static void on_screen_loaded(lv_event_t *e)
{
	(void)e;
	sms_mark_read(chat_idx);
	rebuild_thread();
}

static void on_screen_unloaded(lv_event_t *e)
{
	(void)e;
	chat_list  = NULL;
	chat_input = NULL;
}

/* Keyboard opening: shrink the thread so its bottom sits above the lifted
 * input bar (which floats just over the keyboard), then pin to the newest
 * message so it isn't hidden behind the keyboard. */
static void on_input_focus(lv_event_t *e)
{
	(void)e;
	lv_obj_set_height(chat_list,
			  SCREEN_H - APP_HEADER_HEIGHT - GUI_KEYBOARD_HEIGHT - INPUT_H);
	lv_obj_update_layout(chat_list);
	/* Only pin to the newest bubble when the thread actually overflows the
	 * (now shorter) list. For a short thread scroll_bottom is NEGATIVE — the
	 * unbounded scroll_by would then shove the top-aligned bubbles DOWN into
	 * the middle. Leave a short thread pinned at the top instead. */
	int32_t sb = lv_obj_get_scroll_bottom(chat_list);
	if (sb > 0)
		lv_obj_scroll_by(chat_list, 0, -sb, LV_ANIM_OFF);
}

static void on_input_blur(lv_event_t *e)
{
	(void)e;
	lv_obj_set_height(chat_list,
			  SCREEN_H - APP_HEADER_HEIGHT - NAV_BAR_HEIGHT - INPUT_H);
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
	lv_obj_set_height(chat_input, 52);
	lv_obj_set_flex_grow(chat_input, 1);
	/* Keep the caret solid (no blink) so the bigger font's caret doesn't
	 * jitter the line vertically on each blink frame. */
	lv_obj_set_style_anim_duration(chat_input, 0, LV_PART_CURSOR);
	lv_obj_set_style_bg_color(chat_input, theme_color(THEME_BG), 0);
	lv_obj_set_style_bg_opa(chat_input, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(chat_input, 20, 0);
	lv_obj_set_style_border_width(chat_input, 0, 0);
	lv_obj_set_style_text_color(chat_input, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(chat_input, &lv_font_montserrat_20, 0);
	lv_obj_set_style_text_color(chat_input, theme_color(THEME_SUBTEXT),
				    LV_PART_TEXTAREA_PLACEHOLDER);

	lv_obj_t *send = lv_button_create(bar);
	lv_obj_remove_style_all(send);
	lv_obj_set_size(send, 40, 40);
	/* Visual size stays 40x40, but the circular target sits right at the
	 * screen's right edge (worst spot for touch accuracy) — pad the actual
	 * hit area by 12px on every side without changing how it looks. */
	lv_obj_set_ext_click_area(send, 12);
	lv_obj_set_style_radius(send, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_opa(send, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(send, theme_color(THEME_ACCENT), 0);
	lv_obj_set_style_bg_grad_color(send, theme_color(THEME_ACCENT_2), 0);
	lv_obj_set_style_bg_grad_dir(send, LV_GRAD_DIR_VER, 0);
	lv_obj_add_event_cb(send, on_send, LV_EVENT_CLICKED, NULL);

	lv_obj_t *glyph = lv_label_create(send);
	lv_label_set_text(glyph, LV_SYMBOL_UPLOAD);
	lv_obj_set_style_text_color(glyph, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(glyph, &lv_font_montserrat_20, 0);
	lv_obj_center(glyph);

	gui_keyboard_attach(chat_input, LV_KEYBOARD_MODE_TEXT_LOWER);
	lv_obj_add_event_cb(chat_input, on_input_focus, LV_EVENT_FOCUSED, NULL);
	lv_obj_add_event_cb(chat_input, on_input_blur, LV_EVENT_DEFOCUSED, NULL);
	return bar;
}

void sms_chat_create(lv_obj_t *screen, void *arg)
{
	chat_idx = (int)(long)arg;
	const struct sms_conversation *c = sms_conversation_get(chat_idx);
	const char *peer = c ? c->peer : NULL;
	const char *title = peer ? peer : "Chat";
	app_header_create(screen, title, NULL);

	lv_obj_t *input_bar = build_input_bar(screen);
	gui_keyboard_set_lift(input_bar, -(int32_t)NAV_BAR_HEIGHT);

	int list_top    = APP_HEADER_HEIGHT;
	int list_bottom = NAV_BAR_HEIGHT + INPUT_H;
	chat_list = lv_obj_create(screen);
	lv_obj_remove_style_all(chat_list);
	lv_obj_set_size(chat_list, lv_pct(100), SCREEN_H - list_top - list_bottom);
	lv_obj_align(chat_list, LV_ALIGN_TOP_MID, 0, list_top);
	lv_obj_set_style_pad_hor(chat_list, 12, 0);
	lv_obj_set_style_pad_ver(chat_list, 8, 0);
	lv_obj_set_style_pad_row(chat_list, 10, 0);
	lv_obj_set_scroll_dir(chat_list, LV_DIR_VER);
	lv_obj_clear_flag(chat_list, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
	lv_obj_set_scrollbar_mode(chat_list, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_style_bg_color(chat_list, theme_color(THEME_SUBTEXT), LV_PART_SCROLLBAR);
	lv_obj_set_style_bg_opa(chat_list, LV_OPA_70, LV_PART_SCROLLBAR);
	lv_obj_set_style_width(chat_list, 4, LV_PART_SCROLLBAR);
	lv_obj_set_style_radius(chat_list, 2, LV_PART_SCROLLBAR);
	lv_obj_set_style_pad_right(chat_list, 2, LV_PART_SCROLLBAR);
	lv_obj_set_flex_flow(chat_list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(chat_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);

	lv_obj_add_event_cb(screen, on_screen_loaded, LV_EVENT_SCREEN_LOADED, NULL);
	lv_obj_add_event_cb(screen, on_screen_unloaded, LV_EVENT_SCREEN_UNLOAD_START, NULL);

	sms_mark_read(chat_idx);
	rebuild_thread();
}
