/*
 * arch/arm/mm/pgd.S - Page global directory space (16KB, 16KB-aligned)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * Must be placed right after kernel BSS by the linker script.
 */
.section .pgd, "aw"
.align 14
.space 16384
