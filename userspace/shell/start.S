/*
 * userspace/shell/start.S - User-space shell entry point
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
	.text
	.align 2
	.globl	_start
	.type	_start, %function
_start:
	bl	main
	mov	r7, #1
	svc	#0
	b	_start
	.size	_start, . - _start
