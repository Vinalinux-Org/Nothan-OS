/* ============================================================
 * mmc.h
 * ------------------------------------------------------------
 * MMC/SD Card Driver — AM335x MMC0
 * ============================================================ */

#ifndef MMC_H
#define MMC_H

#include "types.h"

int mmc_init(void);
int mmc_read_sectors(uint32_t lba, uint32_t count, void *dst);
int mmc_write_sectors(uint32_t lba, uint32_t count, const void *src);

#endif /* MMC_H */
