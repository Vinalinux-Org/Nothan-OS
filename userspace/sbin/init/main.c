/*
 * sbin/init/main.c - PID 1 init process
 *
 * Spawns /bin/sh and respawns it if it exits.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../../lib/syscall.h"

void main(void)
{
	while (1) {
		long pid = spawn("/bin/sh");
		if (pid < 0) {
			yield();
			continue;
		}

		/* Wait for shell to exit */
		while (1) {
			struct task_info tasks[16];
			long count = gettasklist(tasks, 16);
			int found = 0;
			for (long i = 0; i < count; i++) {
				if (tasks[i].pid == pid) { found = 1; break; }
			}
			if (!found) break;
			yield();
		}
		/* Shell exited — respawn */
	}
}
