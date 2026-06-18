/*
 * screens/home.c - Home screen with status bar and 3x3 app launcher
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "home.h"
#include "../widgets/status_bar.h"
#include "../widgets/app_tile.h"

struct app_def {
	const char *symbol;
	const char *label;
	uint32_t    color;
};

/* 9 apps in row-major order — Messages is index 1 so the demo flow
 * highlights it after the home screen settles. */
static const struct app_def apps[9] = {
	{ LV_SYMBOL_CALL,     "Phone",    0x2ECC71 },
	{ LV_SYMBOL_ENVELOPE, "Messages", 0x3498DB },
	{ LV_SYMBOL_USB,      "Contacts", 0x9B59B6 },
	{ LV_SYMBOL_GPS,      "Maps",     0xE67E22 },
	{ LV_SYMBOL_EDIT,     "Notes",    0xF1C40F },
	{ LV_SYMBOL_SETTINGS, "Settings", 0x7F8C8D },
	{ LV_SYMBOL_LIST,     "Calc",     0x16A085 },
	{ LV_SYMBOL_IMAGE,    "Camera",   0xE74C3C },
	{ LV_SYMBOL_AUDIO,    "Music",    0xE91E63 },
};

lv_obj_t *home_create(lv_obj_t *parent)
{
	lv_obj_set_style_bg_color(parent, lv_color_hex(0xF5F5F5), 0);
	lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

	status_bar_create(parent);

	/* Grid container under the status bar */
	lv_obj_t *grid = lv_obj_create(parent);
	lv_obj_set_size(grid, lv_pct(100), 640 - 32);
	lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 32);
	lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(grid, 0, 0);
	lv_obj_set_style_pad_all(grid, 16, 0);
	lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

	static lv_coord_t col_dsc[] = {
		LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST,
	};
	static lv_coord_t row_dsc[] = {
		LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST,
	};
	lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

	lv_obj_t *messages_tile = NULL;
	for (int i = 0; i < 9; i++) {
		lv_obj_t *tile = app_tile_create(grid, apps[i].symbol,
						 apps[i].label, apps[i].color);
		lv_obj_set_grid_cell(tile,
				     LV_GRID_ALIGN_CENTER, i % 3, 1,
				     LV_GRID_ALIGN_CENTER, i / 3, 1);
		if (i == 1)
			messages_tile = tile;
	}

	return messages_tile;
}
