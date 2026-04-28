/* ============================================================
 * exception_handlers.c
 * ------------------------------------------------------------
 * Exception handlers (C implementation)
 * ============================================================ */

#include "exception.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
#include "assert.h"
#include "proc.h"

/* SIGSEGV exit code — parent sees 139 = 128 + 11 from wait(). */
#define FAULT_EXIT_SIGSEGV  139

/* ============================================================
 * Helper Functions
 * ============================================================ */

static void print_exception_context(struct exception_context *ctx)
{
    pr_info("\n");
    pr_info("Exception Context:\n");
    pr_info("------------------\n");
    pr_info("PC (LR):   0x%08x\n", ctx->lr);
    pr_info("SPSR:      0x%08x\n", ctx->spsr);
    pr_info("\n");
    pr_info("Registers:\n");
    pr_info("r0:  0x%08x    r1:  0x%08x\n", ctx->r0, ctx->r1);
    pr_info("r2:  0x%08x    r3:  0x%08x\n", ctx->r2, ctx->r3);
    pr_info("r4:  0x%08x    r5:  0x%08x\n", ctx->r4, ctx->r5);
    pr_info("r6:  0x%08x    r7:  0x%08x\n", ctx->r6, ctx->r7);
    pr_info("r8:  0x%08x    r9:  0x%08x\n", ctx->r8, ctx->r9);
    pr_info("r10: 0x%08x    r11: 0x%08x\n", ctx->r10, ctx->r11);
    pr_info("r12: 0x%08x\n", ctx->r12);
    pr_info("\n");
}

static const char *decode_cpu_mode(uint32_t spsr)
{
    switch (spsr & 0x1F)
    {
    case 0x10:
        return "USR";
    case 0x11:
        return "FIQ";
    case 0x12:
        return "IRQ";
    case 0x13:
        return "SVC";
    case 0x17:
        return "ABT";
    case 0x1B:
        return "UND";
    case 0x1F:
        return "SYS";
    default:
        return "???";
    }
}

/**
 * Halt kernel with error message
 */
static void halt_kernel(void)
{
    pr_info("\n");
    pr_info("====================================\n");
    pr_info("       KERNEL HALTED (FATAL)        \n");
    pr_info("====================================\n");
    pr_info("\n");

    /* Infinite loop */
    while (1)
        ;
}

/* ============================================================
 * Exception Handlers
 * ============================================================ */

/**
 * Undefined Instruction Exception Handler
 *
 * Called when CPU encounters an invalid or unsupported instruction.
 * This is a FATAL exception - no recovery attempted.
 */
void handle_undefined_instruction(struct exception_context *ctx)
{
    pr_info("\n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    pr_info("!! UNDEFINED INSTRUCTION EXCEPTION\n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    pr_info("\n");
    pr_info("The CPU encountered an invalid or unsupported instruction.\n");
    pr_info("This is a FATAL exception.\n");

    pr_info("\n");
    pr_info("Possible causes:\n");
    pr_info("  - Invalid opcode in code\n");
    pr_info("  - VFP/NEON instruction without enabling coprocessor\n");
    pr_info("  - Corrupted code in memory\n");
    pr_info("  - Executing data as code\n");

    pr_info("\n");
    pr_info("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);

    halt_kernel();
}

/**
 * Supervisor Call (SVC) Exception Handler (Fallback)
 *
 * This handler should NOT be reached - SVC is handled by dedicated
 * assembly stub that calls svc_handler() directly.
 * If this executes, it indicates a configuration error.
 */
void handle_svc(struct exception_context *ctx)
{
    pr_info("\n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    pr_info("!! SUPERVISOR CALL (SVC) EXCEPTION\n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    pr_info("\n");
    pr_info("SVC instruction was executed.\n");
    pr_info("System calls should be handled by svc_handler(), not here.\n");

    pr_info("\n");
    pr_info("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);

    halt_kernel();
}

/**
 * Prefetch Abort Exception Handler
 *
 * Called when instruction fetch fails.
 * Terminates user tasks on fault, panics on kernel faults.
 */
void handle_prefetch_abort(struct exception_context *ctx)
{
    pr_info("\n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    pr_info("!! PREFETCH ABORT EXCEPTION        \n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    pr_info("\n");
    pr_info("Instruction fetch failed.\n");
    pr_info("This is a FATAL exception.\n");

    pr_info("\n");
    pr_info("Possible causes:\n");
    pr_info("  - Branching to invalid address\n");
    pr_info("  - MMU permission fault (if MMU enabled)\n");
    pr_info("  - Memory not present at fetch address\n");
    pr_info("  - Alignment fault\n");

    pr_info("\n");
    pr_info("\n");
    pr_info("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);

    /* FAULT CONTAINMENT LOGIC */
    if ((ctx->spsr & 0x1F) == 0x10)
    {
        pr_err("[FAULT] User prefetch abort: task %d ('%s') — SIGSEGV\n",
                    current ? current->id : -1,
                    current ? current->name : "???");
        if (!current) PANIC("User fault with no current task");

        /* Switch to SVC before calling C kernel paths that touch task state. */
        __asm__ volatile ("cps #0x13");
        do_exit(FAULT_EXIT_SIGSEGV);
        return;
    }

    pr_err("[FAULT] KERNEL PANIC: Prefetch Abort in Privileged Mode!\n");
    halt_kernel();
}

/**
 * Decode DFSR Fault Status (FS) field
 * ARMv7-A short-descriptor format: FS = DFSR[10,3:0]
 */
static const char *decode_fault_status(uint32_t dfsr)
{
    /* FS = bit[10] concatenated with bits[3:0] */
    uint32_t fs = (dfsr & 0xF) | ((dfsr >> 6) & 0x10);

    switch (fs)
    {
    case 0x01:
        return "Alignment fault";
    case 0x02:
        return "Debug event";
    case 0x04:
        return "Instruction cache maintenance fault";
    case 0x05:
        return "Translation fault (Section)";
    case 0x07:
        return "Translation fault (Page)";
    case 0x08:
        return "Synchronous external abort (non-translation)";
    case 0x09:
        return "Domain fault (Section)";
    case 0x0B:
        return "Domain fault (Page)";
    case 0x0C:
        return "Sync external abort on translation (L1)";
    case 0x0D:
        return "Permission fault (Section)";
    case 0x0E:
        return "Sync external abort on translation (L2)";
    case 0x0F:
        return "Permission fault (Page)";
    case 0x16:
        return "Asynchronous external abort";
    default:
        return "Unknown fault";
    }
}

/**
 * Data Abort Exception Handler
 *
 * Called when data access (load/store) fails.
 * Reads DFSR/DFAR for detailed fault diagnostics.
 * Terminates user tasks on fault, panics on kernel faults.
 */
void handle_data_abort(struct exception_context *ctx)
{
    /* Read Fault Status and Address registers (CP15 c5/c6)
     * ARM ARM B4.1.52 (DFSR) and B4.1.44 (DFAR) */
    uint32_t dfsr, dfar;
    __asm__ __volatile__("mrc p15, 0, %0, c5, c0, 0" : "=r"(dfsr));
    __asm__ __volatile__("mrc p15, 0, %0, c6, c0, 0" : "=r"(dfar));

    pr_info("\n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    pr_info("!! DATA ABORT EXCEPTION            \n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    pr_info("\n");
    pr_info("Data access (load/store) failed.\n");

    /* Fault diagnostics from DFSR/DFAR */
    pr_info("\n");
    pr_info("Fault Details:\n");
    pr_info("  DFAR (Fault Address): 0x%08x\n", dfar);
    pr_info("  DFSR (Fault Status):  0x%08x\n", dfsr);
    pr_info("  Fault Type: %s\n", decode_fault_status(dfsr));
    pr_info("  Access Type: %s\n", (dfsr & (1 << 11)) ? "WRITE" : "READ");
    pr_info("  Domain: %u\n", (dfsr >> 4) & 0xF);

    pr_info("\n");
    pr_info("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);

    /* FAULT CONTAINMENT LOGIC */
    if ((ctx->spsr & 0x1F) == 0x10)
    {
        pr_err("[FAULT] User data abort at PC=0x%08x DFAR=0x%08x: task %d ('%s') — SIGSEGV\n",
                    ctx->lr, dfar,
                    current ? current->id : -1,
                    current ? current->name : "???");
        if (!current) PANIC("User fault with no current task");

        __asm__ volatile ("cps #0x13");
        do_exit(FAULT_EXIT_SIGSEGV);
        return;
    }

    pr_err("[FAULT] KERNEL PANIC: Data Abort in Privileged Mode!\n");
    halt_kernel();
}

/**
 * IRQ Exception Handler (Fallback)
 *
 * This handler should NOT be reached - IRQ is handled by dedicated
 * assembly stub that calls irq_handler() directly.
 * If this executes, it indicates a configuration error.
 */
void handle_irq(struct exception_context *ctx)
{
    pr_info("\n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    pr_info("!! UNEXPECTED IRQ EXCEPTION        \n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    pr_info("\n");
    pr_info("IRQ exception occurred, but IRQ should be MASKED!\n");
    pr_info("This indicates a kernel bug.\n");

    pr_info("\n");
    pr_info("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);

    halt_kernel();
}

/**
 * FIQ Exception Handler (Stub)
 *
 * Called when fast interrupt occurs.
 * FIQ is NOT USED in this project.
 * If this executes, it indicates a serious bug.
 */
void handle_fiq(struct exception_context *ctx)
{
    pr_info("\n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    pr_info("!! UNEXPECTED FIQ EXCEPTION        \n");
    pr_info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    pr_info("\n");
    pr_info("FIQ exception occurred, but FIQ is NOT USED!\n");
    pr_info("This indicates a serious kernel bug.\n");

    pr_info("\n");
    pr_info("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);

    halt_kernel();
}

/* ============================================================
 * Future Enhancements
 * ------------------------------------------------------------
 * 1. Read IFSR/IFAR for Prefetch Abort diagnostics (similar to DFSR/DFAR)
 * 2. Stack trace / backtrace capability
 * 3. Exception recovery for recoverable faults (future)
 * ============================================================ */