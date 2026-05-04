# CLAUDE.md — VinixOS

**Trước mỗi task hardware/driver**: đọc [reference/index.md](reference/index.md).

---

## 1. Hard Rules

### Toolchain

- KHÔNG mix `arm-none-eabi` và `arm-linux-gnueabihf`
- Thứ tự build BẮT BUỘC: `userspace` trước, `kernel` sau

### Hardware

- Register I/O: `mmio_read32(addr)` / `mmio_write32(addr, val)` — không raw pointer cast
- Address + bit field: xác minh từ AM335x TRM, không đoán
- Driver không hardcode base address — lấy từ `platform_get_resource()` → `board-bbb.c`

### Driver init

- Mọi HW init trong `probe()`. KHÔNG có public `xxx_init()` gọi từ `main.c`.

### Linux as Reference

Tuân thủ kiến trúc, naming, pattern của Linux — nhưng **từng dòng code tự viết, tự chịu trách nhiệm**, không copy.

- **Được**: học pattern (VFS ops table, platform probe, wait_queue, slab), dùng Linux naming (`task_struct`, `spinlock_t`, `kmalloc`, `pr_info`, ...)
- **Cấm**: copy, fork, port, paraphrase từ Linux, musl, glibc, BusyBox, lwIP, FatFs, xv6, Zephyr, TCC hoặc bất kỳ upstream nào; paste code từ internet rồi sửa tên biến
- **Workflow**: đọc reference → hiểu sequence → đóng file → viết từ đầu. Mở lại ≥ 2 lần cho 1 function → rewrite từ memory

---

## 2. Code Generation

**Không đủ thông tin → DỪNG, hỏi. Không bịa.**

Luôn hỏi trước nếu: register address / IRQ / init sequence chưa verify TRM | file chưa đọc | behavior ambiguous.

### Format khi hỏi

```
Để viết [function/driver], tôi cần:
- [thông tin A] → [nguồn: TRM Ch.XX / file path]
Bạn chỉ tôi đọc file nào, hoặc cung cấp trực tiếp?
```

### Cấm gen placeholder

`0xDEADBEEF`, magic số tự đặt, register address từ internet chưa verify TRM, `/* TODO */`.

---

## 3. Definition of Done

Không tuyên bố "works" chỉ dựa compile. Sau mỗi feature:

> Build sạch, cần verify trên hardware.

---

## 4. Debug

Không JTAG. **UART log là công cụ duy nhất.**

### Cách debug bằng log

Đặt checkpoint print trước và sau mỗi thao tác nguy hiểm:

```c
pr_info("[MODULE] before hw reset — ctrl = 0x%08x\n", ctrl);
mmio_write32(base + CTRL, RESET_VAL);
pr_info("[MODULE] after hw reset  — ctrl = 0x%08x\n", mmio_read32(base + CTRL));
```

Bắt buộc **readback register sau write** khi bring-up. Nếu readback không khớp → report.

### Khi gặp bug

Yêu cầu từ user **trước khi phân tích**:

1. Toàn bộ UART log từ đầu boot
2. Dòng log cuối cùng trước hang/crash
3. Loại exception (Data Abort, Prefetch Abort, Undefined)
4. DFAR, DFSR, PC nếu là abort

Không đoán nguyên nhân khi chưa có UART log.

---

## 5. References

| Cần | File |
|-----|------|
| Driver development (quy trình, template, initcall levels) | [driver-development-guide.md](reference/driver-development-guide.md) |
| Coding style (naming, file layout, logging) | [coding-style.md](reference/coding-style.md) |
| Comment rules | [comment-style.md](reference/comment-style.md) |
| Commit rules | [commit-style.md](reference/commit-style.md) |
| Project context (components, status, WIP) | [project_context.md](reference/project_context.md) |
