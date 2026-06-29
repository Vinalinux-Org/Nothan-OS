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
