/*
 * lib/crt0.S - C runtime startup for user-space programs
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
/*
 * The kernel hands us a stack with argc/argv already on it (see
 * setup_argv_block in kernel/spawn.c):
 *
 *   [sp+4 ...] argv[0], argv[1], ..., NULL
 *   [sp]       argc
 *
 * so argc is one load and argv is one add. sp is 8-byte aligned on entry,
 * which is what AAPCS requires of a call boundary, so main() can be called
 * without touching it.
 *
 * main() may be declared int main(void) - it just ignores r0/r1, which are
 * caller-saved. Programs that want their arguments declare the full form.
 */
	.text
	.align 2
	.globl	_start
	.type	_start, %function
_start:
	ldr	r0, [sp]		@ argc
	add	r1, sp, #4		@ argv
	bl	main
	mov	r7, #1			@ __NR_exit; r0 is main's return value
	svc	#0
	b	_start
	.size	_start, . - _start
