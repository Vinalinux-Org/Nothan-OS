/* ============================================================
 * vinix/printk.h
 * ------------------------------------------------------------
 * printk() — kernel log entry point. KERN_* loglevel prefix
 * strings are empty today; a future printk dispatcher can parse
 * the leading "<N>" to filter by level.
 *
 * Implementation lives in kernel/printk/printk.c. Drivers should
 * prefer pr_info / pr_err / pr_warn / pr_debug to plain printk.
 * ============================================================ */

#ifndef VINIX_PRINTK_H
#define VINIX_PRINTK_H

void printk(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#define KERN_EMERG   ""
#define KERN_ALERT   ""
#define KERN_CRIT    ""
#define KERN_ERR     ""
#define KERN_WARN    ""
#define KERN_NOTICE  ""
#define KERN_INFO    ""
#define KERN_DEBUG   ""

#define pr_emerg(fmt, ...)       printk(KERN_EMERG  fmt, ##__VA_ARGS__)
#define pr_alert(fmt, ...)       printk(KERN_ALERT  fmt, ##__VA_ARGS__)
#define pr_crit(fmt, ...)        printk(KERN_CRIT   fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)         printk(KERN_ERR    fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...)        printk(KERN_WARN   fmt, ##__VA_ARGS__)
#define pr_notice(fmt, ...)      printk(KERN_NOTICE fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...)        printk(KERN_INFO   fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...)       printk(KERN_DEBUG  fmt, ##__VA_ARGS__)

#endif /* VINIX_PRINTK_H */
