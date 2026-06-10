/*
 * kernel/fs/fat/fat.c - FAT32 Core and Superblock handling
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "fat.h"
#include <nothan/printk.h>
#include <nothan/slab.h>


static inline u32 rd32(const u8 *p) {
	return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static struct super_operations fat32_s_op = {
	.alloc_inode = NULL, /* To be implemented in Step 4 */
	.destroy_inode = NULL,
	.read_inode = NULL,
	.lookup_root = fat32_lookup_root,
};

/**
 * fat32_mount() - Read BPB and initialize FAT32 in-memory structures
 * @sb: The super_block to initialize
 *
 * Return: 0 on success, < 0 on error
 */
int fat32_mount(struct super_block *sb)
{
	struct block_device *bdev = sb->s_bdev;
	u8 buf[512]; /* Read 1 sector */

	if (bdev->ops->read_block(bdev, 0, buf) != 0) {
		printk("[FAT] Failed to read sector 0\n");
		return -1;
	}

	struct fat32_bpb *bpb = (struct fat32_bpb *)buf;

	/* Basic validation */
	if (buf[510] != 0x55 || buf[511] != 0xAA) {
		printk("[FAT] Invalid BPB signature\n");
		return -1;
	}

	if (bpb->bytes_per_sector != 512 && bpb->bytes_per_sector != 1024 &&
	    bpb->bytes_per_sector != 2048 && bpb->bytes_per_sector != 4096) {
		printk("[FAT] Unsupported sector size %d\n", bpb->bytes_per_sector);
		return -1;
	}

	if (bpb->sectors_per_cluster == 0 ||
	    (bpb->sectors_per_cluster & (bpb->sectors_per_cluster - 1))) {
		printk("[FAT] Invalid sectors per cluster %d\n", bpb->sectors_per_cluster);
		return -1;
	}

	/* FAT32 specific checks */
	if (bpb->root_entry_count != 0 || bpb->fat_size_16 != 0 || bpb->total_sectors_16 != 0) {
		printk("[FAT] Not a FAT32 filesystem (might be FAT16)\n");
		return -1;
	}

	/* Initialize fs_info */
	struct fat32_fs_info *info = (struct fat32_fs_info *)kmalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -1;

	info->bytes_per_sector = bpb->bytes_per_sector;
	info->sectors_per_cluster = bpb->sectors_per_cluster;
	info->bytes_per_cluster = bpb->bytes_per_sector * bpb->sectors_per_cluster;
	
	info->fat_start_lba = bpb->reserved_sector_count;
	info->cluster_start_lba = info->fat_start_lba + (bpb->num_fats * bpb->fat_size_32);
	info->root_cluster = bpb->root_cluster;

	sb->s_fs_info = info;
	sb->s_op = &fat32_s_op;
	sb->s_blocksize = bpb->bytes_per_sector;

	printk("[FAT] Mounted (%u B/sector, %u s/cl, %u FAT sectors)\n",
	       (unsigned int)bpb->bytes_per_sector,
	       (unsigned int)bpb->sectors_per_cluster,
	       (unsigned int)bpb->fat_size_32);

	return 0;
}

/**
 * fat32_get_next_cluster() - Look up the FAT table to get the next cluster
 * @sb: The super_block
 * @current_cluster: The current cluster number
 *
 * Return: The next cluster number, or FAT32_EOC if end of file.
 */
uint32_t fat32_get_next_cluster(struct super_block *sb, uint32_t current_cluster)
{
	struct fat32_fs_info *info = (struct fat32_fs_info *)sb->s_fs_info;
	struct block_device *bdev = sb->s_bdev;
	u8 buf[512];

	/* In FAT32, each entry is 4 bytes. */
	uint32_t fat_offset = current_cluster * 4;
	uint32_t fat_sector = info->fat_start_lba + (fat_offset / info->bytes_per_sector);
	uint32_t ent_offset = fat_offset % info->bytes_per_sector;

	if (bdev->ops->read_block(bdev, fat_sector, buf) != 0) {
		printk("[FAT] Failed to read FAT sector %u\n", (unsigned int)fat_sector);
		return FAT32_EOC; /* Treat read error as EOF */
	}

	uint32_t next_cluster = rd32(&buf[ent_offset]) & 0x0FFFFFFF;
	
	if (next_cluster >= FAT32_EOC)
		return FAT32_EOC;
	
	return next_cluster;
}
