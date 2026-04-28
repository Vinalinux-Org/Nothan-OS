# 12 Touchscreen Controller

This chapter describes the touchscreen controller of the device.

| 12.1<br>12.2<br>12.3 | Introduction<br>Integration<br>Functional Description | 1828<br>1829<br>1831 |
|----------------------|-------------------------------------------------------|----------------------|
| 12.4                 | Operational Modes                                     | 1834                 |
| 12.5                 | Touchscreen Controller Registers                      | 1838                 |

## **12.1 Introduction**

The touchscreen controller and analog-to-digital converter subsystem (TSC\_ADC\_SS) is an 8-channel general-purpose analog-to-digital converter (ADC) with optional support for interleaving touchscreen (TS) conversions for a 4-wire, 5-wire, or 8-wire resistive panel. The TSC\_ADC\_SS can be configured for use in one of the following applications:

- 8 general-purpose ADC channels
- 4-wire TSC with 4 general-purpose ADC channels
- 5-wire TSC with 3 general-purpose ADC channels
- 8-wire TSC

### *12.1.1 TSC\_ADC Features*

The main features of the TSC\_ADC\_SS include:

- Support for 4-wire, 5-wire, and 8-wire resistive TS panels
- Support for interleaving TS capture and general-purpose ADC modes
- Programmable FSM sequencer that supports 16 steps:
  - Software register bit for start of conversion
  - Optional start of conversion HW synchronized to Pen touch or external HW event (but not both)
  - Single conversion (one-shot)
  - Continuous conversions
  - Sequence through all input channels based on a mask
  - Programmable OpenDelay before sampling each channel
  - Programmable sampling delay for each channel
  - Programmable averaging of input samples 16/8/4/2/1
  - Differential or singled ended mode setting for each channel
  - Store data in either of two FIFO groups
  - Option to encode channel number with data
  - Support for servicing FIFOs via DMA or CPU
  - Programmable DMA Request event (for each FIFO)
  - Dynamically enable or disable channel inputs during operation
  - Stop bit to end conversion
- Support for the following interrupts and status, with masking:
  - Interrupt for HW pen (touch) event
  - Interrupt for HW Pen Up event
  - Interrupt after a sequence of conversions (all non-masked channels)
  - Interrupt for FIFO threshold levels
  - Interrupt if sampled data is out of a programmable range
  - Interrupt for FIFO overflow and underflow conditions
  - Status bit to indicate if ADC is busy converting

### *12.1.2 Unsupported TSC\_ADC\_SS Features*

This device supports all TSC\_ADC\_SS features.

## 12.2 Integration

Figure 12-1 shows the integration of the TSC\_ADC module in the device.

Figure 12-1. TSC\_ADC Integration

```
CLK_M_OSC
   |
   v
+--------+                          +-----------------------------------------------+
|  PRCM  |---- pd_wkup_adc_fclk --->|             TSC_ADC Subsystem                 |
+--------+                          |                                               |
                                    |  adc_clk                                      |
L3Slow Interconnect <-------------->|  +---------------------------+                |
                                    |  | DMA SLV OCP2VBus Bridge   |                |
L4Wakeup Interconnect <------------>|  +---------------------------+                |
                                    |  +---------------------------+                |
                                    |  | MMA SLV OCP2VBus Bridge   |                |
                                    |  +---------------------------+                |
                                    |                 |                             |
                                    |                 v                             |
                                    |        +----------------+                     |
                                    |        |      SCR       |                     |
                                    |        | (M/S ports)    |                     |
                                    |        +----------------+                     |
                                    |          |       |       \                    |
                                    |          |       |        \                   |
                                    |          |       |         v                  |
                                    |          |       |     +--------+             |
                                    |          |       +---->| FIFO1  | 64x16-bit   |
                                    |          |             +--------+             |
                                    |          |             +--------+             |
                                    |          +------------>| FIFO0  | 64x16-bit   |
                                    |                        +--------+             |
                                    |                            |   ^              |
                                    |                            v   |              |
                                    |                   +-------------------+       |
                                    |                   |   Sequencer FSM   |<----->|
                                    |                   +-------------------+       |
                                    |                            |                  |
                                    |                            v                  |
                                    |                     +-----------+             |
                                    |                     |   AFE     |             |
                                    |                     |   ADC     |<----------> |
                                    |                     +-----------+    TSC_ADC  |
                                    |                                      Pads     |
                                    |                                     AN[7:0]   |
                                    |                                               |
                                    |   +-----+                                     |
                                    |   | IPG |<----- gen_intr ---------------------+--> MPU Subsystem,
                                    |   +-----+          fifo0_dreq --------------------> PRU-ICSS, WakeM3
                                    |                    fifo1_dreq --------------------> EDMA
                                    |                                               |
                                    |  ext_hw_event <--------------------------------+
                                    +-----------------------------------------------+

+-------------------------------+        (event mux)        ADV_EVTCAPT[3:0]
| TIMER7  point_pend            |----\         +-----+           |
| TIMER6  point_pend            |-----\------> | 4   |           v
| TIMER5  point_pend            |------\       | 3   |    +----------------+
| TIMER4  point_pend            |-------\----> | 2   |--->| Control Module |
| PRU-ICSS pr1_host_intr0       |--------\---> | 1   |    |  ADC_EVT_CAPT  |
+-------------------------------+         \--->| 0   |    +----------------+
                                               +-----+
```

pr1\_host\_intr[0:7] corresponds to Host-2 to Host-9 of the PRU-ICSS interrupt controller.

### 12.2.1 TSC\_ADC Connectivity Attributes

The general connectivity attributes for the TSC\_ADC module are summarized in Table 12-1.

Table 12-1. TSC\_ADC Connectivity Attributes

| Attributes          | Type                                                                                |
|---------------------|-------------------------------------------------------------------------------------|
| Power domain        | Wakeup Domain                                                                       |
| Clock domain        | PD_WKUP_L4_WKUP_GCLK (OCP) PD_WKUP_ADC_FCLK (Func)                                  |
| Reset signals       | WKUP_DOM_RST_N                                                                      |
| Idle/Wakeup signals | Smart idle<br>Wakeup                                                                |
| Interrupt request   | 1 interrupt to MPU Subsystem (ADC_TSC_GENINT), PRU-ICSS (gen_intr_pend), and WakeM3 |
| DMA request         | 2 Events (tsc_adc_FIFO0, tsc_adc_FIFO1)                                             |
| Physical address    | L3 Slow slave port (DMA)<br>L4 Wkup slave port (MMR)                                |

### *12.2.2 TSC\_ADC Clock and Reset Management*

The TSC\_ADC has two clock domains. The ADC uses the adc\_clk. The bus interfaces, FIFOs, sequencer, and all other lofic use the ocp\_clk.

#### **Table 12-2. TSC\_ADC Clock Signals**

| Clock Signal                      | Max Freq  | Reference / Source | Comments                           |
|-----------------------------------|-----------|--------------------|------------------------------------|
| ocp_clk<br>OCP / Functional clock | 100 MHz   | CORE_CLKOUTM4 / 2  | pd_pwkup_l4_wkup_gclk<br>From PRCM |
| adc_clk<br>ADC clock              | 24 MHz(1) | CLK_M_OSC          | pd_wkup_adc_fclk<br>From PRCM      |

<sup>(1)</sup> When using master input clock frequencies (CLK\_M\_OSC) above 24 MHz (that is, 25 or 26 MHz), the ADC clock must be divided down using the ADC\_CLKDIV register, which will reduce the maximum potential sample rate. The maximum sample rate can only be achieved using a 24-MHz master input clock.

### *12.2.3 TSC\_ADC Pin List*

The TSC\_ADC external interface signals are shown in [Table](#page-3-0) 12-3.

### **Table 12-3. TSC\_ADC Pin List**

| Pin     | Type  | Description                              |
|---------|-------|------------------------------------------|
| AN[7:0] | I     | Analog Input                             |
| VREFN   | Power | Analog Reference Input Negative Terminal |
| VREFP   | Power | Analog Reference Input Positive Terminal |

## **12.3 Functional Description**

Before enabling the TSC\_ADC\_SS module, the user must first program the Step Configuration registers in order to configure a channel input to be sampled. There are 16 programmable Step Configuration registers which are used by the sequencer to control which switches to turn on or off (inputs to the AFE), which channel to sample, and which mode to use (hardware-triggered or software-enabled, one-shot or continuous, averaging, where to save the FIFO data, and more).

### *12.3.1 Hardware-Synchronized or Software-Enabled*

The user can control the start behavior of each step by deciding if a channel should be sampled immediately (software-enabled) after it is enabled, or if the channel should wait for a hardware (HW) event to occur first (a HW event must either be mapped to the touch screen PEN event or mapped to the HW event input signal, but not both). Each step can be configured independently using the STEPCONFIGx register.

### *12.3.2 Open Delay and Sample Delay*

The user can program the delay between driving the inputs to the AFE and the time to send the start of conversion signal. This delay can be used to allow the voltages to stabilize on the touch screen panel before sampling. This delay is called "open delay" and can also be programmed to zero. The user also has control of the sampling time (width of the start of conversion signal) to the AFE which is called the "sample delay". The open delay and sample delay for each step can be independently configured using the STEPDELAYx register.

### *12.3.3 Averaging of Samples (1, 2, 4, 8, and 16)*

Each step has the capability to average the sampled data. The valid averaging options are 1 (no average), 2, 4, 8, and 16. If averaging is turned on, then the channel is immediately sampled again (up to 16 times) and final averaged sample data is stored in the FIFO. Each step can be independently configured using the STEPCONFIGx registers.

### *12.3.4 One-Shot (Single) or Continuous Mode*

When the sequencer finishes cycling through all the enabled steps, the user can decide if the sequencer should stop (one-shot), or loop back and schedule the step again (continuous).

If one-shot mode is selected, the sequencer will take care of disabling the step enable bit after the conversion. If continuous mode is selected, it is the software's responsibility to turn off the step enable bit.

### *12.3.5 Interrupts*

The following interrupts are supported through enable bits and are maskable.

The HW Pen event interrupt, also known as the Pen-down interrupt, is generated when the user presses the touchscreen. This can only occur if the AFE is configured properly (that is, one of the Pen Ctrl bits must be enabled, and also the correct setting for a path to ground in the STEPCONFIGx registers). Although the HW Pen interrupt can be disabled by the software (SW), the event will still trigger the sequencer to start if the step is configured as a HW-synchronized event. The Pen-down interrupt is an asynchronous event and can be used even if the TSC\_ADC\_SS clocks are disabled. The Pen-down interrupt can be used as a wakeup source.

An END\_OF\_SEQUENCE interrupt is generated after the sequencer finishes servicing the last enabled step.

A Pen-up event interrupt, also known as the Pen-up interrupt, can only be generated when using HW steps with the charge steps enabled. If a Pen-down event caused the HW steps to be scheduled and no Pen-down is present after the sequencer finished servicing the charge step, then a Pen-up interrupt is generated. To detect Pen-up interrupts, the charge step must share the same configuration as the idle step.

Each FIFO has support for generating interrupts when the FIFO word count has reached a programmable threshold level. The user can program the desired word count at which the CPU should be interrupted. Whenever the threshold counter value is reached, it sets the FIFOxTHRESHOLD interrupt flag, and the CPU is interrupted if the FIFOxTHRESHOLD interrupt enable bit is set. The user can clear the interrupt flag, after emptying the FIFO, by writing a '1' to the FIFOxTHRESHOLD interrupt status bit. To determine how many samples are currently in the FIFO at a given moment, the FIFOxCOUNT register can be read by the CPU.

The FIFO can also generate FIFOx\_OVERRUN and FIFOx\_UNDERFLOW interrupts. The user can mask these events by programming the IRQENABLE\_CLR register. To clear a FIFO underflow or FIFO overrun interrupt, the user should write a '1' to the status bit in the IRQSTS register. The TSC\_ADC\_SS does not recover from these conditions automatically. Therefore, the software will need to disable and then again enable the TSC\_ADC\_SS. Before the user can turn the module back on, the user must read the ADCSTAT register to check if the status of the ADC FSM is idle and the current step is the idle step.

### *12.3.6 DMA Requests*

Each FIFO group can be serviced by either a DMA or by the CPU. To generate DMA requests, the user must set the enable bit in the DMAENABLE\_SET Register. Also, the user can program the desired number of words to generate a DMA request using the DMAxREQ register. When the FIFO level reaches or exceeds that value, a DMA request is generated.

The DMA slave port allows for burst reads in order to effectively move the FIFO data. Internally, the OCP DMA address (MSB) is decoded for either FIFO 0 or FIFO 1. The lower bits of the DMA addresses are ignored since the FIFO pointers are incremented internally.

### *12.3.7 Analog Front End (AFE) Functional Block Diagram*

The AFE features are listed below, and some are controlled by the TSC\_ADC\_SS:

- 12-bit ADC
- Sampling rate can be as fast as every 15 ADC clock cycles
- Support for internal ADC clock divider logic
- Support for configuring the delay between samples also the sampling time

```
                 (HHV / VDDA domain)
VREFP  o--[buf]--------------------+-------------------------------+
VREFN  o---------------------------|-------------------------------|
INTREF o (no internal connection)  |                               |
                                   |     (switching/network)       |
XPUL/AN0 o-------------------------+----.                          |
XNUR/AN1 o------------------------------|                          |
YPUL/AN2 o------------------------------|-----.                    |
YNLR/AN3 o------------------------------|-----|-----.              |
AN4     o------------------------------ |     |     |              |
AN5     o-------------------------------+-----+-----+-----.        |
AN6     o----------------------------------------------+--|-----.  |
AN7     o----------------------------------------------+--+-----|--+
                                                    (selected lines)
                     ^ YPPSW / XNPSW / XPPSW (touch switch controls)

                         SEL_INP<3:0>                     SEL_RFP<2:0>
                              |                                |
                              v                                v
                        +-------------+                    +----------+
                        |  AMUX 9:1   |------------------->| AMUX 5:1 |----+
                        +-------------+        INP         +----------+    |
                              ^                                   |        |
                              |                                   | INT_VREFP
                         (from XPUL/XNUR/YPUL/YNLR/AN4..AN7 etc)  v        |
                                                                    +------v------+
                         SEL_INM<3:0>                               |     ADC     |
                              |                                     |             |
                              v                                     +------^------+
                        +-------------+        INM                         |
                        |  AMUX 9:1   |-----------------------------------+
                        +-------------+
                              ^
                              | (VREFN can be selected)

                       SEL_RFM<1:0>
                            |
                            v
                         +----------+
                         | AMUX 4:1 |----> VREFM ----> (to ADC)
                         +----------+
                         inputs: VSSA, XNUL, YNLR, VREFN(1)

ADC outputs / controls (right side):
- ADCOUT<11:0>
- EOC
- CLK
- PD
- DIFF_CNTRL
- START

Pen & IRQ:
                     +------------------+
SEL_RFP<2:0> ------->| Pen & IRQ Control|----> PENIRQ<1:0>
                     +------------------+----> PENCTR<1:0>

Bias:
                 +----------+     ADC_BIAS
REXT_BIAS (no internal connection)  |
                 |                  v
                 +--------------->+----------+     BIAS_SEL
                                  | AMUX 2:1 |----> (to pads)
                                  +----------+
                                        ^
                                        |
                                  +-------------+
                                  | Internal    |
                                  | Bias        |
                                  +-------------+

Supplies/pads (blue pins are I/O pads):
- VSSA_ADC(1), VDDA_ADC(1)
- VSSA, VDDA
- WPPNSW, YNNSW, YPNSW, XNNSW (bottom switch pins)
```
Blue pins are I/O pads

Figure 12-2. Functional Block Diagram

- (1) In the device-specific datasheet:
- VDDA\_ADC and VSSA\_ADC are referred to as "Internal References"
- VREFP and VREFN are referred to as "External References"

## **12.4 Operational Modes**

The sequencer is completely controlled by software and behaves accordingly to how the **Step Registers** are programmed. A **step** is the general term for sampling a channel input. It is defined by the programmer who decides which input values to send to the AFE as well as how and when to sample a channel input.

The choices for each step can all be programmed using the STEPCONFIGx registers.

A step requires using these registers:

- STEPENABLE: Enables or disables the step
- STEPCONFIGx: Controls the input values to the ADC (the reference voltages, the pull up/down transistor biasing, which input channel to sample, differential control, HW synchronized or SW enabled, averaging, and which FIFO group to save the data)
- STEPDELAYx: Controls the OpenDelay (the time between driving the AFE inputs until sending the SOC signal to the AFE), and also controls the SampleDelay (the time for the ADC to sample the input signal)

The sequencer supports a total of 16 programmable steps, a touchscreen charge step, and an idle step. Each step requires using the registers listed above. However, the idle step does not have an enable bit, so it will always be enabled, or a delay register. In addition, the ADC does not actually sample a channel during the idle and touchscreen charge steps.

Assuming all the steps are configured as general-purpose mode (no touchscreen), then the steps would be configured as SW enabled. When the TSC\_ADC\_SS is first enabled, the sequencer will always start in the Idle step and then wait for a STEPENABLE[n] bit to turn on. After a step is enabled, the sequencer will start with the lowest step (1) and continue until step (16). If a step is not enabled, then sequencer will skip to the next step. If all steps are disabled, then the sequencer will remain in the IDLE state and continue to apply the settings in the IDLECONFIG register.

Assuming a touchscreen-only mode (no general-purpose channels) the steps could be configured as HW synchronized triggered (mapped to the Pen event). The sequencer would wait in the IDLE state until a Pen-down event occurred and then begin the HW step conversions. The charge step, which occurs after the last HW-synchronized step is finished, is designed to charge the capacitance in a touch panel when the appropriate bias transistor is enabled. The purpose of the charge step is to prepare the TSC for the next Pen-down event.

Assuming a mixed mode application (touchscreen and general-purpose channels), the user can configure the steps as either HW-triggered (mapped to a Pen event) or SW-enabled. If the sequencer is in the idle state and a Pen-down event occurs, the HW-synchronized steps are always scheduled first, followed by the charge step, and cannot be preempted by SW steps. If there is no HW event, then the SW-enabled steps are scheduled instead.

If a Pen-down event occurs while the sequencer is in the middle of scheduling the SW steps, the user can program the scheduler to allow preemption. If the HW preemption control bit is enabled in the CTRL register, the sequencer will stop the scheduled SW sequence and schedule the HW steps. After the last HW step and charge step are completed, the sequencer will continue from the next SW step (before the preemption occurred). If the HW preemption is disabled, then the touch event will be ignored until the last software step is completed; if the touch event is removed before the last software step is finished, then the touch event will be missed.

Even if a touchscreen is not present, the user can still configure the steps to be HW-synchronized by mapping to the ext\_hw\_event signal shown in [Figure](#page-2-1) 12-1. This HW event input signal can be driven at the SOC from a number of external inputs chosen by the ADC\_EVT\_CAPT register in the Control Module.

When mapping is set for the ext\_hw\_event signal, then the TSC\_ADC\_SS will wait for a rising edge transition (from low to high) before starting. The ext\_hw\_event signal is captured on the internal L4 OCP clock domain. The ext\_hw\_event signal should be held for at least two TSC\_ADC\_SS OCP clocks (L4 frequency).

An END\_OF\_SEQUENCE interrupt is generated after the last active step is completed before going back to the IDLE state. The END\_OF\_SEQUENCE interrupt does not mean data is in the FIFO (should use the FIFO interrupts and FIFOxCOUNT register).

### *12.4.1 PenCtrl and PenIRQ*

The Pen IRQ can only occur if the correct AFE\_Pen\_Ctrl bits are high in the CTRL register and if the correct ground transistor biasing is set in the STEPCONFIGx and IDLECONFIG registers.

Setting the AFE\_Pen\_Ctrl bits in the CTRL register will enable a weak internal pull-up resistor on AIN0 for 4-wire configurations and AIN4 for 5-wire.

If a step is configured as HW-synchronized, the sequencer will override the AFE\_Pen\_Ctrl bits set by the software (bits 6:5) once it transitions from the Idle step. The sequencer will automatically mask the AFE\_Pen\_Ctrl bits (override them and turn them off) so that the ADC can get an accurate measurement from the x and y-axes. After the last HW-synchronized step, the sequence will go to the Charge step and the pen override mask is removed and the values set by the software (bits 6:5) will have control. The Pendown events will be temporarily ignored during the Charge step (HW will mask any potential glitches that may occur)

If the sequencer is not using the HW synchronized approach, (all the steps are configured as software enabled), then it is the software programmer's responsibility to correctly turn on and off the AFE\_Pen\_Ctrl bits to receive the correct measurements from the touchscreen. The software must enable the Charge step and ignore any potential glitches.

It is also possible to detect the Pen-down event even if all the STEPENABLE[n] bits are off. By setting the appropriate AFE\_Pen\_Ctrl bit to 1, and configuring the IDLECONFIG register to bias the correct transistor to ground, the Pen-down event will generate. The flowchart for the sequencer is shown in [Figure](#page-9-0) 12-3 and an example timing diagram in [Figure](#page-10-0) 12-4.

```
Figure 12-3. Sequencer FSM (text)

+----------------------------------------------------+
| IDLE  (apply Idle Step Config)                     |
|  - Set N=0                                         |
|  - Set pen_down_flag = 1                           |
|  - Set pen_override_mask = 1                       |
|  - Ignore pen IRQs                                 |
+-----------------------------+----------------------+
                              |
                              v
                   +--------------------+
                   | StepEnable[N] = 1? |
                   +----+-----------+---+
                        |Yes        |No
                        v           v
                 +-------------+   +----------------------------+
                 | HW event?   |   | Update Shadow StepEnable   |
                 +--+-------+--+   | Reg                        |
                    |Yes    |No    +----------------------------+
                    v       v
      +-------------------+   +--------------------+
      | If HW[N] and      |   | Is any SW?         |
      | StepEnable[N] ?   |   +----+-----------+---+
      +----+----------+---+        |Yes        |No
           |Yes       |No          v           v
           v          |       +---------------------+
 +------------------+ |       | If sw[N] and        |
 | Apply StepConfig | |       | StepEnable[N] ?     |
 | [N]              | |       +----+-----------+----+
 +------------------+ |            |Yes        |No
           |          |            v           v
           v          |    +------------------+   +---------+
 +------------------+ |    | Apply StepConfig |   | Incr N  |
 | Wait OpenDelay   | |    | [N]              |   +----+----+
 | [N]              | |    +------------------+       |
 +------------------+ |            |                  |
           |          |            v                  |
           v          |    +------------------+       |
 +------------------+ |    | Wait OpenDelay   |       |
 | Wait SampleDelay | |    | [N]              |       |
 | [N]              | |    +------------------+       |
 +------------------+ |            |                  |
           |          |            v                  |
           v          |    +------------------+       |
 +------------------+ |    | Wait SampleDelay |       |
 | ADC Conversion   | |    | [N]              |       |
 +------------------+ |    +------------------+       |
           |          |            |                  |
           v          |            v                  |
     +-----------+    |    +------------------+       |
     | AVG[N]?   |----+    | ADC Conversion   |       |
     +--+-----+--+ Yes     +------------------+       |
        |No   |                                   (loop)
        v     v
 (Reset StepEnable[n] if One-Shot[n])        +-----------+
        |                                    | AVG[N]?   |
        v                                    +--+-----+--+
+----------------------------+                  |No   |Yes
| Looped all enabled HW      |<-----------------+     |
| steps?                     |                        v
+----+-------------------+---+          (Reset StepEnable[n] if One-Shot[n])
     |Yes                |No                             |
     v                   |                               v
Reset pen_override_mask  |                   +----------------------------+
     |                   |                   | Looped all enabled SW      |
     v                   |                   | steps?                     |
+----------------------------------------------------+----+-----------+---+
| If TSC Charge step enabled: apply Charge StepConfig|    |Yes        |No |
|  and OpenDelay (ignore any pen irq during this)    |    v           |   |
| If pen_down_flag==1 and now pen is up: generate    | +------------------+
|  PEN_UP interrupt and reset pen_down_flag          | | Generate END_OF_ |
+----------------------------------------------------+ | SEQUENCE int     |
                                                       +------------------+

Preemption path (center logic):
- If "Preemption enabled?" = Yes:
    Set preempt_flag = 1; Save N
- When SW exists:
    If preempt_flag = 1 -> increment N
    else set N to first SW Stepconfig
    Reset preempt_flag
- Bottom note:
    If preempt_flag is 1, restore N; else set N to first SW Stepconfig
```
<sup>\*</sup> HW event can either be Pen-down or input HW event, but not both

[Figure](#page-9-0) 12-3 does not actually represent clock cycles but instead illustrates how the scheduler will work. However, each shaded box above does represent a FSM state and will use a clock cycle. Using the minimum delay values, the ADC can sample at 15 ADC clocks per sample. Below is an example timing diagram illustrating the states of the sequencer and also the showing when the STEPCONFIGx and the STEPDELAYx registers values are applied. The below example assumes the steps are software controlled, and averaging is turned off.

```
ADC timing (ASCII)

Enable     _________‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾

FSM        [ Idle ]--[ Open ]-[ SOC ]------[ Wait for EOC ]------[ Open ]-[ SOC ]--

           Apply IdleConfig
                      Apply STEPCONFIGx & STEPDELAYx                  Apply STEPCONFIGx & STEPDELAYx

                         <--- OpenDelay[N] --->                    <--- OpenDelay[N+1] --->

                             <--- SampleDelay[N] --->                   <--- SampleDelay[N+1] --->

Data[11:0]  XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|<---- 13 clock cycles ---->|==== Data Valid ====|XXXXXXXXXXXXXXX
                                            (after EOC edge; shown for the 2nd conversion window)
``
```

**Figure 12-4. Example Timing Diagram for Sequencer**

The idle step is always enabled and applied when the FSM is in the IDLE state (that is, either waiting for a HW event or waiting for a step to be enabled). The idle step cannot be disabled.

Once the TSC\_ADC\_SS is enabled and assuming at least one step is active, the FSM will transition from the idle state and apply the first active STEPCONFIGx and STEPDELAYx register settings. It is possible for the Open Delay value to be 0, and the FSM will immediately skip to the Sample Delay state, which is a minimum of one clock cycle. The ADC will begin the sampling on the falling edge of the SOC signal. After the ADC is finished converting the channel data (13 cycles later), the EOC signal is sent and the FSM will then apply the next active step.

This process is repeated and continued (from step 1 to step 16) until the last active step is completed.

## **12.5 Touchscreen Controller Registers**

### *12.5.1 TSC\_ADC\_SS Registers*

[Table](#page-11-1) 12-4 lists the memory-mapped registers for the TSC\_ADC\_SS. All register offset addresses not listed in [Table](#page-11-1) 12-4 should be considered as reserved locations and the register contents should not be modified.

**Table 12-4. TSC\_ADC\_SS Registers**

| Offset | Acronym              | Register Name | Section           |
|--------|----------------------|---------------|-------------------|
| 0h     | REVISION             |               | Section 12.5.1.1  |
| 10h    | SYSCONFIG            |               | Section 12.5.1.2  |
| 24h    | IRQSTATUS_RAW        |               | Section 12.5.1.3  |
| 28h    | IRQSTATUS            |               | Section 12.5.1.4  |
| 2Ch    | IRQENABLE_SET        |               | Section 12.5.1.5  |
| 30h    | IRQENABLE_CLR        |               | Section 12.5.1.6  |
| 34h    | IRQWAKEUP            |               | Section 12.5.1.7  |
| 38h    | DMAENABLE_SET        |               | Section 12.5.1.8  |
| 3Ch    | DMAENABLE_CLR        |               | Section 12.5.1.9  |
| 40h    | CTRL                 |               | Section 12.5.1.10 |
| 44h    | ADCSTAT              |               | Section 12.5.1.11 |
| 48h    | ADCRANGE             |               | Section 12.5.1.12 |
| 4Ch    | ADC_CLKDIV           |               | Section 12.5.1.13 |
| 50h    | ADC_MISC             |               | Section 12.5.1.14 |
| 54h    | STEPENABLE           |               | Section 12.5.1.15 |
| 58h    | IDLECONFIG           |               | Section 12.5.1.16 |
| 5Ch    | TS_CHARGE_STEPCONFIG |               | Section 12.5.1.17 |
| 60h    | TS_CHARGE_DELAY      |               | Section 12.5.1.18 |
| 64h    | STEPCONFIG1          |               | Section 12.5.1.19 |
| 68h    | STEPDELAY1           |               | Section 12.5.1.20 |
| 6Ch    | STEPCONFIG2          |               | Section 12.5.1.21 |
| 70h    | STEPDELAY2           |               | Section 12.5.1.22 |
| 74h    | STEPCONFIG3          |               | Section 12.5.1.23 |
| 78h    | STEPDELAY3           |               | Section 12.5.1.24 |
| 7Ch    | STEPCONFIG4          |               | Section 12.5.1.25 |
| 80h    | STEPDELAY4           |               | Section 12.5.1.26 |
| 84h    | STEPCONFIG5          |               | Section 12.5.1.27 |
| 88h    | STEPDELAY5           |               | Section 12.5.1.28 |
| 8Ch    | STEPCONFIG6          |               | Section 12.5.1.29 |
| 90h    | STEPDELAY6           |               | Section 12.5.1.30 |
| 94h    | STEPCONFIG7          |               | Section 12.5.1.31 |
| 98h    | STEPDELAY7           |               | Section 12.5.1.32 |
| 9Ch    | STEPCONFIG8          |               | Section 12.5.1.33 |
| A0h    | STEPDELAY8           |               | Section 12.5.1.34 |
| A4h    | STEPCONFIG9          |               | Section 12.5.1.35 |
| A8h    | STEPDELAY9           |               | Section 12.5.1.36 |
| ACh    | STEPCONFIG10         |               | Section 12.5.1.37 |
| B0h    | STEPDELAY10          |               | Section 12.5.1.38 |
| B4h    | STEPCONFIG11         |               | Section 12.5.1.39 |
| B8h    | STEPDELAY11          |               | Section 12.5.1.40 |
| BCh    | STEPCONFIG12         |               | Section 12.5.1.41 |

**Table 12-4. TSC\_ADC\_SS Registers (continued)**

| Offset | Acronym        | Register Name | Section           |
|--------|----------------|---------------|-------------------|
| C0h    | STEPDELAY12    |               | Section 12.5.1.42 |
| C4h    | STEPCONFIG13   |               | Section 12.5.1.43 |
| C8h    | STEPDELAY13    |               | Section 12.5.1.44 |
| CCh    | STEPCONFIG14   |               | Section 12.5.1.45 |
| D0h    | STEPDELAY14    |               | Section 12.5.1.46 |
| D4h    | STEPCONFIG15   |               | Section 12.5.1.47 |
| D8h    | STEPDELAY15    |               | Section 12.5.1.48 |
| DCh    | STEPCONFIG16   |               | Section 12.5.1.49 |
| E0h    | STEPDELAY16    |               | Section 12.5.1.50 |
| E4h    | FIFO0COUNT     |               | Section 12.5.1.51 |
| E8h    | FIFO0THRESHOLD |               | Section 12.5.1.52 |
| ECh    | DMA0REQ        |               | Section 12.5.1.53 |
| F0h    | FIFO1COUNT     |               | Section 12.5.1.54 |
| F4h    | FIFO1THRESHOLD |               | Section 12.5.1.55 |
| F8h    | DMA1REQ        |               | Section 12.5.1.56 |
| 100h   | FIFO0DATA      |               | Section 12.5.1.57 |
| 200h   | FIFO1DATA      |               | Section 12.5.1.58 |

#### **12.5.1.1 REVISION Register (offset = 0h) [reset = 47300001h]**

Register mask: FFFFFFFFh

REVISION is shown in [Figure](#page-13-1) 12-5 and described in [Table](#page-13-2) 12-5.

Revision Register

**Figure 12-5. REVISION Register**

| 31     | 30     | 29       | 28     | 27 | 26      | 25      | 24 |  |  |
|--------|--------|----------|--------|----|---------|---------|----|--|--|
| SCHEME |        | RESERVED |        |    | FUNC    |         |    |  |  |
| R-1h   |        | R-0h     |        |    | R-730h  |         |    |  |  |
| 23     | 22     | 21       | 20     | 19 | 18      | 17      | 16 |  |  |
|        |        |          | FUNC   |    |         |         |    |  |  |
|        |        |          | R-730h |    |         |         |    |  |  |
| 15     | 14     | 13       | 12     | 11 | 10      | 9       | 8  |  |  |
|        |        | R_RTL    |        |    |         | X_MAJOR |    |  |  |
|        |        | R-0h     |        |    |         | R-0h    |    |  |  |
| 7      | 6      | 5        | 4      | 3  | 2       | 1       | 0  |  |  |
|        | CUSTOM |          |        |    | Y_MINOR |         |    |  |  |
|        | R-0h   |          |        |    | R-1h    |         |    |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-5. REVISION Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                      |  |  |
|-------|----------|------|-------|--------------------------------------------------|--|--|
| 31-30 | SCHEME   | R    | 1h    | HL 0.8 scheme                                    |  |  |
| 29-28 | RESERVED | R    | 0h    | Always read as 0.<br>Writes have no affect.      |  |  |
| 27-16 | FUNC     | R    | 730h  | Functional Number                                |  |  |
| 15-11 | R_RTL    | R    | 0h    | RTL revision.<br>Will vary depending on release. |  |  |
| 10-8  | X_MAJOR  | R    | 0h    | Major revision.                                  |  |  |
| 7-6   | CUSTOM   | R    | 0h    | Custom revision.                                 |  |  |
| 5-0   | Y_MINOR  | R    | 1h    | Minor revision                                   |  |  |

#### **12.5.1.2 SYSCONFIG Register (offset = 10h) [reset = 0h]**

Register mask: FFFFFFFFh

SYSCONFIG is shown in [Figure](#page-14-1) 12-6 and described in [Table](#page-14-2) 12-6.

SysConfig Register

### **Figure 12-6. SYSCONFIG Register**

| 31 | 30                               | 29     | 28       | 27 | 26     | 25 | 24     |  |  |  |  |  |
|----|----------------------------------|--------|----------|----|--------|----|--------|--|--|--|--|--|
|    | RESERVED                         |        |          |    |        |    |        |  |  |  |  |  |
|    | R/W-0h                           |        |          |    |        |    |        |  |  |  |  |  |
| 23 | 22                               | 21     | 20       | 19 | 18     | 17 | 16     |  |  |  |  |  |
|    | RESERVED                         |        |          |    |        |    |        |  |  |  |  |  |
|    |                                  |        | R/W-0h   |    |        |    |        |  |  |  |  |  |
| 15 | 14                               | 13     | 12       | 11 | 10     | 9  | 8      |  |  |  |  |  |
|    |                                  |        | RESERVED |    |        |    |        |  |  |  |  |  |
|    |                                  |        | R/W-0h   |    |        |    |        |  |  |  |  |  |
| 7  | 6                                | 5      | 4        | 3  | 2      | 1  | 0      |  |  |  |  |  |
|    | RESERVED<br>IdleMode<br>RESERVED |        |          |    |        |    |        |  |  |  |  |  |
|    |                                  | R/W-0h |          |    | R/W-0h |    | R/W-0h |  |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-6. SYSCONFIG Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------|
| 31-4 | RESERVED | R/W  | 0h    |                                                                                                                                            |
| 3-2  | IdleMode | R/W  | 0h    | 00 = Force Idle (always acknowledges).<br>01 = No Idle Mode (never acknowledges).<br>10 = Smart-Idle Mode.<br>11 = Smart Idle with Wakeup. |
| 1-0  | RESERVED | R/W  | 0h    |                                                                                                                                            |

#### **12.5.1.3 IRQSTATUS\_RAW Register (offset = 24h) [reset = 0h]**

Register mask: FFFFFFFFh

IRQSTATUS\_RAW is shown in [Figure](#page-15-1) 12-7 and described in [Table](#page-15-2) 12-7.

IRQ status (unmasked)

**Figure 12-7. IRQSTATUS\_RAW Register**

| 31                  | 30            | 29                  | 28                  | 27            | 26                       | 25                  | 24                            |  |  |  |  |  |
|---------------------|---------------|---------------------|---------------------|---------------|--------------------------|---------------------|-------------------------------|--|--|--|--|--|
|                     | RESERVED      |                     |                     |               |                          |                     |                               |  |  |  |  |  |
|                     | R/W-0h        |                     |                     |               |                          |                     |                               |  |  |  |  |  |
| 23                  | 22            | 21                  | 20                  | 19            | 18                       | 17                  | 16                            |  |  |  |  |  |
|                     |               |                     |                     | RESERVED      |                          |                     |                               |  |  |  |  |  |
|                     | R/W-0h        |                     |                     |               |                          |                     |                               |  |  |  |  |  |
| 15                  | 14            | 13                  | 12                  | 11            | 10                       | 9                   | 8                             |  |  |  |  |  |
|                     |               | RESERVED            |                     |               | PEN_IRQ_sync<br>hronized | Pen_Up_Event        | Out_of_Range                  |  |  |  |  |  |
|                     |               | R/W-0h              |                     |               | R/W-0h                   | R/W-0h              | R/W-0h                        |  |  |  |  |  |
| 7                   | 6             | 5                   | 4                   | 3             | 2                        | 1                   | 0                             |  |  |  |  |  |
| FIFO1_Underfl<br>ow | FIFO1_Overrun | FIFO1_Thresho<br>ld | FIFO0_Underfl<br>ow | FIFO0_Overrun | FIFO0_Thresho<br>ld      | End_of_Seque<br>nce | HW_Pen_Event<br>_asynchronous |  |  |  |  |  |
| R/W-0h              | R/W-0h        | R/W-0h              | R/W-0h              | R/W-0h        | R/W-0h                   | R/W-0h              | R/W-0h                        |  |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-7. IRQSTATUS\_RAW Register Field Descriptions**

| Bit   | Field                | Type | Reset | Description                                                                                                   |
|-------|----------------------|------|-------|---------------------------------------------------------------------------------------------------------------|
| 31-11 | RESERVED             | R/W  | 0h    |                                                                                                               |
| 10    | PEN_IRQ_synchronized | R/W  | 0h    | Write 0 = No action.<br>Write 1 = Set event (debug).<br>Read 0 = No event pending.<br>Read 1 = Event pending. |
| 9     | Pen_Up_Event         | R/W  | 0h    | Write 0 = No action.<br>Write 1 = Set event (debug).<br>Read 0 = No event pending.<br>Read 1 = Event pending. |
| 8     | Out_of_Range         | R/W  | 0h    | Write 0 = No action.<br>Write 1 = Set event (debug).<br>Read 0 = No event pending.<br>Read 1 = Event pending. |
| 7     | FIFO1_Underflow      | R/W  | 0h    | Write 0 = No action.<br>Write 1 = Set event (debug).<br>Read 0 = No event pending.<br>Read 1 = Event pending. |
| 6     | FIFO1_Overrun        | R/W  | 0h    | Write 0 = No action.<br>Write 1 = Set event (debug).<br>Read 0 = No event pending.<br>Read 1 = Event pending. |
| 5     | FIFO1_Threshold      | R/W  | 0h    | Write 0 = No action.<br>Write 1 = Set event (debug).<br>Read 0 = No event pending.<br>Read 1 = Event pending. |
| 4     | FIFO0_Underflow      | R/W  | 0h    | Write 0 = No action.<br>Write 1 = Set event (debug).<br>Read 0 = No event pending.<br>Read 1 = Event pending. |
| 3     | FIFO0_Overrun        | R/W  | 0h    | Write 0 = No action.<br>Write 1 = Set event (debug).<br>Read 0 = No event pending.<br>Read 1 = Event pending. |

**Table 12-7. IRQSTATUS\_RAW Register Field Descriptions (continued)**

| Bit | Field                         | Type | Reset | Description                                                                                                   |
|-----|-------------------------------|------|-------|---------------------------------------------------------------------------------------------------------------|
| 2   | FIFO0_Threshold               | R/W  | 0h    | Write 0 = No action.<br>Write 1 = Set event (debug).<br>Read 0 = No event pending.<br>Read 1 = Event pending. |
| 1   | End_of_Sequence               | R/W  | 0h    | Write 0 = No action.<br>Write 1 = Set event (debug).<br>Read 0 = No event pending.<br>Read 1 = Event pending. |
| 0   | HW_Pen_Event_asynchro<br>nous | R/W  | 0h    | Write 0 = No action.<br>Write 1 = Set event (debug).<br>Read 0 = No event pending.<br>Read 1 = Event pending. |

#### **12.5.1.4 IRQSTATUS Register (offset = 28h) [reset = 0h]**

Register mask: FFFFFFFFh

IRQSTATUS is shown in [Figure](#page-17-1) 12-8 and described in [Table](#page-17-2) 12-8.

IRQ status (masked)

**Figure 12-8. IRQSTATUS Register**

| 31                  | 30            | 29                  | 28                  | 27            | 26                           | 25                  | 24                            |  |  |  |  |
|---------------------|---------------|---------------------|---------------------|---------------|------------------------------|---------------------|-------------------------------|--|--|--|--|
|                     | RESERVED      |                     |                     |               |                              |                     |                               |  |  |  |  |
|                     | R/W-0h        |                     |                     |               |                              |                     |                               |  |  |  |  |
| 23                  | 22            | 21                  | 20                  | 19            | 18                           | 17                  | 16                            |  |  |  |  |
|                     |               |                     |                     | RESERVED      |                              |                     |                               |  |  |  |  |
|                     | R/W-0h        |                     |                     |               |                              |                     |                               |  |  |  |  |
| 15                  | 14            | 13                  | 12                  | 11            | 10                           | 9                   | 8                             |  |  |  |  |
|                     |               | RESERVED            |                     |               | HW_Pen_Event<br>_synchronous | Pen_Up_event        | Out_of_Range                  |  |  |  |  |
|                     |               | R/W-0h              |                     |               | R/W-0h                       | R/W-0h              | R/W-0h                        |  |  |  |  |
| 7                   | 6             | 5                   | 4                   | 3             | 2                            | 1                   | 0                             |  |  |  |  |
| FIFO1_Underfl<br>ow | FIFO1_Overrun | FIFO1_Thresho<br>ld | FIFO0_Underfl<br>ow | FIFO0_Overrun | FIFO0_Thresho<br>ld          | End_of_Seque<br>nce | HW_Pen_Event<br>_asynchronous |  |  |  |  |
| R/W-0h              | R/W-0h        | R/W-0h              | R/W-0h              | R/W-0h        | R/W-0h                       | R/W-0h              | R/W-0h                        |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-8. IRQSTATUS Register Field Descriptions**

| Bit   | Field                        | Type | Reset | Description                                                                                                             |
|-------|------------------------------|------|-------|-------------------------------------------------------------------------------------------------------------------------|
| 31-11 | RESERVED                     | R/W  | 0h    |                                                                                                                         |
| 10    | HW_Pen_Event_synchron<br>ous | R/W  | 0h    | Write 0 = No action.<br>Read 0 = No (enabled) event pending.<br>Read 1 = Event pending.<br>Write 1 = Clear (raw) event. |
| 9     | Pen_Up_event                 | R/W  | 0h    | Write 0 = No action.<br>Read 0 = No (enabled) event pending.<br>Read 1 = Event pending.<br>Write 1 = Clear (raw) event. |
| 8     | Out_of_Range                 | R/W  | 0h    | Write 0 = No action.<br>Read 0 = No (enabled) event pending.<br>Read 1 = Event pending.<br>Write 1 = Clear (raw) event. |
| 7     | FIFO1_Underflow              | R/W  | 0h    | Write 0 = No action.<br>Read 0 = No (enabled) event pending.<br>Read 1 = Event pending.<br>Write 1 = Clear (raw) event. |
| 6     | FIFO1_Overrun                | R/W  | 0h    | Write 0 = No action.<br>Read 0 = No (enabled) event pending.<br>Read 1 = Event pending.<br>Write 1 = Clear (raw) event. |
| 5     | FIFO1_Threshold              | R/W  | 0h    | Write 0 = No action.<br>Read 0 = No (enabled) event pending.<br>Read 1 = Event pending.<br>Write 1 = Clear (raw) event. |
| 4     | FIFO0_Underflow              | R/W  | 0h    | Write 0 = No action.<br>Read 0 = No (enabled) event pending.<br>Read 1 = Event pending.<br>Write 1 = Clear (raw) event. |
| 3     | FIFO0_Overrun                | R/W  | 0h    | Write 0 = No action.<br>Read 0 = No (enabled) event pending.<br>Read 1 = Event pending.<br>Write 1 = Clear (raw) event. |

**Table 12-8. IRQSTATUS Register Field Descriptions (continued)**

| Bit | Field                         | Type | Reset | Description                                                                                                             |
|-----|-------------------------------|------|-------|-------------------------------------------------------------------------------------------------------------------------|
| 2   | FIFO0_Threshold               | R/W  | 0h    | Write 0 = No action.<br>Read 0 = No (enabled) event pending.<br>Read 1 = Event pending.<br>Write 1 = Clear (raw) event. |
| 1   | End_of_Sequence               | R/W  | 0h    | Write 0 = No action.<br>Read 0 = No (enabled) event pending.<br>Read 1 = Event pending.<br>Write 1 = Clear (raw) event. |
| 0   | HW_Pen_Event_asynchro<br>nous | R/W  | 0h    | Write 0 = No action.<br>Read 0 = No (enabled) event pending.<br>Read 1 = Event pending.<br>Write 1 = Clear (raw) event. |

#### **12.5.1.5 IRQENABLE\_SET Register (offset = 2Ch) [reset = 0h]**

Register mask: FFFFFFFFh

IRQENABLE\_SET is shown in [Figure](#page-19-1) 12-9 and described in [Table](#page-19-2) 12-9.

IRQ enable set bits

### **Figure 12-9. IRQENABLE\_SET Register**

| 31                  | 30            | 29                  | 28                  | 27            | 26                           | 25                  | 24                            |  |  |
|---------------------|---------------|---------------------|---------------------|---------------|------------------------------|---------------------|-------------------------------|--|--|
|                     | RESERVED      |                     |                     |               |                              |                     |                               |  |  |
|                     | R/W-0h        |                     |                     |               |                              |                     |                               |  |  |
| 23                  | 22            | 21                  | 20                  | 19            | 18                           | 17                  | 16                            |  |  |
|                     | RESERVED      |                     |                     |               |                              |                     |                               |  |  |
|                     | R/W-0h        |                     |                     |               |                              |                     |                               |  |  |
| 15                  | 14            | 13                  | 12                  | 11            | 10                           | 9                   | 8                             |  |  |
|                     |               | RESERVED            |                     |               | HW_Pen_Event<br>_synchronous | Pen_Up_event        | Out_of_Range                  |  |  |
|                     |               | R/W-0h              |                     |               | R/W-0h                       | R/W-0h              | R/W-0h                        |  |  |
| 7                   | 6             | 5                   | 4                   | 3             | 2                            | 1                   | 0                             |  |  |
| FIFO1_Underfl<br>ow | FIFO1_Overrun | FIFO1_Thresho<br>ld | FIFO0_Underfl<br>ow | FIFO0_Overrun | FIFO0_Thresho<br>ld          | End_of_Seque<br>nce | HW_Pen_Event<br>_asynchronous |  |  |
| R/W-0h              | R/W-0h        | R/W-0h              | R/W-0h              | R/W-0h        | R/W-0h                       | R/W-0h              | R/W-0h                        |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-9. IRQENABLE\_SET Register Field Descriptions**

| Bit   | Field                        | Type | Reset                                                                                                                             | Description                                                                                                                 |
|-------|------------------------------|------|-----------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| 31-11 | RESERVED                     | R/W  | 0h                                                                                                                                |                                                                                                                             |
| 10    | HW_Pen_Event_synchron<br>ous | R/W  | 0h<br>Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Enable interrupt. |                                                                                                                             |
| 9     | Pen_Up_event                 | R/W  | 0h<br>Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Enable interrupt. |                                                                                                                             |
| 8     | Out_of_Range                 | R/W  | 0h<br>Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Enable interrupt. |                                                                                                                             |
| 7     | FIFO1_Underflow              | R/W  | 0h                                                                                                                                | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Enable interrupt. |
| 6     | FIFO1_Overrun                | R/W  | 0h<br>Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Enable interrupt. |                                                                                                                             |
| 5     | FIFO1_Threshold              | R/W  | 0h<br>Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Enable interrupt. |                                                                                                                             |
| 4     | FIFO0_Underflow              | R/W  | 0h                                                                                                                                | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Enable interrupt. |
| 3     | FIFO0_Overrun                | R/W  | 0h                                                                                                                                | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Enable interrupt. |

**Table 12-9. IRQENABLE\_SET Register Field Descriptions (continued)**

| Bit | Field                         | Type | Reset | Description                                                                                                                 |
|-----|-------------------------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------|
| 2   | FIFO0_Threshold               | R/W  | 0h    | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Enable interrupt. |
| 1   | End_of_Sequence               | R/W  | 0h    | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Enable interrupt. |
| 0   | HW_Pen_Event_asynchro<br>nous | R/W  | 0h    | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Enable interrupt. |

#### **12.5.1.6 IRQENABLE\_CLR Register (offset = 30h) [reset = 0h]**

Register mask: FFFFFFFFh

IRQENABLE\_CLR is shown in [Figure](#page-21-1) 12-10 and described in Table [12-10.](#page-21-2)

IRQ enable clear bits

**Figure 12-10. IRQENABLE\_CLR Register**

| 31                  | 30            | 29                  | 28                  | 27            | 26                           | 25                  | 24                            |  |
|---------------------|---------------|---------------------|---------------------|---------------|------------------------------|---------------------|-------------------------------|--|
|                     |               |                     |                     | RESERVED      |                              |                     |                               |  |
|                     | R/W-0h        |                     |                     |               |                              |                     |                               |  |
| 23                  | 22            | 21                  | 20                  | 19            | 18                           | 17                  | 16                            |  |
|                     | RESERVED      |                     |                     |               |                              |                     |                               |  |
|                     | R/W-0h        |                     |                     |               |                              |                     |                               |  |
| 15                  | 14            | 13                  | 12                  | 11            | 10                           | 9                   | 8                             |  |
|                     |               | RESERVED            |                     |               | HW_Pen_Event<br>_synchronous | Pen_Up_Event        | Out_of_Range                  |  |
|                     |               | R/W-0h              |                     |               | R/W-0h                       | R/W-0h              | R/W-0h                        |  |
| 7                   | 6             | 5                   | 4                   | 3             | 2                            | 1                   | 0                             |  |
| FIFO1_Underfl<br>ow | FIFO1_Overrun | FIFO1_Thresho<br>ld | FIFO0_Underfl<br>ow | FIFO0_Overrun | FIFO0_Thresho<br>ld          | End_of_Seque<br>nce | HW_Pen_Event<br>_asynchronous |  |
| R/W-0h              | R/W-0h        | R/W-0h              | R/W-0h              | R/W-0h        | R/W-0h                       | R/W-0h              | R/W-0h                        |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-10. IRQENABLE\_CLR Register Field Descriptions**

| Bit   | Field                        | Type | Reset                                                                                                                              | Description                                                                                                                  |
|-------|------------------------------|------|------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------|
| 31-11 | RESERVED                     | R/W  | 0h                                                                                                                                 |                                                                                                                              |
| 10    | HW_Pen_Event_synchron<br>ous | R/W  | 0h<br>Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Disable interrupt. |                                                                                                                              |
| 9     | Pen_Up_Event                 | R/W  | 0h<br>Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Disable interrupt. |                                                                                                                              |
| 8     | Out_of_Range                 | R/W  | 0h<br>Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Disable interrupt. |                                                                                                                              |
| 7     | FIFO1_Underflow              | R/W  | 0h                                                                                                                                 | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Disable interrupt. |
| 6     | FIFO1_Overrun                | R/W  | 0h                                                                                                                                 | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Disable interrupt. |
| 5     | FIFO1_Threshold              | R/W  | 0h                                                                                                                                 | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Disable interrupt. |
| 4     | FIFO0_Underflow              | R/W  | 0h                                                                                                                                 | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Disable interrupt. |
| 3     | FIFO0_Overrun                | R/W  | 0h                                                                                                                                 | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Disable interrupt. |

**Table 12-10. IRQENABLE\_CLR Register Field Descriptions (continued)**

| Bit | Field                         | Type | Reset | Description                                                                                                                  |
|-----|-------------------------------|------|-------|------------------------------------------------------------------------------------------------------------------------------|
| 2   | FIFO0_Threshold               | R/W  | 0h    | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Disable interrupt. |
| 1   | End_of_Sequence               | R/W  | 0h    | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Disable interrupt. |
| 0   | HW_Pen_Event_asynchro<br>nous | R/W  | 0h    | Write 0 = No action.<br>Read 0 = Interrupt disabled (masked).<br>Read 1 = Interrupt enabled.<br>Write 1 = Disable interrupt. |

#### **12.5.1.7 IRQWAKEUP Register (offset = 34h) [reset = 0h]**

Register mask: FFFFFFFFh

IRQWAKEUP is shown in [Figure](#page-23-1) 12-11 and described in Table [12-11](#page-23-2).

IRQ wakeup enable

**Figure 12-11. IRQWAKEUP Register**

| 31       | 30       | 29 | 28       | 27       | 26 | 25 | 24      |  |  |  |
|----------|----------|----|----------|----------|----|----|---------|--|--|--|
|          | RESERVED |    |          |          |    |    |         |  |  |  |
| R/W-0h   |          |    |          |          |    |    |         |  |  |  |
| 23       | 22       | 21 | 20       | 19       | 18 | 17 | 16      |  |  |  |
| RESERVED |          |    |          |          |    |    |         |  |  |  |
| R/W-0h   |          |    |          |          |    |    |         |  |  |  |
| 15       | 14       | 13 | 12       | 11       | 10 | 9  | 8       |  |  |  |
|          |          |    |          | RESERVED |    |    |         |  |  |  |
|          |          |    |          | R/W-0h   |    |    |         |  |  |  |
| 7        | 6        | 5  | 4        | 3        | 2  | 1  | 0       |  |  |  |
|          |          |    | RESERVED |          |    |    | WAKEEN0 |  |  |  |
|          | R/W-0h   |    |          |          |    |    |         |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-11. IRQWAKEUP Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                        |
|------|----------|------|-------|------------------------------------------------------------------------------------|
| 31-1 | RESERVED | R/W  | 0h    |                                                                                    |
| 0    | WAKEEN0  | R/W  | 0h    | Wakeup generation for HW Pen event.<br>0 = Wakeup disabled.<br>1 = Wakeup enabled. |

#### **12.5.1.8 DMAENABLE\_SET Register (offset = 38h) [reset = 0h]**

Register mask: FFFFFFFFh

DMAENABLE\_SET is shown in [Figure](#page-24-1) 12-12 and described in Table [12-12.](#page-24-2)

Per-Line DMA set

**Figure 12-12. DMAENABLE\_SET Register**

| 31       | 30       | 29       | 28 | 27       | 26 | 25       | 24       |  |  |
|----------|----------|----------|----|----------|----|----------|----------|--|--|
|          | RESERVED |          |    |          |    |          |          |  |  |
|          | R/W-0h   |          |    |          |    |          |          |  |  |
| 23       | 22       | 21       | 20 | 19       | 18 | 17       | 16       |  |  |
| RESERVED |          |          |    |          |    |          |          |  |  |
| R/W-0h   |          |          |    |          |    |          |          |  |  |
| 15       | 14       | 13       | 12 | 11       | 10 | 9        | 8        |  |  |
|          |          |          |    | RESERVED |    |          |          |  |  |
|          |          |          |    | R/W-0h   |    |          |          |  |  |
| 7        | 6        | 5        | 4  | 3        | 2  | 1        | 0        |  |  |
|          |          | RESERVED |    |          |    | Enable_1 | Enable_0 |  |  |
|          | R/W-0h   |          |    |          |    |          | R/W-0h   |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-12. DMAENABLE\_SET Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                   |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| 31-2 | RESERVED | R/W  | 0h    |                                                                                                                                               |
| 1    | Enable_1 | R/W  | 0h    | Enable DMA request FIFO 1.<br>Write 0 = No action.<br>Read 0 = DMA line disabled.<br>Read 1 = DMA line enabled.<br>Write 1 = Enable DMA line. |
| 0    | Enable_0 | R/W  | 0h    | Enable DMA request FIFO 0.<br>Write 0 = No action.<br>Read 0 = DMA line disabled.<br>Read 1 = DMA line enabled.<br>Write 1 = Enable DMA line. |

#### **12.5.1.9 DMAENABLE\_CLR Register (offset = 3Ch) [reset = 0h]**

Register mask: FFFFFFFFh

DMAENABLE\_CLR is shown in [Figure](#page-25-1) 12-13 and described in Table [12-13](#page-25-2).

Per-Line DMA clr

**Figure 12-13. DMAENABLE\_CLR Register**

| 31     | 30                 | 29 | 28       | 27       | 26 | 25 | 24     |  |  |
|--------|--------------------|----|----------|----------|----|----|--------|--|--|
|        | RESERVED           |    |          |          |    |    |        |  |  |
|        | R/W-0h             |    |          |          |    |    |        |  |  |
| 23     | 22                 | 21 | 20       | 19       | 18 | 17 | 16     |  |  |
|        | RESERVED           |    |          |          |    |    |        |  |  |
| R/W-0h |                    |    |          |          |    |    |        |  |  |
| 15     | 14                 | 13 | 12       | 11       | 10 | 9  | 8      |  |  |
|        |                    |    |          | RESERVED |    |    |        |  |  |
|        |                    |    |          | R/W-0h   |    |    |        |  |  |
| 7      | 6                  | 5  | 4        | 3        | 2  | 1  | 0      |  |  |
|        |                    |    | Enable_1 | Enable_0 |    |    |        |  |  |
|        | RESERVED<br>R/W-0h |    |          |          |    |    | R/W-0h |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-13. DMAENABLE\_CLR Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                     |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-2 | RESERVED | R/W  | 0h    |                                                                                                                                                 |
| 1    | Enable_1 | R/W  | 0h    | Disable DMA request FIFO 1.<br>Write 0 = No action.<br>Read 0 = DMA line disabled.<br>Read 1 = DMA line enabled.<br>Write 1 = Disable DMA line. |
| 0    | Enable_0 | R/W  | 0h    | Disable DMA request FIFO 0.<br>Write 0 = No action.<br>Read 0 = DMA line disabled.<br>Read 1 = DMA line enabled.<br>Write 1 = Disable DMA line. |

#### **12.5.1.10 CTRL Register (offset = 40h) [reset = 0h]**

Register mask: FFFFFFFFh

CTRL is shown in [Figure](#page-26-1) 12-14 and described in Table [12-14](#page-26-2).

@TSC\_ADC\_SS Control Register

**Figure 12-14. CTRL Register**

| 31                      | 30           | 29 | 28         | 27                  | 26                                           | 25          | 24                   |
|-------------------------|--------------|----|------------|---------------------|----------------------------------------------|-------------|----------------------|
|                         |              |    |            | RESERVED            |                                              |             |                      |
|                         |              |    |            | R-0h                |                                              |             |                      |
| 23                      | 22           | 21 | 20         | 19                  | 18                                           | 17          | 16                   |
|                         |              |    |            | RESERVED            |                                              |             |                      |
|                         |              |    |            | R-0h                |                                              |             |                      |
| 15                      | 14           | 13 | 12         | 11                  | 10                                           | 9           | 8                    |
|                         |              |    | RESERVED   |                     |                                              | HW_preempt  | HW_event_ma<br>pping |
|                         |              |    | R-0h       |                     |                                              | R/W-0h      | R/W-0h               |
| 7                       | 6            | 5  | 4          | 3                   | 2                                            | 1           | 0                    |
| Touch_Screen_<br>Enable | AFE_Pen_Ctrl |    | Power_Down | ADC_Bias_Sel<br>ect | StepConfig_Wri<br>teProtect_n_act<br>ive_low | Step_ID_tag | Enable               |
| R/W-0h                  | R/W-0h       |    | R/W-0h     | R/W-0h              | R/W-0h                                       | R/W-0h      | R/W-0h               |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-14. CTRL Register Field Descriptions**

| Bit   | Field                                    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                         |
|-------|------------------------------------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED                                 | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                     |
| 9     | HW_preempt                               | R/W  | 0h    | 0 = SW steps are not pre-empted by HW events.<br>1 = SW steps are pre-empted by HW events                                                                                                                                                                                                                                                           |
| 8     | HW_event_mapping                         | R/W  | 0h    | 0 = Map HW event to Pen touch irq (from AFE).<br>1 = Map HW event to HW event input.                                                                                                                                                                                                                                                                |
| 7     | Touch_Screen_Enable                      | R/W  | 0h    | 0 = Touchscreen transistors disabled.<br>1 = Touchscreen transistors enabled                                                                                                                                                                                                                                                                        |
| 6-5   | AFE_Pen_Ctrl                             | R/W  | 0h    | These two bits are sent directly to the AFE Pen Ctrl inputs.<br>Bit 6 controls the Wiper touch (5 wire modes)Bit 5 controls the X+<br>touch (4 wire modes)User also needs to make sure the ground path<br>is connected properly for pen interrupt to occur (using the<br>StepConfig registers)Refer to section 4 interrupts for more<br>information |
| 4     | Power_Down                               | R/W  | 0h    | ADC Power Down control.<br>0 = AFE is powered up (default).<br>1 = Write 1 to power down AFE (the tsc_adc_ss enable (bit 0)<br>should also be set to off)                                                                                                                                                                                           |
| 3     | ADC_Bias_Select                          | R/W  | 0h    | Select Bias to AFE.<br>0 = Internal.<br>1 = Reserved.                                                                                                                                                                                                                                                                                               |
| 2     | StepConfig_WriteProtect_<br>n_active_low | R/W  | 0h    | 0 = Step configuration registers are protected (not writable).<br>1 = Step configuration registers are not protected (writable).                                                                                                                                                                                                                    |
| 1     | Step_ID_tag                              | R/W  | 0h    | Writing 1 to this bit will store the Step ID number with the captured<br>ADC data in the FIFO.<br>0 = Write zeroes.<br>1 = Store the channel ID tag.                                                                                                                                                                                                |
| 0     | Enable                                   | R/W  | 0h    | TSC_ADC_SS module enable bit.<br>After programming all the steps and configuration registers, write a<br>1to this bit to turn on TSC_ADC_SS.<br>Writing a 0 will disable the module (after the current conversion).                                                                                                                                 |

#### **12.5.1.11 ADCSTAT Register (offset = 44h) [reset = 10h]**

Register mask: FFFFFFFFh

ADCSTAT is shown in [Figure](#page-27-1) 12-15 and described in Table [12-15.](#page-27-2)

General Status bits @TSC\_ADC\_SS\_Sequencer\_Status Register

**Figure 12-15. ADCSTAT Register**

| 31       | 30       | 29       | 28       | 27 | 26      | 25 | 24 |
|----------|----------|----------|----------|----|---------|----|----|
|          |          |          | RESERVED |    |         |    |    |
|          |          |          | R-0h     |    |         |    |    |
| 23       | 22       | 21       | 20       | 19 | 18      | 17 | 16 |
|          |          |          | RESERVED |    |         |    |    |
|          |          |          | R-0h     |    |         |    |    |
| 15       | 14       | 13       | 12       | 11 | 10      | 9  | 8  |
|          |          |          | RESERVED |    |         |    |    |
|          |          |          | R-0h     |    |         |    |    |
| 7        | 6        | 5        | 4        | 3  | 2       | 1  | 0  |
| PEN_IRQ1 | PEN_IRQ0 | FSM_BUSY |          |    | STEP_ID |    |    |
| R-0h     | R-0h     | R-0h     |          |    | R-10h   |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-15. ADCSTAT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                 |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | RESERVED | R    | 0h    | -                                                                                                                                                                                                                                                                                                                                                                           |
| 7    | PEN_IRQ1 | R    | 0h    | PEN_IRQ[1] status                                                                                                                                                                                                                                                                                                                                                           |
| 6    | PEN_IRQ0 | R    | 0h    | PEN_IRQ[0] status                                                                                                                                                                                                                                                                                                                                                           |
| 5    | FSM_BUSY | R    | 0h    | Status of OCP FSM and ADC FSM.<br>0 = Idle.<br>1 = Busy.                                                                                                                                                                                                                                                                                                                    |
| 4-0  | STEP_ID  | R    | 10h   | Encoded values:.<br>10000 = Idle.<br>10001 = Charge.<br>00000 = Step 1.<br>00001 = Step 2.<br>00010 = Step 3.<br>00011 = Step 4.<br>00100 = Step 5.<br>00101 = Step 6.<br>00110 = Step 7.<br>00111 = Step 8.<br>01000 = Step 9.<br>01001 = Step 10.<br>01010 = Step 11.<br>01011 = Step 12.<br>01100 = Step 13.<br>01101 = Step 14.<br>01110 = Step 15.<br>01111 = Step 16. |

#### **12.5.1.12 ADCRANGE Register (offset = 48h) [reset = 0h]**

Register mask: FFFFFFFFh

ADCRANGE is shown in [Figure](#page-28-1) 12-16 and described in Table [12-16.](#page-28-2)

High and Low Range Threshold@TSC\_ADC\_SS\_Range\_Check Register

**Figure 12-16. ADCRANGE Register**

| 31 | 30 | 29       | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21              | 20 | 19 | 18 | 17 | 16 |
|----|----|----------|----|----|----|----|----|----|----|-----------------|----|----|----|----|----|
|    |    | RESERVED |    |    |    |    |    |    |    | High_Range_Data |    |    |    |    |    |
|    |    | R-0h     |    |    |    |    |    |    |    | R/W-0h          |    |    |    |    |    |
| 15 | 14 | 13       | 12 | 11 | 10 | 9  | 8  | 7  | 6  | 5               | 4  | 3  | 2  | 1  | 0  |
|    |    | RESERVED |    |    |    |    |    |    |    | Low_Range_Data  |    |    |    |    |    |
|    |    | R/W-0h   |    |    |    |    |    |    |    | R/W-0h          |    |    |    |    |    |
|    |    |          |    |    |    |    |    |    |    |                 |    |    |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-16. ADCRANGE Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                                      |
|-------|-----------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R    | 0h    |                                                                                                                                  |
| 27-16 | High_Range_Data | R/W  | 0h    | Sampled ADC data is compared to this value.<br>If the sampled data is greater than the value, then an interrupt is<br>generated. |
| 15-12 | RESERVED        | R/W  | 0h    | Reserved.                                                                                                                        |
| 11-0  | Low_Range_Data  | R/W  | 0h    | Sampled ADC data is compared to this value.<br>If the sampled data is less than the value, then an interrupt is<br>generated.    |

#### **12.5.1.13 ADC\_CLKDIV Register (offset = 4Ch) [reset = 0h]**

Register mask: FFFFFFFFh

ADC\_CLKDIV is shown in [Figure](#page-29-1) 12-17 and described in Table [12-17.](#page-29-2)

ADC clock divider register@TSC\_ADC\_SS\_Clock\_Divider Register

#### **Figure 12-17. ADC\_CLKDIV Register**

| 31 |                  | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15     | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7          | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|------------------|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|--------|----|----|----|----|----|---|---|------------|---|---|---|---|---|---|---|
|    |                  |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |        |    |    |    |    |    |   |   | ADC_ClkDiv |   |   |   |   |   |   |   |
|    | RESERVED<br>R-0h |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-0h |    |    |    |    |    |   |   |            |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-17. ADC\_CLKDIV Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                                                                                               |
|-------|------------|------|-------|-----------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED   | R    | 0h    |                                                                                                           |
| 15-0  | ADC_ClkDiv | R/W  | 0h    | The input ADC clock will be divided by this value and sent to the<br>AFE.<br>Program to the value minus 1 |

#### **12.5.1.14 ADC\_MISC Register (offset = 50h) [reset = 0h]**

Register mask: FFFFFFFFh

ADC\_MISC is shown in [Figure](#page-30-1) 12-18 and described in Table [12-18.](#page-30-2)

AFE misc debug@TSC\_ADC\_SS\_MISC Register

**Figure 12-18. ADC\_MISC Register**

| 31 | 30 | 29               | 28 | 27       | 26              | 25 | 24 |
|----|----|------------------|----|----------|-----------------|----|----|
|    |    |                  |    | RESERVED |                 |    |    |
|    |    |                  |    | R-0h     |                 |    |    |
| 23 | 22 | 21               | 20 | 19       | 18              | 17 | 16 |
|    |    |                  |    | RESERVED |                 |    |    |
|    |    |                  |    | R-0h     |                 |    |    |
| 15 | 14 | 13               | 12 | 11       | 10              | 9  | 8  |
|    |    |                  |    | RESERVED |                 |    |    |
|    |    |                  |    | R-0h     |                 |    |    |
| 7  | 6  | 5                | 4  | 3        | 2               | 1  | 0  |
|    |    | AFE_Spare_Output |    |          | AFE_Spare_Input |    |    |
|    |    | R-0h             |    |          | R/W-0h          |    |    |
|    |    |                  |    |          |                 |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-18. ADC\_MISC Register Field Descriptions**

| Bit  | Field            | Type | Reset | Description                                                          |
|------|------------------|------|-------|----------------------------------------------------------------------|
| 31-8 | RESERVED         | R    | 0h    | RESERVED.                                                            |
| 7-4  | AFE_Spare_Output | R    | 0h    | Connected to AFE Spare Output pins.<br>Reserved in normal operation. |
| 3-0  | AFE_Spare_Input  | R/W  | 0h    | Connected to AFE Spare Input pins.<br>Reserved in normal operation.  |

#### **12.5.1.15 STEPENABLE Register (offset = 54h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPENABLE is shown in [Figure](#page-31-1) 12-19 and described in Table [12-19.](#page-31-2)

Step Enable

**Figure 12-19. STEPENABLE Register**

| 31     | 30     | 29     | 28       | 27       | 26     | 25     | 24        |
|--------|--------|--------|----------|----------|--------|--------|-----------|
|        |        |        |          | RESERVED |        |        |           |
|        |        |        |          | R-0h     |        |        |           |
| 23     | 22     | 21     | 20       | 19       | 18     | 17     | 16        |
|        |        |        | RESERVED |          |        |        | STEP16    |
|        |        |        | R-0h     |          |        |        | R/W-0h    |
| 15     | 14     | 13     | 12       | 11       | 10     | 9      | 8         |
| STEP15 | STEP14 | STEP13 | STEP12   | STEP11   | STEP10 | STEP9  | STEP8     |
| R/W-0h | R/W-0h | R/W-0h | R/W-0h   | R/W-0h   | R/W-0h | R/W-0h | R/W-0h    |
| 7      | 6      | 5      | 4        | 3        | 2      | 1      | 0         |
| STEP7  | STEP6  | STEP5  | STEP4    | STEP3    | STEP2  | STEP1  | TS_Charge |
| R/W-0h | R/W-0h | R/W-0h | R/W-0h   | R/W-0h   | R/W-0h | R/W-0h | R/W-0h    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-19. STEPENABLE Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description           |
|-------|-----------|------|-------|-----------------------|
| 31-17 | RESERVED  | R    | 0h    | RESERVED.             |
| 16    | STEP16    | R/W  | 0h    | Enable step 16        |
| 15    | STEP15    | R/W  | 0h    | Enable step 15        |
| 14    | STEP14    | R/W  | 0h    | Enable step 14        |
| 13    | STEP13    | R/W  | 0h    | Enable step 13        |
| 12    | STEP12    | R/W  | 0h    | Enable step 12        |
| 11    | STEP11    | R/W  | 0h    | Enable step 11        |
| 10    | STEP10    | R/W  | 0h    | Enable step 10        |
| 9     | STEP9     | R/W  | 0h    | Enable step 9         |
| 8     | STEP8     | R/W  | 0h    | Enable step 8         |
| 7     | STEP7     | R/W  | 0h    | Enable step 7         |
| 6     | STEP6     | R/W  | 0h    | Enable step 6         |
| 5     | STEP5     | R/W  | 0h    | Enable step 5         |
| 4     | STEP4     | R/W  | 0h    | Enable step 4         |
| 3     | STEP3     | R/W  | 0h    | Enable step 3         |
| 2     | STEP2     | R/W  | 0h    | Enable step 2         |
| 1     | STEP1     | R/W  | 0h    | Enable step 1         |
| 0     | TS_Charge | R/W  | 0h    | Enable TS Charge step |

#### **12.5.1.16 IDLECONFIG Register (offset = 58h) [reset = 0h]**

Register mask: FFFFFFFFh

IDLECONFIG is shown in [Figure](#page-32-1) 12-20 and described in Table [12-20](#page-32-2).

Idle Step configuration@TSC\_ADC\_SS\_IDLE\_StepConfig Register

### **Figure 12-20. IDLECONFIG Register**

| 31                  | 30                     | 29              | 28 | 27        | 26        | 25              | 24                  |  |
|---------------------|------------------------|-----------------|----|-----------|-----------|-----------------|---------------------|--|
|                     |                        | RESERVED        |    |           |           | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |
|                     |                        | R/W-0h          |    |           |           | R/W-0h          | R/W-0h              |  |
| 23                  | 22                     | 21              | 20 | 19        | 18        | 17              | 16                  |  |
| SEL_RFM_SW<br>C_1_0 |                        | SEL_INP_SWC_3_0 |    |           |           | SEL_INM_SWC_3_0 |                     |  |
| R/W-0h              |                        | R/W-0h          |    |           | R/W-0h    |                 |                     |  |
| 15                  | 14                     | 13              | 12 | 11        | 10        | 9               | 8                   |  |
| SEL_INM_SW<br>C_3_0 |                        | SEL_RFP_SWC_2_0 |    | WPNSW_SWC | YPNSW_SWC | XNPSW_SWC       | YNNSW_SWC           |  |
| R/W-0h              |                        | R/W-0h          |    | R/W-0h    | R/W-0h    | R/W-0h          | R/W-0h              |  |
| 7                   | 6                      | 5               | 4  | 3         | 2         | 1               | 0                   |  |
| YPPSW_SWC           | XNNSW_SWC<br>XPPSW_SWC |                 |    |           | RESERVED  |                 |                     |  |
| R/W-0h              | R/W-0h<br>R/W-0h       |                 |    |           | R/W-0h    |                 |                     |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-20. IDLECONFIG Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-26 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin.<br>0 = Single Ended.<br>1 = Differential Pair Enable.                                    |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA_ADC.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                        |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for neg differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                      |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |
| 7     | YPPSW_SWC       | R/W  | 0h    | YPPSW pin SW configuration                                                                                         |
| 6     | XNNSW_SWC       | R/W  | 0h    | XNNSW pin SW configuration                                                                                         |

**Table 12-20. IDLECONFIG Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                |
|-----|-----------|------|-------|----------------------------|
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration |
| 4-0 | RESERVED  | R/W  | 0h    |                            |

#### **12.5.1.17 TS\_CHARGE\_STEPCONFIG Register (offset = 5Ch) [reset = 0h]**

Register mask: FFFFFFFFh

TS\_CHARGE\_STEPCONFIG is shown in [Figure](#page-34-1) 12-21 and described in Table [12-21.](#page-34-2)

TS Charge StepConfiguration@TSC\_ADC\_SS\_TS\_Charge\_StepConfig Register

**Figure 12-21. TS\_CHARGE\_STEPCONFIG Register**

| 31                  | 30        | 29              | 28 | 27               | 26             | 25             | 24                  |  |  |  |
|---------------------|-----------|-----------------|----|------------------|----------------|----------------|---------------------|--|--|--|
|                     |           | RESERVED        |    |                  |                | Diff_CNTRL     | SEL_RFM_SW<br>C_1_0 |  |  |  |
|                     |           | R/W-0h          |    |                  | R/W-0h         | R/W-0h         |                     |  |  |  |
| 23                  | 22        | 21              | 20 | 19               | 18<br>17<br>16 |                |                     |  |  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0 |    |                  |                | SEL_INM_SWM3_0 |                     |  |  |  |
| R/W-0h              |           | R/W-0h          |    |                  | R/W-0h         |                |                     |  |  |  |
| 15                  | 14        | 13              | 12 | 11               | 10             | 9              | 8                   |  |  |  |
| SEL_INM_SW<br>M3_0  |           | SEL_RFP_SWC_2_0 |    | WPNSW_SWC        | YPNSW_SWC      | XNPSW_SWC      | YNNSW_SWC           |  |  |  |
| R/W-0h              |           | R/W-0h          |    | R/W-0h           | R/W-0h         | R/W-0h         | R/W-0h              |  |  |  |
| 7                   | 6         | 5               | 4  | 3<br>2<br>1<br>0 |                |                |                     |  |  |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |    |                  | RESERVED       |                |                     |  |  |  |
| R/W-0h              | R/W-0h    | R/W-0h          |    |                  | R/W-0h         |                |                     |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-21. TS\_CHARGE\_STEPCONFIG Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                  |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------|
| 31-26 | RESERVED        | R/W  | 0h    |                                                                                                              |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin.<br>0 = Single Ended.<br>1 = Differential Pair Enable.                              |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                      |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                    |
| 18-15 | SEL_INM_SWM3_0  | R/W  | 0h    | SEL_INM pins for neg differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = INTREF. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                   |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                   |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                   |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                   |
| 7     | YPPSW_SWC       | R/W  | 0h    | YPPSW pin SW configuration                                                                                   |
| 6     | XNNSW_SWC       | R/W  | 0h    | XNNSW pin SW configuration                                                                                   |

#### **Table 12-21. TS\_CHARGE\_STEPCONFIG Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                |
|-----|-----------|------|-------|----------------------------|
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration |
| 4-0 | RESERVED  | R/W  | 0h    |                            |

#### **12.5.1.18 TS\_CHARGE\_DELAY Register (offset = 60h) [reset = 1h]**

Register mask: FFFFFFFFh

TS\_CHARGE\_DELAY is shown in [Figure](#page-36-1) 12-22 and described in Table [12-22.](#page-36-2)

TS Charge Delay Register@TSC\_ADC\_SS\_TS\_Charge\_StepDelay Register

**Figure 12-22. TS\_CHARGE\_DELAY Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25       | 24     | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8         | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----------|--------|----|----|----|----|----|----|----|----|----|----|----|----|----|----|---|-----------|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    | RESERVED |        |    |    |    |    |    |    |    |    |    |    |    |    |    |    |   | OpenDelay |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |          | R/W-0h |    |    |    |    |    |    |    |    |    |    |    |    |    |    |   | R/W-1h    |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-22. TS\_CHARGE\_DELAY Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description                                                                                                                                                         |
|-------|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-18 | RESERVED  | R/W  | 0h    |                                                                                                                                                                     |
| 17-0  | OpenDelay | R/W  | 1h    | Program the # of ADC clock cycles to wait between applying the<br>step configuration registers and going back to the IDLE state.<br>(Value must be greater than 0.) |

#### **12.5.1.19 STEPCONFIG1 Register (offset = 64h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG1 is shown in [Figure](#page-37-1) 12-23 and described in Table [12-23.](#page-37-2)

Step Configuration 1

**Figure 12-23. STEPCONFIG1 Register**

| 31                  | 30        | 29              | 28 | 27                | 26          | 25              | 24                  |  |  |
|---------------------|-----------|-----------------|----|-------------------|-------------|-----------------|---------------------|--|--|
|                     |           | RESERVED        |    | Range_check       | FIFO_select | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |           | R/W-0h          |    | R/W-0h            | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 23                  | 22        | 21              | 20 | 19                | 18          | 17              | 16                  |  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0 |    |                   |             | SEL_INM_SWC_3_0 |                     |  |  |
| R/W-0h              |           | R/W-0h          |    |                   | R/W-0h      |                 |                     |  |  |
| 15                  | 14        | 13              | 12 | 11                | 10          | 9               | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |    | WPNSW_SWC         | YPNSW_SWC   | XNPSW_SWC       | YNNSW_SWC           |  |  |
| R/W-0h              |           | R/W-0h          |    | R/W-0h            | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 7                   | 6         | 5               | 4  | 3                 | 2           | 1<br>0          |                     |  |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |    | Averaging<br>Mode |             |                 |                     |  |  |
| R/W-0h              | R/W-0h    | R/W-0h          |    | R/W-0h            |             |                 | R/W-0h              |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-23. STEPCONFIG1 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO 1.                                                   |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-23. STEPCONFIG1 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                           |
|-----|-----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                            |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                            |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                            |
| 4-2 | Averaging | R/W  | 0h    | Number of samplings to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                    |

#### **12.5.1.20 STEPDELAY1 Register (offset = 68h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY1 is shown in [Figure](#page-39-1) 12-24 and described in Table [12-24.](#page-39-2)

Step Delay Register 1

**Figure 12-24. STEPDELAY1 Register**

| 31 | 30 | 29 | 28 | 27          | 26 | 25 | 24 | 23        | 22 | 21 | 20       | 19 | 18 | 17        | 16 |
|----|----|----|----|-------------|----|----|----|-----------|----|----|----------|----|----|-----------|----|
|    |    |    |    | SampleDelay |    |    |    |           |    |    | RESERVED |    |    | OpenDelay |    |
|    |    |    |    | R/W-0h      |    |    |    |           |    |    | R/W-0h   |    |    | R/W-0h    |    |
| 15 | 14 | 13 | 12 | 11          | 10 | 9  | 8  | 7         | 6  | 5  | 4        | 3  | 2  | 1         | 0  |
|    |    |    |    |             |    |    |    | OpenDelay |    |    |          |    |    |           |    |
|    |    |    |    |             |    |    |    | R/W-0h    |    |    |          |    |    |           |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-24. STEPDELAY1 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.21 STEPCONFIG2 Register (offset = 6Ch) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG2 is shown in [Figure](#page-40-1) 12-25 and described in Table [12-25.](#page-40-2)

Step Configuration 2

**Figure 12-25. STEPCONFIG2 Register**

| 31                  | 30        | 29              | 28 | 27                | 26          | 25              | 24                  |  |  |
|---------------------|-----------|-----------------|----|-------------------|-------------|-----------------|---------------------|--|--|
|                     |           | RESERVED        |    | Range_check       | FIFO_select | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |           | R/W-0h          |    | R/W-0h            | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 23                  | 22        | 21              | 20 | 19                | 18          | 17              | 16                  |  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0 |    |                   |             | SEL_INM_SWC_3_0 |                     |  |  |
| R/W-0h              |           | R/W-0h          |    |                   | R/W-0h      |                 |                     |  |  |
| 15                  | 14        | 13              | 12 | 11                | 10          | 9               | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |    | WPNSW_SWC         | YPNSW_SWC   | XNPSW_SWC       | YNNSW_SWC           |  |  |
| R/W-0h              |           | R/W-0h          |    | R/W-0h            | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 7                   | 6         | 5               | 4  | 3                 | 2           | 1<br>0          |                     |  |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |    | Averaging<br>Mode |             |                 |                     |  |  |
| R/W-0h              | R/W-0h    | R/W-0h          |    | R/W-0h            |             |                 | R/W-0h              |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-25. STEPCONFIG2 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-25. STEPCONFIG2 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.22 STEPDELAY2 Register (offset = 70h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY2 is shown in [Figure](#page-42-1) 12-26 and described in Table [12-26.](#page-42-2)

Step Delay Register 2

**Figure 12-26. STEPDELAY2 Register**

| 31                      | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23        | 22 | 21 | 20 | 19     | 18 | 17        | 16 |
|-------------------------|----|----|----|----|----|----|----|-----------|----|----|----|--------|----|-----------|----|
| SampleDelay<br>RESERVED |    |    |    |    |    |    |    |           |    |    |    |        |    | OpenDelay |    |
| R/W-0h<br>R/W-0h        |    |    |    |    |    |    |    |           |    |    |    | R/W-0h |    |           |    |
| 15                      | 14 | 13 | 12 | 11 | 10 | 9  | 8  | 7         | 6  | 5  | 4  | 3      | 2  | 1         | 0  |
|                         |    |    |    |    |    |    |    | OpenDelay |    |    |    |        |    |           |    |
|                         |    |    |    |    |    |    |    | R/W-0h    |    |    |    |        |    |           |    |
|                         |    |    |    |    |    |    |    |           |    |    |    |        |    |           |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-26. STEPDELAY2 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |  |  |  |  |  |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|--|--|--|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |  |  |  |  |  |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |  |  |  |  |  |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |  |  |  |  |  |

#### **12.5.1.23 STEPCONFIG3 Register (offset = 74h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG3 is shown in [Figure](#page-43-1) 12-27 and described in Table [12-27.](#page-43-2)

Step Configuration 3

**Figure 12-27. STEPCONFIG3 Register**

| 31                  | 30        | 29              | 28 | 27                | 26          | 25              | 24                  |  |  |
|---------------------|-----------|-----------------|----|-------------------|-------------|-----------------|---------------------|--|--|
|                     |           | RESERVED        |    | Range_check       | FIFO_select | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |           | R/W-0h          |    | R/W-0h            | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 23                  | 22        | 21              | 20 | 19                | 18          | 17<br>16        |                     |  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0 |    |                   |             | SEL_INM_SWC_3_0 |                     |  |  |
| R/W-0h              |           | R/W-0h          |    |                   |             | R/W-0h          |                     |  |  |
| 15                  | 14        | 13              | 12 | 11                | 10          | 9               | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |    | WPNSW_SWC         | YPNSW_SWC   | XNPSW_SWC       | YNNSW_SWC           |  |  |
| R/W-0h              |           | R/W-0h          |    | R/W-0h            | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 7                   | 6         | 5               | 4  | 3                 | 2           | 1               | 0                   |  |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |    | Averaging<br>Mode |             |                 |                     |  |  |
| R/W-0h              | R/W-0h    | R/W-0h          |    | R/W-0h            | R/W-0h      |                 |                     |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-27. STEPCONFIG3 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-27. STEPCONFIG3 Register Field Descriptions (continued)**

| Bit | Field                  | Type | Reset | Description                                                                                                                                                         |
|-----|------------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC              | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC<br>R/W<br>0h |      |       | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC              | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging              | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode                   | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.24 STEPDELAY3 Register (offset = 78h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY3 is shown in [Figure](#page-45-1) 12-28 and described in Table [12-28.](#page-45-2)

Step Delay Register 3

**Figure 12-28. STEPDELAY3 Register**

| 31     | 30 | 29 | 28 | 27          | 26 | 25 | 24 | 23        | 22 | 21 | 20       | 19 | 18 | 17        | 16 |
|--------|----|----|----|-------------|----|----|----|-----------|----|----|----------|----|----|-----------|----|
|        |    |    |    | SampleDelay |    |    |    |           |    |    | RESERVED |    |    | OpenDelay |    |
|        |    |    |    | R/W-0h      |    |    |    |           |    |    | R/W-0h   |    |    | R/W-0h    |    |
| 15     | 14 | 13 | 12 | 11          | 10 | 9  | 8  | 7         | 6  | 5  | 4        | 3  | 2  | 1         | 0  |
|        |    |    |    |             |    |    |    | OpenDelay |    |    |          |    |    |           |    |
| R/W-0h |    |    |    |             |    |    |    |           |    |    |          |    |    |           |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-28. STEPDELAY3 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |  |  |  |  |  |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|--|--|--|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |  |  |  |  |  |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |  |  |  |  |  |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |  |  |  |  |  |

#### **12.5.1.25 STEPCONFIG4 Register (offset = 7Ch) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG4 is shown in [Figure](#page-46-1) 12-29 and described in Table [12-29.](#page-46-2)

Step Configuration 4

**Figure 12-29. STEPCONFIG4 Register**

| 31                  | 30        | 29              | 28 | 27                | 26          | 25              | 24                  |  |  |
|---------------------|-----------|-----------------|----|-------------------|-------------|-----------------|---------------------|--|--|
|                     |           | RESERVED        |    | Range_check       | FIFO_select | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |           | R/W-0h          |    | R/W-0h            | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 23                  | 22        | 21              | 20 | 19                | 18          | 17<br>16        |                     |  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0 |    |                   |             | SEL_INM_SWC_3_0 |                     |  |  |
| R/W-0h              |           | R/W-0h          |    |                   |             | R/W-0h          |                     |  |  |
| 15                  | 14        | 13              | 12 | 11                | 10          | 9               | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |    | WPNSW_SWC         | YPNSW_SWC   | XNPSW_SWC       | YNNSW_SWC           |  |  |
| R/W-0h              |           | R/W-0h          |    | R/W-0h            | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 7                   | 6         | 5               | 4  | 3                 | 2           | 1               | 0                   |  |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |    | Averaging<br>Mode |             |                 |                     |  |  |
| R/W-0h              | R/W-0h    | R/W-0h          |    | R/W-0h            | R/W-0h      |                 |                     |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-29. STEPCONFIG4 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-29. STEPCONFIG4 Register Field Descriptions (continued)**

| Bit | Field                  | Type | Reset | Description                                                                                                                                                         |
|-----|------------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC              | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC<br>R/W<br>0h |      |       | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC              | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging              | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode                   | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.26 STEPDELAY4 Register (offset = 80h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY4 is shown in [Figure](#page-48-1) 12-30 and described in Table [12-30.](#page-48-2)

Step Delay Register 4

**Figure 12-30. STEPDELAY4 Register**

| 31                      | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23        | 22 | 21 | 20 | 19     | 18 | 17        | 16 |
|-------------------------|----|----|----|----|----|----|----|-----------|----|----|----|--------|----|-----------|----|
| SampleDelay<br>RESERVED |    |    |    |    |    |    |    |           |    |    |    |        |    | OpenDelay |    |
| R/W-0h<br>R/W-0h        |    |    |    |    |    |    |    |           |    |    |    | R/W-0h |    |           |    |
| 15                      | 14 | 13 | 12 | 11 | 10 | 9  | 8  | 7         | 6  | 5  | 4  | 3      | 2  | 1         | 0  |
|                         |    |    |    |    |    |    |    | OpenDelay |    |    |    |        |    |           |    |
|                         |    |    |    |    |    |    |    | R/W-0h    |    |    |    |        |    |           |    |
|                         |    |    |    |    |    |    |    |           |    |    |    |        |    |           |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-30. STEPDELAY4 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |  |  |  |  |  |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|--|--|--|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |  |  |  |  |  |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |  |  |  |  |  |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |  |  |  |  |  |

#### **12.5.1.27 STEPCONFIG5 Register (offset = 84h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG5 is shown in [Figure](#page-49-1) 12-31 and described in Table [12-31.](#page-49-2)

Step Configuration 5

**Figure 12-31. STEPCONFIG5 Register**

| 31                  | 30        | 29              | 28 | 27                | 26          | 25              | 24                  |  |  |
|---------------------|-----------|-----------------|----|-------------------|-------------|-----------------|---------------------|--|--|
|                     |           | RESERVED        |    | Range_check       | FIFO_select | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |           | R/W-0h          |    | R/W-0h            | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 23                  | 22        | 21              | 20 | 19                | 18          | 17<br>16        |                     |  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0 |    |                   |             | SEL_INM_SWC_3_0 |                     |  |  |
| R/W-0h              |           | R/W-0h          |    |                   |             | R/W-0h          |                     |  |  |
| 15                  | 14        | 13              | 12 | 11                | 10          | 9               | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |    | WPNSW_SWC         | YPNSW_SWC   | XNPSW_SWC       | YNNSW_SWC           |  |  |
| R/W-0h              |           | R/W-0h          |    | R/W-0h            | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 7                   | 6         | 5               | 4  | 3                 | 2           | 1               | 0                   |  |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |    | Averaging<br>Mode |             |                 |                     |  |  |
| R/W-0h              | R/W-0h    | R/W-0h          |    | R/W-0h            | R/W-0h      |                 |                     |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-31. STEPCONFIG5 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-31. STEPCONFIG5 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.28 STEPDELAY5 Register (offset = 88h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY5 is shown in [Figure](#page-51-1) 12-32 and described in Table [12-32.](#page-51-2)

Step Delay Register 5

**Figure 12-32. STEPDELAY5 Register**

| 31     | 30        | 29 | 28 | 27          | 26 | 25 | 24 | 23     | 22 | 21 | 20       | 19 | 18 | 17        | 16 |
|--------|-----------|----|----|-------------|----|----|----|--------|----|----|----------|----|----|-----------|----|
|        |           |    |    | SampleDelay |    |    |    |        |    |    | RESERVED |    |    | OpenDelay |    |
| R/W-0h |           |    |    |             |    |    |    | R/W-0h |    |    |          |    |    | R/W-0h    |    |
| 15     | 14        | 13 | 12 | 11          | 10 | 9  | 8  | 7      | 6  | 5  | 4        | 3  | 2  | 1         | 0  |
|        | OpenDelay |    |    |             |    |    |    |        |    |    |          |    |    |           |    |
| R/W-0h |           |    |    |             |    |    |    |        |    |    |          |    |    |           |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-32. STEPDELAY5 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.29 STEPCONFIG6 Register (offset = 8Ch) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG6 is shown in [Figure](#page-52-1) 12-33 and described in Table [12-33.](#page-52-2)

Step Configuration 6

**Figure 12-33. STEPCONFIG6 Register**

| 31                  | 30                     | 29              | 28 | 27          | 26          | 25              | 24                  |  |  |
|---------------------|------------------------|-----------------|----|-------------|-------------|-----------------|---------------------|--|--|
|                     |                        | RESERVED        |    | Range_check | FIFO_select | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |                        | R/W-0h          |    | R/W-0h      | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 23                  | 22                     | 21              | 20 | 19          | 18          | 17              | 16                  |  |  |
| SEL_RFM_SW<br>C_1_0 |                        | SEL_INP_SWC_3_0 |    |             |             | SEL_INM_SWC_3_0 |                     |  |  |
| R/W-0h              |                        | R/W-0h          |    |             | R/W-0h      |                 |                     |  |  |
| 15                  | 14                     | 13              | 12 | 11          | 10          | 9               | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |                        | SEL_RFP_SWC_2_0 |    | WPNSW_SWC   | YPNSW_SWC   | XNPSW_SWC       | YNNSW_SWC           |  |  |
| R/W-0h              |                        | R/W-0h          |    | R/W-0h      | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 7                   | 6                      | 5               | 4  | 3           | 2           | 1               | 0                   |  |  |
| YPPSW_SWC           | XNNSW_SWC<br>XPPSW_SWC |                 |    | Averaging   |             | Mode            |                     |  |  |
| R/W-0h              | R/W-0h                 | R/W-0h          |    | R/W-0h      |             |                 | R/W-0h              |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-33. STEPCONFIG6 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-33. STEPCONFIG6 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.30 STEPDELAY6 Register (offset = 90h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY6 is shown in [Figure](#page-54-1) 12-34 and described in Table [12-34.](#page-54-2)

Step Delay Register 6

**Figure 12-34. STEPDELAY6 Register**

|        | 31        | 30                      | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21     | 20 | 19        | 18 | 17     | 16 |
|--------|-----------|-------------------------|----|----|----|----|----|----|----|----|--------|----|-----------|----|--------|----|
|        |           | SampleDelay<br>RESERVED |    |    |    |    |    |    |    |    |        |    | OpenDelay |    |        |    |
| R/W-0h |           |                         |    |    |    |    |    |    |    |    | R/W-0h |    |           |    | R/W-0h |    |
|        | 15        | 14                      | 13 | 12 | 11 | 10 | 9  | 8  | 7  | 6  | 5      | 4  | 3         | 2  | 1      | 0  |
|        | OpenDelay |                         |    |    |    |    |    |    |    |    |        |    |           |    |        |    |
|        | R/W-0h    |                         |    |    |    |    |    |    |    |    |        |    |           |    |        |    |
|        |           |                         |    |    |    |    |    |    |    |    |        |    |           |    |        |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-34. STEPDELAY6 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.31 STEPCONFIG7 Register (offset = 94h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG7 is shown in [Figure](#page-55-1) 12-35 and described in Table [12-35.](#page-55-2)

Step Configuration 7

**Figure 12-35. STEPCONFIG7 Register**

| 31                  | 30                     | 29              | 28 | 27          | 26          | 25              | 24                  |  |  |
|---------------------|------------------------|-----------------|----|-------------|-------------|-----------------|---------------------|--|--|
|                     |                        | RESERVED        |    | Range_check | FIFO_select | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |                        | R/W-0h          |    | R/W-0h      | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 23                  | 22                     | 21              | 20 | 19          | 18          | 17              | 16                  |  |  |
| SEL_RFM_SW<br>C_1_0 |                        | SEL_INP_SWC_3_0 |    |             |             | SEL_INM_SWC_3_0 |                     |  |  |
| R/W-0h              |                        | R/W-0h          |    |             | R/W-0h      |                 |                     |  |  |
| 15                  | 14                     | 13              | 12 | 11          | 10          | 9               | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |                        | SEL_RFP_SWC_2_0 |    | WPNSW_SWC   | YPNSW_SWC   | XNPSW_SWC       | YNNSW_SWC           |  |  |
| R/W-0h              |                        | R/W-0h          |    | R/W-0h      | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 7                   | 6                      | 5               | 4  | 3           | 2           | 1               | 0                   |  |  |
| YPPSW_SWC           | XNNSW_SWC<br>XPPSW_SWC |                 |    | Averaging   |             | Mode            |                     |  |  |
| R/W-0h              | R/W-0h                 | R/W-0h          |    | R/W-0h      |             |                 | R/W-0h              |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-35. STEPCONFIG7 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-35. STEPCONFIG7 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.32 STEPDELAY7 Register (offset = 98h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY7 is shown in [Figure](#page-57-1) 12-36 and described in Table [12-36.](#page-57-2)

Step Delay Register 7

**Figure 12-36. STEPDELAY7 Register**

| 31     | 30        | 29 | 28 | 27          | 26 | 25 | 24 | 23     | 22 | 21 | 20       | 19 | 18 | 17        | 16 |
|--------|-----------|----|----|-------------|----|----|----|--------|----|----|----------|----|----|-----------|----|
|        |           |    |    | SampleDelay |    |    |    |        |    |    | RESERVED |    |    | OpenDelay |    |
| R/W-0h |           |    |    |             |    |    |    | R/W-0h |    |    |          |    |    | R/W-0h    |    |
| 15     | 14        | 13 | 12 | 11          | 10 | 9  | 8  | 7      | 6  | 5  | 4        | 3  | 2  | 1         | 0  |
|        | OpenDelay |    |    |             |    |    |    |        |    |    |          |    |    |           |    |
| R/W-0h |           |    |    |             |    |    |    |        |    |    |          |    |    |           |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-36. STEPDELAY7 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.33 STEPCONFIG8 Register (offset = 9Ch) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG8 is shown in [Figure](#page-58-1) 12-37 and described in Table [12-37.](#page-58-2)

Step Configuration 8

**Figure 12-37. STEPCONFIG8 Register**

| 31                  | 30                     | 29              | 28 | 27                | 26                         | 25              | 24                  |  |  |
|---------------------|------------------------|-----------------|----|-------------------|----------------------------|-----------------|---------------------|--|--|
|                     |                        | RESERVED        |    | Range_check       | FIFO_select                | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |                        | R/W-0h          |    | R/W-0h            | R/W-0h<br>R/W-0h<br>R/W-0h |                 |                     |  |  |
| 23                  | 22                     | 21              | 20 | 19                | 18                         | 17              | 16                  |  |  |
| SEL_RFM_SW<br>C_1_0 |                        | SEL_INP_SWC_3_0 |    |                   |                            | SEL_INM_SWC_3_0 |                     |  |  |
| R/W-0h              |                        | R/W-0h          |    |                   |                            | R/W-0h          |                     |  |  |
| 15                  | 14                     | 13              | 12 | 11                | 10                         | 9               | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |                        | SEL_RFP_SWC_2_0 |    | WPNSW_SWC         | YPNSW_SWC                  | XNPSW_SWC       | YNNSW_SWC           |  |  |
| R/W-0h              |                        | R/W-0h          |    | R/W-0h            | R/W-0h                     | R/W-0h          | R/W-0h              |  |  |
| 7                   | 6                      | 5               | 4  | 3                 | 2                          | 1               | 0                   |  |  |
| YPPSW_SWC           | XNNSW_SWC<br>XPPSW_SWC |                 |    | Averaging<br>Mode |                            |                 |                     |  |  |
| R/W-0h              | R/W-0h                 | R/W-0h          |    | R/W-0h            | R/W-0h                     |                 |                     |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-37. STEPCONFIG8 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-37. STEPCONFIG8 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.34 STEPDELAY8 Register (offset = A0h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY8 is shown in [Figure](#page-60-1) 12-38 and described in Table [12-38.](#page-60-2)

Step Delay Register 8

**Figure 12-38. STEPDELAY8 Register**

| 31               | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23        | 22       | 21 | 20 | 19 | 18     | 17 | 16        |  |  |
|------------------|----|----|----|----|----|----|----|-----------|----------|----|----|----|--------|----|-----------|--|--|
| SampleDelay      |    |    |    |    |    |    |    |           | RESERVED |    |    |    |        |    | OpenDelay |  |  |
| R/W-0h<br>R/W-0h |    |    |    |    |    |    |    |           |          |    |    |    | R/W-0h |    |           |  |  |
| 15               | 14 | 13 | 12 | 11 | 10 | 9  | 8  | 7         | 6        | 5  | 4  | 3  | 2      | 1  | 0         |  |  |
|                  |    |    |    |    |    |    |    | OpenDelay |          |    |    |    |        |    |           |  |  |
|                  |    |    |    |    |    |    |    | R/W-0h    |          |    |    |    |        |    |           |  |  |
|                  |    |    |    |    |    |    |    |           |          |    |    |    |        |    |           |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-38. STEPDELAY8 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.35 STEPCONFIG9 Register (offset = A4h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG9 is shown in [Figure](#page-61-1) 12-39 and described in Table [12-39.](#page-61-2)

Step Configuration 9

**Figure 12-39. STEPCONFIG9 Register**

| 31                  | 30        | 29              | 28 | 27               | 26                         | 25              | 24                  |  |  |
|---------------------|-----------|-----------------|----|------------------|----------------------------|-----------------|---------------------|--|--|
|                     |           | RESERVED        |    | Range_check      | FIFO_select                | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |           | R/W-0h          |    | R/W-0h           | R/W-0h<br>R/W-0h<br>R/W-0h |                 |                     |  |  |
| 23                  | 22        | 21              | 20 | 19               | 18                         | 17              | 16                  |  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0 |    |                  |                            | SEL_INM_SWC_3_0 |                     |  |  |
| R/W-0h              |           | R/W-0h          |    |                  | R/W-0h                     |                 |                     |  |  |
| 15                  | 14        | 13              | 12 | 11               | 10                         | 9               | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |    | WPNSW_SWC        | YPNSW_SWC                  | XNPSW_SWC       | YNNSW_SWC           |  |  |
| R/W-0h              |           | R/W-0h          |    | R/W-0h           | R/W-0h                     | R/W-0h          | R/W-0h              |  |  |
| 7                   | 6         | 5               | 4  | 3                | 2                          | 1               | 0                   |  |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |    | Averaging        |                            | Mode            |                     |  |  |
| R/W-0h              | R/W-0h    | R/W-0h          |    | R/W-0h<br>R/W-0h |                            |                 |                     |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-39. STEPCONFIG9 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO 1.<br>= FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-39. STEPCONFIG9 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.36 STEPDELAY9 Register (offset = A8h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY9 is shown in [Figure](#page-63-1) 12-40 and described in Table [12-40.](#page-63-2)

Step Delay Register 9

**Figure 12-40. STEPDELAY9 Register**

| 31 | 30          | 29 | 28 | 27 | 26     | 25 | 24 | 23        | 22       | 21 | 20 | 19 | 18 | 17 | 16        |  |  |
|----|-------------|----|----|----|--------|----|----|-----------|----------|----|----|----|----|----|-----------|--|--|
|    | SampleDelay |    |    |    |        |    |    |           | RESERVED |    |    |    |    |    | OpenDelay |  |  |
|    |             |    |    |    | R/W-0h |    |    | R/W-0h    |          |    |    |    |    |    |           |  |  |
| 15 | 14          | 13 | 12 | 11 | 10     | 9  | 8  | 7         | 6        | 5  | 4  | 3  | 2  | 1  | 0         |  |  |
|    |             |    |    |    |        |    |    | OpenDelay |          |    |    |    |    |    |           |  |  |
|    |             |    |    |    |        |    |    | R/W-0h    |          |    |    |    |    |    |           |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-40. STEPDELAY9 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.37 STEPCONFIG10 Register (offset = ACh) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG10 is shown in [Figure](#page-64-1) 12-41 and described in Table [12-41](#page-64-2).

Step Configuration 10

**Figure 12-41. STEPCONFIG10 Register**

| 31                  | 30        | 29              | 28              | 27               | 26             | 25              | 24                  |  |  |
|---------------------|-----------|-----------------|-----------------|------------------|----------------|-----------------|---------------------|--|--|
|                     |           | RESERVED        |                 | Range_check      | FIFO_select    | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |           | R/W-0h          |                 | R/W-0h           | R/W-0h         | R/W-0h          |                     |  |  |
| 23                  | 22        | 21              | 20              | 19               | 18<br>17<br>16 |                 |                     |  |  |
| SEL_RFM_SW<br>C_1_0 |           |                 | SEL_INP_SWC_3_0 |                  |                | SEL_INM_SWC_3_0 |                     |  |  |
| R/W-0h              |           |                 | R/W-0h          |                  |                | R/W-0h          |                     |  |  |
| 15                  | 14        | 13              | 12              | 11               | 10             | 9               | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |                 | WPNSW_SWC        | YPNSW_SWC      | XNPSW_SWC       | YNNSW_SWC           |  |  |
| R/W-0h              |           | R/W-0h          |                 | R/W-0h           | R/W-0h         | R/W-0h          | R/W-0h              |  |  |
| 7                   | 6         | 5               | 4               | 3                | 2              | 1               | 0                   |  |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |                 | Averaging        | Mode           |                 |                     |  |  |
| R/W-0h              | R/W-0h    | R/W-0h          |                 | R/W-0h<br>R/W-0h |                |                 |                     |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-41. STEPCONFIG10 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-41. STEPCONFIG10 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |  |  |  |  |  |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|--|--|--|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |  |  |  |  |  |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |  |  |  |  |  |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |  |  |  |  |  |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |  |  |  |  |  |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |  |  |  |  |  |

#### **12.5.1.38 STEPDELAY10 Register (offset = B0h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY10 is shown in [Figure](#page-66-1) 12-42 and described in Table [12-42](#page-66-2).

Step Delay Register 10

**Figure 12-42. STEPDELAY10 Register**

| 31          | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23        | 22       | 21 | 20 | 19 | 18 | 17 | 16        |  |  |
|-------------|----|----|----|----|----|----|----|-----------|----------|----|----|----|----|----|-----------|--|--|
| SampleDelay |    |    |    |    |    |    |    |           | RESERVED |    |    |    |    |    | OpenDelay |  |  |
| R/W-0h      |    |    |    |    |    |    |    | R/W-0h    |          |    |    |    |    |    | R/W-0h    |  |  |
| 15          | 14 | 13 | 12 | 11 | 10 | 9  | 8  | 7         | 6        | 5  | 4  | 3  | 2  | 1  | 0         |  |  |
|             |    |    |    |    |    |    |    | OpenDelay |          |    |    |    |    |    |           |  |  |
|             |    |    |    |    |    |    |    | R/W-0h    |          |    |    |    |    |    |           |  |  |
|             |    |    |    |    |    |    |    |           |          |    |    |    |    |    |           |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-42. STEPDELAY10 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.39 STEPCONFIG11 Register (offset = B4h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG11 is shown in [Figure](#page-67-1) 12-43 and described in Table [12-43](#page-67-2).

Step Configuration 11

**Figure 12-43. STEPCONFIG11 Register**

| 31                  | 30        | 29              | 28              | 27          | 26          | 25              | 24                  |  |
|---------------------|-----------|-----------------|-----------------|-------------|-------------|-----------------|---------------------|--|
|                     |           | RESERVED        |                 | Range_check | FIFO_select | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |
|                     |           | R/W-0h          |                 | R/W-0h      | R/W-0h      | R/W-0h          | R/W-0h              |  |
| 23                  | 22        | 21              | 20              | 19          | 18          | 17              | 16                  |  |
| SEL_RFM_SW<br>C_1_0 |           |                 | SEL_INP_SWC_3_0 |             |             | SEL_INM_SWC_3_0 |                     |  |
| R/W-0h              |           |                 | R/W-0h          |             |             | R/W-0h          |                     |  |
| 15                  | 14        | 13              | 12              | 11          | 10          | 9               | 8                   |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |                 | WPNSW_SWC   | YPNSW_SWC   | YNNSW_SWC       |                     |  |
| R/W-0h              |           | R/W-0h          |                 | R/W-0h      | R/W-0h      | R/W-0h          | R/W-0h              |  |
| 7                   | 6         | 5               | 4               | 3           | 2           | 1               | 0                   |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |                 | Averaging   |             | Mode            |                     |  |
| R/W-0h              | R/W-0h    | R/W-0h          |                 | R/W-0h      | R/W-0h      |                 |                     |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-43. STEPCONFIG11 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-43. STEPCONFIG11 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.40 STEPDELAY11 Register (offset = B8h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY11 is shown in [Figure](#page-69-1) 12-44 and described in Table [12-44](#page-69-2).

Step Delay Register 11

**Figure 12-44. STEPDELAY11 Register**

| 31          | 30     | 29 | 28 | 27 | 26 | 25 | 24 | 23        | 22 | 21 | 20 | 19 | 18 | 17        | 16 |  |
|-------------|--------|----|----|----|----|----|----|-----------|----|----|----|----|----|-----------|----|--|
| SampleDelay |        |    |    |    |    |    |    | RESERVED  |    |    |    |    |    | OpenDelay |    |  |
| R/W-0h      |        |    |    |    |    |    |    | R/W-0h    |    |    |    |    |    | R/W-0h    |    |  |
| 15          | 14     | 13 | 12 | 11 | 10 | 9  | 8  | 7         | 6  | 5  | 4  | 3  | 2  | 1         | 0  |  |
|             |        |    |    |    |    |    |    | OpenDelay |    |    |    |    |    |           |    |  |
|             | R/W-0h |    |    |    |    |    |    |           |    |    |    |    |    |           |    |  |
|             |        |    |    |    |    |    |    |           |    |    |    |    |    |           |    |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-44. STEPDELAY11 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.41 STEPCONFIG12 Register (offset = BCh) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG12 is shown in [Figure](#page-70-1) 12-45 and described in Table [12-45](#page-70-2).

Step Configuration 12

**Figure 12-45. STEPCONFIG12 Register**

| 31                  | 30        | 29              | 28     | 27          | 26                                  | 25              | 24                  |  |
|---------------------|-----------|-----------------|--------|-------------|-------------------------------------|-----------------|---------------------|--|
|                     |           | RESERVED        |        | Range_check | FIFO_select                         | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |
|                     |           | R/W-0h          |        | R/W-0h      | R/W-0h                              | R/W-0h          | R/W-0h              |  |
| 23                  | 22        | 21              | 20     | 19          | 18                                  | 17              | 16                  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0 |        |             |                                     | SEL_INM_SWC_3_0 |                     |  |
| R/W-0h              |           |                 | R/W-0h |             |                                     | R/W-0h          |                     |  |
| 15                  | 14        | 13              | 12     | 11          | 10                                  | 9               | 8                   |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |        | WPNSW_SWC   | YPNSW_SWC<br>XNPSW_SWC<br>YNNSW_SWC |                 |                     |  |
| R/W-0h              |           | R/W-0h          |        | R/W-0h      | R/W-0h                              | R/W-0h          | R/W-0h              |  |
| 7                   | 6         | 5               | 4      | 3           | 2                                   | 1               | 0                   |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |        | Averaging   |                                     |                 | Mode                |  |
| R/W-0h              | R/W-0h    | R/W-0h          |        | R/W-0h      | R/W-0h                              |                 |                     |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-45. STEPCONFIG12 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-45. STEPCONFIG12 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.42 STEPDELAY12 Register (offset = C0h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY12 is shown in [Figure](#page-72-1) 12-46 and described in Table [12-46](#page-72-2).

Step Delay Register 12

**Figure 12-46. STEPDELAY12 Register**

|             | 31        | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23       | 22 | 21 | 20 | 19 | 18 | 17        | 16     |  |
|-------------|-----------|----|----|----|----|----|----|----|----------|----|----|----|----|----|-----------|--------|--|
| SampleDelay |           |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    | OpenDelay |        |  |
| R/W-0h      |           |    |    |    |    |    |    |    | R/W-0h   |    |    |    |    |    |           | R/W-0h |  |
|             | 15        | 14 | 13 | 12 | 11 | 10 | 9  | 8  | 7        | 6  | 5  | 4  | 3  | 2  | 1         | 0      |  |
|             | OpenDelay |    |    |    |    |    |    |    |          |    |    |    |    |    |           |        |  |
|             | R/W-0h    |    |    |    |    |    |    |    |          |    |    |    |    |    |           |        |  |
|             |           |    |    |    |    |    |    |    |          |    |    |    |    |    |           |        |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-46. STEPDELAY12 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.43 STEPCONFIG13 Register (offset = C4h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG13 is shown in [Figure](#page-73-1) 12-47 and described in Table [12-47](#page-73-2).

Step Configuration 13

**Figure 12-47. STEPCONFIG13 Register**

| 31                  | 30        | 29               | 28 | 27          | 26          | 25              | 24                  |  |  |
|---------------------|-----------|------------------|----|-------------|-------------|-----------------|---------------------|--|--|
|                     |           | RESERVED         |    | Range_check | FIFO_select | Diff_CNTRL      | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |           | R/W-0h           |    | R/W-0h      | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 23                  | 22        | 21               | 20 | 19          | 18          | 17              | 16                  |  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0  |    |             |             | SEL_INM_SWC_3_0 |                     |  |  |
| R/W-0h              |           | R/W-0h           |    |             | R/W-0h      |                 |                     |  |  |
| 15                  | 14        | 13               | 12 | 11          | 10          | 9               | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0  |    | WPNSW_SWC   | YPNSW_SWC   | YNNSW_SWC       |                     |  |  |
| R/W-0h              |           | R/W-0h           |    | R/W-0h      | R/W-0h      | R/W-0h          | R/W-0h              |  |  |
| 7                   | 6         | 5                |    | 3           | 2           | 1               | 0                   |  |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC        |    | Averaging   |             |                 | Mode                |  |  |
| R/W-0h              | R/W-0h    | R/W-0h<br>R/W-0h |    |             |             |                 |                     |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-47. STEPCONFIG13 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-47. STEPCONFIG13 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.44 STEPDELAY13 Register (offset = C8h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY13 is shown in [Figure](#page-75-1) 12-48 and described in Table [12-48](#page-75-2).

Step Delay Register 13

**Figure 12-48. STEPDELAY13 Register**

| 31          | 30     | 29 | 28 | 27 | 26 | 25 | 24 | 23        | 22 | 21 | 20 | 19 | 18 | 17        | 16 |  |
|-------------|--------|----|----|----|----|----|----|-----------|----|----|----|----|----|-----------|----|--|
| SampleDelay |        |    |    |    |    |    |    | RESERVED  |    |    |    |    |    | OpenDelay |    |  |
| R/W-0h      |        |    |    |    |    |    |    | R/W-0h    |    |    |    |    |    | R/W-0h    |    |  |
| 15          | 14     | 13 | 12 | 11 | 10 | 9  | 8  | 7         | 6  | 5  | 4  | 3  | 2  | 1         | 0  |  |
|             |        |    |    |    |    |    |    | OpenDelay |    |    |    |    |    |           |    |  |
|             | R/W-0h |    |    |    |    |    |    |           |    |    |    |    |    |           |    |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-48. STEPDELAY13 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.45 STEPCONFIG14 Register (offset = CCh) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG14 is shown in [Figure](#page-76-1) 12-49 and described in Table [12-49](#page-76-2).

Step Configuration 14

**Figure 12-49. STEPCONFIG14 Register**

| 31                  | 30        | 29              | 28     | 27                | 26              | 25         | 24                  |  |
|---------------------|-----------|-----------------|--------|-------------------|-----------------|------------|---------------------|--|
|                     |           | RESERVED        |        | Range_check       | FIFO_select     | Diff_CNTRL | SEL_RFM_SW<br>C_1_0 |  |
|                     |           | R/W-0h          |        | R/W-0h            | R/W-0h          | R/W-0h     | R/W-0h              |  |
| 23                  | 22        | 21              | 20     | 19                | 18              | 17         | 16                  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0 |        |                   | SEL_INM_SWC_3_0 |            |                     |  |
| R/W-0h              |           |                 | R/W-0h |                   |                 | R/W-0h     |                     |  |
| 15                  | 14        | 13              | 12     | 11                | 10              | 9          | 8                   |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |        | WPNSW_SWC         | YPNSW_SWC       | XNPSW_SWC  | YNNSW_SWC           |  |
| R/W-0h              |           | R/W-0h          |        | R/W-0h            | R/W-0h          | R/W-0h     | R/W-0h              |  |
| 7                   | 6         | 5               | 4      | 3                 | 2               | 1          | 0                   |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |        | Averaging<br>Mode |                 |            |                     |  |
| R/W-0h              | R/W-0h    | R/W-0h          |        | R/W-0h            |                 |            | R/W-0h              |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-49. STEPCONFIG14 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-49. STEPCONFIG14 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.46 STEPDELAY14 Register (offset = D0h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY14 is shown in [Figure](#page-78-1) 12-50 and described in Table [12-50](#page-78-2).

Step Delay Register 14

**Figure 12-50. STEPDELAY14 Register**

| 31          | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23        | 22 | 21 | 20       | 19 | 18 | 17 | 16        |
|-------------|----|----|----|----|----|----|----|-----------|----|----|----------|----|----|----|-----------|
| SampleDelay |    |    |    |    |    |    |    |           |    |    | RESERVED |    |    |    | OpenDelay |
| R/W-0h      |    |    |    |    |    |    |    |           |    |    | R/W-0h   |    |    |    | R/W-0h    |
| 15          | 14 | 13 | 12 | 11 | 10 | 9  | 8  | 7         | 6  | 5  | 4        | 3  | 2  | 1  | 0         |
|             |    |    |    |    |    |    |    | OpenDelay |    |    |          |    |    |    |           |
|             |    |    |    |    |    |    |    | R/W-0h    |    |    |          |    |    |    |           |
|             |    |    |    |    |    |    |    |           |    |    |          |    |    |    |           |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-50. STEPDELAY14 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.47 STEPCONFIG15 Register (offset = D4h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG15 is shown in [Figure](#page-79-1) 12-51 and described in Table [12-51](#page-79-2).

Step Configuration 15

**Figure 12-51. STEPCONFIG15 Register**

| 31                  | 30        | 29              | 28     | 27                | 26              | 25         | 24                  |  |
|---------------------|-----------|-----------------|--------|-------------------|-----------------|------------|---------------------|--|
|                     |           | RESERVED        |        | Range_check       | FIFO_select     | Diff_CNTRL | SEL_RFM_SW<br>C_1_0 |  |
|                     |           | R/W-0h          |        | R/W-0h            | R/W-0h          | R/W-0h     | R/W-0h              |  |
| 23                  | 22        | 21              | 20     | 19                | 18              | 17         | 16                  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0 |        |                   | SEL_INM_SWC_3_0 |            |                     |  |
| R/W-0h              |           |                 | R/W-0h |                   |                 | R/W-0h     |                     |  |
| 15                  | 14        | 13              | 12     | 11                | 10              | 9          | 8                   |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |        | WPNSW_SWC         | YPNSW_SWC       | XNPSW_SWC  | YNNSW_SWC           |  |
| R/W-0h              |           | R/W-0h          |        | R/W-0h            | R/W-0h          | R/W-0h     | R/W-0h              |  |
| 7                   | 6         | 5               | 4      | 3                 | 2               | 1          | 0                   |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |        | Averaging<br>Mode |                 |            |                     |  |
| R/W-0h              | R/W-0h    | R/W-0h          |        | R/W-0h            |                 |            | R/W-0h              |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-51. STEPCONFIG15 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-51. STEPCONFIG15 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.48 STEPDELAY15 Register (offset = D8h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY15 is shown in [Figure](#page-81-1) 12-52 and described in Table [12-52](#page-81-2).

Step Delay Register 15

**Figure 12-52. STEPDELAY15 Register**

| 31 | 30          | 29 | 28 | 27 | 26 | 25 | 24 | 23        | 22 | 21 | 20       | 19 | 18 | 17        | 16 |
|----|-------------|----|----|----|----|----|----|-----------|----|----|----------|----|----|-----------|----|
|    | SampleDelay |    |    |    |    |    |    |           |    |    | RESERVED |    |    | OpenDelay |    |
|    | R/W-0h      |    |    |    |    |    |    |           |    |    | R/W-0h   |    |    | R/W-0h    |    |
| 15 | 14          | 13 | 12 | 11 | 10 | 9  | 8  | 7         | 6  | 5  | 4        | 3  | 2  | 1         | 0  |
|    |             |    |    |    |    |    |    | OpenDelay |    |    |          |    |    |           |    |
|    |             |    |    |    |    |    |    | R/W-0h    |    |    |          |    |    |           |    |
|    |             |    |    |    |    |    |    |           |    |    |          |    |    |           |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-52. STEPDELAY15 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.49 STEPCONFIG16 Register (offset = DCh) [reset = 0h]**

Register mask: FFFFFFFFh

STEPCONFIG16 is shown in [Figure](#page-82-1) 12-53 and described in Table [12-53](#page-82-2).

Step Configuration 16

**Figure 12-53. STEPCONFIG16 Register**

| 31                  | 30        | 29              | 28 | 27                | 26              | 25         | 24                  |  |  |
|---------------------|-----------|-----------------|----|-------------------|-----------------|------------|---------------------|--|--|
|                     |           | RESERVED        |    | Range_check       | FIFO_select     | Diff_CNTRL | SEL_RFM_SW<br>C_1_0 |  |  |
|                     |           | R/W-0h          |    | R/W-0h            | R/W-0h          | R/W-0h     | R/W-0h              |  |  |
| 23                  | 22        | 21              | 20 | 19                | 18              | 17         | 16                  |  |  |
| SEL_RFM_SW<br>C_1_0 |           | SEL_INP_SWC_3_0 |    |                   | SEL_INM_SWC_3_0 |            |                     |  |  |
| R/W-0h              |           | R/W-0h          |    |                   |                 | R/W-0h     |                     |  |  |
| 15                  | 14        | 13              | 12 | 11                | 10              | 9          | 8                   |  |  |
| SEL_INM_SW<br>C_3_0 |           | SEL_RFP_SWC_2_0 |    | WPNSW_SWC         | YPNSW_SWC       | XNPSW_SWC  | YNNSW_SWC           |  |  |
| R/W-0h              |           | R/W-0h          |    | R/W-0h            | R/W-0h          | R/W-0h     | R/W-0h              |  |  |
| 7                   | 6         | 5               | 4  | 3                 | 2               | 1          | 0                   |  |  |
| YPPSW_SWC           | XNNSW_SWC | XPPSW_SWC       |    | Averaging<br>Mode |                 |            |                     |  |  |
| R/W-0h              | R/W-0h    | R/W-0h          |    | R/W-0h            |                 |            | R/W-0h              |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-53. STEPCONFIG16 Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                        |
|-------|-----------------|------|-------|--------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED        | R/W  | 0h    |                                                                                                                    |
| 27    | Range_check     | R/W  | 0h    | 0 = Disable out-of-range check.<br>1 = Compare ADC data with range check register.                                 |
| 26    | FIFO_select     | R/W  | 0h    | Sampled data will be stored in FIFO.<br>0 = FIFO.<br>1 = FIFO1.                                                    |
| 25    | Diff_CNTRL      | R/W  | 0h    | Differential Control Pin                                                                                           |
| 24-23 | SEL_RFM_SWC_1_0 | R/W  | 0h    | SEL_RFM pins SW configuration.<br>00 = VSSA.<br>01 = XNUR.<br>10 = YNLR.<br>11 = VREFN.                            |
| 22-19 | SEL_INP_SWC_3_0 | R/W  | 0h    | SEL_INP pins SW configuration.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                          |
| 18-15 | SEL_INM_SWC_3_0 | R/W  | 0h    | SEL_INM pins for negative differential.<br>0000 = Channel 1.<br>0111 = Channel 8.<br>1xxx = VREFN.                 |
| 14-12 | SEL_RFP_SWC_2_0 | R/W  | 0h    | SEL_RFP pins SW configuration.<br>000 = VDDA_ADC.<br>001 = XPUL.<br>010 = YPLL.<br>011 = VREFP.<br>1xx = Reserved. |
| 11    | WPNSW_SWC       | R/W  | 0h    | WPNSW pin SW configuration                                                                                         |
| 10    | YPNSW_SWC       | R/W  | 0h    | YPNSW pin SW configuration                                                                                         |
| 9     | XNPSW_SWC       | R/W  | 0h    | XNPSW pin SW configuration                                                                                         |
| 8     | YNNSW_SWC       | R/W  | 0h    | YNNSW pin SW configuration                                                                                         |

**Table 12-53. STEPCONFIG16 Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                         |
|-----|-----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | YPPSW_SWC | R/W  | 0h    | YPPSW pin SW configuration                                                                                                                                          |
| 6   | XNNSW_SWC | R/W  | 0h    | XNNSW pin SW configuration                                                                                                                                          |
| 5   | XPPSW_SWC | R/W  | 0h    | XPPSW pin SW configuration                                                                                                                                          |
| 4-2 | Averaging | R/W  | 0h    | Number of samples to average:<br>000 = No average.<br>001 = 2 samples average.<br>010 = 4 samples average.<br>011 = 8 samples average.<br>100 = 16 samples average. |
| 1-0 | Mode      | R/W  | 0h    | 00 = SW enabled, one-shot.<br>01 = SW enabled, continuous.<br>10 = HW synchronized, one-shot.<br>11 = HW synchronized, continuous.                                  |

#### **12.5.1.50 STEPDELAY16 Register (offset = E0h) [reset = 0h]**

Register mask: FFFFFFFFh

STEPDELAY16 is shown in [Figure](#page-84-1) 12-54 and described in Table [12-54](#page-84-2).

Step Delay Register 16

**Figure 12-54. STEPDELAY16 Register**

| 31 | 30 | 29 | 28 | 27          | 26 | 25 | 24 | 23        | 22 | 21 | 20       | 19 | 18 | 17 | 16        |
|----|----|----|----|-------------|----|----|----|-----------|----|----|----------|----|----|----|-----------|
|    |    |    |    | SampleDelay |    |    |    |           |    |    | RESERVED |    |    |    | OpenDelay |
|    |    |    |    | R/W-0h      |    |    |    |           |    |    | R/W-0h   |    |    |    | R/W-0h    |
| 15 | 14 | 13 | 12 | 11          | 10 | 9  | 8  | 7         | 6  | 5  | 4        | 3  | 2  | 1  | 0         |
|    |    |    |    |             |    |    |    | OpenDelay |    |    |          |    |    |    |           |
|    |    |    |    |             |    |    |    | R/W-0h    |    |    |          |    |    |    |           |
|    |    |    |    |             |    |    |    |           |    |    |          |    |    |    |           |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-54. STEPDELAY16 Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                        |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | SampleDelay | R/W  | 0h    | This register will control the number of ADC clock cycles to sample<br>(hold SOC high).<br>Any value programmed here will be added to the minimum<br>requirement of 1 clock cycle. |
| 23-18 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                    |
| 17-0  | OpenDelay   | R/W  | 0h    | Program the number of ADC clock cycles to wait after applying the<br>step configuration registers and before sending the start of ADC<br>conversion                                |

#### **12.5.1.51 FIFO0COUNT Register (offset = E4h) [reset = 0h]**

Register mask: FFFFFFFFh

FIFO0COUNT is shown in [Figure](#page-85-1) 12-55 and described in Table [12-55.](#page-85-2)

FIFO0 word count@TSC\_ADC\_SS\_FIFO0 Word Count Register

**Figure 12-55. FIFO0COUNT Register**

| 31 | 30 | 29 | 28 | 27       | 26 | 25 | 24 | 23       | 22 | 21 | 20 | 19             | 18 | 17 | 16 |
|----|----|----|----|----------|----|----|----|----------|----|----|----|----------------|----|----|----|
|    |    |    |    |          |    |    |    | RESERVED |    |    |    |                |    |    |    |
|    |    |    |    |          |    |    |    | R-0h     |    |    |    |                |    |    |    |
| 15 | 14 | 13 | 12 | 11       | 10 | 9  | 8  | 7        | 6  | 5  | 4  | 3              | 2  | 1  | 0  |
|    |    |    |    | RESERVED |    |    |    |          |    |    |    | Words_in_FIFO0 |    |    |    |
|    |    |    |    | R-0h     |    |    |    |          |    |    |    | R-0h           |    |    |    |
|    |    |    |    |          |    |    |    |          |    |    |    |                |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-55. FIFO0COUNT Register Field Descriptions**

| Bit  | Field          | Type | Reset | Description                            |
|------|----------------|------|-------|----------------------------------------|
| 31-7 | RESERVED       | R    | 0h    | RESERVED                               |
| 6-0  | Words_in_FIFO0 | R    | 0h    | Number of words currently in the FIFO0 |

#### **12.5.1.52 FIFO0THRESHOLD Register (offset = E8h) [reset = 0h]**

Register mask: FFFFFFFFh

FIFO0THRESHOLD is shown in [Figure](#page-86-1) 12-56 and described in Table [12-56](#page-86-2).

FIFO0 Threshold trigger@TSC\_ADC\_SS\_FIFO0 Threshold Level Register

### **Figure 12-56. FIFO0THRESHOLD Register**

| 31 | 30       | 29 | 28 | 27       | 26                    | 25 | 24 |
|----|----------|----|----|----------|-----------------------|----|----|
|    |          |    |    | RESERVED |                       |    |    |
|    |          |    |    | R-0h     |                       |    |    |
| 23 | 22       | 21 | 20 | 19       | 18                    | 17 | 16 |
|    |          |    |    | RESERVED |                       |    |    |
|    |          |    |    | R-0h     |                       |    |    |
| 15 | 14       | 13 | 12 | 11       | 10                    | 9  | 8  |
|    |          |    |    | RESERVED |                       |    |    |
|    |          |    |    | R-0h     |                       |    |    |
| 7  | 6        | 5  | 4  | 3        | 2                     | 1  | 0  |
|    | RESERVED |    |    |          | FIFO0_threshold_Level |    |    |
|    | R-0h     |    |    |          | R/W-0h                |    |    |
|    |          |    |    |          |                       |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 12-56. FIFO0THRESHOLD Register Field Descriptions**

| Bit  | Field                 | Type | Reset | Description                                                                                                           |
|------|-----------------------|------|-------|-----------------------------------------------------------------------------------------------------------------------|
| 31-6 | RESERVED              | R    | 0h    |                                                                                                                       |
| 5-0  | FIFO0_threshold_Level | R/W  | 0h    | Program the desired FIFO0 data sample level to reach before<br>generating interrupt to CPU (program to value minus 1) |

#### **12.5.1.53 DMA0REQ Register (offset = ECh) [reset = 0h]**

Register mask: FFFFFFFFh

DMA0REQ is shown in [Figure](#page-87-1) 12-57 and described in Table [12-57.](#page-87-2)

FIFO0 DMA req0 trigger@TSC\_ADC\_SS\_FIFO0 DMA REQUEST Register

**Figure 12-57. DMA0REQ Register**

| 31 | 30 | 29 | 28 | 27 | 26       | 25 | 24 | 23       | 22 | 21 | 20 | 19 | 18                | 17 | 16 |
|----|----|----|----|----|----------|----|----|----------|----|----|----|----|-------------------|----|----|
|    |    |    |    |    |          |    |    | RESERVED |    |    |    |    |                   |    |    |
|    |    |    |    |    |          |    |    | R-0h     |    |    |    |    |                   |    |    |
| 15 | 14 | 13 | 12 | 11 | 10       | 9  | 8  | 7        | 6  | 5  | 4  | 3  | 2                 | 1  | 0  |
|    |    |    |    |    | RESERVED |    |    |          |    |    |    |    | DMA_Request_Level |    |    |
|    |    |    |    |    | R-0h     |    |    |          |    |    |    |    | R/W-0h            |    |    |
|    |    |    |    |    |          |    |    |          |    |    |    |    |                   |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-57. DMA0REQ Register Field Descriptions**

| Bit  | Field             | Type | Reset | Description                                                                            |
|------|-------------------|------|-------|----------------------------------------------------------------------------------------|
| 31-6 | RESERVED          | R    | 0h    | RESERVED                                                                               |
| 5-0  | DMA_Request_Level | R/W  | 0h    | Number of words in FIFO0 before generating a DMA request<br>(program to value minus 1) |

#### **12.5.1.54 FIFO1COUNT Register (offset = F0h) [reset = 0h]**

Register mask: FFFFFFFFh

FIFO1COUNT is shown in [Figure](#page-88-1) 12-58 and described in Table [12-58.](#page-88-2)

FIFO1 word count@TSC\_ADC\_SS\_FIFO1 Word Count Register

### **Figure 12-58. FIFO1COUNT Register**

| 31 | 30 | 29 | 28 | 27       | 26 | 25 | 24 | 23       | 22 | 21 | 20 | 19             | 18 | 17 | 16 |
|----|----|----|----|----------|----|----|----|----------|----|----|----|----------------|----|----|----|
|    |    |    |    |          |    |    |    | RESERVED |    |    |    |                |    |    |    |
|    |    |    |    |          |    |    |    | R-0h     |    |    |    |                |    |    |    |
| 15 | 14 | 13 | 12 | 11       | 10 | 9  | 8  | 7        | 6  | 5  | 4  | 3              | 2  | 1  | 0  |
|    |    |    |    | RESERVED |    |    |    |          |    |    |    | Words_in_FIFO0 |    |    |    |
|    |    |    |    | R-0h     |    |    |    |          |    |    |    | R-0h           |    |    |    |
|    |    |    |    |          |    |    |    |          |    |    |    |                |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-58. FIFO1COUNT Register Field Descriptions**

| Bit  | Field          | Type | Reset | Description                            |
|------|----------------|------|-------|----------------------------------------|
| 31-7 | RESERVED       | R    | 0h    | RESERVED                               |
| 6-0  | Words_in_FIFO0 | R    | 0h    | Number of words currently in the FIFO0 |

#### **12.5.1.55 FIFO1THRESHOLD Register (offset = F4h) [reset = 0h]**

Register mask: FFFFFFFFh

FIFO1THRESHOLD is shown in [Figure](#page-89-1) 12-59 and described in Table [12-59](#page-89-2).

FIFO1 Threshold trigger@TSC\_ADC\_SS\_FIFO1 Threshold Level Register

**Figure 12-59. FIFO1THRESHOLD Register**

| 31 | 30       | 29 | 28 | 27                    | 26 | 25 | 24 |
|----|----------|----|----|-----------------------|----|----|----|
|    |          |    |    | RESERVED              |    |    |    |
|    |          |    |    | R-0h                  |    |    |    |
| 23 | 22       | 21 | 20 | 19                    | 18 | 17 | 16 |
|    |          |    |    | RESERVED              |    |    |    |
|    |          |    |    | R-0h                  |    |    |    |
| 15 | 14       | 13 | 12 | 11                    | 10 | 9  | 8  |
|    |          |    |    | RESERVED              |    |    |    |
|    |          |    |    | R-0h                  |    |    |    |
| 7  | 6        | 5  | 4  | 3                     | 2  | 1  | 0  |
|    | RESERVED |    |    | FIFO0_threshold_Level |    |    |    |
|    | R-0h     |    |    | R/W-0h                |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-59. FIFO1THRESHOLD Register Field Descriptions**

| Bit  | Field                 | Type | Reset | Description                                                                                                           |
|------|-----------------------|------|-------|-----------------------------------------------------------------------------------------------------------------------|
| 31-6 | RESERVED              | R    | 0h    |                                                                                                                       |
| 5-0  | FIFO0_threshold_Level | R/W  | 0h    | Program the desired FIFO0 data sample level to reach before<br>generating interrupt to CPU (program to value minus 1) |

#### **12.5.1.56 DMA1REQ Register (offset = F8h) [reset = 0h]**

Register mask: FFFFFFFFh

DMA1REQ is shown in [Figure](#page-90-1) 12-60 and described in Table [12-60.](#page-90-2)

FIFO1 DMA req1 trigger@TSC\_ADC\_SS\_FIFO1 DMA Request Register

**Figure 12-60. DMA1REQ Register**

| 31 | 30 | 29 | 28 | 27 | 26       | 25 | 24 | 23       | 22 | 21 | 20 | 19 | 18                | 17 | 16 |
|----|----|----|----|----|----------|----|----|----------|----|----|----|----|-------------------|----|----|
|    |    |    |    |    |          |    |    | RESERVED |    |    |    |    |                   |    |    |
|    |    |    |    |    |          |    |    | R-0h     |    |    |    |    |                   |    |    |
| 15 | 14 | 13 | 12 | 11 | 10       | 9  | 8  | 7        | 6  | 5  | 4  | 3  | 2                 | 1  | 0  |
|    |    |    |    |    | RESERVED |    |    |          |    |    |    |    | DMA_Request_Level |    |    |
|    |    |    |    |    | R-0h     |    |    |          |    |    |    |    | R/W-0h            |    |    |
|    |    |    |    |    |          |    |    |          |    |    |    |    |                   |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-60. DMA1REQ Register Field Descriptions**

| Bit  | Field             | Type | Reset | Description                                                                            |
|------|-------------------|------|-------|----------------------------------------------------------------------------------------|
| 31-6 | RESERVED          | R    | 0h    | RESERVED                                                                               |
| 5-0  | DMA_Request_Level | R/W  | 0h    | Number of words in FIFO0 before generating a DMA request<br>(program to value minus 1) |

#### **12.5.1.57 FIFO0DATA Register (offset = 100h) [reset = 0h]**

Register mask: FFFFFFFFh

FIFO0DATA is shown in [Figure](#page-91-1) 12-61 and described in Table [12-61](#page-91-2).

ADC\_ FIFO0 \_READ Data @TSC\_ADC\_SS\_FIFO0 READ Register

#### **Figure 12-61. FIFO0DATA Register**

| 31 | 30 | 29       | 28 | 27 | 26       | 25 | 24 | 23 | 22 | 21      | 20 | 19 | 18 | 17        | 16 |
|----|----|----------|----|----|----------|----|----|----|----|---------|----|----|----|-----------|----|
|    |    |          |    |    | RESERVED |    |    |    |    |         |    |    |    | ADCCHNLID |    |
|    |    |          |    |    | R-0h     |    |    |    |    |         |    |    |    | R-0h      |    |
| 15 | 14 | 13       | 12 | 11 | 10       | 9  | 8  | 7  | 6  | 5       | 4  | 3  | 2  | 1         | 0  |
|    |    | RESERVED |    |    |          |    |    |    |    | ADCDATA |    |    |    |           |    |
|    |    | R-0h     |    |    |          |    |    |    |    | R-0h    |    |    |    |           |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 12-61. FIFO0DATA Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description                                                                                            |
|-------|-----------|------|-------|--------------------------------------------------------------------------------------------------------|
| 31-20 | RESERVED  | R    | 0h    | RESERVED.                                                                                              |
| 19-16 | ADCCHNLID | R    | 0h    | Optional ID tag of channel that captured the data.<br>If tag option is disabled, these bits will be 0. |
| 15-12 | RESERVED  | R    | 0h    |                                                                                                        |
| 11-0  | ADCDATA   | R    | 0h    | 12 bit sampled ADC converted data value stored in FIFO 0.                                              |

#### **12.5.1.58 FIFO1DATA Register (offset = 200h) [reset = 0h]**

Register mask: FFFFFFFFh

FIFO1DATA is shown in [Figure](#page-92-1) 12-62 and described in Table [12-62](#page-92-2).

ADC FIFO1\_READ Data@TSC\_ADC\_SS\_FIFO1 READ Register

### **Figure 12-62. FIFO1DATA Register**

| 31 | 30       | 29 | 28 | 27 | 26 | 25       | 24 | 23 | 22 | 21      | 20 | 19 | 18 | 17        | 16 |
|----|----------|----|----|----|----|----------|----|----|----|---------|----|----|----|-----------|----|
|    |          |    |    |    |    | RESERVED |    |    |    |         |    |    |    | ADCCHNLID |    |
|    |          |    |    |    |    | R-0h     |    |    |    |         |    |    |    | R-0h      |    |
| 15 | 14       | 13 | 12 | 11 | 10 | 9        | 8  | 7  | 6  | 5       | 4  | 3  | 2  | 1         | 0  |
|    | RESERVED |    |    |    |    |          |    |    |    | ADCDATA |    |    |    |           |    |
|    | R-0h     |    |    |    |    |          |    |    |    | R-0h    |    |    |    |           |    |
|    |          |    |    |    |    |          |    |    |    |         |    |    |    |           |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 12-62. FIFO1DATA Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description                                                                                            |
|-------|-----------|------|-------|--------------------------------------------------------------------------------------------------------|
| 31-20 | RESERVED  | R    | 0h    | RESERVED                                                                                               |
| 19-16 | ADCCHNLID | R    | 0h    | Optional ID tag of channel that captured the data.<br>If tag option is disabled, these bits will be 0. |
| 15-12 | RESERVED  | R    | 0h    | RESERVED                                                                                               |
| 11-0  | ADCDATA   | R    | 0h    | 12 bit sampled ADC converted data value stored in FIFO 1.                                              |
