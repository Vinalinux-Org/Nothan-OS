#ifndef _PRINTK_H
#define _PRINTK_H

#include <stdarg.h>
#include <nothan/time.h>

int printk(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));

int vsnprintf(char *buf, unsigned long size, const char *fmt, va_list args);

/*
 * Rate limiting, because the log is the only instrument this box has and it is
 * possible to break it by using it.
 *
 * A receive path that printed a summary every thirty-two frames was fine at
 * conversational rates and drowned at seventeen thousand: five hundred
 * summaries into a 115200 baud line, which cannot carry them, so lines came
 * out spliced through the middle of other lines.  The measurement destroyed
 * the thing it was being read from, at exactly the load worth reading about.
 *
 * The limit is in time rather than in events for the same reason.  "Every N
 * frames" is a rate that rises with traffic — it is loudest when the console
 * has least room — while "at most once every N milliseconds" is a rate the
 * wire can be checked against once and holds at any load.
 *
 * What is dropped is counted and reported by the next line that gets through.
 * A log that quietly prints less under load is a log that lies about load.
 */
struct ratelimit {
	unsigned long	interval;	/* jiffies between allowances */
	unsigned long	burst;		/* how many per interval */
	unsigned long	begin;
	unsigned long	printed;
	unsigned long	dropped;
	unsigned long	last_dropped;	/* dropped before the current line */
	int		started;
};

#define DEFINE_RATELIMIT(name, ms, n)					\
	static struct ratelimit name = {				\
		.interval = ((unsigned long)(ms) * HZ) / 1000UL,	\
		.burst    = (n),					\
	}

/*
 * May this message be printed?  When it returns non-zero, @last_dropped holds
 * how many were suppressed since the previous one — say so in the message.
 */
int ratelimit_allow(struct ratelimit *r);

#endif /* _PRINTK_H */
