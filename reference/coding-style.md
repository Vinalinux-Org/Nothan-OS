# VinixOS — Coding Style

## Table of Contents

1. [Coding Style](#1-coding-style)
2. [Naming Convention](#2-naming-convention)
3. [File Layout](#3-file-layout)

---

## 1. Coding Style

### Hard rules

| Rule | Lý do |
| --- | --- |
| 4 space indent, **không tab** | Diff ổn định cross-editor |
| Line ≤ 100 cột | Đọc song song 2 file trên màn hình |
| `snake_case` cho variable / function | Linux convention |
| `UPPER_SNAKE` cho macro / `#define` | Phân biệt với function ngay từ caller site |
| `PascalCase` chỉ cho enum tag (`enum TaskState`) — value vẫn `UPPER_SNAKE` | Linux convention |
| Public symbol BẮT BUỘC module prefix | Tránh collision khi link kernel |
| Include dùng `""` — `<>` chỉ cho `<stdarg.h>` | Project tự host header, không có system include |

### Brace style

```c
/* Function — brace xuống dòng mới */
static int omap_uart_probe(struct platform_device *pdev)
{
    if (!pdev)
        return -EINVAL;

    /* Control flow — brace cùng dòng */
    if (irq < 0) {
        pr_err("[UART] invalid IRQ\n");
        return -EINVAL;
    }

    return 0;
}
```

Lý do: Linux dùng exact pattern này. Function-brace-newline làm grep `^{` tìm function definition dễ.

### Variable declaration

```c
/* Đúng — declare ở đầu scope, gom theo type */
static int uart_probe(struct platform_device *pdev)
{
    struct resource *mem;
    uint32_t base;
    int irq;

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    irq = platform_get_irq(pdev, 0);
    base = mem ? mem->start : UART0_BASE;
    ...
}
```

```c
/* Sai — C99 mid-scope declaration, mix type & init */
static int uart_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq = platform_get_irq(pdev, 0);
    uint32_t base = mem ? mem->start : UART0_BASE;

    if (irq < 0) {
        struct foo bar;  /* khai báo giữa scope — NO */
        ...
    }
}
```

**Exception**: loop counter `for (int i = 0; ...)` được phép — quá phổ biến, phản đối là cứng nhắc.

### Static vs public

- File-local function / state → **luôn** `static`
- Public symbol → khai báo trong header, **bắt buộc** module prefix (xem [§2](#2-naming-convention))
- Không dùng `extern` trong .c file — chỉ trong .h

### Integer types

```c
/* HW register / fixed-width */
uint32_t reg;
uint8_t  byte;

/* Generic int */
int      err;
size_t   len;

/* KHÔNG dùng */
unsigned long  /* size phụ thuộc platform — bare-metal phải explicit */
long           /* same */
```

### Assembly

- Inline comment: `@` (ARM convention)
- Banner giống C
- Local label: `1:`, `2:` (numeric forward reference)
- Function nameless symbol cấm — phải có `.global <name>` + label

```asm
@ Save callee-saved registers before C call
push {r4-r11, lr}

bl  c_function

pop {r4-r11, lr}
bx  lr
```

---

## 2. Naming Convention

### Module prefix — bắt buộc cho mọi public symbol

| Subsystem | Prefix | Ví dụ |
| --- | --- | --- |
| UART | `uart_` | `uart_putc`, `uart_register_driver` |
| Scheduler | `scheduler_` / `sched_` | `scheduler_tick`, `sched_init` |
| Interrupt | `intc_` | `intc_enable_irq` |
| VFS | `vfs_` | `vfs_open`, `vfs_read` |
| Platform | `platform_` | `platform_get_resource` |
| MMU | `mmu_` | `mmu_map_page` |
| Block | `blk_` | `blk_register` |
| MMC | `mmc_` | `mmc_alloc_host` |
| Framebuffer | `fb_` | `fb_register` |
| I2C | `i2c_` | `i2c_add_adapter` |

Vendor-specific driver thêm SoC prefix: `omap_uart_*`, `omap_hsmmc_*`, `tilcdc_*`. Generic core function dùng subsystem prefix: `uart_*`, `mmc_*`.

### Mapping Linux → VinixOS

#### Memory

```c
/* Allocator */
void *kmalloc(size_t size, gfp_t flags);
void  kfree(const void *ptr);
struct page *alloc_pages(gfp_t flags, unsigned int order);
void  free_pages(struct page *page, unsigned int order);

/* VM */
struct vm_area_struct;
struct mm_struct;
```

#### Process / scheduler

```c
struct task_struct;
extern struct task_struct *current;

enum task_state {
    TASK_RUNNING,
    TASK_INTERRUPTIBLE,
    TASK_UNINTERRUPTIBLE,
    TASK_STOPPED,
    TASK_ZOMBIE,
};
```

#### Concurrency

```c
spinlock_t          lock;
struct mutex        mtx;
wait_queue_head_t   wq;
atomic_t            counter;

smp_mb();   /* full barrier */
smp_rmb();  /* read barrier */
smp_wmb();  /* write barrier */
```

### Forbidden names

| Cấm | Lý do |
| --- | --- |
| `tmp`, `data`, `buf` không có context | Vô nghĩa, grep ra hàng trăm match |
| `do_stuff`, `handle_it`, `process_thing` | Không nói lên action |
| `MyDriver`, `myFunc` | PascalCase + tiếng Anh non-idiomatic |
| `vinix_xxx_xxx_xxx_init_v2` | Cờ phase / version trong tên |
| `tty_*` (cho non-tty), `task_*` (cho non-task) | Kéo namespace Linux sai chỗ |

---

## 3. File Layout

### Per-file structure (.c)

```c
/*
 * drivers/path/file.c — One-line module description
 *
 * 1-2 dòng nói WHAT module provide hoặc constraint chính.
 */

/* 1. Includes — local trước, project sau, mach cuối */
#include "types.h"
#include "uart.h"
#include "vinix/init.h"
#include "vinix/errno.h"
#include "mach/memmap.h"
#include "mach/irqs.h"

/* 2. Local #define — register offset, bit field */
#define UART_RHR        0x00
#define LSR_DR          (1 << 0)

/* 3. Static state + forward declaration */
static struct uart_port omap_port;
static int omap_uart_remove(struct platform_device *pdev);

/* 4. Static helpers */
static void uart_rx_irq_handler(void *data) { ... }

/* 5. Public functions — match header order */
void uart_putc(char c) { ... }

/* 6. Driver registration (cuối file) */
static struct platform_driver omap_uart_driver = { ... };
module_platform_driver(omap_uart_driver);
```

### Per-file structure (.h)

```c
#ifndef VINIX_UART_H
#define VINIX_UART_H

#include "types.h"   /* dependency tối thiểu */

/* Forward declaration nếu chỉ cần con trỏ */
struct platform_device;

/* Public API — group theo function */
void uart_putc(char c);
int  uart_getc(void);

/* Inline helper — chỉ khi thực sự ngắn (≤ 5 LOC) */
static inline int uart_rx_available(void)
{
    return uart_rx_count() > 0;
}

#endif /* VINIX_UART_H */
```

**Header guard format**: `VINIX_<PATH>_H` hoặc `<MODULE>_H`. Không dùng `_H_` trailing.

### Driver folder layout (subsystem có core + host)

```
drivers/mmc/
├── core/
│   ├── core.c            ← mmc_alloc_host, mmc_add_host (subsystem core)
│   └── mmc_block.c       ← block device wrapper
└── host/
    └── omap_hsmmc.c      ← AM335x host controller
```

Nguyên tắc: **core/** không bao giờ chứa SoC-specific code. Port sang Allwinner = thêm `host/sun8i_mmc.c`, không sửa `core/`.

---

## Phụ lục — Coding Smell

| Smell | Fix |
| --- | --- |
| `if (ptr != NULL)` thay vì `if (ptr)` | Linux convention dùng implicit |
| Function > 100 LOC không tách | Tách thành static helpers |
| Magic number không define | `#define` ở đầu file với comment lý do |
| Variable name thiếu context (`tmp`, `buf`) | Đặt tên mô tả nội dung |



