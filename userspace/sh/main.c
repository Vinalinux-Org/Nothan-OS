/*
 * sh/main.c - NothanOS shell
 *
 * All commands are built-in; no external process spawning.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../lib/syscall.h"
#include "../lib/string.h"
#include "../lib/stdio.h"
#include "../lib/types.h"

#define LINE_BUF 128
#define MAX_ARGS 8
#define CWD_LEN  64

static void cmd_help(void)
{
	puts("\n");
	puts("  help\t\t\tShow this message\n");
	puts("  clear\t\t\tClear screen\n");
	puts("  cd <path>\t\tChange directory\n");
	puts("  ls\t\t\tList current directory\n");
	puts("  ps\t\t\tList running tasks\n");
	puts("  kill <pid>\t\tTerminate a task\n");
	puts("  info\t\t\tMemory info\n");
	puts("  uname\t\t\tOS identification\n");
	puts("  reboot\t\tWarm reboot\n");
	puts("  shutdown\t\tHalt system\n");
	puts("  simstat\t\tCheck SIM status\n");
	putchar('\n');
}

static void cmd_ls(const char *cwd)
{
	struct file_entry files[32];
	long count = listdir(cwd, files, 32);
	if (count < 0) {
		puts("ls: failed\n");
		return;
	}
	if (count == 0) {
		puts("(empty)\n");
		return;
	}

	puts("\n");
	for (long i = 0; i < count; i++) {
		const char *name = files[i].name;
		int len = 0;
		while (name[len]) {
			len++;
		}
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

static const char *state_str(int state)
{
	switch (state) {
	case 0: return "RUNNING";
	case 1: return "SLEEP";
	case 2: return "BLOCKED";
	case 4: return "STOPPED";
	default: return "???";
	}
}

static void cmd_ps(void)
{
	struct task_info tasks[16];
	long count = gettasklist(tasks, 16);
	if (count <= 0) {
		puts("No tasks\n");
		return;
	}

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

static void cmd_info(void)
{
	struct sys_info si;
	sysinfo(&si);

	unsigned long total_kb = si.total_pages * 4;
	unsigned long free_kb  = si.free_pages * 4;
	unsigned long used_kb  = total_kb - free_kb;
	unsigned long pct      = total_kb > 0 ? used_kb * 100 / total_kb : 0;

	unsigned long used_pages = si.total_pages - si.free_pages;

	putchar('\n');
	puts("  ---------------------\n");
	puts("  Memory Info\n");
	puts("  ---------------------\n");
	puts("  Total:  ");
	putint(total_kb, 8);
	puts(" KB  (");
	putint(si.total_pages, 6);
	puts(" pages)\n");

	puts("  Used:   ");
	putint(used_kb, 8);
	puts(" KB  (");
	putint(used_pages, 6);
	puts(" pages)  ");
	putint(pct, 2);
	puts("%\n");

	puts("  Free:   ");
	putint(free_kb, 8);
	puts(" KB  (");
	putint(si.free_pages, 6);
	puts(" pages)  ");
	putint(100 - pct, 2);
	puts("%\n");
	puts("  ---------------------\n");
}

static void cmd_uname(void)
{
	struct uname_info u;
	uname(&u);
	puts(u.sysname);
	putchar(' ');
	puts(u.version);
	putchar(' ');
	puts(u.machine);
	putchar('\n');
}

/* ─── simstat: check SIM state via phone daemon IPC ─── */

#define FR_MAGIC0  0xAA
#define FR_MAGIC1  0x55
#define FR_BUF_MAX 512

static uint16_t crc16(const uint8_t *data, int len)
{
	uint16_t crc = 0xFFFF;
	for (int i = 0; i < len; i++) {
		crc ^= (uint16_t)data[i] << 8;
		for (int j = 0; j < 8; j++) {
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc <<= 1;
		}
	}
	return crc;
}

static void cmd_simstat(void)
{
	/* Tell the daemon to re-query AT+CPIN? (fire & forget via phone_fe). */
	int fd = open("/dev/phone_fe", O_RDWR);
	if (fd >= 0) {
		const char *json = "{\"type\":\"CMD_SIM_STAT\"}";
		int jlen = 0;
		while (json[jlen]) jlen++;
		uint8_t frame[FR_BUF_MAX];
		int total = 4 + jlen + 2;
		frame[0] = FR_MAGIC0;
		frame[1] = FR_MAGIC1;
		frame[2] = (uint8_t)(jlen & 0xFF);
		frame[3] = (uint8_t)((jlen >> 8) & 0xFF);
		int i;
		for (i = 0; i < jlen; i++) frame[4 + i] = (uint8_t)json[i];
		uint16_t crc_val = crc16(frame + 4, (size_t)jlen);
		frame[4 + jlen]     = (uint8_t)(crc_val & 0xFF);
		frame[4 + jlen + 1] = (uint8_t)((crc_val >> 8) & 0xFF);
		writefile(fd, (const char *)frame, (unsigned long)total);
		close(fd);
	}

	/* Yield a few times so the daemon has time to query the modem
	 * and update /SIMSTATE before we read it. */
	unsigned long deadline = getticks() * 10 + 4000;
	while (getticks() * 10 < deadline)
		yield();

	/* Read the status file the daemon wrote. */
	char state[24];
	int n = 0, sf = open("/SIMSTATE", O_RDONLY);
	if (sf < 0) {
		puts("SIM state: UNKNOWN (daemon not running?)\n");
		return;
	}
	long r;
	while (n < 23 && (r = read(sf, (uint8_t *)state + n, 1)) > 0)
		n++;
	close(sf);
	state[n] = '\0';

	puts("SIM state: ");
	puts(state);
	putchar('\n');
}

static void execute(char *line, char *cwd)
{
	char *argv[MAX_ARGS];
	int argc = 0;

	while (*line == ' ' || *line == '\t') {
		line++;
	}
	if (*line == '\0') {
		return;
	}

	argv[argc++] = line;
	while (*line) {
		if (*line == ' ' || *line == '\t') {
			*line++ = '\0';
			while (*line == ' ' || *line == '\t') {
				line++;
			}
			if (*line && argc < MAX_ARGS) {
				argv[argc++] = line;
			}
			continue;
		}
		line++;
	}

	if (argc == 0) {
		return;
	}

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
		}
	} else if (strcmp(cmd, "kill") == 0) {
		if (argc < 2) {
			puts("usage: kill <pid>\n");
		} else {
			int pid = 0;
			const char *s = argv[1];
			while (*s >= '0' && *s <= '9') {
				pid = pid * 10 + (*s++ - '0');
			}
			if (kill(pid) < 0) {
				puts("kill: pid not found\n");
			}
		}
	} else if (strcmp(cmd, "ps") == 0) {
		cmd_ps();
	} else if (strcmp(cmd, "info") == 0) {
		cmd_info();
	} else if (strcmp(cmd, "uname") == 0) {
		cmd_uname();
	} else if (strcmp(cmd, "reboot") == 0) {
		reboot(REBOOT_WARM);
	} else if (strcmp(cmd, "shutdown") == 0) {
		reboot(REBOOT_HALT);
	} else if (strcmp(cmd, "simstat") == 0) {
		cmd_simstat();
	} else {
		puts("Unknown: '");
		puts(cmd);
		puts("'. Type 'help' for available commands.\n");
	}
}

void main(void)
{
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
			if (n <= 0) {
				yield();
				continue;
			}

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
