/*
 * drivers/ui/ui_main.c — LVGL UI task for Nothan-OS
 *
 * Boot sequence:
 *   1. lv_init()
 *   2. lv_port_disp_init()   — 800×480 framebuffer
 *   3. lv_port_indev_init()  — USB HID mouse (hides raw cursor)
 *   4. cursor_canvas_init()  — LVGL canvas arrow cursor on LV_LAYER_SYS
 *   5. lv_scr_load(lock)     — start with lock screen
 *   6. render loop
 *
 * Timer tick: DMTimer2 IRQ calls lv_tick_inc(10) every 10 ms.
 */

#include "lvgl/lvgl.h"
#include "lv_port.h"
#include "cursor_canvas.h"
#include "types.h"
#include "task.h"
#include "sleep.h"
#include "vinix/printk.h"

lv_obj_t *ui_lock_create(void);

#define UI_STACK_SIZE   8192
static uint8_t           ui_stack[UI_STACK_SIZE] __attribute__((aligned(4096)));
static struct task_struct ui_task_struct;

static void ui_task_fn(void)
{
    pr_info("[UI] LVGL initialising (Nothan-OS 800x480)\n");

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    /* Attach canvas cursor to mouse indev — replaces raw XOR cursor */
    cursor_canvas_init(lv_port_get_mouse_indev());

    pr_info("[UI] Loading lock screen\n");
    lv_scr_load(ui_lock_create());

    pr_info("[UI] Entering render loop\n");
    while (1) {
        lv_task_handler();
        msleep(5);
    }
}

struct task_struct *get_ui_task(void)
{
    ui_task_struct.name = "ui";
    task_stack_init(&ui_task_struct, ui_task_fn, ui_stack, UI_STACK_SIZE);
    return &ui_task_struct;
}
