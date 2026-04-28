# SD Card Driver

Tài liệu tham khảo cho việc phát triển SD card driver trong kernel VinixOS.

## Tài liệu liên quan (trong reference)

| File | Mô tả |
|------|-------|
| `am335x/Chapter_18_Multimedia_Card_MMC.md` | MMC/SD controller registers, init sequence, data transfer |
| `am335x/Chapter_08_Power_Reset_and_Clock_Management_PRCM.md` | Enable clock cho MMC0 (CM_PER_MMC0_CLKCTRL) |
| `am335x/Chapter_09_Control_Module.md` | Pin mux MMC0 (DAT0-3, CLK, CMD) |

## Source tham khảo

| Source | Ghi chú |
|--------|---------|
| `reference/drivers/sdcard/source/omap_hsmmc.c` | Linux host controller driver |
| `bootloader/src/mmc.c` | Đã chạy được trên board thật |

