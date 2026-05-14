# GPIO Driver

Tài liệu tham khảo cho việc phát triển GPIO driver trong kernel VinixOS.

## Status

| Module | Status | Session |
|--------|--------|---------|
| `omap_gpio.c` + `gpio.h` | ⏳ Planned — Phase 1 | — |
| IRQ support (`gpio_to_irq`) | ⏳ Planned — Phase 2 | — |
| Pin mux config | ⏳ Planned — Phase 3 | — |
| Debounce hardware | ⏳ Planned — Phase 4 | — |

## Tài liệu liên quan (trong reference)

| File | Mô tả |
|------|-------|
| `am335x/Chapter_25_General-Purpose_InputOutput.md` | GPIO registers, operating modes, debounce — tài liệu chính |
| `am335x/Chapter_02_Memory_Map.md` | Base addresses GPIO0–3 |
| `am335x/Chapter_06_Interrupts.md` | IRQ 96/97 (GPIO0A/B), 98/99 (GPIO1A/B), 32/33 (GPIO2A/B), 62/63 (GPIO3A/B) |
| `am335x/Chapter_08_Power_Reset_and_Clock_Management_PRCM.md` | CM_WKUP_GPIO0_CLKCTRL, CM_PER_GPIO1–3_CLKCTRL |
| `am335x/Chapter_09_Control_Module.md` | Pin mux pad registers — cần cho Phase 3 |

## Hardware Facts (verified từ BBB schematic + TRM)

### GPIO module map
| Bank | Base | Domain | PRCM offset |
|------|------|--------|------------|
| GPIO0 | 0x44E07000 | L4_WKUP | CM_WKUP + 0x08 |
| GPIO1 | 0x4804C000 | L4_PER | CM_PER + 0xAC |
| GPIO2 | 0x481AC000 | L4_PER | CM_PER + 0xB0 |
| GPIO3 | 0x481AE000 | L4_PER | CM_PER + 0xB4 |

### BBB User LEDs (schematic verified, active-high)
| LED | GPIO | gpio number |
|-----|------|------------|
| USR0 | GPIO1_21 | 53 |
| USR1 | GPIO1_22 | 54 |
| USR2 | GPIO1_23 | 55 |
| USR3 | GPIO1_24 | 56 |

GPIO number convention: `bank × 32 + pin`

### MMU và pin mux
| Thông tin | Trạng thái |
|-----------|-----------|
| MMU mapping GPIO0 | Đã có — nằm trong L4_WKUP (1MB map) |
| MMU mapping GPIO1–3 | Đã có — nằm trong L4_PER (3MB map) |
| Pin mux USR LEDs | Đã cấu hình bởi boot ROM — không cần set ở Phase 1 |

## Source tham khảo

| Module | Source | Ghi chú |
|--------|--------|---------|
| omap_gpio | Linux `drivers/gpio/gpio-omap.c` | probe sequence, clock enable, OE/DATAIN/DATAOUT pattern |
