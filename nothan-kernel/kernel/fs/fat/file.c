/*
 * kernel/fs/fat/file.c - FAT32 File Data Operations
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "fat.h"
#include <nothan/slab.h>
#include <nothan/printk.h>

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
	struct gendisk *disk = sb->s_bdev;

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
			if (disk->fops->read_block(disk, first_sector + s, sec_buf) != 0) {
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

/*
 * fat_update_dirent() - Write back a file's size and first cluster into
 * its on-disk 32-byte directory entry. Located via inode->i_private,
 * recorded when the inode was looked up or created.
 */
static int fat_update_dirent(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	struct fat32_fs_info *info = (struct fat32_fs_info *)sb->s_fs_info;
	struct gendisk *disk = sb->s_bdev;
	struct fat32_inode_private *priv = inode->i_private;
	u8 buf[512];

	if (!priv || info->bytes_per_sector > sizeof(buf))
		return -1;

	if (disk->fops->read_block(disk, priv->dirent_sector, buf) != 0)
		return -1;

	struct fat32_dir_entry *ent =
		(struct fat32_dir_entry *)(buf + priv->dirent_offset);
	ent->file_size   = inode->i_size;
	ent->fst_clus_hi = (u16)(inode->i_ino >> 16);
	ent->fst_clus_lo = (u16)(inode->i_ino & 0xFFFF);

	return disk->fops->write_block(disk, priv->dirent_sector, buf);
}

/**
 * fat32_file_write() - Write data to a file, extending its cluster chain
 * @f: The file object
 * @buf: User buffer
 * @count: Number of bytes to write
 *
 * Allocates clusters as needed (first cluster for an empty file, then
 * chain extension), does read-modify-write for partial sectors, and
 * writes back the directory entry when the size or first cluster change.
 *
 * Return: Number of bytes written, or < 0 on error.
 */
int fat32_file_write(struct file *f, const char *buf, size_t count)
{
	struct inode *inode = f->f_inode;
	struct super_block *sb = inode->i_sb;
	struct fat32_fs_info *info = (struct fat32_fs_info *)sb->s_fs_info;
	struct gendisk *disk = sb->s_bdev;
	int meta_dirty = 0;

	if (count == 0)
		return 0;
	if (!disk->fops->write_block)
		return -1;

	uint32_t cluster_size = info->bytes_per_cluster;
	uint32_t current_cluster = inode->i_ino;

	/* Empty file: allocate its first cluster. */
	if (current_cluster < 2) {
		uint32_t first = fat32_alloc_cluster(sb);
		if (first == 0)
			return -1;
		inode->i_ino = first;
		current_cluster = first;
		meta_dirty = 1;
	}

	/* Walk to the cluster holding f_pos, extending the chain if short. */
	uint32_t cluster_offset = f->f_pos / cluster_size;
	for (uint32_t i = 0; i < cluster_offset; i++) {
		uint32_t next = fat32_get_next_cluster(sb, current_cluster);
		if (next >= FAT32_EOC) {
			next = fat32_alloc_cluster(sb);
			if (next == 0)
				return -1;
			if (fat32_set_fat_entry(sb, current_cluster, next) != 0)
				return -1;
		}
		current_cluster = next;
	}

	u8 *sec_buf = kmalloc(info->bytes_per_sector, GFP_KERNEL);
	if (!sec_buf)
		return -1;

	uint32_t bytes_written = 0;
	uint32_t offset_in_cluster = f->f_pos % cluster_size;

	while (bytes_written < count && current_cluster < FAT32_EOC) {
		uint32_t first_sector = fat32_cluster_to_sector(info, current_cluster);
		uint32_t sector_offset = offset_in_cluster / info->bytes_per_sector;
		uint32_t offset_in_sector = offset_in_cluster % info->bytes_per_sector;

		for (uint32_t s = sector_offset;
		     s < info->sectors_per_cluster && bytes_written < count; s++) {
			uint32_t to_copy = info->bytes_per_sector - offset_in_sector;
			if (to_copy > count - bytes_written)
				to_copy = count - bytes_written;

			/* Partial sector: preserve the bytes we are not touching. */
			if (offset_in_sector != 0 || to_copy != info->bytes_per_sector) {
				if (disk->fops->read_block(disk, first_sector + s, sec_buf) != 0) {
					for (uint32_t k = 0; k < info->bytes_per_sector; k++)
						sec_buf[k] = 0;
				}
			}

			const char *src = buf + bytes_written;
			u8 *dst = sec_buf + offset_in_sector;
			for (uint32_t k = 0; k < to_copy; k++)
				dst[k] = (u8)src[k];

			if (disk->fops->write_block(disk, first_sector + s, sec_buf) != 0) {
				kfree(sec_buf);
				goto out;
			}

			bytes_written += to_copy;
			offset_in_sector = 0;
		}

		offset_in_cluster = 0;
		if (bytes_written < count) {
			uint32_t next = fat32_get_next_cluster(sb, current_cluster);
			if (next >= FAT32_EOC) {
				next = fat32_alloc_cluster(sb);
				if (next == 0)
					break;
				if (fat32_set_fat_entry(sb, current_cluster, next) != 0)
					break;
			}
			current_cluster = next;
		}
	}

	kfree(sec_buf);

out:
	f->f_pos += bytes_written;
	if (f->f_pos > inode->i_size) {
		inode->i_size = f->f_pos;
		meta_dirty = 1;
	}
	if (meta_dirty && bytes_written > 0)
		fat_update_dirent(inode);

	return bytes_written > 0 ? (int)bytes_written : -1;
}

const struct file_operations fat32_file_operations = {
	.read = fat32_file_read,
	.write = fat32_file_write,
	.open = NULL,
	.release = NULL,
};
