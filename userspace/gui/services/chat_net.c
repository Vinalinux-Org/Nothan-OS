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
#include "call.h"
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
	char frame[CHAT_TEXT_MAX + 1];
	unsigned int len = 0;

	if (chat_fd < 0 || !text)
		return -1;

	while (text[len])
		len++;
	if (len == 0 || len > CHAT_TEXT_MAX)
		return -1;

	/*
	 * Copied into a local frame to prepend the type byte.  The alternative
	 * is a scatter-gather send, which the syscall does not have and which
	 * would exist only to avoid copying 160 bytes at the speed a person
	 * types.
	 */
	frame[0] = CHAT_MSG_TEXT;
	for (unsigned int i = 0; i < len; i++)
		frame[1 + i] = text[i];

	for (int i = 0; i < 4; i++)
		m.addr.ip[i] = ip[i];
	m.addr.port = port;
	m.buf       = frame;
	m.len       = len + 1;

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

int chat_net_send_ctl(const unsigned char *ip, unsigned short port,
		      unsigned char type)
{
	struct sock_msg m;
	unsigned char frame[1];

	if (chat_fd < 0)
		return -1;

	frame[0] = type;

	for (int i = 0; i < 4; i++)
		m.addr.ip[i] = ip[i];
	m.addr.port = port;
	m.buf       = frame;
	m.len       = 1;

	if (sock_send(chat_fd, &m) < 0) {
		gui_logf("chat: send queue full, control %u not sent\n", type);
		return -1;
	}
	return 0;
}

void chat_net_pump(void)
{
	struct sock_msg m;
	char buf[CHAT_TEXT_MAX + 2];
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

		/*
		 * A datagram with no type byte is not this protocol.  Refused
		 * and counted rather than guessed at: the transport already
		 * proved it arrived intact, so a frame too short to have a type
		 * means the far end is speaking something else, and treating it
		 * as an empty message would put a blank bubble on the screen.
		 */
		if (n < 1) {
			gui_logf("chat: %ld-byte frame is not this protocol\n", n);
			continue;
		}

		if (buf[0] == CHAT_MSG_TEXT) {
			buf[n] = '\0';
			chat_receive_from(m.addr.ip, buf + 1);
		} else {
			call_on_control(m.addr.ip, (unsigned char)buf[0]);
		}
	}
}
