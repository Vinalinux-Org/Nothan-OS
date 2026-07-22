#ifndef _NOTHAN_MSGQ_H
#define _NOTHAN_MSGQ_H

#include <nothan/wait.h>

/**
 * struct msgq - bounded, blocking message queue
 * @buf:       caller-provided storage, at least max_msgs * msg_size bytes
 * @msg_size:  fixed size of one message (bytes)
 * @max_msgs:  capacity in messages (fixed — bounded, no growth)
 * @used:      messages currently queued
 * @head:      next slot to write
 * @tail:      next slot to read
 * @not_empty: recv() waiters — woken by send()
 * @not_full:  send() waiters — woken by recv()
 *
 * A fixed-slot ring with two wait queues: send() sleeps while full, recv()
 * sleeps while empty. Storage is caller-owned (static) so the queue itself
 * allocates nothing. Use from task context only (after scheduling starts).
 */
struct msgq {
	char			*buf;
	unsigned int		msg_size;
	unsigned int		max_msgs;
	unsigned int		used;
	unsigned int		head;
	unsigned int		tail;
	struct wait_queue_head	not_empty;
	struct wait_queue_head	not_full;
};

void msgq_init(struct msgq *q, void *buf, unsigned int msg_size,
	       unsigned int max_msgs);
void msgq_send(struct msgq *q, const void *msg);   /* blocks while full  */
void msgq_recv(struct msgq *q, void *out);         /* blocks while empty */

/*
 * System message queues — a fixed, boot-created set exposed to userspace via
 * the msgq_send/msgq_recv syscalls. Referenced by a small integer qid. All
 * carry fixed-size MSGQ_MSG_SIZE messages. Bounded (no dynamic mq_open yet).
 */
#define MSGQ_NR		4	/* number of system queues */
#define MSGQ_MSG_SIZE	64	/* bytes per message */

void msgq_sys_init(void);			/* init the MSGQ_NR system queues */
struct msgq *msgq_get(unsigned int qid);	/* &sys_queue[qid], or NULL if qid invalid */

#endif /* _NOTHAN_MSGQ_H */
