/*
 * bin/shutdown/main.c - Halt the system
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../../lib/syscall.h"

void main(void)
{
	reboot(REBOOT_HALT);
	while (1);
}
