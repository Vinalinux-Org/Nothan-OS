/*
 * sim/chat_net_sim.c - the Chat transport, absent
 *
 * Replaces gui/services/chat_net.c on the host, exactly as modem_client_sim.c
 * replaces modem_client.c: the real one calls sock_open/sock_send/sock_recv,
 * which are inline SVC instructions and do not exist here.
 *
 * No-ops rather than a loopback.  A simulator that answered its own messages
 * would show a conversation working while proving nothing about the transport,
 * and the transport is the one part the simulator cannot test — it is a kernel
 * away.  Sending logs and stops; the store still updates, so the screens
 * behave here exactly as they do on a board with nobody at the other end.
 */

#include "services/chat_net.h"
#include "core/log.h"

void chat_net_init(void)
{
	gui_logf("chat: simulator, no socket — messages stay local\n");
}

int chat_net_send(const unsigned char *ip, unsigned short port,
		  const char *text)
{
	gui_logf("chat: [sim] would send to %u.%u.%u.%u:%u: %s\n",
		 ip[0], ip[1], ip[2], ip[3], port, text);
	return 0;
}

void chat_net_pump(void)
{
}
