/*
 * include/nothan/tty.h — shared TTY console interface
 *
 * Producers feed decoded characters here; syscall and devfs consumers read
 * through this layer instead of touching the console input queue directly.
 */

#ifndef NOTHAN_TTY_H
#define NOTHAN_TTY_H

#include "types.h"

int tty_receive_char(uint8_t ch);
int tty_get_char(void);
int tty_read_char(void);
int tty_input_available(void);
void tty_input_clear(void);
int tty_write_buf(const void *buf, uint32_t len);

#endif /* NOTHAN_TTY_H */
