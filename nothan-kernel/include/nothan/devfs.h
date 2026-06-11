/*
 * include/nothan/devfs.h - devfs public API
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#ifndef _NOTHAN_DEVFS_H
#define _NOTHAN_DEVFS_H

#include <nothan/fs.h>

extern struct super_block *devfs_sb;

int devfs_mount(void);

#endif /* _NOTHAN_DEVFS_H */
