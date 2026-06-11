/*
 * bin/sh/main.c - NothanOS shell
 *
 * Built-ins: help, clear, run, kill, cd, ls
 * All other commands are looked up in /bin/<cmd>.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../../lib/syscall.h"
#include "../../lib/string.h"
#include "../../lib/stdio.h"

#define LINE_BUF 128
#define MAX_ARGS 8
#define CWD_LEN  64

static void cmd_help(void)
{
	puts("\n");
	puts("  help\t\t\tShow this message\n");
	puts("  clear\t\t\tClear screen\n");
	puts("  cd <path>\t\tChange directory (/ and /dev only)\n");
	puts("  ls\t\t\tList current directory\n");
	puts("  run <name>\t\tRun a user program\n");
	puts("  kill <pid>\t\tTerminate a task\n");
	puts("  ps, info, uname\tSystem commands\n");
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

static void cmd_ls(const char *cwd)
{
	struct file_entry files[32];
	long count = listdir(cwd, files, 32);
	if (count < 0) { puts("ls: failed\n"); return; }
	if (count == 0) { puts("(empty)\n"); return; }

	puts("\n");
	for (long i = 0; i < count; i++) {
		const char *name = files[i].name;
		int len = 0;
		while (name[len]) len++;
		int is_dir = len > 0 && name[len - 1] == '/';

		putchar(' ');
		putpad(name, 20);
		if (is_dir)
			puts("       -\n");
		else {
			putint(files[i].size, 8);
			puts(" bytes\n");
		}
	}
	putchar('\n');
}

/* execute() returns 1 if cwd was changed (caller should re-fetch) */
static int execute(char *line, char *cwd)
{
	char *argv[MAX_ARGS];
	int argc = 0;

	while (*line == ' ' || *line == '\t') line++;
	if (*line == '\0') return 0;

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

	if (argc == 0) return 0;

	const char *cmd = argv[0];

	if (strcmp(cmd, "help") == 0) {
		cmd_help();
	} else if (strcmp(cmd, "clear") == 0) {
		putnchar('\n', 200);
		puts("\033[H");
	} else if (strcmp(cmd, "ls") == 0) {
		cmd_ls(cwd);
	} else if (strcmp(cmd, "cd") == 0) {
		const char *path = (argc >= 2) ? argv[1] : "/";
		if (chdir(path) < 0) {
			puts("cd: ");
			puts(path);
			puts(": not allowed\n");
		} else {
			getcwd(cwd, CWD_LEN);
			return 1;
		}
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
	return 0;
}

void main(void)
{
	/* Keep all mutable state on the stack — avoids BSS page mapping issues */
	char cwd[CWD_LEN];
	char line[LINE_BUF];
	unsigned int pos;

	cwd[0] = '/';
	cwd[1] = '\0';

	while (1) {
		puts("[");
		puts(cwd);
		puts("]> ");

		pos = 0;
		line[0] = '\0';

		while (1) {
			char c;
			long n = read(0, &c, 1);
			if (n <= 0) { yield(); continue; }

			if (c == '\r' || c == '\n') {
				putchar('\n');
				execute(line, cwd);
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
