# VinixOS — Comment Style

## 1. Default: KHÔNG viết comment

> Test: xóa comment đi — code còn đọc được không? Còn → comment thừa, xóa.

Code well-named, function ngắn, struct field tự giải thích thì không cần comment. AI có xu hướng over-comment — phải kiềm.

---

## 2. 5 trường hợp ĐƯỢC viết

### (1) Hardware constraint không hiển nhiên

```c
/* Wait for TX shift register idle before clock reconfiguration;
 * touching PRCM while the shift register is transmitting drops bytes. */
while (!(mmio_read32(base + UART_LSR) & LSR_TEMT));
```

### (2) Magic value — giải thích lý do, không giải thích value

```c
/* FB at 0x80800000 = kernel(5MB) + user(1MB) + 2MB margin */
#define FRAMEBUFFER_BASE 0x80800000
```

```c
/* Sai — giải thích cái đã thấy */
#define FRAMEBUFFER_BASE 0x80800000  /* framebuffer base address */
```

### (3) Invariant tinh tế

```c
/* RX ring buffer is shared with IRQ handler — must hold uart_lock
 * before any read/write to head/tail indices. */
static char rx_ring[RX_RING_SIZE];
```

### (4) `CRITICAL:` cho interrupt-mode hoặc context-sensitive code

```c
/* CRITICAL: called from IRQ context — no kmalloc, no mutex, no printk
 * with format string > 64 chars (kernel log buffer is per-CPU). */
static void uart_rx_irq_handler(void *data) { ... }
```

### (5) Cross-reference TRM cho hardcoded sequence

```c
/* AM335x TRM 19.5.1.1.2 — software reset sequence:
 * 1. Set SYSCONFIG.SOFTRESET
 * 2. Poll SYSSTATUS.RESETDONE
 * 3. Configure FCR before LCR */
mmio_write32(base + UART_SYSC, 0x02);
while (!(mmio_read32(base + UART_SYSS) & 0x01));
```

---

## 3. KHÔNG viết — anti-patterns

```c
/* SAI — banner dài dòng */
/*
 * ============================================================
 * Function: uart_putc
 * Description: Sends a character to UART
 * Parameters:
 *   c - the character
 * Returns: void
 * Author: ...
 * Date: ...
 * History:
 *   2026-04-01 — initial version
 * ============================================================
 */
void uart_putc(char c) { ... }
```

```c
/* SAI — giải thích WHAT */
i++;  /* increment i */
mmio_write32(base + UART_FCR, 0x07);  /* write 0x07 to FCR */
```

```c
/* SAI — phase marker */
/* P1 init */
uart_init();
/* TODO P2: add interrupt support */
```

```c
/* SAI — PR / issue reference (rot quickly) */
/* Fix for issue #42 — buffer overflow */
/* See PR #128 */
/* Used by sched.c and timer.c */
```

```c
/* SAI — "imported from" / "based on" */
/* Imported from Linux v6.5 drivers/tty/serial/8250.c */
/* Based on FreeBSD ata driver */
```

→ **Vi phạm § "Linux as Reference, Not Source".** Xóa file, viết lại.

---

## 4. File header — block 4-6 dòng, không separator

```c
/*
 * drivers/tty/serial/omap_serial.c — AM335x UART0 driver
 *
 * TX polling, RX interrupt-driven via serial_core ring buffer.
 */
```

Cấu trúc:
- **Dòng 1**: open comment `/*`
- **Dòng 2**: `path/file.c — One-line module description` (path tương đối từ project root, dấu em-dash, mô tả ngắn)
- **Dòng 3**: blank `*`
- **Dòng 4-5**: 1-2 dòng mô tả WHAT module provide hoặc constraint chính (optional nếu module name đã đủ)
- **Dòng cuối**: close `*/`

KHÔNG: SPDX, Copyright, Author, Date, History, `===`/`***` separator. Git lo blame/log/license. Project chưa có license header, không bịa ra.

---

## 5. Struct public trong header — kernel-doc `/** */`

Cho VSCode hover doc hoạt động + cho ai đọc include thấy contract:

```c
/**
 * struct uart_port - serial port runtime state
 * @base:  MMIO base address (verified by platform_get_resource)
 * @irq:   hardware IRQ number
 * @ops:   driver callback table
 * @lock:  protects ring buffer head/tail
 */
struct uart_port {
    uint32_t            base;
    int                 irq;
    const struct uart_ops *ops;
    spinlock_t          lock;
};
```

KHÔNG dùng kernel-doc cho struct internal trong .c file — chỉ public struct trong .h.

---

## 6. Javadoc-style — chỉ khi thực sự cần

Viết khi và **chỉ khi**:
- Precondition không hiển nhiên (caller phải hold lock, IRQ disabled, ...)
- Side effect ẩn (modify global state, free memory caller-passed)
- Return value có edge case không thể infer từ type

```c
/**
 * uart_serial_rx_push - push byte vào RX ring buffer
 * @c: byte received from HW
 *
 * Callable từ IRQ context. Drop silently nếu ring full —
 * caller không thể biết drop xảy ra.
 */
static void uart_serial_rx_push(char c) { ... }
```

Function ngắn 5 dòng, parameter tên rõ ràng, return type explicit → **không cần Javadoc**.

---

## Phụ lục — Comment Smell

| Smell | Fix |
| --- | --- |
| `/* TODO: ... */` không có context | Xóa hoặc thay bằng `BUG_ON`/issue tracker |
| `/* Used by xxx.c */` | Xóa — git grep tự tìm |
| `/* Imported from Linux ... */` | **Vi phạm**, viết lại file từ đầu |
| `/* Phase 1: ... */` | Xóa khi phase đóng |
| Comment trùng tên function | Xóa |



