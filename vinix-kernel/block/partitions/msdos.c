/* ============================================================
 * mbr.c
 * ------------------------------------------------------------
 * MBR partition table parser.
 * ============================================================ */

#include "types.h"
#include "mbr.h"
#include "mmc.h"
#include "uart.h"
#include "syscalls.h"

#define MBR_SIG_OFFSET          0x1FE
#define MBR_SIG_VALUE           0xAA55
#define MBR_PART_TABLE_OFFSET   0x1BE
#define MBR_PART_ENTRY_SIZE     16
#define MBR_NUM_PARTITIONS      4

#define PART_TYPE_FAT32_CHS     0x0B
#define PART_TYPE_FAT32_LBA     0x0C

static uint8_t mbr_sector_buf[512];

static uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

int mbr_find_fat32(uint32_t *lba_out, uint32_t *size_out)
{
    int i;

    if (lba_out == NULL) {
        return E_FAIL;
    }

    pr_info("[MBR] Reading sector 0...\n");

    if (mmc_read_sectors(0, 1, mbr_sector_buf) != E_OK) {
        pr_err("[MBR] ERROR: Failed to read MBR sector\n");
        return E_FAIL;
    }

    uint16_t sig = read_le16(&mbr_sector_buf[MBR_SIG_OFFSET]);
    if (sig != MBR_SIG_VALUE) {
        pr_err("[MBR] ERROR: Invalid signature 0x%x (expected 0xAA55)\n", sig);
        return E_FAIL;
    }

    for (i = 0; i < MBR_NUM_PARTITIONS; i++) {
        const uint8_t *entry = &mbr_sector_buf[MBR_PART_TABLE_OFFSET + i * MBR_PART_ENTRY_SIZE];
        uint8_t type = entry[0x04];

        if (type == PART_TYPE_FAT32_CHS || type == PART_TYPE_FAT32_LBA) {
            uint32_t lba  = read_le32(&entry[0x08]);
            uint32_t size = read_le32(&entry[0x0C]);

            pr_info("[MBR] Found FAT32 partition %d: LBA=%u, size=%u sectors\n",
                        i, lba, size);

            *lba_out = lba;
            if (size_out != NULL) {
                *size_out = size;
            }
            return E_OK;
        }
    }

    pr_err("[MBR] ERROR: No FAT32 partition found in table\n");
    return E_FAIL;
}
