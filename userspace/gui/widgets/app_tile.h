#ifndef __GUI_APP_TILE_H
#define __GUI_APP_TILE_H

#include "lvgl/lvgl.h"

/*
 * app_tile_create() - rounded tile with a colored badge containing a
 * symbol and a label underneath. Used as one cell in the home grid.
 */
lv_obj_t *app_tile_create(lv_obj_t *parent, const char *symbol,
			  const char *label, uint32_t color);

#endif
