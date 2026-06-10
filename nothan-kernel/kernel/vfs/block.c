/*
 * kernel/vfs/block.c - Block device management
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include <nothan/block.h>
#include <nothan/printk.h>

#define MAX_BDEVS 4

static struct block_device *bdev_table[MAX_BDEVS];

/* Simple string comparison since we don't have string.h yet */
static int strcmp(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/**
 * register_block_device() - Register a new block device
 * @bdev: Pointer to the block device structure
 */
void register_block_device(struct block_device *bdev)
{
	for (int i = 0; i < MAX_BDEVS; i++) {
		if (!bdev_table[i]) {
			bdev_table[i] = bdev;
			printk("[BLOCK] Registered device '%s'\n", bdev->name);
			return;
		}
	}
	printk("[BLOCK] Cannot register '%s', table full\n", bdev->name);
}

/**
 * get_block_device() - Look up a block device by name
 * @name: Name of the block device (e.g., "sd0")
 *
 * Return: Pointer to block_device, or NULL if not found.
 */
struct block_device *get_block_device(const char *name)
{
	for (int i = 0; i < MAX_BDEVS; i++) {
		if (bdev_table[i] && bdev_table[i]->name) {
			if (strcmp(name, bdev_table[i]->name) == 0)
				return bdev_table[i];
		}
	}
	return NULL;
}

/* =========================================================================
 * Mock Block Device (RAM-based) for testing VFS without hardware
 * ========================================================================= */

#define MOCK_BLOCK_SIZE 512
#define MOCK_BLOCKS 128 /* 64 KB total */
static u8 mock_ram_disk[MOCK_BLOCKS * MOCK_BLOCK_SIZE];

static int mock_read_block(struct block_device *bdev, uint32_t block, void *buf)
{
	if (block >= bdev->total_blocks)
		return -1;
	
	const u8 *src = mock_ram_disk + (block * bdev->block_size);
	u8 *dst = (u8 *)buf;
	for (uint32_t i = 0; i < bdev->block_size; i++)
		dst[i] = src[i];
	
	return 0;
}

static int mock_write_block(struct block_device *bdev, uint32_t block, const void *buf)
{
	if (block >= bdev->total_blocks)
		return -1;
	
	u8 *dst = mock_ram_disk + (block * bdev->block_size);
	const u8 *src = (const u8 *)buf;
	for (uint32_t i = 0; i < bdev->block_size; i++)
		dst[i] = src[i];
	
	return 0;
}

static struct block_device_operations mock_bdev_ops = {
	.read_block = mock_read_block,
	.write_block = mock_write_block,
};

static struct block_device mock_bdev = {
	.name = "mock0",
	.ops = &mock_bdev_ops,
	.private_data = NULL,
	.block_size = MOCK_BLOCK_SIZE,
	.total_blocks = MOCK_BLOCKS,
};

/**
 * mock_bdev_init() - Initialize the mock block device
 */
void mock_bdev_init(void)
{
	/* Fill sector 0 with a fake FAT32 BPB */
	u8 *bpb = mock_ram_disk;
	bpb[11] = 0x00; bpb[12] = 0x02; /* 512 bytes per sector */
	bpb[13] = 8;                    /* 8 sectors per cluster (4KB) */
	bpb[14] = 32; bpb[15] = 0;      /* 32 reserved sectors */
	bpb[16] = 2;                    /* 2 FATs */
	bpb[32] = 0x00; bpb[33] = 0x20; bpb[34] = 0x00; bpb[35] = 0x00; /* Total sectors: 8192 (4MB) */
	bpb[36] = 0x10; bpb[37] = 0x00; bpb[38] = 0x00; bpb[39] = 0x00; /* FAT size: 16 sectors */
	bpb[44] = 2; bpb[45] = 0x00; bpb[46] = 0x00; bpb[47] = 0x00;    /* Root cluster: 2 */

	/* BPB signature at bytes 510-511 */
	bpb[510] = 0x55;
	bpb[511] = 0xAA;

	/* FAT table: cluster 2 → EOC (root dir), cluster 3 → EOC (SHELL.BIN data) */
	u8 *fat_table = mock_ram_disk + (32 * 512); /* FAT starts at sector 32 */
	fat_table[8]  = 0xFF; fat_table[9]  = 0xFF; fat_table[10] = 0xFF; fat_table[11] = 0x0F; /* cluster 2 = EOC */
	fat_table[12] = 0xFF; fat_table[13] = 0xFF; fat_table[14] = 0xFF; fat_table[15] = 0x0F; /* cluster 3 = EOC */

	/* Fake Directory Entry at Root Cluster (Cluster 2)
	 * FAT_START = 32, FAT_SIZE = 16 * 2 = 32. DATA_START = 64.
	 * So Cluster 2 is at Sector 64.
	 */
	u8 *root_dir = mock_ram_disk + (64 * 512);
	/* "SHELL   BIN", attr=0x20 (Archive) */
	root_dir[0] = 'S'; root_dir[1] = 'H'; root_dir[2] = 'E'; root_dir[3] = 'L'; root_dir[4] = 'L';
	root_dir[5] = ' '; root_dir[6] = ' '; root_dir[7] = ' '; 
	root_dir[8] = 'B'; root_dir[9] = 'I'; root_dir[10] = 'N';
	root_dir[11] = 0x20; 
	/* Start cluster = 3 (0x0003) */
	root_dir[20] = 0x00; root_dir[21] = 0x00; /* fst_clus_hi */
	root_dir[26] = 0x03; root_dir[27] = 0x00; /* fst_clus_lo */
	/* File size = 44 bytes */
	root_dir[28] = 44; root_dir[29] = 0x00; root_dir[30] = 0x00; root_dir[31] = 0x00;
	
	/* Second entry: End of directory (0x00) is already there since ram disk is 0-initialized */

	/* SHELL.BIN data is at cluster 3 */
	/* cluster 3 sector = cluster_start_lba + (3 - 2) * sectors_per_cluster = 64 + 8 = 72 */
	u8 *shell_data = mock_ram_disk + (72 * 512);
	const char *msg = "Hello from NothanOS FAT32 mock file system!";
	for (int i = 0; msg[i]; i++) {
		shell_data[i] = msg[i];
	}

	register_block_device(&mock_bdev);
	printk("[BLOCK] Mock RAM disk initialized with Fake FAT32 BPB\n");
}
