#ifndef __GUI_CONTACTS_ADD_H
#define __GUI_CONTACTS_ADD_H

#include "lvgl/lvgl.h"

/*
 * contacts_add_create() - Form to add a contact: name + phone fields and
 * a Save button. A nav builder; @arg is unused. Save commits to the
 * contacts store and pops back to the list.
 */
void contacts_add_create(lv_obj_t *screen, void *arg);

#endif
