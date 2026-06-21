#ifndef __GUI_CONTACTS_LIST_H
#define __GUI_CONTACTS_LIST_H

#include "lvgl/lvgl.h"

/*
 * contacts_list_create() - Contacts app landing screen: header with a
 * "+" action, a search box, and an alphabetically grouped, scrollable
 * list of contacts. A nav builder; @arg is unused. Tapping a row pushes
 * the contact detail; tapping "+" pushes the add screen.
 */
void contacts_list_create(lv_obj_t *screen, void *arg);

#endif
