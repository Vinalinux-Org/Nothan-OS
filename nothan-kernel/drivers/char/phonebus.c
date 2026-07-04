/*
 * drivers/char/phonebus.c - In-kernel loopback IPC channel between two
 * user-space processes (the telephony backend daemon and the GUI).
 *
 * Exposes two cross-wired character-device endpoints:
 *
 *     /dev/phone_be   backend daemon side
 *     /dev/phone_fe   GUI frontend side
 *
 * Bytes written to one endpoint become readable bytes at the other:
 *
 *     write(/dev/phone_be) -> [be2fe ring] -> read(/dev/phone_fe)
 *     write(/dev/phone_fe) -> [fe2be ring] -> read(/dev/phone_be)
 *
 * It is a virtual serial loopback (a "null-modem cable") with no hardware.
 * Each direction is a single-producer / single-consumer byte ring, so no
 * locking is needed: exactly one writer and one reader touch each ring,
 * mirroring the RX ring in omap-serial.c. Correct on this single-core
 * target because producer and consumer only alternate across a context
 * switch (a full barrier); it assumes ONE opener per endpoint (the daemon
 * owns phone_be, the GUI owns phone_fe).
 *
 * Reads are non-blocking: they return whatever is available, or -1 when the
 * ring is empty (same convention as /dev/ttyS0 and /dev/input0), so callers
 * poll. Writes return the number of bytes accepted, which may be short if
 * the ring is full. The framed-JSON protocol (phone_frame) rides unchanged
 * on top of this byte stream.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/cdev.h>
#include <nothan/fs.h>
#include <nothan/printk.h>
#include <nothan/init.h>

#define PHONEBUS_RING_SIZE  8192u           /* must be a power of two */
#define PHONEBUS_RING_MASK  (PHONEBUS_RING_SIZE - 1u)

struct phonebus_ring {
	u8           data[PHONEBUS_RING_SIZE];
	volatile u32 head;   /* producer advances after writing data */
	volatile u32 tail;   /* consumer advances after reading data */
};

/*
 * be2fe: produced by phone_be write(), consumed by phone_fe read().
 * fe2be: produced by phone_fe write(), consumed by phone_be read().
 */
static struct phonebus_ring be2fe;
static struct phonebus_ring fe2be;

/* Drain up to @count bytes from @r into @buf. Non-blocking. */
static int phonebus_ring_read(struct phonebus_ring *r, char *buf, size_t count)
{
	size_t i = 0;
	u32 tail = r->tail;

	while (i < count && tail != r->head) {
		buf[i++] = (char)r->data[tail];
		tail = (tail + 1) & PHONEBUS_RING_MASK;
	}
	r->tail = tail;
	return i > 0 ? (int)i : -1;
}

/*
 * Enqueue up to @count bytes from @buf into @r. Returns the number of bytes
 * accepted; a short return means the ring filled (caller should yield/retry).
 */
static int phonebus_ring_write(struct phonebus_ring *r, const char *buf, size_t count)
{
	size_t i = 0;
	u32 head = r->head;

	while (i < count) {
		u32 next = (head + 1) & PHONEBUS_RING_MASK;
		if (next == r->tail)
			break;          /* ring full — leave the rest for later */
		r->data[head] = (u8)buf[i++];
		head = next;
	}
	r->head = head;
	return (int)i;
}

/* ---- backend endpoint: /dev/phone_be ---- */
static int phone_be_read(struct file *file, char *buf, size_t count)
{
	(void)file;
	return phonebus_ring_read(&fe2be, buf, count);
}

static int phone_be_write(struct file *file, const char *buf, size_t count)
{
	(void)file;
	return phonebus_ring_write(&be2fe, buf, count);
}

/* ---- frontend endpoint: /dev/phone_fe ---- */
static int phone_fe_read(struct file *file, char *buf, size_t count)
{
	(void)file;
	return phonebus_ring_read(&be2fe, buf, count);
}

static int phone_fe_write(struct file *file, const char *buf, size_t count)
{
	(void)file;
	return phonebus_ring_write(&fe2be, buf, count);
}

static const struct file_operations phone_be_fops = {
	.read  = phone_be_read,
	.write = phone_be_write,
};

static const struct file_operations phone_fe_fops = {
	.read  = phone_fe_read,
	.write = phone_fe_write,
};

static struct cdev phone_be_cdev = {
	.dev  = MKDEV(60, 0),
	.fops = &phone_be_fops,
	.name = "phone_be",
};

static struct cdev phone_fe_cdev = {
	.dev  = MKDEV(60, 1),
	.fops = &phone_fe_fops,
	.name = "phone_fe",
};

static int __init phonebus_init(void)
{
	cdev_register(&phone_be_cdev);
	cdev_register(&phone_fe_cdev);
	printk("[PHONEBUS] /dev/phone_be + /dev/phone_fe registered (ring %u B/dir)\n",
	       PHONEBUS_RING_SIZE);
	return 0;
}
device_initcall(phonebus_init);
