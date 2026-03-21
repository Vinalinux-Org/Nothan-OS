/* ============================================================
 * scheduler.h
 * ------------------------------------------------------------
 * Scheduler public API for VinixOS
 *
 * The scheduler manages task execution.  It consists of two
 * independent layers:
 *
 *   1. MECHANISM — Architecture-specific context save/restore
 *      (context_switch.S).  This never changes between
 *      algorithms.
 *
 *   2. POLICY — The algorithm that decides which task runs
 *      next (round_robin.c, edf.c, …).  Selected at compile
 *      time via SCHED_ALGO in the Makefile.
 *
 * The public API below is algorithm-independent.  Callers
 * (main.c, svc_handler.c, timer ISR) use the same functions
 * regardless of which algorithm is active.
 *
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "task.h"
#include "scheduler_config.h"

/* ============================================================
 * Core Scheduler API (algorithm-independent)
 * ============================================================ */

/**
 * Initialize scheduler subsystem
 * Must be called before any other scheduler functions.
 * Selects the scheduling algorithm based on compile-time config.
 */
void scheduler_init(void);

/**
 * Add a task to the scheduler
 *
 * The task must already have its stack initialized via
 * task_stack_init().  For EDF scheduling, set task->deadline
 * and task->period before calling this function.
 *
 * @param task Pointer to initialized task structure
 * @return Task ID (>= 0) on success, -1 if task table full
 */
int scheduler_add_task(struct task_struct *task);

/**
 * Terminate a task (Fault Handling)
 *
 * Marks the task as ZOMBIE.  If the terminated task is the
 * currently running task, forces an immediate yield.
 *
 * @param id Task ID to terminate
 */
void scheduler_terminate_task(uint32_t id);

/**
 * Start the scheduler
 *
 * This function does NOT return. It:
 * 1. Enables timer interrupts
 * 2. Loads first task context
 * 3. Jumps to first task
 *
 * REQUIRES: At least one task added, IRQ enabled in CPSR
 */
void scheduler_start(void) __attribute__((noreturn));

/**
 * Timer tick handler (called from timer ISR)
 *
 * Sets need_reschedule flag and invokes algorithm-specific
 * per-tick logic (e.g., deadline tracking for EDF).
 *
 * CRITICAL: Runs in IRQ mode — must be very fast, no blocking.
 */
void scheduler_tick(void);

/**
 * Voluntary task yield
 *
 * Called by tasks when they detect the need_reschedule flag.
 * Invokes the scheduling algorithm to pick the next task, then
 * performs the actual context switch in SVC mode (safe).
 */
void scheduler_yield(void);

/**
 * Get current running task
 * @return Pointer to current task, or NULL if scheduler not started
 */
struct task_struct *scheduler_current_task(void);

/**
 * Get list of tasks
 *
 * Fills a user buffer with process_info_t entries for all tasks.
 *
 * @param buf       User buffer (array of process_info_t)
 * @param max_count Max number of entries
 * @return Number of tasks filled, or -1 on failure
 */
int scheduler_get_tasks(void *buf, uint32_t max_count);

/* ============================================================
 * EDF-Specific API
 * ============================================================
 *
 * These functions are available regardless of the selected
 * algorithm.  When round-robin is active, they have no effect
 * (the deadline/period fields are simply ignored by the
 * round-robin pick_next logic).
 *
 * Usage example (in main.c or task setup code):
 *
 *   struct task_struct my_task;
 *   my_task.name = "sensor_reader";
 *   task_stack_init(&my_task, sensor_entry, stack, STACK_SIZE);
 *   scheduler_set_deadline(&my_task, 100, 50);   // deadline=100, period=50
 *   scheduler_add_task(&my_task);
 */

/**
 * Set EDF scheduling parameters for a task
 *
 * @param task     Pointer to the task
 * @param deadline Absolute deadline in scheduler ticks
 *                 (0 = no deadline, treated as "infinite")
 * @param period   Period for periodic tasks in ticks
 *                 (0 = aperiodic / one-shot)
 */
void scheduler_set_deadline(struct task_struct *task,
                            uint32_t deadline,
                            uint32_t period);

#endif /* SCHEDULER_H */
