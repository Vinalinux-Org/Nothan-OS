/*
 * include/nothan/cdev.h - Character device framework
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#ifndef _NOTHAN_CDEV_H
#define _NOTHAN_CDEV_H

#include <nothan/types.h>
#include <nothan/fs.h>

typedef u32 dev_t;

#define MAJOR(dev)       ((dev) >> 20)
#define MINOR(dev)       ((dev) & 0xFFFFFU)
#define MKDEV(ma, mi)    (((u32)(ma) << 20) | ((u32)(mi) & 0xFFFFFU))

#define CDEV_NAME_LEN    16
#define CDEV_TABLE_SIZE  32

struct cdev {
	dev_t                         dev;
	const struct file_operations *fops;
	char                          name[CDEV_NAME_LEN];
};

int  cdev_register(struct cdev *cdev);
void cdev_unregister(struct cdev *cdev);
struct cdev *cdev_lookup_by_name(const char *name);
struct cdev *cdev_lookup_by_dev(dev_t dev);
struct cdev *cdev_lookup_by_index(int index);

#endif /* _NOTHAN_CDEV_H */
