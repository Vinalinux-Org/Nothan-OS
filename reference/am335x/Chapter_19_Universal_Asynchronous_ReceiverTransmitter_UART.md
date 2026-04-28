# 19 Universal Asynchronous Receiver/Transmitter (UART)

## Chapter 19: Universal Asynchronous Receiver/Transmitter (UART)

---

## 19.1 Introduction

The AM335x UART/IrDA module is a full-duplex universal asynchronous receiver/transmitter with 16C750 compatibility. The device contains 6 UART instances (UART0-UART5).

### 19.1.1 UART Mode Features

- **16C750 compatibility**
- **Baud rate:** 300 bps to 3.6864 Mbps
- **Auto-baud:** 1200 bps to 115.2 Kbps
- **Flow control:**
  - Software flow control (programmable Xon/Xoff characters)
  - Hardware flow control (programmable Auto-RTS and Auto-CTS)
- **Programmable serial interface:**
  - Character length: 5, 6, 7, or 8 bits
  - Parity: Even, odd, mark (always 1), space (always 0), or no parity
  - Stop bits: 1, 1.5, or 2 stop bits
- **False start bit detection**
- **Line break generation and detection**
- **Modem control:** CTS, RTS, DSR, DTR, RI, and DCD
- **Fully prioritized interrupt system**
- **Internal loopback and test capabilities**
- **64-byte TX and RX FIFOs**

### 19.1.2 IrDA Mode Features

- **IrDA 1.4 support:**
  - SIR (Slow Infrared)
  - MIR (Medium Infrared)
  - FIR (Fast Infrared)
  - VFIR (Very Fast Infrared) - Not supported
- **Frame formatting:**
  - Variable xBOF character addition
  - EOF character addition
- **CRC:** Uplink/downlink generation and detection
- **Asynchronous transparency** (automatic break character insertion)
- **8-entry status FIFO** with selectable trigger levels
- **Error detection:**
  - Framing error
  - CRC error
  - Illegal symbol (FIR)
  - Abort pattern (SIR, MIR)

### 19.1.3 CIR Mode Features

- **Consumer IR support** for remote control applications
- **Transmit and receive capability**
- **Free data format** (supports private standards)
- **Configurable:**
  - Selectable bit rate
  - Configurable carrier frequency
  - Carrier duty cycle: 1/2, 5/12, 1/3, or 1/4

### 19.1.4 UART Instances

| Instance | Base Address | Features |
|----------|--------------|----------|
| UART0 | 0x44E09000 | Wakeup capability, CTS/RTS only |
| UART1 | 0x48022000 | Full modem control (CTS/RTS/DSR/DTR/RI/DCD) |
| UART2 | 0x48024000 | CTS/RTS only |
| UART3 | 0x481A6000 | CTS/RTS only |
| UART4 | 0x481A8000 | CTS/RTS only |
| UART5 | 0x481AA000 | CTS/RTS only |

**Limitations:**
- Full modem control on UART0, UART2-5: DCD, DSR, DTR, RI not pinned out
- Wakeup on UART1-5: Not supported (no SWake connection)
- DMA mode 2 and 3: Not supported (only mode 0 and 1 with EDMA)

---

## 19.2 Functional Description

### 19.2.1 Operating Modes

The UART supports three functional modes:

1. **UART Mode** - Standard asynchronous serial communication
2. **IrDA Mode** - Infrared communication (SIR, MIR, FIR)
3. **CIR Mode** - Consumer IR for remote control

### 19.2.2 FIFO Management

#### TX and RX FIFOs
- **Size:** 64 bytes each
- **Programmable trigger levels** for interrupt/DMA generation
- **Independent control** of TX and RX FIFOs

#### FIFO Trigger Levels

**Transmit FIFO Trigger:**
- Interrupt/DMA generated when FIFO level ≤ trigger level
- Granularity: 1 character or 4 characters
- Range: 0-63 characters (4-char granularity: 0-60)

**Receive FIFO Trigger:**
- Interrupt/DMA generated when FIFO level ≥ trigger level  
- Granularity: 1 character or 4 characters
- Range: 1-63 characters (4-char granularity: 4-60)

#### FIFO Operating Modes

**Interrupt Mode:**
- CPU services FIFO based on trigger levels
- Interrupt generated on trigger threshold

**Polled Mode:**
- CPU polls FIFO status
- No interrupts used

**DMA Mode:**
- EDMA services FIFO automatically
- DMA Mode 0: Single request per character
- DMA Mode 1: Burst request when trigger reached

### 19.2.3 Interrupt Management

#### UART Mode Interrupts

| Interrupt | Description | Priority |
|-----------|-------------|----------|
| RHR_IT | Receiver holding register interrupt | Highest |
| THR_IT | Transmitter holding register interrupt | - |
| LINE_STS_IT | Receiver line status interrupt (error) | - |
| MODEM_STS_IT | Modem status interrupt | - |
| XOFF_IT | Xoff character detected | - |
| RTS_IT | RTS level changed | - |
| CTS_IT | CTS level changed | Lowest |

**Interrupt Handling:**
- Read UART_IIR to identify interrupt source
- Service interrupt based on priority
- Clear interrupt by reading appropriate register or clearing condition

#### Wake-up Interrupt

Only UART0 supports wake-up from low-power modes:
- GPIO pad wake-up
- RX activity wake-up
- CTS/DSR/RI/DCD wake-up (if supported)

### 19.2.4 Power Management

#### Module Power Saving

**UART Mode:**
- **Idle state:** Module enters idle when no activity
- **IDLEMODE:** Configurable via UART_SYSC register
  - Force-idle
  - No-idle
  - Smart-idle
  - Smart-idle wakeup

**Sleep Mode:**
- Enable via UART_IER[4] SLEEP_MODE
- Module enters sleep when:
  - No TX activity
  - No RX activity  
  - No modem status change
- Wake-up on:
  - RX activity
  - Modem status change
  - Interrupt

#### System Power Saving

**Clock gating:** Functional and interface clocks can be gated
**Wake-up sources:** Configured via UART_WER register

---

## 19.3 Protocol Formatting

### 19.3.1 UART Mode

#### 19.3.1.1 Baud Rate Generation

Baud rate determined by divisor value programmed in DLL and DLH registers.

**Formula:**
```
Baud Rate = UART_CLK / (16 × Divisor)

Where:
- UART_CLK = Functional clock (typically 48 MHz)
- Divisor = (DLH << 8) | DLL
```

**Divisor Calculation:**
```
Divisor = UART_CLK / (16 × Desired_Baud_Rate)
```

**Common Baud Rates (48 MHz clock):**

| Baud Rate | Divisor (Decimal) | DLH | DLL |
|-----------|------------------|-----|-----|
| 300 | 10000 | 0x27 | 0x10 |
| 1200 | 2500 | 0x09 | 0xC4 |
| 2400 | 1250 | 0x04 | 0xE2 |
| 4800 | 625 | 0x02 | 0x71 |
| 9600 | 312 | 0x01 | 0x38 |
| 19200 | 156 | 0x00 | 0x9C |
| 38400 | 78 | 0x00 | 0x4E |
| 57600 | 52 | 0x00 | 0x34 |
| 115200 | 26 | 0x00 | 0x1A |
| 230400 | 13 | 0x00 | 0x0D |
| 460800 | 6 | 0x00 | 0x06 |
| 921600 | 3 | 0x00 | 0x03 |

**Mode Selection:**

Baud rate mode selected via UART_MDR1[2:0] MODE_SELECT:

| Value | Mode | Max Baud Rate |
|-------|------|---------------|
| 0x0 | UART 16x mode | UART_CLK/16 |
| 0x1 | SIR mode | UART_CLK/16 |
| 0x2 | UART 16x auto-baud | - |
| 0x3 | UART 13x mode | UART_CLK/13 |
| 0x4 | MIR mode | - |
| 0x5 | FIR mode | - |
| 0x6 | CIR mode | - |
| 0x7 | Disabled | - |

#### 19.3.1.2 Frame Formatting

**Character Length:**
- Configured via UART_LCR[1:0] CHAR_LENGTH
- 0x0 = 5 bits
- 0x1 = 6 bits
- 0x2 = 7 bits
- 0x3 = 8 bits

**Parity:**
- Enable: UART_LCR[3] PARITY_EN
- Type: UART_LCR[5:4] PARITY_TYPE_2:PARITY_TYPE_1
  - 00 = Odd parity
  - 01 = Even parity
  - 10 = Mark (parity bit always 1)
  - 11 = Space (parity bit always 0)

**Stop Bits:**
- Configured via UART_LCR[2] NB_STOP
- 0 = 1 stop bit
- 1 = 1.5 stop bits (5-bit char) or 2 stop bits (6-8 bit char)

**Frame Structure:**
```
Start | Data (5-8 bits) | Parity (optional) | Stop (1, 1.5, or 2)
```

#### 19.3.1.3 Hardware Flow Control

**Auto-RTS:**
- Automatically controls RTS output based on RX FIFO level
- Enable: UART_EFR[6] AUTO_RTS_EN
- Thresholds:
  - AUTO_RTS_START: RTS asserted when FIFO ≤ start level
  - AUTO_RTS_HALT: RTS de-asserted when FIFO ≥ halt level
- Configured via UART_TCR[7:0]

**Auto-CTS:**
- Automatically halts TX when CTS de-asserted
- Enable: UART_EFR[7] AUTO_CTS_EN
- TX resumes when CTS re-asserted

**Threshold Values:**

| TCR Bits | Threshold | Effective Level |
|----------|-----------|----------------|
| [7:4] | AUTO_RTS_START | Threshold × 4 |
| [3:0] | AUTO_RTS_HALT | Threshold × 4 |

#### 19.3.1.4 Software Flow Control

**XON/XOFF Protocol:**
- Transmitter sends XOFF when RX FIFO reaches threshold
- Transmitter sends XON when RX FIFO falls below threshold
- Receiver stops transmission on XOFF receipt
- Receiver resumes transmission on XON receipt

**Configuration:**
- Enable: UART_EFR[3:2]
  - Bit[3]: Transmit XON/XOFF
  - Bit[2]: Receive XON/XOFF
- Characters: Programmable via UART_XON1, UART_XON2, UART_XOFF1, UART_XOFF2

**XON/XOFF Modes:**
- XON1/XOFF1 only
- XON1 and XON2 / XOFF1 and XOFF2
- XON1 and XON2 / XOFF1 only

#### 19.3.1.5 Error Detection

**Line Status Errors:**

| Error | Bit | Description |
|-------|-----|-------------|
| Overrun Error | UART_LSR[1] OE | New data received before previous data read |
| Parity Error | UART_LSR[2] PE | Received parity doesn't match expected |
| Framing Error | UART_LSR[3] FE | Stop bit not detected at expected position |
| Break Interrupt | UART_LSR[4] BI | RX held low for longer than full frame |
| FIFO Error | UART_LSR[7] RX_FIFO_E | At least one error in RX FIFO |

**Error Handling:**
- Errors flagged in UART_LSR register
- Line status interrupt generated if enabled
- Error data marked in FIFO (FIFO mode)

#### 19.3.1.6 Autobauding

Automatic baud rate detection for incoming data.

**Supported Modes:**
- **Mode 1:** Based on '@' character (0x40)
- **Mode 2:** Based on any character

**Operation:**
1. Enable autobaud: UART_MDR1[2:0] = 0x2
2. Send autobaud character
3. Module detects baud rate
4. Divisor automatically updated
5. Normal operation resumes

**Range:** 1200 bps to 115.2 Kbps

---

## 19.4 Register Access Modes

### 19.4.1 Access Mode Overview

UART register access depends on operating mode. Three access modes available:

| Mode | UART_LCR Value | Purpose |
|------|---------------|---------|
| **Operational Mode** | UART_LCR[7] = 0 | Normal operation, data transfer |
| **Configuration Mode A** | UART_LCR = 0x80 (or 0x8X, X≠F) | Access DLL, DLH, some config registers |
| **Configuration Mode B** | UART_LCR = 0xBF | Access EFR, XON/XOFF, trigger registers |

### 19.4.2 Register Access Submodes

Within each mode, some registers require additional enable bits:

**TCR_TLR Submode:**
- Condition: UART_EFR[4] ENHANCED_EN = 1 AND UART_MCR[6] TCR_TLR = 1
- Provides access to: UART_TCR, UART_TLR

**MSR_SPR Submode:**
- Condition: UART_EFR[4] ENHANCED_EN = 0 OR UART_MCR[6] TCR_TLR = 0
- Provides access to: UART_MSR, UART_SPR

**XOFF Submode (Config Mode B only):**
- Condition: UART_EFR[4] ENHANCED_EN = 0 OR UART_MCR[6] TCR_TLR = 0
- Provides access to: UART_XOFF1, UART_XOFF2

### 19.4.3 Register Mapping by Mode

| Offset | Config Mode A | Config Mode B | Operational Mode |
|--------|--------------|---------------|------------------|
| 0x00 | UART_DLL (R/W) | UART_DLL (R/W) | UART_RHR (R) / UART_THR (W) |
| 0x04 | UART_DLH (R/W) | UART_DLH (R/W) | UART_IER (R/W) |
| 0x08 | UART_IIR (R) / UART_FCR (W) | UART_EFR (R/W) | UART_IIR (R) / UART_FCR (W) |
| 0x0C | UART_LCR (R/W) | UART_LCR (R/W) | UART_LCR (R/W) |
| 0x10 | UART_MCR (R/W) | UART_XON1 (R/W) | UART_MCR (R/W) |
| 0x14 | UART_LSR (R) | UART_XON2 (R/W) | UART_LSR (R) |
| 0x18 | UART_MSR/TCR (R/W) | UART_TCR/XOFF1 (R/W) | UART_MSR/TCR (R/W) |
| 0x1C | UART_SPR/TLR (R/W) | UART_TLR/XOFF2 (R/W) | UART_SPR/TLR (R/W) |

---

## 19.5 Key UART Registers

### 19.5.1 Control and Status Registers

#### UART_THR - Transmit Holding Register (Offset 0x00, Operational Mode, Write)

Transmit data register.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [7:0] | THR | W | Transmit data byte |

#### UART_RHR - Receive Holding Register (Offset 0x00, Operational Mode, Read)

Receive data register.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [7:0] | RHR | R | Received data byte |

#### UART_DLL - Divisor Latches Low (Offset 0x00, Config Mode A/B)

Lower byte of baud rate divisor.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [7:0] | CLOCK_LSB | R/W | Divisor low byte |

#### UART_DLH - Divisor Latches High (Offset 0x04, Config Mode A/B)

Upper byte of baud rate divisor.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [5:0] | CLOCK_MSB | R/W | Divisor high byte (6 bits) |

#### UART_IER - Interrupt Enable Register (Offset 0x04, Operational Mode)

Enable/disable interrupts.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | CTS_IT | R/W | 0 | CTS interrupt enable |
| [6] | RTS_IT | R/W | 0 | RTS interrupt enable |
| [5] | XOFF_IT | R/W | 0 | XOFF interrupt enable |
| [4] | SLEEP_MODE | R/W | 0 | Sleep mode enable |
| [3] | MODEM_STS_IT | R/W | 0 | Modem status interrupt enable |
| [2] | LINE_STS_IT | R/W | 0 | Line status interrupt enable |
| [1] | THR_IT | R/W | 0 | THR empty interrupt enable |
| [0] | RHR_IT | R/W | 0 | RHR data available interrupt enable |

#### UART_IIR_UART - Interrupt Identification Register UART (Offset 0x08, Read, reset=0x01)

Identify pending interrupt in UART mode. Read-only register.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:6] | FCR_MIRROR | R | 0 | Mirror contents of FCR[0] on both bits (FIFO enabled status) |
| [5:1] | IT_TYPE | R | 0 | Interrupt type (7 possible interrupts): 0x0=Modem interrupt (priority 4), 0x1=THR interrupt (priority 3), 0x2=RHR interrupt (priority 2), 0x3=Receiver line status error (priority 1), 0x6=RX timeout (priority 2), 0x8=Xoff/special character (priority 5), 0x10=CTS/RTS/DSR change state from active to inactive (priority 6) |
| [0] | IT_PENDING | R | 1 | Interrupt pending: 0=An interrupt is pending, 1=No interrupt pending (reset=1 means no interrupt) |

#### UART_FCR - FIFO Control Register (Offset 0x08, Write)

Configure FIFOs.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [7:6] | RX_FIFO_TRIG | W | RX FIFO trigger:<br>0x0 = 8 characters<br>0x1 = 16 characters<br>0x2 = 56 characters<br>0x3 = 60 characters |
| [5:4] | TX_FIFO_TRIG | W | TX FIFO trigger:<br>0x0 = 8 spaces<br>0x1 = 16 spaces<br>0x2 = 32 spaces<br>0x3 = 56 spaces |
| [3] | DMA_MODE | W | DMA mode enable |
| [2] | TX_FIFO_CLEAR | W | Clear TX FIFO (self-clearing) |
| [1] | RX_FIFO_CLEAR | W | Clear RX FIFO (self-clearing) |
| [0] | FIFO_ENABLE | W | FIFO enable |

#### UART_LCR - Line Control Register (Offset 0x0C)

Configure frame format and access mode.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | DIV_EN | R/W | 0 | Divisor latch enable (selects config mode A/B) |
| [6] | BREAK_EN | R/W | 0 | Break control enable |
| [5] | PARITY_TYPE_2 | R/W | 0 | Parity type bit 2 |
| [4] | PARITY_TYPE_1 | R/W | 0 | Parity type bit 1 |
| [3] | PARITY_EN | R/W | 0 | Parity enable |
| [2] | NB_STOP | R/W | 0 | Number of stop bits |
| [1:0] | CHAR_LENGTH | R/W | 0 | Character length |

#### UART_MCR - Modem Control Register (Offset 0x10)

Control modem signals.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [6] | TCR_TLR | R/W | 0 | TCR/TLR access enable |
| [5] | XON_EN | R/W | 0 | XON any enable |
| [4] | LOOPBACK_EN | R/W | 0 | Loopback mode enable |
| [3] | CD_STS_CH | R/W | 0 | CD status change (loopback) |
| [2] | RI_STS_CH | R/W | 0 | RI status change (loopback) |
| [1] | RTS | R/W | 0 | RTS output control |
| [0] | DTR | R/W | 0 | DTR output control |

#### UART_LSR_UART - Line Status Register UART (Offset 0x14, Read-only, reset=0x60)

Line status and errors in UART mode. LSR[4:2] reflect error bits (BI, FE, PE) of character at top of RX FIFO. LSR[7] is set when there is an error anywhere in the RX FIFO and cleared only when no more errors remain. Reading LSR clears OE if set. Reading LSR does NOT increment the RX FIFO read pointer (that requires reading RHR).

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | RXFIFOSTS | R | 0 | RX FIFO error: 0=Normal operation, 1=At least one parity error, framing error, or break indication in the RX FIFO |
| [6] | TXSRE | R | 1 | TX shift register empty: 0=Transmitter hold (TX FIFO) and shift registers not empty, 1=Both TX FIFO and shift registers empty |
| [5] | TXFIFOE | R | 1 | TX FIFO empty: 0=Transmit hold register not empty, 1=Transmit hold register empty (transmission not necessarily completed) |
| [4] | RXBI | R | 0 | Break interrupt: 0=No break condition, 1=Break detected (RX input was low for one character + 1 bit time) |
| [3] | RXFE | R | 0 | Framing error: 0=No framing error, 1=Framing error (received data did not have valid stop bit) |
| [2] | RXPE | R | 0 | Parity error: 0=No parity error, 1=Parity error in data being read from RX FIFO |
| [1] | RXOE | R | 0 | Overrun error: 0=No overrun error, 1=Overrun error (character in receive shift register not transferred to RX FIFO because FIFO was full) |
| [0] | RXFIFOE | R | 0 | RX FIFO data available: 0=No data in receive FIFO, 1=At least one data character in RX FIFO |

#### UART_MSR - Modem Status Register (Offset 0x18, MSR_SPR submode)

Modem status signals.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [7] | NCD_STS | R | DCD level (complement) |
| [6] | NRI_STS | R | RI level (complement) |
| [5] | NDSR_STS | R | DSR level (complement) |
| [4] | NCTS_STS | R | CTS level (complement) |
| [3] | DCD_STS | R | DCD changed |
| [2] | RI_STS | R | RI trailing edge detected |
| [1] | DSR_STS | R | DSR changed |
| [0] | CTS_STS | R | CTS changed |

### 19.5.2 Enhanced Feature Registers

#### UART_EFR - Enhanced Feature Register (Offset 0x08, Config Mode B)

Enable enhanced features.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | AUTO_CTS_EN | R/W | 0 | Auto-CTS enable |
| [6] | AUTO_RTS_EN | R/W | 0 | Auto-RTS enable |
| [5] | SPECIAL_CHAR_DETECT | R/W | 0 | Special character detect |
| [4] | ENHANCED_EN | R/W | 0 | Enhanced features enable |
| [3:0] | SW_FLOW_CONTROL | R/W | 0 | Software flow control:<br>Bit[3]: TX XON/XOFF<br>Bit[2]: RX XON/XOFF<br>Bit[1]: XON2/XOFF2 enable<br>Bit[0]: XON1/XOFF1 enable |

#### UART_TCR - Transmission Control Register (Offset 0x18, TCR_TLR submode)

Hardware flow control thresholds.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:4] | RX_FIFO_TRIG_START | R/W | 0 | Auto-RTS start threshold (×4) |
| [3:0] | RX_FIFO_TRIG_HALT | R/W | 0 | Auto-RTS halt threshold (×4) |

#### UART_TLR - Trigger Level Register (Offset 0x1C, TCR_TLR submode)

FIFO trigger levels for DMA mode.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:4] | RX_FIFO_TRIG_DMA | R/W | 0 | RX FIFO DMA trigger |
| [3:0] | TX_FIFO_TRIG_DMA | R/W | 0 | TX FIFO DMA trigger |

### 19.5.3 Mode and Configuration Registers

#### UART_MDR1 - Mode Definition Register 1 (Offset 0x20, reset=0x07)

Select operating mode. MDR1[2:0] must be programmed on startup after configuration registers. Value must not be changed during normal operation. If disabled (MODE_SELECT=7h), UART mode interrupt requests can still be generated unless disabled through IER.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | FRAME_END_MODE | R/W | 0 | IrDA mode only: 0=Frame-length method, 1=Set EOT bit method |
| [6] | SIP_MODE | R/W | 0 | MIR/FIR modes only: 0=Manual SIP mode (controlled by ACREG[3]), 1=Automatic SIP mode (generated after each transmission) |
| [5] | SCT | R/W | 0 | Store and control transmission: 0=Starts IR transmission when value written to THR, 1=Starts IR transmission with control of ACREG[2]. Before any transmission, no reception must be ongoing. |
| [4] | SET_TXIR | R/W | 0 | Configure IR transceiver: 0=If MDR2[7]=0 no action; if MDR2[7]=1 TXIR pin forced low, 1=TXIR pin forced high (independent of MDR2[7]) |
| [3] | IRSLEEP | R/W | 0 | IrDA/CIR sleep mode: 0=IrDA/CIR sleep mode disabled, 1=IrDA/CIR sleep mode enabled |
| [2:0] | MODE_SELECT | R/W | 7 | Operating mode: 0x0=UART 16x mode, 0x1=SIR mode, 0x2=UART 16x auto-baud, 0x3=UART 13x mode, 0x4=MIR mode, 0x5=FIR mode, 0x6=CIR mode, 0x7=Disabled (default) |

#### UART_MDR2 - Mode Definition Register 2 (Offset 0x24)

Additional mode configuration. MDR2[0] describes status of TX status interrupt in IIR[5]. MDR2[2:1] set trigger level for frame status FIFO (8 entries), must be programmed before mode is programmed in MDR1[2:0]. MDR2[6] gives flexibility to invert RX pin (by default, RX pin is inverted because most transceivers invert the IR receive pin).

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | SET_TXIR_ALT | R/W | 0 | Alternate functionality for MDR1[4]: 0=Normal mode, 1=Alternate mode for SETTXIR |
| [6] | IRRXINVERT | R/W | 0 | Invert RX pin before voting/sampling logic (IR mode only, does not affect UART modem mode RX): 0=Inversion is performed (default), 1=No inversion performed |
| [5:4] | CIR_PULSE_MODE | R/W | 0 | CIR pulse modulation: high level pulse width: 0x0=3/12 cycles, 0x1=4/12 cycles, 0x2=5/12 cycles, 0x3=6/12 cycles |
| [3] | UART_PULSE | R/W | 0 | UART mode only: pulse shaping: 0=Normal UART mode, 1=UART mode with pulse shaping |
| [2:1] | STS_FIFO_TRIG | R/W | 0 | IrDA mode only: frame status FIFO threshold: 0=1 entry, 1=4 entries, 2=7 entries, 3=8 entries |
| [0] | IRTX_UNDERRUN | R | 0 | IrDA TX status interrupt. When IIR[5] TX status interrupt occurs: 0=Last bit of frame was transmitted successfully, 1=TX underrun error occurred |

#### UART_SCR - Supplementary Control Register (Offset 0x40)

Additional control features. Bit 4 enables wake-up interrupt, but this interrupt is NOT mapped into IIR register. When interrupt occurs with no interrupt pending in IIR, check SSR[1]. To clear wake-up interrupt, reset SCR[4] to 0.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | RX_TRIG_GRANU1 | R/W | 0 | RX trigger granularity: 0=Granularity of 4 chars disabled, 1=Granularity of 1 char enabled |
| [6] | TX_TRIG_GRANU1 | R/W | 0 | TX trigger granularity: 0=Granularity of 4 chars disabled, 1=Granularity of 1 char enabled |
| [5] | DSR_IT | R/W | 0 | DSR interrupt enable: 0=DSR (active-low) interrupt disabled, 1=DSR (active-low) interrupt enabled |
| [4] | RXCTSDSRWAKEUPENABLE | R/W | 0 | RX/CTS wake-up enable: 0=Disables WAKEUP interrupt and clears SSR[1], 1=Waits for falling edge of RX, CTS, or DSR pins to generate interrupt |
| [3] | TXEMPTYCTLIT | R/W | 0 | TX empty control: 0=Normal mode for THR interrupt, 1=THR interrupt generated when TX FIFO AND TX shift register are both empty |
| [2:1] | DMA_MODE_2 | R/W | 0 | DMA mode (valid if SCR[0]=1): 0=DMA mode 0 (no DMA), 1=DMA mode 1 (UART_NDMA_REQ[0] in TX, [1] in RX), 2=DMA mode 2 (UART_NDMA_REQ[0] in RX), 3=DMA mode 3 (UART_NDMA_REQ[0] in TX) |
| [0] | DMA_MODE_CTL | R/W | 0 | DMA mode control: 0=DMA MODE set with FCR[3], 1=DMA MODE set with SCR[2:1] |

### 19.5.4 System Configuration Registers

#### UART_SYSC - System Configuration Register (Offset 0x54)

Module configuration and reset.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [4:3] | IDLEMODE | R/W | 0 | Idle mode:<br>0x0 = Force-idle<br>0x1 = No-idle<br>0x2 = Smart-idle<br>0x3 = Smart-idle wakeup |
| [2] | ENAWAKEUP | R/W | 0 | Wakeup enable |
| [1] | SOFTRESET | R/W | 0 | Software reset (self-clearing) |
| [0] | AUTOIDLE | R/W | 0 | Auto-idle enable |

#### UART_SYSS - System Status Register (Offset 0x58)

Module status.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [0] | RESETDONE | R | Reset complete status (0=Ongoing, 1=Complete) |

#### UART_WER - Wakeup Enable Register (Offset 0x5C, reset=0xFF)

Configure wakeup sources (UART0 only). Even if wakeup is disabled for certain events, if these events are also interrupts to the UART, the UART still registers the interrupt. All bits reset to 1 (all events enabled by default).

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | TXWAKEUPEN | R/W | 1 | TX wakeup: 0=Not allowed to wakeup system, 1=Event (THRIT/TXDMA request/TXSTATUSIT) can wakeup system |
| [6] | RLS_INTERRUPT | R/W | 1 | Receiver line status interrupt wakeup: 0=Not allowed, 1=Can wakeup |
| [5] | RHR_INTERRUPT | R/W | 1 | RHR interrupt wakeup: 0=Not allowed, 1=Can wakeup |
| [4] | RX_ACTIVITY | R/W | 1 | RX activity wakeup: 0=Not allowed, 1=Can wakeup |
| [3] | DCD_ACTIVITY | R/W | 1 | DCD activity wakeup: 0=Not allowed, 1=Can wakeup |
| [2] | RI_ACTIVITY | R/W | 1 | RI activity wakeup: 0=Not allowed, 1=Can wakeup |
| [1] | DSR_ACTIVITY | R/W | 1 | DSR activity wakeup: 0=Not allowed, 1=Can wakeup |
| [0] | CTS_ACTIVITY | R/W | 1 | CTS activity wakeup: 0=Not allowed, 1=Can wakeup |

### 19.5.5 IrDA/CIR Mode Registers

#### UART_IER_IRDA - Interrupt Enable Register IrDA (Offset 0x04, IrDA mode)

IrDA interrupt enable. In IrDA mode, EFR[4] has no impact on access to IER[7:4]. TXSTATUSIT interrupt reflects two possible conditions; read MDR2[0] to determine which.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | EOFIT | R/W | 0 | Received EOF interrupt: 0=Disabled, 1=Enabled |
| [6] | LINESTSIT | R/W | 0 | Receiver line status interrupt: 0=Disabled, 1=Enabled |
| [5] | TXSTATUSIT | R/W | 0 | TX status interrupt: 0=Disabled, 1=Enabled |
| [4] | STSFIFOTRIGIT | R/W | 0 | Status FIFO trigger level interrupt: 0=Disabled, 1=Enabled |
| [3] | RXOVERRUNIT | R/W | 0 | RX overrun interrupt: 0=Disabled, 1=Enabled |
| [2] | LASTRXBYTEIT | R/W | 0 | Last byte of frame in RX FIFO interrupt: 0=Disabled, 1=Enabled |
| [1] | THRIT | R/W | 0 | THR interrupt: 0=Disabled, 1=Enabled |
| [0] | RHRIT | R/W | 0 | RHR interrupt: 0=Disabled, 1=Enabled |

#### UART_IER_CIR - Interrupt Enable Register CIR (Offset 0x04, CIR mode)

CIR interrupt enable. In CIR mode, TXSTATUSIT has only one meaning (MDR2[0]=0). RXSTOPIT interrupt generated based on value set in EBLR register.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:6] | RESERVED | R | 0 | Reserved |
| [5] | TXSTATUSIT | R/W | 0 | TX status interrupt: 0=Disabled, 1=Enabled |
| [4] | RESERVED | R | 0 | Reserved |
| [3] | RXOVERRUNIT | R/W | 0 | RX overrun interrupt: 0=Disabled, 1=Enabled |
| [2] | RXSTOPIT | R/W | 0 | RX stop interrupt: 0=Disabled, 1=Enabled |
| [1] | THRIT | R/W | 0 | THR interrupt: 0=Disabled, 1=Enabled |
| [0] | RHRIT | R/W | 0 | RHR interrupt: 0=Disabled, 1=Enabled |

#### UART_IIR_CIR - Interrupt Identification Register CIR (Offset 0x08, CIR mode, Read)

Interrupt source in CIR mode. Read-only register.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:6] | RESERVED | R | 0 | Reserved |
| [5] | TXSTATUSIT | R | 0 | TX status interrupt: 0=Inactive, 1=Active |
| [4] | RESERVED | R | 0 | Reserved |
| [3] | RXOEIT | R | 0 | RX overrun interrupt: 0=Inactive, 1=Active |
| [2] | RXSTOPIT | R | 0 | Receive stop interrupt: 0=Inactive, 1=Active |
| [1] | THRIT | R | 0 | THR interrupt: 0=Inactive, 1=Active |
| [0] | RHRIT | R | 0 | RHR interrupt: 0=Inactive, 1=Active |

#### UART_IIR_IRDA - Interrupt Identification Register IrDA (Offset 0x08, IrDA mode, Read)

Interrupt source in IrDA mode. Read-only register.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | EOF_IT | R | 0 | Received EOF interrupt: 0=Inactive, 1=Active |
| [6] | LINE_STS_IT | R | 0 | Receiver line status interrupt: 0=Inactive, 1=Active |
| [5] | TX_STATUS_IT | R | 0 | TX status interrupt: 0=Inactive, 1=Active |
| [4] | STS_FIFO_IT | R | 0 | Status FIFO trigger level interrupt: 0=Inactive, 1=Active |
| [3] | RX_OE_IT | R | 0 | RX overrun interrupt: 0=Inactive, 1=Active |
| [2] | RX_FIFO_LAST_BYTE_IT | R | 0 | Last byte of frame in RX FIFO interrupt: 0=Inactive, 1=Active |
| [1] | THR_IT | R | 0 | THR interrupt: 0=Inactive, 1=Active |
| [0] | RHR_IT | R | 0 | RHR interrupt: 0=Inactive, 1=Active |

#### UART_LSR_CIR - Line Status Register CIR (Offset 0x14, CIR mode, reset=0x81)

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | THREMPTY | R | 1 | TX FIFO empty: 0=TX FIFO not empty, 1=TX FIFO empty (transmission not necessarily completed) |
| [6] | RESERVED | R | 0 | Reserved |
| [5] | RXSTOP | R | 0 | RX stop (generated based on EBLR value): 0=Reception ongoing/waiting for new frame, 1=Reception completed. Cleared on single read of LSR. |
| [4:1] | RESERVED | R | 0 | Reserved |
| [0] | RXFIFOE | R | 1 | RX FIFO empty: 0=At least one data character in RX FIFO, 1=No data in receive FIFO |

#### UART_LSR_IRDA - Line Status Register IrDA (Offset 0x14, IrDA mode, reset=0xA3)

When IrDA LSR is read, bits [4:2] reflect error bits (FL, CRC, ABORT) of frame at top of status FIFO.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | THR_EMPTY | R | 1 | TX FIFO empty: 0=TX FIFO not empty, 1=TX FIFO empty (transmission not necessarily completed) |
| [6] | STS_FIFO_FULL | R | 0 | Status FIFO full: 0=Not full, 1=Full |
| [5] | RX_LAST_BYTE | R | 1 | RX FIFO contains last byte of frame to be read. Used to determine frame boundary. Cleared on single read of LSR. |
| [4] | FRAME_TOO_LONG | R | 0 | Frame too long error in frame at top of status FIFO. Reception terminated when error detected. |
| [3] | ABORT | R | 0 | Abort pattern received (SIR/MIR) or illegal symbol (FIR). |
| [2] | CRC | R | 0 | CRC error in frame at top of status FIFO. |
| [1] | STS_FIFO_E | R | 1 | Status FIFO empty: 0=Not empty, 1=Empty |
| [0] | RX_FIFO_E | R | 1 | RX FIFO empty: 0=At least one data character, 1=No data in receive FIFO |

#### UART_XON1_ADDR1 - XON1/ADDR1 Register (Offset 0x10, Config Mode B)

In UART mode: XON1 character. In IrDA mode: ADDR1 address 1.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | XONWORD1 | R/W | 0 | 8-bit XON1 character in UART modes, or ADDR1 address 1 in IrDA modes |

#### UART_XON2_ADDR2 - XON2/ADDR2 Register (Offset 0x14, Config Mode B)

In UART mode: XON2 character. In IrDA mode: ADDR2 address 2.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | XONWORD2 | R/W | 0 | 8-bit XON2 character in UART modes, or ADDR2 address 2 in IrDA modes |

#### UART_XOFF1 - XOFF1 Register (Offset 0x18, XOFF submode)

XOFF1 character storage (UART mode only).

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | XOFFWORD1 | R/W | 0 | 8-bit XOFF1 character in UART modes |

#### UART_XOFF2 - XOFF2 Register (Offset 0x1C, XOFF submode)

XOFF2 character storage (UART mode only).

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | XOFFWORD2 | R/W | 0 | 8-bit XOFF2 character in UART modes |

#### UART_SPR - Scratchpad Register (Offset 0x1C, MSR_SPR submode)

General-purpose scratchpad register. Does not control the module. Used to hold temporary data.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | SPR_WORD | R/W | 0 | Scratchpad register, value 0 to FFh |

### 19.5.6 IrDA Frame Length and Status FIFO Registers

#### UART_TXFLL - Transmit Frame Length Low Register (Offset 0x28, IrDA/FIR mode)

LSB of 13-bit transmit frame length (in bytes). Used if frame-length method of frame closing is used.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | TXFLL | W | 0 | LSB register for frame length specification |

#### UART_TXFLH - Transmit Frame Length High Register (Offset 0x2C, IrDA/FIR mode)

MSB of 13-bit transmit frame length.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [4:0] | TXFLH | W | 0 | MSB register for frame length specification, value 0 to 0x1F |

#### UART_RXFLL - Received Frame Length Low Register (Offset 0x30, IrDA mode)

LSB of 12-bit maximum receive frame length. If intended maximum receive frame length is n bytes, program RXFLL+RXFLH to n+3 in SIR/MIR modes, or n+6 in FIR mode.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | RXFLL | W | 0 | LSB for specifying frame length in reception |

#### UART_RXFLH - Received Frame Length High Register (Offset 0x34, IrDA mode)

MSB of 12-bit maximum receive frame length.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [3:0] | RXFLH | W | 0 | MSB for specifying frame length in reception, value 0 to 0xF |

#### UART_SFLSR - Status FIFO Line Status Register (Offset 0x28, IrDA mode, Read)

Reading SFLSR reads frame status information from the status FIFO and increments the status FIFO read pointer. This register does NOT physically exist. SFREGL and SFREGH must be read first.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [4] | OE_ERROR | R | 0 | Overrun error in RX FIFO when frame at top of RX FIFO was received |
| [3] | FRAME_TOO_LONG_ERROR | R | 0 | Frame-length too long error in frame at top of RX FIFO |
| [2] | ABORT_DETECT | R | 0 | Abort pattern detected in frame at top of RX FIFO |
| [1] | CRC_ERROR | R | 0 | CRC error in frame at top of RX FIFO |

#### UART_SFREGL - Status FIFO Register Low (Offset 0x30, IrDA mode, Read)

LSB part of received frame length from status FIFO. This register does NOT physically exist. Reading does NOT alter status FIFO read pointer. Must be read BEFORE pointer is incremented by reading SFLSR.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | SFREGL | R | 0 | LSB part of frame length, value 0 to FFh |

#### UART_SFREGH - Status FIFO Register High (Offset 0x34, IrDA mode, Read)

MSB part of received frame length from status FIFO. Same usage notes as SFREGL.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [3:0] | SFREGH | R | 0 | MSB part of frame length, value 0 to 0xF |

#### UART_RESUME - RESUME Register (Offset 0x2C, IrDA/CIR mode, Read)

Clears internal flags that halt transmission/reception when an underrun/overrun error occurs. Reading this register resumes the halted operation. This register does NOT physically exist and always reads as 0x00.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | RESUME | R | 0 | Dummy read to restart TX or RX. Value 0 to FFh. |

#### UART_BLR - BOF Control Register (Offset 0x38, IrDA mode, reset=0x40)

Controls BOF start pattern selection for SIR mode.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | STSFIFORESET | R/W | 0 | Status FIFO reset. Self-clearing bit. |
| [6] | XBOFTYPE | R/W | 1 | SIR xBOF select: 0=FFh start pattern used, 1=C0h start pattern used (default) |
| [5:0] | RESERVED | R | 0 | Reserved |

#### UART_UASR - UART Autobauding Status Register (Offset 0x38, UART autobaud mode, Read)

Returns speed, number of bits per character, and parity type in UART autobauding mode. Input frequency must be fixed at 48 MHz. Supports only 7 and 8-bit characters (5 and 6 bits not supported). 7-bit char with space parity not supported. Baud range: 1200-115200 bps (10 possibilities).

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:6] | PARITYTYPE | R | 0 | Parity type detected: 0=No parity, 1=Space parity, 2=Even parity, 3=Odd parity |
| [5] | BITBYCHAR | R | 0 | Bits per character: 0=7-bit, 1=8-bit |
| [4:0] | SPEED | R | 0 | Speed detected: 0=None, 1=115200, 2=57600, 3=38400, 4=28800, 5=19200, 6=14400, 7=9600, 8=4800, 9=2400, 0xA=1200, 0xB-0x1F=Reserved |

#### UART_ACREG - Auxiliary Control Register (Offset 0x3C, IrDA/CIR mode)

IrDA/CIR auxiliary control. If TX FIFO is not empty and MDR1[5]=1, IrDA starts new transfer with data of previous frame as soon as abort frame has been sent. Therefore, TX FIFO must be reset before sending an abort frame.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7] | PULSETYPE | R/W | 0 | SIR pulse-width: 0=3/16 of baud-rate, 1=1.6 microseconds |
| [6] | SDMOD | R/W | 0 | SD/MODE pin output: 0=SD pin set high, 1=SD pin set low |
| [5] | DISIRRX | R/W | 0 | Disable RX input: 0=Normal (RX auto-disabled during transmit), 1=Disable RX input permanently |
| [4] | DISTXUNDERRUN | R/W | 0 | Disable TX underrun: 0=Long stop bits cannot be transmitted, TX underrun enabled, 1=Long stop bits can be transmitted |
| [3] | SENDSIP | R/W | 0 | MIR/FIR: Send serial infrared interaction pulse (SIP) at end of transmission. Auto-clears at end of SIP. |
| [2] | SCTXEN | R/W | 0 | Store and control TX start: When MDR1[5]=1, writing 1 starts frame transmission. Self-clearing. |
| [1] | ABORTEN | R/W | 0 | Frame abort: Writing 1 aborts current frame transmission (no end flag or CRC appended). |
| [0] | EOTEN | R/W | 0 | End of transmission enable: Writing 1 enables end-of-transmission detection. |

### 19.5.7 Additional Registers

#### UART_SSR - Supplementary Status Register (Offset 0x44, reset=0x04)

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [2] | DMACOUNTERRST | R/W | 1 | DMA counter reset: 0=DMA counter not reset when FIFO is reset via FCR[1]/FCR[2], 1=DMA counter reset when corresponding FIFO is reset |
| [1] | RXCTSDSRWAKEUPSTS | R | 0 | Pin falling edge detection. Reset only when SCR[4] is reset to 0: 0=No falling edge on RX/CTS/DSR, 1=Falling edge occurred on RX, CTS, or DSR |
| [0] | TXFIFOFULL | R | 0 | TX FIFO full: 0=TX FIFO not full, 1=TX FIFO full |

#### UART_EBLR - BOF Length Register (Offset 0x48)

In IrDA SIR: specifies number of BOF+xBOF to transmit (value must include the BOF character; 0=1 BOF + 255 xBOFs). In IrDA MIR: specifies number of additional start flags. In CIR: specifies number of consecutive zeros to receive before generating RXSTOP interrupt.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | EBLR | R/W | 0 | IrDA: Number of xBOFs (0=1BOF+255xBOF, 1=1BOF+0xBOF, N=1BOF+(N-1)xBOF). CIR: Number of consecutive zero bits before RXSTOP interrupt (0=Feature disabled, 1=After 1 zero, FFh=After 255 zeros) |

#### UART_MVR - Module Version Register (Offset 0x50, Read-only)

Hardware version. Reset value is fixed by hardware. Reset has no effect.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [10:8] | MAJORREV | R | 0 | Major revision number of the module |
| [5:0] | MINORREV | R | 0 | Minor revision number of the module |

#### UART_CFPS - Carrier Frequency Prescaler Register (Offset 0x60, CIR mode, reset=0x69)

CIR carrier frequency prescaler. Since CIR works at 30-56.8 kHz, the 48 MHz clock must be prescaled. Formula: Carrier_freq = 48MHz / (CFPS x 12). Default value 105 (0x69) = 38.1 kHz.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | CFPS | R/W | 0x69 | System clock prescaler for 12x multiple. CFPS=0 is NOT supported. Examples: 133=30.08kHz, 122=32.79kHz, 111=36.04kHz, 105=38.1kHz (default), 100=40kHz, 70=57.14kHz |

#### UART_RXFIFO_LVL - Received FIFO Level Register (Offset 0x64, Read-only)

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | RXFIFO_LVL | R | 0 | Current level (fill count) of the RX FIFO |

#### UART_TXFIFO_LVL - Transmit FIFO Level Register (Offset 0x68, Read-only)

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | TXFIFO_LVL | R | 0 | Current level (fill count) of the TX FIFO |

#### UART_IER2 - Interrupt Enable Register 2 (Offset 0x6C)

Enables RX/TX FIFO empty interrupts.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [1] | EN_TXFIFO_EMPTY | R/W | 0 | TX FIFO empty interrupt enable: 0=Disabled, 1=Enabled |
| [0] | EN_RXFIFO_EMPTY | R/W | 0 | RX FIFO empty interrupt enable: 0=Disabled, 1=Enabled |

#### UART_ISR2 - Interrupt Status Register 2 (Offset 0x70)

Displays status of RX/TX FIFO empty interrupts.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [1] | TXFIFO_EMPTY_STS | R/W | 0 | TX FIFO empty interrupt status: 0=Not pending, 1=Pending |
| [0] | RXFIFO_EMPTY_STS | R/W | 0 | RX FIFO empty interrupt status: 0=Not pending, 1=Pending |

#### UART_FREQ_SEL - Frequency Select Register (Offset 0x74)

Sets sample per bit when non-default frequency is used. MDR3[1] NONDEFAULT_FREQ must be set to 1 after writing this value. Value must be >= 6.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [7:0] | FREQ_SEL | R/W | 0 | Sample per bit for non-default fclk frequency. Must be >=6. MDR3[1] must be set to 1 after this is written. |

#### UART_MDR3 - Mode Definition Register 3 (Offset 0x80)

Additional mode control. Changing NONDEFAULT_FREQ bit (to any value) automatically disables device by setting MDR[2:0] to 111.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [2] | SET_DMA_TX_THRESHOLD | R/W | 0 | TX DMA threshold: 0=Use 64-TX_trigger as DMA threshold, 1=Enable TX_DMA_THRESHOLD register for custom threshold |
| [1] | NONDEFAULT_FREQ | R/W | 0 | Non-default clock: 0=Disabled, 1=Enabled (set FREQ_SEL and DLH/DLL accordingly) |
| [0] | DISABLE_CIR_RX_DEMOD | R/W | 0 | CIR RX demodulation: 0=CIR RX demodulation enabled, 1=Disabled (bypass demodulation) |

#### UART_TX_DMA_THRESHOLD - TX DMA Threshold Register (Offset 0x84)

Manually sets TX DMA threshold level. UART_MDR3[2] SET_TX_DMA_THRESHOLD must be 1. Constraint: value + tx_trigger_level must equal 64 (TX FIFO size). If not satisfied, 64-tx_trigger_level is used without modifying this register.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [5:0] | TX_DMA_THRESHOLD | R/W | 0 | TX DMA threshold level. Must satisfy: value + tx_trigger_level = 64 |


### 19.6.1 Software Reset

**Purpose:** Clear all UART registers to default state.

**Sequence:**
1. Set UART_SYSC[1] SOFTRESET = 1
2. Poll UART_SYSS[0] RESETDONE until = 1

### 19.6.2 FIFO and DMA Configuration

**Purpose:** Enable FIFOs and configure DMA mode.

**Sequence:**
1. Save UART_LCR value
2. Set UART_LCR = 0xBF (Config Mode B)
3. Save UART_EFR[4] ENHANCED_EN
4. Set UART_EFR[4] ENHANCED_EN = 1
5. Set UART_LCR = 0x80 (Config Mode A)
6. Save UART_MCR[6] TCR_TLR
7. Set UART_MCR[6] TCR_TLR = 1
8. Configure UART_FCR:
   - UART_FCR[7:6] RX_FIFO_TRIG
   - UART_FCR[5:4] TX_FIFO_TRIG
   - UART_FCR[3] DMA_MODE
   - UART_FCR[0] FIFO_ENABLE
9. Set UART_LCR = 0xBF (Config Mode B)
10. Configure UART_TLR:
    - UART_TLR[7:4] RX_FIFO_TRIG_DMA
    - UART_TLR[3:0] TX_FIFO_TRIG_DMA
11. Configure UART_SCR:
    - UART_SCR[7] RX_TRIG_GRANU1
    - UART_SCR[6] TX_TRIG_GRANU1
    - UART_SCR[2:1] DMA_MODE_2
    - UART_SCR[0] DMA_MODE_CTL
12. Restore UART_EFR[4] ENHANCED_EN
13. Set UART_LCR = 0x80 (Config Mode A)
14. Restore UART_MCR[6] TCR_TLR
15. Restore UART_LCR

### 19.6.3 Protocol and Baud Rate Configuration

**Purpose:** Configure baud rate, frame format, and interrupts.

**Sequence:**
1. Disable UART: UART_MDR1[2:0] MODE_SELECT = 0x7
2. Set UART_LCR = 0xBF (Config Mode B)
3. Save UART_EFR[4] ENHANCED_EN
4. Set UART_EFR[4] ENHANCED_EN = 1
5. Set UART_LCR = 0x00 (Operational Mode)
6. Clear UART_IER = 0x00 (disable interrupts, SLEEP_MODE = 0)
7. Set UART_LCR = 0xBF (Config Mode B)
8. Set divisor:
   - UART_DLL = divisor & 0xFF
   - UART_DLH = (divisor >> 8) & 0x3F
9. Set UART_LCR = 0x00 (Operational Mode)
10. Configure UART_IER (enable desired interrupts)
11. Set UART_LCR = 0xBF (Config Mode B)
12. Restore UART_EFR[4] ENHANCED_EN
13. Configure UART_LCR (frame format):
    - UART_LCR[7] DIV_EN = 0
    - UART_LCR[6] BREAK_EN = 0
    - UART_LCR[5:4] PARITY_TYPE
    - UART_LCR[3] PARITY_EN
    - UART_LCR[2] NB_STOP
    - UART_LCR[1:0] CHAR_LENGTH
14. Set UART_MDR1[2:0] MODE_SELECT (enable UART mode)

### 19.6.4 Hardware Flow Control Configuration

**Purpose:** Enable Auto-RTS/Auto-CTS.

**Sequence:**
1. Save UART_LCR
2. Set UART_LCR = 0x80 (Config Mode A)
3. Save UART_MCR[6] TCR_TLR
4. Set UART_MCR[6] TCR_TLR = 1
5. Set UART_LCR = 0xBF (Config Mode B)
6. Save UART_EFR[4] ENHANCED_EN
7. Set UART_EFR[4] ENHANCED_EN = 1
8. Configure UART_TCR:
   - UART_TCR[7:4] AUTO_RTS_START
   - UART_TCR[3:0] AUTO_RTS_HALT
9. Configure UART_EFR:
   - UART_EFR[7] AUTO_CTS_EN
   - UART_EFR[6] AUTO_RTS_EN
   - Restore UART_EFR[4] ENHANCED_EN
10. Set UART_LCR = 0x80 (Config Mode A)
11. Restore UART_MCR[6] TCR_TLR
12. Restore UART_LCR

### 19.6.5 Software Flow Control Configuration

**Purpose:** Enable XON/XOFF flow control.

**Sequence:**
1. Save UART_LCR
2. Set UART_LCR = 0xBF (Config Mode B)
3. Save UART_EFR[4] ENHANCED_EN
4. Set UART_EFR[4] ENHANCED_EN = 0 (XOFF submode)
5. Configure XON/XOFF characters:
   - UART_XON1_ADDR1
   - UART_XON2_ADDR2
   - UART_XOFF1
   - UART_XOFF2
6. Set UART_EFR[4] ENHANCED_EN = 1
7. Set UART_LCR = 0x80 (Config Mode A)
8. Save UART_MCR[6] TCR_TLR
9. Set UART_MCR[6] TCR_TLR = 1
10. Configure UART_TCR thresholds
11. Set UART_LCR = 0xBF (Config Mode B)
12. Configure UART_EFR[3:0] SW_FLOW_CONTROL
13. Restore UART_EFR[4] ENHANCED_EN
14. Set UART_LCR = 0x80 (Config Mode A)
15. Restore UART_MCR[6] TCR_TLR
16. Restore UART_LCR

---

## 19.7 Key Concepts Summary

### 19.7.1 Register Access Flow

```
Register Access Modes:
├── Operational Mode (LCR[7]=0)
│   ├── THR/RHR, IER, IIR/FCR, MCR, LSR
│   └── Submodes: MSR_SPR, TCR_TLR
├── Configuration Mode A (LCR=0x80)
│   ├── DLL, DLH, MCR
│   └── Submodes: MSR_SPR, TCR_TLR
└── Configuration Mode B (LCR=0xBF)
    ├── DLL, DLH, EFR, XON/XOFF
    └── Submodes: TCR_TLR, XOFF
```

### 19.7.2 FIFO Trigger Configuration

```
Trigger Level = Trigger Value × Granularity

Granularity:
- RX_TRIG_GRANU1 = 0: 4 characters
- RX_TRIG_GRANU1 = 1: 1 character
- TX_TRIG_GRANU1 = 0: 4 characters  
- TX_TRIG_GRANU1 = 1: 1 character

Example:
RX_FIFO_TRIG_DMA = 8, RX_TRIG_GRANU1 = 0
→ Trigger at 8 × 4 = 32 characters

RX_FIFO_TRIG_DMA = 8, RX_TRIG_GRANU1 = 1
→ Trigger at 8 × 1 = 8 characters
```

### 19.7.3 Baud Rate Calculation

```
Standard Divisor:
Divisor = UART_CLK / (16 × Baud_Rate)
Baud_Rate = UART_CLK / (16 × Divisor)

13x Mode Divisor:
Divisor = UART_CLK / (13 × Baud_Rate)
Baud_Rate = UART_CLK / (13 × Divisor)

Divisor Range: 1 to 16383 (14-bit)
```

### 19.7.4 Flow Control Thresholds

```
Hardware Flow Control:
RTS_START = AUTO_RTS_START × 4 (characters)
RTS_HALT = AUTO_RTS_HALT × 4 (characters)

Example:
AUTO_RTS_START = 12 → Assert RTS when FIFO ≤ 48 bytes
AUTO_RTS_HALT = 15 → De-assert RTS when FIFO ≥ 60 bytes
```

### 19.7.5 Interrupt Priority

```
Interrupt Priority (Highest to Lowest):
1. LINE_STS_IT (Errors: OE, PE, FE, BI)
2. RHR_IT (RX data available)
3. Character Timeout (No RX activity)
4. THR_IT (TX FIFO empty)
5. MODEM_STS_IT (CTS, DSR, RI, DCD change)
6. XOFF_IT (XON/XOFF detected)
7. RTS_IT (RTS level change)
8. CTS_IT (CTS level change)
```

---

## 19.8 Important Notes

1. **Register Access:** Always use proper access mode sequence to avoid unexpected behavior

2. **FIFO Enable:** Once enabled, FIFO can only be disabled by software reset or clearing FCR[0]

3. **Divisor Changes:** Disable UART (MDR1 = 0x7) before changing divisor values

4. **Sleep Mode:** Clear IER[4] SLEEP_MODE before accessing DLL/DLH

5. **Enhanced Features:** Set EFR[4] ENHANCED_EN = 1 to access IER[7:4], TCR, TLR

6. **FCR Register:** Write-only register, cannot be read back

7. **Hardware Flow Control:** Cannot be used simultaneously with software flow control

8. **Interrupt Clearing:** Read LSR to clear line status interrupt, read IIR to clear others

9. **Wakeup:** Only UART0 supports wakeup from low-power modes

10. **DMA Mode:** Use appropriate trigger levels and granularity for efficient DMA operation
