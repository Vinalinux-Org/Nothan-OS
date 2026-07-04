#ifndef _WAIT_H
#define _WAIT_H

#include <nothan/types.h>
#include <nothan/mm.h>
#include <nothan/sched.h>

/**
 * struct wait_queue_head - queue of tasks waiting for an event
 * @task_list: linked list of sleeping tasks
 */
struct wait_queue_head {
	struct list_head task_list;
};

void wait_event(struct wait_queue_head *wq);
void wake_up(struct wait_queue_head *wq);

#endif /* _WAIT_H */
