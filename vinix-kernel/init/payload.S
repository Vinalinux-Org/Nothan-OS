/*
 * init/payload.S — embedded user payload
 *
 * Includes the init binary as raw bytes in .rodata.
 * main.c copies it to USER_SPACE_VA and launches it as PID 1.
 */

.syntax unified
.section .rodata

.global _shell_payload_start
.global _shell_payload_end

.align 4
_shell_payload_start:
    .incbin "../userspace/build/apps/init/init.bin"
_shell_payload_end:
