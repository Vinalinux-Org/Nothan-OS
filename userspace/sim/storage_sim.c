/*
 * sim/storage_sim.c - Storage backend stub for the GUI simulator
 *
 * The simulator runs on the host with no kernel, VFS, or SD card, so
 * persistence is meaningless here. storage_read() reports "no file" so
 * services keep their seeded in-RAM mock; storage_write() pretends to
 * succeed. Replaces gui/services/storage.c in the sim build.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "services/storage.h"

int storage_read(const char *path, void *buf, int max_len)
{
	(void)path;
	(void)buf;
	(void)max_len;
	return -1; /* no persistent store in the simulator */
}

int storage_write(const char *path, const void *buf, int len)
{
	(void)path;
	(void)buf;
	return len; /* pretend the write succeeded */
}
