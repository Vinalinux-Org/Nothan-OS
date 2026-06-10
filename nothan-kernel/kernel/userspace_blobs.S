/*
 * kernel/userspace_blobs.S - Embedded user-space program binaries
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
.section .rodata, "a"
	.globl	_binary_user_shell_start
	.globl	_binary_user_shell_end
_binary_user_shell_start:
	.incbin	"build/shell.usr"
_binary_user_shell_end:
