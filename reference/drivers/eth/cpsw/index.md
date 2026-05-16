# CPSW Driver

Tài liệu tham khảo cho việc phát triển CPSW (MAC + DMA + ALE) driver trong kernel NothanOS.

## Tài liệu liên quan (trong reference)

| File | Mô tả |
|------|-------|
| `am335x/Chapter_14_Ethernet_Subsystem.md` | CPSW_3G registers, CPDMA descriptor layout, ALE, init sequence (§14.4.6) |
| `am335x/Chapter_02_Memory_Map.md` | CPSW_SS (0x4A100000), CPSW_PORT/CPDMA/ALE/SL1/WR, CPPI_RAM (0x4A102000) |
| `am335x/Chapter_06_Interrupts.md` | IRQ 40–43 (3PGSWRXTHR0/RXINT0/TXINT0/MISC0) |
| `am335x/Chapter_08_Power_Reset_and_Clock_Management_PRCM.md` | CM_PER_CPGMAC0_CLKCTRL (0x44E00014), CM_PER_CPSW_CLKSTCTRL (0x44E00144) |
| `am335x/Chapter_09_Control_Module.md` | GMII_SEL (0x44E10650) chọn MII mode, conf_gmii1_* pad registers |

## Source tham khảo

| Source | Ghi chú |
|--------|---------|
| [`source/README.md`](source/README.md) | cpsw.c v4.5 — probe, ndo_open, RX/TX handler, ALE, CPDMA |
