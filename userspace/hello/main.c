/*
 * userspace/hello/main.c - First user-space program
 *
 * Prints "hi from user\n" repeatedly.
 * Written in C — compiled separately from the kernel.
 */
#include "../lib/syscall.h"

static const char msg[] = "hi from user\n";

void main(void)
{
	for (;;) {
		write(msg);
		yield();
	}
}
