/* ============================================================
 * round_robin.c
 * ------------------------------------------------------------
 * Round-Robin scheduling algorithm for VinixOS
 *
 * EDUCATIONAL NOTE:
 *   Round-Robin is the simplest preemptive scheduling algorithm.
 *   It gives each task an equal time-slice (quantum) and cycles
 *   through all READY tasks in order:
 *
 *     Task 0 → Task 1 → Task 2 → … → Task 0 → …
 *
 *   Advantages:
 *     - Simple to implement and understand
 *     - Fair — every task gets the same CPU time
 *     - Low scheduling overhead
 *
 *   Disadvantages:
 *     - No priority support
 *     - Cannot guarantee deadlines (not suitable for hard
 *       real-time systems)
 *     - Response time depends on the number of tasks
 *
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#include "scheduler_algo.h"
#include "task.h"
#include "uart.h"

/* ============================================================
 * Algorithm State
 * ============================================================
 *
 * Round-robin needs no per-task metadata.  The only state is
 * the "last visited" index, which is managed by the core
 * scheduler (current_task_index).
 */

/* ============================================================
 * init — Initialize round-robin state
 * ============================================================
 *
 * Nothing to do: round-robin has no internal data structures.
 */
static void rr_init(void)
{
    uart_printf("[SCHED-RR] Round-Robin algorithm initialized\n");
}

/* ============================================================
 * pick_next — Select the next task to run
 * ============================================================
 *
 * Algorithm:
 *   Starting from the slot after the current task, scan the
 *   task array in circular order.  Return the first READY task
 *   found.  If we wrap all the way around without finding one,
 *   return NULL (no switch needed — current task keeps running).
 *
 * @param current        Currently running task
 * @param current_index  Index of current task in the array
 * @param tasks          Array of task pointers
 * @param task_count     Number of registered tasks
 * @param max_tasks      Size of the tasks[] array (MAX_TASKS)
 * @param out_index      [out] Set to the index of the chosen task
 * @return Pointer to next task, or NULL if no switch is needed
 */
static struct task_struct *rr_pick_next(struct task_struct *current,
                                         uint32_t current_index,
                                         struct task_struct **tasks,
                                         uint32_t task_count,
                                         uint32_t max_tasks,
                                         uint32_t *out_index)
{
    uint32_t next_index = current_index;
    uint32_t search_count = 0;

    /*
     * Scan up to max_tasks slots.  We start from (current + 1)
     * and wrap around.  The first READY task wins.
     */
    while (search_count < max_tasks) {
        next_index = (next_index + 1) % max_tasks;
        search_count++;

        if (tasks[next_index] != NULL &&
            tasks[next_index]->state == TASK_STATE_READY) {
            *out_index = next_index;
            return tasks[next_index];
        }
    }

    /* No other READY task found — stay on current */
    return NULL;
}

/* ============================================================
 * on_task_added — Notification when a task is registered
 * ============================================================
 *
 * Round-robin needs no special bookkeeping for new tasks.
 */
static void rr_on_task_added(struct task_struct *task, uint32_t task_id)
{
    (void)task;
    (void)task_id;
    /* No algorithm-specific setup needed */
}

/* ============================================================
 * on_task_terminated — Notification when a task dies
 * ============================================================
 *
 * Round-robin needs no cleanup.
 */
static void rr_on_task_terminated(struct task_struct *task)
{
    (void)task;
    /* No algorithm-specific cleanup needed */
}

/* ============================================================
 * on_tick — Timer tick handler
 * ============================================================
 *
 * Round-robin does not use tick information.  The core
 * scheduler already sets the need_reschedule flag.
 */
static void rr_on_tick(void)
{
    /* Nothing to do */
}

/* ============================================================
 * Algorithm Descriptor
 * ============================================================
 *
 * This static struct is returned by sched_algo_round_robin().
 * The core scheduler stores a pointer to it and dispatches
 * all scheduling decisions through these function pointers.
 */
static const struct sched_algo rr_algo = {
    .name               = "Round-Robin",
    .init               = rr_init,
    .pick_next          = rr_pick_next,
    .on_task_added      = rr_on_task_added,
    .on_task_terminated = rr_on_task_terminated,
    .on_tick            = rr_on_tick,
};

/**
 * Get the Round-Robin algorithm descriptor
 * @return Pointer to the round-robin sched_algo struct
 */
const struct sched_algo *sched_algo_round_robin(void)
{
    return &rr_algo;
}
