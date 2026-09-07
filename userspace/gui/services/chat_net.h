#ifndef __GUI_CHAT_NET_H
#define __GUI_CHAT_NET_H

/*
 * services/chat_net.h - the socket under the Chat app
 *
 * The same shape modem_client.h has for SMS, and split for the same reason:
 * the store knows what a conversation is, and this knows how a message
 * travels.  Keeping them apart is what lets the screens be built and looked
 * at before either transport exists, and it is why chat.c contains no syscall
 * at all — the simulator compiles chat.c unchanged on a laptop and swaps this
 * file for a stub, exactly as it does for modem_client.c.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

/*
 * One port for both ends.
 *
 * Not a client port and a server port, because there is no client and no
 * server: either box can type first.  Both bind the same number, so a message
 * arriving from a peer already carries the address to answer on, and a second
 * board needs no configuration beyond its own IP.
 */
#define CHAT_PORT	6000

/* Open the socket.  Safe to call when there is none — everything below then
 * becomes a no-op and the app still runs, which is how it behaved all the way
 * through building the screens. */
void chat_net_init(void);

/*
 * Hand one message to the transport.  Returns 0 if it was accepted for
 * delivery, -1 if it was not.
 *
 * Accepted is not delivered: underneath is the reliable transport, so this
 * returns as soon as the message is queued and the acknowledgement, the
 * retransmissions and the ARP lookup all happen after.  That is deliberate —
 * the bubble should appear when the user releases the button, not one round
 * trip later.
 */
int  chat_net_send(const unsigned char *ip, unsigned short port,
		   const char *text);

/*
 * Drain whatever has arrived, delivering each message into the store.
 * Never blocks.  Called once per GUI loop, like modem_pump().
 */
void chat_net_pump(void);

#endif
