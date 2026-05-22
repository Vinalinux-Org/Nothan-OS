/*
 * include/nothan/kbd_input.h — minimal keyboard input core.
 */

#ifndef NOTHAN_KBD_INPUT_H
#define NOTHAN_KBD_INPUT_H

#include "types.h"

int kbd_input_publish_char(uint8_t ch);
int kbd_input_read_char(void);
int kbd_input_available(void);

#endif /* NOTHAN_KBD_INPUT_H */
