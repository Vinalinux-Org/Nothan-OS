/*
 * drivers/tty/console_input.c — shared console input queue
 *
 * Owns the input ring and wait queue behind the public TTY interface.
 */

#include "cpu.h"
#include "nothan/console_input.h"
#include "nothan/common_subsystem.h"
#include "nothan/tty.h"

struct console_input_buffer {
    uint8_t           data[CONSOLE_INPUT_BUFFER_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t overflow;
};

static struct console_input_buffer input_buffer;

wait_queue_head_t console_input_wq = { .head = 0 };

int console_input_push(uint8_t ch)
{
    uint32_t flags;
    uint32_t next_head;

    flags = irq_save();
    next_head = (input_buffer.head + 1) % CONSOLE_INPUT_BUFFER_SIZE;
    if (next_head == input_buffer.tail) {
        input_buffer.overflow++;
        irq_restore(flags);
        return -1;
    }

    input_buffer.data[input_buffer.head] = ch;
    input_buffer.head = next_head;
    irq_restore(flags);

    wake_up(&console_input_wq);
    return 0;
}

int console_input_getc(void)
{
    uint32_t flags;
    uint8_t ch;

    flags = irq_save();
    if (input_buffer.head == input_buffer.tail) {
        irq_restore(flags);
        return -1;
    }

    ch = input_buffer.data[input_buffer.tail];
    input_buffer.tail = (input_buffer.tail + 1) % CONSOLE_INPUT_BUFFER_SIZE;

    irq_restore(flags);
    return (int)ch;
}

int console_input_available(void)
{
    uint32_t head;
    uint32_t tail;

    head = input_buffer.head;
    tail = input_buffer.tail;

    if (head >= tail)
        return head - tail;
    return CONSOLE_INPUT_BUFFER_SIZE - tail + head;
}

void console_input_clear(void)
{
    uint32_t flags;

    flags = irq_save();
    input_buffer.head = 0;
    input_buffer.tail = 0;
    irq_restore(flags);
}

int tty_receive_char(uint8_t ch)
{
    return console_input_push(ch);
}

int tty_get_char(void)
{
    return console_input_getc();
}

int tty_read_char(void)
{
    int ch;

    wait_event(console_input_wq, console_input_available() > 0);

    ch = tty_get_char();
    return ch;
}

int tty_input_available(void)
{
    return console_input_available();
}

void tty_input_clear(void)
{
    console_input_clear();
}

int tty_write_buf(const void *buf, uint32_t len)
{
    return common_subsystem_write(buf, len);
}
