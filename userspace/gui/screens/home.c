/*
 * screens/home.c - Home: status bar + search + 4-col app grid + dock
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "home.h"
#include "../theme/theme.h"
#include "../core/log.h"
#include "../widgets/status_bar.h"
#include "../widgets/app_tile.h"
#include "../widgets/nav_bar.h"

#define STATUS_H   28
#define SEARCH_H   40
#define DOCK_H     76
#define DOCK_FLOAT 8	/* gap between the floating dock and the nav bar */

/* The scrollable app grid, kept so the demo sweep can reach it without
 * threading a handle back through the nav builder signature. */
static lv_obj_t *home_grid;

struct app_def {
	const char *symbol;
	const char *label;
	uint32_t    color;
};

static const struct app_def apps[] = {
	{ LV_SYMBOL_CALL,      "Phone",    0x7C3AED },
	{ LV_SYMBOL_ENVELOPE,  "Messages", 0x3B82F6 },
	{ LV_SYMBOL_LIST,      "Contacts", 0xEC4899 },
	{ LV_SYMBOL_GPS,       "Maps",     0xF59E0B },
	{ LV_SYMBOL_EDIT,      "Notes",    0xEAB308 },
	{ LV_SYMBOL_SETTINGS,  "Settings", 0x64748B },
	{ LV_SYMBOL_PLUS,      "Calc",     0x14B8A6 },
	{ LV_SYMBOL_IMAGE,     "Camera",   0xEF4444 },
	{ LV_SYMBOL_AUDIO,     "Music",    0xDB2777 },
	{ LV_SYMBOL_FILE,      "Files",    0x6366F1 },
	{ LV_SYMBOL_VIDEO,     "Video",    0x10B981 },
	{ LV_SYMBOL_BELL,      "Clock",    0xF97316 },
	{ LV_SYMBOL_WIFI,      "Wi-Fi",    0x06B6D4 },
	{ LV_SYMBOL_BLUETOOTH, "BT",       0x8B5CF6 },
	{ LV_SYMBOL_BATTERY_3, "Power",    0x84CC16 },
	{ LV_SYMBOL_CHARGE,    "Stats",    0xF43F5E },
	{ LV_SYMBOL_HOME,      "Smart",    0xA855F7 },
	{ LV_SYMBOL_DOWNLOAD,  "Updates",  0x0EA5E9 },
	{ LV_SYMBOL_SAVE,      "Backup",   0x65A30D },
	{ LV_SYMBOL_KEYBOARD,  "Input",    0xD97706 },
	{ LV_SYMBOL_TRASH,     "Cleaner",  0xE11D48 },
	{ LV_SYMBOL_REFRESH,   "Sync",     0x0891B2 },
	{ LV_SYMBOL_SHUFFLE,   "Random",   0xCA8A04 },
	{ LV_SYMBOL_DRIVE,     "Storage",  0x7E22CE },
};
#define APP_COUNT  (int)(sizeof(apps) / sizeof(apps[0]))

static const struct app_def dock_apps[4] = {
	{ LV_SYMBOL_CALL,     NULL, 0x22C55E },
	{ LV_SYMBOL_ENVELOPE, NULL, 0x3B82F6 },
	{ LV_SYMBOL_LIST,     NULL, 0xEC4899 },
	{ LV_SYMBOL_IMAGE,    NULL, 0x64748B },
};

static void build_search_bar(lv_obj_t *parent)
{
	/* A real one-line lv_textarea — focusable and tappable, with a
	 * proper placeholder, instead of a faux box with a centered label.
	 * Typing needs an input device + keyboard (LV_USE_KEYBOARD is off
	 * for now), but the widget tree is already correct. */
	lv_obj_t *search = lv_textarea_create(parent);
	lv_textarea_set_one_line(search, true);
	lv_textarea_set_placeholder_text(search, "Search");
	lv_obj_set_size(search, lv_pct(92), SEARCH_H);
	lv_obj_align(search, LV_ALIGN_TOP_MID, 0, STATUS_H + 12);

	lv_obj_set_style_bg_color(search, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(search, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(search, SEARCH_H / 2, 0);
	lv_obj_set_style_border_width(search, 0, 0);
	lv_obj_set_style_text_color(search, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(search, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_align(search, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(search, theme_color(THEME_SUBTEXT),
				    LV_PART_TEXTAREA_PLACEHOLDER);
}

static void build_dock(lv_obj_t *parent)
{
	/* Floating rounded tray: inset from the screen edges, lifted above
	 * the nav bar, on a lighter surface with a soft shadow so it reads
	 * as a panel hovering over the wallpaper (iOS-style dock). */
	lv_obj_t *dock = lv_obj_create(parent);
	lv_obj_remove_style_all(dock);
	lv_obj_set_size(dock, lv_pct(92), DOCK_H);
	lv_obj_align(dock, LV_ALIGN_BOTTOM_MID, 0, -(NAV_BAR_HEIGHT + DOCK_FLOAT));
	lv_obj_set_style_bg_color(dock, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(dock, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(dock, RADIUS_LG, 0);
	lv_obj_set_style_shadow_color(dock, lv_color_black(), 0);
	lv_obj_set_style_shadow_opa(dock, LV_OPA_30, 0);
	lv_obj_set_style_shadow_width(dock, 16, 0);
	lv_obj_set_style_shadow_offset_y(dock, 2, 0);
	lv_obj_clear_flag(dock, LV_OBJ_FLAG_SCROLLABLE);

	/* Flex row + SPACE_EVENLY spreads the four icons with equal gaps. */
	lv_obj_set_flex_flow(dock, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(dock, LV_FLEX_ALIGN_SPACE_EVENLY,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	for (int i = 0; i < 4; i++)
		app_tile_create(dock, dock_apps[i].symbol, dock_apps[i].label,
				dock_apps[i].color);
}

void home_create(lv_obj_t *parent, void *arg)
{
	(void)arg;
	gui_log("screen: home\n");
	lv_obj_set_style_bg_color(parent, theme_color(THEME_BG), 0);
	lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

	status_bar_create(parent);
	build_search_bar(parent);

	/* Scrollable 4-col grid between search bar and the floating dock.
	 * grid_bottom leaves an 8px gap above the dock, then the dock, its
	 * float gap, and the nav bar below. */
	int grid_top    = STATUS_H + 12 + SEARCH_H + 16;
	int grid_bottom = DOCK_H + DOCK_FLOAT + NAV_BAR_HEIGHT + 8;
	lv_obj_t *grid = lv_obj_create(parent);
	lv_obj_remove_style_all(grid);
	lv_obj_set_size(grid, lv_pct(100), 640 - grid_top - grid_bottom);
	lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, grid_top);
	lv_obj_set_style_pad_ver(grid, 8, 0);
	lv_obj_set_style_pad_row(grid, 32, 0);	/* breathing room between rows */
	lv_obj_set_scroll_dir(grid, LV_DIR_VER);
	/* AUTO: shown whenever the list overflows. MODE_ACTIVE would only
	 * draw while a touch indev is actively scrolling — and there is no
	 * touch yet (HDMI output), nor does code-driven scrolling count, so
	 * ACTIVE would never appear. Switch to ACTIVE once touch lands. */
	lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_style_bg_color(grid, theme_color(THEME_SUBTEXT),
				  LV_PART_SCROLLBAR);
	lv_obj_set_style_bg_opa(grid, LV_OPA_70, LV_PART_SCROLLBAR);
	lv_obj_set_style_width(grid, 4, LV_PART_SCROLLBAR);
	lv_obj_set_style_radius(grid, 2, LV_PART_SCROLLBAR);
	lv_obj_set_style_pad_right(grid, 2, LV_PART_SCROLLBAR);

	/* Flex wrap + SPACE_EVENLY: LVGL hands out equal horizontal gaps,
	 * including the left and right edges, so columns are symmetric by
	 * construction — no manual pad_hor math that can drift one side.
	 * 4 fixed-width tiles per row, matching the dock. */
	lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	for (int i = 0; i < APP_COUNT; i++)
		app_tile_create(grid, apps[i].symbol, apps[i].label,
				apps[i].color);

	build_dock(parent);

	/* The system nav bar is owned by the navigation stack and drawn on
	 * the display top layer, so the home screen does not create its own.
	 * grid_bottom above still reserves NAV_BAR_HEIGHT for it. */
	home_grid = grid;
}

static void scroll_exec_cb(void *grid, int32_t v)
{
	lv_obj_scroll_to_y((lv_obj_t *)grid, v, LV_ANIM_OFF);
}

void home_scroll_to_end(int to_bottom, int duration_ms)
{
	lv_obj_t *grid = home_grid;
	if (!grid)
		return;

	int cur    = lv_obj_get_scroll_top(grid);	/* current position */
	int target = to_bottom ? cur + lv_obj_get_scroll_bottom(grid) : 0;

	lv_anim_t a;
	lv_anim_init(&a);
	lv_anim_set_var(&a, grid);
	lv_anim_set_values(&a, cur, target);
	lv_anim_set_time(&a, duration_ms);
	lv_anim_set_exec_cb(&a, scroll_exec_cb);
	lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
	lv_anim_start(&a);
}
