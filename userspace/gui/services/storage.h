#ifndef __GUI_STORAGE_H
#define __GUI_STORAGE_H

/*
 * Storage primitives — the thin seam between the service data layer and
 * a persistent file. Services serialize their state into a flat buffer
 * and hand it here; the backend decides where bytes actually live.
 *
 * On hardware this is the FAT32 filesystem on the SD card (storage.c,
 * via open/read/writefile syscalls). In the GUI simulator there is no
 * kernel, so storage_sim.c is a no-op and services fall back to their
 * in-RAM mock — persistence is a hardware-only concern.
 */

/*
 * storage_read() - Load a file's contents into @buf.
 * Returns the number of bytes read, or -1 if the file does not exist
 * or cannot be read (callers treat -1 as "keep the in-RAM default").
 */
int storage_read(const char *path, void *buf, int max_len);

/*
 * storage_write() - Create/overwrite @path with @len bytes from @buf.
 * Returns bytes written, or -1 on error.
 */
int storage_write(const char *path, const void *buf, int len);

#endif
