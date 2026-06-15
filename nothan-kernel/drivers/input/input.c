/*
 * drivers/input/input.c - Input character device (/dev/input0)
 *
 * Exposes touch/keyboard input to userspace via read().
 * Returns no data until a real input driver registers via input_register_ops().
 * This avoids competing with the UART console (/dev/ttyS0) for RX bytes.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/cdev.h>
#include <nothan/fs.h>
#include <nothan/input.h>
#include <nothan/printk.h>
#include <nothan/init.h>

static struct input_ops *registered_ops;

void input_register_ops(struct input_ops *ops)
{
	registered_ops = ops;
	printk("[INPUT] input backend registered\n");
}

static int input0_read(struct file *file, char *buf, size_t count)
{
	(void)file;
	size_t i = 0;

	if (!registered_ops || !registered_ops->read_key)
		return -1;

	while (i < count) {
		int c = registered_ops->read_key();
		if (c < 0)
			break;
		buf[i++] = (char)c;
	}
	return i > 0 ? (int)i : -1;
}

static const struct file_operations input0_fops = {
	.read = input0_read,
};

static struct cdev input0_cdev = {
	.dev  = MKDEV(13, 0),
	.fops = &input0_fops,
	.name = "input0",
};

static int __init input_init(void)
{
	cdev_register(&input0_cdev);
	printk("[INPUT] /dev/input0 registered\n");
	return 0;
}
device_initcall(input_init);
