# Hardware - BeagleBone Black

Tài liệu phần cứng của board BeagleBone Black (BBB). Dùng để tra cứu address, IRQ, pin mux — không viết DTS.

## Files

| File | Mô tả |
|------|-------|
| `am33xx.dtsi` | Toàn bộ peripheral nodes của SoC AM33xx — address, IRQ |
| `am33xx.h` | Macro address/IRQ cho AM33xx |
| `am335x-bone-common.dtsi` | Pin mux, peripheral enable cho BeagleBone |
| `am335x-boneblack-common.dtsi` | Peripheral BBB-specific — HDMI, eMMC |
| `am335x-boneblack.dts` | Top-level DTS của BBB |
| `bbb_p8_header_pinout.md` | Pinout P8 header (46 pins) — GPIO, mode 0-7 |
| `bbb_p9_header_pinout.md` | Pinout P9 header (46 pins) — GPIO, mode 0-7 |
| `beagle_bone_black_schematic.json` | Schematic board BBB — net connections, component values |

## Thứ tự tra cứu

1. **Linux 2.6 source** — board data, address, IRQ
2. **DTS files** — nếu Linux source không đủ, trace address/IRQ/pin mux tại đây
3. **P8/P9 pinout + schematic** — debug hardware signal, xác nhận net connection
