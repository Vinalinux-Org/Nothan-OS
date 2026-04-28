/*
 * include/vinix/cdev.h — character device descriptor
 *
 * A driver fills struct cdev (name, fops, priv) and calls
 * cdev_register(); devfs enumerates the registry and dispatches
 * read/write/ioctl through fops with an ephemeral struct file.
 */

#ifndef VINIX_CDEV_H
#define VINIX_CDEV_H

#include "types.h"
#include "vinix/fs.h"

struct cdev {
    const char                   *name;   /* e.g. "tty", "null" */
    const struct file_operations *fops;
    void                         *priv;   /* copied to file->private_data */
};

int               cdev_register(const struct cdev *cd);
int               cdev_count(void);
const struct cdev *cdev_at(uint32_t index);
int               cdev_find(const char *name);

#endif /* VINIX_CDEV_H */
