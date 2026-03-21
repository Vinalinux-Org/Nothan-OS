/* ============================================================
 * scheduler_algo.h
 * ------------------------------------------------------------
 * Abstract scheduling algorithm interface for VinixOS
 *
 * EDUCATIONAL NOTE:
 *   This header defines a "strategy" pattern for scheduling.
 *   Each algorithm (round-robin, EDF, etc.) provides its own
 *   implementation of the operations below. The core scheduler
 *   (scheduler.c) calls through the struct sched_algo pointer
 *   without knowing which algorithm is active.
 *
 *   This separation lets us:
 *   1. Add new algorithms without touching the core scheduler
 *   2. Select the algorithm at compile time (see scheduler_config.h)
 *   3. Test each algorithm independently
 *
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#ifndef SCHEDULER_ALGO_H
#define SCHEDULER_ALGO_H

#include "task.h"
#include "scheduler_config.h"

/* ============================================================
 * Scheduling Algorithm Interface
 * ============================================================
 *
 * Every scheduling algorithm must fill in a struct sched_algo
 * with valid function pointers for each operation.
 *
 * Responsibilities:
 *   - init()              : Set up algorithm-specific state
 *   - pick_next()         : Choose the next task to run
 *   - on_task_added()     : Bookkeeping when a task enters the system
 *   - on_task_terminated(): Bookkeeping when a task is removed
 *   - on_tick()           : Per-tick logic (runs in IRQ context!)
 */
struct sched_algo {
    /**
     * Human-readable algorithm name (for boot log)
     * Example: "Round-Robin", "EDF (Early Deadline First)"
     */
    const char *name;

    /**
     * Initialize algorithm-specific data structures.
     * Called once from scheduler_init().
     */
    void (*init)(void);

    /**
     * Select the next task to run.
     *
     * This is the core scheduling decision. The function examines
     * the task array and returns a pointer to the task that should
     * run next, or NULL if no context switch is needed (e.g., the
     * current task should keep running).
     *
     * @param current        Pointer to the currently running task
     * @param current_index  Index of current task in the task array
     * @param tasks          Array of task pointers (may contain NULLs)
     * @param task_count     Number of tasks currently registered
     * @param max_tasks      Size of the tasks[] array (MAX_TASKS)
     * @param out_index      [out] Index of the selected task
     * @return Pointer to the next task, or NULL if no switch needed
     *
     * IMPORTANT: This function must NOT modify task states; the
     *            caller (scheduler_yield) handles state transitions.
     */
    struct task_struct *(*pick_next)(struct task_struct *current,
                                     uint32_t current_index,
                                     struct task_struct **tasks,
                                     uint32_t task_count,
                                     uint32_t max_tasks,
                                     uint32_t *out_index);

    /**
     * Called when a new task is added to the scheduler.
     * Use this for algorithm-specific initialization of the task.
     *
     * @param task    Pointer to the newly added task
     * @param task_id The ID assigned to the task
     */
    void (*on_task_added)(struct task_struct *task, uint32_t task_id);

    /**
     * Called when a task is terminated (marked ZOMBIE).
     * Use this to clean up algorithm-specific per-task state.
     *
     * @param task Pointer to the terminated task
     */
    void (*on_task_terminated)(struct task_struct *task);

    /**
     * Called on each timer tick from the timer ISR.
     *
     * WARNING: This runs in IRQ context!
     *   - Keep it very short and simple
     *   - Do NOT call blocking functions
     *   - Do NOT call context_switch or scheduler_yield
     *   - Only safe operations: increment counters, set flags
     *
     * For round-robin, this is a no-op (the core scheduler sets
     * the need_reschedule flag). For EDF, this can update the
     * global tick counter used for deadline comparison.
     */
    void (*on_tick)(void);
};

/* ============================================================
 * Algorithm Registration
 * ============================================================
 *
 * Each algorithm module (round_robin.c, edf.c) provides a
 * function that returns a pointer to its struct sched_algo.
 *
 * The core scheduler calls sched_algo_current() to get the
 * active algorithm (selected at compile time via
 * scheduler_config.h).
 */

/** Get the compile-time selected scheduling algorithm */
const struct sched_algo *sched_algo_current(void);

/** Round-Robin algorithm descriptor (always compiled) */
const struct sched_algo *sched_algo_round_robin(void);

/** EDF algorithm descriptor (always compiled) */
const struct sched_algo *sched_algo_edf(void);

#endif /* SCHEDULER_ALGO_H */
