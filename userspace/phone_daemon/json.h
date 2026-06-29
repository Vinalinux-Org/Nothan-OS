/*
 * json.h — minimal JSON encoder/decoder for phone_daemon
 *
 * Encoder: builder pattern using a json_builder_t.
 *   json_begin(&b, buf, size) → json_str/json_int (N times) → json_end(&b)
 *   b.buf is always null-terminated after each call.
 *
 * Decoder: strstr-based key scanner.
 *   json_get_str(json, "key", out, size) → 0 ok / -1 not found
 *   json_get_int(json, "key", &val)      → 0 ok / -1 not found
 *
 * Limitations: values must not contain '"' (no escape handling).
 * This is sufficient for all SIM7600CE ↔ RPi4 message types.
 */

#ifndef PHONE_JSON_H
#define PHONE_JSON_H

#include "../lib/types.h"

typedef struct {
    char   *buf;
    size_t  size;
    size_t  pos;
} json_builder_t;

/* Encoder */
void json_begin(json_builder_t *b, char *buf, size_t size);
void json_str(json_builder_t *b, const char *key, const char *val);
void json_int(json_builder_t *b, const char *key, int val);
void json_end(json_builder_t *b);

/* Decoder */
int json_get_str(const char *json, const char *key, char *out, size_t out_size);
int json_get_int(const char *json, const char *key, int *out);

#endif /* PHONE_JSON_H */
