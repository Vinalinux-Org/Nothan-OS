/* ============================================================
 * fat32.h
 * ------------------------------------------------------------
 * FAT32 Filesystem Driver
 * ============================================================ */

#ifndef FAT32_H
#define FAT32_H

#include "types.h"
#include "vfs.h"

/* Parses BPB, caches geometry, scans root directory. Must be called
 * after mmc_init() since it reads from the SD card. */
int fat32_init(uint32_t partition_lba);

struct vfs_operations *fat32_get_operations(void);

#endif /* FAT32_H */
