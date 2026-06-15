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

	.globl	_binary_user_example_start
	.globl	_binary_user_example_end
_binary_user_example_start:
	.incbin	"build/example.usr"
_binary_user_example_end:

	.globl	_binary_user_gui_start
	.globl	_binary_user_gui_end
_binary_user_gui_start:
	.incbin	"build/gui.usr"
_binary_user_gui_end:
