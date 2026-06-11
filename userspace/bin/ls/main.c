/*
 * bin/ls/main.c - List files in root directory
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../../lib/syscall.h"
#include "../../lib/stdio.h"

void main(void)
{
	struct file_entry files[16];
	long count = listdir("/", files, 16);
	if (count < 0) { puts("ls: failed\n"); user_exit(1); }
	if (count == 0) { puts("(empty)\n"); user_exit(0); }

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
	user_exit(0);
}
