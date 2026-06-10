/*
 * userspace/shell/main.c - NothanOS user shell
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../lib/syscall.h"

#define LINE_BUF 128
#define MAX_ARGS 8

static char line[LINE_BUF];
static unsigned int pos;

static int compare(const char *a, const char *b)
{
	while (*a && *b && *a == *b) { a++; b++; }
	return (unsigned char)*a - (unsigned char)*b;
}

static void putchar(char c)
{
	if (c == '\n')
		writefile(1, "\r", 1);
	writefile(1, &c, 1);
}

static void puts(const char *s)
{
	while (*s) {
		if (*s == '\n')
			writefile(1, "\r", 1);
		writefile(1, s, 1);
		s++;
	}
}

static void putnchar(char c, int n)
{
	for (int i = 0; i < n; i++) putchar(c);
}

static void putint(int n, int width)
{
	char buf[12];
	int i = 0, j;
	if (n < 0) { putchar('-'); n = -n; }
	if (n == 0) { buf[i++] = '0'; }
	while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
	for (j = i; j < width; j++) putchar(' ');
	while (i > 0) putchar(buf[--i]);
}

static void putpad(const char *s, int width)
{
	int len = 0;
	while (s[len]) len++;
	puts(s);
	putnchar(' ', width - len);
}

static const char *state_str(int state)
{
	switch (state) {
	case 0: return "RUNNING";
	case 1: return "SLEEP";
	case 2: return "BLOCKED";
	default: return "ZOMBIE";
	}
}

static void cmd_help(void)
{
	puts("\n");
	puts("  help\t\tShow this message\n");
	puts("  ps\t\tList running tasks\n");
	puts("  info\t\tShow system info\n");
	puts("  ls\t\tList files\n");
	puts("  clear\t\tClear screen\n");
	puts("  ./<file>\tExecute a .bin file\n");
	putchar('\n');
}

static void cmd_info(void)
{
	struct sys_info si;
	sysinfo(&si);

	unsigned long total_kb = si.total_pages * 4;
	unsigned long free_kb = si.free_pages * 4;
	unsigned long used_kb = total_kb - free_kb;
	unsigned long pct_used = 0;
	if (total_kb > 0)
		pct_used = used_kb * 100 / total_kb;

	putchar('\n');
	puts("  ---------------------\n");
	puts("  Memory Info\n");
	puts("  ---------------------\n");
	puts("  Total:  ");
	putint(total_kb, 8);
	puts(" KB\n");
	puts("  Used:   ");
	putint(used_kb, 8);
	puts(" KB (");
	putint(pct_used, 2);
	puts("%)\n");
	puts("  Free:   ");
	putint(free_kb, 8);
	puts(" KB (");
	putint(100 - pct_used, 2);
	puts("%)\n");
	puts("  ---------------------\n");
}

static void cmd_ps(void)
{
	struct task_info tasks[16];
	long count = gettasklist(tasks, 16);
	if (count <= 0) { puts("No tasks\n"); return; }

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
}

static void cmd_ls(void)
{
	struct file_entry files[16];
	long count = listdir("/", files, 16);
	if (count < 0) { puts("ls failed\n"); return; }
	if (count == 0) { puts("(empty)\n"); return; }

	puts("\n");
	for (long i = 0; i < count; i++) {
		putchar(' ');
		putpad(files[i].name, 20);
		putint(files[i].size, 8);
		puts(" bytes\n");
	}
	putchar('\n');
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

	if (compare(cmd, "help") == 0)		cmd_help();
	else if (compare(cmd, "ps") == 0)	cmd_ps();
	else if (compare(cmd, "info") == 0)	cmd_info();
	else if (compare(cmd, "ls") == 0)	cmd_ls();
	else if (compare(cmd, "clear") == 0) { putnchar('\n', 200); puts("\033[H"); }
	else if (cmd[0] == '.' && cmd[1] == '/') {
		long pid = exec(cmd + 2);
		if (pid < 0) {
			puts("exec failed: ");
			puts(cmd);
			putchar('\n');
		} else {
			puts("Started task pid=");
			putint(pid, 0);
			putchar('\n');
		}
	} else {
		puts("Unknown: '");
		puts(cmd);
		puts("'. Type 'help' for available commands.\n");
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
