/*
 * include/nothan/console_input.h — private shared console input queue
 *
 * TTY core owns this queue; drivers and syscall/devfs code should use
 * nothan/tty.h instead of this low-level buffer interface.
 */

#ifndef NOTHAN_CONSOLE_INPUT_H
#define NOTHAN_CONSOLE_INPUT_H

#include "types.h"
#include "wait_queue.h"

#define CONSOLE_INPUT_BUFFER_SIZE 256

extern wait_queue_head_t console_input_wq;

int console_input_push(uint8_t ch);
int console_input_getc(void);
int console_input_available(void);
void console_input_clear(void);

#endif /* NOTHAN_CONSOLE_INPUT_H */
