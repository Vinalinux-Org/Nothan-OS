/*
 * init/main.c — kernel entry point
 *
 * Called from entry.S after MMU and stack setup.  Orchestrates
 * hardware bring-up, subsystem initialization, and scheduler start.
 * This function never returns.
 */

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

extern void sync_selftest(void);

/*
 * Embedded User Payload
 *
 * The initial shell application is linked directly into the kernel image
 * to avoid requiring a working filesystem on early boot.
 */
extern uint8_t _shell_payload_start;
extern uint8_t _shell_payload_end;

/* Initial User Process State */
static struct task_struct shell_task;

/*
 * We allocate the user stack at the top of the 1MB User Space memory
 * region (0x40000000 -> 0x40100000). The stack grows downwards.
 */
#define USER_STACK_BASE (USER_SPACE_VA + (USER_SPACE_MB * 1024 * 1024))
#define USER_STACK_SIZE 4096


/*
 * kernel_main - The main boot orchestration function
 *
 * This function must never return. It transitions the system from
 * basic hardware initialization into a fully multitasking environment.
 */
void kernel_main(void)
{
    /*
     * Execute early initialization calls:
     * Level 1: Board file registers the platform bus and static devices.
     * Level 3: Early hardware like UART for log output and Watchdog disable.
     */
    do_initcalls(1);
    do_initcalls(3);

    pr_info("\n\n");
    pr_info("========================================\n");
    pr_info(" VinixOS: Interactive Shell\n");
    pr_info("========================================\n\n");

    /*
     * Finalize MMU initialization by removing the identity mapping that
     * was used during the boot process. Also, update the Vector Base
     * Address Register (VBAR) to point to our high virtual address space
     * exception vectors. We are now running exclusively at VA 0xC0xxxxxx.
     */
    mmu_init();

    page_alloc_init();
    page_alloc_selftest();

    slab_init();
    slab_selftest();

    vmm_init();
    vmm_selftest();

    sync_selftest();

    /*
     * Initialize the graphics subsystem early to show the boot screen.
     * The LCDC hardware must start generating the pixel clock before the
     * TDA19988 HDMI transmitter is configured.
     *
     * Bring-up sequence:
     * 1. Initialize I2C (required for TDA config)
     * 2. Configure LCDC and start the raster to provide the pixel clock
     * 3. Initialize the TDA19988 HDMI chip
     */
    i2c_init();
    i2c_register_adapter();
    i2c_scan();
    lcdc_init();                /* Configure LCDC + DPLL (raster NOT started yet) */
    lcdc_start_raster();        /* Start pixel clock — TDA needs this for TMDS */
    tda19988_init();            /* Full TDA config with pixel clock present */

    lcdc_register_fb();         /* fb_info -> fbdev so fb_init reads via subsystem */
    fb_init();

    /*
     * Subsystem Init: Initialize the interrupt controller before
     * any timers or MMC storage devices are brought up.
     */
    do_initcalls(4);
    irq_init();
    uart_enable_rx_interrupt();

    /*
     * Virtual File System Bring-up
     *
     * We initialize the block cache and VFS, then attempt to mount the
     * primary FAT32 partition from the SD card as the root filesystem.
     */
    pr_info("[BOOT] Initializing Virtual File System...\n");
    bcache_init();
    vfs_init();

    /*
     * Execute filesystem initcalls. The MMC host probes via mmc-core
     * and registers the gendisk "mmc0" which is required for FAT32.
     */
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

    /*
     * Load the User-Space Payload
     *
     * Copies the embedded shell executable from kernel memory into
     * the designated 1MB user-space region.
     */
    uint32_t payload_size = (uint32_t)&_shell_payload_end - (uint32_t)&_shell_payload_start;
    pr_info("[BOOT] Loading User App Payload to 0x%x (Size: %d bytes)\n", USER_SPACE_VA, payload_size);

    uint8_t *src = &_shell_payload_start;
    uint8_t *dst = (uint8_t *)USER_SPACE_VA;
    for (uint32_t i = 0; i < payload_size; i++)
        dst[i] = src[i];
    pr_info("[BOOT] Payload successfully copied to User Space.\n");

    /*
     * Initialize the Multitasking Scheduler
     *
     * The scheduler takes over CPU execution. We register the idle task
     * and the initial user shell task.
     */
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

    /*
     * Display the HDMI Boot Screen.
     * We use a temporary free-running timer for accurate delays during
     * rendering. This must run before the final scheduler timer is set up.
     */
    timer_early_init();
    boot_screen_run();

    /*
     * Device Init: Now reconfigure the timer for the scheduler and run
     * all remaining device drivers.
     */
    do_initcalls(6);

    /*
     * Run the final system self-tests to ensure critical structures
     * like the block cache and procfs are fully functional.
     */
    selftest_run_all();

    pr_info("[BOOT] Boot complete. Starting scheduler...\n");

    irq_enable();
    scheduler_start();

    /*
     * The kernel main thread now becomes the idle loop.
     * The scheduler will never return from this point.
     */
    while (1)
    {
        pr_emerg("PANIC: Scheduler returned!\n");
    }
}