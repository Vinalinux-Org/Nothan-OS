/* ============================================================
 * vinix/tty.h
 * ------------------------------------------------------------
 * tty_driver / tty_operations — line-discipline-facing vtable
 * shared by serial, console, and pty backends.
 *
 * Today the only TTY is /dev/tty, which goes straight to the
 * UART. This header defines the interface for when a real
 * tty_io.c core lands (line discipline, echo, canonical mode).
 * ============================================================ */

#ifndef VINIX_TTY_H
#define VINIX_TTY_H

#include "types.h"
#include "vinix/fs.h"

struct tty_struct;

struct tty_operations {
    int  (*open) (struct tty_struct *tty, struct file *filp);
    void (*close)(struct tty_struct *tty, struct file *filp);
    int  (*write)(struct tty_struct *tty, const unsigned char *buf, int count);
    int  (*write_room)(struct tty_struct *tty);
};

struct tty_driver {
    const char                   *driver_name;
    const char                   *name;       /* /dev node prefix, e.g., "ttyS" */
    int                           major;
    int                           minor_start;
    int                           num;        /* number of minors */
    const struct tty_operations  *ops;
};

int tty_register_driver(struct tty_driver *drv);

#endif /* VINIX_TTY_H */
