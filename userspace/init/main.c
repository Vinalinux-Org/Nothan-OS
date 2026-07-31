/*
 * init/main.c - PID 1, the root of the process tree
 *
 * init is an ordinary user process. The kernel creates exactly one of these at
 * boot (init_task_create) and then stops deciding what runs: WHICH services
 * start is read from /etc/inittab at runtime, and each one is started through
 * the same sys_spawn any process may call.
 *
 * Nothing about the service set is compiled in - not into the kernel, and not
 * into init either. Adding a service means putting a binary on the disk and a
 * line in the config; no rebuild of anything.
 *
 * init has two jobs, in order:
 *
 *   1. START what /etc/inittab lists. One path per line; blank lines and
 *      lines beginning with '#' are ignored.
 *
 *   2. REAP forever. Every orphan in the system is reparented to PID 1, so if
 *      nobody collects them the corpses accumulate until reboot - a task_struct
 *      each, holding a PID that is never reused. wait() blocks when there is
 *      nothing to collect, so this loop costs no CPU; init sleeps except in the
 *      instant after a child dies.
 *
 * init must never return. main() returning would call exit(), and a dead PID 1
 * leaves every future orphan pointing at a freed task_struct - which is why the
 * kernel panics if this process ever dies (see __do_exit).
 */

#include "../lib/syscall.h"
#include "../lib/printf.h"

#define INITTAB      "/etc/inittab"
#define INITTAB_MAX  1024	/* a service list, not a document */
#define PATH_MAX     64

/*
 * start_services() - spawn everything /etc/inittab lists
 *
 * The file is read whole and parsed in place: it is a handful of short lines,
 * and a line-at-a-time reader would need buffering machinery for no gain.
 *
 * A failed spawn is reported but NOT fatal: the kernel has already logged why
 * (missing file, bad header, out of memory), and one service missing is a
 * system that boots degraded rather than not at all. init itself dying takes
 * the whole machine down, so it does not get to die over a daemon.
 *
 * Return: number of services started.
 */
static int start_services(void)
{
	static char buf[INITTAB_MAX + 1];

	long fd = open(INITTAB, O_RDONLY);

	if (fd < 0) {
		printf("[INIT] no %s - starting nothing\n", INITTAB);
		return 0;
	}

	long n = read((int)fd, buf, INITTAB_MAX);

	close((int)fd);

	if (n <= 0) {
		printf("[INIT] %s is empty\n", INITTAB);
		return 0;
	}
	buf[n] = '\0';

	int started = 0;
	long i = 0;

	while (i < n) {
		char path[PATH_MAX];
		int  len = 0;

		/* Copy one line, stopping at the newline or the buffer end. */
		while (i < n && buf[i] != '\n' && buf[i] != '\r') {
			if (len < PATH_MAX - 1)
				path[len++] = buf[i];
			i++;
		}
		while (i < n && (buf[i] == '\n' || buf[i] == '\r'))
			i++;			/* swallow the line ending */

		path[len] = '\0';

		if (len == 0 || path[0] == '#')
			continue;		/* blank or comment */

		long pid = spawn(path);

		if (pid < 0) {
			printf("[INIT] FAILED to spawn %s\n", path);
		} else {
			printf("[INIT] spawned %s pid=%ld\n", path, pid);
			started++;
		}
	}

	return started;
}

/*
 * reap_forever() - collect dead children until the machine is switched off
 *
 * wait() blocks while this process has children and none of them has died, so
 * the only spinning risk is the case where it has NO children at all: every
 * service failed to start, or every one of them has already been collected.
 * Sleeping for a second there turns a busy loop into an idle one.
 */
static void reap_forever(void)
{
	for (;;) {
		struct exit_status st;
		long pid = wait(&st);

		if (pid < 0) {
			msleep(1000);	/* no children to wait on - do not spin */
			continue;
		}

		printf("[INIT] reaped pid=%ld (%s %d)\n", pid,
		       st.how == EXIT_HOW_KILLED ? "killed by" : "status",
		       st.value);
	}
}

int main(void)
{
	printf("[INIT] pid=%ld up\n", getpid());

	int started = start_services();

	printf("[INIT] %d service(s) started\n", started);

	reap_forever();

	/* NOTREACHED - reap_forever() never returns, and init must not exit. */
	return 0;
}
