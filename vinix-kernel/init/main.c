/* ============================================================
 * main.c
 * ------------------------------------------------------------
 * Kernel Entry Point
 * ============================================================ */

#include "uart.h"
#include "scheduler.h"
#include "idle.h"
#include "timer.h"
#include "irq.h"
#include "intc.h"
#include "mmu.h"
#include "platform_device.h"
#include "vinix/init.h"
#include "page_alloc.h"
#include "slab.h"
#include "vmm.h"
#include "atomic.h"
#include "spinlock.h"
#include "cpu.h"

extern void sync_selftest(void);
#include "vfs.h"
#include "devfs.h"
#include "procfs.h"
#include "mmc.h"
#include "mbr.h"
#include "fat32.h"
#include "buffer_cache.h"
#include "vinix/blkdev.h"
#include "selftest.h"
#include "syscalls.h"
#include "types.h"
#include "i2c.h"
#include "lcdc.h"
#include "tda19988.h"
#include "fb.h"
#include "boot_screen.h"
/* ============================================================
 * User Space Payload (Defined in payload.S)
 * ============================================================ */
extern uint8_t _shell_payload_start;
extern uint8_t _shell_payload_end;

/* ============================================================
 * User App Memory Definitions
 * ============================================================ */
static struct task_struct shell_task;

/* We use the end of the 1MB User Space (0x40000000 -> 0x40100000)
 * as the stack base for the User Task. Let's reserve the top 4KB. */
#define USER_STACK_BASE (USER_SPACE_VA + (USER_SPACE_MB * 1024 * 1024))
#define USER_STACK_SIZE 4096

/* ============================================================
 * Kernel Main
 * ============================================================ */
void kernel_main(void)
{
    /* core_initcall — board file registers platform bus + devices.
     * arch_initcall — early HW (uart for log output, watchdog disable). */
    do_initcalls(1);
    do_initcalls(3);

    pr_info("\n\n");
    pr_info("========================================\n");
    pr_info(" VinixOS: Interactive Shell\n");
    pr_info("========================================\n\n");

    /* 1.5 MMU Phase B — remove identity map, update VBAR to high VA.
     * MMU was already enabled by entry.S (Phase A trampoline).
     * We are now running at VA 0xC0xxxxxx. */
    mmu_init();

    page_alloc_init();
    page_alloc_selftest();

    slab_init();
    slab_selftest();

    vmm_init();
    vmm_selftest();

    sync_selftest();

    /* Graphics subsystem: pixel clock must be running before TDA can output TMDS.
     * Unlike QNX (where U-Boot already started LCDC), we're bare-metal —
     * LCDC raster must start first to provide pixel clock on LCD_PCLK pin.
     *
     * Order: I2C → LCDC (config + raster start) → TDA (full init with pixel clock) */
    i2c_init();
    i2c_register_adapter();
    i2c_scan();
    lcdc_init();                /* Configure LCDC + DPLL (raster NOT started yet) */
    lcdc_start_raster();        /* Start pixel clock — TDA needs this for TMDS */
    tda19988_init();            /* Full TDA config with pixel clock present */

    lcdc_register_fb();         /* fb_info -> fbdev so fb_init reads via subsystem */
    fb_init();

    /* subsys_initcall — interrupt controller before timers/MMC. */
    do_initcalls(4);
    irq_init();
    uart_enable_rx_interrupt();

    /* 1.6 Initialize VFS and mount FAT32 rootfs from SD card */
    pr_info("[BOOT] Initializing Virtual File System...\n");
    bcache_init();
    vfs_init();

    /* fs_initcall — host probes via mmc-core, registers gendisk "mmc0". */
    do_initcalls(5);
    if (!get_gendisk("mmc0")) {
        pr_err("[BOOT] ERROR: MMC driver probe failed\n");
        while (1);
    }

    uint32_t part_lba;
    if (mbr_find_fat32(&part_lba, NULL) != E_OK) {
        pr_err("[BOOT] ERROR: No FAT32 partition found on SD card\n");
        while (1);
    }

    if (fat32_init(part_lba) != E_OK) {
        pr_err("[BOOT] ERROR: FAT32 init failed\n");
        while (1);
    }

    if (vfs_mount("/", fat32_get_operations()) != E_OK) {
        pr_err("[BOOT] ERROR: Failed to mount FAT32 at /\n");
        while (1);
    }

    if (vfs_mount("/dev", devfs_init()) != E_OK) {
        pr_err("[BOOT] ERROR: Failed to mount devfs at /dev\n");
        while (1);
    }

    if (vfs_mount("/proc", procfs_init()) != E_OK) {
        pr_err("[BOOT] ERROR: Failed to mount procfs at /proc\n");
        while (1);
    }

    /* 1.7 Load User Payload */
    uint32_t payload_size = (uint32_t)&_shell_payload_end - (uint32_t)&_shell_payload_start;
    pr_info("[BOOT] Loading User App Payload to 0x%x (Size: %d bytes)\n", USER_SPACE_VA, payload_size);

    uint8_t *src = &_shell_payload_start;
    uint8_t *dst = (uint8_t *)USER_SPACE_VA;
    for (uint32_t i = 0; i < payload_size; i++)
        dst[i] = src[i];
    pr_info("[BOOT] Payload successfully copied to User Space.\n");

    /* 2. Schedule Initialization */
    scheduler_init();

    struct task_struct *idle_ptr = get_idle_task();
    scheduler_add_task(idle_ptr);

    shell_task.name = "init";
    shell_task.state = TASK_RUNNING;
    shell_task.id = 0;

    task_stack_init(&shell_task, (void (*)(void))USER_SPACE_VA,
                    (void *)(USER_STACK_BASE - USER_STACK_SIZE),
                    USER_STACK_SIZE);

    if (scheduler_add_task(&shell_task) < 0)
        pr_info("[BOOT] Failed to add User App Task\n");

    pr_info("[BOOT] UART init complete. Starting HDMI boot screen...\n");

    /* 3. Boot screen on HDMI — uses timer_early_init() for accurate delay.
     *    MUST run BEFORE timer_init() which reconfigures timer to 10ms auto-reload.
     *    delay_ms() needs free-running counter, not auto-reload. */
    timer_early_init();
    boot_screen_run();

    /* 4. Now reconfigure timer for scheduler — runs all device_initcall(s).
     * Currently only omap_dmtimer; remaining drivers move here as their
     * subsystem cores ship. */
    do_initcalls(6);

    /* Final gate: integration selftest (bcache, procfs). Panics on fail. */
    selftest_run_all();

    pr_info("[BOOT] Boot complete. Starting scheduler...\n");

    irq_enable();
    scheduler_start();

    /* Should never reach here */
    while (1)
    {
        pr_emerg("PANIC: Scheduler returned!\n");
    }
}