/*
 * screens/chat_thread.c - Chat: one conversation, and the call button
 *
 * The bubble layout follows sms_chat.c closely, and where it does the reason
 * is that they are the same object to the reader — a thread that looked
 * different in two apps would be two designs, not one product.  What this
 * screen has that the SMS one cannot is the video call action in the header:
 * a call here is a peer at an address, which is a thing this box can start on
 * its own, whereas a video call over a GSM modem is not a thing at all.
 *
 * THE CALL BUTTON DOES NOT CALL YET.  It logs and returns.  The video path it
 * will drive already works — 30 fps both ways, measured — but it draws from
 * inside the kernel straight onto the panel, and LVGL draws onto the same
 * panel through /dev/fb0 from out here.  Two writers, one framebuffer.  That
 * has to be settled before the button is wired, and settling it is a design
 * decision rather than a line of code, so the button is honest about being a
 * button and nothing else until then.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "chat_thread.h"
#include "video_call.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../core/keyboard.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../services/chat.h"
#include "../services/call.h"

#define INPUT_H     68
#define BUBBLE_MAXW 240

static int       thread_idx;
static lv_obj_t *thread_input;
static lv_obj_t *thread_list;


/*
 * The Chat app hides the system nav bar and carries its own back chevron, so
 * every screen in it keeps the bar gone while it is up and puts it back when
 * it goes.  Each screen owning both halves is what makes the sequence work
 * from anywhere: a call screen opened from Home restores the bar on the way
 * out, and one opened from a thread has it hidden again by the thread's own
 * load event a frame later.
 */
static void on_chrome_show(lv_event_t *e)
{
	(void)e;
	nav_show_chrome(false);
}

static void on_chrome_restore(lv_event_t *e)
{
	(void)e;
	nav_show_chrome(true);
}

static void add_bubble(lv_obj_t *list, const struct chat_message *m)
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

static void rebuild_thread(void)
{
	const struct chat_peer *p = chat_peer_get(thread_idx);

	if (!thread_list || !p)
		return;

	lv_obj_clean(thread_list);
	for (int i = 0; i < p->count; i++)
		add_bubble(thread_list, &p->msg[i]);

	lv_obj_update_layout(thread_list);

	/*
	 * Pin to the newest bubble only when the thread actually overflows.
	 * For a short thread scroll_bottom is negative, and scrolling by it
	 * would push the top-aligned bubbles down into the middle of an empty
	 * screen — the same trap sms_chat.c documents, and worth repeating
	 * here because the symptom looks like a layout bug rather than a
	 * scroll one.
	 */
	int32_t sb = lv_obj_get_scroll_bottom(thread_list);
	if (sb > 0)
		lv_obj_scroll_by(thread_list, 0, -sb, LV_ANIM_OFF);
}

static void on_send(lv_event_t *e)
{
	(void)e;
	const char *text = lv_textarea_get_text(thread_input);

	if (!text || !text[0])
		return;

	if (chat_send(thread_idx, text) == 0) {
		lv_textarea_set_text(thread_input, "");
		rebuild_thread();
	}
}

/*
 * Open the in-call screen.
 *
 * The screen is real and the call is not: there is no signalling yet, so
 * nothing tells the far end, and no pixels move.  Pushing it anyway is worth
 * more than a button that logs — the layout the video will land in exists and
 * can be looked at, and the two things still missing (an invite on the wire,
 * an owner for the framebuffer) are named on screen rather than only in a
 * comment.
 */
static void on_video_call(lv_event_t *e)
{
	(void)e;
	const struct chat_peer *p = chat_peer_get(thread_idx);

	(void)p;
	if (call_dial(thread_idx) != 0) {
		gui_logf("event: cannot call, already in one\n");
		return;
	}
	nav_push(video_call_create, (void *)(long)thread_idx);
}

static void on_input_focus(lv_event_t *e)
{
	(void)e;
	lv_obj_set_height(thread_list,
			  SCREEN_H - APP_HEADER_HEIGHT - GUI_KEYBOARD_HEIGHT - INPUT_H);
	lv_obj_update_layout(thread_list);

	int32_t sb = lv_obj_get_scroll_bottom(thread_list);
	if (sb > 0)
		lv_obj_scroll_by(thread_list, 0, -sb, LV_ANIM_OFF);
}

static void on_input_blur(lv_event_t *e)
{
	(void)e;
	lv_obj_set_height(thread_list,
			  SCREEN_H - APP_HEADER_HEIGHT - INPUT_H);
}

static lv_obj_t *build_input_bar(lv_obj_t *parent)
{
	lv_obj_t *bar = lv_obj_create(parent);
	lv_obj_remove_style_all(bar);
	lv_obj_set_size(bar, lv_pct(100), INPUT_H);
	lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_set_style_bg_color(bar, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_hor(bar, 10, 0);
	lv_obj_set_style_pad_column(bar, 8, 0);
	lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

	thread_input = lv_textarea_create(bar);
	lv_textarea_set_one_line(thread_input, true);
	lv_textarea_set_placeholder_text(thread_input, "Message");
	lv_obj_set_height(thread_input, 52);
	lv_obj_set_flex_grow(thread_input, 1);
	lv_obj_set_style_anim_duration(thread_input, 0, LV_PART_CURSOR);
	lv_obj_set_style_bg_color(thread_input, theme_color(THEME_BG), 0);
	lv_obj_set_style_bg_opa(thread_input, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(thread_input, 20, 0);
	lv_obj_set_style_border_width(thread_input, 0, 0);
	lv_obj_set_style_text_color(thread_input, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(thread_input, &lv_font_montserrat_20, 0);
	lv_obj_set_style_text_color(thread_input, theme_color(THEME_SUBTEXT),
				    LV_PART_TEXTAREA_PLACEHOLDER);

	lv_obj_t *send = lv_button_create(bar);
	lv_obj_remove_style_all(send);
	lv_obj_set_size(send, 40, 40);
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

	gui_keyboard_attach(thread_input, LV_KEYBOARD_MODE_TEXT_LOWER);
	lv_obj_add_event_cb(thread_input, on_input_focus, LV_EVENT_FOCUSED, NULL);
	lv_obj_add_event_cb(thread_input, on_input_blur, LV_EVENT_DEFOCUSED, NULL);
	return bar;
}

/*
 * Re-read the store.  Fired on SCREEN_LOADED, which arrives both when this
 * screen is opened and when chat.c sends it by hand after a message lands, so
 * a thread left open updates itself without the transport knowing a screen
 * exists.
 */
static void on_screen_loaded(lv_event_t *e)
{
	(void)e;
	chat_mark_read(thread_idx);
	rebuild_thread();
}

/*
 * Forget the objects before LVGL frees them.  @thread_list outlives this
 * screen as a pointer but not as an object, and rebuild_thread() would then
 * clean a dangling one — a use-after-free reached from the network path,
 * which is the worst place to find it.
 */
static void on_screen_unloaded(lv_event_t *e)
{
	(void)e;
	thread_list  = NULL;
	thread_input = NULL;
}

void chat_thread_create(lv_obj_t *screen, void *arg)
{
	thread_idx = (int)(long)arg;

	const struct chat_peer *p = chat_peer_get(thread_idx);
	lv_obj_t *call = app_header_back(screen, p ? p->name : "Chat",
					   LV_SYMBOL_VIDEO);
	if (call) {
		lv_obj_add_event_cb(call, on_video_call, LV_EVENT_CLICKED, NULL);

		/*
		 * Filled, round, accent — a call is the one thing on this
		 * screen that reaches another machine, and it read as
		 * decoration next to the title while it was a bare glyph.
		 * Restyled here rather than in app_header.c because that
		 * button is shared with Phone and Messages, whose right-hand
		 * actions are ordinary and should stay quiet.
		 */
		lv_obj_set_size(call, 44, 44);
		lv_obj_align(call, LV_ALIGN_RIGHT_MID, -8, 0);
		lv_obj_set_style_radius(call, LV_RADIUS_CIRCLE, 0);
		lv_obj_set_style_bg_opa(call, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(call, theme_color(THEME_ACCENT), 0);
		lv_obj_set_style_bg_grad_color(call, theme_color(THEME_ACCENT_2), 0);
		lv_obj_set_style_bg_grad_dir(call, LV_GRAD_DIR_VER, 0);
	}

	lv_obj_t *input_bar = build_input_bar(screen);
	gui_keyboard_set_lift(input_bar, 0);

	int list_top    = APP_HEADER_HEIGHT;
	int list_bottom = INPUT_H;

	thread_list = lv_obj_create(screen);
	lv_obj_remove_style_all(thread_list);
	lv_obj_set_size(thread_list, lv_pct(100), SCREEN_H - list_top - list_bottom);
	lv_obj_align(thread_list, LV_ALIGN_TOP_MID, 0, list_top);
	lv_obj_set_style_pad_hor(thread_list, 12, 0);
	lv_obj_set_style_pad_ver(thread_list, 8, 0);
	lv_obj_set_style_pad_row(thread_list, 10, 0);
	lv_obj_set_scroll_dir(thread_list, LV_DIR_VER);
	lv_obj_clear_flag(thread_list,
			  LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
	lv_obj_set_scrollbar_mode(thread_list, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_style_bg_color(thread_list, theme_color(THEME_SUBTEXT),
				  LV_PART_SCROLLBAR);
	lv_obj_set_style_bg_opa(thread_list, LV_OPA_70, LV_PART_SCROLLBAR);
	lv_obj_set_style_width(thread_list, 4, LV_PART_SCROLLBAR);
	lv_obj_set_style_radius(thread_list, 2, LV_PART_SCROLLBAR);
	lv_obj_set_flex_flow(thread_list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(thread_list, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

	lv_obj_add_event_cb(screen, on_chrome_show, LV_EVENT_SCREEN_LOADED, NULL);
	lv_obj_add_event_cb(screen, on_chrome_restore, LV_EVENT_DELETE, NULL);
	lv_obj_add_event_cb(screen, on_screen_loaded, LV_EVENT_SCREEN_LOADED, NULL);
	lv_obj_add_event_cb(screen, on_screen_unloaded,
			    LV_EVENT_SCREEN_UNLOAD_START, NULL);

	chat_mark_read(thread_idx);
	rebuild_thread();
}
