# 10 — Subsystem Reference

Quick lookup cho mọi `vinix/*` header — driver writer cần subsystem nào mở section đó.

> Đối tượng: tham khảo nhanh khi viết driver. Skeleton đầy đủ + walk-through cho ethernet xem [driver-development-guide.md](driver-development-guide.md).

---

## init — boot-time dispatch

**Header**: `vinix/init.h`
**Impl**: `init/initcall.c`

### Macros
```c
#define __init      /* function placed in .init.text — discardable */
#define __initdata  /* data placed in .init.data */

core_initcall(fn)        /* level 1 — bus_register, ... */
postcore_initcall(fn)    /* level 2 */
arch_initcall(fn)        /* level 3 — early HW (uart) */
subsys_initcall(fn)      /* level 4 — IRQ controller */
fs_initcall(fn)          /* level 5 — block devices, FS */
device_initcall(fn)      /* level 6 — most drivers */
late_initcall(fn)        /* level 7 — selftest */

module_platform_driver(drv)   /* shorthand: device_initcall + register */
```

### Pattern
```c
static int __init my_driver_init(void)
{
    return platform_driver_register(&my_driver);
}
arch_initcall(my_driver_init);   /* hoặc subsys/fs/device tùy ordering */
```

`main.c` gọi `do_initcalls(N)` ở mỗi level — driver tự register, không phải khai báo manual.

---

## printk — kernel logging

**Header**: `vinix/printk.h`
**Impl**: `kernel/printk/printk.c`

### Macros
```c
void printk(const char *fmt, ...);

pr_emerg(fmt, ...)   /* KERN_EMERG */
pr_alert(fmt, ...)
pr_crit(fmt, ...)
pr_err(fmt, ...)     /* error */
pr_warn(fmt, ...)    /* warning */
pr_notice(fmt, ...)
pr_info(fmt, ...)    /* informational — most common */
pr_debug(fmt, ...)
```

### Pattern
```c
pr_info("[MODULE] base 0x%08x irq %d\n", base, irq);
pr_err("[MODULE] init failed: %d\n", ret);
```

`uart_printf` là alias backward-compat — ưu tiên `pr_*` ở code mới.

---

## errno — Linux error codes

**Header**: `vinix/errno.h`

### Constants (positive, dùng dưới dạng `-E*`)
```c
EPERM=1, ENOENT=2, EIO=5, ENOMEM=12, EFAULT=14, EBUSY=16,
EEXIST=17, ENOTDIR=20, EISDIR=21, EINVAL=22, EMFILE=24,
ENOSPC=28, EAGAIN=11, ...
```

### Pattern
```c
if (!arg)            return -EINVAL;
if (!buf)            return -ENOMEM;
if (slot_full)       return -ENOSPC;
if (already_taken)   return -EBUSY;
```

**Lưu ý**: Syscall boundary vẫn dùng `E_OK/E_FAIL/E_INVAL/...` từ `syscalls.h` (ABI với userspace). Internal kernel paths dùng `-EXXX`.

---

## fs / cdev — character device

**Header**: `vinix/fs.h`, `vinix/cdev.h`
**Impl**: `drivers/char/char_dev.c`

### Structs
```c
struct file_operations {
    int     (*open)   (struct file *);
    int     (*release)(struct file *);
    int     (*read)   (struct file *, void *buf, uint32_t len);
    int     (*write)  (struct file *, const void *buf, uint32_t len);
    int     (*ioctl)  (struct file *, uint32_t cmd, uint32_t arg);
    int32_t (*llseek) (struct file *, int32_t off, int whence);
};

struct file {
    const struct file_operations *f_op;
    void                         *private_data;
    uint32_t                      f_pos;
    uint32_t                      f_flags;
};

struct cdev {
    const char                   *name;     /* "tty", "null" */
    const struct file_operations *fops;
    void                         *priv;
};

int cdev_register(const struct cdev *c);
```

### Pattern (driver creates a /dev node)
```c
static int my_read(struct file *f, void *buf, uint32_t len) { ... }
static int my_write(struct file *f, const void *buf, uint32_t len) { ... }

static const struct file_operations my_fops = {
    .read  = my_read,
    .write = my_write,
};

static const struct cdev my_cdev = {
    .name = "mydev",
    .fops = &my_fops,
};

cdev_register(&my_cdev);   /* exposes /dev/mydev via devfs */
```

---

## blkdev — block device

**Header**: `vinix/blkdev.h`
**Impl**: `block/block.c`

### Structs
```c
struct block_device_operations {
    int (*read_sectors) (struct gendisk *, uint32_t lba, uint32_t count, void *);
    int (*write_sectors)(struct gendisk *, uint32_t lba, uint32_t count, const void *);
};

struct gendisk {
    const char                            *name;          /* "mmc0" */
    uint32_t                               sector_size;   /* usually 512 */
    uint32_t                               total_sectors;
    const struct block_device_operations  *fops;
    void                                  *priv;
};

int             add_disk(struct gendisk *disk);
struct gendisk *get_gendisk(const char *name);
int             blk_read (struct gendisk *, uint32_t lba, uint32_t count, void *);
int             blk_write(struct gendisk *, uint32_t lba, uint32_t count, const void *);
```

### Pattern
```c
static int my_read(struct gendisk *d, uint32_t lba, uint32_t count, void *buf) { ... }

static const struct block_device_operations my_blk_ops = {
    .read_sectors = my_read,
    .write_sectors = my_write,
};

static struct gendisk my_disk = {
    .name = "mydisk",
    .sector_size = 512,
    .fops = &my_blk_ops,
};

add_disk(&my_disk);   /* available qua get_gendisk("mydisk") */
```

---

## irqchip — interrupt controller

**Header**: `vinix/irqchip.h`
**Impl**: `kernel/irqchip/irqchip.c`

### Structs
```c
struct irq_data {
    uint32_t           irq;
    struct irq_chip   *chip;
    void              *chip_data;
};

struct irq_chip {
    const char *name;
    void (*irq_mask)   (struct irq_data *);
    void (*irq_unmask) (struct irq_data *);
    void (*irq_ack)    (struct irq_data *);
    void (*irq_eoi)    (struct irq_data *);
};

int irqchip_register(struct irq_chip *chip);
```

### Pattern
```c
static void my_intc_mask(struct irq_data *d)   { hw_mask(d->irq); }
static void my_intc_unmask(struct irq_data *d) { hw_unmask(d->irq); }

static struct irq_chip my_intc_chip = {
    .name = "my-intc",
    .irq_mask = my_intc_mask,
    .irq_unmask = my_intc_unmask,
};

irqchip_register(&my_intc_chip);   /* enable_irq/disable_irq dispatch tới đây */
```

---

## clocksource / clockevents — tick source

**Header**: `vinix/clocksource.h`
**Impl**: `kernel/time/clockevents.c`

### Structs
```c
struct clock_event_device {
    const char *name;
    uint32_t    rating;        /* highest rated wins */
    int  (*set_state_periodic)(struct clock_event_device *);
    int  (*set_state_oneshot) (struct clock_event_device *);
    int  (*set_next_event)    (uint32_t evt, struct clock_event_device *);
    void (*event_handler)     (struct clock_event_device *);  /* core sets */
};

int clockevents_register_device(struct clock_event_device *dev);
```

### Pattern
```c
static struct clock_event_device my_clkevt = {
    .name = "my-timer",
    .rating = 200,
    .set_state_periodic = my_set_periodic,
};

clockevents_register_device(&my_clkevt);
/* Driver IRQ handler: my_clkevt.event_handler(&my_clkevt); */
```

---

## i2c — bus + transfer

**Header**: `vinix/i2c.h`
**Impl**: `kernel/i2c/i2c-core.c`

### Structs
```c
struct i2c_msg {
    uint16_t  addr;     /* 7-bit slave */
    uint16_t  flags;    /* I2C_M_RD = read */
    uint16_t  len;
    uint8_t  *buf;
};

struct i2c_algorithm {
    int (*master_xfer)(struct i2c_adapter *, struct i2c_msg *, int count);
};

struct i2c_adapter {
    const char                  *name;
    int                          nr;
    const struct i2c_algorithm  *algo;
    void                        *priv;
};

int                 i2c_add_adapter(struct i2c_adapter *adap);
int                 i2c_transfer  (struct i2c_adapter *adap,
                                   struct i2c_msg *msgs, int count);
struct i2c_adapter *i2c_get_adapter(int nr);
```

### Pattern host (adapter)
```c
static int my_xfer(struct i2c_adapter *a, struct i2c_msg *msgs, int n) { ... }

static const struct i2c_algorithm my_algo = { .master_xfer = my_xfer };
static struct i2c_adapter my_adap = { .name = "my-i2c", .algo = &my_algo };

i2c_add_adapter(&my_adap);
```

### Pattern client
```c
struct i2c_adapter *adap = i2c_get_adapter(0);
uint8_t buf[2] = { reg, value };
struct i2c_msg msg = { .addr = SLAVE_ADDR, .flags = 0, .len = 2, .buf = buf };
i2c_transfer(adap, &msg, 1);
```

---

## mmc — host + block bridge

**Header**: `vinix/mmc/host.h`
**Impl**: `kernel/mmc/{core,mmc_block}.c`

### Structs
```c
struct mmc_host_ops {
    void (*request)(struct mmc_host *, struct mmc_request *);
    int  (*get_cd) (struct mmc_host *);
    int  (*get_ro) (struct mmc_host *);
};

struct mmc_host {
    const char                  *name;
    void                        *priv;
    const struct mmc_host_ops   *ops;
    uint32_t                     f_min, f_max, ocr_avail;
};

struct mmc_host *mmc_alloc_host(int extra_priv, const char *name);
int              mmc_add_host  (struct mmc_host *host);

/* MVP shortcut — driver passes plain sector-I/O fns; mmc_block
 * builds the gendisk + block_device_operations + add_disk. */
int mmc_block_register(struct mmc_host *host,
                       int (*read_fn) (uint32_t lba, uint32_t count, void *),
                       int (*write_fn)(uint32_t lba, uint32_t count, const void *),
                       uint32_t total_sectors);
```

### Pattern
```c
struct mmc_host *host = mmc_alloc_host(0, "mmc0");
mmc_add_host(host);
mmc_block_register(host, my_read_sectors, my_write_sectors, total_sects);
/* gendisk "mmc0" appears via add_disk automatically */
```

---

## serial_core — UART framework

**Header**: `vinix/serial_core.h`
**Impl**: `kernel/tty/serial/serial_core.c`

### Structs (contract — full uart_driver migration future)
```c
struct uart_ops {
    uint32_t (*tx_empty)    (struct uart_port *);
    void     (*start_tx)    (struct uart_port *);
    void     (*stop_tx)     (struct uart_port *);
    int      (*startup)     (struct uart_port *);
    void     (*shutdown)    (struct uart_port *);
    void     (*set_termios) (struct uart_port *, uint32_t cflag);
};

struct uart_port {
    void         *membase;
    uint32_t      irq;
    uint32_t      uartclk, fifosize;
    int           line;
    const struct uart_ops *ops;
    void         *priv;
};

int  uart_register_driver(struct uart_driver *drv);
int  uart_add_one_port    (struct uart_driver *drv, struct uart_port *port);
```

### Today's MVP API (used by omap_serial)
```c
int  uart_serial_rx_push(uint8_t ch);   /* IRQ pushes byte */
int  uart_getc(void);                    /* non-blocking read */
int  uart_rx_available(void);
extern wait_queue_head_t uart_rx_wq;     /* wait_event(uart_rx_wq, cond) */
```

---

## fb — framebuffer

**Header**: `vinix/fb.h`
**Impl**: `drivers/video/fbdev/fbmem.c`

### Structs
```c
struct fb_var_screeninfo {
    uint32_t  xres, yres, bits_per_pixel;
    uint32_t  pixclock, left/right/upper/lower_margin, hsync_len, vsync_len;
};

struct fb_fix_screeninfo {
    char      id[16];
    uint32_t  smem_start, smem_len, line_length;
};

struct fb_info {
    int                          node;
    struct fb_var_screeninfo     var;
    struct fb_fix_screeninfo     fix;
    void                        *screen_base;
    const struct fb_ops         *fbops;
    void                        *priv;
};

int register_framebuffer  (struct fb_info *fb);
int unregister_framebuffer(struct fb_info *fb);
```

### Pattern
```c
static struct fb_info my_fb = {
    .var = { .xres = 800, .yres = 600, .bits_per_pixel = 16 },
    .fix = { .id = "mydrv", .line_length = 800 * 2,
             .smem_start = (uint32_t)fb_pixels,
             .smem_len   = 800 * 600 * 2 },
};
my_fb.screen_base = fb_pixels;
register_framebuffer(&my_fb);
/* fbmem now reads geometry from this fb_info on fb_init() */
```

---

## tty — line discipline (header only — defer)

**Header**: `vinix/tty.h`
**Status**: contract chỉ — chưa có impl. Defer cho đến khi có multi-tty consumer.

---

## watchdog — pet timer (header only — defer)

**Header**: `vinix/watchdog.h`
**Status**: contract chỉ — chưa có impl. Defer cho đến khi userspace cần ping `/dev/watchdog`.

---

## netdevice — ethernet (P7 — chưa ship)

**Header**: `vinix/netdevice.h` (sẽ define)
**Status**: contract đã định trong [driver-development-guide.md](driver-development-guide.md). Driver team có thể viết stub mode chờ kernel core P7.

---

## platform_device — driver model (foundation)

**Header**: `platform_device.h` (legacy path — không trong vinix/)
**Impl**: `drivers/base/{device,platform}.c`

### Structs
```c
struct platform_device {
    struct device dev;
    const char   *name;
    uint32_t      base;
    int           irq;
    const char   *clk_id;
};

struct platform_driver {
    struct driver drv;
    int (*probe) (struct platform_device *);
    int (*remove)(struct platform_device *);
};

int  platform_driver_register(struct platform_driver *pdrv);
struct resource *platform_get_resource(struct platform_device *, uint32_t type, unsigned idx);
int  platform_get_irq        (struct platform_device *, unsigned idx);
```

### Pattern (mọi driver bắt đầu từ đây)
```c
static int my_probe(struct platform_device *pdev) {
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq              = platform_get_irq(pdev, 0);
    /* HW init using mem->start, irq */
    /* Then register with subsystem (cdev, gendisk, i2c_adapter, ...) */
    return 0;
}

static struct platform_driver my_driver = {
    .drv   = { .name = "my-name" },
    .probe = my_probe,
};
module_platform_driver(my_driver);
```

Pair với platform_device khai báo trong [arch/arm/mach-omap2/board-bbb.c](../vinix-kernel/arch/arm/mach-omap2/board-bbb.c).

---

## Tài liệu liên quan

- [driver-development-guide.md](driver-development-guide.md) — full skeleton template (ethernet)
- [04-interrupt-and-exception.md](04-interrupt-and-exception.md) — IRQ infrastructure
- [02-kernel-initialization.md](02-kernel-initialization.md) — boot init order
- [99-system-overview.md](99-system-overview.md) — kiến trúc tổng thể
- [CLAUDE.md](../../CLAUDE.md) — coding rules + subsystem conventions
