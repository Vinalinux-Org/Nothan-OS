/*
 * kernel/userspace_blobs.S - Embedded user-space program binaries
 *
 * Each user binary is built separately, placed here via .incbin,
 * and exposed as _binary_user_<name>_start / _binary_user_<name>_end
 * symbols for loading by fork.c.
 */
	.section .rodata, "a"
	.globl	_binary_user_hello_start
	.globl	_binary_user_hello_end
_binary_user_hello_start:
	.incbin	"build/hello.usr"
_binary_user_hello_end:
