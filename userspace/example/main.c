/*
 * userspace/example/main.c - Example user program for NothanOS
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../lib/syscall.h"

void main(void)
{
	write("Welcome to NothanOS!\n");
	user_exit(0);
	while (1);
}
