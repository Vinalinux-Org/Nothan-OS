/* ============================================================
 * scheduler_config.h
 * ------------------------------------------------------------
 * Compile-time scheduler configuration for VinixOS
 *
 * This header allows selecting the scheduling algorithm at
 * compile time. Change the algorithm by setting SCHED_ALGO
 * in the kernel Makefile:
 *
 *   make SCHED_ALGO=EDF          # Early Deadline First
 *   make SCHED_ALGO=ROUND_ROBIN  # Round-Robin (default)
 *
 * The Makefile passes -DSCHED_ALGO_<name> to the compiler.
 *
 * EDUCATIONAL NOTE:
 *   Separating the *policy* (which task to run next) from
 *   the *mechanism* (how to save/restore CPU context) is a
 *   fundamental OS design principle. This file controls the
 *   policy; context_switch.S provides the mechanism.
 * ============================================================ */

#ifndef SCHEDULER_CONFIG_H
#define SCHEDULER_CONFIG_H

/* ============================================================
 * Algorithm Selection
 * ============================================================
 *
 * Exactly ONE of the following macros should be defined.
 * If none is defined (e.g., user forgot), default to
 * ROUND_ROBIN for backward compatibility.
 *
 * Available algorithms:
 *   SCHED_ALGO_ROUND_ROBIN  — Each READY task runs in turn
 *                              for one time-slice (10 ms).
 *                              Simple, fair, no priorities.
 *
 *   SCHED_ALGO_EDF          — Early Deadline First.
 *                              At each scheduling point, the
 *                              READY task with the nearest
 *                              absolute deadline is selected.
 *                              Optimal for single-processor
 *                              real-time scheduling.
 */
#if !defined(SCHED_ALGO_ROUND_ROBIN) && !defined(SCHED_ALGO_EDF)
#define SCHED_ALGO_ROUND_ROBIN   /* Default algorithm */
#endif

/* ============================================================
 * Scheduler Limits
 * ============================================================ */

/** Maximum number of tasks the scheduler can manage */
#define MAX_TASKS       4

/** Idle task always occupies slot 0 in the task array */
#define IDLE_TASK_ID    0

#endif /* SCHEDULER_CONFIG_H */
