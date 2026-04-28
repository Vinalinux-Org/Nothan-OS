/* ============================================================
 * scheduler.h
 * ------------------------------------------------------------
 * Round-robin preemptive scheduler interface.
 * ============================================================ */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "task.h"

/* ============================================================
 * Scheduler Configuration
 * ============================================================ */

#define MAX_TASKS       5       /* idle + init + shell + 2 dynamic slots */
#define IDLE_TASK_ID    0       /* Idle task always at index 0 */

/* ============================================================
 * Scheduler API
 * ============================================================ */

void scheduler_init(void);

/* Returns -1 if the task table is full. */
int scheduler_add_task(struct task_struct *task);

void scheduler_terminate_task(uint32_t id);

/* Precondition: at least one task added, IRQ enabled in CPSR. */
void scheduler_start(void) __attribute__((noreturn));

/* IRQ context — sets need_reschedule, never context-switches. */
void scheduler_tick(void);

int scheduler_get_tasks(void *buf, uint32_t max_count);

/* Performs the actual context switch in SVC mode. */
void schedule(void);

/* Returns NULL if the scheduler hasn't started. */
struct task_struct *scheduler_current_task(void);

/* Linux-style — `current` expands to the active task pointer. */
#define current  scheduler_current_task()

/* Raw slot access for fork/wait; returns NULL on empty slot. */
struct task_struct *tasks_array_get(uint32_t idx);

/* Add a task that already carries its own ->id slot (used by fork). */
int scheduler_add_forked(struct task_struct *task);

/* Release a slot after wait() reaps a zombie. */
void scheduler_release_slot(uint32_t idx);

#endif /* SCHEDULER_H */
