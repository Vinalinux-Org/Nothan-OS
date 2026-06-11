/*
 * bin/uname/main.c - Print OS identification
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../../lib/syscall.h"
#include "../../lib/stdio.h"

void main(void)
{
	struct uname_info u;
	uname(&u);
	puts(u.sysname);
	putchar(' ');
	puts(u.version);
	putchar(' ');
	puts(u.machine);
	putchar('\n');
	user_exit(0);
}
