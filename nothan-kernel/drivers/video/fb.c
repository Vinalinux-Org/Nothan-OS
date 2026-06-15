/*
 * drivers/video/fb.c - Framebuffer character device (/dev/fb0)
 *
 * Hardware-agnostic framebuffer interface for userspace GUI.
 * flush_cb in lv_port_disp.c sends dirty regions via FB_FLUSH ioctl.
 * The actual hardware driver (HDMI, LCD, e-ink) hooks in via fb_register_ops().
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/cdev.h>
#include <nothan/fs.h>
#include <nothan/fb.h>
#include <nothan/printk.h>
#include <nothan/init.h>

static struct fb_ops *registered_ops;

void fb_register_ops(struct fb_ops *ops)
{
	registered_ops = ops;
	printk("[FB] display backend registered\n");
}

static const struct fb_info fb0_info = {
	.width  = 360,
	.height = 640,
	.bpp    = 16,   /* RGB565 */
};

static int fb0_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	(void)file;

	switch (cmd) {
	case FB_GET_INFO: {
		struct fb_info *dst = (struct fb_info *)arg;
		*dst = fb0_info;
		return 0;
	}
	case FB_FLUSH: {
		struct fb_flush *f = (struct fb_flush *)arg;
		if (registered_ops && registered_ops->flush)
			registered_ops->flush(f->x1, f->y1, f->x2, f->y2,
					      (const void *)f->data, f->len);
		return 0;
	}
	case FB_FLIP:
		if (registered_ops && registered_ops->flip)
			registered_ops->flip();
		return 0;
	default:
		return -1;
	}
}

static const struct file_operations fb0_fops = {
	.ioctl = fb0_ioctl,
};

static struct cdev fb0_cdev = {
	.dev  = MKDEV(29, 0),
	.fops = &fb0_fops,
	.name = "fb0",
};

static int __init fb_init(void)
{
	cdev_register(&fb0_cdev);
	printk("[FB] /dev/fb0 registered (360x640 RGB565)\n");
	return 0;
}
device_initcall(fb_init);
