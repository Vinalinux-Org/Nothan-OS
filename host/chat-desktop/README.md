# Nothan Chat — app nhắn tin trên máy host

Đầu bên kia của cuộc trò chuyện với board. Board chạy app LVGL trong
`userspace/gui/`, còn đây là cái chạy trên máy bàn.

**Đã nối mạng.** Nhắn tin và gọi (phần báo hiệu) chạy thật với board.
Chưa có lưu trữ — dữ liệu là một list trong bộ nhớ, đóng app là mất.
Chưa có video: khung đen trong cửa sổ gọi vẫn là chỗ để dành.

## Chạy

```sh
./run.sh
```

Lần đầu cần dựng venv:

```sh
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
```

## File

| File | Nội dung |
|---|---|
| `main.py` | Cửa sổ chính: cột hội thoại + khung chat, dữ liệu giả |
| `widgets.py` | Avatar tròn, dòng hội thoại, bong bóng tin nhắn |
| `call.py` | Cửa sổ cuộc gọi video (khung rỗng) |
| `net.py` | Transport tin cậy + khung tin nhắn, nói đúng giao thức của board |
| `theme.py` | Bảng màu + stylesheet, tông tối |

## Mạng

Board mở socket kiểu `SOCK_RELIABLE`, nên đây **không phải UDP trần**.
`net.py` viết lại hai lớp của board bằng Python:

| Lớp | Nguồn trên board |
|---|---|
| 12 byte đầu: magic, kiểu, session, seq | `nothan-kernel/include/nothan/rel.h` |
| 1 byte kiểu: TEXT/INVITE/ACCEPT/REJECT/BYE | `userspace/gui/services/chat_net.h` |

Stop-and-wait, một tin đang bay, gửi lại mỗi 200 ms, bỏ cuộc sau 10 lần —
**giống hệt board**, không phải vì đơn giản hơn mà vì phía kia làm vậy: một
cửa sổ trượt ở đây sẽ gửi ba tin liên tiếp cho một máy chỉ xử lý từng cái
một, và hai cái sau bị vứt như là "vượt trước".

`ChatLink.tick()` không chặn, gọi từ `QTimer` mỗi 20 ms — cùng nhịp với
task `rel` trên board, nên hai bên hết giờ gần như cùng lúc.

Kiểm bằng cách cho hai `ChatLink` nói chuyện với nhau qua loopback với 50%
gói bị vứt cả hai chiều: cả bốn tin vẫn tới, đúng thứ tự, mỗi tin một lần.

## Địa chỉ

Hội thoại đầu trỏ `10.42.0.2:6000` — board. Sửa trong `CONVERSATIONS`
ở `main.py` nếu board mang địa chỉ khác.
