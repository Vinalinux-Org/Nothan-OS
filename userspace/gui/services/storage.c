/*
 * services/storage.c - Persistent storage backend (hardware)
 *
 * storage_read() maps straight onto the FAT32 filesystem through the VFS
 * syscalls (only used at boot to load a saved blob, before the GUI is
 * interactive — a blocking read there is harmless).
 *
 * storage_write() does NOT touch the SD card itself: a real FAT write can
 * take a while, and the GUI is a single task with no threads, so a
 * blocking write here would freeze touch input for however long the card
 * takes. Instead it hands (path, blob) off to storage_daemon over the
 * storage bus (storagebus.c) and returns immediately; the daemon does the
 * actual open()+writefile()+close() on its own task, on its own time.
 * See storage_daemon.c for the wire format.
 *
 * The GUI simulator builds storage_sim.c instead (no kernel there).
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "storage.h"
#include "../../lib/syscall.h"

#define STORAGE_BUS_DEV  "/dev/storage_fe"
#define STORAGE_MAGIC0   0xB5
#define STORAGE_MAGIC1   0x0B

int storage_read(const char *path, void *buf, int max_len)
{
	long fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	long n = read(fd, buf, (unsigned long)max_len);
	close(fd);
	return (int)n;
}

/* Push @count bytes into the storage bus, retrying (yield-and-retry) until
 * the ring accepts all of them. This can still loop a few times for a big
 * blob (the ring is smaller than e.g. the whole SMS store), but draining is
 * a RAM-speed copy on storage_daemon's side, not an SD card write — nothing
 * like the latency this replaces. */
static void bus_write_all(long fd, const char *buf, int count)
{
	int off = 0;
	while (off < count) {
		long n = writefile(fd, buf + off, (unsigned long)(count - off));
		if (n > 0) {
			off += (int)n;
		} else {
			yield();
		}
	}
}

int storage_write(const char *path, const void *buf, int len)
{
	long fd = open(STORAGE_BUS_DEV, O_WRONLY);
	if (fd < 0) {
		return -1;
	}

	int path_len = 0;
	while (path[path_len]) {
		path_len++;
	}

	unsigned char hdr[8];
	hdr[0] = STORAGE_MAGIC0;
	hdr[1] = STORAGE_MAGIC1;
	hdr[2] = (unsigned char)(path_len & 0xFF);
	hdr[3] = (unsigned char)((path_len >> 8) & 0xFF);
	hdr[4] = (unsigned char)(len & 0xFF);
	hdr[5] = (unsigned char)((len >> 8) & 0xFF);
	hdr[6] = (unsigned char)((len >> 16) & 0xFF);
	hdr[7] = (unsigned char)((len >> 24) & 0xFF);

	bus_write_all(fd, (const char *)hdr, sizeof(hdr));
	bus_write_all(fd, path, path_len);
	bus_write_all(fd, (const char *)buf, len);
	close(fd);
	return len;
}
