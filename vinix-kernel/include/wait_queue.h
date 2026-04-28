/* ============================================================
 * wait_queue.h
 * ------------------------------------------------------------
 * Blocking wait on a condition, woken by wake_up().
 * ============================================================ */

#ifndef WAIT_QUEUE_H
#define WAIT_QUEUE_H

#include "types.h"
#include "task.h"
#include "scheduler.h"

typedef struct wait_queue_head {
    struct task_struct *head;   /* chain of BLOCKED waiters */
} wait_queue_head_t;

#define DECLARE_WAIT_QUEUE_HEAD(name)  wait_queue_head_t name = { .head = 0 }

static inline void init_waitqueue_head(wait_queue_head_t *wq) { wq->head = 0; }

void wake_up(wait_queue_head_t *wq);

extern volatile bool need_reschedule;

/* wait_event: block until cond becomes true.
 *
 * Race-safe: the cond check + self-enqueue happen under IRQ-disable,
 * so wake_up() from IRQ context cannot slip between them. After the
 * enqueue we re-enable IRQs and yield — if a wake_up() fires in that
 * brief window it finds us on the queue and moves us to READY. */
#define wait_event(wq_var, cond) do {                                      \
    while (1) {                                                            \
        uint32_t __we_flags;                                               \
        __asm__ __volatile__("mrs %0, cpsr\n\tcpsid i"                     \
                             : "=r"(__we_flags) :: "memory");              \
        if (cond) {                                                        \
            __asm__ __volatile__("msr cpsr_c, %0"                          \
                                 :: "r"(__we_flags) : "memory", "cc");     \
            break;                                                         \
        }                                                                  \
        struct task_struct *__we_me = current;            \
        if (__we_me) {                                                     \
            __we_me->wait_next = (wq_var).head;                            \
            (wq_var).head     = __we_me;                                   \
            __we_me->state    = TASK_INTERRUPTIBLE;                        \
            need_reschedule   = true;                                      \
        }                                                                  \
        __asm__ __volatile__("msr cpsr_c, %0"                              \
                             :: "r"(__we_flags) : "memory", "cc");         \
        schedule();                                                 \
    }                                                                      \
} while (0)

#endif
