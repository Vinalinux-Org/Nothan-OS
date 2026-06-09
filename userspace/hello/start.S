/*
 * userspace/hello/start.S - User-space entry point
 *
 * Sets up the C environment and calls main().
 * If main returns, exit via syscall #1.
 */
	.text
	.align 2
	.globl	_start
	.type	_start, %function
_start:
	bl	main
	@ exit(main's return value in r0) — __NR_exit = 1
	mov	r7, #1
	svc	#0
	b	_start
	.size	_start, . - _start
