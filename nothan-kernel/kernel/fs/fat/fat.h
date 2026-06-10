#ifndef _NOTHAN_FAT_H
#define _NOTHAN_FAT_H

#include <nothan/types.h>
#include <nothan/fs.h>

#define FAT32_EOC 0x0FFFFFF8

#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LFN        0x0F
#define ATTR_VOLUME     0x08

struct fat32_bpb {
	u8  jmp_boot[3];
	u8  oem_name[8];
	u16 bytes_per_sector;
	u8  sectors_per_cluster;
	u16 reserved_sector_count;
	u8  num_fats;
	u16 root_entry_count;
	u16 total_sectors_16;
	u8  media;
	u16 fat_size_16;
	u16 sectors_per_track;
	u16 num_heads;
	u32 hidden_sectors;
	u32 total_sectors_32;
	u32 fat_size_32;
	u16 ext_flags;
	u16 fs_version;
	u32 root_cluster;
	u16 fs_info;
	u16 backup_boot_sector;
	u8  reserved[12];
	u8  drive_number;
	u8  reserved1;
	u8  boot_signature;
	u32 volume_id;
	u8  volume_label[11];
	u8  fs_type[8];
} __attribute__((packed));

struct fat32_fs_info {
	uint32_t fat_start_lba;
	uint32_t cluster_start_lba;
	uint32_t root_cluster;
	uint32_t sectors_per_cluster;
	uint32_t bytes_per_sector;
	uint32_t bytes_per_cluster;
};

struct fat32_dir_entry {
	u8  name[11];
	u8  attr;
	u8  nt_res;
	u8  crt_time_tenth;
	u16 crt_time;
	u16 crt_date;
	u16 last_acc_date;
	u16 fst_clus_hi;
	u16 wrt_time;
	u16 wrt_date;
	u16 fst_clus_lo;
	u32 file_size;
} __attribute__((packed));

static inline uint32_t fat32_cluster_to_sector(struct fat32_fs_info *info, uint32_t cluster)
{
	return info->cluster_start_lba + ((cluster - 2) * info->sectors_per_cluster);
}

int fat32_mount(struct super_block *sb);
uint32_t fat32_get_next_cluster(struct super_block *sb, uint32_t current_cluster);
void fat32_list_dir(struct super_block *sb, uint32_t dir_cluster);
struct inode *fat32_lookup_root(struct super_block *sb, const char *name);
struct inode *fat32_dirlookup(struct inode *dir, const char *name);
int fat32_readdir(struct inode *dir, struct file_entry *buf, int max);

extern const struct file_operations fat32_file_operations;

#endif /* _NOTHAN_FAT_H */
