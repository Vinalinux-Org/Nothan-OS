/* ============================================================
 * vinix/blkdev.h
 * ------------------------------------------------------------
 * Block layer — gendisk + block_device_operations vtable.
 * Drivers fill struct gendisk and call add_disk() to expose it.
 *
 * Today gendisk holds both disk metadata and the fops vtable —
 * Linux splits this further (gendisk + struct block_device per
 * open). MVP collapses since there is no partition device model
 * yet; partitions/msdos.c still parses the MBR but produces
 * partition LBA offsets, not separate gendisks.
 * ============================================================ */

#ifndef VINIX_BLKDEV_H
#define VINIX_BLKDEV_H

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

#endif /* VINIX_BLKDEV_H */
