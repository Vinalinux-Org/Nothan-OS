# 6 Interrupts

**SPRUH73Q – October 2011 – Revised December 2019**

This section describes the interrupts for the device.

| Topic | Page |
|-------|------|
| 6.1 Functional Description | 531 |
| 6.2 Basic Programming Model | 534 |
| 6.3 ARM Cortex-A8 Interrupts | 543 |
| 6.4 Crypto DMA Events | 547 |
| 6.5 PWM Events | 549 |
| 6.6 Interrupt Controller Registers | 550 |

---

## 6.1 Functional Description

The interrupt controller processes incoming interrupts by masking and priority sorting to produce the interrupt signals for the processor to which it is attached. Figure 6-1 shows the top-level view of interrupt processing.

> **NOTE:** FIQ is not available on general-purpose (GP) devices.

### Figure 6-1. Interrupt Controller Block Diagram

```
                          IRQ_q
                       Interrupt of
                        bank p
                          │
    Software Interrupt    ▼
    ┌──────────┐       ┌─────┐
    │ ISR_SETp │──────►│ OR  │
    └──────────┘       └──┬──┘
                          │
                          ├──────────────────► ┌────────┐
                          │                    │  ITRp  │  Interrupt Input Status
              Mask        │                    └────────┘
           ┌───────┐      │
           │ MIRp  │──►┌──┴───────────┐
           └───────┘   │              │
                       │   Priority   │
    Priority Threshold │  Comparator  │
    ┌───────────┐      │              │
    │ THRESHOLD │─────►│ If(INT Prio  │
    └───────────┘      │  >Threshold) │
                       └──────┬───────┘
                              │
    Interrupt Priority        │
    and FIQ/IRQ Steering      ▼
    ┌───────┐          ┌─────────────────┐
    │ ILRq  │          │  IRQ/FIQ        │
    │PRIORITY│────────►│  Selector       │──►┌─────────────────┐
    │FIQNIRQ │         └─────────────────┘   │ PENDING_IRQp    │
    └───────┘                │               │ PENDING_FIQp    │
                             │               └─────────────────┘
    New Agreement Bits       ▼
    ┌────────────────┐  ┌───────────────┐   ┌───────────────────────┐
    │    Control     │  │Priority Sorting│   │Active Interrupt Nb,   │
    │ NEWFIQAGR      │─►│               │──►│Spurious Flag & Priority│
    │ NEWIRQAGR      │  │ IRQ    FIQ    │   │ SIR_FIQ  FIQ_PRIORITY │
    └────────────────┘  │Priority Priority│  │ SIR_IRQ  IRQ_PRIORITY │
                        │ Sorter  Sorter │   └───────────────────────┘
                        └───┬───────┬───┘
                            │       │
                         IRQ Input FIQ Input
                            └───┬───┘
                             Processor
```

### 6.1.1 Interrupt Processing

#### 6.1.1.1 Input Selection

The INTC supports only level-sensitive incoming interrupt detection. A peripheral asserting an interrupt maintains it until software has handled the interrupt and instructed the peripheral to deassert the interrupt.

A software interrupt is generated if the corresponding bit in the MPU_INTC.INTC_ISR_SETn register is set (register bank number: n = [0,1,2,3] for the MPU subsystem INTC, 128 incoming interrupt lines are supported). The software interrupt clears when the corresponding bit in the MPU_INTC.INTC_ISR_CLEARn register is written. Typical use of this feature is software debugging.

#### 6.1.1.2 Masking

#### 6.1.1.2.1 Individual Masking

Detection of interrupts on each incoming interrupt line can be enabled or disabled independently by the MPU_INTC.INTC_MIRn interrupt mask register. In response to an unmasked incoming interrupt, the INTC can generate one of two types of interrupt requests to the processor:

- **IRQ:** low-priority interrupt request
- **FIQ:** fast interrupt request (Not available on General Purpose (GP) devices)

The type of interrupt request is determined by the MPU_INTC.INTC_ILRm[0] FIQNIRQ bit (m= [0,127]). The current incoming interrupt status before masking is readable from the MPU_INTC.INTC_ITRn register. After masking and IRQ/FIQ selection, and before priority sorting is done, the interrupt status is readable from the MPU_INTC.INTC_PENDING_IRQn and MPU_INTC.INTC_PENDING_FIQn registers.

#### 6.1.1.2.2 Priority Masking

To enable faster processing of high-priority interrupts, a programmable priority masking threshold is provided (the MPU_INTC.INTC_THRESHOLD[7:0] PRIORITYTHRESHOLD field). This priority threshold allows preemption by higher priority interrupts; all interrupts of lower or equal priority than the threshold are masked. However, priority 0 can never be masked by this threshold; a priority threshold of 0 is treated the same way as priority 1. PRIORITY and PRIORITYTHRESHOLD fields values can be set between 0x0 and 0x7F; 0x0 is the highest priority and 0x7F is the lowest priority. When priority masking is not necessary, a priority threshold value of 0xFF disables the priority threshold mechanism. This value is also the reset default for backward compatibility with previous versions of the INTC.

#### 6.1.1.3 Priority Sorting

A priority level (0 being the highest) is assigned to each incoming interrupt line. Both the priority level and the interrupt request type are configured by the MPU_INTC.INTC_ILRm register. If more than one incoming interrupt with the same priority level and interrupt request type occur simultaneously, the highest-numbered interrupt is serviced first. When one or more unmasked incoming interrupts are detected, the INTC separates between IRQ and FIQ using the corresponding MPU_INTC.INTC_ILRm[0] FIQNIRQ bit.

The result is placed in INTC_PENDING_IRQn or INTC_PENDING_FIQn. If no other interrupts are currently being processed, INTC asserts IRQ/FIQ and starts the priority computation. Priority sorting for IRQ and FIQ can execute in parallel. Each IRQ/FIQ priority sorter determines the highest priority interrupt number and that number is placed in the corresponding MPU_INTC.INTC_SIR_IRQ[6:0] ACTIVEIRQ field or MPU_INTC.INTC_SIR_FIQ[6:0] ACTIVEFIQ field. The value is preserved until the corresponding MPU_INTC.INTC_CONTROL NEWIRQAGR or NEWFIQAGR bit is set. Once the interrupting peripheral device has been serviced and the incoming interrupt deasserted, the user must write to the appropriate NEWIRQAGR or NEWFIQAGR bit to indicate to the INTC the interrupt has been handled. If there are any pending unmasked incoming interrupts for this interrupt request type, the INTC restarts the appropriate priority sorter; otherwise, the IRQ or FIQ interrupt line is deasserted.

### 6.1.2 Register Protection

If the MPU_INTC.INTC_PROTECTION[0] PROTECTION bit is set, access to the INTC registers is restricted to privileged mode. Access to the MPU_INTC.INTC_PROTECTION register is always restricted to privileged mode. For more information, see Section 6.6.1.7, INTC_PROTECTION Register (offset = 4Ch) [reset = 0h].

### 6.1.3 Module Power Saving

The INTC provides an auto-idle function in its three clock domains:

- Interface clock
- Functional clock
- Synchronizer clock

The interface clock auto-idle power-saving mode is enabled if the MPU_INTC.INTC_SYSCONFIG[0] AUTOIDLE bit is set to 1. When this mode is enabled and there is no activity on the bus interface, the interface clock is disabled internally to the module, thus reducing power consumption. When there is new activity on the bus interface, the interface clock restarts without any latency penalty. After reset, this mode is disabled, by default.

The functional clock auto-idle power-saving mode is enabled if the MPU_INTC.INTC_IDLE[0] FUNCIDLE bit is set to 0. When this mode is enabled and there is no active interrupt (IRQ or FIQ interrupt being processed or generated) or no pending incoming interrupt, the functional clock is disabled internally to the module, thus reducing power consumption.

When a new unmasked incoming interrupt is detected, the functional clock restarts and the INTC processes the interrupt. If this mode is disabled, the interrupt latency is reduced by one cycle. After reset, this mode is enabled, by default.

The synchronizer clock allows external asynchronous interrupts to be resynchronized before they are masked. The synchronizer input clock has an auto-idle power-saving mode enabled if the MPU_INTC.INTC_IDLE[1] TURBO bit is set to 1. If the auto-idle mode is enabled, the standby power is reduced, but the IRQ or FIQ interrupt latency increases from four to six functional clock cycles. This feature can be enabled dynamically according to the requirements of the device. After reset, this mode is disabled, by default.

### 6.1.4 Error Handling

The following accesses will cause an error:

- Privilege violation (attempt to access PROTECTION register in user mode or any register in user mode if Protection bit is set)
- Unsupported commands

The following accesses will not cause any error response:

- Access to a non-decoded address
- Write to a read-only register

### 6.1.5 Interrupt Handling

The IRQ/FIQ interrupt generation takes four INTC functional clock cycles (plus or minus one cycle) if the MPU_INTC.INTC_IDLE[1] TURBO bit is set to 0. If the TURBO bit is set to 1, the interrupt generation takes six cycles, but power consumption is reduced while waiting for an interrupt. These latencies can be reduced by one cycle by disabling functional clock auto-idle (MPU_INTC.INTC_IDLE[0] FUNCIDLE bit set to 1), but power consumption is increased, so the benefit is minimal.

To minimize interrupt latency when an unmasked interrupt occurs, the IRQ or FIQ interrupt is generated before priority sorting completion. The priority sorting takes 10 functional clock cycles, which is less than the minimum number of cycles required for the MPU to switch to the interrupt context after reception of the IRQ or FIQ event.

Any read of the MPU_INTC.INTC_SIR_IRQ or MPU_INTC.INTC_SIR_FIQ register during the priority sorting process stalls until priority sorting is complete and the relevant register is updated. However, the delay between the interrupt request being generated and the interrupt service routine being executed is such that priority sorting always completes before the MPU_INTC.INTC_SIR_IRQ or MPU_INTC.INTC_SIR_FIQ register is read.

---

## 6.2 Basic Programming Model

### 6.2.1 Initialization Sequence

1. Program the MPU_INTC.INTC_SYSCONFIG register: If necessary, enable the interface clock autogating by setting the AUTOIDLE bit.
2. Program the MPU_INTC.INTC_IDLE register: If necessary, disable functional clock autogating or enable synchronizer autogating by setting the FUNCIDLE bit or TURBO bit accordingly.
3. Program the MPU_INTC.INTC_ILRm register for each interrupt line: Assign a priority level and set the FIQNIRQ bit for an FIQ interrupt (by default, interrupts are mapped to IRQ and priority is 0x0 [highest]).
4. Program the MPU_INTC.INTC_MIRn register: Enable interrupts (by default, all interrupt lines are masked). NOTE: To program the MPU_INTC.INTC_MIRn register, the MPU_INTC.INTC_MIR_SETn and MPU_INTC.INTC_MIR_CLEARn registers are provided to facilitate the masking, even if it is possible for backward-compatibility to write directly to the MPU_INTC.INTC_MIRn register.

### 6.2.2 INTC Processing Sequence

After the INTC_MIRn and INTC_ILRm registers are configured to enable and assign priorities to incoming interrupts, the interrupt is processed as explained in the following subsections. IRQ and FIQ processing sequences are quite similar, the differences for the FIQ sequence are shown after a '/' character in the code below.

1. One or more unmasked incoming interrupts (M_IRQ_n signals) are received and IRQ or FIQ outputs (IRQ/FIQ) are not currently asserted.
2. If the INTC_ILRm[0] FIQNIRQ bit is cleared to 0, the MPU_INTC_IRQ output signal is generated. If the FIQNIRQ bit is set to 1, the MPU_INTC_FIQ output signal is generated.
3. The INTC performs the priority sorting and updates the INTC_SIR_IRQ[6:0] ACTIVEIRQ / INTC_SIR_FIQ[6:0] ACTIVEFIQ field with the current interrupt number.
4. During priority sorting, if the IRQ/FIQ is enabled at the host processor side, the host processor automatically saves the current context and executes the ISR as follows.

The ARM host processor automatically performs the following actions in pseudo code:

```
LR = PC + 4                     /* return link */
SPSR = CPSR                     /* Save CPSR before execution */
CPSR[5] = 0                     /* Execute in ARM state */
CPSR[7] = 1                     /* Disable IRQ */
CPSR[8] = 1                     /* Disable Imprecise Data Aborts */
CPSR[9] = CP15_reg1_EEbit       /* Endianness on exception entry */
if interrupt == IRQ then
  CPSR[4:0] = 0b10010           /* Enter IRQ mode */
  if high vectors configured then
    PC = 0xFFFF0018
  else
    PC = 0x00000018              /* execute interrupt vector */
else if interrupt == FIQ then
  CPSR[4:0] = 0b10001           /* Enter FIQ mode */
  CPSR[6] = 1                   /* Disable FIQ */
  if high vectors configured then
    PC = 0xFFFF001C
  else
    PC = 0x0000001C              /* execute interrupt vector */
endif
```

5. The ISR saves the remaining context, identifies the interrupt source by reading the ACTIVEIRQ/ACTIVEFIQ field, and jumps to the relevant subroutine handler as follows:

> **CAUTION:** The code in steps 5 and 7 is an assembly code compatible with ARM architecture V6 and V7. This code is developed for the Texas Instruments Code Composer Studio tool set. It is a draft version, only tested on an emulated environment.

```asm
;INTC_SIR_IRQ/INTC_SIR_FIQ register address
INTC_SIR_IRQ_ADDR/INTC_SIR_FIQ_ADDR .word 0x48200040/0x48200044
; ACTIVEIRQ bit field mask to get only the bit field
ACTIVEIRQ_MASK .equ 0x7F

_IRQ_ISR/_FIQ_ISR:
  ; Save the critical context
  STMFD SP!, {R0-R12, LR}          ; Save working registers and the Link register
  MRS   R11, SPSR                   ; Save the SPSR into R11

  ; Get the number of the highest priority active IRQ/FIQ
  LDR   R10, INTC_SIR_IRQ_ADDR/INTC_SIR_FIQ_ADDR
  LDR   R10, [R10]                  ; Get the INTC_SIR_IRQ/INTC_SIR_FIQ register
  AND   R10, R10, #ACTIVEIRQ_MASK   ; Apply the mask to get the active IRQ number

  ; Jump to relevant subroutine handler
  LDR   PC, [PC, R10, lsl #2]       ; PC base address points this instruction + 8
  NOP                                ; To index the table by the PC

  ; Table of handler start addresses
  .word IRQ0handler                  ;For IRQ0 of BANK0
  .word IRQ1handler
  .word IRQ2handler
```

6. The subroutine handler executes code specific to the peripheral generating the interrupt by handling the event and deasserting the interrupt condition at the peripheral side.

```asm
; IRQ0 subroutine
IRQ0handler:
  ; Save working registers
  STMFD SP!, {R0-R1}
  ; Now read-modify-write the peripheral module status register
  ; to de-assert the M_IRQ_0 interrupt signal
  ; De-Assert the peripheral interrupt
  MOV   R0, #0x7                           ; Mask for 3 flags
  LDR   R1, MODULE0_STATUS_REG_ADDR        ; Get the address of the module Status Register
  STR   R0, [R1]                           ; Clear the 3 flags
  ; Restore working registers
  LDMFD SP!, {R0-R1}
  ; Jump to the end part of the ISR
  B     IRQ_ISR_end/FIQ_ISR_end
```

7. After the return of the subroutine, the ISR sets the NEWIRQAGR/NEWFIQAGR bit to enable the processing of subsequent pending IRQs/FIQs and to restore ARM context in the following code. Because the writes are posted on an Interconnect bus, to be sure that the preceding writes are done before enabling IRQs/FIQs, a Data Synchronization Barrier is used. This operation ensure that the IRQ/FIQ line is de-asserted before IRQ/FIQ enabling. After that, the INTC processes any other pending interrupts or deasserts the IRQ/FIQ signal if there is no interrupt.

```asm
; INTC_CONTROL register address
INTC_CONTROL_ADDR .word 0x48200048;
NEWIRQAGR/NEWFIQAGR bit mask to set only the NEWIRQAGR/NEWFIQAGR bit
NEWIRQAGR_MASK/NEWFIQAGR_MASK .equ 0x01/0x02

IRQ_ISR_end/FIQ_ISR_end:
  ; Allow new IRQs/FIQs at INTC side
  MOV   R0, #NEWIRQAGR_MASK/NEWFIQAGR_MASK
  LDR   R1, INTC_CONTROL_ADDR
  STR   R0, [R1]                ; Write the NEWIRQAGR/NEWFIQAGR bit to allow new IRQs/FIQ

  ; Data Synchronization Barrier
  MOV   R0, #0
  MCR   P15, #0, R0, C7, C10, #4

  ; restore critical context
  MSR   SPSR, R11               ; Restore the SPSR from R11
  LDMFD SP!, {R0-R12, LR}       ; Restore working registers and Link register

  ; Return after handling the interrupt
  SUBS  PC, LR, #4
```

8. After the ISR return, the ARM automatically restores its context as follows:

```
CPSR = SPSR
PC = LR
```

Figure 6-2 shows the IRQ/FIQ processing sequence from the originating device peripheral module to the main program interruption.

The priority sorting mechanism is frozen during an interrupt processing sequence. If an interrupt condition occurs during this time, the interrupt is not lost. It is sorted when the NEWIRQAGR/NEWFIQAGR bit is set (priority sorting is reactivated).

### Figure 6-2. IRQ/FIQ Processing Sequence

```
Hardware                                    Software
┌─────────────────────────┐
│  SOC Peripheral Module  │
│  Step 1: M_IRQ_n Assert │
└────────────┬────────────┘
             ▼
┌─────────────────────────┐
│       MPU INTC          │
│ If IRQ_n not masked and │            ┌──────────────────────┐
│ configured as IRQ/FIQ,  │            │    Main Program      │
│ MPU_INTC_IRQ/FIQ assert │            │ Exec instruction 1   │
└────────────┬────────────┘            │ Exec instruction N   │
  Step 2:    │                         └──────────┬───────────┘
  MPU_INTC_IRQ/FIQ Asserted                      │
             ▼                                    │
┌─────────────────────────┐     Branch  ┌─────────▼──────────────┐
│ ARM Host Processor (4)  │────────────►│ ISR in IRQ/FIQ Mode(5) │
│ Finish current instr N  │             │ Save ARM context       │
│ Store next addr to LR   │             │ Identify interrupt src │
│ Save CPSR to SPSR       │             │ Branch to handler      │
│ Enter IRQ/FIQ mode      │             └─────────┬──────────────┘
│ Disable IRQs/FIQs       │                 Branch│
│ Execute interrupt vector │                       ▼
└─────────────────────────┘             ┌────────────────────────┐
                                        │ Subroutine Handler (6) │
                                        │ Handle the event       │
                                        │ Deassert M_IRQ_n       │
                                        └─────────┬──────────────┘
                                             Branch│
                                                   ▼
                                        ┌────────────────────────┐
┌─────────────────────────┐    Return   │ ISR in IRQ/FIQ Mode(7) │
│ ARM Host Processor (8)  │◄────────────│ Set NEWIRQAGR/NEWFIQAGR│
│ Restore CPSR            │             │ Restore ARM context    │
│ Restore PC              │    Return   └────────────────────────┘
└─────────────────────────┘────────────►┌────────────────────────┐
                                        │    Main Program        │
                                        │ Exec instruction N+1   │
                                        └────────────────────────┘
```

### 6.2.3 INTC Preemptive Processing Sequence

Preemptive interrupts, also called nested interrupts, can reduce the latencies for higher priority interrupts. A preemptive ISR can be suspended by a higher priority interrupt. Thus, the higher priority interrupt can be served immediately. Nested interrupts must be used carefully to avoid using corrupted data. Programmers must save corruptible registers and enable IRQ or FIQ at ARM side. IRQ and FIQ processing sequences are quite similar, the differences for the FIQ sequence are shown after a '/' character in the code below.

To enable IRQ/FIQ preemption by higher priority IRQs/FIQs, programmers can follow this procedure to write the ISR.

At the beginning of an IRQ/FIQ ISR:

1. Save the ARM critical context registers.
2. Save the INTC_THRESHOLD PRIORITYTHRESHOLD field before modifying it.
3. Read the active interrupt priority in the INTC_IRQ_PRIORITY IRQPRIORITY / INTC_FIQ_PRIORITY FIQPRIORITY field and write it to the PRIORITYTHRESHOLD field (see note 1).
4. Read the active interrupt number in the INTC_SIR_IRQ[6:0] ACTIVEIRQ / INTC_SIR_FIQ[6:0] ACTIVEFIQ field to identify the interrupt source.
5. Write 1 to the appropriate INTC_CONTROL NEWIRQAGR and NEWFIQAGR bit (see note 2) while an interrupt is still processing to allow only higher priority interrupts to preempt.
6. Because the writes are posted on an Interconnect bus, to be sure that the preceding writes are done before enabling IRQs/FIQs, a Data Synchronization Barrier is used. This operation ensure that the IRQ line is de-asserted before IRQ/FIQ enabling.
7. Enable IRQ/FIQ at ARM side.
8. Jump to the relevant subroutine handler.

The following sample code shows the previous steps:

> **CAUTION:** The following code is an assembly code compatible with ARM architecture V6 and V7. This code is developed for the Texas Instruments Code Composer Studio tool set. It is a draft version, only tested on an emulated environment.

```asm
; bit field mask to get only the bit field
ACTIVEPRIO_MASK .equ 0x7F

_IRQ_ISR:
  ; Step 1 : Save the critical context
  STMFD SP!, {R0-R12, LR}          ; Save working registers
  MRS   R11, SPSR                   ; Save the SPSR into R11

  ; Step 2 : Save the INTC_THRESHOLD register into R12
  LDR   R0, INTC_THRESHOLD_ADDR
  LDR   R12, [R0]
```

> **(1)** The priority-threshold mechanism is enabled automatically when writing a priority in the range of 0x00 to 0x3F. Writing a value of 0xFF (reset default) disables the priority-threshold mechanism. Values between 0x3F and 0xFF must not be used. When the hardware priority threshold is in use, the priorities of interrupts selected as FIQ or IRQ become linked; otherwise, they are independent. When they are linked, all FIQ priorities must be set higher than all IRQ priorities to maintain the relative priority of FIQ over IRQ.
>
> **(2)** When handling FIQs using the priority-threshold mechanism, both NEWFIQAGR and NEWIRQAGR bits must be written at the same time to ensure that the new priority threshold is applied while an IRQ sort is in progress. This IRQ will not have been seen by the ARM, as it will have been masked on entry to the FIQ ISR. However, the source of the IRQ remains active and it is finally processed when the priority threshold falls to a priority sufficiently low to allow it to be processed. The precaution of writing to New FIQ Agreement is not required during an IRQ ISR, as FIQ sorting is not affected (provided all FIQ priorities are higher than all IRQ priorities).

```asm
  ; Step 3 : Get the priority of the highest priority active IRQ
  LDR   R1, INTC_IRQ_PRIORITY_ADDR/INTC_FIQ_PRIORITY_ADDR
  LDR   R1, [R1]                    ; Get the INTC_IRQ_PRIORITY/INTC_FIQ_PRIORITY register
  AND   R1, R1, #ACTIVEPRIO_MASK    ; Apply the mask to get the priority of the IRQ
  STR   R1, [R0]                    ; Write it to the INTC_THRESHOLD register

  ; Step 4 : Get the number of the highest priority active IRQ
  LDR   R10, INTC_SIR_IRQ_ADDR/INTC_SIR_FIQ_ADDR
  LDR   R10, [R10]                  ; Get the INTC_SIR_IRQ/INTC_SIR_FIQ register
  AND   R10, R10, #ACTIVEIRQ_MASK   ; Apply the mask to get the active IRQ number

  ; Step 5 : Allow new IRQs and FIQs at INTC side
  MOV   R0, #0x1/0x3                ; Get the NEWIRQAGR and NEWFIQAGR bit position
  LDR   R1, INTC_CONTROL_ADDR
  STR   R0, [R1]                    ; Write the NEWIRQAGR and NEWFIQAGR bit

  ; Step 6 : Data Synchronization Barrier
  MOV   R0, #0
  MCR   P15, #0, R0, C7, C10, #4

  ; Step 7 : Read-modify-write the CPSR to enable IRQs/FIQs at ARM side
  MRS   R0, CPSR                    ; Read the status register
  BIC   R0, R0, #0x80/0x40          ; Clear the I/F bit
  MSR   CPSR, R0                    ; Write it back to enable IRQs

  ; Step 8 : Jump to relevant subroutine handler
  LDR   PC, [PC, R10, lsl #2]       ; PC base address points this instruction + 8
  NOP                                ; To index the table by the PC

  ; Table of handler start addresses
  .word IRQ0handler                  ;IRQ0 BANK0
  .word IRQ1handler
  .word IRQ2handler
```

After the return of the relevant IRQ/FIQ subroutine handler:

1. Disable IRQs/FIQs at ARM side.
2. Restore the INTC_THRESHOLD PRIORITYTHRESHOLD field.
3. Restore the ARM critical context registers.

The following sample code shows the three previous steps:

> **CAUTION:** The following code is an assembly code compatible with ARM architecture V6 and V7. This code is developed for the Texas Instruments Code Composer Studio tool set. It is a draft version, only tested on an emulated environment.

```asm
IRQ_ISR_end:
  ; Step 1 : Read-modify-write the CPSR to disable IRQs/FIQs at ARM side
  MRS   R0, CPSR                    ; Read the CPSR
  ORR   R0, R0, #0x80/0x40          ; Set the I/F bit
  MSR   CPSR, R0                    ; Write it back to disable IRQs

  ; Step 2 : Restore the INTC_THRESHOLD register from R12
  LDR   R0, INTC_THRESHOLD_ADDR
  STR   R12, [R0]

  ; Step 3 : Restore critical context
  MSR   SPSR, R11                   ; Restore the SPSR from R11
  LDMFD SP!, {R0-R12, LR}           ; Restore working registers and Link register

  ; Return after handling the interrupt
  SUBS  PC, LR, #4
```

Figure 6-3 shows the nested IRQ/FIQ processing sequence from the originating device peripheral module to the main program interruption.

### Figure 6-3. Nested IRQ/FIQ Processing Sequence

```
Hardware                                    Software
┌─────────────────────────┐
│  SOC Peripheral Module  │
│  Step 1: M_IRQ_n Assert │
└────────────┬────────────┘
             ▼
┌─────────────────────────┐
│       MPU INTC          │
│ If IRQ_n not masked and │            ┌──────────────────────┐
│ configured as IRQ/FIQ,  │            │    Main Program      │
│ MPU_INTC_IRQ/FIQ assert │            │ Exec instruction 1   │
└────────────┬────────────┘            │ Exec instruction N   │
  Step 2:    │                         └──────────┬───────────┘
  MPU_INTC_IRQ/FIQ Asserted                      │
             ▼                                    │
┌─────────────────────────┐     Branch  ┌─────────▼──────────────────┐
│ ARM Host Processor      │────────────►│ ISR in IRQ/FIQ Mode (Step5)│
│ Finish current instr N  │             │ Save ARM critical context  │
│ Store next addr to LR   │             │ Save INTC priority thresh  │
│ Save CPSR to SPSR       │             │ Get active IRQ priority    │
│ Enter IRQ/FIQ mode      │             │ Set IRQ prio to threshold  │
│ Disable IRQs/FIQs       │             │ Identify interrupt source  │
│ Execute interrupt vector │             │ Set NEWIRQAGR/NEWFIQAGR   │
└─────────────────────────┘             │ Enable IRQ/FIQ at ARM side │
                                        │ Jump to ISR handler        │
                                        └─────────┬──────────────────┘
                                             Branch│
                                                   ▼
                                        ┌────────────────────────────┐
                                        │ Subroutine Handler         │
                                        │ Handle the event           │
                                        │ Deassert M_IRQ_n           │
                                        └─────────┬──────────────────┘
                                             Branch│
                                                   ▼
                                        ┌────────────────────────────┐
┌─────────────────────────┐    Return   │ ISR in IRQ/FIQ Mode        │
│ ARM Host Processor (8)  │◄────────────│ Disable IRQ/FIQ at ARM side│
│ Restore CPSR            │             │ Restore INTC priority thresh│
│ Restore PC              │    Return   │ Restore ARM critical context│
└─────────────────────────┘────────────►└────────────────────────────┘
                                        ┌────────────────────────────┐
                                        │    Main Program            │
                                        │ Exec instruction N+1       │
                                        └────────────────────────────┘
```

### 6.2.4 Interrupt Preemption

If wanting to enable pre-emption by higher priority interrupts, the ISR should read the active interrupt priority and write it to the priority threshold register. Writing a '1' to the appropriate NEW_IRQ_AGR or NEW_FIQ_AGR bits of the CONTROL register while still processing the interrupt will now allow only higher priority interrupts to pre-empt.

For each level of pre-emption, the programmer must save the threshold value before modifying it and restore it at the end of that ISR level.

The priority threshold mechanism is enabled automatically when writing a priority in the range of 0 to 7Fh as will be read from the IRQ_PRIORITY and FIQ_PRIORITY registers. Writing a value of FFh (reset default) disables the priority threshold mechanism.

When the hardware priority threshold is in use the priorities of interrupts selected as FIQ or IRQ become linked, otherwise they are independent. When linked, it is required that all FIQ priorities be set higher than all IRQ priorities to maintain the relative priority of FIQ over IRQ.

When handling FIQs using the priority threshold mechanism, it is required to write both New FIQ Agreement and New IRQ Agreement bits at the same time to cover the case that the new priority threshold is applied while an IRQ sorting is in progress. This IRQ will not have been seen by the ARM as it will have been masked on entry to the FIQ ISR. However, the source of the IRQ will remain active and it will be finally processed when the priority threshold falls to a low enough priority. The precaution of writing to New FIQ Agreement (as well as New IRQ Agreement) is not required during an IRQ ISR as FIQ sorting will not be affected (provided all FIQ priorities are higher than all IRQ priorities).

### 6.2.5 ARM A8 INTC Spurious Interrupt Handling

The spurious flag indicates whether the result of the sorting (a window of 10 INTC functional clock cycles after the interrupt assertion) is invalid. The sorting is invalid if:

- The interrupt that triggered the sorting is no longer active during the sorting.
- A change in the mask has affected the result during the sorting time.

As a result, the values in the INTC_MIRn, INTC_ILRm, or INTC_MIR_SETn registers must not be changed while the corresponding interrupt is asserted. Only the active interrupt input that triggered the sort can be masked before it turns on the sort. If these registers are changed within the 10-cycle window after the interrupt assertion, the resulting values of the following registers become invalid:

- INTC_SIR_IRQ
- INTC_SIR_FIQ
- INTC_IRQ_PRIORITY
- INTC_FIQ_PRIORITY

This condition is detected for both IRQ and FIQ, and the invalid status is flagged across the SPURIOUSIRQFLAG and SPURIOUSFIQFLAG bit fields in the SIR and PRIORITY registers. A 0 indicates valid and a 1 indicates invalid interrupt number and priority. The invalid indication can be tested in software as a false register value.

> **NOTE:**
> 1. The INTC_SIR_IRQ[31:7] SPURIOUSIRQFLAG bit field is a copy of the INTC_IRQ_PRIORITY[31:7] SPURIOUSIRQFLAG bit field.
> 2. The INTC_SIR_FIQ[31:7] SPURIOUSFIQFLAG bit field is a copy of the INTC_FIQ_PRIORITY[31:7] SPURIOUSFIQFLAG bit field.

---

## 6.3 ARM Cortex-A8 Interrupts

(1) For differences in operation based on AM335x silicon revisions, see Section 1.2, Silicon Revision Functional Differences and Enhancements.

(2) pr1_host_intr[0:7] corresponds to Host-2 to Host-9 of the PRU-ICSS interrupt controller.

### Table 6-1. ARM Cortex-A8 Interrupts

| Int Number | Acronym/name | Source | Signal Name |
|------------|-------------|--------|-------------|
| 0 | EMUINT | MPU Subsystem Internal | Emulation interrupt (EMUICINTR) |
| 1 | COMMTX | MPU Subsystem Internal | CortexA8 COMMTX |
| 2 | COMMRX | MPU Subsystem Internal | CortexA8 COMMRX |
| 3 | BENCH | MPU Subsystem Internal | CortexA8 NPMUIRQ |
| 4 | ELM_IRQ | ELM | Sinterrupt (Error location process completion) |
| 5 | SSM_WFI_IRQ | MPU Subsystem Internal (SSM) | MPU s/s Secure State Machine Wait for Interrupt (WFI) tracking mechanism |
| 5 | Reserved | | |
| 6 | SSM_IRQ | MPU Subsystem Internal (SSM) | MPU s/s Secure State Machine interrupt generation for Public FIQ support |
| 6 | Reserved | | |
| 7 | NMI | External Pin (active low)(1) | nmi_int |
| 8 | SEC_EVNT | Firewalls | security_events_irq |
| 8 | Reserved | | |
| 9 | L3DEBUG | L3 | l3_FlagMux_top_FlagOut1 |
| 10 | L3APPINT | L3 | l3_FlagMux_top_FlagOut0 |
| 11 | PRCMINT | PRCM | irq_mpu |
| 12 | EDMACOMPINT | TPCC (EDMA) | tpcc_int_pend_po0 |
| 13 | EDMAMPERR | TPCC (EDMA) | tpcc_mpint_pend_po |
| 14 | EDMAERRINT | TPCC (EDMA) | tpcc_errint_pend_po |
| 15 | WDT0INT | WDTIMER0 | PO_INT_PEND |
| 15 | Reserved | | |
| 16 | ADC_TSC_GENINT | ADC_TSC (Touchscreen Controller) | gen_intr_pend |
| 17 | USBSSINT | USBSS | usbss_intr_pend |
| 18 | USBINT0 | USBSS | usb0_intr_pend |
| 19 | USBINT1 | USBSS | usb1_intr_pend |
| 20 | PRU_ICSS_EVTOUT0 | pr1_host[0] output/events exported from PRU-ICSS(2) | pr1_host_intr0_intr_pend |
| 21 | PRU_ICSS_EVTOUT1 | pr1_host[1] output/events exported from PRU-ICSS(2) | pr1_host_intr1_intr_pend |
| 22 | PRU_ICSS_EVTOUT2 | pr1_host[2] output/events exported from PRU-ICSS(2) | pr1_host_intr2_intr_pend |
| 23 | PRU_ICSS_EVTOUT3 | pr1_host[3] output/events exported from PRU-ICSS(2) | pr1_host_intr3_intr_pend |
| 24 | PRU_ICSS_EVTOUT4 | pr1_host[4] output/events exported from PRU-ICSS(2) | pr1_host_intr4_intr_pend |
| 25 | PRU_ICSS_EVTOUT5 | pr1_host[5] output/events exported from PRU-ICSS(2) | pr1_host_intr5_intr_pend |
| 26 | PRU_ICSS_EVTOUT6 | pr1_host[6] output/events exported from PRU-ICSS(2) | pr1_host_intr6_intr_pend |
| 27 | PRU_ICSS_EVTOUT7 | pr1_host[7] output/events exported from PRU-ICSS(2) | pr1_host_intr7_intr_pend |
| 28 | MMCSD1INT | MMCSD1 | SINTERRUPTN |
| 29 | MMCSD2INT | MMCSD2 | SINTERRUPTN |
| 30 | I2C2INT | I2C2 | POINTRPEND |
| 31 | eCAP0INT | eCAP0 event/interrupt | ecap_intr_intr_pend |
| 32 | GPIOINT2A | GPIO 2 | POINTRPEND1 |
| 33 | GPIOINT2B | GPIO 2 | POINTRPEND2 |
| 34 | USBWAKEUP | USBSS | slv0p_Swakeup |
| 35 | PCIeWAKEUP | PCIe | sle_idlep_SWakeup |
| 35 | Reserved | | |
| 36 | LCDCINT | LCDC | lcd_irq |
| 37 | GFXINT | SGX530 | THALIAIRQ |
| 38 | Reserved | | |
| 39 | ePWM2INT | eHRPWM2 (PWM Subsystem) | epwm_intr_intr_pend |
| 40 | 3PGSWRXTHR0 (RX_THRESH_PULSE) | CPSW (Ethernet) | c0_rx_thresh_pend |
| 41 | 3PGSWRXINT0 (RX_PULSE) | CPSW (Ethernet) | c0_rx_pend |
| 42 | 3PGSWTXINT0 (TX_PULSE) | CPSW (Ethernet) | c0_tx_pend |
| 43 | 3PGSWMISC0 (MISC_PULSE) | CPSW (Ethernet) | c0_misc_pend |
| 44 | UART3INT | UART3 | niq |
| 45 | UART4INT | UART4 | niq |
| 46 | UART5INT | UART5 | niq |
| 47 | eCAP1INT | eCAP1 (PWM Subsystem) | ecap_intr_intr_pend |
| 48 | PCIINT0 | PCIe | pcie_int_i_intr0_pend |
| 49 | PCIINT1 | PCIe | pcie_int_i_intr1_pend |
| 50 | PCIINT2 | PCIe | pcie_int_i_intr2_pend |
| 51 | PCIINT3 | PCIe | pcie_int_i_intr3_pend |
| 48 | Reserved | | |
| 49 | Reserved | | |
| 50 | Reserved | | |
| 51 | Reserved | | |
| 52 | DCAN0_INT0 | DCAN0 | dcan_intr0_intr_pend |
| 53 | DCAN0_INT1 | DCAN0 | dcan_intr1_intr_pend |
| 54 | DCAN0_PARITY | DCAN0 | dcan_uerr_intr_pend |
| 55 | DCAN1_INT0 | DCAN1 | dcan_intr0_intr_pend |
| 56 | DCAN1_INT1 | DCAN1 | dcan_intr1_intr_pend |
| 57 | DCAN1_PARITY | DCAN1 | dcan_uerr_intr_pend |
| 58 | ePWM0_TZINT | eHRPWM0 TZ interrupt (PWM Subsystem) | epwm_tz_intr_pend |
| 59 | ePWM1_TZINT | eHRPWM1 TZ interrupt (PWM Subsystem) | epwm_tz_intr_pend |
| 60 | ePWM2_TZINT | eHRPWM2 TZ interrupt (PWM Subsystem) | epwm_tz_intr_pend |
| 61 | eCAP2INT | eCAP2 (PWM Subsystem) | ecap_intr_intr_pend |
| 62 | GPIOINT3A | GPIO 3 | POINTRPEND1 |
| 63 | GPIOINT3B | GPIO 3 | POINTRPEND2 |
| 64 | MMCSD0INT | MMCSD0 | SINTERRUPTN |
| 65 | McSPI0INT | McSPI0 | SINTERRUPTN |
| 66 | TINT0 | Timer0 | POINTR_PEND |
| 67 | TINT1_1MS | DMTIMER_1ms | POINTR_PEND |
| 68 | TINT2 | DMTIMER2 | POINTR_PEND |
| 69 | TINT3 | DMTIMER3 | POINTR_PEND |
| 70 | I2C0INT | I2C0 | POINTRPEND |
| 71 | I2C1INT | I2C1 | POINTRPEND |
| 72 | UART0INT | UART0 | niq |
| 73 | UART1INT | UART1 | niq |
| 74 | UART2INT | UART2 | niq |
| 75 | RTCINT | RTC | timer_intr_pend |
| 76 | RTCALARMINT | RTC | alarm_intr_pend |
| 77 | MBINT0 | Mailbox0 (mail_u0_irq) | initiator_sinterrupt_q_n0 |
| 78 | M3_TXEV | Wake M3 Subsystem | TXEV |
| 79 | eQEP0INT | eQEP0 (PWM Subsystem) | eqep_intr_intr_pend |
| 80 | MCATXINT0 | McASP0 | mcasp_x_intr_pend |
| 81 | MCARXINT0 | McASP0 | mcasp_r_intr_pend |
| 82 | MCATXINT1 | McASP1 | mcasp_x_intr_pend |
| 83 | MCARXINT1 | McASP1 | mcasp_r_intr_pend |
| 84 | Reserved | | |
| 85 | Reserved | | |
| 86 | ePWM0INT | eHRPWM0 (PWM Subsystem) | epwm_intr_intr_pend |
| 87 | ePWM1INT | eHRPWM1 (PWM Subsystem) | epwm_intr_intr_pend |
| 88 | eQEP1INT | eQEP1 (PWM Subsystem) | eqep_intr_intr_pend |
| 89 | eQEP2INT | eQEP2 (PWM Subsystem) | eqep_intr_intr_pend |
| 90 | DMA_INTR_PIN2 | External DMA/Interrupt Pin2 (xdma_event_intr2) | pi_x_dma_event_intr2 |
| 91 | WDT1INT (Public Watchdog) | WDTIMER1 | PO_INT_PEND |
| 92 | TINT4 | DMTIMER4 | POINTR_PEND |
| 93 | TINT5 | DMTIMER5 | POINTR_PEND |
| 94 | TINT6 | DMTIMER6 | POINTR_PEND |
| 95 | TINT7 | DMTIMER7 | POINTR_PEND |
| 96 | GPIOINT0A | GPIO 0 | POINTRPEND1 |
| 97 | GPIOINT0B | GPIO 0 | POINTRPEND2 |
| 98 | GPIOINT1A | GPIO 1 | POINTRPEND1 |
| 99 | GPIOINT1B | GPIO 1 | POINTRPEND2 |
| 100 | GPMCINT | GPMC | gpmc_sinterrupt |
| 101 | DDRERR0 | EMIF | sys_err_intr_pend |
| 102 | AES0_IRQ_S | AES module 0 Interrupt secure side | AES_SINTREQUEST_S |
| 102 | Reserved | | |
| 103 | AES0_IRQ_P | AES module 0 Interrupt public side | AES_SINTREQUEST_P |
| 103 | Reserved | | |
| 104–107 | Reserved | | |
| 108 | SHA_IRQ_S | SHA2 crypto-accelerator secure side | SHA_SINTREQUEST_S |
| 109 | SHA_IRQ_P | SHA2 crypto-accelerator public side | SHA_SINTREQUEST_P |
| 110 | FPKA_SINTREQUEST_S | PKA | PKA_SINTREQUEST_S |
| 111 | RNG_IRQ | RNG | TRNG_intr_pend |
| 112 | TCERRINT0 | TPTC0 | tptc_erint_pend_po |
| 113 | TCERRINT1 | TPTC1 | tptc_erint_pend_po |
| 114 | TCERRINT2 | TPTC2 | tptc_erint_pend_po |
| 115 | ADC_TSC_PENINT | ADC_TSC | pen_intr_pend |
| 116–119 | Reserved | | |
| 120 | SMRFLX_MPU subsystem | Smart Reflex 0 | intrpend |
| 121 | SMRFLX_Core | Smart Reflex 1 | intrpend |
| 122 | Reserved | | |
| 123 | DMA_INTR_PIN0 | External DMA/Interrupt Pin0 (xdma_event_intr0) | pi_x_dma_event_intr0 |
| 124 | DMA_INTR_PIN1 | External DMA/Interrupt Pin1 (xdma_event_intr1) | pi_x_dma_event_intr1 |
| 125 | McSPI1INT | McSPI1 | SINTERRUPTN |
| 126 | Reserved | | |
| 127 | Reserved | | |

---

## 6.4 Crypto DMA Events

### Table 6-2. Crypto DMA Events

| Event# | Event Name | Source Module |
|--------|-----------|--------------|
| 1 | AES0_s_dma_ctx_in_req | AES Module 0: Request a new context on the Secure HIB |
| 2 | AES0_s_dma_data_in_req | AES Module 0: Request input data on the Secured HIB |
| 3 | AES0_s_dma_data_out_req | AES Module 0: Request output data read on the Secured HIB |
| 4 | AES0_p_dma_ctx_in_req | AES Module 0: Request a new context on the Public HIB |
| 5 | AES0_p_dma_data_in_req | AES Module 0: Request input data on the Public HIB |
| 6 | AES0_p_dma_data_out_req | AES Module 0: Request output data read on the Public HIB |
| 7 | AES1_s_dma_ctx_in_req | AES Module 1: Request a new context on the Secured HIB |
| 8 | AES1_s_dma_data_in_req | AES Module 1: Request input data on the Secured HIB |
| 9 | AES1_s_dma_data_out_req | AES Module 1: Request output data read on the Secured HIB |
| 10 | AES1_p_dma_ctx_in_req | AES Module 1: Request a new context on the Public HIB |
| 11 | AES1_p_dma_data_in_req | AES Module 1: Request input data on the Public HIB |
| 12 | AES1_p_dma_data_out_req | AES Module 1: Request output data read on the Public HIB |
| 13 | Reserved | |
| 14 | Reserved | |
| 15 | DES_s_dma_ctx_in_req | DES Module: Request a new context on the secure HIB |
| 16 | DES_s_dma_data_in_req | DES Module: Request input data on the secure HIB |
| 17 | DES_s_dma_data_out_req | DES Module: Request output data read on the secure HIB |
| 18 | DES_p_dma_ctx_in_req | DES Module: Request a new context on the public HIB |
| 19 | DES_p_dma_data_in_req | DES Module: Request input data on the public HIB |
| 20 | DES_p_dma_data_out_req | DES Module: Request output data read on the public HIB |
| 21 | SHA2_dma_ctxin_s | SHA2MD5 Module 1: Request a context on secure HIB |
| 22 | SHA2_dma_din_s | SHA2MD5 Module 1: Request input data on secure HIB |
| 23 | SHA2_dma_ctxout_s | SHA2MD5 Module 1: Request output data/context on secure HIB |
| 24 | SHA2_dma_ctxin_p | SHA2MD5 Module 1: Request a context on public HIB |
| 25 | SHA2_dma_din_p | SHA2MD5 Module 1: Request input data on public HIB |
| 26 | SHA2_dma_ctxout_p | SHA2MD5 Module 1: Request output data/context on public HIB |
| 27 | AES0_s_dma_context_out_req | AES Module 0: Request the authentication result (TAG) or result IV read on the Secured HIB |
| 28 | AES0_p_dma_context_out_req | AES Module 0: Request the authentication result (TAG) or result IV read on the Public HIB |
| 29 | AES1_s_dma_context_out_req | AES Module 1: Request the authentication result (TAG) or result IV read on the Secured HIB |
| 30 | AES1_p_dma_context_out_req | AES Module 1: Request the authentication result (TAG) or result IV read on the Public HIB |
| 31 | Reserved | |
| 32 | Reserved | |

---

## 6.5 PWM Events

### Table 6-3. Timer and eCAP Event Capture

| Event # | IP | Interrupt Name/Pin |
|---------|----|--------------------|
| 0 | For Timer 5 MUX input from IO signal TIMER5 | TIMER5 IO pin |
| 0 | For Timer 6 MUX input from IO signal TIMER6 | TIMER6 IO pin |
| 0 | For Timer 7 MUX input from IO signal TIMER7 | TIMER7 IO pin |
| 0 | For eCAP 0 MUX input from IO signal eCAP0 | eCAP0 IO pin |
| 0 | For eCAP 1 MUX input from IO signal eCAP1 | eCAP1 IO pin |
| 0 | For eCAP 2 MUX input from IO signal eCAP2 | eCAP2 IO pin |
| 1 | UART0 | UART0INT |
| 2 | UART1 | UART1INT |
| 3 | UART2 | UART2INT |
| 4 | UART3 | UART3INT |
| 5 | UART4 | UART4INT |
| 6 | UART5 | UART5INT |
| 7 | 3PGSW | 3PGSWRXTHR0 |
| 8 | 3PGSW | 3PGSWRXINT0 |
| 9 | 3PGSW | 3PGSWTXINT0 |
| 10 | 3PGSW | 3PGSWMISC0 |
| 11 | McASP0 | MCATXINT0 |
| 12 | McASP0 | MCARXINT0 |
| 13 | McASP1 | MCATXINT1 |
| 14 | McASP1 | MCARXINT1 |
| 15 | Reserved | Reserved |
| 16 | Reserved | Reserved |
| 17 | GPIO 0 | GPIOINT0A |
| 18 | GPIO 0 | GPIOINT0B |
| 19 | GPIO 1 | GPIOINT1A |
| 20 | GPIO 1 | GPIOINT1B |
| 21 | GPIO 2 | GPIOINT2A |
| 22 | GPIO 2 | GPIOINT2B |
| 23 | GPIO 3 | GPIOINT3A |
| 24 | GPIO 3 | GPIOINT3B |
| 25 | DCAN0 | DCAN0_INT0 |
| 26 | DCAN0 | DCAN0_INT1 |
| 27 | DCAN0 | DCAN0_PARITY |
| 28 | DCAN1 | DCAN1_INT0 |
| 29 | DCAN1 | DCAN1_INT1 |
| 30 | DCAN1 | DCAN1_PARITY |

---

## 6.6 Interrupt Controller Registers

> **NOTE:** FIQ is not available on general-purpose (GP) devices.

### 6.6.1 INTC Registers

Table 6-4 lists the memory-mapped registers for the INTC. All register offset addresses not listed in Table 6-4 should be considered as reserved locations and the register contents should not be modified.

### Table 6-4. INTC Registers

| Offset | Acronym | Register Name | Section |
|--------|---------|--------------|---------|
| 0h | INTC_REVISION | | 6.6.1.1 |
| 10h | INTC_SYSCONFIG | | 6.6.1.2 |
| 14h | INTC_SYSSTATUS | | 6.6.1.3 |
| 40h | INTC_SIR_IRQ | | 6.6.1.4 |
| 44h | INTC_SIR_FIQ | | 6.6.1.5 |
| 48h | INTC_CONTROL | | 6.6.1.6 |
| 4Ch | INTC_PROTECTION | | 6.6.1.7 |
| 50h | INTC_IDLE | | 6.6.1.8 |
| 60h | INTC_IRQ_PRIORITY | | 6.6.1.9 |
| 64h | INTC_FIQ_PRIORITY | | 6.6.1.10 |
| 68h | INTC_THRESHOLD | | 6.6.1.11 |
| 80h | INTC_ITR0 | | 6.6.1.12 |
| 84h | INTC_MIR0 | | 6.6.1.13 |
| 88h | INTC_MIR_CLEAR0 | | 6.6.1.14 |
| 8Ch | INTC_MIR_SET0 | | 6.6.1.15 |
| 90h | INTC_ISR_SET0 | | 6.6.1.16 |
| 94h | INTC_ISR_CLEAR0 | | 6.6.1.17 |
| 98h | INTC_PENDING_IRQ0 | | 6.6.1.18 |
| 9Ch | INTC_PENDING_FIQ0 | | 6.6.1.19 |
| A0h | INTC_ITR1 | | 6.6.1.20 |
| A4h | INTC_MIR1 | | 6.6.1.21 |
| A8h | INTC_MIR_CLEAR1 | | 6.6.1.22 |
| ACh | INTC_MIR_SET1 | | 6.6.1.23 |
| B0h | INTC_ISR_SET1 | | 6.6.1.24 |
| B4h | INTC_ISR_CLEAR1 | | 6.6.1.25 |
| B8h | INTC_PENDING_IRQ1 | | 6.6.1.26 |
| BCh | INTC_PENDING_FIQ1 | | 6.6.1.27 |
| C0h | INTC_ITR2 | | 6.6.1.28 |
| C4h | INTC_MIR2 | | 6.6.1.29 |
| C8h | INTC_MIR_CLEAR2 | | 6.6.1.30 |
| CCh | INTC_MIR_SET2 | | 6.6.1.31 |
| D0h | INTC_ISR_SET2 | | 6.6.1.32 |
| D4h | INTC_ISR_CLEAR2 | | 6.6.1.33 |
| D8h | INTC_PENDING_IRQ2 | | 6.6.1.34 |
| DCh | INTC_PENDING_FIQ2 | | 6.6.1.35 |
| E0h | INTC_ITR3 | | 6.6.1.36 |
| E4h | INTC_MIR3 | | 6.6.1.37 |
| E8h | INTC_MIR_CLEAR3 | | 6.6.1.38 |
| ECh | INTC_MIR_SET3 | | 6.6.1.39 |
| F0h | INTC_ISR_SET3 | | 6.6.1.40 |
| F4h | INTC_ISR_CLEAR3 | | 6.6.1.41 |
| F8h | INTC_PENDING_IRQ3 | | 6.6.1.42 |
| FCh | INTC_PENDING_FIQ3 | | 6.6.1.43 |
| 100h–2FCh | INTC_ILR_0 to INTC_ILR_127 | | 6.6.1.44 |

---

#### 6.6.1.1 INTC_REVISION Register (offset = 0h) [reset = 50h]

This register contains the IP revision code.

```
 31                                    8  7       4  3       0
┌────────────────────────────────────────┬──────────┬──────────┐
│              RESERVED                  │  Rev[7:4]│  Rev[3:0]│
│               R-0h                     │      R-50h          │
└────────────────────────────────────────┴──────────┴──────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-8 | RESERVED | R | 0h | Reads returns 0 |
| 7-0 | Rev | R | 50h | IP revision. [7:4] Major revision, [3:0] Minor revision. Examples: 0x10 for 1.0, 0x21 for 2.1 |

---

#### 6.6.1.2 INTC_SYSCONFIG Register (offset = 10h) [reset = 0h]

This register controls the various parameters of the OCP interface.

```
 31          5  4  3  2       1          0
┌─────────────┬────┬──┬───────┬──────────┐
│  RESERVED   │Rsv │Rsv│SoftReset│Autoidle│
│   R/W-0h    │R-0h│R/W│ R/W-0h │ R/W-0h │
└─────────────┴────┴──┴───────┴──────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-5 | RESERVED | R/W | 0h | |
| 4-3 | RESERVED | R | 0h | Write 0s for future compatibility. Reads returns 0 |
| 2 | RESERVED | R/W | 0h | |
| 1 | SoftReset | R/W | 0h | Software reset. Set this bit to trigger a module reset. The bit is automatically reset by the hardware. During reads, it always returns 0. |
| 0 | Autoidle | R/W | 0h | Internal OCP clock gating strategy. 0h = clkfree: OCP clock is free running. 1h = autoClkGate: Automatic OCP clock gating strategy is applied, based on the OCP interface activity |

---

#### 6.6.1.3 INTC_SYSSTATUS Register (offset = 14h) [reset = 0h]

This register provides status information about the module.

```
 31                              1     0
┌─────────────────────────────────┬─────────┐
│           RESERVED              │ResetDone│
│             R-0h                │   R-X   │
└─────────────────────────────────┴─────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-8 | RESERVED | R | 0h | |
| 7-1 | RESERVED | R | 0h | Reserved for OCP socket status information. Read returns 0 |
| 0 | ResetDone | R | X | Internal reset monitoring. 0h = rstOngoing: Internal module reset is on-going. 1h = rstComp: Reset completed |

---

#### 6.6.1.4 INTC_SIR_IRQ Register (offset = 40h) [reset = FFFFFF80h]

This register supplies the currently active IRQ interrupt number.

```
 31                              7  6             0
┌─────────────────────────────────┬────────────────┐
│         SpuriousIRQ             │   ActiveIRQ    │
│        R/W-1FFFFFFh             │    R/W-0h      │
└─────────────────────────────────┴────────────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-7 | SpuriousIRQ | R/W | 1FFFFFFh | Spurious IRQ flag |
| 6-0 | ActiveIRQ | R/W | 0h | Active IRQ number |

---

#### 6.6.1.5 INTC_SIR_FIQ Register (offset = 44h) [reset = FFFFFF80h]

This register supplies the currently active FIQ interrupt number.

```
 31                              7  6             0
┌─────────────────────────────────┬────────────────┐
│         SpuriousFIQ             │   ActiveFIQ    │
│        R-1FFFFFFh               │     R-0h       │
└─────────────────────────────────┴────────────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-7 | SpuriousFIQ | R | 1FFFFFFh | Spurious FIQ flag |
| 6-0 | ActiveFIQ | R | 0h | Active FIQ number |

---

#### 6.6.1.6 INTC_CONTROL Register (offset = 48h) [reset = 0h]

This register contains the new interrupt agreement bits.

```
 31                              2     1          0
┌─────────────────────────────────┬──────────┬──────────┐
│           RESERVED              │NewFIQAgr │NewIRQAgr │
│             R-0h                │  W-0h    │  W-0h    │
└─────────────────────────────────┴──────────┴──────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-2 | RESERVED | R | 0h | Write 0s for future compatibility. Reads returns 0 |
| 1 | NewFIQAgr | W | 0h | Reset FIQ output and enable new FIQ generation. 0h(Write) = no function effect. 1h(Write) = Reset FIQ output and enable new FIQ generation |
| 0 | NewIRQAgr | W | 0h | New IRQ generation. 0h(Write) = no function effect. 1h(Write) = Reset IRQ output and enable new IRQ generation |

---

#### 6.6.1.7 INTC_PROTECTION Register (offset = 4Ch) [reset = 0h]

This register controls protection of the other registers. This register can only be accessed in privileged mode, regardless of the current value of the protection bit.

```
 31                              1        0
┌─────────────────────────────────┬──────────┐
│           RESERVED              │Protection│
│             R-0h                │ R/W-0h   │
└─────────────────────────────────┴──────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-1 | RESERVED | R | 0h | Write 0s for future compatibility. Reads returns 0 |
| 0 | Protection | R/W | 0h | Protection mode. 0h = ProtMDis: Protection mode disabled (default). 1h = ProtMEnb: Protection mode enabled. When enabled, only privileged mode can access registers. |

---

#### 6.6.1.8 INTC_IDLE Register (offset = 50h) [reset = 0h]

This register controls the clock auto-idle for the functional clock and the input synchronizers.

```
 31                              2     1        0
┌─────────────────────────────────┬────────┬────────┐
│           RESERVED              │ Turbo  │FuncIdle│
│             R-0h                │R/W-0h  │R/W-0h  │
└─────────────────────────────────┴────────┴────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-2 | RESERVED | R | 0h | Write 0s for future compatibility. Reads returns 0 |
| 1 | Turbo | R/W | 0h | Input synchronizer clock auto-gating. 0h = SyncFree: Input synchronizer clock is free running (default). 1h = SyncAuto: Input synchronizer clock is auto-gated based on interrupt input activity |
| 0 | FuncIdle | R/W | 0h | Functional clock auto-idle mode. 0h = FuncAuto: Functional clock gating strategy is applied (default). 1h = FuncFree: Functional clock is free-running |

---

#### 6.6.1.9 INTC_IRQ_PRIORITY Register (offset = 60h) [reset = FFFFFFC0h]

This register supplies the currently active IRQ priority level.

```
 31                              7  6             0
┌─────────────────────────────────┬────────────────┐
│       SpuriousIRQflag           │  IRQPriority   │
│        R-1FFFFFFh               │     R-40h      │
└─────────────────────────────────┴────────────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-7 | SpuriousIRQflag | R | 1FFFFFFh | Spurious IRQ flag |
| 6-0 | IRQPriority | R | 40h | Current IRQ priority |

---

#### 6.6.1.10 INTC_FIQ_PRIORITY Register (offset = 64h) [reset = FFFFFFC0h]

This register supplies the currently active FIQ priority level.

```
 31                              7  6             0
┌─────────────────────────────────┬────────────────┐
│       SpuriousFIQflag           │  FIQPriority   │
│        R-1FFFFFFh               │     R-40h      │
└─────────────────────────────────┴────────────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-7 | SpuriousFIQflag | R | 1FFFFFFh | Spurious FIQ flag |
| 6-0 | FIQPriority | R | 40h | Current FIQ priority |

---

#### 6.6.1.11 INTC_THRESHOLD Register (offset = 68h) [reset = FFh]

This register sets the priority threshold.

```
 31                      8  7                    0
┌─────────────────────────┬───────────────────────┐
│       RESERVED          │  PriorityThreshold    │
│         R-0h            │      R/W-FFh          │
└─────────────────────────┴───────────────────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-8 | RESERVED | R | 0h | Reads returns 0 |
| 7-0 | PriorityThreshold | R/W | FFh | Priority threshold. Values used are 00h to 3Fh. Value FFh disables the threshold. |

---

#### 6.6.1.12 INTC_ITR0 Register (offset = 80h) [reset = 0h]

This register shows the raw interrupt input status before masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | Itr | R | X | Interrupt status before masking |

---

#### 6.6.1.13 INTC_MIR0 Register (offset = 84h) [reset = FFFFFFFFh]

This register contains the interrupt mask.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | Mir | R/W | FFFFFFFFh | Interrupt mask |

---

#### 6.6.1.14 INTC_MIR_CLEAR0 Register (offset = 88h) [reset = 0h]

This register is used to clear the interrupt mask bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | MirClear | W | X | Write 1 clears the mask bit to 0, reads return 0 |

---

#### 6.6.1.15 INTC_MIR_SET0 Register (offset = 8Ch) [reset = 0h]

This register is used to set the interrupt mask bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | MirSet | W | X | Write 1 sets the mask bit to 1, reads return 0 |

---

#### 6.6.1.16 INTC_ISR_SET0 Register (offset = 90h) [reset = 0h]

This register is used to set the software interrupt bits. It is also used to read the currently active software interrupts.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | IsrSet | R/W | 0h | Reads returns the currently active software interrupts, Write 1 sets the software interrupt bits to 1 |

---

#### 6.6.1.17 INTC_ISR_CLEAR0 Register (offset = 94h) [reset = 0h]

This register is used to clear the software interrupt bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | IsrClear | W | X | Write 1 clears the software interrupt bits to 0, reads return 0 |

---

#### 6.6.1.18 INTC_PENDING_IRQ0 Register (offset = 98h) [reset = 0h]

This register contains the IRQ status after masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | PendingIRQ | R | 0h | IRQ status after masking |

---

#### 6.6.1.19 INTC_PENDING_FIQ0 Register (offset = 9Ch) [reset = 0h]

This register contains the FIQ status after masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | PendingFIQ | R | 0h | FIQ status after masking |

---

#### 6.6.1.20 INTC_ITR1 Register (offset = A0h) [reset = 0h]

This register shows the raw interrupt input status before masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | Itr | R | X | Interrupt status before masking |

---

#### 6.6.1.21 INTC_MIR1 Register (offset = A4h) [reset = FFFFFFFFh]

This register contains the interrupt mask.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | Mir | R/W | FFFFFFFFh | Interrupt mask |

---

#### 6.6.1.22 INTC_MIR_CLEAR1 Register (offset = A8h) [reset = 0h]

This register is used to clear the interrupt mask bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | MirClear | W | X | Write 1 clears the mask bit to 0, reads return 0 |

---

#### 6.6.1.23 INTC_MIR_SET1 Register (offset = ACh) [reset = 0h]

This register is used to set the interrupt mask bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | MirSet | W | X | Write 1 sets the mask bit to 1, reads return 0 |

---

#### 6.6.1.24 INTC_ISR_SET1 Register (offset = B0h) [reset = 0h]

This register is used to set the software interrupt bits. It is also used to read the currently active software interrupts.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | IsrSet | R/W | 0h | Reads returns the currently active software interrupts, Write 1 sets the software interrupt bits to 1 |

---

#### 6.6.1.25 INTC_ISR_CLEAR1 Register (offset = B4h) [reset = 0h]

This register is used to clear the software interrupt bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | IsrClear | W | X | Write 1 clears the software interrupt bits to 0, reads return 0 |

---

#### 6.6.1.26 INTC_PENDING_IRQ1 Register (offset = B8h) [reset = 0h]

This register contains the IRQ status after masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | PendingIRQ | R | 0h | IRQ status after masking |

---

#### 6.6.1.27 INTC_PENDING_FIQ1 Register (offset = BCh) [reset = 0h]

This register contains the FIQ status after masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | PendingFIQ | R | 0h | FIQ status after masking |

---

#### 6.6.1.28 INTC_ITR2 Register (offset = C0h) [reset = 0h]

This register shows the raw interrupt input status before masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | Itr | R | X | Interrupt status before masking |

---

#### 6.6.1.29 INTC_MIR2 Register (offset = C4h) [reset = FFFFFFFFh]

This register contains the interrupt mask.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | Mir | R/W | FFFFFFFFh | Interrupt mask |

---

#### 6.6.1.30 INTC_MIR_CLEAR2 Register (offset = C8h) [reset = 0h]

This register is used to clear the interrupt mask bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | MirClear | W | X | Write 1 clears the mask bit to 0, reads return 0 |

---

#### 6.6.1.31 INTC_MIR_SET2 Register (offset = CCh) [reset = 0h]

This register is used to set the interrupt mask bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | MirSet | W | X | Write 1 sets the mask bit to 1, reads return 0 |

---

#### 6.6.1.32 INTC_ISR_SET2 Register (offset = D0h) [reset = 0h]

This register is used to set the software interrupt bits. It is also used to read the currently active software interrupts.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | IsrSet | R/W | 0h | Reads returns the currently active software interrupts, Write 1 sets the software interrupt bits to 1 |

---

#### 6.6.1.33 INTC_ISR_CLEAR2 Register (offset = D4h) [reset = 0h]

This register is used to clear the software interrupt bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | IsrClear | W | X | Write 1 clears the software interrupt bits to 0, reads return 0 |

---

#### 6.6.1.34 INTC_PENDING_IRQ2 Register (offset = D8h) [reset = 0h]

This register contains the IRQ status after masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | PendingIRQ | R | 0h | IRQ status after masking |

---

#### 6.6.1.35 INTC_PENDING_FIQ2 Register (offset = DCh) [reset = 0h]

This register contains the FIQ status after masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | PendingFIQ | R | 0h | FIQ status after masking |

---

#### 6.6.1.36 INTC_ITR3 Register (offset = E0h) [reset = 0h]

This register shows the raw interrupt input status before masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | Itr | R | X | Interrupt status before masking |

---

#### 6.6.1.37 INTC_MIR3 Register (offset = E4h) [reset = FFFFFFFFh]

This register contains the interrupt mask.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | Mir | R/W | FFFFFFFFh | Interrupt mask |

---

#### 6.6.1.38 INTC_MIR_CLEAR3 Register (offset = E8h) [reset = 0h]

This register is used to clear the interrupt mask bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | MirClear | W | X | Write 1 clears the mask bit to 0, reads return 0 |

---

#### 6.6.1.39 INTC_MIR_SET3 Register (offset = ECh) [reset = 0h]

This register is used to set the interrupt mask bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | MirSet | W | X | Write 1 sets the mask bit to 1, reads return 0 |

---

#### 6.6.1.40 INTC_ISR_SET3 Register (offset = F0h) [reset = 0h]

This register is used to set the software interrupt bits. It is also used to read the currently active software interrupts.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | IsrSet | R/W | 0h | Reads returns the currently active software interrupts, Write 1 sets the software interrupt bits to 1 |

---

#### 6.6.1.41 INTC_ISR_CLEAR3 Register (offset = F4h) [reset = 0h]

This register is used to clear the software interrupt bits.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | IsrClear | W | X | Write 1 clears the software interrupt bits to 0, reads return 0 |

---

#### 6.6.1.42 INTC_PENDING_IRQ3 Register (offset = F8h) [reset = 0h]

This register contains the IRQ status after masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | PendingIRQ | R | 0h | IRQ status after masking |

---

#### 6.6.1.43 INTC_PENDING_FIQ3 Register (offset = FCh) [reset = 0h]

This register contains the FIQ status after masking.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | PendingFIQ | R | 0h | FIQ status after masking |

---

#### 6.6.1.44 INTC_ILR_0 to INTC_ILR_127 Register (offset = 100h to 2FCh) [reset = 0h]

The INTC_ILRx registers contain the priority for the interrupts and the FIQ/IRQ steering.

```
 31                      8  7       2     1        0
┌─────────────────────────┬──────────┬────────┬────────┐
│       RESERVED          │ Priority │RESERVED│FIQnIRQ │
│         R-0h            │ R/W-0h   │R/W-0h  │R/W-0h  │
└─────────────────────────┴──────────┴────────┴────────┘
```

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-8 | RESERVED | R | 0h | Write 0s for future compatibility. Reads returns 0 |
| 7-2 | Priority | R/W | 0h | Interrupt priority |
| 1 | RESERVED | R/W | 0h | |
| 0 | FIQnIRQ | R/W | 0h | Interrupt IRQ/FIQ mapping. 0h = IntIRQ: Interrupt is routed to IRQ. 1h = IntFIQ: Interrupt is routed to FIQ (this selection is reserved on GP devices). |
