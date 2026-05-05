# MDIO Driver

Tài liệu tham khảo cho việc phát triển MDIO bus driver trong kernel VinixOS.

## Tài liệu liên quan (trong reference)

| File | Mô tả |
|------|-------|
| `am335x/Chapter_14_Ethernet_Subsystem.md` | MDIO registers (§14.5.10), init sequence (§14.4.3), PHY read/write (§14.4.4–5) |
| `am335x/Chapter_02_Memory_Map.md` | MDIO base address (0x4A101000) |

## Source tham khảo

| Source | Ghi chú |
|--------|---------|
| [`source/README.md`](source/README.md) | davinci_mdio.c — probe, read/write, clock divisor |
