/*
 * kernel/userspace_blobs.S - the one user-space binary embedded in the kernel
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * Only init lives here. Every other program is a file on the disk, loaded by
 * path at runtime - the kernel keeps no list of what exists.
 *
 * init is the exception because it is the bootstrap: when it is created there
 * is no process to have asked for it. Embedding it also means the machine can
 * always reach user space, even with an unreadable SD card - init then starts
 * nothing and says so, which is a diagnosable boot rather than a dead one.
 */
.section .rodata, "a"
	.globl	_binary_user_init_start
	.globl	_binary_user_init_end
_binary_user_init_start:
	.incbin	"build/init.usr"
_binary_user_init_end:
