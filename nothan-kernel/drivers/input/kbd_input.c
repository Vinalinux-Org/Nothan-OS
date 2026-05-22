/*
 * drivers/input/kbd_input.c — minimal processed-char keyboard input core.
 */

#include "cpu.h"
#include "nothan/errno.h"
#include "nothan/kbd_input.h"
#include "nothan/tty.h"
#include "wait_queue.h"

#define KBD_INPUT_QUEUE_SIZE 64

struct kbd_input_queue {
    uint8_t data[KBD_INPUT_QUEUE_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t overflow;
};

static struct kbd_input_queue kbd_queue;
static wait_queue_head_t kbd_input_wq = { .head = 0 };

int kbd_input_available(void)
{
    uint32_t head;
    uint32_t tail;

    head = kbd_queue.head;
    tail = kbd_queue.tail;

    if (head >= tail)
        return head - tail;
    return KBD_INPUT_QUEUE_SIZE - tail + head;
}

int kbd_input_publish_char(uint8_t ch)
{
    uint32_t flags;
    uint32_t next_head;
    int tty_ret;

    tty_ret = tty_receive_char(ch);

    flags = irq_save();
    next_head = (kbd_queue.head + 1) % KBD_INPUT_QUEUE_SIZE;
    if (next_head == kbd_queue.tail) {
        kbd_queue.overflow++;
        irq_restore(flags);
        return -EAGAIN;
    }

    kbd_queue.data[kbd_queue.head] = ch;
    kbd_queue.head = next_head;
    irq_restore(flags);

    wake_up(&kbd_input_wq);
    return tty_ret;
}

int kbd_input_read_char(void)
{
    uint32_t flags;
    uint8_t ch;

    wait_event(kbd_input_wq, kbd_input_available() > 0);

    flags = irq_save();
    if (kbd_queue.head == kbd_queue.tail) {
        irq_restore(flags);
        return -EAGAIN;
    }

    ch = kbd_queue.data[kbd_queue.tail];
    kbd_queue.tail = (kbd_queue.tail + 1) % KBD_INPUT_QUEUE_SIZE;
    irq_restore(flags);
    return (int)ch;
}
