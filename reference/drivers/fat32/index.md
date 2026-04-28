# FAT32 Driver

Tài liệu tham khảo cho việc phát triển FAT32 filesystem driver trong kernel VinixOS.

## Tài liệu liên quan (trong reference)

| File | Mô tả |
|------|-------|
| `drivers/fat32/fatgen103.md` | Microsoft FAT32 specification — BPB layout, FAT entry, directory entry, cluster chain |

## Source tham khảo

| Source | Ghi chú |
|--------|---------|
| `drivers/fat32/source/ff.c` + `ff.h` | FatFs (ChaN) — embedded FAT, không có OS/malloc, gần VinixOS nhất |
| `drivers/fat32/source/diskio.h` + `diskio.c` | Block device abstraction interface — pattern tách filesystem khỏi hardware |
| `drivers/fat32/source/fatent.c` | Linux kernel — cluster chain traversal logic |
| `drivers/fat32/source/dir.c` | Linux kernel — directory entry traversal |
| `drivers/fat32/source/file.c` | Linux kernel — file read logic |
