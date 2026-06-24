/*
 * arch/arm/mm/uaccess.c - user-pointer validation for syscalls
 *
 * mmu_map_user() lays out three user regions in each task's private address
 * space: code at USER_CODE_VA, bss immediately above it, and the stack just
 * below sp_top. A legitimate user buffer lies entirely within one of them.
 * access_ok() enforces exactly that, so a syscall can never be tricked into
 * touching kernel or unmapped memory through a user-supplied pointer.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include <nothan/uaccess.h>
#include <nothan/sched.h>
#include <nothan/mm.h>

static bool range_in(unsigned long a, unsigned long size,
		     unsigned long start, unsigned long npages)
{
	unsigned long end;

	if (npages == 0)
		return false;
	end = start + npages * PAGE_SIZE;
	return a >= start && (a + size) <= end;
}

bool access_ok(const void *ptr, size_t size)
{
	struct mm_struct *mm = runqueue.curr ? runqueue.curr->mm : NULL;
	unsigned long a = (unsigned long)ptr;
	unsigned long code_start, bss_start, stack_start;

	if (!mm)		/* kernel thread — no user address space */
		return false;
	if (size == 0)
		return true;
	if (a + size < a)	/* address + length wraps around */
		return false;

	code_start  = USER_CODE_VA;
	bss_start   = code_start + (unsigned long)mm->code_pages * PAGE_SIZE;
	stack_start = mm->sp_top - (unsigned long)mm->stack_pages * PAGE_SIZE;

	if (range_in(a, size, code_start, mm->code_pages))
		return true;
	if (range_in(a, size, bss_start, mm->bss_pages))
		return true;
	if (range_in(a, size, stack_start, mm->stack_pages))
		return true;
	return false;
}

int copy_to_user(void *to, const void *from, size_t n)
{
	char *d = (char *)to;
	const char *s = (const char *)from;

	if (!access_ok(to, n))
		return -1;
	for (size_t i = 0; i < n; i++)
		d[i] = s[i];
	return 0;
}

int copy_from_user(void *to, const void *from, size_t n)
{
	char *d = (char *)to;
	const char *s = (const char *)from;

	if (!access_ok(from, n))
		return -1;
	for (size_t i = 0; i < n; i++)
		d[i] = s[i];
	return 0;
}

long strnlen_user(const char *s, long max)
{
	/*
	 * Validate one byte at a time: the string may start near the top of
	 * its region, so a NUL must appear before the region ends. The first
	 * failing access_ok() means we walked off the mapping without a NUL.
	 */
	for (long i = 0; i < max; i++) {
		if (!access_ok(s + i, 1))
			return -1;
		if (s[i] == '\0')
			return i;
	}
	return -1;	/* no NUL within max */
}
