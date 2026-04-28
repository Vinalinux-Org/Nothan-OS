/*
 * include/vinix/mmc/host.h — MMC host controller interface
 *
 * Drivers fill struct mmc_host and struct mmc_host_ops,
 * then call mmc_add_host() to register with the MMC core.
 */

#ifndef VINIX_MMC_HOST_H
#define VINIX_MMC_HOST_H

#include "types.h"

struct mmc_host;
struct mmc_command;
struct mmc_data;

struct mmc_request {
    struct mmc_command *cmd;
    struct mmc_data    *data;
};

struct mmc_host_ops {
    void (*request)   (struct mmc_host *host, struct mmc_request *mrq);
    int  (*get_cd)    (struct mmc_host *host);   /* card detect */
    int  (*get_ro)    (struct mmc_host *host);   /* read-only */
    void (*set_ios)   (struct mmc_host *host);
};

struct mmc_host {
    const char                  *name;
    void                        *priv;
    const struct mmc_host_ops   *ops;
    uint32_t                     f_min;     /* Hz */
    uint32_t                     f_max;
    uint32_t                     ocr_avail;
};

struct mmc_host *mmc_alloc_host(int extra_priv, const char *name);
int              mmc_add_host  (struct mmc_host *host);
void             mmc_remove_host(struct mmc_host *host);

/* mmc -> block bridge. Host driver registers a disk by passing
 * its sector I/O callbacks; mmc_block builds the gendisk and
 * calls add_disk. total_sectors=0 means unknown (MVP). */
int mmc_block_register(struct mmc_host *host,
                       int (*read_fn) (uint32_t lba, uint32_t count, void *),
                       int (*write_fn)(uint32_t lba, uint32_t count, const void *),
                       uint32_t total_sectors);

#endif /* VINIX_MMC_HOST_H */
