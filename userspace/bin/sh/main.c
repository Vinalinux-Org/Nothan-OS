/*
 * bin/sh/main.c - NothanOS shell
 *
 * Built-ins: help, clear, run, kill
 * All other commands are looked up in /bin/<cmd>.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../../lib/syscall.h"
#include "../../lib/string.h"
#include "../../lib/stdio.h"

#define LINE_BUF 128
#define MAX_ARGS 8

static char line[LINE_BUF];
static unsigned int pos;

static void cmd_help(void)
{
	puts("\n");
	puts("  help\t\t\tShow this message\n");
	puts("  clear\t\t\tClear screen\n");
	puts("  run <name>\t\tRun a user program\n");
	puts("  kill <pid>\t\tTerminate a task\n");
	puts("  ps, ls, info, uname\tSystem commands\n");
	puts("  reboot, shutdown\tPower commands\n");
	putchar('\n');
}

static void wait_pid(long pid)
{
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
}

static void execute(char *line)
{
	char *argv[MAX_ARGS];
	int argc = 0;

	while (*line == ' ' || *line == '\t') line++;
	if (*line == '\0') return;

	argv[argc++] = line;
	while (*line) {
		if (*line == ' ' || *line == '\t') {
			*line++ = '\0';
			while (*line == ' ' || *line == '\t') line++;
			if (*line && argc < MAX_ARGS)
				argv[argc++] = line;
			continue;
		}
		line++;
	}

	if (argc == 0) return;

	const char *cmd = argv[0];

	if (strcmp(cmd, "help") == 0) {
		cmd_help();
	} else if (strcmp(cmd, "clear") == 0) {
		putnchar('\n', 200);
		puts("\033[H");
	} else if (strcmp(cmd, "kill") == 0) {
		if (argc < 2) {
			puts("usage: kill <pid>\n");
		} else {
			int pid = 0;
			const char *s = argv[1];
			while (*s >= '0' && *s <= '9')
				pid = pid * 10 + (*s++ - '0');
			if (kill(pid) < 0)
				puts("kill: pid not found\n");
		}
	} else if (strcmp(cmd, "run") == 0) {
		if (argc < 2) {
			puts("usage: run <name>\n");
		} else {
			long pid = spawn(argv[1]);
			if (pid < 0) {
				puts("not found: ");
				puts(argv[1]);
				putchar('\n');
			} else {
				wait_pid(pid);
			}
		}
	} else {
		/* Look up in /bin/<cmd> */
		char path[32];
		int i = 0;
		const char *prefix = "/bin/";
		while (*prefix) path[i++] = *prefix++;
		const char *p = cmd;
		while (*p && i < 30) path[i++] = *p++;
		path[i] = '\0';

		long pid = spawn(path);
		if (pid < 0) {
			puts("Unknown: '");
			puts(cmd);
			puts("'. Type 'help' for available commands.\n");
		} else {
			wait_pid(pid);
		}
	}
}

void main(void)
{
	while (1) {
		puts("> ");
		pos = 0;
		line[0] = '\0';

		while (1) {
			char c;
			long n = read(0, &c, 1);
			if (n <= 0) { yield(); continue; }

			if (c == '\r' || c == '\n') {
				putchar('\n');
				execute(line);
				break;
			} else if (c == '\b' || c == 0x7F) {
				if (pos > 0) {
					pos--;
					putchar('\b');
					putchar(' ');
					putchar('\b');
				}
			} else if (c >= 32 && c <= 126) {
				if (pos < LINE_BUF - 1) {
					line[pos++] = c;
					line[pos] = '\0';
					putchar(c);
				}
			}
		}
	}
}
