# Source tham khảo — CPSW

## Primary reference

- [cpsw.c v4.5 (Bootlin)](https://elixir.bootlin.com/linux/v4.5/source/drivers/net/ethernet/ti/cpsw.c) — legacy single-MAC version, dễ đọc hơn cpsw_new
- [cpsw_new.c (mainline)](https://github.com/torvalds/linux/blob/master/drivers/net/ethernet/ti/cpsw_new.c) — phiên bản hiện tại (phức tạp hơn, dual-MAC)
- [Kernel docs CPSW](https://docs.kernel.org/networking/device_drivers/ethernet/ti/cpsw.html) — tổng quan architecture

## Đọc những gì

- `cpsw_probe()` — resource map, clock enable, ALE init, slave config, MDIO attach
- `cpsw_ndo_open()` — DMA channel open, interrupt enable, PHY connect, MAC address set
- `cpsw_rx_handler()` / `cpsw_tx_handler()` — CPDMA descriptor processing, skb alloc/free
- `struct cpsw_priv` — state: regs, slaves[], ale, cpdma_ctlr
- `cpsw_ale_add_mcast()` / `cpsw_ale_add_ucast()` — ALE table management

---

## TRM sections dùng trong SS6 implementation

### Chapter 02 — Memory Map

Sub-module base addresses (relative offset từ CPSW_SS base `0x4A100000`):

| Sub-module | Absolute | Offset |
|------------|----------|--------|
| CPSW_SS | 0x4A100000 | +0x000 |
| CPSW_PORT | 0x4A100100 | +0x100 |
| CPDMA | 0x4A100800 | +0x800 |
| CPDMA StateRAM | 0x4A100A00 | +0xA00 |
| CPSW_ALE | 0x4A100D00 | +0xD00 |
| CPSW_SL1 | 0x4A100D80 | +0xD80 |
| CPSW_WR | 0x4A101200 | +0x1200 |
| CPPI_RAM | 0x4A102000 | — (separate region, 8KB) |

CPPI_RAM layout dùng trong driver (8192 bytes total):
```
0x0000: rx_bd[4]   — 4 × 16 bytes = 64 bytes
0x0040: tx_bd      — 16 bytes
0x0100: rx_buf[4]  — 4 × 1536 = 6144 bytes
0x1900: tx_buf     — 1536 bytes    (total 7824 < 8192)
```

### Chapter 06 — Interrupts

| IRQ | Tên | Dùng cho |
|-----|-----|---------|
| 40 | 3PGSWRXTHR0 | RX threshold (không dùng MVP) |
| 41 | 3PGSWRXINT0 | RX interrupt → `cpsw_rx_irq` |
| 42 | 3PGSWTXINT0 | TX interrupt → `cpsw_tx_irq` |
| 43 | 3PGSWMISC0 | MISC (không dùng MVP) |

### Chapter 09 — Control Module

**MAC address fused registers:**

| Register | Offset | Byte mapping |
|----------|--------|-------------|
| MAC_ID0_LO | CTRL_MODULE_BASE + 0x630 | [31:24]=mac[2], [23:16]=mac[3], [15:8]=mac[4], [7:0]=mac[5] |
| MAC_ID0_HI | CTRL_MODULE_BASE + 0x634 | [15:8]=mac[0], [7:0]=mac[1] |

**GMII_SEL** (CTRL_MODULE_BASE + 0x650): field [2:1] chọn mode port 1:
- `0b00` = MII (BBB dùng — R117 pulls RMIISEL GND)
- `0b01` = RMII
- `0b10` = RGMII

### Chapter 14 — Ethernet Subsystem

**Reset sequence** — bắt buộc theo thứ tự (nếu sai → timeout hoặc hung):
```
SL1_SOFT_RESET → CPDMA_SOFT_RESET → SS_SOFT_RESET → WR_SOFT_RESET
```
Poll bit[0] clear để xác nhận mỗi bước xong.

**CPSW_SS registers (offset từ ss_base):**

| Offset | Register | Dùng |
|--------|----------|------|
| 0x08 | SS_SOFT_RESET | Reset toàn bộ switch |
| 0x0C | SS_STAT_PORT_EN | Enable stats port 0+1: write `0x3` |

**CPSW_PORT registers (offset từ port_base = ss_base + 0x100):**

| Offset | Register | Byte order |
|--------|----------|-----------|
| 0x120 | PORT_P1_SA_LO | [15:8]=mac[0], [7:0]=mac[1] |
| 0x124 | PORT_P1_SA_HI | [31:24]=mac[2], [23:16]=mac[3], [15:8]=mac[4], [7:0]=mac[5] |

**CPDMA registers (offset từ cpdma_base = ss_base + 0x800):**

| Offset | Register | Giá trị dùng |
|--------|----------|-------------|
| 0x04 | TX_CONTROL | Write `1` để enable TX |
| 0x14 | RX_CONTROL | Write `1` để enable RX |
| 0x1C | SOFT_RESET | Poll bit[0] clear |
| 0x28 | RX_BUF_OFFSET | Write `0` (no offset) |
| 0x88 | TX_INTMASK_SET | Write `1` (channel 0) |
| 0xA8 | RX_INTMASK_SET | Write `1` (channel 0) |
| 0x94 | EOI_VECTOR | Write `1`=RX, `2`=TX sau mỗi IRQ handler |

**CPDMA StateRAM (offset từ cpdma_sr_base = ss_base + 0xA00):**

| Offset | Register | Mô tả |
|--------|----------|-------|
| 0x00 | TX0_HDP | TX head descriptor pointer — write BD addr để kick TX |
| 0x20 | RX0_HDP | RX head descriptor pointer — write BD addr để kick RX |
| 0x40 | TX0_CP | TX completion pointer — write BD addr để ack |
| 0x60 | RX0_CP | RX completion pointer — write BD addr để ack |

Zero toàn bộ 8 channel HDP/CP trước khi enable — bắt buộc.

**ALE registers (offset từ ale_base = ss_base + 0xD00):**

| Offset | Register | Giá trị dùng |
|--------|----------|-------------|
| 0x08 | ALE_CONTROL | `ENABLE(31) \| CLEAR_TBL(30) \| BYPASS(4)` = `0xC0000010` |
| 0x40 | ALE_PORTCTL0 | `3` (FORWARD) |
| 0x44 | ALE_PORTCTL1 | `3` (FORWARD) |
| 0x48 | ALE_PORTCTL2 | `3` (FORWARD) |

Bypass mode: tất cả frame từ Port 1 forward thẳng về Port 0 (host), không qua CAM table lookup.

**CPSW_SL1 registers (offset từ sl1_base = ss_base + 0xD80):**

| Offset | Register | Bits |
|--------|----------|------|
| 0x04 | MACCONTROL | bit[5]=GMII_EN, bit[0]=FULLDUPLEX |
| 0x0C | SOFT_RESET | Poll bit[0] clear |

**CPSW_WR registers (offset từ wr_base = ss_base + 0x1200):**

| Offset | Register | Dùng |
|--------|----------|------|
| 0x04 | WR_SOFT_RESET | Reset interrupt wrapper |
| 0x14 | WR_C0_RX_EN | Write `1` enable RX IRQ core 0 |
| 0x18 | WR_C0_TX_EN | Write `1` enable TX IRQ core 0 |

**Buffer Descriptor format** (4 × 32-bit words, viết bằng mmio vì CPPI_RAM strongly ordered):

```
word 0 [+0x00]: next BD pointer (0 = end of chain)
word 1 [+0x04]: buffer physical address
word 2 [+0x08]: buffer length (capacity khi init; actual len do CPDMA fill)
word 3 [+0x0C]: flags | packet_length
```

BD flags (word 3):

| Bit | Name | Mô tả |
|-----|------|-------|
| 31 | BD_SOP | Start of packet |
| 30 | BD_EOP | End of packet |
| 29 | BD_OWNER | `1`=host owns BD, `0`=CPDMA done |
| 28 | BD_EOQ | CPDMA reached end of queue — restart HDP nếu thấy |
| [10:0] | PKT_LEN | Actual received byte count (CPDMA fills on RX complete) |

---

## Ghi chú

- Dùng cpsw.c v4.5 làm reference chính — single-MAC, không có dual-emac complexity
- CPPI_RAM strongly ordered: bắt buộc dùng `mmio_read32/write32`, không dùng pointer cast hay memcpy trực tiếp
- Pattern reference: Linux `drivers/net/ethernet/ti/cpsw.c` — re-implemented
