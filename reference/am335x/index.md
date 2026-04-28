# AM335x Technical Reference Manual

Tài liệu TRM của SoC AM335x (Texas Instruments) — register map, peripheral configuration, clock/power management. Đây là tài liệu chính khi viết bất kỳ driver nào cho VinixOS trên BeagleBone Black.

## Files

| File | Mô tả |
|------|-------|
| `Chapter_02_Memory_Map.md` | Memory map toàn bộ SoC — base address của mọi peripheral |
| `Chapter_03_ARM_MPU_Subsystem.md` | ARM Cortex-A8 core, cache, MMU |
| `Chapter_04_Programmable_Real-Time_Unit_Subsystem_and_Industrial_Communication_Subsystem_PRU-ICSS.md` | PRU-ICSS subsystem |
| `Chapter_05_Graphics_Accelerator_SGX.md` | GPU SGX530 |
| `Chapter_06_Interrupts.md` | Interrupt controller (INTC) — IRQ routing, priority, registers |
| `Chapter_07_Memory_Subsystem.md` | EMIF, DDR3, GPMC |
| `Chapter_08_Power_Reset_and_Clock_Management_PRCM.md` | Clock domains, DPLL config, module clock enable |
| `Chapter_09_Control_Module.md` | Pin mux registers, IO control |
| `Chapter_10_Interconnects.md` | L3/L4 bus interconnects |
| `Chapter_11_Enhanced_Direct_Memory_Access_EDMA.md` | EDMA3 — DMA transfers |
| `Chapter_12_Touchscreen_Controller.md` | TSC/ADC |
| `Chapter_13_LCD_Controller.md` | LCDC — display controller, raster mode |
| `Chapter_14_Ethernet_Subsystem.md` | CPSW Ethernet — registers, MDIO |
| `Chapter_15_Pulse-Width_Modulation_Subsystem_PWMSS.md` | PWM, eCAP, eQEP |
| `Chapter_16_Universal_Serial_Bus_USB.md` | USB0/USB1 controller |
| `Chapter_17_Interprocessor_Communication.md` | Mailbox, spinlock |
| `Chapter_18_Multimedia_Card_MMC.md` | MMC/SD/SDIO controller — registers, init sequence, data transfer |
| `Chapter_19_Universal_Asynchronous_ReceiverTransmitter_UART.md` | UART — baud rate, FIFO, interrupts |
| `Chapter_20_Timers.md` | DMTimer — counter, auto-reload, IRQ |
| `Chapter_21_I2C.md` | I2C master/slave — registers, transfer protocol |
| `Chapter_22_Multichannel_Audio_Serial_Port_McASP.md` | Audio serial port |
| `Chapter_23_Controller_Area_Network_CAN.md` | CAN bus controller |
| `Chapter_24_Multichannel_Serial_Port_Interface_McSPI.md` | SPI controller |
| `Chapter_25_General-Purpose_InputOutput.md` | GPIO — direction, data, interrupt |
| `Chapter_26_Initialization.md` | Boot sequence, device initialization |
| `Chapter_27_Debug_Subsystem.md` | JTAG, ETM, debug registers |
