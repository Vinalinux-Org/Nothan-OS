/*
 * include/nothan/fs.h — VFS file abstractions
 *
 * struct file and struct file_operations vtable.
 * struct file is constructed ephemerally by devfs/procfs on each call.
 */

#ifndef NOTHAN_FS_H
#define NOTHAN_FS_H

#include "types.h"

struct file;

struct file_operations {
    int     (*open)   (struct file *file);
    int     (*release)(struct file *file);
    int     (*read)   (struct file *file, void *buf, uint32_t len);
    int     (*write)  (struct file *file, const void *buf, uint32_t len);
    int     (*ioctl)  (struct file *file, uint32_t cmd, uint32_t arg);
    int32_t (*llseek) (struct file *file, int32_t offset, int whence);
};

struct file {
    const struct file_operations *f_op;
    void                         *private_data;
    uint32_t                      f_pos;
    uint32_t                      f_flags;
};

#endif /* NOTHAN_FS_H */
