/* ============================================================
 * idle.h
 * ------------------------------------------------------------
 * Idle task header.
 * ============================================================ */

#ifndef IDLE_H
#define IDLE_H

#include "task.h"

struct task_struct *get_idle_task(void);

#endif /* IDLE_H */
