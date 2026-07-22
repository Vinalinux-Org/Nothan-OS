/*
 * kernel/ipc/msgq.c - Bounded, blocking message queue
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * Fixed-slot ring + two wait queues (not_empty / not_full). send() sleeps
 * while full, recv() sleeps while empty; each wakes the other side after it
 * makes room / adds a message. Built on the Layer 0 sleep/wake primitives.
 *
 * Like completion.c, the whole "test condition -> queue self -> __schedule ->
 * touch the ring" sequence is held under a single local_irq_save() region;
 * __schedule() runs and returns with IRQs still masked. This keeps the ring
 * indices + count consistent against a waker firing from IRQ context, and is
 * why we use __prepare_to_wait()/__schedule() directly rather than the
 * wait_event() macro (which would drop the mask before we touch the ring).
 */

#include <nothan/msgq.h>
#include <nothan/sched.h>
#include <asm/irqflags.h>

void msgq_init(struct msgq *q, void *buf, unsigned int msg_size,
	       unsigned int max_msgs)
{
	q->buf      = (char *)buf;
	q->msg_size = msg_size;
	q->max_msgs = max_msgs;
	q->used     = 0;
	q->head     = 0;
	q->tail     = 0;
	init_waitqueue_head(&q->not_empty);
	init_waitqueue_head(&q->not_full);
}

void msgq_send(struct msgq *q, const void *msg)
{
	unsigned long flags;

	local_irq_save(flags);

	while (q->used == q->max_msgs) {		/* full — sleep on not_full */
		__prepare_to_wait(&q->not_full, TASK_UNINTERRUPTIBLE);
		__schedule();
	}
	__finish_wait();

	char *slot = q->buf + q->head * q->msg_size;
	for (unsigned int i = 0; i < q->msg_size; i++)
		slot[i] = ((const char *)msg)[i];
	if (++q->head == q->max_msgs)
		q->head = 0;
	q->used++;

	local_irq_restore(flags);

	wake_up(&q->not_empty);				/* one waiting recv() */
}

void msgq_recv(struct msgq *q, void *out)
{
	unsigned long flags;

	local_irq_save(flags);

	while (q->used == 0) {				/* empty — sleep on not_empty */
		__prepare_to_wait(&q->not_empty, TASK_UNINTERRUPTIBLE);
		__schedule();
	}
	__finish_wait();

	char *slot = q->buf + q->tail * q->msg_size;
	for (unsigned int i = 0; i < q->msg_size; i++)
		((char *)out)[i] = slot[i];
	if (++q->tail == q->max_msgs)
		q->tail = 0;
	q->used--;

	local_irq_restore(flags);

	wake_up(&q->not_full);				/* one waiting send() */
}

/*
 * System queues: a fixed set created at boot, referenced by qid from the
 * msgq_send/msgq_recv syscalls. Storage is static (bounded, no kmalloc).
 */

#define MSGQ_SYS_SLOTS	8	/* messages per system queue */

static struct msgq sys_queues[MSGQ_NR];
static char sys_bufs[MSGQ_NR][MSGQ_MSG_SIZE * MSGQ_SYS_SLOTS];

void msgq_sys_init(void)
{
	for (unsigned int i = 0; i < MSGQ_NR; i++)
		msgq_init(&sys_queues[i], sys_bufs[i], MSGQ_MSG_SIZE,
			  MSGQ_SYS_SLOTS);
}

struct msgq *msgq_get(unsigned int qid)
{
	if (qid >= MSGQ_NR)
		return NULL;
	return &sys_queues[qid];
}
