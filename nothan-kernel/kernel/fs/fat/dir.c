/*
 * kernel/fs/fat/dir.c - FAT32 Directory Operations
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "fat.h"
#include <nothan/printk.h>
#include <nothan/slab.h>

/*
 * Format an 8.3 raw name ("HELLO   TXT") into a human-readable form
 * ("HELLO.TXT" or "HELLO" if no extension).
 */
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

/**
 * fat32_list_dir() - List contents of a directory cluster (For testing)
 * @sb: The super_block
 * @dir_cluster: The starting cluster of the directory
 */
void fat32_list_dir(struct super_block *sb, uint32_t dir_cluster)
{
	struct fat32_fs_info *info = (struct fat32_fs_info *)sb->s_fs_info;
	struct block_device *bdev = sb->s_bdev;
	u8 buf[512];
	uint32_t current_cluster = dir_cluster;

	if (info->bytes_per_sector > sizeof(buf)) {
		printk("[FAT] Sector size too large for dir buffer (%u)\n",
		       (unsigned int)info->bytes_per_sector);
		return;
	}

	printk("[FAT] --- Directory Listing (Cluster %u) ---\n", (unsigned int)dir_cluster);

	while (current_cluster < FAT32_EOC) {
		uint32_t first_sector = fat32_cluster_to_sector(info, current_cluster);
		
		for (uint32_t i = 0; i < info->sectors_per_cluster; i++) {
			if (bdev->ops->read_block(bdev, first_sector + i, buf) != 0) {
				printk("[FAT] Error reading dir sector\n");
				return;
			}

			struct fat32_dir_entry *ent = (struct fat32_dir_entry *)buf;
			uint32_t entries_per_sector = info->bytes_per_sector / sizeof(struct fat32_dir_entry);
			
			for (uint32_t j = 0; j < entries_per_sector; j++, ent++) {
				if (ent->name[0] == 0x00) {
					/* End of directory */
					printk("[FAT] --- End of Directory ---\n");
					return;
				}
				if (ent->name[0] == 0xE5) /* Deleted entry */
					continue;
				if (ent->attr == 0x0F) /* LFN entry */
					continue;
				if (ent->attr & 0x08) /* Volume Label */
					continue;

				/* Format and print the 8.3 name */
				char name_buf[13];
				fmt_name_83(ent->name, name_buf);

				uint32_t start_cluster = ((uint32_t)ent->fst_clus_hi << 16) | ent->fst_clus_lo;
				
				if (ent->attr & 0x10) {
					printk("[FAT] [DIR]  %s (Cluster: %u)\n", 
						name_buf, (unsigned int)start_cluster);
				} else {
					printk("[FAT] [FILE] %s (Size: %u bytes, Cluster: %u)\n", 
						name_buf, (unsigned int)ent->file_size, (unsigned int)start_cluster);
				}
			}
		}
		current_cluster = fat32_get_next_cluster(sb, current_cluster);
	}
	printk("[FAT] --- End of Directory ---\n");
}

static int inline_strcmp(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/**
 * fat32_lookup_root() - Search for a file in the root directory
 * @sb: The super_block
 * @name: Name of the file to find (e.g. "SHELL.BIN")
 *
 * Return: A newly allocated inode, or NULL if not found
 */
struct inode *fat32_lookup_root(struct super_block *sb, const char *name)
{
	struct fat32_fs_info *info = (struct fat32_fs_info *)sb->s_fs_info;
	struct block_device *bdev = sb->s_bdev;
	u8 buf[512];
	uint32_t current_cluster = info->root_cluster;

	if (info->bytes_per_sector > sizeof(buf))
		return NULL;

	while (current_cluster < FAT32_EOC) {
		uint32_t first_sector = fat32_cluster_to_sector(info, current_cluster);
		
		for (uint32_t i = 0; i < info->sectors_per_cluster; i++) {
			if (bdev->ops->read_block(bdev, first_sector + i, buf) != 0)
				return NULL;

			struct fat32_dir_entry *ent = (struct fat32_dir_entry *)buf;
			uint32_t entries_per_sector = info->bytes_per_sector / sizeof(struct fat32_dir_entry);
			
			for (uint32_t j = 0; j < entries_per_sector; j++, ent++) {
				if (ent->name[0] == 0x00)
					return NULL; /* End of dir */
				if (ent->name[0] == 0xE5)
					continue;
				if (ent->attr == 0x0F || (ent->attr & 0x08))
					continue;

				char name_buf[13];
				fmt_name_83(ent->name, name_buf);

				if (inline_strcmp(name_buf, name) == 0) {
					/* Found it! Allocate inode */
					struct inode *inode = kmalloc(sizeof(struct inode), GFP_KERNEL);
					if (!inode)
						return NULL;
					
					inode->i_ino = ((uint32_t)ent->fst_clus_hi << 16) | ent->fst_clus_lo;
					inode->i_size = ent->file_size;
					inode->i_sb = sb;
					inode->i_fop = &fat32_file_operations;
					inode->i_mode = 0; /* regular file */
					
					return inode;
				}
			}
		}
		current_cluster = fat32_get_next_cluster(sb, current_cluster);
	}
	return NULL;
}
