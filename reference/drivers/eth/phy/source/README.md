# Source tham khảo — PHY (LAN8710A)

## Primary reference

- [smsc.c v4.6 (Bootlin)](https://elixir.bootlin.com/linux/v4.6/source/drivers/net/phy/smsc.c) — SMSC PHY family driver, bao gồm LAN8710A
- [smsc.c (mainline)](https://github.com/torvalds/linux/blob/master/drivers/net/phy/smsc.c) — phiên bản mới nhất

## Đọc những gì

- `struct phy_driver` entry cho `LAN8710A` — `.phy_id = 0x0007c0f0`, `.phy_id_mask = 0xfffffff0`
  (bits[3:0] của PHYID2 là revision, thay đổi theo silicon lot — BBB thực tế = 0xC0F1)
- `smsc_phy_config_init()` — energy-detect power-down, mode config
- `lan87xx_read_status()` — link status, speed/duplex detection
- `smsc_phy_reset()` — soft reset sequence, poll BMCR_RESET clear

## Ghi chú

- PHY address BBB = `0x0` (PHYAD[2:0]=000, xác nhận từ schematic R115/R118/R116)
- NothanOS PHY layer đơn giản hơn: không cần full phy_driver framework, chỉ cần read/write qua MDIO + check BMSR link bit
- Pattern reference: Linux `drivers/net/phy/smsc.c` — re-implemented

## Board-specific facts (BBB thực tế)

| Thông tin | Giá trị | Nguồn |
|---|---|---|
| PHYID1 | `0x0007` | đọc từ hardware |
| PHYID2 raw | `0xC0F1` | đọc từ hardware — rev1 silicon |
| PHYID2 family | `0xC0F0` | Linux `.phy_id` base pattern (rev0) |
| PHYID2 mask | `0xFFF0` | Linux `.phy_id_mask = 0xfffffff0` → bits[3:0] = revision |

**Lưu ý quan trọng khi implement:** `.phy_id` trong Linux là *family pattern*, không phải
giá trị chip thực. Silicon revision (bits[3:0]) chỉ biết sau khi đọc hardware.
Implement PHẢI dùng mask — không hardcode full PHYID2:

```c
/* SAI — fail nếu silicon revision khác */
if (id2 == LAN8710A_PHYID2) ...

/* ĐÚNG */
if ((id2 & LAN8710A_PHYID2_MASK) == (LAN8710A_PHYID2 & LAN8710A_PHYID2_MASK)) ...
```
