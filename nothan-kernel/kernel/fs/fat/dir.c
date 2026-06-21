/*
 * kernel/fs/fat/dir.c - FAT32 Directory Operations
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "fat.h"
#include <nothan/printk.h>
#include <nothan/slab.h>

static void fmt_name_83(const u8 raw[11], char *out)
{
	int si, di = 0;
	for (si = 0; si < 8 && raw[si] != ' '; si++)
		out[di++] = raw[si];
	if (raw[8] != ' ') {
		out[di++] = '.';
		for (si = 8; si < 11 && raw[si] != ' '; si++)
			out[di++] = raw[si];
	}
	out[di] = '\0';
}

static void name_to_raw(const char *in, u8 raw[11])
{
	int i, pos;
	for (i = 0; i < 11; i++)
		raw[i] = ' ';
	pos = 0;
	while (in[pos] && in[pos] != '.' && pos < 8) {
		char c = in[pos];
		if (c >= 'a' && c <= 'z')
			c -= 32;
		raw[pos] = (u8)c;
		pos++;
	}
	if (in[pos] == '.') {
		pos++;
		for (i = 0; i < 3 && in[pos]; i++, pos++) {
			char c = in[pos];
			if (c >= 'a' && c <= 'z')
				c -= 32;
			raw[8 + i] = (u8)c;
		}
	}
}

/**
 * fat32_list_dir() - Print all entries in a directory cluster chain
 * @sb: The super_block
 * @dir_cluster: Starting cluster of the directory
 */
void fat32_list_dir(struct super_block *sb, uint32_t dir_cluster)
{
	struct fat32_fs_info *info = (struct fat32_fs_info *)sb->s_fs_info;
	struct gendisk *disk = sb->s_bdev;
	u8 buf[512];
	uint32_t current_cluster = dir_cluster;

	if (info->bytes_per_sector > sizeof(buf))
		return;

	while (current_cluster < FAT32_EOC) {
		uint32_t first_sector = fat32_cluster_to_sector(info, current_cluster);
		for (uint32_t i = 0; i < info->sectors_per_cluster; i++) {
			if (disk->fops->read_block(disk, first_sector + i, buf) != 0)
				return;
			struct fat32_dir_entry *ent = (struct fat32_dir_entry *)buf;
			uint32_t n = info->bytes_per_sector / sizeof(struct fat32_dir_entry);
			for (uint32_t j = 0; j < n; j++, ent++) {
				if (ent->name[0] == 0x00)
					return;
				if (ent->name[0] == 0xE5)
					continue;
				if (ent->attr == ATTR_LFN || (ent->attr & ATTR_VOLUME))
					continue;
				char name[13];
				fmt_name_83(ent->name, name);
				uint32_t cl = ((uint32_t)ent->fst_clus_hi << 16) | ent->fst_clus_lo;
				if (ent->attr & ATTR_DIRECTORY)
					printk("[FAT] [DIR]  %s (Cluster: %u)\n",
					       name, (unsigned int)cl);
				else
					printk("[FAT] [FILE] %s (Size: %u bytes, Cluster: %u)\n",
					       name, (unsigned int)ent->file_size, (unsigned int)cl);
			}
		}
		current_cluster = fat32_get_next_cluster(sb, current_cluster);
	}
}

static struct inode *fat_entry_to_inode(struct super_block *sb,
	const struct fat32_dir_entry *ent, uint32_t dirent_sector,
	uint32_t dirent_offset)
{
	struct inode *inode = (struct inode *)kmalloc(sizeof(*inode), GFP_KERNEL);
	if (!inode)
		return NULL;

	inode->i_ino     = ((uint32_t)ent->fst_clus_hi << 16) | ent->fst_clus_lo;
	inode->i_size    = ent->file_size;
	inode->i_sb      = sb;
	inode->i_mode    = (ent->attr & ATTR_DIRECTORY) ? S_IFDIR : S_IFREG;
	inode->i_fop     = (ent->attr & ATTR_DIRECTORY) ? NULL : &fat32_file_operations;
	inode->i_private = NULL;

	/* Remember where the entry lives so writes can update file_size. */
	struct fat32_inode_private *priv =
		(struct fat32_inode_private *)kmalloc(sizeof(*priv), GFP_KERNEL);
	if (priv) {
		priv->dirent_sector = dirent_sector;
		priv->dirent_offset = dirent_offset;
		inode->i_private = priv;
	}
	return inode;
}

/**
 * fat32_lookup_root() - Look up a name in the root directory
 * @sb: The super_block
 * @name: Entry name (case-insensitive 8.3 form)
 *
 * Return: Allocated inode, or NULL if not found.
 */
struct inode *fat32_lookup_root(struct super_block *sb, const char *name)
{
	struct inode *root_inode = (struct inode *)kmalloc(sizeof(*root_inode), GFP_KERNEL);
	if (!root_inode)
		return NULL;

	root_inode->i_ino     = ((struct fat32_fs_info *)sb->s_fs_info)->root_cluster;
	root_inode->i_sb      = sb;
	root_inode->i_mode    = S_IFDIR;
	root_inode->i_private = NULL;

	struct inode *result = fat32_dirlookup(root_inode, name);
	kfree(root_inode); /* temporary search root — dirlookup copies what it needs */
	return result;
}

/**
 * fat32_dirlookup() - Look up one path component in a directory
 * @dir: Inode of the directory to search (i_ino = start cluster)
 * @name: Component name (case-insensitive, 8.3 format)
 *
 * Return: Allocated inode for the found entry, or NULL if not found.
 */
struct inode *fat32_dirlookup(struct inode *dir, const char *name)
{
	struct fat32_fs_info *info = (struct fat32_fs_info *)dir->i_sb->s_fs_info;
	struct gendisk *disk = dir->i_sb->s_bdev;
	u8 buf[512];
	uint32_t cluster = dir->i_ino;
	u8 query[11];

	if (info->bytes_per_sector > sizeof(buf))
		return NULL;

	name_to_raw(name, query);

	while (cluster >= 2 && cluster < FAT32_EOC) {
		uint32_t base = fat32_cluster_to_sector(info, cluster);
		for (uint32_t s = 0; s < info->sectors_per_cluster; s++) {
			if (disk->fops->read_block(disk, base + s, buf) != 0)
				return NULL;
			struct fat32_dir_entry *ent = (struct fat32_dir_entry *)buf;
			uint32_t n = info->bytes_per_sector / sizeof(struct fat32_dir_entry);
			for (uint32_t j = 0; j < n; j++, ent++) {
				if (ent->name[0] == 0x00)
					return NULL;
				if (ent->name[0] == 0xE5)
					continue;
				if (ent->attr == ATTR_LFN || (ent->attr & ATTR_VOLUME))
					continue;

				int match = 1;
				for (int k = 0; k < 11; k++) {
					if (ent->name[k] != query[k]) {
						match = 0;
						break;
					}
				}
				if (match) {
					uint32_t ent_off = (u8 *)ent - buf;
					return fat_entry_to_inode(dir->i_sb, ent,
								  base + s, ent_off);
				}
			}
		}
		cluster = fat32_get_next_cluster(dir->i_sb, cluster);
	}
	return NULL;
}

/* Fill a fresh 32-byte regular-file directory entry (empty, cluster 0). */
static void build_file_dirent(struct fat32_dir_entry *ent, const u8 raw[11])
{
	for (int k = 0; k < 11; k++)
		ent->name[k] = raw[k];
	ent->attr           = ATTR_ARCHIVE;
	ent->nt_res         = 0;
	ent->crt_time_tenth = 0;
	ent->crt_time       = 0;
	ent->crt_date       = 0;
	ent->last_acc_date  = 0;
	ent->fst_clus_hi    = 0;
	ent->wrt_time       = 0;
	ent->wrt_date       = 0;
	ent->fst_clus_lo    = 0;
	ent->file_size      = 0;
}

/**
 * fat32_create_file() - Create an empty regular file in a directory
 * @sb: The super_block
 * @dir: Directory inode to create the entry in (i_ino = start cluster)
 * @name: New file name (8.3, case-insensitive)
 *
 * Reuses a deleted (0xE5) or unused (0x00) directory slot, extending the
 * directory with a fresh cluster if none is free. The file starts empty
 * (first cluster 0, size 0); fat32_file_write() allocates clusters on
 * first write.
 *
 * Return: Allocated inode for the new file, or NULL on error.
 */
struct inode *fat32_create_file(struct super_block *sb, struct inode *dir,
				const char *name)
{
	struct fat32_fs_info *info = (struct fat32_fs_info *)sb->s_fs_info;
	struct gendisk *disk = sb->s_bdev;
	u8 buf[512];
	uint32_t cluster = dir->i_ino;
	uint32_t prev_cluster = cluster;
	u8 raw[11];

	if (info->bytes_per_sector > sizeof(buf) || !disk->fops->write_block)
		return NULL;

	name_to_raw(name, raw);

	/* Scan existing directory clusters for a free slot. */
	while (cluster >= 2 && cluster < FAT32_EOC) {
		uint32_t base = fat32_cluster_to_sector(info, cluster);
		for (uint32_t s = 0; s < info->sectors_per_cluster; s++) {
			if (disk->fops->read_block(disk, base + s, buf) != 0)
				return NULL;
			uint32_t n = info->bytes_per_sector / sizeof(struct fat32_dir_entry);
			struct fat32_dir_entry *ent = (struct fat32_dir_entry *)buf;
			for (uint32_t j = 0; j < n; j++, ent++) {
				if (ent->name[0] != 0x00 && ent->name[0] != 0xE5)
					continue;
				build_file_dirent(ent, raw);
				if (disk->fops->write_block(disk, base + s, buf) != 0)
					return NULL;
				uint32_t off = (u8 *)ent - buf;
				return fat_entry_to_inode(sb, ent, base + s, off);
			}
		}
		prev_cluster = cluster;
		cluster = fat32_get_next_cluster(sb, cluster);
	}

	/* Directory full — append a new cluster and place the entry first. */
	uint32_t newcl = fat32_alloc_cluster(sb);
	if (newcl == 0)
		return NULL;
	if (fat32_set_fat_entry(sb, prev_cluster, newcl) != 0)
		return NULL;

	uint32_t base = fat32_cluster_to_sector(info, newcl);
	for (uint32_t b = 0; b < info->bytes_per_sector; b++)
		buf[b] = 0;
	/* Zero every sector of the new directory cluster first. */
	for (uint32_t s = 0; s < info->sectors_per_cluster; s++) {
		if (disk->fops->write_block(disk, base + s, buf) != 0)
			return NULL;
	}

	struct fat32_dir_entry *ent = (struct fat32_dir_entry *)buf;
	build_file_dirent(ent, raw);
	if (disk->fops->write_block(disk, base, buf) != 0)
		return NULL;
	return fat_entry_to_inode(sb, ent, base, 0);
}

/**
 * fat32_readdir() - Fill buffer with directory entries
 * @dir: Inode of the directory (i_ino = start cluster)
 * @buf: Output buffer of file_entry structs
 * @max: Maximum number of entries to fill
 *
 * Return: Number of entries filled.
 */
int fat32_readdir(struct inode *dir, struct file_entry *buf, int max)
{
	struct fat32_fs_info *info = (struct fat32_fs_info *)dir->i_sb->s_fs_info;
	struct gendisk *disk = dir->i_sb->s_bdev;
	u8 buf512[512];
	uint32_t cluster = dir->i_ino;
	int count = 0;

	if (info->bytes_per_sector > sizeof(buf512))
		return 0;

	while (cluster >= 2 && cluster < FAT32_EOC && count < max) {
		uint32_t base = fat32_cluster_to_sector(info, cluster);
		for (uint32_t s = 0; s < info->sectors_per_cluster && count < max; s++) {
			if (disk->fops->read_block(disk, base + s, buf512) != 0)
				return count;
			struct fat32_dir_entry *ent = (struct fat32_dir_entry *)buf512;
			uint32_t n = info->bytes_per_sector / sizeof(struct fat32_dir_entry);
			for (uint32_t j = 0; j < n && count < max; j++, ent++) {
				if (ent->name[0] == 0x00)
					return count;
				if (ent->name[0] == 0xE5)
					continue;
				if (ent->attr == ATTR_LFN || (ent->attr & ATTR_VOLUME))
					continue;
				fmt_name_83(ent->name, buf[count].name);
				buf[count].size = ent->file_size;
				/* Append '/' to directory names so callers can distinguish them. */
				if (ent->attr & ATTR_DIRECTORY) {
					int sl = 0;
					while (buf[count].name[sl])
						sl++;
					if (sl < FILE_NAME_LEN - 1) {
						buf[count].name[sl]     = '/';
						buf[count].name[sl + 1] = '\0';
					}
				}
				count++;
			}
		}
		cluster = fat32_get_next_cluster(dir->i_sb, cluster);
	}
	return count;
}

