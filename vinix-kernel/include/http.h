/*
 * include/http.h — HTTP/SSE application layer public interface
 */
#ifndef HTTP_H
#define HTTP_H
#include "types.h"

uint16_t http_rx(const unsigned char *req, uint16_t req_len,
                 unsigned char *resp, uint16_t resp_max,
                 int *out_keep_alive, int *out_conn_type);
uint16_t http_sse_frame(unsigned char *buf, uint16_t buf_max);
#endif
