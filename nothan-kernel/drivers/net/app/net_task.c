/*
 * drivers/net/app/net_task.c — periodic web timer: SSE push + keep-alive timeout
 */

#include "task.h"
#include "sleep.h"
#include "uart.h"
#include "types.h"
#include "string.h"
#include "cpustat.h"
#include "tcp.h"
#include "http.h"

#define NET_STACK_SIZE  4096

static uint8_t           net_stack[NET_STACK_SIZE] __attribute__((aligned(4096)));
static struct task_struct net_task_struct;

static void net_task_fn(void)
{
    unsigned char frame[128];
    uint16_t      len;

    while (1) {
        msleep(1000);
        cpustat_update();
        tcp_poll();
        len = http_sse_frame(frame, sizeof(frame));
        if (len > 0)
            tcp_sse_push(frame, len);
    }
}

struct task_struct *get_net_task(void)
{
    net_task_struct.name = "net";
    task_stack_init(&net_task_struct, net_task_fn, net_stack, NET_STACK_SIZE);
    return &net_task_struct;
}
