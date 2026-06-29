/*
 * gui/services/modem_client.h - User-space IPC client to the phone_daemon
 *
 * Opens /dev/phone_fe (the frontend endpoint of phonebus) and exchanges
 * framed-JSON messages with the daemon. This replaces the mock radio:
 * incoming events are dispatched to telephony.c / messages.c, and user
 * actions are forwarded to the daemon as commands.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#ifndef GUI_MODEM_CLIENT_H
#define GUI_MODEM_CLIENT_H

/* Initialise the IPC channel. Call once at startup (after call_ui_init). */
void modem_client_init(void);

/* Poll /dev/phone_fe for incoming frames and dispatch events. Call every
 * iteration of the LVGL loop (before lv_task_handler). */
void modem_pump(void);

/* Commands — called by telephony.c / messages.c after the optimistic state
 * change. No-op if the IPC channel is down. */
void modem_cmd_dial(const char *number);
void modem_cmd_answer(void);
void modem_cmd_hangup(void);
void modem_cmd_reject(void);
void modem_cmd_mute(int on);
void modem_cmd_sms_send(const char *peer, const char *text);

#endif /* GUI_MODEM_CLIENT_H */
