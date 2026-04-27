/* ============================================================
 * mmc/block.c
 * ------------------------------------------------------------
 * MMC -> block bridge. Wraps an mmc_host's sector I/O in a
 * gendisk + block_device_operations and registers it with the
 * block layer (add_disk).
 *
 * Linux's mmc_block builds struct mmc_request from each block
 * I/O and calls host->ops->request. MVP shortcut: host driver
 * passes plain read/write callbacks (no request object) which
 * mmc_block stores and trampolines via the gendisk fops.
 * ============================================================ */

#include "vinix/mmc/host.h"
#include "vinix/blkdev.h"
#include "vinix/printk.h"
#include "vinix/errno.h"

#define MAX_MMC_DISKS 2

struct mmc_disk_io {
    int (*read_sectors) (uint32_t lba, uint32_t count, void *buf);
    int (*write_sectors)(uint32_t lba, uint32_t count, const void *buf);
};

struct mmc_disk {
    struct gendisk                          disk;
    struct block_device_operations          fops;
    struct mmc_disk_io                      io;
    int                                     in_use;
};

static struct mmc_disk disks[MAX_MMC_DISKS];

/* Single-host MVP: read/write trampolines call into the host's
 * sector callbacks via a global. When a second host appears,
 * thread the disk pointer through gendisk->priv. */
static struct mmc_disk *current_disk = 0;

static int mmc_block_read(struct gendisk *disk, uint32_t lba,
                          uint32_t count, void *buf)
{
    (void)disk;
    if (!current_disk || !current_disk->io.read_sectors) return -EIO;
    return current_disk->io.read_sectors(lba, count, buf);
}

static int mmc_block_write(struct gendisk *disk, uint32_t lba,
                           uint32_t count, const void *buf)
{
    (void)disk;
    if (!current_disk || !current_disk->io.write_sectors) return -EIO;
    return current_disk->io.write_sectors(lba, count, buf);
}

int mmc_block_register(struct mmc_host *host,
                       int (*read_fn) (uint32_t, uint32_t, void *),
                       int (*write_fn)(uint32_t, uint32_t, const void *),
                       uint32_t total_sectors)
{
    /* Find a free slot */
    struct mmc_disk *md = 0;
    for (int i = 0; i < MAX_MMC_DISKS; i++) {
        if (!disks[i].in_use) { md = &disks[i]; break; }
    }
    if (!md) return -ENOSPC;

    md->io.read_sectors  = read_fn;
    md->io.write_sectors = write_fn;
    md->fops.read_sectors  = mmc_block_read;
    md->fops.write_sectors = mmc_block_write;
    md->disk.name          = host ? host->name : "mmc?";
    md->disk.sector_size   = 512;
    md->disk.total_sectors = total_sectors;
    md->disk.fops          = &md->fops;
    md->disk.priv          = host;
    md->in_use             = 1;

    current_disk = md;
    return add_disk(&md->disk);
}
