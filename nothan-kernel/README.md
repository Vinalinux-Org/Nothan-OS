# NothanOS

Hệ điều hành tối giản cho ARMv7-A, chạy trên BeagleBone Black.

## Tổng Quan

NothanOS là bare-metal monolithic kernel tự viết 100% từ zero — không dựa trên Unix/Linux/BSD. API shape theo convention Linux để dễ đọc/audit, nhưng mọi dòng code là của NothanOS:

- Boot chain: ROM → MLO → kernel, 4-layer HAL (kernel / arch / platform / drivers)
- Memory: bitmap page allocator, slab (`kmem_cache_*`), `kmalloc/kfree`, VMM, L1 section + L2 page table, per-process TTBR0
- Concurrency: `spinlock_t`, `atomic_t`, `wait_queue_head_t`, `wait_event/wake_up`, preemptive scheduler, blocking I/O
- Process: `fork` / `exec` / `wait` / `exit` / `kill`, SIGSEGV isolation, per-process fd table, MAX_TASKS = 5
- Syscalls (22): AAPCS-compliant SVC interface, `copy_from/to_user`, negative errno
- VFS + FAT32 (subdir read + unlink + rename) + devfs (`/dev/tty`, `/dev/null`) + procfs (`/proc/...`) + block layer + LRU buffer cache
- Driver model: Linux-style `platform_device/driver` + `platform_get_resource` matching engine
- Userspace: init (PID 1) + shell fork+exec + 10 external coreutils + nothanlibc POSIX subset

**Target Hardware**: BeagleBone Black (TI AM335x, Cortex-A8)

## Tính Năng

- Boot chain hoàn chỉnh: ROM → MLO → Kernel
- MMU-based virtual memory (User: 0x40000000, Kernel: 0xC0000000), L1 section + L2 4KB pages, per-process TTBR0
- Exception handling (7 types, DFSR/DFAR decode, user page fault → SIGSEGV, kernel fault → PANIC)
- Preemptive round-robin scheduler (10ms tick, single priority, wait queue + sleep)
- 22 syscalls qua SVC: `fork/exec/wait/exit/kill`, `open/read/write/close/lseek/dup/dup2`, `unlink/rename/listdir`, `getpid/getppid`, `yield`, `get_tasks/get_meminfo`, `devlist`
- Process model: `task_struct`, `current`, MAX_TASKS=5, fork+exec ELF loader, per-process fd table, SIGSEGV/SIGKILL isolation
- Memory: page allocator + slab + `kmalloc(GFP_KERNEL)` + VMM (`vm_area_struct`, `mm_struct`), stack canary
- Concurrency: `spinlock_t` (LDREX/STREX), `atomic_t`, `wait_event`/`wake_up`, blocking `sys_read` qua UART RX wait queue
- VFS multi-FS: FAT32 rootfs (subdir read + unlink + rename) + devfs (`/dev/tty`, `/dev/null`) + procfs (`/proc/meminfo`, `/version`, `/mounts`, `/<pid>/status`)
- Block layer + buffer cache LRU 64×512 B write-back
- Driver model: Linux-style `platform_device/driver`, `platform_get_resource`, bus matching, 4 drivers wired (uart/timer/intc/mmc)
- Userspace: init (PID 1) + shell + 10 external ELFs (`ls cat echo ps kill pwd free uname rm mv`) + nothanlibc POSIX subset (~1.4 KLOC)
- HDMI 800×600 RGB565 via TDA19988 + LCDC framebuffer, UART console 115200 8N1

## Cấu Trúc Project

```text
nothan-kernel/
├── bootloader/              # MLO (SRAM stage, boots kernel from SD sector 2048)
├── arch/arm/                # entry.S, MMU asm, context switch, exception vectors
│   └── mach-omap2/          # BBB/AM3358 board: memory map, IRQ numbers, platform device table
├── init/                    # main.c, initcall.c, payload.S
├── kernel/                  # core kernel: sched, locking, irq, time, printk, fork/exec/wait
├── drivers/                 # HW drivers + subsystem cores:
│   ├── tty/                 # serial_core.c, serial/omap_serial.c
│   ├── irqchip/             # irq-omap-intc.c
│   ├── clocksource/         # timer-omap-dm.c
│   ├── mmc/                 # core/core.c, core/mmc_block.c, host/omap_hsmmc.c
│   ├── i2c/                 # i2c-core.c, busses/i2c-omap.c
│   ├── gpu/drm/             # tilcdc, tda998x
│   ├── video/               # boot_screen.c, fbdev/fbmem.c, fbdev/fbcon.c
│   ├── watchdog/            # omap_wdt.c
│   ├── base/                # device.c, platform.c (driver model)
│   └── char/                # char_dev.c
├── fs/                      # vfs.c, fat32.c, devfs.c, procfs.c
├── mm/                      # page_alloc.c, slab.c, vmm.c
├── block/                   # block.c, buffer_cache.c, partitions/msdos.c
├── lib/                     # string.c, format.c, fonts/, test/selftest.c
├── include/                 # kernel headers
│   └── nothan/               # subsystem headers (i2c.h, mmc/host.h, serial_core.h, ...)
├── userspace/
│   ├── apps/                # init, shell, ls, cat, echo, ps, kill, pwd, free, uname, rm, mv, hello
│   ├── lib/                 # crt0.S, syscall.c
│   └── nothanlibc/           # POSIX subset: string, stdio, stdlib, unistd, fcntl, ctype, signal, sys/*
└── Documentation/           # tài liệu kỹ thuật
```

## Yêu Cầu Hệ Thống

**OS**: Ubuntu 22.04 LTS (khuyến nghị)

**Hardware**:
- BeagleBone Black board
- microSD card (tối thiểu 128MB)
- USB-to-Serial adapter (3.3V TTL, 115200 8N1)

**Software**:
```bash
# Cài dependencies
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi
sudo apt-get install python3 python3-pip make screen

# Hoặc dùng script tự động (chạy từ thư mục gốc RefARM-OS/)
sudo bash scripts/setup-environment.sh
```

## Build

```bash
# Build toàn bộ (bootloader + kernel + userspace)
make

# Hoặc build riêng
make -C bootloader
make -C userspace
make -C kernel
```

## Deploy Lên SD Card

```bash
# One-shot: build + deploy userspace ELFs vào FAT32 + flash MLO/kernel
sudo ./scripts/deploy_and_flash.sh /dev/sdX

# Chỉ ghi đè MLO + kernel (không đụng FAT32 rootfs):
sudo ./scripts/flash_sdcard.sh /dev/sdX
```

## Chạy

1. Cắm SD card vào BeagleBone Black
2. Kết nối serial console:
   ```bash
   screen /dev/ttyUSB0 115200
   ```
3. Bật nguồn
4. Tương tác với shell

## Shell Commands

Shell fork+exec ELF từ `/bin/` — auto-prepend `/bin/` nếu command không có `/`.

```text
# Built-ins
$ help                    # list built-ins
$ cd /etc
$ pwd
$ exit

# External ELFs trên FAT32 (/bin/)
$ ls /                    # root: bin sbin etc
$ ls /bin                 # 10 utilities + hello
$ cat /etc/motd
$ echo "hi" > /tmp/a      # redirect >, >>, <
$ ps                      # liệt kê task đang chạy
$ kill <pid>              # SIGKILL
$ free                    # đọc /proc/meminfo
$ uname                   # NothanOS
$ rm /tmp/a
$ mv /tmp/x /tmp/y

# procfs introspection
$ cat /proc/meminfo
$ cat /proc/mounts
$ cat /proc/1/status      # init
```

## Memory Map

### Physical Memory

```text
0x80000000 - 0x807FFFFF: Kernel image + DDR pool (first 8 MB)
0x80800000 - 0x80BFFFFF: HDMI framebuffer (4 MB, non-cacheable)
0x80C00000 - 0x87FFFFFF: Page allocator pool (112 MB bitmap)
0x44E00000 - 0x44E0FFFF: L4_WKUP peripherals (PRCM, UART0, WDT1, Control Module)
0x48000000 - 0x482FFFFF: L4_PER peripherals (INTC, DMTimer, MMC0, I2C0)
0x4830E000 - 0x4830EFFF: LCDC
```

### Virtual Memory

```text
0x40000000 - 0x400FFFFF: User space — 1 MB per process, per-process L2 + TTBR0
0xC0000000 - 0xC04FFFFF: Kernel DDR (5 MB, kernel-only, cached)
0xC1000000 - 0xC7FFFFFF: Kernel page pool (112 MB, slab + kmalloc + VMM allocations)
0x44E00000 / 0x48000000: Peripherals (identity mapped, Strongly Ordered)
```

## Kiến Trúc

### Boot Sequence

```
Power On → ROM Code → MLO → Kernel (entry.S) → kernel_main() → Scheduler
```

**MLO** khởi tạo:

- Clock configuration (DDR PLL @ 400 MHz)
- DDR3 memory (128 MB)
- MMC/SD interface
- Load `kernel.bin` từ SD sector 2048 vào DDR `0x80000000`

**Kernel entry**:

- Clear BSS, setup L1 section table (MMU Phase A — identity + high VA)
- Enable MMU, reload stack pointers, trampoline sang VA `0xC0000000`
- `platform_init()` → `driver_init_all()` (Linux-style platform bus matching)
- Init `page_alloc → slab → kmalloc → vmm` (memory foundation)
- Init sync primitives, scheduler, VFS + mount FAT32 / devfs / procfs
- Spawn init (PID 1 từ embedded payload), scheduler takeover

### MMU Configuration

- **L1 section table** (4096 × 1 MB): kernel VA 0xC0000000, peripherals identity-mapped
- **L2 page table** 4 KB granularity cho user VA 0x40000000 — mỗi process có L2 + pgd riêng
- TTBR0 switch trong `context_switch.S` khi task có `mm` khác
- User AP = User RW, Kernel AP = Kernel-only, Peripherals Strongly Ordered

### Task Scheduling

- Round-robin preemptive, single priority, 10 ms tick (DMTimer2)
- MAX_TASKS = 5 (idle + init + shell + 2 dynamic slot cho fork/exec)
- States: `TASK_RUNNING`, `TASK_INTERRUPTIBLE`, `TASK_UNINTERRUPTIBLE`, `TASK_STOPPED`, `TASK_ZOMBIE`
- Context switch: save/restore r0–r12, SP, LR, SPSR, SP_usr, LR_usr, TTBR0 nếu `mm` khác
- Blocking I/O qua `wait_event` / `wake_up` (ví dụ `sys_read` trên UART RX)

### System Calls

**ABI**: AAPCS-compliant qua SVC

- r7 = syscall number (0–21)
- r0–r3 = arguments (copy_from_user/copy_to_user cho con trỏ)
- r0 = return value (negative errno nếu lỗi)

**22 syscalls** (`kernel/include/syscalls.h`):

| Nhóm | Syscalls |
| ------ | ---------- |
| I/O | `write`, `read`, `open`, `close`, `dup`, `dup2` |
| File | `read_file`, `write_file`, `listdir`, `unlink`, `rename` |
| Process | `fork`, `exec`, `wait`, `exit`, `kill`, `getpid`, `getppid`, `yield` |
| Info | `get_tasks`, `get_meminfo`, `devlist` |

### Filesystem

- VFS: Abstraction layer (open/read/write/close/lookup ops)
- FAT32: Rootfs trên SD card (subdir read, unlink, rename)
- devfs: `/dev/tty`, `/dev/null`
- procfs: `/proc/meminfo`, `/proc/version`, `/proc/mounts`, `/proc/<pid>/status`
- Block layer + buffer cache LRU 64×512 B write-back

## Tài Liệu

Tài liệu kỹ thuật chi tiết trong `Documentation/`:

1. `01-boot-and-bringup.md` - Boot sequence
2. `02-kernel-initialization.md` - Kernel startup
3. `03-memory-and-mmu.md` - Memory management
4. `04-interrupt-and-exception.md` - Exception handling
5. `05-task-and-scheduler.md` - Task scheduling
6. `06-syscall-mechanism.md` - Syscall interface
7. `08-userspace-application.md` - User applications
8. `99-system-overview.md` - Big picture

## Development

### Thêm File Vào Rootfs

1. Copy file vào FAT32 partition đã mount (`/media/$USER/NOTHAN/...`)
2. `sync` + `umount` trước khi rút SD

### Tạo User Application Mới

1. Tạo app directory `userspace/apps/<name>/` với `<name>.c`
2. `#include <stdio.h>`/`<unistd.h>`/... (nothanlibc headers)
3. `main(int argc, char **argv)` nhận argv từ shell
4. Build: `make -C userspace` — ELF tự động xuất hiện ở `userspace/build/apps/<name>/<name>.elf`
5. Deploy: `sudo ./scripts/deploy_and_flash.sh /dev/sdX` — copy ELF vào `/bin/<name>` trên FAT32
6. Trên BBB: `$ /bin/<name>` hoặc `$ <name>` (shell auto-prepend `/bin/`)

## Debugging

**Serial Console**:
```bash
screen /dev/ttyUSB0 115200
# hoặc
minicom -D /dev/ttyUSB0 -b 115200
```

**Common Issues**:

- Boot hang trước `[TIMER]`: UART connection lỗi, MLO sai sector trên SD
- Data Abort / Prefetch Abort: exception handler dump r0–r12, SPSR, DFSR/DFAR để trace
- User page fault: task bị SIGSEGV, kernel sống; xem `[EXC]` log để debug
- No shell prompt: init payload load OK chưa (xem `[BOOT] Loading User App Payload`), `/bin/sh` có trên FAT32 chưa

**Debug Output**: `uart_printf` là công cụ chính. Trace macros trong `kernel/include/trace.h` có thể bật/tắt từng subsystem.

## Testing

Selftest chạy tự động mỗi lần boot — `[TEST]` prefix. Test harness ở `kernel/src/kernel/test/selftest.c` — hiện có 2 integration test (bcache hit-rate, procfs content). Thêm test bằng cách append vào `tests[]` array, panic on fail.

## Nguyên Tắc Thiết Kế

1. **100% hand-written** — không copy/fork/port từ Linux/musl/BusyBox/lwIP. Mọi dòng code là của NothanOS.
2. **Linux-inspired API shape** — `task_struct`, `platform_driver`, `spinlock_t`, `wait_event`, `kmalloc(GFP_KERNEL)` — dễ đọc cho người biết Linux nhưng không dính dáng upstream.
3. **Userspace-driven kernel** — mỗi feature kernel phải có consumer trong userspace/demo. Không consumer → defer.
4. **Correctness over performance** — flush toàn bộ TLB, no nested interrupts, `-O2` không aggressive.
5. **Explicit over implicit** — explicit MMU setup, explicit stack reload, explicit pointer validation (`copy_from_user`).
6. **Real hardware, no emulation** — mọi DoD pass trên BeagleBone Black thật.

## Giới Hạn (MVP scope)

- Single-core only (ARMv7-A Cortex-A8, chưa test SMP)
- MAX_TASKS = 5 (idle + init + shell + 2 dynamic slot)
- Single-user (no setuid/getuid), no cgroup/namespace
- No pipe syscall `|` (defer v1.1)
- No TCP (UDP/ICMP/ARP only khi P7 xong)
- No USB host, no wifi/BT, no GPU
- No secure boot, no signal handler userspace (chỉ SIGKILL/SIGSEGV)
- Không có editor on device — edit config trên host, flash lại
- Signal set chỉ gồm SIGKILL + SIGSEGV, không SIGTERM/SIGCHLD

Các giới hạn này giữ codebase đơn giản và tập trung vào core OS concepts.
