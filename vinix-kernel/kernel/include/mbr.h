/* ============================================================
 * mbr.h
 * ------------------------------------------------------------
 * MBR Partition Table Parser
 * ============================================================ */

#ifndef MBR_H
#define MBR_H

#include "types.h"

/* Locate the first FAT32 partition (type 0x0B or 0x0C) in sector 0's
 * partition table. size_out may be NULL if size is not needed. */
int mbr_find_fat32(uint32_t *lba_out, uint32_t *size_out);

#endif /* MBR_H */
