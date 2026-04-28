# 9 Control Module

## Chapter 9: Control Module Reference

---

## 9.1 Introduction

The Control Module provides centralized control for device configuration and status not addressed within peripherals or the rest of the device infrastructure. It provides interfaces to control:

- **Functional I/O multiplexing** (pin muxing)
- **Emulation controls**
- **Device control and status**
- **DDR PHY control**
- **I/O control registers**
- **EDMA event multiplexing**

**Base Address:** 0x44E10000

**Important:** MPU must be in privileged mode to write to Control Module registers. Writes from user mode will not work.

---

## 9.2 Functional Description

### 9.2.1 Control Module Initialization

**Power-On Reset (POR) Behavior:**
- Reset values define safe state for device
- Only boot-time modules associated with pads
- Other module inputs internally tied
- Output pads turned off

**Post-Reset Configuration:**
- Software configures pad multiplexing
- Software configures pad electrical characteristics
- Software sets device-specific configuration

**Boot Configuration:**
- CONTROL_STATUS[7:0] SYS_BOOT reflects sys_boot pins
- Captured at POR in PRCM module
- Configuration pins sampled during PORz active period
- Final value before PORz rising edge latched

---

## 9.3 Pad Control Registers

### 9.3.1 Overview

Each configurable pin has its own 32-bit configuration register controlling:
- Pin multiplexing (mode selection)
- Pull-up/pull-down configuration
- Slew rate control
- Input buffer enable

**Register Format (conf_<module>_<pin>):**

| Bits | Field | Description |
|------|-------|-------------|
| [31:7] | Reserved | Read returns 0 |
| [6] | SLEWCTRL | Slew rate control:<br>0 = Fast<br>1 = Slow |
| [5] | RXACTIVE | Input enable:<br>0 = Receiver disabled (output only)<br>1 = Receiver enabled (input or output) |
| [4] | PULLTYPESEL | Pull type selection:<br>0 = Pulldown selected<br>1 = Pullup selected |
| [3] | PULLUDEN | Pull enable:<br>0 = Pullup/pulldown enabled<br>1 = Pullup/pulldown disabled |
| [2:0] | MUXMODE | Functional signal mux select (0-7) |

### 9.3.2 Mode Selection

**MUXMODE Field Values:**

| MUXMODE | Selected Mode | Description |
|---------|---------------|-------------|
| 0b000 (0) | Primary Mode | Function corresponds to pin name |
| 0b001 (1) | Mode 1 | Alternate function 1 |
| 0b010 (2) | Mode 2 | Alternate function 2 |
| 0b011 (3) | Mode 3 | Alternate function 3 |
| 0b100 (4) | Mode 4 | Alternate function 4 |
| 0b101 (5) | Mode 5 | Alternate function 5 |
| 0b110 (6) | Mode 6 | Alternate function 6 |
| 0b111 (7) | Mode 7 | Alternate function 7 / GPIO mode |

**Reset Value:** Most pads default to MUXMODE = 0b111 (Mode 7), except pads used at boot time.

**Mode 0:** Primary mode - function mapped to pin corresponds to pin name  
**Modes 1-7:** Alternate functions - some modes unused (no functional configuration)

**CAUTION:** The multiplexer is not glitch-free. Signal may glitch for a few nanoseconds during MUXMODE change. Ensure glitch does not cause contention or negatively impact external devices.

### 9.3.3 Pull Selection

**Pull Configuration Options:**

| PULLTYPESEL | PULLUDEN | Result |
|-------------|----------|--------|
| 0 (Pulldown) | 0 (Enabled) | Pulldown selected and activated |
| 0 (Pulldown) | 1 (Disabled) | Pulldown selected but not activated |
| 1 (Pullup) | 0 (Enabled) | Pullup selected and activated |
| 1 (Pullup) | 1 (Disabled) | Pullup selected but not activated |

**Power Optimization:** No automatic gating of pull resistors when pad configured as output. Recommended to disable internal pull resistors on output-only pads to avoid unnecessary power consumption.

### 9.3.4 Input Buffer Control (RXACTIVE)

**Purpose:** Enable/disable input buffer for power management and device isolation.

**Settings:**
- **0:** Receiver disabled - output only mode
- **1:** Receiver enabled - input or bidirectional mode

**Use Cases:**
- Power leakage reduction
- Device isolation through I/O
- Output-only signal configuration

### 9.3.5 Slew Rate Control (SLEWCTRL)

**Purpose:** Control signal edge rate to manage EMI and signal integrity.

**Settings:**
- **0:** Fast slew rate - faster edge transitions
- **1:** Slow slew rate - slower edge transitions, reduced EMI

**Note:** Some peripherals do not support slow slew rate. Refer to device datasheet for peripheral-specific slew rate support.

---

## 9.4 Pin Multiplexing Examples

### 9.4.1 Common Pin Configurations

**GPIO Configuration:**
```
MUXMODE = 7 (GPIO mode)
RXACTIVE = 1 (input enabled for reads)
PULLUDEN = 1 (pull disabled - external pull typically used)
```

**UART RX Pin:**
```
MUXMODE = 0 (or appropriate UART mode)
RXACTIVE = 1 (input enabled)
PULLTYPESEL = 1 (pullup)
PULLUDEN = 0 (pullup enabled)
```

**UART TX Pin:**
```
MUXMODE = 0 (or appropriate UART mode)
RXACTIVE = 0 (output only)
PULLUDEN = 1 (pull disabled)
```

**I2C Pin (Open-Drain):**
```
MUXMODE = 0 (or appropriate I2C mode)
RXACTIVE = 1 (bidirectional)
PULLTYPESEL = 1 (pullup)
PULLUDEN = 0 (pullup enabled - or use external pullup)
```

**SPI MOSI/MISO:**
```
MUXMODE = 0 (or appropriate SPI mode)
RXACTIVE = 1 (bidirectional for MISO, output for MOSI)
PULLUDEN = 1 (pull disabled - push-pull driver)
```

---

## 9.5 EDMA Event Multiplexing

### 9.5.1 Overview

**Problem:** Device has more DMA events than TPCC's maximum (64 events)

**Solution:** Event crossbar multiplexes extra events with direct-mapped events

**Control:** Mux control registers in Control Module select event routing to TPCC

**Default:** Direct-mapped event (mux selection = 0)

### 9.5.2 Event Crossbar Configuration

Each DMA channel can be configured to receive events from multiple sources through crossbar multiplexing.

**Registers:** tpcc_evt_mux_0_3 through tpcc_evt_mux_60_63

**Format:** Each register contains mux settings for 4 consecutive channels

---

## 9.6 Device Control and Status

### 9.6.1 Boot Status

**CONTROL_STATUS Register:** Reflects system boot and device type configuration

**SYS_BOOT[15:0] Field:**
- Configuration input pins captured at POR
- Sampled continuously during PORz active period
- Final value before PORz rising edge latched
- Determines boot device and configuration

### 9.6.2 Interprocessor Communication (IPC)

**Purpose:** Message passing between Cortex-M3 and Cortex-A8 MPU

**Registers:** IPC_MSG_REG0 through IPC_MSG_REG7

**Usage:**
- IPC_MSG_REG0: Command ID and status
- IPC_MSG_REG1-2: Command parameters
- IPC_MSG_REG3: Response from CM3
- IPC_MSG_REG4-6: Reserved
- IPC_MSG_REG7: Customer use

**M3_TXEV_EOI Register:** Clear/enable TX event from Cortex-M3 to Cortex-A8

**Reference:** See Section 8.1.4.6 for Cortex-M3 power management IPC usage

---

## 9.7 Priority Control

### 9.7.1 Interconnect Priority Control

**Registers:** INIT_PRIORITY_0, INIT_PRIORITY_1

**Purpose:** Control bus initiator priority at interconnects

**Default:** All initiators equal priority, round-robin allocation

**Priority Values:**

| Value | Priority Level |
|-------|---------------|
| 00 | Low priority |
| 01 | Medium priority |
| 10 | Reserved |
| 11 | High priority |

**Use Case:** Dynamic priority escalation for time-critical transfers

### 9.7.2 EMIF Priority Control

**Register:** MREQPRIO

**Purpose:** Set access priorities for masters accessing EMIF (DDR)

**Priority Range:** 000b (highest) to 111b (lowest)

**Configuration:**
- Each master has dedicated priority field
- Lower numeric value = higher priority
- Affects DDR access arbitration

---

## 9.8 Peripheral Control

### 9.8.1 USB Control and Status

#### USB_CTRLn Registers

**USB PHY Control:**
- PHY power control
- OTG settings
- GPIO mode configuration
- Charger detection control

**USB Lines as UART:**
- USB_CTRLn.GPIOMODE: Configure USB lines as GPIO
- Allows USB data lines for UART TX/RX

#### USB_STSn Registers

**USB PHY Status:**
- PHY operational status
- Connection detection
- VBUS status

### 9.8.2 USB Charger Detection

**Features:**
- Automatic detection of Charging Downstream Port (CDP)
- Automatic detection of Dedicated Charging Port (DCP)
- Compliant with USB Battery Charging Specification v1.1
- Charger enable signal output (USBx_CE)
- Manual and automatic modes
- Requires only 3.3V supply to operate

**Control Fields (USB_CTRLx):**

| Field | Description |
|-------|-------------|
| CDET_EXTCTL | 0 = Automatic detection<br>1 = Manual mode |
| CHGDET_RSTRT | Restart charger detection:<br>1→0 = Initiate detection<br>1 = Disable CE output |
| CHGDET_DIS | 0 = Charger detection enabled<br>1 = Charger detection powered down |
| CM_PWRDN | 0 = PHY powered up<br>1 = PHY powered down |

**Operation Sequence:**
1. Enable PHY: CM_PWRDN = 0
2. Enable charger detection: CHGDET_DIS = 0
3. Enable automatic mode: CDET_EXTCTL = 0
4. Initiate detection: CHGDET_RSTRT 1→0
5. Monitor USBx_CE output
6. If charger detected: USBx_CE goes high
7. To disable: Set CHGDET_DIS = 1 or CHGDET_RSTRT = 1

**Detection Steps Performed:**
1. VBUS Detect
2. Data Contact Detect
3. Primary Detection

**Note:** Secondary Detection (CDP vs DCP distinction) not implemented. USBx_CE only operates when USBx_ID grounded (host mode).

### 9.8.3 Ethernet MII Mode Selection

**Register:** GMII_SEL

**Configuration Options:**
- MII mode selection
- RMII mode selection
- RGMII mode selection
- Clock source selection
- Delay mode configuration

**Modes:**
- MII (Media Independent Interface)
- RMII (Reduced MII)
- RGMII (Reduced Gigabit MII)

### 9.8.4 Ethernet Reset Isolation

**Register:** RESET_ISO

**Purpose:** Allow warm reset without disrupting Ethernet switch traffic

**ISO_CONTROL Field:**
- When enabled: Warm reset blocked to EMAC switch
- When disabled (default): Warm reset propagates normally
- Cold/POR resets always propagate

**Protected Registers (when enabled):**
- GMII_SEL
- CONF_GPMC_A[11:0]
- Ethernet MII configuration registers
- MDIO/MDC configuration registers

### 9.8.5 Timer/eCAP Event Capture Control

**Registers:**
- TIMER_EVT_CAPTURE: Timer 5, 6, 7 event sources
- ECAP_EVT_CAPTURE: eCAP 0, 1, 2 event sources

**Purpose:** Select event capture sources for timers and eCAP modules

**Available Sources:**
- Timer I/O pins
- eCAP I/O pins
- Internal event sources
- Cross-connected timer/eCAP events

### 9.8.6 ADC Capture Control

**Registers:** ADC_x_AFE_CTRL

**Purpose:** Configure ADC analog front-end

**Controls:**
- Input selection
- Reference voltage selection
- Analog frontend parameters

### 9.8.7 SRAM LDO Control

**Register:** SMA2 (SRAM LDO Control)

**Purpose:** Control internal LDO for SRAM retention

**Configuration:**
- LDO enable/disable
- Auto-ramp enable for retention mode
- Used in low-power modes (DeepSleep0)

---

## 9.9 DDR PHY Control

### 9.9.1 Overview

Control Module contains registers for DDR PHY configuration:
- PHY initialization
- Drive strength control
- Impedance calibration
- Delay line configuration

**Registers:**
- DDR_IO_CTRL
- VTP_CTRL
- DDR_CKE_CTRL
- DDR_CMD_x_IOCTRL
- DDR_DATA_x_IOCTRL

### 9.9.2 VTP Calibration Control

**VTP_CTRL Register:**

**Purpose:** Control impedance calibration for DDR I/O

**Key Fields:**
- ENABLE: Enable VTP calibration
- READY: Calibration complete status
- FILTER: Filter control for calibration
- CLRZ: Clear calibration logic

**Calibration Sequence:**
1. Clear VTP: CLRZ = 0
2. Enable VTP: ENABLE = 1
3. Release clear: CLRZ = 1
4. Wait for READY = 1
5. Set FILTER = 1
6. DDR PHY ready for use

---

## 9.10 Key Control Module Registers

### 9.10.1 CONTROL_STATUS (Offset 0x40)

Boot and device status.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [31:23] | Reserved | R | - |
| [22:16] | SYSBOOT1 | R | System boot configuration [15:9] |
| [15:8] | SYSBOOT0 | R | System boot configuration [8:1] |
| [7:0] | DEVICETYPE | R | Device type |

### 9.10.2 CONF_<MODULE>_<PIN> (Various Offsets)

Pad control registers. See Section 9.3.1 for field descriptions.

### 9.10.3 IPC_MSG_REG0-7 (Offsets 0x400-0x41C)

Interprocessor communication registers.

| Register | Offset | Purpose |
|----------|--------|---------|
| IPC_MSG_REG0 | 0x400 | CMD_STAT[15:0], CMD_ID[31:16] |
| IPC_MSG_REG1 | 0x404 | Command parameter 1 |
| IPC_MSG_REG2 | 0x408 | Command parameter 2 |
| IPC_MSG_REG3 | 0x40C | CM3 response |
| IPC_MSG_REG4 | 0x410 | Reserved |
| IPC_MSG_REG5 | 0x414 | Reserved |
| IPC_MSG_REG6 | 0x418 | Reserved |
| IPC_MSG_REG7 | 0x41C | Customer use |

### 9.10.4 M3_TXEV_EOI (Offset 0x424)

Cortex-M3 TX event end-of-interrupt.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [0] | M3_TXEOI_ENABLE | W | Write 1 to clear M3 TX event |

### 9.10.5 MREQPRIO (Offset 0x4A4)

EMIF master request priority.

**Format:** Each master has 3-bit priority field

**Masters:**
- MPU
- PRU
- Graphics
- DMA
- Others

**Priority:** 000b (highest) to 111b (lowest)

### 9.10.6 USB_CTRL0/1 (Offsets 0x620, 0x628)

USB PHY control.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [17] | CM_PWRDN | R/W | PHY power down:<br>0 = Powered up<br>1 = Powered down |
| [16] | OTG_PWRDN | R/W | OTG comparators power down |
| [8] | CHGDET_DIS | R/W | Charger detection disable |
| [7] | CHGDET_RSTRT | R/W | Charger detection restart |
| [4] | CDET_EXTCTL | R/W | Charger detection external control |
| [3:2] | GPIOMODE | R/W | GPIO mode selection |

### 9.10.7 GMII_SEL (Offset 0x650)

Ethernet MII mode selection.

| Bits | Field | Description |
|------|-------|-------------|
| [2:1] | GMII1_SEL | Port 1 mode:<br>00 = MII<br>01 = RMII<br>10 = RGMII |
| [0] | RMII1_IO_CLK_EN | RMII reference clock enable |

### 9.10.8 RESET_ISO (Offset 0x65C)

Reset isolation control.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [0] | ISO_CONTROL | R/W | Reset isolation:<br>0 = Disabled (warm reset propagates)<br>1 = Enabled (warm reset blocked) |

### 9.10.9 VTP_CTRL (Offset 0x60C)

DDR VTP calibration control.

| Bits | Field | Type | Description |
|------|-------|------|-------------|
| [6] | ENABLE | R/W | VTP enable |
| [5] | READY | R | Calibration ready (read-only) |
| [4] | FILTER | R/W | Filter enable |
| [0] | CLRZ | R/W | Clear calibration logic |

### 9.10.10 DDR_IO_CTRL (Offset 0xE04)

DDR I/O control.

**Purpose:** Configure DDR PHY I/O parameters

**Fields:** Drive strength, pull settings, slew rate for DDR signals

---

## 9.11 Programming Sequences

### 9.11.1 Pin Mux Configuration

**Configure pin for specific peripheral:**

1. Determine required MUXMODE value from pin mux tables
2. Configure pad control register:
   - Set MUXMODE to desired mode
   - Set RXACTIVE (1 for input/bidirectional, 0 for output-only)
   - Configure PULLUDEN and PULLTYPESEL as needed
   - Set SLEWCTRL if applicable
3. Verify no conflicts with other pin assignments

### 9.11.2 USB Charger Detection

**Automatic detection:**

1. Power up PHY: USB_CTRL0.CM_PWRDN = 0
2. Enable charger detection: USB_CTRL0.CHGDET_DIS = 0
3. Enable automatic mode: USB_CTRL0.CDET_EXTCTL = 0
4. Ensure restart cleared: USB_CTRL0.CHGDET_RSTRT = 1
5. Initiate detection: USB_CTRL0.CHGDET_RSTRT = 0
6. Monitor USBx_CE pin for charger detection
7. When done, disable: USB_CTRL0.CHGDET_DIS = 1

### 9.11.3 VTP Calibration

**DDR I/O impedance calibration:**

1. Clear VTP: VTP_CTRL.CLRZ = 0
2. Enable VTP: VTP_CTRL.ENABLE = 1
3. Release clear: VTP_CTRL.CLRZ = 1
4. Poll VTP_CTRL.READY until = 1
5. Enable filter: VTP_CTRL.FILTER = 1
6. Proceed with DDR initialization

### 9.11.4 Ethernet Mode Selection

**Configure Ethernet interface mode:**

1. Write GMII_SEL register:
   - Set GMII1_SEL[2:1] for desired mode
   - Set RMII1_IO_CLK_EN if using RMII
2. Configure corresponding pin mux registers
3. Enable Ethernet module clocks
4. Initialize Ethernet PHY

---

## 9.12 Key Concepts Summary

### 9.12.1 Pin Multiplexing

```
Pin Configuration Components:
├── MUXMODE [2:0] → Function selection (0-7)
├── RXACTIVE [5] → Input buffer enable
├── PULLUDEN [3] → Pull resistor enable
├── PULLTYPESEL [4] → Pull up/down select
└── SLEWCTRL [6] → Edge rate control
```

### 9.12.2 Priority Hierarchy

```
Priority Levels:
Interconnect:
├── 11 (3) → High priority
├── 01 (1) → Medium priority
├── 00 (0) → Low priority
└── 10 (2) → Reserved

EMIF:
├── 000 (0) → Highest priority
├── 001-110 → Decreasing priority
└── 111 (7) → Lowest priority
```

### 9.12.3 USB Charger Detection States

```
Detection Flow:
1. VBUS Detect
   ↓
2. Data Contact Detect
   ↓
3. Primary Detection
   ↓
4. Charger Found → USBx_CE = High
   OR
   No Charger → USBx_CE = Low
```

### 9.12.4 Register Access Privilege

```
Access Requirements:
├── Read: User mode or Privileged mode
└── Write: Privileged mode ONLY
    └── MPU must be in privileged mode
```

---

## 9.13 Important Notes

1. **Privileged Mode Required:** All Control Module register writes require MPU in privileged mode

2. **Pin Mux Glitches:** Multiplexer not glitch-free. Ensure no contention during mode changes

3. **Pull Resistors:** No automatic gating when pad configured as output. Disable manually for power optimization

4. **Boot Configuration:** SYS_BOOT pins sampled at POR and latched in CONTROL_STATUS register

5. **USB Charger Detection:** Only operates in host mode (USBx_ID grounded)

6. **Reset Isolation:** Protects Ethernet operation during warm reset but not cold reset

7. **VTP Calibration:** Must complete before DDR operation. Check READY bit

8. **IPC Registers:** Follow documented sequence for Cortex-M3 communication (see Chapter 8)

9. **Default Modes:** Most pads default to Mode 7 (GPIO) at reset

10. **EDMA Crossbar:** Default to direct-mapped events. Configure crossbar as needed for additional event sources
