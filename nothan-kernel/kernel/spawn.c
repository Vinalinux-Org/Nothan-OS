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

	*--sp = (unsigned long)task_entry;	/* lr → PC */
	*--sp = 0;				/* r11 */
	*--sp = 0;				/* r10 */
	*--sp = 0;				/* r9 */
	*--sp = 0;				/* r8 */
	*--sp = 0;				/* r7 */
	*--sp = 0;				/* r6 */
	*--sp = (unsigned long)task_exit;	/* r5 */
	*--sp = (unsigned long)fn;		/* r4 */

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

	unsigned int code_pages = (blob_size + PAGE_SIZE - 1) / PAGE_SIZE;
	unsigned int order = 0;
	while ((1u << order) < code_pages)
		order++;

	struct page *code_pg = alloc_pages(GFP_USER, order);
	if (!code_pg) {
		kfree(mm);
		kfree(ksp);
		kfree(p);
		return NULL;
	}

	struct zone *zone = get_zone();
	unsigned long code_pa = page_to_phys(zone, code_pg);
	mm->code_pages = code_pages;

	u8 *code_kva = (u8 *)phys_to_kva(code_pa);
	for (unsigned long i = 0; i < blob_size; i++)
		code_kva[i] = blob_start[i];

	struct page *stack_pg = alloc_pages(GFP_USER, 0);
	if (!stack_pg) {
		__free_pages(code_pg, order);
		kfree(mm);
		kfree(ksp);
		kfree(p);
		return NULL;
	}
	unsigned long stack_pa = page_to_phys(zone, stack_pg);

	/*
	 * L2 page table must be 1KB-aligned.  slab classes ≥ 1024 bytes are
	 * power-of-2 aligned, so kmalloc(1024) satisfies the constraint.
	 */
	u32 *l2 = (u32 *)kmalloc(1024, GFP_KERNEL);
	if (!l2) {
		__free_pages(stack_pg, 0);
		__free_pages(code_pg, order);
		kfree(mm);
		kfree(ksp);
		kfree(p);
		return NULL;
	}

	mm->l2       = l2;
	mm->l1_idx   = 0;		/* L1[0] covers VA 0x00000000–0x000FFFFF */
	mm->code_pa  = code_pa;
	mm->stack_pa = stack_pa;
	mm->entry_va = 0x00010000;	/* user code starts here */
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
	*--sp = (unsigned long)user_task_trampoline;	/* pc */
	*--sp = 0;					/* r11 */
	*--sp = 0;					/* r10 */
	*--sp = 0;					/* r9 */
	*--sp = 0;					/* r8 */
	*--sp = 0;					/* r7 */
	*--sp = 0;					/* r6 */
	*--sp = mm->entry_va;				/* r5 = user entry VA */
	*--sp = mm->sp_top;				/* r4 = user stack top */

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
