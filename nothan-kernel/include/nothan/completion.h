#ifndef _NOTHAN_COMPLETION_H
#define _NOTHAN_COMPLETION_H

#include <nothan/mm.h>
#include <nothan/wait.h>

struct completion {
	unsigned int		done;
	struct wait_queue_head	wait;
};

#define init_completion(c)					\
	do {							\
		(c)->done = 0;					\
		list_init(&(c)->wait.task_list);		\
	} while (0)

void wait_for_completion(struct completion *c);
void complete(struct completion *c);

#endif /* _NOTHAN_COMPLETION_H */
