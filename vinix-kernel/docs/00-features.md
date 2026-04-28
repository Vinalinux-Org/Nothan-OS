# VinixOS — Tính năng

> Bare-metal OS, monolithic kernel, single-core ARMv7-A, 100% tự viết bằng tay.
> Target hardware: BeagleBone Black (SoC AM3358, Cortex-A8).

---

## 1. Định danh hệ thống

| Thuộc tính | Giá trị |
| --- | --- |
| Loại | Bare-metal OS, monolithic kernel |
| Kiến trúc CPU | ARMv7-A (Cortex-A8), single-core |
| Primary target | BeagleBone Black (AM3358, 512 MB DDR3) |
| Boot time | < 5 giây từ power-on đến shell prompt |
| RAM footprint | < 32 MB kernel, < 50 MB toàn hệ thống idle |
| Kernel binary | ~66 KB |
| Toolchain | `arm-none-eabi-gcc` bare-metal full C |

---

## 2. Kiến trúc 4-layer HAL

```text
kernel/                — generic C: mm, sched, vfs, proc, ipc
arch/arm/              — ARMv7 CPU: MMU asm, context switch, exception vector
arch/arm/mach-omap2/   — AM3358 SoC + BBB board: memory map, clocks, IRQ, device table
drivers/               — driver impls (uart, mmc, lcdc, tda19988, intc, timer, i2c, …)
```

**Hệ quả portability:** port sang SoC ARMv7 khác = viết `arch/arm/mach-<new>/` + driver mới. `kernel/` không đổi 1 dòng.

---

## 3. Boot + Bring-up

- **MLO bootloader** tự viết
  - RBL stage-1 load MLO từ SD MMC0 (FAT32 partition)
  - MLO setup clock PLL, DDR3 EMIF, load kernel image vào `0x80000000`
- **Kernel boot**
  - Phase A: identity-map MMU, jump lên high VA
  - Phase B: switch TTBR1, remap kernel vào `0xC0000000`, peripheral vào device section
  - Init vector table, IRQ/SVC/Abort stacks per-mode
  - `platform_init()` enable clock tree, populate device table
  - `driver_init_all()` 3 level (EARLY → NORMAL → LATE), probe theo platform bus
  - Spawn init (PID 1) → exec `/sbin/init` → fork shell

---

## 4. Memory Management

| Subsystem | Chi tiết |
| --- | --- |
| MMU | ARMv7 short-descriptor, 2-level (L1 1 MB section + L2 4 KB page) |
| Kernel VA | `0xC0000000` cao, kernel heap + static |
| User VA | User text + stack + heap grow qua `sbrk` |
| Per-process page table | L1 pgd riêng cho mỗi `task_struct`, switch TTBR0 khi context switch |
| Page allocator | Bitmap page frame allocator |
| Slab cache | Fixed size-class, `kmem_cache_create/alloc/free` |
| Kernel heap | `kmalloc(size, GFP_KERNEL)` / `kfree` |
| VMA | `struct vm_area_struct` + `struct mm_struct` per-process, fork full page copy |
| Page fault | User fault → kill task, kernel sống; kernel fault → PANIC dump |

---

## 5. Scheduler + Concurrency

- Preemptive round-robin, single priority, tick 10 ms (DMTimer2)
- MAX_TASKS = 5
- Task states: `TASK_STATE_READY`, `TASK_STATE_RUNNING`, `TASK_STATE_BLOCKED`, `TASK_STATE_ZOMBIE`
- Context switch: full register save + TTBR0 switch + TLB flush tiến trình

**Concurrency primitives:**

| Primitive | API |
| --- | --- |
| Spinlock (LDREX/STREX) | `spin_lock/unlock`, `spin_lock_irqsave/unlock_irqrestore` |
| Atomic | `atomic_read/set`, `atomic_add_return`, `atomic_cmpxchg`, `atomic_inc/dec` |
| Memory barrier | `smp_mb()`, `smp_rmb()`, `smp_wmb()`, `barrier()`, `dmb/dsb/isb` |
| Wait queue | `wait_queue_head_t`, `DECLARE_WAIT_QUEUE_HEAD`, `wait_event(wq, cond)`, `wake_up(wq)` |
| Sleep | `msleep(ms)` qua jiffies tick (10 ms granularity), `sleep_tick()` wake trong IRQ |

---

## 6. Process Model

**Syscall core:**

```c
int  fork(void);
int  execve(const char *path, char *const argv[], char *const envp[]);
void _exit(int status);
int  wait(int *status);
int  getpid(void);
int  getppid(void);
int  kill(pid_t pid, int sig);
```

**Đặc trưng:**

- `struct task_struct` per-task với `pid`, `ppid`, `exit_status`, `mm`, `files[]`
- PID = slot index
- fd table per-process trong `current->files[]`
- argv passthrough end-to-end qua exec
- ELF32 ARM loader (validate magic, load PT_LOAD segments)
- Crash isolation: SIGSEGV task → bị kill, shell sống tiếp

**Signal:**

- `SIGKILL` (9) — qua `kill(pid, SIGKILL)`
- `SIGSEGV` (11) — auto từ page fault user-mode

---

## 7. IPC + Device I/O

| Tính năng | Chi tiết |
| --- | --- |
| Shell redirect `>`, `<`, `>>` | `open` + `dup2` trước exec |
| Background job `&` | Fork không wait |
| `/dev/tty` | Char device bound UART0, init mở 3 lần cho fd 0/1/2 |
| `/dev/null` | Char device, write discard, read trả 0 |
| devfs | Mount `/dev`, dispatch char_device ops |
| Char device framework | `struct char_device { name, read, write }` |

---

## 8. Filesystem

### VFS layer (Linux-style)

- `struct file`, `struct inode`, `struct dentry`, `struct super_block`
- `struct file_operations`, `struct inode_operations`, `struct super_operations`
- Multiple mount slot, longest-prefix resolution
- FD table per-process

### FAT32 driver

- Subdirectory traversal (path `/bin/ls` → root lookup "bin" → cluster chain → lookup "ls")
- 8.3 filename
- Read + write + unlink + rename
- Buffer cache LRU, dirty + write-back + periodic sync

### Block layer

- `struct block_device { read, write, sector_size, total_sectors }`
- MMC0 register `blkdev_mmc0`, polled I/O 512 B sectors

### procfs (virtual FS)

- `/proc/meminfo`
- `/proc/version`
- `/proc/mounts`
- `/proc/<pid>/status`

### Rootfs layout (FHS)

```text
/bin/        ls cat echo ps kill pwd free uname rm mv sh hello
/sbin/       init
/etc/        inittab, motd, hostname
/dev/        tty, null           (devfs)
/proc/       meminfo, version, mounts, <pid>/status  (procfs)
/tmp/        writable scratch (FAT32)
/home/       user data
```

---

## 9. Driver Model (Linux platform_driver)

Pattern y hệt `drivers/base/platform.c` của Linux:

```c
struct platform_device { const char *name; uint32_t base; int irq; const char *clk_id; ... };
struct platform_driver { struct driver drv; int (*probe)(struct platform_device *); ... };

platform_driver_register(&omap_uart_driver);
struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
int irq = platform_get_irq(pdev, 0);
```

**Driver đã port sang `platform_driver` model:**

| Driver | Peripheral | Chức năng |
| --- | --- | --- |
| `uart` | UART0 @ `0x44E09000` | IRQ RX ring buffer, polled TX, 115200 8N1 |
| `intc` | INTC @ `0x48200000` | 128 IRQ table, priority, per-IRQ enable/disable |
| `timer` | Timer2 @ `0x48040000` | 10 ms tick cho scheduler |
| `mmc` | MMC0 @ `0x48060000` | 512 B sector polled, SD init |

**Driver hardcoded init (port dần theo cùng pattern):**

| Driver | Chức năng |
| --- | --- |
| `lcdc` | LCDC controller, RGB565 raster |
| `tda19988` | HDMI transmitter qua I2C0 |
| `fb` | Framebuffer text console |
| `i2c` | I2C0, polled master |
| `watchdog` | WDT1 disable (BBB boot requirement) |
| `mbr` | MBR parse cho FAT32 partition |

**Device enumeration qua `arch/arm/mach-omap2/board-bbb.c`:** address + IRQ + clock id đi từ table → `probe()` qua `platform_get_resource`.

---

## 10. Syscall ABI

**22 syscall, SVC số = syscall number:**

| Nhóm | Syscall |
| --- | --- |
| I/O + FS | `read`, `write`, `open`, `close`, `read_file`, `write_file`, `listdir`, `unlink`, `rename` |
| Process | `fork`, `exec`, `exit`, `wait`, `yield`, `getpid`, `getppid`, `kill` |
| FD ops | `dup`, `dup2` |
| Introspection | `get_tasks`, `get_meminfo`, `devlist` |

**Convention:**

- Arguments r0–r3 (AAPCS), syscall number trong r7
- Return trong r0, negative errno on failure
- User pointer validate qua `access_ok` + `copy_from_user` / `copy_to_user`

---

## 11. HDMI Display

- LCDC raster mode, RGB565 framebuffer @ `0x80800000` (non-cacheable)
- TDA19988 HDMI encoder qua I2C0
- Kernel text console
- Boot log + shell prompt hiển thị song song trên UART và HDMI

---

## 12. Exception + Fault Handling

Exception vector đầy đủ 7 entry (Reset / Undef / SVC / Prefetch / Data / IRQ / FIQ).

Mỗi abort:

- Dump r0–r12, lr, sp, spsr, pc
- Data Abort: decode DFSR + DFAR (translation / permission / domain)
- Banner `[PANIC]` hoặc `[SIGSEGV]` tùy mode
- User-mode fault → kill task + wake parent; kernel-mode fault → PANIC halt

**Debug tools trong kernel:**

- `BUG_ON(cond)` — assert + PANIC dump
- `WARN_ON(cond)` — warn + continue
- `pr_info/pr_err/pr_warn/pr_debug` với `[MODULE]` prefix

---

## 13. vinixlibc — POSIX subset (hand-written)

| Module | Function |
| --- | --- |
| `string.h` | `strlen`, `strcmp/n`, `strcpy/n`, `strcat`, `strchr/rchr`, `strstr`, `memcpy/set/cmp/move` |
| `stdio.h` printf | `printf`, `vprintf`, `snprintf`, `sprintf`, `vsnprintf`, `vsprintf`, `fprintf`, `vfprintf` |
| `stdio.h` FILE | `FILE`, `stdin/stdout/stderr`, `fopen/fclose`, `fread/fwrite`, `fgetc/fputc`, `getchar/putchar`, `fgets/fputs/puts`, `fflush`, `feof`, `ferror` |
| `stdlib.h` | `malloc/free/calloc/realloc` (K&R free-list), `exit`, `atoi/atol`, `strtol`, `itoa`, `abs` |
| `unistd.h` | Syscall wrapper: `read/write/open/close`, `dup/dup2`, `fork/execve/_exit`, `wait`, `getpid/getppid`, `kill`, `unlink/rename` |
| `fcntl.h` | `open` flags (`O_RDONLY`, `O_WRONLY`, `O_CREAT`, …) |
| `sys/stat.h` | `struct stat`, `stat`, `fstat` |
| `sys/wait.h` | `WIFEXITED/WEXITSTATUS/WIFSIGNALED/WTERMSIG` |
| `signal.h` | `raise` |
| `errno.h` | Global `errno` |
| `ctype.h` | `isdigit/isalpha/isalnum/isspace/isupper/islower/isprint`, `toupper/tolower` |

---

## 14. Userspace — init + shell + utilities

Mỗi utility 1 ELF riêng (không symlink-to-single-binary).

| Binary | Path | Vai trò | Đặc điểm |
| --- | --- | --- | --- |
| `init` | `/sbin/init` | PID 1, spawn + reap | Đọc `/etc/inittab`, fork + exec `/bin/sh` trên `/dev/tty`, wait + respawn, reap orphan zombie |
| `sh` | `/bin/sh` | Shell tương tác | Parser + quoting + comment `#`, redirect `>` `<` `>>` qua open + dup2, background `&` qua fork-không-wait, built-in `cd/exit/help`, auto `/bin/` prefix |
| `ls` | `/bin/ls` | List directory | 1 cột, tên + size |
| `cat` | `/bin/cat` | Dump file | Ghép nhiều file ra stdout |
| `echo` | `/bin/echo` | Print arg | Write args ra stdout |
| `ps` | `/bin/ps` | Process list | Format bảng PID/NAME/STATE qua `sys_get_tasks` |
| `kill` | `/bin/kill` | Gửi SIGKILL | `kill(pid, SIGKILL)` syscall |
| `pwd` | `/bin/pwd` | Current dir | `getcwd` wrapper |
| `free` | `/bin/free` | RAM usage | Stream `/proc/meminfo` qua `open` + `read` |
| `uname` | `/bin/uname` | System info | Via syscall |
| `rm` | `/bin/rm` | Xoá file | `unlink` |
| `mv` | `/bin/mv` | Rename | `rename` syscall |
| `hello` | `/bin/hello` | Smoke test | vinixlibc printf + malloc sanity |

**Tạo/edit file trên device:**

```text
echo "line 1" > /tmp/a          # tạo
echo "line 2" >> /tmp/a         # append
cat /tmp/a                      # verify
```

---

## 15. VinCC — Cross-compiler end-user

Python 3, chạy trên host (laptop), không self-hosting trên device. Pipeline hoàn chỉnh: `Source .c → Preprocessor → Lexer → Parser → Semantic → IR → CodeGen → Assembler → Linker → ELF32 ARM`. Tuân thủ AAPCS calling convention. Runtime library tích hợp (reflibc).

**Workflow:**

```bash
# Trên laptop
vincc hello.c -o hello
cp hello /media/sd/bin/hello

# BBB reboot → shell
# /bin/hello → output
```

**Subset C hỗ trợ:**

- Kiểu dữ liệu: `int`, `char`, pointer, array
- Cấu trúc điều khiển: `if/else`, `while`, `for`, `return`, `break`, `continue`
- Hàm: định nghĩa, gọi hàm, đệ quy (tối đa 4 tham số qua r0–r3)
- Toán tử: số học, so sánh, logic, bitwise, gán

VinCC chỉ compile end-user C program. Kernel + vinixlibc + system tools dùng `arm-none-eabi-gcc`.

---

## 16. Observability

| Tool | Chức năng |
| --- | --- |
| `ps` | Liệt kê task qua `sys_get_tasks` — PID/NAME/STATE |
| `free` | Total/used/free memory từ `/proc/meminfo` |
| `cat /proc/version` | Kernel version + build timestamp |
| `cat /proc/mounts` | Mount table |
| `cat /proc/<pid>/status` | Task metadata |
| UART log | `pr_info/err/warn/debug` với `[MODULE]` prefix, 115200 8N1 |
| HDMI console | Mirror boot log + shell I/O |

**Self-test harness:**

- `page_alloc_selftest()` — trong `mm/page_alloc.c`, chạy inline trong init
- `slab_selftest()` — trong `mm/slab.c`
- `vmm_selftest()` — trong `mm/vmm.c`
- `sync_selftest()` — trong `sync/sync_selftest.c`
- Integration harness [test/selftest.c](../vinix-kernel/lib/test/selftest.c): `bcache_hit` + `procfs_read`
- Fail → `PANIC`

---
