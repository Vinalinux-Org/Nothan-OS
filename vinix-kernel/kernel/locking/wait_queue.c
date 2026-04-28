/* ============================================================
 * wait_queue.c
 * ------------------------------------------------------------
 * Wait queue — pop-one-waiter wake_up, IRQ-disable serialized.
 * ============================================================ */

#include "wait_queue.h"
#include "task.h"
#include "types.h"

void wake_up(wait_queue_head_t *wq)
{
    uint32_t flags;
    __asm__ __volatile__("mrs %0, cpsr\n\tcpsid i" : "=r"(flags) :: "memory");

    struct task_struct *t = wq->head;
    if (t != 0)
    {
        wq->head     = t->wait_next;
        t->wait_next = 0;
        t->state     = TASK_RUNNING;
    }

    __asm__ __volatile__("msr cpsr_c, %0" :: "r"(flags) : "memory", "cc");
}
