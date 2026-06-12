# CLAUDE.md — NothanOS

**Trước mỗi task hardware/driver**: đọc [reference/index.md](reference/index.md).

## Hard Rules

### Toolchain

- KHÔNG mix `arm-none-eabi` và `arm-linux-gnueabihf`

### Driver

- Register address / bit field: verify từ `reference/am335x/`, không đoán
- Board-specific data (base address, IRQ) tách riêng khỏi driver — driver phải portable, board data thuộc về `board-bbb.c`
- Mọi HW init trong `probe()`. KHÔNG có public `xxx_init()` gọi từ `main.c`.

### Design Philosophy

Lấy established kernel design patterns làm tiêu chuẩn — không biết làm gì thì hỏi "kernel trưởng thành làm thế nào?". Nhưng NothanOS không bị ràng buộc bởi legacy. Nếu có cách tốt hơn và có lý do rõ ràng thì diverge — ví dụ dùng `spawn` thay `fork` vì `fork` giữ lại vì tương thích UNIX, không phải vì nó tốt hơn.

- **Được**: học pattern, diverge khi có lý do
- **Cấm**: copy, fork, port, paraphrase từ bất kỳ upstream nào
- **Workflow**: tham khảo kernel design patterns → hiểu → viết từ đầu

---

## Code Generation

**Trước mỗi module mới, theo thứ tự:**

1. Hiểu kiến trúc tổng quan — hệ thống hiện tại ra sao, module mới fit vào đâu, luồng dữ liệu từ user đến hardware đi qua những gì
2. Trao đổi với dev để thống nhất thiết kế — dev là người quyết định
3. Tham khảo Linux source để học pattern
4. Mới đi vào chi tiết register / implementation

Không tự ý bắt đầu code khi chưa qua bước 1 và 2.

**Không đủ thông tin → DỪNG, hỏi. Không bịa.**

Hỏi trước nếu: register address / IRQ / init sequence chưa verify | file chưa đọc | behavior ambiguous.

---

## Debug

Không JTAG. **UART log là công cụ duy nhất.**

Khi gặp bug, yêu cầu trước khi phân tích:

1. Toàn bộ UART log từ đầu boot
2. Dòng log cuối cùng trước hang/crash
3. Loại exception + DFAR, DFSR, PC nếu là abort
