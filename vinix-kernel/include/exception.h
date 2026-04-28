/* ============================================================
 * exception.h
 * ------------------------------------------------------------
 * ARMv7-A exception handling interface.
 * ============================================================ */

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "types.h"

/* CRITICAL: layout matches exception_entry stubs — STMFD pushes
 * r0-r12,LR first, then SPSR last. SPSR ends up at the lowest
 * address (SP points here). */
struct exception_context {
    uint32_t spsr;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t lr;
};

/* ============================================================
 * Exception Handler Prototypes — must NOT return (fatal).
 * ============================================================ */

void handle_undefined_instruction(struct exception_context *ctx);
void handle_svc(struct exception_context *ctx);
void handle_prefetch_abort(struct exception_context *ctx);
void handle_data_abort(struct exception_context *ctx);
void handle_irq(struct exception_context *ctx);
void handle_fiq(struct exception_context *ctx);

#endif /* EXCEPTION_H */