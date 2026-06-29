/*
 * json.c — minimal JSON encoder / decoder for phone_daemon

 * Written by Bui Dinh Hien <buihien29112002@gmail.com>
 *
 * The encoder now escapes ", \, and control chars (\n \r \t) so that an SMS
 * body containing those characters still produces VALID JSON — otherwise the
 * frontend's JSON.parse() throws and the frame (e.g. an inbound SMS) is silently
 * dropped.  The decoder understands the matching escapes so commands carrying
 * such characters round-trip correctly.
 */

#include "json.h"
#include "../lib/types.h"
#include "../lib/string.h"
#include "../lib/printf.h"

#ifndef NULL
#  define NULL ((void *)0)
#endif

/* ==================================================================
 * Internal helpers
 * ================================================================== */

/*
 * Append src[0..n) to builder's buffer, clamping at the safe limit.
 * Always keeps buf null-terminated.
 */
static void jbuf_append(json_builder_t *b, const char *src, size_t n)
{
    size_t avail;

    if (b == NULL || src == NULL || b->pos >= b->size - 1)
        return;

    avail = b->size - 1 - b->pos;
    if (n > avail)
        n = avail;

    memcpy(b->buf + b->pos, src, n);
    b->pos += n;
    b->buf[b->pos] = '\0';
}

/* Append a single char (escape-aware callers handle the escaping). */
static void jbuf_putc(json_builder_t *b, char c)
{
    if (b == NULL || b->pos >= b->size - 1)
        return;
    b->buf[b->pos++] = c;
    b->buf[b->pos]   = '\0';
}

/* Append a JSON-escaped string value (without surrounding quotes). */
static void jbuf_append_escaped(json_builder_t *b, const char *s)
{
    if (b == NULL || s == NULL)
        return;
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        switch (c) {
            case '"':  jbuf_append(b, "\\\"", 2); break;
            case '\\': jbuf_append(b, "\\\\", 2); break;
            case '\n': jbuf_append(b, "\\n", 2);  break;
            case '\r': jbuf_append(b, "\\r", 2);  break;
            case '\t': jbuf_append(b, "\\t", 2);  break;
            default:
                if (c < 0x20) {
                    char u[8];
                    int n = snprintf(u, sizeof(u), "\\u%04x", c);
                    if (n > 0) {
                        jbuf_append(b, u, (size_t)n);
                    }
                } else {
                    jbuf_putc(b, (char)c);
                }
        }
    }
}

/* ==================================================================
 * Encoder
 * ================================================================== */

/**
 * json_begin() - Initialize a JSON builder
 * @b: Pointer to the JSON builder
 * @buf: Buffer to store the JSON string
 * @size: Size of the buffer
 */
void json_begin(json_builder_t *b, char *buf, size_t size)
{
    if (b == NULL || buf == NULL || size < 3)
        return;

    b->buf  = buf;
    b->size = size;
    b->pos  = 0;

    buf[0] = '{';
    buf[1] = '\0';
    b->pos = 1;
}

/**
 * json_str() - Append a string key-value pair to the JSON object
 * @b: Pointer to the JSON builder
 * @key: Key string
 * @val: Value string
 */
void json_str(json_builder_t *b, const char *key, const char *val)
{
    if (b == NULL || key == NULL || val == NULL)
        return;

    if (b->pos > 1)
        jbuf_putc(b, ',');
    jbuf_putc(b, '"');
    jbuf_append_escaped(b, key);
    jbuf_append(b, "\":\"", 3);
    jbuf_append_escaped(b, val);
    jbuf_putc(b, '"');
}

/**
 * json_int() - Append an integer key-value pair to the JSON object
 * @b: Pointer to the JSON builder
 * @key: Key string
 * @val: Integer value
 */
void json_int(json_builder_t *b, const char *key, int val)
{
    char tmp[128];
    int  n;

    if (b == NULL || key == NULL)
        return;

    if (b->pos > 1)
        n = snprintf(tmp, sizeof(tmp), ",\"%s\":%d", key, val);
    else
        n = snprintf(tmp, sizeof(tmp),  "\"%s\":%d", key, val);

    if (n > 0)
        jbuf_append(b, tmp, (size_t)n);
}

/**
 * json_end() - Finalize the JSON object
 * @b: Pointer to the JSON builder
 */
void json_end(json_builder_t *b)
{
    if (b == NULL || b->buf == NULL)
        return;
    if (b->pos < b->size - 1) {
        b->buf[b->pos++] = '}';
        b->buf[b->pos]   = '\0';
    }
}

/* ==================================================================
 * Decoder
 * ================================================================== */

/* Find `needle` at object level: the char before its opening quote must be
 * '{' or ',' — inside a string value the quote is always escaped (preceded
 * by '\\'), so this rejects false matches inside text content. */
static const char *json_find_key(const char *json, const char *needle)
{
    const char *p = json;
    size_t nlen = strlen(needle);
    while ((p = strstr(p, needle)) != NULL) {
        if (p > json && (p[-1] == '{' || p[-1] == ','))
            return p + nlen;
        p += 1;
    }
    return NULL;
}

/**
 * json_get_str() - Extract the string value for key from a JSON object
 * @json: The JSON string
 * @key: The key to search for
 * @out: Output buffer for the string value
 * @out_size: Size of the output buffer
 *
 * Searches for `"key":"` then copies the quoted value into out[out_size],
 * decoding \" \\ \n \r \t \/ and \uXXXX (BMP→UTF-8) escapes.
 *
 * Return: 0 on success, -1 if key not found or value unterminated.
 */
int json_get_str(const char *json, const char *key, char *out, size_t out_size)
{
    char        needle[128];
    const char *p;
    size_t      n;

    if (json == NULL || key == NULL || out == NULL || out_size == 0)
        return -1;

    snprintf(needle, sizeof(needle), "\"%s\":\"", key);

    p = json_find_key(json, needle);
    if (p == NULL)
        return -1;

    n = 0;
    while (n < out_size - 1 && *p && *p != '"') {
        if (*p == '\\' && p[1]) {
            p++;
            switch (*p) {
                case 'n': out[n++] = '\n'; break;
                case 'r': out[n++] = '\r'; break;
                case 't': out[n++] = '\t'; break;
                case 'b': out[n++] = '\b'; break;
                case 'f': out[n++] = '\f'; break;
                case '/': out[n++] = '/';  break;
                case '"': out[n++] = '"';  break;
                case '\\': out[n++] = '\\'; break;
                case 'u': {
                    /* \uXXXX → UTF-8 (BMP; surrogates emit '?' — no emoji in
                     * this use case). Each branch self-checks remaining space
                     * since the outer loop only guarantees room for 1 byte. */
                    unsigned v = 0; int i;
                    for (i = 0; i < 4 && p[1]; i++) {
                        char c = *++p;
                        v <<= 4;
                        if (c >= '0' && c <= '9') {
                            v |= (unsigned)(c - '0');
                        } else if (c >= 'a' && c <= 'f') {
                            v |= (unsigned)(c - 'a' + 10);
                        } else if (c >= 'A' && c <= 'F') {
                            v |= (unsigned)(c - 'A' + 10);
                        }
                    }
                    if (v >= 0xD800 && v <= 0xDFFF) {
                        if (n < out_size - 1) {
                            out[n++] = '?';
                        }
                    } else if (v < 0x80) {
                        if (n < out_size - 1) {
                            out[n++] = (char)v;
                        }
                    } else if (v < 0x800) {
                        if (n + 2 <= out_size - 1) {
                            out[n++] = (char)(0xC0 | (v >> 6));
                            out[n++] = (char)(0x80 | (v & 0x3F));
                        }
                    } else {
                        if (n + 3 <= out_size - 1) {
                            out[n++] = (char)(0xE0 | (v >> 12));
                            out[n++] = (char)(0x80 | ((v >> 6) & 0x3F));
                            out[n++] = (char)(0x80 | (v & 0x3F));
                        }
                    }
                    break;
                }
                default: out[n++] = *p; break;
            }
            p++;
        } else {
            out[n++] = *p++;
        }
    }
    out[n] = '\0';

    return (*p == '"') ? 0 : -1;
}

/**
 * json_get_int() - Extract the integer value for key from a JSON object
 * @json: The JSON string
 * @key: The key to search for
 * @out: Pointer to store the extracted integer
 *
 * Return: 0 on success, -1 if key not found.
 */
int json_get_int(const char *json, const char *key, int *out)
{
    char        needle[128];
    const char *p;

    if (json == NULL || key == NULL || out == NULL)
        return -1;

    snprintf(needle, sizeof(needle), "\"%s\":", key);

    p = json_find_key(json, needle);
    if (p == NULL)
        return -1;

    *out = atoi(p);
    return 0;
}
