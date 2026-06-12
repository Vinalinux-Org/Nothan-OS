/*
 * bootloader/src/ddr.c - DDR3 initialization for AM335x
 *
 * Target: MT41K256M16HA-125 (512 MB DDR3, 16-bit width)
 * DDR clock: 400 MHz (tCK = 2.5 ns), EMIF interface: 200 MHz
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "am335x.h"
#include "boot.h"

/*
 * Timing values for MT41K256M16HA-125 @ 400 MHz.
 * All cycle counts: ceil(t_ns / 2.5) - 1 where applicable.
 *
 * EMIF_TIM1 (offset 0x18):
 *   T_RP=5, T_RCD=5, T_WR=5, T_RAS=14, T_RC=20, T_RRD=3, T_WTR=3
 *
 * EMIF_TIM2 (offset 0x20):
 *   T_XP=2, T_XSNR=106, T_XSRD=512, T_RTP=3, T_CKE=2
 *
 * EMIF_TIM3 (offset 0x28):
 *   T_PDLL_UL=5, ZQ_ZQCS=64, T_RFC=103, T_RAS_MAX=max
 *
 * EMIF_REF_CTRL: 3120 DDR cycles = 7.8 µs (matches DDR3 tREFI)
 */
#define EMIF_TIM1		0x0AAAE51B
#define EMIF_TIM2		0x266B7FDA
#define EMIF_TIM3		0x501F867F
#define EMIF_REF_CTRL		0x00000C30

/*
 * SDRAM_CONFIG:
 *   DDR3, 16-bit bus (NARROW_MODE), CL=6, CWL=5,
 *   15-bit row, 8 banks, 10-bit column
 */
#define EMIF_CFG		0x61C05332

/*
 * DDR PHY control: READ_LATENCY=7 (CL6 + 1 pipeline),
 * PHY_ENABLE_DYNAMIC_PWRDN=1
 */
#define DDR_PHY_CTRL		0x00100007

#define DDR_IO_CTRL_VAL		0x0000018B

#define VTP_CTRL_ENABLE		(1 << 6)	/* CLRZ: 1 = start calibration */
#define VTP_CTRL_READY		(1 << 5)	/* set when calibration complete */

void ddr_init(void)
{
	uint32_t i;

	/* VTP calibrates pad impedance. Must reset (CLRZ=0) before enabling. */
	writel(readl(VTP_CTRL) & ~VTP_CTRL_ENABLE, VTP_CTRL);
	writel(readl(VTP_CTRL) | VTP_CTRL_ENABLE, VTP_CTRL);
	for (i = 0; i < 10000; i++) {
		if (readl(VTP_CTRL) & VTP_CTRL_READY)
			break;
	}

	writel(DDR_IO_CTRL_VAL, DDR_IO_CTRL);
	writel(DDR_IO_CTRL_VAL, DDR_DATA0_IOCTRL);
	writel(DDR_IO_CTRL_VAL, DDR_DATA1_IOCTRL);
	writel(0x1, DDR_CKE_CTRL);

	writel(0x80, DDR_CMD0_SLAVE_RATIO_0);
	writel(0x80, DDR_CMD1_SLAVE_RATIO_0);
	writel(0x80, DDR_CMD2_SLAVE_RATIO_0);
	writel(0x00, DDR_CMD0_INVERT_CLKOUT_0);
	writel(0x00, DDR_CMD1_INVERT_CLKOUT_0);
	writel(0x00, DDR_CMD2_INVERT_CLKOUT_0);

	writel(0x38, DDR_DATA0_RD_DQS_SLAVE_RATIO_0);
	writel(0x44, DDR_DATA0_WR_DQS_SLAVE_RATIO_0);
	writel(0x94, DDR_DATA0_FIFO_WE_SLAVE_RATIO_0);
	writel(0x7D, DDR_DATA0_WR_DATA_SLAVE_RATIO_0);
	writel(0x00, DDR_DATA0_GATE_LEVEL_INIT_RATIO_0);

	writel(0x38, DDR_DATA1_RD_DQS_SLAVE_RATIO_0);
	writel(0x44, DDR_DATA1_WR_DQS_SLAVE_RATIO_0);
	writel(0x94, DDR_DATA1_FIFO_WE_SLAVE_RATIO_0);
	writel(0x7D, DDR_DATA1_WR_DATA_SLAVE_RATIO_0);
	writel(0x00, DDR_DATA1_GATE_LEVEL_INIT_RATIO_0);

	writel(DDR_PHY_CTRL, EMIF_DDR_PHY_CTRL_1);
	writel(DDR_PHY_CTRL, EMIF_DDR_PHY_CTRL_2);
	writel(EMIF_TIM1, EMIF_SDRAM_TIM_1);
	writel(EMIF_TIM2, EMIF_SDRAM_TIM_2);
	writel(EMIF_TIM3, EMIF_SDRAM_TIM_3);
	writel(EMIF_REF_CTRL, EMIF_SDRAM_REF_CTRL);
	writel(EMIF_CFG, EMIF_SDRAM_CONFIG);

	/* Allow time for ZQ calibration and mode register programming. */
	for (i = 0; i < 5000; i++)
		;

	writel(EMIF_REF_CTRL, EMIF_SDRAM_REF_CTRL);
}

int ddr_test(void)
{
	volatile uint32_t *ddr = (volatile uint32_t *)DDR_BASE;
	uint32_t pattern1 = 0xAA55AA55;
	uint32_t pattern2 = 0x55AA55AA;
	int i;

	for (i = 0; i < 1024; i++)
		ddr[i] = pattern1;
	for (i = 0; i < 1024; i++) {
		if (ddr[i] != pattern1) {
			uart_puts("\r\nFAIL P1 @ ");
			uart_print_hex((uint32_t)&ddr[i]);
			return -1;
		}
	}

	for (i = 0; i < 1024; i++)
		ddr[i] = pattern2;
	for (i = 0; i < 1024; i++) {
		if (ddr[i] != pattern2) {
			uart_puts("\r\nFAIL P2 @ ");
			uart_print_hex((uint32_t)&ddr[i]);
			return -1;
		}
	}

	return 0;
}
