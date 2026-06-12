# NothanOS Kernel

Kernel chính của NothanOS. Monolithic, ARMv7-A, viết từ đầu cho AM3358 trên BeagleBone Black.

Tài liệu này dành cho người đọc, sửa, hoặc thêm code vào kernel. Người chỉ muốn build và chạy nên đọc [README ở root](../README.md).

---

## Bản đồ bộ nhớ

Kernel chạy ở virtual address cao, tách biệt hoàn toàn với userspace ở virtual address thấp.

| Vùng | Địa chỉ | Mô tả |
| --- | --- | --- |
| User VA | `0x40000000` – `0xBFFFFFFF` | Per-process: text, stack, heap. Mapping theo TTBR0. |
| Kernel VA | `0xC0000000` – `0xDFFFFFFF` | Kernel image + linear map của DDR3. TTBR1, shared. |
| Peripherals | `0x44000000` – `0x4A000000` | Mapped device memory: INTC, UART, MMC, GPIO, Control Module. |
| DDR3 (phys) | `0x80000000` – `0x9FFFFFFF` | 512 MB RAM thật. Linear-mapped vào `0xC0000000`. |

Macro trong linker script:

```text
PHYS_OFFSET = 0x80000000   # DDR3 base
PAGE_OFFSET = 0xC0000000   # Kernel VA base
MMU_OFFSET  = PAGE_OFFSET - PHYS_OFFSET
```

Page table dùng ARMv7 short-descriptor format: L1 sections 1 MB cho linear map và peripherals, L2 pages 4 KB cho per-process userspace.

---

## Boot flow

1. **`arch/arm/kernel/head.S`** — kernel entry. CPU đang chạy ở physical address `0x80000000` với MMU off. Setup stack tạm, gọi `mmu_init`.
2. **`arch/arm/mm/mmu.c`** — dựng page table ban đầu: identity-map vùng `.idmap.text` (để code chạy được tại cả PA lẫn VA trong lúc bật MMU), map kernel vào `0xC0000000`, map peripherals.
3. **`arch/arm/mm/mmu_enable.S`** — bật MMU, nhảy lên VA cao.
4. **`init/main.c::kernel_main`** — chạy ở VA cao, MMU đã active. Init platform, gọi `do_initcalls()`.
5. **`drivers/base/init.c`** — duyệt initcall theo 3 level: `EARLY` (UART, INTC, timer), `NORMAL` (MMC, GPIO, pinctrl), `LATE` (filesystem mount, userspace spawn).
6. **`kernel/spawn.c`** — spawn `/sbin/init` (PID 1) từ payload nhúng trong kernel.
7. Scheduler bắt đầu tick, init exec sang `/bin/sh`.

---

## Kiến trúc subsystem

Kernel chia thành các nhóm chức năng lớn, mỗi nhóm có mục tiêu rõ ràng và ranh giới chặt chẽ với các nhóm khác. Phần này mô tả từng nhóm: vai trò của nó trong kernel, các module bên trong, và trạng thái hiện tại.

```text
                       ┌───────────────────────────────────────┐
                       │             Userspace (ELF)            │
                       └──────────────────┬─────────────────────┘
                                          │ SVC
              ┌───────────────────────────┴───────────────────────────┐
              │                    Syscall layer                       │
              └───────────────────────────┬───────────────────────────┘
                                          │
   ┌──────────────┬───────────────┬───────┴────────┬─────────────────┐
   │   Process    │    Memory     │  Filesystem    │  Driver model   │
   │  & Scheduler │   Management  │     & VFS      │ (platform bus)  │
   └──────┬───────┴──────┬────────┴────────┬───────┴────────┬────────┘
          │              │                 │                │
   ┌──────┴──────────────┴─────────────────┴────────────────┴────────┐
   │                  IRQ / Exception / Time                          │
   └─────────────────────────────┬────────────────────────────────────┘
                                 │
   ┌─────────────────────────────┴────────────────────────────────────┐
   │         arch/arm: boot, MMU, vectors, context switch              │
   └──────────────────────────────────────────────────────────────────┘
```

### 1. Nền tảng kiến trúc (`arch/arm/`)

Lớp dưới cùng, phụ thuộc trực tiếp vào ARMv7-A. Mọi thứ liên quan đến CPU, MMU, exception vectors, context switch đều nằm ở đây. Đổi sang SoC khác cùng ARMv7-A chỉ cần đổi `mach-*/`; đổi sang kiến trúc khác (ARMv8, RISC-V) cần thay toàn bộ `arch/`.

| Module | Vai trò |
| --- | --- |
| `kernel/head.S` | Kernel entry sau khi bootloader bàn giao quyền điều khiển |
| `kernel/vectors.S` | Bảng exception vector (7 entry) |
| `kernel/traps.c` | C-side exception handler, register dump |
| `kernel/switch_to.S` | Context switch: save/restore r4–r11, sp, lr, swap TTBR0 |
| `mm/mmu.c` | Dựng page table, vmap, unmap |
| `mm/mmu_enable.S` | Bật MMU và nhảy lên VA cao |
| `mach-omap2/board-bbb.c` | Board data: platform device table, IRQ, pin mux init cho BBB |
| `kernel.ld` | Linker script — layout của kernel image |

Trạng thái: hoàn chỉnh. MMU 2-level page table chạy ổn, exception vectors phục hồi được userspace fault mà không kill kernel.

### 2. Quản lý tiến trình & lập lịch (`kernel/sched/`, `kernel/spawn.c`, `kernel/exit.c`, `kernel/syscall.c`)

Tạo, lập lịch, chuyển đổi và huỷ tiến trình. Cung cấp ABI giữa userspace và kernel.

| Module | Vai trò |
| --- | --- |
| `sched/core.c` | Preemptive round-robin, time slice 10 ms, single run queue |
| `sched/rt.c` | O(1) priority queue |
| `sched/wait.c` | Wait queue cho blocking I/O |
| `sched/completion.c` | Completion primitive cho đồng bộ giữa task và IRQ |
| `spawn.c` | Tạo task mới từ ELF blob (payload nhúng hoặc đọc từ FS) |
| `exit.c` | Process termination, zombie handling, reap |
| `syscall.c` | SVC dispatch, 22 syscall |

Trạng thái: `MAX_TASKS = 5`, đủ cho init + shell + vài background job. Task state machine `READY`/`RUNNING`/`BLOCKED`/`ZOMBIE` hoạt động ổn định.

### 3. Quản lý bộ nhớ (`mm/`)

Cấp phát physical page, object kernel, page table cho process.

| Module | Vai trò |
| --- | --- |
| `page_alloc.c` | Buddy allocator, order 0 (4 KB) đến order 10 (4 MB) |
| `slab.c` | Slab cache cho fixed-size object trên top của buddy |

Trạng thái: buddy + slab chạy ổn. Per-process page table riêng, context switch swap TTBR0 và flush toàn bộ TLB (không dùng ASID — đơn giản, đổi performance lấy correctness).

### 4. Xử lý ngắt & ngoại lệ (`kernel/irq/`, `arch/arm/kernel/traps.c`)

Tiếp nhận signal từ hardware và CPU exception, dispatch về handler đăng ký.

| Module | Vai trò |
| --- | --- |
| `irq/irq_core.c` | IRQ handler registration và dispatch |
| `arch/arm/kernel/vectors.S` | Bảng vector 7 entry |
| `arch/arm/kernel/traps.c` | Xử lý undef / abort, decode DFSR/DFAR |

Trạng thái: 7 vector đầy đủ. Userspace fault → SIGSEGV; kernel fault → PANIC dump. Không nested interrupt.

### 5. Time (`kernel/time/`)

Cung cấp tick định kỳ cho scheduler và API delay.

| Module | Vai trò |
| --- | --- |
| `time/timer.c` | DMTimer2 tick 10 ms, jiffies counter |
| `time/delay.c` | `udelay` busy-wait, `msleep` blocking |

Trạng thái: 10 ms tick ổn định. Chưa có high-resolution timer.

### 6. Hệ thống file & block I/O (`kernel/vfs/`, `kernel/fs/`)

VFS layer trừu tượng nhiều filesystem dưới một giao diện chung. Block layer trừu tượng phần cứng lưu trữ.

| Module | Vai trò |
| --- | --- |
| `vfs/vfs.c` | Mount, lookup theo longest-prefix, dentry cache |
| `vfs/block.c` | Block device interface |
| `fs/fat/` | FAT32 driver — đọc/ghi, subdirectory, 8.3 filename |
| `fs/devfs/` | `/dev` — virtual character device filesystem |

Trạng thái: mount rootfs FAT32 từ SD, `/dev` cho UART và null device. Đã có procfs cơ bản (`/proc/meminfo`, `/proc/<pid>/status`). Buffer cache LRU + write-back đang hoạt động.

### 7. Driver model & device drivers (`drivers/`)

Driver model theo pattern platform bus của Linux: driver mô tả khả năng, device mô tả tài nguyên cụ thể, bus core ghép cặp dựa trên tên. Driver phải portable — không hardcode base address hay IRQ. Mọi giá trị board-specific nằm trong `arch/arm/mach-omap2/board-bbb.c`. Driver lấy tài nguyên qua `platform_get_resource()`. Mọi hardware init xảy ra trong `probe()`, không có `xxx_init()` public.

**Driver framework (`drivers/base/`)**

| Module | Vai trò |
| --- | --- |
| `platform.c` | `platform_device`, `platform_driver`, bus matching |
| `bus.c` | Device/driver bus framework chung |
| `init.c` | 3-level initcall (`EARLY` / `NORMAL` / `LATE`) |
| `cdev.c` | Character device framework |

**Console (`drivers/tty/`, `drivers/irqchip/`)**

| Module | Vai trò |
| --- | --- |
| `tty/serial/omap-serial.c` | UART0 driver — 115200 8N1, RX ring buffer qua IRQ |
| `irqchip/irq-omap-intc.c` | INTC interrupt controller (128 IRQ, priority queue) |

**Timer (`drivers/clocksource/`)**

| Module | Vai trò |
| --- | --- |
| `clocksource/timer-ti-dm.c` | DMTimer2 — clock source cho scheduler tick |

**Storage (`drivers/mmc/`, `drivers/block/`)**

| Module | Vai trò |
| --- | --- |
| `mmc/omap-hsmmc.c` | MMC0 SD card controller — polled I/O, 512B sector |
| `block/genhd.c` | Generic disk framework + MBR parser |

**GPIO & Pin control (`drivers/gpio/`, `drivers/pinctrl/`)**

| Module | Vai trò |
| --- | --- |
| `gpio/gpiolib.c` | GPIO framework chung |
| `gpio/gpio-omap.c` | AM335x GPIO driver — 4 bank (`gpio0..3`) |
| `pinctrl/pinctrl-am335x.c` | Pin mux qua Control Module |

Trạng thái: tất cả driver trên đã probe và chạy ổn định trên BBB thật. Chưa có driver cho ethernet (CPSW), USB, framebuffer, audio, ADC.

### 8. Đồng bộ hoá

Primitive chia sẻ giữa các nhóm trên — không phải subsystem độc lập, mà là toolbox dùng xuyên suốt.

- **Spinlock** (`spinlock_t`) — LDREX/STREX, dùng trong critical section ngắn, không sleep
- **Atomic ops** (`atomic_t`) — `atomic_read`/`set`/`add_return`/`cmpxchg`
- **Memory barriers** — `smp_mb`, `dmb`, `dsb`, `isb` cho ordering
- **Wait queue** — blocking, đánh thức khi sự kiện xảy ra
- **Completion** — đồng bộ giữa task và IRQ handler

### 9. Logging (`kernel/printk.c`)

Một kênh duy nhất ra UART0. Driver dùng `pr_info` / `pr_err` / `pr_warn` / `pr_debug` với prefix `[<MODULE>]` để dễ trace. Đây là công cụ debug chính của kernel — không có JTAG.

---

## Syscall ABI

22 syscalls, dispatch qua instruction `svc #0`. Theo AAPCS:

- `r7` chứa syscall number
- `r0`–`r5` chứa argument
- `r0` trả về kết quả (âm = `-errno`)

Bảng syscall đầy đủ trong `include/nothan/syscall.h`. Phân nhóm:

| Nhóm | Syscalls |
| --- | --- |
| Process | `fork`, `exec`, `exit`, `wait`, `yield`, `getpid`, `getppid`, `kill` |
| I/O | `read`, `write`, `open`, `close`, `dup`, `dup2` |
| Filesystem | `read_file`, `write_file`, `listdir`, `unlink`, `rename`, `chdir`, `getcwd` |
| Introspection | `get_tasks`, `get_meminfo`, `devlist` |

Mọi con trỏ từ userspace phải đi qua `copy_from_user` / `copy_to_user`. Không dereference trực tiếp.

---

## Tham chiếu

- [Root README](../README.md) — bối cảnh project, mục tiêu, build & flash
- [CLAUDE.md](../CLAUDE.md) — hard rules dành cho contributor
- [reference/am335x/](../reference/am335x/) — TRM theo subsystem
- [reference/arm-arch/](../reference/arm-arch/) — ARM ARM (instruction set, exception model)
- [reference/hardware-beagleboneblack/](../reference/hardware-beagleboneblack/) — BBB schematic, pinout
