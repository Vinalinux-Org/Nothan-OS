/*
 * bin/info/main.c - Show system memory info
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../../lib/syscall.h"
#include "../../lib/stdio.h"

void main(void)
{
	struct sys_info si;
	sysinfo(&si);

	unsigned long total_kb = si.total_pages * 4;
	unsigned long free_kb  = si.free_pages * 4;
	unsigned long used_kb  = total_kb - free_kb;
	unsigned long pct      = total_kb > 0 ? used_kb * 100 / total_kb : 0;

	putchar('\n');
	puts("  ---------------------\n");
	puts("  Memory Info\n");
	puts("  ---------------------\n");
	puts("  Total:  "); putint(total_kb, 8); puts(" KB\n");
	puts("  Used:   "); putint(used_kb,  8); puts(" KB ("); putint(pct, 2);       puts("%)\n");
	puts("  Free:   "); putint(free_kb,  8); puts(" KB ("); putint(100 - pct, 2); puts("%)\n");
	puts("  ---------------------\n");
	user_exit(0);
}
