/*
 * services/chat_net.c - Chat over the reliable transport
 *
 * Thin on purpose.  Everything that makes a message arrive in order and
 * arrive at all — sequence, acknowledgement, retransmission, the ARP lookup
 * for a peer this box has never addressed — lives in the kernel behind
 * SOCK_RELIABLE.  What is left here is opening the socket, matching an
 * arriving datagram to a conversation, and handing it to the store.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "chat_net.h"
#include "chat.h"
#include "../core/log.h"
#include "../../lib/syscall.h"

static int chat_fd = -1;

void chat_net_init(void)
{
	chat_fd = (int)sock_open(CHAT_PORT, SOCK_RELIABLE);
	if (chat_fd < 0) {
		/*
		 * Say so once and carry on.  A Chat app that refuses to open
		 * because the network did is worse than one whose messages stay
		 * local: the screens, the store and the composer are all still
		 * correct, and this is exactly the state they were built and
		 * reviewed in.
		 */
		gui_logf("chat: no socket on port %d — messages stay local\n",
			 CHAT_PORT);
		return;
	}
	gui_logf("chat: port %d, reliable\n", CHAT_PORT);
}

int chat_net_send(const unsigned char *ip, unsigned short port,
		  const char *text)
{
	struct sock_msg m;
	unsigned int len = 0;

	if (chat_fd < 0 || !text)
		return -1;

	while (text[len])
		len++;
	if (len == 0 || len > CHAT_TEXT_MAX)
		return -1;

	for (int i = 0; i < 4; i++)
		m.addr.ip[i] = ip[i];
	m.addr.port = port;
	m.buf       = (void *)text;
	m.len       = len;

	if (sock_send(chat_fd, &m) < 0) {
		/*
		 * The only way a reliable send is refused is a full queue —
		 * four messages already waiting, which means the peer has been
		 * silent for a while.  Not the same as lost, and worth saying
		 * differently: the message never entered the transport, so no
		 * retransmission will ever carry it.
		 */
		gui_logf("chat: send queue full, message not sent\n");
		return -1;
	}
	return 0;
}

void chat_net_pump(void)
{
	struct sock_msg m;
	char buf[CHAT_TEXT_MAX + 1];
	long n;

	if (chat_fd < 0)
		return;

	/*
	 * Drain, do not take one.  The transport delivers into a ring of eight
	 * and this runs once per GUI frame; taking a single message per frame
	 * would fall behind a peer sending faster than the panel refreshes and
	 * the ring would start dropping — at which point messages go missing on
	 * a transport whose entire purpose is that they do not.
	 */
	for (;;) {
		m.buf = buf;
		m.len = sizeof(buf) - 1;

		n = sock_recv(chat_fd, &m);
		if (n <= 0)
			return;

		buf[n] = '\0';
		chat_receive_from(m.addr.ip, buf);
	}
}
