/*
 * kernel/sched/idle.c — idle task
 *
 * Loops and yields when no other task is runnable.
 */

#include "idle.h"
#include "task.h"
#include "scheduler.h"
#include "cpu.h"
#include "cpustat.h"
#include "types.h"

#define IDLE_STACK_SIZE 4096

static uint8_t            idle_stack[IDLE_STACK_SIZE] __attribute__((aligned(4096), section(".user_stack")));
static struct task_struct idle_task_struct;

static void idle_task(void)
{
    uint32_t wfi_start;

    while (1) {
        need_reschedule = true;
        schedule();

        /* WFI brackets measured so cpustat can compute real CPU%:
         * cycles between begin and end are idle; everything else is busy. */
        wfi_start = cpustat_wfi_begin();
        wfi();
        cpustat_wfi_end(wfi_start);
    }
}

struct task_struct *get_idle_task(void)
{
    idle_task_struct.name  = "idle";
    idle_task_struct.state = TASK_RUNNING;
    idle_task_struct.id    = 0;

    task_stack_init(&idle_task_struct, idle_task, idle_stack, IDLE_STACK_SIZE);

    return &idle_task_struct;
}
