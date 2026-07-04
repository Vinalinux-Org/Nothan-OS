/*
 * screens/home.c - Home: status bar + search + 4-col app grid + dock
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "home.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../widgets/status_bar.h"
#include "../widgets/app_tile.h"
#include "../widgets/nav_bar.h"
#include "call_log.h"
#include "sms_list.h"
#include "contacts_list.h"

#define STATUS_H   STATUS_BAR_HEIGHT
#define SEARCH_H   40
#define DOCK_H     76
#define DOCK_FLOAT 18	/* gap between the floating dock and the nav bar */


struct app_def {
	const char    *symbol;
	const char    *label;
	uint32_t       color;
	nav_builder_fn builder;   /* NULL = stub app (no screen yet) */
};

static const struct app_def apps[] = {
	{ LV_SYMBOL_CALL,      "Phone",    0x7C3AED, call_log_create },
	{ LV_SYMBOL_ENVELOPE,  "Messages", 0x3B82F6, sms_list_create },
	{ LV_SYMBOL_LIST,      "Contacts", 0xEC4899, contacts_list_create },
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

static lv_obj_t *s_grid;

static void on_screen_unloaded(lv_event_t *e)
{
	(void)e;
	s_grid = NULL;
}

void home_scroll_to_end(int to_end, int animated)
{
	if (!s_grid) return;
	lv_anim_enable_t anim = animated ? LV_ANIM_ON : LV_ANIM_OFF;
	if (to_end) {
		uint32_t n = lv_obj_get_child_count(s_grid);
		if (n) lv_obj_scroll_to_view(lv_obj_get_child(s_grid, (int32_t)(n - 1)), anim);
	} else {
		lv_obj_scroll_to_y(s_grid, 0, anim);
	}
}

static const struct app_def dock_apps[4] = {
	{ LV_SYMBOL_CALL,     NULL, 0x22C55E, call_log_create },
	{ LV_SYMBOL_ENVELOPE, NULL, 0x3B82F6, sms_list_create },
	{ LV_SYMBOL_LIST,     NULL, 0xEC4899, contacts_list_create },
	{ LV_SYMBOL_IMAGE,    NULL, 0x64748B },
};

/* Generic tile click handler — user_data is a pointer to the app_def entry.
 * Dock tiles have label=NULL so we look up the name from apps[] by builder. */
static void on_app_tile(lv_event_t *e)
{
	const struct app_def *app = (const struct app_def *)lv_event_get_user_data(e);
	if (!app || !app->builder) return;
	const char *name = app->label;
	if (!name) {
		for (int i = 0; i < APP_COUNT; i++)
			if (apps[i].builder == app->builder) { name = apps[i].label; break; }
	}
	gui_logf("event: open app %s\n", name ? name : "?");
	nav_push(app->builder, NULL);
}

static void build_search_bar(lv_obj_t *parent)
{
	/* Faux search field: no input device yet, so a styled pill + centered
	 * "Search" label instead of a real lv_textarea. A textarea here always
	 * drew its cursor (a left-border line) that the default theme restyles
	 * regardless of our overrides, and could show a scrollbar — none of which
	 * is wanted with no keyboard. Swap to lv_textarea when touch + KB land. */
	lv_obj_t *search = lv_obj_create(parent);
	lv_obj_remove_style_all(search);
	lv_obj_set_size(search, lv_pct(92), SEARCH_H);
	lv_obj_align(search, LV_ALIGN_TOP_MID, 0, STATUS_H + 12);
	lv_obj_set_style_bg_color(search, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(search, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(search, SEARCH_H / 2, 0);
	lv_obj_clear_flag(search, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *ph = lv_label_create(search);
	lv_label_set_text(ph, "Search");
	lv_obj_set_style_text_color(ph, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(ph, &lv_font_montserrat_14, 0);
	lv_obj_center(ph);
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
	/* Dock shadow OFF — same LVGL SW shadow-blur overrun as the app tiles
	 * (see widgets/app_tile.c). The lighter surface alone separates the
	 * dock from the wallpaper well enough. */
	lv_obj_clear_flag(dock, LV_OBJ_FLAG_SCROLLABLE);

	/* Same 4-column grid as the app grid (and same 92% width), so each dock
	 * icon sits exactly under its column above. One full-height row centers
	 * the icons vertically in the tray. */
	static const int32_t dock_col[] = {
		LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	static const int32_t dock_row[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
	lv_obj_set_grid_dsc_array(dock, dock_col, dock_row);

	for (int i = 0; i < 4; i++) {
		lv_obj_t *t = app_tile_create(dock, dock_apps[i].symbol,
					      dock_apps[i].label,
					      dock_apps[i].color);
		lv_obj_set_grid_cell(t, LV_GRID_ALIGN_CENTER, i, 1,
				     LV_GRID_ALIGN_CENTER, 0, 1);
		if (dock_apps[i].builder) {
			lv_obj_t *badge = lv_obj_get_child(t, 0);
			lv_obj_add_event_cb(badge, on_app_tile, LV_EVENT_CLICKED,
					    (void *)&dock_apps[i]);
		}
	}
}

void home_create(lv_obj_t *parent, void *arg)
{
	(void)arg;
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
	/* 92% wide, same as the dock, so both share identical 4-column geometry
	 * and the dock icons line up under the grid columns. */
	lv_obj_set_size(grid, lv_pct(92), SCREEN_H - grid_top - grid_bottom);
	lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, grid_top);
	lv_obj_set_style_pad_ver(grid, 8, 0);
	lv_obj_set_scroll_dir(grid, LV_DIR_VER);
	/* No elastic over-scroll / momentum fling: the over-scroll path trips a
	 * NULL deref in this LVGL build when dragged past the end. Plain clamped
	 * scrolling is fine for a launcher grid. */
	lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
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

	/* Fixed 4-column grid: each column is an equal fraction of the width,
	 * so tiles never reflow into extra columns when the screen resolution
	 * changes (the flex-wrap version packed 6 columns at 480px). Rows are
	 * CONTENT-sized; APP_COUNT/4 = 6 rows for the current app set. */
	static const int32_t col_dsc[] = {
		LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	static const int32_t row_dsc[] = {
		LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
		LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	};
	lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
	lv_obj_set_style_pad_row(grid, 18, 0);	/* breathing room between rows */

	for (int i = 0; i < APP_COUNT; i++) {
		lv_obj_t *t = app_tile_create(grid, apps[i].symbol,
					      apps[i].label, apps[i].color);
		lv_obj_set_grid_cell(t, LV_GRID_ALIGN_CENTER, i % 4, 1,
				     LV_GRID_ALIGN_START, i / 4, 1);
	}

	s_grid = grid;
	lv_obj_add_event_cb(parent, on_screen_unloaded,
			    LV_EVENT_SCREEN_UNLOAD_START, NULL);

	build_dock(parent);
}
