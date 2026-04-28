/* ============================================================
 * vinix/fs.h
 * ------------------------------------------------------------
 * struct file + struct file_operations — Linux-style vtable
 * any character/block driver implements.
 *
 * Today struct file is constructed ephemerally by devfs/procfs
 * on each read/write call (no inode → no persistent file table
 * outside per-process FDs in vfs_fd). Future inode work will
 * link f_inode and persist file across calls.
 * ============================================================ */

#ifndef VINIX_FS_H
#define VINIX_FS_H

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

#endif /* VINIX_FS_H */
