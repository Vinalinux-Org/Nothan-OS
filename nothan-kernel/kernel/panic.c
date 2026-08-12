/*
 * kernel/panic.c - last words
 *
 * With UART as the only debugging tool, what gets printed at the moment of
 * failure is the entire investigation.  A register dump alone has repeatedly
 * turned out not to be enough this month: it says an address faulted, but not
 * whose address it was, not what should have been there, and not what the
 * system was doing at the time.  Everything here exists to answer one of those.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <stdarg.h>
#include <asm/irqflags.h>
#include <nothan/types.h>
#include <nothan/panic.h>
#include <nothan/printk.h>
#include <nothan/sched.h>
#include <nothan/mm.h>

/*
 * The virtual memory map, in the order mmu_init() installs it — later entries
 * overwrite earlier ones where they overlap, so the search runs backwards to
 * report what the hardware would actually resolve.
 */
static const struct {
	unsigned long base;
	unsigned long size;
	const char *name;
} regions[] = {
	{ 0xC0000000UL, 0x20000000UL, "kernel direct map" },
	{ 0xF0000000UL, 0x02000000UL, "MMIO L4_PER"       },
	{ 0xF0E00000UL, 0x01000000UL, "MMIO L4_WKUP"      },
	{ 0xF2000000UL, 0x00200000UL, "MMIO L4_FAST"      },
	{ 0xF3000000UL, 0x00100000UL, "MMIO USB"          },
};

#define NR_REGIONS	(sizeof(regions) / sizeof(regions[0]))

static int region_of(unsigned long addr)
{
	int i;

	for (i = NR_REGIONS - 1; i >= 0; i--)
		if (addr >= regions[i].base &&
		    addr < regions[i].base + regions[i].size)
			return i;

	return -1;
}

/**
 * panic_describe_addr() - say what an address is, not just what it equals
 * @label: what the address is (e.g. "DFAR")
 * @addr: the address
 *
 * A bare hex number needs a human with the memory map in front of them.  This
 * turns it into a sentence.
 *
 * When the address belongs to no region, it tries flipping each bit in turn to
 * see whether a single-bit change would land inside one.  That is a guess and
 * is labelled as such — but it is the guess that matters, because a lone
 * corrupted bit is what marginal timing, a bad DRAM line and a torn pointer
 * all look like from here.  This kernel spent a long day on a fault at
 * 0x800fe664 that was kernel VA 0xC00fe664 with bit 30 lost; the machine could
 * have said so in one line.
 */
void panic_describe_addr(const char *label, unsigned long addr)
{
	int r = region_of(addr);
	unsigned int bit;

	if (r >= 0) {
		printk("  %s 0x%08lx: %s +0x%lx\n",
		       label, addr, regions[r].name, addr - regions[r].base);
		return;
	}

	if (addr < PAGE_SIZE) {
		printk("  %s 0x%08lx: null page — a small integer used as a"
		       " pointer\n", label, addr);
		return;
	}

	printk("  %s 0x%08lx: no mapped region\n", label, addr);

	for (bit = 0; bit < 32; bit++) {
		unsigned long guess = addr ^ (1UL << bit);
		int g = region_of(guess);

		if (g >= 0) {
			printk("    hint: bit %u flipped gives 0x%08lx (%s)\n",
			       bit, guess, regions[g].name);
			return;
		}
	}
}

/**
 * panic_dump_tasks() - who was running, and what else was runnable
 */
void panic_dump_tasks(void)
{
	struct task_struct *cur = runqueue.curr;
	int prio;

	if (!cur) {
		printk("  current: none (before first schedule)\n");
		return;
	}

	printk("  current: pid=%d \"%s\" prio=%d state=0x%x\n",
	       cur->pid, cur->comm, cur->prio, cur->__state);

	/*
	 * Kernel stack bounds, so "did it run off its own stack" is a question
	 * the dump answers rather than one it leaves open.  idle has a static
	 * stack and no kstack_base.
	 */
	if (cur->kstack_base)
		printk("  kstack:  0x%08lx..0x%08lx (sp now 0x%08lx)\n",
		       (unsigned long)cur->kstack_base,
		       (unsigned long)cur->kstack_base + PAGE_SIZE,
		       (unsigned long)cur->stack);

	printk("  runqueue: %u runnable\n", runqueue.nr_running);

	for (prio = 0; prio < MAX_PRIO; prio++) {
		struct list_head *pos;

		list_for_each(pos, &runqueue.active.queue[prio]) {
			struct sched_rt_entity *rt =
				list_entry(pos, struct sched_rt_entity, run_list);
			struct task_struct *t =
				container_of(rt, struct task_struct, rt);

			printk("    prio %2d: pid=%d \"%s\"\n",
			       prio, t->pid, t->comm);
		}
	}
}

void panic(const char *fmt, ...)
{
	va_list args;
	char buf[192];

	/*
	 * Nothing else runs from here on.  Not for safety — the machine is
	 * already wrong — but so the dump below describes one moment rather
	 * than a moving target.
	 */
	local_irq_disable();

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	printk("\n=== KERNEL PANIC ===\n");
	printk("  %s\n", buf);
	panic_dump_tasks();
	printk("=== halted ===\n");

	while (1)
		;
}
