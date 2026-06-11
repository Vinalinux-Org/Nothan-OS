/*
 * include/nothan/genhd.h - Generic disk (gendisk) framework
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#ifndef _NOTHAN_GENHD_H
#define _NOTHAN_GENHD_H

#include <nothan/types.h>

#define DISK_MAX_PARTITIONS  8
#define DISK_NAME_LEN        16

struct gendisk;

struct block_device_operations {
	int (*read_block)(struct gendisk *disk, u64 sector, void *buf);
	int (*write_block)(struct gendisk *disk, u64 sector, const void *buf);
};

struct hd_struct {
	u64  start_sect;
	u64  nr_sects;
	int  partno;        /* 0 = whole disk, 1..N = partitions */
	int  valid;
};

struct gendisk {
	char                              disk_name[DISK_NAME_LEN];
	int                               major;
	int                               first_minor;
	const struct block_device_operations *fops;
	void                             *private_data;
	u64                               capacity;
	struct hd_struct                  part[DISK_MAX_PARTITIONS];
	int                               nr_parts;
};

int  add_disk(struct gendisk *disk);
void del_gendisk(struct gendisk *disk);

struct gendisk *gendisk_lookup_by_name(const char *name);

/* Read one 512-byte sector from a partition (sector is relative to partition start) */
int blkdev_read(const char *devname, u64 sector, void *buf);

#endif /* _NOTHAN_GENHD_H */
