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

/*
 * User stack top VA. Lives high (near TASK_SIZE, like Linux) so it stays
 * far from the low code+bss region — bss/heap can grow without ever
 * reaching the stack. The stack itself occupies the pages just below.
 */
#define USER_STACK_TOP  0xBF000000UL

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
 * Pull the whole image into @dst.
 *
 * A short read is a failure, not a partial success: a truncated program is
 * indistinguishable from a valid one once it is running, and it would fault
 * somewhere far from here with a PC that means nothing.
 */
static int bin_source_load(struct bin_source *src, u8 *dst, const char *name)
{
	if (src->mem) {
		for (unsigned long i = 0; i < src->size; i++)
			dst[i] = src->mem[i];
		return 0;
	}

	unsigned long done = 0;

	while (done < src->size) {
		int n = vfs_read(src->fd, (char *)(dst + done),
				 src->size - done);

		if (n <= 0) {
			printk("[SPAWN] %s: short read at %lu/%lu B\n",
			       name, done, src->size);
			return -1;
		}
		done += (unsigned long)n;
	}
	return 0;
}

/**
 * user_task_create_image() - Create a user-mode task from a program image
 * @name: Task name for debugging
 * @src:  where to read the image from (embedded blob or open file)
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
						  struct bin_source *src)
{
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
	if (!p->files) {
		kfree(p);
		return NULL;
	}

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
		files_free(p->files);
		kfree(p);
		return NULL;
	}

	struct mm_struct *mm = (struct mm_struct *)kmalloc(sizeof(*mm), GFP_KERNEL);
	if (!mm) {
		kfree(ksp);
		files_free(p->files);
		kfree(p);
		return NULL;
	}

	unsigned long blob_size = src->size;

	/* Too small to even hold a header — nothing to validate against. */
	if (blob_size < sizeof(struct user_bin_header)) {
		printk("[SPAWN] %s: image too small (%lu B)\n", name, blob_size);
		kfree(mm);
		kfree(ksp);
		files_free(p->files);
		kfree(p);
		return NULL;
	}

	unsigned int code_pages = (blob_size + PAGE_SIZE - 1) / PAGE_SIZE;
	unsigned int order = 0;
	while ((1u << order) < code_pages)
		order++;

	struct page *code_pg = alloc_pages(GFP_USER, order);
	if (!code_pg) {
		printk("[SPAWN] %s: alloc_pages(code, order=%u) failed\n", name, order);
		kfree(mm);
		kfree(ksp);
		files_free(p->files);
		kfree(p);
		return NULL;
	}

	struct zone *zone = get_zone();
	unsigned long code_pa = page_to_phys(zone, code_pg);
	mm->code_pages = code_pages;

	u8 *code_kva = (u8 *)phys_to_kva(code_pa);

	if (bin_source_load(src, code_kva, name)) {
		__free_pages(code_pg, order);
		kfree(mm);
		kfree(ksp);
		files_free(p->files);
		kfree(p);
		return NULL;
	}

	/*
	 * Validate the header now that the image is in memory. Same check for
	 * both sources: a corrupt file on the SD card and a mislinked blob fail
	 * identically, and say so before anything runs.
	 */
	struct user_bin_header *hdr = (struct user_bin_header *)code_kva;

	if (hdr->magic != USER_BIN_MAGIC) {
		printk("[SPAWN] %s: bad magic 0x%x (expected 0x%x)\n",
		       name, (unsigned)hdr->magic, (unsigned)USER_BIN_MAGIC);
		__free_pages(code_pg, order);
		kfree(mm);
		kfree(ksp);
		files_free(p->files);
		kfree(p);
		return NULL;
	}
	unsigned long bss_size = hdr->bss_size;

	printk("[SPAWN] %s: image=%lu B, bss=%lu B\n", name, blob_size, bss_size);

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
				kfree(mm);
				kfree(ksp);
				files_free(p->files);
		kfree(p);
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
				kfree(mm);
				kfree(ksp);
				files_free(p->files);
		kfree(p);
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
		kfree(mm);
		kfree(ksp);
		files_free(p->files);
		kfree(p);
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
		kfree(mm);
		kfree(ksp);
		files_free(p->files);
		kfree(p);
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

	return user_task_create_image(name, &src);
}

/**
 * user_task_create_file() - create a task from a program file
 * @name: task name for debugging
 * @path: absolute path to the image
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
struct task_struct *user_task_create_file(const char *name, const char *path)
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
	struct task_struct *p = user_task_create_image(name, &src);

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
int spawn_path(const char *path)
{
	struct task_struct *p = user_task_create_file(basename(path), path);

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

