/* ============================================================
 * scheduler.c
 * ------------------------------------------------------------
 * Core scheduler — task management and context switch dispatch
 *
 * EDUCATIONAL NOTE:
 *   This file implements the scheduler *mechanism*: managing
 *   the task array, handling the need_reschedule flag, checking
 *   stack canaries, and calling context_switch().
 *
 *   The scheduling *policy* (which task to run next) is
 *   delegated to a pluggable algorithm module via the
 *   struct sched_algo interface (see scheduler_algo.h).
 *
 *   Algorithm selection is controlled at compile time by
 *   defining SCHED_ALGO in the Makefile:
 *     make SCHED_ALGO=ROUND_ROBIN   (default)
 *     make SCHED_ALGO=EDF
 *
 * Architecture: ARM Cortex-A8 (BeagleBone Black)
 * ============================================================ */

#include "scheduler.h"
#include "scheduler_algo.h"
#include "task.h"
#include "uart.h"
#include "cpu.h"
#include "string.h"
#include "types.h"
#include "trace.h"
#include "assert.h"
#include "syscalls.h" /* For process_info_t */

/* ============================================================
 * External Assembly Functions (context_switch.S)
 * ============================================================
 *
 * These two functions are the lowest-level mechanism for
 * switching CPU context between tasks.  They are written in
 * assembly because they must manipulate banked ARM registers
 * (SP_usr, LR_usr) that are not accessible from C.
 *
 * The scheduling algorithm NEVER calls these directly — only
 * this file (scheduler.c) does.
 */

/** Save current task registers, load next task registers */
extern void context_switch(struct task_struct *current, struct task_struct *next);

/** Load the very first task (no current task to save) */
extern void start_first_task(struct task_struct *first);

/* ============================================================
 * Compile-Time Algorithm Selection
 * ============================================================
 *
 * sched_algo_current() returns a pointer to the struct sched_algo
 * selected by scheduler_config.h.  All policy calls go through
 * this pointer.
 */
const struct sched_algo *sched_algo_current(void)
{
#if defined(SCHED_ALGO_EDF)
    return sched_algo_edf();
#else
    return sched_algo_round_robin();
#endif
}

/* ============================================================
 * Scheduler Data Structures
 * ============================================================ */

/** Task array — each slot holds a pointer to a task_struct */
static struct task_struct *tasks[MAX_TASKS];

/** Number of tasks currently registered */
static uint32_t task_count = 0;

/** Magic value placed at the bottom of each task's stack.
 *  If this value is overwritten, we have a stack overflow. */
#define STACK_CANARY_VALUE  0xDEADBEEF

/** Pointer to the currently running task */
static struct task_struct *current_task = NULL;

/** Index of the current task in the tasks[] array */
static uint32_t current_task_index = 0;

/** True after scheduler_start() has been called */
static bool scheduler_started = false;

/** Active scheduling algorithm (set during scheduler_init) */
static const struct sched_algo *algo = NULL;

/* ============================================================
 * Global Reschedule Flag
 * ============================================================
 *
 * Set by scheduler_tick() (in IRQ mode) when a time-slice
 * expires.  Checked by tasks in their main loop or by
 * scheduler_yield().  Cleared when a context switch completes.
 *
 * This two-phase approach (flag + deferred switch) avoids doing
 * a context switch inside the IRQ handler, which would be
 * unsafe because the IRQ stack is shared among all tasks.
 */
volatile bool need_reschedule = false;

/* ============================================================
 * Forward declarations for EDF-specific helpers
 * ============================================================ */
#if defined(SCHED_ALGO_EDF)
extern void edf_advance_deadline(struct task_struct *task);
#endif

/* ============================================================
 * scheduler_init — Initialize the scheduler subsystem
 * ============================================================ */
void scheduler_init(void)
{
    uart_printf("[SCHED] Initializing scheduler...\n");

    /* Select the scheduling algorithm */
    algo = sched_algo_current();
    uart_printf("[SCHED] Algorithm: %s\n", algo->name);

    /* Reset task array */
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i] = NULL;
    }

    /*
     * Verify structure alignment for assembly access.
     *
     * context_switch.S accesses:
     *   context.sp     at offset 52  (13 uint32_t fields × 4 bytes)
     *   context.sp_usr at offset 64  (16 uint32_t fields × 4 bytes)
     *
     * If these offsets don't match, register save/restore will
     * corrupt memory — a very hard-to-debug failure.
     */
    if (__builtin_offsetof(struct task_struct, context.sp) != 52 ||
        __builtin_offsetof(struct task_struct, context.sp_usr) != 64) {
        PANIC("Struct alignment mismatch with Assembly!");
    }

    task_count = 0;
    current_task = NULL;
    current_task_index = 0;
    scheduler_started = false;

    /* Initialize algorithm-specific state */
    algo->init();

    TRACE_SCHED("Scheduler initialized (MAX_TASKS: %d)", MAX_TASKS);
}

/* ============================================================
 * scheduler_add_task — Register a task with the scheduler
 * ============================================================ */
int scheduler_add_task(struct task_struct *task)
{
    if (task_count >= MAX_TASKS) {
        uart_printf("[SCHED] ERROR: Task table full (%d/%d)\n",
                    task_count, MAX_TASKS);
        return -1;
    }

    /* Add task to array */
    tasks[task_count] = task;
    task->id = task_count;
    task->state = TASK_STATE_READY;

    uart_printf("[SCHED] Added task %d: '%s'\n",
                task->id, task->name ? task->name : "(unnamed)");
    uart_printf("  Stack: 0x%08x - 0x%08x (%u bytes)\n",
                (uint32_t)task->stack_base,
                (uint32_t)task->stack_base + task->stack_size,
                task->stack_size);

    /* Notify the algorithm (for algorithm-specific bookkeeping) */
    algo->on_task_added(task, task->id);

    task_count++;

    return task->id;
}

/* ============================================================
 * scheduler_start — Begin task execution (does NOT return)
 * ============================================================ */
void scheduler_start(void)
{
    uart_printf("\n[SCHED] Starting scheduler...\n");

    if (task_count == 0) {
        uart_printf("[SCHED] ERROR: No tasks to run!\n");
        while (1);  /* Halt */
    }

    uart_printf("[SCHED] %d task(s) ready\n", task_count);

    /* Mark first task as running */
    current_task = tasks[0];
    current_task_index = 0;
    current_task->state = TASK_STATE_RUNNING;
    scheduler_started = true;

    uart_printf("[SCHED] Starting task 0: '%s'\n", current_task->name);

    TRACE_SCHED("Jumping to first task (NO RETURN)...");

    /*
     * Load first task context and jump to it.
     * This restores the task's stack pointer, registers, and
     * jumps to its entry point via svc_exit_trampoline.
     */
    start_first_task(current_task);

    /* Should never reach here */
    uart_printf("[SCHED] FATAL: Returned from start_first_task!\n");
    while (1);
}

/* ============================================================
 * scheduler_tick — Timer ISR callback
 * ============================================================
 *
 * CRITICAL: This runs in IRQ mode!
 * We CANNOT safely call context_switch() here because:
 * - IRQ stack is shared
 * - Nested interrupts could corrupt stack
 *
 * Solution: Just set a flag and let tasks yield voluntarily.
 */
void scheduler_tick(void)
{
    /* Ignore ticks before scheduler starts */
    if (!scheduler_started) {
        return;
    }

    /*
     * IRQ-safe operation: Just set flag.
     * Tasks will check this and call scheduler_yield().
     */
    need_reschedule = true;

    /* Let the algorithm do per-tick processing (e.g., EDF tick counter) */
    algo->on_tick();
}

/* ============================================================
 * scheduler_terminate_task — Kill a task
 * ============================================================
 *
 * Called when a task crashes (Data/Prefetch Abort) or exits.
 */
void scheduler_terminate_task(uint32_t id)
{
    if (id >= MAX_TASKS || tasks[id] == NULL) {
        return;
    }

    struct task_struct *task = tasks[id];

    uart_printf("[SCHED] TERMINATING Task %d: '%s'\n", task->id, task->name);

    /* Mark as ZOMBIE */
    task->state = TASK_STATE_ZOMBIE;

    /* Notify the algorithm */
    algo->on_task_terminated(task);

    /* If current task is killing itself, yield immediately */
    if (current_task == task) {
        uart_printf("[SCHED] Task %d IS SUICIDE - Yielding...\n", task->id);

        /*
         * CRITICAL: We are likely in ABT/UND Mode (Exception Context).
         * We MUST switch to SVC Mode before calling scheduler_yield.
         * Otherwise, context_switch will use SP_abt/und which is wrong.
         */
        __asm__ volatile (
            "cps #0x13 \n\t"
        );

        need_reschedule = true; /* Force yield */
        scheduler_yield();
    }
}

/* ============================================================
 * scheduler_yield — Voluntary task switch
 * ============================================================
 *
 * Called by tasks when they detect the need_reschedule flag.
 * Runs in SVC mode (task context), so safe to switch.
 *
 * Flow:
 *   1. Check & clear need_reschedule flag
 *   2. Stack canary check
 *   3. Ask the algorithm to pick_next()
 *   4. Handle fallback (deadlock prevention)
 *   5. Update task states
 *   6. Call context_switch()
 */
void scheduler_yield(void)
{
    struct task_struct *prev_task;
    struct task_struct *next_task;
    uint32_t next_index;

    /* Check if reschedule is actually needed */
    if (!need_reschedule) {
        return;
    }

    /* Clear flag atomically */
    need_reschedule = false;

    /* ---- Stack integrity check (canary) ---- */
    if (current_task != NULL) {
        uint32_t *canary_ptr = (uint32_t *)current_task->stack_base;
        if (*canary_ptr != STACK_CANARY_VALUE) {
            TRACE_SCHED("FATAL: Stack overflow detected in task %d ('%s')",
                        current_task->id, current_task->name);
            uart_printf("[SCHED] Canary Addr: 0x%08x. Expected: 0x%08x. Actual: 0x%08x\n",
                        (uint32_t)canary_ptr, STACK_CANARY_VALUE, *canary_ptr);
            PANIC("Stack Canary Corrupted!");
        }
    }

    /* Only one task? No need to switch */
    if (task_count == 1) {
        return;
    }

    /* Save pointer to current task */
    prev_task = current_task;

    /* ---- Ask the algorithm to pick the next task ---- */
    next_index = current_task_index;
    next_task = algo->pick_next(current_task, current_task_index,
                                tasks, task_count, MAX_TASKS,
                                &next_index);

    /*
     * If pick_next returns NULL, no switch is needed.
     * But we must handle the edge case where the current task
     * is a ZOMBIE (it died but no other task was found).
     */
    if (next_task == NULL) {
        if (prev_task->state == TASK_STATE_ZOMBIE) {
            /* Deadlock prevention: force idle task READY */
            if (tasks[0] != NULL && tasks[0]->state != TASK_STATE_ZOMBIE) {
                uart_printf("[SCHED] Warning: No READY task found, "
                            "forcing idle task READY to prevent deadlock\n");
                tasks[0]->state = TASK_STATE_READY;
                next_task = tasks[0];
                next_index = 0;
            } else {
                PANIC("Scheduler Deadlock - No runnable tasks!");
            }
        } else {
            /* Current task continues — no switch */
            return;
        }
    }

    /* Sanity check: selected task must be READY */
    if (next_task->state != TASK_STATE_READY) {
        uart_printf("[SCHED] ERROR: Task %u not READY (state=%u)\n",
                    next_task->id, next_task->state);

        /* Fallback: Force idle to READY if it exists */
        if (tasks[0] != NULL) {
            uart_printf("[SCHED] Warning: Forcing idle task READY to prevent deadlock\n");
            tasks[0]->state = TASK_STATE_READY;
            next_task = tasks[0];
            next_index = 0;
        } else {
            PANIC("Scheduler Deadlock - No READY tasks!");
        }
    }

    /* ---- Update task states ---- */
    if (prev_task->state != TASK_STATE_ZOMBIE) {
        prev_task->state = TASK_STATE_READY;
    }
    next_task->state = TASK_STATE_RUNNING;

    /* Update global scheduler state */
    current_task_index = next_index;
    current_task = next_task;

    /*
     * EDF-specific: Advance the previous task's deadline if it
     * is a periodic task.  This ensures the task's next activation
     * has a deadline one period later.
     */
#if defined(SCHED_ALGO_EDF)
    if (prev_task->state == TASK_STATE_READY) {
        edf_advance_deadline(prev_task);
    }
#endif

    /* ---- Perform context switch ---- */
    context_switch(prev_task, next_task);
}

/* ============================================================
 * scheduler_current_task — Get current running task
 * ============================================================ */
struct task_struct *scheduler_current_task(void)
{
    return current_task;
}

/* ============================================================
 * scheduler_set_deadline — Set EDF parameters for a task
 * ============================================================
 *
 * This function can be called regardless of the active algorithm.
 * When round-robin is active, the fields are set but ignored
 * by the pick_next logic.
 */
void scheduler_set_deadline(struct task_struct *task,
                            uint32_t deadline,
                            uint32_t period)
{
    if (task == NULL) {
        return;
    }
    task->deadline = deadline;
    task->period = period;
}

/* ============================================================
 * scheduler_get_tasks — Get list of tasks for user space
 * ============================================================ */
int scheduler_get_tasks(void *buf, uint32_t max_count)
{
    process_info_t *info = (process_info_t *)buf;
    uint32_t count = 0;

    /*
     * NOTE: Buffer validation is handled by svc_handler.c's
     * validate_user_pointer() before calling this function.
     */

    for (int i = 0; i < MAX_TASKS && count < max_count; i++) {
        struct task_struct *t = tasks[i];
        if (t != NULL) {
            info[count].id = t->id;

            /* Copy name using string library */
            const char *src = t->name ? t->name : "unknown";
            int copy_len = 0;
            while (src[copy_len] && copy_len < 31) {
                copy_len++;
            }
            memcpy(info[count].name, src, copy_len);
            info[count].name[copy_len] = '\0';

            info[count].state = t->state;

            count++;
        }
    }

    return count;
}
