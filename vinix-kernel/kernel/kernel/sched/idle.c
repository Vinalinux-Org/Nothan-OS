/* ============================================================
 * idle.c
 * ------------------------------------------------------------
 * Idle task — loops and yields when nothing else is runnable.
 * ============================================================ */

#include "idle.h"
#include "task.h"
#include "scheduler.h"
#include "types.h"

/* Idle task stack (4KB) */
#define IDLE_STACK_SIZE 4096
static uint8_t idle_stack[IDLE_STACK_SIZE] __attribute__((aligned(4096), section(".user_stack")));

/* Idle task structure */
static struct task_struct idle_task_struct;

#define PRINT_INTERVAL 100000 /* Reduced for visibility during frequent context switches */

static void idle_task(void)
{
    uint32_t counter = 0;

    while (1)
    {
        counter++;

        if (counter % PRINT_INTERVAL == 0)
        {
        }

        /* CRITICAL: must arm need_reschedule before yielding —
         * schedule() is gated on the flag, so without this
         * idle returns immediately and busy-loops until the next
         * tick (~10ms), producing visible shell I/O lag. */
        extern volatile bool need_reschedule;
        need_reschedule = true;
        schedule();

        /* WFI: sleep until next IRQ if yield didn't switch — keeps
         * idle from burning CPU when nothing else is READY. */
        __asm__ volatile("wfi");
    }
}

struct task_struct *get_idle_task(void)
{
    idle_task_struct.name = "idle";
    idle_task_struct.state = TASK_RUNNING;
    idle_task_struct.id = 0;

    task_stack_init(&idle_task_struct, idle_task, idle_stack, IDLE_STACK_SIZE);

    return &idle_task_struct;
}