#ifndef _NOTHAN_BLOCK_H
#define _NOTHAN_BLOCK_H

#include <nothan/types.h>

struct block_device;

struct block_device_operations {
	int (*read_block)(struct block_device *bdev, uint32_t block, void *buf);
	int (*write_block)(struct block_device *bdev, uint32_t block, const void *buf);
};

struct block_device {
	const char *name;
	const struct block_device_operations *ops;
	void *private_data;
	uint32_t block_size;
	uint32_t total_blocks;
};

void register_block_device(struct block_device *bdev);
struct block_device *get_block_device(const char *name);

#endif /* _NOTHAN_BLOCK_H */
