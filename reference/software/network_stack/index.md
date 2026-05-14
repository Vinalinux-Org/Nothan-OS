# Network Stack

Tài liệu cho software networking stack của VinixOS — các tầng giao thức phía trên Ethernet driver.

---

## Status

| Tầng | Module | Trạng thái | Session |
|------|--------|------------|---------|
| Ethernet | `vnet.c` — dispatch EtherType | ✅ Done | SS11 |
| ARP | `vnet.c::arp_reply()` — trả lời ARP request | ✅ Done — hardware confirmed | SS11/SS12 |
| ICMP | `vnet.c::icmp_reply()` — echo reply | ✅ Done — ping 4/4, RTT ~9ms | SS11/SS12 |
| IP | `ip.c` — ip_rx(), ip_tx() | ✅ Done — hardware confirmed | SS13 |
| TCP | `tcp.c` — tcp_rx(), tcp_send() | ✅ Done — hardware confirmed | SS13 |
| HTTP | `http.c` — http_rx(), http_sse_frame() | ✅ Done — hardware confirmed | SS15 |

---

## Architecture

```
vnet_rx(skb)
    ├── EtherType 0x0806 → arp_reply()       ✅
    └── EtherType 0x0800 → ip_rx()           ✅
                               ├── proto 1 ICMP → icmp_reply()   ✅
                               └── proto 6 TCP  → tcp_rx()       ✅
                                                      └── port 80 → http_rx() ✅
```

---

## Naming Convention

Tên file và function phản ánh đúng tầng giao thức và mục tiêu của hàm.

| Tầng | File | Function | Ý nghĩa |
|------|------|----------|---------|
| IP   | `ip.c`   | `ip_rx(skb)` | Nhận gói IP, đọc field `protocol`, dispatch xuống |
|      |          | `ip_tx(skb, dst_ip, proto)` | Build IP header, gửi qua net_core |
| TCP  | `tcp.c`  | `tcp_rx(skb, ip)` | Nhận TCP segment, dispatch theo state machine |
|      |          | `tcp_send(conn, seq, ack, flags, data, len)` | Gửi mọi loại TCP frame — SYN-ACK, ACK, FIN, data |
|      |          | `tcp_poll()` | Đóng keep-alive connection idle quá 5s |
|      |          | `tcp_sse_push(frame, len)` | Push SSE frame đến tất cả SSE connection |
| HTTP | `http.c` | `http_rx(req, len, resp, max, ka, ctype)` | Parse request, build response, phân loại HTTP vs SSE |
|      |          | `http_sse_frame(buf, max)` | Build SSE frame chứa metrics hiện tại |

### Struct và State

```c
struct iphdr  { ... }   /* IP header — overlay trực tiếp lên skb->data + ETH_HDR */
struct tcphdr { ... }   /* TCP header — overlay trực tiếp lên IP payload */

struct tcp_conn {
    uint32_t  remote_ip;
    uint16_t  remote_port;
    uint32_t  seq, ack;
    int       state;
};

/* TCP states */
#define TCP_LISTEN       0
#define TCP_SYN_RCVD     1
#define TCP_ESTABLISHED  2
#define TCP_LAST_ACK     3
```

---

## Files đã tạo

```
vinix-kernel/drivers/net/ipv4/
    ip.c      — ip_rx, ip_tx
    tcp.c     — tcp_rx, tcp_send, tcp_poll, tcp_sse_push

vinix-kernel/drivers/net/app/
    http.c    — http_rx, http_sse_frame
    net_task.c — background task, SSE push mỗi giây

vinix-kernel/include/
    ip.h      — public interface ip layer
    tcp.h     — public interface tcp layer
    http.h    — public interface http layer
    net_task.h — get_net_task()
```

---

## Bối cảnh từng giao thức

Mỗi giao thức ra đời để giải quyết đúng 1 bài toán. Hiểu bài toán trước — cơ chế sẽ tự nhiên.

### IP — bài toán địa chỉ
Có hàng triệu máy trên mạng, làm sao biết gói tin gửi cho ai?
IP gắn địa chỉ số vào mỗi gói rồi **gửi đi và quên** — không quan tâm có đến nơi không.
- Ưu: nhanh, đơn giản, không giữ trạng thái
- Nhược: không đảm bảo đến nơi, không đảm bảo thứ tự

### TCP — bài toán đảm bảo
IP không đáng tin, nhưng browser cần nhận đủ toàn bộ trang web đúng thứ tự.
TCP yêu cầu **xác nhận mỗi gói** (ACK). Không có ACK → gửi lại. Hai bên bắt tay trước, chào tạm biệt sau.
- Ưu: đảm bảo dữ liệu đến đủ, đúng thứ tự
- Nhược: chậm hơn IP, phức tạp hơn vì phải nhớ trạng thái

### HTTP — bài toán ngôn ngữ
TCP cho một đường ống đáng tin, nhưng browser và server cần nói chuyện được với nhau — hỏi gì, trả lời gì, định dạng ra sao?
HTTP là quy ước văn bản thuần: browser hỏi `GET /`, server trả lời `200 OK` kèm HTML.
- Ưu: đơn giản, con người đọc được, dễ debug
- Nhược: mỗi request phải mở TCP connection mới (HTTP/1.0)

---

## Plan thực hiện

Thứ tự viết code theo dependency — tầng dưới xong trước tầng trên mới gọi được:

| Bước | Tầng | Lý do |
|------|------|-------|
| 1 | `ip.c` | Độc lập, không phụ thuộc ai |
| 2 | `tcp.c` | Cần `ip_tx()` để gửi SYN-ACK |
| 3 | `http.c` | Cần `tcp_send()` để gửi response |
| 4 | `vnet.c` | Sửa 2 dòng để nối `ip_rx()` vào — làm cuối |

---

## Hướng dẫn cho AI session tiếp theo

**Đọc file này trước khi bắt đầu bất kỳ task nào trong network stack.**

### Cách làm việc với user

- Giải thích **bài toán trước**, cơ chế sau — không bắt đầu bằng register hay struct
- Khi user chưa có base: dùng ngôn ngữ đời thường, tránh thuật ngữ kỹ thuật dày đặc
- Flow: overview để user quyết định hướng → drill down từng tầng khi user confirm
- Show diff trước khi apply bất kỳ thay đổi nào, chờ confirm

### Lịch sử kiến trúc

#### v0.1 — Plan ban đầu (Pull model)
```
Client ──request──▶ tcp_syn_ack()
                    tcp_ack()          ← mỗi state = 1 hàm riêng
                    http_rx()
                    tcp_fin()
       ◀──response─ (đóng connection)
```
- 1 connection tĩnh toàn cục `g_conn`
- HTTP/1.0 — mỗi request mở/đóng connection
- Client hỏi → server trả lời → xong

#### v0.2 — Implemented (Push model)
```
conn_table[4]
  ├── conn HTTP  — request/response, keep-alive 5s timeout
  └── conn SSE  ── net_task ──▶ push mỗi giây ──▶ browser
                    tcp_send(conn, flags, data, len)
                         ↑ 1 hàm duy nhất cho mọi state
```
- Pool 4 connections đồng thời, 2 loại: `CONN_TYPE_HTTP` và `CONN_TYPE_SSE`
- HTTP/1.1 keep-alive — connection tái sử dụng
- `net_task` chạy ngầm, chủ động push data mỗi giây
- Server tự gửi khi có data mới, không cần client hỏi
