/*
 * kernel/fs/fat/file.c - FAT32 File Data Operations
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "fat.h"
#include <nothan/slab.h>

/**
 * fat32_file_read() - Read data from a file
 * @f: The file object
 * @buf: User buffer
 * @count: Number of bytes to read
 *
 * Return: Number of bytes read, or < 0 on error
 */
int fat32_file_read(struct file *f, char *buf, size_t count)
{
	struct inode *inode = f->f_inode;
	struct super_block *sb = inode->i_sb;
	struct fat32_fs_info *info = (struct fat32_fs_info *)sb->s_fs_info;
	struct block_device *bdev = sb->s_bdev;

	if (f->f_pos >= inode->i_size)
		return 0; /* EOF */

	if (f->f_pos + count > inode->i_size)
		count = inode->i_size - f->f_pos;

	uint32_t current_cluster = inode->i_ino; /* We store start cluster in i_ino */
	uint32_t cluster_size = info->bytes_per_cluster;

	/* Advance to the correct starting cluster */
	uint32_t cluster_offset = f->f_pos / cluster_size;
	for (uint32_t i = 0; i < cluster_offset; i++) {
		current_cluster = fat32_get_next_cluster(sb, current_cluster);
		if (current_cluster >= FAT32_EOC)
			return 0; /* Unexpected EOF */
	}

	uint32_t bytes_read = 0;
	uint32_t offset_in_cluster = f->f_pos % cluster_size;

	/* Create a temporary buffer for reading full sectors */
	u8 *sec_buf = kmalloc(info->bytes_per_sector, GFP_KERNEL);
	if (!sec_buf)
		return -1;

	while (bytes_read < count && current_cluster < FAT32_EOC) {
		uint32_t first_sector = fat32_cluster_to_sector(info, current_cluster);
		uint32_t sector_offset = offset_in_cluster / info->bytes_per_sector;
		uint32_t offset_in_sector = offset_in_cluster % info->bytes_per_sector;

		for (uint32_t s = sector_offset; s < info->sectors_per_cluster && bytes_read < count; s++) {
			if (bdev->ops->read_block(bdev, first_sector + s, sec_buf) != 0) {
				kfree(sec_buf);
				return bytes_read > 0 ? (int)bytes_read : -1;
			}

			uint32_t to_copy = info->bytes_per_sector - offset_in_sector;
			if (to_copy > count - bytes_read)
				to_copy = count - bytes_read;

			char *dst = buf + bytes_read;
			const u8 *src = sec_buf + offset_in_sector;
			for (uint32_t k = 0; k < to_copy; k++)
				dst[k] = src[k];

			bytes_read += to_copy;
			offset_in_sector = 0; /* Next sectors are aligned */
		}

		offset_in_cluster = 0;	/* next clusters start at offset 0 */
		if (bytes_read < count)
			current_cluster = fat32_get_next_cluster(sb, current_cluster);
	}

	kfree(sec_buf);
	f->f_pos += bytes_read;
	return bytes_read;
}

const struct file_operations fat32_file_operations = {
	.read = fat32_file_read,
	.write = NULL,
	.open = NULL,
	.release = NULL,
};
