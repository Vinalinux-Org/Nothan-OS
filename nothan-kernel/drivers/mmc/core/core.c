/*
 * drivers/mmc/core/core.c — MMC host registry
 *
 * Host controller drivers call mmc_alloc_host() to obtain a slot,
 * fill the ops vtable, then call mmc_add_host() to expose it.
 * Card identification (CMD0/CMD8/ACMD41/...) currently lives in
 * the host driver; this file owns only the host registry.
 */

#include "nothan/mmc/host.h"
#include "nothan/printk.h"
#include "nothan/errno.h"
#include "string.h"

#define MAX_MMC_HOSTS 2

static struct mmc_host hosts[MAX_MMC_HOSTS];
static int             num_hosts = 0;

struct mmc_host *mmc_alloc_host(int extra_priv, const char *name)
{
    (void)extra_priv;        /* MVP: priv attached separately by driver */

    if (num_hosts >= MAX_MMC_HOSTS) return 0;

    struct mmc_host *h = &hosts[num_hosts];
    h->name = name;
    h->priv = 0;
    h->ops  = 0;
    return h;
}

int mmc_add_host(struct mmc_host *host)
{
    if (!host) return -EINVAL;

    /* host already came from our static pool; just commit the slot. */
    num_hosts++;
    pr_info("[MMC] host %s registered\n", host->name ? host->name : "?");
    return 0;
}

void mmc_remove_host(struct mmc_host *host)
{
    (void)host;        /* MVP: no removal — drivers compile-static */
}
