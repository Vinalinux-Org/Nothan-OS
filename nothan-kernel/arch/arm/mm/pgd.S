/*
 * Page table space — 16KB aligned to 16KB boundary.
 * Must be placed right after kernel BSS by the linker script.
 */
.section .pgd, "aw"
.align 14
.space 16384
