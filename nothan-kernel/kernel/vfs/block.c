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

	/* FAT table: cluster 2-6 -> EOC */
	u8 *fat_table = mock_ram_disk + (32 * 512);
	for (int i = 2; i <= 7; i++) {
		int off = i * 4;
		fat_table[off] = 0xFF; fat_table[off+1] = 0xFF;
		fat_table[off+2] = 0xFF; fat_table[off+3] = 0x0F;
	}

	/* Root directory at Cluster 2 (sector 64) */
	u8 *root_dir = mock_ram_disk + (64 * 512);

	/* Entry 0: SHELL.BIN (deleted) */
	root_dir[0] = 0xE5; /* 0xE5 is the FAT32 marker for deleted file */

	/* Entry 1: EXAMPLE.BIN, archive */
	root_dir[32] = 'E'; root_dir[33] = 'X'; root_dir[34] = 'A'; root_dir[35] = 'M'; root_dir[36] = 'P';
	root_dir[37] = 'L'; root_dir[38] = 'E'; root_dir[39] = ' ';
	root_dir[40] = 'B'; root_dir[41] = 'I'; root_dir[42] = 'N';
	root_dir[43] = 0x20;
	root_dir[52] = 0; root_dir[53] = 0;
	root_dir[58] = 7; root_dir[59] = 0;
	root_dir[60] = 74; root_dir[61] = 0; root_dir[62] = 0; root_dir[63] = 0;

	/* Entry 2: "bin" directory */
	int e2 = 64;
	root_dir[e2+0] = 'B'; root_dir[e2+1] = 'I'; root_dir[e2+2] = 'N';
	root_dir[e2+11] = 0x10;
	root_dir[e2+20] = 0; root_dir[e2+21] = 0;
	root_dir[e2+26] = 4; root_dir[e2+27] = 0;

	/* Entry 3: "sbin" directory */
	int e3 = 96;
	root_dir[e3+0] = 'S'; root_dir[e3+1] = 'B'; root_dir[e3+2] = 'I'; root_dir[e3+3] = 'N';
	root_dir[e3+11] = 0x10;
	root_dir[e3+20] = 0; root_dir[e3+21] = 0;
	root_dir[e3+26] = 5; root_dir[e3+27] = 0;

	/* Entry 4: End marker (zeroed ram) */

	/* SHELL.BIN data at cluster 3 (sector 72) */
	u8 *shell_data = mock_ram_disk + (72 * 512);
	const char *msg = "Hello from NothanOS FAT32 mock file system!";
	for (int i = 0; msg[i]; i++) shell_data[i] = msg[i];

	/* /bin directory at cluster 4 (sector 80): hello.bin */
	u8 *bin_dir = mock_ram_disk + (80 * 512);
	bin_dir[0] = 'H'; bin_dir[1] = 'E'; bin_dir[2] = 'L'; bin_dir[3] = 'L'; bin_dir[4] = 'O';
	bin_dir[5] = ' '; bin_dir[6] = ' '; bin_dir[7] = ' ';
	bin_dir[8] = 'B'; bin_dir[9] = 'I'; bin_dir[10] = 'N';
	bin_dir[11] = 0x20;
	bin_dir[20] = 0; bin_dir[21] = 0;
	bin_dir[26] = 6; bin_dir[27] = 0;
	bin_dir[28] = 48; bin_dir[29] = 0; bin_dir[30] = 0; bin_dir[31] = 0;

	/* /bin/hello.bin data at cluster 6 (sector 96) */
	u8 *hello_data = mock_ram_disk + (96 * 512);
	const char *hmsg = "Hello from /bin/hello.bin!\n";
	for (int i = 0; hmsg[i]; i++) hello_data[i] = hmsg[i];

	/* /sbin at cluster 5 (sector 88): empty directory */

	/* EXAMPLE.BIN at cluster 7 (sector 104), 74 bytes */
	{
		u8 *d = mock_ram_disk + (104 * 512);
		d[0]=0x02; d[1]=0x00; d[2]=0x00; d[3]=0xeb; d[4]=0x01; d[5]=0x70; d[6]=0xa0; d[7]=0xe3;
		d[8]=0x00; d[9]=0x00; d[10]=0x00; d[11]=0xef; d[12]=0xfb; d[13]=0xff; d[14]=0xff; d[15]=0xea;
		d[16]=0x04; d[17]=0x70; d[18]=0x2d; d[19]=0xe5; d[20]=0x34; d[21]=0x00; d[22]=0x00; d[23]=0xe3;
		d[24]=0x03; d[25]=0x70; d[26]=0xa0; d[27]=0xe3; d[28]=0x01; d[29]=0x00; d[30]=0x40; d[31]=0xe3;
		d[32]=0x00; d[33]=0x00; d[34]=0x00; d[35]=0xef; d[36]=0x00; d[37]=0x00; d[38]=0xa0; d[39]=0xe3;
		d[40]=0x01; d[41]=0x70; d[42]=0xa0; d[43]=0xe3; d[44]=0x00; d[45]=0x00; d[46]=0x00; d[47]=0xef;
		d[48]=0xfe; d[49]=0xff; d[50]=0xff; d[51]=0xea; d[52]=0x57; d[53]=0x65; d[54]=0x6c; d[55]=0x63;
		d[56]=0x6f; d[57]=0x6d; d[58]=0x65; d[59]=0x20; d[60]=0x74; d[61]=0x6f; d[62]=0x20; d[63]=0x4e;
		d[64]=0x6f; d[65]=0x74; d[66]=0x68; d[67]=0x61; d[68]=0x6e; d[69]=0x4f; d[70]=0x53; d[71]=0x21;
		d[72]=0x0a; d[73]=0x00;
	}	register_block_device(&mock_bdev);
	printk("[BLOCK] Mock RAM disk initialized with Fake FAT32 BPB\n");
}
