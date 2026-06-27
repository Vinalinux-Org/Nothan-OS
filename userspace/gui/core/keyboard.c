/*
 * core/keyboard.c - Singleton on-screen iOS-style keyboard
 *
 * Sits on lv_layer_top() for the app lifetime. Shown/hidden automatically
 * when any registered textarea is focused. Uses custom button maps to
 * remove the default ugly 1#/ABC mode-switch buttons.
 *
 * gui_keyboard_set_lift(obj, y_ofs) — optional: registers a "companion"
 * widget that floats above the keyboard when it opens and drops back to
 * y_ofs (BOTTOM_MID-relative) when the keyboard closes.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "keyboard.h"
#include "log.h"
#include "../theme/theme.h"
#include "../port/lv_port_indev.h"

#define KB_HEIGHT  GUI_KEYBOARD_HEIGHT

/*
 * iOS-style maps. LVGL 9 has no LV_SYMBOL_SHIFT — mode switch is done via
 * "ABC" (lc → uc) and "abc" (uc → lc) literal strings, handled by the
 * default keyboard event callback.
 */
static const char *const kb_map_lc[] = {
	"1","2","3","4","5","6","7","8","9","0","\n",
	"q","w","e","r","t","y","u","i","o","p","\n",
	"a","s","d","f","g","h","j","k","l","\n",
	"ABC","z","x","c","v","b","n","m",".",",",LV_SYMBOL_BACKSPACE,"\n",
	LV_SYMBOL_KEYBOARD," ",LV_SYMBOL_OK,""
};
static const lv_btnmatrix_ctrl_t kb_ctrl_lc[] = {
	/* number row: 10 × 1 */
	1,1,1,1,1,1,1,1,1,1,
	/* QWERTY:    10 × 1 */
	1,1,1,1,1,1,1,1,1,1,
	/* ASDF:       9 × 1  (auto-stretch fills row width) */
	1,1,1,1,1,1,1,1,1,
	/* ZXCV: shift(2) + 9 keys + backspace(2) */
	2,1,1,1,1,1,1,1,1,1,2,
	/* bottom: close-kb(2) + space(6) + ok(2) */
	2,6,2
};

static const char *const kb_map_uc[] = {
	"1","2","3","4","5","6","7","8","9","0","\n",
	"Q","W","E","R","T","Y","U","I","O","P","\n",
	"A","S","D","F","G","H","J","K","L","\n",
	"abc","Z","X","C","V","B","N","M",".",",",LV_SYMBOL_BACKSPACE,"\n",
	LV_SYMBOL_KEYBOARD," ",LV_SYMBOL_OK,""
};
/* same ctrl layout as lower-case */

static const char *const kb_map_num[] = {
	"1","2","3","\n",
	"4","5","6","\n",
	"7","8","9","\n",
	LV_SYMBOL_BACKSPACE,"0","+","\n",
	LV_SYMBOL_OK,""
};
static const lv_btnmatrix_ctrl_t kb_ctrl_num[] = {
	1,1,1, 1,1,1, 1,1,1, 1,1,1, 3
};

static lv_obj_t *kb;
static lv_obj_t *lift_obj;
static int32_t   lift_y_closed;

void gui_keyboard_set_lift(lv_obj_t *obj, int32_t y_ofs_closed)
{
	lift_obj      = obj;
	lift_y_closed = y_ofs_closed;
}

static void kb_done_cb(lv_event_t *e)
{
	(void)e;
	/* Restore companion bar before hiding — works for both READY and CANCEL. */
	if (lift_obj)
		lv_obj_align(lift_obj, LV_ALIGN_BOTTOM_MID, 0, lift_y_closed);
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

static void ta_focused_cb(lv_event_t *e)
{
	lv_keyboard_mode_t mode =
		(lv_keyboard_mode_t)(intptr_t)lv_event_get_user_data(e);
	lv_obj_t *ta = lv_event_get_target(e);
	lv_keyboard_set_mode(kb, mode);
	lv_keyboard_set_textarea(kb, ta);
	lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
	if (lift_obj)
		lv_obj_align(lift_obj, LV_ALIGN_BOTTOM_MID, 0, -KB_HEIGHT);
}

static void ta_gone_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t       *ta   = lv_event_get_target(e);

	/* DEFOCUSED: keyboard dismissed by losing focus — restore bar. */
	if (code == LV_EVENT_DEFOCUSED && lift_obj)
		lv_obj_align(lift_obj, LV_ALIGN_BOTTOM_MID, 0, lift_y_closed);
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
	/* DELETE: the textarea's screen is tearing down. Drop every dangling
	 * reference to it — the keyboard keeps a textarea pointer internally and
	 * would use-after-free it on the next focus/keypress (set_textarea even
	 * touches the OLD textarea), and lift_obj may point into this screen. */
	if (code == LV_EVENT_DELETE) {
		lift_obj = NULL;
		if (kb && lv_keyboard_get_textarea(kb) == ta)
			lv_keyboard_set_textarea(kb, NULL);
	}
}

void gui_keyboard_init(void)
{
	kb = lv_keyboard_create(lv_layer_top());
	lv_obj_set_size(kb, SCREEN_W, KB_HEIGHT);
	lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

	lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, kb_map_lc, kb_ctrl_lc);
	lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_UPPER, kb_map_uc, kb_ctrl_lc);
	lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_NUMBER,     kb_map_num, kb_ctrl_num);

	lv_obj_add_event_cb(kb, kb_done_cb, LV_EVENT_READY,  NULL);
	lv_obj_add_event_cb(kb, kb_done_cb, LV_EVENT_CANCEL, NULL);

	/* Background */
	lv_obj_set_style_bg_color(kb, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(kb, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(kb, 0, 0);
	lv_obj_set_style_radius(kb, 0, 0);
	lv_obj_set_style_pad_all(kb, 6, 0);
	lv_obj_set_style_pad_row(kb, 6, 0);
	lv_obj_set_style_pad_column(kb, 6, 0);

	/* Keys: dark pill style */
	lv_obj_set_style_bg_color(kb, lv_color_hex(0x1E293B), LV_PART_ITEMS);
	lv_obj_set_style_bg_opa(kb, LV_OPA_COVER, LV_PART_ITEMS);
	lv_obj_set_style_border_width(kb, 0, LV_PART_ITEMS);
	lv_obj_set_style_shadow_width(kb, 0, LV_PART_ITEMS);
	lv_obj_set_style_radius(kb, RADIUS_SM, LV_PART_ITEMS);
	lv_obj_set_style_text_color(kb, theme_color(THEME_TEXT), LV_PART_ITEMS);
	lv_obj_set_style_text_font(kb, &lv_font_montserrat_16, LV_PART_ITEMS);
	/* Pressed key */
	lv_obj_set_style_bg_color(kb, theme_color(THEME_ACCENT),
				   LV_PART_ITEMS | LV_STATE_PRESSED);
	/* Shift active (CHECKED = uppercase mode active) */
	lv_obj_set_style_bg_color(kb, theme_color(THEME_ACCENT),
				   LV_PART_ITEMS | LV_STATE_CHECKED);

	gui_log("keyboard: ready\n");
}

void gui_keyboard_attach(lv_obj_t *ta, lv_keyboard_mode_t mode)
{
	theme_apply_cursor(ta);
	/* FOCUSED: normal focus gain. CLICKED: re-open after ✓ dismiss while
	 * still focused (LVGL won't re-fire FOCUSED in that case). */
	lv_obj_add_event_cb(ta, ta_focused_cb, LV_EVENT_FOCUSED,
			    (void *)(intptr_t)mode);
	lv_obj_add_event_cb(ta, ta_focused_cb, LV_EVENT_CLICKED,
			    (void *)(intptr_t)mode);
	lv_obj_add_event_cb(ta, ta_gone_cb, LV_EVENT_DEFOCUSED, NULL);
	lv_obj_add_event_cb(ta, ta_gone_cb, LV_EVENT_DELETE,    NULL);
	sim_register_ta(ta);
}
