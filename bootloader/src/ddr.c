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

/* vtp_ctrl register fields (Control Module offset 0xE0C, TRM Table 9-63) */
#define VTP_CTRL_ENABLE		(1 << 6)	/* enable: 1 = dynamic VTP compensation */
#define VTP_CTRL_READY		(1 << 5)	/* ready: 1 = training complete */
#define VTP_CTRL_CLRZ		(1 << 0)	/* clrz: reset FSM on low pulse, start on high */

/* SDRAM_REF_CTRL[31]: disable init and refresh while programming EMIF */
#define EMIF_INITREF_DIS	(1u << 31)

/*
 * ZQ calibration for MT41K256M16HA-125 (from u-boot ddr_defs.h):
 *   CS0EN=1, SFEXITEN=1, periodic interval and ZQ timing per DDR3 spec.
 */
#define EMIF_ZQ_CFG		0x50074BE4

void ddr_init(void)
{
	uint32_t i;

	/*
	 * VTP calibrates DDR pad impedance. Sequence (TRM Table 9-63):
	 *   1. Set enable (bit 6) → dynamic compensation mode
	 *   2. Clear clrz (bit 0) → reset FSM
	 *   3. Set clrz (bit 0) → start calibration
	 *   4. Poll ready (bit 5)
	 * enable and clrz are separate bits; toggling only enable (old code)
	 * leaves clrz=0 so the FSM never starts — works on cold boot because
	 * ROM left VTP in a good state, fails on warm reset.
	 */
	writel(readl(VTP_CTRL) | VTP_CTRL_ENABLE, VTP_CTRL);
	writel(readl(VTP_CTRL) & ~VTP_CTRL_CLRZ, VTP_CTRL);
	writel(readl(VTP_CTRL) | VTP_CTRL_CLRZ, VTP_CTRL);
	for (i = 0; i < 10000; i++) {
		if (readl(VTP_CTRL) & VTP_CTRL_READY)
			break;
	}

	/* IO control for command and data macros (all pads same value). */
	writel(DDR_IO_CTRL_VAL, DDR_IO_CTRL);
	writel(DDR_IO_CTRL_VAL, DDR_CMD0_IOCTRL);
	writel(DDR_IO_CTRL_VAL, DDR_CMD1_IOCTRL);
	writel(DDR_IO_CTRL_VAL, DDR_CMD2_IOCTRL);
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

	/*
	 * Disable init+refresh before programming PHY/timing registers.
	 * Without INITREF_DIS, a warm reset leaves EMIF partially configured
	 * and stray refreshes race with register writes.
	 *
	 * Then explicitly clear SDRAM_CONFIG. On warm reset the EMIF retains
	 * the previous value (0x61C05332). Writing the same value again may not
	 * re-trigger the DDR3 init sequence; clearing to 0 first guarantees the
	 * subsequent write is a state transition that forces re-initialization.
	 */
	writel(EMIF_INITREF_DIS | EMIF_REF_CTRL, EMIF_SDRAM_REF_CTRL);
	writel(0, EMIF_SDRAM_CONFIG);

	writel(DDR_PHY_CTRL, EMIF_DDR_PHY_CTRL_1);
	writel(DDR_PHY_CTRL, EMIF_DDR_PHY_CTRL_2);
	writel(EMIF_TIM1, EMIF_SDRAM_TIM_1);
	writel(EMIF_TIM2, EMIF_SDRAM_TIM_2);
	writel(EMIF_TIM3, EMIF_SDRAM_TIM_3);

	writel(EMIF_ZQ_CFG, EMIF_ZQ_CONFIG);

	/*
	 * Write secure Control Status mirror first, then enable refresh, then
	 * write SDRAM_CONFIG. DDR3 power-on init (DDR_RESET_N assertion, CKE
	 * sequence, mode registers) is triggered by the SDRAM_CONFIG write and
	 * requires INITREF_DIS=0 at that moment — writing SDRAM_CONFIG while
	 * INITREF_DIS=1 prevents the init sequence on cold boot.
	 */
	writel(EMIF_CFG, SECURE_EMIF_SDRAM_CONFIG);
	writel(EMIF_REF_CTRL, EMIF_SDRAM_REF_CTRL);
	writel(EMIF_CFG, EMIF_SDRAM_CONFIG);

	for (i = 0; i < 500000; i++)
		;

	writel(EMIF_REF_CTRL, EMIF_SDRAM_REF_CTRL);
	writel(EMIF_REF_CTRL, EMIF_SDRAM_REF_CTRL_SHDW);
}

int ddr_test(int silent)
{
	volatile uint32_t *ddr = (volatile uint32_t *)DDR_BASE;
	uint32_t pattern1 = 0xAA55AA55;
	uint32_t pattern2 = 0x55AA55AA;
	int i;

	for (i = 0; i < 1024; i++)
		ddr[i] = pattern1;
	for (i = 0; i < 1024; i++) {
		if (ddr[i] != pattern1) {
			if (!silent) {
				uart_puts("FAIL P1 @ ");
				uart_print_hex((uint32_t)&ddr[i]);
				uart_puts(" got ");
				uart_print_hex(ddr[i]);
				uart_puts("\r\n");
			}
			return -1;
		}
	}

	for (i = 0; i < 1024; i++)
		ddr[i] = pattern2;
	for (i = 0; i < 1024; i++) {
		if (ddr[i] != pattern2) {
			if (!silent) {
				uart_puts("FAIL P2 @ ");
				uart_print_hex((uint32_t)&ddr[i]);
				uart_puts("\r\n");
			}
			return -1;
		}
	}

	return 0;
}
