/*
 * drivers/char/storagebus.c - In-kernel one-way IPC channel from the GUI to
 * a dedicated storage-writer task, so a slow FAT/SD write never blocks the
 * GUI's own task (touch input, LVGL rendering).
 *
 * Exposes two endpoints, one direction only (GUI -> storage daemon):
 *
 *     /dev/storage_fe   GUI side (write-only: enqueue a save request)
 *     /dev/storage_be   storage_daemon side (read-only: dequeue requests)
 *
 * Same design as phonebus.c: a lock-free single-producer/single-consumer
 * byte ring (no locking needed on this single-core target — producer and
 * consumer only alternate across a context switch, a full barrier). Reads
 * are non-blocking (return -1 when empty); writes may be short if the ring
 * is full, so callers loop+yield until fully drained (see storage.c).
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/cdev.h>
#include <nothan/fs.h>
#include <nothan/printk.h>
#include <nothan/init.h>

#define STORAGEBUS_RING_SIZE  65536u        /* must be a power of two */
#define STORAGEBUS_RING_MASK  (STORAGEBUS_RING_SIZE - 1u)

struct storagebus_ring {
	u8           data[STORAGEBUS_RING_SIZE];
	volatile u32 head;   /* producer advances after writing data */
	volatile u32 tail;   /* consumer advances after reading data */
};

/* fe2be: produced by storage_fe write(), consumed by storage_be read(). */
static struct storagebus_ring fe2be;

static int storagebus_ring_read(struct storagebus_ring *r, char *buf, size_t count)
{
	size_t i = 0;
	u32 tail = r->tail;

	while (i < count && tail != r->head) {
		buf[i++] = (char)r->data[tail];
		tail = (tail + 1) & STORAGEBUS_RING_MASK;
	}
	r->tail = tail;
	return i > 0 ? (int)i : -1;
}

static int storagebus_ring_write(struct storagebus_ring *r, const char *buf, size_t count)
{
	size_t i = 0;
	u32 head = r->head;

	while (i < count) {
		u32 next = (head + 1) & STORAGEBUS_RING_MASK;
		if (next == r->tail)
			break;          /* ring full — leave the rest for later */
		r->data[head] = (u8)buf[i++];
		head = next;
	}
	r->head = head;
	return (int)i;
}

/* ---- backend endpoint: /dev/storage_be (storage_daemon reads) ---- */
static int storage_be_read(struct file *file, char *buf, size_t count)
{
	(void)file;
	return storagebus_ring_read(&fe2be, buf, count);
}

/* ---- frontend endpoint: /dev/storage_fe (GUI writes) ---- */
static int storage_fe_write(struct file *file, const char *buf, size_t count)
{
	(void)file;
	return storagebus_ring_write(&fe2be, buf, count);
}

static const struct file_operations storage_be_fops = {
	.read  = storage_be_read,
};

static const struct file_operations storage_fe_fops = {
	.write = storage_fe_write,
};

static struct cdev storage_be_cdev = {
	.dev  = MKDEV(61, 0),
	.fops = &storage_be_fops,
	.name = "storage_be",
};

static struct cdev storage_fe_cdev = {
	.dev  = MKDEV(61, 1),
	.fops = &storage_fe_fops,
	.name = "storage_fe",
};

static int __init storagebus_init(void)
{
	cdev_register(&storage_be_cdev);
	cdev_register(&storage_fe_cdev);
	printk("[STORAGEBUS] /dev/storage_be + /dev/storage_fe registered (ring %u B)\n",
	       STORAGEBUS_RING_SIZE);
	return 0;
}
device_initcall(storagebus_init);
