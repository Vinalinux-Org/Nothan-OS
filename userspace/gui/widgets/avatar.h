#ifndef __GUI_AVATAR_H
#define __GUI_AVATAR_H

#include "lvgl/lvgl.h"

/*
 * avatar_create() - Round accent-gradient avatar showing a single
 * initial. @size is the diameter and @font sizes the letter. The caller
 * positions it (in a flex row, centered, etc.). Shared by Contacts and
 * Messages so every contact circle looks identical.
 */
lv_obj_t *avatar_create(lv_obj_t *parent, char initial, int size,
			const lv_font_t *font);

#endif
