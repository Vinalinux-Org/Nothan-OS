/*
 * drivers/video/vsource.c - who the camera is
 *
 * The whole of the registry, because there is one camera.  It is a separate
 * file rather than a few lines inside whichever driver happens to register,
 * for the same reason net_core.c is separate from cpsw.c: a seam that lives
 * inside one of its implementations is not a seam.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/video.h>
#include <nothan/printk.h>

/*
 * PROTECTION: none needed.  Written once during an initcall, before any task
 * that reads it has been created.
 */
static struct video_source *the_source;

int video_source_register(struct video_source *src)
{
	if (!src || !src->capture || !src->release)
		return -1;

	if (!src->width || !src->height)
		return -1;

	if (the_source) {
		printk("[VID] %s refused: %s is already registered\n",
		       src->name, the_source->name);
		return -1;
	}

	the_source = src;

	printk("[VID] %s registered, %lux%lu, %lu bytes a frame\n",
	       src->name,
	       (unsigned long)src->width, (unsigned long)src->height,
	       (unsigned long)video_frame_bytes(src));
	return 0;
}

struct video_source *video_source_get(void)
{
	return the_source;
}
