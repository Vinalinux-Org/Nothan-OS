/*
 * include/exception.h — ARMv7-A exception handling interface
 *
 * struct exception_context field order must match the STMFD push
 * sequence in exception_entry.S: r0-r12 then LR first, SPSR last
 * (at lowest address, where SP points on entry).
 */

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "types.h"


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



void handle_undefined_instruction(struct exception_context *ctx);
void handle_svc(struct exception_context *ctx);
void handle_prefetch_abort(struct exception_context *ctx);
void handle_data_abort(struct exception_context *ctx);
void handle_irq(struct exception_context *ctx);
void handle_fiq(struct exception_context *ctx);

#endif /* EXCEPTION_H */