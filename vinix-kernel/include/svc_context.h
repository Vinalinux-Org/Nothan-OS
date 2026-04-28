/* ============================================================
 * svc_context.h
 * ------------------------------------------------------------
 * SVC exception frame layout.
 * ============================================================ */

/* CRITICAL: field order must mirror exception_entry_svc in
 * exception_entry.S — keep in sync. */

#ifndef SVC_CONTEXT_H
#define SVC_CONTEXT_H

#include "types.h"

struct svc_context {
    uint32_t spsr;
    uint32_t pad;
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
    uint32_t lr;  /* user return PC (LR_svc) */
};

#endif /* SVC_CONTEXT_H */
