# Driver templates

Skeleton files for new VinixOS hardware drivers. Copy the matching
template into `vinix-kernel/drivers/<subsystem>/<name>.c` and replace
every `TEMPLATE:` annotation with the real value.

| File | Use for |
|------|---------|
| [`platform_driver.c`](platform_driver.c) | Memory-mapped peripheral on the platform bus (UART, MMC, timer, watchdog, IRQ controller, framebuffer, etc.). |
| [`i2c_driver.c`](i2c_driver.c) | Chip on the I2C bus (HDMI encoder, codec, EEPROM, sensor). Note: i2c client framework still WIP — see template header. |

## Gold reference

When in doubt, read the canonical compliant driver in the codebase:

* Platform driver — [`vinix-kernel/drivers/tty/serial/omap_serial.c`](../../vinix-kernel/drivers/tty/serial/omap_serial.c)

`omap_serial.c` is the only driver guaranteed to follow every rule in
[CLAUDE.md §9](../../CLAUDE.md). Do **not** clone any other driver as a
starting point until A.2 driver convention enforcement is fully
complete (display chain still pending).

## Hard rules (lifted from CLAUDE.md §9)

1. **No hardcoded peripheral base in the driver file.**
   Pull SoC addresses from `<mach/prcm.h>` / `<mach/control.h>`, and
   the per-instance base/IRQ from `platform_get_resource()`.
2. **`probe()` is the only entry point.** Never expose `xxx_init()`
   for `init/main.c` to call directly.
3. **Register through the right subsystem class.** Don't skip to
   `cdev_register` if a tty/mmc/fb/i2c host wrapper exists.
4. **Add a board entry** in `vinix-kernel/arch/arm/mach-omap2/board-bbb.c`
   (`bbb_devices[]` for platform devices, `bbb_i2c0_devices[]` for
   i2c clients) — matching by string name.
5. **Pick the right initcall level** (see the comment in
   `platform_driver.c`).
6. **Inter-driver dependencies use `-EPROBE_DEFER`**, not initcall
   ordering tricks. Provider sets a global ready flag; consumer
   returns `-EPROBE_DEFER` until that flag is set; core retries
   deferred probes on every initcall pass.
