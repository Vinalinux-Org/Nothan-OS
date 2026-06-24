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

static int next_pid = 1;

static void task_exit(void)
{
	do_exit(0);
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
	p->exit_state = 0;
	p->pid        = next_pid++;
	p->tgid       = p->pid;
	p->prio       = prio;
	p->rt.time_slice = RR_TIMESLICE;
	p->rt.on_rq   = 0;
	p->exit_code  = 0;
	p->mm         = NULL;
	p->cwd[0]     = '/';
	p->cwd[1]     = '\0';

	/* Parent = current task, or self if called before the scheduler starts. */
	if (runqueue.curr) {
		p->real_parent = runqueue.curr;
		p->parent      = runqueue.curr;
	} else {
		p->real_parent = p;
		p->parent      = p;
	}

	unsigned int i = 0;
	for (; i < 15 && name[i]; i++)
		p->comm[i] = name[i];
	p->comm[i] = '\0';

	return p;
}

extern void user_task_trampoline(void);

/*
 * User stack top VA. Lives high (near TASK_SIZE, like Linux) so it stays
 * far from the low code+bss region — bss/heap can grow without ever
 * reaching the stack. The stack itself occupies the pages just below.
 */
#define USER_STACK_TOP  0xBF000000UL

/* Embedded user-space binaries (linked in by userspace_blobs.S). */
extern char _binary_user_shell_start[];
extern char _binary_user_shell_end[];
extern char _binary_user_gui_start[];
extern char _binary_user_gui_end[];

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

/**
 * user_task_create_bin() - Create a user-mode task from a binary blob
 * @name: Task name for debugging
 * @blob_start: Start of the binary image in kernel memory
 * @blob_end: End of the binary image in kernel memory
 *
 * Allocates a task_struct, kernel stack (SVC mode), mm_struct, one or
 * more 4KB code pages, a 4KB user stack page, and a 1KB L2 table.
 * Copies the binary into the code pages and sets up the L2 mapping so
 * the task can run at PL0.
 *
 * Return: Pointer to the task_struct ready to enqueue, or NULL on failure.
 */
struct task_struct *user_task_create_bin(const char *name,
	char *blob_start, char *blob_end)
{
	struct task_struct *p = (struct task_struct *)kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return NULL;

	/*
	 * Kernel (SVC) stack: 16 KB. 4 KB was risky — a user task takes a
	 * syscall (vector_svc re-enables IRQs), and a timer IRQ can then nest
	 * vector_irq → irq_handler → schedule → __switch_to on top of the
	 * syscall frame on this same stack. An overflow corrupts the adjacent
	 * kmalloc allocation (an L2 table, task_struct…) → random faults.
	 */
#define KSTACK_SIZE  (4u * PAGE_SIZE)
	unsigned long *ksp = (unsigned long *)kmalloc(KSTACK_SIZE, GFP_KERNEL);
	if (!ksp) {
		kfree(p);
		return NULL;
	}

	struct mm_struct *mm = (struct mm_struct *)kmalloc(sizeof(*mm), GFP_KERNEL);
	if (!mm) {
		kfree(ksp);
		kfree(p);
		return NULL;
	}

	unsigned long blob_size = (unsigned long)(blob_end - blob_start);

	/* Parse binary header. Reject if magic mismatches or blob too small. */
	if (blob_size < sizeof(struct user_bin_header)) {
		printk("[SPAWN] %s: blob too small (%lu B)\n", name, blob_size);
		kfree(mm); kfree(ksp); kfree(p);
		return NULL;
	}
	struct user_bin_header *hdr = (struct user_bin_header *)blob_start;
	if (hdr->magic != USER_BIN_MAGIC) {
		printk("[SPAWN] %s: bad magic 0x%x (expected 0x%x)\n",
		       name, (unsigned)hdr->magic, (unsigned)USER_BIN_MAGIC);
		kfree(mm); kfree(ksp); kfree(p);
		return NULL;
	}
	unsigned long bss_size = hdr->bss_size;
	printk("[SPAWN] %s: blob=%lu B, bss=%lu B\n", name, blob_size, bss_size);

	unsigned int code_pages = (blob_size + PAGE_SIZE - 1) / PAGE_SIZE;
	unsigned int order = 0;
	while ((1u << order) < code_pages)
		order++;

	struct page *code_pg = alloc_pages(GFP_USER, order);
	if (!code_pg) {
		printk("[SPAWN] %s: alloc_pages(code, order=%u) failed\n", name, order);
		kfree(mm); kfree(ksp); kfree(p);
		return NULL;
	}

	struct zone *zone = get_zone();
	unsigned long code_pa = page_to_phys(zone, code_pg);
	mm->code_pages = code_pages;

	u8 *code_kva = (u8 *)phys_to_kva(code_pa);
	for (unsigned long i = 0; i < blob_size; i++)
		code_kva[i] = blob_start[i];

	/*
	 * Allocate and zero BSS pages. Linker pads .data to a 4 KB boundary
	 * so BSS starts on a fresh page right after the code pages.
	 */
	unsigned int bss_pages = 0;

	mm->nr_bss_chunks = 0;

	if (bss_size > 0) {
		bss_pages = (bss_size + PAGE_SIZE - 1) / PAGE_SIZE;

		/*
		 * Scatter-allocate bss as a few contiguous chunks: take the
		 * largest buddy block that fits the remainder, backing off the
		 * order if the buddy can't satisfy it. This lifts the old single
		 * 4 MB block ceiling (MAX_ORDER) and tolerates fragmentation; the
		 * chunks need not be contiguous with each other — mmu_map_user()
		 * maps them into consecutive user VAs.
		 */
		unsigned int remaining = bss_pages;
		while (remaining > 0) {
			if (mm->nr_bss_chunks >= MM_MAX_BSS_CHUNKS) {
				printk("[SPAWN] %s: bss too large (%u pages)\n",
				       name, bss_pages);
				mm_free_bss_chunks(mm, zone);
				__free_pages(code_pg, order);
				kfree(mm); kfree(ksp); kfree(p);
				return NULL;
			}

			unsigned int ord = 0;
			while ((1u << (ord + 1)) <= remaining && ord < MAX_ORDER)
				ord++;

			struct page *pg = alloc_pages(GFP_USER, ord);
			while (!pg && ord > 0) {
				ord--;
				pg = alloc_pages(GFP_USER, ord);
			}
			if (!pg) {
				printk("[SPAWN] %s: alloc_pages(bss) failed\n", name);
				mm_free_bss_chunks(mm, zone);
				__free_pages(code_pg, order);
				kfree(mm); kfree(ksp); kfree(p);
				return NULL;
			}

			unsigned long cpa = page_to_phys(zone, pg);
			mm->bss_chunks[mm->nr_bss_chunks].pa    = cpa;
			mm->bss_chunks[mm->nr_bss_chunks].order = ord;
			mm->nr_bss_chunks++;

			u8 *kva = (u8 *)phys_to_kva(cpa);
			unsigned long nbytes = (unsigned long)(1u << ord) << PAGE_SHIFT;
			for (unsigned long i = 0; i < nbytes; i++)
				kva[i] = 0;

			remaining -= (1u << ord);
		}
	}
	mm->bss_pages = bss_pages;

	/*
	 * User stack: 128 KB (32 pages). 4 KB was far too little for LVGL;
	 * 32 KB held LVGL 9.2 but 9.5's deeper draw/render call chains overflow
	 * it (Data Abort writing just below the stack bottom at ~0xBEFF8000).
	 * The high stack VA leaves room to grow without nearing bss.
	 */
#define USER_STACK_ORDER  5
#define USER_STACK_PAGES  (1u << USER_STACK_ORDER)
	struct page *stack_pg = alloc_pages(GFP_USER, USER_STACK_ORDER);
	if (!stack_pg) {
		printk("[SPAWN] %s: alloc_pages(stack) failed\n", name);
		mm_free_bss_chunks(mm, zone);
		__free_pages(code_pg, order);
		kfree(mm); kfree(ksp); kfree(p);
		return NULL;
	}
	unsigned long stack_pa = page_to_phys(zone, stack_pg);

	mm->code_pa     = code_pa;
	mm->stack_pa    = stack_pa;
	mm->stack_pages = USER_STACK_PAGES;
	mm->entry_va    = USER_BIN_ENTRY;	/* _start, after 16-byte binary header */
	mm->sp_top      = USER_STACK_TOP;	/* high VA; stack grows down from here */

	/*
	 * Private page tables: a 16 KB L1 (kernel half shared with swapper),
	 * then L2 tables for code (low), bss (after code) and the high stack.
	 * The stack lives near TASK_SIZE so a large bss can never reach it.
	 */
	if (pgd_alloc(mm) || mmu_map_user(mm)) {
		pgd_free(mm);
		__free_pages(stack_pg, USER_STACK_ORDER);
		mm_free_bss_chunks(mm, zone);
		__free_pages(code_pg, order);
		kfree(mm); kfree(ksp); kfree(p);
		return NULL;
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
	p->exit_state = 0;
	p->pid        = next_pid++;
	p->tgid       = p->pid;
	p->prio       = DEFAULT_PRIO;
	p->rt.time_slice = RR_TIMESLICE;
	p->rt.on_rq   = 0;
	p->mm         = mm;
	p->exit_code  = 0;
	p->cwd[0]     = '/';
	p->cwd[1]     = '\0';

	if (runqueue.curr) {
		p->real_parent = runqueue.curr;
		p->parent      = runqueue.curr;
		/* Inherit cwd from parent */
		unsigned int ci = 0;
		while (ci < 63 && runqueue.curr->cwd[ci]) {
			p->cwd[ci] = runqueue.curr->cwd[ci];
			ci++;
		}
		p->cwd[ci] = '\0';
	} else {
		p->real_parent = p;
		p->parent      = p;
	}

	unsigned int i = 0;
	for (; i < 15 && name[i]; i++)
		p->comm[i] = name[i];
	p->comm[i] = '\0';

	printk("[SPAWN] user task \"%s\" pid=%d, code_pa=0x%lx, stack_pa=0x%lx\n",
	       p->comm, p->pid, code_pa, stack_pa);
	printk("[SPAWN]   pgd_pa=0x%lx nr_l2=%u sp_top=0x%lx (stack high VA)\n",
	       mm->pgd_pa, mm->nr_l2, mm->sp_top);

	/* User VA memory map — the only mapped regions; everything else in the
	 * 0..0xBF000000 range is UNMAPPED (touching it = translation fault). */
	{
		unsigned long code_va  = 0x00010000UL;
		unsigned long code_end = code_va + (unsigned long)code_pages * PAGE_SIZE;
		unsigned long bss_va   = code_end;
		unsigned long bss_end  = bss_va + (unsigned long)bss_pages * PAGE_SIZE;
		unsigned long stk_top  = USER_STACK_TOP;
		unsigned long stk_bot  = stk_top - (unsigned long)USER_STACK_PAGES * PAGE_SIZE;
		printk("[SPAWN]   MAP code [%08lx-%08lx) %luK | data+bss [%08lx-%08lx) %luK | stack [%08lx-%08lx) %luK\n",
		       code_va, code_end, (code_end - code_va) / 1024,
		       bss_va, bss_end, (bss_end - bss_va) / 1024,
		       stk_bot, stk_top, (stk_top - stk_bot) / 1024);
		printk("[SPAWN]   first UNMAPPED above bss = 0x%08lx  (a fault at/just past here = ran off the bss/pool top)\n",
		       bss_end);
	}

	return p;
}

/**
 * user_task_create() - Create the init shell task from the embedded binary
 * @name: Task name passed to user_task_create_bin()
 *
 * Return: Pointer to the task_struct, or NULL on failure.
 */
struct task_struct *user_task_create(const char *name)
{
	return user_task_create_bin(name, _binary_user_shell_start,
				    _binary_user_shell_end);
}

/**
 * user_task_create_gui() - Create the GUI task from the embedded binary
 *
 * Return: Pointer to the task_struct, or NULL on failure.
 */
struct task_struct *user_task_create_gui(void)
{
	return user_task_create_bin("gui", _binary_user_gui_start,
				    _binary_user_gui_end);
}

/**
 * kernel_spawn() - Spawn a user task from a VFS path
 * @path: Absolute path to the binary on the mounted filesystem
 *
 * Reads the binary from the VFS and creates a user task.
 * Used during early boot to launch /sbin/init from SD card.
 *
 * Return: Pointer to the task_struct, or NULL on failure.
 */
struct task_struct *kernel_spawn(const char *path)
{
	int fd = vfs_open(path, 0);
	if (fd < 0)
		return NULL;

	unsigned long buf_size = 8192;
	u8 *buf = (u8 *)kmalloc(buf_size, GFP_KERNEL);
	if (!buf) {
		vfs_close(fd);
		return NULL;
	}

	int bytes = vfs_read(fd, (char *)buf, buf_size);
	vfs_close(fd);

	if (bytes <= 0) {
		kfree(buf);
		return NULL;
	}

	struct task_struct *tsk = user_task_create_bin(path, (char *)buf,
						       (char *)(buf + bytes));
	kfree(buf);
	return tsk;
}
