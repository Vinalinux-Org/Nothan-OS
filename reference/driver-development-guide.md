# VinixOS — Driver Development Guide

> File này dành cho developer mới hoặc AI cần viết driver cho VinixOS từ đầu.
> Đọc xong file này là đủ để bắt tay viết driver. Không cần đọc Linux source.
>
> Style/comment/commit rules → [`CLAUDE.md`](../CLAUDE.md) + [`reference/coding_standards.md`](coding_standards.md)
> Hardware addresses → [`reference/index.md`](index.md)

---

## 1. Kiến trúc — Driver Hoạt Động Như Thế Nào

```
boot
 │
 ├─ do_initcalls(1) ─── board-bbb.c:bbb_platform_init
 │                         └─ platform_device_register() cho mỗi HW entry
 │                              (omap-uart, omap-intc, omap-i2c, ...)
 │
 ├─ do_initcalls(3) ─── arch drivers (uart, wdt)
 │
 ├─ do_initcalls(4) ─── subsystem drivers (IRQ controller, I2C host)
 │
 ├─ do_initcalls(5) ─── filesystem drivers (MMC → SD card block layer)
 │
 └─ do_initcalls(6) ─── device drivers (display, timer, I2C clients)
```

**Platform bus flow** — khi `platform_device_register()` được gọi:
1. Bus lưu device vào danh sách
2. Bus tìm `platform_driver` có `.name` khớp
3. Nếu tìm được → gọi `driver.probe(pdev)`
4. Nếu driver chưa load → đợi đến khi `platform_driver_register()` chạy, sau đó retry

**Nguồn sự thật duy nhất cho hardware layout:**
- Device → [`arch/arm/mach-omap2/board-bbb.c`](../vinix-kernel/arch/arm/mach-omap2/board-bbb.c)
- IRQ numbers → [`arch/arm/mach-omap2/include/mach/irqs.h`](../vinix-kernel/arch/arm/mach-omap2/include/mach/irqs.h)
- Base addresses → [`arch/arm/mach-omap2/include/mach/memmap.h`](../vinix-kernel/arch/arm/mach-omap2/include/mach/memmap.h)
- Clock control → [`arch/arm/mach-omap2/include/mach/prcm.h`](../vinix-kernel/arch/arm/mach-omap2/include/mach/prcm.h)

---

## 2. Quy Trình 5 Bước

### Bước 0 — Thu thập thông tin hardware (KHÔNG bỏ qua)

Trước khi viết 1 dòng code, xác minh đủ 5 thứ này từ AM335x TRM:

| Cần biết | Nguồn |
| --- | --- |
| Base address | TRM Ch.02 Memory Map hoặc `mach/memmap.h` |
| Register offset + bit field | TRM chapter của peripheral |
| IRQ number | TRM Ch.06 hoặc `mach/irqs.h` |
| Clock enable sequence | TRM Ch.08 PRCM |
| Init sequence / timing | `reference/drivers/<name>/index.md` nếu có |
| Peripheral nằm trong vùng MMU nào | `include/mmu.h` — xem `PERIPH_L4_WKUP_PA`, `PERIPH_L4_PER_PA`, `PERIPH_L4_FAST_PA` |

**Thiếu bất kỳ thứ nào → DỪNG. Hỏi trước, không đoán.**
Sai base address = brick device. Sai IRQ = silent failure. Sai init sequence = undefined behavior.

> **MMU trap:** VinixOS dùng static mapping — không có `ioremap()`. Nếu peripheral nằm trong vùng chưa được map trong `mmu_build_page_table_boot()`, driver sẽ DATA ABORT ngay lần `mmio_write32()` đầu tiên. Kiểm tra `arch/arm/mm/mmu.c` — nếu vùng chưa có (ví dụ L4_FAST cho CPSW/MDIO) → thêm vào `mmu_build_page_table_boot()`, `mmu_new_pgd()`, và `mmu_init()` trước khi viết bất kỳ dòng driver nào.

### Bước 1 — Thêm entry vào board-bbb.c

Mỗi HW peripheral cần 1 entry static trong `bbb_devices[]`. Không có entry = bus không biết device tồn tại = driver không bao giờ được probe.

```c
/* File: arch/arm/mach-omap2/board-bbb.c */

static struct platform_device omap_xxx0 = {
    .name   = "omap-xxx",          /* phải khớp với driver .drv.name */
    .base   = 0xAABBCCDD,          /* từ TRM Ch.02 / mach/memmap.h   */
    .irq    = PLATFORM_IRQ_XXX,    /* từ mach/irqs.h; 0 nếu polling   */
    .clk_id = "xxx0",              /* clock gate id cho PRCM          */
};

/* Thêm vào bbb_devices[] */
static struct platform_device *bbb_devices[] = {
    ...
    &omap_xxx0,   /* ← thêm vào đây */
};
```

### Bước 2 — Tạo driver file

Tạo file tại đường dẫn phù hợp theo Linux subsystem layout:

```
drivers/
  tty/serial/     ← UART
  irqchip/        ← IRQ controller
  clocksource/    ← Timer
  mmc/host/       ← MMC host controller
  i2c/busses/     ← I2C host controller
  gpu/drm/<name>/ ← Display
  video/fbdev/    ← Framebuffer
  watchdog/       ← Watchdog
  base/           ← Platform bus core (đừng đụng vào)
```

Template driver (copy và điền vào):

```c
/*
 * drivers/<subsystem>/<vendor>_<peripheral>.c — One-line description
 *
 * Mô tả ngắn WHAT driver này provide.
 */

#include "types.h"
#include "mmio.h"
#include "platform_device.h"
#include "vinix/init.h"
#include "vinix/errno.h"
#include "mach/prcm.h"
#include "mach/memmap.h"
#include "mach/irqs.h"
/* header subsystem phù hợp — xem bảng §4 */

/* Register offsets — từ TRM, không đoán */
#define XXX_CTRL    0x00
#define XXX_STATUS  0x04

/* Bit fields */
#define CTRL_EN     (1 << 0)
#define STATUS_BUSY (1 << 0)

static int omap_xxx_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq              = platform_get_irq(pdev, 0);
    uint32_t base        = mem ? mem->start : 0;

    pr_info("[XXX] probing %s @ 0x%08x irq %d\n", pdev->name, base, irq);

    if (!mem)
        return -EINVAL;

    /* --- Clock enable (TRM Ch.08) --- */
    /* ... */

    /* --- Hardware reset + configure --- */
    /* ... */

    /* --- IRQ (bỏ qua nếu polling mode) --- */
    if (irq > 0) {
        if (request_irq(irq, omap_xxx_irq_handler, 0, "omap-xxx", NULL))
            return -EIO;
        enable_irq(irq);
    }

    /* --- Đăng ký với subsystem --- */
    return xxx_register(...);
}

static int omap_xxx_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver omap_xxx_driver = {
    .drv    = { .name = "omap-xxx" },   /* phải khớp board-bbb.c .name */
    .probe  = omap_xxx_probe,
    .remove = omap_xxx_remove,
};

static int __init omap_xxx_driver_init(void)
{
    return platform_driver_register(&omap_xxx_driver);
}
<INITCALL_LEVEL>(omap_xxx_driver_init);   /* xem §3 */
```

**Luật bất di bất dịch:**
- Tất cả HW init nằm trong `probe()` — không có public `xxx_init()` gọi từ main.c
- `pdev->name` phải khớp chính xác với `.drv.name` trong driver struct
- Luôn dùng `platform_get_resource()` để lấy base address — không hardcode trong driver

### Bước 3 — Chọn initcall level

```
Driver của bạn là gì?
│
├─ Platform bus / board device table?
│   → core_initcall   (level 1) — đã dùng cho board-bbb.c, không thêm nữa
│
├─ Early console / watchdog disable?
│   → arch_initcall   (level 3)
│
├─ IRQ controller, I2C host, SPI host?
│   (các driver mà driver khác phụ thuộc vào)
│   → subsys_initcall (level 4)
│
├─ Block layer, MMC host, storage?
│   → fs_initcall     (level 5)
│
└─ Mọi thứ còn lại — display, timer, sensor, I2C client?
    → device_initcall (level 6)
    → hoặc dùng macro module_platform_driver(<drv>) thay vì tự viết
```

**Lý do level quan trọng:** `main.c` gọi `do_initcalls()` theo thứ tự 1→3→4→5→6. Driver ở level cao hơn dependency của nó sẽ probe thành công. Driver ở level thấp hơn dependency → dùng `EPROBE_DEFER`.

**Dùng `module_platform_driver` hay explicit initcall?**

```c
/* Cách 1 — macro (tương đương device_initcall, level 6) */
module_platform_driver(omap_xxx_driver);

/* Cách 2 — explicit (dùng khi cần level khác 6) */
static int __init omap_xxx_driver_init(void)
{
    return platform_driver_register(&omap_xxx_driver);
}
subsys_initcall(omap_xxx_driver_init);   /* level 4 */
```

### Bước 4 — Xử lý dependency (EPROBE_DEFER)

Nếu driver cần một subsystem khác chưa chắc ready:

```c
static int omap_xxx_probe(struct platform_device *pdev)
{
    /* I2C host phải ready trước khi I2C client probe */
    if (!i2c_adapter_ready())
        return -EPROBE_DEFER;   /* platform bus sẽ retry sau */

    /* ... rest of probe ... */
}
```

Platform bus sẽ tự động retry tất cả deferred driver sau mỗi successful probe. Không cần loop hay timer.

**Chỉ dùng EPROBE_DEFER khi** dependency thực sự có thể chưa load. Nếu HW không tồn tại → `-ENODEV`. Alloc fail → `-ENOMEM`. Register error → `-EIO`.

### Bước 5 — Đăng ký với subsystem

Cuối `probe()`, đăng ký driver với subsystem core phù hợp:

| Driver type | Header | Đăng ký |
| --- | --- | --- |
| UART | `vinix/serial_core.h` | `uart_register_driver()` + `uart_add_one_port()` |
| Character device | `vinix/cdev.h` | `cdev_register()` |
| Block device | `vinix/blkdev.h` | `add_disk()` |
| I2C host | `vinix/i2c.h` | `i2c_add_adapter()` |
| MMC host | `vinix/mmc/host.h` | `mmc_alloc_host()` + `mmc_add_host()` |
| Framebuffer | `vinix/fb.h` | `register_framebuffer()` |
| IRQ chip | `vinix/irqchip.h` | `irqchip_register()` |
| Watchdog | `vinix/watchdog.h` | `watchdog_register_device()` |
| Network | `vinix/netdevice.h` | `register_netdev()` |

---

## 3. Worked Example — omap_serial.c

> Gold reference driver: [`drivers/tty/serial/omap_serial.c`](../vinix-kernel/drivers/tty/serial/omap_serial.c)
> Đọc file này khi bắt đầu bất kỳ driver mới nào. 100% compliant.

**Board entry** (`board-bbb.c`):
```c
static struct platform_device omap_uart0 = {
    .name   = "omap-uart",
    .base   = 0x44E09000,          /* TRM Ch.02: UART0 base */
    .irq    = PLATFORM_IRQ_UART0,  /* mach/irqs.h */
    .clk_id = "uart0",
};
```

**Driver probe** — flow từ probe() của omap_serial:

```
probe() vào
  │
  ├─ Lấy base/irq từ platform_get_resource()
  │
  ├─ Đợi TX shift reg idle (LSR_TEMT)  ← WHY: PRCM touch mid-TX drops bytes
  │
  ├─ Enable UART0 module clock (CM_PER_UART0_CLKCTRL)
  │   └─ Poll IDLEST == FUNCTIONAL
  │
  ├─ Configure FIFO (FCR), SCR, IER
  │
  ├─ request_irq() + enable_irq()
  │
  └─ return 0  ← không gọi uart_register_driver ở đây, đã tự quản lý ring buffer
```

**Initcall level:** `subsys_initcall` (level 4) — UART phải up trước bất kỳ subsystem nào cần console output.

**Điều driver này làm ĐÚNG:**
- Lấy base từ `platform_get_resource()`, fallback sang `UART0_BASE` chỉ cho earlycon path
- Mọi HW init trong probe(), không có gì ở main.c
- IRQ handler `uart_rx_irq_handler` là static
- Readback register sau khi write (trong bring-up `pr_info`)
- `[UART]` prefix cho mọi log

---

## 4. Initcall Levels — Thứ Tự Thực Tế

`main.c` gọi initcall theo thứ tự sau (confirmed từ source):

```
do_initcalls(1)  →  core_initcall    board-bbb.c registers devices
                                     (phải xong trước mọi driver probe)

do_initcalls(3)  →  arch_initcall    omap_serial (UART console)
                                     omap_wdt (watchdog disable)

do_initcalls(4)  →  subsys_initcall  irq-omap-intc (IRQ controller)
                                     i2c-omap (I2C host)

    [VFS + MMC setup giữa chừng trong main.c]

do_initcalls(5)  →  fs_initcall      omap_hsmmc (MMC host → SD card)

do_initcalls(6)  →  device_initcall  timer-omap-dm (DMTimer)
                                     tilcdc + tda998x (HDMI display)
```

Level 2 (`postcore_initcall`) và level 7 (`late_initcall`) hiện không được gọi trong main.c — không dùng cho đến khi main.c thêm `do_initcalls(2/7)`.

---

## 5. Debugging

VinixOS không có JTAG. UART log là công cụ duy nhất.

**Pattern checkpoint:**
```c
pr_info("[XXX] Step A: before hw_reset\n");
mmio_write32(base + CTRL, CTRL_RESET);
pr_info("[XXX] Step A: after hw_reset, readback=0x%08x\n",
        mmio_read32(base + CTRL));
```

**Register readback bắt buộc trong bring-up:**
```c
mmio_write32(base + OFFSET, value);
pr_info("[DRV] wrote 0x%08x → readback 0x%08x\n",
        value, mmio_read32(base + OFFSET));
```

**Khi probe() không được gọi:**
1. Kiểm tra `.name` trong driver struct có khớp chính xác với board-bbb.c entry không (case-sensitive, kể cả dấu `-`)
2. Kiểm tra device đã được add vào `bbb_devices[]` chưa
3. Kiểm tra initcall level — driver có thể load trước khi bus ready

**Khi driver probe() fail:**
- Return `-EPROBE_DEFER` nếu dependency chưa ready → bus tự retry
- Return `-EIO` nếu HW không respond (timeout, không đọc được revision register)
- Không return 0 khi có error — che giấu bug

**Exception / Abort:**
Yêu cầu DFAR, DFSR, PC từ UART log. Nếu handler chưa print → thêm vào `arch/arm/exceptions/` trước khi debug tiếp.

---

## 6. Definition of Done

**Driver chỉ được gọi là done khi:**

- [ ] Compile không warning
- [ ] Boot log in `[XXX] probing <name> @ 0x... irq N` từ probe()
- [ ] Boot log in confirmation thành công (ví dụ: `[UART] I2C0 initialized`)
- [ ] Subsystem registration thành công (ví dụ: disk mount, framebuffer ready)
- [ ] User test trên BBB thật và confirm qua UART log

**Không bao giờ** tuyên bố driver "works" chỉ dựa compile. Câu chuẩn khi done:

> "Build sạch, không warning. Bạn flash + test trên BBB thật giúp — đợi UART log để confirm."

---

## 7. Checklist Nhanh (Pre-Submit)

```
Hardware info
 [ ] Base address verified từ TRM Ch.02 / mach/memmap.h
 [ ] Register offset + bit field từ TRM chapter của peripheral
 [ ] IRQ number từ mach/irqs.h hoặc TRM Ch.06
 [ ] Clock enable verified từ TRM Ch.08
 [ ] Peripheral region (L4_WKUP/L4_PER/L4_FAST) đã có trong mmu_build_page_table_boot() — thêm nếu thiếu

board-bbb.c
 [ ] Có entry trong bbb_devices[]
 [ ] .name khớp với driver .drv.name
 [ ] .base và .irq đúng
 [ ] .clk_id set (hoặc 0 nếu không dùng)

Driver file
 [ ] Mọi HW init trong probe() — không có public xxx_init()
 [ ] platform_get_resource() dùng để lấy base (không hardcode)
 [ ] Initcall level đúng theo decision tree §3
 [ ] Subsystem registration ở cuối probe()
 [ ] [MODULE] prefix trong mọi pr_info/pr_err
 [ ] Return đúng errno khi probe fail

Test
 [ ] Build sạch
 [ ] Boot log có probe message
 [ ] User test trên hardware thật
```

---

## Phụ lục — File Reference

| Cần gì | File |
| --- | --- |
| Board device table | [`arch/arm/mach-omap2/board-bbb.c`](../vinix-kernel/arch/arm/mach-omap2/board-bbb.c) |
| IRQ numbers | [`mach/irqs.h`](../vinix-kernel/arch/arm/mach-omap2/include/mach/irqs.h) |
| Base addresses | [`mach/memmap.h`](../vinix-kernel/arch/arm/mach-omap2/include/mach/memmap.h) |
| PRCM clock IDs | [`mach/prcm.h`](../vinix-kernel/arch/arm/mach-omap2/include/mach/prcm.h) |
| Initcall macros | [`include/vinix/init.h`](../vinix-kernel/include/vinix/init.h) |
| Platform bus API | [`drivers/base/platform.c`](../vinix-kernel/drivers/base/platform.c) |
| Gold reference driver | [`drivers/tty/serial/omap_serial.c`](../vinix-kernel/drivers/tty/serial/omap_serial.c) |
| Driver template skeleton | [`Documentation/driver-template/`](../Documentation/driver-template/) |
| AM335x TRM reference | [`reference/am335x/`](am335x/) |
| Style + comment rules | [`CLAUDE.md`](../CLAUDE.md) · [`coding_standards.md`](coding_standards.md) |
