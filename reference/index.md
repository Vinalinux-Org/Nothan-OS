# reference — Index

Tài liệu kỹ thuật phục vụ AI-assisted development cho VinixOS trên BeagleBone Black.

## Folders

| Folder | Mô tả | Index |
|--------|-------|-------|
| `am335x/` | AM335x TRM — register map, peripheral config, clock/power | [index](am335x/index.md) |
| `arm-arch/` | ARM architecture — instruction set, exception handling, assembly | [index](arm-arch/index.md) |
| `hardware-beagleboneblack/` | BBB schematic, P8/P9 pinout | [index](hardware-beagleboneblack/index.md) |
| `drivers/` | Tài liệu theo từng driver (sdcard, fat32, eth, ...) | [sdcard](drivers/sdcard/index.md) · [fat32](drivers/fat32/index.md) · [eth](drivers/eth/index.md) |
| `software/` | Tài liệu cho pure-software subsystem (network stack, ...) | [network_stack](software/network_stack/index.md) |

## Project Context

Đọc [`project_context.md`](project_context.md) trước khi bắt đầu bất kỳ task nào — tóm tắt trạng thái project, conventions, và pointer đến tài liệu liên quan.

## Coding Standards

Deep-dive về style / naming / comment / commit — [`coding_standards.md`](coding_standards.md). [`CLAUDE.md`](../CLAUDE.md) là enforcer ngắn gọn (luôn được AI load); file này là rationale + ví dụ đúng/sai + edge case.

## Driver Development Guide

Hướng dẫn viết driver mới từ đầu đến cuối — [`driver-development-guide.md`](driver-development-guide.md). Standalone, self-contained: không cần biết project trước. Bao gồm kiến trúc platform bus, quy trình 5 bước, worked example (omap_serial), initcall decision tree, debug pattern, checklist.
