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

/*
 * The USB subsystem's own idle control, and the reason none of the rest of
 * this file could work.
 *
 * A module on this bus has two ports: the slave port answers the CPU, and the
 * master port is what the module uses to reach DDR on its own.  MIDLEMODE
 * gates the master, so force-idle would have made this a perfectly
 * configurable device that could never move a byte by itself — which fit the
 * evidence exactly.  It reads 2, smart-idle: the master was free all along.
 *
 * The read stays because that was worth establishing and is worth not having
 * to establish again.  Nothing here writes it: it was never wrong, and
 * changing hardware state to match a hypothesis that has already been
 * disproved is how a driver accumulates settings nobody can justify.
 */
#define USBSS_SYSCONFIG		(CPPI_GLUE + 0x10)
#  define SYSC_SIDLE_SHIFT	2
#  define SYSC_MIDLE_SHIFT	4
#  define SYSC_IDLE_MASK	3u
#  define SYSC_IDLE_NONE	1u	/* never idle: master stays able to run */
#define CPPI_CTRL		0x2000
#define CPPI_SCHED		0x3000
#define CPPI_QMGR		0x4000

/*
 * The channel controller and the scheduler.
 *
 * The scheduler is a fixed round-robin table: every entry names a channel and
 * a direction, and the engine walks it forever.  Nothing is dynamic about it —
 * an endpoint gets its turn because its channel appears in the table, and a
 * channel that is disabled simply has nothing to do when its turn comes.
 */
#define DMA_RXGCR(x)		(CPPI_CTRL + 0x808 + (x) * 0x20)
/*
 * Where a receive channel takes its empty buffers from.
 *
 * The completion queue in RXGCR tells the channel where to report; this tells
 * it where to fetch.  Without it the channel is enabled, correctly configured,
 * pointed at the right completion queue — and has no idea that the descriptors
 * waiting on queue 16 are meant for it.  It never fetches, so it never
 * reports, and that silence is indistinguishable from a channel that was never
 * turned on, which is what made it cost eight flash cycles to find.
 *
 * It is missing from the vendor's channel-configure path, where every other
 * setting lives.  It is written once when a channel is allocated, and again on
 * resume — two places that look like housekeeping rather than setup.
 */
#define DMA_RXHPCRA(x)		(DMA_RXGCR(x) + 0x04)
#  define GCR_CHAN_ENABLE	(1u << 31)
#  define GCR_STARV_RETRY	(1u << 24)
#  define GCR_DESC_TYPE_HOST	(1u << 14)

/*
 * The teardown free-descriptor queue.  Nothing here tears anything down yet,
 * but the engine is told where teardown descriptors live as part of coming up,
 * and it was the one line of the vendor's init sequence this file had skipped.
 */
#define DMA_TDFDQ		(CPPI_CTRL + 0x04)
#define CPPI_Q_TEARDOWN		31

#define DMA_SCHED_CTRL		(CPPI_SCHED + 0x00)
#  define SCHED_CTRL_EN		(1u << 31)
#define DMA_SCHED_WORD(x)	(CPPI_SCHED + 0x800 + (x) * 4)
#  define SCHED_ENTRY_IS_RX	0x80u

/* How many channels this controller has; am33xx.dtsi says dma-channels = 30. */
#define CPPI_NCHAN		30

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
 * One bit per queue, thirty-two queues to a word: whether anything is waiting.
 * Reading this is how completion is noticed without an interrupt, and without
 * popping a queue that may be empty.
 */
#define QMGR_PEND(x)		(CPPI_QMGR + 0x90 + (x) * 4)

/*
 * Host descriptor, eight words.  The fields are the vendor driver's, and the
 * two that look redundant are not: pd4 and pd7 both carry the buffer address
 * because one belongs to the packet and one to the buffer, and the engine uses
 * them for different things when descriptors are chained.
 */
#define DESC_TYPE_HOST_SH	27
#define DESC_TYPE_HOST		0x10u
#define DESC_TYPE_USB		(5u << 26)
#define DESC_PD_COMPLETE	(1u << 31)
#define PD2_ZERO_LENGTH		(1u << 19)

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

/*
 * Page-aligned, not descriptor-aligned.
 *
 * The vendor gets both of these regions from the coherent allocator, which
 * hands back page-aligned memory that the CPU does not cache.  A .bss array
 * aligned only to a descriptor is a quieter thing than it looks: the queue
 * manager indexes a descriptor by its offset from the region base, and a base
 * the hardware did not expect to be arbitrary is the kind of assumption that
 * survives a software push and pop — which is all stage 1 proved — and fails
 * the moment the engine itself walks the region.
 */
static u8 cppi_descs[CPPI_NDESC * CPPI_DESC_SIZE]
	__attribute__((aligned(4096)));

/*
 * Linking RAM: one word per descriptor, which the queue manager uses to thread
 * them together.  It is the engine's own scratch space — nothing here ever
 * reads it — but leaving it unset means every queue operation writes through a
 * null pointer inside the hardware.
 */
static u32 cppi_lram[CPPI_NDESC] __attribute__((aligned(4096)));

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

/* The raw descriptor words, for when the length looks wrong. */
void cppi_desc_dump(unsigned int i, const char *tag)
{
	u32 *d = (u32 *)cppi_desc(i);

	invalidate_dcache_range((unsigned long)d,
				(unsigned long)d + CPPI_DESC_SIZE);
	printk("[CPPI] %s desc%u: %08x %08x %08x %08x %08x %08x %08x %08x\n",
	       tag, i, (unsigned int)d[0], (unsigned int)d[1],
	       (unsigned int)d[2], (unsigned int)d[3], (unsigned int)d[4],
	       (unsigned int)d[5], (unsigned int)d[6], (unsigned int)d[7]);
}

int cppi_desc_index(u32 desc_phys)
{
	u32 base = cppi_desc_phys(0);
	u32 off;

	if (desc_phys < base)
		return -1;

	off = desc_phys - base;
	if (off % CPPI_DESC_SIZE || off / CPPI_DESC_SIZE >= CPPI_NDESC)
		return -1;

	return (int)(off / CPPI_DESC_SIZE);
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

int cppi_queue_pending(unsigned int queue)
{
	return (rd(QMGR_PEND(queue / 32u)) >> (queue % 32u)) & 1u;
}

u32 cppi_desc_prep_rx(unsigned int i, u32 buf_phys, unsigned int len,
		      unsigned int done_queue)
{
	u32 *d = (u32 *)cppi_desc(i);

	d[0] = (DESC_TYPE_HOST << DESC_TYPE_HOST_SH) | len;
	d[1] = 0;
	d[2] = DESC_TYPE_USB | done_queue;	/* where to report back */
	d[3] = len;				/* packet size */
	d[4] = buf_phys;
	d[5] = 0;
	d[6] = DESC_PD_COMPLETE | len;		/* buffer size */
	d[7] = buf_phys;

	/*
	 * The descriptor is ordinary cached memory and the engine reads it
	 * from DDR, so it has to be pushed out before the address is offered.
	 * cppi_push() has the barrier; this only writes.
	 */
	flush_dcache_range((unsigned long)d, (unsigned long)d + CPPI_DESC_SIZE);

	return cppi_desc_phys(i);
}

unsigned int cppi_desc_len(unsigned int i)
{
	u32 *d = (u32 *)cppi_desc(i);

	/*
	 * The engine wrote this descriptor back, so the CPU's cached copy is
	 * stale — invalidate before believing any of it.  Forgetting this
	 * reads the length that was *asked for* rather than the one delivered,
	 * which is the same number on a full packet and quietly wrong on every
	 * short one.
	 */
	invalidate_dcache_range((unsigned long)d,
				(unsigned long)d + CPPI_DESC_SIZE);

	/*
	 * Twenty-two bits, and the zero-length flag beats them.  The vendor's
	 * mask is (1 << (21 + 1)) - 1 — the constant is named for the field's
	 * top bit, not its width, which is the kind of off-by-one that only
	 * shows up on lengths nothing here will ever send.
	 */
	if (d[2] & PD2_ZERO_LENGTH)
		return 0;

	return d[0] & ((1u << 22) - 1u);
}

/*
 * Turn a receive channel on.
 *
 * STARV_RETRY matters for isochronous: if the engine finds no descriptor
 * waiting when a packet arrives it retries rather than giving up on the
 * channel, which is the difference between dropping one packet and stopping
 * the stream.  It is exactly the failure the polled path had to be rescued
 * from with a watchdog.
 */
void cppi_rx_channel_open(unsigned int chan, unsigned int free_queue,
			  unsigned int done_queue)
{
	/*
	 * Both halves of the channel's world, then the enable.
	 *
	 * Only the enable bit of RXGCR reads back — the rest of the register is
	 * write-only, which was established the expensive way — so there is
	 * nothing to verify here and no point printing it.  What matters is
	 * that the fetch side is configured at all: it was missing for eight
	 * flash cycles, and its absence looked exactly like everything else
	 * being wrong.
	 */
	wr(DMA_RXHPCRA(chan), free_queue);
	wr(DMA_RXGCR(chan), GCR_CHAN_ENABLE | GCR_STARV_RETRY
			    | GCR_DESC_TYPE_HOST | done_queue);
}

/*
 * An enabled channel with nothing on its free queue is worse than a disabled
 * one: the controller still hands it packets and it drops them, silently and
 * at full rate.  That is what halved the frame rate the first time this ran —
 * the packet count was unchanged and half the bytes were gone, which is the
 * signature of data going somewhere that does not count it.
 */
void cppi_rx_channel_close(unsigned int chan)
{
	wr(DMA_RXGCR(chan), 0);
}

/*
 * What the engine thinks its own state is.
 *
 * Every impasse in this driver has ended the same way: a measurement, not an
 * argument.  "No completion" has at least three causes that look identical from
 * the completion queue — the channel never ran, the scheduler never gave it a
 * turn, or it ran and found nothing to do — and these two registers separate
 * them.  A channel enable that does not read back means the write did not take;
 * a scheduler that is not running means no channel gets a turn at all.
 */
void cppi_dump(unsigned int chan)
{
	printk("[CPPI] rxgcr[%u]=%08x sched=%08x\n", chan,
	       (unsigned int)rd(DMA_RXGCR(chan)),
	       (unsigned int)rd(DMA_SCHED_CTRL));
}

/*
 * The round-robin table the engine walks.
 *
 * Every channel gets an entry in both directions, whether or not it is enabled
 * — a disabled channel simply has nothing to do when its turn comes, and
 * building the table around which endpoints happen to be open would mean
 * rewriting it every time one opens.  Four entries to a word, so fifteen words
 * cover thirty channels.
 */
static void cppi_sched_init(void)
{
	unsigned int ch, word = 0;

	wr(DMA_SCHED_CTRL, 0);

	for (ch = 0; ch < CPPI_NCHAN; ch += 2) {
		u32 reg = (u32)ch
			| ((u32)(ch | SCHED_ENTRY_IS_RX) << 8)
			| ((u32)(ch + 1) << 16)
			| ((u32)((ch + 1) | SCHED_ENTRY_IS_RX) << 24);

		wr(DMA_SCHED_WORD(word++), reg);
	}

	/* Last entry index, then run. */
	wr(DMA_SCHED_CTRL, (CPPI_NCHAN * 2u - 1u) | SCHED_CTRL_EN);
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

	/*
	 * Both regions are written by the engine and live in ordinary cached
	 * memory, so every line the CPU still holds over them has to go before
	 * the hardware is told they exist.  Zeroing .bss at boot dirtied them;
	 * an eviction afterwards would put those zeroes back on top of what the
	 * engine had written, at whatever moment the cache chose — which is the
	 * kind of fault that looks like the hardware being unreliable.  Nothing
	 * reads the linking RAM from software, so cleaning it once here is the
	 * only handling it needs.
	 */
	/*
	 * Ungate the master port before anything is told to use it.  Printed
	 * before and after, because "the module was already able to do this"
	 * and "it was not" are different findings and only one of them makes
	 * this write the fix.
	 */
	{
		u32 sysc = rd(USBSS_SYSCONFIG);

		printk("[CPPI] usbss sysconfig %08x (midle %u, smart = master"
		       " already free to run)\n", (unsigned int)sysc,
		       (unsigned int)((sysc >> SYSC_MIDLE_SHIFT)
				      & SYSC_IDLE_MASK));
	}

	flush_dcache_range((unsigned long)cppi_descs,
			   (unsigned long)cppi_descs + sizeof(cppi_descs));
	flush_dcache_range((unsigned long)cppi_lram,
			   (unsigned long)cppi_lram + sizeof(cppi_lram));

	wr(DMA_TDFDQ, CPPI_Q_TEARDOWN);
	cppi_sched_init();

	printk("[CPPI] qmgr: %d descs of %d B at PA 0x%08lx, lram 0x%08lx,"
	       " memctrl 0x%08lx\n",
	       CPPI_NDESC, CPPI_DESC_SIZE,
	       (unsigned long)kva_to_phys(cppi_descs),
	       (unsigned long)kva_to_phys(cppi_lram),
	       (unsigned long)memctrl);

	return cppi_selftest();
}
