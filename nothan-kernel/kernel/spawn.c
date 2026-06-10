/*
 * kernel/spawn.c - Task creation and kernel thread setup
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/slab.h>
#include <nothan/printk.h>

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
 * @name: Name of the task
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

	/* Fill the stack from the top, matching __switch_to layout:
	 *   stmfd sp!, {r4-r11, lr}
	 *   → [sp+0]=r4, [sp+32]=lr
	 *   ldmfd sp!, {r4-r11, pc}
	 *   → r4 = [sp+0], pc = [sp+32] (= lr slot)
	 *
	 * We push: r4=fn, r5=task_exit, r6-r11=0, lr=task_entry
	 */
	sp = (unsigned long *)((char *)sp + PAGE_SIZE);

	*--sp = (unsigned long)task_entry;	/* lr → PC */
	*--sp = 0;							/* r11 */
	*--sp = 0;							/* r10 */
	*--sp = 0;							/* r9 */
	*--sp = 0;							/* r8 */
	*--sp = 0;							/* r7 */
	*--sp = 0;							/* r6 */
	*--sp = (unsigned long)task_exit;	/* r5 */
	*--sp = (unsigned long)fn;			/* r4 */

	p->stack = sp;
	p->user_sp = 0;
	p->user_lr = 0;
	p->__state = TASK_RUNNING;
	p->exit_state = 0;
	p->pid = next_pid++;
	p->tgid = p->pid;
	p->prio = prio;
	p->rt.time_slice = RR_TIMESLICE;
	p->rt.on_rq = 0;
	p->exit_code = 0;
	/* Process tree: parent = current, or self if no current */
	if (runqueue.curr) {
		p->real_parent = runqueue.curr;
		p->parent = runqueue.curr;
	} else {
		p->real_parent = p;
		p->parent = p;
	}
	unsigned int i = 0;
	for (; i < 15 && name[i]; i++)
		p->comm[i] = name[i];
	p->comm[i] = '\0';

	return p;
}

extern void user_task_trampoline(void);
extern void mmu_map_user(struct mm_struct *mm);

/*
 * Embedded user-space binaries (provided by userspace_blobs.S)
 */
extern char _binary_user_shell_start[];
extern char _binary_user_shell_end[];

/**
 * user_task_create_bin() - create a user task from an embedded binary
 * @name: task name for debugging
 * @blob_start: start of embedded binary
 * @blob_end: end of embedded binary
 *
 * Allocates:
 *   - task_struct + kernel stack (SVC mode stack for exceptions/syscalls)
 *   - mm_struct
 *   - 4KB code page  (VA 0x00010000, contains user_code_blob)
 *   - 4KB stack page (VA 0x000FF000, user stack grows down from 0x00100000)
 *   - 1KB L2 table   (must be 1KB-aligned; over-alloc a page and align)
 *
 * Return: pointer to task_struct ready to enqueue, or NULL on failure.
 */
struct task_struct *user_task_create_bin(const char *name,
	char *blob_start, char *blob_end)
{
	/* Allocate task_struct */
	struct task_struct *p = (struct task_struct *)kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return NULL;

	/* Allocate kernel stack (SVC mode) */
	unsigned long *ksp = (unsigned long *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!ksp) {
		kfree(p);
		return NULL;
	}

	/* Allocate mm_struct */
	struct mm_struct *mm = (struct mm_struct *)kmalloc(sizeof(*mm), GFP_KERNEL);
	if (!mm) {
		kfree(ksp);
		kfree(p);
		return NULL;
	}

	unsigned long blob_size = (unsigned long)(blob_end - blob_start);

	/* Calculate number of 4KB pages needed */
	unsigned int code_pages = (blob_size + PAGE_SIZE - 1) / PAGE_SIZE;
	unsigned int order = 0;
	while ((1u << order) < code_pages)
		order++;

	/* Allocate user code pages */
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

	/* copy binary to code pages */
	u8 *code_kva = (u8 *)phys_to_kva(code_pa);
	for (unsigned long i = 0; i < blob_size; i++)
		code_kva[i] = blob_start[i];

	/* Allocate user stack page */
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
	 * L2 page table (must be 1KB-aligned)
	 * kmalloc returns at least 4-byte aligned; slab class >=1024 gives
	 * 1KB-aligned naturally since slab objects are power-of-2 aligned.
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

	/* Fill mm_struct */
	mm->l2       = l2;
	mm->l1_idx   = 0;               /* L1[0] covers VA 0x00000000–0x000FFFFF */
	mm->code_pa  = code_pa;
	mm->stack_pa = stack_pa;
	mm->entry_va = 0x00010000;      /* user code starts at VA 0x00010000 */
	mm->sp_top   = 0x00100000;      /* stack grows down from VA 0x00100000 */

	/* install L2 into global L1, flush TLB */
	mmu_map_user(mm);

	/*
	 * Build __switch_to kernel stack frame
	 *
	 * __switch_to: ldmfd sp!, {r4-r11, pc}
	 *   r4 <- [sp+0]  = user_sp_top  (user stack initial sp)
	 *   r5 <- [sp+4]  = entry_va     (user entry point)
	 *   r6-r11 = 0
	 *   pc <- [sp+32] = user_task_trampoline
	 */
	unsigned long *sp = (unsigned long *)((char *)ksp + PAGE_SIZE);
	*--sp = (unsigned long)user_task_trampoline; /* pc  */
	*--sp = 0;  /* r11 */
	*--sp = 0;  /* r10 */
	*--sp = 0;  /* r9  */
	*--sp = 0;  /* r8  */
	*--sp = 0;  /* r7  */
	*--sp = 0;  /* r6  */
	*--sp = mm->entry_va;   /* r5 = user entry VA */
	*--sp = mm->sp_top;     /* r4 = user stack top */

	/* Init task_struct */
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

struct task_struct *user_task_create(const char *name)
{
	return user_task_create_bin(name, _binary_user_shell_start,
				    _binary_user_shell_end);
}
