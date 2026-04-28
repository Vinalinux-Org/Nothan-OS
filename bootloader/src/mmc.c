/* ============================================================
 * mmc.c
 * ------------------------------------------------------------
 * AM335x MMC/SD raw-sector read driver.
 * ============================================================ */

#include "am335x.h"
#include "boot.h"

static int sdhc_card = 0;

/* MMC Status register bits */
#define MMC_STAT_CC         BIT(0)   /* Command complete */
#define MMC_STAT_TC         BIT(1)   /* Transfer complete */
#define MMC_STAT_ERRI       BIT(15)  /* Error interrupt */
#define MMC_STAT_BRR        BIT(5)   /* Buffer read ready */

/* MMC Command register flags */
#define MMC_CMD_RSP_NONE    (0 << 16)  /* No response */
#define MMC_CMD_RSP_136     (1 << 16)  /* 136-bit response */
#define MMC_CMD_RSP_48      (2 << 16)  /* 48-bit response */
#define MMC_CMD_RSP_48_BUSY (3 << 16)  /* 48-bit response with busy */

#define MMC_CMD_CCCE        BIT(19) /* Command CRC Check Enable */
#define MMC_CMD_CICE        BIT(20) /* Command Index Check Enable */
#define MMC_CMD_DP          BIT(21) /* Data Present */
#define MMC_CMD_DDIR_READ   BIT(4)  /* Data Direction: Read */
#define MMC_CMD_DDIR_WRITE  (0)     /* Data Direction: Write */

/* Response type combinations */
#define MMC_RSP_NONE        (MMC_CMD_RSP_NONE)
#define MMC_RSP_R1          (MMC_CMD_RSP_48 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_RSP_R1b         (MMC_CMD_RSP_48_BUSY | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_RSP_R2          (MMC_CMD_RSP_136 | MMC_CMD_CCCE)
#define MMC_RSP_R3          (MMC_CMD_RSP_48)
#define MMC_RSP_R6          (MMC_CMD_RSP_48 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_RSP_R7          (MMC_CMD_RSP_48 | MMC_CMD_CCCE | MMC_CMD_CICE)

static int mmc_send_cmd(uint32_t cmd, uint32_t arg, uint32_t flags)
{
    uint32_t status;

    writel(0xFFFFFFFF, MMC_STAT);
    writel(arg, MMC_ARG);
    writel((cmd << 24) | flags, MMC_CMD);

    int timeout = 10000000;
    do {
        status = readl(MMC_STAT);
        if (--timeout == 0) {
            return -1;
        }
    } while ((status & (MMC_STAT_CC | MMC_STAT_ERRI)) == 0);

    if (status & MMC_STAT_ERRI) {
        /* SD spec: must reset cmd line after error or controller
         * stays wedged in error state. */
        uint32_t sysctl = readl(MMC_SYSCTL);
        writel(sysctl | MMC_SYSCTL_SRC, MMC_SYSCTL);

        int srctimeout = 100000;
        while ((readl(MMC_SYSCTL) & MMC_SYSCTL_SRC) && (--srctimeout > 0));

        writel(0xFFFFFFFF, MMC_STAT);
        return -1;
    }

    writel(MMC_STAT_CC, MMC_STAT);
    return 0;
}

int mmc_init(void)
{
    uint32_t rsp;
    uart_putc('a');

    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_DAT3);
    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_DAT2);
    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_DAT1);
    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_DAT0);
    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_CLK);
    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_CMD);

    writel(0x02, MMC_SYSCONFIG);
    int timeout = 1000000;
    while ((readl(MMC_SYSSTATUS) & 0x01) == 0) {
        if (--timeout == 0) {
            uart_puts("MMC:    Timeout waiting for SYSSTATUS reset\r\n");
            return -1;
        }
    }

    /* Keep clocks live during init — prevents PM gating mid-sequence. */
    writel(0x00000308, MMC_SYSCONFIG);

    writel(0x00000000, MMC_CON);

    writel(readl(MMC_SYSCTL) | (1 << 25) | (1 << 26), MMC_SYSCTL);
    timeout = 10000000;
    while (readl(MMC_SYSCTL) & ((1 << 25) | (1 << 26))) {
        if (--timeout == 0) return -1;
    }

    writel(readl(MMC_CAPA) | (1 << 24), MMC_CAPA);

    /* SD spec: voltage BEFORE power. Reversed = undefined card state. */
    writel(MMC_HCTL_SDVS_3_3V, MMC_HCTL);
    writel(readl(MMC_HCTL) | MMC_HCTL_SDBP, MMC_HCTL);

    timeout = 100000;
    while ((readl(MMC_HCTL) & MMC_HCTL_SDBP) == 0) {
        if (--timeout == 0) {
            uart_puts("MMC:    Failed to enable bus power (SDBP)\r\n");
            return -1;
        }
    }

    /* 400 kHz init clock: ICE=1, CEN=0, CLKD=240 (96MHz/240), DTO=14. */
    writel(0x000E3C01, MMC_SYSCTL);
    delay(1000);

    timeout = 1000000;
    while ((readl(MMC_SYSCTL) & MMC_SYSCTL_ICS) == 0) {
        if (--timeout == 0) {
            uart_puts("MMC:    Timeout waiting for Internal Clock Stable\r\n");
            return -1;
        }
    }

    writel(readl(MMC_SYSCTL) | MMC_SYSCTL_CEN, MMC_SYSCTL);

    /* Polled mode still requires IE/ISE — controller raises STAT only
     * if interrupts are enabled in IE. */
    writel(0xFFFFFFFF, MMC_IE);
    writel(0xFFFFFFFF, MMC_ISE);
    writel(0xFFFFFFFF, MMC_STAT);

    /* SD spec: drive >=80 clocks before any command. */
    writel(readl(MMC_CON) | (1 << 1), MMC_CON);
    writel(0x00000000, MMC_CMD);
    delay(10000);

    timeout = 1000000;
    while ((readl(MMC_STAT) & BIT(0)) == 0) {
        if (--timeout == 0) {
            uart_puts("MMC:    Timeout waiting for 80-clock init\r\n");
            return -1;
        }
    }
    writel(BIT(0), MMC_STAT);
    writel(readl(MMC_CON) & ~(1 << 1), MMC_CON);
    delay(5000);
    writel(0xFFFFFFFF, MMC_STAT);

    mmc_send_cmd(0, 0, MMC_RSP_NONE);   /* CMD0 — reset to idle */
    delay(5000);

    mmc_send_cmd(8, 0x1AA, MMC_RSP_R7); /* CMD8 — SDv2 voltage probe */

    for (int i = 0; i < 2000; i++) {
        if (mmc_send_cmd(55, 0x00000000, MMC_RSP_R1) != 0) continue; /* CMD55 */
        if (mmc_send_cmd(41, 0x40300000, MMC_RSP_R3) != 0) continue; /* ACMD41 */

        if (readl(MMC_RSP10) & (1U << 31)) {
            if (readl(MMC_RSP10) & (1 << 30)) {
                sdhc_card = 1;
            }
            break;
        }
        delay(1000);
    }

    mmc_send_cmd(2, 0, MMC_RSP_R2);          /* CMD2 — get CID */
    mmc_send_cmd(3, 0, MMC_RSP_R6);          /* CMD3 — get RCA */
    rsp = readl(MMC_RSP10);
    uint32_t rca = (rsp >> 16) & 0xFFFF;

    mmc_send_cmd(9, rca << 16, MMC_RSP_R2);  /* CMD9 — get CSD */
    mmc_send_cmd(7, rca << 16, MMC_RSP_R1b); /* CMD7 — select card */

    uint32_t sysctl = readl(MMC_SYSCTL);
    sysctl = (sysctl & ~(0xF << 16)) | (14 << 16);
    writel(sysctl, MMC_SYSCTL);

    /* Switch to ~24 MHz (96MHz/CLKD=4). Required ordering:
     * disable CEN → reprogram CLKD → wait ICS → re-enable CEN. */
    {
        uint32_t sysctl;

        sysctl = readl(MMC_SYSCTL);
        sysctl &= ~(1 << 2);
        writel(sysctl, MMC_SYSCTL);

        sysctl = readl(MMC_SYSCTL);
        sysctl &= ~(0xFFC0);
        sysctl |= (4 << 6);
        writel(sysctl, MMC_SYSCTL);

        timeout = 1000000;
        while ((readl(MMC_SYSCTL) & 0x0002) == 0) {
            if (--timeout == 0) return -1;
        }

        writel(readl(MMC_SYSCTL) | (1 << 2), MMC_SYSCTL);
    }

    mmc_send_cmd(16, 512, MMC_RSP_R1); /* CMD16 — block length (SDSC) */

    return 0;
}

int mmc_read_sectors(uint32_t start_sector, uint32_t count, void *dest)
{
    uint32_t *buf = (uint32_t *)dest;
    uint32_t i, j;

    for (i = 0; i < count; i++) {
        writel((1 << 16) | 512, MMC_BLK);
        writel(0xFFFFFFFF, MMC_STAT);

        /* CMD17 arg: SDSC = byte addr (sector*512), SDHC = block#. */
        uint32_t arg = sdhc_card ? (start_sector + i) : ((start_sector + i) * 512);
        writel(arg, MMC_ARG);
        writel((17 << 24) | MMC_RSP_R1 | MMC_CMD_DP | MMC_CMD_DDIR_READ, MMC_CMD);

        int timeout = 10000000;
        while ((readl(MMC_STAT) & MMC_STAT_BRR) == 0) {
            uint32_t stat = readl(MMC_STAT);
            if (stat & MMC_STAT_ERRI) {
                panic("MMC Read ERR");
                return -1;
            }
            if (--timeout == 0) {
                panic("MMC Read BRR T/O");
                return -1;
            }
        }

        for (j = 0; j < 128; j++) {
            *buf++ = readl(MMC_DATA);
        }

        timeout = 10000000;
        while ((readl(MMC_STAT) & MMC_STAT_TC) == 0) {
            if (--timeout == 0) {
                panic("MMC Read TC T/O");
                return -1;
            }
        }

        writel(0xFFFFFFFF, MMC_STAT);
    }

    return 0;
}