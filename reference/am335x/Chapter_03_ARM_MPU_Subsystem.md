# 3 ARM MPU Subsystem

**Document:** SPRUH73Q - October 2011 - Revised December 2019  
**Copyright:** Texas Instruments Incorporated  
**Chapter:** 3 - ARM MPU Subsystem

---

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [Subsystem Integration](#subsystem-integration)
4. [Clock and Reset Distribution](#clock-and-reset-distribution)
5. [ARM Cortex-A8 Core](#arm-cortex-a8-core)
6. [Interrupt Controller](#interrupt-controller)
7. [Power Management](#power-management)
8. [Programming Model](#programming-model)

---

## Overview

The Microprocessor Unit (MPU) subsystem handles transactions between the ARM core (ARM Cortex-A8 Processor), the L3 interconnect, and the interrupt controller (INTC). The MPU subsystem is a hard macro that integrates the ARM Cortex-A8 Processor with additional logic for protocol conversion, emulation, interrupt handling, and debug enhancements.

### Architecture Highlights

- **Processor:** ARM Cortex-A8 (ARMv7 compatible)
- **Execution:** Dual-issue, in-order execution engine
- **Caches:** Integrated L1 and L2 caches
- **SIMD:** NEON SIMD Media Processing Unit
- **Debug:** CoreSight compliant logic for debug and emulation
- **Clock Domains:** Three functional clock domains with asynchronous bridges

### Block Diagram Components

```
┌─────────────────────────────────────────────────────────┐
│                    MPU Subsystem                        │
│  ┌────────────────────────────────────────────────┐    │
│  │          ARM Cortex-A8                         │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐    │    │
│  │  │ Integer  │  │  NEON    │  │ L1 I     │    │    │
│  │  │ Core     │  │  Core    │  │ 32KB     │    │    │
│  │  └──────────┘  └──────────┘  │ w/SED    │    │    │
│  │                               └──────────┘    │    │
│  │  ┌──────────┐  ┌──────────────────────────┐  │    │
│  │  │ L1 D     │  │ L2 256KB w/ECC           │  │    │
│  │  │ 32KB     │  │                          │  │    │
│  │  │ w/SED    │  └──────────────────────────┘  │    │
│  │  └──────────┘                                 │    │
│  └────────────────────────────────────────────────┘    │
│                          │                              │
│                    ┌─────▼──────┐                       │
│                    │  AXI2OCP   │ 128/64 bit            │
│                    └─────┬──────┘                       │
│                          │                              │
│         ┌────────────────┼────────────────┐            │
│         │                │                │            │
│    ┌────▼────┐     ┌────▼────┐     ┌────▼────┐        │
│    │ AINTC   │     │ I2ASYNC │     │ OCM RAM │        │
│    │ 275 MHz │     │ 550 MHz │     │ 64KB    │        │
│    └─────────┘     └─────────┘     └─────────┘        │
│                                                         │
│    ┌─────────────┐         ┌──────────────┐           │
│    │ ICECrusher  │         │ ROM 176KB    │           │
│    │ Debug       │         │              │           │
│    └─────────────┘         └──────────────┘           │
└─────────────────────────────────────────────────────────┘
         │                                        │
    System Interrupts                        To L3 Interconnect
```

---

## Features

### ARM Cortex-A8 Microprocessor

**Architecture:**
- ARM Architecture version 7 ISA
- 2-issue, in-order execution pipeline
- Backward compatible with previous ARM ISA versions

**Instruction Sets:**
- Standard ARM instruction set
- Thumb-2 technology for code density
- Jazelle-X Java accelerator
- Media extensions (NEON)

**L1 Cache:**
- Instruction and Data Cache: 32KB each
- 4-way set associative
- 16-word line length
- 128-bit interface
- SED (Single Error Detection) support

**L2 Cache:**
- Size: 256KB
- 8-way set associative
- 16-word line length
- 128-bit interface to L1
- ECC/Parity support
- Hardware or software clearing of valid bits

**Memory Management:**
- Enhanced MMU with TLB
- Fully associative separate ITLB: 32 entries
- Fully associative separate DTLB: 32 entries
- Mapping sizes: 4KB, 64KB, 1MB, 16MB
- Extended physical address ranges

**Coprocessors:**

| Coprocessor | Description |
|-------------|-------------|
| NEON | Media coprocessor implementing Advanced SIMD architecture |
| VFP | VFPv3 architecture, IEEE 754 compliant floating-point |

**Branch Prediction:**
- Branch Target Address Cache: 512 entries

**External Interface:**
- AXI protocol
- 128-bit data width

**Debug Support:**
- Embedded Trace Macrocell (ETM) for non-invasive debugging
- ARMv7 debug with watchpoint and breakpoint registers
- 32-bit Advanced Peripheral Bus (APB) slave interface
- CoreSight debug systems compatible

**On-chip Memory:**
- ROM: 176 KB
- RAM (OCM): 64 KB

### AXI2OCP Bridge

- OCP 2.2 compliant
- Single Request Multiple Data Protocol
- Two ports support
- Multiple targets with three OCP ports:
  - 128-bit
  - 64-bit
  - 32-bit

### Interrupt Controller

- Up to 128 interrupt requests
- Individual priority per interrupt
- Programmable interrupt type (nIRQ or nFIQ)
- Independent priority sorting for nFIQ and nIRQ
- Level-sensitive inputs

### Emulation/Debug

- CoreSight Architecture compatible
- JTAG-based debug via DAP
- Trace support via TPIU
- 32KB ETB (Embedded Trace Buffer) at chip level

### Clock Generation

- Managed through PRCM (Power, Reset, and Clock Management)
- Embedded DPLL for ARM clock source
- Clock divider for internal modules

---

## Subsystem Integration

### Integrated Submodules

**ARM Cortex-A8 Processor:**
- High processing capability
- NEON technology for mobile multimedia acceleration
- AXI bus communication with AXI2OCP bridge
- Interrupt reception from MPU INTC

**Interrupt Controller (AINTC):**
- Handles module interrupts
- See Chapter 6 for detailed interrupt handling

**AXI2OCP Bridge:**
- Communication between ARM (AXI) and modules (OCP L3)
- Interface to INTC (OCP)

**I2Async Bridge:**
- Asynchronous OCP to OCP interface
- Between AXI2OCP bridge (MPU subsystem) and T2Async bridge (external)

**Clock Generator:**
- Provides divided clocks to internal modules
- Input: MPU_CLK from PRCM

**In-Circuit Emulator (ICECrusher):**
- CoreSight Architecture compliant
- Debugging capabilities

### Signal Interface

```
┌─────────────────────────────────────────────────────┐
│                   MPU Subsystem                     │
│                                                     │
│  PRCM Inputs:                                       │
│  - MPU_CLK           (Clock input)                  │
│  - MPU_RST           (MPU reset)                    │
│  - CORE_RST          (Core/INTC reset)              │
│  - NEON_RST          (NEON reset)                   │
│  - EMU_RST           (Emulation reset)              │
│                                                     │
│  System Inputs:                                     │
│  - sys_nirq          (System interrupts)            │
│                                                     │
│  Outputs:                                           │
│  - MPU_MSTANDBY      (Standby status)               │
│  - To L3 Interconnect (OCP Master 0/1)              │
│                                                     │
└─────────────────────────────────────────────────────┘
```

---

## Clock and Reset Distribution

### Clock Distribution

The MPU subsystem includes an embedded DPLL which sources the clock for the ARM Cortex-A8 processor. A clock divider derives clocks for other internal modules.

**Clock Architecture:**

```
MPU_DPLL → MPU_CLK → Clock Generator
                           │
                           ├── ARM_FCLK (ARM core)
                           ├── AXI2OCP_FCLK (ARM_FCLK / 2)
                           ├── INTC_FCLK (ARM_FCLK / 2)
                           ├── ICECRUSHER_FCLK (ARM_FCLK / 2)
                           ├── I2ASYNC_FCLK (ARM_FCLK / 2)
                           └── EMU_CLOCKS (async, max ARM_FCLK / 3)
```

**Clock Frequencies:**

| Clock Signal | Frequency | Purpose |
|--------------|-----------|---------|
| ARM_FCLK | MPU_CLK | ARM core, NEON, L2 cache, ETM, internal RAMs |
| AXI2OCP_FCLK | MPU_CLK / 2 | OCP interface bridge |
| MPU_INTC_FCLK | MPU_CLK / 2 | Interrupt controller |
| ICECRUSHER_FCLK | MPU_CLK / 2 | Debug APB interface |
| I2ASYNC_FCLK | MPU_CLK / 2 | Asynchronous bridge (MPU side) |
| T2ASYNC | Core Clock | Asynchronous bridge (device side, from PRCM) |
| EMU_CLOCKS | Max ARM_FCLK / 3 | Emulation (async to ARM core) |

**Clock Divider Configuration:**

The output clock divider can be programmed with:
- Register: `CM_DIV_M2_DPLL_MPU.DPLL_CLKOUT_DIV`
- Frequency relative to ARM core
- See Chapter 8 (PRCM) for details

### Reset Distribution

All resets provided by PRCM and controlled by the clock generator module.

**Reset Signals:**

| Signal | Source | Target |
|--------|--------|--------|
| MPU_RSTPWRON | PRCM | MPU power-on reset |
| MPU_RST | PRCM | MPU subsystem modules |
| CORE_RST | PRCM | INTC, ARM core |
| NEON_RST | PRCM | NEON coprocessor |
| EMU_RSTPWRON | PRCM | Emulation power-on reset |
| EMU_RST | PRCM | Emulation modules |

**Reset Scheme:**

```
PRCM
 │
 ├── MPU_RSTPWRON ──→ Clock Generator
 ├── MPU_RST ────────→ AXI2OCP, I2Async
 ├── CORE_RST ───────→ ARM Core, INTC
 ├── NEON_RST ───────→ NEON
 ├── EMU_RSTPWRON ───→ Emulation
 └── EMU_RST ────────→ ICECrusher, ETM
```

For complete details about clocks, resets, and power domains, see Chapter 8 (PRCM).

---

## ARM Cortex-A8 Core

### Overview

The ARM Cortex-A8 processor incorporates technologies from the ARMv7 architecture:

- **NEON:** Media and signal processing
- **Jazelle RCT:** Acceleration of real-time compilers
- **Thumb-2:** Code density
- **VFPv3:** Floating-point architecture

### AXI Bus Interface

The AXI bus interface is the main interface to the ARM system bus:

- Performs L2 cache fills
- Handles non-cacheable accesses for instructions and data
- 128-bit and 64-bit wide input/output data buses
- Multiple outstanding requests support
- Wide range of bus clock to core clock ratios
- Synchronous bus clock with core clock

**Transaction Rules:**

Memory region attributes determine:
- Ordering
- Posting
- Pipeline synchronization

See ARM Cortex-A8 Technical Reference Manual for complete programming model.

### Secure Monitor Calls

The device supports special secure monitor functions for accessing ARM core registers in privileged mode.

**Available Service IDs:**

| Service ID (R12) | Function |
|------------------|----------|
| 0x100 | Write value in R0 to Auxiliary Control Register |
| 0x101 | Write value in R0 to Non-Secure Access Control Register |
| 0x102 | Write value in R0 to L2 Cache Auxiliary Control Register |

**Procedure:**

1. Write appropriate service ID to R12
2. Load R0 with value to write
3. Perform barrier operations
4. Use `SMC #1` (or `SMI #1`) for secure monitor call

**Example: Enabling ECC on L2 Cache**

```assembly
_enableL2ECC:
    STMFD sp!, {r0 - r4}          ; Save context of R0-R4
    MRC p15, #1, r0, c9, c0, #2   ; Read L2 Cache Aux Control Reg into R0
    MOV r1, #0                     ; Clear R1
    MOVT r1, #0x1020               ; Enable mask for ECC (bits 21 and 28)
    ORR r0, r0, r1                 ; OR with original register value
    MOV r12, #0x0102               ; Setup service ID in R12
    MCR p15,#0x0,r1,c7,c5,#6      ; Invalidate entire branch predictor array
    DSB                            ; Data synchronization barrier
    ISB                            ; Instruction synchronization barrier
    DMB                            ; Data memory barrier
    SMC #1                         ; Secure monitor call
    LDMFD sp!, {r0 - r4}          ; Restore R0-R4
    MOV pc, lr                     ; Return
```

**Important:** This function must be executed in ARM privileged mode.

### Supported Features Summary

| Feature | Description |
|---------|-------------|
| ARM ISA Version | ARMv7 with Thumb-2, Jazelle-X, Media extensions |
| Backward Compatibility | Previous ARM ISA versions supported |
| L1 I-cache | 32KB, 4-way, 16-word line, 128-bit interface |
| L1 D-cache | 32KB, 4-way, 16-word line, 128-bit interface |
| L2 Cache | 256KB, 8-way, 16-word line, 128-bit to L1, ECC/Parity |
| L2 Valid Bits | Software loop or hardware clearing |
| TLB | Fully associative, separate ITLB (32 entries) and DTLB (32 entries) |
| CoreSight ETM | Embedded with ARM core, 32KB ETB at chip level |
| Branch Target Cache | 512 entries |
| MMU | Enhanced with 4KB, 64KB, 1MB, 16MB mapping sizes |
| NEON | Enhanced throughput for media workloads, VFP-Lite support |
| ROM | 176KB flat memory |
| RAM | 64KB flat memory |
| Internal Bus | 128-bit AXI from Cortex-A8 via AXI2OCP bridge |
| OCP Ports | 128-bit and 64-bit asynchronous |
| Low Interrupt Latency | Closely coupled INTC with 128 interrupt lines |
| Vectored Interrupt | Controller port present |
| Debug | JTAG-based via DAP |
| Trace | Supported via TPIU |
| External Coprocessor | Not supported |

---

## Interrupt Controller

### Overview

The Host ARM Interrupt Controller (AINTC) prioritizes service requests from system peripherals and generates nIRQ or nFIQ to the ARM processor.

**Interface:**
- Connects to ARM via AXI port through AXI2OCP bridge
- Runs at half the processor speed
- Handles up to 128 interrupt requests

### Features

- **Inputs:** Up to 128 level-sensitive interrupt inputs
- **Priority:** Individual priority for each interrupt input
- **Steering:** Each interrupt can be steered to nFIQ or nIRQ
- **Sorting:** Independent priority sorting for nFIQ and nIRQ

### Configuration

All interrupt configuration is programmable:
- Interrupt type (nIRQ or nFIQ)
- Priority levels
- Enable/disable per interrupt

For detailed interrupt handling, see Chapter 6 (Interrupts).

---

## Power Management

### Power Domains

The MPU subsystem is divided into four power domains controlled by PRCM:

```
┌─────────────────────────────────────────────────┐
│          MPU Subsystem Power Domains            │
│                                                 │
│  ┌───────────────────────────────────────┐     │
│  │  MPU Domain                            │     │
│  │  - ARM Core                            │     │
│  │  - AXI2OCP                             │     │
│  │  - I2Async Bridge                      │     │
│  │  - ARM L1 and L2 periphery logic       │     │
│  │  - ICE-Crusher                         │     │
│  │  - ETM, APB modules                    │     │
│  └───────────────────────────────────────┘     │
│                                                 │
│  ┌───────────────────────────────────────┐     │
│  │  NEON Domain                           │     │
│  │  - ARM NEON accelerator                │     │
│  └───────────────────────────────────────┘     │
│                                                 │
│  ┌───────────────────────────────────────┐     │
│  │  CORE Domain                           │     │
│  │  - MPU Interrupt Controller (INTC)     │     │
│  └───────────────────────────────────────┘     │
│                                                 │
│  ┌───────────────────────────────────────┐     │
│  │  EMU Domain                            │     │
│  │  - ETB (Embedded Trace Buffer)         │     │
│  │  - DAP (Debug Access Port)             │     │
│  └───────────────────────────────────────┘     │
└─────────────────────────────────────────────────┘
```

**Important Notes:**

- Emulation and core domains are not fully embedded in MPU subsystem
- L1 and L2 array memories have separate control signals controlled by PRCM
- For physical power domains and voltage domains, see Chapter 8 (PRCM)

### Power States

Each power domain can be in one of three states:

| State | Logic Power | Memory Power | Clocks |
|-------|-------------|--------------|--------|
| Active | On | On or Off | On (at least one clock) |
| Inactive | On | On or Off | Off |
| Off | Off | Off | Off (all clocks) |

### Power Modes

The MPU subsystem supports multiple operational power modes. The PRCM manages all transitions by controlling:
- Domain clocks
- Domain resets
- Domain logic power switches
- Memory power switches

**Standby Status:**

Check MPU standby status with: `CM_MPU_MPU_CLKCTRL.STBYST`

**Dependencies:**

- MPU must be on when core power is on
- Device power management prevents INTC OFF state when MPU domain is on
- NEON has independent power-off mode when not in use

**L1 Cache Retention:**

CAUTION: The MPU L1 cache memory does not support retention mode. Its array switch is controlled together with MPU logic. L1 retention control signals exist at PRCM boundary but are not used. ARM L2 can be put into retention independently.

### Supported Operation Modes

| Mode | MPU & ARM Logic | ARM L2 RAM | NEON | INTC | Device Core | ETM APB/ATB Debug |
|------|----------------|------------|------|------|-------------|-------------------|
| 1 | Active | Active | Active | Active | Disabled or enabled | |
| 2 | Active | Active | OFF | Active | Disabled or enabled | |
| 3 | Active | RET | Active | Active | Disabled or enabled | |
| 4 | Active | RET | OFF | Active | Disabled or enabled | |
| 5 | Active | OFF | Active | Active | Disabled or enabled | |
| 6 | Active | OFF | OFF | Active | Disabled or enabled | |
| 7 | OFF | RET | OFF | OFF | Disabled or enabled | |
| 8 | Standby | Active | Standby | Active | Disabled or enabled | |
| 9 | Standby | Active | OFF | Active | Disabled or enabled | |
| 10 | Standby | RET | Standby | Active | Disabled or enabled | |
| 11 | Standby | RET | OFF | Active | Disabled or enabled | |
| 12 | Standby | OFF | Standby | Active | Disabled or enabled | |
| 13 | Standby | OFF | OFF | Active | Disabled or enabled | |
| 14 | OFF | OFF | OFF | OFF | Disabled or enabled | |

**Key:**
- RET = Retention
- OFF = Powered off
- Active = Fully operational

**Requirements:**

The MPU subsystem must be in a power mode where:
- MPU power domain is in standby or off
- NEON power domain is in standby or off
- Debug power domain is in standby or off
- INTC power domain is in standby or off

---

## Programming Model

For detailed register descriptions used for MPU configuration, see Chapter 8 (PRCM).

### Clock Control

For clock configuration settings, see Chapter 8 (PRCM).

### Power Mode Transitions

#### Basic Power-On Reset

**Sequence:**

1. Reset INTC (CORE_RST) and MPU subsystem modules (MPU_RST)
2. Clocks must be active during MPU reset and CORE reset

**Applicability:**
- Initial power-up
- Wakeup from device off mode

#### MPU Into Standby Mode

**Sequence:**

1. ARM core initiates standby via software (CP15 - WFI instruction)
2. MPU modules requested internally to enter idle (after ARM standby detected)
3. MPU standby output asserted to PRCM (all outputs guaranteed at reset values)
4. PRCM requests INTC to enter idle mode
5. INTC acknowledges to PRCM

**Notes:**
- INTC SWAKEUP output is a hardware signal to PRCM for IDLE request/acknowledge handshake status
- In debug mode, ICECrusher can prevent MPU subsystem from entering IDLE mode

#### MPU Out of Standby Mode

**Sequence:**

1. PRCM starts clocks through DPLL programming
2. Detect active clocking via DPLL status output
3. Initiate interrupt through INTC to wake up ARM core from STANDBYWFI mode

**Applicability:**
- Exit from standby
- Wakeup from device off mode

#### MPU Power On From Powered-Off State

**Sequence:**

1. **Power-Up Order (minimize current peaking):**
   - MPU Power On
   - NEON Power On
   - Core Power On (INTC)
   - Follow ordered sequence per power switch daisy chain

2. **Reset Sequence:**
   - Core domain must be on and reset before MPU reset
   - Follow Basic Power-On Reset sequence

**Important:** Core domain must be on and reset before the MPU can be reset.

---

## Register Configuration

### Clock Management Registers

**CM_DIV_M2_DPLL_MPU:**
- Field: `DPLL_CLKOUT_DIV`
- Purpose: Configure clock divider for MPU subsystem
- Result: Frequencies relative to ARM core

**CM_MPU_MPU_CLKCTRL:**
- Field: `STBYST`
- Purpose: Check MPU standby status

See Chapter 8 (PRCM) for complete register map and detailed descriptions.

### CP15 Registers (Secure Monitor Access)

**Auxiliary Control Register:**
- Access: Secure monitor call with service ID 0x100
- Write value in R0

**Non-Secure Access Control Register:**
- Access: Secure monitor call with service ID 0x101
- Write value in R0

**L2 Cache Auxiliary Control Register:**
- Access: Secure monitor call with service ID 0x102
- Write value in R0
- Example use: Enable L2 ECC (set bits 21 and 28)

---

## System Integration Notes

### Memory Map

**On-Chip Memory:**
- ROM: 176KB (code and data)
- RAM (OCM): 64KB (SRAM internal)

### Interconnect

**L3 Interconnect:**
- Connection via I2Async → T2Async bridges
- OCP Master 0: 128-bit
- OCP Master 1: 64-bit

### Debug Architecture

**CoreSight Components:**
- ETM: Embedded in ARM core
- ETB: 32KB at chip level (DebugSS)
- DAP: Debug Access Port
- TPIU: Trace Port Interface Unit
- ICECrusher: Debug interface controller

**Access:**
- JTAG-based debug
- APB slave interface (32-bit)
- ATB (Advanced Trace Bus) for trace data

### Performance Considerations

**Optimization Tips:**

1. **Cache Management:**
   - L1: 32KB I-cache and D-cache with SED
   - L2: 256KB with ECC
   - Use cache for frequently accessed code/data

2. **NEON Utilization:**
   - Enable for media and signal processing workloads
   - Power down when not needed to save power

3. **Interrupt Latency:**
   - Use low interrupt latency configuration (SCTLR.FI)
   - Closely coupled INTC reduces response time

4. **Branch Prediction:**
   - 512-entry BTAC reduces pipeline stalls
   - Optimize code for predictable branches

5. **Power Management:**
   - Use appropriate power modes based on workload
   - Transition between modes as needed
   - Consider retention modes for quick resume

---

## Appendix: Technical Specifications

### ARM Cortex-A8 Pipeline

- **Issue Width:** 2 instructions per cycle (dual-issue)
- **Execution:** In-order
- **Branch Prediction:** 512-entry BTAC
- **Return Stack:** Hardware return stack

### Memory Hierarchy

```
┌────────────────────────────────────┐
│  ARM Core                          │
│  ┌──────────┐      ┌──────────┐   │
│  │  L1 I    │      │  L1 D    │   │
│  │  32KB    │      │  32KB    │   │
│  │  4-way   │      │  4-way   │   │
│  └────┬─────┘      └────┬─────┘   │
│       │                 │          │
│       └────────┬────────┘          │
│                │                   │
│         ┌──────▼──────┐            │
│         │  L2 Cache   │            │
│         │  256KB      │            │
│         │  8-way      │            │
│         │  ECC/Parity │            │
│         └──────┬──────┘            │
└────────────────┼───────────────────┘
                 │
          ┌──────▼──────┐
          │  AXI Bus    │
          │  128-bit    │
          └──────┬──────┘
                 │
          ┌──────▼──────┐
          │  L3 / DDR   │
          └─────────────┘
```

### TLB Configuration

**Instruction TLB:**
- Entries: 32
- Associativity: Fully associative
- Page sizes: 4KB, 64KB, 1MB, 16MB

**Data TLB:**
- Entries: 32
- Associativity: Fully associative
- Page sizes: 4KB, 64KB, 1MB, 16MB

### Bus Specifications

**AXI Interface:**
- Protocol: AMBA AXI
- Data width: 128-bit
- Address width: 32-bit
- Outstanding transactions: Multiple
- Clock ratio: Synchronous with core, supports various ratios

**OCP Interface:**
- Protocol: OCP 2.2
- SRMD: Single Request Multiple Data
- Port widths: 128-bit, 64-bit, 32-bit
- Asynchronous operation: Via I2Async/T2Async bridges

---

## References

1. ARM Cortex-A8 Technical Reference Manual
2. TI AM335x Technical Reference Manual - Chapter 6: Interrupts
3. TI AM335x Technical Reference Manual - Chapter 8: PRCM
4. ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition
5. AMBA AXI Protocol Specification
6. OCP-IP Protocol Specification 2.2
7. ARM CoreSight Architecture Specification

---

**Document Version:** Based on SPRUH73Q (December 2019)  
**Last Updated:** February 2026  
**Format:** Markdown Reference Guide
