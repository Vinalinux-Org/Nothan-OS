/*
 * net/socket.c - the seam os-architecture.md §7.2 draws as "app ──socket──►"
 *
 * Everything on the network up to now has been a kernel task: video_tx owns a
 * port, video_rx owns another, and each drains its own ring in its own
 * scheduler band.  That is the right shape for media, where the deadline
 * belongs to the band and a copy out to a process would buy nothing.
 *
 * Chat is not that.  It is a user process, it speaks first, and it needs the
 * network the way any other process needs a device — through a descriptor it
 * opens, uses and closes.  This file is that descriptor, and it is deliberately
 * thin: the socket underneath is the same struct udp_sock every kernel task
 * uses, and nothing here reimplements a transport.
 *
 * WHY A POOL AND NOT AN ALLOCATION.  udp.h keeps the bound-socket list lock
 * free by writing it only at initcall time, before any frame can arrive.  A
 * process opening a socket at runtime would break that — not on the insert,
 * which a walker survives, but on the close: unhooking a link while the
 * receive task is standing on it has no safe outcome, and the alternative is a
 * lock on the hottest path in the stack.  So the pool registers all its entries
 * at boot with no port, and open and close only lend an entry an address.  The
 * list never changes shape.  The cost is a fixed ceiling on open sockets, which
 * is a number this box can state; the benefit is that the receive path is
 * exactly as it was.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/udp.h>
#include <nothan/rel.h>
#include <nothan/fs.h>
#include <nothan/printk.h>
#include <nothan/init.h>
#include <nothan/uaccess.h>
#include <nothan/syscall.h>
#include <asm/irqflags.h>

/*
 * Four.  Chat wants one, the call signalling that follows wants one, and two
 * spare is the difference between adding an app and editing this file.  Each
 * costs a udp_sock — eight MTU-sized slots, about 12 KB — so the whole pool is
 * 48 KB out of 505 MB.  The ceiling exists to keep the list fixed, not to save
 * memory, and the counter below says when it has been reached.
 */
#define USOCK_MAX	4

struct usock {
	struct udp_sock	sock;
	struct rel_sock	rel;		/* used only when @reliable */
	int		reliable;
	int		in_use;
};

static struct usock usock_pool[USOCK_MAX];
static const char *usock_names[USOCK_MAX] = {
	"usock0", "usock1", "usock2", "usock3",
};
static unsigned long usock_exhausted;

/*
 * Closing the descriptor is the only way a socket comes back.
 *
 * There is no sock_close() syscall because there does not need to be: the
 * descriptor goes through vfs_close() like any other, and this hook runs from
 * there.  A process that exits without closing leaks an entry until the fd
 * table is torn down, which is the same deal every other descriptor here gets.
 */
static int sock_release(struct inode *inode, struct file *file)
{
	struct usock *u = (struct usock *)file->private_data;

	(void)inode;

	if (!u)
		return 0;

	/*
	 * Detach before releasing the port, so the transport task can never be
	 * left holding a socket whose ring another owner is already draining.
	 */
	if (u->reliable) {
		rel_detach(&u->rel);
		u->reliable = 0;
	}

	udp_port_release(&u->sock);
	u->in_use = 0;
	return 0;
}

/*
 * read() and write() are refused rather than mapped onto the datagram calls.
 *
 * A stream interface would have to invent the missing half of every operation:
 * a read that cannot say who sent the bytes, a write with nowhere to send
 * them.  Answering that with a remembered "current peer" is the shape that
 * makes a protocol quietly wrong the first time two peers talk at once.
 */
static int sock_read(struct file *file, char *buf, size_t count)
{
	(void)file; (void)buf; (void)count;
	return -1;
}

static int sock_write(struct file *file, const char *buf, size_t count)
{
	(void)file; (void)buf; (void)count;
	return -1;
}

const struct file_operations sock_fops = {
	.open    = NULL,
	.release = sock_release,
	.read    = sock_read,
	.write   = sock_write,
	.ioctl   = NULL,
};

/**
 * sock_open() - lend out a pooled socket bound to @port
 * @reliable: 0 for datagrams, 1 for the ordered, acknowledged transport
 *
 * The choice is made here and never again.  It could have been a flag on each
 * send, and that would be worse: the two have different delivery promises, and
 * a socket whose promise changes per call is one an application cannot reason
 * about — nor could the peer, which has no way to know which mode a datagram
 * was sent in.
 *
 * Return: a file descriptor, or -1.
 */
int sock_open(u16 port, int reliable)
{
	unsigned long flags;
	struct usock *u = NULL;
	int fd;

	/*
	 * Reserving the entry under a masked interrupt, then doing the rest
	 * outside it, mirrors vfs_open().  The window that matters is between
	 * finding a free slot and claiming it; everything after can be preempted
	 * safely because the slot is already nobody else's.
	 */
	flags = local_irq_save();
	for (int i = 0; i < USOCK_MAX; i++) {
		if (!usock_pool[i].in_use) {
			usock_pool[i].in_use = 1;
			u = &usock_pool[i];
			break;
		}
	}
	local_irq_restore(flags);

	if (!u) {
		usock_exhausted++;
		printk("[SOCK] no free socket for port %lu (%d in use)\n",
		       (unsigned long)port, USOCK_MAX);
		return -1;
	}

	if (udp_port_claim(&u->sock, port) != 0) {
		u->in_use = 0;
		return -1;
	}

	fd = vfs_install_file(&sock_fops, u);
	if (fd < 0) {
		udp_port_release(&u->sock);
		u->in_use = 0;
		return -1;
	}

	/*
	 * Attached last, once nothing else can fail.  From this call the
	 * transport task is the only consumer of the socket's ring, and a
	 * failure path that released the port afterwards would leave it
	 * draining a socket that had been handed to somebody else.
	 */
	u->reliable = reliable ? 1 : 0;
	if (u->reliable)
		rel_attach(&u->rel, &u->sock);

	return fd;
}

/**
 * sock_send() - one datagram to an address the caller names
 *
 * @data points into the calling process.  It is handed to udp_send_to()
 * unchanged rather than copied into a bounce buffer first, because the copy
 * that matters happens anyway when the payload goes into the transmit frame,
 * and a second one would cost 1472 bytes of a 4 KB kernel stack to save
 * nothing.  What makes that safe is the access_ok() in the syscall above:
 * the range is proven mapped before this is reached, and this kernel has no
 * demand paging that could take it away afterwards.
 *
 * Return: @len on success, -2 if the peer's link address is not known yet, -1
 * on any other refusal.  The two are kept apart all the way out to the caller
 * — see udp_send_to() for why.
 */
int sock_send(struct file *file, const struct sock_addr *addr,
	      const void *data, unsigned int len)
{
	struct usock *u = (struct usock *)file->private_data;
	int ret;

	if (!u || len > UDP_MAX_PAYLOAD)
		return -1;

	/*
	 * A reliable send only queues.  It cannot report -2, because there is
	 * nothing for the caller to retry: an unresolved address is one of the
	 * things the retransmission timer already covers, so the answer to
	 * "not yet" is to say nothing and let the transport handle it.
	 */
	if (u->reliable) {
		if (rel_send(&u->rel, addr->ip, addr->port, data, len) != 0)
			return -1;
		return (int)len;
	}

	ret = udp_send_to(&u->sock, addr->ip, addr->port, data, len);
	if (ret != 0)
		return ret;		/* -1 refused, -2 still resolving */

	return (int)len;
}

/**
 * sock_recv() - take the oldest datagram, without blocking
 * @out: filled with the sender's address
 *
 * Non-blocking, because the first caller is a GUI process whose loop must keep
 * running: a blocking receive there would stop the frame that draws the
 * message it is waiting for.  A task that would rather sleep than spin is a
 * real want, but it is a second entry point, not a flag on this one — the two
 * have different failure modes and a caller should have to say which it meant.
 *
 * Return: bytes copied, 0 if nothing has arrived, -1 on error.
 */
int sock_recv(struct file *file, struct sock_addr *out, void *buf,
	      unsigned int len)
{
	struct usock *u = (struct usock *)file->private_data;
	struct udp_datagram *dg;
	unsigned int n;
	u8 *dst = (u8 *)buf;

	if (!u)
		return -1;

	/*
	 * Never udp_poll() a reliable socket: ring.h allows one consumer, and
	 * on this socket that consumer is the transport task.  Two would take
	 * turns losing each other's datagrams.
	 */
	if (u->reliable)
		return rel_recv(&u->rel, out->ip, &out->port, buf, len);

	dg = udp_poll(&u->sock);
	if (!dg)
		return 0;

	n = dg->len < len ? dg->len : len;
	for (unsigned int i = 0; i < n; i++)
		dst[i] = dg->data[i];

	for (int i = 0; i < IP_ALEN; i++)
		out->ip[i] = dg->src_ip[i];
	out->port = dg->src_port;

	udp_done(&u->sock);
	return (int)n;
}

void sock_dump_stats(void)
{
	int used = 0;

	for (int i = 0; i < USOCK_MAX; i++)
		if (usock_pool[i].in_use)
			used++;

	printk("[SOCK] %d of %d in use, %lu refused for lack of a slot\n",
	       used, USOCK_MAX, usock_exhausted);
}

/*
 * Registered before any frame can arrive, which is the whole point: after this
 * runs, opening a socket never touches the list again.
 */
static int __init sock_pool_init(void)
{
	for (int i = 0; i < USOCK_MAX; i++) {
		if (udp_bind(&usock_pool[i].sock, 0, usock_names[i]) != 0)
			printk("[SOCK] pool entry %d refused\n", i);
	}
	printk("[SOCK] %d user sockets available\n", USOCK_MAX);
	return 0;
}
device_initcall(sock_pool_init);
