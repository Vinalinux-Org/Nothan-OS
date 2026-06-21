/*
 * services/storage.c - Persistent storage backend (hardware)
 *
 * Maps the storage primitives onto the FAT32 filesystem through the
 * VFS syscalls. O_CREAT makes writes self-provisioning: the first save
 * after a fresh format creates the file. Each save rewrites the whole
 * blob from offset 0, so callers pass a fixed-size record.
 *
 * The GUI simulator builds storage_sim.c instead (no kernel there).
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "storage.h"
#include "../../lib/syscall.h"

int storage_read(const char *path, void *buf, int max_len)
{
	long fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;
	long n = read(fd, buf, (unsigned long)max_len);
	close(fd);
	return (int)n;
}

int storage_write(const char *path, const void *buf, int len)
{
	long fd = open(path, O_WRONLY | O_CREAT);
	if (fd < 0)
		return -1;
	long n = writefile(fd, buf, (unsigned long)len);
	close(fd);
	return (int)n;
}
