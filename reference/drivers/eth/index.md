# Ethernet Driver

Tài liệu tham khảo cho việc phát triển Ethernet driver trong kernel VinixOS.

## Status

| Module | Status | Session |
|--------|--------|---------|
| MDIO | ✅ Done — probe OK, PHY addr 0x0 detected, bus registered | SS4 |
| PHY | ✅ Done — phy_probe/init/update_link OK, hardware confirmed (PHYID=0007:c0f1, link UP) | SS5 |
| CPSW | ✅ Done — hardware confirmed, boot log OK, frame RX/TX path wired | SS6 |
| Net Core | ✅ Done — register_netdev, netif_* helpers, skbuff alloc/free | SS6 |
| IP Stack | ⬜ Chưa bắt đầu — cần để ping | — |

**SS4:** `drivers/net/ethernet/ti/omap_mdio.c` + `include/vinix/mdio.h` + board entry + MMU L4_FAST mapping
**SS5:** `include/vinix/phy.h` + phy_probe/init/update_link trong omap_mdio.c + PHYID2 mask fix
**SS6:** `drivers/net/ethernet/ti/omap_cpsw.c` (full driver) + `drivers/net/net_core.c` + `drivers/net/skbuff.c` + `mach/control.h` (MAC_ID0) + Makefile. Chưa commit.

## Tài liệu liên quan (trong reference)

| File | Mô tả |
|------|-------|
| `am335x/Chapter_14_Ethernet_Subsystem.md` | CPSW registers, MDIO registers, init sequence (§14.4) |
| `am335x/Chapter_02_Memory_Map.md` | Base addresses — L4 Fast table |
| `am335x/Chapter_06_Interrupts.md` | IRQ 40–43 (3PGSWRXTHR0/RXINT0/TXINT0/MISC0) |
| `am335x/Chapter_08_Power_Reset_and_Clock_Management_PRCM.md` | CM_PER_CPGMAC0_CLKCTRL, CM_PER_CPSW_CLKSTCTRL |
| `am335x/Chapter_09_Control_Module.md` | GMII_SEL (§9.10.7), conf_gmii1_* pad registers |
| `hardware-beagleboneblack/beaglebone_black_schematic.json` | U14 PHY wiring, PHYAD strap resistors |

## Hardware Facts (verified từ schematic U14)

| Thông tin | Giá trị | Nguồn xác minh |
|-----------|---------|----------------|
| PHY part | LAN8710A-EZC-TR | schematic refdes U14 |
| Interface | MII (không phải RMII/RGMII) | R117 pull-down 10k → GND trên RMIISEL pin |
| PHY address | `0x0` | PHYAD[2:0]=000 — R115/R118/R116 pull-down 10k → GND |
| PHY clock | 25 MHz crystal ngoài (Y3) | schematic, không dùng clock từ SoC |
| PHY reset | ETH_RESETn active-low | điều khiển bởi voltage supervisor U1 |

## Source tham khảo

| Module | Source | Ghi chú |
|--------|--------|---------|
| MDIO | [`mdio/source/README.md`](mdio/source/README.md) | davinci_mdio.c — probe, clock divisor, read/write |
| PHY | [`phy/source/README.md`](phy/source/README.md) | smsc.c — LAN8710A config, reset, link status |
| CPSW | [`cpsw/source/README.md`](cpsw/source/README.md) | cpsw.c v4.5 — probe, DMA, ALE, RX/TX |

## Sub-modules

| Module | Layer | Tài liệu |
|--------|-------|---------|
| CPSW | MAC + DMA + ALE — register trong SoC | [cpsw/index.md](cpsw/index.md) |
| MDIO | Bus transport — register trong SoC | [mdio/index.md](mdio/index.md) |
| PHY | External chip LAN8710A — truy cập qua MDIO | [phy/index.md](phy/index.md) |
