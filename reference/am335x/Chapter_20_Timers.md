# 20 Timers

## Chapter 20: DMTimer Reference

---

## 20.1 Introduction

### 20.1.1 Overview

The DMTimer (Dual-Mode Timer) module contains a free-running upward counter with auto-reload capability on overflow. The timer counter can be read and written in real-time while counting. The module includes compare logic to allow interrupt events on programmable counter matching values.

**Key Capabilities:**
- Free-running upward counter with auto-reload
- Real-time read/write of counter value
- Compare logic for programmable match interrupts
- Dedicated output signal for PWM or timing stamps
- Dedicated input signal for automatic counter capture
- Programmable clock divider (prescaler)
- Write-posted mode for improved performance
- OCP peripheral bus compatible

### 20.1.2 Features

The timer consists of the following features:

- **Counter timer with capture and compare modes**
- **Auto-reload mode**
- **Start-stop mode**
- **Programmable divider clock source**
- **16-32 bit addressing**
- **"On the fly" read/write registers**
- **Interrupts generated on:**
  - Overflow
  - Compare (match)
  - Capture
- **Interrupt enable**
- **Wake-up enable** (Timer0 only)
- **Write posted mode** (reduced access latency)
- **Dedicated I/O:**
  - Input trigger for capture mode (PIEVENTCAPT)
  - Output trigger/PWM signal (PORTIMERPWM)
  - General purpose output (PORGPOCFG)
- **OCP interface compatible**

### 20.1.3 Timer Resolution

Timer resolution and interrupt period depend on selected input clock and prescaler value.

| Clock | Prescaler | Resolution | Interrupt Period Range |
|-------|-----------|------------|----------------------|
| 32.768 kHz | 1 (min) | 31.25 µs | 31.25 µs to ~36h 35m |
| 32.768 kHz | 256 (max) | 8 ms | 8 ms to ~391d 22h 48m |
| 25 MHz | 1 (min) | 40 ns | 40 ns to ~171.8s |
| 25 MHz | 256 (max) | 10.24 µs | ~20.5 µs to ~24h 32m |

---

## 20.2 Functional Modes

The DMTimer supports three functional modes:

1. **Timer Mode** - Free-running counter with overflow
2. **Capture Mode** - Capture counter value on external event
3. **Compare Mode** - Generate interrupt/output on counter match

By default after reset, capture and compare modes are disabled.

### 20.2.1 Timer Mode

The timer is an upward counter that can be started and stopped at any time through the Timer Control Register (TCLR ST bit).

**Operation:**
- Counter can be loaded when stopped or on-the-fly (while counting)
- Counter loading methods:
  - Direct write to TCRR (Timer Counter Register)
  - Trigger write to TTGR loads TLDR value into TCRR
- Counter value can be read at any time
- Counter stopped and cleared to 0 on module reset
- Timer remains stopped after reset release

**One-Shot Mode (TCLR AR bit = 0):**
- Counter stops after overflow
- Counter value remains at zero

**Auto-Reload Mode (TCLR AR bit = 1):**
- TCRR reloaded with TLDR value after overflow
- Continuous counting

**Overflow Handling:**
- Interrupt can be issued if IRQENABLE_SET OVF_IT_FLAG = 1
- Output pin PORTIMERPWM can generate:
  - One positive pulse (prescaler duration)
  - Toggle current value

**Note:** Do not put overflow value (0xFFFF_FFFF) in TLDR as it can lead to undesired results.

### 20.2.2 Capture Mode

Timer value in TCRR can be captured and saved in TCAR1 or TCAR2 when a transition is detected on input pin (PIEVENTCAPT).

**Transition Detection:**
- Rising edge
- Falling edge  
- Both edges
- Selected via TCLR TCM bit

**Capture Modes:**

#### Single Capture (TCLR CAPT_MODE = 0)
- First enabled capture event → counter value saved in TCAR1
- All subsequent events ignored until:
  - Detection logic reset, OR
  - Interrupt status register cleared (write 1 to TCAR_IT_FLAG position)

#### Dual Capture (TCLR CAPT_MODE = 1)
- First enabled capture event → counter value saved in TCAR1
- Second enabled capture event → counter value saved in TCAR2
- All subsequent events ignored until interrupt serviced
- Useful for period calculation of external clock

**Capture Operation:**
- Edge detection logic reset when capture interrupt serviced
- TCAR_IT_FLAG bit cleared in IRQSTATUS
- Timer functional clock samples input pin
- Pulses must exceed functional clock period for detection
- Interrupt issued if IRQENABLE_SET TCAR_IT_FLAG = 1

### 20.2.3 Compare Mode

Timer value (TCRR) is continuously compared to Timer Match Register (TMAR) value.

**Operation:**
- Enable by setting TCLR CE bit = 1
- TMAR value can be loaded anytime (counting or stopped)
- When TCRR matches TMAR:
  - Interrupt issued if IRQENABLE_SET MAT_EN_FLAG = 1
  - Output pin PORTIMERPWM can toggle or pulse

**Implementation:**
1. Write compare value to TMAR
2. Set TCLR CE bit
3. Avoid setting CE before writing TMAR (prevents unwanted interrupts from reset value matching)

**Output Signal:**
- PORTIMERPWM programmed via TCLR TRG and PT bits
- Can generate:
  - One positive pulse (TIMER clock duration)
  - Toggle mode (invert current value)
- Triggers on overflow and match events

### 20.2.4 Prescaler Functionality

Prescaler divides timer counter input clock frequency.

**Configuration:**
- Enable: TCLR bit 5 (PRE)
- Division ratio: 2^n configured in TCLR PTV field
- Prescaler counter reset when:
  - Timer counter stopped
  - Timer counter reloaded on-the-fly

**Prescaler Behavior:**

| Event | Prescaler Counter | Timer Counter |
|-------|------------------|---------------|
| Overflow (Auto-reload ON) | Reset | TLDR |
| TCRR Write | Reset | TCRR |
| TTGR Write | Reset | TLDR |
| Stop | Reset | Frozen |

### 20.2.5 Pulse-Width Modulation (PWM)

Timer can provide programmable PWM output on PORTIMERPWM pin.

**PWM Configuration:**
- TCLR TRG bits determine toggle event:
  - Overflow only
  - Match only
  - Both overflow and match
- TCLR SCPWM bit sets/clears output when:
  - Counter stopped, OR
  - Triggering is off
- Allows deterministic output state when modulation stopped

**PWM Operation:**
- Overflow pulse generated each (0xFFFF_FFFF - TLDR + 1) count
- Match pulse generated when TCRR reaches TMAR value
- Output toggles based on TCLR TRG and PT bits
- Modulation stops synchronously when TRG cleared and overflow occurs

**Note:** In overflow and match mode, match event ignored until first overflow occurs after mode setup.

---

## 20.3 Integration and Hardware Architecture

### 20.3.1 Timer Connectivity Attributes

**Timer0 Connectivity:**

| Attribute | Value |
|-----------|-------|
| Power Domain | Wakeup domain |
| Interface Clock Domain | PD_WKUP_L4_WKUP_GCLK |
| Functional Clock Domain | PD_WKUP_TIMER0_GCLK |
| Reset Signals | WKUP_DOM_RST_N |
| Idle/Wakeup Signals | Idle, Slave Wakeup |
| Interrupt Requests | 1 to MPU Subsystem (TINT0), 1 to WakeM3 |
| DMA Requests | None |
| Physical Address | L4 Wakeup slave port |

**Timer2-7 Connectivity:**

| Attribute | Value |
|-----------|-------|
| Power Domain | Peripheral Domain |
| Interface Clock Domain | PD_PER_L4LS_GCLK (OCP) |
| Functional Clock Domains | PD_PER_TIMER2_GCLK ... PD_PER_TIMER7_GCLK (one per timer) |
| Reset Signals | PER_DOM_RST_N |
| Idle/Wakeup Signals | Smart Idle (no wakeup capabilities) |
| Interrupt Requests | 1 per timer to MPU Subsystem (TINT2-TINT7); Timer4-7 also routed to TSC_ADC event capture mux |
| DMA Requests | Interrupt requests redirected as DMA requests (1 per instance TINTx) |
| Physical Address | L4 Peripheral slave port |

### 20.3.2 Clock Sources

**Timer0:** Functional clock is fixed to internal 32 kHz RC clock (CLK_RC32K).

**Timer2-7:** Functional clock selected via PRCM CLKSEL_TIMERx_CLK register from three sources:
- System clock (CLK_M_OSC)
- PER PLL generated 32.768 kHz clock (CLK_32KHZ)
- External timer input clock (TCLKIN)

### 20.3.3 Clock Signals

| Clock Signal | Max Freq | Source | Notes |
|-------------|---------|--------|-------|
| PICLKOCP (Timer0 interface) | 100 MHz | CORE_CLKOUTM4/2 | pd_wkup_l4_wkup_gclk from PRCM |
| PICLKTIMER (Timer0 functional) | 26 MHz | CLK_RC32K | pd_wkup_timer0_gclk from PRCM |
| PICLKOCP (Timer2-7 interface) | 100 MHz | CORE_CLKOUTM4/2 | pd_per_l4ls_gclk from PRCM |
| PICLKTIMER (Timer2-7 functional) | 26 MHz | CLK_M_OSC, CLK_32KHZ, or TCLKIN | pd_per_timerX_gclk from PRCM |

**Note:** PICLKTIMER must be less than or equal to PICLKOCP/4.

### 20.3.4 Pin List

| Pin | Type | Description |
|-----|------|-------------|
| TCLKIN | Input | External timer clock source |
| TIMER4 | I/O | Timer 4 trigger input or PWM output |
| TIMER5 | I/O | Timer 5 trigger input or PWM output |
| TIMER6 | I/O | Timer 6 trigger input or PWM output |
| TIMER7 | I/O | Timer 7 trigger input or PWM output |

The PIEVENTCAPT input and PORTIMERPWM output signals are muxed onto a single TIMER I/O pad. Pin direction is controlled by PORGPOCFG signal (GPO_CFG bit 14 of TCLR).

---

## 20.4 Clock and Reset Management

### 20.4.1 Clock Domains

DMTimer operates across two clock domains:
- **OCP clock domain** - Interface clock for register access
- **Timer clock domain** - Functional clock for counter operation

**Synchronization:**
- Special logic handles resynchronization between domains
- At reset: full resynchronization path used (works with any clock ratio)
- Performance impact: access latency in OCP clock cycles

### 20.4.2 Write-Posted Mode

Improves access latency under restricted clock ratio conditions.

**Operation:**
- Enable: Set POSTED bit in TSICR (Timer Synchronous Interface Control Register)
- OCP write command granted before write completes in timer clock domain
- Allows concurrent writes to timer registers
- Software monitors completion via TWPS (Timer Write Posting Status Register)

**Benefits:**
- Reduced access latency
- Concurrent register writes possible
- Software-visible synchronization status

**Requirements:**
- Restricted clock ratio conditions must be met
- Check TWPS bits to ensure write completion before dependent operations

---

## 20.5 Additional Functional Details

### 20.5.1 Timer Counting Rate

The timer counter consists of a prescaler stage and a timer counter stage. The prescaler is clocked by the timer clock and acts as a clock divider for the timer counter stage.

**Prescaler Clock Ratios (Table 20-7):**

| PRE | PTV | Divisor (PS) |
|-----|-----|-------------|
| 0 | X | 1 (prescaler disabled) |
| 1 | 0 | 2 |
| 1 | 1 | 4 |
| 1 | 2 | 8 |
| 1 | 3 | 16 |
| 1 | 4 | 32 |
| 1 | 5 | 64 |
| 1 | 6 | 128 |
| 1 | 7 | 256 |

**Timer Rate Equation:**

```
Interrupt Period = (0xFFFFFFFF - TLDR + 1) x Timer_Clock_Period x PS
```

Where: Timer_Clock_Period = 1 / Timer_Clock_Frequency, PS = 2^(PTV+1) when PRE=1.

**Example with 32 kHz clock, PRE=0:**

| TLDR | Interrupt Period |
|------|-----------------|
| 0x00000000 | ~37 hours |
| 0xFFFF0000 | 2 seconds |
| 0xFFFFFFF0 | 500 µs |
| 0xFFFFFFFE | 62.5 µs |

### 20.5.2 Timer Under Emulation (Debug)

To configure the timer to stop during emulation suspend events (e.g., debugger breakpoints):

1. Set `TIOCP_CFG.EMUFREE = 0` — allows the Suspend_Control signal from the Debug Subsystem to stop and start the timer. If EMUFREE=1, the suspend signal is ignored and the timer runs free regardless of debug events.
2. Set the appropriate `xxx_Suspend_Control` register = 0x9 in the Debug Subsystem (Chapter 27) for the desired peripheral.

### 20.5.3 16-bit Register Access Rules

All registers are 32-bit wide but are 16-bit addressable. Important rules for 16-bit access:

- For **write** operations: LSB16 must be written first, then MSB16.
- For OCP-only registers (TIDR, TIOCP_CFG, IRQSTATUS_RAW, IRQSTATUS, IRQENABLE_SET, IRQENABLE_CLR, IRQWAKEEN, TSICR): MSB access may be skipped if upper 16 bits need no update.
- For **functional registers** (TCLR, TCRR, TLDR, TTGR, TMAR): write must be complete — MSB16 must also be written even if not changing upper bits.

For **reading TCRR** in 16-bit mode (atomic read sequence):
1. Read lower 16 bits (offset 0x3C). This freezes/latches the upper 16 bits into a temporary register.
2. Read upper 16 bits (offset 0x3E). Returns the latched value.

Same atomic read sequence applies to **TCAR1** (offset 0x50/0x52) and **TCAR2** (offset 0x58/0x5A).

---

## 20.6 Register Access

### 20.6.1 Programming Timer Registers

Registers can be written when timer is running or stopped.

**Write Access Types:**
- **Posted Mode:** Write granted immediately, completion monitored via TWPS
- **Non-Posted Mode:** Write completes before grant (synchronous)

### 20.6.2 Reading Timer Registers

Registers can be read anytime.

**Read Access:**
- Values sampled from timer clock domain
- Synchronization handled automatically
- Read latency depends on mode (posted/non-posted)

### 20.6.3 OCP Error Generation

Errors generated for:
- Access to reserved register addresses
- Invalid access patterns

### 20.6.4 Write Posted Mode Details

#### Write Posted (TSICR POSTED = 1)

**Timing:**
- Command accepted immediately
- Actual write completes after synchronization
- Status tracked in TWPS register

**TWPS Status Bits:**
- One bit per write-able register
- Bit = 1: Write pending
- Bit = 0: Write completed

**Stall Period:**
- Typically no stall on write
- Software must check TWPS before critical operations

#### Write Non-Posted (TSICR POSTED = 0)

**Timing:**
- Command accepted after write completes
- Guaranteed synchronization before grant
- Stall period: 2 Timer clocks + 2 OCP clocks

### 20.6.5 Read Access Details

#### Read Posted (TSICR POSTED = 1)

**Timing:**
- Command accepted quickly
- Value sampled with minimal latency

#### Read Non-Posted (TSICR POSTED = 0)

**Timing:**
- Stall max: 3 OCP clocks + 2.5 Timer clocks
- Read sample time: 1 OCP clock + 2.5 Timer clocks

---

## 20.7 Timer Registers

### 20.7.1 Register Summary

Base addresses for Timer instances:
- **DMTIMER0:** 0x44E05000
- **DMTIMER1_1MS:** 0x44E31000
- **DMTIMER2:** 0x48040000
- **DMTIMER3:** 0x48042000
- **DMTIMER4:** 0x48044000
- **DMTIMER5:** 0x48046000
- **DMTIMER6:** 0x48048000
- **DMTIMER7:** 0x4804A000

| Offset | Register | Description |
|--------|----------|-------------|
| 0x00 | TIDR | Identification Register |
| 0x10 | TIOCP_CFG | Timer OCP Configuration Register |
| 0x20 | IRQ_EOI | Timer IRQ End-of-Interrupt Register |
| 0x24 | IRQSTATUS_RAW | Timer Status Raw Register |
| 0x28 | IRQSTATUS | Timer Status Register |
| 0x2C | IRQENABLE_SET | Timer Interrupt Enable Set Register |
| 0x30 | IRQENABLE_CLR | Timer Interrupt Enable Clear Register |
| 0x34 | IRQWAKEEN | Timer IRQ Wakeup Enable Register |
| 0x38 | TCLR | Timer Control Register |
| 0x3C | TCRR | Timer Counter Register |
| 0x40 | TLDR | Timer Load Register |
| 0x44 | TTGR | Timer Trigger Register |
| 0x48 | TWPS | Timer Write Posting Bits Register |
| 0x4C | TMAR | Timer Match Register |
| 0x50 | TCAR1 | Timer Capture Register 1 |
| 0x54 | TSICR | Timer Synchronous Interface Control Register |
| 0x58 | TCAR2 | Timer Capture Register 2 |

### 20.7.2 TIDR - Identification Register (Offset 0x00)

Read-only register containing module revision number.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:30] | SCHEME | R | 0x1 | Scheme identifier |
| [29:28] | Reserved | R | 0x0 | - |
| [27:16] | FUNC | R | 0x0 | Function identifier |
| [15:11] | R_RTL | R | 0x0 | RTL revision |
| [10:8] | X_MAJOR | R | 0x1 | Major revision |
| [7:6] | CUSTOM | R | 0x0 | Custom version |
| [5:0] | Y_MINOR | R | 0x0 | Minor revision |

### 20.7.3 TIOCP_CFG - OCP Configuration Register (Offset 0x10)

Controls OCP interface behavior.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:4] | Reserved | R | 0x0 | - |
| [3:2] | IDLEMODE | R/W | 0x0 | Power management:<br>0x0 = Force-idle<br>0x1 = No-idle<br>0x2 = Smart-idle<br>0x3 = Smart-idle wakeup |
| [1] | EMUFREE | R/W | 0x0 | Emulation mode:<br>0 = Timer frozen in debug<br>1 = Timer runs in debug |
| [0] | SOFTRESET | R/W | 0x0 | Software reset:<br>Write 1 to trigger reset<br>Read 0 when reset complete |

### 20.7.4 IRQ_EOI - IRQ End-of-Interrupt (Offset 0x20)

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:1] | Reserved | R | 0x0 | - |
| [0] | DMAEvent_Ack | R/W | 0x0 | Write 0 to acknowledge DMA event has been completed. Module will generate another DMA event only after previous one is acknowledged. Read always returns 0. |

**Note:** This register must be written after DMA completes for subsequent DMA events to be triggered.

### 20.7.5 IRQSTATUS_RAW - Status Raw Register (Offset 0x24)

Raw interrupt status (before masking).

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:3] | Reserved | R | 0x0 | - |
| [2] | TCAR_IT_FLAG | R/W | 0x0 | Capture event:<br>0 = No event<br>1 = Event pending |
| [1] | OVF_IT_FLAG | R/W | 0x0 | Overflow event:<br>0 = No event<br>1 = Event pending |
| [0] | MAT_IT_FLAG | R/W | 0x0 | Match event:<br>0 = No event<br>1 = Event pending |

### 20.7.6 IRQSTATUS - Status Register (Offset 0x28)

Enabled interrupt status (after masking). Write 1 to clear.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:3] | Reserved | R | 0x0 | - |
| [2] | TCAR_IT_FLAG | R/W | 0x0 | Capture interrupt status |
| [1] | OVF_IT_FLAG | R/W | 0x0 | Overflow interrupt status |
| [0] | MAT_IT_FLAG | R/W | 0x0 | Match interrupt status |

**Note:** Status not set unless event enabled. Write 1 to clear (clears raw status).

### 20.7.7 IRQENABLE_SET - Interrupt Enable Set (Offset 0x2C)

Enable interrupts. Write 1 to enable.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:3] | Reserved | R | 0x0 | - |
| [2] | TCAR_EN_FLAG | R/W | 0x0 | Enable capture interrupt |
| [1] | OVF_EN_FLAG | R/W | 0x0 | Enable overflow interrupt |
| [0] | MAT_EN_FLAG | R/W | 0x0 | Enable match interrupt |

### 20.7.8 IRQENABLE_CLR - Interrupt Enable Clear (Offset 0x30)

Disable interrupts. Write 1 to disable.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:3] | Reserved | R | 0x0 | - |
| [2] | TCAR_EN_FLAG | R/W | 0x0 | Disable capture interrupt |
| [1] | OVF_EN_FLAG | R/W | 0x0 | Disable overflow interrupt |
| [0] | MAT_EN_FLAG | R/W | 0x0 | Disable match interrupt |

### 20.7.9 IRQWAKEEN - IRQ Wakeup Enable (Offset 0x34)

Enable wakeup generation. Only applicable to Timer0.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:3] | Reserved | R | 0x0 | - |
| [2] | TCAR_WUP_ENA | R/W | 0x0 | Enable capture wakeup |
| [1] | OVF_WUP_ENA | R/W | 0x0 | Enable overflow wakeup |
| [0] | MAT_WUP_ENA | R/W | 0x0 | Enable match wakeup |

### 20.7.10 TCLR - Timer Control Register (Offset 0x38)

Main control register for timer configuration.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:15] | Reserved | R | 0x0 | - |
| [14] | GPO_CFG | R/W | 0x0 | General purpose output, drives PORGPOCFG pin directly:<br>0 = PORGPOCFG drives 0, configures timer pin as output<br>1 = PORGPOCFG drives 1, configures timer pin as input |
| [13] | CAPT_MODE | R/W | 0x0 | Capture mode:<br>0 = Single capture (TCAR1 only)<br>1 = Capture on second event (dual capture: TCAR1 then TCAR2) |
| [12] | PT | R/W | 0x0 | Pulse or toggle mode on PORTIMERPWM output pin:<br>0 = Pulse<br>1 = Toggle |
| [11:10] | TRG | R/W | 0x0 | Trigger output mode on PORTIMERPWM output pin:<br>0x0 = No trigger<br>0x1 = Trigger on overflow<br>0x2 = Trigger on overflow and match<br>0x3 = Reserved |
| [9:8] | TCM | R/W | 0x0 | Transition Capture Mode on PIEVENTCAPT input pin:<br>0x0 = No capture<br>0x1 = Capture on low-to-high transition (rising edge)<br>0x2 = Capture on high-to-low transition (falling edge)<br>0x3 = Capture on both edge transitions |
| [7] | SCPWM | R/W | 0x0 | Set/clear PORTIMERPWM output (should be set while timer stopped or trigger off):<br>0 = Clear PORTIMERPWM pin, select positive pulse for pulse mode<br>1 = Set PORTIMERPWM pin, select negative pulse for pulse mode |
| [6] | CE | R/W | 0x0 | Compare enable:<br>0 = Compare mode disabled<br>1 = Compare mode enabled |
| [5] | PRE | R/W | 0x0 | Prescaler enable:<br>0 = Timer clock input clocks counter directly<br>1 = Divided input clocks counter |
| [4:2] | PTV | R/W | 0x0 | Pre-scale clock Timer value (divisor = 2^(PTV+1) when PRE=1):<br>0x0 = Divide by 2<br>0x1 = Divide by 4<br>0x2 = Divide by 8<br>0x3 = Divide by 16<br>0x4 = Divide by 32<br>0x5 = Divide by 64<br>0x6 = Divide by 128<br>0x7 = Divide by 256 |
| [1] | AR | R/W | 0x0 | Auto-reload mode:<br>0 = One-shot timer<br>1 = Auto-reload timer |
| [0] | ST | R/W | 0x0 | Start/Stop timer. In one-shot mode (AR=0), automatically reset by hardware on counter overflow.<br>0 = Stop timer (counter frozen)<br>1 = Start timer |

### 20.7.11 TCRR - Timer Counter Register (Offset 0x3C)

Current counter value. Read/write anytime.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:0] | TIMER_COUNTER | R/W | 0x0 | Current counter value |

### 20.7.12 TLDR - Timer Load Register (Offset 0x40)

Value loaded into counter on overflow (auto-reload mode) or trigger.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:0] | LOAD_VALUE | R/W | 0x0 | Timer load value |

**Note:** Do not write 0xFFFF_FFFF to avoid undesired behavior.

### 20.7.13 TTGR - Timer Trigger Register (Offset 0x44)

Write any value to trigger reload of TCRR from TLDR. Read always returns 0xFFFF_FFFF. Reload occurs regardless of AR field value.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:0] | TTGR_VALUE | R/W | 0xFFFFFFFF | Write any value to trigger: TCRR loaded from TLDR, prescaler counter cleared. Reload is done regardless of AR bit in TCLR. |

### 20.7.14 TWPS - Timer Write Posting Status (Offset 0x48)

Write posting status for each register. Read-only.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:5] | Reserved | R | 0x0 | - |
| [4] | W_PEND_TMAR | R | 0x0 | TMAR write pending:<br>0 = Write completed<br>1 = Write pending |
| [3] | W_PEND_TTGR | R | 0x0 | TTGR write pending:<br>0 = Write completed<br>1 = Write pending |
| [2] | W_PEND_TLDR | R | 0x0 | TLDR write pending:<br>0 = Write completed<br>1 = Write pending |
| [1] | W_PEND_TCRR | R | 0x0 | TCRR write pending:<br>0 = Write completed<br>1 = Write pending |
| [0] | W_PEND_TCLR | R | 0x0 | TCLR write pending:<br>0 = Write completed<br>1 = Write pending |

**Usage:** Check appropriate bit = 0 before dependent operation.

### 20.7.15 TMAR - Timer Match Register (Offset 0x4C)

Compare value for match mode.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:0] | COMPARE_VALUE | R/W | 0x0 | Compare value for match detection |

### 20.7.16 TCAR1 - Timer Capture Register 1 (Offset 0x50)

Captured counter value (first capture or single capture mode).

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:0] | CAPTURED_VALUE | R | 0x0 | Captured counter value |

### 20.7.17 TSICR - Synchronous Interface Control (Offset 0x54)

Controls synchronization mode.

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:3] | Reserved | R | 0x0 | - |
| [2] | POSTED | R/W | 0x1 | Posted mode (reset value depends on hardware integration; read to get actual HW config):<br>0 = Posted mode inactive (delays command accept; use when freq(timer) >= freq(OCP)/4)<br>1 = Posted mode active (requires freq(timer) < freq(OCP)/4) |
| [1] | SFT | R/W | 0x0 | Software reset of functional part of module. Read always returns 0.<br>0 = Software reset enabled<br>1 = Software reset disabled |
| [0] | Reserved | R | 0x0 | - |

### 20.7.18 TCAR2 - Timer Capture Register 2 (Offset 0x58)

Captured counter value (second capture in dual capture mode).

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| [31:0] | CAPTURED_VALUE | R | 0x0 | Captured counter value |

---

## 20.8 Programming Sequences

### 20.8.1 Timer Mode - One Shot

```
1. Stop timer: TCLR.ST = 0
2. Configure timer:
   - TCLR.AR = 0 (one-shot mode)
   - TCLR.PRE = 0/1 (prescaler disable/enable)
   - TCLR.PTV = prescaler value
3. Load counter: TCRR = initial value
4. Enable overflow interrupt: IRQENABLE_SET.OVF_EN_FLAG = 1
5. Start timer: TCLR.ST = 1
6. Wait for overflow interrupt
7. Service interrupt: Write 1 to IRQSTATUS.OVF_IT_FLAG
```

### 20.8.2 Timer Mode - Auto Reload

```
1. Stop timer: TCLR.ST = 0
2. Configure timer:
   - TCLR.AR = 1 (auto-reload mode)
   - TCLR.PRE = 0/1 (prescaler disable/enable)
   - TCLR.PTV = prescaler value
3. Load reload value: TLDR = reload value
4. Load counter: TCRR = initial value (or trigger via TTGR)
5. Enable overflow interrupt: IRQENABLE_SET.OVF_EN_FLAG = 1
6. Start timer: TCLR.ST = 1
7. Counter reloads from TLDR on each overflow
```

### 20.8.3 Capture Mode - Single Capture

```
1. Configure capture:
   - TCLR.CAPT_MODE = 0 (single capture)
   - TCLR.TCM = 1/2/3 (rising/falling/both edges)
2. Enable capture interrupt: IRQENABLE_SET.TCAR_EN_FLAG = 1
3. Start timer: TCLR.ST = 1
4. Wait for capture interrupt
5. Read captured value from TCAR1
6. Clear interrupt: Write 1 to IRQSTATUS.TCAR_IT_FLAG
7. Ready for next capture
```

### 20.8.4 Capture Mode - Dual Capture (Period Measurement)

```
1. Configure capture:
   - TCLR.CAPT_MODE = 1 (dual capture)
   - TCLR.TCM = 1 (rising edge for period measurement)
2. Enable capture interrupt: IRQENABLE_SET.TCAR_EN_FLAG = 1
3. Start timer: TCLR.ST = 1
4. Wait for capture interrupt (first edge)
5. Wait for capture interrupt (second edge)
6. Read TCAR1 (first edge timestamp)
7. Read TCAR2 (second edge timestamp)
8. Calculate period: Period = TCAR2 - TCAR1
9. Clear interrupt: Write 1 to IRQSTATUS.TCAR_IT_FLAG
10. Ready for next period measurement
```

### 20.8.5 Compare Mode

```
1. Stop timer: TCLR.ST = 0
2. Write compare value: TMAR = match value
3. Configure timer:
   - TCLR.CE = 1 (compare enable)
   - TCLR.AR = 1 (typically auto-reload)
4. Load reload value: TLDR = reload value
5. Load counter: TCRR = initial value
6. Enable match interrupt: IRQENABLE_SET.MAT_EN_FLAG = 1
7. Start timer: TCLR.ST = 1
8. Wait for match interrupt
9. Service interrupt: Write 1 to IRQSTATUS.MAT_IT_FLAG
```

### 20.8.6 PWM Mode

```
1. Stop timer: TCLR.ST = 0
2. Configure PWM:
   - TCLR.AR = 1 (auto-reload for continuous PWM)
   - TCLR.CE = 1 (compare enable for duty cycle)
   - TCLR.PT = 1 (toggle mode)
   - TCLR.TRG = 2 (trigger on overflow and match)
   - TCLR.SCPWM = 0/1 (default output level)
3. Set period: TLDR = 0xFFFFFFFF - period + 1
4. Set duty cycle: TMAR = 0xFFFFFFFF - duty_cycle + 1
5. Load counter: TCRR = TLDR (or write to TTGR)
6. Start timer: TCLR.ST = 1
7. PWM signal generated on PORTIMERPWM pin
```

**PWM Calculations:**
- Frequency = Timer_Clock / ((0xFFFFFFFF - TLDR + 1) * Prescaler)
- Duty Cycle = (TMAR - TLDR) / (0xFFFFFFFF - TLDR + 1)

### 20.8.7 Using Write-Posted Mode

```
1. Enable posted mode: TSICR.POSTED = 1
2. Write to timer register (e.g., TCRR = value)
3. Check write completion:
   while (TWPS.W_PEND_TCRR) {
       // Wait for write to complete
   }
4. Proceed with dependent operation
```

---

## 20.9 Key Concepts Summary

### 20.9.1 Operating Modes

```
Timer Mode Hierarchy:
├── Timer Mode (Free-running counter)
│   ├── One-Shot (stops on overflow)
│   └── Auto-Reload (reloads from TLDR)
├── Capture Mode (save counter on external event)
│   ├── Single Capture (TCAR1 only)
│   └── Dual Capture (TCAR1 then TCAR2)
└── Compare Mode (interrupt/output on match)
    ├── Match Interrupt
    └── PWM Generation
```

### 20.9.2 Clock Architecture

```
Input Clock → Prescaler (optional) → Timer Counter → Overflow/Match
                ÷2^(PTV+1)             (TCRR)          Events
                
Prescaler Values:
PTV=0: ÷2    PTV=4: ÷32
PTV=1: ÷4    PTV=5: ÷64
PTV=2: ÷8    PTV=6: ÷128
PTV=3: ÷16   PTV=7: ÷256
```

### 20.9.3 Interrupt Sources

```
Timer Interrupts:
├── Overflow (OVF_IT_FLAG)
│   └── Counter reaches 0xFFFFFFFF and rolls over
├── Match (MAT_IT_FLAG)
│   └── Counter (TCRR) equals Match register (TMAR)
└── Capture (TCAR_IT_FLAG)
    └── External event detected on PIEVENTCAPT pin
```

### 20.9.4 Register Dependencies

```
Counter Operation:
TCRR ← TLDR (when TTGR written or overflow with AR=1)
TCRR → TCAR1/TCAR2 (when capture event occurs)
TCRR ≟ TMAR (compare operation when CE=1)

Control Flow:
TCLR.ST → Start/Stop counter
TCLR.AR → Auto-reload behavior
TCLR.CE → Compare enable
TCLR.TCM → Capture edge select
TCLR.TRG → Output trigger mode
```

### 20.9.5 Access Latency

```
Non-Posted Mode:
Write: 2 Timer clocks + 2 OCP clocks
Read:  3 OCP clocks + 2.5 Timer clocks

Posted Mode:
Write: Immediate grant, check TWPS for completion
Read:  Minimal latency

Trade-off:
- Non-Posted: Guaranteed synchronization, higher latency
- Posted: Lower latency, software must check TWPS
```

### 20.9.6 Common Use Cases

| Application | Mode | Configuration |
|-------------|------|---------------|
| Periodic Interrupt | Auto-Reload Timer | AR=1, Enable OVF interrupt |
| Timeout Detection | One-Shot Timer | AR=0, Enable OVF interrupt |
| Event Timestamping | Capture Mode | Single capture, TCM=edge type |
| Frequency Measurement | Dual Capture | CAPT_MODE=1, TCM=1 (rising) |
| PWM Generation | Compare + Auto-Reload | CE=1, AR=1, TRG=2, PT=1 |
| Precise Delay | Compare Mode | CE=1, one-shot or auto-reload |

---

## 20.10 Important Notes

1. **Register Access:** All timer registers can be accessed while timer is running (on-the-fly access)

2. **TLDR Value:** Never write 0xFFFF_FFFF to TLDR as it causes undefined behavior

3. **Compare Mode Setup:** Always write TMAR before setting TCLR.CE to avoid spurious match interrupts

4. **Capture Logic Reset:** Capture detection logic automatically resets when interrupt is serviced (TCAR_IT_FLAG cleared)

5. **Posted Mode:** When using posted mode, always check TWPS before operations dependent on previous writes

6. **Prescaler Reset:** Prescaler counter resets on timer stop, reload, or write to TCRR

7. **PWM Match Event:** In overflow and match mode, first match event after mode setup is ignored until first overflow occurs

8. **Wakeup Capability:** Only Timer0 supports wakeup generation from low power modes

9. **Counter Width:** Timer counter is 32-bit, can count from 0 to 0xFFFF_FFFF

10. **Interrupt Clearing:** Write 1 to interrupt status bit to clear (writing 0 has no effect)
