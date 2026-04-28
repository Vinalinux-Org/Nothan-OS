/* ============================================================
 * printf.c — hand-written printf family built on vsnprintf().
 *
 * Format grammar (subset):
 *   %[-][0][width][l]{d,u,x,X,o,p,s,c,%}
 *
 *   -       left-align
 *   0       zero-pad (ignored if '-')
 *   width   decimal, 0..999
 *   l       'long' (treated the same as int — 32-bit target)
 *
 * No floating point, no %lld, no %n, no precision. Enough for
 * the shell and every utility on the roadmap.
 * ============================================================ */

#include "stdio.h"
#include "string.h"
#include "types.h"
#include "user_syscall.h"

/* Shared header — both sink types start with `count` so fmt_walk
 * reads the final length back without knowing which sink ran. */
struct ctx_hdr {
    size_t count;
};

struct out_ctx {
    size_t   count;
    char    *buf;
    size_t   cap;          /* 0 = unlimited sprintf */
};

/* Buffered fd writer — groups bytes so one sys_write covers many
 * format tokens instead of one syscall per character. */
struct fd_ctx {
    size_t count;
    int    fd;
    char   staging[128];
    size_t staged;
};

static void fd_flush(struct fd_ctx *c)
{
    if (c->staged == 0) return;
    if (c->fd == 1 || c->fd == 2) {
        sys_write(c->staging, (uint32_t)c->staged);
    } else {
        sys_write_file(c->fd, c->staging, (uint32_t)c->staged);
    }
    c->staged = 0;
}

typedef void (*emit_fn)(void *ctx, char ch);

static void emit_buf(void *ctx, char ch)
{
    struct out_ctx *c = (struct out_ctx *)ctx;
    if (c->cap == 0 || c->count + 1 < c->cap) {
        c->buf[c->count] = ch;
    }
    c->count++;
}

static void emit_fd(void *ctx, char ch)
{
    struct fd_ctx *c = (struct fd_ctx *)ctx;
    c->staging[c->staged++] = ch;
    c->count++;
    if (c->staged >= sizeof(c->staging)) fd_flush(c);
}

static int digit_to_char(uint32_t d)
{
    return (d < 10) ? (int)('0' + d) : (int)('a' + d - 10);
}

static int digit_to_char_upper(uint32_t d)
{
    return (d < 10) ? (int)('0' + d) : (int)('A' + d - 10);
}

/* Render `value` in base into tmp[], return its length. */
static int render_uint(uint32_t value, uint32_t base, bool upper, char *tmp)
{
    if (value == 0) { tmp[0] = '0'; return 1; }

    char rev[12];
    int n = 0;
    while (value > 0) {
        uint32_t d = value % base;
        rev[n++] = (char)(upper ? digit_to_char_upper(d) : digit_to_char(d));
        value /= base;
    }
    for (int i = 0; i < n; i++) tmp[i] = rev[n - 1 - i];
    return n;
}

static void pad(emit_fn emit, void *ctx, int count, char c)
{
    while (count-- > 0) emit(ctx, c);
}

static void emit_str(emit_fn emit, void *ctx, const char *s, int len)
{
    for (int i = 0; i < len; i++) emit(ctx, s[i]);
}

/* Core formatter — emits via the provided callback, which bumps
 * the ctx->count field. Return value is the total character count. */
static size_t fmt_walk(emit_fn emit, void *ctx, const char *fmt, va_list ap)
{
    const char *p = fmt;

    while (*p) {
        if (*p != '%') {
            emit(ctx, *p++);
            continue;
        }
        p++;  /* skip '%' */

        int left_align = 0;
        int zero_pad = 0;
        int width = 0;

        /* flags */
        while (*p == '-' || *p == '0') {
            if (*p == '-') left_align = 1;
            if (*p == '0') zero_pad = 1;
            p++;
        }
        if (left_align) zero_pad = 0;

        /* width */
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }

        /* length modifier — treat 'l' as no-op on a 32-bit target */
        if (*p == 'l') p++;

        char tmp[16];
        int len = 0;
        int spec = *p;

        switch (spec) {
        case 'd': {
            int32_t v = va_arg(ap, int32_t);
            bool neg = (v < 0);
            uint32_t uv = neg ? (uint32_t)(-v) : (uint32_t)v;
            len = render_uint(uv, 10, false, tmp);
            if (neg) {
                for (int i = len; i > 0; i--) tmp[i] = tmp[i - 1];
                tmp[0] = '-';
                len++;
            }
            break;
        }
        case 'u': len = render_uint(va_arg(ap, uint32_t), 10, false, tmp); break;
        case 'x': len = render_uint(va_arg(ap, uint32_t), 16, false, tmp); break;
        case 'X': len = render_uint(va_arg(ap, uint32_t), 16, true,  tmp); break;
        case 'o': len = render_uint(va_arg(ap, uint32_t),  8, false, tmp); break;
        case 'p': {
            uint32_t v = (uint32_t)(uintptr_t)va_arg(ap, void *);
            tmp[0] = '0'; tmp[1] = 'x';
            len = 2 + render_uint(v, 16, false, tmp + 2);
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int slen = (int)strlen(s);
            int space = width > slen ? width - slen : 0;
            if (!left_align) pad(emit, ctx, space, ' ');
            emit_str(emit, ctx, s, slen);
            if (left_align)  pad(emit, ctx, space, ' ');
            p++;
            continue;
        }
        case 'c': {
            char ch = (char)va_arg(ap, int);
            int space = width > 1 ? width - 1 : 0;
            if (!left_align) pad(emit, ctx, space, ' ');
            emit(ctx, ch);
            if (left_align)  pad(emit, ctx, space, ' ');
            p++;
            continue;
        }
        case '%':
            emit(ctx, '%');
            p++;
            continue;
        default:
            emit(ctx, '%');
            emit(ctx, (char)spec);
            if (spec) p++;
            continue;
        }

        /* Numeric path: apply width + padding to tmp[0..len). */
        int space = width > len ? width - len : 0;
        if (!left_align) {
            if (zero_pad) {
                /* Keep sign in front of the padding for negatives. */
                int start = 0;
                if (len > 0 && tmp[0] == '-') { emit(ctx, '-'); start = 1; }
                for (int i = 0; i < space; i++) emit(ctx, '0');
                emit_str(emit, ctx, tmp + start, len - start);
            } else {
                pad(emit, ctx, space, ' ');
                emit_str(emit, ctx, tmp, len);
            }
        } else {
            emit_str(emit, ctx, tmp, len);
            pad(emit, ctx, space, ' ');
        }
        p++;
    }
    return ((struct ctx_hdr *)ctx)->count;
}

/* ============================================================
 * Public API
 * ============================================================ */

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    struct out_ctx c = { .count = 0, .buf = buf, .cap = size };
    size_t n = fmt_walk(emit_buf, &c, fmt, ap);
    if (buf && size > 0) {
        buf[(n < size) ? n : size - 1] = '\0';
    }
    return (int)n;
}

int vsprintf(char *buf, const char *fmt, va_list ap)
{
    struct out_ctx c = { .count = 0, .buf = buf, .cap = 0 };
    size_t n = fmt_walk(emit_buf, &c, fmt, ap);
    if (buf) buf[n] = '\0';
    return (int)n;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = vsprintf(buf, fmt, ap);
    va_end(ap);
    return n;
}

/* printf() and vprintf() are weak so programs with redirect-aware
 * wrappers (the shell's built-in router) can override without
 * losing access to the rest of vinixlibc. */

__attribute__((weak))
int vprintf(const char *fmt, va_list ap)
{
    struct fd_ctx c = { .count = 0, .fd = 1, .staging = {0}, .staged = 0 };
    fmt_walk(emit_fd, &c, fmt, ap);
    fd_flush(&c);
    return (int)c.count;
}

__attribute__((weak))
int printf(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = vprintf(fmt, ap);
    va_end(ap);
    return n;
}

/* Routed to file.c's fprintf — a raw-fd variant for early callers. */
int vdprintf_fd(int fd, const char *fmt, va_list ap)
{
    struct fd_ctx c = { .count = 0, .fd = fd, .staging = {0}, .staged = 0 };
    fmt_walk(emit_fd, &c, fmt, ap);
    fd_flush(&c);
    return (int)c.count;
}
