/*
 * drivers/net/ethernet/ti/cpsw.c - AM335x CPSW Ethernet, bring-up
 *
 * Single port: CPSW_3G port 1 driving a LAN8710A over MII, which is the only
 * configuration this board has — am335x-bone-common.dtsi says slaves = <1>,
 * phy-mode = "mii", phy address 0, and nothing else is wired.
 *
 * This file is the first step only: clocks, pads, reset, and two registers
 * read back whose correct values are known before the board is powered on.
 * No traffic yet.  The order matters — roadmap §0 asks for phases short enough
 * to be flashed and looked at, and a driver that brings up seven blocks at
 * once and then fails tells you nothing about which of the seven.
 *
 * Two known answers, so a wrong result is a wrong result rather than a thing
 * to interpret:
 *
 *   ALE IDVER   0x00290104   TRM §14.5.1.1, reset value
 *   PHY ID      0x0007 C0Fx  LAN8710A; the low nibble is a silicon revision
 *
 * Reading a register that has a documented reset value is the cheapest test
 * there is: it proves the clock is on, the pads are muxed, the reset released,
 * and the address correct, in one line.  §2.2 of the roadmap used the same
 * trick for the MPU frequency and it caught a real mistake immediately.
 *
 * Derived from the driver in nothan_os_old, which reached working traffic on
 * this same board — so the hardware questions it already answered (pad list,
 * reset order, sub-block offsets, MDIO divider) are taken as given rather than
 * re-derived.  Everything about the *software* shape is not: the old receive
 * path allocated memory and copied a frame inside the interrupt handler, which
 * §9.2 rules out and which the UART storm measured earlier this week showed
 * the price of.  That part is rebuilt around nothan/ring.h when RX arrives.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/config.h>
#include <nothan/types.h>
#include <nothan/mmio.h>
#include <nothan/platform.h>
#include <nothan/printk.h>
#include <nothan/init.h>
#include <nothan/delay.h>

/* ---- PRCM ---------------------------------------------------------- */
#define CM_PER_BASE		0x44E00000
#define CM_PER_CPGMAC0_CLKCTRL	(CM_PER_BASE + 0x14)	/* TRM Ch08 */
#define MODULEMODE_ENABLE	0x2
#define IDLEST_MASK		(0x3 << 16)
#define IDLEST_FUNC		(0x0 << 16)

/* ---- Control module: MII1 and MDIO pads ---------------------------- */
#define CTRL_BASE		0x44E10000

/*
 * TRM Ch09 §9.3.1.50: RXACTIVE at bit 5, PUDEN at bit 3, mux mode in [2:0].
 * Mode 0 is MII1 for all of these pads.  Receive pads and both clocks need
 * the input buffer on; transmit pads do not.  TX_CLK is an input in MII —
 * the PHY drives it — which is the one entry in this list that looks wrong
 * and is not.
 */
#define PAD_IN			0x28	/* RXACTIVE | PUDEN | mode 0 */
#define PAD_OUT			0x08	/* PUDEN | mode 0 */

#define CONF_MII1_COL		(CTRL_BASE + 0x908)
#define CONF_MII1_CRS		(CTRL_BASE + 0x90C)
#define CONF_MII1_RX_ER		(CTRL_BASE + 0x910)
#define CONF_MII1_TX_EN		(CTRL_BASE + 0x914)
#define CONF_MII1_RX_DV		(CTRL_BASE + 0x918)
#define CONF_MII1_TXD3		(CTRL_BASE + 0x91C)
#define CONF_MII1_TXD2		(CTRL_BASE + 0x920)
#define CONF_MII1_TXD1		(CTRL_BASE + 0x924)
#define CONF_MII1_TXD0		(CTRL_BASE + 0x928)
#define CONF_MII1_TX_CLK	(CTRL_BASE + 0x92C)
#define CONF_MII1_RX_CLK	(CTRL_BASE + 0x930)
#define CONF_MII1_RXD3		(CTRL_BASE + 0x934)
#define CONF_MII1_RXD2		(CTRL_BASE + 0x938)
#define CONF_MII1_RXD1		(CTRL_BASE + 0x93C)
#define CONF_MII1_RXD0		(CTRL_BASE + 0x940)
#define CONF_MDIO_DATA		(CTRL_BASE + 0x948)
#define CONF_MDIO_CLK		(CTRL_BASE + 0x94C)

/* ---- CPSW sub-blocks, offsets from the subsystem base --------------- */
#define CPSW_SS			0x0000
#define CPSW_PORT		0x0100
#define CPSW_CPDMA		0x0800
#define CPSW_STATERAM		0x0A00
#define CPSW_ALE		0x0D00
#define CPSW_SL1		0x0D80
#define CPSW_MDIO		0x1000
#define CPSW_WR			0x1200

#define SS_SOFT_RESET		0x08
#define SL_SOFT_RESET		0x0C
#define CPDMA_SOFT_RESET	0x1C
#define WR_SOFT_RESET		0x04

#define ALE_IDVER		0x00
#define ALE_IDVER_RESET		0x00290104u	/* TRM §14.5.1.1 */

/* ---- MDIO ---------------------------------------------------------- */
#define MDIO_CONTROL		0x04
#define MDIO_ALIVE		0x08
#define MDIO_USERACCESS0	0x80

#define MDIO_CTRL_ENABLE	(1u << 30)
#define MDIO_UA_GO		(1u << 31)
#define MDIO_UA_ACK		(1u << 29)
#define MDIO_UA_DATA		0xFFFFu

/*
 * CPSW runs at 125 MHz and MDC must stay under 2.5 MHz, so 125/56 = 2.23 MHz.
 * The field holds divider-minus-one, which is the sort of off-by-one that
 * produces a bus that almost works.
 */
#define MDIO_CLKDIV		55

#define PHY_ADDR		0	/* am335x-bone-common.dtsi */
#define PHY_REG_ID1		2
#define PHY_REG_ID2		3
#define LAN8710A_ID1		0x0007u
#define LAN8710A_ID2		0xC0F0u	/* low nibble is silicon revision */
#define LAN8710A_ID2_MASK	0xFFF0u

#define CPSW_TIMEOUT		100000

static u32 cpsw_va;		/* subsystem base, already translated */

static inline u32 rd(u32 off)		{ return mmio_read32(cpsw_va + off); }
static inline void wr(u32 off, u32 v)	{ mmio_write32(cpsw_va + off, v); }

/*
 * Every block here resets by writing 1 and waiting for the hardware to clear
 * it.  Bounded, because a block whose clock never arrived would otherwise hang
 * the boot it is supposed to be reporting on — the same rule the UART probe
 * learned.
 */
static int cpsw_soft_reset(const char *what, u32 off)
{
	int timeout = CPSW_TIMEOUT;

	wr(off, 1);
	while ((rd(off) & 1) && --timeout)
		;

	if (!timeout) {
		printk("[CPSW] %s reset did not complete\n", what);
		return -1;
	}
	return 0;
}

static int cpsw_clock_enable(void)
{
	u32 va = phys_to_mmio(CM_PER_CPGMAC0_CLKCTRL);
	int timeout = CPSW_TIMEOUT;
	u32 v;

	v = mmio_read32(va);
	mmio_write32(va, (v & ~0x3u) | MODULEMODE_ENABLE);

	while (--timeout) {
		v = mmio_read32(va);
		if ((v & IDLEST_MASK) == IDLEST_FUNC && (v & 0x3) == MODULEMODE_ENABLE)
			return 0;
	}

	printk("[CPSW] module clock never became functional (CLKCTRL=0x%08lx)\n",
	       (unsigned long)v);
	return -1;
}

static void cpsw_pinmux(void)
{
	static const struct { u32 reg; u32 val; } pads[] = {
		/* Receive side and both clocks: input buffer on. */
		{ CONF_MII1_COL,    PAD_IN  },
		{ CONF_MII1_CRS,    PAD_IN  },
		{ CONF_MII1_RX_ER,  PAD_IN  },
		{ CONF_MII1_RX_DV,  PAD_IN  },
		{ CONF_MII1_RX_CLK, PAD_IN  },
		{ CONF_MII1_TX_CLK, PAD_IN  },	/* PHY drives it in MII */
		{ CONF_MII1_RXD3,   PAD_IN  },
		{ CONF_MII1_RXD2,   PAD_IN  },
		{ CONF_MII1_RXD1,   PAD_IN  },
		{ CONF_MII1_RXD0,   PAD_IN  },
		/* Transmit side: output only. */
		{ CONF_MII1_TX_EN,  PAD_OUT },
		{ CONF_MII1_TXD3,   PAD_OUT },
		{ CONF_MII1_TXD2,   PAD_OUT },
		{ CONF_MII1_TXD1,   PAD_OUT },
		{ CONF_MII1_TXD0,   PAD_OUT },
		/* MDIO is bidirectional; MDC is driven by us but the pad is
		 * configured the same way u-boot and Linux configure it. */
		{ CONF_MDIO_DATA,   PAD_IN  },
		{ CONF_MDIO_CLK,    PAD_OUT },
	};
	unsigned int i;

	for (i = 0; i < sizeof(pads) / sizeof(pads[0]); i++)
		mmio_write32(phys_to_mmio(pads[i].reg), pads[i].val);

	printk("[CPSW] MII1 + MDIO pads configured (%u pads, mode 0)\n",
	       (unsigned int)(sizeof(pads) / sizeof(pads[0])));
}

/*
 * One MDIO read.  Returns the 16-bit value, or -1.
 *
 * The controller does the framing; software sets GO and waits for it to clear,
 * then checks ACK.  A PHY that is absent clears GO with ACK low rather than
 * hanging, which is the difference between "no PHY" and "no bus".
 */
static int mdio_read(unsigned int phy, unsigned int reg)
{
	u32 off = CPSW_MDIO + MDIO_USERACCESS0;
	int timeout = CPSW_TIMEOUT;
	u32 v;

	wr(off, MDIO_UA_GO | ((reg & 0x1F) << 21) | ((phy & 0x1F) << 16));

	while ((rd(off) & MDIO_UA_GO) && --timeout)
		;
	if (!timeout)
		return -1;

	v = rd(off);
	if (!(v & MDIO_UA_ACK))
		return -1;

	return (int)(v & MDIO_UA_DATA);
}

static int cpsw_mdio_init(void)
{
	int timeout = CPSW_TIMEOUT;
	u32 alive;

	wr(CPSW_MDIO + MDIO_CONTROL, MDIO_CTRL_ENABLE | MDIO_CLKDIV);

	/*
	 * The controller scans the bus by itself and sets a bit in ALIVE for
	 * every PHY that answers.  Waiting for that rather than going straight
	 * to a read means "the bus works and something is on it" and "the PHY
	 * says what we expect" stay separate answers.
	 */
	do {
		alive = rd(CPSW_MDIO + MDIO_ALIVE);
	} while (!alive && --timeout);

	printk("[CPSW] MDIO alive mask 0x%08lx\n", (unsigned long)alive);

	if (!(alive & (1u << PHY_ADDR))) {
		printk("[CPSW] no PHY answered at address %d\n", PHY_ADDR);
		return -1;
	}
	return 0;
}

static int cpsw_probe(struct platform_device *pdev)
{
	int id1, id2;
	u32 idver;

	cpsw_va = phys_to_mmio(pdev->base);
	printk("[CPSW] probing at PA 0x%08lx (VA 0x%08lx), irq %d\n",
	       (unsigned long)pdev->base, (unsigned long)cpsw_va, pdev->irq);

	if (cpsw_clock_enable())
		return -1;

	cpsw_pinmux();

	/*
	 * Reset order taken from the working driver: the port MAC first, then
	 * the DMA, the wrapper, and the subsystem last.  Resetting the
	 * subsystem first would pull the ground out from under the blocks that
	 * have not been reset yet.
	 */
	if (cpsw_soft_reset("SL1",   CPSW_SL1   + SL_SOFT_RESET) ||
	    cpsw_soft_reset("CPDMA", CPSW_CPDMA + CPDMA_SOFT_RESET) ||
	    cpsw_soft_reset("WR",    CPSW_WR    + WR_SOFT_RESET) ||
	    cpsw_soft_reset("SS",    CPSW_SS    + SS_SOFT_RESET))
		return -1;

	/* First known answer: does the block exist, clocked, at this address? */
	idver = rd(CPSW_ALE + ALE_IDVER);
	printk("[CPSW] ALE idver 0x%08lx (expect 0x%08lx) %s\n",
	       (unsigned long)idver, (unsigned long)ALE_IDVER_RESET,
	       idver == ALE_IDVER_RESET ? "OK" : "MISMATCH");

	if (cpsw_mdio_init())
		return -1;

	/* Second known answer: is the PHY the one this board is supposed to
	 * have, and is the MDIO bus carrying real data rather than zeros? */
	id1 = mdio_read(PHY_ADDR, PHY_REG_ID1);
	id2 = mdio_read(PHY_ADDR, PHY_REG_ID2);

	if (id1 < 0 || id2 < 0) {
		printk("[CPSW] PHY id read failed\n");
		return -1;
	}

	printk("[CPSW] PHY id %04lx:%04lx (expect %04lx:%04lx, rev %lu) %s\n",
	       (unsigned long)id1, (unsigned long)id2,
	       (unsigned long)LAN8710A_ID1, (unsigned long)LAN8710A_ID2,
	       (unsigned long)(id2 & 0xF),
	       (id1 == LAN8710A_ID1 &&
		(id2 & LAN8710A_ID2_MASK) == LAN8710A_ID2)
		       ? "OK" : "MISMATCH");

	printk("[CPSW] bring-up done; no traffic yet\n");
	return 0;
}

static struct platform_driver cpsw_driver = {
	.drv = {
		.name = "cpsw",
	},
	.probe = cpsw_probe,
};

static int __init cpsw_init(void)
{
	if (!CONFIG_ETHERNET)
		return 0;

	return platform_driver_register(&cpsw_driver);
}
device_initcall(cpsw_init);
