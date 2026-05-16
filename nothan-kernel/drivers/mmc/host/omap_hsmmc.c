/*
 * AM335x MMC0 Host Controller Driver
 *
 * Provides low-level access to the SD/MMC interface. It supports basic
 * block I/O operations including card initialization, 512-byte sector
 * reads (CMD17), and 512-byte sector writes (CMD24).
 */

#include "types.h"
#include "mmio.h"
#include "mmc.h"
#include "uart.h"
#include "syscalls.h"
#include "mach/prcm.h"
#include "mach/control.h"
#include "platform_device.h"
#include "nothan/mmc/host.h"
#include "nothan/errno.h"
#include "nothan/init.h"

/* MMC0 pin multiplexing configuration registers */
#define CONF_MMC0_DAT3          (CTRL_MODULE_BASE + 0x8F0)
#define CONF_MMC0_DAT2          (CTRL_MODULE_BASE + 0x8F4)
#define CONF_MMC0_DAT1          (CTRL_MODULE_BASE + 0x8F8)
#define CONF_MMC0_DAT0          (CTRL_MODULE_BASE + 0x8FC)
#define CONF_MMC0_CLK           (CTRL_MODULE_BASE + 0x900)
#define CONF_MMC0_CMD           (CTRL_MODULE_BASE + 0x904)

#define PIN_MODE_0              0

/* MMC0 controller register offsets */
#define MMC_SYSCONFIG           0x110
#define MMC_SYSSTATUS           0x114
#define MMC_CON                 0x12C
#define MMC_BLK                 0x204
#define MMC_ARG                 0x208
#define MMC_CMD                 0x20C
#define MMC_RSP10               0x210
#define MMC_DATA                0x220
#define MMC_HCTL                0x228
#define MMC_SYSCTL              0x22C
#define MMC_STAT                0x230
#define MMC_IE                  0x234
#define MMC_ISE                 0x238
#define MMC_CAPA                0x240

/* Set in probe() from platform_get_resource — used by all register accessors. */
static uint32_t mmc_base;

/* MMC_HCTL bits */
#define MMC_HCTL_SDBP           (1 << 8)         /* SD Bus Power */
#define MMC_HCTL_SDVS_3_3V      (0x7 << 9)       /* SD Bus Voltage 3.3V */

/* MMC_SYSCTL bits */
#define MMC_SYSCTL_ICE          (1 << 0)         /* Internal Clock Enable */
#define MMC_SYSCTL_ICS          (1 << 1)         /* Internal Clock Stable */
#define MMC_SYSCTL_CEN          (1 << 2)         /* Clock Enable (to card) */
#define MMC_SYSCTL_SRC          (1 << 25)        /* Soft Reset CMD line */
#define MMC_SYSCTL_SRD          (1 << 26)        /* Soft Reset DAT line */

/* MMC_STAT bits */
#define MMC_STAT_CC             (1 << 0)         /* Command Complete */
#define MMC_STAT_TC             (1 << 1)         /* Transfer Complete */
#define MMC_STAT_BWR            (1 << 4)         /* Buffer Write Ready */
#define MMC_STAT_BRR            (1 << 5)         /* Buffer Read Ready */
#define MMC_STAT_ERRI           (1 << 15)        /* Error Interrupt */

/* MMC_CMD flags */
#define MMC_CMD_RSP_NONE        (0 << 16)
#define MMC_CMD_RSP_136         (1 << 16)
#define MMC_CMD_RSP_48          (2 << 16)
#define MMC_CMD_RSP_48_BUSY     (3 << 16)
#define MMC_CMD_DDIR_READ       (1 << 4)
#define MMC_CMD_DDIR_WRITE      (0)
#define MMC_CMD_DP              (1 << 21)        /* Data Present */
#define MMC_CMD_CICE            (1 << 20)        /* Command Index Check Enable */
#define MMC_CMD_CCCE            (1 << 19)        /* Command CRC Check Enable */

/* Response type combinations */
#define MMC_RSP_NONE            (MMC_CMD_RSP_NONE)
#define MMC_RSP_R1              (MMC_CMD_RSP_48 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_RSP_R1b             (MMC_CMD_RSP_48_BUSY | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_RSP_R2              (MMC_CMD_RSP_136 | MMC_CMD_CCCE)
#define MMC_RSP_R3              (MMC_CMD_RSP_48)
#define MMC_RSP_R6              (MMC_CMD_RSP_48 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_RSP_R7              (MMC_CMD_RSP_48 | MMC_CMD_CCCE | MMC_CMD_CICE)

/*
 * Driver State
 */

static int sdhc_card = 0;    /* 1 = SDHC (block addressing), 0 = SDSC (byte addr) */

/* Busy-wait used during init, before timer driver is available. */
static void mmc_delay(volatile uint32_t count)
{
    while (count--) {
        __asm__ volatile("nop");
    }
}

/*
 * mmc_enable_clocks - Power up the MMC0 controller
 *
 * CRITICAL: The bootloader inherits the MMC0 clock state from the ROM.
 * The kernel must enable the PRCM explicitly, otherwise MMC register
 * reads will return all-ones and the initialization will hang.
 */
static int mmc_enable_clocks(void)
{
    uint32_t val;
    int timeout = 100000;

    val = mmio_read32(CM_PER_MMC0_CLKCTRL);
    val = (val & ~MODULEMODE_MASK) | MODULEMODE_ENABLE;
    mmio_write32(CM_PER_MMC0_CLKCTRL, val);

    while ((mmio_read32(CM_PER_MMC0_CLKCTRL) & IDLEST_MASK) != IDLEST_FUNCTIONAL) {
        if (--timeout == 0) {
            pr_err("[MMC] ERROR: PRCM clock enable timeout\n");
            return E_FAIL;
        }
    }

    return E_OK;
}

/*
 * mmc_send_cmd - Issue a command to the SD card
 * @cmd: The SD command index
 * @arg: The 32-bit command argument
 * @flags: Response and transfer direction flags
 *
 * Sends a command via the MMC controller and waits for completion.
 * If an error occurs, it resets the command line to prevent the controller
 * from deadlocking on subsequent commands.
 */
static int mmc_send_cmd(uint32_t cmd, uint32_t arg, uint32_t flags)
{
    uint32_t status;
    int timeout = 10000000;

    mmio_write32(mmc_base + MMC_STAT, 0xFFFFFFFF);
    mmio_write32(mmc_base + MMC_ARG, arg);
    mmio_write32(mmc_base + MMC_CMD, (cmd << 24) | flags);

    do {
        status = mmio_read32(mmc_base + MMC_STAT);
        if (--timeout == 0) {
            return E_FAIL;
        }
    } while ((status & (MMC_STAT_CC | MMC_STAT_ERRI)) == 0);

    if (status & MMC_STAT_ERRI) {
        /* SD spec requires CMD-line reset after error, otherwise
         * controller stays stuck and every subsequent command fails. */
        uint32_t sysctl = mmio_read32(mmc_base + MMC_SYSCTL);
        mmio_write32(mmc_base + MMC_SYSCTL, sysctl | MMC_SYSCTL_SRC);

        int src_timeout = 100000;
        while ((mmio_read32(mmc_base + MMC_SYSCTL) & MMC_SYSCTL_SRC) && (--src_timeout > 0));

        mmio_write32(mmc_base + MMC_STAT, 0xFFFFFFFF);
        return E_FAIL;
    }

    mmio_write32(mmc_base + MMC_STAT, MMC_STAT_CC);
    return E_OK;
}

/*
 * mmc_init - Initialize the host controller and probe the card
 *
 * This performs a full hardware reset, configures the pin multiplexing,
 * sets the initial 400kHz clock, and walks through the SD card
 * identification sequence (CMD8, ACMD41, CMD2, CMD3).
 * Finally, it increases the clock speed to 24MHz for data transfers.
 */
int mmc_init(void)
{
    uint32_t rsp, rca;
    int timeout;

    pr_info("[MMC] Initializing MMC0 controller...\n");

    if (mmc_enable_clocks() != E_OK) {
        return E_FAIL;
    }

    mmio_write32(CONF_MMC0_DAT3, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
    mmio_write32(CONF_MMC0_DAT2, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
    mmio_write32(CONF_MMC0_DAT1, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
    mmio_write32(CONF_MMC0_DAT0, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
    mmio_write32(CONF_MMC0_CLK,  PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
    mmio_write32(CONF_MMC0_CMD,  PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);

    mmio_write32(mmc_base + MMC_SYSCONFIG, 0x02);
    timeout = 1000000;
    while ((mmio_read32(mmc_base + MMC_SYSSTATUS) & 0x01) == 0) {
        if (--timeout == 0) {
            pr_err("[MMC] ERROR: SYSSTATUS reset timeout\n");
            return E_FAIL;
        }
    }

    /* Keep clocks ungated during init, otherwise power mgmt may cut
     * them mid-sequence and subsequent commands silently fail. */
    mmio_write32(mmc_base + MMC_SYSCONFIG, 0x00000308);

    mmio_write32(mmc_base + MMC_CON, 0x00000000);

    mmio_write32(mmc_base + MMC_SYSCTL, mmio_read32(mmc_base + MMC_SYSCTL) | MMC_SYSCTL_SRC | MMC_SYSCTL_SRD);
    timeout = 10000000;
    while (mmio_read32(mmc_base + MMC_SYSCTL) & (MMC_SYSCTL_SRC | MMC_SYSCTL_SRD)) {
        if (--timeout == 0) {
            pr_err("[MMC] ERROR: CMD/DAT reset timeout\n");
            return E_FAIL;
        }
    }

    mmio_write32(mmc_base + MMC_CAPA, mmio_read32(mmc_base + MMC_CAPA) | (1 << 24));

    /* SD spec: set voltage first, THEN assert SDBP. Reversed order
     * leaves the card in an undefined power state. */
    mmio_write32(mmc_base + MMC_HCTL, MMC_HCTL_SDVS_3_3V);
    mmio_write32(mmc_base + MMC_HCTL, mmio_read32(mmc_base + MMC_HCTL) | MMC_HCTL_SDBP);

    timeout = 100000;
    while ((mmio_read32(mmc_base + MMC_HCTL) & MMC_HCTL_SDBP) == 0) {
        if (--timeout == 0) {
            pr_err("[MMC] ERROR: SDBP timeout\n");
            return E_FAIL;
        }
    }

    /* 400kHz for the init phase per SD spec: 96MHz / CLKD=240 */
    mmio_write32(mmc_base + MMC_SYSCTL, 0x000E3C01);
    mmc_delay(1000);

    timeout = 1000000;
    while ((mmio_read32(mmc_base + MMC_SYSCTL) & MMC_SYSCTL_ICS) == 0) {
        if (--timeout == 0) {
            pr_err("[MMC] ERROR: ICS timeout\n");
            return E_FAIL;
        }
    }

    mmio_write32(mmc_base + MMC_SYSCTL, mmio_read32(mmc_base + MMC_SYSCTL) | MMC_SYSCTL_CEN);

    mmio_write32(mmc_base + MMC_IE,  0xFFFFFFFF);
    mmio_write32(mmc_base + MMC_ISE, 0xFFFFFFFF);
    mmio_write32(mmc_base + MMC_STAT, 0xFFFFFFFF);

    /* 80-clock wakeup: SD spec requires ≥74 clocks with CMD line high
     * before any command. Controller handles this via SD_CON[INIT]. */
    mmio_write32(mmc_base + MMC_CON, mmio_read32(mmc_base + MMC_CON) | (1 << 1));
    mmio_write32(mmc_base + MMC_CMD, 0x00000000);
    mmc_delay(10000);

    timeout = 1000000;
    while ((mmio_read32(mmc_base + MMC_STAT) & MMC_STAT_CC) == 0) {
        if (--timeout == 0) {
            pr_err("[MMC] ERROR: 80-clock init timeout\n");
            return E_FAIL;
        }
    }
    mmio_write32(mmc_base + MMC_STAT, MMC_STAT_CC);
    mmio_write32(mmc_base + MMC_CON, mmio_read32(mmc_base + MMC_CON) & ~(1 << 1));
    mmc_delay(5000);
    mmio_write32(mmc_base + MMC_STAT, 0xFFFFFFFF);

    /* Card identification sequence per SD spec */
    mmc_send_cmd(0, 0, MMC_RSP_NONE);
    mmc_delay(5000);

    /* CMD8 — SEND_IF_COND (check SDv2 voltage) */
    mmc_send_cmd(8, 0x1AA, MMC_RSP_R7);

    /* ACMD41 — SD_SEND_OP_COND loop */
    for (int i = 0; i < 2000; i++) {
        /* CMD55 = APP_CMD (prefix required before any ACMD) */
        if (mmc_send_cmd(55, 0x00000000, MMC_RSP_R1) != E_OK) {
            continue;
        }
        /* ACMD41 = SD_SEND_OP_COND; HCS bit + 3.3V voltage window */
        if (mmc_send_cmd(41, 0x40300000, MMC_RSP_R3) != E_OK) {
            continue;
        }

        rsp = mmio_read32(mmc_base + MMC_RSP10);
        if (rsp & (1U << 31)) {
            /* Card ready; bit 30 = HCS (SDHC/SDXC) */
            if (rsp & (1 << 30)) {
                sdhc_card = 1;
            }
            break;
        }
        mmc_delay(1000);
    }

    mmc_send_cmd(2, 0, MMC_RSP_R2);     /* CMD2 — ALL_SEND_CID */

    mmc_send_cmd(3, 0, MMC_RSP_R6);     /* CMD3 — SEND_RELATIVE_ADDR */
    rca = (mmio_read32(mmc_base + MMC_RSP10) >> 16) & 0xFFFF;

    mmc_send_cmd(9, rca << 16, MMC_RSP_R2);  /* CMD9 — SEND_CSD */
    mmc_send_cmd(7, rca << 16, MMC_RSP_R1b); /* CMD7 — SELECT_CARD */

    /* Widen data timeout to max (DTO field = 14) */
    {
        uint32_t sysctl = mmio_read32(mmc_base + MMC_SYSCTL);
        sysctl = (sysctl & ~(0xF << 16)) | (14 << 16);
        mmio_write32(mmc_base + MMC_SYSCTL, sysctl);
    }

    /* Bump card clock from 400kHz → 24MHz (CLKD = 4, since 96MHz/4 = 24MHz).
     * Sequence: disable CEN, change CLKD, wait ICS, re-enable CEN. */
    {
        uint32_t sysctl;

        sysctl = mmio_read32(mmc_base + MMC_SYSCTL);
        sysctl &= ~MMC_SYSCTL_CEN;
        mmio_write32(mmc_base + MMC_SYSCTL, sysctl);

        sysctl = mmio_read32(mmc_base + MMC_SYSCTL);
        sysctl &= ~(0xFFC0);
        sysctl |= (4 << 6);
        mmio_write32(mmc_base + MMC_SYSCTL, sysctl);

        timeout = 1000000;
        while ((mmio_read32(mmc_base + MMC_SYSCTL) & MMC_SYSCTL_ICS) == 0) {
            if (--timeout == 0) {
                pr_err("[MMC] ERROR: ICS timeout at 24MHz\n");
                return E_FAIL;
            }
        }

        mmio_write32(mmc_base + MMC_SYSCTL, mmio_read32(mmc_base + MMC_SYSCTL) | MMC_SYSCTL_CEN);
    }

    /* CMD16 — SET_BLOCKLEN = 512. Required for SDSC, no-op on SDHC. */
    mmc_send_cmd(16, 512, MMC_RSP_R1);

    pr_info("[MMC] Init complete — card type: %s\n",
                sdhc_card ? "SDHC (block addr)" : "SDSC (byte addr)");
    return E_OK;
}

/*
 * mmc_read_sectors - Read blocks from the SD card
 * @lba: Logical Block Address (sector number)
 * @count: Number of 512-byte sectors to read
 * @dst: Destination buffer
 *
 * Uses CMD17 to sequentially read blocks from the SD card into memory.
 */
int mmc_read_sectors(uint32_t lba, uint32_t count, void *dst)
{
    uint32_t *buf = (uint32_t *)dst;
    uint32_t i, j;
    int timeout;

    for (i = 0; i < count; i++) {
        mmio_write32(mmc_base + MMC_BLK, (1 << 16) | 512);
        mmio_write32(mmc_base + MMC_STAT, 0xFFFFFFFF);

        /* SDSC takes byte address, SDHC takes block number */
        uint32_t arg = sdhc_card ? (lba + i) : ((lba + i) * 512);
        mmio_write32(mmc_base + MMC_ARG, arg);
        mmio_write32(mmc_base + MMC_CMD, (17 << 24) | MMC_RSP_R1 | MMC_CMD_DP | MMC_CMD_DDIR_READ);

        timeout = 10000000;
        while ((mmio_read32(mmc_base + MMC_STAT) & MMC_STAT_BRR) == 0) {
            uint32_t stat = mmio_read32(mmc_base + MMC_STAT);
            if (stat & MMC_STAT_ERRI) {
                pr_err("[MMC] ERROR: read error at LBA %u\n", lba + i);
                return E_FAIL;
            }
            if (--timeout == 0) {
                pr_err("[MMC] ERROR: BRR timeout at LBA %u\n", lba + i);
                return E_FAIL;
            }
        }

        for (j = 0; j < 128; j++) {
            *buf++ = mmio_read32(mmc_base + MMC_DATA);
        }

        timeout = 10000000;
        while ((mmio_read32(mmc_base + MMC_STAT) & MMC_STAT_TC) == 0) {
            if (--timeout == 0) {
                pr_err("[MMC] ERROR: TC timeout at LBA %u\n", lba + i);
                return E_FAIL;
            }
        }

        mmio_write32(mmc_base + MMC_STAT, 0xFFFFFFFF);
    }

    return E_OK;
}

/*
 * mmc_write_sectors - Write blocks to the SD card
 * @lba: Logical Block Address (sector number)
 * @count: Number of 512-byte sectors to write
 * @src: Source buffer
 *
 * Uses CMD24 to sequentially write blocks from memory to the SD card.
 * Includes a mandatory controller reset sequence upon write failure.
 */
int mmc_write_sectors(uint32_t lba, uint32_t count, const void *src)
{
    const uint32_t *buf = (const uint32_t *)src;
    uint32_t i, j;
    int timeout;

    for (i = 0; i < count; i++) {
        mmio_write32(mmc_base + MMC_BLK, (1 << 16) | 512);
        mmio_write32(mmc_base + MMC_STAT, 0xFFFFFFFF);

        uint32_t arg = sdhc_card ? (lba + i) : ((lba + i) * 512);
        mmio_write32(mmc_base + MMC_ARG, arg);
        mmio_write32(mmc_base + MMC_CMD, (24 << 24) | MMC_RSP_R1 | MMC_CMD_DP | MMC_CMD_DDIR_WRITE);

        timeout = 10000000;
        while ((mmio_read32(mmc_base + MMC_STAT) & MMC_STAT_BWR) == 0) {
            uint32_t stat = mmio_read32(mmc_base + MMC_STAT);
            if (stat & MMC_STAT_ERRI) {
                pr_err("[MMC] ERROR: write error at LBA %u\n", lba + i);
                /* CMD+DAT reset required after write error — without it
                 * the controller stays wedged and all subsequent writes fail. */
                uint32_t sysctl = mmio_read32(mmc_base + MMC_SYSCTL);
                mmio_write32(mmc_base + MMC_SYSCTL, sysctl | MMC_SYSCTL_SRC | MMC_SYSCTL_SRD);
                int t = 100000;
                while ((mmio_read32(mmc_base + MMC_SYSCTL) & (MMC_SYSCTL_SRC | MMC_SYSCTL_SRD)) && --t > 0);
                mmio_write32(mmc_base + MMC_STAT, 0xFFFFFFFF);
                return E_FAIL;
            }
            if (--timeout == 0) {
                pr_err("[MMC] ERROR: BWR timeout at LBA %u\n", lba + i);
                return E_FAIL;
            }
        }

        for (j = 0; j < 128; j++) {
            mmio_write32(mmc_base + MMC_DATA, *buf++);
        }

        timeout = 10000000;
        while ((mmio_read32(mmc_base + MMC_STAT) & MMC_STAT_TC) == 0) {
            if (mmio_read32(mmc_base + MMC_STAT) & MMC_STAT_ERRI) {
                pr_err("[MMC] ERROR: TC error at LBA %u\n", lba + i);
                return E_FAIL;
            }
            if (--timeout == 0) {
                pr_err("[MMC] ERROR: write TC timeout at LBA %u\n", lba + i);
                return E_FAIL;
            }
        }

        mmio_write32(mmc_base + MMC_STAT, 0xFFFFFFFF);
    }

    return E_OK;
}

/*
 * Platform driver wiring
 * Registers this driver as an mmc_host, allowing the generic
 * mmc/block.c core to construct the gendisk device.
 */

static int omap_hsmmc_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!mem)
        return -ENODEV;

    mmc_base = mem->start;
    pr_info("[MMC] probing %s @ 0x%08x\n", pdev->name, mmc_base);

    int rc = mmc_init();
    if (rc != E_OK) return rc;

    struct mmc_host *host = mmc_alloc_host(0, "mmc0");
    if (!host) return E_FAIL;
    mmc_add_host(host);
    mmc_block_register(host, mmc_read_sectors, mmc_write_sectors, 0);
    return E_OK;
}

static struct platform_driver omap_hsmmc_driver = {
    .drv   = { .name = "omap-hsmmc" },
    .probe = omap_hsmmc_probe,
};

static int __init omap_hsmmc_driver_init(void)
{
    return platform_driver_register(&omap_hsmmc_driver);
}
fs_initcall(omap_hsmmc_driver_init);
