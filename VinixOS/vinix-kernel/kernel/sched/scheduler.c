/* ============================================================
 * scheduler.c
 * ------------------------------------------------------------
 * Simple round-robin preemptive scheduler
 * ============================================================ */

#include "scheduler.h"
#include "task.h"
#include "sleep.h"
#include "mmu.h"
#include "uart.h"
#include "cpu.h"
#include "string.h"
#include "types.h"
#include "trace.h"
#include "assert.h"
#include "syscalls.h" /* For process_info_t */

/* ============================================================
 * External Assembly Functions
 * ============================================================ */

/* Defined in context_switch.S */
extern void context_switch(struct task_struct *current, struct task_struct *next);
extern void start_first_task(struct task_struct *first);

/* ============================================================
 * Scheduler Data Structures
 * ============================================================ */

/* Static task array */
static struct task_struct *tasks[MAX_TASKS];
static uint32_t task_count = 0;

/* Stack Canary Value (Must match task.c) */
#define STACK_CANARY_VALUE  0xDEADBEEF

/* Current running task */
static struct task_struct *current_task = NULL;

/* Next task index for round-robin */
static uint32_t current_task_index = 0;

/* Scheduler enabled flag */
static bool scheduler_started = false;

/* ============================================================
 * Scheduler Implementation
 * ============================================================ */

/**
 * Initialize scheduler
 * Must be called before any other scheduler functions
 */
void scheduler_init(void)
{
    pr_info("[SCHED] Initializing scheduler...\n");
    
    /* Reset task array */
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i] = NULL;
    }
    
    /* 
     * Verify structure alignment for assembly access
     * Offsets `context.sp` and `context.sp_usr` must match definition in `task_struct`.
     */
    if (__builtin_offsetof(struct task_struct, context.sp) != 52 ||
        __builtin_offsetof(struct task_struct, context.sp_usr) != 64 ||
        __builtin_offsetof(struct task_struct, pgd_pa) != 116) {
        PANIC("Struct alignment mismatch with Assembly!");
    }
    
    task_count = 0;
    current_task = NULL;
    current_task_index = 0;
    scheduler_started = false;
    
    TRACE_SCHED("Scheduler initialized (MAX_TASKS: %d)", MAX_TASKS);
}

int scheduler_add_task(struct task_struct *task)
{
    if (task_count >= MAX_TASKS) {
        pr_err("[SCHED] ERROR: Task table full (%d/%d)\n", 
                    task_count, MAX_TASKS);
        return -1;
    }
    
    /* Add task to array */
    tasks[task_count] = task;
    task->id = task_count;
    task->state = TASK_RUNNING;

    /* Process model: pid == slot index, no parent yet (fork overrides ppid). */
    task->pid         = task_count;
    task->ppid        = -1;
    task->exit_status = 0;
    if (task->pgd_pa == 0)
    {
        task->pgd_pa = mmu_kernel_pgd_pa();
    }
    
    pr_info("[SCHED] added task %d: '%s'\n",
                task->id, task->name ? task->name : "(unnamed)");

    task_count++;
    
    return task->id;
}

/**
 * Start the scheduler
 * This function does NOT return
 * 
 * REQUIRES:
 * - At least one task added
 * - IRQ enabled in CPSR
 */
void scheduler_start(void)
{
    if (task_count == 0) {
        pr_err("[SCHED] ERROR: no tasks to run\n");
        while (1);
    }

    current_task = tasks[0];
    current_task_index = 0;
    current_task->state = TASK_RUNNING;
    scheduler_started = true;

    pr_info("[SCHED] starting with %d task(s), first='%s'\n",
                task_count, current_task->name);
    
    /* Load first task context and jump to it
     * This will:
     * - Load task's stack pointer
     * - Restore task's SPSR
     * - Restore task's registers
     * - Jump to task entry point via MOVS PC, LR
     */
    start_first_task(current_task);
    
    /* Should never reach here */
    pr_info("[SCHED] FATAL: Returned from start_first_task!\n");
    while (1);
}

/* ============================================================
 * Global Reschedule Flag
 * ============================================================
 * 
 * Set by IRQ handler (scheduler_tick) when time slice expires.
 * Checked by tasks in their main loop.
 * Cleared when context switch completes.
 */
volatile bool need_reschedule = false;

/* CRITICAL: runs in IRQ mode — cannot context_switch() here
 * because IRQ stack is shared and nested IRQs would corrupt it.
 * Set need_reschedule and let tasks yield voluntarily.
 */
void scheduler_tick(void)
{
    if (!scheduler_started) {
        return;
    }

    /* Walk the sleep list, wake anyone whose wake_tick has passed. */
    sleep_tick();

    /* IRQ-safe: just flag. Tasks pick this up via schedule(). */
    need_reschedule = true;
}

/* ============================================================
 * scheduler_terminate_task - Kill a task
 * ============================================================
 * 
 * Called when a task crashes (Data/Prefetch Abort)
 */
void scheduler_terminate_task(uint32_t id)
{
    if (id >= MAX_TASKS || tasks[id] == NULL) {
        return;
    }
    
    struct task_struct *task = tasks[id];
    
    pr_info("[SCHED] TERMINATING Task %d: '%s'\n", task->id, task->name);
    
    /* Mark as ZOMBIE */
    task->state = TASK_ZOMBIE;
    
    /* If current task is killing itself, yield immediately */
    if (current_task == task) {
        pr_info("[SCHED] Task %d IS SUICIDE - Yielding...\n", task->id);
        
        /* 
         * CRITICAL: We are likely in ABT/UND Mode (Exception Context).
         * We MUST switch to SVC Mode before calling schedule/context_switch.
         * Otherwise, context_switch will use SP_abt/und which is wrong for the next task.
         * 
         * Logic:
         * 1. Switch to SVC Mode (keeping IRQs disabled).
         * 2. Call schedule().
         * 
         * Note: We don't care about saving the current ABT stack/regs because 
         * this task is DEAD. We just need a safe environment to switch FROM.
         */
        
        /* Switch to SVC Mode (0x13) | IRQ Disabled (0x80) | FIQ Disabled (0x40) */
        __asm__ volatile (
            "cps #0x13 \n\t"
        );
        
        need_reschedule = true; /* Force yield */
        schedule();
    }
}

/* ============================================================
 * schedule - Voluntary Task Switch
 * ============================================================
 * 
 * Called by tasks when they detect need_reschedule flag.
 * Runs in SVC mode (task context), so safe to switch.
 */
void schedule(void)
{
    struct task_struct *prev_task;
    struct task_struct *next_task;
    uint32_t next_index;

    if (!need_reschedule) {
        return;
    }

    need_reschedule = false;

    if (current_task != NULL) {
        uint32_t *canary_ptr = (uint32_t *)current_task->stack_base;
        if (*canary_ptr != STACK_CANARY_VALUE) {
            TRACE_SCHED("FATAL: Stack overflow detected in task %d ('%s')",
                        current_task->id, current_task->name);
            pr_info("[SCHED] Canary Addr: 0x%08x. Expected: 0x%08x. Actual: 0x%08x\n",
                        (uint32_t)canary_ptr, STACK_CANARY_VALUE, *canary_ptr);
            PANIC("Stack Canary Corrupted!");
        }
    }

    if (task_count == 1) {
        return;
    }

    prev_task = current_task;

    next_index = current_task_index;
    next_task = prev_task;
    uint32_t search_count = 0;

    while (search_count < MAX_TASKS) {
        next_index = (next_index + 1) % MAX_TASKS;
        search_count++;

        if (tasks[next_index] != NULL &&
            tasks[next_index]->state == TASK_RUNNING) {
            next_task = tasks[next_index];
            break;
        }
    }

    /* Sanity check + fallback */
    if (next_task == prev_task) {
        if (prev_task->state == TASK_ZOMBIE) {
            if (tasks[0] != NULL && tasks[0]->state != TASK_ZOMBIE) {
                pr_info("[SCHED] Warning: No READY task found, forcing idle task READY to prevent deadlock\n");
                tasks[0]->state = TASK_RUNNING;
                next_task = tasks[0];
                next_index = 0;
            } else {
                PANIC("Scheduler Deadlock - No runnable tasks!");
            }
        } else {
            return;
        }
    }

    if (next_task->state != TASK_RUNNING) {
        pr_err("[SCHED] ERROR: Task %u not READY (state=%u)\n",
                    next_task->id, next_task->state);
        
        /* Fallback: Force idle to READY if it exists */
        if (tasks[0] != NULL) {
            pr_info("[SCHED] Warning: Forcing idle task READY to prevent deadlock\n");
            tasks[0]->state = TASK_RUNNING;
            next_task = tasks[0];
            next_index = 0;
        } else {
            PANIC("Scheduler Deadlock - No READY tasks!");
        }
    }
    
    /* Keep BLOCKED and ZOMBIE as-is; only a RUNNING task returns to READY. */
    if (prev_task->state == TASK_RUNNING) {
        prev_task->state = TASK_RUNNING;
    }
    next_task->state = TASK_RUNNING;

    /* Update global scheduler state */
    current_task_index = next_index;
    current_task = next_task;

    /* CRITICAL: TTBR0 swap lives inside context_switch() asm, between
     * saving prev's SP and loading next's — otherwise either stack VA
     * may briefly resolve under the wrong pgd. */
    context_switch(prev_task, next_task);
}

struct task_struct *scheduler_current_task(void)
{
    return current_task;
}

struct task_struct *tasks_array_get(uint32_t idx)
{
    if (idx >= MAX_TASKS) return 0;
    return tasks[idx];
}

int scheduler_add_forked(struct task_struct *task)
{
    uint32_t slot = task->id;
    if (slot >= MAX_TASKS || tasks[slot] != 0)
    {
        return -1;
    }
    tasks[slot] = task;
    task->state = TASK_RUNNING;
    task_count++;
    return (int)slot;
}

void scheduler_release_slot(uint32_t idx)
{
    if (idx >= MAX_TASKS) return;
    if (tasks[idx] != 0)
    {
        tasks[idx] = 0;
        if (task_count > 0) task_count--;
    }
}

/* Caller (svc_handler) has already validated buf via validate_user_pointer(). */
int scheduler_get_tasks(void *buf, uint32_t max_count)
{
    process_info_t *info = (process_info_t *)buf;
    uint32_t count = 0;


    for (int i = 0; i < MAX_TASKS && count < max_count; i++) {
        struct task_struct *t = tasks[i];
        if (t != NULL) {
            info[count].id = t->id;
            
            /* Copy name using string library */
            const char *src = t->name ? t->name : "unknown";
            int copy_len = 0;
            while(src[copy_len] && copy_len < 31) {
                copy_len++;
            }
            memcpy(info[count].name, src, copy_len);
            info[count].name[copy_len] = '\0';
            
            info[count].state = t->state;
            
            count++;
        }
    }
    
    return count;
}
