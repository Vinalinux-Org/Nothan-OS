# PHY Driver — LAN8710A

Tài liệu tham khảo cho việc phát triển PHY driver (LAN8710A-EZC-TR) trong kernel VinixOS.

## Tài liệu liên quan (trong reference)

| File | Mô tả |
|------|-------|
| `hardware-beagleboneblack/beaglebone_black_schematic.json` | U14 pinout, strap resistors PHYAD/RMIISEL/MODE, reset circuit U1 |

## Source tham khảo

| Source | Ghi chú |
|--------|---------|
| [`source/README.md`](source/README.md) | smsc.c — LAN8710A phy_driver, config_init, reset, link status |
