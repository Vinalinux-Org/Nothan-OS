/*
 * lib/test/selftest.c — one-shot test harness
 *
 * Bundles subsystem self-tests under a unified pass/fail
 * counter and adds integration checks for procfs + buffer cache.
 */

#include "selftest.h"
#include "uart.h"
#include "assert.h"
#include "page_alloc.h"
#include "slab.h"
#include "buffer_cache.h"
#include "vinix/blkdev.h"
#include "procfs.h"
#include "vfs.h"
#include "syscalls.h"
#include "string.h"

/* Per-subsystem selftests run inside their _init() and panic on
 * failure. This harness covers integration paths with no other
 * obvious smoke test. */

static int test_bcache_hit(void)
{
    struct gendisk *bdev = get_gendisk("mmc0");
    if (!bdev) return -1;

    uint32_t miss0 = bcache_misses();

    /* First read misses; second read of the same LBA should hit. */
    struct buffer_head *a = bread(bdev, 8192);
    if (!a) return -2;
    brelse(a);

    uint32_t hit0 = bcache_hits();
    struct buffer_head *b = bread(bdev, 8192);
    if (!b) return -3;
    brelse(b);
    uint32_t hit1 = bcache_hits();

    if (hit1 <= hit0) return -4;
    (void)miss0;
    return 0;
}

static int test_procfs_content(void)
{
    struct vfs_operations *ops = procfs_init();
    if (!ops || !ops->lookup || !ops->read) return -1;

    int idx = ops->lookup("version");
    if (idx < 0) return -2;

    char buf[128];
    int n = ops->read(idx, 0, buf, sizeof(buf));
    if (n <= 0) return -3;

    /* Must start with "VinixOS" — regression tripwire if someone
     * breaks kvsnprintf or the gen_buf machinery. */
    if (n < 7 || memcmp(buf, "VinixOS", 7) != 0) return -4;
    return 0;
}



static const struct selftest tests[] = {
    { "bcache_hit",   test_bcache_hit   },
    { "procfs_read",  test_procfs_content },
};

#define NTESTS (int)(sizeof(tests) / sizeof(tests[0]))

void selftest_run_all(void)
{
    for (int i = 0; i < NTESTS; i++) {
        int rc = tests[i].run();
        if (rc != 0) {
            pr_info("[TEST] FAIL %s (rc=%d)\n", tests[i].name, rc);
            PANIC("selftest failure");
        }
    }
}
