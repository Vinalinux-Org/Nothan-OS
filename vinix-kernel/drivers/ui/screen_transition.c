/*
 * drivers/ui/screen_transition.c — slide-up transition for Nothan-OS UI
 *
 * Slides active screen out (y: 0 → -480, ease_in, 300 ms), then loads
 * the next screen.
 */

#include "screen_transition.h"
#include "vinix/printk.h"

typedef struct {
    lv_obj_t *next_scr;
    int       delete_current;
} transition_ctx_t;

int is_transitioning = 0;

static transition_ctx_t ctx;

static void y_exec_cb(void *var, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)var, (lv_coord_t)v);
}

static void anim_done_cb(lv_anim_t *a)
{
    lv_obj_t *old_scr = (lv_obj_t *)a->var;
    lv_obj_t *next    = ctx.next_scr;
    int       do_del  = ctx.delete_current;

    ctx.next_scr       = NULL;
    ctx.delete_current = 0;
    is_transitioning   = 0;

    lv_scr_load(next);
    if (do_del)
        lv_obj_del(old_scr);

    pr_info("[TRANS] done\n");
}

void transition_to(lv_obj_t *next_scr, int delete_current)
{
    if (is_transitioning)
        return;

    lv_obj_t *cur = lv_scr_act();

    is_transitioning   = 1;
    ctx.next_scr       = next_scr;
    ctx.delete_current = delete_current;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, cur);
    lv_anim_set_exec_cb(&a, y_exec_cb);
    lv_anim_set_values(&a, 0, -480);        /* 480 px screen height */
    lv_anim_set_time(&a, 300);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_ready_cb(&a, anim_done_cb);
    lv_anim_start(&a);

    pr_info("[TRANS] start -> %p del=%d\n", next_scr, delete_current);
}
