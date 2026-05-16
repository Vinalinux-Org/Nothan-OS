/* ============================================================
 * stdlib.c — numeric conversion + K&R-style malloc/free over a
 * fixed pool. No sbrk / no mmap yet.
 *
 * malloc layout (pattern reference: K&R §8.7 — re-implemented):
 *   Each block carries a header { size_in_units, next_free }.
 *   The free list is a circular singly-linked list kept sorted
 *   by address so adjacent free blocks can be coalesced in O(1).
 * ============================================================ */

#include "stdlib.h"
#include "string.h"
#include "ctype.h"
#include "user_syscall.h"

/* ============================================================
 * Integer parsing
 * ============================================================ */

int abs(int v) { return v < 0 ? -v : v; }

int atoi(const char *s)  { return (int)strtol(s, 0, 10); }
long atol(const char *s) { return      strtol(s, 0, 10); }

long strtol(const char *s, char **endptr, int base)
{
    while (isspace((int)*s)) s++;

    int sign = 1;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;

    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2; base = 16;
    } else if (base == 0 && s[0] == '0') {
        s++; base = 8;
    } else if (base == 0) {
        base = 10;
    }

    long value = 0;
    while (*s) {
        int c = (int)(unsigned char)*s;
        int digit;
        if (isdigit(c))      digit = c - '0';
        else if (isalpha(c)) digit = (tolower(c) - 'a') + 10;
        else                 break;
        if (digit >= base)   break;
        value = value * base + digit;
        s++;
    }

    if (endptr) *endptr = (char *)s;
    return sign * value;
}

/* Convert value to string in `buf`. Supports base 2..36. Only
 * base 10 honours the sign. Returns buf. */
char *itoa(int value, char *buf, int base)
{
    char tmp[34];
    int neg = (base == 10 && value < 0);
    unsigned long v = neg ? (unsigned long)(-(long)value) : (unsigned long)(unsigned int)value;
    int n = 0;

    if (v == 0) tmp[n++] = '0';
    while (v > 0) {
        int d = (int)(v % (unsigned long)base);
        tmp[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (unsigned long)base;
    }
    if (neg) tmp[n++] = '-';

    int i = 0;
    while (n > 0) buf[i++] = tmp[--n];
    buf[i] = '\0';
    return buf;
}

/* ============================================================
 * malloc / free — fixed 64 KB pool, K&R-style free list.
 * ============================================================ */

#define POOL_BYTES (64u * 1024u)

typedef struct header {
    size_t          size_units;  /* including this header */
    struct header  *next;        /* next free block */
} header_t;

static uint8_t   pool[POOL_BYTES] __attribute__((aligned(8)));
static header_t *free_list = 0;
static bool      initialized = false;

static void pool_init(void)
{
    size_t total_units = POOL_BYTES / sizeof(header_t);
    header_t *h = (header_t *)pool;
    h->size_units = total_units;
    h->next = h;  /* circular — single block covering the whole pool */
    free_list = h;
    initialized = true;
}

void *malloc(size_t nbytes)
{
    if (nbytes == 0) return 0;
    if (!initialized) pool_init();

    size_t need = (nbytes + sizeof(header_t) - 1) / sizeof(header_t) + 1;
    header_t *prev = free_list;

    for (header_t *p = prev->next; ; prev = p, p = p->next) {
        if (p->size_units >= need) {
            if (p->size_units == need) {
                /* Exact fit — unlink whole block. */
                prev->next = p->next;
            } else {
                /* Split: shrink p, hand out its tail. */
                p->size_units -= need;
                p += p->size_units;
                p->size_units = need;
            }
            free_list = prev;
            return (void *)(p + 1);
        }
        if (p == free_list) {
            /* Walked the whole ring — out of space. */
            return 0;
        }
    }
}

void free(void *ptr)
{
    if (!ptr) return;

    header_t *bp = (header_t *)ptr - 1;
    header_t *p  = free_list;

    /* Find insertion point — free list stays ordered by address. */
    while (!(bp > p && bp < p->next)) {
        if (p >= p->next && (bp > p || bp < p->next)) break;
        p = p->next;
    }

    /* Coalesce with block after. */
    if (bp + bp->size_units == p->next) {
        bp->size_units += p->next->size_units;
        bp->next        = p->next->next;
    } else {
        bp->next = p->next;
    }

    /* Coalesce with block before. */
    if (p + p->size_units == bp) {
        p->size_units += bp->size_units;
        p->next        = bp->next;
    } else {
        p->next = bp;
    }

    free_list = p;
}

void *calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, size_t size)
{
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return 0; }

    header_t *bp = (header_t *)ptr - 1;
    size_t old_bytes = (bp->size_units - 1) * sizeof(header_t);

    void *n = malloc(size);
    if (!n) return 0;
    memcpy(n, ptr, size < old_bytes ? size : old_bytes);
    free(ptr);
    return n;
}

/* ============================================================
 * exit
 * ============================================================ */

void exit(int status)
{
    sys_exit(status);
    while (1) { }  /* unreachable */
}
