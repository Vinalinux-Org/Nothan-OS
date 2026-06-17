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

	p->stack      = sp;
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
extern void mmu_map_user(struct mm_struct *mm);

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

	unsigned long *ksp = (unsigned long *)kmalloc(PAGE_SIZE, GFP_KERNEL);
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
	unsigned long bss_pa = 0;
	unsigned int bss_pages = 0;
	unsigned int bss_order = 0;
	struct page *bss_pg = NULL;

	if (bss_size > 0) {
		bss_pages = (bss_size + PAGE_SIZE - 1) / PAGE_SIZE;
		while ((1u << bss_order) < bss_pages)
			bss_order++;

		bss_pg = alloc_pages(GFP_USER, bss_order);
		if (!bss_pg) {
			printk("[SPAWN] %s: alloc_pages(bss, order=%u) failed\n",
			       name, bss_order);
			__free_pages(code_pg, order);
			kfree(mm); kfree(ksp); kfree(p);
			return NULL;
		}
		bss_pa = page_to_phys(zone, bss_pg);

		u8 *bss_kva = (u8 *)phys_to_kva(bss_pa);
		for (unsigned long i = 0; i < (unsigned long)bss_pages * PAGE_SIZE; i++)
			bss_kva[i] = 0;
	}
	mm->bss_pa    = bss_pa;
	mm->bss_pages = bss_pages;

	struct page *stack_pg = alloc_pages(GFP_USER, 0);
	if (!stack_pg) {
		printk("[SPAWN] %s: alloc_pages(stack) failed\n", name);
		if (bss_pg)
			__free_pages(bss_pg, bss_order);
		__free_pages(code_pg, order);
		kfree(mm); kfree(ksp); kfree(p);
		return NULL;
	}
	unsigned long stack_pa = page_to_phys(zone, stack_pg);

	/*
	 * L2 page table must be 1KB-aligned. slab classes ≥ 1024 bytes are
	 * power-of-2 aligned, so kmalloc(1024) satisfies the constraint.
	 */
	u32 *l2 = (u32 *)kmalloc(1024, GFP_KERNEL);
	if (!l2) {
		__free_pages(stack_pg, 0);
		if (bss_pg)
			__free_pages(bss_pg, bss_order);
		__free_pages(code_pg, order);
		kfree(mm); kfree(ksp); kfree(p);
		return NULL;
	}

	mm->l2       = l2;
	mm->l1_idx   = 0;		/* L1[0] covers VA 0x00000000–0x000FFFFF */
	mm->code_pa  = code_pa;
	mm->stack_pa = stack_pa;
	mm->entry_va = USER_BIN_ENTRY;	/* _start, after 16-byte binary header */
	mm->sp_top   = 0x00100000;	/* user stack grows down from here */

	mmu_map_user(mm);

	/*
	 * Pre-build the __switch_to kernel stack frame:
	 *   ldmfd sp!, {r4-r11, pc}
	 *   r4 = user stack top  (initial sp_usr)
	 *   r5 = user entry VA   (initial pc)
	 *   r6-r11 = 0
	 *   pc = user_task_trampoline
	 */
	unsigned long *sp = (unsigned long *)((char *)ksp + PAGE_SIZE);
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
