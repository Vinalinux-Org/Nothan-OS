# 2 Memory Map

This section describes the memory map for the device.

## 2.1 ARM Cortex-A8 Memory Map

> **(1)** The first 1 MB of address space 0x0–0xFFFFF is inaccessible externally.
> **(2)** Ex/R/W – Execute/Read/Write.


### Table 2-1. L3 Memory Map

| Block Name | Start_address(hex) | End_address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
| GPMC (External Memory) | 0x0000_0000(1) | 0x1FFF_FFFF | 512 MB | 8-/16-bit External Memory (Ex/R/W)(2) |
| Reserved | 0x2000_0000 | 0x3FFF_FFFF | 512 MB | Reserved |
| Boot ROM | 0x4000_0000 | 0x4001_FFFF | 128 KB |  |
|  | 0x4002_0000 | 0x4002_BFFF | 48 KB | 32-bit Ex/R(2) – Public |
| Reserved | 0x4002_C000 | 0x400F_FFFF | 848 KB | Reserved |
| Reserved | 0x4010_0000 | 0x401F_FFFF | 1 MB | Reserved |
| Reserved | 0x4020_0000 | 0x402E_FFFF | 960 KB | Reserved |
| Reserved | 0x402F_0000 | 0x402F_03FF | 64 KB | Reserved |
| SRAM internal | 0x402F_0400 | 0x402F_FFFF |  | 32-bit Ex/R/W(2) |
| L3 OCMC0 | 0x4030_0000 | 0x4030_FFFF | 64 KB | 32-bit Ex/R/W(2) OCMC SRAM |
| Reserved | 0x4031_0000 | 0x403F_FFFF | 960 KB | Reserved |
| Reserved | 0x4040_0000 | 0x4041_FFFF | 128 KB | Reserved |
| Reserved | 0x4042_0000 | 0x404F_FFFF | 896 KB | Reserved |
| Reserved | 0x4050_0000 | 0x405F_FFFF | 1 MB | Reserved |
| Reserved | 0x4060_0000 | 0x407F_FFFF | 2 MB | Reserved |
| Reserved | 0x4080_0000 | 0x4083_FFFF | 256 KB | Reserved |
| Reserved | 0x4084_0000 | 0x40DF_FFFF | 5888 KB | Reserved |
| Reserved | 0x40E0_0000 | 0x40E0_7FFF | 32 KB | Reserved |
| Reserved | 0x40E0_8000 | 0x40EF_FFFF | 992 KB | Reserved |
| Reserved | 0x40F0_0000 | 0x40F0_7FFF | 32 KB | Reserved |
| Reserved | 0x40F0_8000 | 0x40FF_FFFF | 992 KB | Reserved |
| Reserved | 0x4100_0000 | 0x41FF_FFFF | 16 MB | Reserved |
| Reserved | 0x4200_0000 | 0x43FF_FFFF | 32 MB | Reserved |
| L3F CFG Regs | 0x4400_0000 | 0x443F_FFFF | 4 MB | L3Fast configuration registers |
| Reserved | 0x4440_0000 | 0x447F_FFFF | 4 MB | Reserved |
| L3S CFG Regs | 0x4480_0000 | 0x44BF_FFFF | 4 MB | L3Slow configuration registers |
| L4_WKUP | 0x44C0_0000 | 0x44FF_FFFF | 4 MB | L4_WKUP |
| Reserved | 0x4500_0000 | 0x45FF_FFFF | 16 MB | Reserved |
| McASP0 Data | 0x4600_0000 | 0x463F_FFFF | 4 MB | McASP0 Data Registers |
| McASP1 Data | 0x4640_0000 | 0x467F_FFFF | 4 MB | McASP1 Data Registers |
| Reserved | 0x4680_0000 | 0x46FF_FFFF | 8 MB | Reserved |
| Reserved | 0x4700_0000 | 0x473F_FFFF | 4 MB | Reserved |


*Table 2-1. L3 Memory Map (continued)*

| Block Name | Start_address(hex) | End_address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
| USBSS | 0x4740_0000 | 0x4740_0FFF | 32 KB | USB Subsystem Registers |
| USB0 | 0x4740_1000 | 0x4740_12FF |  | USB0 Controller Registers |
| USB0_PHY | 0x4740_1300 | 0x4740_13FF |  | USB0 PHY Registers |
| USB0 Core | 0x4740_1400 | 0x4740_17FF |  | USB0 Core Registers |
| USB1 | 0x4740_1800 | 0x4740_1AFF |  | USB1 Controller Registers |
| USB1_PHY | 0x4740_1B00 | 0x4740_1BFF |  | USB1 PHY Registers |
| USB1 Core | 0x4740_1C00 | 0x4740_1FFF |  | USB1 Core Registers |
| USB CPPI DMA Controller | 0x4740_2000 | 0x4740_2FFF |  | USB CPPI DMA Controller Registers |
| USB CPPI DMA Scheduler | 0x4740_3000 | 0x4740_3FFF |  | USB CPPI DMA Scheduler Registers |
| USB Queue Manager | 0x4740_4000 | 0x4740_7FFF |  | USB Queue Manager Registers |
| Reserved | 0x4740_8000 | 0x477F_FFFF | 4 MB-32 KB | Reserved |
| Reserved | 0x4780_0000 | 0x4780_FFFF | 64 KB | Reserved |
| MMCHS2 | 0x4781_0000 | 0x4781_FFFF | 64 KB | MMCHS2 |
| Reserved | 0x4782_0000 | 0x47BF_FFFF | 4 MB-128 KB | Reserved |
| Reserved | 0x47C0_0000 | 0x47FF_FFFF | 4 MB | Reserved |
| L4_PER | 0x4800_0000 | 0x48FF_FFFF | 16 MB | L4Peripheral(see L4_PE Rtable) |
| TPCC(EDMA3CC) | 0x4900_0000 | 0x490F_FFFF | 1 MB | EDMA3 Channel Controller Registers |
| Reserved | 0x4910_0000 | 0x497F_FFFF | 7 MB | Reserved |
| TPTC0(EDMA3TC0) | 0x4980_0000 | 0x498F_FFFF | 1 MB | EDMA3Transfer Controller0 Registers |
| TPTC1(EDMA3TC1) | 0x4990_0000 | 0x499F_FFFF | 1 MB | EDMA3Transfer Controller1 Registers |
| TPTC2(EDMA3TC2) | 0x49A0_0000 | 0x49AF_FFFF | 1 MB | EDMA3Transfer Controller2 Registers |
| Reserved | 0x49B0_0000 | 0x49BF_FFFF | 1 MB | Reserved |
| Reserved | 0x49C0_0000 | 0x49FF_FFFF | 4 MB | Reserved |
| L4_FAST | 0x4A00_0000 | 0x4AFF_FFFF | 16 MB | L4_FAST |
| Reserved | 0x4B00_0000 | 0x4B13_FFFF | 1280 KB | Reserved |
| Debug SS_ETM | 0x4B14_0000 | 0x4B14_0FFF | 4 KB | CortexA8ETM Unit |
| Debug SS_Debug | 0x4B14_1000 | 0x4B14_1FFF | 4 KB | CortexA8Debug Unit |
| Debug SS_CTI | 0x4B14_2000 | 0x4B14_2FFF | 4 KB | CortexA8CTI Unit |
| Debug SS_ICE | 0x4B14_3000 | 0x4B14_3FFF | 4 KB | ICE Crusher CSAPB |
| Reserved | 0x4B14_4000 | 0x4B15_FFFF | 128 KB | Reserved |
| Debug SS_DRM | 0x4B16_0000 | 0x4B16_0FFF | 4 KB | Debug Subsystem:Debug Resource Manager |
| Debug SS_ETB | 0x4B16_2000 | 0x4B16_2FFF | 4 KB | Debug Subsystem:Embedded Trace Buffer |
| Reserved | 0x4B16_3000 | 0x4BFF_FFFF | 15 MB-396 KB | Reserved |
| EMIF0 | 0x4C00_0000 | 0x4CFF_FFFF | 16 MB | EMIF0Configurationregisters |
| Reserved | 0x4D00_0000 | 0x4DFF_FFFF | 16 MB | Reserved |
| Reserved | 0x4E00_0000 | 0x4FFF_FFFF | 32 MB | Reserved |
| GPMC | 0x5000_0000 | 0x50FF_FFFF | 16 MB | GPMC Configurationregisters |
| Reserved | 0x5100_0000 | 0x52FF_FFFF | 32 MB | Reserved |
| Reserved | 0x5300_0000 | 0x530F_FFFF | 1 MB | Reserved |
|  | 0x5310_0000 | 0x531F_FFFF | 1 MB | Reserved |
| Reserved | 0x5320_0000 | 0x533F_FFFF | 2 MB | Reserved |
| Reserved | 0x5340_0000 | 0x534F_FFFF | 1 MB | Reserved |


*Table 2-1. L3 Memory Map (continued)*

| Block Name | Start_address(hex) | End_address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
|  | 0x5350_0000 | 0x535F_FFFF | 1 MB | Reserved |
| Reserved | 0x5360_0000 | 0x54BF_FFFF | 22 MB | Reserved |
| ADC_TSCDMA | 0x54C0_0000 | 0x54FF_FFFF | 4 MB | ADC_TSCDMA Port |
| Reserved | 0x5500_0000 | 0x55FF_FFFF | 16 MB | Reserved |
| SGX530 | 0x5600_0000 | 0x56FF_FFFF | 16 MB | SGX530 Slave Port |
| Reserved | 0x5700_0000 | 0x57FF_FFFF | 16 MB | Reserved |
| Reserved | 0x5800_0000 | 0x58FF_FFFF | 16 MB | Reserved |
| Reserved | 0x5900_0000 | 0x59FF_FFFF | 16 MB | Reserved |
| Reserved | 0x5A00_0000 | 0x5AFF_FFFF | 16 MB | Reserved |
| Reserved | 0x5B00_0000 | 0x5BFF_FFFF | 16 MB | Reserved |
| Reserved | 0x5C00_0000 | 0x5DFF_FFFF | 32 MB | Reserved |
| Reserved | 0x5E00_0000 | 0x5FFF_FFFF | 32 MB | Reserved |
| Reserved | 0x6000_0000 | 0x7FFF_FFFF | 512 MB | Reserved |
| EMIF0SDRAM | 0x8000_0000 | 0xBFFF_FFFF | 1 GB | 8-/16-bit External Memory (Ex/R/W)(2) |
| Reserved | 0xC000_0000 | 0xFFFF_FFFF | 1 GB | Reserved |


### Table 2-2. L4\_WKUP Peripheral Memory Map

| Region Name | Start Address(hex) | End Address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
| L4_WKU Pconfiguration | 0x44C0_0000 | 0x44C0_07FF | 2 KB | Address/Protection(AP) |
|  | 0x44C0_0800 | 0x44C0_0FFF | 2 KB | Link Agent(LA) |
|  | 0x44C0_1000 | 0x44C0_13FF | 1 KB | Initiator Port(IP0) |
|  | 0x44C0_1400 | 0x44C0_17FF | 1 KB | Initiator Port(IP1) |
| Reserved | 0x44C0_1800 | 0x44C0_1FFF | 2 KB | Reserved(IP2 – IP3) |
| Reserved | 0x44C0_2000 | 0x44CF_FFFF | 1 MB-8 KB | Reserved |
| Reserved | 0x44D0_0000 | 0x44D0_3FFF | 16 KB | Reserved |
|  | 0x44D0_4000 | 0x44D0_4FFF | 4 KB | Reserved |
| Reserved | 0x44D0_5000 | 0x44D7_FFFF | 492 KB | Reserved |
| Reserved | 0x44D8_0000 | 0x44D8_1FFF | 8 KB | Reserved |
|  | 0x44D8_2000 | 0x44D8_2FFF | 4 KB | Reserved |
| Reserved | 0x44D8_3000 | 0x44DF_FFFF | 500 KB | Reserved |
| CM_PER | 0x44E0_0000 | 0x44E0_03FF | 1 KB | Clock Module Peripheral Registers |
| CM_WKUP | 0x44E0_0400 | 0x44E0_04FF | 256 Bytes | Clock Module Wakeup Registers |
| CM_DPLL | 0x44E0_0500 | 0x44E0_05FF | 256 Bytes | Clock Module PLL Registers |
| CM_MPU | 0x44E0_0600 | 0x44E0_06FF | 256 Bytes | Clock Module MPU Registers |
| CM_DEVICE | 0x44E0_0700 | 0x44E0_07FF | 256 Bytes | Clock Module Device Registers |
| CM_RTC | 0x44E0_0800 | 0x44E0_08FF | 256 Bytes | Clock Module RTC Registers |
| CM_GFX | 0x44E0_0900 | 0x44E0_09FF | 256 Bytes | Clock Module Graphics Controller Registers |
| CM_CEFUSE | 0x44E0_0A00 | 0x44E0_0AFF | 256 Bytes | Clock Module Efuse Registers |
| PRM_IRQ | 0x44E0_0B00 | 0x44E0_0BFF | 256 Bytes | Power Reset Module Interrupt Registers |
| PRM_PER | 0x44E0_0C00 | 0x44E0_0CFF | 256 Bytes | Power Reset Module Peripheral Registers |
| PRM_WKUP | 0x44E0_0D00 | 0x44E0_0DFF | 256 Bytes | Power Reset Module Wakeup Registers |
| PRM_MPU | 0x44E0_0E00 | 0x44E0_0EFF | 256 Bytes | Power Reset Module MPU Registers |


*Table 2-2. L4\_WKUP Peripheral Memory Map (continued)*

| Region Name | Start Address(hex) | End Address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
| PRM_DEV | 0x44E0_0F00 | 0x44E0_0FFF | 256 Bytes | Power Reset Module Device Registers |
| PRM_RTC | 0x44E0_1000 | 0x44E0_10FF | 256 Bytes | Power Reset Module RTC Registers |
| PRM_GFX | 0x44E0_1100 | 0x44E0_11FF | 256 Bytes | Power Reset Module Graphics Controller Registers |
| PRM_CEFUSE | 0x44E0_1200 | 0x44E0_12FF | 256 Bytes | Power Reset Module Efuse Registers |
| Reserved | 0x44E0_3000 | 0x44E0_3FFF | 4 KB | Reserved |
|  | 0x44E0_4000 | 0x44E0_4FFF | 4 KB | Reserved |
| DMTIMER0 | 0x44E0_5000 | 0x44E0_5FFF | 4 KB | DM Timer0Registers |
|  | 0x44E0_6000 | 0x44E0_6FFF | 4 KB | Reserved |
| GPIO0 | 0x44E0_7000 | 0x44E0_7FFF | 4 KB | GPIO Registers |
|  | 0x44E0_8000 | 0x44E0_8FFF | 4 KB | Reserved |
| UART0 | 0x44E0_9000 | 0x44E0_9FFF | 4 KB | UART Registers |
|  | 0x44E0_A000 | 0x44E0_AFFF | 4 KB | Reserved |
| I2C0 | 0x44E0_B000 | 0x44E0_BFFF | 4 KB | I2CRegisters |
|  | 0x44E0_C000 | 0x44E0_CFFF | 4 KB | Reserved |
| ADC_TSC | 0x44E0_D000 | 0x44E0_EFFF | 8 KB | ADC_TSC Registers |
|  | 0x44E0_F000 | 0x44E0_FFFF | 4 KB | Reserved |
| Control Module | 0x44E1_0000 | 0x44E1_1FFF | 128 KB | Control Module Registers |
| DDR2/3/m DDRPHY | 0x44E1_2000 | 0x44E1_23FF |  | DDR2/3/m DDRPHY Registers |
| Reserved | 0x44E1_2400 | 0x44E3_0FFF | 4 KB | Reserved |
| DMTIMER1_1MS (Accurate1mstimer) | 0x44E3_1000 | 0x44E3_1FFF | 4 KB | DM Timer11ms Registers |
|  | 0x44E3_2000 | 0x44E3_2FFF | 4 KB | Reserved |
| Reserved | 0x44E3_3000 | 0x44E3_3FFF | 4 KB | Reserved |
|  | 0x44E3_4000 | 0x44E3_4FFF | 4 KB | Reserved |
| WDT1 | 0x44E3_5000 | 0x44E3_5FFF | 4 KB | Watchdog Timer Registers |
|  | 0x44E3_6000 | 0x44E3_6FFF | 4 KB | Reserved |
| Smart Reflex0 | 0x44E3_7000 | 0x44E3_7FFF | 4 KB | L3Registers |
|  | 0x44E3_8000 | 0x44E3_8FFF | 4 KB | Reserved |
| Smart Reflex1 | 0x44E3_9000 | 0x44E3_9FFF | 4 KB | L3Registers |
|  | 0x44E3_A000 | 0x44E3_AFFF | 4 KB | Reserved |
| Reserved | 0x44E3_B000 | 0x44E3_DFFF | 12 KB | Reserved |
| RTCSS | 0x44E3_E000 | 0x44E3_EFFF | 4 KB | RTC Registers |
|  | 0x44E3_F000 | 0x44E3_FFFF | 4 KB | Reserved |
| Debug SS Instrumentation HW Master1Port | 0x44E4_0000 | 0x44E7_FFFF | 256 KB | Debug Registers |
|  | 0x44E8_0000 | 0x44E8_0FFF | 4 KB | Reserved |
| Reserved | 0x44E8_1000 | 0x44EF_FFFF | 508 KB | Reserved |
| Reserved | 0x44F0_0000 | 0x44FF_FFFF | 1 MB | Reserved |


### Table 2-3. L4\_PER Peripheral Memory Map

| Device Name | Start_address(hex) | End_address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
| Reserved | 0x4800_0000 | 0x4800_07FF | 2 KB | Reserved |
|  | 0x4800_0800 | 0x4800_0FFF | 2 KB | Reserved |
|  | 0x4800_1000 | 0x4800_13FF | 1 KB | Reserved |


*Table 2-3. L4\_PER Peripheral Memory Map (continued)*

| Device Name | Start_address(hex) | End_address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
|  | 0x4800_1400 | 0x4800_17FF | 1 KB | Reserved |
|  | 0x4800_1800 | 0x4800_1BFF | 1 KB | Reserved |
|  | 0x4800_1C00 | 0x4800_1FFF | 1 KB | Reserved |
| Reserved | 0x4800_2000 | 0x4800_3FFF | 8 KB | Reserved |
| Reserved | 0x4800_4000 | 0x4800_7FFF | 16 KB | Reserved |
| Reserved | 0x4800_8000 | 0x4800_8FFF | 4 KB | Reserved |
|  | 0x4800_9000 | 0x4800_9FFF | 4 KB | Reserved |
| Reserved | 0x4800_A000 | 0x4800_FFFF | 24 KB | Reserved |
| Reserved | 0x4801_0000 | 0x4801_0FFF | 4 KB | Reserved |
|  | 0x4801_1000 | 0x4801_1FFF | 4 KB | Reserved |
| Reserved | 0x4801_2000 | 0x4801_3FFF | 8 KB | Reserved |
| Reserved | 0x4801_4000 | 0x4801_FFFF | 48 KB | Reserved |
| Reserved | 0x4802_0000 | 0x4802_0FFF | 4 KB | Reserved |
|  | 0x4802_1000 | 0x4802_1FFF | 4 KB | Reserved |
| UART1 | 0x4802_2000 | 0x4802_2FFF | 4 KB | UART1 Registers |
|  | 0x4802_3000 | 0x4802_3FFF | 4 KB | Reserved |
| UART2 | 0x4802_4000 | 0x4802_4FFF | 4 KB | UART2 Registers |
|  | 0x4802_5000 | 0x4802_5FFF | 4 KB | Reserved |
| Reserved | 0x4802_6000 | 0x4802_7FFF | 8 KB | Reserved |
| Reserved | 0x4802_8000 | 0x4802_8FFF | 4 KB | Reserved |
|  | 0x4802_9000 | 0x4802_9FFF | 4 KB | Reserved |
| I2C1 | 0x4802_A000 | 0x4802_AFFF | 4 KB | I2C1 Registers |
|  | 0x4802_B000 | 0x4802_BFFF | 4 KB | Reserved |
| Reserved | 0x4802_C000 | 0x4802_CFFF | 4 KB | Reserved |
|  | 0x4802_D000 | 0x4802_DFFF | 4 KB | Reserved |
| Reserved | 0x4802_E000 | 0x4802_EFFF | 4 KB | Reserved |
|  | 0x4802_F000 | 0x4802_FFFF | 4 KB | Reserved |
| McSPI0 | 0x4803_0000 | 0x4803_0FFF | 4 KB | McSPI0 Registers |
|  | 0x4803_1000 | 0x4803_1FFF | 4 KB | Reserved |
| Reserved | 0x4803_2000 | 0x4803_2FFF | 4 KB | Reserved |
|  | 0x4803_3000 | 0x4803_3FFF | 4 KB | Reserved |
| Reserved | 0x4803_4000 | 0x4803_4FFF | 4 KB | Reserved |
|  | 0x4803_5000 | 0x4803_5FFF | 4 KB | Reserved |
| Reserved | 0x4803_6000 | 0x4803_6FFF | 4 KB | Reserved |
|  | 0x4803_7000 | 0x4803_7FFF | 4 KB | Reserved |
| McASP0 CFG | 0x4803_8000 | 0x4803_9FFF | 8 KB | McASP0 CFG Registers |
|  | 0x4803_A000 | 0x4803_AFFF | 4 KB | Reserved |
| Reserved | 0x4803_B000 | 0x4803_BFFF | 4 KB | Reserved |
| McASP1 CFG | 0x4803_C000 | 0x4803_DFFF | 8 KB | McASP1 CFG Registers |
|  | 0x4803_E000 | 0x4803_EFFF | 4 KB | Reserved |
| Reserved | 0x4803_F000 | 0x4803_FFFF | 4 KB | Reserved |
| DMTIMER2 | 0x4804_0000 | 0x4804_0FFF | 4 KB | DM Timer2Registers |
|  | 0x4804_1000 | 0x4804_1FFF | 4 KB | Reserved |
| DMTIMER3 | 0x4804_2000 | 0x4804_2FFF | 4 KB | DM Timer3Registers |
|  | 0x4804_3000 | 0x4804_3FFF | 4 KB | Reserved |
| DMTIMER4 | 0x4804_4000 | 0x4804_4FFF | 4 KB | DM Timer4Registers |
|  | 0x4804_5000 | 0x4804_5FFF | 4 KB | Reserved |


*Table 2-3. L4\_PER Peripheral Memory Map (continued)*

| Device Name | Start_address(hex) | End_address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
| DMTIMER5 | 0x4804_6000 | 0x4804_6FFF | 4 KB | DM Timer5Registers |
|  | 0x4804_7000 | 0x4804_7FFF | 4 KB | Reserved |
| DMTIMER6 | 0x4804_8000 | 0x4804_8FFF | 4 KB | DM Timer6Registers |
|  | 0x4804_9000 | 0x4804_9FFF | 4 KB | L4 Interconnect |
| DMTIMER7 | 0x4804_A000 | 0x4804_AFFF | 4 KB | DM Timer7Registers |
|  | 0x4804_B000 | 0x4804_BFFF | 4 KB | Reserved |
| GPIO1 | 0x4804_C000 | 0x4804_CFFF | 4 KB | GPIO1 Registers |
|  | 0x4804_D000 | 0x4804_DFFF | 4 KB | Reserved |
| Reserved | 0x4804_E000 | 0x4804_FFFF | 8 KB | Reserved |
| Reserved | 0x4805_0000 | 0x4805_FFFF | 64 KB | Reserved |
| MMCHS0 | 0x4806_0000 | 0x4806_0FFF | 4 KB | MMCHS0 Registers |
|  | 0x4806_1000 | 0x4806_1FFF | 4 KB | Reserved |
| Reserved | 0x4806_2000 | 0x4807_FFFF | 120 KB | Reserved |
| ELM | 0x4808_0000 | 0x4808_FFFF | 64 KB | ELM Registers |
|  | 0x4809_0000 | 0x4809_0FFF | 4 KB | Reserved |
| Reserved | 0x4809_1000 | 0x4809_FFFF | 60 KB | Reserved |
| Reserved | 0x480A_0000 | 0x480A_FFFF | 64 KB | Reserved |
|  | 0x480B_0000 | 0x480B_0FFF | 4 KB | Reserved |
| Reserved | 0x480B_1000 | 0x480B_FFFF | 60 KB | Reserved |
| Reserved | 0x480C_0000 | 0x480C_0FFF | 4 KB | Reserved |
|  | 0x480C_1000 | 0x480C_1FFF | 4 KB | Reserved |
| Reserved | 0x480C_2000 | 0x480C_2FFF | 4 KB | Reserved |
|  | 0x480C_3000 | 0x480C_3FFF | 4 KB | Reserved |
| Reserved | 0x480C_4000 | 0x480C_7FFF | 16 KB | Reserved |
| Mailbox 0 | 0x480C_8000 | 0x480C_8FFF | 4 KB | Mailbox Registers |
|  | 0x480C_9000 | 0x480C_9FFF | 4 KB | Reserved |
| Spinlock | 0x480C_A000 | 0x480C_AFFF | 4 KB | Spinlock Registers |
|  | 0x480C_B000 | 0x480C_BFFF | 4 KB | Reserved |
| Reserved | 0x480C_C000 | 0x480F_FFFF | 208 KB | Reserved |
| Reserved | 0x4810_0000 | 0x4811_FFFF | 128 KB | Reserved |
|  | 0x4812_0000 | 0x4812_0FFF | 4 KB | Reserved |
| Reserved | 0x4812_1000 | 0x4812_1FFF | 4 KB | Reserved |
| Reserved | 0x4812_2000 | 0x4812_2FFF | 4 KB | Reserved |
|  | 0x4812_3000 | 0x4812_3FFF | 4 KB | Reserved |
| Reserved | 0x4812_4000 | 0x4813_FFFF | 112 KB | Reserved |
| Reserved | 0x4814_0000 | 0x4815_FFFF | 128 KB | Reserved |
|  | 0x4816_0000 | 0x4816_0FFF | 4K | Reserved |
| Reserved | 0x4816_1000 | 0x4817_FFFF | 124 KB | Reserved |
| Reserved | 0x4818_0000 | 0x4818_2FFF | 12 KB | Reserved |
|  | 0x4818_3000 | 0x4818_3FFF | 4 KB | Reserved |
| Reserved | 0x4818_4000 | 0x4818_7FFF | 16 KB | Reserved |
| Reserved | 0x4818_8000 | 0x4818_8FFF | 4 KB | Reserved |
|  | 0x4818_9000 | 0x4818_9FFF | 4 KB | Reserved |
| Reserved | 0x4818_A000 | 0x4818_AFFF | 4 KB | Reserved |
|  | 0x4818_B000 | 0x4818_BFFF | 4 KB | Reserved |
| OCP Watchpoint | 0x4818_C000 | 0x4818_CFFF | 4 KB | OCP Watchpoint Registers |
|  | 0x4818_D000 | 0x4818_DFFF | 4 KB | Reserved |


*Table 2-3. L4\_PER Peripheral Memory Map (continued)*

| Device Name | Start_address(hex) | End_address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
| Reserved | 0x4818_E000 | 0x4818_EFFF | 4 KB | Reserved |
|  | 0x4818_F000 | 0x4818_FFFF | 4 KB | Reserved |
| Reserved | 0x4819_0000 | 0x4819_0FFF | 4 KB | Reserved |
|  | 0x4819_1000 | 0x4819_1FFF | 4 KB | Reserved |
| Reserved | 0x4819_2000 | 0x4819_2FFF | 4 KB | Reserved |
|  | 0x4819_3000 | 0x4819_3FFF | 4 KB | Reserved |
| Reserved | 0x4819_4000 | 0x4819_BFFF | 32 KB | Reserved |
| I2C2 | 0x4819_C000 | 0x4819_CFFF | 4 KB | I2C2 Registers |
|  | 0x4819_D000 | 0x4819_DFFF | 4 KB | Reserved |
| Reserved | 0x4819_E000 | 0x4819_EFFF | 4 KB | Reserved |
|  | 0x4819_F000 | 0x4819_FFFF | 4 KB | Reserved |
| McSPI1 | 0x481A_0000 | 0x481A_0FFF | 4 KB | McSPI1 Registers |
|  | 0x481A_1000 | 0x481A_1FFF | 4 KB | Reserved |
| Reserved | 0x481A_2000 | 0x481A_5FFF | 16 KB | Reserved |
| UART3 | 0x481A_6000 | 0x481A_6FFF | 4 KB | UART3 Registers |
|  | 0x481A_7000 | 0x481A_7FFF | 4 KB | Reserved |
| UART4 | 0x481A_8000 | 0x481A_8FFF | 4 KB | UART4 Registers |
|  | 0x481A_9000 | 0x481A_9FFF | 4 KB | Reserved |
| UART5 | 0x481A_A000 | 0x481A_AFFF | 4 KB | UART5 Registers |
|  | 0x481A_B000 | 0x481A_BFFF | 4 KB | Reserved |
| GPIO2 | 0x481A_C000 | 0x481A_CFFF | 4 KB | GPIO2 Registers |
|  | 0x481A_D000 | 0x481A_DFFF | 4 KB | Reserved |
| GPIO3 | 0x481A_E000 | 0x481A_EFFF | 4 KB | GPIO3 Registers |
|  | 0x481A_F000 | 0x481A_FFFF | 4 KB | Reserved |
| Reserved | 0x481B_0000 | 0x481B_FFFF | 64 KB | Reserved |
|  | 0x481C_0000 | 0x481C_0FFF | 4 KB | Reserved |
| Reserved | 0x481C_1000 | 0x481C_1FFF | 4 KB | Reserved |
|  | 0x481C_2000 | 0x481C_2FFF | 4 KB | Reserved |
| Reserved | 0x481C_3000 | 0x481C_9FFF | 28 KB | Reserved |
| Reserved | 0x481C_A000 | 0x481C_AFFF | 4 KB | Reserved |
|  | 0x481C_B000 | 0x481C_BFFF | 4 KB | Reserved |
| DCAN0 | 0x481C_C000 | 0x481C_DFFF | 8 KB | DCAN0 Registers |
|  | 0x481C_E000 | 0x481C_FFFF | 8 KB | Reserved |
| DCAN1 | 0x481D_0000 | 0x481D_1FFF | 8 KB | DCAN1 Registers |
|  | 0x481D_2000 | 0x481D_3FFF | 8 KB | Reserved |
| Reserved | 0x481D_4000 | 0x481D_4FFF | 4 KB | Reserved |
|  | 0x481D_5000 | 0x481D_5FFF | 4 KB | Reserved |
| Reserved | 0x481D_6000 | 0x481D_6FFF | 4 KB | Reserved |
|  | 0x481D_7000 | 0x481D_7FFF | 4 KB | Reserved |
| MMC1 | 0x481D_8000 | 0x481D_8FFF | 4 KB | MMC1 Registers |
|  | 0x481D_9000 | 0x481D_9FFF | 4 KB | Reserved |
| Reserved | 0x481D_A000 | 0x481F_FFFF | 152 KB | Reserved |
| Interruptcontroller (INTCPS) | 0x4820_0000 | 0x4820_0FFF | 4 KB | Interrupt controller Registers |
| Reserved | 0x4820_1000 | 0x4823_FFFF | 252 KB | Reserved |
| MPUS Sconfigregister | 0x4824_0000 | 0x4824_0FFF | 4 KB | Host AR Mnon-shareddevice mapping |
| Reserved | 0x4824_1000 | 0x4827_FFFF | 252 KB | Reserved |


*Table 2-3. L4\_PER Peripheral Memory Map (continued)*

| Device Name | Start_address(hex) | End_address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
| Reserved | 0x4828_0000 | 0x4828_0FFF | 4 KB | Reserved |
| Reserved | 0x4828_1000 | 0x482F_FFFF | 508 KB | Reserved |
| PWM Subsystem0 | 0x4830_0000 | 0x4830_00FF | 4 KB | PWMSS0 Configuration Registers |
| e CAP0 | 0x4830_0100 | 0x4830_017F |  | PWMS Se CAP0Registers |
| e QEP0 | 0x4830_0180 | 0x4830_01FF |  | PWMS Se QEP0Registers |
| e PWM0 | 0x4830_0200 | 0x4830_025F |  | PWMS Se PWM0Registers |
|  | 0x4830_0260 | 0x4830_1FFF | 4 KB | Reserved |
| PWM Subsystem1 | 0x4830_2000 | 0x4830_20FF | 4 KB | PWMSS1Configuration Registers |
| e CAP1 | 0x4830_2100 | 0x4830_217F |  | PWMS Se CAP1Registers |
| e QEP1 | 0x4830_2180 | 0x4830_21FF |  | PWMS Se QEP1Registers |
| e PWM1 | 0x4830_2200 | 0x4830_225F |  | PWMS Se PWM1Registers |
|  | 0x4830_2260 | 0x4830_3FFF | 4 KB | Reserved |
| PWM Subsystem2 | 0x4830_4000 | 0x4830_40FF | 4 KB | PWMSS2Configuration Registers |
| e CAP2 | 0x4830_4100 | 0x4830_417F |  | PWMS Se CAP2Registers |
| e QEP2 | 0x4830_4180 | 0x4830_41FF |  | PWMS Se QEP2Registers |
| e PWM2 | 0x4830_4200 | 0x4830_425F |  | PWMS Se PWM2Registers |
|  | 0x4830_4260 | 0x4830_5FFF | 4 KB | Reserved |
| Reserved | 0x4830_6000 | 0x4830_DFFF | 32 KB | Reserved |
| LCD Controller | 0x4830_E000 | 0x4830_EFFF | 4 KB | LCD Registers |
|  | 0x4830_F000 | 0x4830_FFFF | 4 KB | Reserved |
| Reserved | 0x4831_0000 | 0x4831_1FFF | 8 KB | Reserved |
|  | 0x4831_2000 | 0x4831_2FFF | 4 KB | Reserved |
| Reserved | 0x4831_3000 | 0x4831_7FFF | 20 KB | Reserved |
| Reserved | 0x4831_8000 | 0x4831_BFFF | 16 KB | Reserved |
|  | 0x4831_C000 | 0x4831_CFFF | 4 KB | Reserved |
| Reserved | 0x4831_D000 | 0x4831_FFFF | 12 KB | Reserved |
| Reserved | 0x4832_0000 | 0x4832_5FFF | 16 KB | Reserved |
| Reserved | 0x4832_6000 | 0x48FF_FFFF | 13 MB-152 KB | Reserved |


### Table 2-4. L4 Fast Peripheral Memory Map

| Device Name | Start_address(hex) | End_address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
| L4_Fast configuration | 0x4A00_0000 | 0x4A00_07FF | 2 KB | Address/Protection(AP) |
|  | 0x4A00_0800 | 0x4A00_0FFF | 2 KB | Link Agent(LA) |
|  | 0x4A00_1000 | 0x4A00_13FF | 1 KB | Initiator Port(IP0) |
|  | 0x4A00_1400 | 0x4A00_17FF | 1 KB | Reserved |
|  | 0x4A00_1800 | 0x4A00_1FFF | 2 KB | Reserved(IP2 – IP3) |
| Reserved | 0x4A00_2000 | 0x4A07_FFFF | 504 KB | Reserved |
| Reserved | 0x4A08_0000 | 0x4A09_FFFF | 128 KB | Reserved |
|  | 0x4A0A_0000 | 0x4A0A_0FFF | 4 KB | Reserved |
| Reserved | 0x4A0A_1000 | 0x4A0F_FFFF | 380 KB | Reserved |
| CPSW_SS | 0x4A10_0000 | 0x4A10_7FFF | 32 KB | Ethernet Switch Subsystem |
| CPSW_PORT | 0x4A10_0100 | 0x4A10_07FF |  | Ethernet Switch Port Control |
| CPSW_CPDMA | 0x4A10_0800 | 0x4A10_08FF |  | CPPIDMA Controller Module |
| CPSW_STATS | 0x4A10_0900 | 0x4A10_09FF |  | Ethernet Statistics |


*Table 2-4. L4 Fast Peripheral Memory Map (continued)*

| Device Name | Start_address(hex) | End_address(hex) | Size | Description |
| --- | --- | --- | --- | --- |
| CPSW_STATERAM | 0x4A10_0A00 | 0x4A10_0BFF |  | CPPIDMA State RAM |
| CPSW_CPTS | 0x4A10_0C00 | 0x4A10_0CFF |  | Ethernet Time Sync Module |
| CPSW_ALE | 0x4A10_0D00 | 0x4A10_0D7F |  | Ethernet Address Lookup Engine |
| CPSW_SL1 | 0x4A10_0D80 | 0x4A10_0DBF |  | Ethernet Sliverfor Port1 |
| CPSW_SL2 | 0x4A10_0DC0 | 0x4A10_0DFF |  | Ethernet Sliverfor Port2 |
| Reserved | 0x4A10_0E00 | 0x4A10_0FFF |  | Reserved |
| MDIO | 0x4A10_1000 | 0x4A10_10FF |  | Ethernet MDIO Controller |
| Reserved | 0x4A10_1100 | 0x4A10_11FF |  | Reserved |
| CPSW_WR | 0x4A10_1200 | 0x4A10_1FFF |  | Ethernet Subsystem Wrapperfor RMII/RGMII |
| CPPI_RAM | 0x4A10_2000 | 0x4A10_3FFF |  | Communications Port Programming Interface RAM |
| Reserved | 0x4A10_9000 | 0x4A13_FFFF | 220 KB | Reserved |
| Reserved | 0x4A14_0000 | 0x4A14_FFFF | 64 KB | Reserved |
|  | 0x4A15_0000 | 0x4A15_0FFF | 4 KB | Reserved |
| Reserved | 0x4A15_1000 | 0x4A17_FFFF | 188 KB | Reserved |
| Reserved | 0x4A18_0000 | 0x4A1A_1FFF | 136 KB | Reserved |
| Reserved | 0x4A1A_2000 | 0x4A1A_3FFF | 8 KB | Reserved |
|  | 0x4A1A_4000 | 0x4A1A_4FFF | 4 KB | Reserved |
| Reserved | 0x4A1A_5000 | 0x4A1A_5FFF | 4 KB | Reserved |
|  | 0x4A1A_6000 | 0x4A1A_6FFF | 4 KB | Reserved |
| Reserved | 0x4A1A_7000 | 0x4A1A_7FFF | 4 KB | Reserved |
| Reserved | 0x4A1A_8000 | 0x4A1A_9FFF | 8 KB | Reserved |
|  | 0x4A1A_A000 | 0x4A1A_AFFF | 4 KB | Reserved |
| Reserved | 0x4A1A_B000 | 0x4A1A_BFFF | 4 KB | Reserved |
|  | 0x4A1A_C000 | 0x4A1A_CFFF | 4 KB | Reserved |
| Reserved | 0x4A1A_D000 | 0x4A1A_DFFF | 4 KB | Reserved |
| Reserved | 0x4A1A_E000 | 0x4A1A_FFFF | 8 KB | Reserved |
|  | 0x4A1B_0000 | 0x4A1B_0FFF | 4 KB | Reserved |
| Reserved | 0x4A1B_1000 | 0x4A1B_1FFF | 4 KB | Reserved |
|  | 0x4A1B_2000 | 0x4A1B_2FFF | 4 KB | Reserved |
| Reserved | 0x4A1B_3000 | 0x4A1B_3FFF | 4 KB | Reserved |
|  | 0x4A1B_4000 | 0x4A1B_4FFF | 4 KB | Reserved |
| Reserved | 0x4A1B_5000 | 0x4A1B_5FFF | 4 KB | Reserved |
|  | 0x4A1B_6000 | 0x4A1B_6FFF | 4 KB | Reserved |
| Reserved | 0x4A1B_4000 | 0x4A1F_FFFF | 304 KB | Reserved |
| Reserved | 0x4A20_0000 | 0x4A2F_FFFF | 1 MB | Reserved |
| PRU_ICSS | 0x4A30_0000 | 0x4A37_FFFF | 512 KB | PRU-ICSS Instruction/Data/Control Space |
|  | 0x4A38_0000 | 0x4A38_0FFF | 4 KB | Reserved |
| Reserved | 0x4A38_1000 | 0x4A3F_FFFF | 508 KB | Reserved |
| Reserved | 0x4A40_0000 | 0x4AFF_FFFF | 12 MB | Reserved |


---
**Notes:**
1. The first 1 MB of address space (0x0–0xFFFFF) is inaccessible externally.
2. Ex/R/W – Execute/Read/Write.

*Source: SPRUH73Q – October 2011 – Revised December 2019, Texas Instruments Incorporated*
