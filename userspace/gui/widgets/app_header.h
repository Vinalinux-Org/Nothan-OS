#ifndef __GUI_APP_HEADER_H
#define __GUI_APP_HEADER_H

#include "lvgl/lvgl.h"

#define APP_HEADER_HEIGHT  60

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

/*
 * app_header_back() - the same bar with a back chevron on the left.
 *
 * A separate function rather than a parameter on the one above, because the
 * screens that use that one — Phone, Messages, Contacts — navigate with the
 * system nav bar and have never had a back control of their own.  Adding one
 * to them would be a visible change to three apps in order to serve a fourth.
 *
 * The Chat app needs it because it hides the system nav bar: an app with its
 * own tab bar along the bottom does not need a second row of buttons under it,
 * and the moment that bar is gone, every screen has to carry its own way out.
 */
lv_obj_t *app_header_back(lv_obj_t *parent, const char *title,
			  const char *right_symbol);

#endif
