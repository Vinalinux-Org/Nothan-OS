/*
 * drivers/net/app/http.c — HTTP/1.1 server: URL routing, dynamic dashboard, SSE
 */

#include "uart.h"
#include "string.h"
#include "types.h"
#include "sleep.h"
#include "page_alloc.h"
#include "cpustat.h"

#define HDR_RESERVE     110

#define CONN_TYPE_HTTP  0
#define CONN_TYPE_SSE   1

static unsigned char *buf_ptr;
static uint16_t       buf_pos;
static uint16_t       buf_max;

static void ap(const char *s)
{
    while (*s && buf_pos < buf_max)
        buf_ptr[buf_pos++] = (unsigned char)*s++;
}

static void apn_clean(uint32_t v)
{
    char tmp[12];
    int  len = 0, i;
    if (v == 0) { ap("0"); return; }
    while (v) { tmp[len++] = '0' + (v % 10); v /= 10; }
    for (i = len - 1; i >= 0; i--) {
        if (buf_pos < buf_max)
            buf_ptr[buf_pos++] = tmp[i];
    }
}

static void ap2(uint32_t v)
{
    if (buf_pos < buf_max) buf_ptr[buf_pos++] = '0' + (v / 10);
    if (buf_pos < buf_max) buf_ptr[buf_pos++] = '0' + (v % 10);
}

static void ap_uptime(void)
{
    uint32_t s  = jiffies / 100;
    uint32_t h  = s / 3600;
    uint32_t m  = (s % 3600) / 60;
    uint32_t sc = s % 60;
    ap2(h); ap(":"); ap2(m); ap(":"); ap2(sc);
}

static uint32_t mem_used_pct(void)
{
    uint32_t total = page_alloc_total_pages();
    uint32_t free  = page_alloc_free_pages();
    if (total == 0) return 0;
    /* page_alloc tracks dynamic heap only; add 16MB static overhead */
    uint32_t static_pages = (16 * 1024 * 1024) / 4096;
    uint32_t used = (total - free) + static_pages;
    if (used > total) used = total;
    return (used * 100) / total;
}

static void ap_mem_pct(void) { apn_clean(mem_used_pct()); }
static void ap_mem_bar(void) { apn_clean(mem_used_pct()); }

static void generate_html(void)
{
    ap("<!DOCTYPE html><html lang=en><head>"
       "<meta charset=UTF-8>"
       "<meta name=viewport content='width=device-width,initial-scale=1'>"
       "<title>VinixOS Dashboard</title>"
       "<style>"
       "*{margin:0;padding:0;box-sizing:border-box}"
       "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;"
           "background:#0d1117;color:#e6edf3;display:flex;height:100vh;overflow:hidden}"
       ".sidebar{width:210px;background:#161b22;border-right:1px solid #30363d;"
           "display:flex;flex-direction:column}"
       ".dev-info{padding:20px 16px;border-bottom:1px solid #30363d}"
       ".dot{display:inline-block;width:8px;height:8px;background:#3fb950;"
           "border-radius:50%;margin-right:8px}"
       ".dev-name{font-size:14px;font-weight:600}"
       ".dev-sub{font-size:12px;color:#8b949e;margin-top:4px;padding-left:16px}"
       ".nav-grp{padding:14px 16px 6px;font-size:11px;font-weight:600;"
           "color:#8b949e;letter-spacing:.6px}"
       ".nav-item{display:flex;align-items:center;gap:9px;padding:8px 12px;"
           "margin:2px 8px;border-radius:6px;font-size:13px;color:#8b949e}"
       ".nav-item.active{background:#1f3d2e;color:#3fb950;font-weight:500}"
       ".sidebar-foot{margin-top:auto;padding:14px 16px;font-size:11px;"
           "color:#8b949e;border-top:1px solid #30363d}"
       ".main{flex:1;overflow-y:auto;padding:24px}"
       ".hdr{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px}"
       ".hdr-title{font-size:20px;font-weight:600}"
       ".metrics{display:grid;grid-template-columns:repeat(4,1fr);gap:12px;margin-bottom:16px}"
       ".mc{background:#161b22;border:1px solid #30363d;border-radius:8px;padding:16px}"
       ".mc-lbl{font-size:12px;color:#8b949e;margin-bottom:8px}"
       ".mc-val{font-size:28px;font-weight:700}"
       ".mc-val span{font-size:14px;font-weight:400}"
       ".mc-val.g{color:#3fb950}"
       ".mc-val.b{color:#58a6ff}"
       ".mc-val.ip{font-size:15px;color:#3fb950;margin-top:4px}"
       ".bar{height:3px;background:#21262d;border-radius:2px;margin-top:10px}"
       ".bar-fill{height:100%;border-radius:2px}"
       ".bar-fill.g{background:#3fb950}"
       ".bar-fill.b{background:#58a6ff}"
       ".card{background:#161b22;border:1px solid #30363d;border-radius:8px;"
           "padding:20px;margin-bottom:14px}"
       ".card-title{font-size:14px;font-weight:600;margin-bottom:16px;"
           "display:flex;align-items:center;gap:8px;color:#e6edf3}"
       ".gpio-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}"
       ".gpio-item{background:#0d1117;border:1px solid #30363d;border-radius:8px;padding:16px}"
       ".gpio-name{font-size:13px;font-weight:600;margin-bottom:3px}"
       ".gpio-pin{font-size:11px;color:#8b949e;margin-bottom:12px}"
       ".gpio-row{display:flex;justify-content:space-between;align-items:center}"
       ".gpio-st{font-size:12px;font-weight:600;color:#6e7681}"
       ".tbl{width:100%;border-collapse:collapse}"
       ".tbl tr{border-bottom:1px solid #21262d}"
       ".tbl tr:last-child{border-bottom:none}"
       ".tbl td{padding:10px 0;font-size:13px}"
       ".tbl td:first-child{color:#8b949e;width:42%}"
       ".tbl td:last-child{text-align:right;font-weight:500}"
       ".badge{display:inline-block;padding:2px 10px;border-radius:12px;"
           "font-size:11px;font-weight:600}"
       ".badge.g{background:#1f3d2e;color:#3fb950}"
       ".badge.b{background:#1c2d3d;color:#58a6ff}"
       "</style>"
       "<script>"
       "var es=new EventSource('/events');"
       "es.onmessage=function(e){"
           "var d=JSON.parse(e.data);"
           "var u=d.uptime,h=Math.floor(u/3600),m=Math.floor((u%3600)/60),s=u%60;"
           "document.getElementById('up').textContent="
               "(h<10?'0':'')+h+':'+(m<10?'0':'')+m+':'+(s<10?'0':'')+s;"
           "document.getElementById('mem').textContent=d.mem_pct;"
           "document.getElementById('membar').style.width=d.mem_pct+'%';"
           "document.getElementById('cpu').textContent=d.cpu_pct;"
           "document.getElementById('cpubar').style.width=d.cpu_pct+'%';"
       "};"
       "</script>"
       "</head><body>"

       "<div class=sidebar>"
         "<div class=dev-info>"
           "<div><span class=dot></span><span class=dev-name>BBB-Device-01</span></div>"
           "<div class=dev-sub>BeagleBone Black</div>"
         "</div>"
         "<div class=nav-grp>MONITOR</div>"
         "<div class='nav-item active'>&#9632; Overview</div>"
         "<div class=nav-grp>SYSTEM</div>"
         "<div class=nav-item>&#9711; Network</div>"
         "<div class=nav-item>&#9881; Settings</div>"
         "<div class=sidebar-foot>Bare Metal OS v1.0</div>"
       "</div>"

       "<div class=main>"
         "<div class=hdr>"
           "<div class=hdr-title>System overview</div>"
         "</div>"

         "<div class=metrics>"
           "<div class=mc>"
             "<div class=mc-lbl>CPU usage</div>"
             "<div class='mc-val g'><span id=cpu>--</span><span>%</span></div>"
             "<div class=bar><div class='bar-fill g' id=cpubar style='width:0%'></div></div>"
           "</div>"
           "<div class=mc>"
             "<div class=mc-lbl>Memory</div>"
             "<div class='mc-val b'><span id=mem>");
    ap_mem_pct();
    ap("</span><span>%</span></div>"
             "<div class=bar><div class='bar-fill b' id=membar style='width:");
    ap_mem_bar();
    ap("%'></div></div>"
           "</div>"
           "<div class=mc>"
             "<div class=mc-lbl>Uptime</div>"
             "<div class=mc-val style='font-size:22px' id=up>");
    ap_uptime();
    ap("</div>"
           "</div>"
           "<div class=mc>"
             "<div class=mc-lbl>IP address</div>"
             "<div class='mc-val ip'>192.168.2.100</div>"
           "</div>"
         "</div>"

         "<div class=card>"
           "<div class=card-title>&#9889; GPIO control</div>"
           "<div class=gpio-grid>"
             "<div class=gpio-item>"
               "<div class=gpio-name>LED 1</div>"
               "<div class=gpio-pin>GPIO P8_12</div>"
               "<div class=gpio-row><span class=gpio-st>LOW</span></div>"
             "</div>"
             "<div class=gpio-item>"
               "<div class=gpio-name>LED 2</div>"
               "<div class=gpio-pin>GPIO P8_14</div>"
               "<div class=gpio-row><span class=gpio-st>LOW</span></div>"
             "</div>"
             "<div class=gpio-item>"
               "<div class=gpio-name>Relay</div>"
               "<div class=gpio-pin>GPIO P9_15</div>"
               "<div class=gpio-row><span class=gpio-st>LOW</span></div>"
             "</div>"
           "</div>"
         "</div>"

         "<div class=card>"
           "<div class=card-title>&#9432; System info</div>"
           "<table class=tbl>"
             "<tr><td>Board</td><td>BeagleBone Black Rev C</td></tr>"
             "<tr><td>CPU</td><td>AM335x Cortex-A8 @ 1GHz</td></tr>"
             "<tr><td>RAM total</td><td>256 MB DDR3</td></tr>"
             "<tr><td>OS</td><td><span class='badge b'>Bare Metal</span></td></tr>"
             "<tr><td>Web server</td><td><span class='badge g'>Running</span></td></tr>"
             "<tr><td>MAC address</td><td>98:89:24:75:E6:3E</td></tr>"
           "</table>"
         "</div>"
       "</div>"
       "</body></html>");
}

/* ── request parser ──────────────────────────────────────────────── */

static void parse_req(const unsigned char *data, uint16_t len,
                      char *path_out, uint16_t path_max,
                      int *keep_alive_out)
{
    uint16_t i = 0, plen = 0;

    *keep_alive_out = 0;
    path_out[0] = '/';
    path_out[1] = '\0';

    while (i < len && data[i] != ' ') i++;  /* skip method */
    if (i >= len) return;
    i++;

    while (i < len && data[i] != ' ' && data[i] != '\r') {
        if (plen < path_max - 1) path_out[plen++] = (char)data[i];
        i++;
    }
    path_out[plen] = '\0';

    for (; i + 11 < len; i++) {
        if (data[i]  !='C'||data[i+1]!='o'||data[i+2]!='n'||data[i+3]!='n'||
            data[i+4]!='e'||data[i+5]!='c'||data[i+6]!='t'||data[i+7]!='i'||
            data[i+8]!='o'||data[i+9]!='n'||data[i+10]!=':') continue;
        uint16_t j = i + 11;
        while (j < len && data[j] == ' ') j++;
        if (j + 10 <= len &&
            data[j]=='k'&&data[j+1]=='e'&&data[j+2]=='e'&&data[j+3]=='p'&&
            data[j+4]=='-'&&data[j+5]=='a'&&data[j+6]=='l'&&data[j+7]=='i'&&
            data[j+8]=='v'&&data[j+9]=='e')
            *keep_alive_out = 1;
        break;
    }
}

/* ── shared header + body framer ─────────────────────────────────── */

static uint16_t build_response(unsigned char *resp, uint16_t body_len,
                                const char *ctype, int keep_alive)
{
    char     hdr[HDR_RESERVE];
    int      n = 0, tlen, j;
    uint32_t v;
    char     tmp[12];
    const char *p;

    for (p = "HTTP/1.1 200 OK\r\nContent-Type: "; *p; ) hdr[n++] = *p++;
    for (p = ctype; *p; ) hdr[n++] = *p++;
    for (p = "\r\nContent-Length: "; *p; ) hdr[n++] = *p++;

    v = body_len; tlen = 0;
    if (!v) { tmp[tlen++] = '0'; }
    else { while (v) { tmp[tlen++] = '0' + (v % 10); v /= 10; } }
    for (j = tlen - 1; j >= 0; j--) hdr[n++] = tmp[j];

    for (p = keep_alive ? "\r\nConnection: keep-alive\r\n\r\n"
                        : "\r\nConnection: close\r\n\r\n"; *p; ) hdr[n++] = *p++;

    if ((uint16_t)n > HDR_RESERVE) { pr_err("[HTTP] HDR_RESERVE overflow\n"); return 0; }

    for (j = 0; j < (int)body_len; j++) resp[n + j] = resp[HDR_RESERVE + j];
    for (j = 0; j < n; j++) resp[j] = (unsigned char)hdr[j];
    return (uint16_t)n + body_len;
}

/* ── route handlers ──────────────────────────────────────────────── */

static uint16_t handle_root(unsigned char *resp, uint16_t resp_max, int ka)
{
    buf_ptr = resp + HDR_RESERVE;
    buf_pos = 0;
    buf_max = resp_max - HDR_RESERVE;
    generate_html();
    return build_response(resp, buf_pos, "text/html", ka);
}

static uint16_t handle_api_stats(unsigned char *resp, uint16_t resp_max, int ka)
{
    buf_ptr = resp + HDR_RESERVE;
    buf_pos = 0;
    buf_max = resp_max - HDR_RESERVE;
    ap("{\"uptime\":"); apn_clean(jiffies / 100);
    ap(",\"mem_pct\":"); apn_clean(mem_used_pct());
    ap(",\"cpu_pct\":"); apn_clean(cpustat_pct());
    ap("}");
    return build_response(resp, buf_pos, "application/json", ka);
}

static uint16_t handle_sse_open(unsigned char *resp, uint16_t resp_max)
{
    const char *s = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/event-stream\r\n"
                    "Cache-Control: no-cache\r\n"
                    "Connection: keep-alive\r\n"
                    "Access-Control-Allow-Origin: *\r\n\r\n";
    uint16_t len = 0;
    while (*s && len < resp_max) resp[len++] = (unsigned char)*s++;
    return len;
}

static uint16_t handle_404(unsigned char *resp, uint16_t resp_max)
{
    const char *s = "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 9\r\n"
                    "Connection: close\r\n\r\n"
                    "Not Found";
    uint16_t len = 0;
    while (*s && len < resp_max) resp[len++] = (unsigned char)*s++;
    return len;
}

/* ── SSE frame builder (called from net_task every 1s) ───────────── */

uint16_t http_sse_frame(unsigned char *frame_buf, uint16_t frame_max)
{
    unsigned char *sv_ptr = buf_ptr;
    uint16_t sv_pos = buf_pos, sv_max = buf_max, len;

    buf_ptr = frame_buf; buf_pos = 0; buf_max = frame_max;
    ap("data: {\"uptime\":"); apn_clean(jiffies / 100);
    ap(",\"mem_pct\":"); apn_clean(mem_used_pct());
    ap(",\"cpu_pct\":"); apn_clean(cpustat_pct()); ap("}\n\n");
    len = buf_pos;

    buf_ptr = sv_ptr; buf_pos = sv_pos; buf_max = sv_max;
    return len;
}

/* ── main entry point ────────────────────────────────────────────── */

uint16_t http_rx(const unsigned char *req, uint16_t req_len,
                 unsigned char *resp, uint16_t resp_max,
                 int *out_keep_alive, int *out_conn_type)
{
    char     path[64];
    int      ka = 0;
    uint16_t len;

    *out_keep_alive = 0;
    *out_conn_type  = CONN_TYPE_HTTP;

    if (resp_max < HDR_RESERVE + 128) return 0;

    parse_req(req, req_len, path, sizeof(path), &ka);
    *out_keep_alive = ka;

    if (path[0] == '/' && path[1] == '\0') {
        len = handle_root(resp, resp_max, ka);
    } else if (memcmp(path, "/api/stats", 10) == 0) {
        len = handle_api_stats(resp, resp_max, ka);
    } else if (memcmp(path, "/events", 7) == 0) {
        *out_keep_alive = 1;
        *out_conn_type  = CONN_TYPE_SSE;
        len = handle_sse_open(resp, resp_max);
    } else {
        len = handle_404(resp, resp_max);
    }

    return len;
}
