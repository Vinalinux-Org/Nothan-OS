# VinixOS — Project Context

File này cung cấp context nhanh cho AI khi bắt đầu một task mới, tránh phải đọc toàn bộ codebase.

---

## Tổng quan

VinixOS là một bare-metal OS tự phát triển từ đầu, chạy trên **BeagleBone Black (BBB)**.

- **SoC:** Texas Instruments AM3358
- **CPU:** ARMv7-A Cortex-A8
- **Trạng thái:** Boot được trên board thật, shell tương tác qua UART, HDMI display hoạt động

---

## Cấu trúc project

```
vinix-kernel/
├── bootloader/     ← MLO: clock, DDR3, load kernel từ SD card
├── kernel/         ← Kernel chính
│   ├── src/
│   │   ├── arch/arm/   ← entry.S, vectors, context_switch, MMU
│   │   ├── drivers/    ← uart, timer, intc, i2c, lcdc, tda19988, fb
│   │   └── kernel/     ← main, scheduler, fs (vfs + ramfs), mmu, syscall
│   └── include/        ← public headers
├── userspace/      ← Shell app chạy ở User Mode 0x40000000
└── docs/           ← Tài liệu kỹ thuật từng subsystem

CrossCompiler/      ← Phase 2: Python cross compiler → ARMv7-A (HOÀN THÀNH)
```

---

## Những gì đã implement xong

| Subsystem | Ghi chú |
|-----------|---------|
| Bootloader (MLO) | Clock, DDR3, UART, load kernel từ SD |
| Boot sequence | ROM → MLO → entry.S → kernel_main |
| MMU | 3G/1G split, VA 0xC0000000 kernel / 0x40000000 user |
| Exception handling | 7 exception types, INTC, IRQ |
| Scheduler | Preemptive round-robin, DMTimer2 10ms tick |
| Context switch | Save/restore registers, SPSR |
| Syscall (SVC) | 11 syscalls: write, read, open, close, exit, yield, ... |
| UART driver | TX polling, RX interrupt-driven, ring buffer |
| Timer driver | DMTimer2, early init + scheduler tick |
| INTC driver | AM335x interrupt controller |
| I2C driver | I2C0 cho TDA19988 |
| LCDC driver | 800×600 RGB565 |
| HDMI (TDA19988) | HDMI output qua I2C |
| Framebuffer | fb_init, boot screen |
| VFS layer | vfs_operations function pointers, FD table |
| RAMFS | Read-only, files embed vào kernel image lúc build |
| Shell (userspace) | ls, cat, ps, meminfo, help — chạy ở User Mode |
| MMC driver (bootloader) | Đọc sector từ SD card — chỉ dùng trong bootloader |

---

## Đang phát triển

### SD card driver trong kernel
- Port `bootloader/src/mmc.c` vào `vinix-kernel/drivers/mmc/host/omap_hsmmc.c`
- Thêm write support (`mmc_write_sectors`)
- Tài liệu: `docs_trainingAI/drivers/sdcard/index.md`

### FAT32 filesystem driver
- Thay thế RAMFS bằng FAT32 từ SD card làm root `/`
- Stack: MMC driver → block device abstraction → FAT32 driver → `vfs_mount("/", &fat32_ops)`
- Phase 1: read-only (mount, ls, cat)
- Phase 2: write support (lưu data người dùng)
- Tài liệu: `docs_trainingAI/drivers/fat32/index.md`


