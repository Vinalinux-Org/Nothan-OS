/*
 * kernel/fs/fat/fat.c - FAT32 Core and Superblock handling
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "fat.h"
#include <nothan/printk.h>
#include <nothan/slab.h>

static void fat32_destroy_inode(struct inode *inode)
{
	if (!inode)
		return;
	if (inode->i_private)
		kfree(inode->i_private);
	kfree(inode);
}

static struct super_operations fat32_s_op = {
	.alloc_inode   = NULL,
	.destroy_inode = fat32_destroy_inode,
	.read_inode    = NULL,
	.lookup_root   = fat32_lookup_root,
	.dirlookup     = fat32_dirlookup,
	.create        = fat32_create_file,
	.readdir       = fat32_readdir,
};

static inline u32 rd32(const u8 *p)
{
	return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

/*
 * Scan the MBR partition table (offsets 446–509) for a FAT32 partition
 * (type 0x0B or 0x0C). Returns the partition start LBA, or 0 if not found.
 */
static uint32_t mbr_find_fat32_partition(const u8 *mbr)
{
	for (int i = 0; i < 4; i++) {
		const u8 *entry = mbr + 446 + i * 16;
		u8 type = entry[4];
		if (type == 0x0B || type == 0x0C) {
			uint32_t lba = (u32)entry[8]  | ((u32)entry[9]  << 8) |
				       ((u32)entry[10] << 16) | ((u32)entry[11] << 24);
			if (lba > 0)
				return lba;
		}
	}
	return 0;
}

/**
 * fat32_mount() - Read BPB and initialize FAT32 in-memory structures
 * @sb: The super_block to initialize
 *
 * Handles both direct FAT32 volumes (mock disk, no MBR) and SD cards
 * with an MBR partition table. In the MBR case, scans for a FAT32
 * partition (type 0x0B/0x0C) and reads the BPB from its start LBA.
 *
 * Return: 0 on success, -1 on error.
 */
int fat32_mount(struct super_block *sb)
{
	struct gendisk *disk = sb->s_bdev;
	u8 buf[512];
	uint32_t part_lba = 0;

	if (disk->fops->read_block(disk, 0, buf) != 0) {
		printk("[FAT] Failed to read sector 0\n");
		return -1;
	}

	if (buf[510] != 0x55 || buf[511] != 0xAA) {
		printk("[FAT] Invalid signature at sector 0\n");
		return -1;
	}

	/*
	 * Distinguish MBR from a direct BPB: a valid BPB has bytes_per_sector
	 * at offset 11 equal to 512/1024/2048/4096. An MBR has 0 there.
	 */
	u16 bps = (u16)buf[11] | ((u16)buf[12] << 8);
	if (bps != 512 && bps != 1024 && bps != 2048 && bps != 4096) {
		part_lba = mbr_find_fat32_partition(buf);
		if (part_lba == 0) {
			printk("[FAT] MBR found but no FAT32 partition\n");
			return -1;
		}
		printk("[FAT] MBR: FAT32 partition at LBA %u\n", (unsigned int)part_lba);
		if (disk->fops->read_block(disk, part_lba, buf) != 0) {
			printk("[FAT] Failed to read BPB at LBA %u\n", (unsigned int)part_lba);
			return -1;
		}
	}

	struct fat32_bpb *bpb = (struct fat32_bpb *)buf;

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

	/* root_entry_count and fat_size_16 must be 0 on FAT32 — if not, this is FAT16. */
	if (bpb->root_entry_count != 0 || bpb->fat_size_16 != 0 || bpb->total_sectors_16 != 0) {
		printk("[FAT] Not a FAT32 filesystem (might be FAT16)\n");
		return -1;
	}

	struct fat32_fs_info *info = (struct fat32_fs_info *)kmalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -1;

	info->part_lba            = part_lba;
	info->bytes_per_sector    = bpb->bytes_per_sector;
	info->sectors_per_cluster = bpb->sectors_per_cluster;
	info->bytes_per_cluster   = bpb->bytes_per_sector * bpb->sectors_per_cluster;
	info->fat_start_lba       = part_lba + bpb->reserved_sector_count;
	info->cluster_start_lba   = info->fat_start_lba + (bpb->num_fats * bpb->fat_size_32);
	info->root_cluster        = bpb->root_cluster;
	info->fat_size_32         = bpb->fat_size_32;
	info->num_fats            = bpb->num_fats;

	/* Data region = total - reserved - all FAT copies; clusters start at 2. */
	uint32_t data_sectors = bpb->total_sectors_32 - bpb->reserved_sector_count -
				(bpb->num_fats * bpb->fat_size_32);
	info->total_clusters      = data_sectors / bpb->sectors_per_cluster;

	sb->s_fs_info  = info;
	sb->s_op       = &fat32_s_op;
	sb->s_blocksize = bpb->bytes_per_sector;

	struct inode *root = (struct inode *)kmalloc(sizeof(*root), GFP_KERNEL);
	if (!root)
		return -1;

	root->i_ino     = info->root_cluster;
	root->i_size    = 0;
	root->i_mode    = S_IFDIR;
	root->i_sb      = sb;
	root->i_fop     = NULL;
	root->i_private = NULL;

	sb->s_root = (struct dentry *)kmalloc(sizeof(struct dentry), GFP_KERNEL);
	if (!sb->s_root)
		return -1;

	sb->s_root->d_inode  = root;
	sb->s_root->d_parent = NULL;

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
 * Return: The next cluster number, or FAT32_EOC if end of chain.
 */
uint32_t fat32_get_next_cluster(struct super_block *sb, uint32_t current_cluster)
{
	struct fat32_fs_info *info = (struct fat32_fs_info *)sb->s_fs_info;
	struct gendisk *disk = sb->s_bdev;
	u8 buf[512];

	/* Each FAT32 entry is 4 bytes wide. */
	uint32_t fat_offset = current_cluster * 4;
	uint32_t fat_sector = info->fat_start_lba + (fat_offset / info->bytes_per_sector);
	uint32_t ent_offset = fat_offset % info->bytes_per_sector;

	if (disk->fops->read_block(disk, fat_sector, buf) != 0) {
		printk("[FAT] Failed to read FAT sector %u\n", (unsigned int)fat_sector);
		return FAT32_EOC;
	}

	uint32_t next_cluster = rd32(&buf[ent_offset]) & 0x0FFFFFFF;

	if (next_cluster >= FAT32_EOC)
		return FAT32_EOC;

	return next_cluster;
}

/**
 * fat32_set_fat_entry() - Write a value into a cluster's FAT entry
 * @sb: The super_block
 * @cluster: Cluster whose FAT entry to update
 * @value: New 28-bit value (next cluster, or FAT32_EOC to end a chain)
 *
 * Updates every FAT copy so the redundant tables stay consistent. The
 * top 4 bits of each entry are reserved and preserved.
 *
 * Return: 0 on success, -1 on error.
 */
int fat32_set_fat_entry(struct super_block *sb, uint32_t cluster, uint32_t value)
{
	struct fat32_fs_info *info = (struct fat32_fs_info *)sb->s_fs_info;
	struct gendisk *disk = sb->s_bdev;
	u8 buf[512];

	if (info->bytes_per_sector > sizeof(buf))
		return -1;

	uint32_t fat_offset = cluster * 4;
	uint32_t sector_in_fat = fat_offset / info->bytes_per_sector;
	uint32_t ent_offset = fat_offset % info->bytes_per_sector;

	for (uint32_t f = 0; f < info->num_fats; f++) {
		uint32_t fat_sector = info->fat_start_lba +
				      f * info->fat_size_32 + sector_in_fat;

		if (disk->fops->read_block(disk, fat_sector, buf) != 0)
			return -1;

		/* Preserve the reserved top 4 bits of the existing entry. */
		uint32_t old = rd32(&buf[ent_offset]);
		uint32_t merged = (old & 0xF0000000) | (value & 0x0FFFFFFF);
		buf[ent_offset]     = (u8)(merged & 0xFF);
		buf[ent_offset + 1] = (u8)((merged >> 8) & 0xFF);
		buf[ent_offset + 2] = (u8)((merged >> 16) & 0xFF);
		buf[ent_offset + 3] = (u8)((merged >> 24) & 0xFF);

		if (disk->fops->write_block(disk, fat_sector, buf) != 0)
			return -1;
	}

	return 0;
}

/**
 * fat32_alloc_cluster() - Find a free cluster and mark it end-of-chain
 * @sb: The super_block
 *
 * Linear scan of the FAT from cluster 2 for an entry equal to 0 (free),
 * reading one FAT sector at a time and reusing it across the 128 entries
 * it holds. The found cluster is marked FAT32_EOC before returning so a
 * concurrent caller cannot hand out the same one.
 *
 * Return: Allocated cluster number (>= 2), or 0 if the volume is full.
 */
uint32_t fat32_alloc_cluster(struct super_block *sb)
{
	struct fat32_fs_info *info = (struct fat32_fs_info *)sb->s_fs_info;
	struct gendisk *disk = sb->s_bdev;
	u8 buf[512];

	if (info->bytes_per_sector > sizeof(buf))
		return 0;

	uint32_t entries_per_sector = info->bytes_per_sector / 4;
	uint32_t last_cluster = info->total_clusters + 1; /* clusters 2 .. N+1 */
	uint32_t cached_sector = 0xFFFFFFFF;

	for (uint32_t cluster = 2; cluster <= last_cluster; cluster++) {
		uint32_t sector_in_fat = cluster / entries_per_sector;
		uint32_t fat_sector = info->fat_start_lba + sector_in_fat;

		if (fat_sector != cached_sector) {
			if (disk->fops->read_block(disk, fat_sector, buf) != 0)
				return 0;
			cached_sector = fat_sector;
		}

		uint32_t ent_offset = (cluster % entries_per_sector) * 4;
		uint32_t entry = rd32(&buf[ent_offset]) & 0x0FFFFFFF;
		if (entry == 0) {
			if (fat32_set_fat_entry(sb, cluster, FAT32_EOC) != 0)
				return 0;
			return cluster;
		}
	}

	return 0; /* no free cluster */
}
