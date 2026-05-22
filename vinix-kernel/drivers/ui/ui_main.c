/*
 * drivers/ui/ui_main.c — LVGL UI task for Nothan-OS
 *
 * Boot sequence:
 *   1. lv_init()
 *   2. lv_port_disp_init()   — 800×480 framebuffer
 *   3. lv_port_indev_init()  — USB HID mouse (hides raw cursor)
 *   4. lv_scr_load(lock)     — start with lock screen
 *   5. cursor_canvas_init()  — attach canvas cursor AFTER first screen load
 *   6. render loop
 *
 * Timer tick: DMTimer2 IRQ calls lv_tick_inc(10) every 10 ms.
 *
 * Stack: 32 KB — LVGL render pipeline + widget creation needs ~15-20 KB peak.
 */

#include "lvgl/lvgl.h"
#include "lv_port.h"
#include "cursor_canvas.h"
#include "types.h"
#include "task.h"
#include "timer.h"
#include "vinix/printk.h"

lv_obj_t *ui_lock_create(void);

#define UI_STACK_SIZE   (32 * 1024)
static uint8_t           ui_stack[UI_STACK_SIZE] __attribute__((aligned(4096)));
static struct task_struct ui_task_struct;

static void ui_task_fn(void)
{
    pr_info("[UI] LVGL initialising (Nothan-OS 800x480)\n");

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    pr_info("[UI] Loading lock screen\n");
    lv_scr_load(ui_lock_create());

    /* Attach canvas cursor after screen is loaded so lv_layer_sys() is ready */
    cursor_canvas_init(lv_port_get_mouse_indev());

    pr_info("[UI] Entering render loop\n");
    while (1) {
        lv_task_handler();
        delay_ms(5);
    }
}

struct task_struct *get_ui_task(void)
{
    ui_task_struct.name = "ui";
    task_stack_init(&ui_task_struct, ui_task_fn, ui_stack, UI_STACK_SIZE);
    return &ui_task_struct;
}
