# 26 Initialization

## 26.1 Functional Description

This chapter describes the booting functionality of the device, referred to hereafter as **ROM Code**. The booting functionality covers the following features:

- **Memory Booting**: Booting the device by starting code stored on permanent memories like flash-memory or memory cards. This process is totally autonomous and can be done on device cold or warm reset.

- **Peripheral Booting**: Booting the device by downloading the executable code over a communication interface like UART, USB or Ethernet. This process is needed for flashing a device.

The device always starts up in secure mode. The ROM Code takes care of early initialization. The ROM code switches the device into public mode. Hence the Public ROM Code provides run-time services for cache maintenance.

---

### 26.1.1 Device Types

This device has two types of production devices: a high-secure (HS) device and a general-purpose (GP) device. The fundamental difference between the two types of production devices is the method to secure booting. The HS Device doesn't allow booting code which is not trusted (authenticated), that is digitally signed and checked for its integrity. The GP Device doesn't support such security features disabled and does not require signed code for booting the device. Image formats are different depending on the device type and described in this chapter.

---

### 26.1.2 Architecture

The architecture of the Public ROM Code is shown in the diagram below. It is split into three main layers with a top-down approach: high-level, drivers, and hardware abstraction layer (HAL). One layer communicates with a lower level layer through a unified interface.

**Architecture Layers:**

- **High-level layer**: In charge of the main tasks of the Public ROM Code: watchdog and clocks configuration and main booting routine.

- **Drivers layer**: Implements the logical and communication protocols for any booting device in accordance with the interface specification.

- **HAL (Hardware Abstraction Layer)**: Implements the lowest level code for interacting with the hardware infrastructure IPs. End booting devices are attached to device IO pads.

*[Sơ đồ Figure 26-1: Public ROM Code Architecture - Quan trọng - Giữ lại]*

```
┌─────────────────────────────────────────────────────────────────┐
│                            HLOS                                  │
└─────────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────────┐
│                    Public ROM Code (High Level)                  │
│  ┌──────┐  ┌────────┐  ┌──────────┐  ┌─────┐                   │
│  │ MAIN │  │CLOCKS  │  │ BOOTING  │  │ DFT │                    │
│  └──────┘  └────────┘  └──────────┘  └─────┘                    │
│  ┌──────┐  ┌────────┐  ┌──────┐  ┌─────┐  ┌──────┐  ┌──────┐  │
│  │RNDIS │  │SYSTEM  │  │SWCFG │  │ FAT │  │XMODEM│  │BOOTP │  │TFTP │
│  └──────┘  └────────┘  └──────┘  └─────┘  └──────┘  └──────┘  └──────┘
├─────────────────────────────────────────────────────────────────┤
│                    Public ROM Code drivers                       │
│  ┌──────┐  ┌──────┐  ┌─────┐  ┌─────┐  ┌─────┐  ┌──────┐      │
│  │ EMAC │  │ UART │  │ USB │  │ SPI │  │ XIP │  │ NAND │  │eMMCSD│
│  └──────┘  └──────┘  └─────┘  └─────┘  └─────┘  └──────┘  └──────┘
├─────────────────────────────────────────────────────────────────┤
│                     Public ROM Code HAL                          │
│  ┌─────┐  ┌──────────┐  ┌─────┐  ┌────────┐  ┌─────────┐       │
│  │ USB │  │DMTIMERTMS│  │ ELM │  │WDTIMER │  │CONTROL  │  │PRM │  │CM │
│  └─────┘  └──────────┘  └─────┘  └────────┘  └─────────┘  └───┘ └───┘
│  ┌──────┐  ┌──────┐  ┌─────┐  ┌─────┐  ┌──────┐  ┌──────┐      │
│  │MMCHS │  │ GPMC │  │ SPI │  │ I2C │  │CKGEN │  │ UART │       │
│  └──────┘  └──────┘  └─────┘  └─────┘  └──────┘  └──────┘       │
└─────────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────────┐
│                            HW                                    │
│  ┌────────┐  ┌──────────┐  ┌────────┐  ┌─────┐  ┌──────┐       │
│  │MMC+SD  │  │NAND flash│  │SPI flash│  │ XIP │  │ UART │  │EMAC│
│  │cards   │  │          │  │         │  │     │  │      │  │    │
│  └────────┘  └──────────┘  └────────┘  └─────┘  └──────┘  └────┘
│             ┌──────┐                                              │
│             │ USB  │                                              │
│             └──────┘                                              │
└─────────────────────────────────────────────────────────────────┘
```

---

### 26.1.3 Functionality

The booting procedure is illustrated in the high level flow diagrams below. On this device the Public ROM Code starts upon completion of the secure startup (performed by the Secure ROM Code). The Public ROM Code performs early initialization as part of the public start-up procedure.

The booting device list is created based on the **SYSBOOT** pins. A booting device can be a **memory booting device** (soldered flash memory or temporarily booting device like memory card) or a **peripheral interface** connected to a host.

The main loop of the booting procedure goes through the booting device list and tries to search for an image from the currently selected image device. This loop is exited if a valid booting image is found and successfully executed or upon watchdog expiration.

The image authentication procedure is performed prior to image execution on an **HS Device**. Failure in authentication procedure leads to branching to a "dead loop" for a secure device (generates a watchdog reset).

*[Sơ đồ Figure 26-2 và 26-3: Public ROM Code Boot Procedure - Quan trọng - Giữ lại]*

**Figure 26-2: ROM Code Boot Procedure (Simplified)**

```
System start-up
      │
      ▼
Set up the booting device list
      │
      ▼
Take next device from the list
      │
      ▼
  ┌─────────────┐
  │  Memory     │ Yes → Memory Booting
  │  Device?    │
  └─────────────┘
       │ No
       ▼
   Peripheral Booting
       │
       ▼
   ┌──────────┐
   │ Success? │ Yes → Image execution
   └──────────┘
       │ No
       │
       ▼ (Loop back to next device)
```

**Figure 26-3: ROM Code Boot Procedure (Detailed - HS Device)**

```
System start-up
      │
      ▼
Set up the booting device list
      │
      ▼
Take next device from the list
      │
      ▼
  ┌─────────────┐
  │  Memory     │ Yes → Memory Booting
  │  Device?    │
  └─────────────┘
       │ No
       ▼
   Peripheral Booting
       │
       ▼
   ┌──────────┐
   │ Success? │ No → Get next device
   └──────────┘
       │ Yes
       ▼
   ┌─────────────┐
   │ HS Device?  │ No → Image execution
   └─────────────┘
       │ Yes
       ▼
   Image authentication
       │
       ▼
   ┌──────────────────┐
   │ Authentication   │ No → Dead loop (watchdog reset)
   │ successful?      │
   └──────────────────┘
       │ Yes
       ▼
   Image execution
```

---

### 26.1.4 Memory Map

#### 26.1.4.1 Public ROM Memory Map

The on-chip ROM memory map is shown in the figures below. The bottom part of the physical ROM contains the Secure ROM Code. The upper part holds the Public ROM Code. The Public ROM Code mapping consists in the following:

- Exception vectors
- CRC
- Dead loops collection
- Code and const data sections
- ROM Version

*[Sơ đồ Figure 26-4 và 26-5: ROM Memory Map - Quan trọng - Giữ lại]*

**Figure 26-4: ROM Memory Map (Physical)**

```
0x2BFFF ┌─────────────────┐
        │  ROM Version    │
0x2BFFC └─────────────────┘
        │                 │
        │      Code       │
        │                 │
0x20100 ├─────────────────┤
        │  Dead loops     │
0x20080 ├─────────────────┤
0x20020 │ Public ROM CRC  │
        ├─────────────────┤
0x20000 │ ROM Exc. Vectors│
        └─────────────────┘
```

**Figure 26-5: ROM Memory Map (Logical - GP/HS)**

```
        ┌─────────────────┐ 0x2BFFF
        │  ROM Version    │
        │                 │ 0x2BFFC
        ├─────────────────┤
        │                 │
        │      Code       │
        │                 │
        │                 │ 0x20100
        ├─────────────────┤
        │  Dead loops     │ 0x20080
        ├─────────────────┤
        │ Public ROM CRC  │ 0x20020
        ├─────────────────┤
Public  │ ROM Exc. Vectors│ 0x20000
ROM     └─────────────────┘
48KB            ╲
                 ╲ Secure ROM
0x20000          ╲ 128KB
                  ╲
                   ╲
                    ╲
0x00000 ────────────────────
```

---

### Public ROM Exception Vectors

Exception vectors table lists the Public ROM exception vectors. The reset exception is redirected to the Public ROM Code startup. Other exceptions are redirected to their RAM handlers by loading appropriate addresses into the PC register.

**Table 26-1: ROM Exception Vectors**

| Address | Exception | Content |
|---------|-----------|---------|
| 20000h | Reset | Branch to the Public ROM Code startup |
| 20004h | Undefined | PC = 4030CE04h |
| 20008h | SWI | PC = 4030CE08h |
| 2000Ch | Pre-fetch abort | PC = 4030CE0Ch |
| 20010h | Data abort | PC = 4030CE10h |
| 20014h | Unused | PC = 4030CE14h |
| 20018h | IRQ | PC = 4030CE18h |
| 2001Ch | FIQ | PC = 4030CE1Ch |

---

### Public ROM Code CRC

The Public ROM Code CRC is calculated as 32 bit CRC code (CRC-32 IEEE 802.3) for the address range 20000h-2BFFFh. The four bytes CRC code is stored at location 20020h.

---

### Dead Loops

Built-in dead loops are used for different purposes as shown in the table below. All dead loops are branch instructions coded in ARM mode. The list below lists typical uses of these dead loops which can be called from ROM Code in order to execute a dead loop. The function is an assembly code in ARM mode which takes the dead loop address as first register parameter. However there exists a special case which can be called directly from the ROM Code, but can only be used on a GP device. For an HS device, the function must first call the requested dead loop. The function is located at address 2000Ch. In addition the function clears global cold reset status upon issuing the global SW reset.

**Table 26-2: Dead Loops**

| Address | Purpose |
|---------|---------|
| 20080h | Undefined exception default handler |
| 20084h | SWI exception default handler |
| 20088h | Pre-fetch abort exception default handler |
| 2008Ch | Data abort exception default handler |
| 20090h | Unused exception default handler |
| 20094h | IRQ exception default handler |
| 20098h | FIQ exception default handler |
| 2009Ch | Validation tests PASS |
| 200A0h | Validation tests FAIL |
| 200A4h | Reserved |
| 200A8h | Image not executed or returned |
| 200ACh | Reserved |
| 200B0h | Reserved |
| 200B4h | Reserved |
| 200B8h | Reserved |
| 200BCh | Reserved |

---

### Code

This space is used to hold code and constant data.

---

### Public ROM Code Version

The ROM Code version consists of two decimal numbers: major and minor. It can be used to identify the ROM Code release version in a given IC version. The ROM Code version is stored at address 2BFCh as a 32bits hexadecimal value located at address 2BFFCh.

---

#### 26.1.4.2 Public RAM Memory Map

The Public ROM Code makes use of the on chip RAM module connected to the L3 interconnect (further referred to as **L3 RAM**). Its usage is shown in the figures below. The Public RAM memory map ranges from address 402F0400h to 4030FFFFh on a **GP Device**.

*[Sơ đồ Figure 26-6 và 26-7: Public RAM Memory Map - Có thể đơn giản hóa]*

**Public RAM Memory Layout (GP Device):**

```
0x4030FFFF ┌─────────────────────────┐
           │ Static Variables        │
           │ Tracing Data            │
0x4030CE00 │ RAM Exc. Vectors        │
           ├─────────────────────────┤
           │ 8KB Public stack        │
0x4030B800 ├─────────────────────────┤
           │                         │
           │                         │
           │   Downloaded Image      │
           │                         │
           │                         │
0x402F0400 └─────────────────────────┘
(GP)       0x402F0400

HS Device: Reserved area 0x4030F000 to 0x4030008900
```

---

### Download Image

This area is used by the Public ROM Code to store the downloaded boot image. It can be up to 109KB on the GP Device and 46KB for the HS device.

---

### Public Stack

Space reserved for stack.

---

### RAM Exception Vectors

The RAM exception vectors enable a simple means for redirecting exceptions to custom handlers. The table shows the addresses reserved for RAM exception vectors. The six exception handlers are relocated in RAM as a consecutive list of branch instructions in ARM mode. These instructions are executed when an exception occurs since they are called from the ROM exception vectors. Undefined, SWI, Unused and FIQ exceptions are redirected to a hardcoded dead loop. Pre-fetch abort, data abort, and IRQ exception are redirected to pre-defined ROM handlers. User code can redirect any exception to a custom handler either by writing its address to the appropriate location from 4030CE04h to 4030CE1Ch or by overriding the branch (load into PC) instruction between addresses from 4030CE04h to 4030CE1Ch.

**Table 26-3: RAM Exception Vectors**

| Address | Exception | Content |
|---------|-----------|---------|
| 4030CE00h | Reserved | Reserved |
| 4030CE04h | Undefined | PC = [4030CE24h] |
| 4030CE08h | SWI | PC = [4030CE28h] |
| 4030CE0Ch | Pre-fetch abort | PC = [4030CE2Ch] |
| 4030CE10h | Data abort | PC = [4030CE30h] |
| 4030CE14h | Unused | PC = [4030CE34h] |
| 4030CE18h | IRQ | PC = [4030CE38h] |
| 4030CE1Ch | FIQ | PC = [4030CE3Ch] |
| 4030CE20h | Reserved | 20090h |
| 4030CE24h | Undefined | 20080h |
| 4030CE28h | SWI | 20084h |
| 4030CE2Ch | Pre-fetch abort | Address of default pre-fetch abort handler |
| 4030CE30h | Data abort | Address of default data abort handler |
| 4030CE34h | Unused | 20090h |
| 4030CE38h | IRQ | Address of default IRQ handler |
| 4030CE3Ch | FIQ | 20098h |

**Note:** 
- The default handlers for pre-fetch and data abort are performing reads from CP15 debug registers to retrieve the reason of the abort.
- In case of pre-fetch abort, the IFAR register is read and stored into the R1 register. Then the ROM Code jumps to the pre-fetch abort dead loop (20088h)
- In case of data abort, the DFAR register is read from CP15 and stored into R0. The DFSR register is read and stored into the R1 register. Then the ROM Code jumps to the data abort dead loop (2008Ch)

---

### Tracing Data

This area contains trace vectors reflecting the execution path of the public boot. The section describes the usage of the different trace vectors and lists all the possible trace codes.

**Table 26-4: Tracing Data**

| Address | Size[bytes] | Description |
|---------|-------------|-------------|
| 4030CE40h | 4 | Current tracing vector, word 1 |
| 4030CE44h | 4 | Current tracing vector, word 2 |
| 4030CE48h | 4 | Current tracing vector, word 3 |
| 4030CE4Ch | 4 | Current copy of the PRM_RSTST register (reset reasons) |
| 4030CE50h | 4 | Cold reset run tracing vector, word 1 |
| 4030CE54h | 4 | Cold reset run tracing vector, word 2 |
| 4030CE58h | 4 | Cold reset run tracing vector, word 3 |
| 4030CE5Ch | 4 | Reserved |
| 4030CE60h | 4 | Reserved |
| 4030CE64h | 4 | Reserved |

---

### Static Variables

This area holds the ROM Code static variables used during boot time.

---

### 26.1.5 Start-up and Configuration

#### 26.1.5.1 ROM Code Start-up

On this device the main MPU subsystem always starts its execution in secure mode after reset due to the **TrustZone architecture** (the Secure ROM Code implements the reset handler).

The Public ROM Code is physically located at the address 20000h that is immediately next to the Secure ROM Code.

*[Sơ đồ Figure 26-8: ROM Code Startup Sequence - Quan trọng - Đơn giản hóa]*

**ROM Code Startup Sequence (Simplified):**

```
System start-up
      ↓
Initialization
      ↓
__main() (stack setup)
      ↓
main()
      ↓
MPU WDT1 setup
      ↓
DPLLs and clocks configurations
      ↓
Booting
```

*[Sơ đồ Figure 26-9: ROM Code Startup Sequence (Detailed) - Bỏ qua, đã có phiên bản đơn giản]*

As shown at top of Figure 26-8, the CPU jumps to the Public ROM Code reset vector once it has completed the secure-side initialization.

Once in public mode, Upon system startup, the CPU performs the public-side initialization and stack setup (complete auto generated C- initialization or "scatter loading"). Then it configures the watchdog WDT1 (set to three minutes), performs system clocks configuration. Finally it jumps to the booting routine.

---

#### 26.1.5.2 CPU State at Public Startup

The CPU **L1 instruction cache** and **branch prediction** mechanisms are not activated as part of the Secure ROM Code initialization. The data base address is configured to the reset vector of Public ROM Code (20000h). MMU is left switched off during the public boot (hence L1 data cache off).

---

#### 26.1.5.3 Clocking Configuration

The device supports the following frequencies based on SYSBOOT[15:14]

**Table 26-5: Crystal Frequencies Supported**

| SYSBOOT[15:14] | Crystal Frequency |
|----------------|-------------------|
| 00b | 19.2 MHz |
| 01b | 24 MHz |
| 10b | 25 MHz |
| 11b | 26 MHz |

The ROM Code configures the clocks and DPLLs which are necessary for ROM Code execution:

- L3 ADPLLS locked to provide 200MHz clocks for peripheral blocks.
- MPU ADPLLS is locked to provide 500 MHz for the A8.
- PER ADPLLL1 is locked to provide 960MHz and 192MHz for peripheral blocks.

The table below summarizes the ROM Code default settings for clocks. This default configuration enables all the ROM Code functionalities with minimized needs on power during boot.

**Table 26-6: ROM Code Default Clock Settings**

| Clock | Frequency [MHz] | Source |
|-------|----------------|--------|
| L3F_CLK | 200 | CORE_CLKOUTM4 |
| SPI_CLK | 48 | PER_CLKOUTM2 |
| MMC_CLK | 96 | PER_CLKOUTM2 |
| UART_CLK | 48 | PER_CLKOUTM2 |
| I2C_CLK | 48 | PER_CLKOUTM2 |
| MPU_CLK | 500 | MPU_PLL |
| USB_PHY_CLK | 960 MHz | PER_CLKDCOLDO |

The DPLLs and GPMC clock dividers are configured with the ROM Code default values after cold or warm reset in order to give the same working conditions to the Public ROM Code sequence.

---

### 26.1.6 Booting

#### 26.1.6.1 Overview

The boot procedure shows the booting procedure. First a **booting device list** is created. The list consists of all devices which will be searched for a booting image. The list is filled in based on the **SYSBOOT** pins.

*[Sơ đồ Figure 26-10: ROM Code Booting Procedure - Quan trọng - Đã có ở trên, bỏ qua]*

Once the booting device list is set up, the booting routine examines the devices enumerated in the list sequentially and either executes booting from memory or peripheral booting depending on the selected booting device type. The memory booting procedure is executed when the booting device type is one of NOR, NAND, MMC or SPI-EEPROM. The peripheral booting is executed when the booting device type is Ethernet, USB or UART.

The memory booting procedure reads data from a memory type device. If a valid booting image is found and successfully loaded into internal memory, the code begins to execute.

- If the device is an HS device then the image is authenticated and Initial SW started upon successful authentication
- If the device is a GP device the Initial SW is simply started (no authentication involved).

The peripheral booting procedure downloads data from a host (commonly a PC) to the device device by means of Ethernet, USB or UART links. The ROM Code uses a host-slave protocol for the synchronization. Upon successful UART, USB or Ethernet connection the host sends the image binary contents. The peripheral booting procedure is detailed in Section 26.1.9.

If the memory or peripheral booting fails for all devices enumerated in the device list then the ROM Code gets into a loop, waiting for the watchdog to reset the system.

---

#### 26.1.6.2 Device List

The ROM Code creates the **device list** based on information gathered from the **SYSBOOT configuration pins** sensed in the control module. The pins are used to index the device table from which the list of devices is extracted

#### 26.1.6.2.1 SYSBOOT Configuration Pins

Table 26-7 contains the SYSBOOT configuration pins.

**Table 26-7: SYSBOOT Configuration Pins (Summary)**

| SYSBOOT Bits | Function |
|--------------|----------|
| SYSBOOT[15:14] | Crystal Frequency (19.2/24/25/26 MHz) |
| SYSBOOT[13:12] | Set to 00b for normal operation |
| SYSBOOT[11:10] | For XIP/NAND boot: Mixed/non-mixed device<br>For NAND boot: must be 00b |
| SYSBOOT[9] | For NAND/NAND I2C: ECC handling by ROM/NAND<br>For Fast External Boot: must be 0b |
| SYSBOOT[8] | Device type (8-bit/16-bit) |
| SYSBOOT[7:6] | For EMAC boot: PHY mode |
| SYSBOOT[5] | CLKOUT1 settings |
| SYSBOOT[4:0] | Boot Sequence selection |

*[Bảng chi tiết Table 26-7 đầy đủ từ các hình ảnh - Rất quan trọng - Giữ nguyên nhưng format lại cho gọn]*

**Detailed SYSBOOT Configuration (Selected Rows):**

| SYSBOOT[15:14] | [13:12] | [11:10] | [9] | [8] | [7:6] | [5] | [4:0] | Boot Sequence |
|----------------|---------|---------|-----|-----|-------|-----|-------|---------------|
| **Crystal Freq** | **Operation** | **XIP/NAND** | **ECC** | **Width** | **PHY** | **CLK** | **Seq** | **1st→2nd→3rd→4th** |
| 00b=19.2MHz | 00b | For XIP: 00b=non-mixed | Don't care | 0=8-bit | Don't care | 0=CLKOUT1 disabled | 00001b | UART0 → XIP(MUX1)[2] → MMC0 → SPI0 |
| 01b=24MHz | (all values reserved) | device | for ROM code | device | for ROM code | 1=CLKOUT1 enabled | | |
| 10b=25MHz | | 0xb=mixed dev | | 1=16-bit | | | | |
| 11b=26MHz | | x1b=reserved | | | | | | |

**Common Boot Sequences (from full table):**

- **00001b**: UART0 → XIP(MUX1)[2] → MMC0 → SPI0
- **00010b**: UART0 → XIP w/ WAIT[1] (MUX1)[2] → MMC0 → SPI0
- **00100b**: UART0 → SPI0 → XIP(MUX2)[2] → NAND I2C
- **00110b**: EMAC1 → SPI0 → NAND → NAND I2C
- **01000b**: EMAC1 → MMC0 → XIP(MUX2)[2] → NAND
- **01010b**: EMAC1 → MMC0 → NAND I2C → USB0
- **10000b**: XIP(MUX1)[2] → UART0 → EMAC1 → MMC0
- **10001b**: XIP w/ WAIT[1] (MUX1)[2] → UART0 → EMAC1 → MMC0
- **10010b**: NAND → NAND I2C → USB0 → UART0
- **10011b**: NAND → NAND I2C → MMC0 → UART0
- **10100b**: NAND → NAND12 → SPI0 → EMAC1
- **10101b**: NAND I2C → MMC0 → EMAC1 → UART0
- **11000b**: USB0 → NAND → SPI0 → MMC0
- **11011b**: Fast External Boot → EMAC1 → UART0 → Reserved

**SYSBOOT Configuration Pins Notes:**

1. WAIT is monitored on GPMC_WAIT0.
2. MUX1 and MUX2 designate which group of XIP signals are used. Each group is defined in Table 26-9.
3. Note that even though some bits may be a "don't care" for ROM code, all SYSBOOT values are latched into the CONTROL_STATUS register and may be used by software after ROM execution has completed.
4. SYSBOOT[15:0] terminals are respectively LCD_DATA[15:0] inputs, latched on the rising edge of PWRONRSTn.

---

#### 26.1.6.2.2 Device List Decoding

The ROM Code uses the row pointed by the SYSBOOT pins value. The device list is filled in with the 1st to 4th devices.

Table 26-7 is the decoding table for SYSBOOT pin configuration. The following shortcuts are used in the table:

- **MMC1**: MMC or SD card (MMC port 1)
- **MMC0**: MMC or SD card (MMC port 0)
- **NAND / NAND I2C**: NAND flash memory / read geometry from EEPROM on I2C0
- **XIP**: NOR or other XIP device without wait monitoring
- **XIP w/ WAIT**: NOR or other XIP device with wait monitoring
- **NAND / MUX**: Boot with XIP_MUX1 signals detailed in Table 26-9
- **MUX2**: Boot with XIP_MUX2 signals detailed in Table 26-9
- **UART0**: UART interface (UART port 0)
- **EMAC1**: Ethernet interface (EMAC port 1)
- **SPI0**: SPI EEPROM (SPI 0, CS0)
- **USB0**: USB interface (USB0)

**Note:** For any SYSBOOT value that is selected, please be aware of the pin muxing implications. For example, if the boot mode selected is EMAC1, NAND, SPI0, NAND I2C, the device will drive EMAC, GPMC, and I2C pins depending on whether the boot device finally succeeds or not. So if a specific boot mode in the sequence chosen is not used then the components using those particular signals that are not mapped to external components are not in contention with the ROM (using these signals). For specific details of the pins driven by each device, please refer the description of that boot device later in this chapter.

To extend the boot flow to boot from devices that are not natively supported by the ROM, SPI boot can be used. Using SPI, the table can be stored in an SPI stick, the system can be configured to boot from a SPI flash, and the code for configuring the USB and booting from a USB stick can be loaded into the SPI flash. This is known as a secondary boot.

The values corresponding to SYSBOOT[4:0]= x1111 provide a bypass mode booting feature.

The fast external boot feature consists of minimal execution by the ROM Code for configuring the GPMC interface and then directly jump to the code contained in the connected external XIP device connected to CS0.

---

### 26.1.7 Fast External Booting

#### 26.1.7.1 Overview

The Fast External boot feature:

- Consists of a blind jump in ARM mode to address 0x08000000 in an external XIP device connected to CS0
- The jump is performed with minimum on-chip ROM Code execution (only configures GPMC interface), without configuring any PLL
- Allows the customer to create its own booting code
- Is set up by means of the configuration pins, see Table 26-7.
- Addr/Data muxed device or a non-muxed (selected using SYSBOOT[11:10]) device in connected in XIP_MUX2 configuration
- Bus width selected by SYSBOOT[8]
- CS0 chip select
- No wait monitoring is available

---

## Summary

### Các điểm quan trọng về ROM Code

#### Device Types
- **HS Device**: High-Secure device - requires authenticated (digitally signed) boot code
- **GP Device**: General-Purpose device - no authentication required

#### Boot Sources
The device can boot from:
- **Memory devices**: NOR, NAND, MMC/SD, SPI-EEPROM
- **Peripheral interfaces**: UART, USB, Ethernet (EMAC)
- **XIP devices**: eXecute In Place devices (NOR flash)

#### Boot Process Flow
1. System starts in Secure Mode (Secure ROM Code)
2. Jump to Public ROM Code (address 20000h)
3. Public ROM initialization
4. Create booting device list based on SYSBOOT pins
5. Try each device in list sequentially
6. For HS devices: authenticate image before execution
7. Execute valid image or watchdog reset on failure

#### Memory Map
- **Public ROM**: 0x20000 - 0x2BFFF (48KB on GP, less on HS due to Secure ROM)
- **Public RAM**: 0x402F0400 - 0x4030FFFF
  - Download Image area: up to 109KB (GP) or 46KB (HS)
  - Stack: 8KB
  - Exception vectors: 0x4030CE00 - 0x4030CE3F
  - Tracing data and static variables

#### Clock Configuration
Default clocks configured by ROM Code:
- L3F_CLK: 200 MHz
- MPU_CLK: 500 MHz
- SPI_CLK: 48 MHz
- MMC_CLK: 96 MHz
- UART_CLK: 48 MHz
- I2C_CLK: 48 MHz
- USB_PHY_CLK: 960 MHz

#### SYSBOOT Pins
- **[15:14]**: Crystal frequency (19.2/24/25/26 MHz)
- **[13:12]**: Must be 00b for normal operation
- **[11:10]**: XIP/NAND device configuration
- **[9]**: ECC handling selection
- **[8]**: Bus width (8-bit or 16-bit)
- **[7:6]**: EMAC PHY mode
- **[5]**: CLKOUT1 enable/disable
- **[4:0]**: Boot sequence selection (32 possible sequences)

#### Boot Sequence
ROM Code tries up to 4 devices per boot sequence, configured by SYSBOOT[4:0]. Common sequences include:
- UART → XIP → MMC → SPI
- NAND → MMC → USB → UART
- EMAC → NAND → SPI → MMC

#### Fast External Boot
- Minimal ROM execution
- Direct jump to 0x08000000
- GPMC interface only
- No PLL configuration
- Controlled by SYSBOOT configuration

#### Exception Handling
ROM provides:
- Exception vectors at 0x20000
- RAM exception handlers at 0x4030CE00
- Dead loops for undefined exceptions
- Custom handler support via RAM vectors

#### Security Features (HS Device)
- Image authentication required
- Digital signature verification
- Failed authentication → dead loop → watchdog reset
- TrustZone architecture support
---

#### 26.1.7.2 External Booting

Figure 26-11 shows the Fast External Boot procedure. The code does not make use of RAM and is designed for fast execution.

**Figure 26-11: Fast External Boot Procedure**

```
Fast External Boot
       │
       ▼
   ┌─────────────┐
   │ GP Device?  │ No → (Normal boot)
   └─────────────┘
       │ Yes
       ▼
   ┌──────────────────────┐
   │Configuration pins     │ No → (Normal boot)
   │indicate fast          │
   │external boot?         │
   └──────────────────────┘
       │ Yes
       ▼
   Configure and enable GPMC
       │
       ▼
   Jump to address 0x08000000 in ARM mode
       │
       ▼
   Jump to external SW
```

---

### 26.1.8 Memory Booting

#### 26.1.8.1 Overview

The memory booting procedure takes care of starting an external code located in memory device types.

**Figure 26-12 & 26-13: Memory Booting Flow**

*Simple Flow:*
```
Memory Booting
      │
      ▼
  ┌─────────────┐
  │Device is XIP│ No → Copy image into target RAM → Copying failed → Return fail
  │type?        │
  └─────────────┘
      │ Yes
      ▼
  Execute Initial SW
```

*Flow with Authentication (HS Device):*
```
Memory Booting
      │
      ▼
  ┌─────────────┐
  │Device is XIP│ No → Copy image into target RAM → Copying failed → Return fail
  │type?        │
  └─────────────┘
      │ Yes
      │
      ▼
  ┌─────────────┐
  │HS device?   │ No → (Execute)
  └─────────────┘
      │ Yes
      ▼
  Authentication
      │
      ▼
  ┌──────────────────┐
  │Authentication    │ Yes → Execute Initial SW
  │successful?       │
  └──────────────────┘
      │ No
      ▼
  Dead loop
```

There are two groups of memory booting devices distinguished by the need of **code shadowing**. The ROM Shadowing means performing a code copy to an directly addressable memory (a directly addressable RAM area) from where the code can be executed. Devices which are directly addressable are called **eXecute In Place (XIP) devices**.

---

#### 26.1.8.2 XIP Memory

The ROM Code can boot directly from XIP devices. A typical XIP device is a NOR flash memory. Support for XIP devices is performed under the following assumptions:

- Uses GPMC as the communication interface
- Up to 1Gbit (128Mbytes) memories can be connected
- Both x8 and x16 data bus width
- Asynchronous protocol
- Supports address/data multiplexed mode and non-muxed mode
- GPMC clock is 100 MHz
- Device connected to CS0 mapped to address 0x08000000
- Wait pin signal GPMC_WAIT0 is monitored depending on the SYSBOOT pin configuration (XIP / XIP w/ WAIT)
- Flexible muxing options for gpmc_a0-gpmc_a11 for non-muxed XIP devices

Depending on the SYSBOOT pin configuration the ROM Code decides whether to use the WAIT0 signal monitoring or not. Wait pin polarity is set to stall accessing memory when the WAIT0 pin is low. The wait monitoring is intended to be used with memories which require long time for initialization after reset or need to pause while reading data. The boot procedure from XIP device can be described as such:

- Configure GPMC for XIP device access
- Set the image location to 0x08000000
- Verify if bootable image is present at the image location
- If the image has been found, start the execution
- If the image has not been found, return from XIP booting to the main booting loop

#### 26.1.8.2.1 XIP Initialization and Detection

**GPMC initialization:**

Figure 26-14 and Table 26-8 describe the GPMC timing settings set for XIP boot and other address-data accessible devices.

**Table 26-8: XIP Timings Parameters**

| Parameter | Description | Value (clock cycles) |
|-----------|-------------|---------------------|
| tw | write cycle period | 17 |
| trl | read cycle period | 17 |
| tCEmt | CE low time | 0 |
| tCEoff | CE high time | 16 |
| tADVon | ADV low time | 1 |
| tADVoff | ADV high time | 2 |
| tOEon | OE low time | 3 |
| tWEon | WE low time | 3 |
| tWdata | data latch time | 15 |
| tOEoff | OE high time | 16 |
| tWEoff | WE high time | 15 |
| tCSEXTRADELAY | CS Extra Delay | 3/4 |

The one clock cycle is 20ns, which corresponds to 50-MHz frequency.

**Device detection:**

There is no specific identification routine executed prior to booting from an XIP device.

#### 26.1.8.2.2 Pins Used

The list of device pins that are configured by the ROM in the case of NOR boot mode are as follows. Please note that all the pins might not be driven at boot time. The decision on which pins need to be driven is done based on the type of NOR flash selected. The pins that are not listed below are not guaranteed by the ROM code and might be driven in default. Specifically, external logic is needed for address lines A16 and above to the memory, in addition to any other desired signals during non-muxed NOR boot. Similarly for Muxed NOR Boot, address lines A16 and above to the memory are not controlled by the ROM and need to be managed externally during boot, or ensure proper addressing to all the memory signals.

Once the initial software starts running, it can appropriately configure the pinmux setting for the lines and enable the specific settings in GPMC to drive all the address lines.

**Table 26-9: Pins Used for Non-Muxed NOR Boot**

| Signal name | Pin used in XIP_MUX1 mode | Pin used in XIP_MUX2 mode |
|-------------|---------------------------|---------------------------|
| CS0 | GPMC_CSN0 | GPMC_CSN0 |
| ADVN_ALE | GPMC_ADVN_ALE | GPMC_ADVN_ALE |
| OEN_REN | GPMC_OEN_REN | GPMC_OEN_REN |
| BEON_CLE | GPMC_BEN0_CLE | GPMC_BEN0_CLE |
| BE1N | GPMC_BE1N | GPMC_CSN2 and GPMC_BE1N |
| CLK | GPMC_CLK | none |
| WEN | GPMC_WEN | GPMC_WEN |
| WAIT0 | GPMC_WAIT0 | GPMC_WAIT0 |
| WAIT1 | none | GPMC_CLK |
| AD0 - AD15 | GPMC_AD0 - GPMC_AD15 | GPMC_AD0 - GPMC_AD15 |
| A0 | GPMC_A0 | LCD_DATA0 |
| A1 | GPMC_A1 | LCD_DATA1 |
| A2 | GPMC_A2 | LCD_DATA2 |
| A3 | GPMC_A3 | LCD_DATA3 |
| A4 | GPMC_A4 | LCD_DATA4 |
| A5 | GPMC_A5 | LCD_DATA5 |
| A6 | GPMC_A6 | LCD_DATA6 |
| A7 | GPMC_A7 | LCD_DATA7 |
| A8 | GPMC_A8 | LCD_VSYNC |
| A9 | GPMC_A9 | LCD_HSYNC |
| A10 | GPMC_A10 | LCD_PCLK |
| A11 | GPMC_A11 | LCD_AC_BIAS_EN |

**Table 26-10: Pins Used for Muxed NOR Boot**

| Signal name | Pin used in XIP_MUX1 mode | Pin used in XIP_MUX2 mode |
|-------------|---------------------------|---------------------------|
| CS0 | GPMC_CSN0 | GPMC_CSN0 |
| ADVN_ALE | GPMC_ADVN_ALE | GPMC_ADVN_ALE |
| OEN_REN | GPMC_OEN_REN | GPMC_OEN_REN |
| BEON_CLE | GPMC_BEN0_CLE | GPMC_BEN0_CLE |
| BE1N | GPMC_BE1N | GPMC_CSN2 and GPMC_BE1N |
| CLK | GPMC_CLK | none |
| WEN | GPMC_WEN | GPMC_WEN |
| WAIT0 | GPMC_WAIT0 | GPMC_WAIT0 |
| WAIT1 | none | GPMC_CLK |
| AD0 - AD15 | GPMC_AD0 - GPMC_AD15 | GPMC_AD0 - GPMC_AD15 |

#### 26.1.8.2.3 SYSBOOT Pins

Some of the SYSBOOT pins have special meanings when NOR boot is selected.

**Table 26-11: Special SYSBOOT Pins for NOR Boot**

| SYSBOOT[n] | Description |
|------------|-------------|
| [8] | 0 = 8-bit device<br>1 = 16-bit device |
| [11:10] | 00b = Non-muxed device<br>10b = Muxed device<br>x1b = Reserved |

---

#### 26.1.8.3 Image Shadowing for Non-XIP Memories

#### 26.1.8.3.1 Shadowing on GP Device

The **GP Device** shadowing uses the approach in Figure 26-15.

**Figure 26-15: Image Shadowing on GP Device**

```
Memory booting
      │
      ▼
   ┌──────────────────────┐    Device detection
   │ Initialization       │──→ Initialization failed ──→ Return fail
   │ and                  │
   │ Device detection     │
   └──────────────────────┘
      │
      ▼
   Set first / next valid block
      │                    No more blocks ──→ Return fail
      ▼                    
   ┌─────────────┐
   │Copying      │ No more sectors to read
   │failed       │◄──┐
   └─────────────┘   │
      │              │
      ▼              │
   Read a sector ────┘
      │
      ▼
   Store the loaded sector with
   Initial SW in the target buffer
      │
      ▼
   ┌──────────────────┐
   │Loading Initial SW│ No ──→ (continue reading)
   │Completed?        │
   └──────────────────┘
      │ Yes
      ▼
   Initial SW execution
```

---

#### 26.1.8.4 NAND

The NAND flash memory is not XIP and requires shadowing before the code can be executed.

#### 26.1.8.4.1 Features

- Uses GPMC as the communication interface
- Device from 512MBit (64MByte)
- x8 and x16 bus width
- Support for large page size (2048 bytes + 64 spare bytes) or very large page size 4096 bytes + 128/218 spare bytes)
- Only supports devices where Chip Select can be de-asserted during read, program or erase cycles, without interrupting the operation
- Device Identification based on ONFI or ROM table
- ECC correction is bits/sector for most devices (16b/sector for devices with large spare area)
- Support for disabling ECC correction, so that the in-built ECC correction mechanisms on some NANDs can be used
- GPMC timings adjusted for NAND access
- GPMC clock is 50MHz
- Device connected to GPMC_CSN0
- Wait pin signal GPMC_WAIT0 connected to NAND BUSY output
- Four physical blocks are searched for an image. The block size depends on device

#### 26.1.8.4.2 Initialization and Detection

The initialization routine for NAND devices consists in three parts: GPMC initialization, device detection with parameters determination and finally bad block detection.

**ONFI Support:**

The NAND identification starts with ONFI detection. For more information on ONFI standard, see the Open NAND Flash Interface (www.onfi.org).

**GPMC Initialization:**

The GPMC interface is configured as such it can be used for accessing NAND devices. The address bus is released since a NAND device does not use it. The data bus width is initially set to 16 bits; and changed to 8 bits if needed after device parameters determination. The following scheme is applied since NAND devices require different timings when compared to regular NOR devices:

**Table 26-12: NAND Timings Parameters**

| Parameter | Description | Value [clock cycles] |
|-----------|-------------|---------------------|
| tw | write cycle period | 30 |
| trl | read cycle period | 30 |
| tCEmt | CE low (not marked on the figure) | 0 |
| tCEoff | CE low to OE low time | 7 |
| tWEmt | CE low to WE low time | 5 |
| tWdata | CE low to data latch time | 21 |
| tOEoff | CE low to OE high time | 24 |
| tWEoff | CE low to WE high time | 22 |

Figure 26-16 and Table 26-12 describes the timings configured for NAND device access. The one clock cycle is 20 ns, which correspond to 50-MHz frequency.

**Device Detection and Parameters:**

The ROM Code first performs device wait for device auto initialization (with 250ms timeout) with polling of the ready information. Then, it needs to identify the NAND type connected to the GPMC interface. The GPMC is initialized in 16 bits, asynchronous mode. The NAND device is reset (command FFh) and its status is polled until ready for operation (with 200ms timeout). The ONFI Read ID (command 90h / address 20h) is sent to the NAND device. If it replies with the ONFI signature (4 bytes) then a Read parameters page (command ECh) is sent. If the parameters page does not have the ONFI signature, then the ONFI identification passes, the information shown in Table 26-13 is then extracted: page size, spare area size, number of pages per block, and the addressing mode.

**Table 26-14: Supported NAND Devices**

| Capacity | Device ID | Bus Width | Page size |
|----------|-----------|-----------|-----------|
| 16 Gb | 88 | x8 | 2048 |
| 16 Gb | 96 | x16 | 2048 |
| 32 Gb | D7 | x8 | 2048/4096 |
| 32 Gb | C7 | x16 | 2048/4096 |
| 32 Gb | A7 | x8 | 2048/4096 |
| 32 Gb | 87 | x16 | 2048/4096 |
| 32 Gb | 87 | x8 | 2048 |
| 32 Gb | 97 | x16 | 2048 |
| 64 Gb | DE | x8 | 2048/4096 |
| 64 Gb | CE | x16 | 2048/4096 |
| 64 Gb | AE | x8 | 2048/4096 |
| 64 Gb | BE | x16 | 2048/4096 |

When the parameters are retrieved from the ROM table: page size and block size is updated based on 4th byte of NAND ID data. Due to inconsistency amongst different manufacturers, only devices which has been recognized to be at least 2GB (included) have these parameters updated. Therefore, the ROM Code supports 4kB page devices but only if their size, according to the table, is at least 2Gb. Devices which are smaller than 2Gb have the block size parameter fixed to 128kB. Table 26-15 shows the 4th ID Data byte encoding used in ROM Code.

**Table 26-15: 4th NAND ID Data Byte**

| Item | Description | I/O # | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|------|-------------|---|---|---|---|---|---|---|---|---|
| Page Size | 1kB | | | | | | | | 0 | 0 |
| | 2kB | | | | | | | | 0 | 1 |
| | 4kB | | | | | | | | 1 | 0 |
| | 8kB | | | | | | | | 1 | 1 |
| Cell type | 2 levels | | | | | | 0 | 0 | | |
| | 4 levels | | | | | | 0 | 1 | | |
| | 8 levels | | | | | | 1 | 0 | | |
| | 16 levels | | | | | | 1 | 1 | | |
| Block Size | 64kB | | | 0 | 0 | | | | | |
| | 128kB | | | 0 | 1 | | | | | |
| | 256kB | | | 1 | 0 | | | | | |
| | 512kB | | | 1 | 1 | | | | | |

**Reading NAND Geometry From I2C EEPROM:**

ROM supports a special boot mode called NANDI2C to support NAND devices whose geometry cannot be detected by the ROM automatically using methods described in the previous section. If this boot mode is selected, the ROM code tries to read NAND geometry from an I2C EEPROM. If the read is successful, ROM code then proceeds to normal NAND boot, beginning with reading bad block information.

**Table 26-16: Pins Used for NANDI2C Boot for I2C EEPROM Access**

| Signal name | Pin Used in Device |
|-------------|-------------------|
| I2C_SCL | i2c0_scl |
| I2C_SDA | i2c0_sda |

ROM accesses the I2C EEPROM at I2C slave address **0x50** and reads 7 bytes starting from address offset **0x80**. The ROM sends a 2-byte address to perform a selective read from the I2C EEPROM, thus the ROM can only support devices that are minimum of 32Kb in size. The format of this (NAND geometry information) is as follows:

**Table 26-17: NAND Geometry Information on I2C EEPROM**

| Byte address | Information | |
|--------------|-------------|-------------|
| | Upper nibble | Lower nibble |
| 0x80 | Magic Number – 0x10 | |
| 0x81 | Magic Number – 0xb3 | |
| 0x82 | Magic Number – 0x57 | |
| 0x83 | Magic Number – 0xa6 | |
| 0x84 | NAND column address (wordbyte offset within page) size in bytes. Example: 2 | NAND row address (page offset) size in bytes. Example: 3 |
| 0x85 | Page size (2ⁿ) exponent 'N'. Example (for page size of 2048): 11 | Pages per block (2ⁿ) exponent 'N'. Example (for number of blocks 64): 6 |
| 0x86 | NAND bus width<br>0 = 8-bit device<br>1 = 16-bit device | ECC Type<br>00b = No ECC<br>01b = BCH8<br>10b = BCH16<br>11b = Reserved |

**ECC Correction:**

The default ECC correction applied is BCH 8b/sector using the GPMC and ELM hardware.

Referring ID codes 88h, c5h, c7h, DEh, CEh, AEh, BEh, when manufacturer code (first ID byte) is 98h the Cell type information is checked in the 4th byte of ID data. If it is equal to 10b then the ECC correction applied is BCH 16b/sector.

In addition ECC computation done by the ROM can be turned off completely by using SYSBOOT[9]. This is particularly useful when interfacing with NAND devices that have built in ECC engines.

**Table 26-18: ECC Configuration for NAND Boot**

| SYSBOOT[9] | ECC Computation |
|------------|-----------------|
| 0 | ECC done by ROM |
| 1 | ECC handled by NAND |

**Figure 26-17: NAND Device Detection**

```
NAND detection
      │
      ▼
   Wait for device initialization
   (max timeout 250ms)
      │               Timeout reached
      ▼               ────────────────→ (timeout, failed)
   ┌─────────────┐
   │Device ready?│ No ──→ (wait)
   └─────────────┘
      │ Yes
      ▼
   Issue Read command
   (max timeout 200ms)
      │
      ▼
   ┌─────────────┐
   │Device ready?│ No ──→ (wait)
   └─────────────┘
      │ Yes
      ▼
   Issue Read ID (ONFI) command
      │
      ▼
   ┌──────────────────┐
   │Device replied     │ Yes ──┐
   │ONFI?             │        │
   └──────────────────┘        │
      │ No                     │
      ▼                        │
   Issue Reset command         │
      │                        │
      ▼                        ▼
   ┌─────────────┐      Issue Read parameters page
   │Device ready?│ No   command
   └─────────────┘        │
      │ Yes               ▼
      ▼              Extract NAND parameters
   Issue Read ID        from device parameters page
   (standard) command    │
      │                  │
      ▼                  │
   ┌─────────────┐       │
   │Device ID    │ No    │
   │in the table?│──────→│
   └─────────────┘       │
      │ Yes              │
      ▼                  │
   Extract NAND parameters from table
      │                  │
      │◄─────────────────┘
      ▼
   Update page size, block size, ECC
   correction for devices > 1 Gb
      │
      ▼
   Read Invalid Blocks
   information
      │
      ▼
   Success
```

**Bad Block Verification:**

Invalid blocks are blocks which contain invalid bits whose reliability cannot be guaranteed by the manufacturer. Those bits are identified in the factory or during the programming and reported in the initial pages of the block. The ROM code always performs that validation when searching for a bootable image. If the Code is looking for an image in the first four blocks, it must detect block validity status of these blocks. Blocks which are detected as invalid are not accessed later on. Blocks validity status is coded in the spare areas of the first two pages of a block (first byte equal to Fh in 1st and 2nd pages for an 8 bits device / first word equal to FFFFh in 1st and 2nd page for a 16 bits device).

Figure 26-18 depicts the invalid block detection routine. The routine consists in reading spare areas and checking validity data pattern.

**Figure 26-18: NAND Invalid Blocks Detection**

```
Read Invalid Blocks
Information
      │                                 ┌─────────────────────┐
      ▼                                 │ For first 4 blocks  │
   Read 1st and 2nd spare               │                     │
   sectors                              │                     │
      │                                 │                     │
      ▼                                 │                     │
   ┌────────────────┐                  │                     │
   │Invalid Block   │ Yes ──→ Set Invalid Block Flag        │
   │Information?    │                   │                     │
   │0xFF or 0xFFFF  │                   │                     │
   │for 16-bit      │                   │                     │
   │devices?        │                   │                     │
   └────────────────┘                   │                     │
      │ No                              │                     │
      ▼                                 │                     │
   Clear Invalid Block Flag             │                     │
      │                                 │                     │
      └─────────────────────────────────┘
```

#### 26.1.8.4.2.1 NAND Read Sector Procedure

The ROM Code reads data from NAND devices in 512 bytes sectors. The read function fails in two cases:

- The accessed sector is within a block marked as invalid
- The accessed sector contains an error which cannot be corrected with ECC

Figure 26-19 shows the read sector routine for NAND devices. The ROM Code uses normal read (command 00h 30h) for reading NAND page data.

**Figure 26-19: NAND Read Sector Procedure**

```
Read Sector
      │
      ▼
   ┌─────────────┐
   │Is this block│ Yes ──→ (return failed)
   │invalid      │
   └─────────────┘
      │ No
      ▼
   Read 512 bytes sector
      │
      ▼
   ┌──────────────┐
   │GPMC ECC      │ Yes ──→ Correct Error ──→ Failed
   │?             │              │
   │NAND ECC      │              │
   └──────────────┘              │
      │ No                       │
      │                   Success│
      ▼                          ▼
   Success                    (return success)
```

Page data can contain errors due to memory alteration. The ROM Code uses an ECC correction algorithm to detect and possibly correct those errors. The ECC algorithm used is BCH with capability for correcting 8b or 16b errors per sector. The BCH data is automatically calculated by the GPMC on reading each 512 bytes sector. The computed ECC is compared with the ECC stored in the spare area (corresponding spare area for Figure 26-20 and Figure 26-21 show the mapping of ECC data stored in the spare area for respectively 2KB-page and 4KB- page devices. If both ECC data are equal then the Read sector function returns as if no errors occured. If there is a difference, the ROM Code invokes an ECC error(s) in the corresponding sector (this procedure is assisted by the ELM hardware) and returns the data if successful. If errors are uncorrectable, the function returns with FAIL.

When the 512 byte sector successfully passes the ECC check, the ROM checks for a valid configuration header. A configuration header is considered to be valid if the first value (the Start field) contains data other than 0x00000000 or 0xFFFFFFFF. If the Start field is not valid, the ROM reads the next 512-byte sector, performs the ECC procedure described above, and checks for a valid configuration header. This process is repeated for the whole block until a valid header is found. If a valid header is not found in the first block, the ROM attempts the same process in the next block. For NAND, the ROM checks for a valid header in the first four blocks. If no valid header is found in the first 4 blocks, the NAND boot fails, and the ROM moves on to the next valid boot source as dictated by boot order signals.

#### 26.1.8.4.2.2 Pins Used

The list of device pins that are configured by the ROM in the case of NAND boot mode are as follows. Please note that all the pins might not be driven at boot time.

**NOTE:** Caution must be taken when using an 8-bit NAND. The ROM initially configures all address and data signals (including AD15) the GPMC uses when attempting to read configuration values from the NAND. If you use an 8-bit NAND, and connect AD15-AD8 to other functions (GPIOs, for example), there may be contention on these signals during the boot phase. AD15-AD8 are configured as outputs and will be driven by the ROM if NAND boot is selected. Ensure external circuits will not be in contention with these driven outputs.

**Table 26-19: Pins Used for NAND Boot**

| Signal name | Pin Used in Device |
|-------------|-------------------|
| CS0 | GPMC_CSN0 |
| ADVN_ALE | GPMC_ADVN_ALE |
| OEN_REN | GPMC_OEN_REN |
| BEON_CLE | GPMC_BEN0_CLE |
| BE1N | GPMC_BE1N |
| WEN | GPMC_WEN |
| WAIT | GPMC_WAIT0 |
| CLK | GPMC_CLK |
| AD0 - AD15 | GPMC_AD0 - GPMC_AD15 |

#### 26.1.8.4.2.3 SYSBOOT Pins

SYSBOOT[11:10] has a special meaning when NAND boot is selected. SYSBOOT[11:10] must be 0.

---

#### 26.1.8.5 MMC / SD Cards

#### 26.1.8.5.1 Overview

The ROM Code supports booting from MMC / SD cards in the following conditions:

- MMC/SD Cards compliant to the Multimedia Card System Specification and Secure Digital I/O Card Specification of low and high capacities
- MMC/SD cards connected to MMC0 or MMC1
- Support of x1 or x8 V on MMC0 and MMC1
- Initial 1-bit MMC Mode, optional 4-bit mode, if device supports it
- Clock Frequency: identification mode: 400 KHz; data transfer mode up to 10 MHz
- Only one card connected to the bus
- Raw mode, image data read directly from sectors in the user area
- File system mode (FAT12/16/32 supported with or without Master Boot Record), image data is read from a booting file

#### 26.1.8.5.2 System Interconnection

Each interface has booting restrictions on which type of memory it supports:

- **MMC0** supports booting from eMMC/SD card cage and also supports booting from eMMC/eSD/managed NAND memory devices with less than 4GB capacity
- **MMC1** supports booting from eMMC/eSD/managed NAND memory device with 4GB capacity or greater

The restriction is a result of many eMMC devices not being compliant with the eMMC v4.41 specification. If you must boot from two different card cages, many MMC/SD cards will boot from MMC1 but will not boot some older cards may not boot because MMC1 boot uses a slightly different boot sequence to support booting eMMC. Generally, external eMMC devices will be well tested for compatibility, only MMC0 should be used to boot from the card cage. Similarly for maximum compatibility, booting from eMMC/eSD/managed NAND should only be performed on MMC1.

Note that the above restrictions only apply to booting from each port. Drivers can be written for either port to support any desired interface.

**Note:**

- The ROM Code does not handle the card detection feature on card cage
- If MMC is used the GPMC interface is not usable, due to pin muxing options
- MMC1 supports sector mode without querying the card

#### 26.1.8.5.3 Booting Procedure

The high level flowchart of the eMMC / eSD and MMC/SD booting procedure is depicted in Figure 26-22.

**Figure 26-22: MMC/SD Booting**

```
MMC/ SD Booting
      │
      ▼
   Initialize the MMC / SD driver
      │
      │                Not detected
      ▼                ────────────→ Failed
   ┌──────────────────┐
   │Detect card       │
   │ Detected         │
   │  memory          │
   └──────────────────┘
      │ Detected
      ▼
   Configure the card address
   (RCA)
      │
      ▼              No
   ┌─────────────┐◄────┐
   │Booting file │     │
   │found?       │     │
   └─────────────┘     │
      │ Yes            │
      ▼                │
   Get the booting file│
      │                │
      │                │
      ▼                │
   ┌─────────────┐    │
   │'Raw mode'   │ No─┘
   │detected?    │
   └─────────────┘
      │ Yes
      ▼
   Get raw data
      │
      ▼
   Success
```

#### 26.1.8.5.4 Initialization and Detection

The ROM Code attempts to initialize the memory device or card connected on MMC interface. If neither memory device nor card is detected then the ROM Code carries on to the next booting device. The standard identification process is named **Card Address (RCA)** assignment and so forth. However, the ROM Code assumes that only one memory or card is present on the bus. This first sequence is done using the CMD0 and CMD1 signaling to be common to SD and MMC devices.

MMC and SD standards detail this phase as **initialization phase**. Both standards differ in the first commands involved: CMD1 for MMC and ACMD41 for SD. The ROM Code uses this difference to discriminate between MMC and SD devices: CMD1 is supported only by the MMC standard whereas ACMD41 is only supported by SD standard. The ROM Code first sends a CMD1 to the device and expects a response only if an MMC device is connected. If no response is received then ACMD41 (ACMD41 is made out of CMD55 and ACMD41) is sent and a response is expected from an SD device. If no response is received then it is assumed that no device is connected and the ROM Code exits the MMC/SD Booting procedure with FAIL. This is detection procedure shown in Figure 26-23.

Another point to note is the difference in the arguments to CMD1 between MMC0 and MMC1. At first the ROM queries from the card SECTOR_MODE capability using READ_OCR from the card. If MMC0, the response from the card is simply reflected back to the card as the argument for all subsequent CMD1, till the card is in the READY state. On MMC1, Bit30 of the response received from the card is set to 1 by the ROM, and this modified value is used as the argument for subsequent CMD1. This is done to indicate to the card that the ROM supports sector addressing. This mode might not be supported by older/legacy/non-standard versions of cards.

**Figure 26-23: MMC/SD Detection Procedure**

```
MMC / SD detection
      │
      ▼
   Send CMD1 command
      │
      │         No response received
      ▼         ───────────────────→ (try SD)
   ┌─────────────┐
   │Timeout      │
   │waiting for  │
   │answer?      │
   └─────────────┘
      │ Response received
      ▼
   MMC device
   
   (If no MMC response)
      │
      ▼
   Send CMD55
   Send ACMD41
      │
      ▼
   ┌─────────────┐
   │Timeout      │ Yes ──→ No device detected
   │waiting for  │
   │answer?      │
   └─────────────┘
      │ No
      ▼
   SD device
```

The contents of an MMC/SD card or an eMMC/eSD device may be formatted as raw binary or within a FAT filesystem. The ROM Code reads out raw sectors from image or the booting file within the file system and boots from it.

#### 26.1.8.5.5 MMC/SD Read Sector Procedure in Raw Mode

In raw mode the booting image can be located at one of the four consecutive locations in the main area: offset 0x0 (0x0), offset 0x20000 (128KB), offset 0x40000 (256KB), or offset 0x60000 (384KB). For this boot mode, the image size shall not exceed 128KB in size. However it is possible to flash a device with an image greater than 128KB by flashing at one of the aforementioned locations. Therefore the ROM Code does not check the image size. The only drawback is that the image size must not span the next subsequent image boundary.

The ROM will check the first sector (offset 0x0) for the presence of the TOC structure as described in Section 26.1.10. If the sector contains a valid CHSETTINGS item, the ROM will use that header to see next 512-byte section, uses its size and destination information to download the image to the destination address, and starts execution there. If the TOC header is complete the boot. If it is not, the boot fails and the invalid, the ROM checks for a redundant image in the next location (0x20000), and repeats the process to test a valid TOC is available. The TOC is considered invalid if the first location is either 0xFFFFFFFF or 0x00000000 (which is typical of a missing card or an erased image), or if the CHSETTINGS item is missing or empty.

#### 26.1.8.5.6 MMC/SD Read Sector Procedure in FAT Mode

MMC/SD Cards or eMMC/ eSD devices may hold a FAT file system which ROM Code is able to read and process. The image booting procedure is taken from a specific file. For this boot mode the booting file has to be located in the root directory on an active primary partition of type FAT12/16 or FAT32.

An MMC/SD card or eMMC/ eSD device can be configured either as floppy-like or hard-drive-like:

- When configured as floppy-like, a single file system is present without any Master Boot Record (MBR) holding a partition table
- When configured as hard-drive-like, an MBR is present in the first sector. This MBR holds a table of partitions, one of which must be FAT12/16/32, primary and active

---

#### 26.1.8.5.7 Partition and FAT Structure Details

**Table 26-21: Partition Entry**

| Offset | Length[bytes] | Entry Description | Value |
|--------|--------------|-------------------|-------|
| 0x00h | 1 | Partition State | 00h: Inactive/80h: Active |
| 0x01h | 1 | Partition Start Head | H_s |
| 0x02h | 2 | Partition Start Cylinder and Sector | C_s[7:0],C_s[9:8] S_s[5:0] |
| 0x04h | 1 | Partition Type | See Table 26-22 for partial partition types |
| 0x05h | 16 | Partition End Head | H_e |
| 0x06h | 2 | Partition End Cylinder and Sector | C_e[7:0],C_e[9:8] S_e[5:0] |
| 0x08h | 4 | First sector position relative to the beginning of media | LBA_s=C_s H_s+H_s S_s+S_s-1 |
| 0x0Ch | 4 | Number of sectors in partition | LBA_e=C_e H_e+H_e S_e+S_e-1\NBS= LBA_e-LBA_s+1 |

**Table 26-22: Partition Types**

| Partition Type | Description |
|---------------|-------------|
| 01h | FAT12 |
| 04h, 06h, 0Eh | FAT16 |
| 0Bh, 0Ch, 0Fh | FAT32 |

The way the ROM Code detects whether a sector is the 1st sector of an MBR or not is described in Figure 26-25.

The ROM Code first checks if the signature is present. Each partition entry is checked:
- If its type is set to 00h then all fields in the entry must be 00h
- The partition is checked to be within physical boundaries, i.e. the partition is located inside and it's size fits the total physical sectors.

**Figure 26-25: MBR Detection Procedure**

```
MBR detection
      │
      ▼
   ┌─────────────────┐
   │0xAA55 Signature │ No ──→ (not MBR)
   │at offset 0x1FE? │
   └─────────────────┘
      │ Yes
      ▼
   ┌──────────────────────────────────────┐
   │                                       │
   │  ┌─────────────┐                     │
   │  │Partition    │ Yes ──→ Are all    │ Yes
   │  │type is 00h? │         fields 00h?│────→
   │  └─────────────┘                     │
   │      │ No                            │
   │      ▼                               │ For all 4 entries
   │  ┌─────────────┐                     │
   │  │Partition    │ No ◄────────────────┤
   │  │within       │                     │
   │  │physical     │                     │
   │  │boundaries?  │                     │
   │  └─────────────┘                     │
   │      │ Yes                           │
   │      └───────────────────────────────┘
   │
   ▼
Success
```

Once identified, the ROM Code gets the partition using the procedure described in Figure 26-26. The partition type is checked to be FAT12/16 or FAT32. Its state must be 00h or 80h (if there are more than one active partition the test fails). The ROM Code returns with FAIL if no active primary FAT12/16/32 could be found.

**Figure 26-26: MBR, Get Partition**

```
MBR get partition
      │
      ▼                      ┌────────────────────┐
   ┌─────────────┐           │                    │
   │Partition    │ No ───────┤                    │
   │type is      │           │                    │
   │FAT12/16/32  │           │                    │
   └─────────────┘           │                    │
      │ Yes                  │ For all 4 entries  │
      ▼                      │                    │
   ┌─────────────┐           │                    │
   │Is it active │ No ───────┤                    │
   └─────────────┘           │                    │
      │ Yes                  │                    │
      ▼                      │                    │
   ┌──────────────────┐      │                    │
   │An active         │ Yes ─┤                    │
   │partition has     │      │                    │
   │been already      │      │                    │
   │found             │      │                    │
   └──────────────────┘      │                    │
      │ No                   │                    │
      ▼                      │                    │
   ┌─────────────┐           │                    │
   │Valid         │ No ──────┘                    │
   │partition was │                               │
   │found         │                               │
   └─────────────┘                                │
      │ Yes                                       │
      ▼                                           │
   Success                                        │
                                                  ▼
                                              Failed
```

#### 26.1.8.5.7.2 FAT12/16/32 Boot Sector

The FAT file system is made out of several parts:
- Boot Sector which holds the BIOS Parameter Block (BPB)
- File Allocation Table (FAT) which describes the use of each cluster of the partition
- Data Area which holds the Files, Directories and Root Directory (for FAT12/16, the Root Directory has a specific fixed location).

The Boot Sector is described in Table 26-23.

**Note:** In the following description, all the fields whose names start with BPB_ are part of the BPB. All the fields whose names start with BS_ are part of the Boot Sector and not really part of the BPB (not mandatory), they are not used at all by the ROM Code.

**Table 26-23: FAT Boot Sector**

| Offset | Length[bytes] | Name | Description |
|--------|--------------|------|-------------|
| 0x00h | 3 | BS_jmpBoot | Jump instruction to Boot Code (not used) |
| 0x03h | 8 | BS_OEMName | Name of the System which created the partition |
| 0x0Bh | 2 | BPB_BytsPerSec | Counts of Bytes per sector (usually 512) |
| 0x0Dh | 1 | BPB_SecPerClus | Number of sectors per allocation unit |
| 0x0Eh | 2 | BPB_RsvdSecCnt | Number of reserved sectors for the Boot Sector for FAT12/16 is 1, for FAT32 is typically 32 |
| 0x10h | 1 | BPB_NumFATs | Number of copies of FAT, usually 2 |
| 0x11h | 2 | BPB_RootEntCnt | For FAT12/16, number of 32 bytes entries in the Root Directory (multiple of BPB_BytsPerSector) 200h for FAT12 this value) |
| 0x13h | 2 | BPB_TotSec16 | Total Count of sectors on the volume. If the size is bigger than 10000h or for FAT32, this field is 0 and BPB_TotSec32 holds the true value |
| 0x15h | 1 | BPB_Media | Media Type, usually F8h: fixed, non-removable |
| 0x16h | 2 | BPB_FATSz16 | For FAT12/16, size in sectors of one FAT for FAT32, holds 0 |
| 0x18h | 2 | BPB_SecPerTrk | Number of sectors per track, 63 for SD/MMC |
| 0x1Ah | 2 | BPB_NumHeads | Number of heads, 255 for SD/MMC |
| 0x1Ch | 4 | BPS_HiddSec | Number of sectors preceding the partition |
| 0x20h | 4 | BPB_TotSec32 | Total Count of sectors on the volume. If the size is smaller than 10000h (for FAT12/16), this field is 0 and BPB_TotSec16 is valid |

**Table 26-23: FAT Boot Sector (continued)**

| Offset | Length[bytes] | Name | Description |
|--------|--------------|------|-------------|
| 0x24h | 4 | BPB_FATSz32 | Size in sectors of one FAT. Field BPB_FATSz16 must be 0 |
| 0x28h | 2 | BPB_ExtFlags | FAT Flags |
| | | | [7]: 0=FAT is mirrored; 1=Only one FAT is active |
| | | | [3:0]: Number of used FAT if the mirroring used |
| 0x2Ah | 2 | BPB_FSVer | File system Version Number |
| 0x2Ch | 4 | BPB_RootClus | First Cluster number of the Root Directory |
| 0x30h | 2 | BPB_FSInfo | Sector number of FSINFO Structure in the reserved area, usually 1 |
| 0x32h | 2 | BPB_BkBootSec | If non-zero, indicates the sector number in the reserved-area of a copy of this Boot Sector |
| 0x34h | 12 | BPB_Reserved | Reserved, set to 00h |
| 0x40h | 1 | BS_DrvNum | Drive Number |
| 0x41h | 1 | BS_Reserved1 | 00h |
| 0x42h | 1 | BS_BootSig | Extended Boot Signature 29h: indicates that the following 3 fields are present |
| 0x43h | 4 | BS_VolID | Volume Serial Number |
| 0x47h | 11 | BS_VolLab | Volume Label |
| 0x52h | 8 | BS_FilSysType | File system Type: "FAT12", "FAT16", "FAT32" Note: This field is not mandatory (i.e BS_) therefore it cannot be used to indentify the partition type. |
| 0x1FEh | 2 | BPB_Signature | AA55h |

**AASI:**

To check whether or not a sector holds a valid FAT12/16/32 partition, only fields starting with BPB can be checked as they are mandatory. The fields starting from offset 0x24h to 0x1FDh cannot be used for the FAT12/16 filesystem. Roughly, the procedure is detailed in Figure 26-27. After a sector is identified, the ROM Code checks if the BPB_Signature is equal to AA55h. Then it checks some fields which must have some specific values (BPB_BytesPerSec, BPB_SecPerClus, BPB_RsvdSecCnt, BPB_NumFATs, BPB_RootEntCnt) If the geometry of the device is known (valid CHS for device size < 4Gbytes) then it is compared against BPB_SecPerTrk and BPB_NumHeads fields. If an MBR was found before, the partition size is also checked:

The field BPB_TotSec16 is used if the value is not 0 and if the partition offset (in the MBR) is 65518 or less (or BPB_TotSec32=0), otherwise BPB_TotSec32 is used (BPB_TotSec16=0). The partition sector offset is also checked: BPB_HiddSec = MBR_Partition_Offset (if this value is not 0 as some old systems do not update this field correctly). The last step is to decide which type of FAT file system it is. The ROM Code computes the number of clusters in the Data Area part of the partition:

Where Nb clusters is given by the size of the Data Area:

**Figure 26-27: FAT Detection Procedure**

```
Is there a FAT?
      │
      ▼
   ┌─────────────┐
   │0xAA55       │ Yes ──→ BPB_BytsPerSec ──→ Yes ──→ BPB_SecPerClus ──→ Yes
   │at offset    │         =                         1, 2, 4, 8, 16
   │0x1FE        │         512                       32, 64 or 128
   └─────────────┘
      │ No                │ No                        │ No
      │                   │                           │
      ▼                   ▼                           │
   ┌─────────────┐     ┌─────────────┐               │
   │BPB_RsvdSecCnt│ Yes│BPB_NumFATs  │ Yes ──→ BPB_RootEntCnt ──→ Yes
   │> 0          │     │= 2          │         or
   └─────────────┘     └─────────────┘         BPB_BytsPerSec/
      │ No                │ No                  32
      │                   │                     │ No
      │                   │                     │
      ▼                   ▼                     ▼
   ┌─────────────┐                         (loop back to retry)
   │Was there    │ Yes ──→ BPB_TotSec16 or ──→ Yes
   │an MBR?      │         BPB_TotSec32 =
   └─────────────┘         MBR_Partition_Size
      │ No                │ No
      │                   │
      ▼                   ▼
   Compute data area size to
   determine FAT type
      │
      ▼
   Success:
   FAT12,
   FAT16 or
   FAT32
```

#### 26.1.8.5.7.3 FAT12/16/32 Root Directory

The next task for the ROM Code is to find the booting file named **"MLO"** inside the Root Directory of the FAT12/16/32 file system. The file is not searched in any other location.

For a FAT12/16 file system, the Root Directory is a fixed location which is cluster 0. For a FAT32 file system, its cluster location is given by BPB_RootClus. The general formulae to find the sector number of a cluster is given by:

*Cluster_sector = BPB_RsvdSecCnt + BPB_NumFATs x BPB_FATSz + Cluster x BPB_SecPerClus*

Note: BPB_FatSz is BPB_FatSz16 for FAT12/16 or BPB_FatSz32 for FAT32

Note: the BPB_HiddSec isn't used. If even when the FAT file system is located somewhere other than on sector 0 (floppy-like). The ROM Code actually uses the partition offset taken from the MBR instead so this field can be wrong. If no MBR was found (floppy-like) the value 0 is used.

Each entry in the Root Directory is 32 bytes long and held information about the file, i.e: filename, date of creation, rights and sector location which is described in Table 26-24.

The ROM Code checks each entry in the Root Directory until either the booting file is found or the entry is empty (first byte is 00h) or when the end of the Root Directory has been reached. Entries with first byte at E5h (erased file) and with first byte at E5h (erased file) are ignored. When a valid file is found, the first cluster offset of the file is read from the DIR_FstClusHI/DIR_FstClusLo fields.

There is a slight difference between FAT12/16 and FAT32 when handling the Root Directory. On FAT12/16, this directory has a fixed location (see above) and length fixed by BPB_RootEntCnt which is the total number of 32 bytes entries. Handling this directory is therefore straight forward. On FAT32, the Root Directory is like any other directory. The File Allocation Table (FAT) has to be used in order to retrieve each sector of the Directory. The way the FAT is handled is described in the next paragraph.

**Table 26-24: FAT Directory Entry**

| Offset | Length[bytes] | Name | Description |
|--------|--------------|------|-------------|
| 0x00h | 11 | DIR_Name | Short Name (8+3) |
| 0x0Bh | 1 | DIR_Attr | File Attributes |
| | | | ATTR_READ_ONLY: 01h |
| | | | ATTR_HIDDEN: 02h |
| | | | ATTR_SYSTEM: 04h |
| | | | ATTR_VOLUME_ID: 08h |
| | | | ATTR_DIRECTORY: 10h |
| | | | ATTR_ARCHIVE: 20h |
| | | | ATTR_READ_ONLY \| ATTR_HIDDEN \| ATTR_SYSTEM \| ATTR_VOLUME_ID |
| 0x0Ch | 1 | DIR_NTRes | Reserved, set to 00h |
| 0x0Dh | 1 | DIR_CrtTimeTenth | Millisecond stamp at file creation |
| 0x0Eh | 2 | DIR_CrtTime | Time file was created |
| 0x10h | 2 | DIR_CrtDate | Date file was created |
| 0x12h | 2 | DIR_LstAccDate | Last Access date |
| 0x14h | 2 | DIR_FstClusHI | High word of this entry's first cluster number |
| 0x16h | 2 | DIR_WrtTime | Time of last write |
| 0x18h | 2 | DIR_WrtDate | Date of last write |
| 0x1Ah | 2 | DIR_FstClusLo | Low word of this entry's first cluster number |
| 0x1Ch | 4 | DIR_FileSize | File size in bytes |

#### 26.1.8.5.7.4 FAT12/16/32 File Allocation Table

The ROM Code needs to read the FAT in order to retrieve sectors either for the booting file or for the Root Directory (in case FAT32).

There can be multiple copies of the FAT inside the file system (ROM Code supports only 2) located after the Boot Sector:

*FAT_sector = BPB_HiddSec+BPB_RsvdSecCnt+BPB_FatSz x n*

Its size is given by BPB_FATSz16 or BPB_FATSz32. The ROM Code checks each copy of the FAT if identical. If case the values are not equal, it uses the value from the last FAT copy.

With FAT32 file system, the copy system can be disabled according to a flag located in BPB_ExtFlags[7]. If this flag is set then FAT_BPB_ExtFlags[3:0] is used. In this case no verification is made by the ROM Code with other copies of FAT.

The FAT is a simple array of values each refering to a cluster located in the Data Area. One entry of the array is 12, 16 or 32 bits depending on the file system in use.

The value inside an entry defines whether the cluster is being used or not and if another cluster has to be taken into account. This creates a linked chain of clusters defining the file. The meaning of an entry is described in Table 26-25.

**Note:** For compatibility reasons, clusters 0 and 1 are not used for files and those entries must contain FF8h and FFFh (for FAT12); FFF8h and FFFFh (for FAT16); ?FFFFFF8h and ?FFFFFFh (for FAT32).

**Table 26-25: FAT Entry Description**

| FAT12 | FAT16 | FAT32 | Description |
|-------|-------|-------|-------------|
| 000h | 0000h | ?0000000h | Free Cluster |
| 001h | 0001h | ?0000001h | Reserved Cluster |
| 002h-FEFh | 0002h-FFEFh | 00000002h-?FFFFFEFh | Used Cluster: value points to next cluster |
| FF0h-FF6h | FFF0h-FFF6h | ?FFFFFF0h-?FFFFFF6h | Reserved values |
| FF7h | FFF7h | ?FFFFFF7h | Bad Cluster |
| FF8h-FFFh | FFF8h-FFFFh | ?FFFFFF8h-?FFFFFFFh | Last cluster in file |

**Note:** FAT32 uses only bits [27:0]; the upper 4 bits are usually 0 and should be left untouched.

When accessing the Root Directory for FAT32, the ROM Code just starts from the Root Directory Cluster entry and follows the linked chain to retrieve the clusters.

When the booting file has been found, the ROM buffers each FAT entry corresponding to the file in a sector way. This means each cluster is translated to one or several sectors depending on how many sectors are in a cluster (BPB_SecPerClus). This buffer is used later on by the booting procedure to access the file.

#### 26.1.8.5.8 Pins Used

The list of device pins that are configured by the ROM in the case of MMC boot mode are as follows. Please note that all the pins might not be driven at boot time.

**Table 26-26: Pins Used for MMC0 Boot**

| Signal name | Pin Used in Device |
|-------------|-------------------|
| clk | mmc0_clk |
| cmd | mmc0_cmd |
| dat0 | mmc0_dat0 |
| dat1 | mmc0_dat1 |
| dat2 | mmc0_dat2 |
| dat3 | mmc0_dat3 |

**Table 26-27: Pins Used for MMC1 Boot**

| Signal name | Pin Used in Device |
|-------------|-------------------|
| clk | gpmc_csn1 |
| cmd | gpmc_csn2 |
| dat0 | gpmc_ad0 |
| dat1 | gpmc_ad1 |
| dat2 | gpmc_ad2 |
| dat3 | gpmc_ad3 |

---

#### 26.1.8.6 SPI

SPI EEPROMs or SPI flashes have an EEPROM or NOR flash backend and they connect to the device using the serial SPI protocol.

These devices typically operate in three stages: the command stage, the address stage and the data transfer stage. The command is usually an 8 bit value followed by the address (depending on the size of the device) followed by data to be read or written.

Because of the need for fewer pins, these devices are comparatively inexpensive, easy for board layout, and are the devices of choice when cost, complexity and form factor are critical considerations.

#### 26.1.8.6.1 Features

- Supports 12-MHz clock (50% duty cycle)
- Supports only SPI Mode 3 (clock polarity = 1, clock phase = 1)
- Supports only 24-bit addressable EEPROMs
- Supports only 4-pin SPI mode (CS, CLK, Serial Input, Serial Output)
- The boot devices must be connected to chip select 0 and must support the read command (03h)
- The boot image is copied into internal memory and then executed

#### 26.1.8.6.2 Initialization and Detection

The ROM Code initializes the SPI controller and clocks for communicating with the SPI device. The controller is initialized in Mode 3 and the clock is setup to operate at 12 MHz. There is no specific device identification routine prior to the execution by the ROM code to identify whether a boot device is preset or not. If no SPI device is present, the sector read will return only 0xFFFFFFFF and the SPI boot will be treated as failed.

#### 26.1.8.6.3 SPI Read Sector Procedure

The ROM Code reads SPI data from the boot device in 512 byte sectors. For each call to the SPI Read Sector routine, the SPI Read Command (0x03) is sent along with the 24 bit start address of the data to be read. Each Sector = 512bytes and the ROM bootloader will attempt the following:

1. Read Sector 1, Check the address: 0x0
2. Read Sector 2, Check the address: 0x200
3. Read Sector 3, Check the address: 0x400
4. Read Sector 4, Check the address: 0x600

The addresses mentioned above should contain the image size. If the value of the addresses mentioned above is neither 0x0 nor 0xFFFFFFFF, then the boot will proceed else it will move to the next sector. If no image is found or the checking fails sectors, the ROM bootloader will move to the next device.

From the next iteration onwards, a dummy value is transmitted on the master out line and the data is received on the master in line. This needs to be done because SPI protocol always operated in full duplex mode. The dummy value transmitted by the ROM is the Read Command appended to the start address. The data from the boot device is received MSB first.

As the A8 is a little-endian processor, and SPI devices in a big-endian format, this means that while writing to the flash, care needs to be taken to write the image in a big-endian format. This way we can avoid doing the endian conversion at boot time, thus improving boot performance.

#### 26.1.8.6.4 Pins Used

The list of device pins that are configured by the ROM in the case of SPI boot mode are as follows. Please note that all the pins might not be driven at boot time.

**Table 26-28: Pins Used for SPI Boot**

| Signal name | Pin Used in Device |
|-------------|-------------------|
| cs | spi0_cs0 |
| miso | spi0_d0 |
| mosi | spi0_d1 |
| clk | spi0_sclk |

---

#### 26.1.8.7 Blocks and Sectors Search Summary

Table 26-29 summarizes numbers of blocks and sectors which are searched during the memory booting from devices requiring image shadowing. NAND is organized with blocks, which are erasable units.

**Table 26-29: Blocks and Sectors Searched on Non-XIP Memories**

| Memory | Maximum Number of Blocks Checked | Number of Sectors Searched |
|--------|----------------------------------|---------------------------|
| NAND | first 4 | Number of sectors in one block⁽¹⁾ |
| SPI, eMMC/eSD and MMC/SD cards (raw mode) | first 4 | 1 |

⁽¹⁾ Depends on NAND geometry

Regarding MMC/SD card booting in FAT mode, the file system area is searched for one file.

---

### 26.1.9 Peripheral Booting

#### 26.1.9.1 Overview

The ROM Code can boot from three different peripheral interfaces:
- EMAC: 1000/100/10 Mbps Ethernet, using standard TCP/IP network boot protocols BOOTP and TFTP
- USB: Full-speed, client mode
- UART: 115.2Kbps, 8 bits, no parity, 1 stop bit, no flow control

The purpose of booting from a peripheral interface is to download an boot image from an external host (typically a PC). This booting method is mostly used for programming flash memory and initial flashing during device (e.g. in the case of initial flashing, firmware update or servicing).

#### 26.1.9.2 Boot Image Location and Size

The boot image is downloaded directly into internal RAM at the location 0x402F0400 on GP devices. The maximum size of downloaded image is 109 KB.

#### 26.1.9.3 Peripheral Boot Procedure Overview

**Figure 26-28 & 26-29: Peripheral Booting Procedure**

```
Peripheral Booting
      │
      ▼
   Initialize peripheral and
   ping the host
      │
      ▼
   ┌─────────────┐
   │Does the host│ No ──→ Boot Failed. Return to
   │respond?     │        framework to try next
   └─────────────┘        device in the device list.
      │ Yes
      │              No
      ▼              ◄────┐
   Start image transfer from  │
   host to OCMC RAM           │
      │                       │
      ▼                       │
   ┌──────────────────┐      │
   │Transfer          │ Yes ─→ Transfer control to
   │completed         │        0x402F0400 (GP)
   │successfully?     │        0x40300000 (HS)
   └──────────────────┘
```

---

#### 26.1.9.4 EMAC Boot Procedure

**NOTE:** See *AM335x ARM Cortex-A8 Microprocessors (MPUs) Silicon Errata* (literature number SPRZ360) for limitations of EMAC booting.

#### 26.1.9.4.1 Device Initialization

- EMAC boot uses the CPGMAC port 1 of the device
- Supports connecting to external Ethernet devices using the MII, RMII, RGMII and MDIO pins, based on SYSBOOT pin settings
- In dual-port configurations, the ROM code assumes that the Ethernet PHY with the lowest MDIO address (0-31) is connected to CPGMAC port 1
- Device uses EFUSE registers mac_id0_lo and mac_id0_hi in the control module for Ethernet MAC address of the device
- Device detects if the PHY is alive on the MDIO interface and
  - Reads the STATUS register to check if Ethernet link is active
  - Reads the CONTROL register to detect the auto-negotiated mode of operation
  - Is the mode full-duplex or half duplex
  - Speed of operation. 1000/100/10 Mbps. Link speed is determined by reading the Auto-Negotiation Advertisement and Auto-Negotiation Link Partner Base Page Ability registers in the device (See Section 1.2, *Silicon Revision Functional Differences and Enhancements*, for differences in operation based on AM335x silicon revision.)
  - Waits for five seconds for auto negotiation to complete before timing out
- ROM expects an external 50-MHz reference clock requirement when using RMII

#### 26.1.9.4.2 BOOTP (RFC 951)

The device then proceeds to obtain the IP and Boot information using BOOTP protocol. The device prepares and broadcasts a BOOTP message that has the following information:

- Device MAC address in "chaddr" field – to uniquely identify the device to the server
- "vendor-class-identifier" option number (RFC 1497, RFC 1533). Servers could use this information to identify the device type. The value present is **"AM335x ROM"**. (Note: See Section 1.2, *Silicon Revision Functional Differences and Enhancements*, for differences in operation based on AM335x silicon revision.)
- "Client-identifier" option number 61 (RFC 1497, RFC 1533). This has the ASIC-ID structure which contains additional info for the device.

The device then expects a BOOTP response that provides the following information for the booting to proceed:

- Device IP address from "yiaddr" field
- Subnetmask from extended option 1 (RFC 1497, RFC 1533)
- Gateway IP from extended option number 3 (RFC 1497, RFC 1533) or from "giaddr" field of BOOTP response
- Boot image filename from "file" field
- TFTP server IP address from the "siaddr" field

Timeouts and retries:
- Exponentially increasing timeouts starting from 4s, doubling for each retry.
- 5 retries

#### 26.1.9.4.3 TFTP (RFC 1350)

After a successful BOOTP completion, the device initiates the TFTP download of the boot image into SRAM. The device has the capability to reach a TFTP server within the local subnet or outside, through the gateway.

Timeouts and retries:
- Timeout of 1s to receive a response for the READ request
- 5 retries for the READ request
- Retries are managed by server once data transfer starts (server re-sends a data packet if the ACK was not received within a timeout value)
- Device has a 60s timeout to complete the data transfer, to handle the scenario if the server dies in the middle of a data transfer

#### 26.1.9.4.4 Pins Used

The list of device pins that are configured by the ROM in the case of EMAC boot mode are as follows. Please note that all the pins might not be driven at boot time.

**Table 26-30: Pins Used for EMAC Boot in MII Mode**

| Signal Name | Pin Used in Device | Pin Mux Mode |
|-------------|-------------------|--------------|
| gmii1_col | MII1_COL | 0 |
| gmii1_crs | MII1_CRS | 0 |
| gmii1_rxer | MII1_RX_ER | 0 |
| gmii1_txen | MII1_TX_EN | 0 |
| gmii1_rxdv | MII1_RX_DV | 0 |
| gmii1_txd[3:0] | MII1_TXD[3:0] | 0 |
| gmii1_txclk | MII1_TX_CLK | 0 |
| gmii1_rxclk | MII1_RX_CLK | 0 |
| gmii1_rxd[3:0] | MII1_RXD[3:0] | 0 |
| mdio_data | MDIO | 0 |
| mdio_clk | MDC | 0 |

**Table 26-31: Pins Used for EMAC Boot in RGMII Mode**

| Signal Name | Pin Used in Device | Pin Mux Mode |
|-------------|-------------------|--------------|
| rgmii1_tctl | MII1_TX_EN | 2 |
| rgmii1_rctl | MII1_RX_DV | 2 |
| rgmii1_td[3:0] | MII1_TXD[3:0] | 2 |
| rgmii1_tclk | MII1_TX_CLK | 2 |
| rgmii1_rclk | MII1_RX_CLK | 2 |
| rgmii1_rd[3:0] | MII1_RXD[3:0] | 2 |
| mdio_data | MDIO | 0 |
| mdio_clk | MDC | 0 |

**Table 26-32: Pins Used for EMAC Boot in RMII Mode**

| Signal Name | Pin Used in Device | Pin Mux Mode |
|-------------|-------------------|--------------|
| rmii1_crs_dv | MII1_CRS | 1 |
| rmii1_rxer | MII1_RX_ER | 1 |
| rmii1_txen | MII1_TX_EN | 1 |
| rmii1_txd[1:0] | MII1_TXD[1:0] | 1 |
| rmii1_rxd[1:0] | MII1_RXD[1:0] | 1 |
| rmii1_refclk | RMII1_REF_CLK (Driven by External 50-MHz Source) | 0 |
| mdio_data | MDIO | 0 |
| mdio_clk | MDC | 0 |

#### 26.1.9.4.5 SYSBOOT Pins

Some of the SYSBOOT pins have special meanings when EMAC boot is selected.

**Table 26-33: Ethernet PHY Mode Selection**

| SYSBOOT[7:6] | PHY Mode |
|--------------|----------|
| 00b | MII |
| 01b | RMII |
| 10b | Reserved |
| 11b | RGMII without internal delay |

---

#### 26.1.9.5 UART Boot Procedure

#### 26.1.9.5.1 Device Initialization

- UART boot uses UART0
- UART0 is configured to run at 115200 baud, 8-bits, no parity, 1 stop bit and no flow control

#### 26.1.9.5.2 Boot Image Download

- UART boot uses ancient protocol to receive the boot image
- Utilities like hyperterm, teraterm, minicom can be used on the PC side to download the boot image to the board
- With x-modem packet size of 1K throughout is roughly about 4KBytes/Sec
- The ROM code will ping the host 10 times in 3s to start x-modem transfer. If host does not respond, UART boot will timeout
- Once the transfer has started, if the host does not send any packet for 3s, UART boot will time out
- If more than one consecutive bytes of data in the packet is more than 2ms, the host is requested to re-transmit the entire packet again
- Error checking using the CRC-16 support in x-modem. If an error is detected, the host is requested to re-transmit the packet again

#### 26.1.9.5.3 Pins Used

The list of device pins that are configured by the ROM in the case of UART boot mode are as follows.

**Note:** All the pins might not be driven at boot time.

**Table 26-34: Pins Used for UART Boot**

| Signal name | Pin Used in Device |
|-------------|-------------------|
| rx | uart0_rxd |
| tx | uart0_txd |

---

#### 26.1.9.6 USB Boot Procedure

**NOTE:** See *AM335x ARM Cortex-A8 Microprocessors (MPUs) Silicon Errata* (literature number SPRZ360) for limitations of USB booting.

#### 26.1.9.6.1 Device Initialization

The ROM code supports booting from the USB interface under the following conditions:
- When the high-speed USB OTG (USBOTGHS) IP is used through USB0 interface
- USB operates in full-speed, client mode
- USB will operate only in device-powered mode
- Integrated transceiver (through UTMI)
- The enumeration default timeout is 3s (USB timeout)
- ROM code uses the default value of DATAPOLARITY

Even though using an OTG capable hardware, the ROM Code does not handle any OTG specific feature.

#### 26.1.9.6.1.1 Overview

In case of boot from USB is chosen by the SYSBOOT pin configuration:
- The USBOTGHS hardware and PRCM clocks are configured for UTMI mode
- The ROM Code continues with the USB procedure only if the USB cable is detected present (i.e. VBUS is detected at transceiver level and communicated as such through the UTPI traffic). If not, the initialization procedure is aborted
- The USB code implements the RNDIS class driver
- From a user's perspective, USB boot is indistinguishable from Ethernet boot. Refer to Section 26.1.9.4.2 for information on the BOOTP protocol
- The USB initialization procedure is shown in Figure 26-30

**Figure 26-30: USB Initialization Procedure**

```
USB Initialization
      │
      ▼
   Internal USB driver setup
      │
      ▼
   I/O setup for UTMI mode
      │
      ▼
   ┌─────────────────┐
   │USB cable        │ No ──→ Failed
   │attached?        │
   └─────────────────┘
      │ Yes
      ▼
   USB Connect
```

#### 26.1.9.6.1.2 Enumeration Descriptors

The device descriptor parameters which are used during enumeration are listed in Table 26-35. The default Vendor ID and Product ID can be automatically overridden by the customer by programming the values that are used to store these values.

**Table 26-35: Customized Descriptor Parameters**

| Parameter | Size [bytes] | TI Default Values |
|-----------|--------------|-------------------|
| Device ID code | 2 | 0000h |
| Device Class | 1 | 02h |
| Device Sub-Class | 1 | 00h |
| Device Protocol | 1 | 00h |
| Manufacturer | String | "Texas Instruments" |
| Product⁽¹⁾ | String | "AM335x USB" |
| Serial number | 1 | 0h |

⁽¹⁾ See Section 1.2, *Silicon Revision Functional Differences and Enhancements*, for differences in operation based on AM335x silicon revision.

#### 26.1.9.6.2 Image Download Procedure

- The device supports USB client boot only
- The ROM implements as RNDIS class driver, so the device enumerates as an ethernet port
- Standard RNDIS drivers present on Linux and Windows are picked up during the enumeration. No special drivers need to be installed
- Once the enumeration is complete, the customer can download the boot image using any standard TFTP server application

**Figure 26-31: Image Transfer for USB Boot**

```
HOST PC                          Device
(Windows/Linux)                  ROM Code
     │                                │
     │                                │
TFTP Application                TFTP Client Boot
     │                                │
RNDIS Driver                    RNDIS Class Driver
     │                                │
USB HW                          USB Low level Driver
     │                                │
     │◄──────────────────────────────►│
          Ethernet emulation over USB cable
```

#### 26.1.9.6.3 Pins Used

The list of the device pins that are configured by the ROM in the case of USB boot mode are as follows. Please note that all the pins might not be driven at boot time.

**Table 26-36: Pins Used for USB Boot**

| Signal Name | Pin Used in Device |
|-------------|-------------------|
| USB0_DM | USB0_DM |
| USB0_DP | USB0_DP |
| USB0_ID | USB0_ID |
| USB0_VBUS | USB0_VBUS |

---

#### 26.1.9.7 ASIC ID structure

The ASIC ID size is 58 bytes for UART and 81 bytes for others. The fields of this structure are unused. This structure is included purely for legacy purposes.

---

### 26.1.10 Image Format

#### 26.1.10.1 Overview

All preceding sections describe how the ROM Code searches and detects a boot image from a memory or a peripheral device type. This section describes the format of the boot image.

A boot image is basically made out of 2 major parts:
- The software to execute
- A header containing the destination address and size of the image for non XIP memory devices

The mandatory section of a boot image contains the software which will be loaded into the memory and executed. An overview of the image formats is shown in Figure 26-32:

**Figure 26-32: Image Formats on GP Devices**

```
a) GP device                    b) GP device
   non-XIP Memory Booting          Peripheral Booting and XIP Memory Booting

┌─────────────────┐           ┌─────────────────┐
│ GP Image Header │           │                 │
├─────────────────┤           │                 │
│                 │           │ Initial Software│
│ Initial Software│           │                 │
│                 │           │                 │
└─────────────────┘           └─────────────────┘
```

**Figure 26-33: Image Formats on GP and HS Devices**

```
a) GP device                    b) GP device
   non-XIP Memory Booting          Peripheral Booting and XIP Memory Booting

┌─────────────────┐           ┌─────────────────┐           Legend:
│ GP Image Header │           │                 │           ┌───┐
├─────────────────┤           │                 │           │   │ mandatory
│                 │           │ Initial Software│           └───┘
│ Initial Software│           │                 │           ┌ - ┐
│                 │           │                 │           │   │ optional
└─────────────────┘           └─────────────────┘           └ - ┘
                                                            *Must fit info one
                                                             512 bytes sector
            c) HS device
               Memory and Peripheral Booting

            ┌─────────────────┐
            │      TOC        │
            ├─────────────────┤
            │Public Keys      │
            │  Certificate    │
            ├─────────────────┤
            │      PPA        │
            ├ - - - - - - - - ┤
            │ R&D Certificate │
            ├ - - - - - - - - ┤
            │Initial Software │
            │  Certificate    │
            ├─────────────────┤
            │ Initial Software│
            └─────────────────┘
```

**a) GP Non-XIP Memory Booting**

Used for memories which require shadowing (e.g. MMC). Image must begin with a GP header which contains information on image size and destination address.

**b) GP Peripheral Booting and XIP Memory Booting**

When memory device is of XIP type (e.g. NOR) the GP header is not needed and the image can contain code for direct execution. The same image format is used for peripheral booting (where the code is transferred to internal RAM).

#### 26.1.10.2 Image Format for GP Device

For a **GP Device** there is no security involved and therefore neither keys nor certificates are needed in the boot image.

When the booting memory device is non-XIP (e.g. MMC) the image must contain a small header (referred to as GP header) with the size of the software to load and the destination address where to store it.

The GP header is not needed when booting from an XIP memory device (e.g. NOR) or in case of peripheral booting. In this case, the peripheral or memory booting image starts directly with executable code.

**Table 26-37: GP Device Image Format**

| Field | Non-XIP Device Offset | XIP Device Offset | Size[bytes] | Description |
|-------|-----------------------|-------------------|-------------|-------------|
| Size | 0000h | - | 4 | Size of the image |
| Destination | 0004h | - | 4 | Address where to store the image for entry |
| Image | 0008h | 0000h | x | Executable code |

Note: The "Destination" address field stands for both:
- Target address for the image copy from the non-XIP storage to the target XIP location (e.g., internal RAM or SDRAM)
- Entry point for image code

In other words the user must take care to locate the code entry point to the target address for image copy.

---

### 26.1.11 Table of Contents

The Table of Contents (TOC) is a header needed only in GP devices while using MMC RAW mode. This must not be confused with the TOC used in HS devices. The TOC is 512 bytes long and consists of a maximum of 2 TOC items of 32 bytes located one after the other. The second TOC item cannot be filled by FFh. Each TOC item contains information required by the ROM Code to find a valid image in RAW mode, as illustrated in Table 26-38. To detect RAW mode, the ROM also needs the magic values mentioned in Table 26-39. Other than the TOC item fields and magic values, all the other bytes in the 512-byte TOC must be zero.

**Table 26-38: The TOC Item Fields**

| Offset | Field | Size (bytes) | Description |
|--------|-------|--------------|-------------|
| 0x00h | Start | 4 | 0x00000040 |
| 0x04h | Size | 4 | 0x0000000C |
| 0x08h | Flags | 4 | Not used; should be zero |
| 0x0Ch | Align | 4 | Not used; should be zero |
| 0x10h | Load Address | 4 | Not used; should be zero |
| 0x14h | Filename | 12 | 12 character long name of sub image, including the zero ('\0') terminator. The ASCII representation is "CHSETTINGS." |

**Table 26-39: Magic Values for MMC RAW Mode**

| Offset | Value |
|--------|-------|
| 40h | 0xC0C0C0C1 |
| 44h | 0x00000100 |

The ROM Code recognizes the TOC based on the filename described in Table 26-40.

**Table 26-40: Filenames in TOC for GP Device**

| Filename | Description |
|----------|-------------|
| CHSETTINGS | Magic string used by ROM |

---

### 26.1.12 Authentication and Code Execution

#### 26.1.12.1 Overview

One of the early steps of the Public ROM Code execution is to search for a boot image from the requested medium (configured by the SYSBOOT pins) and copy it to RAM if needed. If the device is of GP type and boot interface is non-XIP then the image is simply copied to the provided destination address (part of external RAM) and then executed. If the boot interface is of XIP type then the image copy is not needed and execution is directly given to the XIP memory.

#### 26.1.12.2 Execution

The image is executed at the time the ROM Code performs the branch to the first executable instruction inside the image. For a GP Device in Peripheral booting, the execution address is the first address inside the GP header. The branch is performed in public ARM supervisor mode. The A8 register points to the Booting Parameters structure which contains various information about the booting execution. Table 26-41 details this structure.

**Table 26-41: Booting Parameters Structure**

| Offset | Field | Size (bytes) | Description |
|--------|-------|--------------|-------------|
| 00h | Reserved | 4 | Reserved |
| 04h | Memory booting device descriptor address | 4 | Pointer to the memory device descriptor that has been used during the memory booting |
| 08h | Current Booting Device | 1 | Code of device used for booting:<br>00h – void, no device<br>01h – XIP MUX1 memory<br>02h – XIP/WAIT MUX 1 (wait monitoring on)<br>03h – XIP MUX2 memory<br>04h – XIP/WAIT MUX 2 (wait monitoring on)<br>05h – NAND<br>06h – NAND with I2C<br>08h – MMC/SD port 0<br>09h – MMC/SD port 1<br>9h – SPI<br>41h – UART0<br>44h – USB<br>45h – CPGMAC0 |
| 09h | Reset Reason | 1 | Current reset reason bit mask<br>[bit=1 event present]<br>[0] – Power-on (cold) reset<br>[1] – Global warm software reset<br>[2] – Reserved<br>[3] – Reserved security violation reset<br>[4] – WDT1 timer reset<br>[5] – Global external warm reset secure watchdog reset<br>Other bits – Reserved<br>(Note: ROM code does not clear any of these bits. |
| 0Ah | Reserved | 1 | Reserved |

⁽¹⁾ More detailed information is found in each memory's booting description.

---

### 26.1.13 Wakeup

#### 26.1.13.1 Overview

The device supports a comprehensive set of low-powered states to meet aggressive power budget requirements. The different low power modes supported, from the lowest power consumption state to the highest power consumption state, include:

- RTC Only
- DeepSleep 0
- DeepSleep 1
- Standby
- Active

Of these states, the ROM code is involved only in the first three, as the A8 registers (particularly the PC) are preserved in all the other states. So for these modes, on wake up, execution will resume from the next instruction following the WFI.

In the RTC only mode, the ROM code involvement is trivial. A wakeup from the RTC only mode, from the ROM perspective, is indistinguishable from a power on reset. In DeepSleep 0 and DeepSleep 1 state, the ROM code should detect whether wakeup has occurred and branch to a user-defined return address, rather than perform a full boot.

#### 26.1.13.2 Wakeup Booting by ROM Code

In this device, when the A8 is in OFF mode, execution begins from the reset vector on wakeup. As the reset vector lies in ROM, the ROM code entity will take control of the A8 on a wakeup.

It must be noted here that in the device, in all the modes other than RTC-only, the L3 OCMC RAM is held in retention. This is a fundamental assumption of the ROM code. Because the contents of this RAM is not lost in these power-down modes, it is possible to return to a location in this memory.

This does away with the need of the ROM having to restore PLL and EMIF settings, which would have been needed if the ROM had to put A8 image from address, as DRAM is held in self refresh. The job of dialing up the PLLs and restoring EMIF and other peripheral register values is the responsibility of the user code. The recommendation is that such restoration code be placed in the OCMC RAM so that wakeup procedure is compatible between secure and non-secure devices.

In the past, the restore complex L4 and PLL register programming code in the ROM has made wakeup debugging very complicated. This has also traditionally been a source of many bugs, as the wakeup procedures are complicated and difficult to test exhaustively in pre-silicon.

The current architecture does away with these problems. The flow in the ROM on wakeup is kept simple and minimal. It only involves identifying that the reset reason is a Wakeup, and then branching to a return address, rather than proceeding with a full boot. This also helps in improving the time required to wakeup a system that is in Deep Sleep.

---

**Figure 26-34: Wakeup Booting by ROM**

```
Rom Init
      │
      ▼
   ┌─────────────────┐
   │Is the Reset     │ Yes ──→ Read the return address
   │reason for       │         from IPO register
   │Wakeup?          │         (0x44e11a28 [RTC])
   └─────────────────┘               │
      │ No                           ▼
      ▼                          ┌─────────────────┐
   Read SYSBOOT pins and         │Is the Return    │
   proceed with                  │address valid?   │
   regular ROM boot              └─────────────────┘
                                      │
                                      │ No
                                      ▼
                                 ┌─────────────────┐
                                 │  Dead Loop      │
                                 │(wait for        │
                                 │WatchDog)        │
                                 └─────────────────┘
                                      │
                                      │ Yes
                                      ▼
                                 Branch to the Return
                                 Address
```

**Figure 26-35: Wakeup Booting by ROM (Full Flow)**

```
Secure Rom Init
      │
      ▼
   Public Rom Init
      │
      ▼
   ┌─────────────────┐
   │Is the Reset     │ Yes ──→ Read the return address
   │reason for       │         from IPO register
   │Wakeup?          │         (0x44e11a28 [RTC])
   └─────────────────┘               │
      │ No                           ▼
      ▼                          ┌─────────────────┐
   Read SYSBOOT pins and         │Is the Return    │
   proceed with                  │address valid?   │
   regular ROM boot              └─────────────────┘
                                      │
                                      │ No
                                      ▼
                                 ┌─────────────────┐
                                 │  Dead Loop      │
                                 │(wait for        │
                                 │WatchDog)        │
                                 └─────────────────┘
                                      │
                                      │ Yes
                                      ▼
                                 Branch to the Return
                                 Address
```

---

### 26.1.14 Tracing

Tracing in the Public ROM Code includes three 32-bit vectors for which each bit corresponds to a particular "way point" in the ROM Code execution sequence (see Table 26-4). Tracing vectors are initialized at the very beginning of the startup phase and updated all along the boot process.

There are two sets of tracing vectors. The first set is the current trace information (after cold or warm reset). The second set holds a copy of trace vectors collected at the first ROM Code run after cold reset. As a consequence after a warm reset it is possible to have visibility on the boot scenario that occurred during cold reset.

**Table 26-42: Tracing Vectors**

| Trace vector | Bit No. | Group | Meaning |
|--------------|---------|-------|---------|
| 1 | 0 | Memory Boot | Memory booting device MMC/SD1 or SPI0 |
| 1 | 1 | General | Entered main function |
| 1 | 2 | General | Running after the cold reset |
| 1 | 3 | Boot | Main booting routine entered |
| 1 | 4 | Memory Boot | Memory booting started |
| 1 | 5 | Peripheral Boot | Peripheral booting started |
| 1 | 6 | Boot | Booting loop reached last device |
| 1 | 7 | Boot | GP header found |
| 1 | 8 | Boot | Reserved |
| 1 | 9 | Boot | Reserved |
| 1 | 10 | Peripheral Boot | Reserved |
| 1 | 11 | Peripheral Boot | Reserved |
| 1 | 12 | Peripheral Boot | Device initialized |
| 1 | 13 | Peripheral Boot | ASIC Id sent |
| 1 | 14 | Peripheral Boot | Image received |
| 1 | 15 | Peripheral Boot | Peripheral booting failed |
| 1 | 16 | Peripheral Boot | Booting Message not received (timeout) |
| 1 | 17 | Peripheral Boot | Image size not received(timeout) |
| 1 | 18 | Peripheral Boot | Image not received (timeout) |
| 1 | 19 | Reserved | Reserved |
| 1 | 20 | Configuration Header | CHSETTINGS found |
| 1 | 21 | Configuration Header | CHSETTINGS executed |
| 1 | 22 | Configuration Header | CHRAM executed |
| 1 | 23 | Configuration Header | CHFLASH executed |
| 1 | 24 | Configuration Header | CHMMCSD clocks executed |
| 1 | 25 | Configuration Header | CHMMCSD bus width executed |
| 1 | 26 | Reserved | Reserved |
| 1 | 27 | Reserved | Reserved |
| 1 | 28 | Reserved | Reserved |
| 1 | 29 | Reserved | Reserved |
| 1 | 30 | Reserved | Reserved |
| 1 | 31 | Reserved | Reserved |
| 2 | 0 | Companion chip | Reserved |
| 2 | 1 | Companion chip | Reserved |
| 2 | 2 | Companion chip | Reserved |
| 2 | 3 | Companion chip | Reserved |
| 2 | 4 | USB | USB connect |
| 2 | 5 | USB | USB configured state |
| 2 | 6 | USB | USB VBUS valid |
| 2 | 7 | USB | USB session valid |
| 2 | 8 | Reserved | Reserved |
| 2 | 9 | Reserved | Reserved |
| 2 | 10 | Reserved | Reserved |
| 2 | 11 | Reserved | Reserved |
| 2 | 12 | Memory Boot | Memory booting trial 0 |
| 2 | 13 | Memory Boot | Memory booting trial 1 |
| 2 | 14 | Memory Boot | Memory booting trial 2 |
| 2 | 15 | Memory Boot | Memory booting trial 3 |
| 2 | 16 | Memory Boot | Execute GP image |
| 2 | 17 | Peripheral Boot | Start authentication of peripheral boot image |
| 2 | 18 | Memory & Peripheral Boot | Start image authentication |
| 2 | 19 | Memory & Peripheral Boot | Image authentication failed |
| 2 | 20 | Memory & Peripheral Boot | Reserved |
| 2 | 21 | Memory & Peripheral Boot | Reserved |
| 2 | 22 | Memory & Peripheral Boot | Reserved |
| 2 | 23 | Memory & Peripheral Boot | Reserved |
| 2 | 24 | Memory & Peripheral Boot | Reserved |
| 2 | 25 | Memory & Peripheral Boot | Reserved |
| 2 | 26 | Memory & Peripheral Boot | Reserved |
| 2 | 27 | Memory & Peripheral Boot | Reserved |
| 2 | 28 | Memory & Peripheral Boot | Authentication procedure failed |
| 2 | 29 | Reserved | Reserved |
| 2 | 30 | Reserved | Reserved |
| 2 | 31 | Reserved | Reserved |
| 3 | 0 | Memory Boot | Memory booting device NULL |
| 3 | 1 | Memory Boot | Memory booting device XIP (MUX2) |
| 3 | 2 | Memory Boot | Memory booting device XIP w/WAIT (MUX2) |
| 3 | 3 | Memory Boot | Memory booting device XIP (MUX1) |
| 3 | 4 | Memory Boot | Memory booting device XIP w/ WAIT (MUX1) |
| 3 | 5 | Memory Boot | Memory booting device NAND |
| 3 | 6 | Reserved | Reserved |
| 3 | 7 | Memory Boot | Memory booting device MMC/SD1 |
| 3 | 8 | Reserved | Reserved |
| 3 | 9 | Reserved | Reserved |
| 3 | 10 | Memory Boot | Memory booting device NAND_I2C |
| 3 | 11 | Reserved | Reserved |
| 3 | 12 | Memory Boot | Memory booting device MMC0 |
| 3 | 13 | Reserved | Reserved |
| 3 | 14 | Reserved | Reserved |
| 3 | 15 | Reserved | Reserved |
| 3 | 16 | Reserved | Peripheral booting device UART0 |
| 3 | 17 | Reserved | Reserved |
| 3 | 18 | Reserved | Reserved |
| 3 | 19 | Reserved | Reserved |
| 3 | 20 | Peripheral Boot | Peripheral booting device USB |
| 3 | 21 | Reserved | Reserved |
| 3 | 22 | Peripheral Boot | Peripheral booting device CPGMAC0 |
| 3 | 23 | Reserved | Reserved |
| 3 | 24 | Reserved | Peripheral booting device NULL |
| 3 | 25 | Reserved | Reserved |
| 3 | 26 | Reserved | Reserved |
| 3 | 27 | Reserved | Reserved |
| 3 | 28 | Reserved | Reserved |
| 3 | 29 | Reserved | Reserved |
| 3 | 30 | Reserved | Reserved |
| 3 | 31 | Reserved | Reserved |

---

## Summary

This comprehensive document covers the complete AM335x ROM Code boot process, including:

### Key Features Covered:

1. **Boot ROM Architecture** - Three-layer design (High-level, Drivers, HAL)

2. **Device Types** - GP (General Purpose) and HS (High Secure) devices

3. **Boot Sources**:
   - Memory devices: XIP (NOR), NAND, MMC/SD, SPI-EEPROM
   - Peripheral interfaces: UART, USB, Ethernet (EMAC)

4. **Boot Flow**:
   - SYSBOOT pin configuration for boot sequence
   - Device detection and initialization
   - Image shadowing for non-XIP devices
   - Authentication for HS devices
   - Image execution

5. **Memory Booting Details**:
   - XIP Memory (NOR flash) - Direct execution
   - NAND - With ECC correction (BCH8/16), ONFI support
   - MMC/SD - RAW mode and FAT filesystem support
   - SPI - Serial EEPROM boot

6. **Peripheral Booting**:
   - EMAC - BOOTP/TFTP protocols, multiple PHY modes
   - UART - X-modem protocol at 115200 baud
   - USB - RNDIS class driver implementation

7. **Advanced Features**:
   - Image formats for GP and HS devices
   - Table of Contents (TOC) for MMC RAW mode
   - Wakeup booting from low-power states
   - Tracing vectors for debugging

8. **Technical Specifications**:
   - Memory maps and address spaces
   - Clock configurations
   - Pin multiplexing for each boot mode
   - Timing parameters for various interfaces

This document serves as a complete reference for understanding the AM335x boot process and can be used for:
- System design and hardware configuration
- Boot sequence debugging
- Custom bootloader development
- Training and educational purposes

---

**End of AM335x ROM Code Boot Documentation**
