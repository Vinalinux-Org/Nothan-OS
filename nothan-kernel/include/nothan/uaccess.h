#ifndef _NOTHAN_UACCESS_H
#define _NOTHAN_UACCESS_H

#include <nothan/types.h>

/*
 * User-space access boundary.
 *
 * The kernel runs syscalls with the calling task's TTBR0 active, so it CAN
 * dereference a user VA directly. The danger is that nothing stops a user
 * from handing the kernel a *kernel-space* (VA >= 0xC0000000) or unmapped
 * pointer and having the kernel read/write there on the user's behalf — e.g.
 * read(fd, (void *)0xC0001000, n) would scribble file data over the kernel.
 * access_ok() proves a range lies inside the caller's own mapped user
 * regions (code, bss, stack) before any such access.
 *
 * Scope limit, stated honestly: this validates that an address range is
 * legitimate user memory, NOT that a destination buffer is big enough for a
 * given count. A buffer's true size is known only to the caller; an
 * over-long count that still lands inside mapped user memory is a caller bug
 * the kernel cannot detect.
 */

/* True if [ptr, ptr+size) lies entirely within the current task's user map. */
bool access_ok(const void *ptr, size_t size);

/*
 * Copy @n bytes kernel<->user after an access_ok() check on the user side.
 * Return 0 on success, -1 if the user range is invalid.
 */
int copy_to_user(void *to, const void *from, size_t n);
int copy_from_user(void *to, const void *from, size_t n);

/*
 * Length of a NUL-terminated user string, capped at @max. Returns -1 if @s
 * is not valid user memory or no NUL is found before the end of its region
 * (or before @max) — i.e. the string is unterminated/runs out of bounds.
 */
long strnlen_user(const char *s, long max);

#endif /* _NOTHAN_UACCESS_H */
