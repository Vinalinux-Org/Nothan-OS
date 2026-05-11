/*
 * drivers/net/app/http.c — HTTP/1.0 server: static HTML response
 */

#include "uart.h"
#include "string.h"
#include "types.h"

static const char response[] =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html><body>"
    "<h1>VinixOS</h1>"
    "<p>Running on BeagleBone Black (AM3358)</p>"
    "</body></html>";

uint16_t http_rx(const unsigned char *req, uint16_t req_len,
                 unsigned char *resp, uint16_t resp_max)
{
    uint16_t len = (uint16_t)(sizeof(response) - 1);
    (void)req; (void)req_len;
    if (len > resp_max)
        len = resp_max;
    memcpy(resp, response, len);
    pr_info("[HTTP] GET / -> 200 OK\n");
    return len;
}
