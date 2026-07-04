/*
 * drivers/base/cdev.c - Character device registry
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * Simple static table of registered char devices.
 * Drivers call cdev_register() during probe(); devfs uses
 * cdev_lookup_by_name() to resolve open("/dev/<name>") paths.
 */

#include <nothan/cdev.h>
#include <nothan/printk.h>

static struct cdev *cdev_table[CDEV_TABLE_SIZE];

static int cdev_strcmp(const char *a, const char *b)
{
	while (*a && *a == *b) { a++; b++; }
	return *a - *b;
}

/**
 * cdev_register() - Register a character device
 * @cdev: Pointer to the cdev to register. Must have .name and .fops set.
 *
 * Return: 0 on success, -1 if the table is full or name is duplicate.
 */
int cdev_register(struct cdev *cdev)
{
	if (!cdev || !cdev->name[0] || !cdev->fops)
		return -1;

	for (int i = 0; i < CDEV_TABLE_SIZE; i++) {
		if (cdev_table[i] && cdev_table[i] == cdev)
			return -1;
	}

	for (int i = 0; i < CDEV_TABLE_SIZE; i++) {
		if (!cdev_table[i]) {
			cdev_table[i] = cdev;
			printk("[CDEV] registered '%s' major=%lu minor=%lu\n",
				cdev->name,
				(unsigned long)MAJOR(cdev->dev),
				(unsigned long)MINOR(cdev->dev));
			return 0;
		}
	}

	printk("[CDEV] table full, cannot register '%s'\n", cdev->name);
	return -1;
}

/**
 * cdev_unregister() - Unregister a character device
 * @cdev: Pointer to the cdev to remove.
 */
void cdev_unregister(struct cdev *cdev)
{
	if (!cdev)
		return;
	for (int i = 0; i < CDEV_TABLE_SIZE; i++) {
		if (cdev_table[i] == cdev) {
			cdev_table[i] = NULL;
			printk("[CDEV] unregistered '%s'\n", cdev->name);
			return;
		}
	}
}

/**
 * cdev_lookup_by_name() - Find a cdev by device name
 * @name: Device name (e.g. "ttyS0", "i2c0")
 *
 * Return: Pointer to the matching cdev, or NULL if not found.
 */
struct cdev *cdev_lookup_by_name(const char *name)
{
	if (!name)
		return NULL;
	for (int i = 0; i < CDEV_TABLE_SIZE; i++) {
		if (!cdev_table[i])
			continue;
		if (cdev_strcmp(cdev_table[i]->name, name) == 0)
			return cdev_table[i];
	}
	return NULL;
}

/**
 * cdev_lookup_by_dev() - Find a cdev by device number
 * @dev: Device number (major:minor encoded with MKDEV)
 *
 * Return: Pointer to the matching cdev, or NULL if not found.
 */
struct cdev *cdev_lookup_by_dev(dev_t dev)
{
	for (int i = 0; i < CDEV_TABLE_SIZE; i++) {
		if (cdev_table[i] && cdev_table[i]->dev == dev)
			return cdev_table[i];
	}
	return NULL;
}

/**
 * cdev_lookup_by_index() - Return the cdev at slot @index in the table.
 * @index: Table index [0, CDEV_TABLE_SIZE).
 *
 * Used by devfs_readdir() to iterate all registered devices.
 * Return: Pointer to cdev, or NULL if slot is empty or index out of range.
 */
struct cdev *cdev_lookup_by_index(int index)
{
	if (index < 0 || index >= CDEV_TABLE_SIZE)
		return NULL;
	return cdev_table[index];
}
