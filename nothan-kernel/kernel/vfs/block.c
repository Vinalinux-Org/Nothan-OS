/*
 * kernel/vfs/block.c - Block device management
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include <nothan/block.h>
#include <nothan/printk.h>

#define MAX_BDEVS 4

static struct block_device *bdev_table[MAX_BDEVS];

/* No string.h in bare-metal, provide a local strcmp. */
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
 * @name: Name of the block device (e.g., "mock0")
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

