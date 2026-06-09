/*
 * kernel/fork.c - Task creation and kernel thread setup
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/slab.h>

static int next_pid = 1;

static void task_exit(void)
{
	runqueue.curr->__state = TASK_UNINTERRUPTIBLE;
	runqueue.curr->exit_state = EXIT_ZOMBIE;
	schedule();
	while (1)
		;
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
	p->__state = TASK_RUNNING;
	p->pid = next_pid++;
	p->prio = prio;
	p->rt.time_slice = RR_TIMESLICE;
	p->rt.on_rq = 0;
	unsigned int i = 0;
	for (; i < 15 && name[i]; i++)
		p->comm[i] = name[i];
	p->comm[i] = '\0';

	return p;
}
