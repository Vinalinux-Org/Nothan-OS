/* ============================================================
 * sleep.c
 * ------------------------------------------------------------
 * msleep() + sleep_tick().
 * ============================================================ */

#include "sleep.h"
#include "scheduler.h"
#include "task.h"
#include "types.h"

extern volatile bool need_reschedule;

volatile uint32_t jiffies = 0;

static struct task_struct *sleep_head;

void msleep(uint32_t ms)
{
    if (ms == 0)
    {
        return;
    }

    uint32_t ticks = ms / TICK_MS;
    if (ticks == 0)
    {
        ticks = 1;
    }

    uint32_t flags;
    __asm__ __volatile__("mrs %0, cpsr\n\tcpsid i" : "=r"(flags) :: "memory");

    struct task_struct *me = current;
    if (me != 0)
    {
        me->wake_tick   = jiffies + ticks;
        me->sleep_next  = sleep_head;
        sleep_head      = me;
        me->state       = TASK_INTERRUPTIBLE;
        need_reschedule = true;
    }

    __asm__ __volatile__("msr cpsr_c, %0" :: "r"(flags) : "memory", "cc");

    schedule();
}

void sleep_tick(void)
{
    jiffies++;

    struct task_struct **pp = &sleep_head;
    while (*pp != 0)
    {
        struct task_struct *t = *pp;
        if (jiffies >= t->wake_tick)
        {
            *pp            = t->sleep_next;
            t->sleep_next  = 0;
            t->wake_tick   = 0;
            t->state       = TASK_RUNNING;
        }
        else
        {
            pp = &t->sleep_next;
        }
    }
}
