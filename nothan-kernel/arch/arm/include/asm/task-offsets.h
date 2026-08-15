#ifndef _ASM_TASK_OFFSETS_H
#define _ASM_TASK_OFFSETS_H

/*
 * arch/arm/include/asm/task-offsets.h - struct task_struct offsets for assembly
 *
 * switch_to.S reaches into task_struct by byte offset, because assembly has no
 * other way to.  That makes the field order of a C struct part of the contract
 * of an .S file, with nothing connecting the two — and the two drifted.
 *
 * They drifted like this: the assembly was written when user_sp and user_lr
 * were the second and third fields, so it saved the banked user-mode SP and LR
 * at +4 and +8.  kstack_base and kstack_size were later added ahead of them,
 * and from then on every context switch wrote the user stack pointer over the
 * outgoing task's kernel-stack base, and the user link register over its size.
 *
 * It hid for a long time, and the way it hid is worth knowing.  For a task
 * that never runs in user mode the value goes round in a circle: switching in
 * loads kstack_base into sp_usr, switching out writes sp_usr back to
 * kstack_base, and nothing in between disturbs it — so the field reads correct
 * every time anyone looks.  It only breaks when sp_usr genuinely changes,
 * which is to say when a real user task runs.
 *
 * The damage it was heading for was worse than a wrong number in a dump:
 * reap_dead() calls kfree(z->kstack_base) when a task is reaped, so the first
 * user task to exit would have handed the allocator a user-space address.
 *
 * So the offsets live here, once, and kernel/sched/core.c asserts them against
 * the C definition at compile time.  Adding a field ahead of user_sp now fails
 * the build instead of corrupting a task_struct at every switch.
 */

#define TSK_STACK	0	/* void *stack    — saved SVC sp */
#define TSK_USER_SP	12	/* unsigned long user_sp */
#define TSK_USER_LR	16	/* unsigned long user_lr */

#endif /* _ASM_TASK_OFFSETS_H */
