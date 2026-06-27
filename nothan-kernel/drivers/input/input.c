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

	if (!registered_ops)
		return -1;

	/* Pointer (touch) backend: return one raw pointer-state record. */
	if (registered_ops->get_pointer) {
		int x = 0, y = 0, pressed = 0;
		if (count < INPUT_POINTER_RECORD)
			return -1;
		registered_ops->get_pointer(&x, &y, &pressed);
		buf[0] = (char)(x & 0xFF);
		buf[1] = (char)((x >> 8) & 0xFF);
		buf[2] = (char)(y & 0xFF);
		buf[3] = (char)((y >> 8) & 0xFF);
		buf[4] = (char)(pressed ? 1 : 0);
		return INPUT_POINTER_RECORD;
	}

	if (!registered_ops->read_key)
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
