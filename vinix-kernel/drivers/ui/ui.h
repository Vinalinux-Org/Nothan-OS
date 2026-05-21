/*
 * drivers/ui/ui.h — Nothan-OS UI system public interface
 */

#ifndef UI_H
#define UI_H

#include "task.h"

struct task_struct *get_ui_task(void);

#endif /* UI_H */
