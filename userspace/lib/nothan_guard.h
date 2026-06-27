/*
 * nothan_guard.h — guard-page allocator for the host over-run repro.
 *
 * AddressSanitizer red-zones are only ~16 bytes, so a FAR over-run (the
 * hardware Data Abort writes ~3.7 KB past the pool, into what on the host is
 * valid RAM) slips past them and stays silent. This reproduces the exact
 * hardware condition instead: a PROT_NONE guard page immediately after the
 * buffer, so a write ANY distance past the end faults at once (SIGSEGV) with
 * a symbolised backtrace — just like the unmapped page above __bss_end.
 *
 * The returned block is END-ALIGNED: [ptr, ptr+size) butts right up against
 * the guard, so the very first byte past the buffer traps. Used both as
 * LVGL's LV_MEM_POOL_ALLOC (the TLSF pool) and for the display port's render
 * and rotate buffers — the three large statics that live near __bss_end on
 * hardware.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#ifndef NOTHAN_GUARD_H
#define NOTHAN_GUARD_H

#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

static inline void *nothan_guarded_alloc(size_t size)
{
	const size_t pg = 4096;
	size_t rounded = (size + pg - 1) & ~(pg - 1);
	unsigned char *m = (unsigned char *)mmap(NULL, rounded + pg,
						 PROT_READ | PROT_WRITE,
						 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (m == MAP_FAILED)
		return NULL;
	/* Guard page right after the buffer — write past = instant fault. */
	mprotect(m + rounded, pg, PROT_NONE);
	/* End-align so [ret, ret+size) ends exactly at the guard. */
	return m + (rounded - size);
}

#endif /* NOTHAN_GUARD_H */
