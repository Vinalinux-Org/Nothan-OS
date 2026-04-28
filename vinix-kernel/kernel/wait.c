/* ============================================================
 * wait.c
 * ------------------------------------------------------------
 * do_exit / do_wait — process exit and parent reaping.
 * ============================================================ */

#include "proc.h"
#include "task.h"
#include "scheduler.h"
#include "wait_queue.h"
#include "mmu.h"
#include "page_alloc.h"
#include "slab.h"
#include "uart.h"
#include "types.h"

/* Single global queue — every parent waiting in wait() blocks here.
 * wake_up_all is not needed because each waiter rechecks its own
 * children list via wait_event(). */
static wait_queue_head_t parent_wq = { .head = 0 };

extern volatile bool need_reschedule;

static int has_zombie_child(int ppid)
{
    for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
        struct task_struct *t = tasks_array_get(i);
        if (t != 0 && t->ppid == ppid && t->state == TASK_ZOMBIE)
        {
            return 1;
        }
    }
    return 0;
}

void do_exit(int status)
{
    struct task_struct *me = current;
    if (me == 0) return;

    me->exit_status = status;
    me->state       = TASK_ZOMBIE;

    /* Wake every parent waiter — they individually re-check their own
     * children list. Simple pattern, fine at MAX_TASKS=5. */
    while (parent_wq.head != 0)
    {
        wake_up(&parent_wq);
    }

    /* Yield — scheduler picks someone else. We never come back. */
    need_reschedule = true;
    schedule();

    /* Fallback if a bug lets us return. */
    while (1) { }
}

int do_wait(int *status_out)
{
    struct task_struct *me = current;
    if (me == 0) return -1;

    wait_event(parent_wq, has_zombie_child(me->pid));

    for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
        struct task_struct *t = tasks_array_get(i);
        if (t != 0 && t->ppid == me->pid && t->state == TASK_ZOMBIE)
        {
            int pid = t->pid;
            int st  = t->exit_status;

            uint32_t pgd_pa       = t->pgd_pa;
            uint32_t user_pa      = t->user_pa;
            uint32_t user_order   = t->user_order;
            uint32_t kstack_pa    = t->kstack_pa;
            uint32_t kstack_order = t->kstack_order;

            scheduler_release_slot(i);
            kfree(t);

            if (pgd_pa != 0 && pgd_pa != mmu_kernel_pgd_pa())
            {
                mmu_free_pgd(pgd_pa);
            }
            if (user_pa != 0)
            {
                free_pages(user_pa, user_order);
            }
            if (kstack_pa != 0)
            {
                free_pages(kstack_pa, kstack_order);
            }

            if (status_out != 0) *status_out = st;
            return pid;
        }
    }

    return -1;
}

int do_kill_by_pid(int pid, int exit_status)
{
    struct task_struct *me = current;
    if (pid < 0 || (uint32_t)pid >= MAX_TASKS)
    {
        return -1;
    }
    if (me != 0 && me->pid == pid)
    {
        return -1;
    }

    struct task_struct *t = tasks_array_get((uint32_t)pid);
    if (t == 0 || t->ppid < 0)
    {
        /* Untouchable: idle (ppid=-1) and the shell container. */
        return -1;
    }
    if (t->state == TASK_ZOMBIE)
    {
        return 0;
    }

    t->exit_status = exit_status;
    t->state       = TASK_ZOMBIE;

    while (parent_wq.head != 0)
    {
        wake_up(&parent_wq);
    }
    return 0;
}
