/* ============================================================
 * task.h
 * ------------------------------------------------------------
 * Task structure and task management for VinixOS
 * Target: BeagleBone Black (ARMv7-A)
 *
 * EDUCATIONAL NOTE:
 *   The task_struct is the "Task Control Block" (TCB) — the
 *   kernel's representation of a running (or runnable) task.
 *   It stores everything the kernel needs to suspend and resume
 *   a task: CPU registers, stack pointer, scheduling metadata.
 *
 *   Fields used only by specific algorithms (e.g., deadline
 *   and period for EDF) are always present in the struct for
 *   simplicity.  They are ignored when a different algorithm
 *   is selected at compile time.
 * ============================================================ */

#ifndef TASK_H
#define TASK_H

#include "types.h"

/* ============================================================
 * Task Context Structure
 * ============================================================ */

/**
 * Task CPU context - saved/restored during context switch
 * 
 * CRITICAL: Field order must EXACTLY match context_switch.S
 * assembly code. Any mismatch will cause register corruption.
 * 
 * Stack frame layout (low → high address):
 *   [SPSR] [LR] [r0-r12] ← SP
 *   Total: 60 bytes
 * 
 * Per ARM Architecture Manual Part B (Exception Handling):
 * - r0-r12: General purpose registers (shared across modes)
 * - SP: r13_svc (task's stack pointer in SVC mode)
 * - LR: r14_svc (task's link register in SVC mode)
 * - SPSR: Saved Program Status Register (task's CPSR snapshot)
 * 
 * NOTE: PC is NOT stored separately
 * - For interrupted tasks: PC is in LR_irq (handled by exception)
 * - For new tasks: Initial PC in stack frame (loaded by context restore)
 */
struct task_context {
    /* General purpose registers (52 bytes) */
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
    
    /* Mode-specific registers - SVC mode (8 bytes) */
    uint32_t sp;      // r13_svc: Stack pointer
    uint32_t lr;      // r14_svc: Link register (return address)
    
    /* Processor state (4 bytes) */
    uint32_t spsr;    // Saved Program Status Register
    
    /* User Mode Stack Pointer (Added for Phase 7 Fix) */
    uint32_t sp_usr;  // r13_usr
    uint32_t lr_usr;  // r14_usr
    
    // Total size: 72 bytes (18 words)
};

/* ============================================================
 * Task Control Block
 * ============================================================ */

/**
 * Task state values
 */
#define TASK_STATE_READY    0   /* Ready to run */
#define TASK_STATE_RUNNING  1   /* Currently executing */
#define TASK_STATE_BLOCKED  2   /* Waiting for event (not used yet) */
#define TASK_STATE_ZOMBIE   3   /* Task terminated/killed */

/**
 * Task Control Block (TCB) — complete task descriptor
 *
 * LAYOUT WARNING:
 *   The 'context' field MUST remain the first member of this
 *   struct.  Assembly code (context_switch.S) accesses fields
 *   at fixed byte offsets from the struct pointer:
 *     context.sp      @ offset 52
 *     context.sp_usr  @ offset 64
 *   Adding fields AFTER 'id' is safe; inserting fields before
 *   or inside 'context' will break the assembly contract.
 */
struct task_struct {
    struct task_context context;    /* Saved CPU state (72 bytes) */
    void *stack_base;               /* Pointer to stack bottom */
    uint32_t stack_size;            /* Stack size in bytes */
    uint32_t state;                 /* Task state (READY/RUNNING/BLOCKED) */
    const char *name;               /* Task name (for debugging) */
    uint32_t id;                    /* Task ID */

    /* ----------------------------------------------------------
     * EDF (Early Deadline First) scheduling parameters
     * ----------------------------------------------------------
     * These fields are used only when SCHED_ALGO_EDF is selected.
     * When using round-robin, they are present but ignored.
     *
     *   deadline — Absolute deadline expressed in scheduler ticks.
     *              The EDF algorithm always picks the READY task
     *              with the smallest deadline value.
     *              A value of 0 means "no deadline" (treated as
     *              the largest possible deadline, i.e., lowest
     *              priority in EDF terms).
     *
     *   period   — For periodic real-time tasks, the interval
     *              (in ticks) between successive deadlines.
     *              After the task yields, its deadline is advanced
     *              by this amount.  0 means aperiodic (one-shot
     *              or best-effort task).
     * ---------------------------------------------------------- */
    uint32_t deadline;              /* Absolute deadline (ticks) */
    uint32_t period;                /* Period for periodic tasks  */
};

/* ============================================================
 * Task Stack Initialization
 * ============================================================ */

/**
 * Initialize task stack for a new task
 * 
 * Creates initial stack frame so that when context_switch() loads
 * the task for the first time, it will:
 * - Start executing at entry_point
 * - Have initial CPSR in SVC mode with IRQ enabled
 * - Have clean register state
 * 
 * @param task        Pointer to task structure
 * @param entry_point Task entry point function
 * @param stack_base  Pointer to stack bottom (high address)
 * @param stack_size  Stack size in bytes
 * 
 * Stack layout after init (grows down from stack_base):
 *   [SPSR: SVC mode, IRQ enabled]
 *   [LR: entry_point]
 *   [r0-r12: all zeros]
 *   ← task->context.sp (points here)
 */
void task_stack_init(struct task_struct *task, 
                     void (*entry_point)(void),
                     void *stack_base, 
                     uint32_t stack_size);

#endif /* TASK_H */
