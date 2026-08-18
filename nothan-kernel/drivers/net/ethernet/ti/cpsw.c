/*
 * drivers/net/ethernet/ti/cpsw.c - AM335x CPSW Ethernet, bring-up
 *
 * Single port: CPSW_3G port 1 driving a LAN8710A over MII, which is the only
 * configuration this board has — am335x-bone-common.dtsi says slaves = <1>,
 * phy-mode = "mii", phy address 0, and nothing else is wired.
 *
 * Built in four steps, each flashed and looked at before the next — roadmap §0
 * — and each with an answer known before the board was powered on, so a wrong
 * result was a wrong result rather than something to interpret:
 *
 *   clocks, pads, reset   ALE IDVER matches its documented reset value
 *   MDIO and PHY          the PHY identifies itself as the one this board has
 *   link                  speed and duplex agree with the machine at the far end
 *   traffic               frames arrive, and a real stack answers what is sent
 *
 * Frames now leave through the netdev seam (nothan/netdev.h) rather than being
 * printed here.  Above that line nothing knows this is a wire.
 *
 * The two registers with documented reset values:
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
#include <nothan/ring.h>
#include <nothan/wait.h>
#include <nothan/sched.h>
#include <nothan/irq.h>
#include <nothan/netdev.h>

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
#define MDIO_UA_WRITE		(1u << 30)
#define MDIO_UA_ACK		(1u << 29)
#define MDIO_UA_DATA		0xFFFFu

/*
 * CPSW runs at 125 MHz and MDC must stay under 2.5 MHz, so 125/56 = 2.23 MHz.
 * The field holds divider-minus-one, which is the sort of off-by-one that
 * produces a bus that almost works.
 */
#define MDIO_CLKDIV		55

#define PHY_ADDR		0	/* am335x-bone-common.dtsi */

/*
 * IEEE 802.3 clause 22 register set — the part every PHY implements.
 *
 * Speed and duplex come from ADVERTISE and LPA rather than from the LAN8710A's
 * vendor register 31.  The standard registers say the same thing and say it on
 * any PHY, which matters because the board this becomes is not chosen yet
 * (os-architecture.md §14) and a vendor register is one more thing that would
 * have to be found and re-verified when it changes.
 */
#define MII_BMCR		0
#define MII_BMSR		1
#define MII_PHYSID1		2
#define MII_PHYSID2		3
#define MII_ADVERTISE		4
#define MII_LPA			5

#define BMCR_ANRESTART		(1u << 9)
#define BMCR_RESET		(1u << 15)
#define BMCR_ANENABLE		(1u << 12)

#define BMSR_LSTATUS		(1u << 2)	/* latching low — read twice */
#define BMSR_ANEGCOMPLETE	(1u << 5)

/* Technology bits, identical in ADVERTISE and LPA. */
#define LPA_10HALF		(1u << 5)
#define LPA_10FULL		(1u << 6)
#define LPA_100HALF		(1u << 7)
#define LPA_100FULL		(1u << 8)

#define LAN8710A_ID1		0x0007u
#define LAN8710A_ID2		0xC0F0u	/* low nibble is silicon revision */
#define LAN8710A_ID2_MASK	0xFFF0u

/*
 * How long to wait for auto-negotiation, in milliseconds.
 *
 * Boot continues either way: a box that refused to finish starting because a
 * cable was missing would be a worse machine than one that says so and carries
 * on.  So the only cost of a generous limit is a slower boot with no cable,
 * and the cost of a tight one is reporting a working link as broken.
 *
 * Measured on this board against a laptop: 1690 ms.  The first value here was
 * 2000, which passed with 15% to spare — close enough that a slower partner,
 * or a switch still bringing its own port up, would have been reported as no
 * cable.  The standard allows several seconds, so allow several seconds.
 */
#define PHY_ANEG_TIMEOUT_MS	5000

#define CPSW_TIMEOUT		100000

/* ---- Port, MAC and switch configuration ---------------------------- */
#define PORT_P1_SA_LO		(CPSW_PORT + 0x120)
#define PORT_P1_SA_HI		(CPSW_PORT + 0x124)

#define SL_MACCONTROL		(CPSW_SL1 + 0x04)
#define MAC_FULLDUPLEX		(1u << 0)
#define MAC_GMII_EN		(1u << 5)

/*
 * The MAC's own counters, at subsystem + 0x900 on this CPSW version.
 *
 * Worth reading because software counters cannot see the frames software never
 * heard about.  A burst that lost sixty per cent of small datagrams showed
 * "17024 frames in, 16992 out" — the board answered nearly everything it saw,
 * and everything it did not see was invisible.  Rx DMA Overruns is precisely
 * the frames the MAC had nowhere to put: no free descriptor at the start or in
 * the middle of reception (TRM §14.3.2.20.1.13).  It is the difference between
 * "this board dropped it" and "the other end never sent it".
 */
#define CPSW_STATS		0x0900
#define STATS_RX_GOOD		(CPSW_STATS + 0x00)
#define STATS_RX_CRC		(CPSW_STATS + 0x10)
#define STATS_RX_SOF_OVERRUN	(CPSW_STATS + 0x84)
#define STATS_RX_MOF_OVERRUN	(CPSW_STATS + 0x88)
#define STATS_RX_DMA_OVERRUN	(CPSW_STATS + 0x8C)

/* Statistics are off out of reset and count nothing until this is set. */
#define SS_STAT_PORT_EN		(CPSW_SS + 0x0C)
#define STAT_EN_ALL		0x7

#define ALE_CONTROL		(CPSW_ALE + 0x08)
#define ALE_PORTCTL(n)		(CPSW_ALE + 0x40 + (n) * 4)
#define ALE_CTL_ENABLE		(1u << 31)
#define ALE_CTL_CLEAR_TBL	(1u << 30)
#define ALE_CTL_BYPASS		(1u << 4)
#define ALE_PORT_FORWARD	3

/* ---- CPDMA --------------------------------------------------------- */
#define CPDMA_TX_CONTROL	(CPSW_CPDMA + 0x04)
#define CPDMA_RX_CONTROL	(CPSW_CPDMA + 0x14)
#define CPDMA_RX_BUFFER_OFFSET	(CPSW_CPDMA + 0x28)
#define CPDMA_TX_INTMASK_SET	(CPSW_CPDMA + 0x88)
#define CPDMA_EOI_VECTOR	(CPSW_CPDMA + 0x94)
#define CPDMA_RX_INTMASK_SET	(CPSW_CPDMA + 0xA8)
#define CPDMA_EOI_RX		1
#define CPDMA_EOI_TX		2

#define SR_TX0_HDP		(CPSW_STATERAM + 0x00)
#define SR_RX0_HDP		(CPSW_STATERAM + 0x20)
#define SR_TX0_CP		(CPSW_STATERAM + 0x40)
#define SR_RX0_CP		(CPSW_STATERAM + 0x60)

#define WR_C0_RX_EN		(CPSW_WR + 0x14)
#define WR_C0_TX_EN		(CPSW_WR + 0x18)

/* ---- Buffer descriptors -------------------------------------------- */
#define BD_SOP			(1u << 31)
#define BD_EOP			(1u << 30)
#define BD_OWNER		(1u << 29)
#define BD_EOQ			(1u << 28)
#define BD_TO_PORT_EN		(1u << 20)
#define BD_TO_PORT1		(1u << 16)
#define BD_PKT_LEN_MASK		0x7FFu

/*
 * Descriptors and packet buffers both live in the subsystem's own 8 KB of
 * CPPI RAM rather than in DDR, which is the single decision that keeps this
 * step small.  CPPI RAM is strongly ordered device memory, so the CPU and the
 * DMA see the same bytes with no cache maintenance anywhere — the whole class
 * of "DMA wrote it, the CPU read a stale line, nothing faulted" simply does
 * not arise (os-architecture.md §3.4).
 *
 * The ceiling that buys is real and worth stating now: four buffers, and every
 * byte read out costs an MMIO access.  That will not carry 12 Mbit/s of video.
 * Moving buffers to DDR with EDMA is the throughput step, and that is when the
 * DMA API — the last outstanding piece of Phase 7 — gets built, with a
 * consumer that exists.
 *
 * TRM §14.3.2.4.1: descriptors must be addressed from 0x4a102000.  The
 * addresses written *into* a descriptor are what the DMA engine will use, so
 * they are physical; the addresses this code dereferences are virtual.  Mixing
 * those two up produces a DMA that writes somewhere else entirely.
 */
#define CPPI_PA			0x4A102000u
#define CPPI_SIZE		0x2000u		/* 8 KB, and all of it descriptors */
#define CPSW_BUF_SIZE		1536		/* 24 cache lines exactly */

/*
 * Sixty-four receive buffers, in DDR.
 *
 * They used to be four, in CPPI RAM, and the two facts were the same fact:
 * four buffers of 1536 bytes plus one for transmit filled 7680 of the 8192
 * bytes there is, so a fifth was not a decision anyone could make.  Four
 * buffers is 484 microseconds of slack at line rate, and a burst measured 990
 * datagrams dropped against 605 delivered.
 *
 * Nothing ever required the buffers to be there.  The CPDMA reads and writes
 * ordinary memory; only the descriptors have to live in CPPI RAM, and they are
 * sixteen bytes each, so moving the buffers out leaves room for 512 of them
 * where there was room for four.  Sixty-four buffers is 96 KB of a 512 MB
 * machine and about 7.7 milliseconds of slack.
 *
 * The alignment is not decoration.  Invalidating a cache line that is only
 * partly ours throws away whatever else shares it, so a DMA buffer owns whole
 * lines start to finish: aligned to 64, and a size that is a multiple of it.
 */
#define CPSW_RX_COUNT		64

static u8 rx_buf[CPSW_RX_COUNT][CPSW_BUF_SIZE]
	__attribute__((aligned(DCACHE_LINE_SIZE)));

#define RX_BUF_VA(i)		((unsigned long)rx_buf[i])
#define RX_BUF_PA(i)		kva_to_phys(rx_buf[i])

/*
 * One transmit buffer, in DDR beside the receive ones and for the same reason.
 *
 * It sat in CPPI RAM until the cost of putting it there was measured: device
 * memory, written a word at a time by the CPU, at 48.6 ns per byte against
 * 11.0 for everything the receive path does to a byte.  Four fifths of the
 * transmit cost was the destination, not the work.  Here the CPU writes cached
 * DDR at full speed and one clean pass hands it to the DMA.
 *
 * One, not several.  A second buffer would let the next frame be built while
 * this one is on the wire, worth about a tenth of the maximum rate — but only
 * at the maximum rate, and reaching it needs a transmit chain with the same
 * completion and misqueue handling the receive side needed.  The buffer count
 * is a number in this driver; the seam above it already has the shape that
 * stops caring what the number is.
 */
static u8 tx_buf[CPSW_BUF_SIZE] __attribute__((aligned(DCACHE_LINE_SIZE)));

#define TX_BUF_VA		((unsigned long)tx_buf)
#define TX_BUF_PA		kva_to_phys(tx_buf)

/* Descriptors stay in CPPI RAM: sixteen bytes each, written once per frame. */
#define RX_BD_PA(i)		(CPPI_PA + (i) * 16u)
#define RX_BD_VA(i)		(cppi_va + (i) * 16u)

#define TX_BD_PA		(CPPI_PA + CPSW_RX_COUNT * 16u)
#define TX_BD_VA		(cppi_va + CPSW_RX_COUNT * 16u)

#define ETH_MIN_FRAME		60	/* without FCS; the MAC pads nothing */

static u32 cpsw_va;		/* subsystem base, already translated */
static u32 cppi_va;		/* CPPI RAM, already translated */
static u8  cpsw_mac[6];
static int link_full_duplex;

/*
 * How many frames one pass of the poller will take before it goes round again.
 *
 * It is a bound on time, not on the ring: with four buffers it can never be
 * reached, and that is the intent — a backstop, not a routine limit.  Sizing
 * it to today's buffer count would put a number that has nothing to do with
 * buffers into a constant named after them, and would be wrong the day the
 * buffers move to DDR and stop being four.
 *
 * Sixty-four MTU frames is about nine milliseconds of copying, which sounds
 * long until one remembers where it runs: the poller is a task in the NET
 * band, so audio preempts it.  That is the entire reason the work was moved
 * out of the interrupt.
 */
#define CPSW_RX_BUDGET		64

static DEFINE_WAIT_QUEUE(rx_wait);

static unsigned int rx_head;		/* next descriptor the DMA will finish */
static unsigned int rx_tail;		/* last descriptor in the chain */
static unsigned long rx_frames;		/* frames handed up */
static unsigned long rx_malformed;	/* length outside what a frame can be */

/*
 * Evidence, not decoration.
 *
 * @rx_isr_entries against @rx_frames is the ratio that says whether the
 * interrupt is behaving: one entry per burst is the masking working, one entry
 * per frame is it not.  @rx_poll_max says how close the budget ever came to
 * mattering.  Both exist because the failure that produced this code — a board
 * that stopped answering anything, shell included — was diagnosed by argument
 * rather than by measurement, and an argument cannot be checked on the next
 * board.
 */
static unsigned long rx_isr_entries;
static unsigned long rx_unmasks;
static unsigned long rx_poll_max;
static unsigned long rx_chain_broken;

/* At most one report every two seconds, whatever the load. */
DEFINE_RATELIMIT(rx_stat_rl, 2000, 1);

static DEFINE_WAIT_QUEUE(tx_wait);
static volatile int tx_busy;		/* one buffer, so one frame in flight */


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

static int mdio_write(unsigned int phy, unsigned int reg, u16 val)
{
	u32 off = CPSW_MDIO + MDIO_USERACCESS0;
	int timeout = CPSW_TIMEOUT;

	wr(off, MDIO_UA_GO | MDIO_UA_WRITE |
	   ((reg & 0x1F) << 21) | ((phy & 0x1F) << 16) | val);

	while ((rd(off) & MDIO_UA_GO) && --timeout)
		;

	return timeout ? 0 : -1;
}

/*
 * Report the link, and what it negotiated.
 *
 * BMSR is read twice because the link bit latches low: a link that dropped and
 * came back still reads down once, and reporting that first read would be a
 * driver that lies about the present in order to remember the past.
 *
 * Speed and duplex are whatever both ends advertised — the intersection of
 * ADVERTISE and LPA, taking the best of what is left.  Asking the PHY what it
 * settled on would need a vendor register; deriving it needs none, and gives
 * the same answer.
 */
static void cpsw_report_link(void)
{
	int bmsr, bmcr, adv, lpa, common;
	int waited = 0;
	int timeout;

	/*
	 * What the bootloader left behind, before anything here changes it.
	 * Printed because the first version of this function found link down
	 * with a cable attached, and the state it started from was the one
	 * thing the log could not say.
	 */
	printk("[CPSW] PHY initial bmcr 0x%04lx bmsr 0x%04lx\n",
	       (unsigned long)mdio_read(PHY_ADDR, MII_BMCR),
	       (unsigned long)mdio_read(PHY_ADDR, MII_BMSR));

	/*
	 * Reset the PHY and wait for the bit to clear itself.
	 *
	 * Skipping this was the difference between this driver and the one in
	 * nothan_os_old that reached traffic on this board.  A PHY left in
	 * whatever mode the bootloader chose will answer MDIO perfectly while
	 * declining to negotiate, which reads as a cable fault and is not one.
	 */
	mdio_write(PHY_ADDR, MII_BMCR, BMCR_RESET);

	timeout = 1000;
	while (timeout--) {
		bmcr = mdio_read(PHY_ADDR, MII_BMCR);
		if (bmcr < 0) {
			printk("[CPSW] PHY unreadable during reset\n");
			return;
		}
		if (!(bmcr & BMCR_RESET))
			break;
		mdelay(1);
	}
	if (timeout <= 0) {
		printk("[CPSW] PHY reset never cleared (bmcr 0x%04lx)\n",
		       (unsigned long)bmcr);
		return;
	}

	/* Make sure negotiation is actually running before waiting for it. */
	mdio_write(PHY_ADDR, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);

	/*
	 * Read it back.  Reads are proven — BMSR returns this PHY's real
	 * capability bits — but nothing has yet shown that a *write* reaches
	 * it, and a write that quietly does nothing looks exactly like a PHY
	 * that will not negotiate.  Separating those two is worth one line.
	 */
	bmcr = mdio_read(PHY_ADDR, MII_BMCR);
	printk("[CPSW] PHY bmcr after autoneg request 0x%04lx (expect bits"
	       " 0x%04lx set) %s\n",
	       (unsigned long)bmcr, (unsigned long)BMCR_ANENABLE,
	       (bmcr >= 0 && (bmcr & BMCR_ANENABLE)) ? "write OK"
						     : "WRITE NOT TAKING");

	for (;;) {
		mdio_read(PHY_ADDR, MII_BMSR);		/* clear the latch */
		bmsr = mdio_read(PHY_ADDR, MII_BMSR);

		if (bmsr < 0) {
			printk("[CPSW] link: BMSR read failed\n");
			return;
		}
		if ((bmsr & BMSR_LSTATUS) && (bmsr & BMSR_ANEGCOMPLETE))
			break;
		if (waited >= PHY_ANEG_TIMEOUT_MS) {
			printk("[CPSW] link DOWN after %d ms"
			       " (bmsr 0x%04lx: link %s, autoneg %s)\n",
			       waited, (unsigned long)bmsr,
			       (bmsr & BMSR_LSTATUS) ? "up" : "down",
			       (bmsr & BMSR_ANEGCOMPLETE) ? "done" : "incomplete");
			printk("[CPSW] no cable, or nothing at the other end\n");
			return;
		}
		mdelay(10);
		waited += 10;
	}

	adv = mdio_read(PHY_ADDR, MII_ADVERTISE);
	lpa = mdio_read(PHY_ADDR, MII_LPA);
	if (adv < 0 || lpa < 0) {
		printk("[CPSW] link UP but capabilities unreadable\n");
		return;
	}

	common = adv & lpa;

	link_full_duplex = (common & (LPA_100FULL | LPA_10FULL)) ? 1 : 0;

	printk("[CPSW] link UP after %d ms: %s, %s duplex"
	       " (adv 0x%04lx lpa 0x%04lx)\n",
	       waited,
	       (common & (LPA_100FULL | LPA_100HALF)) ? "100 Mbit" : "10 Mbit",
	       (common & (LPA_100FULL | LPA_10FULL))  ? "full" : "half",
	       (unsigned long)adv, (unsigned long)lpa);

	if (!common)
		printk("[CPSW] WARNING: no common capability — check the pads\n");
}

static void cpsw_bd_write(u32 bd_va, u32 next_pa, u32 buf_pa, u32 len, u32 flags)
{
	mmio_write32(bd_va +  0, next_pa);
	mmio_write32(bd_va +  4, buf_pa);
	mmio_write32(bd_va +  8, len);
	mmio_write32(bd_va + 12, flags);
}

/*
 * ALE in bypass: every frame goes straight to the host port.
 *
 * The address lookup engine is a switch's forwarding table, and this board
 * has one external port — there is nothing to switch between.  Bypass says so
 * directly instead of programming a table whose only entry would be "send
 * everything to the host".
 */
static void cpsw_ale_init(void)
{
	int i;

	/* Counters are off out of reset; turn them on before any frame. */
	wr(SS_STAT_PORT_EN, STAT_EN_ALL);

	wr(ALE_CONTROL, ALE_CTL_ENABLE | ALE_CTL_CLEAR_TBL | ALE_CTL_BYPASS);
	for (i = 0; i < 3; i++)
		wr(ALE_PORTCTL(i), ALE_PORT_FORWARD);

	printk("[CPSW] ALE bypass, control 0x%08lx\n",
	       (unsigned long)rd(ALE_CONTROL));
}

/*
 * Port address and MAC mode.
 *
 * Duplex comes from what auto-negotiation actually settled on, not from a
 * constant.  A MAC configured full against a half-duplex partner does not
 * fail loudly — it collides, retries, and loses frames in a pattern that
 * looks like a bad cable, which is a long way from the line that hard-coded
 * it.  The driver this is derived from wrote MAC_FULLDUPLEX unconditionally.
 *
 * KNOWN LIMITATION: read once, here, and never looked at again.  Boot with the
 * cable out and the link comes up later as full duplex while this MAC stays
 * half, and the half-duplex receiver is gated off for the whole time the port
 * is transmitting.  Measured, because it happened: a bidirectional video call
 * lost 79% of what arrived — 28 of every 132 datagrams — while transmit stayed
 * perfect and every counter in the machine read clean.  0 CRC errors, 0 DMA
 * overruns, 0 dropped by any ring: the frames were refused by the MAC before
 * anything here could count them.  Plugging the cable in first and rebooting
 * took the same test to 132 of 132.
 *
 * A box that gets its cable plugged in after it is switched on is an ordinary
 * box, so this wants link-change detection and a MAC reconfigured when the
 * PHY renegotiates.  Until then the failure is silent, and this comment is the
 * only place that says what it looks like.
 */
static void cpsw_port_init(void)
{
	u32 sa_lo = ((u32)cpsw_mac[0] << 8) | cpsw_mac[1];
	u32 sa_hi = ((u32)cpsw_mac[2] << 24) | ((u32)cpsw_mac[3] << 16) |
		    ((u32)cpsw_mac[4] << 8)  |  (u32)cpsw_mac[5];
	u32 mac_ctl = MAC_GMII_EN;

	wr(PORT_P1_SA_LO, sa_lo);
	wr(PORT_P1_SA_HI, sa_hi);

	if (link_full_duplex)
		mac_ctl |= MAC_FULLDUPLEX;

	wr(SL_MACCONTROL, mac_ctl);

	printk("[CPSW] MAC %02lx:%02lx:%02lx:%02lx:%02lx:%02lx, %s duplex,"
	       " maccontrol 0x%08lx\n",
	       (unsigned long)cpsw_mac[0], (unsigned long)cpsw_mac[1],
	       (unsigned long)cpsw_mac[2], (unsigned long)cpsw_mac[3],
	       (unsigned long)cpsw_mac[4], (unsigned long)cpsw_mac[5],
	       link_full_duplex ? "full" : "half",
	       (unsigned long)rd(SL_MACCONTROL));
}

static void cpsw_rx_ring_init(void)
{
	int i;

	for (i = 0; i < CPSW_RX_COUNT; i++) {
		u32 next = (i < CPSW_RX_COUNT - 1) ? RX_BD_PA(i + 1) : 0;

		/*
		 * Drop any cached copy before the DMA is allowed to write
		 * here.  A line still sitting in the cache would be read in
		 * preference to what the engine puts in memory, and a line
		 * that happened to be dirty would later be written back over
		 * it.  Neither faults; both produce a frame that is wrong in
		 * the middle.
		 */
		invalidate_dcache_range(RX_BUF_VA(i),
					RX_BUF_VA(i) + CPSW_BUF_SIZE);

		cpsw_bd_write(RX_BD_VA(i), next, RX_BUF_PA(i),
			      CPSW_BUF_SIZE, BD_OWNER);
	}

	rx_head = 0;
	rx_tail = CPSW_RX_COUNT - 1;
	wr(SR_RX0_HDP, RX_BD_PA(0));
}

static void cpsw_cpdma_init(void)
{
	int i;

	/* Every head and completion pointer must be zero before a channel is
	 * enabled; the hardware treats a stale pointer as a live queue. */
	for (i = 0; i < 8; i++) {
		wr(SR_TX0_HDP + i * 4, 0);
		wr(SR_RX0_HDP + i * 4, 0);
		wr(SR_TX0_CP  + i * 4, 0);
		wr(SR_RX0_CP  + i * 4, 0);
	}

	wr(CPDMA_RX_BUFFER_OFFSET, 0);
	wr(CPDMA_TX_CONTROL, 1);
	wr(CPDMA_RX_CONTROL, 1);

	cpsw_rx_ring_init();

	wr(CPDMA_RX_INTMASK_SET, 1);
	wr(CPDMA_TX_INTMASK_SET, 1);
	wr(WR_C0_RX_EN, 1);
	wr(WR_C0_TX_EN, 1);
}

/*
 * Receive interrupt: recognise, queue, wake.  Nothing else.
 *
 * The frame stays where the DMA put it and the descriptor stays held until a
 * task has copied it out — an interrupt that copied 1536 bytes through
 * strongly ordered memory and then ran a protocol stack is what the driver
 * this is derived from did, and what §9.2 exists to prevent.  The measurement
 * that made the cost concrete is in §9.2.1: an ISR misbehaving took 73% of
 * this machine and a third of its clock ticks.
 */
static void cpsw_rx_isr(unsigned int irq)
{
	(void)irq;

	/*
	 * Mask the source, acknowledge, wake the poller.  No descriptor is
	 * read here and none is acknowledged, and that is the whole design.
	 *
	 * §14 of the TRM: the receive interrupt is a level, and it deasserts
	 * only when the host writes RX0_CP with the address of the descriptor
	 * the port itself last completed.  Anything else leaves it asserted.
	 * A handler that drains descriptors is therefore racing to satisfy a
	 * condition the hardware keeps re-arming, and under sustained input it
	 * loses: the handler that this replaced re-armed buffers from inside
	 * its own loop, the DMA refilled them at line rate, and the loop fed
	 * itself.  The CPU never left interrupt context, so no task ran again
	 * — the board stopped answering ARP, ICMP, UDP and its own console at
	 * the same instant, with no exception to show for it.
	 *
	 * Masking makes the level harmless.  It stays asserted, it stays
	 * unserviced, and it cannot re-enter.  The poller lifts the mask when
	 * it has actually caught up and not before.
	 */
	wr(WR_C0_RX_EN, 0);
	wr(CPDMA_EOI_VECTOR, CPDMA_EOI_RX);

	rx_isr_entries++;
	wake_up(&rx_wait);
}

/*
 * Transmit completion: acknowledge and release the slot.  Nothing else — the
 * frame is already gone, and the only work left is bookkeeping.
 */
static void cpsw_tx_isr(unsigned int irq)
{
	(void)irq;

	wr(SR_TX0_CP, TX_BD_PA);
	wr(CPDMA_EOI_VECTOR, CPDMA_EOI_TX);

	tx_busy = 0;
	wake_up(&tx_wait);
}

/*
 * Claim the transmit buffer, sleeping until the last frame has left.
 *
 * Sleeping rather than polling matters even though transmits are rare here.  A
 * spin would hold the CPU at whatever priority the caller runs at, and the
 * driver this is derived from spun with a bare loop and a timeout; that is the
 * shape that turned out to cost 10.8 ms in the SD card path (§3.6).  The wait
 * queue costs nothing when the slot is free, which is the normal case.
 *
 * The wait is here rather than in the send because the caller writes the buffer
 * in between: waiting after the frame was built would mean building it into
 * memory the DMA was still reading.  That ordering is the entire reason this is
 * two calls and not one.
 */
static u8 *cpsw_tx_alloc(struct netdev *dev)
{
	unsigned long flags;

	(void)dev;

	flags = local_irq_save();
	while (tx_busy)
		wait_event_locked(&tx_wait);
	tx_busy = 1;
	local_irq_restore(flags);

	return tx_buf;
}

static void cpsw_tx_abort(struct netdev *dev)
{
	(void)dev;

	tx_busy = 0;
	wake_up(&tx_wait);
}

/*
 * Hand the buffer to the DMA.  @tx_busy stays set until the completion
 * interrupt clears it — the frame is not gone when this returns, only started.
 *
 * @len is padded to the Ethernet minimum with zeros: a MAC does not invent the
 * padding, and a 42-byte ARP frame put on the wire as 42 bytes is a runt that
 * the receiver discards without telling anyone.
 */
static int cpsw_tx_send(struct netdev *dev, unsigned int len)
{
	unsigned int i;

	if (len > CPSW_BUF_SIZE) {
		cpsw_tx_abort(dev);
		return -1;
	}

	for (i = len; i < ETH_MIN_FRAME; i++)
		tx_buf[i] = 0;

	if (len < ETH_MIN_FRAME)
		len = ETH_MIN_FRAME;

	/*
	 * Clean, not clean-and-invalidate: the DMA only reads this buffer, so
	 * there is nothing it could have written that the cache must be told
	 * about.  A clean over a partial line is safe in a way an invalidate is
	 * not — it writes a neighbour's dirty bytes back rather than discarding
	 * them — so the tail needs no special care here.
	 */
	flush_dcache_range(TX_BUF_VA, TX_BUF_VA + len);

	/*
	 * Directed transmit: with the ALE in bypass there is no forwarding
	 * table to consult, so the descriptor has to name the port itself.
	 */
	cpsw_bd_write(TX_BD_VA, 0, TX_BUF_PA, len,
		      BD_SOP | BD_EOP | BD_OWNER |
		      BD_TO_PORT_EN | BD_TO_PORT1 | (len & BD_PKT_LEN_MASK));

	wr(SR_TX0_HDP, TX_BD_PA);
	return 0;
}

static struct netdev cpsw_netdev = {
	.name     = "eth0",
	.tx_alloc = cpsw_tx_alloc,
	.tx_send  = cpsw_tx_send,
	.tx_abort = cpsw_tx_abort,
};

/*
 * The matching reader is gone.  Receive buffers are ordinary memory now, so
 * there is nothing to read a word at a time out of — the frame is already
 * where the protocol layer wants it.  Only transmit still copies into CPPI
 * RAM, and only until it moves out too.
 */

/*
 * The receive task: NET band, woken by the handler, does the work the handler
 * is not allowed to.
 *
 * For this step that work is to read the Ethernet header and say what arrived.
 * The header is enough to prove the frames are real — the source address of
 * the first ones should be the machine at the other end of the cable, which is
 * an answer that can be checked rather than admired.
 */
/*
 * Which descriptor an address names.  The inverse of RX_BD_PA().
 *
 * Bounded, because the next pointer is read back out of a descriptor the DMA
 * also writes, and the value it yields indexes an array of buffers.  A chain
 * that has come apart would otherwise be a read outside rx_buf[] — and the
 * descriptor immediately past the receive chain is the transmit one, so the
 * first address out of range is a plausible-looking one.  CPSW_RX_COUNT is
 * returned for anything else, which every caller reads as "no successor".
 */
static inline unsigned int rx_bd_index(u32 pa)
{
	unsigned int i;

	if (pa < CPPI_PA)
		return CPSW_RX_COUNT;

	i = (pa - CPPI_PA) / 16u;
	return i < CPSW_RX_COUNT ? i : CPSW_RX_COUNT;
}

/* Is there a completed descriptor waiting?  Also the poller's wait condition. */
static inline int cpsw_rx_ready(void)
{
	return !(mmio_read32(RX_BD_VA(rx_head) + 12) & BD_OWNER);
}

/*
 * Everything worth knowing about the link in one place, software counters and
 * the MAC's own side by side.
 *
 * The hardware ones are what make a loss attributable.  "Frames in" counts
 * what reached this driver; Rx DMA Overruns counts what the MAC threw away
 * before it could, because no descriptor was free.  Without the second, a
 * burst that lost most of its datagrams could be blamed on this board, on the
 * sender, or on the receiver's socket buffer, and there was no way to choose.
 */
static void cpsw_report(unsigned long suppressed)
{
	printk("[CPSW] %lu rx, %lu bad, %lu chain; %lu irq, %lu unmask,"
	       " %lu deep\n",
	       rx_frames, rx_malformed, rx_chain_broken,
	       rx_isr_entries, rx_unmasks, rx_poll_max);

	printk("[CPSW] mac: %lu good, %lu crc, overruns %lu sof / %lu mof"
	       " / %lu dma\n",
	       (unsigned long)rd(STATS_RX_GOOD),
	       (unsigned long)rd(STATS_RX_CRC),
	       (unsigned long)rd(STATS_RX_SOF_OVERRUN),
	       (unsigned long)rd(STATS_RX_MOF_OVERRUN),
	       (unsigned long)rd(STATS_RX_DMA_OVERRUN));

	net_dump_stats();

	if (suppressed)
		printk("[CPSW] (%lu reports suppressed)\n", suppressed);
}

/*
 * Take up to @budget completed frames off the chain.
 *
 * The chain is walked by reading each descriptor's next pointer, never by
 * stepping an index.  Stepping an index assumes descriptors complete in the
 * order they are numbered, which is true only until something reorders the
 * chain — and the EOQ restart below reorders it by design.  Once the two
 * disagree, RX0_CP is written with the address of a descriptor the port did
 * not complete, the interrupt never deasserts, and the failure that follows
 * looks nothing like the line of code that caused it.
 *
 * Returns the number of frames taken.  Fewer than @budget means the chain was
 * drained, which is the only thing that lets the interrupt back in.
 */
static int cpsw_rx_poll(int budget)
{
	int done = 0;

	while (done < budget) {
		unsigned int cur = rx_head;
		u32 bd = RX_BD_VA(cur);
		u32 flags, next_pa;
		unsigned int len;

		flags = mmio_read32(bd + 12);
		if (flags & BD_OWNER)
			break;			/* still the DMA's */

		next_pa = mmio_read32(bd + 0);
		len     = flags & BD_PKT_LEN_MASK;

		if (len >= ETH_HDR_SIZE && len <= CPSW_BUF_SIZE) {
			/*
			 * Nothing is copied out.  The frame was written to
			 * ordinary memory by the engine, so the protocol layer
			 * reads it where it lies; all that is needed first is
			 * to drop whatever the cache may have speculated into
			 * these lines while the DMA was writing them.
			 *
			 * This is the copy that used to cost 92 nanoseconds a
			 * byte, because it came out of CPPI RAM one strongly
			 * ordered word at a time — about 139 microseconds for
			 * a full frame, in each direction.  It is not smaller
			 * now, it is gone.
			 */
			invalidate_dcache_range(RX_BUF_VA(cur),
						RX_BUF_VA(cur) + len);
			rx_frames++;
		} else {
			rx_malformed++;
			len = 0;
		}

		/*
		 * Acknowledge this descriptor by its own address, before it is
		 * re-armed.  This is the write the hardware compares against
		 * what it last completed, and it is the only thing that lowers
		 * the interrupt.
		 */
		wr(SR_RX0_CP, RX_BD_PA(cur));

		/*
		 * The chain continues where this descriptor pointed.  No
		 * successor — either a zero pointer or one that does not name
		 * a receive descriptor — means this buffer, once re-armed
		 * below, is the whole of what is queued.
		 */
		rx_head = next_pa ? rx_bd_index(next_pa) : CPSW_RX_COUNT;
		if (rx_head >= CPSW_RX_COUNT) {
			if (next_pa)
				rx_chain_broken++;
			rx_head = cur;
		}

		/*
		 * Hand the frame up before handing the buffer back, and not
		 * the other way round.  Re-arming makes the buffer the DMA's
		 * again, and the engine will fill it with the next frame off
		 * the wire — while the protocol layer is still reading the
		 * previous one out of it.  When the copy stood in between,
		 * the order did not matter; without it, it is the difference
		 * between a stack and a race.
		 */
		if (len)
			netdev_rx(&cpsw_netdev, (const u8 *)RX_BUF_VA(cur), len);

		invalidate_dcache_range(RX_BUF_VA(cur),
					RX_BUF_VA(cur) + CPSW_BUF_SIZE);

		cpsw_bd_write(bd, 0, RX_BUF_PA(cur), CPSW_BUF_SIZE, BD_OWNER);

		if (rx_tail != cur) {
			mmio_write32(RX_BD_VA(rx_tail) + 0, RX_BD_PA(cur));
			rx_tail = cur;
		}

		/*
		 * EOQ says the engine reached a descriptor with no successor
		 * and stopped.  Restart it at the front of what is queued now,
		 * which after the re-arm above is never empty — and restart
		 * only on EOQ, so a queue still being walked is left alone.
		 */
		if (flags & BD_EOQ)
			wr(SR_RX0_HDP, RX_BD_PA(rx_head));

		done++;

		/*
		 * A count every so often, because a driver that has stopped
		 * and a link that is quiet look identical otherwise — a
		 * distinction this bring-up already had to make once.
		 *
		 * Limited in time, not in frames: see nothan/printk.h.  The
		 * previous "every 32 frames" produced five hundred summaries
		 * during one burst and tore the console apart printing them.
		 */
		if (len && ratelimit_allow(&rx_stat_rl))
			cpsw_report(rx_stat_rl.last_dropped);
	}

	if ((unsigned long)done > rx_poll_max)
		rx_poll_max = (unsigned long)done;

	return done;
}

static void cpsw_rx_task(void)
{
	printk("[CPSW] rx task up, waiting for frames\n");

	for (;;) {
		/*
		 * Hitting the budget means there is probably more, so go round
		 * again and stay masked.  Only a short pass proves the chain is
		 * drained, and only then is it safe to let the level back in.
		 *
		 * Sleeping is done after unmasking and never after a full pass:
		 * a task that slept with the interrupt still masked would never
		 * be woken by anything, which is the one way this arrangement
		 * can deadlock and the reason the order is not interchangeable.
		 */
		if (cpsw_rx_poll(CPSW_RX_BUDGET) < CPSW_RX_BUDGET) {
			wr(WR_C0_RX_EN, 1);
			rx_unmasks++;

			wait_event_cond(&rx_wait, cpsw_rx_ready());
		}
	}
}

#if CONFIG_NET_ARP_PROBE

/*
 * Ask the machine at the other end of the cable a question it will answer.
 *
 * This is not a network stack and is not the beginning of one — it is a way to
 * check that a transmitted frame is well formed, using a real operating system
 * as the judge.  An ARP request for an address the far end actually holds gets
 * a reply, and that reply proves three separate things at once: the frame left
 * the port, a real stack parsed it and found it valid, and the answer came
 * back addressed to this board's own MAC rather than to broadcast — which is
 * the only test so far that exercises the address programmed into the port.
 *
 * The addresses are the ones the laptop is using on the shared link.  Nothing
 * here holds an IP in any meaningful sense; the bytes are only what makes the
 * question answerable.
 */
#define PROBE_LOCAL_IP		{ 10, 42, 0, 2 }
#define PROBE_TARGET_IP		{ 10, 42, 0, 1 }
#define PROBE_COUNT		10
#define PROBE_INTERVAL_MS	1000

static void cpsw_arp_probe_task(void)
{
	static const u8 target_ip[4] = PROBE_TARGET_IP;
	static const u8 local_ip[4]  = PROBE_LOCAL_IP;
	u8 frame[42];
	int i;

	/* Ethernet header: broadcast, from us, ARP. */
	for (i = 0; i < 6; i++)
		frame[i] = 0xFF;
	for (i = 0; i < 6; i++)
		frame[6 + i] = cpsw_mac[i];
	frame[12] = ETH_P_ARP >> 8;
	frame[13] = ETH_P_ARP & 0xFF;

	/* ARP: Ethernet over IPv4, request. */
	frame[14] = 0x00; frame[15] = 0x01;	/* hardware type: Ethernet */
	frame[16] = 0x08; frame[17] = 0x00;	/* protocol type: IPv4     */
	frame[18] = 6;				/* hardware address length */
	frame[19] = 4;				/* protocol address length */
	frame[20] = 0x00; frame[21] = 0x01;	/* operation: request      */

	for (i = 0; i < 6; i++)
		frame[22 + i] = cpsw_mac[i];	/* sender hardware address */
	for (i = 0; i < 4; i++)
		frame[28 + i] = local_ip[i];	/* sender protocol address */
	for (i = 0; i < 6; i++)
		frame[32 + i] = 0;		/* target hardware: unknown */
	for (i = 0; i < 4; i++)
		frame[38 + i] = target_ip[i];	/* target protocol address */

	printk("[CPSW] arp probe: who has %u.%u.%u.%u, tell %u.%u.%u.%u\n",
	       target_ip[0], target_ip[1], target_ip[2], target_ip[3],
	       local_ip[0], local_ip[1], local_ip[2], local_ip[3]);
	printk("[CPSW] a reply addressed to this board's MAC is the pass\n");

	for (i = 0; i < PROBE_COUNT; i++) {
		if (cpsw_tx(frame, sizeof(frame)))
			printk("[CPSW] arp probe %d: transmit refused\n", i + 1);
		else
			printk("[CPSW] arp probe %d sent\n", i + 1);

		msleep(PROBE_INTERVAL_MS);
	}

	printk("[CPSW] arp probe done\n");
}

#endif /* CONFIG_NET_ARP_PROBE */

static int cpsw_probe(struct platform_device *pdev)
{
	int id1, id2;
	u32 idver;

	cpsw_va = phys_to_mmio(pdev->base);
	cppi_va = phys_to_mmio(CPPI_PA);
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
	id1 = mdio_read(PHY_ADDR, MII_PHYSID1);
	id2 = mdio_read(PHY_ADDR, MII_PHYSID2);

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

	cpsw_report_link();

	board_get_mac_addr(cpsw_mac);
	for (int i = 0; i < ETH_ALEN; i++)
		cpsw_netdev.mac[i] = cpsw_mac[i];
	netdev_register(&cpsw_netdev);

	cpsw_ale_init();
	cpsw_port_init();
	cpsw_cpdma_init();

	{
		struct task_struct *t = task_create(cpsw_rx_task, PRIO_NET,
						    "net-rx");
		if (!t) {
			printk("[CPSW] could not create the rx task\n");
			return -1;
		}
		enqueue_task(&runqueue, t);
	}

	request_irq(pdev->irq, cpsw_rx_isr);
	intc_enable_irq(pdev->irq);

	/*
	 * Transmit completion is a separate line: TRM Ch06 lists 41 as
	 * 3PGSWRXINT0 and 42 as 3PGSWTXINT0.  The board table names the
	 * receive one; the transmit one is its neighbour and is derived here
	 * rather than added as a second board entry for the same device.
	 */
	request_irq(pdev->irq + 1, cpsw_tx_isr);
	intc_enable_irq(pdev->irq + 1);

	printk("[CPSW] rx on irq %d, tx on irq %d;"
	       " %d rx buffers of %d bytes in DDR, descriptors in CPPI RAM\n",
	       pdev->irq, pdev->irq + 1, CPSW_RX_COUNT, CPSW_BUF_SIZE);

#if CONFIG_NET_ARP_PROBE
	{
		struct task_struct *t = task_create(cpsw_arp_probe_task,
						    PRIO_BG, "net-arp");
		if (t)
			enqueue_task(&runqueue, t);
	}
#endif
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
