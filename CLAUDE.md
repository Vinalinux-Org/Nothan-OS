# CLAUDE.md — VinixOS

**Trước mỗi task liên quan đến hardware hoặc driver**: đọc [reference/index.md](reference/index.md) để biết tài liệu nào có sẵn, sau đó đọc `reference/project_context.md` nếu cần context tổng quan.

**Style / comment / commit deep-dive**: [reference/coding_standards.md](reference/coding_standards.md). File này (CLAUDE.md) là enforcer ngắn gọn, luôn được load. Khi cần rationale, ví dụ đúng/sai, edge case → đọc deep-dive. Khi xung đột: CLAUDE.md thắng.

---

## 1. Hard Rules

### Toolchain

- KHÔNG mix `arm-none-eabi` và `arm-linux-gnueabihf`
- Thứ tự build BẮT BUỘC: `userspace` trước, `kernel` sau

### Hardware access

- Register I/O: `mmio_read32(addr)` / `mmio_write32(addr, val)` — không raw pointer cast
- Address + bit field: xác minh từ AM335x TRM, không đoán
- Driver không hardcode peripheral base — lấy từ `platform_get_resource()` → `board-bbb.c` → `mach/`
- Register có version/revision field: định nghĩa mask constant, dùng masked comparison — raw value chỉ biết sau khi đọc hardware thật
- Peripheral region phải có trong `mmu_build_page_table_boot()` trước khi driver access — nếu thiếu → DATA ABORT ngay lần write đầu tiên

---

## 2. Linux as Reference — Không phải Source

VinixOS học Linux về **kiến trúc, naming, và pattern** — nhưng mọi dòng code là của VinixOS, tự gõ, tự hiểu, tự chịu trách nhiệm.

### Được làm

- Học architecture pattern (VFS ops table, platform driver probe, wait_queue, slab) từ Linux
- Dùng Linux naming: `struct task_struct`, `spinlock_t`, `file_operations`, `kmalloc`, `pr_info`, ...
- Đọc `reference/drivers/*/source/` để hiểu init sequence, register flow
- Ghi `/* Pattern reference: Linux <file>:<func> — re-implemented */` khi áp dụng pattern

### Tuyệt đối không

- Copy, fork, port, paraphrase bất kỳ dòng code nào từ: Linux, musl, glibc, BusyBox, lwIP, FatFs, xv6, Zephyr, TCC, hoặc bất kỳ upstream nào
- Paste code từ internet rồi sửa tên biến
- Commit message `Port X from Y` hay `Based on Linux X`

### Workflow khi đọc reference

1. Đọc reference để hiểu sequence
2. Đóng file reference lại
3. Viết lại từ đầu bằng VinixOS naming + convention
4. Nếu phải mở lại lần 2-3 cho cùng một function → rewrite hoàn toàn từ memory

### Checklist cuối phase

- [ ] Không có file copy từ upstream trong git diff
- [ ] Mọi commit: `Feat/Refactor/Fix(scope): ...` — không `Port`, không `WIP`

---

## 3. Project Context

**VinixOS** — bare-metal ARM, BeagleBone Black (AM3358, Cortex-A8, ARMv7-A). Không Linux, không emulator, không SDK thương mại.

**Components**
- `vinix-kernel/` — kernel C + ARM assembly, `arm-none-eabi-gcc`
- `bootloader/` — U-Boot replacement tự viết
- `userspace/` — init/sh/ls/cat/ps/..., `arm-none-eabi-gcc`
- `compiler/` — VinCC: Python cross-compiler Subset C → ARMv7 ELF32, chạy trên host
- `vinixlibc/` — POSIX subset ~5K LOC (sau P6)

**`vinix-kernel/` layout**
```
arch/arm/
    entry/          — ARMv7 exception vectors, reset handler
    exceptions/     — data abort, prefetch abort, undef handlers
    kernel/         — cpu.c, smp barrier stubs
    mm/             — mmu.S, page table setup
    scheduler/      — context_switch.S
    mach-omap2/     — AM3358 SoC + BBB board
        board-bbb.c — bbb_devices[] + bbb_i2c0_devices[]
        include/mach/ — memory.h, memmap.h, prcm.h, control.h, irqs.h
init/               — main.c (do_initcalls + VFS mount), payload.S
kernel/             — sched, locking, irq, time, printk (KHÔNG đụng khi port)
drivers/            — HW drivers theo Linux subsystem layout
    tty/serial/     — serial_core.c, omap_serial.c
    irqchip/        — irq-omap-intc.c
    clocksource/    — timer-omap-dm.c
    mmc/core/       — core.c, mmc_block.c
    mmc/host/       — omap_hsmmc.c
    i2c/            — i2c-core.c, busses/i2c-omap.c
    gpu/drm/        — tilcdc, tda998x
    video/fbdev/    — fbmem.c, fbcon.c
    watchdog/       — omap_wdt.c
    net/ethernet/   — omap_cpsw.c, omap_mdio.c
    net/ipv4/       — vnet.c, ip.c, tcp.c
    net/app/        — http.c
    base/           — device.c, platform.c
fs/                 — VFS, FAT32, devfs, procfs
mm/, block/, lib/
include/vinix/      — cdev.h, blkdev.h, serial_core.h, fb.h, i2c.h, mmc/host.h, netdevice.h, mdio.h, phy.h, ...
```

**Port sang SoC khác** = viết `arch/arm/mach-<new>/` + driver mới. `kernel/` + `drivers/*/core/` không đổi dòng nào.

---

## 4. Linux Style — Adapt, Not Copy

VinixOS dùng Linux làm **kim chỉ nam về naming và shape** để code dễ đọc với bất kỳ ai quen Linux. Nhưng mọi implementation là của VinixOS — tự viết theo đúng hiểu biết, không phải translation từ Linux source.

> Mục tiêu: senior dev đọc source thấy ngay "Linux-style OS thu nhỏ" — không phải hobby OS lập dị.

### Naming và data structure follow Linux

| Subsystem | VinixOS dùng |
| --- | --- |
| Memory | `kmalloc/kfree`, `alloc_pages/free_pages`, `struct page`, `struct vm_area_struct` |
| Process | `struct task_struct`, `current`, states `TASK_RUNNING/INTERRUPTIBLE/UNINTERRUPTIBLE/STOPPED/ZOMBIE` |
| Syscall | `do_sys_open`, `do_fork`, `do_exec` — error = negative errno |
| User access | `copy_from_user`, `copy_to_user` |
| Concurrency | `spinlock_t`, `struct mutex`, `wait_queue_head_t`, `atomic_t`, `smp_mb/rmb/wmb` |
| VFS | `struct file/inode/dentry/super_block`, `struct file_operations/inode_operations` |
| Driver model | `struct platform_device/platform_driver`, `platform_get_resource` |
| Utility | `container_of`, `list_head`, `BUG_ON`, `WARN_ON`, `likely/unlikely`, `ARRAY_SIZE` |

### Logging — BẮT BUỘC prefix `[MODULE]`

```c
pr_info("[TIMER] Initializing DMTimer2...\n");
pr_err("[SCHED] No tasks to run!\n");
```

### Scope Linux không dùng

`sysfs`, Device Tree động, RCU, cgroup, namespace, SELinux — không có consumer trong VinixOS MVP.

---

## 5. Driver Development

### THE CONVENTION — không exception

1. Mọi HW driver có 1 entry static trong `board-bbb.c::bbb_devices[]` (platform) hoặc `bbb_i2c0_devices[]` (I2C).
2. Mọi HW init logic nằm trong `probe()`. KHÔNG có public `xxx_init()` gọi từ `main.c`.
3. Đăng ký qua `module_platform_driver(drv)` hoặc `module_i2c_driver(drv)`.
4. Inter-driver dependency: `if (!dep_ready) return -EPROBE_DEFER`. Core retry sau `driver_deferred_probe_trigger()`.
5. `init/main.c` CHỈ chứa `do_initcalls(N)` + core init + VFS mount + deferred probe trigger. KHÔNG gọi driver-specific init trực tiếp.

### Driver template
```c
static int omap_xxx_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq = platform_get_irq(pdev, 0);
    /* HW init, subsystem register */
    return 0;
}

static struct platform_driver omap_xxx_driver = {
    .drv   = { .name = "omap-xxx" },
    .probe = omap_xxx_probe,
    .remove = omap_xxx_remove,
};
module_platform_driver(omap_xxx_driver);
```

Board file entry:
```c
static struct platform_device omap_xxx0 = {
    .name = "omap-xxx",
    .base = 0xADDR,
    .irq  = IRQ_NUM,
};
```

### Subsystem register fn
| Driver | Header | Fn |
| --- | --- | --- |
| UART | `vinix/serial_core.h` | `uart_register_driver`, `uart_add_one_port` |
| Char | `vinix/cdev.h` | `cdev_register` |
| Block | `vinix/blkdev.h` | `add_disk` |
| I2C host | `vinix/i2c.h` | `i2c_add_adapter` |
| MMC host | `vinix/mmc/host.h` | `mmc_alloc_host` + `mmc_add_host` |
| Framebuffer | `vinix/fb.h` | `register_framebuffer` |
| IRQ chip | `vinix/irqchip.h` | `irqchip_register` |
| Watchdog | `vinix/watchdog.h` | `watchdog_register_device` |
| Network (eth)  | `vinix/netdevice.h` | `alloc_netdev` + `register_netdev` |

### Initcall levels
```
Level 1 core_initcall:   bbb_platform_init — register platform_devices
Level 3 arch_initcall:   uart, wdt
Level 4 subsys_initcall: IRQ controller, bus host (i2c, mdio, ...)
Level 5 fs_initcall:     mmc
Level 6 device_initcall: display, timer, i2c clients, network, ...
Level 7 late_initcall:   selftest
→ driver_deferred_probe_trigger()
```

### Pre-requisite checklist (xác minh trước khi viết driver mới)
| Thông tin | Nguồn |
| --- | --- |
| Base address | AM335x TRM Ch.02 |
| Register offset + bit definition | TRM chapter của peripheral |
| Clock enable sequence | AM335x TRM Ch.08 PRCM |
| IRQ number | AM335x TRM Ch.06 |
| Pin mux | AM335x TRM Ch.09 Control Module |

THIẾU bất kỳ → DỪNG NGAY, KHÔNG ĐOÁN. Đọc `reference/drivers/<name>/index.md` trước khi viết.

**Gold reference**: [vinix-kernel/drivers/tty/serial/omap_serial.c](vinix-kernel/drivers/tty/serial/omap_serial.c) — 100% compliant.
**Template skeleton**: [Documentation/driver-template/](Documentation/driver-template/)

---

## 6. Code Generation Protocol

**Không đủ thông tin → DỪNG và hỏi. Không bịa.**

### Bước 0 — luôn làm trước khi viết

**Mọi loại code**: đọc file đang sửa và các header phụ thuộc trước khi gen. Nếu behavior chưa rõ → hỏi, không tự suy diễn.

Loại code khác nhau, pre-requisite khác nhau:

| Loại code | Pre-requisite bắt buộc |
| --- | --- |
| Hardware / driver | TRM verified: address, register offset, IRQ, clock. Đọc `reference/drivers/<name>/index.md` |
| Pure software | Đọc interface hiện tại + `reference/software/<name>/index.md`. Không cần TRM |

### Thông tin bắt buộc trước khi viết driver

- Base address: `mach/memmap.h` hoặc AM335x TRM Ch.02
- Register offset + bit field: TRM chapter của peripheral
- IRQ number: `mach/irqs.h` hoặc TRM Ch.06
- Clock enable: TRM Ch.08 (PRCM)
- Init sequence / timing: `reference/drivers/<name>/index.md`

Thiếu bất kỳ → báo user theo format:

```
Để viết [function/driver], tôi cần:
- [thông tin A] → [nguồn: TRM Ch.XX / file path]
- [thông tin B] → [nguồn: ...]
Bạn chỉ tôi đọc file nào, hoặc cung cấp trực tiếp?
```

### Tuyệt đối không gen placeholder

Không dùng `0xDEADBEEF`, magic number tự đặt, register address copy từ internet hoặc training data mà chưa verify TRM.

### Khi thiếu thông tin — format báo cáo bắt buộc

Dừng lại và liệt kê rõ:

```
Để gen [tên function/file], tôi cần thêm:
- [thông tin A] → [nguồn: AM335x TRM Ch.XX / file path / ...]
- [thông tin B] → [nguồn: ...]

Bạn có thể cung cấp hoặc chỉ tôi đọc file nào không?
```

**KHÔNG được**: gen placeholder value (`0xDEADBEEF`, `/* TODO */`, magic number tự đặt), assume register offset từ project khác, copy address từ internet / training data mà không verify TRM.

### Các tình huống LUÔN LUÔN phải hỏi trước

| Tình huống | Lý do dừng |
| --- | --- |
| Register address / offset chưa verify TRM | Sai address = brick device hoặc undefined behavior |
| IRQ number chưa confirm `mach/irqs.h` | Sai IRQ = silent failure hoặc wrong handler |
| Init sequence chưa có TRM reference | AM335x có ordering dependency không hiển nhiên |
| File muốn sửa chưa được đọc | Có thể overwrite logic quan trọng đang tồn tại |
| Behavior ambiguous ("làm cho nó work") | Viết code sai direction, waste cả session |
| Thêm syscall / VFS path mới | Cần verify ABI + struct layout với kernel hiện tại |

### Khi có đủ thông tin — gen đúng pattern

1. Đọc gold reference hoặc existing driver tương tự trước
2. Copy template từ `Documentation/driver-template/` nếu là driver mới
3. Gen code theo THE CONVENTION (section 5) — không skip subsystem
4. Sau gen: liệt kê rõ "cần verify trên hardware" — không tuyên bố works

---

## 7. Comments

**Mặc định: KHÔNG viết comment.** Nếu xóa comment mà code vẫn đọc được → xóa.

**5 trường hợp ĐƯỢC viết**
1. Hardware constraint không hiển nhiên (ordering, reset, clock dependency)
2. Magic value lý do: `/* FB at 0x80800000 = kernel(5MB)+user(1MB)+2MB margin */`
3. Invariant tinh tế reader có thể phá (buffer sharing, init order, locking rule)
4. `CRITICAL:` — interrupt-mode / sensitive context
5. Cross-reference TRM khi hardcoded sequence

**KHÔNG viết**: banner dài dòng, "imported from", "consumed by Y", giải thích WHAT, struct field trivial, phase marker `/* P1 */`, PR reference.

**File header** — block 4-6 dòng, không separator:
```c
/*
 * drivers/path/file.c — One-line module description
 *
 * 1-2 dòng nói WHAT module provide hoặc constraint chính.
 */
```
KHÔNG: SPDX, Copyright, Author, Date, History, `====` separator. Git lo blame/log/license.

**Struct public trong header** — kernel-doc `/** */` cho VSCode hover:
```c
/**
 * struct uart_port - serial port state
 * @base: MMIO base address
 * @irq:  hardware IRQ number
 */
struct uart_port { ... };
```
Struct internal trong .c → không cần.

**Function header** — KHÔNG mặc định. Chỉ khi: precondition không hiển nhiên, side effect ẩn, return edge case, ordering/locking/IRQ constraint. Function tên không tự giải thích → đổi tên trước, không comment thay.

**Debug print**: comment out bằng `//` khi bring-up. Xóa hẳn sau khi feature ổn định.

→ Deep-dive: [reference/coding_standards.md §5 Comments](reference/coding_standards.md#5-comments)

---

## 8. Coding Style

- 4 space, KHÔNG tab
- `snake_case` variable/function, `UPPER_SNAKE` macro
- Public symbol BẮT BUỘC module prefix: `uart_*`, `scheduler_*`, `intc_*`, `vfs_*`, `platform_*`
- Include dùng `""`, không `<>` trừ `<stdarg.h>`

**File layout**
1. Banner (1-line description)
2. `#include`
3. `#define` register/field local
4. `static` state + forward decl
5. Static helpers
6. Public functions

**Braces**: function → `{` xuống dòng mới; control flow → `{` cùng dòng.

**Assembly**: inline comment dùng `@`. Banner giống C.

→ Deep-dive: [reference/coding_standards.md §2 Coding Style](reference/coding_standards.md#2-coding-style) · [§3 Naming](reference/coding_standards.md#3-naming-convention) · [§4 File Layout](reference/coding_standards.md#4-file-layout)

---

## 9. Debug Workflow

Không JTAG. **UART log là công cụ debug duy nhất.**

**Khi báo bug — gom đủ TRƯỚC KHI phân tích**:
- Toàn bộ UART log từ đầu boot
- Dòng log cuối trước hang/crash
- Loại exception (Data Abort / Prefetch Abort / Undefined)
- Thao tác ngay trước bug

**KHÔNG đoán nguyên nhân khi chưa có UART log.**

**Checkpoint print**:
```c
pr_info("[MODULE] Step X: before\n");
/* operation */
pr_info("[MODULE] Step X: after — reg = 0x%08x\n", mmio_read32(REG));
```

BẮT BUỘC readback register sau write:
```c
mmio_write32(BASE + OFFSET, value);
pr_info("[DRV] wrote 0x%08x, readback = 0x%08x\n",
        value, mmio_read32(BASE + OFFSET));
```

**Exception/Abort**: yêu cầu DFAR, DFSR, PC. Nếu handler chưa print → thêm vào `arch/arm/exceptions/` trước khi debug tiếp.

**Pure software** (network stack, ...): không có register để readback. Debug bằng log tại boundary giữa các tầng — mỗi tầng phải log đủ để xác định frame/data đứt ở đâu mà không cần đoán tầng khác.

---

## 10. Definition of Done

**KHÔNG bao giờ tuyên bố "complete" / "works" chỉ dựa compile.** Luôn nói "build sạch, bạn test trên hardware giúp" và chờ.

**Driver / kernel feature**: compile không warning + user confirm trên BBB thật + boot log sạch.

**Refactor**: compile không warning + không thay đổi behavior.

**Bug fix**: xác định root cause + UART log cho thấy failure biến mất.

**Post-phase cleanup** (khi P0/P1/... hoàn thành):
1. Xóa mọi phase marker: `/* P1 init */`, `/* Phase 1 stub */`, `/* TODO P2 */`
2. Xóa scaffolding / lab code đã xong nhiệm vụ
3. Boot log chỉnh chu: `[MODULE]` prefix consistent, không debug noise
4. Commit cuối phase: `Feat(scope): ...` hoặc `Refactor(scope): ...` — không `WIP`

---

## 11. Commit Style

**Format**: `Type(scope): short description` — không có body bắt buộc.

**Types**: `Feat`, `Fix`, `Refactor`, `Docs`, `Test`

**Ví dụ đúng**:
```
Feat(mmc): implement omap_hsmmc probe and card init sequence
Fix(sched): clear stale task state on SIGKILL
Refactor(drivers): remove hardcoded peripheral bases from omap_serial/wdt
Docs(driver): add platform_driver template skeleton
```

**KHÔNG có** `Co-Authored-By: Claude` trailer trên bất kỳ commit VinixOS nào.
**KHÔNG dùng** `git tag v0.PN-complete` — chỉ commit.
**KHÔNG amend** commit đã push.

→ Deep-dive: [reference/coding_standards.md §8 Commit Style](reference/coding_standards.md#8-commit-style) · [§10 Anti-patterns](reference/coding_standards.md#10-anti-patterns-catalog)

---
