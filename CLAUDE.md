# CLAUDE.md — VinixOS Project Guide

**Plan chi tiết**: [/home/haidoan2098/.claude/plans/t-i-mu-n-b-n-trace-velvet-mitten.md](/home/haidoan2098/.claude/plans/t-i-mu-n-b-n-trace-velvet-mitten.md)

Roadmap P0-P8 + U1 + U2, timeline, architecture decisions, và scope cuts đều nằm trong plan đó. CLAUDđE.md này là rules + conventions bắt buộc mọi session.

---

## 1. Hard Constraints

BẮT BUỘC tuân thủ mọi lúc. Không có ngoại lệ.

**Memory — KHÔNG được:**

- Dùng `malloc`/`free` userspace style — kernel dùng `kmalloc(size, GFP_KERNEL)` / `kfree`
- Dùng buffer lớn trên stack (kernel stack nhỏ)

**Standard Library — KHÔNG được:**

- `#include <stdio.h>`, `<stdlib.h>` trong kernel code
- Dùng `printf()` — phải dùng `uart_printf()` (sau P8: `pr_info/pr_err/pr_warn/pr_debug`)
- Dùng bất cứ libc function nào — chỉ dùng `libc/src/string.c` nội bộ

**Hardware Access — BẮT BUỘC:**

- Truy cập register chỉ qua `mmio_read32(addr)` / `mmio_write32(addr, val)`
- Lấy register address và bit definition từ AM335x TRM — KHÔNG được đoán
- **KHÔNG hardcode peripheral address** trong `vinix-kernel/` hay subsystem core — dùng `arch/arm/mach-omap2/include/mach/` constants

**Toolchain — KHÔNG được:**

- Mix `arm-none-eabi` và `arm-linux-gnueabihf`
- Build kernel trước userspace (thứ tự: `userspace` → `kernel`, luôn luôn)

**VinCC Subset C — CHỈ áp dụng cho user program biên dịch bằng VinCC. KHÔNG áp dụng cho kernel:**

- `struct`, `union`, `enum`, `float`, `double`
- `++` / `--` — dùng `i = i + 1`
- Variadic function, standard library
- Hơn 4 tham số hàm (r0–r3 theo AAPCS)

**Design — KHÔNG được:**

- Dùng Linux BSP hoặc SDK thương mại
- Dùng emulator — chỉ chạy trên hardware thật
- Thêm abstraction layer không cần thiết

**Scope hardcoded (không mở rộng khi chưa có consumer):**

- MAX_TASKS = 5
- Signal set: `SIGKILL`, `SIGSEGV` (KHÔNG `SIGTERM`, `SIGCHLD`)
- `wait()` chỉ wait-any (KHÔNG `waitpid(pid)`)
- Concurrency primitive: spinlock + atomic + wait_queue (KHÔNG mutex, KHÔNG semaphore)
- KHÔNG pipe syscall ở v1 (defer v1.1)
- KHÔNG TCP (UDP/ICMP/ARP only nếu P7)
- KHÔNG editor (device dùng `echo >` redirect)

---

## 2. Rule 7 — 100% Tự Viết (TUYỆT ĐỐI)

**Mọi dòng code trong repo phải được tay gõ, hiểu, chịu trách nhiệm.**

### KHÔNG ĐƯỢC copy/fork/port từ

- libc: musl, glibc, newlib
- Userspace: BusyBox, toybox
- Network: lwIP, uIP, picoTCP
- Filesystem: FatFs, SDFat, Linux ext4 code
- Compiler: TCC, GCC, chibicc, c4, PCC
- Kernel: Linux (any tree), FreeBSD, NetBSD, Minix, xv6, seL4, Zephyr
- Driver: bất kỳ upstream driver code nào

### Reference code ở `docs_trainingAI/drivers/*/source/`

- CHỈ được **đọc để hiểu logic và sequence**
- PHẢI **viết lại từ đầu** theo naming + convention VinixOS
- KHÔNG **copy nguyên block code** — mỗi dòng phải hiểu và adapt
- Pattern thuật toán (ví dụ: free-list malloc K&R, recursive descent parser) OK để học, nhưng **code phải tay gõ**

### Khi nghi ngờ "có đang copy không"

1. Đóng reference source
2. Code lại từ memory + hiểu biết
3. Nếu phải mở reference lần 2-3 cho cùng function → rewrite hoàn toàn sau khi đóng

### Checklist trước commit cuối mỗi phase

- [ ] Không có file copy từ upstream
- [ ] Git log message: "Implement X from scratch", không "Port X from Y"
- [ ] Nếu học pattern từ Linux/BSD: ghi `/* Pattern reference: Linux <file>:<func> — re-implemented */` ở function header, không dùng code
- [ ] Mọi comment là tiếng nói của bạn, không paraphrase

**Narrative with investor:** *"100% Made in Vietnam by hand. Không asterisk."*

---

## 3. Project Context

**VinixOS** — bare-metal ARM platform tự xây từ đầu, chạy trực tiếp trên BeagleBone Black (SoC AM3358, Cortex-A8, ARMv7-A). Không Linux, không emulator, không SDK thương mại.

### Linux-flat layout (vinix-kernel/)

```text
vinix-kernel/
├── arch/arm/                  — ARMv7 CPU: entry, exceptions, MMU asm
│   └── mach-omap2/            — AM3358 SoC + BBB board (was platform/bbb/)
│       └── include/mach/      — memory map, IRQ numbers, board headers
├── init/                      — main.c, payload.S, do_initcalls
├── kernel/                    — generic C: sched, locking, irq, irqchip,
│                                i2c, mmc, time, printk, tty (KHÔNG đụng khi port)
├── drivers/                   — HW drivers, organized by Linux subsystem:
│   ├── tty/serial/            (omap_serial.c)
│   ├── irqchip/               (irq-omap-intc.c)
│   ├── clocksource/           (timer-omap-dm.c)
│   ├── mmc/host/              (omap_hsmmc.c)
│   ├── i2c/busses/            (i2c-omap.c)
│   ├── gpu/drm/{tilcdc,i2c}/  (lcdc, tda998x)
│   ├── video/fbdev/           (fbmem.c, fbcon.c)
│   ├── watchdog/              (omap_wdt.c)
│   ├── base/                  (device.c, platform.c — driver model)
│   └── char/                  (cdev registry)
├── fs/                        — VFS, FAT32, devfs, procfs
├── mm/, block/, lib/          — generic
└── include/
    └── vinix/                 — Linux-style subsystem headers (init, fs,
                                cdev, blkdev, irqchip, i2c, mmc/host,
                                serial_core, tty, fb, clocksource, ...)
```

**Port sang SoC khác** = viết `arch/arm/mach-<new>/` + driver mới. `kernel/` không đổi dòng nào.

### Component

- **VinixOS** — kernel + bootloader + userspace system tools (init/sh/ls/cat/ps/...), C + ARM assembly
- **vinixlibc** — POSIX subset libc tự viết ~5K LOC (sau P6)
- **VinCC** — Python cross-compiler **trên host**: Subset C → ARMv7 ELF32 cho end-user C program chạy trên VinixOS (laptop compile → copy ELF vào SD → BBB execute)

### VinCC scope — chỉ cross-compile user program

VinCC CHỈ dùng để compile chương trình C do end-user viết (hello.c, custom.c), link với vinixlibc, chạy ở tầng userspace VinixOS trên BBB.

VinCC **KHÔNG** compile:

- Kernel / bootloader
- vinixlibc
- VinixOS system tools (init, sh, ls, cat, ps, ...)
- Bất cứ thứ gì thuộc repo VinixOS — những cái đó dùng `arm-none-eabi-gcc` full C

### Toolchain

- VinixOS code (kernel + bootloader + vinixlibc + system tools) → `arm-none-eabi-gcc` (bare-metal, full C)
- End-user C programs (Subset C) → VinCC Python trên host → ELF output copy vào SD

### Thứ tự build

```bash
make -C vinix-kernel userspace   # TRƯỚC — kernel nhúng shell payload
make -C vinix-kernel kernel      # SAU  — kernel lấy shell mới nhất
```

### Memory Map (runtime)

| Region | VA | PA | Thuộc tính |
| --- | --- | --- | --- |
| User space | `0x40000000` | `0x80500000` | 1 MB, RW, cached |
| Kernel DDR | `0xC0000000` | `0x80000000` | 5 MB, kernel-only, cached |
| L4 WKUP (PRCM, UART0, WDT1, Control Module) | `0x44E00000` | identity | 1 MB, device, Strongly Ordered |
| L4 PER (INTC, DMTimer, I2C, MMC0) | `0x48000000` | identity | 4 MB, device, Strongly Ordered |
| Framebuffer | `0x80800000` | identity | 4 MB, non-cacheable |

Constants định nghĩa ở `vinix-kernel/arch/arm/mach-omap2/include/mach/memory.h` + `memmap.h`.

### Knowledge Base

Hardware authoritative ở `docs_trainingAI/`:

- `project_context.md` — đọc trước khi bắt đầu session mới
- `am335x/` — AM335x TRM (27 chapter)
- `arm-arch/` — ARM architecture reference
- `hardware-beagleboneblack/` — BBB P8/P9 pinout + schematic
- `drivers/<name>/index.md` — pointer theo từng driver (TRM chapter liên quan + reference source)

LUÔN đọc `drivers/<name>/index.md` trước khi viết driver mới.

---

## 4. Linux Kernel Alignment

Mọi abstraction (VFS, driver model, task/process, locking, memory) đi theo pattern Linux — naming, data structure, API shape. MVP nhưng ngữ nghĩa Linux.

### Architecture model — 3 main device categories + class subsystems

VinixOS theo Linux device model: userspace thấy thiết bị qua 1 trong 3 main category, drivers register qua **class subsystem** (nằm giữa driver và main category):

```text
Driver  →  Class subsystem  →  Main category  →  Userspace path
─────────────────────────────────────────────────────────────────
omap_serial → tty/serial → CHAR    → /dev/ttyS0
omap_hsmmc  → mmc/host   → BLOCK   → /dev/mmcblk0
tilcdc      → fbdev      → CHAR    → /dev/fb0
omap_wdt    → watchdog   → CHAR    → /dev/watchdog
i2c-omap    → i2c        → bus     → (no /dev — host)
omap_intc   → irqchip    → kernel  → (no /dev — internal)
omap_dmtimer→ clockevents→ kernel  → (no /dev — internal)
cpsw (P7)   → net        → NET     → eth0
```

**Main categories**:

- **Character** (`vinix/cdev.h` + `cdev_register` + `file_operations`)
- **Block** (`vinix/blkdev.h` + `add_disk` + `block_device_operations`)
- **Network** (`vinix/netdevice.h` + `register_netdev` + `net_device_ops`)

**Class subsystems** (mọi driver register qua đây, không skip thẳng vào main category):

- `tty/serial` (`vinix/serial_core.h`, `vinix/tty.h`)
- `mmc` (`vinix/mmc/host.h`)
- `i2c` (`vinix/i2c.h`)
- `fbdev` (`vinix/fb.h`)
- `irqchip` (`vinix/irqchip.h`)
- `clocksource/clockevents` (`vinix/clocksource.h`)
- `watchdog` (`vinix/watchdog.h`)

**Device discovery (DT replacement)**: VinixOS KHÔNG dùng Device Tree. Pattern thay thế = static board file (như Linux `mach-omap1`, x86 platform):

- Platform devices declared trong `arch/arm/mach-omap2/board-bbb.c::bbb_devices[]`
- I2C clients declared trong `bbb_i2c0_devices[]` (`struct i2c_board_info`)
- Core enumerate static arrays khi bus add → instantiate devices → match drivers

**Inter-driver dependencies**: handle qua `-EPROBE_DEFER` mechanism (Linux modern feature, không phụ thuộc DT). Driver return `-EPROBE_DEFER` nếu dependency chưa ready, core retry sau.

### Naming + data structures bắt buộc

**Memory:**

- `kmalloc(size, GFP_KERNEL)` / `kfree` — kernel heap (MVP chỉ GFP_KERNEL)
- `alloc_pages(gfp, order)` / `free_pages`
- `kmem_cache_create/alloc/free` — slab cache
- `struct page`, `struct vm_area_struct`, `struct mm_struct`

**Process:**

- `struct task_struct`, `current` macro
- States: `TASK_RUNNING`, `TASK_INTERRUPTIBLE`, `TASK_UNINTERRUPTIBLE`, `TASK_STOPPED`, `TASK_ZOMBIE`
- Syscall entry: `do_sys_open`, `do_sys_read`, `do_fork`, `do_exec`
- Error: negative errno return
- User access: `copy_from_user`, `copy_to_user`, `access_ok`

**Concurrency:**

- `spinlock_t` + `spin_lock/unlock/lock_irqsave/unlock_irqrestore`
- `wait_queue_head_t` + `DECLARE_WAIT_QUEUE_HEAD` + `wait_event(wq, cond)` + `wake_up(wq)`
- `atomic_t` + `atomic_inc/dec/read/cmpxchg`
- `smp_mb()`, `smp_rmb()`, `smp_wmb()`, `barrier()`

**VFS:**

- `struct file`, `struct inode`, `struct dentry`, `struct super_block`
- `struct file_operations`, `struct inode_operations`, `struct super_operations`

**Driver model:**

- `struct platform_device`, `struct platform_driver`, `struct bus_type`
- `platform_driver_register`, `platform_get_resource`

**Networking (nếu P7):**

- `struct sk_buff`, `struct net_device`, `struct socket`

**Logging:**

- `pr_info`, `pr_err`, `pr_warn`, `pr_debug` — macro wrap `uart_printf` với level + `[MODULE]` prefix

**Utility:**

- `container_of(ptr, type, member)`, `list_head` + `list_add/del/for_each_entry`
- `BUG_ON(cond)`, `WARN_ON(cond)`
- `likely(x)`, `unlikely(x)`
- `ARRAY_SIZE(a)`, `offsetof(t, m)`

### KHÔNG copy từ Linux

- Không `sysfs` (dùng `/proc` ở P8)
- Không Device Tree động (hardcoded `vinix-kernel/arch/arm/mach-omap2/board-bbb.c`)
- Không full cgroup, namespace, audit, SELinux
- Không RCU (spinlock + wait queue đủ)
- Không preemption level phức tạp (single priority)

---

## 5. Userspace-Driven Kernel

**Mỗi feature kernel phải có consumer cụ thể trong U1 hoặc demo script. Không consumer → defer, không add.**

### Ví dụ áp dụng đã cắt

- ✂ Priority scheduling (P2) — không case trong init+shell
- ✂ `mutex`, `semaphore` (P2) — spinlock + wait queue đủ single-core
- ✂ `SIGTERM`, `SIGCHLD` (P3/P4) — không consumer
- ✂ `waitpid(pid)` — chỉ `wait(any)`
- ✂ Orphan reparent — accept zombie leak (init luôn sống)
- ✂ COW fork — full page copy
- ✂ Backtrace (P8) — exception dump hiện tại đủ
- ✂ IFSR/IFAR decode cho prefetch abort
- ✂ Pipe syscall (P4) — demo không dùng `|`, defer v1.1
- ✂ Editor (U1) — dùng `echo >` redirect
- ✂ TCP — hand-write infeasible, UDP only
- ✂ Text processing utilities (grep/head/tail/wc) — không demo

### Rule vàng khi execute

> *"Unix semantics hoàn hảo là siren call đưa dev lạc 6 tháng edge case. MVP dừng ở feature có consumer. Mọi cám dỗ mở rộng → `docs/known_gaps.md`."*

---

## 6. Concurrency & State

VinixOS có **scheduler preemptive round-robin single priority**. MAX_TASKS = 5.

### Primitives available

- `spinlock_t` — critical section ngắn, IRQ-safe
- `wait_queue_head_t` — block + wake pattern
- `atomic_t` — LDREX/STREX
- `smp_mb/rmb/wmb()`, `barrier()`

**KHÔNG có** (do rule 5): mutex, semaphore, RCU, completion, rwlock.

### Invariants hiện tại (có thể tin trước P1)

- Chỉ task shell gọi syscall đụng FS / MMC / driver
- Mọi driver I/O đều polled
- Buffer global static (`sector_buf`, `fat_cache`) an toàn vì chỉ 1 consumer

### Sau P1 (multi-task chạm shared state) — BẮT BUỘC

1. Thêm `spinlock_t` quanh shared mutable state, HOẶC
2. Disable preemption quanh critical section, HOẶC
3. Document rõ lý do task mới không thể race với caller hiện có

**KHÔNG giả định static buffer an toàn** chỉ vì "hiện tại không có gì khác dùng nó". Document invariant ngay chỗ declare, verify trước khi thêm caller mới.

---

## 7. Comments

**Mặc định: KHÔNG viết comment.**

### Yardstick

> *"Nếu xoá comment này, code còn đọc được không? Còn → xoá. Không còn → giữ."*

### 5 trường hợp ĐƯỢC viết

1. **Hardware constraint không hiển nhiên** — ordering theo spec, reset sau error, clock dependency
2. **Magic value lý do** — `/* FB at 0x80800000 = kernel(5MB)+user(1MB)+2MB margin */`
3. **Invariant tinh tế reader có thể phá** — buffer sharing, init order, locking rule
4. **Marker `CRITICAL:`** — interrupt-mode / nhạy cảm context
5. **Cross-reference TRM/spec** khi hardcoded sequence (1 lần, không lặp)

### 8 trường hợp CẮT

1. Banner lặp lại tên file + mô tả dài dòng → banner 1 dòng
2. "Imported from X" — include statement tự nói
3. "Backward compatibility alias" — code tự giải thích
4. "Consumed by Y" — grep tìm consumer
5. Giải thích WHAT (`/* Clear status */` trước `mmio_write32`)
6. Struct field trivial (`uint32_t pa; /* physical base */`)
7. Task/phase marker (`/* P0 refactor */`, `/* Phase 1 stub */`)
8. PR/issue reference (`/* Fix from #123 */` → belongs in commit message)

### Good vs bad

❌ **BAD**:
```c
/* Clear status register */
mmio_write32(MMC_STAT, 0xFFFFFFFF);

/* Set command argument */
mmio_write32(MMC_ARG, arg);
```

✓ **GOOD**:
```c
/* SD spec: voltage BEFORE power. Reversed = undefined card state. */
mmio_write32(MMC_HCTL, MMC_HCTL_SDVS_3_3V);
mmio_write32(MMC_HCTL, mmio_read32(MMC_HCTL) | MMC_HCTL_SDBP);

/* CRITICAL: switch to SVC mode before context_switch. */
cps     #0x13
```

### Banner template (1 dòng description)

```c
/* ============================================================
 * filename.c
 * ------------------------------------------------------------
 * Mô tả 1 dòng
 * ============================================================ */
```

### Javadoc

Chỉ khi:

- Precondition không hiển nhiên ("phải gọi sau X")
- Side effect không lộ ra signature ("flush write cache")
- Return value edge case ("trả E_AGAIN nếu queue đầy")

❌ **BAD** (lặp lại signature):
```c
/**
 * @param fd File descriptor
 * @return 0 on success
 */
int vfs_close(int fd);
```

✓ **GOOD**:
```c
/* Does NOT flush write buffers — caller must sync first. */
int vfs_close(int fd);
```

### Debug print

Tạm comment out khi bring-up: `//` để grep lại. Xoá hẳn sau khi feature ổn định.

### Logging format

BẮT BUỘC prefix mọi log bằng `[MODULE]` viết hoa:

```c
pr_info("[TIMER] Initializing DMTimer2...\n");
pr_err("[SCHED] No tasks to run!\n");
```

---

## 8. Coding Style

4 space, không bao giờ dùng tab. Snake_case variable/function, UPPER_SNAKE macro.

**Public symbol BẮT BUỘC có module prefix:** `uart_*`, `scheduler_*`, `intc_*`, `vfs_*`, `platform_*`, …

**File layout** (mọi file `.c`):

1. File header banner (1-line description)
2. `#include` (dùng `""`, không `<>` trừ `<stdarg.h>`)
3. `#define` cho register/field local
4. `static` state (variable + forward decl)
5. Static helper
6. Public function

**Braces:**

- Function: `{` xuống dòng mới
- Control flow: `{` cùng dòng

**Assembly:** inline comment dùng `@`. Banner giống C.

**Mọi thứ khác** (spacing, line wrap, include order): giữ nhất quán với file xung quanh trong cùng module.

---

## 9. Driver Development Workflow

### THE CONVENTION — mọi HW driver tuân theo, không exception

**Rule 1**: Mọi HW driver PHẢI có 1 entry static trong board file:

- Platform device → `bbb_devices[]` array trong `arch/arm/mach-omap2/board-bbb.c`
- I2C client → `bbb_i2c0_devices[]` (`struct i2c_board_info`) cùng file

**Rule 2**: Mọi HW driver PHẢI có `probe()` function:

- Platform driver: `static int xxx_probe(struct platform_device *pdev)`
- I2C driver: `static int xxx_probe(struct i2c_client *client)`

**Rule 3**: Mọi HW init logic NẰM TRONG `probe()`. KHÔNG có public `xxx_init()` function exposed cho main.c gọi.

**Rule 4**: Đăng ký qua `module_platform_driver(drv)` hoặc `module_i2c_driver(drv)` macro. Macro tự đặt fn pointer vào `.initcall<N>.init` section.

**Rule 5**: Inter-driver dependencies handle qua `-EPROBE_DEFER`:

- Driver depending on another HW: `if (!dep_ready) return -EPROBE_DEFER`
- Provider sets global flag khi ready
- Core retry deferred probes sau mọi initcall pass

**Rule 6**: `init/main.c` CHỈ chứa `do_initcalls(N)` + core init (mmu, page_alloc, slab, vmm, vfs, scheduler) + VFS mount + `driver_deferred_probe_trigger()`. KHÔNG có `xxx_init()`/`xxx_register_adapter()`/`watchdog_disable()` driver-specific calls.

**Documented exceptions** (chỉ 2):

- `timer_early_init()` — pre-driver helper cho `delay_ms()` trước scheduler tick mode
- `uart_enable_rx_interrupt()` — RX setup cần intc + irq_init xong

### Driver template (mọi driver mới copy pattern này)

```c
static int omap_xxx_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq = platform_get_irq(pdev, 0);

    /* HW init using mem->start, irq.
     * Register vào subsystem class (cdev_register / add_disk /
     * i2c_add_adapter / mmc_add_host / register_framebuffer / ...). */
    return 0;
}

static struct platform_driver omap_xxx_driver = {
    .drv = { .name = "omap-xxx" },
    .probe = omap_xxx_probe,
    .remove = omap_xxx_remove,
};
module_platform_driver(omap_xxx_driver);
```

Plus thêm entry vào `bbb_devices[]` (board-bbb.c):

```c
static struct platform_device omap_xxx0 = {
    .name = "omap-xxx",  /* match driver.name */
    .base = 0xADDR,
    .irq  = IRQ_NUM,
};
/* trong bbb_devices[]: ..., &omap_xxx0, ... */
```

Device matching qua `pdev->name == drv->name` (string compare).

### Init order via initcall levels (deterministic)

```text
Level 1 (core_initcall):    bbb_platform_init — register all platform_devices + i2c_board_info
Level 3 (arch_initcall):    early HW (uart, wdt)
Level 4 (subsys_initcall):  IRQ controller, i2c host (auto-instantiates clients)
Level 5 (fs_initcall):      block storage (mmc)
Level 6 (device_initcall):  display, timer, i2c clients (defer-able)
Level 7 (late_initcall):    selftest

After do_initcalls(7): driver_deferred_probe_trigger() — final retry pass
```

### Pre-requisite checklist

CRITICAL: Trước khi viết driver mới, verify TẤT CẢ item. THIẾU bất kỳ → DỪNG NGAY, KHÔNG ĐOÁN:

```text
Để viết driver cho [peripheral], tôi cần từ docs_trainingAI/:
- [item còn thiếu] → AM335x TRM Ch.XX
- [item còn thiếu] → AM335x TRM Ch.XX
```

| Thông tin | Nguồn |
| --- | --- |
| Base address | AM335x TRM Ch.02 — Memory Map |
| Register offset + bit definition | TRM chapter của peripheral |
| Clock enable sequence | AM335x TRM Ch.08 — PRCM |
| IRQ number | AM335x TRM Ch.06 — Interrupts |
| Pin mux / CONF_* | AM335x TRM Ch.09 — Control Module |
| Reset sequence | TRM chapter của peripheral |

**KHÔNG copy register address từ project khác** mà không verify với AM335x TRM.

**Platform data flow:** address + IRQ đi từ `vinix-kernel/arch/arm/mach-omap2/board-bbb.c` → `platform_device` → driver `probe()` qua `platform_get_resource`. Driver KHÔNG hardcode `UART0_BASE` nữa.

### Subsystem class registry — register fn theo loại

Mọi driver phải register qua đúng subsystem class (qua header tương ứng). KHÔNG được skip subsystem để gọi VFS / userspace path trực tiếp.

| Driver loại | Subsystem header | Register fn | Status |
| --- | --- | --- | --- |
| Serial / UART | `vinix/serial_core.h` | `uart_register_driver`, `uart_add_one_port` | partial (ring buffer + RX done) |
| Char device | `vinix/cdev.h` | `cdev_register` (với `file_operations`) | ✅ |
| Block / disk | `vinix/blkdev.h` | `add_disk` (`struct gendisk`) | ✅ |
| I2C bus host | `vinix/i2c.h` | `i2c_add_adapter` | ✅ |
| I2C client | `vinix/i2c.h` | `module_i2c_driver` + `i2c_transfer` | ✅ |
| MMC host | `vinix/mmc/host.h` | `mmc_alloc_host` + `mmc_add_host` | ✅ |
| Framebuffer | `vinix/fb.h` | `register_framebuffer` | ✅ |
| IRQ chip | `vinix/irqchip.h` | `irqchip_register` | ✅ |
| Clock event | `vinix/clocksource.h` | `clockevents_register_device` | ✅ |
| Watchdog | `vinix/watchdog.h` | `watchdog_register_device` | core build trong Phase 4 |
| Network | `vinix/netdevice.h` + `vinix/etherdevice.h` + `vinix/skbuff.h` | `register_netdev` | core framework có (Phase 5), driver chờ |

**Documents tham chiếu**:

- [docs/driver-development-guide.md] — driver writer template (full ethernet skeleton)
- [docs/10-subsystem-reference.md] — pattern + example cho mọi subsystem
- Plan Phase 4 (deferred probe + i2c_board_info + driver migration): `~/.claude/plans/t-i-ngh-s-c-woolly-ember.md`

---

## 10. Debug Workflow

Không có JTAG trong workflow bình thường. **UART log là công cụ debug DUY NHẤT.**

### Khi user báo bug

BẮT BUỘC gom đủ TRƯỚC KHI phân tích hoặc fix:

- Toàn bộ UART log từ đầu boot
- Dòng log cuối trước khi hang / crash
- Loại exception (Data Abort / Prefetch Abort / Undefined)
- Thao tác user ngay trước bug

**KHÔNG đoán nguyên nhân khi chưa có UART log.**

### Debug bằng uart_printf

Checkpoint print trước và sau bước nghi ngờ:

```c
pr_info("[MODULE] Step X: before\n");
/* operation */
pr_info("[MODULE] Step X: after — reg = 0x%08x\n", mmio_read32(REG));
```

BẮT BUỘC print readback register, không chỉ giá trị vừa ghi:

```c
mmio_write32(BASE + OFFSET, value);
pr_info("[DRV] wrote 0x%08x, readback = 0x%08x\n",
        value, mmio_read32(BASE + OFFSET));
```

### Exception / Abort

Khi gặp Data/Prefetch Abort, yêu cầu user:

```text
Chạy lại và cung cấp:
1. Toàn bộ UART log
2. DFAR — Data Fault Address Register
3. DFSR — Data Fault Status Register
4. PC tại thời điểm abort
```

Nếu exception handler chưa print những register đó — đề xuất thêm vào `exception_handlers.c` TRƯỚC khi debug tiếp.

---

## 11. Definition of Done

Task **chưa xong** cho đến khi đủ item tương ứng:

**Driver / kernel feature:**

- Compile sạch (không warning)
- User confirm thành công trên BeagleBone Black thật — không tự giả định
- Boot log sạch (không có error bất ngờ)

**Refactor / cleanup:**

- Compile sạch
- Không thay đổi behavior — kernel binary size bằng hoặc nhỏ hơn = dấu hiệu tốt
- Nói rõ với user rằng thay đổi preserve behavior để họ skip re-test

**Bug fix:**

- Xác định root cause (không chỉ suppress triệu chứng)
- UART log cho thấy failure mode đã biến mất
- User không báo regression ở khu vực liên quan

**Scope checklist phase (từ plan file)**: mọi phase có checklist riêng trong plan — tick đủ mới commit.

**Post-phase cleanup (khi 1 phase P0/P1/... hoàn thành):**

1. Xoá mọi phase-specific comment — `/* P1 init */`, `/* Phase 1 stub */`, `/* TODO P2 */`, `/* lab — remove after PN */`. Code = final form, không WIP markers.
2. Xoá scaffolding / lab code đã xong nhiệm vụ — ví dụ `mmu_lab.c` xoá sau P1 core stable.
3. Boot log chỉnh chu — `[MODULE]` prefix consistent, không debug checkpoint noise, message format final đọc như production log.
4. Git log cuối phase là pattern `Feat(scope): ...` / `Refactor(scope): ...`, không `WIP`, không `fix up`.
5. Sau khi verify trên BBB thật → `git tag v0.PN-complete`.

**Git commit — KHÔNG có `Co-Authored-By: Claude`** trailer trên bất kỳ commit VinixOS nào.

**KHÔNG BAO GIỜ tuyên bố "complete" / "works" chỉ dựa compile thành công.** Luôn nói "build sạch, bạn test trên hardware giúp" và chờ.

---

## 12. Scope Discipline

Match phạm vi hành động chính xác với điều user hỏi.

**Khi user nói "fix X", KHÔNG đụng Y** ngay cả khi Y có vấn đề tương tự. Ghi nhận Y như follow-up riêng.

**Khi không chắc file trong scope hay không, HỎI** — không extend task im lặng. "Nhân tiện tôi cũng..." là con đường over-refactor.

**Refactor đụng hơn 5 file cần user confirm** trước khi bắt đầu. Trình bày scope estimate trước, chờ go-ahead.

**Prefer rolling cleanup hơn big-bang refactor.** Khi một vấn đề style/quality tồn tại khắp codebase, chỉ fix trong file đang sửa vì lý do khác.

**Khi feature mới đụng architecture** (scheduler, VFS, MMU, syscall ABI, driver model) — dừng lại, reference plan file, propose design trước khi viết code.

### Hardcoded scope limits (không mở rộng khi chưa có consumer)

- MAX_TASKS = 5 — KHÔNG raise
- Signal set: `SIGKILL` + `SIGSEGV` only
- Concurrency: spinlock + atomic + wait_queue only
- 1 priority scheduling
- `wait()` any-child only
- No pipe syscall (v1.1)
- No editor on device
- No TCP (UDP only)
- vinixlibc POSIX subset ~5K LOC — KHÔNG grow beyond

### Feature muốn mở rộng?

1. Kiểm tra plan file — có trong roadmap chưa?
2. Có consumer cụ thể trong U1/demo chưa?
3. Nếu không → `docs/known_gaps.md`, KHÔNG add vào phase đang chạy
