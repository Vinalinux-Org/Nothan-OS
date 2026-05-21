/*
 * include/vinix/mouse_cursor.h — mouse cursor API
 */

#ifndef VINIX_MOUSE_CURSOR_H
#define VINIX_MOUSE_CURSOR_H

#include "vinix/input.h"

void    cursor_set_input_dev(struct input_dev *idev);
uint8_t cursor_get_buttons(void);   /* bit 0=left, 1=right, 2=middle */
void    cursor_get_pos(int32_t *x, int32_t *y);
void    cursor_hide(void);
void    cursor_show(void);

#endif
