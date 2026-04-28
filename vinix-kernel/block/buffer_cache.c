/*
 * block/buffer_cache.c — write-back block cache
 *
 * Fixed-size array of BCACHE_BUFFERS slots.  On a miss, the least
 * recently used valid slot is evicted (LRU by last_used tick).
 * Dirty buffers are written back before eviction; bsync() flushes
 * all dirty buffers on demand.
 *
 * No reference counting: a buffer_head pointer returned by bread()
 * is valid only until the next bread() call that might evict the
 * same slot.  Callers must finish using it before calling bread()
 * again on the same device.
 */

#include "buffer_cache.h"
#include "string.h"
#include "uart.h"
#include "syscalls.h"

static struct buffer_head buffers[BCACHE_BUFFERS];
static uint32_t tick_counter = 0;
static uint32_t stat_hits   = 0;
static uint32_t stat_misses = 0;

void bcache_init(void)
{
    for (int i = 0; i < BCACHE_BUFFERS; i++) {
        buffers[i].bdev      = 0;
        buffers[i].lba       = 0;
        buffers[i].valid     = false;
        buffers[i].dirty     = false;
        buffers[i].last_used = 0;
    }
    tick_counter = 0;
    stat_hits = 0;
    stat_misses = 0;
    pr_info("[BCACHE] %d buffers x %d B (%d KB total)\n",
                BCACHE_BUFFERS, BCACHE_BLOCK_SIZE,
                (BCACHE_BUFFERS * BCACHE_BLOCK_SIZE) / 1024);
}

static struct buffer_head *find_cached(struct gendisk *bdev, uint32_t lba)
{
    for (int i = 0; i < BCACHE_BUFFERS; i++) {
        if (buffers[i].valid && buffers[i].bdev == bdev && buffers[i].lba == lba) {
            return &buffers[i];
        }
    }
    return 0;
}

static struct buffer_head *pick_victim(void)
{
    /* Prefer an empty slot; otherwise oldest last_used. */
    struct buffer_head *victim = &buffers[0];
    for (int i = 0; i < BCACHE_BUFFERS; i++) {
        if (!buffers[i].valid) return &buffers[i];
        if (buffers[i].last_used < victim->last_used) victim = &buffers[i];
    }
    return victim;
}

static int flush_buffer(struct buffer_head *bh)
{
    if (!bh->valid || !bh->dirty) return E_OK;
    int rc = blk_write(bh->bdev, bh->lba, 1, bh->data);
    if (rc == E_OK) bh->dirty = false;
    return rc;
}

struct buffer_head *bread(struct gendisk *bdev, uint32_t lba)
{
    struct buffer_head *bh = find_cached(bdev, lba);
    if (bh) {
        bh->last_used = ++tick_counter;
        stat_hits++;
        return bh;
    }

    stat_misses++;
    bh = pick_victim();
    if (bh->valid && bh->dirty) {
        if (flush_buffer(bh) != E_OK) {
            pr_info("[BCACHE] flush failed for lba %u\n", bh->lba);
            return 0;
        }
    }

    if (blk_read(bdev, lba, 1, bh->data) != E_OK) {
        bh->valid = false;
        return 0;
    }

    bh->bdev      = bdev;
    bh->lba       = lba;
    bh->valid     = true;
    bh->dirty     = false;
    bh->last_used = ++tick_counter;
    return bh;
}

void brelse(struct buffer_head *bh)
{
    /* MVP: no refcount. bread() returns a pointer good until the
     * buffer is evicted; callers must finish with it before the
     * next bread() that might touch the same slot. */
    (void)bh;
}

void bmark_dirty(struct buffer_head *bh)
{
    if (bh && bh->valid) bh->dirty = true;
}

void binvalidate(struct gendisk *bdev, uint32_t lba)
{
    struct buffer_head *bh = find_cached(bdev, lba);
    if (bh) {
        bh->valid = false;
        bh->dirty = false;
    }
}

int bsync(void)
{
    int last_err = E_OK;
    for (int i = 0; i < BCACHE_BUFFERS; i++) {
        if (buffers[i].valid && buffers[i].dirty) {
            int rc = flush_buffer(&buffers[i]);
            if (rc != E_OK) last_err = rc;
        }
    }
    return last_err;
}

uint32_t bcache_hits(void)   { return stat_hits; }
uint32_t bcache_misses(void) { return stat_misses; }
