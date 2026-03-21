/* ============================================================
 * edf.c
 * ------------------------------------------------------------
 * Early Deadline First (EDF) scheduling algorithm for VinixOS
 *
 * EDUCATIONAL NOTE:
 *   EDF is an optimal uniprocessor real-time scheduling
 *   algorithm.  At every scheduling decision, EDF selects the
 *   READY task whose absolute deadline is the nearest (smallest
 *   deadline value).
 *
 *   Key properties:
 *     - Optimal: If any algorithm can schedule a task set
 *       without missing deadlines, EDF can too.
 *     - Dynamic priority: A task's effective priority changes
 *       over time as its deadline approaches.
 *     - Utilization bound: A periodic task set is schedulable
 *       if and only if total utilization U ≤ 1.
 *
 *   How it works:
 *     1. Each task has an absolute deadline (in scheduler ticks).
 *     2. When the scheduler picks the next task, it scans all
 *        READY tasks and selects the one with the smallest
 *        deadline value.
 *     3. For periodic tasks, after a task yields, its deadline
 *        is advanced by its period (deadline += period).
 *     4. Tasks with deadline == 0 are treated as "no deadline"
 *        (best-effort) and get the lowest priority.
 *
 *   Example timeline (two periodic tasks):
 *
 *     Task A: period=5, WCET=2 → deadlines at t=5, 10, 15, …
 *     Task B: period=8, WCET=3 → deadlines at t=8, 16, 24, …
 *
 *     t=0:  A(dl=5) wins over B(dl=8) → run A
 *     t=2:  A yields, dl advances to 10 → B(dl=8) wins → run B
 *     t=5:  B yields, dl advances to 16 → A(dl=10) wins → run A
 *     …
 *
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#include "scheduler_algo.h"
#include "task.h"
#include "uart.h"

/* ============================================================
 * Internal State
 * ============================================================ */

/**
 * Global tick counter for EDF deadline tracking.
 *
 * Incremented on each timer tick (10 ms default).
 * Used to:
 *   - Compare absolute deadlines across tasks
 *   - Advance periodic task deadlines after they yield
 *
 * Wraps at UINT32_MAX (~497 days at 10 ms ticks), which is
 * acceptable for an educational OS.
 */
static uint32_t edf_tick_counter = 0;

/* ============================================================
 * init — Initialize EDF state
 * ============================================================ */
static void edf_init(void)
{
    edf_tick_counter = 0;
    uart_printf("[SCHED-EDF] EDF (Early Deadline First) algorithm initialized\n");
    uart_printf("[SCHED-EDF] Tasks with deadline=0 treated as best-effort (lowest priority)\n");
}

/* ============================================================
 * pick_next — Select the READY task with the earliest deadline
 * ============================================================
 *
 * Algorithm:
 *   1. Scan all task slots.
 *   2. Skip NULL, ZOMBIE, and RUNNING tasks.
 *   3. Among READY tasks, pick the one with the smallest
 *      non-zero deadline.
 *   4. Tasks with deadline == 0 are "best-effort" — they only
 *      run when no deadline-constrained task is READY.
 *   5. If multiple tasks have the same deadline, the one with
 *      the lower index wins (deterministic tie-breaking).
 *
 * Complexity: O(MAX_TASKS) — acceptable for small task counts.
 *
 * @param current        Currently running task
 * @param current_index  Index of current task in the array
 * @param tasks          Array of task pointers
 * @param task_count     Number of registered tasks
 * @param max_tasks      Size of the tasks[] array (MAX_TASKS)
 * @param out_index      [out] Set to the index of the chosen task
 * @return Pointer to next task, or NULL if no switch is needed
 */
static struct task_struct *edf_pick_next(struct task_struct *current,
                                          uint32_t current_index,
                                          struct task_struct **tasks,
                                          uint32_t task_count,
                                          uint32_t max_tasks,
                                          uint32_t *out_index)
{
    struct task_struct *best = NULL;        /* Best candidate so far */
    uint32_t best_deadline = 0xFFFFFFFF;   /* Smallest deadline seen */
    uint32_t best_index = current_index;   /* Index of best candidate */
    (void)task_count;                      /* Unused — we scan all slots */

    /*
     * Phase 1: Find the READY task with the earliest (smallest)
     * non-zero deadline.
     */
    for (uint32_t i = 0; i < max_tasks; i++) {
        struct task_struct *t = tasks[i];

        /* Skip empty slots and non-READY tasks */
        if (t == NULL || t->state != TASK_STATE_READY) {
            continue;
        }

        /*
         * A deadline of 0 means "no deadline" (best-effort).
         * These tasks are only selected if no deadline-constrained
         * task is available (handled in Phase 2 below).
         */
        if (t->deadline == 0) {
            continue;
        }

        /* Pick the task with the smallest deadline */
        if (t->deadline < best_deadline) {
            best_deadline = t->deadline;
            best = t;
            best_index = i;
        }
    }

    /*
     * Phase 2: If no deadline-constrained task is READY, fall back
     * to best-effort tasks.  Among best-effort tasks (deadline == 0),
     * we use simple round-robin ordering to maintain fairness.
     */
    if (best == NULL) {
        uint32_t next = current_index;
        for (uint32_t count = 0; count < max_tasks; count++) {
            next = (next + 1) % max_tasks;
            struct task_struct *t = tasks[next];

            if (t != NULL && t->state == TASK_STATE_READY) {
                best = t;
                best_index = next;
                break;
            }
        }
    }

    /* No READY task found — current keeps running */
    if (best == NULL) {
        return NULL;
    }

    /* If the best task IS the current task, no switch needed */
    if (best == current && current->state != TASK_STATE_ZOMBIE) {
        return NULL;
    }

    *out_index = best_index;
    return best;
}

/* ============================================================
 * on_task_added — Initialize EDF parameters for new task
 * ============================================================
 *
 * If the caller has not set a deadline (deadline == 0), this
 * is a best-effort task and no special setup is needed.
 *
 * If a deadline is set, we log it for observability.
 */
static void edf_on_task_added(struct task_struct *task, uint32_t task_id)
{
    if (task->deadline > 0) {
        uart_printf("[SCHED-EDF] Task %u '%s': deadline=%u, period=%u\n",
                    task_id,
                    task->name ? task->name : "(unnamed)",
                    task->deadline,
                    task->period);
    } else {
        uart_printf("[SCHED-EDF] Task %u '%s': best-effort (no deadline)\n",
                    task_id,
                    task->name ? task->name : "(unnamed)");
    }
}

/* ============================================================
 * on_task_terminated — Clean up EDF state for dead task
 * ============================================================
 *
 * Reset scheduling parameters so they don't confuse
 * future analysis or debug output.
 */
static void edf_on_task_terminated(struct task_struct *task)
{
    task->deadline = 0;
    task->period = 0;
}

/* ============================================================
 * on_tick — Per-tick EDF processing (IRQ context!)
 * ============================================================
 *
 * Increments the global tick counter.  This counter is used
 * for deadline comparison and periodic deadline advancement.
 *
 * WARNING: Runs in IRQ mode — must be minimal and non-blocking.
 */
static void edf_on_tick(void)
{
    edf_tick_counter++;
}

/* ============================================================
 * edf_advance_deadline — Advance periodic task deadline
 * ============================================================
 *
 * Called by the core scheduler after a periodic task yields.
 * Advances the task's absolute deadline by its period.
 *
 * For non-periodic tasks (period == 0), this is a no-op.
 *
 * This function is NOT part of the sched_algo interface — it
 * is called directly from scheduler_yield() after a context
 * switch for EDF scheduling.  This is acceptable because the
 * core scheduler knows it is running EDF when the feature is
 * compiled in.  An alternative design would add an
 * "on_yield()" callback to sched_algo, but that adds
 * complexity for a single use case.
 */
void edf_advance_deadline(struct task_struct *task)
{
    if (task == NULL) {
        return;
    }

    /*
     * Only advance deadline for periodic tasks that have a
     * non-zero period.  Aperiodic tasks keep their original
     * deadline until explicitly reset.
     */
    if (task->period > 0 && task->deadline > 0) {
        task->deadline += task->period;
    }
}

/* ============================================================
 * Public: Get current EDF tick counter
 * ============================================================
 *
 * Useful for setting initial deadlines relative to "now":
 *   task->deadline = edf_get_tick() + desired_delay;
 */
uint32_t edf_get_tick(void)
{
    return edf_tick_counter;
}

/* ============================================================
 * Algorithm Descriptor
 * ============================================================ */
static const struct sched_algo edf_algo = {
    .name               = "EDF (Early Deadline First)",
    .init               = edf_init,
    .pick_next          = edf_pick_next,
    .on_task_added      = edf_on_task_added,
    .on_task_terminated = edf_on_task_terminated,
    .on_tick            = edf_on_tick,
};

/**
 * Get the EDF algorithm descriptor
 * @return Pointer to the EDF sched_algo struct
 */
const struct sched_algo *sched_algo_edf(void)
{
    return &edf_algo;
}
