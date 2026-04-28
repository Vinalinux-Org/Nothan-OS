# Hardware - BeagleBone Black

Tài liệu phần cứng của board BeagleBone Black (BBB). Dùng để tra cứu pin mux, schematic khi phát triển driver hoặc debug hardware.

## Files

| File | Mô tả |
|------|-------|
| `bbb_p8_header_pinout.md` | Pinout đầy đủ của P8 header (46 pins) — GPIO, mode 0-7, physical pin, ghi chú |
| `bbb_p9_header_pinout.md` | Pinout đầy đủ của P9 header (46 pins) — GPIO, mode 0-7, physical pin, ghi chú |
| `beaglebone_black_schematic.json` | Schematic toàn bộ board BBB dạng JSON — net connections, component values |

## Khi nào dùng

- Viết driver cần biết pin nào map với peripheral nào → xem P8/P9 pinout
- Cần xác nhận net connection giữa SoC và peripheral → xem schematic
- Debug hardware signal → kết hợp pinout + schematic
