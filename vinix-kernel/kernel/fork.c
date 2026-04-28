/*
 * kernel/fork.c — fork() and process cloning
 */

#include "proc.h"
#include "task.h"
#include "scheduler.h"
#include "mmu.h"
#include "page_alloc.h"
#include "slab.h"
#include "uart.h"
#include "types.h"

extern void svc_exit_trampoline(void);

#define USER_MEM_PAGES_ORDER  8             /* 2^8 = 256 pages = 1 MB */
#define USER_MEM_SIZE         (1u << 20)    /* 1 MB */
#define KSTACK_PAGES_ORDER    0             /* 1 page = 4 KB */
#define KSTACK_SIZE           4096u
#define STACK_CANARY          0xDEADBEEFu

static void fork_build_child_stack(struct task_struct *child,
                                    void *kstack_base,
                                    struct svc_context *parent_ctx)
{
    *(uint32_t *)kstack_base = STACK_CANARY;

    uint32_t *sp = (uint32_t *)((char *)kstack_base + KSTACK_SIZE);

    /* Frame 1 — user context (high -> low).
     * svc_exit_trampoline pops this to return to user mode. */
    *--sp = parent_ctx->lr;     /* user return PC */
    *--sp = parent_ctx->r12;
    *--sp = parent_ctx->r11;
    *--sp = parent_ctx->r10;
    *--sp = parent_ctx->r9;
    *--sp = parent_ctx->r8;
    *--sp = parent_ctx->r7;
    *--sp = parent_ctx->r6;
    *--sp = parent_ctx->r5;
    *--sp = parent_ctx->r4;
    *--sp = parent_ctx->r3;
    *--sp = parent_ctx->r2;
    *--sp = parent_ctx->r1;
    *--sp = 0;                  /* r0 = 0 for child (fork return) */
    *--sp = 0;                  /* padding */
    *--sp = parent_ctx->spsr;   /* SPSR (user mode 0x10) */

    /* Frame 2 — kernel context popped by context_switch. */
    *--sp = (uint32_t)svc_exit_trampoline;  /* LR */
    for (int i = 0; i < 9; i++) { *--sp = 0; }  /* r3..r11 */

    child->context.sp = (uint32_t)sp;
}

int do_fork(struct svc_context *parent_ctx)
{
    struct task_struct *parent = current;
    if (parent == 0)
    {
        return -1;
    }

    /* Find free slot (skip idle and original shell). */
    int slot = -1;
    for (uint32_t i = 2; i < MAX_TASKS; i++)
    {
        if (tasks_array_get(i) == 0)
        {
            slot = (int)i;
            break;
        }
    }
    if (slot < 0)
    {
        pr_info("[FORK] no free task slot\n");
        return -1;
    }

    struct task_struct *child = kmalloc(sizeof(*child), GFP_KERNEL);
    if (child == 0)
    {
        return -1;
    }
    for (uint32_t b = 0; b < sizeof(*child); b++)
    {
        ((uint8_t *)child)[b] = 0;
    }

    uint32_t user_pa = alloc_pages(GFP_KERNEL, USER_MEM_PAGES_ORDER);
    if (user_pa == 0)
    {
        kfree(child);
        return -1;
    }

    uint32_t kstack_pa = alloc_pages(GFP_KERNEL, KSTACK_PAGES_ORDER);
    if (kstack_pa == 0)
    {
        free_pages(user_pa, USER_MEM_PAGES_ORDER);
        kfree(child);
        return -1;
    }

    uint32_t pgd_pa = mmu_new_pgd();
    if (pgd_pa == 0)
    {
        free_pages(kstack_pa, KSTACK_PAGES_ORDER);
        free_pages(user_pa, USER_MEM_PAGES_ORDER);
        kfree(child);
        return -1;
    }

    /* Copy parent user memory 1 MB (parent's VA 0x40000000 active now). */
    void *src = (void *)USER_SPACE_VA;
    void *dst = (void *)(user_pa + VA_OFFSET);
    for (uint32_t w = 0; w < USER_MEM_SIZE / 4u; w++)
    {
        ((uint32_t *)dst)[w] = ((uint32_t *)src)[w];
    }

    /* Install user section (1 MB) in child's pgd at VA 0x40000000. */
    uint32_t *child_pgd_va = (uint32_t *)(pgd_pa + VA_OFFSET);
    mmu_install_user_section(child_pgd_va, USER_SPACE_VA, user_pa);

    /* Build child's kernel stack mirroring parent's SVC frame. */
    void *kstack_va = (void *)(kstack_pa + VA_OFFSET);
    fork_build_child_stack(child, kstack_va, parent_ctx);
    child->stack_base = kstack_va;
    child->stack_size = KSTACK_SIZE;

    /* User SP/LR — read parent's live System-mode registers. */
    uint32_t parent_sp_usr, parent_lr_usr;
    __asm__ __volatile__(
        "cps #0x1F\n\t"
        "mov %0, sp\n\t"
        "mov %1, lr\n\t"
        "cps #0x13\n\t"
        : "=r"(parent_sp_usr), "=r"(parent_lr_usr) :: "memory");
    child->context.sp_usr = parent_sp_usr;
    child->context.lr_usr = parent_lr_usr;

    /* Process identity + state. */
    child->id           = (uint32_t)slot;
    child->pid          = slot;
    child->ppid         = parent->pid;
    child->exit_status  = 0;
    child->state        = TASK_RUNNING;
    child->pgd_pa       = pgd_pa;
    child->user_pa      = user_pa;
    child->user_order   = USER_MEM_PAGES_ORDER;
    child->kstack_pa    = kstack_pa;
    child->kstack_order = KSTACK_PAGES_ORDER;
    child->name         = "forked";

    /* Inherit fd table by value — no offset/flag sharing between processes. */
    for (uint32_t i = 0; i < MAX_FDS; i++)
    {
        child->files[i] = parent->files[i];
    }

    scheduler_add_forked(child);
    pr_info("[FORK] parent pid=%d -> child pid=%d (user_pa=0x%x pgd=0x%x)\n",
                parent->pid, child->pid, user_pa, pgd_pa);

    return slot;
}
