/* ============================================================
 * sync_selftest.c
 * ------------------------------------------------------------
 * Single-threaded mechanics check for atomic + spinlock.
 * ============================================================ */

#include "atomic.h"
#include "spinlock.h"
#include "uart.h"
#include "types.h"

void sync_selftest(void)
{
    atomic_t a = ATOMIC_INIT(0);

    for (int i = 0; i < 1000; i++)
    {
        atomic_inc(&a);
    }
    if (atomic_read(&a) != 1000)
    {
        pr_info("[SYNC] FAIL: atomic_inc x1000 -> %d\n", atomic_read(&a));
        return;
    }
    for (int i = 0; i < 500; i++)
    {
        atomic_dec(&a);
    }
    if (atomic_read(&a) != 500)
    {
        pr_info("[SYNC] FAIL: atomic_dec x500 -> %d\n", atomic_read(&a));
        return;
    }

    int32_t old = atomic_cmpxchg(&a, 500, 777);
    if (old != 500 || atomic_read(&a) != 777)
    {
        pr_info("[SYNC] FAIL: cmpxchg(500,777) old=%d cur=%d\n",
                    old, atomic_read(&a));
        return;
    }
    old = atomic_cmpxchg(&a, 500, 0);
    if (old != 777 || atomic_read(&a) != 777)
    {
        pr_info("[SYNC] FAIL: cmpxchg wrong-expect old=%d cur=%d\n",
                    old, atomic_read(&a));
        return;
    }

    spinlock_t lock = SPINLOCK_INIT;
    spin_lock(&lock);
    if (lock.locked != 1)
    {
        pr_info("[SYNC] FAIL: spin_lock did not mark locked=1\n");
        return;
    }
    spin_unlock(&lock);
    if (lock.locked != 0)
    {
        pr_info("[SYNC] FAIL: spin_unlock did not clear locked\n");
        return;
    }

    uint32_t flags = spin_lock_irqsave(&lock);
    spin_unlock_irqrestore(&lock, flags);
}
