/*
 * drivers/mmc/omap-hsmmc.c - OMAP High Speed MMC Host Controller Driver
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/platform.h>
#include <nothan/printk.h>
#include <nothan/mmio.h>
#include <nothan/block.h>
#include <nothan/init.h>

/* Register offsets */
#define MMCHS_SYSCONFIG  0x010
#define MMCHS_SYSSTATUS  0x014
#define MMCHS_CON        0x02C
#define MMCHS_BLK        0x104
#define MMCHS_ARG        0x108
#define MMCHS_CMD        0x10C
#define MMCHS_RSP10      0x110
#define MMCHS_DATA       0x120
#define MMCHS_PSTATE     0x124
#define MMCHS_HCTL       0x128
#define MMCHS_SYSCTL     0x12C
#define MMCHS_STAT       0x130
#define MMCHS_IE         0x134
#define MMCHS_CAPA       0x240

/* STAT bits */
#define STAT_CC          (1 << 0)
#define STAT_TC          (1 << 1)
#define STAT_BWR         (1 << 4)
#define STAT_BRR         (1 << 5)
#define STAT_ERRI        (1 << 15)

/* SYSCTL bits */
#define SYSCTL_ICE       (1 << 0)
#define SYSCTL_ICS       (1 << 1)
#define SYSCTL_CEN       (1 << 2)
#define SYSCTL_SRC       (1 << 25)
#define SYSCTL_SRD       (1 << 26)

/* CON bits */
#define CON_INIT         (1 << 1)

/* PSTATE bits */
#define PSTATE_CMDFREE   (1 << 0)

/* HCTL bits */
#define HCTL_SDBP        (1 << 8)
#define HCTL_SDVS_3_3V   (7 << 9)

/* CAPA bits */
#define CAPA_VS30        (1 << 24)

/* PRCM: CM_PER_MMC0_CLKCTRL at L4_WKUP VA 0xF0E00000 + offset 0x3C */
#define CM_PER_MMC0_CLKCTRL    0xF0E0003C
#define CM_PER_L3PER_CLKSTCTRL 0xF0E00014
#define MODULEMODE_ENABLE      2
#define IDLEST_MASK            (3 << 16)
#define IDLEST_FUNCTIONAL      0
#define CLKTRCTRL_SW_WKUP     2

/* Pinmux: MMC0 pins via CONTROL_MODULE (VA: PA 0x44E10000 → 0xF0E10000) */
#define CONF_MMC0_DAT3   (0xF0E10000 + 0x8F0)
#define CONF_MMC0_DAT2   (0xF0E10000 + 0x8F4)
#define CONF_MMC0_DAT1   (0xF0E10000 + 0x8F8)
#define CONF_MMC0_DAT0   (0xF0E10000 + 0x8FC)
#define CONF_MMC0_CLK    (0xF0E10000 + 0x900)
#define CONF_MMC0_CMD    (0xF0E10000 + 0x904)
#define PIN_MODE_0        0
#define PIN_INPUT_EN      (1 << 5)
#define PIN_PULLUP_EN     (1 << 3)

static uint32_t mmc_base;
static uint32_t card_rca;

static inline void mmc_write(uint32_t reg, uint32_t val)
{
	mmio_write32(mmc_base + reg, val);
}

static inline uint32_t mmc_read(uint32_t reg)
{
	return mmio_read32(mmc_base + reg);
}

/*
 * mmc_reset_cmd_line - Soft reset the MMC command line
 */
static void mmc_reset_cmd_line(void)
{
	uint32_t sysctl = mmc_read(MMCHS_SYSCTL);
	mmc_write(MMCHS_SYSCTL, sysctl | SYSCTL_SRC);
	int timeout = 100000;
	while ((mmc_read(MMCHS_SYSCTL) & SYSCTL_SRC) && --timeout)
		;
	mmc_write(MMCHS_STAT, 0xFFFFFFFF);
}

/*
 * mmc_enable_clocks - Enable PRCM clocks for MMC0
 */
static int mmc_enable_clocks(void)
{
	int timeout;
	uint32_t val;

	/* Step 1: Wake L3_PER clock domain */
	val = mmio_read32(CM_PER_L3PER_CLKSTCTRL);
	if ((val & 0x3) != CLKTRCTRL_SW_WKUP) {
		mmio_write32(CM_PER_L3PER_CLKSTCTRL, CLKTRCTRL_SW_WKUP);
		timeout = 10000;
		while (((mmio_read32(CM_PER_L3PER_CLKSTCTRL) & 0x3) != CLKTRCTRL_SW_WKUP) && --timeout)
			;
	}

	/* Step 2: Enable MMC0 module clock */
	val = mmio_read32(CM_PER_MMC0_CLKCTRL);
	val = (val & ~3) | MODULEMODE_ENABLE;
	mmio_write32(CM_PER_MMC0_CLKCTRL, val);

	timeout = 100000;
	while ((mmio_read32(CM_PER_MMC0_CLKCTRL) & IDLEST_MASK) !=
	       (IDLEST_FUNCTIONAL << 16)) {
		if (--timeout == 0) {
			printk("[MMC] PRCM clock enable timeout\n");
			return -1;
		}
	}
	return 0;
}

/*
 * mmc_config_pins - Configure pinmux for MMC0
 */
static void mmc_config_pins(void)
{
	mmio_write32(CONF_MMC0_DAT3, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
	mmio_write32(CONF_MMC0_DAT2, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
	mmio_write32(CONF_MMC0_DAT1, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
	mmio_write32(CONF_MMC0_DAT0, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
	mmio_write32(CONF_MMC0_CLK,  PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
	mmio_write32(CONF_MMC0_CMD,  PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
}

static int mmc_send_cmd(uint32_t cmd, uint32_t arg)
{
	int timeout;

	mmc_write(MMCHS_STAT, 0xFFFFFFFF);

	/* Wait for command line free */
	timeout = 100000;
	while (mmc_read(MMCHS_PSTATE) & PSTATE_CMDFREE) {
		if (--timeout == 0) {
			printk("[MMC] CMD line free timeout\n");
			return -1;
		}
	}

	mmc_write(MMCHS_ARG, arg);
	mmc_write(MMCHS_CMD, cmd);

	/* Wait for command complete or error */
	timeout = 10000000;
	while (1) {
		uint32_t stat = mmc_read(MMCHS_STAT);
		if (stat & STAT_ERRI) {
			mmc_reset_cmd_line();
			printk("[MMC] Command error, STAT=0x%x\n", (unsigned int)stat);
			return -1;
		}
		if (stat & STAT_CC)
			break;
		if (--timeout == 0) {
			mmc_reset_cmd_line();
			printk("[MMC] CMD timeout\n");
			return -1;
		}
	}
	mmc_write(MMCHS_STAT, STAT_CC);
	return 0;
}

static int omap_hsmmc_read_block(struct block_device *bdev, uint32_t block, void *buf)
{
	int timeout;

	mmc_write(MMCHS_BLK, 512 | (1 << 16));

	uint32_t cmd17 = (17 << 24) | (1 << 21) | (2 << 16);
	if (mmc_send_cmd(cmd17, block) != 0) {
		printk("[MMC] Read block %u failed\n", (unsigned int)block);
		return -1;
	}

	/* Wait for Buffer Read Ready */
	timeout = 10000000;
	while (!(mmc_read(MMCHS_STAT) & STAT_BRR)) {
		if (mmc_read(MMCHS_STAT) & STAT_ERRI) {
			printk("[MMC] BRR error at block %u\n", (unsigned int)block);
			return -1;
		}
		if (--timeout == 0) {
			printk("[MMC] BRR timeout at block %u\n", (unsigned int)block);
			return -1;
		}
	}
	mmc_write(MMCHS_STAT, STAT_BRR);

	/* Read 512 bytes (128 words) */
	uint32_t *p = (uint32_t *)buf;
	for (int i = 0; i < 128; i++)
		p[i] = mmc_read(MMCHS_DATA);

	/* Wait for Transfer Complete */
	timeout = 10000000;
	while (!(mmc_read(MMCHS_STAT) & STAT_TC)) {
		if (mmc_read(MMCHS_STAT) & STAT_ERRI) {
			printk("[MMC] TC error at block %u\n", (unsigned int)block);
			return -1;
		}
		if (--timeout == 0) {
			printk("[MMC] TC timeout at block %u\n", (unsigned int)block);
			return -1;
		}
	}
	mmc_write(MMCHS_STAT, STAT_TC);

	return 0;
}

static struct block_device_operations omap_hsmmc_ops = {
	.read_block = omap_hsmmc_read_block,
	.write_block = NULL,
};

static struct block_device omap_hsmmc_bdev = {
	.name = "sd0",
	.ops = &omap_hsmmc_ops,
	.private_data = NULL,
	.block_size = 512,
	.total_blocks = 7744512, /* approx 4GB */
};

static int omap_hsmmc_probe(struct platform_device *pdev)
{
	int timeout;

	mmc_base = phys_to_mmio(pdev->base);
	printk("[MMC] Probing OMAP HSMMC at 0x%x\n", (unsigned int)mmc_base);

	/* Enable PRCM clock for MMC0 */
	if (mmc_enable_clocks() != 0)
		return -1;

	/* Configure MMC0 pins */
	mmc_config_pins();

	/* Soft reset */
	mmc_write(MMCHS_SYSCONFIG, 0x2);
	timeout = 1000000;
	while (!(mmc_read(MMCHS_SYSSTATUS) & 0x1)) {
		if (--timeout == 0) {
			printk("[MMC] Soft reset timeout\n");
			return -1;
		}
	}

	/* Set CAPA voltage support bit (VS30 = 3.0V) */
	mmc_write(MMCHS_CAPA, mmc_read(MMCHS_CAPA) | CAPA_VS30);

	/* Set voltage 3.0V + power on */
	mmc_write(MMCHS_HCTL, HCTL_SDVS_3_3V);
	mmc_write(MMCHS_HCTL, mmc_read(MMCHS_HCTL) | HCTL_SDBP);
	timeout = 100000;
	while (!(mmc_read(MMCHS_HCTL) & HCTL_SDBP)) {
		if (--timeout == 0) {
			printk("[MMC] SDBP timeout\n");
			return -1;
		}
	}

	/* Enable internal clock, wait for stable */
	mmc_write(MMCHS_SYSCTL, SYSCTL_ICE);
	timeout = 1000000;
	while (!(mmc_read(MMCHS_SYSCTL) & SYSCTL_ICS)) {
		if (--timeout == 0) {
			printk("[MMC] ICS timeout\n");
			return -1;
		}
	}

	/* Enable clock to card */
	mmc_write(MMCHS_SYSCTL, mmc_read(MMCHS_SYSCTL) | SYSCTL_CEN);

	/* Send INIT stream (80 clocks) */
	mmc_write(MMCHS_CON, CON_INIT);
	mmc_write(MMCHS_CMD, 0);
	timeout = 1000000;
	while (!(mmc_read(MMCHS_STAT) & STAT_CC)) {
		if (--timeout == 0) {
			printk("[MMC] INIT stream timeout\n");
			return -1;
		}
	}
	mmc_write(MMCHS_STAT, STAT_CC);
	mmc_write(MMCHS_CON, mmc_read(MMCHS_CON) & ~CON_INIT);

	/* Card initialization sequence */
	if (mmc_send_cmd((0 << 24), 0) != 0)
		return -1; /* CMD0 */
	if (mmc_send_cmd((8 << 24) | (2 << 16), 0x1AA) != 0) {   /* CMD8 */
		printk("[MMC] Card does not support CMD8 (not SDv2?)\n");
		return -1;
	}

	/* ACMD41 loop */
	timeout = 2000000;
	while (1) {
		if (mmc_send_cmd((55 << 24) | (2 << 16), 0) != 0)   /* CMD55 */
			continue;
		if (mmc_send_cmd((41 << 24) | (2 << 16), 0x40300000) != 0) /* ACMD41 */
			continue;
		if (mmc_read(MMCHS_RSP10) & (1 << 31))
			break; /* Card ready */
		if (--timeout == 0) {
			printk("[MMC] ACMD41 timeout (no card?)\n");
			return -1;
		}
	}

	if (mmc_send_cmd((2 << 24) | (1 << 16), 0) != 0)
		return -1; /* CMD2 */
	if (mmc_send_cmd((3 << 24) | (2 << 16), 0) != 0)
		return -1; /* CMD3 */
	card_rca = mmc_read(MMCHS_RSP10) >> 16;
	printk("[MMC] Card detected, RCA=0x%04x\n", (unsigned int)card_rca);

	if (mmc_send_cmd((7 << 24) | (2 << 16), card_rca << 16) != 0)
		return -1; /* CMD7 */
	if (mmc_send_cmd((16 << 24) | (2 << 16), 512) != 0)
		return -1; /* CMD16 */

	register_block_device(&omap_hsmmc_bdev);
	printk("[MMC] Registered block device 'sd0'\n");
	return 0;
}

static struct platform_driver omap_hsmmc_driver = {
	.drv = {
		.name = "omap_mmc",
	},
	.probe = omap_hsmmc_probe,
};

static int __init omap_hsmmc_init(void)
{
	return platform_driver_register(&omap_hsmmc_driver);
}
device_initcall(omap_hsmmc_init);
