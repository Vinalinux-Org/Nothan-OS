# VinixOS — Vietnamese Operating System

**Bare-metal operating system + cross-compiler cho ARMv7-A, chạy trên phần cứng thật (BeagleBone Black).**

Được phát triển bởi **Vinalinux** như một reference platform để học và hiểu toàn bộ software stack từ power-on đến running compiled programs — không dùng Linux, không dùng existing OS, không có emulation.

---

## Screenshots

### Boot Log

![Boot Log](VinixOS/docs/images/boot-log.png)

### Splash Screen

![Splash Screen](VinixOS/docs/images/logo.png)

### Home Screen

![Home Screen](VinixOS/docs/images/home.png)

---

## Tổng Quan

Project gồm 2 components chính:

| Component | Mô Tả | Ngôn Ngữ |
|-----------|-------|---------|
| **VinixOS** | Bare-metal OS: bootloader, kernel, userspace shell, HDMI display | C, ARM Assembly |
| **VinCC** | Cross-compiler: Subset C → ARMv7-A ELF binary | Python |

---

## VinixOS — Hệ Điều Hành

### Bootloader (MLO)

- Khởi tạo hardware: clocks, watchdog, DDR3 memory, UART
- Load kernel từ SD card (sector 2048) vào DDR3 `0x80000000`
- Jump sang kernel với ARM boot parameters (r1 = MACH\_TYPE)

### Kernel

| Subsystem | Chi Tiết |
|-----------|---------|
| **MMU** | Virtual memory 3G/1G split: User `0x40000000`, Kernel `0xC0000000` |
| **Exception Handling** | 7 exception types, INTC 128 IRQ sources, vector table tại VA |
| **Scheduler** | Preemptive round-robin, 10ms tick (DMTimer2), context switch ARM assembly |
| **System Calls** | 11 syscalls qua SVC instruction, AAPCS-compliant, pointer validation |
| **VFS + RAMFS** | Virtual filesystem abstraction, in-memory read-only FS, build-time embedding |
| **HDMI Display** | 800×600 RGB565, TDA19988 HDMI transmitter qua I2C, framebuffer API |

### HDMI Graphics Stack

```
Framebuffer API (fb.c)  ←  800×600 RGB565, font 8×16, fb_fillcircle
        ↓
LCDC Driver (AM335x)    ←  Raster mode, 40MHz pixel clock (DPLL)
        ↓
TDA19988 (I2C-controlled) ←  HDMI transmitter, TMDS output
        ↓
HDMI Output 800×600@60Hz
```

**Boot screen sequence:**
1. **Boot Log** — 16 hardware init steps với `[ OK ]` indicators, animation 300ms/dòng
2. **Splash** — "VINIX OS" scale-6, animated dots, ~4.5 giây
3. **Home Launcher** — status bar (clock, wifi, battery), app icon grid 4×2, page dots

### Userspace

- **Shell** — chạy ở User Mode (`CPSR = 0x10`), isolated hoàn toàn khỏi kernel
- **Syscall wrappers** — `write`, `read`, `yield`, `exit`, `open`, `read_file`, `close`, `listdir`
- **crt0.S** — setup stack, clear BSS, call `main()`, `sys_exit()`

---

## VinCC — Cross Compiler

Compiler viết bằng Python, compile **Subset C** → **ARMv7-A ELF32 binary** chạy trên VinixOS.

### Pipeline

```
Source (.c) → [Lexer] → [Parser] → [Semantic] → [IR Gen] → [Codegen] → [Assembler] → [Linker] → ELF
```

| Phase | Module | Output |
|-------|--------|--------|
| Lexer | `frontend.lexer` | Token stream |
| Parser | `frontend.parser` | AST |
| Semantic Analyzer | `frontend.semantic` | Annotated AST + Symbol Table |
| IR Generator | `middleend.ir` | Three-Address Code (3AC) |
| Code Generator | `backend.armv7a` | ARM Assembly (.s) |
| Assembler | wraps `arm-linux-gnueabihf-as` | Object file (.o) |
| Linker | wraps `arm-linux-gnueabihf-ld` | ELF32 binary |

### Subset C Support

| Category | Features |
|----------|---------|
| Types | `int` (32-bit), `char` (8-bit), `void`, pointers (`int*`, `char*`), 1D arrays |
| Control Flow | `if/else`, `while`, `for`, `return` |
| Functions | Definition, call, recursion (max 4 params via r0-r3) |
| Operators | Arithmetic, comparison, logical, bitwise, assignment |

---

## Hardware

| Component | Spec |
|-----------|------|
| Board | BeagleBone Black Rev.C |
| SoC | Texas Instruments AM335x |
| CPU | ARM Cortex-A8 @ 800 MHz (ARMv7-A) |
| RAM | 512 MB DDR3 @ 400 MHz |
| Storage | microSD card |
| Display | HDMI via TDA19988 (800×600@60Hz) |
| Console | UART0 @ 115200 8N1 |

---

## Yêu Cầu Hệ Thống

### Phần Cứng

- **BeagleBone Black** (~$60)
- **microSD card** ≥ 128 MB
- **USB-to-Serial adapter** (3.3V TTL) — GND, RX, TX — cho UART console
- **HDMI monitor** — để xem boot screen và home launcher
- **5V/2A power supply**

### Phần Mềm

**OS**: Ubuntu 22.04 LTS (khuyến nghị)

**Dependencies**:

```bash
sudo apt-get update
sudo apt-get install -y \
    gcc-arm-none-eabi binutils-arm-none-eabi \
    binutils-arm-linux-gnueabihf \
    python3 python3-pip \
    build-essential make git \
    screen minicom
```

---

## Cài Đặt và Build

### Bước 1: Clone

```bash
git clone git@github.com:Vinalinux-Org/Vinix-OS.git
cd Vinix-OS
```

### Bước 2: Build VinixOS (bootloader + kernel + userspace)

```bash
make -C VinixOS
```

Output:
- `VinixOS/bootloader/build/MLO` — first-stage bootloader
- `VinixOS/kernel/build/kernel.bin` — kernel với embedded shell

### Bước 3: Build VinCC Runtime

```bash
cd CrossCompiler
make runtime
```

### Bước 4: Compile Test Program

```bash
cd CrossCompiler
python3 -m toolchain.main -o test_hello tests/programs/test_hello.c
```

### Bước 5: Embed vào VinixOS

```bash
# Đặt binary vào initfs → tự động có trong RAMFS
cp test_hello VinixOS/initfs/

# Rebuild kernel
make -C VinixOS kernel
```

### Bước 6: Flash SD Card

```bash
# Tìm device SD card
lsblk   # trước khi cắm
lsblk   # sau khi cắm → device mới là SD card (vd: /dev/sdb)

# Flash
sudo bash scripts/flash_sdcard.sh /dev/sdX
```

> ⚠️ **Cảnh báo:** Lệnh này ghi đè toàn bộ SD card. Kiểm tra kỹ device trước khi chạy.

### Bước 7: Kết Nối Serial Console

```bash
screen /dev/ttyUSB0 115200
# hoặc
minicom -D /dev/ttyUSB0 -b 115200
```

Nếu cần thêm quyền:
```bash
sudo usermod -a -G dialout $USER
# Logout và login lại
```

### Bước 8: Boot BeagleBone Black

1. Cắm SD card vào BeagleBone Black
2. Giữ nút **BOOT** (gần SD card slot)
3. Cắm nguồn 5V
4. Thả nút BOOT sau 2–3 giây

Trên UART console:

```
========================================
VinixOS Bootloader
========================================
DDR:    Initializing 512MB DDR3... Done
MMC:    Loading kernel from SD card...
Boot:   Jumping to kernel @ 0x80000000
========================================

[BOOT] VinixOS: Interactive Shell
[BOOT] Loading User App Payload to 0x40000000
[BOOT] UART init complete. Starting HDMI boot screen...
[BOOT] Boot complete. Starting scheduler...

VinixOS Shell
Type 'help' for commands

$ _
```

Trên **HDMI monitor**: Boot log → Splash → Home launcher

### Bước 9: Chạy Program

```
$ ls
test_hello  hello.txt  info.txt

$ exec test_hello
Hello, VinixOS!

$ help
Available commands:
  help     - Show this help
  ls       - List files in /
  cat      - Display file content
  ps       - Show running tasks
  meminfo  - Show memory layout
  exec     - Execute a program
```

---

## End-to-End Workflow

```
1. Viết chương trình C (Subset C)
        ↓
2. Compile với VinCC
   python3 -m toolchain.main -o myapp myapp.c
        ↓
3. Nhúng vào VinixOS
   cp myapp VinixOS/initfs/
   make -C VinixOS kernel
        ↓
4. Flash SD card
   bash scripts/flash_sdcard.sh /dev/sdX
        ↓
5. Boot BeagleBone Black
   → UART console: tương tác với shell
   → HDMI: boot animations + home launcher
        ↓
6. Chạy trên hardware thật
   $ exec myapp
```

---

## Cấu Trúc Project

```
Vinix-OS/
├── VinixOS/
│   ├── bootloader/          ← MLO (SRAM @ 0x402F0400)
│   ├── kernel/
│   │   ├── src/
│   │   │   ├── arch/arm/    ← entry.S, MMU, context switch
│   │   │   ├── drivers/     ← UART, Timer, INTC, I2C, LCDC, TDA19988, fb
│   │   │   ├── kernel/      ← main, scheduler, syscalls, MMU, VFS
│   │   │   └── ui/          ← boot_screen.c (boot log, splash, home)
│   │   └── include/
│   ├── userspace/           ← shell app, crt0.S, syscall wrappers
│   ├── initfs/              ← files baked vào RAMFS lúc build
│   └── docs/                ← 9 tài liệu kỹ thuật
│
├── CrossCompiler/
│   ├── toolchain/
│   │   ├── frontend/        ← Lexer, Parser, Semantic Analyzer
│   │   ├── middleend/ir/    ← IR Generator (3AC)
│   │   ├── backend/armv7a/  ← Code Generator, Register Allocator
│   │   └── runtime/         ← crt0.S, syscalls.S, divmod.S, app.ld
│   └── docs/                ← 5 tài liệu kỹ thuật
│
├── scripts/
│   ├── setup-environment.sh
│   ├── flash_sdcard.sh
│   └── generate_ramfs_table.py
│
├── CLAUDE.md                ← Project guide cho AI-assisted development
├── Makefile
└── README.md
```

---

## Project Status

| Component | Trạng Thái | Tài Liệu |
|-----------|-----------|---------|
| Bootloader (MLO) | ✅ Hoàn thành | [01-boot-and-bringup.md](VinixOS/docs/01-boot-and-bringup.md) |
| Kernel Core | ✅ Hoàn thành | [02-kernel-initialization.md](VinixOS/docs/02-kernel-initialization.md) |
| Memory Management (MMU) | ✅ Hoàn thành | [03-memory-and-mmu.md](VinixOS/docs/03-memory-and-mmu.md) |
| Interrupt Handling | ✅ Hoàn thành | [04-interrupt-and-exception.md](VinixOS/docs/04-interrupt-and-exception.md) |
| Task Scheduler | ✅ Hoàn thành | [05-task-and-scheduler.md](VinixOS/docs/05-task-and-scheduler.md) |
| System Calls | ✅ Hoàn thành | [06-syscall-mechanism.md](VinixOS/docs/06-syscall-mechanism.md) |
| Filesystem (VFS + RAMFS) | ✅ Hoàn thành | [07-filesystem-vfs-ramfs.md](VinixOS/docs/07-filesystem-vfs-ramfs.md) |
| Userspace Shell | ✅ Hoàn thành | [08-userspace-application.md](VinixOS/docs/08-userspace-application.md) |
| HDMI Display (800×600) | ✅ Hoàn thành | [CLAUDE.md](CLAUDE.md) |
| Boot Screen (Log + Splash) | ✅ Hoàn thành | `kernel/src/ui/boot_screen.c` |
| Home Launcher (Icon Grid) | ✅ Hoàn thành | `kernel/src/ui/boot_screen.c` |
| Compiler Frontend | ✅ Hoàn thành | [architecture.md](CrossCompiler/docs/architecture.md) |
| Compiler IR | ✅ Hoàn thành | [ir_format.md](CrossCompiler/docs/ir_format.md) |
| Compiler Backend | ✅ Hoàn thành | [codegen_strategy.md](CrossCompiler/docs/codegen_strategy.md) |
| Runtime Library | ✅ Hoàn thành | [usage_guide.md](CrossCompiler/docs/usage_guide.md) |

---

## Tài Liệu

### VinixOS (`VinixOS/docs/`)

| File | Nội Dung |
|------|---------|
| [01-boot-and-bringup.md](VinixOS/docs/01-boot-and-bringup.md) | ROM → MLO → entry.S, MMU Phase A |
| [02-kernel-initialization.md](VinixOS/docs/02-kernel-initialization.md) | `kernel_main()` step-by-step |
| [03-memory-and-mmu.md](VinixOS/docs/03-memory-and-mmu.md) | Page table, address translation |
| [04-interrupt-and-exception.md](VinixOS/docs/04-interrupt-and-exception.md) | INTC, IRQ flow, exception handlers |
| [05-task-and-scheduler.md](VinixOS/docs/05-task-and-scheduler.md) | Context switch, round-robin |
| [06-syscall-mechanism.md](VinixOS/docs/06-syscall-mechanism.md) | SVC ABI, pointer validation |
| [07-filesystem-vfs-ramfs.md](VinixOS/docs/07-filesystem-vfs-ramfs.md) | VFS abstraction, RAMFS |
| [08-userspace-application.md](VinixOS/docs/08-userspace-application.md) | crt0.S, linker script, shell |
| [99-system-overview.md](VinixOS/docs/99-system-overview.md) | Big picture, flows, memory map |

### CrossCompiler (`CrossCompiler/docs/`)

| File | Nội Dung |
|------|---------|
| [architecture.md](CrossCompiler/docs/architecture.md) | 7-phase pipeline, module organization |
| [usage_guide.md](CrossCompiler/docs/usage_guide.md) | Install, options, examples |
| [subset_c_spec.md](CrossCompiler/docs/subset_c_spec.md) | Ngôn ngữ Subset C specification |
| [ir_format.md](CrossCompiler/docs/ir_format.md) | Three-Address Code IR format |
| [codegen_strategy.md](CrossCompiler/docs/codegen_strategy.md) | Register allocation, ARM mapping |

---

## Nguyên Tắc Thiết Kế

| Principle | Ví Dụ |
|-----------|-------|
| **Simplicity over features** | 1-level page table, round-robin scheduler, static task array |
| **Correctness over performance** | Flush toàn bộ TLB, no nested interrupts, -O2 chỉ |
| **Explicit over implicit** | Explicit MMU setup, explicit stack reload, explicit pointer validation |
| **Real hardware, no emulation** | Chạy trực tiếp trên BeagleBone Black |

---

## Troubleshooting

**Boot fails (không thấy output UART):**
- Verify SD card format đúng (FAT32 partition + MLO ở sector 0)
- Check serial connection: GND-GND, RX-TX, TX-RX, đúng 3.3V TTL
- Verify baudrate 115200 8N1

**Không có HDMI output:**
- HDMI cần pixel clock từ LCDC trước khi TDA19988 khởi động — kiểm tra thứ tự init
- Monitor phải support 800×600@60Hz

**Data Abort / MMU Fault:**
- Check page table configuration trong `mmu.c`
- Verify VBAR được update đúng sau `mmu_init()`

**Build fails:**
- Verify `arm-none-eabi-gcc` đã install: `arm-none-eabi-gcc --version`
- Build order: userspace trước kernel (`make -C VinixOS userspace` rồi `make -C VinixOS kernel`)

**VinCC compiler error:**
- Kiểm tra feature có trong Subset C: xem [subset_c_spec.md](CrossCompiler/docs/subset_c_spec.md)
- `++`/`--` không hỗ trợ — dùng `i = i + 1`

---

## FAQ

**Tại sao không dùng Linux hoặc RTOS có sẵn?**
Đây là educational project. OS có sẵn che giấu implementation details. VinixOS implement mọi thứ từ đầu để có thể hiểu từng layer.

**Tại sao BeagleBone Black?**
Giá ~$60, AM335x được document công khai, không cần proprietary tools, ARMv7-A real hardware.

**Tại sao ARMv7-A thay vì ARMv8/RISC-V?**
ARMv7-A đơn giản hơn (32-bit, 1 exception level) nhưng vẫn đại diện cho production embedded systems.

**VinCC có thể compile chương trình phức tạp không?**
VinCC hỗ trợ Subset C — đủ cho algorithms, data structures, và I/O qua syscalls. Không có `struct`, `malloc`, hay standard library.

---

## License

Developed by **Vinalinux** — Vietnamese Operating System Project.
