/*
 * drivers/block/genhd.c - Generic disk registry, MBR partition parser,
 *                          and /dev/sdaN cdev exposure.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/genhd.h>
#include <nothan/cdev.h>
#include <nothan/fs.h>
#include <nothan/printk.h>
#include <nothan/types.h>

#define GENDISK_TABLE_SIZE  4

static struct gendisk *disk_table[GENDISK_TABLE_SIZE];

static int gd_strcmp(const char *a, const char *b)
{
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return *a - *b;
}

static void gd_strcpy(char *dst, const char *src, int max)
{
	int i = 0;
	while (i < max - 1 && src[i]) {
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
}

static void gd_itoa(unsigned int v, char *buf)
{
	if (v == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return;
	}
	char tmp[12];
	int i = 0;
	while (v) {
		tmp[i++] = '0' + v % 10;
		v /= 10;
	}
	int j = 0;
	while (i--)
		buf[j++] = tmp[i];
	buf[j] = '\0';
}

/* minor = disk_index*16 + partno  (0 = whole disk) */
#define BLOCK_MAJOR  8
#define MINORS_PER_DISK 16

#define MAX_PART_CDEVS  (GENDISK_TABLE_SIZE * (DISK_MAX_PARTITIONS + 1))

struct part_cdev_entry {
	struct cdev      cdev;
	struct gendisk  *disk;
	int              partno;   /* 0 = whole disk */
	int              in_use;
};

static struct part_cdev_entry part_cdev_pool[MAX_PART_CDEVS];

static int blk_open(struct inode *inode, struct file *file)
{
	(void)inode; (void)file;
	return 0;
}

static int blk_release(struct inode *inode, struct file *file)
{
	(void)inode; (void)file;
	return 0;
}

static int blk_read(struct file *file, char *buf, size_t count)
{
	(void)file; (void)buf; (void)count;
	return -1; /* use ioctl / blkdev_read() for now */
}

static int blk_write(struct file *file, const char *buf, size_t count)
{
	(void)file; (void)buf; (void)count;
	return -1;
}

static const struct file_operations blk_fops = {
	.open    = blk_open,
	.release = blk_release,
	.read    = blk_read,
	.write   = blk_write,
	.ioctl   = NULL,
};

#define MBR_SIGNATURE_OFFSET  510
#define MBR_PART_TABLE_OFFSET 446
#define MBR_PART_ENTRY_SIZE   16
#define MBR_MAX_PARTS         4

struct mbr_entry {
	u8  status;
	u8  chs_first[3];
	u8  type;
	u8  chs_last[3];
	u32 lba_start;
	u32 lba_size;
} __attribute__((packed));

static int parse_mbr(struct gendisk *disk)
{
	u8 mbr[512];

	if (disk->fops->read_block(disk, 0, mbr) != 0) {
		printk("[BLOCK] %s: MBR read failed\n", disk->disk_name);
		return -1;
	}

	u16 sig = (u16)mbr[MBR_SIGNATURE_OFFSET] |
	          ((u16)mbr[MBR_SIGNATURE_OFFSET + 1] << 8);
	if (sig != 0xAA55) {
		printk("[BLOCK] %s: no MBR signature (got 0x%04x)\n",
		       disk->disk_name, (unsigned int)sig);
		return -1;
	}

	disk->nr_parts = 0;
	const u8 *pt = mbr + MBR_PART_TABLE_OFFSET;

	for (int i = 0; i < MBR_MAX_PARTS; i++) {
		const struct mbr_entry *e = (const struct mbr_entry *)(pt + i * MBR_PART_ENTRY_SIZE);
		if (e->type == 0 || e->lba_size == 0)
			continue;

		int p = disk->nr_parts;
		disk->part[p].partno     = i + 1;
		disk->part[p].start_sect = e->lba_start;
		disk->part[p].nr_sects   = e->lba_size;
		disk->part[p].valid      = 1;
		disk->nr_parts++;

		printk("[BLOCK] %s%d: start=%llu size=%llu sectors type=0x%02x\n",
		       disk->disk_name, i + 1,
		       (unsigned long long)e->lba_start,
		       (unsigned long long)e->lba_size,
		       (unsigned int)e->type);
	}

	return 0;
}

static struct part_cdev_entry *alloc_part_cdev(void)
{
	for (int i = 0; i < MAX_PART_CDEVS; i++) {
		if (!part_cdev_pool[i].in_use) {
			part_cdev_pool[i].in_use = 1;
			return &part_cdev_pool[i];
		}
	}
	return NULL;
}

static void register_disk_cdev(struct gendisk *disk, int disk_idx, int partno)
{
	struct part_cdev_entry *e = alloc_part_cdev();
	if (!e) {
		printk("[BLOCK] cdev pool full\n");
		return;
	}

	e->disk   = disk;
	e->partno = partno;

	gd_strcpy(e->cdev.name, disk->disk_name, CDEV_NAME_LEN);
	if (partno > 0) {
		char num[4];
		gd_itoa((unsigned int)partno, num);
		int len = 0;
		while (e->cdev.name[len]) len++;
		int j = 0;
		while (num[j] && len + j < CDEV_NAME_LEN - 1) {
			e->cdev.name[len + j] = num[j];
			j++;
		}
		e->cdev.name[len + j] = '\0';
	}

	e->cdev.dev  = MKDEV(BLOCK_MAJOR, disk_idx * MINORS_PER_DISK + partno);
	e->cdev.fops = &blk_fops;

	cdev_register(&e->cdev);
}

/**
 * add_disk() - Register a disk, parse MBR, expose cdevs in /dev/
 * @disk: Populated gendisk. Must have disk_name, fops, capacity set.
 *
 * Return: 0 on success, -1 on error.
 */
int add_disk(struct gendisk *disk)
{
	if (!disk || !disk->fops || !disk->fops->read_block)
		return -1;

	int slot = -1;
	for (int i = 0; i < GENDISK_TABLE_SIZE; i++) {
		if (!disk_table[i]) {
			slot = i;
			break;
		}
	}
	if (slot < 0) {
		printk("[BLOCK] disk table full\n");
		return -1;
	}
	disk_table[slot] = disk;

	printk("[BLOCK] %s: registered, capacity=%llu sectors\n",
	       disk->disk_name, (unsigned long long)disk->capacity);

	/* Whole-disk cdev */
	register_disk_cdev(disk, slot, 0);

	/* Parse MBR and register partition cdevs */
	if (parse_mbr(disk) == 0) {
		for (int p = 0; p < disk->nr_parts; p++)
			register_disk_cdev(disk, slot, disk->part[p].partno);
	}

	return 0;
}

/**
 * del_gendisk() - Unregister a disk (does not free memory)
 */
void del_gendisk(struct gendisk *disk)
{
	for (int i = 0; i < GENDISK_TABLE_SIZE; i++) {
		if (disk_table[i] == disk) {
			disk_table[i] = NULL;
			return;
		}
	}
}

/**
 * gendisk_lookup_by_name() - Find a gendisk by its disk_name
 */
struct gendisk *gendisk_lookup_by_name(const char *name)
{
	for (int i = 0; i < GENDISK_TABLE_SIZE; i++) {
		if (disk_table[i] && gd_strcmp(disk_table[i]->disk_name, name) == 0)
			return disk_table[i];
	}
	return NULL;
}

/**
 * blkdev_read() - Read one 512-byte sector from a named block device.
 * @devname: "sda" (whole disk) or "sda1" (partition) as registered in /dev/
 * @sector:  Logical sector number (relative to partition start for partitions)
 * @buf:     512-byte output buffer
 *
 * Return: 0 on success, -1 on error.
 */
int blkdev_read(const char *devname, u64 sector, void *buf)
{
	for (int i = 0; i < MAX_PART_CDEVS; i++) {
		if (!part_cdev_pool[i].in_use)
			continue;
		if (gd_strcmp(part_cdev_pool[i].cdev.name, devname) != 0)
			continue;

		struct part_cdev_entry *e = &part_cdev_pool[i];
		u64 abs_sector;

		if (e->partno == 0) {
			abs_sector = sector;
		} else {
			struct gendisk *disk = e->disk;
			abs_sector = (u64)-1;
			for (int p = 0; p < disk->nr_parts; p++) {
				if (disk->part[p].partno == e->partno) {
					abs_sector = disk->part[p].start_sect + sector;
					break;
				}
			}
			if (abs_sector == (u64)-1)
				return -1;
		}

		return e->disk->fops->read_block(e->disk, abs_sector, buf);
	}
	return -1;
}
