/* ============================================================
 * task.h
 * ------------------------------------------------------------
 * Task struct and task-management interface.
 * ============================================================ */

#ifndef TASK_H
#define TASK_H

#include "types.h"
#include "vfs.h"

/* ============================================================
 * Task Context Structure
 * ============================================================ */

/* CRITICAL: field order must exactly match context_switch.S —
 * any mismatch corrupts registers.
 * PC is NOT stored separately: interrupted tasks use LR_irq;
 * new tasks load PC from their initial stack frame. */
struct task_context {
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

    uint32_t sp;      /* r13_svc */
    uint32_t lr;      /* r14_svc */

    uint32_t spsr;
    uint32_t sp_usr;
    uint32_t lr_usr;
};

/* ============================================================
 * Task Control Block
 * ============================================================ */

/* Linux-aligned task states. RUNNING covers both "on CPU" and "on
 * runqueue waiting" — current task is identified by current pointer
 * comparison, not by separate state. */
#define TASK_RUNNING         0x00
#define TASK_INTERRUPTIBLE   0x01
#define TASK_UNINTERRUPTIBLE 0x02
#define TASK_ZOMBIE          0x20

/* CRITICAL: fields up to `id` have frozen offsets — context_switch.S
 * reads context.sp / context.sp_usr by offset. Add new fields AFTER id. */
struct task_struct {
    struct task_context context;    /* Saved CPU state (72 bytes) */
    void *stack_base;               /* Pointer to stack bottom */
    uint32_t stack_size;            /* Stack size in bytes */
    uint32_t state;                 /* TASK_RUNNING / INTERRUPTIBLE / ZOMBIE */
    const char *name;               /* Debug name */
    uint32_t id;                    /* Task ID */

    /* Sync — per-task linked-list slots + sleep deadline. */
    struct task_struct *wait_next;   /* next in a wait_queue_head */
    struct task_struct *sleep_next;  /* next in the sleep list */
    uint32_t            wake_tick;   /* jiffies at which msleep wakes */

    /* Process model — populated when added to scheduler. */
    int32_t             pid;         /* = scheduler slot index (0..4) */
    int32_t             ppid;        /* parent pid, -1 if none */
    int32_t             exit_status; /* exit(status) value, valid when ZOMBIE */
    uint32_t            pgd_pa;      /* TTBR0 for this task */

    /* Pages owned by this task — released by do_wait reaper.
     * Zero = not owned (idle/shell use static allocations). */
    uint32_t            user_pa;
    uint32_t            user_order;
    uint32_t            kstack_pa;
    uint32_t            kstack_order;

    /* Per-process file-descriptor table. Boot tasks zero-init in BSS;
     * fork copies parent's table by value — no shared offset/refcount. */
    struct vfs_fd       files[MAX_FDS];
};

/* ============================================================
 * Task Stack Initialization
 * ============================================================ */

/* Builds the first stack frame so the inaugural context_switch()
 * resumes at entry_point with SPSR=SVC+IRQ-on and zeroed registers.
 * `stack_base` is the high address; the stack grows downward. */
void task_stack_init(struct task_struct *task,
                     void (*entry_point)(void),
                     void *stack_base,
                     uint32_t stack_size);

#endif /* TASK_H */
