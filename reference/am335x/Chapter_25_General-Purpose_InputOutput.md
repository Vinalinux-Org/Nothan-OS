# 25 General-Purpose Input/Output

## 25.1 Introduction

### 25.1.1 Purpose of the Peripheral

The general-purpose interface combines four general-purpose input/output (GPIO) modules. Each GPIO module provides 32 dedicated general-purpose pins with input and output capabilities; thus, the general purpose interface supports up to **128 (4 × 32) pins**. 

These pins can be configured for the following applications:

- **Data input (capture)/output (drive)**
- **Keyboard interface** with a debounce cell
- **Interrupt generation** in active mode upon the detection of external events. Detected events are processed by two parallel independent interrupt-generation submodules to support biprocessor operations.
- **Wake-up request generation** in idle mode upon the detection of external events.

### 25.1.2 GPIO Features

Each GPIO module is made up of **32 identical channels**. Each channel can be configured to be used in the following applications:

- Data input/output
- Keyboard interface with a de-bouncing cell
- Synchronous interrupt generation (in active mode) upon the detection of external events (signal transition(s) and/or signal level(s))
- Wake-up request generation (in Idle mode) upon the detection of signal transition(s)

**Global features of the GPIO interface are:**

- Synchronous interrupt requests from each channel are processed by two identical interrupt generation sub-modules to be used independently by the ARM Subsystem
- Wake-up requests from input channels are merged together to issue one wake-up signal to the system
- Shared registers can be accessed through "Set & Clear" protocol

### 25.1.3 Unsupported GPIO Features

⚠️ **Important:** The wake-up feature of the GPIO modules is only supported on **GPIO0**.

---

## 25.2 Integration

The device instantiates four GPIO_V2 modules. Each GPIO module provides the support for 32 dedicated pins with input and output configuration capabilities. Input signals can be used to generate interruptions and wake-up signal. Two Interrupt lines are available for bi-processor operation. Pins can be dedicated to be used as a keyboard controller.

With four GPIO modules, the device allows for a maximum of **128 GPIO pins**. (The exact number available varies as a function of the device configuration and pin muxing.) 

- **GPIO0** is in the Wakeup domain and may be used to wake up the device via external sources. 
- **GPIO[1:3]** are located in the peripheral domain.

*[Sơ đồ GPIO integration - Figure 25-1 và Figure 25-2 - bỏ qua trong phiên bản text]*

### 25.2.3 GPIO Pin List

Each GPIO module includes 32 interface I/Os. These signals are designated as shown in the table below. Note that for this device, most of these signals will be multiplexed with functional signals from other interfaces.

**Table 25-4: GPIO Pin List**

| Pin | Type | Description |
|-----|------|-------------|
| GPIO0_[31:0] | I/O | General Purpose Input-Output pins |
| GPIO1_[31:0] | I/O | General Purpose Input-Output pins |
| GPIO2_[31:0] | I/O | General Purpose Input-Output pins |
| GPIO3_[31:0] | I/O | General Purpose Input-Output pins |
| GPIO4_[31:0] | I/O | General Purpose Input-Output pins |
| GPIO5_[31:0] | I/O | General Purpose Input-Output pins |

---

## 25.3 Functional Description

This section discusses the operational details and basic functions of the GPIO peripheral.

### 25.3.1 Operating Modes

Four operating modes are defined for the module:

#### 1. Active Mode
The module is running synchronously on the interface clock, interrupt can be generated according to the configuration and the external signals.

#### 2. Idle Mode
The module is in a waiting state, interface clock can be stopped, no interrupt can be generated, a wake-up signal can be generated according to the configuration and external signals.

⚠️ **Note:** Check the chip top-level functional specification for the availability of the debouncing clock while in Idle mode. If the debouncing clock is active, the debouncing cell can be used to sample and to filter the input to generate a wakeup event. Otherwise (debouncing clock inactive), the debouncing cell cannot be used, as it would gate all input signals.

#### 3. Inactive Mode
The module has no activity, interface clock can be stopped, no interrupt can be generated, and the wake-up feature is inhibited.

#### 4. Disabled Mode
The module is not used, internal clock paths are gated, no interrupt or wake-up request can be generated.

The Idle and Inactive modes are configured within the module and activated on request by the host processor through system interface sideband signals. The Disabled mode is set by software through a dedicated configuration bit. It unconditionally gates the internal clock paths not use for the system interface. 

All module registers are **8, 16 or 32-bit accessible** through the OCP compatible interface (little endian encoding). In active mode, the event detection (level or transition) is performed in the GPIO module using the interface clock. The detection's precision is set by the frequency of this clock and the selected internal gating scheme.

---

### 25.3.2 Clocking and Reset Strategy

#### 25.3.2.1 Clocks

GPIO module runs using **two clocks**:

1. **Debouncing clock** - Used for the debouncing sub-module logic (without the corresponding configuration registers). This module can sample the input line and filters the input level using a programmed delay.

2. **Interface clock** - Provided by the peripheral bus (OCP compatible system interface). It is used through the entire GPIO module (except within the debouncing sub-module logic). It clocks the OCP interface and the internal logic. Clock gating features allow adapting the module power consumption to the activity.

#### 25.3.2.2 Clocks, Gating and Active Edge Definitions

The interface clock provided by the peripheral bus (OCP compatible system interface) is used through the entire GPIO module. Two clock domains are defined: the OCP interface and the internal logic. Each clock domain can be controlled independently. 

- Sampling operations for the data capture and for the events detection are done using the **rising edge**. 
- The data loaded in the data output register (GPIO_DATAOUT) is set at the output GPIO pins synchronously with the **rising edge** of the interface clock.

**Five clock gating features are available:**

1. Clock for the system interface logic can be gated when the module is not accessed, if the **AUTOIDLE** configuration bit in the system configuration register (GPIO_SYSCONFIG) is set. Otherwise, this logic is free running on the interface clock.

2. Clock for the input data sample logic can be gated when the data in register is not accessed.

3. Four clock groups are used for the logic in the synchronous events detection. Each 8 input GPIO_V2 pins group will have a separate enable signal depending on the edge/level detection register setting. If a group requires no detection, then the corresponding clock will be gated. All channels are also gated using a **'one out of N' scheme**. N can take the values 1, 2, 4 or 8. The interface clock is enabled for this logic one cycle every N cycles. When N is equal to 1, there is no gating and this logic is free running on the interface clock. When N is between 2 to 8, this logic is running at the equivalent frequency of interface clock frequency divided by N.

4. In **Inactive mode**, all internal clock paths are gated.

5. In **Disabled mode**, all internal clock paths not used for the system interface are gated. All GPIO registers are accessible synchronously with the interface clock.

#### 25.3.2.3 Sleep Mode Request and Acknowledge

Upon a Sleep mode request issued by the host processor, the GPIO module goes to the Idle mode according to the **IDLEMODE** field in the system configuration register (GPIO_SYSCONFIG).

- **IDLEMODE = 0 (Force-Idle mode):** The GPIO goes in Inactive mode independently of the internal module state and the Idle acknowledge is unconditionally sent. In Force-Idle mode, the module is in Inactive mode and its wake-up feature is totally inhibited.

- **IDLEMODE = 1h (No-Idle mode):** The GPIO does not go to the Idle mode and the Idle acknowledge is never sent.

- **IDLEMODE = 2h (Smart-Idle mode) or IDLEMODE = 3h (Smart-Idle mode):** The GPIO module evaluates its internal capability to have the interface clock switched off. Once there is no more internal activity (the data input register completed to capture the input GPIO pins, there is no pending interrupt, all interrupt status bits are cleared, and there is no write access to GPIO_DEBOUNCINGTIME register pending to be synchronized), the Idle acknowledge is asserted and the GPIO enters Idle mode, ready to issue a wake-up request when the expected transition occurs on an enabled GPIO input pin. This wake-up request is effectively sent only if the **ENAWAKEUP** bit in GPIO_SYSCONFIG is set to enable the GPIO wakeup capability. When the system is awake, the Idle Request goes inactive, the Idle acknowledge and wake-up request (if it is the GPIO that triggered the system's wakeup) signals are immediately de-asserted, and the asynchronous wake-up request (if existing) is reflected into the synchronous interrupt status registers.

⚠️ **NOTE:** Idle mode request and Idle acknowledge are system interface sideband signals. Once the GPIO acknowledges the Sleep mode request (Idle acknowledge has been sent), the interface clock can be stopped anytime.

⚠️ **Important:** Upon a Sleep mode request issued by the host processor, the GPIO module goes to the Idle mode only if there is **no active bit** in GPIO_IRQSTATUS_RAW_n registers.

#### 25.3.2.4 Reset

The OCP hardware Reset signal has a global reset action on the GPIO. All configuration registers, all DFFs clocked with the Interface clock or Debouncing clock and all internal state machines are reset when the OCP hardware Reset is active (low level). 

The **RESETDONE** bit in the system status register (GPIO_SYSSTATUS) monitors the internal reset status: it is set when the Reset is completed on both OCP and Debouncing clock domains. 

The **software Reset** (SOFTRESET bit in the system configuration register) has the same effect as the OCP hardware Reset signal, and the RESETDONE bit in GPIO_SYSSTATUS monitors the reset completion.

---

### 25.3.3 Interrupt and Wakeup Events

*[Nội dung về Interrupt và Wakeup Events - có sơ đồ Figure 25-3 đến Figure 25-6 - bỏ qua trong phiên bản text]*

---

### 25.3.4 Peripheral Operating Features

#### 25.3.4.1 Software Reset

Software reset is achieved by setting the SOFTRESET bit in GPIO_SYSCONFIG. When the reset process is completed, the RESETDONE bit is set in GPIO_SYSSTATUS register. The software reset also resets the debouncing clock domain.

#### 25.3.4.2 Register Access Protocols

All GPIO registers are accessible with read/write operations and a subset of three registers (Interrupt enable 1 or 2, wake-up enable and data output) offers an alternate access protocol to set/clear register bits.

#### 25.3.4.2.1 Standard Read/Write Access Protocol

All registers are accessed by the standard read/write protocol. Each standard register has a unique address. A full 32 bits word is accessed for any memory access to this address.

#### 25.3.4.2.2 Alternate Set/Clear Protocol

To allow concurrent programming of shared register by two host processors, three registers (interrupt enable 1 or 2, wake-up enable and data output) are internally accessed through the set and clear protocol using two distinct addresses for each register.

This feature allows you to set or clear specific bits of the register with a single write access to the set register address or to the clear register address. This prevents any concurrent access conflict when both processors try to update the same register, provided they access only bits which are specifically assigned to them.

#### 25.3.4.2.2.1 Set Interrupt Enable Registers (GPIO_SETIRQENABLE1 and GPIO_SETIRQENABLE2):

- A write operation in the set interrupt enable1 (or enable2) register sets the corresponding bit in the interrupt enable1 (or enable2) register when the written bit is 1; a written bit at 0 has no effect.
- A read of the set interrupt enable1 (or enable2) register returns the value of the interrupt enable1 (or enable2) register.

#### 25.3.4.2.2.2 Set Wake-up Enable Register (GPIO_SETWKUENA):

- A write operation in the set wake-up enable register sets the corresponding bit in the wake-up enable register when the written bit is 1; a written bit at 0 has no effect.
- A read of the set wake-up enable register returns the value of the wake-up enable register.

#### 25.3.4.2.2.3 Set Data Output Register (GPIO_SETDATAOUT):

- A write operation in the set data output register sets the corresponding bit in the data output register when the written bit is 1; a written bit at 0 has no effect.
- A read of the set data output register returns the value of the data output register.

#### 25.3.4.2.2.4 Set Instruction Example

Assume the interrupt enable1 (or enable2) register (or the data output register) contains the binary value, `0000 0001 0000 0000h`, and you want to set bits 15, 3, 2, and 1.

With the set instruction feature, write `1000 0000 0000 1110h` at the address of the set interrupt enable1 (or enable2) register (or at the address of the set data output register). After this write operation, a reading of the interrupt enable1 (or enable2) register (or the data output register) returns `1000 0001 0000 1110h`; bits 15, 3, 2, and 1 are set.

⚠️ **NOTE:** Although the general-purpose interface registers are 32-bits wide, only the 16 least significant bits are represented in this example.

#### 25.3.4.3 Data Input (Capture)/Output (Drive)

The output enable register (**GPIO_OE**) controls the output/input capability for each pin. At reset, all the GPIO-related pins are configured as input and output capabilities are disabled. This register is not used within the module; its only function is to carry the pads configuration.

**Output Configuration:**
When configured as an output (the desired bit **reset** in GPIO_OE), the value of the corresponding bit in the GPIO_DATAOUT register is driven on the corresponding GPIO pin. Data is written to the data output register synchronously with the interface clock. This register can be accessed with read/write operations or by using the alternate set and clear protocol register update feature. This feature lets you set or clear specific bits of this register with a single write access to the set data output register (GPIO_SETDATAOUT) or to the clear data output register (GPIO_CLEARDATAOUT) address. 

⚠️ If the application uses a pin as an output and does not want interrupt/wake-up generation from this pin, the application must properly configure the wake-up enable and the interrupt enable registers.

**Input Configuration:**
When configured as an input (the desired bit **set to 1** in GPIO_OE), the state of the input can be read from the corresponding bit in the GPIO_DATAIN register. The input data is sampled synchronously with the interface clock and then captured in the data input register synchronously with the interface clock. When the GPIO pin levels change, they are captured into this register after **two interface clock cycles** (the required cycles to synchronize and to write data). 

⚠️ If the application uses a pin as an input, the application must properly configure the wake-up enable and the interrupt enable registers to the interrupt and wake up feature as needed.

#### 25.3.4.4 Debouncing Time

To enable the debounce feature for a pin, the GPIO configuration registers must be programmed as follows:

1. **The GPIO pin must be configured as input** in the output enable register (write 1 to the corresponding bit of the GPIO_OE register).

2. **The debouncing time must be set** in the debouncing value register (GPIO_DEBOUNCINGTIME). The GPIO_DEBOUNCINGTIME register is used to set the debouncing time for all input lines in the GPIO module. The value is global for all the ports of one GPIO module, so up to six different debouncing values are possible. The debounce cell is running with the debounce clock (32 kHz). This register represents the number of the clock cycle(s) (one cycle is 31 microseconds long) to be used.

   **Debouncing Time Formula:**
   ```
   Debouncing time = (DEBOUNCETIME + 1) × 31 µs
   ```
   Where the DEBOUNCETIME field value in the GPIO_DEBOUNCINGTIME register is from 0 to 255.

3. **The debouncing feature must be enabled** in the debouncing enable register (write 1 to the corresponding DEBOUNCEENABLE bit in the GPIO_DEBOUNCEENABLE register).

#### 25.3.4.5 GPIO as a Keyboard Interface

The general-purpose interface can be used as a keyboard interface.

*[Sơ đồ Figure 25-7 - bỏ qua trong phiên bản text]*

You can dedicate channels based on the keyboard matrix = r × c. The diagram shows row channels configured as inputs with the input debounce feature enabled. The row channels are driven high with an external pull-up. Column channels are configured as outputs and drive a low level.

**Operation:**

When a keyboard matrix key is pressed, the corresponding row and column lines are shorted together and a low level is driven on the corresponding row channel. This generates an interrupt based on the proper configuration.

When the keyboard interrupt is received, the processor can disable the keyboard interrupt and scan the column channels for the key coordinates.

- **The scanning sequence** has as many states as column channels: For each step in the sequence, the processor drives one column channel low and the others high.
- **The processor reads** the values of the row channels and thus detects which keys in the column are pressed.

At the end of the scanning sequence, the processor establishes which keys are pressed. The keyboard interface can then be reconfigured in the interrupt waiting state.

---

## 25.4 GPIO Registers

### 25.4.1 GPIO Registers

Table below lists the memory-mapped registers for the GPIO. All register offset addresses not listed should be considered as reserved locations and the register contents should not be modified.

**Table 25-5: GPIO Registers**

| Offset | Acronym | Register Name |
|--------|---------|---------------|
| 0h | GPIO_REVISION | GPIO Revision Register |
| 10h | GPIO_SYSCONFIG | GPIO System Configuration Register |
| 20h | GPIO_EOI | GPIO End of Interrupt Register |
| 24h | GPIO_IRQSTATUS_RAW_0 | GPIO Interrupt Raw Status Register 0 |
| 28h | GPIO_IRQSTATUS_RAW_1 | GPIO Interrupt Raw Status Register 1 |
| 2Ch | GPIO_IRQSTATUS_0 | GPIO Interrupt Status Register 0 |
| 30h | GPIO_IRQSTATUS_1 | GPIO Interrupt Status Register 1 |
| 34h | GPIO_IRQSTATUS_SET_0 | GPIO Interrupt Status Set Register 0 |
| 38h | GPIO_IRQSTATUS_SET_1 | GPIO Interrupt Status Set Register 1 |
| 3Ch | GPIO_IRQSTATUS_CLR_0 | GPIO Interrupt Status Clear Register 0 |
| 40h | GPIO_IRQSTATUS_CLR_1 | GPIO Interrupt Status Clear Register 1 |
| 44h | GPIO_IRQWAKEN_0 | GPIO Interrupt Wakeup Enable Register 0 |
| 48h | GPIO_IRQWAKEN_1 | GPIO Interrupt Wakeup Enable Register 1 |
| 114h | GPIO_SYSSTATUS | GPIO System Status Register |
| 130h | GPIO_CTRL | GPIO Control Register |
| 134h | GPIO_OE | GPIO Output Enable Register |
| 138h | GPIO_DATAIN | GPIO Data Input Register |
| 13Ch | GPIO_DATAOUT | GPIO Data Output Register |
| 140h | GPIO_LEVELDETECT0 | GPIO Level Detect 0 Register |
| 144h | GPIO_LEVELDETECT1 | GPIO Level Detect 1 Register |
| 148h | GPIO_RISINGDETECT | GPIO Rising Edge Detect Register |
| 14Ch | GPIO_FALLINGDETECT | GPIO Falling Edge Detect Register |
| 150h | GPIO_DEBOUNCENABLE | GPIO Debounce Enable Register |
| 154h | GPIO_DEBOUNCINGTIME | GPIO Debouncing Time Register |
| 190h | GPIO_CLEARDATAOUT | GPIO Clear Data Output Register |
| 194h | GPIO_SETDATAOUT | GPIO Set Data Output Register |

---

### 25.4.2 GPIO Register Details

#### 25.4.1.1 GPIO_REVISION Register (offset = 0h) [reset = 50600801h]

**Description:**
The GPIO revision register is a read only register containing the revision number of the GPIO module. A write to this register has no effect, that is, the same as the reset.

**Register Diagram:**
```
[31:30] SCHEME      [29:28] RESERVED   [27:16] FUNC
[15:11] RTL         [10:8]  MAJOR      [7:6]   CUSTOM     [5:0] MINOR
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-30 | SCHEME | R | 1h | Used to distinguish between old Scheme and current. |
| 29-28 | RESERVED | R | 1h | Reserved |
| 27-16 | FUNC | R | 60h | Indicates a software compatible module family. |
| 15-11 | RTL | R | 1h | RTL version |
| 10-8 | MAJOR | R | 0h | Major Revision |
| 7-6 | CUSTOM | R | 0h | Indicates a special version for a particular device. |
| 5-0 | MINOR | R | 1h | Minor Revision |

---

#### 25.4.1.2 GPIO_SYSCONFIG Register (offset = 10h) [reset = 0h]

**Description:**
The GPIO_SYSCONFIG register controls various parameters of the L4 interconnect. When the AUTOIDLE bit is not set, the GPIO_DATAIN read command has a 3 OCP cycle latency due to the data in sample gating mechanism. When the AUTOIDLE bit is not set, the GPIO_DATAIN read command has a 2 OCP cycle latency.

**Register Diagram:**
```
[31:5]  RESERVED
[4:3]   IDLEMODE    [2] ENAWAKEUP    [1] SOFTRESET    [0] AUTOIDLE
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-5 | RESERVED | R | 0h | Reserved |
| 4-3 | IDLEMODE | R/W | 0h | Power Management, Reg/Ack control.<br>0h = Force-idle. An idle request is acknowledged unconditionally<br>1h = No-idle. An idle request is never acknowledged<br>2h = Smart-idle. Acknowledgment to an idle request is given based on the internal activity of the module<br>3h = Smart Idle Wakeup (GPIO0 only) |
| 2 | ENAWAKEUP | R/W | 0h | 0h = Wakeup generation is disabled<br>1h = Wakeup capability is enabled upon expected transition on input GPIO pin. |
| 1 | SOFTRESET | R/W | 0h | Software reset.<br>This bit is automatically reset by the hardware.<br>During reads, it always returns 0.<br>0h = Normal mode<br>1h = The module is reset |
| 0 | AUTOIDLE | R/W | 0h | Internal interface clock gating strategy<br>0h = Internal interface OCP clock is free-running<br>1h = Automatic internal OCP clock gating, based on the OCP interface activity |

---

#### 25.4.1.3 GPIO_EOI Register (offset = 20h) [reset = 0h]

**Description:**
This module supports DMA events with its interrupt signal. This register must be written to after the DMA completes in order for subsequent DMA events to be triggered from this module.

**Register Diagram:**
```
[31:1]  RESERVED
[0]     DMAEvent_Ack
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-1 | RESERVED | R | 0h | Reserved |
| 0 | DMAEvent_Ack | R/W | 0h | Write 1 to clear the pending DMA event triggered to the DMA. The Module will be able to generate another DMA event only when the previous one has been acknowledged using this register.<br>Reads always returns 0. |

---

#### 25.4.1.4 GPIO_IRQSTATUS_RAW_0 Register (offset = 24h) [reset = 0h]

**Description:**
The GPIO_IRQSTATUS_RAW_0 register provides core status information for the interrupt handling, showing all active events (enabled and not enabled). The fields are read-write. Writing a 1 to a bit sets it to 1, that is, triggers the IRQ (mostly for debug). Writing a 0 has no effect, that is, the register value is not be modified. Only enabled, active events trigger an actual interrupt request on the IRQ output line.

**Register Diagram:**
```
[31:0]  INTLINE[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE[n] | R/W | 0h | Interrupt n status.<br>0h = No effect.<br>1h = IRQ is triggered. |

---

#### 25.4.1.5 GPIO_IRQSTATUS_RAW_1 Register (offset = 28h) [reset = 0h]

**Description:**
The GPIO_IRQSTATUS_RAW_1 register provides core status information for the interrupt handling, showing all active events (enabled and not enabled). The fields are read-write. Writing a 1 to a bit sets it to 1, that is, triggers the IRQ (mostly for debug). Writing a 0 has no effect, that is, the register value is not be modified. Only enabled, active events trigger an actual interrupt request on the IRQ output line.

**Register Diagram:**
```
[31:0]  INTLINE[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE[n] | R/W | 0h | Interrupt n status.<br>0h = No effect.<br>1h = IRQ is triggered. |

---

#### 25.4.1.6 GPIO_IRQSTATUS_0 Register (offset = 2Ch) [reset = 0h]

**Description:**
The GPIO_IRQSTATUS_0 register provides core status information for the interrupt handling, showing all active events which have been enabled. The fields are read-write. Writing a 1 to a bit clears the bit to 0, that is, the IRQ. Writing a 0 has no effect, that is, the register value is not modified. Only enabled, active events trigger an actual interrupt request on the IRQ output line.

**Register Diagram:**
```
[31:0]  INTLINE[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE[n] | R/W1C | 0h | Interrupt n status.<br>0h (W) = No effect.<br>0h (R) = IRQ is not triggered.<br>1h (W) = Clears the IRQ.<br>1h (R) = IRQ is triggered. |

---

#### 25.4.1.7 GPIO_IRQSTATUS_1 Register (offset = 30h) [reset = 0h]

**Description:**
The GPIO_IRQSTATUS_1 register provides core status information for the interrupt handling, showing all active events which have been enabled. The fields are read-write. Writing a 1 to a bit clears the bit to 0, that is, the IRQ. Writing a 0 has no effect, that is, the register value is not modified. Only enabled, active events trigger an actual interrupt request on the IRQ output line.

**Register Diagram:**
```
[31:0]  INTLINE[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE[n] | R/W1C | 0h | Interrupt n status.<br>0h (W) = No effect.<br>0h (R) = IRQ is not triggered.<br>1h (W) = Clears the IRQ.<br>1h (R) = IRQ is triggered. |

---

#### 25.4.1.8 GPIO_IRQSTATUS_SET_0 Register (offset = 34h) [reset = 0h]

**Description:**
All 1-bit fields in the GPIO_IRQSTATUS_SET_0 register enable a specific interrupt event to trigger an interrupt request on the appropriate output line. Writing a 1 to a bit enables the interrupt event. Writing a 0 has no effect, that is, the register value is not modified.

**Register Diagram:**
```
[31:0]  INTLINE[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE[n] | R/W | 0h | Interrupt n enable<br>0h = No effect.<br>1h = Enable IRQ generation. |

---

#### 25.4.1.9 GPIO_IRQSTATUS_SET_1 Register (offset = 38h) [reset = 0h]

**Description:**
All 1-bit fields in the GPIO_IRQSTATUS_SET_1 register enable a specific interrupt event to trigger an interrupt request on the appropriate output line. Writing a 1 to a bit enables the interrupt event. Writing a 0 has no effect, that is, the register value is not modified.

**Register Diagram:**
```
[31:0]  INTLINE[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE[n] | R/W | 0h | Interrupt n enable<br>0h = No effect.<br>1h = Enable IRQ generation. |

---

#### 25.4.1.10 GPIO_IRQSTATUS_CLR_0 Register (offset = 3Ch) [reset = 0h]

**Description:**
All 1-bit fields in the GPIO_IRQSTATUS_CLR_0 register disable a specific interrupt event. Writing a 1 to a bit disables the interrupt field. Writing a 0 has no effect, that is, the register value is not modified.

**Register Diagram:**
```
[31:0]  INTLINE[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE[n] | R/W | 0h | Interrupt n enable<br>0h = No effect.<br>1h = Disable IRQ generation. |

---

#### 25.4.1.11 GPIO_IRQSTATUS_CLR_1 Register (offset = 40h) [reset = 0h]

**Description:**
All 1-bit fields in the GPIO_IRQSTATUS_CLR_1 register disable a specific interrupt event. Writing a 1 to a bit disables the interrupt field. Writing a 0 has no effect, that is, the register value is not modified.

**Register Diagram:**
```
[31:0]  INTLINE[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE[n] | R/W | 0h | Interrupt n enable<br>0h = No effect.<br>1h = Disable IRQ generation. |

---

#### 25.4.1.12 GPIO_IRQWAKEN_0 Register (offset = 44h) [reset = 0h]

**Description:**
Per-event wakeup enable vector (corresponding to first line of interrupt). Every 1-bit field in the GPIO_IRQWAKEN_0 register enables a specific (synchronous) IRQ request source to generate an asynchronous wakeup (on the appropriate wakeup line). This register allows the user to mask the expected transition on input GPIO from generating a wakeup request. The GPIO_IRQWAKEN_0 is programmed synchronously with the interface clock before any idle mode request coming from the host processor. 

**Note:** In Force-Idle mode, the module wake-up feature is totally inhibited. The wake-up generation can also be gated at module level using the EnaWakeup bit in SYSCONFIG register.

**Register Diagram:**
```
[31:0]  INTLINE
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE | R/W | 0h | Wakeup Enable for Interrupt Line<br>0h = Disable wakeup generation.<br>1h = Enable wakeup generation. |

---

#### 25.4.1.13 GPIO_IRQWAKEN_1 Register (offset = 48h) [reset = 0h]

**Description:**
Per-event wakeup enable vector (corresponding to second line of interrupt). Every 1-bit field in the GPIO_IRQWAKEN_1 register enables a specific (synchronous) IRQ request source to generate an asynchronous wakeup (on the appropriate wakeup line). This register allows the user to mask the expected transition on input GPIO from generating a wakeup request. The GPIO_IRQWAKEN_1 is programmed synchronously with the interface clock before any idle mode request coming from the host processor. 

**Note:** In Force-Idle mode, the module wake-up feature is totally inhibited. The wake-up generation can also be gated at module level using the SYSCONFIG register.

**Register Diagram:**
```
[31:0]  INTLINE
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE | R/W | 0h | Wakeup Enable for Interrupt Line<br>0h = Disable wakeup generation.<br>1h = Enable wakeup generation. |

---

#### 25.4.1.14 GPIO_SYSSTATUS Register (offset = 114h) [reset = 0h]

**Description:**
The GPIO_SYSSTATUS register provides the reset status information about the GPIO module. It is a read-only register, a write to this register has no effect.

**Register Diagram:**
```
[31:1]  RESERVED
[0]     RESETDONE
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-1 | RESERVED | R | 0h | Reserved |
| 0 | RESETDONE | R | 0h | Reset status information.<br>0h = Internal Reset is on-going.<br>1h = Reset completed. |

---

#### 25.4.1.15 GPIO_CTRL Register (offset = 130h) [reset = 0h]

**Description:**
The GPIO_CTRL register controls the clock gating functionality. The DISABLEMODULE bit controls a clock gating feature at the module level. When set, this bit forces the clock gating for all internal clock paths. Module internal activity is suspended, System interface is not affected by this bit. System interface clock gating is controlled with the AUTOIDLE bit in the system configuration register (GPIO_SYSCONFIG). This bit is to be used for power saving when the module is not used because of the multiplexing configuration selected at the chip level. This bit has precedence over all other internal configuration bits.

**Register Diagram:**
```
[31:3]  RESERVED
[2:1]   GATINGRATIO    [0] DISABLEMODULE
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-3 | RESERVED | R | 0h | Reserved |
| 2-1 | GATINGRATIO | R/W | 0h | Gating Ratio.<br>Controls the clock gating for the event detection logic.<br>0h = Functional clock is interface clock.<br>1h = Functional clock is interface clock divided by 2.<br>2h = Functional clock is interface clock divided by 4.<br>3h = Functional clock is interface clock divided by 8. |
| 0 | DISABLEMODULE | R/W | 0h | Module Disable<br>0h = Module is enabled, clocks are not gated.<br>1h = Module is disabled, clocks are gated. |

---

#### 25.4.1.16 GPIO_OE Register (offset = 134h) [reset = FFFFFFFFh]

**Description:**
The GPIO_OE register is used to enable the pin's output/input capabilities. At reset, all the GPIO related pins are configured as input and output capabilities are disabled. This register is not used within the module, its only function is to carry the pads configuration. When the application is using a pin as an output and does not want interrupt generation from this pin, the application can/has to properly configure the Interrupt Enable registers.

**Register Diagram:**
```
[31:0]  OUTPUTEN[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | OUTPUTEN[n] | R/W | FFFFFFFFh | Output Data Enable<br>0h = The corresponding GPIO port is configured as an output.<br>1h = The corresponding GPIO port is configured as an input. |

---

#### 25.4.1.17 GPIO_DATAIN Register (offset = 138h) [reset = 0h]

**Description:**
The GPIO_DATAIN register is used to register the data that is read from the GPIO pins. The GPIO_DATAIN register is a read-only register. The input data is sampled synchronously with the interface clock and then captured in the GPIO_DATAIN register synchronously with the interface clock. When the GPIO pin levels are changing, GPIO pin levels are captured into this register after two interface clock cycles (the required cycles to synchronize and to write data in the register). When the AUTOIDLE bit in system configuration register (GPIO_SYSCONFIG) is set, the GPIO_DATAIN read command has a 3 OCP cycle latency due to the data in sample gating mechanism. When the AUTOIDLE bit is not set, the GPIO_DATAIN read command has a 2 OCP cycle latency.

**Register Diagram:**
```
[31:0]  DATAIN
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | DATAIN | R | 0h | Sampled Input Data |

---

#### 25.4.1.18 GPIO_DATAOUT Register (offset = 13Ch) [reset = 0h]

**Description:**
The GPIO_DATAOUT register is used for setting the value of the GPIO output pins. Data is written to the GPIO_DATAOUT register synchronously with the interface clock. This register can be accessed with read/write operations, or through an alternate access protocol that enables you to set or clear specific bits of this register with a single write access to the set data output register (GPIO_SETDATAOUT) or to the clear data output register (GPIO_CLEARDATAOUT) address.

**Register Diagram:**
```
[31:0]  DATAOUT
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | DATAOUT | R/W | 0h | Data to set on output pins |

---

#### 25.4.1.19 GPIO_LEVELDETECT0 Register (offset = 140h) [reset = 0h]

**Description:**
The GPIO_LEVELDETECT0 register is used to enable/disable for each input lines the low-level (0) detection to be used for the interrupt request generation. Enabling at the same time high-level detection and low-level detection for one given pin makes a constant interrupt.

**Register Diagram:**
```
[31:0]  LEVELDETECT0[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | LEVELDETECT0[n] | R/W | 0h | Low Level Interrupt Enable<br>0h = Disable the IRQ assertion on low-level detect.<br>1h = Enable the IRQ assertion on low-level detect. |

---

#### 25.4.1.20 GPIO_LEVELDETECT1 Register (offset = 144h) [reset = 0h]

**Description:**
The GPIO_LEVELDETECT1 register is used to enable/disable for each input lines the high-level (1) detection to be used for the interrupt request generation. Enabling at the same time high-level detection and low-level detection for one given pin makes a constant interrupt.

**Register Diagram:**
```
[31:0]  LEVELDETECT1[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | LEVELDETECT1[n] | R/W | 0h | High Level Interrupt Enable<br>0h = Disable the IRQ assertion on high-level detect.<br>1h = Enable the IRQ assertion on high-level detect. |

---

#### 25.4.1.21 GPIO_RISINGDETECT Register (offset = 148h) [reset = 0h]

**Description:**
The GPIO_RISINGDETECT register is used to enable/disable for each input lines the rising edge detection to be used for the interrupt request generation.

**Register Diagram:**
```
[31:0]  RISINGDETECT[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | RISINGDETECT[n] | R/W | 0h | Rising Edge Interrupt Enable<br>0h = Disable the IRQ assertion on rising edge detect.<br>1h = Enable the IRQ assertion on rising edge detect. |

---

#### 25.4.1.22 GPIO_FALLINGDETECT Register (offset = 14Ch) [reset = 0h]

**Description:**
The GPIO_FALLINGDETECT register is used to enable/disable for each input lines the falling edge detection to be used for the interrupt request generation.

**Register Diagram:**
```
[31:0]  FALLINGDETECT[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | FALLINGDETECT[n] | R/W | 0h | Falling Edge Interrupt Enable<br>0h = Disable the IRQ assertion on falling edge detect.<br>1h = Enable the IRQ assertion on falling edge detect. |

---

#### 25.4.1.23 GPIO_DEBOUNCENABLE Register (offset = 150h) [reset = 0h]

**Description:**
The GPIO_DEBOUNCENABLE register is used to enable/disable the debouncing feature for each input line.

**Register Diagram:**
```
[31:0]  DEBOUNCEENABLE[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | DEBOUNCEENABLE[n] | R/W | 0h | Debounce Enable<br>0h = Debouncing cell is inactive (direct input to the module).<br>1h = Debouncing cell is active for the input line. |

---

#### 25.4.1.24 GPIO_DEBOUNCINGTIME Register (offset = 154h) [reset = 0h]

**Description:**
The GPIO_DEBOUNCINGTIME register is used to set the debouncing time for all input lines. The debouncing cell is running with the debouncing clock (32 kHz). This register represents the number of clock cycles to be used.

**Debouncing Time Formula:**
```
Debouncing time = (DEBOUNCETIME + 1) × 31 µs
```

**Register Diagram:**
```
[31:8]  RESERVED
[7:0]   DEBOUNCETIME
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-8 | RESERVED | R | 0h | Reserved |
| 7-0 | DEBOUNCETIME | R/W | 0h | Debouncing time value (0-255).<br>The debouncing time is calculated as (DEBOUNCETIME + 1) × 31 µs.<br>Example: DEBOUNCETIME = 0 → 31 µs<br>Example: DEBOUNCETIME = 255 → 7.936 ms |

---

#### 25.4.1.25 GPIO_CLEARDATAOUT Register (offset = 190h) [reset = 0h]

**Description:**
The GPIO_CLEARDATAOUT register is used for clearing specific bits in the GPIO_DATAOUT register. This register implements the alternate access protocol (Set & Clear protocol) to clear bits in the data output register. Writing a 1 to a bit clears the corresponding bit in GPIO_DATAOUT. Writing a 0 has no effect.

**Register Diagram:**
```
[31:0]  INTLINE[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE[n] | R/W | 0h | Clear Data Output<br>0h (W) = No effect on GPIO_DATAOUT register.<br>1h (W) = Clear corresponding bit in GPIO_DATAOUT register.<br>Read returns the value of GPIO_DATAOUT register. |

---

#### 25.4.1.26 GPIO_SETDATAOUT Register (offset = 194h) [reset = 0h]

**Description:**
The GPIO_SETDATAOUT register is used for setting specific bits in the GPIO_DATAOUT register. This register implements the alternate access protocol (Set & Clear protocol) to set bits in the data output register. Writing a 1 to a bit sets the corresponding bit in GPIO_DATAOUT. Writing a 0 has no effect.

**Register Diagram:**
```
[31:0]  INTLINE[n]
```

**Field Descriptions:**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | INTLINE[n] | R/W | 0h | Set Data Output<br>0h (W) = No effect on GPIO_DATAOUT register.<br>1h (W) = Set corresponding bit in GPIO_DATAOUT register.<br>Read returns the value of GPIO_DATAOUT register. |

---

## Tóm tắt các điểm quan trọng

### Cấu hình GPIO
- 4 module GPIO, mỗi module 32 pin → Tổng 128 GPIO pins
- GPIO0 hỗ trợ wake-up, nằm trong Wakeup domain
- GPIO1-3 nằm trong Peripheral domain

### Các chế độ hoạt động
- **Active Mode**: Module hoạt động bình thường, có thể tạo interrupt
- **Idle Mode**: Chờ wake-up event, có thể tắt interface clock
- **Inactive Mode**: Không hoạt động, không có interrupt/wake-up
- **Disabled Mode**: Clock bị gate, không có hoạt động nào

### Tính năng chính
- ✅ Input/Output data
- ✅ Keyboard interface với debounce
- ✅ Interrupt generation (2 interrupt lines cho bi-processor)
- ✅ Wake-up generation (chỉ GPIO0)
- ✅ Debouncing với công thức: `(DEBOUNCETIME + 1) × 31 µs`
- ✅ Set/Clear protocol cho concurrent access

### Clock
- **Interface clock**: OCP bus clock cho toàn bộ module
- **Debouncing clock**: 32 kHz clock cho debounce logic

### Registers quan trọng

#### Configuration Registers
- `GPIO_REVISION` (0h): Revision information (read-only)
- `GPIO_SYSCONFIG` (10h): System configuration (IDLEMODE, ENAWAKEUP, SOFTRESET, AUTOIDLE)
- `GPIO_SYSSTATUS` (114h): System status (RESETDONE)
- `GPIO_CTRL` (130h): Clock gating control (GATINGRATIO, DISABLEMODULE)

#### Data Registers
- `GPIO_OE` (134h): Output Enable (0=output, 1=input) - Reset: FFFFFFFFh
- `GPIO_DATAIN` (138h): Read input data (read-only)
- `GPIO_DATAOUT` (13Ch): Write output data
- `GPIO_SETDATAOUT` (194h): Set specific bits (alternate access)
- `GPIO_CLEARDATAOUT` (190h): Clear specific bits (alternate access)

#### Interrupt Registers
- `GPIO_IRQSTATUS_RAW_0/1` (24h/28h): Raw interrupt status (all events)
- `GPIO_IRQSTATUS_0/1` (2Ch/30h): Interrupt status (enabled events only)
- `GPIO_IRQSTATUS_SET_0/1` (34h/38h): Enable interrupt
- `GPIO_IRQSTATUS_CLR_0/1` (3Ch/40h): Disable interrupt
- `GPIO_IRQWAKEN_0/1` (44h/48h): Wake-up enable

#### Detection Registers
- `GPIO_LEVELDETECT0` (140h): Low-level (0) detection enable
- `GPIO_LEVELDETECT1` (144h): High-level (1) detection enable
- `GPIO_RISINGDETECT` (148h): Rising edge detection enable
- `GPIO_FALLINGDETECT` (14Ch): Falling edge detection enable
- `GPIO_DEBOUNCENABLE` (150h): Debounce enable per pin
- `GPIO_DEBOUNCINGTIME` (154h): Debounce time (0-255, global for module)

#### DMA Support
- `GPIO_EOI` (20h): End of Interrupt for DMA event acknowledgment

---

## GPIO Module Base Addresses

| Module | Base Address | Domain | Wake-up Support |
|--------|-------------|--------|-----------------|
| GPIO0 | 0x44E07000 | Wakeup | Yes |
| GPIO1 | 0x4804C000 | Peripheral | No |
| GPIO2 | 0x481AC000 | Peripheral | No |
| GPIO3 | 0x481AE000 | Peripheral | No |

---

## Important Notes

1. **Reset State**: All GPIO pins default to INPUT mode (GPIO_OE = FFFFFFFFh)

2. **Interrupt Lines**: Each GPIO module has 2 interrupt lines (INTLINE_0 and INTLINE_1) for bi-processor operation

3. **Wake-up**: Only GPIO0 supports wake-up functionality

4. **Debounce Clock**: 32 kHz clock → Each tick = 31 µs

5. **Data Latency**: 
   - With AUTOIDLE=0: GPIO_DATAIN read has 2 OCP cycle latency
   - With AUTOIDLE=1: GPIO_DATAIN read has 3 OCP cycle latency

6. **Set/Clear Protocol**: Use GPIO_SETDATAOUT and GPIO_CLEARDATAOUT for atomic bit operations without read-modify-write

7. **Interrupt Clearing**: Write 1 to clear interrupt status (W1C - Write 1 to Clear)

8. **Module Disable**: Set DISABLEMODULE bit in GPIO_CTRL to gate all clocks when module not needed

9. **Smart-Idle**: GPIO can automatically enter idle mode when no activity, but only if no pending interrupts

10. **Concurrent Access**: Multiple processors can safely access same GPIO module using Set/Clear protocol
