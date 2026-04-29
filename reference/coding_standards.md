# VinixOS — Coding Standards (Deep Dive)

> File này là **deep-dive** — rationale, ví dụ đúng/sai, edge case.
> [`CLAUDE.md`](../CLAUDE.md) là **enforcer ngắn gọn** (luôn được AI load).
> Khi hai file xung đột: **CLAUDE.md thắng**. Sửa file này thì check lại CLAUDE.md có cần đồng bộ không.

Mục tiêu: senior dev quen Linux đọc source VinixOS phải nhận ra ngay "Linux-style OS thu nhỏ" — không phải hobby OS lập dị, không phải Linux fork.

---

## Table of Contents

1. [Triết lý — Linux as Reference, Not Source](#1-triết-lý)
2. [Coding Style](#2-coding-style)
3. [Naming Convention](#3-naming-convention)
4. [File Layout](#4-file-layout)
5. [Comments](#5-comments)
6. [Logging](#6-logging)
7. [Driver Convention — Deep](#7-driver-convention--deep)
8. [Commit Style](#8-commit-style)
9. [Workflow Rules](#9-workflow-rules)
10. [Anti-patterns Catalog](#10-anti-patterns-catalog)

---

## 1. Triết lý

VinixOS học Linux 3 thứ và **chỉ** 3 thứ:

| Học | Không học |
| --- | --- |
| **Naming** — `task_struct`, `kmalloc`, `pr_info` | Implementation detail của Linux |
| **Shape** — VFS ops table, platform_driver probe, initcall levels | RCU, sysfs, namespace, cgroup |
| **Pattern** — wait_queue, slab cache, list_head | Build system Kconfig/Kbuild |

### Quy tắc đọc reference

```
1. Mở reference/drivers/<name>/source/*.c
2. Đọc để hiểu sequence (init order, register flow, error path)
3. ĐÓNG file lại
4. Viết từ đầu bằng VinixOS naming + convention
5. Nếu phải mở lại reference 2-3 lần cho cùng 1 function → rewrite hoàn toàn từ memory
```

Lý do đóng file: paraphrase từ memory tạo code có **shape giống** Linux nhưng **không phải Linux**. Mở reference cạnh editor → tay tự động copy structure → vi phạm dòng 1.

### Dấu hiệu đã copy (self-check)

- Variable name lạ với phần còn lại của codebase (e.g. `up`, `ldata`, `tty_struct` trong khi codebase dùng `port`, `ring`, `serial_port`)
- Comment phong cách kernel.org (`/* If foo, then bar.  Otherwise, baz. */`)
- Function dài >100 LOC mà bạn không giải thích được từng nhánh
- File diff lớn hơn lượng thông tin bạn vừa đọc trong reference

→ Tất cả đều là **xóa và viết lại**. Không sửa nhỏ.

---

## 2. Coding Style

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
- Public symbol → khai báo trong header, **bắt buộc** module prefix (xem [§3](#3-naming-convention))
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

## 3. Naming Convention

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

Bảng ở [`CLAUDE.md` §4](../CLAUDE.md) là **canonical**. Mở rộng dưới đây cho 1 vài subsystem cụ thể.

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

## 4. File Layout

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

## 5. Comments

### Default: KHÔNG viết comment

> Test: xóa comment đi — code còn đọc được không? Còn → comment thừa, xóa.

Code well-named, function ngắn, struct field tự giải thích thì không cần comment. AI có xu hướng over-comment — phải kiềm.

### 5 trường hợp ĐƯỢC viết

#### (1) Hardware constraint không hiển nhiên

```c
/* Wait for TX shift register idle before clock reconfiguration;
 * touching PRCM while the shift register is transmitting drops bytes. */
while (!(mmio_read32(base + UART_LSR) & LSR_TEMT));
```

#### (2) Magic value — giải thích lý do, không giải thích value

```c
/* FB at 0x80800000 = kernel(5MB) + user(1MB) + 2MB margin */
#define FRAMEBUFFER_BASE 0x80800000
```

```c
/* Sai — giải thích cái đã thấy */
#define FRAMEBUFFER_BASE 0x80800000  /* framebuffer base address */
```

#### (3) Invariant tinh tế

```c
/* RX ring buffer is shared with IRQ handler — must hold uart_lock
 * before any read/write to head/tail indices. */
static char rx_ring[RX_RING_SIZE];
```

#### (4) `CRITICAL:` cho interrupt-mode hoặc context-sensitive code

```c
/* CRITICAL: called from IRQ context — no kmalloc, no mutex, no printk
 * with format string > 64 chars (kernel log buffer is per-CPU). */
static void uart_rx_irq_handler(void *data) { ... }
```

#### (5) Cross-reference TRM cho hardcoded sequence

```c
/* AM335x TRM 19.5.1.1.2 — software reset sequence:
 * 1. Set SYSCONFIG.SOFTRESET
 * 2. Poll SYSSTATUS.RESETDONE
 * 3. Configure FCR before LCR */
mmio_write32(base + UART_SYSC, 0x02);
while (!(mmio_read32(base + UART_SYSS) & 0x01));
```

### KHÔNG viết — anti-patterns

```c
/* SAI — banner dài dòng */
/*
 * ============================================================
 * Function: uart_putc
 * Description: Sends a character to UART
 * Parameters:
 *   c - the character
 * Returns: void
 * Author: ...
 * Date: ...
 * History:
 *   2026-04-01 — initial version
 * ============================================================
 */
void uart_putc(char c) { ... }
```

```c
/* SAI — giải thích WHAT */
i++;  /* increment i */
mmio_write32(base + UART_FCR, 0x07);  /* write 0x07 to FCR */
```

```c
/* SAI — phase marker */
/* P1 init */
uart_init();
/* TODO P2: add interrupt support */
```

```c
/* SAI — PR / issue reference (rot quickly) */
/* Fix for issue #42 — buffer overflow */
/* See PR #128 */
/* Used by sched.c and timer.c */
```

```c
/* SAI — "imported from" / "based on" */
/* Imported from Linux v6.5 drivers/tty/serial/8250.c */
/* Based on FreeBSD ata driver */
```

→ **Vi phạm § "Linux as Reference, Not Source".** Xóa file, viết lại.

### File header — block 4-6 dòng, không separator

```c
/*
 * drivers/tty/serial/omap_serial.c — AM335x UART0 driver
 *
 * TX polling, RX interrupt-driven via serial_core ring buffer.
 */
```

Cấu trúc:
- **Dòng 1**: open comment `/*`
- **Dòng 2**: `path/file.c — One-line module description` (path tương đối từ project root, dấu em-dash, mô tả ngắn)
- **Dòng 3**: blank `*`
- **Dòng 4-5**: 1-2 dòng mô tả WHAT module provide hoặc constraint chính (optional nếu module name đã đủ)
- **Dòng cuối**: close `*/`

KHÔNG: SPDX, Copyright, Author, Date, History, `===`/`***` separator. Git lo blame/log/license. Project chưa có license header, không bịa ra.

### Struct public trong header — kernel-doc `/** */`

Cho VSCode hover doc hoạt động + cho ai đọc include thấy contract:

```c
/**
 * struct uart_port - serial port runtime state
 * @base:  MMIO base address (verified by platform_get_resource)
 * @irq:   hardware IRQ number
 * @ops:   driver callback table
 * @lock:  protects ring buffer head/tail
 */
struct uart_port {
    uint32_t            base;
    int                 irq;
    const struct uart_ops *ops;
    spinlock_t          lock;
};
```

KHÔNG dùng kernel-doc cho struct internal trong .c file — chỉ public struct trong .h.

### Javadoc-style — chỉ khi thực sự cần

Viết khi và **chỉ khi**:
- Precondition không hiển nhiên (caller phải hold lock, IRQ disabled, ...)
- Side effect ẩn (modify global state, free memory caller-passed)
- Return value có edge case không thể infer từ type

```c
/**
 * uart_serial_rx_push - push byte vào RX ring buffer
 * @c: byte received from HW
 *
 * Callable từ IRQ context. Drop silently nếu ring full —
 * caller không thể biết drop xảy ra.
 */
static void uart_serial_rx_push(char c) { ... }
```

Function ngắn 5 dòng, parameter tên rõ ràng, return type explicit → **không cần Javadoc**.

### Debug print

```c
/* Bring-up: comment out, không xóa hẳn */
// pr_info("[UART] FCR after write = 0x%08x\n", mmio_read32(base + UART_FCR));
mmio_write32(base + UART_FCR, 0x07);
```

Sau khi feature ổn định + test trên hardware → **xóa hẳn** dòng comment.

---

## 6. Logging

### Bắt buộc prefix `[MODULE]`

```c
pr_info("[TIMER] Initializing DMTimer2...\n");
pr_err("[SCHED] No tasks to run!\n");
pr_warn("[MMC] CRC mismatch on block %u\n", blk);
pr_debug("[I2C] start cond, slave 0x%02x\n", addr);
```

Module name: chữ HOA, ngắn (≤ 8 ký tự lý tưởng), match driver folder name.

### Level chọn

| Level | Khi nào |
| --- | --- |
| `pr_err` | HW không response, init fail, irrecoverable error |
| `pr_warn` | Recoverable — CRC error, FIFO overflow, retry succeeded |
| `pr_info` | Boot milestone — driver registered, device probed, mount success |
| `pr_debug` | Bring-up only — register dump, FSM transition |

`pr_debug` phải tắt được tại compile-time (qua macro). KHÔNG ship boot log có pr_debug spam.

### Format

```c
/* Đúng — context + value */
pr_info("[UART] probing %s @ 0x%08x irq %d\n", pdev->name, base, irq);

/* Đúng — error path nói rõ cái gì fail */
pr_err("[MMC] CMD%d timeout (status=0x%08x)\n", cmd_idx, status);

/* Sai — không context */
pr_info("done\n");
pr_err("error!\n");
```

### Boot log clean checklist

Trước commit cuối phase:
- [ ] Mọi pr_info đều là **milestone**, không phải debug print sót lại
- [ ] Mọi prefix `[XXX]` consistent với module name
- [ ] Không có `pr_info("here\n")`, `pr_info("test 1 2 3\n")`
- [ ] Boot log đọc tuần tự thấy được sequence init rõ ràng

---

## 7. Driver Convention — Deep

### Gold reference walkthrough

[`drivers/tty/serial/omap_serial.c`](../vinix-kernel/drivers/tty/serial/omap_serial.c) là driver compliant 100% — đọc khi viết driver mới.

Cấu trúc bắt buộc:

```c
/* 1. Banner */
/*
 * drivers/xxx/yyy_zzz.c — One-line module description
 *
 * 1-2 dòng nói WHAT hoặc constraint chính (optional).
 */

/* 2. Includes */
#include "types.h"
#include "platform_device.h"
#include "vinix/init.h"
#include "mach/memmap.h"
#include "mach/irqs.h"

/* 3. Register offsets / bits */
#define XXX_CTRL    0x00
#define CTRL_EN     (1 << 0)

/* 4. Static state */
static struct xxx_state state;

/* 5. IRQ / helper */
static void xxx_irq_handler(void *data) { ... }

/* 6. Probe — TẤT CẢ HW init ở đây */
static int xxx_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq = platform_get_irq(pdev, 0);
    uint32_t base = mem->start;

    pr_info("[XXX] probing %s @ 0x%08x irq %d\n", pdev->name, base, irq);

    /* Check dependencies — defer if not ready */
    if (!intc_ready())
        return -EPROBE_DEFER;

    /* HW init */
    enable_clock();
    reset_hw(base);
    configure(base);

    /* IRQ */
    if (request_irq(irq, xxx_irq_handler, 0, "xxx", NULL))
        return -EIO;
    enable_irq(irq);

    /* Subsystem register */
    return xxx_register(&state);
}

/* 7. Driver struct + initcall */
static struct platform_driver xxx_driver = {
    .drv   = { .name = "xxx" },
    .probe = xxx_probe,
};

static int __init xxx_driver_init(void)
{
    return platform_driver_register(&xxx_driver);
}
subsys_initcall(xxx_driver_init);
```

### THE CONVENTION — không exception

1. **Một entry per HW** trong `board-bbb.c::bbb_devices[]` (platform) hoặc `bbb_i2c0_devices[]` (I2C).
2. **Mọi HW init nằm trong `probe()`**. KHÔNG public `xxx_init()` gọi từ `main.c`.
3. Đăng ký qua `module_platform_driver(drv)` hoặc `module_i2c_driver(drv)`.
4. Inter-driver dependency: `if (!dep_ready) return -EPROBE_DEFER;` — core sẽ retry sau `driver_deferred_probe_trigger()`.
5. `init/main.c` CHỈ chứa `do_initcalls(N)` + core init + VFS mount + deferred probe trigger.

### Initcall level decision tree

```
Driver bạn viết là gì?
├── Là core SoC infrastructure (PRCM, control module register)?
│   → core_initcall  (Level 1)
├── Là arch-level (UART early console, watchdog)?
│   → arch_initcall  (Level 3)
├── Là subsystem core (IRQ controller, I2C bus, MMC core)?
│   → subsys_initcall (Level 4)
├── Là filesystem-related (block layer, FS module)?
│   → fs_initcall    (Level 5)
├── Là device driver thường (display, timer, sensor)?
│   → device_initcall (Level 6)
└── Là selftest / verify chỉ chạy cuối boot?
    → late_initcall  (Level 7)
```

### EPROBE_DEFER — khi nào dùng

```c
static int omap_i2c_probe(struct platform_device *pdev)
{
    /* I2C controller cần INTC sẵn sàng — nếu chưa → defer */
    if (!intc_is_initialized())
        return -EPROBE_DEFER;

    ...
}
```

Core sẽ thêm driver vào deferred list. Sau khi mọi initcall xong, gọi `driver_deferred_probe_trigger()` → retry tất cả deferred. Nếu vẫn fail → log và bỏ qua.

**KHÔNG** dùng EPROBE_DEFER cho:
- HW thực sự không tồn tại (return `-ENODEV`)
- Allocation fail (return `-ENOMEM`)
- Register sai (return `-EIO`)

### Subsystem registration table

Xem [`CLAUDE.md` §5](../CLAUDE.md). Bảng đó là canonical.

---

## 8. Commit Style

### Format

```
Type(scope): short description (≤ 70 chars)

[Optional body — chỉ khi cần giải thích WHY]
```

**Không** body cho commit nhỏ (typo, rename, format). Body bắt buộc khi:
- Behavior change mà commit message không nói được hết
- Cần ghi lại HW errata, TRM reference cho future debug
- Touch nhiều file — cần giải thích relationship

### Type taxonomy

| Type | Khi nào | Ví dụ |
| --- | --- | --- |
| `Feat` | Tính năng mới | `Feat(mmc): implement omap_hsmmc probe` |
| `Fix` | Bug fix với root cause xác định | `Fix(sched): clear stale state on SIGKILL` |
| `Refactor` | Đổi cấu trúc, không đổi behavior | `Refactor(drivers): drop main.c init calls` |
| `Docs` | Sửa tài liệu / comment | `Docs(driver): add platform_driver template` |
| `Test` | Thêm/sửa test | `Test(mmu): cover unmap edge cases` |

**KHÔNG có**: `WIP`, `Update`, `Change`, `Misc`, `Port`, `Based on`, `Improve`, `Cleanup` (dùng Refactor).

### Scope chọn

- 1 module: `Feat(uart): ...`
- 2-3 module liên quan: `Refactor(drivers): ...`
- Build / scripts: `Refactor(scripts): ...`, `Fix(build): ...`
- Toàn project: hiếm — `Refactor(tree): rename folders kebab-case`

### Description craft

```
Đúng:
  Refactor(i2c): convert omap-i2c to platform_driver, drop main.c direct calls
  Fix(sched): clear stale task state on SIGKILL
  Feat(mmc): implement omap_hsmmc probe and card init sequence

Sai:
  Update i2c driver                          ← không nói gì
  Fix bug                                     ← không root cause
  WIP: working on mmc                         ← phase marker
  Port omap_serial from Linux                 ← cấm "Port"
  Based on Linux drivers/tty/serial/8250.c    ← cấm "Based on"
  Many fixes and improvements                 ← multi-purpose commit
```

### Khi tách commit

1 commit = 1 ý. Nếu phải dùng "and" trong description → cân nhắc tách:

```
Refactor(uart): convert to platform_driver
Refactor(uart): move register defines to header
Fix(uart): handle FIFO overflow on RX
```

vs

```
Refactor(uart): convert to platform_driver and move defines and fix overflow
   ^^^^^^^^^^^^^                                                        ← tách
```

### Cấm tuyệt đối

- `Co-Authored-By: Claude` trailer trên commit VinixOS — **không bao giờ**
- `git tag v0.PN-complete` — chỉ commit, không tag phase
- `git commit --amend` commit đã push — luôn tạo commit mới

---

## 9. Workflow Rules

### Read-before-write

Trước khi sửa file:

1. Đọc toàn bộ file đang sửa
2. Đọc các header phụ thuộc
3. `grep -r 'function_name' .` xem ai gọi
4. Sửa

Trước khi viết driver mới:

1. [`reference/index.md`](index.md) → tìm tài liệu TRM liên quan
2. Đọc đúng chapter TRM
3. Đọc gold reference [`omap_serial.c`](../vinix-kernel/drivers/tty/serial/omap_serial.c)
4. Đọc subsystem header (`vinix/serial_core.h`, ...)
5. Viết

### Pre-flight checklist (driver mới)

- [ ] Base address verified từ AM335x TRM Ch.02 hoặc `mach/memmap.h`
- [ ] Register offset + bit field từ TRM chapter của peripheral
- [ ] IRQ number từ `mach/irqs.h` hoặc TRM Ch.06
- [ ] Clock enable sequence từ TRM Ch.08 PRCM
- [ ] Pin mux từ TRM Ch.09 Control Module (nếu cần)
- [ ] Init timing từ `reference/drivers/<name>/index.md`

Thiếu bất kỳ → **dừng**, hỏi user theo format sau:

```
Để viết [function/driver], tôi cần:
- [thông tin A] → [nguồn: TRM Ch.XX / file path]
- [thông tin B] → [nguồn: ...]
Bạn chỉ tôi đọc file nào, hoặc cung cấp trực tiếp?
```

### Definition of Done

| Loại task | Done = |
| --- | --- |
| Driver / kernel feature | Compile clean + user test trên BBB thật + boot log sạch |
| Refactor | Compile clean + behavior không đổi (verify bằng cùng test cũ) |
| Bug fix | Root cause xác định + UART log cho thấy failure biến mất |
| Docs | Reviewed by user — không có "ship rồi sửa sau" |

**Không bao giờ** tuyên bố "complete" / "works" chỉ dựa compile. Câu chuẩn:

> "Build sạch, không warning. Bạn flash + test trên BBB thật giúp em — em đợi UART log để xác nhận."

### Post-phase cleanup (khi đóng P0/P1/...)

- [ ] Xóa mọi phase marker: `/* P1 init */`, `/* TODO P2 */`, `/* Phase 1 stub */`
- [ ] Xóa scaffolding / lab code đã hết nhiệm vụ
- [ ] Boot log polish: prefix consistent, không debug noise
- [ ] Commit cuối: `Feat(scope): ...` hoặc `Refactor(scope): ...` — không `WIP`
- [ ] **Không** `git tag` — chỉ commit

### Debug workflow

Không có JTAG. **UART log là công cụ debug duy nhất.**

Khi báo bug — gom đủ TRƯỚC KHI phân tích:
1. Toàn bộ UART log từ đầu boot
2. Dòng log cuối trước hang/crash
3. Loại exception (Data Abort / Prefetch Abort / Undefined)
4. Thao tác ngay trước bug

KHÔNG đoán nguyên nhân khi chưa có UART log.

```c
/* Checkpoint print pattern */
pr_info("[MODULE] Step X: before\n");
/* operation */
pr_info("[MODULE] Step X: after — reg = 0x%08x\n", mmio_read32(REG));

/* Readback bắt buộc sau write */
mmio_write32(BASE + OFFSET, value);
pr_info("[DRV] wrote 0x%08x, readback = 0x%08x\n",
        value, mmio_read32(BASE + OFFSET));
```

Exception/Abort → yêu cầu DFAR, DFSR, PC. Nếu handler chưa print các register này → thêm vào `arch/arm/exceptions/` trước khi debug tiếp.

---

## 10. Anti-patterns Catalog

Khi review code (own hoặc AI-generated), check những thứ sau:

### Code smell

| Smell | Fix |
| --- | --- |
| `xxx_init()` public gọi từ main.c | Move logic vào `probe()`, dùng platform_driver |
| Hardcoded register address trong driver | Nhận qua `platform_get_resource()` |
| `if (ptr != NULL)` thay vì `if (ptr)` | Linux convention dùng implicit |
| Multi-line comment giải thích WHAT | Xóa, sửa tên biến cho rõ |
| Function > 100 LOC không tách | Tách thành static helpers |
| Magic number không define | `#define` ở đầu file với comment lý do |
| `goto error;` không có label cleanup hierarchy | OK pattern Linux — không sửa |

### Comment smell

| Smell | Fix |
| --- | --- |
| `/* TODO: ... */` không có context | Xóa hoặc thay bằng `BUG_ON`/issue tracker |
| `/* Used by xxx.c */` | Xóa — git grep tự tìm |
| `/* Imported from Linux ... */` | **Vi phạm**, viết lại file từ đầu |
| `/* Phase 1: ... */` | Xóa khi phase đóng |
| Comment trùng tên function | Xóa |

### Commit smell

| Smell | Fix |
| --- | --- |
| `Update xxx` | Refactor / Feat / Fix với verb cụ thể |
| `Co-Authored-By: Claude` | Xóa trailer |
| `WIP` | Squash hoặc finalize trước commit |
| `Port from Linux` | Vi phạm — viết lại implementation từ đầu |
| Diff > 500 LOC, message 1 dòng | Tách commit theo ý |

### Driver smell

| Smell | Fix |
| --- | --- |
| Driver không có entry trong `bbb_devices[]` | Thêm — không có exception |
| HW init gọi từ main.c | Move vào `probe()` |
| Initcall level sai (UART ở `late_initcall`) | Theo decision tree §7 |
| Không check return của `request_irq` | Add error path |
| Không readback register sau write trong bring-up | Add readback + log |

---

## Phụ lục — Reference Quick Links

- [`CLAUDE.md`](../CLAUDE.md) — enforcer ngắn gọn (canonical rules)
- [`reference/index.md`](index.md) — TRM + driver reference index
- [`reference/project_context.md`](project_context.md) — project state snapshot
- [`vinix-kernel/drivers/tty/serial/omap_serial.c`](../vinix-kernel/drivers/tty/serial/omap_serial.c) — gold reference driver
- [`Documentation/driver-template/`](../Documentation/driver-template/) — driver skeleton

---

> **Maintenance**: file này KHÔNG phải static. Khi convention mới được xác lập (qua user feedback hoặc lesson learned từ bug), update tại đây + đồng bộ lại CLAUDE.md. Lần cuối review: 2026-04-29.
