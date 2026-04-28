/* ============================================================
 * payload.S
 * ------------------------------------------------------------
 * Embeds init — copied to USER_SPACE_VA and run as PID 1.
 * ============================================================ */

.syntax unified
.section .rodata

.global _shell_payload_start
.global _shell_payload_end

.align 4
_shell_payload_start:
    .incbin "../userspace/build/apps/init/init.bin"
_shell_payload_end:
