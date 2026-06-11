/*
 * bin/ps/main.c - List running tasks
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../../lib/syscall.h"
#include "../../lib/stdio.h"

static const char *state_str(int state)
{
	switch (state) {
	case 0: return "RUNNING";
	case 1: return "SLEEP";
	case 2: return "BLOCKED";
	default: return "ZOMBIE";
	}
}

void main(void)
{
	struct task_info tasks[16];
	long count = gettasklist(tasks, 16);
	if (count <= 0) { puts("No tasks\n"); user_exit(0); }

	putchar('\n');
	puts("  PID    NAME                   STATE     PRIO\n");
	puts("  ---    ----                   -----     ----\n");
	for (long i = 0; i < count; i++) {
		putchar(' ');
		putint(tasks[i].pid, 3);
		puts("    ");
		putpad(tasks[i].name, 22);
		putchar(' ');
		putpad(state_str(tasks[i].state), 9);
		putchar(' ');
		putint(tasks[i].prio, 3);
		putchar('\n');
	}
	putchar('\n');
	user_exit(0);
}
