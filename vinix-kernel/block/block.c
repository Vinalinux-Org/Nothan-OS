/* ============================================================
 * block.c
 * ------------------------------------------------------------
 * Flat gendisk registry — Linux's add_disk pattern at minimal
 * scale. Single consumer per disk, no locking.
 * ============================================================ */

/* INVARIANT: single consumer per disk — no locking. Add a
 * spinlock if concurrent mounts ever appear. */

#include "vinix/blkdev.h"
#include "string.h"
#include "uart.h"
#include "syscalls.h"

#define MAX_DISKS 4

static struct gendisk *disks[MAX_DISKS];
static int             num_disks = 0;

int add_disk(struct gendisk *disk)
{
    if (num_disks >= MAX_DISKS) return E_FAIL;
    disks[num_disks++] = disk;
    pr_info("[BLK] registered %s (%u sectors x %u B)\n",
                disk->name, disk->total_sectors, disk->sector_size);
    return E_OK;
}

struct gendisk *get_gendisk(const char *name)
{
    for (int i = 0; i < num_disks; i++) {
        if (disks[i] && strcmp(disks[i]->name, name) == 0) return disks[i];
    }
    return 0;
}

int blk_read(struct gendisk *disk, uint32_t lba, uint32_t count, void *buf)
{
    if (!disk || !disk->fops || !disk->fops->read_sectors) return E_FAIL;
    return disk->fops->read_sectors(disk, lba, count, buf);
}

int blk_write(struct gendisk *disk, uint32_t lba, uint32_t count, const void *buf)
{
    if (!disk || !disk->fops || !disk->fops->write_sectors) return E_PERM;
    return disk->fops->write_sectors(disk, lba, count, buf);
}
