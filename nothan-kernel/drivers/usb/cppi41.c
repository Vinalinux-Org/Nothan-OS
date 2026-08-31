/*
 * drivers/usb/cppi41.c - the DMA engine inside the USB subsystem, brought up
 * one answerable question at a time
 *
 * CPPI 4.1 moves USB packets between the controller's FIFOs and memory without
 * the CPU touching them.  It exists here for one reason: the camera's
 * isochronous endpoint delivers a packet every 125 microseconds, this kernel's
 * tick is one millisecond, and no sleep it can take is shorter than eight
 * deadlines.  Software therefore polls, and polling costs the whole machine —
 * ps reports the drain holding 56 seconds of CPU against idle's 1.9, with idle
 * not moving at all between samples.  That is not a number to optimise; it is
 * a design that cannot be made to work by arranging priorities, and hardware
 * moving the bytes is the only thing that removes the deadline from software.
 *
 * It is also why AUTOREQ exists only alongside DMA.  The two are one mechanism
 * — the engine issues the IN token as it retires a descriptor — and this
 * driver has spent a week doing the other half by hand.
 *
 * Built in stages, each with an answer known before the board is powered on,
 * because the failures here look alike: a queue manager that is misconfigured
 * and a channel that is misconfigured both present as "no packets arrive", and
 * four flash cycles went to exactly that ambiguity earlier this week.
 *
 *   1  queue manager   push a descriptor onto a queue and pop it back
 *   2  one packet      receive a single packet by DMA and print its head
 *   3  the stream      run it, and measure what the CPU gets back
 *
 * This file is stage 1.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/config.h>
#include <nothan/types.h>
#include <nothan/mmio.h>
#include <nothan/mm.h>
#include <nothan/printk.h>
#include <asm/barrier.h>

#include "cppi41.h"

/*
 * Four blocks inside the USB subsystem window, at the offsets the device tree
 * gives them (am33xx.dtsi, dma-controller@2000: "glue", "controller",
 * "scheduler", "queuemgr").  Reading them from the device tree rather than a
 * manual is worth saying: these are the numbers the vendor's own driver is
 * built against, so they are the ones the silicon was tested with.
 */
#define CPPI_GLUE		0x0000
#define CPPI_CTRL		0x2000
#define CPPI_SCHED		0x3000
#define CPPI_QMGR		0x4000

/* Queue manager. */
#define QMGR_LRAM0_BASE		(CPPI_QMGR + 0x80)
#define QMGR_LRAM_SIZE		(CPPI_QMGR + 0x84)
#define QMGR_LRAM1_BASE		(CPPI_QMGR + 0x88)
#define QMGR_MEMBASE(x)		(CPPI_QMGR + 0x1000 + (x) * 0x10)
#define QMGR_MEMCTRL(x)		(CPPI_QMGR + 0x1004 + (x) * 0x10)
#  define MEMCTRL_IDX_SH	16
#  define MEMCTRL_DESC_SH	8

/*
 * A queue is four registers; D is the one that moves descriptors.  Writing an
 * address to it pushes, reading takes the head back — and the low five bits of
 * what comes out are status, not address, which is why the pop masks them off.
 */
#define QMGR_QUEUE_D(n)		(CPPI_QMGR + 0x200C + (n) * 0x10)

/*
 * Descriptors: 64 of them, 32 bytes each.
 *
 * Both numbers are constrained rather than chosen.  The hardware encodes the
 * region as log2 minus five, so a descriptor is at least 32 bytes and a region
 * holds at least 32 of them; 64 is the next step up and covers a frame's worth
 * of isochronous packets several times over.  In .bss, so it is contiguous in
 * the kernel's direct map and its physical address is a subtraction away.
 */
#define CPPI_NDESC		64
#define CPPI_DESC_SIZE		32

static u8 cppi_descs[CPPI_NDESC * CPPI_DESC_SIZE]
	__attribute__((aligned(CPPI_DESC_SIZE)));

/*
 * Linking RAM: one word per descriptor, which the queue manager uses to thread
 * them together.  It is the engine's own scratch space — nothing here ever
 * reads it — but leaving it unset means every queue operation writes through a
 * null pointer inside the hardware.
 */
static u32 cppi_lram[CPPI_NDESC];

static u32 cppi_va;		/* USB subsystem base, already translated */

static inline u32 rd(u32 off)		{ return mmio_read32(cppi_va + off); }
static inline void wr(u32 off, u32 v)	{ mmio_write32(cppi_va + off, v); }

/* log2 for the two sizes above; both are known powers of two. */
static unsigned int ilog2u(unsigned int v)
{
	unsigned int n = 0;

	while (v > 1) {
		v >>= 1;
		n++;
	}
	return n;
}

u32 cppi_desc_phys(unsigned int i)
{
	return kva_to_phys(&cppi_descs[i * CPPI_DESC_SIZE]);
}

void *cppi_desc(unsigned int i)
{
	return &cppi_descs[i * CPPI_DESC_SIZE];
}

/*
 * Push a descriptor onto a queue.
 *
 * The low bits carry how much of the descriptor the engine must fetch, in
 * words past the first six: (32 - 24) / 4 = 2 for a 32-byte descriptor.  It
 * shares the word with the address because descriptors are 32-byte aligned and
 * those bits are therefore always zero.
 */
void cppi_push(unsigned int queue, u32 desc_phys)
{
	dsb();		/* the descriptor must be in memory before it is offered */
	wr(QMGR_QUEUE_D(queue),
	   desc_phys | ((CPPI_DESC_SIZE - 24u) / 4u));
}

u32 cppi_pop(unsigned int queue)
{
	return rd(QMGR_QUEUE_D(queue)) & ~0x1Fu;
}

/*
 * Stage 1: does the queue manager work at all?
 *
 * Push a known descriptor onto a queue nothing is draining, take it back, and
 * compare.  It is a small test and it separates the one thing that cannot be
 * distinguished later: a queue manager that was never configured behaves
 * exactly like a channel that was configured wrongly — both are silence — and
 * that ambiguity has already cost this project four flash cycles.
 *
 * The queue used is the one the camera's endpoint will complete into.  Nothing
 * writes to it until a channel is enabled, so the test is self-contained, and
 * it exercises the queue that actually matters rather than a spare.
 */
static int cppi_selftest(void)
{
	u32 want = cppi_desc_phys(0);
	u32 got;

	/* Drain anything stale, so a leftover cannot pass for a success. */
	while (cppi_pop(CPPI_Q_USB1_EP1_RX_DONE))
		;

	cppi_push(CPPI_Q_USB1_EP1_RX_DONE, want);
	got = cppi_pop(CPPI_Q_USB1_EP1_RX_DONE);

	if (got != want) {
		printk("[CPPI] queue %d FAILED: pushed 0x%08lx, popped 0x%08lx\n",
		       CPPI_Q_USB1_EP1_RX_DONE,
		       (unsigned long)want, (unsigned long)got);
		return -1;
	}

	/* And it must be empty again, or the queue is not really consuming. */
	got = cppi_pop(CPPI_Q_USB1_EP1_RX_DONE);
	if (got) {
		printk("[CPPI] queue %d FAILED: still holds 0x%08lx\n",
		       CPPI_Q_USB1_EP1_RX_DONE, (unsigned long)got);
		return -1;
	}

	printk("[CPPI] queue %d ok: 0x%08lx pushed, popped, empty\n",
	       CPPI_Q_USB1_EP1_RX_DONE, (unsigned long)want);
	return 0;
}

/*
 * Bring the queue manager up.  Returns 0 if the self-test passed.
 *
 * @va is the already-translated USB subsystem base; this file never maps
 * anything, because the mapping is board knowledge and belongs where the rest
 * of it lives.
 */
int cppi_init(u32 va)
{
	u32 memctrl;

	cppi_va = va;

	/* Linking RAM: where, and how many descriptors it must account for. */
	wr(QMGR_LRAM0_BASE, kva_to_phys(cppi_lram));
	wr(QMGR_LRAM_SIZE,  CPPI_NDESC);
	wr(QMGR_LRAM1_BASE, 0);

	/*
	 * The descriptor region, encoded the way the hardware wants it: the
	 * index this region starts at, then log2 of the descriptor size and of
	 * the count, each less five.  Writing the sizes directly would be
	 * wrong by a factor nobody would notice until a queue handed back an
	 * address from the middle of a descriptor.
	 */
	memctrl = (0u << MEMCTRL_IDX_SH)
		| ((ilog2u(CPPI_DESC_SIZE) - 5u) << MEMCTRL_DESC_SH)
		| (ilog2u(CPPI_NDESC) - 5u);

	wr(QMGR_MEMBASE(0), kva_to_phys(cppi_descs));
	wr(QMGR_MEMCTRL(0), memctrl);

	printk("[CPPI] qmgr: %d descs of %d B at PA 0x%08lx, lram 0x%08lx,"
	       " memctrl 0x%08lx\n",
	       CPPI_NDESC, CPPI_DESC_SIZE,
	       (unsigned long)kva_to_phys(cppi_descs),
	       (unsigned long)kva_to_phys(cppi_lram),
	       (unsigned long)memctrl);

	return cppi_selftest();
}
