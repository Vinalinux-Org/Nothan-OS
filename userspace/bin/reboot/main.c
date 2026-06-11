/*
 * bin/reboot/main.c - Reboot the system
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "../../lib/syscall.h"

void main(void)
{
	reboot(REBOOT_WARM);
	while (1);
}
