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
#include <nothan/uaccess.h>

static struct fb_ops *registered_ops;

void fb_register_ops(struct fb_ops *ops)
{
	registered_ops = ops;
	printk("[FB] display backend registered\n");
}

/*
 * Report the framebuffer's RAW scanout geometry — the native landscape surface
 * the LCDC actually drives (matches FB_W/FB_H and the flush bounds in lcdc.c).
 * Rotation to portrait is the GUI's concern, not the framebuffer's, so the
 * userspace port queries this and applies LVGL's 270° rotation itself.
 */
static const struct fb_info fb0_info = {
	.width  = 800,
	.height = 480,
	.bpp    = 16,   /* RGB565 */
};

static int fb0_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	(void)file;

	switch (cmd) {
	case FB_GET_INFO: {
		/* Kernel writes sizeof(fb_info) into a user pointer — prove the
		 * range is the caller's own memory, never kernel/unmapped. */
		if (copy_to_user((void *)arg, &fb0_info, sizeof(fb0_info)))
			return -1;
		return 0;
	}
	case FB_FLUSH: {
		struct fb_flush f;

		/* Pull the request struct out of user space safely. */
		if (copy_from_user(&f, (const void *)arg, sizeof(f)))
			return -1;
		/* The backend will read f.len bytes from f.data; prove that
		 * source range is the caller's own user memory before the read. */
		if (!access_ok((const void *)f.data, f.len))
			return -1;
		if (registered_ops && registered_ops->flush)
			registered_ops->flush(f.x1, f.y1, f.x2, f.y2,
					      (const void *)f.data, f.len);
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
	printk("[FB] /dev/fb0 registered (800x480 RGB565, native landscape)\n");
	return 0;
}
device_initcall(fb_init);
