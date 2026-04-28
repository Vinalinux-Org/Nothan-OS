/* ============================================================
 * format.c — kernel snprintf. Smaller cousin of vinixlibc's
 * printf engine, but consumers are limited (procfs + selftest),
 * so no stream/fd sink — only buffer output.
 *
 * Grammar: %[-][0][width][l]{d,u,x,X,o,p,s,c,%}
 * No precision, no floating point.
 * ============================================================ */

#include "format.h"
#include "string.h"

static void put(char *buf, size_t cap, size_t *pos, char ch)
{
    if (buf && (cap == 0 || *pos + 1 < cap)) buf[*pos] = ch;
    (*pos)++;
}

static int render_uint(uint32_t v, uint32_t base, int upper, char *tmp)
{
    if (v == 0) { tmp[0] = '0'; return 1; }
    char rev[12];
    int n = 0;
    while (v > 0) {
        uint32_t d = v % base;
        rev[n++] = (char)(d < 10 ? '0' + d : (upper ? 'A' : 'a') + d - 10);
        v /= base;
    }
    for (int i = 0; i < n; i++) tmp[i] = rev[n - 1 - i];
    return n;
}

int kvsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    size_t pos = 0;
    const char *p = fmt;

    while (*p) {
        if (*p != '%') { put(buf, size, &pos, *p++); continue; }
        p++;

        int left = 0, zero = 0, width = 0;
        while (*p == '-' || *p == '0') {
            if (*p == '-') left = 1;
            if (*p == '0') zero = 1;
            p++;
        }
        if (left) zero = 0;
        while (*p >= '0' && *p <= '9') { width = width * 10 + (*p - '0'); p++; }
        if (*p == 'l') p++;  /* long on 32-bit target = same */

        char tmp[16];
        int len = 0;
        int spec = *p;

        switch (spec) {
        case 'd': {
            int32_t v = va_arg(ap, int32_t);
            int neg = v < 0;
            uint32_t uv = neg ? (uint32_t)(-v) : (uint32_t)v;
            len = render_uint(uv, 10, 0, tmp);
            if (neg) {
                for (int i = len; i > 0; i--) tmp[i] = tmp[i - 1];
                tmp[0] = '-'; len++;
            }
            break;
        }
        case 'u': len = render_uint(va_arg(ap, uint32_t), 10, 0, tmp); break;
        case 'x': len = render_uint(va_arg(ap, uint32_t), 16, 0, tmp); break;
        case 'X': len = render_uint(va_arg(ap, uint32_t), 16, 1, tmp); break;
        case 'o': len = render_uint(va_arg(ap, uint32_t),  8, 0, tmp); break;
        case 'p': {
            uint32_t v = (uint32_t)(uintptr_t)va_arg(ap, void *);
            tmp[0] = '0'; tmp[1] = 'x';
            len = 2 + render_uint(v, 16, 0, tmp + 2);
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int slen = (int)strlen(s);
            int space = width > slen ? width - slen : 0;
            if (!left) for (int i = 0; i < space; i++) put(buf, size, &pos, ' ');
            for (int i = 0; i < slen; i++) put(buf, size, &pos, s[i]);
            if (left)  for (int i = 0; i < space; i++) put(buf, size, &pos, ' ');
            p++;
            continue;
        }
        case 'c':
            put(buf, size, &pos, (char)va_arg(ap, int));
            p++;
            continue;
        case '%':
            put(buf, size, &pos, '%');
            p++;
            continue;
        default:
            put(buf, size, &pos, '%');
            put(buf, size, &pos, (char)spec);
            if (spec) p++;
            continue;
        }

        int space = width > len ? width - len : 0;
        if (!left) {
            if (zero) {
                int start = 0;
                if (len > 0 && tmp[0] == '-') { put(buf, size, &pos, '-'); start = 1; }
                for (int i = 0; i < space; i++) put(buf, size, &pos, '0');
                for (int i = start; i < len; i++) put(buf, size, &pos, tmp[i]);
            } else {
                for (int i = 0; i < space; i++) put(buf, size, &pos, ' ');
                for (int i = 0; i < len; i++) put(buf, size, &pos, tmp[i]);
            }
        } else {
            for (int i = 0; i < len; i++) put(buf, size, &pos, tmp[i]);
            for (int i = 0; i < space; i++) put(buf, size, &pos, ' ');
        }
        p++;
    }

    if (buf && size > 0) {
        buf[(pos < size) ? pos : size - 1] = '\0';
    }
    return (int)pos;
}

int ksnprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = kvsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}
