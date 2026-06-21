/*
 * screens/dialer.c - Phone: dial pad
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "dialer.h"
#include "active_call.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../services/telephony.h"

#define KEY_SZ  66	/* circular dial-key diameter */

/* Even 3-column × 4-row grid (iPhone-style dial pad). */
static int32_t kp_col_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
				LV_GRID_TEMPLATE_LAST };
static int32_t kp_row_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
				LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

/* Pre-filled sample so the dialer reads as populated without an input
 * device; the keypad still edits it for when touch/keypad lands. */
static char        num_buf[24] = "0912 345 678";
static lv_obj_t   *num_label;
static struct call_info dialer_call;

static const struct { const char *digit; const char *letters; } keys[] = {
	{ "1", "" },    { "2", "ABC" },  { "3", "DEF" },
	{ "4", "GHI" }, { "5", "JKL" },  { "6", "MNO" },
	{ "7", "PQRS" },{ "8", "TUV" },  { "9", "WXYZ" },
	{ "*", "" },    { "0", "+" },    { "#", "" },
};

static int num_len(void)
{
	int n = 0;
	while (num_buf[n])
		n++;
	return n;
}

static void on_key(lv_event_t *e)
{
	const char *d = lv_event_get_user_data(e);
	int n = num_len();
	if (n < (int)sizeof(num_buf) - 1) {
		num_buf[n]     = d[0];
		num_buf[n + 1] = '\0';
		lv_label_set_text(num_label, num_buf);
	}
	gui_logf("event: dial key %s\n", d);
}

static void on_backspace(lv_event_t *e)
{
	(void)e;
	int n = num_len();
	if (n > 0) {
		num_buf[n - 1] = '\0';
		lv_label_set_text(num_label, num_buf);
	}
	gui_log("event: dial backspace\n");
}

static void on_call(lv_event_t *e)
{
	(void)e;
	gui_logf("event: dial call %s\n", num_buf);
	telephony_dial(num_buf);
	dialer_call.name   = NULL;
	dialer_call.number = num_buf;
	nav_push(active_call_create, &dialer_call);
}

static void keypad_btn(lv_obj_t *grid, const char *digit, const char *letters,
		       int row, int col)
{
	/* Round key with a soft surface fill (iPhone dial-key on dark). */
	lv_obj_t *key = lv_button_create(grid);
	lv_obj_remove_style_all(key);
	lv_obj_set_size(key, KEY_SZ, KEY_SZ);
	lv_obj_set_style_radius(key, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(key, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(key, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(key, lv_color_lighten(theme_color(THEME_SURFACE), 40),
				  LV_STATE_PRESSED);
	lv_obj_set_flex_flow(key, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(key, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(key, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(key, on_key, LV_EVENT_CLICKED, (void *)digit);
	lv_obj_set_grid_cell(key, LV_GRID_ALIGN_CENTER, col, 1,
			     LV_GRID_ALIGN_CENTER, row, 1);

	lv_obj_t *d = lv_label_create(key);
	lv_label_set_text(d, digit);
	lv_obj_set_style_text_color(d, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(d, &lv_font_montserrat_24, 0);

	if (letters && letters[0]) {
		lv_obj_t *l = lv_label_create(key);
		lv_label_set_text(l, letters);
		lv_obj_set_style_text_color(l, theme_color(THEME_SUBTEXT), 0);
		lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
	}
}

static lv_obj_t *round_button(lv_obj_t *parent, const char *symbol, uint32_t color)
{
	lv_obj_t *btn = lv_button_create(parent);
	lv_obj_remove_style_all(btn);
	lv_obj_set_size(btn, 64, 64);
	lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(btn, theme_color(color), 0);

	lv_obj_t *glyph = lv_label_create(btn);
	lv_label_set_text(glyph, symbol);
	lv_obj_set_style_text_color(glyph, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(glyph, &lv_font_montserrat_24, 0);
	lv_obj_center(glyph);
	return btn;
}

void dialer_create(lv_obj_t *screen, void *arg)
{
	(void)arg;
	gui_log("screen: dialer\n");

	app_header_create(screen, "Phone", NULL);

	num_label = lv_label_create(screen);
	lv_label_set_text(num_label, num_buf);
	lv_obj_set_style_text_color(num_label, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(num_label, &lv_font_montserrat_24, 0);
	lv_obj_align(num_label, LV_ALIGN_TOP_MID, 0, APP_HEADER_HEIGHT + 20);

	/* Even 4-row × 3-column dial grid. */
	lv_obj_t *grid = lv_obj_create(screen);
	lv_obj_remove_style_all(grid);
	lv_obj_set_size(grid, lv_pct(84), 4 * KEY_SZ + 36);
	lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, APP_HEADER_HEIGHT + 56);
	lv_obj_set_grid_dsc_array(grid, kp_col_dsc, kp_row_dsc);
	lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

	for (int i = 0; i < (int)(sizeof(keys) / sizeof(keys[0])); i++)
		keypad_btn(grid, keys[i].digit, keys[i].letters, i / 3, i % 3);

	/* Call button centered above the nav bar, backspace to its right. */
	lv_obj_t *call = round_button(screen, LV_SYMBOL_CALL, THEME_SUCCESS);
	lv_obj_align(call, LV_ALIGN_BOTTOM_MID, 0, -(NAV_BAR_HEIGHT + 20));
	lv_obj_add_event_cb(call, on_call, LV_EVENT_CLICKED, NULL);

	lv_obj_t *back = lv_button_create(screen);
	lv_obj_remove_style_all(back);
	lv_obj_set_size(back, 48, 48);
	lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 96, -(NAV_BAR_HEIGHT + 28));
	lv_obj_set_style_bg_color(back, theme_color(THEME_TEXT), LV_STATE_PRESSED);
	lv_obj_set_style_bg_opa(back, LV_OPA_10, LV_STATE_PRESSED);
	lv_obj_set_style_radius(back, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(back, on_backspace, LV_EVENT_CLICKED, NULL);

	lv_obj_t *bk = lv_label_create(back);
	lv_label_set_text(bk, LV_SYMBOL_BACKSPACE);
	lv_obj_set_style_text_color(bk, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(bk, &lv_font_montserrat_20, 0);
	lv_obj_center(bk);
}
