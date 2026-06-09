/*
 * userspace_blobs.S - Embedded user-space program binaries
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

.section .rodata, "a"
	.globl	_binary_user_hello_start
	.globl	_binary_user_hello_end
_binary_user_hello_start:
	.incbin	"build/hello.usr"
_binary_user_hello_end:
