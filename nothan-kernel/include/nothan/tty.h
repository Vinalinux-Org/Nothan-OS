/*
 * include/nothan/tty.h — TTY device interface
 */

#ifndef NOTHAN_TTY_H
#define NOTHAN_TTY_H

#include "types.h"
#include "nothan/fs.h"

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

#endif /* NOTHAN_TTY_H */
