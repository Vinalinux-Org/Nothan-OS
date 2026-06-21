#ifndef __GUI_APP_HEADER_H
#define __GUI_APP_HEADER_H

#include "lvgl/lvgl.h"

#define APP_HEADER_HEIGHT  48

/*
 * app_header_create() - Top bar for an app screen: a back chevron on the
 * left (wired to nav_pop), a centered @title, and an optional right-side
 * action button showing @right_symbol (pass NULL for none).
 *
 * Returns the right-action button so the caller can attach a CLICKED
 * handler, or NULL when @right_symbol is NULL.
 */
lv_obj_t *app_header_create(lv_obj_t *parent, const char *title,
			    const char *right_symbol);

#endif
