/*
 * kernel/cachebench.c - infer the cache hierarchy by measuring it
 *
 * The AM335x TRM (Ch03) says this Cortex-A8 has a 32 KB L1 D-cache and a
 * 256 KB L2.  Whether the L2 is actually *enabled* is a different question,
 * and one no document answers:
 *
 *   - u-boot never turns it on: v7_outer_cache_enable() is an empty weak stub
 *     and omap enable_caches() only does L1 I/D.
 *   - On AM335x the Auxiliary Control Register and the L2 Cache Auxiliary
 *     Control Register are reachable only through a secure monitor call
 *     (TRM Ch03, service IDs 0x100 and 0x102).  Reading them from non-secure
 *     state may fault, so even asking is not free.
 *
 * So measure the effect rather than interrogate the state: walk working sets
 * placed on either side of each boundary and compare cost per access.
 *
 * ANSWERED 2026-08-12, at 600 MHz on a stable rail:
 *
 *      8/16/32 KB    3.00 / 3.00 / 3.05   L1 hit
 *      64/128/256    9.42 / 11.02 / 33.54 L2 hit
 *      512 KB - 4 MB 113.7 -> 150.8       DDR, ~250 ns
 *
 * Both knees land exactly where the TRM says they should — 32->64 KB and
 * 256->512 KB — so the L2 is enabled and doing its job.  There is nothing to
 * fix here, and in particular no reason to go near the secure monitor call.
 *
 * Worth keeping the file for the method, not the numbers: the first attempt at
 * this measurement ran on a CPU that was undervolted (1 GHz on the 1.100 V
 * power-on rail) and produced a non-monotonic curve that invited an entirely
 * wrong conclusion.  What exposed that was not the timings but the chain-length
 * check below.
 *
 * The walk is a pointer chase over a pseudo-random permutation of cache lines,
 * not a linear sweep.  Each load's address is the previous load's data, so the
 * hardware can neither prefetch ahead nor overlap misses.  A linear sweep would
 * measure the prefetcher rather than the cache and would show almost no
 * difference between the sizes.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/config.h>
#include <nothan/types.h>
#include <nothan/mm.h>
#include <nothan/mmio.h>
#include <nothan/timer.h>
#include <nothan/printk.h>

#define BENCH_LINE	64u			/* D-cache line, TRM Ch03 */
#define BENCH_STRIDE	(BENCH_LINE / 4u)	/* line stride in u32 units */
#define BENCH_ORDER	10u			/* 2^10 pages = 4 MB, = MAX_ORDER */
#define BENCH_BYTES	(1u << 22)		/* 4 MB */
#define BENCH_ACCESSES	(1u << 16)		/* power of two: divide becomes a shift */

#define TSC_MHZ		24u

/*
 * MPU frequency is not a constant this file may assume: the bootloader picks
 * it at runtime, raising the DPLL to 1 GHz only if the PMIC confirmed the
 * 1.325 V rail, and staying at 600 MHz otherwise.  Hardcoding either value
 * would silently scale every number below by 1.67x on the boots that went the
 * other way.  Read the DPLL instead.
 *
 *      MPU = M / (N+1) * 24 MHz / M2
 *
 * CM_WKUP is at PA 0x44E00400 -> VA 0xF0E00400 (TRM Ch08, and the same
 * offsets u-boot uses in dpll_mpu_regs).
 */
#define CM_CLKSEL_DPLL_MPU	0xF0E0042C
#define CM_DIV_M2_DPLL_MPU	0xF0E004A8

static u32 mpu_clk_mhz(void)
{
	u32 clksel = mmio_read32(CM_CLKSEL_DPLL_MPU);
	u32 m = (clksel >> 8) & 0x7FF;
	u32 n = clksel & 0x7F;
	u32 m2 = mmio_read32(CM_DIV_M2_DPLL_MPU) & 0x1F;

	if (!m2)
		return 0;

	return 24u * m / (n + 1u) / m2;
}

/*
 * Powers of two only: the LCG masks with (nlines - 1), which is a valid
 * modulus only for a power-of-two line count.  A finer grid (48 KB, 192 KB)
 * would need a different permutation generator and is not worth it — the knees
 * are what matter, and a doubling grid brackets each one to within 2x.
 *
 * The first sweep used 16/128/512/4096 KB and produced a shape that does not
 * match the documented 32 KB L1 + 256 KB L2: 128 KB came out at the same cost
 * as 16 KB, and 512 KB came out far cheaper than a 256 KB cache holding a
 * 512 KB cyclic working set can explain.  This grid exists to locate the real
 * knees before anyone interprets the numbers.
 */
static const u32 bench_bytes[] = {
	  8u << 10,   16u << 10,   32u << 10,   64u << 10,  128u << 10,
	256u << 10,  512u << 10, 1024u << 10, 2048u << 10, 4096u << 10,
};

/* Consumes the chase result so the compiler cannot delete the loop. */
static volatile u32 bench_sink;

/*
 * Link every cache line into one cycle that visits all of them exactly once.
 * x -> 5x+1 (mod 2^k) has full period (Hull-Dobell: the increment is odd and
 * a-1 = 4 is divisible by 4), and unlike a fixed stride its address deltas
 * vary, so a stride prefetcher cannot lock onto the pattern.
 */
static void bench_build(u32 *buf, u32 nlines)
{
	u32 cur = 0;
	u32 i;

	for (i = 0; i < nlines; i++) {
		u32 next = (5u * cur + 1u) & (nlines - 1u);

		buf[cur * BENCH_STRIDE] = next * BENCH_STRIDE;
		cur = next;
	}
}

static u32 bench_chase(const u32 *buf, u32 accesses)
{
	u32 idx = 0;
	u32 i;

	for (i = 0; i < accesses; i++)
		idx = buf[idx];

	return idx;
}

/*
 * Walk the chain from 0 and count steps until it returns to 0.
 *
 * bench_build() constructs a single cycle through every line, so a healthy
 * chain returns exactly @nlines.  Anything shorter means a link was
 * overwritten and the walk fell into a sub-cycle — which silently turns the
 * measurement into "a much smaller working set", reporting cache hits that
 * were never tested.  Returns 0 if the walk left the buffer entirely.
 */
static u32 bench_chain_len(const u32 *buf, u32 nlines)
{
	u32 limit = nlines * BENCH_STRIDE;
	u32 idx = buf[0];
	u32 steps = 1;

	while (idx != 0) {
		if (idx >= limit || (idx % BENCH_STRIDE) != 0)
			return 0;		/* left the buffer or lost alignment */
		if (steps > nlines)
			return steps;		/* cycle that never comes back to 0 */
		idx = buf[idx];
		steps++;
	}

	return steps;
}

/*
 * Write a value derived from each word's own index across the whole buffer,
 * then read it back.  Any mismatch means the DRAM did not keep what was put
 * there, or something else wrote into a region the allocator claimed was ours.
 *
 * The XOR of expected against actual is the interesting part: a single bit
 * repeating across failures points at one data line, whereas scattered bits
 * point at a range being reused by something else.
 */
#define BENCH_PATTERN(i)	((i) * 0x9E3779B9u)

static void bench_memtest(u32 *buf, u32 words)
{
	u32 errors = 0;
	u32 first_i = 0, first_want = 0, first_got = 0;
	u32 or_diff = 0;
	u32 i;

	for (i = 0; i < words; i++)
		buf[i] = BENCH_PATTERN(i);

	for (i = 0; i < words; i++) {
		u32 got = buf[i];
		u32 want = BENCH_PATTERN(i);

		if (got != want) {
			if (!errors) {
				first_i = i;
				first_want = want;
				first_got = got;
			}
			or_diff |= (got ^ want);
			errors++;
		}
	}

	if (!errors) {
		printk("[MEMTEST] %lu KB: clean\n", (unsigned long)(words >> 8));
		return;
	}

	printk("[MEMTEST] %lu KB: %lu WORDS BAD\n",
	       (unsigned long)(words >> 8), (unsigned long)errors);
	printk("[MEMTEST]   first at word %lu (VA %p): want %08lx got %08lx\n",
	       (unsigned long)first_i, (void *)&buf[first_i],
	       (unsigned long)first_want, (unsigned long)first_got);
	printk("[MEMTEST]   OR of all bit differences: %08lx\n",
	       (unsigned long)or_diff);
}

/*
 * Returns CPU cycles per access, scaled by 100.
 *
 * The arithmetic avoids any 64-bit division: the kernel links -nostdlib with
 * no libgcc, so __aeabi_uldivmod would not resolve.  BENCH_ACCESSES is 2^16,
 * so that division is a shift, and the remaining divide by 24 is a 32-bit
 * divide by a constant, which the compiler turns into a reciprocal multiply.
 */
static u32 bench_one(u32 *buf, u32 bytes, u32 cpu_mhz, u32 *ticks_out, u32 *len_out)
{
	u32 nlines = bytes / BENCH_LINE;
	u64 t0, t1;
	u32 ticks;

	bench_build(buf, nlines);

	/* One full pass first: measure steady state, not cold-start misses. */
	bench_sink += bench_chase(buf, nlines);

	t0 = timer_cycles();
	bench_sink += bench_chase(buf, BENCH_ACCESSES);
	t1 = timer_cycles();

	ticks = (u32)(t1 - t0);
	*ticks_out = ticks;
	*len_out = bench_chain_len(buf, nlines);

	return (u32)(((u64)ticks * (100u * cpu_mhz)) >> 16) / TSC_MHZ;
}

/**
 * cache_bench() - report measured cost per access across the hierarchy
 *
 * Call before the scheduler tick starts.  The tick would otherwise land inside
 * the timed region and inflate whichever size happened to be running.
 */
void cache_bench(void)
{
	struct zone *zone = get_zone();
	struct page *pg;
	u32 *buf;
	u32 cpu_mhz;
	unsigned int i;

	if (!CONFIG_CACHE_BENCH)
		return;

	cpu_mhz = mpu_clk_mhz();
	if (!cpu_mhz) {
		printk("[CACHE] MPU DPLL reads back as 0 MHz — benchmark skipped\n");
		return;
	}

	pg = alloc_pages(GFP_KERNEL, BENCH_ORDER);
	if (!pg) {
		printk("[CACHE] alloc %u KB failed — benchmark skipped\n",
		       BENCH_BYTES >> 10);
		return;
	}

	buf = phys_to_kva(page_to_phys(zone, pg));

	printk("[CACHE] buffer %lu KB at PA 0x%lx (VA %p), pool base PA 0x%lx\n",
	       (unsigned long)(BENCH_BYTES >> 10),
	       (unsigned long)page_to_phys(zone, pg), (void *)buf,
	       (unsigned long)zone->base_pa);

	/* Integrity before timing: a number measured on memory that does not
	 * hold what was written to it means nothing. */
	bench_memtest(buf, BENCH_BYTES / 4u);

	printk("[CACHE] pointer-chase, %lu accesses per size, MPU %lu MHz (read from DPLL)\n",
	       (unsigned long)BENCH_ACCESSES, (unsigned long)cpu_mhz);

	for (i = 0; i < sizeof(bench_bytes) / sizeof(bench_bytes[0]); i++) {
		u32 ticks = 0, len = 0;
		u32 nlines = bench_bytes[i] / BENCH_LINE;
		u32 x100 = bench_one(buf, bench_bytes[i], cpu_mhz, &ticks, &len);

		printk("[CACHE] %6lu KB : %4lu.%02lu cyc/access  (%lu ticks, %lu us)%s\n",
		       (unsigned long)(bench_bytes[i] >> 10),
		       (unsigned long)(x100 / 100u),
		       (unsigned long)(x100 % 100u),
		       (unsigned long)ticks,
		       (unsigned long)cycles_to_us(ticks),
		       (len == nlines) ? "" : "   <-- CHAIN BROKEN");

		if (len != nlines)
			printk("[CACHE]          chain visits %lu of %lu lines"
			       " — this number measures a smaller set than it claims\n",
			       (unsigned long)len, (unsigned long)nlines);
	}

	/* Re-test the same memory afterwards: a clean run before and a dirty
	 * run after would mean the damage happened while we were using it. */
	bench_memtest(buf, BENCH_BYTES / 4u);

	__free_pages(pg, BENCH_ORDER);
}
