/*
 * kernel/spawn.c - Task creation and kernel thread setup
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/slab.h>
#include <nothan/printk.h>
#include <nothan/fs.h>
#include <nothan/syscall.h>
#include <nothan/delay.h>
#include <nothan/wait.h>
#include <nothan/panic.h>
#include <asm/irqflags.h>

/* No <limits.h> in a freestanding build. */
#define PID_MAX_VALUE		0x7FFFFFFF

static int next_pid = 1;

/**
 * pid_alloc() - hand out the next unused PID
 *
 * next_pid++ is a read-modify-write: load, add, store.  Two creators that
 * interleave between the load and the store both read the same value and both
 * hand it out, so two live tasks share a PID - and pid_hash then resolves that
 * number to whichever one hashed last, making kill() and wait() hit the wrong
 * process.
 *
 * Today this cannot happen by luck, not by design: every task is created from
 * kernel_main at boot, sequentially, before anything else runs.  It becomes
 * live the day spawn() is a syscall and two processes can create at once.
 * Fixed now because the fix is three lines and the failure mode - a signal
 * delivered to the wrong process - is one UART cannot explain.
 *
 * Not reused after a task dies: a recycled PID lets a parent's wait(pid)
 * collect a brand-new process that merely inherited the number. That is
 * affordable, not free - never reusing means the space is consumed forever, so
 * the counter must be allowed to RUN OUT rather than wrap.
 *
 * Wrapping is the dangerous outcome, not exhaustion. Past INT_MAX the counter
 * goes negative; sys_kill's "pid <= 1" guard would then treat every task as
 * protected, pid_hash would still bucket on (unsigned)pid, and PIDs would
 * silently start colliding with live ones. Refusing to allocate is loud and
 * recoverable; wrapping is silent and wrong.
 *
 * Return: a fresh PID, or -1 when the space is gone.
 */
static int pid_alloc(void)
{
	unsigned long flags;
	int pid;

	local_irq_save(flags);
	pid = (next_pid == PID_MAX_VALUE) ? -1 : next_pid++;
	local_irq_restore(flags);

	if (pid < 0)
		pr_err("[SPAWN] PID space exhausted (never reused; %d handed out)\n",
		       PID_MAX_VALUE);

	return pid;
}

static void task_exit(void)
{
	do_exit(0);
}

/**
 * parent_for_new_task() - who owns a task about to be created
 *
 * The creator, normally. The exception is PID 0.
 *
 * PID 0 is not a process. It is the scheduler's fallback - the thing that runs
 * when nothing else can - and it is also whatever kernel_main() happens to be
 * executing as during boot. Letting it be a parent would put a non-process at
 * the root of the process tree and make "the tree" mean two different things.
 * Tasks created during boot are therefore children of init.
 *
 * That fallback is only sound because init exists by the time anything else is
 * created: kernel_main() builds it before do_initcalls(), so even the kernel
 * threads a driver probe starts have a real parent. Create init any later and
 * every one of those would be born with a NULL parent - not crashing, because
 * reap_dead() checks, but unreapable, holding a PID for the rest of the boot.
 *
 * init itself is the sole NULL: it IS the root, and reparent_to_init() refuses
 * to touch it.
 */
static struct task_struct *parent_for_new_task(void)
{
	struct task_struct *cur = runqueue.curr;

	if (cur && cur->pid > 0)
		return cur;

	return init_task;	/* boot-time creation, or PID 0; NULL before init exists */
}

/* init's program image. NOT in blob_table - see the comment there. */
extern char _binary_user_init_start[];
extern char _binary_user_init_end[];

/* Defined below; init_task_create() is the one caller that runs before it. */
struct task_struct *user_task_create_bin(const char *name,
					 char *blob_start, char *blob_end);

/**
 * init_task_create() - create PID 1
 *
 * Must be the FIRST task created, because PIDs are handed out in order and
 * init is defined by its number. Creating anything before it would hand PID 1
 * to that task instead, and sys_kill's "protect pid <= 1" guard would then be
 * shielding whatever happened to be created first rather than init.
 *
 * init is a USER PROCESS, not a kernel thread, and this is the one place a
 * user process is created without a running process asking for it. Everything
 * else reaches user_task_create_bin() through sys_spawn, i.e. because some
 * process called spawn(); init cannot, because at this point in boot there is
 * no process to do the calling. That is the whole of the bootstrap: one
 * hard-coded creation, and from there user space starts user space.
 *
 * What init DOES - which services to start, and reaping forever - lives in
 * userspace/init/main.c. The kernel no longer knows or decides.
 */
struct task_struct *init_task_create(void)
{
	struct task_struct *p = user_task_create_bin("init",
						     _binary_user_init_start,
						     _binary_user_init_end);
	if (!p)
		return NULL;

	init_task = p;
	p->parent = NULL;	/* the root; nobody adopts init */

	return p;
}

extern void task_entry(void);

/**
 * task_create() - Create a new kernel thread
 * @fn: Function pointer to the thread entry
 * @prio: Priority of the task
 * @name: Task name for debugging
 *
 * Return: Pointer to the newly created task_struct, or NULL on failure.
 */
struct task_struct *task_create(void (*fn)(void), int prio, const char *name)
{
	int pid = pid_alloc();

	if (pid < 0)			/* PID space gone — pid_alloc already said so */
		return NULL;

	struct task_struct *p = (struct task_struct *)kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return NULL;

	unsigned long *sp = (unsigned long *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!sp) {
		kfree(p);
		return NULL;
	}
	void *kstack_base = sp;	/* keep the allocation base for kfree on exit */

	/*
	 * Fill the stack from the top, matching __switch_to layout:
	 *   stmfd sp!, {r4-r11, lr}  →  [sp+0]=r4 … [sp+32]=lr
	 *   ldmfd sp!, {r4-r11, pc}  →  r4=[sp+0], pc=[sp+32]
	 *
	 * r4=fn, r5=task_exit (return address), r6-r11=0, lr=task_entry
	 */
	sp = (unsigned long *)((char *)sp + PAGE_SIZE);

	*--sp = (unsigned long)task_entry;	/* lr → pc on context restore */
	*--sp = 0;
	*--sp = 0;
	*--sp = 0;
	*--sp = 0;
	*--sp = 0;
	*--sp = 0;
	*--sp = (unsigned long)task_exit;	/* r5: return address for fn */
	*--sp = (unsigned long)fn;		/* r4: first argument to task_entry */

	p->stack       = sp;
	p->kstack_base = kstack_base;
	p->user_sp    = 0;
	p->user_lr    = 0;
	p->__state    = TASK_RUNNING;
	p->flags      = 0;
	p->pid        = pid;
	p->prio       = prio;
	p->rt.on_rq   = 0;
	p->exit_how   = EXIT_HOW_EXITED;
	p->exit_value = 0;
	p->parent     = parent_for_new_task();
	p->mm         = NULL;
	p->files      = NULL;	/* kernel thread: opens nothing, so owns no table */
	p->refcount   = 1;	/* existence ref; dropped at reap */
	list_init(&p->wait_node);	/* empty = on no wait queue */
	init_waitqueue_head(&p->child_wait);
	/* CFS-lite: start at the current min_vruntime so a fresh task can't hog
	 * the CPU with a stale-low vruntime (place_entity, initial case). */
	p->rt.vruntime = runqueue.min_vruntime;
	p->rt.exec_start = 0;
	p->rt.sum_exec_runtime = 0;
	p->rt.prev_sum_exec_runtime = 0;
	p->cwd[0]     = '/';
	p->cwd[1]     = '\0';

	unsigned int i = 0;
	for (; i < 15 && name[i]; i++)
		p->comm[i] = name[i];
	p->comm[i] = '\0';

	task_register(p);	/* memory-bound registry — cannot fail */

	return p;
}

extern void user_task_trampoline(void);

/* USER_STACK_TOP lives in <nothan/mm.h>: mmu_map_user() and access_ok() must
 * agree with this file about where the stack region is, and a constant defined
 * in one .c file is how they stop agreeing. */

/*
 * init is the ONLY program embedded in the kernel image (userspace_blobs.S).
 * Every other program is a file on disk, loaded by path - the kernel keeps no
 * list of what exists. init is the exception because it is the bootstrap:
 * nothing is running yet to ask for it.
 */

/*
 * NothanOS user binary header — see userspace/lib/user.lds.
 * First 16 bytes of every .bin: kernel reads bss_size to allocate
 * zeroed pages, then jumps to _start at offset 0x10.
 */
#define USER_BIN_MAGIC   0x4E4F5348      /* 'NOSH' */
#define USER_BIN_ENTRY   0x00010010      /* _start (after 16-byte header) */

struct user_bin_header {
	u32 magic;
	u32 bss_size;
	u32 reserved[2];
};

/*
 * struct bin_source - where a program image is being read from
 * @mem:  embedded blob in kernel memory, or NULL
 * @fd:   open file, or -1
 * @size: image length in bytes, known before any of it is read
 *
 * Two sources, one loader. init comes from an embedded blob because at the
 * moment it is created there is no process to have asked for it and no
 * guarantee a filesystem is usable; everything else comes from a file, because
 * which programs exist is not the kernel's business.
 *
 * @size is a field rather than something the loader discovers as it reads:
 * user pages are physically contiguous, so the page count must be right BEFORE
 * the first byte is copied.
 */
struct bin_source {
	const char   *mem;
	int           fd;
	unsigned long size;
};

/*
 * Lay out argc/argv at the top of a freshly allocated user stack, UNIX-style.
 *
 *   USER_STACK_TOP -> [ "prog\0" "arg1\0" ... ]   strings, packed at the top
 *                     [ padding to 8 ]
 *                     [ NULL         ]            argv[argc]
 *                     [ &"arg1"      ]            argv[1]
 *                     [ &"prog"      ]            argv[0]
 *   initial sp     -> [ argc         ]
 *
 * argc sits AT sp and argv[0] immediately above it, so _start can read the two
 * with an ldr and an add (see lib/crt0.S). The block base is 8-aligned because
 * AAPCS requires that of sp at a call boundary, and _start calls main().
 *
 * @stack_kva is the kernel mapping of the same pages the task will see at
 * [USER_STACK_TOP - @bytes, USER_STACK_TOP): the task is not running and its
 * page tables are not live yet, so the block is written through the kernel's
 * direct map and the user addresses are computed, not dereferenced.
 *
 * @argv entries are read, never written, and are valid in the CURRENT context:
 * kernel strings for init, the caller's own strings for sys_spawn - which the
 * caller cannot change underneath us, being blocked in the syscall and having
 * no second thread to do it.
 *
 * Return: initial user sp, or 0 if the block would exceed ARGV_MAX_BYTES.
 */
static unsigned long setup_argv_block(u8 *stack_kva, unsigned long bytes,
				      int argc, const char *const *argv)
{
	unsigned long str_bytes = 0;

	for (int i = 0; i < argc; i++) {
		unsigned long len = 0;

		while (argv[i][len])
			len++;
		str_bytes += len + 1;		/* keep the NUL */
	}

	/* argc + (argc+1) pointers, rounded so the base lands 8-aligned. */
	unsigned long block = ((unsigned long)(argc + 2) * 4 + 7) & ~7UL;

	if (block + str_bytes > ARGV_MAX_BYTES || block + str_bytes > bytes)
		return 0;

	unsigned long stack_bottom_uva = USER_STACK_TOP - bytes;
	unsigned long off = bytes;		/* offset of USER_STACK_TOP */

	/* Strings first, at the very top, so their addresses are known before
	 * the pointer array that has to hold them is written. */
	unsigned long uva[ARGV_MAX_COUNT];

	for (int i = argc - 1; i >= 0; i--) {
		unsigned long len = 0;

		while (argv[i][len])
			len++;
		len++;				/* the NUL travels too */

		off -= len;
		for (unsigned long j = 0; j < len; j++)
			stack_kva[off + j] = (u8)argv[i][j];
		uva[i] = stack_bottom_uva + off;
	}

	off &= ~7UL;				/* strings start 8-aligned */
	off -= block;				/* => sp is 8-aligned too */

	u32 *w = (u32 *)(stack_kva + off);

	w[0] = (u32)argc;
	for (int i = 0; i < argc; i++)
		w[1 + i] = (u32)uva[i];
	w[1 + argc] = 0;			/* argv[argc] = NULL */

	return stack_bottom_uva + off;
}

/*
 * alloc_region() - back @pages of a user region with scattered buddy blocks
 * @chunks/@nr: where to record what was taken (caller pre-zeroes @nr)
 * @pages: how many 4 KB pages the region needs
 * @zero: fill the pages with zeroes (bss wants this; code is overwritten)
 *
 * Takes the largest buddy block that still fits the remainder, backing the
 * order off when the allocator cannot satisfy it. That back-off is the point:
 * a region built this way needs enough FREE memory, not one unbroken run of
 * it, so a fragmented system can still start a program.
 *
 * Failure leaves the chunks recorded so far in place - the caller frees them
 * with mm_free_chunks() on its way out, rather than this having to unwind.
 *
 * Return: 0, or -1 if out of memory or the region needs more than MM_MAX_CHUNKS.
 */
static int alloc_region(struct mm_chunk *chunks, unsigned int *nr,
			unsigned int pages, int zero, struct zone *zone)
{
	unsigned int remaining = pages;

	while (remaining > 0) {
		if (*nr >= MM_MAX_CHUNKS)
			return -1;	/* too fragmented, or region far too big */

		unsigned int ord = 0;

		while ((1u << (ord + 1)) <= remaining && ord < MAX_ORDER)
			ord++;

		struct page *pg = alloc_pages(GFP_USER, ord);

		while (!pg && ord > 0) {	/* buddy has nothing that big */
			ord--;
			pg = alloc_pages(GFP_USER, ord);
		}
		if (!pg)
			return -1;

		unsigned long pa = page_to_phys(zone, pg);

		chunks[*nr].pa    = pa;
		chunks[*nr].order = ord;
		(*nr)++;

		if (zero) {
			u8 *kva = (u8 *)phys_to_kva(pa);
			unsigned long nbytes = (unsigned long)(1u << ord) << PAGE_SHIFT;

			for (unsigned long i = 0; i < nbytes; i++)
				kva[i] = 0;
		}

		remaining -= (1u << ord);
	}
	return 0;
}

/*
 * Pull the whole image into a scattered region.
 *
 * The image is one flat stream but the pages behind it are several separate
 * runs, so the copy walks chunk by chunk. It works out because the chunks will
 * be mapped into CONSECUTIVE user VAs in that same order - byte N of the file
 * lands at USER_CODE_VA + N no matter which chunk holds it.
 *
 * A short read is a failure, not a partial success: a truncated program is
 * indistinguishable from a valid one once it is running, and it would fault
 * somewhere far from here with a PC that means nothing.
 */
static int bin_source_load(struct bin_source *src, const struct mm_chunk *chunks,
			   unsigned int nr, const char *name)
{
	unsigned long done = 0;

	for (unsigned int i = 0; i < nr && done < src->size; i++) {
		u8 *dst = (u8 *)phys_to_kva(chunks[i].pa);
		unsigned long room = (unsigned long)(1u << chunks[i].order) << PAGE_SHIFT;
		unsigned long want = src->size - done;

		if (want > room)
			want = room;

		if (src->mem) {
			for (unsigned long j = 0; j < want; j++)
				dst[j] = src->mem[done + j];
			done += want;
			continue;
		}

		unsigned long got = 0;

		while (got < want) {
			int n = vfs_read(src->fd, (char *)(dst + got), want - got);

			if (n <= 0) {
				printk("[SPAWN] %s: short read at %lu/%lu B\n",
				       name, done + got, src->size);
				return -1;
			}
			got += (unsigned long)n;
		}
		done += got;
	}

	if (done != src->size) {
		printk("[SPAWN] %s: region holds %lu B, image is %lu B\n",
		       name, done, src->size);
		return -1;
	}
	return 0;
}

/**
 * user_task_create_image() - Create a user-mode task from a program image
 * @name: Task name for debugging
 * @src:  where to read the image from (embedded blob or open file)
 * @argc: number of arguments (>= 1; argv[0] is the program name by convention)
 * @argv: the arguments, readable in the caller's current context
 *
 * Allocates a task_struct, kernel stack (SVC mode), mm_struct, one or
 * more 4KB code pages, a 4KB user stack page, and a 1KB L2 table.
 * Loads the image into the code pages and sets up the L2 mapping so
 * the task can run at PL0.
 *
 * The header is validated AFTER the load rather than before, because a file
 * cannot be inspected without reading it anyway. The cost is that a bad image
 * is detected one allocation later; the gain is one loader instead of two.
 *
 * Return: Pointer to the task_struct ready to enqueue, or NULL on failure.
 */
static struct task_struct *user_task_create_image(const char *name,
						  struct bin_source *src,
						  int argc,
						  const char *const *argv)
{
	struct zone *zone = get_zone();
	int pid = pid_alloc();

	if (pid < 0)			/* PID space gone — pid_alloc already said so */
		return NULL;

	struct task_struct *p = (struct task_struct *)kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return NULL;

	/*
	 * Descriptor table first, so every failure path below has exactly one
	 * extra thing to undo. A user task always gets one - it is what lets it
	 * open anything at all - and it is per process, never shared with the
	 * creator.
	 */
	p->files = files_alloc();
	if (!p->files)
		goto err_task;

	/*
	 * Kernel (SVC) stack: 16 KB. 4 KB was risky — a user task takes a
	 * syscall (vector_svc re-enables IRQs), and a timer IRQ can then nest
	 * vector_irq → irq_handler → schedule → __switch_to on top of the
	 * syscall frame on this same stack. An overflow corrupts the adjacent
	 * kmalloc allocation (an L2 table, task_struct…) → random faults.
	 */
#define KSTACK_SIZE  (4u * PAGE_SIZE)
	unsigned long *ksp = (unsigned long *)kmalloc(KSTACK_SIZE, GFP_KERNEL);
	if (!ksp)
		goto err_files;

	struct mm_struct *mm = (struct mm_struct *)kmalloc(sizeof(*mm), GFP_KERNEL);
	if (!mm)
		goto err_ksp;

	/* Both regions start empty so the unwind path can free them at any
	 * point below without asking how far we got. */
	mm->nr_code_chunks = 0;
	mm->nr_bss_chunks  = 0;

	unsigned long blob_size = src->size;

	/* Too small to even hold a header — nothing to validate against. */
	if (blob_size < sizeof(struct user_bin_header)) {
		printk("[SPAWN] %s: image too small (%lu B)\n", name, blob_size);
		goto err_mm;
	}

	unsigned int code_pages = (blob_size + PAGE_SIZE - 1) / PAGE_SIZE;

	if (alloc_region(mm->code_chunks, &mm->nr_code_chunks, code_pages, 0, zone)) {
		printk("[SPAWN] %s: no memory for %u code pages\n", name, code_pages);
		goto err_regions;
	}
	mm->code_pages = code_pages;

	if (bin_source_load(src, mm->code_chunks, mm->nr_code_chunks, name))
		goto err_regions;

	/*
	 * Validate the header now that the image is in memory. Same check for
	 * both sources: a corrupt file on the SD card and a mislinked blob fail
	 * identically, and say so before anything runs.
	 *
	 * The header is in the first chunk: every chunk is at least one page and
	 * the header is 16 bytes, so it cannot straddle a chunk boundary.
	 */
	struct user_bin_header *hdr =
		(struct user_bin_header *)phys_to_kva(mm->code_chunks[0].pa);

	if (hdr->magic != USER_BIN_MAGIC) {
		printk("[SPAWN] %s: bad magic 0x%x (expected 0x%x)\n",
		       name, (unsigned)hdr->magic, (unsigned)USER_BIN_MAGIC);
		goto err_regions;
	}
	unsigned long bss_size = hdr->bss_size;

	printk("[SPAWN] %s: image=%lu B (%u chunks), bss=%lu B\n",
	       name, blob_size, mm->nr_code_chunks, bss_size);

	/*
	 * BSS, zeroed. The linker pads .data to a 4 KB boundary so bss starts on
	 * a fresh page right after the code pages.
	 */
	unsigned int bss_pages = 0;

	if (bss_size > 0) {
		bss_pages = (bss_size + PAGE_SIZE - 1) / PAGE_SIZE;

		if (alloc_region(mm->bss_chunks, &mm->nr_bss_chunks, bss_pages, 1, zone)) {
			printk("[SPAWN] %s: no memory for %u bss pages\n",
			       name, bss_pages);
			goto err_regions;
		}
	}
	mm->bss_pages = bss_pages;

	/*
	 * User stack: 128 KB (32 pages). 4 KB was far too little for LVGL;
	 * 32 KB held LVGL 9.2 but 9.5's deeper draw/render call chains overflow
	 * it (Data Abort writing just below the stack bottom at ~0xBEFF8000).
	 * The high stack VA leaves room to grow without nearing bss.
	 *
	 * One contiguous block, unlike code and bss: it is a single fixed modest
	 * size the allocator can nearly always satisfy, and nothing grows it.
	 */
#define USER_STACK_ORDER  5
#define USER_STACK_PAGES  (1u << USER_STACK_ORDER)
	struct page *stack_pg = alloc_pages(GFP_USER, USER_STACK_ORDER);
	if (!stack_pg) {
		printk("[SPAWN] %s: alloc_pages(stack) failed\n", name);
		goto err_regions;
	}
	unsigned long stack_pa = page_to_phys(zone, stack_pg);

	/*
	 * Put argc/argv at the top of that stack and start the task just below
	 * them. Written through the kernel's direct map: the task is not running
	 * and its page tables are not installed, so the only way to reach these
	 * pages by their user addresses would be to switch address spaces first.
	 */
	unsigned long sp_top = setup_argv_block(
		(u8 *)phys_to_kva(stack_pa),
		(unsigned long)USER_STACK_PAGES * PAGE_SIZE, argc, argv);

	if (!sp_top) {
		printk("[SPAWN] %s: argv too large (max %u B)\n",
		       name, (unsigned)ARGV_MAX_BYTES);
		goto err_stack;
	}

	/*
	 * sp must land INSIDE the region that mmu_map_user() is about to map.
	 * Checked rather than assumed because the two are computed in different
	 * files from different starting points, and when they drifted apart once
	 * before the symptom was a Data Abort on the first user instruction -
	 * with a fault address that says nothing about which of the two was
	 * wrong. Failing here names the task instead.
	 */
	BUG_ON(sp_top > USER_STACK_TOP ||
	       sp_top < USER_STACK_TOP - (unsigned long)USER_STACK_PAGES * PAGE_SIZE);

	mm->stack_pa    = stack_pa;
	mm->stack_pages = USER_STACK_PAGES;
	mm->entry_va    = USER_BIN_ENTRY;	/* _start, after 16-byte binary header */
	mm->sp_top      = sp_top;		/* just below argc/argv; grows down */

	/*
	 * Private page tables: a 16 KB L1 (kernel half shared with swapper),
	 * then L2 tables for code (low), bss (after code) and the high stack.
	 * The stack lives near TASK_SIZE so a large bss can never reach it.
	 */
	if (pgd_alloc(mm) || mmu_map_user(mm)) {
		pgd_free(mm);
		goto err_stack;
	}

	/*
	 * Pre-build the __switch_to kernel stack frame:
	 *   ldmfd sp!, {r4-r11, pc}
	 *   r4 = user stack top  (initial sp_usr)
	 *   r5 = user entry VA   (initial pc)
	 *   r6-r11 = 0
	 *   pc = user_task_trampoline
	 */
	unsigned long *sp = (unsigned long *)((char *)ksp + KSTACK_SIZE);
	*--sp = (unsigned long)user_task_trampoline;
	*--sp = 0;
	*--sp = 0;
	*--sp = 0;
	*--sp = 0;
	*--sp = 0;
	*--sp = 0;
	*--sp = mm->entry_va;	/* r5: user entry VA, becomes pc in user mode */
	*--sp = mm->sp_top;	/* r4: user stack top, becomes sp_usr */

	p->stack      = sp;
	p->kstack_base = ksp;	/* kmalloc base of the kernel stack, for kfree on exit */
	p->user_sp    = mm->sp_top;
	p->user_lr    = 0;
	p->__state    = TASK_RUNNING;
	p->flags      = 0;
	p->pid        = pid;
	p->prio       = DEFAULT_PRIO;
	p->rt.on_rq   = 0;
	p->mm         = mm;
	p->exit_how   = EXIT_HOW_EXITED;
	p->exit_value = 0;
	p->parent     = parent_for_new_task();
	p->refcount   = 1;	/* existence ref; dropped at reap */
	list_init(&p->wait_node);	/* empty = on no wait queue */
	init_waitqueue_head(&p->child_wait);
	/* CFS-lite: start at the current min_vruntime so a fresh task can't hog
	 * the CPU with a stale-low vruntime (place_entity, initial case). */
	p->rt.vruntime = runqueue.min_vruntime;
	p->rt.exec_start = 0;
	p->rt.sum_exec_runtime = 0;
	p->rt.prev_sum_exec_runtime = 0;
	p->cwd[0]     = '/';
	p->cwd[1]     = '\0';

	unsigned int i = 0;
	for (; i < 15 && name[i]; i++)
		p->comm[i] = name[i];
	p->comm[i] = '\0';

	task_register(p);	/* memory-bound registry — cannot fail */

	printk("[SPAWN] user task \"%s\" pid=%d, code=%u chunk(s), stack_pa=0x%lx\n",
	       p->comm, p->pid, mm->nr_code_chunks, stack_pa);
	printk("[SPAWN]   pgd_pa=0x%lx nr_l2=%u sp_top=0x%lx (stack high VA)\n",
	       mm->pgd_pa, mm->nr_l2, mm->sp_top);

	/* User VA memory map — the only mapped regions; everything else in the
	 * 0..0xBF000000 range is UNMAPPED (touching it = translation fault).
	 *
	 * Virtually contiguous even though the pages behind code and bss are not:
	 * that is the whole point of laying the chunks into consecutive VAs. */
	{
		unsigned long code_va  = USER_CODE_VA;
		unsigned long code_end = code_va + (unsigned long)code_pages * PAGE_SIZE;
		unsigned long bss_va   = code_end;
		unsigned long bss_end  = bss_va + (unsigned long)bss_pages * PAGE_SIZE;
		unsigned long stk_top  = USER_STACK_TOP;
		unsigned long stk_bot  = stk_top - (unsigned long)USER_STACK_PAGES * PAGE_SIZE;
		printk("[SPAWN]   MAP code [%08lx-%08lx) %luK %uchk | data+bss [%08lx-%08lx) %luK %uchk | stack [%08lx-%08lx) %luK\n",
		       code_va, code_end, (code_end - code_va) / 1024, mm->nr_code_chunks,
		       bss_va, bss_end, (bss_end - bss_va) / 1024, mm->nr_bss_chunks,
		       stk_bot, stk_top, (stk_top - stk_bot) / 1024);
		printk("[SPAWN]   first UNMAPPED above bss = 0x%08lx  (a fault at/just past here = ran off the bss/pool top)\n",
		       bss_end);
	}

	return p;

	/*
	 * Unwind ladder. Each label undoes exactly one step and falls into the
	 * one below, so a new allocation means one label and one goto - not a
	 * new hand-copied release sequence. There were thirteen of those, and
	 * two had already drifted out of alignment with the rest.
	 */
err_stack:
	__free_pages(stack_pg, USER_STACK_ORDER);
err_regions:
	mm_free_chunks(mm->bss_chunks, &mm->nr_bss_chunks, zone);
	mm_free_chunks(mm->code_chunks, &mm->nr_code_chunks, zone);
err_mm:
	kfree(mm);
err_ksp:
	kfree(ksp);
err_files:
	files_free(p->files);
err_task:
	kfree(p);
	return NULL;
}

/**
 * user_task_create_bin() - create a task from an image embedded in the kernel
 * @name: task name for debugging
 * @blob_start: start of the image in kernel memory
 * @blob_end: end of the image
 *
 * The bootstrap path, and the only user of it is init: at that point in boot
 * nothing else is running to have asked for a process, and nothing guarantees
 * the SD card mounted. Every other program is loaded from a file.
 *
 * Return: task ready to enqueue, or NULL.
 */
struct task_struct *user_task_create_bin(const char *name,
	char *blob_start, char *blob_end)
{
	struct bin_source src = {
		.mem  = blob_start,
		.fd   = -1,
		.size = (unsigned long)(blob_end - blob_start),
	};
	const char *argv[] = { name };

	return user_task_create_image(name, &src, 1, argv);
}

/**
 * user_task_create_file() - create a task from a program file
 * @name: task name for debugging
 * @path: absolute path to the image
 * @argc: number of arguments, or 0 to synthesise argv = { @name }
 * @argv: the arguments, readable in the caller's current context
 *
 * The normal path. The kernel does not know which programs exist - it opens
 * what it is given, checks the header, and refuses anything that is not a
 * NothanOS user image.
 *
 * The fd is opened against whatever process is asking, and closed here on
 * every exit: the loader borrows a descriptor, it does not hand one to the new
 * task (which gets its own empty table).
 *
 * Return: task ready to enqueue, or NULL.
 */
struct task_struct *user_task_create_file(const char *name, const char *path,
					  int argc, const char *const *argv)
{
	int fd = vfs_open(path, O_RDONLY);

	if (fd < 0) {
		printk("[SPAWN] %s: cannot open \"%s\"\n", name, path);
		return NULL;
	}

	long size = vfs_size(fd);

	if (size <= 0) {
		printk("[SPAWN] %s: \"%s\" is empty or unsized\n", name, path);
		vfs_close(fd);
		return NULL;
	}

	struct bin_source src = {
		.mem  = NULL,
		.fd   = fd,
		.size = (unsigned long)size,
	};

	/*
	 * argv[0] always exists, even when the caller passed nothing: a program
	 * reading argv[0] for its own name is ordinary, and making every caller
	 * build a one-element array to allow it would be ceremony.
	 */
	const char *self[] = { name };

	if (argc <= 0) {
		argc = 1;
		argv = self;
	}

	struct task_struct *p = user_task_create_image(name, &src, argc, argv);

	vfs_close(fd);
	return p;
}

/*
 * basename() - the last path component, for task->comm
 *
 * "/bin/gui" is what the caller asked for; "gui" is what belongs in a log line
 * and in ps output. comm is 16 bytes and a path is not.
 */
static const char *basename(const char *path)
{
	const char *last = path;

	for (const char *c = path; *c; c++)
		if (*c == '/')
			last = c + 1;

	return last;
}

/**
 * spawn_path() - create and enqueue a task from a program file
 * @path: absolute path to the image, e.g. "/bin/gui"
 * @argc: number of arguments, or 0 to let the callee synthesise argv[0]
 * @argv: the arguments, readable in the caller's current context
 *
 * The kernel keeps no list of what may be started. It is handed a path, it
 * loads what is there, and it refuses anything without a valid user-image
 * header. Which programs exist, and which of them run at boot, is decided
 * entirely in user space (init reads /etc/inittab).
 *
 * Everything else it needs already exists: user_task_create_file() builds the
 * task and unwinds cleanly on every failure, pid_alloc() hands out a PID
 * safely, files_alloc() gives it an empty descriptor table, and
 * parent_for_new_task() records who asked.
 *
 * Return: PID of the new task, or -1.
 */
int spawn_path(const char *path, int argc, const char *const *argv)
{
	struct task_struct *p = user_task_create_file(basename(path), path,
						      argc, argv);

	if (!p) {
		/*
		 * Loading can fail for reasons the caller cannot tell apart from
		 * out here - missing file, bad header, out of memory - so the
		 * layer that KNOWS has already said which on the log. This line
		 * only records who asked, which that layer does not know.
		 */
		struct task_struct *cur = runqueue.curr;

		pr_err("[SPAWN] refused \"%s\" (asked by pid=%d \"%s\")\n",
		       path, cur ? cur->pid : -1, cur ? cur->comm : "?");
		return -1;
	}

	enqueue_task(&runqueue, p);
	pr_info("[SPAWN] \"%s\" pid=%d parent=%d\n",
		p->comm, p->pid, p->parent ? p->parent->pid : -1);

	return p->pid;
}

