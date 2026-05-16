/*
 * include/nothan/blkdev.h — block device interface
 *
 * gendisk descriptor and block_device_operations vtable.
 * Drivers fill struct gendisk and call add_disk() to register a block device.
 */

#ifndef NOTHAN_BLKDEV_H
#define NOTHAN_BLKDEV_H

#include "types.h"

struct gendisk;

struct block_device_operations {
    int (*read_sectors) (struct gendisk *disk, uint32_t lba,
                         uint32_t count, void *buf);
    int (*write_sectors)(struct gendisk *disk, uint32_t lba,
                         uint32_t count, const void *buf);
};

struct gendisk {
    const char                            *name;          /* e.g. "mmc0" */
    uint32_t                               sector_size;   /* bytes, usually 512 */
    uint32_t                               total_sectors;
    const struct block_device_operations  *fops;
    void                                  *priv;
};

int             add_disk(struct gendisk *disk);
struct gendisk *get_gendisk(const char *name);
int             blk_read (struct gendisk *disk, uint32_t lba, uint32_t count, void *buf);
int             blk_write(struct gendisk *disk, uint32_t lba, uint32_t count, const void *buf);

#endif /* NOTHAN_BLKDEV_H */
