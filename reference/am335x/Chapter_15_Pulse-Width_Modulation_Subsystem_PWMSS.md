# 15 Pulse-Width Modulation Subsystem (PWMSS)

This chapter describes the PWMSS of the device.

| Topic |              |                                                                        | Page         |
|-------|--------------|------------------------------------------------------------------------|--------------|
|       |              |                                                                        |              |
|       | 15.1<br>15.2 | Pulse-Width Modulation Subsystem (PWMSS)<br>Enhanced PWM (ePWM) Module | 2321<br>2330 |
|       | 15.3         | Enhanced Capture (eCAP) Module                                         | 2465         |
|       | 15.4         | Enhanced Quadrature Encoder Pulse (eQEP) Module                        | 2507         |

## **15.1 Pulse-Width Modulation Subsystem (PWMSS)**

### *15.1.1 Introduction*

#### **15.1.1.1 Features**

The general features of the PWMSS are:

#### **eHRPWM**

- Dedicated 16 bit time-base with Period / Frequency control
- Can support 2 independent PWM outputs with Single edge operation
- Can support 2 independent PWM outputs with Dual edge symmetric operation
- Can support 1 independent PWM output with Dual edge asymmetric operation
- Supports Dead-band generation with independent Rising and Falling edge delay control
- Provides asynchronous over-ride control of PWM signals during fault conditions
- Supports "trip zone" allocation of both latched and un-latched fault conditions
- Allows events to trigger both CPU interrupts and start of ADC conversions
- Support PWM chopping by high frequency carrier signal, used for pulse transformer gate drives.
- High-resolution module with programmable delay line.
  - Programmable on a per PWM period basis.
  - Can be inserted either on the rising edge or falling edge of the PWM pulse or both or not at all.

## **eCAP**

- Dedicated input Capture pin
- 32 bit Time Base (counter)
- 4 x 32 bit Time-stamp Capture registers (CAP1-CAP4)
- 4 stage sequencer (Mod4 counter) which is synchronized to external events (ECAPx pin edges)
- Independent Edge polarity (Rising / Falling edge) selection for all 4 events
- Input Capture signal pre-scaling (from 1 to 16)
- One-shot compare register (2 bits) to freeze captures after 1 to 4 Time-stamp events
- Control for continuous Time-stamp captures using a 4 deep circular buffer (CAP1-CAP4) scheme
- Interrupt capabilities on any of the 4 capture events

## **eQEP**

- Input Synchronization
- Quadrature Decoder Unit
- Position Counter and Control unit for position measurement
- Quadrature Edge Capture unit for low speed measurement
- Unit Time base for speed/frequency measurement
- Watchdog Timer for detecting stalls

#### **15.1.1.2 Unsupported Features**

The PWMSS module features not supported are shown in [Table](#page-2-0) 15-1.

#### **Table 15-1. Unsupported Features**

| Feature                  | Reason                           |
|--------------------------|----------------------------------|
| ePWM inputs              | Not pinned out                   |
| ePWM tripzone 1-5 inputs | Only Tripzone0 is pinned out     |
| ePWM digital comparators | Inputs not connected             |
| eQEP quadrature outputs  | Only input signals are connected |
| eCAP3–5                  | Module not used                  |
| eQEP3–5                  | Module not used                  |

### *15.1.2 Integration*

The Pulse Width Modulation Subsystem (PWMSS) includes a single instance of the Enhanced High Resolution Pulse Width Modulator (eHRPWM), Enhanced Capture (eCAP), and Enhanced Quadrature Encoded Pulse (eQEP) modules. This device includes three instantiations of the PWMSS.

```
Device

                         L4 Peripheral Interconnect
                                 <-------------->
                                 <-------------->
+---------------------------+    <-------------->      +-----------------------------------------------+
|                           |                          |                     PWMSS                     |
|  MPU Subsystem,           |<------------------------>|                                               |
|  PRU-ICSS                 |<------------------------>|   epwm_intr_intr_pend                         |
|                           |<------------------------>|   epwm_tz_intr_pend                           |
+---------------------------+<------------------------>|   ecap_intr_intr_pend                         |
                                                       |   eqep_intr_intr_pend                         |
+-------------------+                                  |                                               |
|   Control Module  |------------------------------->  |   epwm_ctl_tblcken                            |
+-------------------+                                  |                                               |
                                                       |   epwm_trip_tz[5:1]                           |
                                                       |   epwm_adc_soca                               |
                                                       |   epwm_adc_socb                               |
                                                       |   epwm_epwma_i                                |
                                                       |   epwm_epwmb_i                                |
                                                       |   epwm_comp_epwmdcmah                         |
                                                       |   epwm_comp_epwmdcmal                         |
                                                       |   epwm_comp_epwmdcmbh                         |
                                                       |   epwm_comp_epwmdcmbl                         |
                                                       |                                               |
                                                       |                              +---------------+|   eHRPWM Pads
                                                       |                              |    eHRPWM      ||--> [PWMA]
                                                       |                              +---------------+|--> [PWMB]
                                                       |                               ^  ^  ^         |<-- [TRIPZONE IN]
                                                       |                               |  |  |         |<-- [SYNC IN]
                                                       |                               |  |  |         |--> [SYNC IN]
                                                       |                                               |
                                                       |                              +---------------+|   eCAP Pads
                                                       |                              |     eCAP       ||<--> [IN PWMOUT]
                                                       |                              +---------------+|
                                                       |                                               |
                                                       |                              +---------------+|   eQEP Pads
                                                       |                              |     eQEP       ||--> [QEPA]
                                                       |                              +---------------+|--> [QEPB]
                                                       |                                               |<-- [INDEX]
                                                       |                                               |<-- [STROBE]
                                                       +-----------------------------------------------+
```
**Figure 15-1. PWMSS Integration**

#### **15.1.2.1 PWMSS Synchronization Detail**

The PWM (eHRPWM) and capture (eCAP) components of the PWMSS provide synchronization signals to allow them to be synchronized to other modules or events. On this device, these signals are connected in a daisy-chain fashion as shown in [Figure](#page-4-0) 15-2.

The eCAP capture events may be selected from among 31 different pins or internal interrupt signals. The event is selected using the corresponding ECAPx\_EVTCAPT field of the ECAP\_EVT\_CAP register in the Control Module.

```
eHRPWM_sync_i (Chip Input)  --->

PW MSS0 (EHRPWM_INPUT_SYNC_G = 1)                          PW MSS0 (ECAP_INPUT_SYNC_G = 0)
+---------------------------+                              +---------------------------+      +------------------+
|          eHRPWM0          |                              |           eCAP0           |<-----|   Chip Events    |
|                           |                              |                           |      | (via mux)        |
|  epwm_sync_syncin   <-----+------------------------------+-> ecap_sync_syncin        |      +------------------+
|  epwm_sync_syn_o  -------->   eHRPWM_sync_o (Chip Output)|  ecap_capin       <-------+------ eCAP0_EVT_SEL
|  epwm_sync_syncout  ------+------------------------------+  ecap_sync_syncout        |      (from Control Module)
+---------------------------+                              +---------------------------+

PW MSS1 (EHRPWM_INPUT_SYNC_G = 0)                          PW MSS1 (ECAP_INPUT_SYNC_G = 0)
+---------------------------+                              +---------------------------+      +------------------+
|          eHRPWM1          |                              |           eCAP1           |<-----|   Chip Events    |
|                           |                              |                           |      | (via mux)        |
|  epwm_sync_syncin   <-----+------------------------------+-> ecap_sync_syncin        |      +------------------+
|  epwm_sync_syn_o          |                              |  ecap_capin       <-------+------ eCAP1_EVT_SEL
|  epwm_sync_syncout  ------+------------------------------+  ecap_sync_syncout        |      (from Control Module)
+---------------------------+                              +---------------------------+

PW MSS2 (EHRPWM_INPUT_SYNC_G = 0)                          PW MSS2 (ECAP_INPUT_SYNC_G = 0)
+---------------------------+                              +---------------------------+      +------------------+
|          eHRPWM2          |                              |           eCAP2           |<-----|   Chip Events    |
|                           |                              |                           |      | (via mux)        |
|  epwm_sync_syncin   <-----+------------------------------+-> ecap_sync_syncin        |      +------------------+
|  epwm_sync_syn_o          |                              |  ecap_capin       <-------+------ eCAP2_EVT_SEL
|  epwm_sync_syncout  ------+------------------------------+  ecap_sync_syncout        |      (from Control Module)
+---------------------------+                              +---------------------------+
```

**Figure 15-2. PWMSS Synchronization**

#### **15.1.2.2 PWMSS Connectivity Attributes**

The general connectivity attributes for the PWMSS module are shown in [Table](#page-4-1) 15-2.

**Table 15-2. PWMSS Connectivity Attributes Attributes Type**

| Attributes          | Type                                                                                                                            |
|---------------------|---------------------------------------------------------------------------------------------------------------------------------|
| Power Domain        | Peripheral Domain                                                                                                               |
| Clock Domain        | PD_PER_L4LS_GCLK                                                                                                                |
| Reset Signals       | PER_DOM_RST_N                                                                                                                   |
| Idle/Wakeup Signals | Smart Idle                                                                                                                      |
| Interrupt Requests  | 2 ePWM interrupts per instance                                                                                                  |
|                     | epwm_intr_intr - Event interrupt, ePWMxINT for ARM<br>subsystem, epwm_intr_intr_pend for PRU-ICSS                               |
|                     | epwm_tz_intr - Tripzone interrupt, ePWMx_TZINT for ARM<br>subsystem, pwm_trip_zone for PRU-ICSS (only 1 for all 3<br>instances) |
|                     | 1 eCAP interrupt per instance                                                                                                   |
|                     | ecap_intr - Capture/PWM event interrupt, eCAPxINT for ARM<br>subsystem, ecap_intr_intr_pend for PRU-ICSS                        |
|                     | 1 eQEP Interrupt per instance                                                                                                   |
|                     | eqep_intr_intr - Event interrupt, eQEPxINT for ARM<br>subsystem, eqep_intr_intr_pend for PRU-ICSS (only for<br>eQEP0)           |

## **Table 15-2. PWMSS Connectivity Attributes (continued)**

| Attributes       | Type                                                 |
|------------------|------------------------------------------------------|
| DMA Requests     | Interrupt requests are redirected as DMA requests:   |
|                  | •<br>1 DMA request from ePWM per instance (ePWMEVTx) |
|                  | •<br>1 DMA request from eCAP per instance (eCAPEVTx) |
|                  | •<br>1 DMA request from eQEP per instance (eQEPEVTx) |
| Physical Address | L4 Peripheral slave port                             |

#### **15.1.2.3 PWMSS Clock and Reset Management**

The PWMSS controllers have separate bus interface and functional clocks.

### **Table 15-3. PWMSS Clock Signals**

| Clock Signal                                  | Max Freq | Reference / Source | Comments                      |
|-----------------------------------------------|----------|--------------------|-------------------------------|
| PWMSS_ocp_clk<br>Interface / Functional clock | 100 MHz  | CORE_CLKOUTM4 / 2  | pd_per_l4ls_gclk<br>from PRCM |

#### **15.1.2.4 PWMSS Pin list**

The external signals for the PWMSS module are shown in the following table.

#### **Table 15-4. PWMSS Pin List**

| Pin                | Type | Description                     |
|--------------------|------|---------------------------------|
| EPWMxA             | O    | PWM output A                    |
| EPWMxB             | O    | PWM output B                    |
| EPWM_SYNCIN        | I    | PWM Sync input                  |
| EPWM_SYNCOUT       | O    | PWM Sync output                 |
| EPWM_TRIPZONE[5:0] | I    | PWM Tripzone inputs             |
| ECAP_CAPIN_APWMOUT | I/O  | eCAP Capture input / PWM output |
| EQEP_A             | I/O  | eQEP Quadrature input/output    |
| EQEP_B             | I/O  | eQEP Quadrature input/output    |
| EQEP_INDEX         | I/O  | eQEP Index input/output         |
| EQEP_STROBE        | I/O  | eQEP Strobe input/output        |

### *15.1.3 PWMSS Registers*

[Table](#page-5-0) 15-5 lists the memory-mapped registers for the PWMSS. All register offset addresses not listed in [Table](#page-5-0) 15-5 should be considered as reserved locations and the register contents should not be modified.

**Table 15-5. PWMSS Registers**

| Offset | Acronym   | Register Name                 | Section          |
|--------|-----------|-------------------------------|------------------|
| 0h     | IDVER     | IP Revision Register          | Section 15.1.3.1 |
| 4h     | SYSCONFIG | System Configuration Register | Section 15.1.3.2 |
| 8h     | CLKCONFIG | Clock Configuration Register  | Section 15.1.3.3 |
| Ch     | CLKSTATUS | Clock Status Register         | Section 15.1.3.4 |

#### **15.1.3.1 IDVER Register (offset = 0h) [reset = 40000000h]**

IDVER is shown in [Figure](#page-6-1) 15-3 and described in [Table](#page-6-2) 15-6.

The IP revision register is used by software to track features, bugs, and compatibility.

#### **Figure 15-3. IDVER Register**

| 31     | 30 | 29       | 28   | 27 | 26      | 25      | 24 |
|--------|----|----------|------|----|---------|---------|----|
| SCHEME |    | RESERVED |      |    | FUNC    |         |    |
| R-1h   |    | R-0h     |      |    | R-0h    |         |    |
| 23     | 22 | 21       | 20   | 19 | 18      | 17      | 16 |
|        |    |          | FUNC |    |         |         |    |
|        |    |          | R-0h |    |         |         |    |
| 15     | 14 | 13       | 12   | 11 | 10      | 9       | 8  |
|        |    | R_RTL    |      |    |         | X_MAJOR |    |
|        |    | R-0h     |      |    |         | R-0h    |    |
| 7      | 6  | 5        | 4    | 3  | 2       | 1       | 0  |
| CUSTOM |    |          |      |    | Y_MINOR |         |    |
| R-0h   |    |          |      |    | R-0h    |         |    |
|        |    |          |      |    |         |         |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-6. IDVER Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                             |
|-------|----------|------|-------|---------------------------------------------------------|
| 31-30 | SCHEME   | R    | 1h    | Used to distinguish between the old scheme and current. |
| 29-28 | RESERVED | R    | 0h    |                                                         |
| 27-16 | FUNC     | R    | 0h    | FUNC                                                    |
| 15-11 | R_RTL    | R    | 0h    | RTL version (R), maintained by IP design owner.         |
| 10-8  | X_MAJOR  | R    | 0h    | Major revision (X)                                      |
| 7-6   | CUSTOM   | R    | 0h    | CUSTOM                                                  |
| 5-0   | Y_MINOR  | R    | 0h    | Minor revision (Y)                                      |

#### **15.1.3.2 SYSCONFIG Register (offset = 4h) [reset = 28h]**

SYSCONFIG is shown in [Figure](#page-7-1) 15-4 and described in [Table](#page-7-2) 15-7.

The system configuration register is used for clock management configuration.

**Figure 15-4. SYSCONFIG Register**

```
31                                                             24
+----------------------------------------------------------------+
|                           RESERVED                             |
|                            R-0h                                |
+----------------------------------------------------------------+

23                                                             16
+----------------------------------------------------------------+
|                           RESERVED                             |
|                            R-0h                                |
+----------------------------------------------------------------+

15                                                              8
+----------------------------------------------------------------+
|                           RESERVED                             |
|                            R-0h                                |
+----------------------------------------------------------------+

7        6        5        4        3        2        1        0
+--------+-----------------+-----------------+--------+----------+
|RESERVED|   STANDBYMODE   |     IDLEMODE    |FREEEMU | SOFTRESET|
| R-0h   |     R/W-2h      |     R/W-2h      | R/W-0h |  R/W-0h  |
+--------+-----------------+-----------------+--------+----------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-7. SYSCONFIG Register Field Descriptions**

| Bit  | Field       | Type | Reset | Description                                                                                                                                                                                                                                                           |
|------|-------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-6 | RESERVED    | R    | 0h    |                                                                                                                                                                                                                                                                       |
| 5-4  | STANDBYMODE | R/W  | 2h    | Configuration of the local initiator state management mode.<br>By definition, initiator may generate read/write transaction as long as<br>it is out of STANDBY state.                                                                                                 |
|      |             |      |       | 0h = Force-standby mode: local initiator is unconditionally placed in<br>standby state. Backup mode, for debug only.                                                                                                                                                  |
|      |             |      |       | 1h = No-standby mode: local initiator is unconditionally placed out of<br>standby state. Backup mode, for debug only.                                                                                                                                                 |
|      |             |      |       | 2h = Smart-standby mode: local initiator standby status depends on<br>local conditions, i.e., the module's functional requirement from the<br>initiator. IP module should not generate (initiator-related) wakeup<br>events.                                          |
|      |             |      |       | 3h = Reserved.                                                                                                                                                                                                                                                        |
| 3-2  | IDLEMODE    | R/W  | 2h    | Configuration of the local target state management mode.<br>By definition, target can handle read/write transaction as long as it is<br>out of IDLE state.                                                                                                            |
|      |             |      |       | 0h = Force-idle mode: local target's idle state follows (acknowledges)<br>the system's idle requests unconditionally, i.e. regardless of the IP<br>module's internal requirements. Backup mode, for debug only.                                                       |
|      |             |      |       | 1h = No-idle mode: local target never enters idle state. Backup<br>mode, for debug only.                                                                                                                                                                              |
|      |             |      |       | 2h = Smart-idle mode: local target's idle state eventually follows<br>(acknowledges) the system's idle requests, depending on the IP<br>module's internal requirements. IP module shall not generate (IRQ<br>or DMA-request-related) wakeup events.<br>3h = Reserved. |
| 1    | FREEEMU     | R/W  | 0h    | Sensitivity to emulation (debug) suspend event from Debug<br>Subsystem.                                                                                                                                                                                               |
|      |             |      |       | 0h = IP module is sensitive to emulation suspend.                                                                                                                                                                                                                     |
|      |             |      |       | 1h = IP module is not sensitive to emulation suspend event. Debug<br>suspend event is ignored.                                                                                                                                                                        |
| 0    | SOFTRESET   | R/W  | 0h    | Software reset (optional)                                                                                                                                                                                                                                             |

#### **15.1.3.3 CLKCONFIG Register (offset = 8h) [reset = 111h]**

CLKCONFIG is shown in [Figure](#page-8-1) 15-5 and described in [Table](#page-8-2) 15-8.

The clock configuration register is used in the PWMSS submodule for clkstop req and clk\_en control.

**Figure 15-5. CLKCONFIG Register**

| 31       | 30 | 29                  | 28         | 27       | 26   | 25                  | 24         |
|----------|----|---------------------|------------|----------|------|---------------------|------------|
|          |    |                     | RESERVED   |          |      |                     |            |
|          |    |                     | R-0h       |          |      |                     |            |
| 23       | 22 | 21                  | 20         | 19       | 18   | 17                  | 16         |
|          |    |                     | RESERVED   |          |      |                     |            |
|          |    |                     | R-0h       |          |      |                     |            |
| 15       | 14 | 13                  | 12         | 11       | 10   | 9                   | 8          |
|          |    | RESERVED            |            |          |      | ePWMCLKSTO<br>P_REQ | ePWMCLK_EN |
|          |    | R-0h                |            |          |      | R/W-0h              | R/W-1h     |
| 7        | 6  | 5                   | 4          | 3        | 2    | 1                   | 0          |
| RESERVED |    | eQEPCLKSTO<br>P_REQ | eQEPCLK_EN | RESERVED |      | eCAPCLKSTO<br>P_REQ | eCAPCLK_EN |
| R-0h     |    | R/W-0h              | R/W-1h     |          | R-0h | R/W-0h              | R/W-1h     |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-8. CLKCONFIG Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                 |
|-------|-----------------|------|-------|-------------------------------------------------------------|
| 31-10 | RESERVED        | R    | 0h    |                                                             |
| 9     | ePWMCLKSTOP_REQ | R/W  | 0h    | This bit controls the clkstop_req input to the ePWM module. |
| 8     | ePWMCLK_EN      | R/W  | 1h    | This bit controls the clk_en input to the ePWM module.      |
| 7-6   | RESERVED        | R    | 0h    |                                                             |
| 5     | eQEPCLKSTOP_REQ | R/W  | 0h    | This bit controls the clkstop_req input to the eQEP module  |
| 4     | eQEPCLK_EN      | R/W  | 1h    | This bit controls the clk_en input to the eQEP module.      |
| 3-2   | RESERVED        | R    | 0h    |                                                             |
| 1     | eCAPCLKSTOP_REQ | R/W  | 0h    | This bit controls the clkstop_req input to the eCAP module. |
| 0     | eCAPCLK_EN      | R/W  | 1h    | This bit controls the clk_en input to the eCAP module.      |

#### **15.1.3.4 CLKSTATUS Register (offset = Ch) [reset = 0h]**

CLKSTATUS is shown in [Figure](#page-9-1) 15-6 and described in [Table](#page-9-2) 15-9.

The clock status register is used in the PWMSS submodule for clkstop ack and clk\_en ack status.

**Figure 15-6. CLKSTATUS Register**

```
31                                                             24
+----------------------------------------------------------------+
|                           RESERVED                             |
|                            R-0h                                |
+----------------------------------------------------------------+

23                                                             16
+----------------------------------------------------------------+
|                           RESERVED                             |
|                            R-0h                                |
+----------------------------------------------------------------+

15                             10    9                 8
+----------------------------------+-------------------+---------+
|             RESERVED             | ePWM_CLKSTOP_ACK  | ePWM_CLK|
|               R-0h               |      R-0h         | EN_ACK  |
+----------------------------------+-------------------+---------+
                                                   (bit8)  R-0h

7                 6        5                 4                3      2      1                 0
+------------------+--------+------------------+----------------+--------+------+-----------------+----------------+
|     RESERVED     |        | eQEP_CLKSTOP_ACK | eQEP_CLK_EN_ACK|RESERVED|      | eCAP_CLKSTOP_ACK| eCAP_CLK_EN_ACK|
|       R-0h       |        |       R-0h       |      R-0h      |  R-0h  |      |       R-0h      |      R-0h      |
+------------------+--------+------------------+----------------+------+------+------------------+-----------------+
                 (bits7:6)             (bit5)            (bit4)   (bits3:2)        (bit1)            (bit0)
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 15-9. CLKSTATUS Register Field Descriptions**

| Bit   | Field            | Type | Reset | Description                                                       |
|-------|------------------|------|-------|-------------------------------------------------------------------|
| 31-10 | RESERVED         | R    | 0h    |                                                                   |
| 9     | ePWM_CLKSTOP_ACK | R    | 0h    | This bit is the clkstop_req_ack status output of the ePWM module. |
| 8     | ePWM_CLK_EN_ACK  | R    | 0h    | This bit is the clk_en status output of the ePWM module.          |
| 7-6   | RESERVED         | R    | 0h    |                                                                   |
| 5     | eQEP_CLKSTOP_ACK | R    | 0h    | This bit is the clkstop_req_ack status output of the eQEP module. |
| 4     | eQEP_CLK_EN_ACK  | R    | 0h    | This bit is the clk_en status output of the eQEP module.          |
| 3-2   | RESERVED         | R    | 0h    |                                                                   |
| 1     | eCAP_CLKSTOP_ACK | R    | 0h    | This bit is the clkstop_req_ack status output of the eCAP module. |
| 0     | eCAP_CLK_EN_ACK  | R    | 0h    | This bit is the clk_en status output of the eCAP module.          |

## **15.2 Enhanced PWM (ePWM) Module**

### *15.2.1 Introduction*

An effective PWM peripheral must be able to generate complex pulse width waveforms with minimal CPU overhead or intervention. It needs to be highly programmable and very flexible while being easy to understand and use. The ePWM unit described here addresses these requirements by allocating all needed timing and control resources on a per PWM channel basis. Cross coupling or sharing of resources has been avoided; instead, the ePWM is built up from smaller single channel modules with separate resources and that can operate together as required to form a system. This modular approach results in an orthogonal architecture and provides a more transparent view of the peripheral structure, helping users to understand its operation quickly.

In this chapter, the letter x within a signal or module name is used to indicate a generic ePWM instance on a device. For example, output signals EPWMxA and EPWMxB refer to the output signals from the ePWMx instance. Thus, EPWM1A and EPWM1B belong to ePWM1 and, likewise, EPWM4A and EPWM4B belong to ePWM4.

#### **15.2.1.1 Submodule Overview**

The ePWM module represents one complete PWM channel composed of two PWM outputs: EPWMxA and EPWMxB. Multiple ePWM modules are instanced within a device as shown in [Figure](#page-11-0) 15-7. Each ePWM instance is identical with one exception. Some instances include a hardware extension that allows more precise control of the PWM outputs. This extension is the high-resolution pulse width modulator (HRPWM) and is described in Section [15.2.2.10.](#page-66-0) See [Section](#page-3-0) 15.1.2 to determine which ePWM instances include this feature. Each ePWM module is indicated by a numerical value starting with 0. For example ePWM0 is the first instance and ePWM2 is the third instance in the system and ePWMx indicates any instance.

The ePWM modules are chained together via a clock synchronization scheme that allows them to operate as a single system when required. Additionally, this synchronization scheme can be extended to the capture peripheral modules (eCAP). The number of modules is device-dependent and based on target application needs. Modules can also operate stand-alone.

Each ePWM module supports the following features:

- Dedicated 16-bit time-base counter with period and frequency control
- Two PWM outputs (EPWMxA and EPWMxB) that can be used in the following configurations::
  - Two independent PWM outputs with single-edge operation
  - Two independent PWM outputs with dual-edge symmetric operation
  - One independent PWM output with dual-edge asymmetric operation
- Asynchronous override control of PWM signals through software.
- Programmable phase-control support for lag or lead operation relative to other ePWM modules.
- Hardware-locked (synchronized) phase relationship on a cycle-by-cycle basis.
- Dead-band generation with independent rising and falling edge delay control.
- Programmable trip zone allocation of both cycle-by-cycle trip and one-shot trip on fault conditions.
- A trip condition can force either high, low, or high-impedance state logic levels at PWM outputs.
- Programmable event prescaling minimizes CPU overhead on interrupts.
- PWM chopping by high-frequency carrier signal, useful for pulse transformer gate drives.

Each ePWM module is connected to the input/output signals shown in [Figure](#page-11-0) 15-7. The signals are described in detail in subsequent sections.

The order in which the ePWM modules are connected may differ from what is shown in [Figure](#page-11-0) 15-7. See Section [15.2.2.3.3.2](#page-22-0) for the synchronization scheme for a particular device. Each ePWM module consists of seven submodules and is connected within a system via the signals shown in [Figure](#page-12-0) 15-8.

```
                         xSYNCI
      +--------------------------------------------------------------+
      |                                                              |
      v                                                              |
+----------------------+          EPWM0A --------------------------->|
|  Interrupt           |<--------- EPWM0INT                          |
|  Controller          |          EPWM0B --------------------------->|        +-----------+
|                      |                                             +------->|  GPIO     |
|                      |                    TZ1 to TZn <-------------+        |  MUX      |
|                      |                               xSYNCO ------>+------->|           |
|                      |                                              \       +-----------+
|                      |                                               \
|                      |   To eCAP1  <----------------------------------o
|                      |
|                      |<--------- EPWM1INT
|                      |
|                      |<--------- EPWMxINT
+----------------------+

                 +---------------------------+
                 |           SYNCI           |
                 |        ePWM0 module       |
                 |           SYNCO           |
                 +---------------------------+
                           |
                           v
                 +---------------------------+
                 |           SYNCI           |
                 |        ePWM1 module       |
                 |           SYNCO           |
                 +---------------------------+
                           |
                           v
                 +---------------------------+
                 |           SYNCI           |
                 |        ePWMx module       |
                 |           SYNCO           |
                 +---------------------------+
                           |
                           v
                     Peripheral Frame 1

From each ePWM module to the GPIO MUX:
- ePWM0: EPWM0A, EPWM0B, (TZ1..TZn), (xSYNCO)
- ePWM1: EPWM1A, EPWM1B, (TZ1..TZn)
- ePWMx: EPWMxA, EPWMxB, (TZ1..TZn)
```

**Figure 15-7. Multiple ePWM Modules**

NOTE: [Figure](#page-11-0) 15-7 is a generic block diagram. For specific implementation, see Section [15.1.2.1](#page-3-1), *PWMSS Synchronization Detail*.

> *[Diagram — signal/block labels not renderable in text format]*

**Figure 15-8. Submodules and Signal Connections for an ePWM Module**

[Figure](#page-13-0) 15-9 shows more internal details of a single ePWM module. The main signals used by the ePWM module are:

- **PWM output signals (EPWMxA and EPWMxB).** The PWM output signals are made available external to the device through the GPIO peripheral described in the system control and interrupts guide for your device.
- **Trip-zone signals (TZ1 to TZn).** These input signals alert the ePWM module of an external fault condition. Each module on a device can be configured to either use or ignore any of the trip-zone signals. The trip-zone signal can be configured as an asynchronous input through the GPIO peripheral. See [Section](#page-3-0) 15.1.2 to determine how many trip-zone pins are available in the device.
- **Time-base synchronization input (EPWMxSYNCI) and output (EPWMxSYNCO) signals.** The synchronization signals daisy chain the ePWM modules together. Each module can be configured to either use or ignore its synchronization input. The clock synchronization input and output signal are brought out to pins only for ePWM0 (ePWM module #0). The synchronization output for ePWM2 (EPWM2SYNCO) is also connected to the SYNCI of the first enhanced capture module (eCAP0).
- **Peripheral Bus.** The peripheral bus is 32-bits wide and allows both 16-bit and 32-bit writes to the ePWM register file.

[Figure](#page-13-0) 15-9 also shows the key internal submodule interconnect signals. Each submodule is described in [Section](#page-14-0) 15.2.2.

> *[Diagram — signal/block labels not renderable in text format]*

**Figure 15-9. ePWM Submodules and Critical Internal Signal Interconnects**

### *15.2.2 Functional Description*

Seven submodules are included in every ePWM peripheral. There are some instances that include a highresolution submodule that allows more precise control of the PWM outputs. Each of these submodules performs specific tasks that can be configured by software.

#### **15.2.2.1 Overview**

Table [15-10](#page-14-1) lists the eight key submodules together with a list of their main configuration parameters. For example, if you need to adjust or control the duty cycle of a PWM waveform, then you should see the counter-compare submodule in Section [15.2.2.4](#page-27-0) for relevant details.

**Table 15-10. Submodule Configuration Parameters**

| Submodule             | Configuration Parameter or Option                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|-----------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Time-base (TB)        | •<br>Scale the time-base clock (TBCLK) relative to the system clock (SYSCLKOUT).<br>•<br>Configure the PWM time-base counter (TBCNT) frequency or period.<br>•<br>Set the mode for the time-base counter:<br>–<br>count-up mode: used for asymmetric PWM<br>–<br>count-down mode: used for asymmetric PWM<br>–<br>count-up-and-down mode: used for symmetric PWM<br>•<br>Configure the time-base phase relative to another ePWM module.<br>•<br>Synchronize the time-base counter between modules through hardware or software.<br>•<br>Configure the direction (up or down) of the time-base counter after a synchronization event.<br>•<br>Configure how the time-base counter will behave when the device is halted by an emulator.<br>•<br>Specify the source for the synchronization output of the ePWM module:<br>–<br>Synchronization input signal<br>–<br>Time-base counter equal to zero |
|                       | –<br>Time-base counter equal to counter-compare B (CMPB)<br>–<br>No output synchronization signal generated.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| Counter-compare (CC)  | •<br>Specify the PWM duty cycle for output EPWMxA and/or output EPWMxB<br>•<br>Specify the time at which switching events occur on the EPWMxA or EPWMxB output                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| Action-qualifier (AQ) | •<br>Specify the type of action taken when a time-base or counter-compare submodule event occurs:<br>–<br>No action taken<br>–<br>Output EPWMxA and/or EPWMxB switched high<br>–<br>Output EPWMxA and/or EPWMxB switched low<br>–<br>Output EPWMxA and/or EPWMxB toggled<br>•<br>Force the PWM output state through software control<br>•<br>Configure and control the PWM dead-band through software                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| Dead-band (DB)        | •<br>Control of traditional complementary dead-band relationship between upper and lower switches<br>•<br>Specify the output rising-edge-delay value<br>•<br>Specify the output falling-edge delay value<br>•<br>Bypass the dead-band module entirely. In this case the PWM waveform is passed through<br>without modification.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| PWM-chopper (PC)      | •<br>Create a chopping (carrier) frequency.<br>•<br>Pulse width of the first pulse in the chopped pulse train.<br>•<br>Duty cycle of the second and subsequent pulses.<br>•<br>Bypass the PWM-chopper module entirely. In this case the PWM waveform is passed through<br>without modification.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |

**Table 15-10. Submodule Configuration Parameters (continued)**

| Submodule                      | Configuration Parameter or Option                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|--------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Trip-zone (TZ)                 | •<br>Configure the ePWM module to react to one, all, or none of the trip-zone pins.<br>•<br>Specify the tripping action taken when a fault occurs:<br>–<br>Force EPWMxA and/or EPWMxB high<br>–<br>Force EPWMxA and/or EPWMxB low<br>–<br>Force EPWMxA and/or EPWMxB to a high-impedance state<br>–<br>Configure EPWMxA and/or EPWMxB to ignore any trip condition.<br>•<br>Configure how often the ePWM will react to each trip-zone pin:<br>–<br>One-shot<br>–<br>Cycle-by-cycle |
|                                | •<br>Enable the trip-zone to initiate an interrupt.<br>•<br>Bypass the trip-zone module entirely.                                                                                                                                                                                                                                                                                                                                                                                  |
| Event-trigger (ET)             | •<br>Enable the ePWM events that will trigger an interrupt.<br>•<br>Specify the rate at which events cause triggers (every occurrence or every second or third<br>occurrence)<br>•<br>Poll, set, or clear event flags                                                                                                                                                                                                                                                              |
| High-Resolution PWM<br>(HRPWM) | •<br>Enable extended time resolution capabilities<br>•<br>Configure finer time granularity control or edge positioning                                                                                                                                                                                                                                                                                                                                                             |

Code examples are provided in the remainder of this chapter that show how to implement various ePWM module configurations. These examples use the constant definitions shown in [Example](#page-15-0) 15-1.

#### *Example 15-1. Constant Definitions Used in the Code Examples*

```
// TBCTL (Time-Base Control)
// = = = = = = = = = = = = = = = = = = = = = = = = = =
// TBCNT MODE bits
#define TB_COUNT_UP 0x0
#define TB_COUNT_DOWN 0x1
#define TB_COUNT_UPDOWN 0x2
#define TB_FREEZE 0x3
// PHSEN bit
#define TB_DISABLE 0x0
#define TB_ENABLE 0x1
// PRDLD bit
#define TB_SHADOW 0x0
#define TB_IMMEDIATE 0x1
// SYNCOSEL bits
#define TB_SYNC_IN 0x0
#define TB_CTR_ZERO 0x1
#define TB_CTR_CMPB 0x2
#define TB_SYNC_DISABLE 0x3
// HSPCLKDIV and CLKDIV bits
#define TB_DIV1 0x0
#define TB_DIV2 0x1
#define TB_DIV4 0x2
// PHSDIR bit
#define TB_DOWN 0x0
#define TB_UP 0x1
// CMPCTL (Compare Control)
// = = = = = = = = = = = = = = = = = = = = = = = = = =
// LOADAMODE and LOADBMODE bits
#define CC_CTR_ZERO 0x0
#define CC_CTR_PRD 0x1
#define CC_CTR_ZERO_PRD 0x2
#define CC_LD_DISABLE 0x3
// SHDWAMODE and SHDWBMODE bits
```

## *Example 15-1. Constant Definitions Used in the Code Examples (continued)*

| #define                      | CC_SHADOW                                                                                  | 0x0        |  |
|------------------------------|--------------------------------------------------------------------------------------------|------------|--|
| #define                      | CC_IMMEDIATE                                                                               | 0x1        |  |
|                              | // AQCTLA and AQCTLB (Action-qualifier Control)                                            |            |  |
|                              | // = = = = = = = = = = = = = = = = = = = = = = = = = =                                     |            |  |
|                              | // ZRO, PRD, CAU, CAD, CBU, CBD bits                                                       |            |  |
| #define                      | AQ_NO_ACTION                                                                               | 0x0        |  |
| #define                      | AQ_CLEAR                                                                                   | 0x1        |  |
| #define                      | AQ_SET                                                                                     | 0x2        |  |
| #define                      | AQ_TOGGLE                                                                                  | 0x3        |  |
| // DBCTL (Dead-Band Control) |                                                                                            |            |  |
|                              | // = = = = = = = = = = = = = = = = = = = = = = = = = =                                     |            |  |
| // MODE bits                 |                                                                                            |            |  |
| #define                      | DB_DISABLE                                                                                 | 0x0        |  |
| #define                      | DBA_ENABLE                                                                                 | 0x1        |  |
| #define                      | DBB_ENABLE                                                                                 | 0x2        |  |
| #define DB_FULL_ENABLE 0x3   |                                                                                            |            |  |
| // POLSEL bits               |                                                                                            |            |  |
| #define<br>#define           | DB_ACTV_HI<br>DB_ACTV_LOC                                                                  | 0x0<br>0x1 |  |
| #define                      | DB_ACTV_HIC                                                                                | 0x2        |  |
| #define                      | DB_ACTV_LO                                                                                 | 0x3        |  |
| // PCCTL (chopper control)   |                                                                                            |            |  |
|                              | // = = = = = = = = = = = = = = = = = = = = = = = = = =                                     |            |  |
| // CHPEN bit                 |                                                                                            |            |  |
| #define                      | CHP_ENABLE                                                                                 | 0x0        |  |
| #define CHP_DISABLE 0x1      |                                                                                            |            |  |
| // CHPFREQ bits              |                                                                                            |            |  |
| #define                      | CHP_DIV1                                                                                   | 0x0        |  |
| #define                      | CHP_DIV2                                                                                   | 0x1        |  |
| #define                      | CHP_DIV3                                                                                   | 0x2        |  |
| #define                      | CHP_DIV4                                                                                   | 0x3        |  |
| #define                      | CHP_DIV5                                                                                   | 0x4        |  |
| #define                      | CHP_DIV6                                                                                   | 0x5        |  |
| #define                      | CHP_DIV7                                                                                   | 0x6        |  |
| #define                      | CHP_DIV8                                                                                   | 0x7        |  |
| // CHPDUTY bits              |                                                                                            |            |  |
| #define                      | CHP1_8TH                                                                                   | 0x0        |  |
| #define                      | CHP2_8TH                                                                                   | 0x1        |  |
| #define                      | CHP3_8TH                                                                                   | 0x2        |  |
| #define                      | CHP4_8TH                                                                                   | 0x3        |  |
| #define                      | CHP5_8TH                                                                                   | 0x4        |  |
| #define                      | CHP6_8TH                                                                                   | 0x5        |  |
| #define                      | CHP7_8TH                                                                                   | 0x6        |  |
| // TZSEL (Trip-zone Select)  |                                                                                            |            |  |
|                              | // = = = = = = = = = = = = = = = = = = = = = = = = = =                                     |            |  |
| // CBCn and OSHTn bits       |                                                                                            |            |  |
| #define                      | TZ_ENABLE                                                                                  | 0x0        |  |
| #define                      | TZ_DISABLE                                                                                 | 0x1        |  |
| // TZCTL (Trip-zone Control) |                                                                                            |            |  |
|                              | // = = = = = = = = = = = = = = = = = = = = = = = = = =                                     |            |  |
| // TZA and TZB bits          |                                                                                            |            |  |
| #define                      | TZ_HIZ                                                                                     | 0x0        |  |
| #define                      | TZ_FORCE_HI                                                                                | 0x1        |  |
| #define                      | TZ_FORCE_LO                                                                                | 0x2        |  |
| #define                      | TZ_DISABLE                                                                                 | 0x3        |  |
|                              | // ETSEL (Event-trigger Select)                                                            |            |  |
|                              | // = = = = = = = = = = = = = = = = = = = = = = = = = =<br>// INTSEL, SOCASEL, SOCBSEL bits |            |  |
| #define                      | ET_CTR_ZERO                                                                                | 0x1        |  |
| #define                      | ET_CTR_PRD                                                                                 | 0x2        |  |
| #define                      | ET_CTRU_CMPA                                                                               | 0x4        |  |
| #define                      | ET_CTRD_CMPA                                                                               | 0x5        |  |
| #define                      | ET_CTRU_CMPB                                                                               | 0x6        |  |
| #define                      | ET_CTRD_CMPB                                                                               | 0x7        |  |
|                              |                                                                                            |            |  |

#### Example 15-1. Constant Definitions Used in the Code Examples (continued)

```
// ETPS (Event-trigger Prescale)
// = = = = = = = = = = = = = = = = = =
```

#### 15.2.2.2 Proper Interrupt Initialization Procedure

When the ePWM peripheral clock is enabled it may be possible that interrupt flags may be set due to spurious events due to the ePWM registers not being properly initialized. The proper procedure for initializing the ePWM peripheral is:

- 1. Disable global interrupts (CPU INTM flag)
- 2. Disable ePWM interrupts
- 3. Initialize peripheral registers
- 4. Clear any spurious ePWM flags
- 5. Enable ePWM interrupts
- 6. Enable global interrupts

#### 15.2.2.3 Time-Base (TB) Submodule

Each ePWM module has its own time-base submodule that determines all of the event timing for the ePWM module. Built-in synchronization logic allows the time-base of multiple ePWM modules to work together as a single system. Figure 15-10 illustrates the time-base module's place within the ePWM.

```
EPWMxSYNCI  ----> +------------------+ <---- EPWMxSYNCO
                  |   Time-Base      |
                  |      (TB)        |
                  +------------------+
                      |        \
                      |         \  (event qualifiers to AQ)
                      |          +--> CTR = PRD
                      |          +--> CTR = 0
                      |          +--> CTR_Dir
                      v
                  +------------------+
                  | Counter Compare  |
                  |      (CC)        |
                  +------------------+
                      |
                      +--> CTR = CMPA
                      +--> CTR = CMPB

                                +---------------------------+
 CTR = PRD -------------------> |                           |
 CTR = 0   -------------------> |   Action Qualifier (AQ)   | ----> EPWMxA ----> +-----+ -> +-----------+ -> +-----------+ -> EPWMxA -> +---------+
 CTR_Dir  ------------------->  |                           | ----> EPWMxB ----> | DB  | -> | PWM-      | -> | Trip      | -> EPWMxB -> | GPIO    |
 CTR=CMPA (from CC) --------->  |                           |                    |     |    | chopper   |    | Zone (TZ) |            | MUX     |
 CTR=CMPB (from CC) --------->  +---------------------------+                    +-----+    |   (PC)    |    +-----------+            +---------+
                                                                                            +-----------+
                                                                                                     ^
                                                                                                     |
                                                                                               TZ1 to TZn

                     (events to interrupt logic)
 CTR = PRD ------------------------------+
 CTR = 0   ------------------------------+
 CTR = CMPA -----------------------------+----> +------------------------------+ ----> EPWMxINT ----> +---------------------+
 CTR = CMPB -----------------------------+      | Event Trigger and Interrupt  |                      | Interrupt controller|
 CTR_Dir  -------------------------------+      |            (ET)              |                      +---------------------+
                                                +------------------------------+

 CTR = 0 -------------------------------> (into TZ)
 EPWMxTZINT ---------------------------> +---------------------+
                                         | Interrupt controller|
                                         +---------------------+
```

Figure 15-10. Time-Base Submodule Block Diagram

#### *15.2.2.3.1 Purpose of the Time-Base Submodule*

You can configure the time-base submodule for the following:

- Specify the ePWM time-base counter (TBCNT) frequency or period to control how often events occur.
- Manage time-base synchronization with other ePWM modules.
- Maintain a phase relationship with other ePWM modules.
- Set the time-base counter to count-up, count-down, or count-up-and-down mode.
- Generate the following events:
  - CTR = PRD: Time-base counter equal to the specified period (TBCNT = TBPRD) .
  - CTR = 0: Time-base counter equal to zero (TBCNT = 0000h).
- Configure the rate of the time-base clock; a prescaled version of the CPU system clock (SYSCLKOUT). This allows the time-base counter to increment/decrement at a slower rate.

#### 15.2.2.3.2 Controlling and Monitoring the Time-Base Submodule

Table 15-11 lists the registers used to control and monitor the time-base submodule.

Table 15-11. Time-Base Submodule Registers

| Acronym | Register Description               | Address Offset | Shadowed |
|---------|------------------------------------|----------------|----------|
| TBCTL   | Time-Base Control Register         | 0h             | No       |
| TBSTS   | Time-Base Status Register          | 2h             | No       |
| TBPHSHR | HRPWM extension Phase Register (1) | 4h             | No       |
| TBPHS   | Time-Base Phase Register           | 6h             | No       |
| TBCNT   | Time-Base Counter Register         | 8h             | No       |
| TBPRD   | Time-Base Period Register          | Ah             | Yes      |

<sup>(1)</sup> This register is available only on ePWM instances that include the high-resolution extension (HRPWM). On ePWM modules that do not include the HRPWM, this location is reserved. See Section 15.1.2 to determine which ePWM instances include this feature.

Figure 15-11 shows the critical signals and registers of the time-base submodule. Table 15-12 provides descriptions of the key signals associated with the time-base submodule.

Figure 15-11. Time-Base Submodule Signals and Registers

```
                           +--------------------------------------+
                           | TBPRD Period Shadow                  |
                           | TBPRD Period Active                  |
                           +--------------------------------------+
                                   ^                 |
                                   | TBCTL[PRDLD]    | (16)
                                   |                 v
TBCNT (16) <-----------------------+-------------+  +--------+
                                                 |  |  CMP   |----> (pulse) CTR = PRD
                                                 +->| (==)   |
TBPRD Active (16) --------------------------------> |        |
                                                    +--------+
                                                        |
                                                        v
+-------------------------------------------------------------------+
|                     TBCNT Counter Active Reg                      |
|                                                                   |
| inputs:                                                           |
|  - CTR = 0                                                        |
|  - CTR_dir                                                        |
|  - CTR_max                                                        |
|  - TBCLK (clk)                                                    |
|                                                                   |
| functions/flags:                                                  |
|  - Zero / Dir / Max                                               |
|  - Counter UP/DOWN                                                |
|  - Reset / Mode / Load  <---- TBCTL[CTRMODE]                      |
+-------------------------------------------------------------------+
            ^ (16)
            |
+---------------------------+
| TBPHS Phase Active Reg    |
+---------------------------+

SYSCLKOUT ---> +------------------+ ---> TBCLK
               | Clock Prescale   |
               +------------------+
                 ^ TBCTL[HSPCLKDIV]
                 ^ TBCTL[CLKDIV]

SYNC path:
EPWMxSYNCI ------------------------------+
                                         |
TBCTL[SWFSYNC] ----> (OR) --------------+----> sync pulse node ----+
                                                                     \
                                                                      \ (when enabled)
                                                                       +---[ TBCTL[PHSEN] ]---+
                                                                                               |
                                                                                               v
                                                                                      +------------------+
                                                                                      | Sync Out Select  |----> EPWMxSYNCO
                                                                                      +------------------+
                                                                                       ^ TBCTL[SYNCOSEL]
                                                                                       |
                                                                                       +-- select source:
                                                                                           - CTR = 0
                                                                                           - CTR = CMPB
                                                                                           - Disable (X)
```

## **Table 15-12. Key Time-Base Signals**

| Signal     | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
|------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| EPWMxSYNCI | Time-base synchronization input.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|            | Input pulse used to synchronize the time-base counter with the counter of ePWM module earlier in the<br>synchronization chain. An ePWM peripheral can be configured to use or ignore this signal. For example, this<br>signal could come from a device pin for the first ePWM module (ePWM0). For subsequent ePWM modules this<br>signal could be passed from another ePWM peripheral, such that EPWM1SYNCI is generated by the ePWM0<br>peripheral, EPWM2SYNCI is generated by ePWM1, and so forth. See Section 15.1.2 for information on the<br>synchronization order of a particular device. |
| EPWMxSYNCO | Time-base synchronization output.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
|            | This output pulse is used to synchronize the counter of an ePWM module later in the synchronization chain.<br>The ePWM module generates this signal from one of three event sources:                                                                                                                                                                                                                                                                                                                                                                                                            |
|            | 1.<br>EPWMxSYNCI (Synchronization input pulse)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|            | 2.<br>CTR = 0: The time-base counter equal to zero (TBCNT = 0000h).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
|            | 3.<br>CTR = CMPB: The time-base counter equal to the counter-compare B (TBCNT = CMPB) register.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| CTR = PRD  | Time-base counter equal to the specified period.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|            | This signal is generated whenever the counter value is equal to the active period register value. That is when<br>TBCNT = TBPRD.                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| CTR = 0    | Time-base counter equal to zero.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|            | This signal is generated whenever the counter value is zero. That is when TBCNT equals 0000h.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| CTR = CMPB | Time-base counter equal to active counter-compare B register (TBCNT = CMPB).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|            | This event is generated by the counter-compare submodule and used by the synchronization out logic.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| CTR_dir    | Time-base counter direction.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|            | Indicates the current direction of the ePWM's time-base counter. This signal is high when the counter is<br>increasing and low when it is decreasing.                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| CTR_max    | Time-base counter equal max value. (TBCNT = FFFFh)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|            | Generated event when the TBCNT value reaches its maximum value. This signal is only used only as a status<br>bit.                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| TBCLK      | Time-base clock.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|            | This is a prescaled version of the system clock (SYSCLKOUT) and is used by all submodules within the<br>ePWM. This clock determines the rate at which time-base counter increments or decrements.                                                                                                                                                                                                                                                                                                                                                                                               |

#### *15.2.2.3.3 Calculating PWM Period and Frequency*

The frequency of PWM events is controlled by the time-base period (TBPRD) register and the mode of the time-base counter. [Figure](#page-21-0) 15-12 shows the period (Tpwm) and frequency (Fpwm) relationships for the upcount, down-count, and up-down-count time-base counter modes when when the period is set to 4 (TBPRD = 4). The time increment for each step is defined by the time-base clock (TBCLK) which is a prescaled version of the system clock (SYSCLKOUT).

The time-base counter has three modes of operation selected by the time-base control register (TBCTL):

- **Up-Down-Count Mode:** In up-down-count mode, the time-base counter starts from zero and increments until the period (TBPRD) value is reached. When the period value is reached, the timebase counter then decrements until it reaches zero. At this point the counter repeats the pattern and begins to increment.
- **Up-Count Mode:** In this mode, the time-base counter starts from zero and increments until it reaches the value in the period register (TBPRD). When the period value is reached, the time-base counter resets to zero and begins to increment once again.
- **Down-Count Mode:** In down-count mode, the time-base counter starts from the period (TBPRD) value and decrements until it reaches zero. When it reaches zero, the time-base counter is reset to the period value and it begins to decrement once again.

```
(1) Up-count (0 -> PRD -> 0)
      <------ T_PWM ------>

4 |          ┌─┐           ┌─┐           ┌─┐     PRD
3 |       ┌─┐│ │        ┌─┐│ │        ┌─┐│ │
2 |    ┌─┐│ ││ │     ┌─┐│ ││ │     ┌─┐│ ││ │
1 | ┌─┐│ ││ ││ │  ┌─┐│ ││ ││ │  ┌─┐│ ││ ││ │
0 └─┘ └┘ └┘ └┘ └┴─┘ └┘ └┘ └┘ └┴─┘ └┘ └┘ └┘ └──> t
    0  1  2  3  4   0  1  2  3  4   0  1  2  3  4

(2) Down-count (PRD -> 0 -> PRD)
      <------ T_PWM ------>

4 ┌─┐            ┌─┐            ┌─┐     PRD
3 │ └┐        ┌─┐│ └┐        ┌─┐│ └┐
2 │  └┐    ┌─┐│ ││  └┐    ┌─┐│ ││  └┐
1 │   └┐┌─┐│ ││ ││   └┐┌─┐│ ││ ││   └┐
0 └────┘└─┘└─┘└─┴└────┘└─┘└─┘└─┴└────┘──> t
    4  3  2  1  0   4  3  2  1  0   4  3  2  1  0

For Up-count and Down-count:
T_PWM = (TBPRD + 1) × T_TBCLK
F_PWM = 1 / T_PWM

(3) Up-down count (0 -> PRD -> 0)
      <----------- T_PWM ----------->      <----------- T_PWM ----------->

4 |          ┌─┐                                ┌─┐
3 |       ┌─┐│ │┌─┐                          ┌─┐│ │┌─┐
2 |    ┌─┐│ ││ ││ │┌─┐                    ┌─┐│ ││ ││ │┌─┐
1 | ┌─┐│ ││ ││ ││ ││ │┌─┐              ┌─┐│ ││ ││ ││ ││ │┌─┐
0 └─┘ └┘ └┘ └┘ └┘ └┘ └┘ └┘ └───────────┘ └┘ └┘ └┘ └┘ └┘ └┘ └──> t
      Up   Down                          Up   Down

For Up-down count:
T_PWM = 2 × TBPRD × T_TBCLK
F_PWM = 1 / T_PWM
```
**Figure 15-12. Time-Base Frequency and Period**

#### *15.2.2.3.3.1 Time-Base Period Shadow Register*

CTR\_dir

The time-base period register (TBPRD) has a shadow register. Shadowing allows the register update to be synchronized with the hardware. The following definitions are used to describe all shadow registers in the ePWM module:

- **Active Register:** The active register controls the hardware and is responsible for actions that the hardware causes or invokes.
- **Shadow Register:** The shadow register buffers or provides a temporary holding location for the active register. It has no direct effect on any control hardware. At a strategic point in time the shadow register's content is transferred to the active register. This prevents corruption or spurious operation due to the register being asynchronously modified by software.

The memory address of the shadow period register is the same as the active register. Which register is written to or read from is determined by the TBCTL[PRDLD] bit. This bit enables and disables the TBPRD shadow register as follows:

- **Time-Base Period Shadow Mode:** The TBPRD shadow register is enabled when TBCTL[PRDLD] = 0. Reads from and writes to the TBPRD memory address go to the shadow register. The shadow register contents are transferred to the active register (TBPRD (Active) ← TBPRD (shadow)) when the time-base counter equals zero (TBCNT = 0000h). By default the TBPRD shadow register is enabled.
- **Time-Base Period Immediate Load Mode:** If immediate load mode is selected (TBCTL[PRDLD] = 1), then a read from or a write to the TBPRD memory address goes directly to the active register.

#### *15.2.2.3.3.2 Time-Base Counter Synchronization*

A time-base synchronization scheme connects all of the ePWM modules on a device. Each ePWM module has a synchronization input (EPWMxSYNCI) and a synchronization output (EPWMxSYNCO). The synchronization input can come from an external pin or another ePWM module. An example of synchronization connections for the remaining ePWM modules is shown in [Section](#page-3-0) 15.1.2.

EPWM1SYNCI ePWM1 EPWM1SYNCO EPWM0SYNCO ePWM0 EPWM0SYNCI EPWM2SYNCO ePWM2 EPWM2SYNCI ePWMx EPWMxSYNCI EPWMxSYNCO GPIO MUX

**Figure 15-13. Time-Base Counter Synchronization Scheme 1**

Each ePWM module can be configured to use or ignore the synchronization input. If the TBCTL[PHSEN] bit is set, then the time-base counter (TBCNT) of the ePWM module will be automatically loaded with the phase register (TBPHS) contents when one of the following conditions occur:

- **EPWMxSYNCI: Synchronization Input Pulse:** The value of the phase register is loaded into the counter register when an input synchronization pulse is detected (TBPHS → TBCNT). This operation occurs on the next valid time-base clock (TBCLK) edge.
- **Software Forced Synchronization Pulse:** Writing a 1 to the TBCTL[SWFSYNC] control bit invokes a software forced synchronization. This pulse is ORed with the synchronization input signal, and therefore has the same effect as a pulse on EPWMxSYNCI.

This feature enables the ePWM module to be automatically synchronized to the time base of another ePWM module. Lead or lag phase control can be added to the waveforms generated by different ePWM modules to synchronize them. In up-down-count mode, the TBCTL[PSHDIR] bit configures the direction of the time-base counter immediately after a synchronization event. The new direction is independent of the direction prior to the synchronization event. The TBPHS bit is ignored in count-up or count-down modes. See [Figure](#page-24-0) 15-14 through [Figure](#page-26-0) 15-17 for examples.

Clearing the TBCTL[PHSEN] bit configures the ePWM to ignore the synchronization input pulse. The synchronization pulse can still be allowed to flow-through to the EPWMxSYNCO and be used to synchronize other ePWM modules. In this way, you can set up a master time-base (for example, ePWM0) and downstream modules (ePWM1 – ePWMx) may elect to run in synchronization with the master.

#### *15.2.2.3.4 Phase Locking the Time-Base Clocks of Multiple ePWM Modules*

The TBCLKEN bit in the PWMSS\_CTRL register in the Control Module can be used to globally synchronize the time-base clocks of all enabled ePWM modules on a device. The TBCLKEN bit is part of the chip configuration registers and is described in Chapter 9. When TBCLKEN = 0, the time-base clock of all ePWM modules is stopped (default). When TBCLKEN = 1, all ePWM time-base clocks are started with the rising edge of TBCLK aligned. For perfectly synchronized TBCLKs, the prescaler bits in the TBCTL register of each ePWM module must be set identically. The proper procedure for enabling the ePWM clocks is as follows:

- 1. Enable the ePWM module clocks.
- 2. Set TBCLKEN = 0. This will stop the time-base clock within any enabled ePWM module.
- 3. Configure the prescaler values and desired ePWM modes.
- 4. Set TBCLKEN = 1.

#### *15.2.2.3.5 Time-Base Counter Modes and Timing Waveforms*

The time-base counter operates in one of four modes:

- Up-count mode which is asymmetrical.
- Down-count mode which is asymmetrical.
- Up-down-count which is symmetrical.
- Frozen where the time-base counter is held constant at the current value.

To illustrate the operation of the first three modes, [Figure](#page-24-0) 15-14 to [Figure](#page-26-0) 15-17 show when events are generated and how the time-base responds to an EPWMxSYNCI signal.

```
Waveforms (relative timing as in figure)

      |---------|---------|---------|---------|---------|---------> t
      t1        t2        t3        t4        t5        t6

FFFFh ^
      |
TBPRD +     /|      /|        /|       /|      /|      /|
      |    / |     / |       / |      / |     / |     / |
TBPHS +   /  |    /  |   /\ /  |     /  |    /  |    /  |
      |  /   |   /   |  /  V   |    /   |   /   |   /   |
0000h +-/----+--/----+-/--------+--/----+--/----+--/----+---->

EPWMxSYNCI
      _____________________________|‾|______________________________

CTR_dir
      ______________________________________________________________

CTR = 0
      _____________|‾|___________|‾|_______________|‾|______|‾|______

CTR = PRD
      _____________|‾|___________|‾|_______________|‾|______|‾|______

CNT_max
      ______________________________________________________________

Notes shown in plot:
- Horizontal reference levels: `TBPRD (value)`, `TBPHS (value)`, bottom `0000h`.
- Dashed vertical markers align the `CTR=0` and `CTR=PRD` pulses with the TBCNT sawtooth edges.
- One `EPWMxSYNCI` pulse occurs mid-stream and causes the brief phase jump in `TBCNT` (reload toward `TBPHS` level).
```
**Figure 15-14. Time-Base Up-Count Mode Waveforms**

**Figure 15-15. Time-Base Down-Count Mode Waveforms**

```
Waveforms (relative timing as in Figure 4 — down-count behavior)

      |---------|---------|---------|---------|---------|---------> t
      t1        t2        t3        t4        t5        t6

FFFFh ^
      |
TBPRD +-+|\--------+|\---------------+|\----+|\-------+|\---------+
      |  | \        | \             __| \    | \       | \        |
TBPHS +  |  \       |  \           /  |  \   |  \__    |  \       |
      |  |   \      |   \         /   |   \  |     \   |   \      |
0000h +--+----\-----+----\-------+----+----\--+------\--+----\-----+---->
          \           \               \          |‾|        \
           \           \               \         SYNC        \
            \           \               \                    \
             +-----------+               +--------------------+
             (wrap/jump to PRD)          (phase jump on SYNC)

EPWMxSYNCI
      _____________________________|‾|______________|‾|______________

CTR_dir
      ________________________________________________________________

CTR = 0
      ________|‾|_________|‾|________________|‾|______|‾|______|‾|_____

CTR = PRD
      ________|‾|_________|‾|________________|‾|______|‾|______|‾|_____

CNT_max
      ________________________________________________________________

Markers/levels in the plot:
- Horizontal reference levels: `TBPRD (value)`, `TBPHS (value)`, bottom `0000h`.
- Dashed vertical lines align the `CTR=0` and `CTR=PRD` pulses with the TBCNT boundary events.
- Two `EPWMxSYNCI` pulses occur mid-stream, each causing a brief phase reload/jump in `TBCNT` toward the `TBPHS` level while counting down.
```

**Figure 15-16. Time-Base Up-Down-Count Waveforms, TBCTL[PHSDIR = 0] Count Down on Synchronization Event**

```
Waveforms (relative timing as in Figure 6 — up-down count with sync events)

      |----------|----------|----------|----------|----------|-----> t
      t1         t2         t3         t4         t5         t6

FFFFh ^
      |
TBPRD +        /\                 /\                    /\            /\
      |       /  \               /  \                  /  \          /  \
TBPHS +      /    \      /\     /    \        /\      /    \        /    \
      |     /      \    /  \   /      \      /  \    /      \      /      \
0000h +----/--------\--/----\-/--------\----/----\--/--------\----/--------\-->
              (PRD)   (0)        (0)             (0)               (PRD)

EPWMxSYNCI
      _______________________|‾|_____________________|‾|_____________________

CTR_dir
      ─────────── UP ───────────┐      ┌── UP ──┐   ┌──── UP ────┐   ┌─ UP ──
                                └─ DOWN ┘       └─DOWN┘          └─DOWN┘

CTR = 0
      ___________________________|‾|___________________________|‾|_____________

CTR = PRD
      _____________|‾|_______________________________________________|‾|______

CNT_max
      ________________________________________________________________________

What the plot shows:
- `TBCNT` is triangular (counts UP to `TBPRD`, then DOWN to `0000h`).
- Each `EPWMxSYNCI` pulse causes a phase reload/jump toward the `TBPHS (value)` level, producing the small “kink” in the triangle.
- `CTR_dir` indicates the counting direction (UP/DOWN) between turning points.
```

**Figure 15-17. Time-Base Up-Down Count Waveforms, TBCTL[PHSDIR = 1] Count Up on Synchronization Event**

```
Waveforms (relative timing as in Figure 4 — up-down count with sync pulses)

      |----------|----------|----------|----------|----------|-----> t
      t1         t2         t3         t4         t5         t6

FFFFh ^
      |
TBPRD +        /\                    /\        /\
      |       /  \                  /  \      /  \
TBPHS +      /    \        |‾|     /    \    /    \
      |     /      \       | |    /      \  /      \
0000h +----/--------\------+-+---/--------\/--------\--------->
              (PRD)   (0)   ^    (PRD)     (PRD)      (0)
                            |
                         sync causes a jump to TBPHS during the cycle
                         (kink/step visible in the triangle)

EPWMxSYNCI
      _______________________|‾|_____________________|‾|_____________________

CTR_dir
      ─────────── UP ───────────┐  ┌── UP ──┐  ┌─ UP ─┐  ┌────── DOWN ──────
                                └──┘  DOWN  └──┘ DOWN └──┘

CTR = 0
      ___________________________|‾|_____________________________________|‾|_

CTR = PRD
      _____________|‾|_____________________|‾|__________|‾|___________________

CNT_max
      ________________________________________________________________________
```

#### 15.2.2.4 Counter-Compare (CC) Submodule

Figure 15-18 illustrates the counter-compare submodule within the ePWM. Figure 15-19 shows the basic structure of the counter-compare submodule.

```
EPWMxSYNCI  ---> +------------------+ <--- EPWMxSYNCO
                 |  Time-Base (TB)  |
                 +------------------+
                      |   |   |
                      |   |   +--> CTR_Dir ------------------------------+
                      |   +-------> CTR = 0 -------------------------+   |
                      +-----------> CTR = PRD ---------------------+ |   |
                                                                      |   |
                 +-----------------------+                            |   |
                 | Counter Compare (CC)  |                            |   |
                 +-----------------------+                            |   |
                      |            |                                  |   |
                      |            +--> CTR = CMPB ----------------+  |   |
                      +--> CTR = CMPA --------------------------+  |  |   |
                                                                   v  v   v
                                                           +----------------------+
                                                           | Action Qualifier (AQ)|
                                                           +----------------------+
                                                               |            |
                                                               |            |
                                                           EPWMxA         EPWMxB
                                                               |            |
                                                               v            v
                                                           +--------+   +--------+
                                                           |  DB    |   |  DB    |   Dead-Band
                                                           +--------+   +--------+
                                                               \            /
                                                                v          v
                                                             +-------------+
                                                             |    PC       |   PWM-chopper
                                                             +-------------+
                                                                   |
                                                                   v
                                                             +-------------+  <--- TZ1 to TZn
                                                             |    TZ       |       (Trip inputs)
                                                             | Trip Zone   |
                                                             +-------------+
                                                               |       |
                                                               |       +----> EPWMxB -----> +----------+
                                                               +----------> EPWMxA ----->  | GPIO MUX  |
                                                                                          +----------+

Event/interrupt generation:
 CTR = PRD  --------------------+
 CTR = 0    --------------------+
 CTR = CMPA --------------------+----> +------------------------------+ ---> EPWMxINT ---> +---------------------+
 CTR = CMPB --------------------+      | Event Trigger & Interrupt (ET)|                  | Interrupt controller |
 CTR_Dir   ---------------------+      +------------------------------+                  +---------------------+

Trip-zone interrupt:
 CTR = 0 -------------------------------> (into TZ logic)
 EPWMxTZINT ----------------------------> +---------------------+
                                         | Interrupt controller |
                                         +---------------------+
```

Figure 15-18. Counter-Compare Submodule

Figure 15-19. Counter-Compare Submodule Signals and Registers

```
                         +----------------------+
                         | Time Base (TB)       |
                         |      Module          |
                         +----------------------+
                                   |
                                   |  TBCNT[15:0]
                                   |==============================+
                                   |                              |
                                   |                              |
                                   v                              v

(Shadow-load select for A)                                   (Comparator A path)
 CTR = PRD ----\                                             TBCNT[15:0] ----\
 CTR = 0   -----)--> [Shadow load] --------------------+                      )--> [Digital comparator A]
               /     ^                                 |  CMPA[15:0] --------/        |
                     | CMPCTL[LOADAMODE]               v                                    +--> CTR = CMPA ----+
                                                     +-------------------------------+      |                   |
                                                     |            CMPA               |      |                   v
                                                     |  Compare A Active Reg.        |<-----+              +------------------------+
                                                     |  Compare A Shadow Reg.        |  CMPCTL[SHDWAMODE]          Action Qualifier |
                                                     +-------------------------------+  CMPCTL[SHDWAFULL]          (AQ) Module      |
                                                                                                           +------------------------+

(Shadow-load select for B)                                   (Comparator B path)
 CTR = PRD ----\                                             TBCNT[15:0] ----\
 CTR = 0   -----)--> [Shadow load] --------------------+                      )--> [Digital comparator B]
               /     ^                                 | CMPB[15:0] --------/        |
                     | CMPCTL[LOADBMODE]               v                                    +--> CTR = CMPB ----+
                                                     +-------------------------------+      |                   |
                                                     |            CMPB               |      |                   v
                                                     |  Compare B Active Reg.        |<-----+              +------------------------+
                                                     |  Compare B Shadow Reg.        |  CMPCTL[SHDWBMODE]  |       Action Qualifier |
                                                     +-------------------------------+  CMPCTL[SHDWBFULL]  |       (AQ) Module      |
                                                                                                           +------------------------+
```

#### *15.2.2.4.1 Purpose of the Counter-Compare Submodule*

The counter-compare submodule takes as input the time-base counter value. This value is continuously compared to the counter-compare A (CMPA) and counter-compare B (CMPB) registers. When the timebase counter is equal to one of the compare registers, the counter-compare unit generates an appropriate event.

The counter-compare submodule:

- Generates events based on programmable time stamps using the CMPA and CMPB registers
  - CTR = CMPA: Time-base counter equals counter-compare A register (TBCNT = CMPA).
  - CTR = CMPB: Time-base counter equals counter-compare B register (TBCNT = CMPB)
- Controls the PWM duty cycle if the action-qualifier submodule is configured appropriately
- Shadows new compare values to prevent corruption or glitches during the active PWM cycle

#### *15.2.2.4.2 Controlling and Monitoring the Counter-Compare Submodule*

Table [15-13](#page-28-0) lists the registers used to control and monitor the counter-compare submodule. Table [15-14](#page-28-1) lists the key signals associated with the counter-compare submodule.

**Table 15-13. Counter-Compare Submodule Registers**

| Acronym | Register Description                           | Address Offset | Shadowed |
|---------|------------------------------------------------|----------------|----------|
| CMPCTL  | Counter-Compare Control Register.              | Eh             | No       |
| CMPAHR  | HRPWM Counter-Compare A Extension Register (1) | 10h            | Yes      |
| CMPA    | Counter-Compare A Register                     | 12h            | Yes      |
| CMPB    | Counter-Compare B Register                     | 14h            | Yes      |

<sup>(1)</sup> This register is available only on ePWM modules with the high-resolution extension (HRPWM). On ePWM modules that do not include the HRPWM, this location is reserved. See [Section](#page-3-0) 15.1.2 to determine which ePWM instances include this feature.

**Table 15-14. Counter-Compare Submodule Key Signals**

| Signal     | Description of Event                                                                                                            | Registers Compared |
|------------|---------------------------------------------------------------------------------------------------------------------------------|--------------------|
| CTR = CMPA | Time-base counter equal to the active counter-compare A value                                                                   | TBCNT = CMPA       |
| CTR = CMPB | Time-base counter equal to the active counter-compare B value                                                                   | TBCNT = CMPB       |
| CTR = PRD  | Time-base counter equal to the active period.<br>Used to load active counter-compare A and B registers from the shadow register | TBCNT = TBPRD      |
| CTR = 0    | Time-base counter equal to zero.<br>Used to load active counter-compare A and B registers from the shadow register              | TBCNT = 0000h      |

#### *15.2.2.4.3 Operational Highlights for the Counter-Compare Submodule*

The counter-compare submodule is responsible for generating two independent compare events based on two compare registers:

- 1. CTR = CMPA: Time-base counter equal to counter-compare A register (TBCNT = CMPA).
- 2. CTR = CMPB: Time-base counter equal to counter-compare B register (TBCNT = CMPB).

For up-count or down-count mode, each event occurs only once per cycle. For up-down-count mode each event occurs twice per cycle, if the compare value is between 0000h and TBPRD; and occurs once per cycle, if the compare value is equal to 0000h or equal to TBPRD. These events are fed into the actionqualifier submodule where they are qualified by the counter direction and converted into actions if enabled. Refer to Section [15.2.2.5.1](#page-32-0) for more details.

The counter-compare registers CMPA and CMPB each have an associated shadow register. Shadowing provides a way to keep updates to the registers synchronized with the hardware. When shadowing is used, updates to the active registers only occurs at strategic points. This prevents corruption or spurious operation due to the register being asynchronously modified by software. The memory address of the active register and the shadow register is identical. Which register is written to or read from is determined by the CMPCTL[SHDWAMODE] and CMPCTL[SHDWBMODE] bits. These bits enable and disable the CMPA shadow register and CMPB shadow register respectively. The behavior of the two load modes is described below:

• **Shadow Mode:** The shadow mode for the CMPA is enabled by clearing the CMPCTL[SHDWAMODE] bit and the shadow register for CMPB is enabled by clearing the CMPCTL[SHDWBMODE] bit. Shadow mode is enabled by default for both CMPA and CMPB.

If the shadow register is enabled then the content of the shadow register is transferred to the active register on one of the following events:

- CTR = PRD: Time-base counter equal to the period (TBCNT = TBPRD).
- CTR = 0: Time-base counter equal to zero (TBCNT = 0000h)
- Both CTR = PRD and CTR = 0

Which of these three events is specified by the CMPCTL[LOADAMODE] and CMPCTL[LOADBMODE] register bits. Only the active register contents are used by the counter-compare submodule to generate events to be sent to the action-qualifier.

• **Immediate Load Mode:** If immediate load mode is selected (TBCTL[SHADWAMODE] = 1 or TBCTL[SHADWBMODE] = 1), then a read from or a write to the register will go directly to the active register.

#### *15.2.2.4.4 Count Mode Timing Waveforms*

The counter-compare module can generate compare events in all three count modes:

- Up-count mode: used to generate an asymmetrical PWM waveform.
- Down-count mode: used to generate an asymmetrical PWM waveform.
- Up-down-count mode: used to generate a symmetrical PWM waveform.

To best illustrate the operation of the first three modes, the timing diagrams in [Figure](#page-30-0) 15-20 to [Figure](#page-31-0) 15- [23](#page-31-0) show when events are generated and how the EPWMxSYNCI signal interacts.

0000h FFFFh CTR = CMPA TBCNT CMPA (value) CMPB (value) TBPHS (value) TBPRD (value) CTR = CMPB EPWMxSYNCI

**Figure 15-20. Counter-Compare Event Waveforms in Up-Count Mode**

NOTE: An EPWMxSYNCI external synchronization event can cause a discontinuity in the TBCNT count sequence. This can lead to a compare event being skipped. This skipping is considered normal operation and must be taken into account.
```
Figure 6 — Counter compare events in DOWN-count mode (text waveform)

      |----------|----------|----------|----------|----------|-----> t
      t1         t2         t3         t4         t5         t6

TBCNT (DOWN ramps; wrap/jump back to TBPRD)
FFFFh ^
      |
TBPRD +--+ \-----------+ \-----------+ \------+ \----------+ \-----------+
CMPA  +  |  \     ●    |  \     ●    |  \     |  \    ●    |  \    ●     |
CMPB  +● |   \      ●  |   \      ●  |   \    |   \     ●  |   \     ●   |
TBPHS +       \             \             \   |    \            \        |
0000h +--------\-------------\-------------\--+-----\------------\--------\-->
                \             \             \ |      \            \
                 \             \             \|       \            \
                  +-------------+-------------+        +------------+
              (jump to TBPRD)                      (sync causes a kink/jump)

EPWMxSYNCI
      _____________________________________________|‾|________________________

CTR = CMPA
      ______________________|‾|____________________|‾|__________|‾|____________

CTR = CMPB
      _|‾|__________________________|‾|__________________________________|‾|___

Legend:
- Horizontal dashed levels: `TBPRD(value)`, `CMPA(value)`, `CMPB(value)`, `TBPHS(value)`, `0000h`.
- Dots (●) mark where the down-counting `TBCNT` crosses `CMPA` or `CMPB`; vertical markers align with the pulses below.
```

**Figure 15-21. Counter-Compare Events in Down-Count Mode**

**Figure 15-22. Counter-Compare Events in Up-Down-Count Mode, TBCTL[PHSDIR = 0] Count Down on Synchronization Event**

```
Figure 4 — Counter compare events in UP-DOWN count mode (text waveform)

      |---------|---------|---------|---------|---------|-------> t

TBCNT (triangle; crosses CMPB and CMPA on the way UP and on the way DOWN)
FFFFh ^
      |
TBPRD +           /\                               /\
CMPA  +      ●   /  \  ●                      ●   /  \  ●
CMPB  +   ●     /    \    ●                ●     /    \    ●
TBPHS +        /      \__                      _/      \
0000h +-------/----------\____---------------/----------\--------->
                      (sync causes a drop/kink here)

EPWMxSYNCI
      ____________________________|‾|_______________________________________

CTR = CMPB
      _________|‾|__________|‾|__________|‾|__________|‾|____________________

CTR = CMPA
      ______________|‾|__________|‾|________________|‾|__________|‾|________

Legend:
- Levels: `TBPRD(value)`, `CMPA(value)`, `CMPB(value)`, `TBPHS(value)`, `0000h`.
- Dots (●) mark the `TBCNT == CMPB` / `TBCNT == CMPA` crossings; vertical alignment corresponds to the pulses on `CTR=CMPB` and `CTR=CMPA`.
- The single `EPWMxSYNCI` pulse produces the mid-cycle “kink” (phase reload) in the triangle.
```

**Figure 15-23. Counter-Compare Events in Up-Down-Count Mode, TBCTL[PHSDIR = 1] Count Up on Synchronization Event**

```
Figure 6 — Counter compare events in UP-DOWN count mode (with sync “kink”)

      |---------|---------|---------|---------|---------|-------> t

TBCNT (triangle; CMPB and CMPA events on both UP and DOWN slopes)
FFFFh ^
      |
TBPRD +           /\                           /\
CMPA  +      ●   /  \  ●                  ●   /  \  ●
CMPB  +   ●     /    \    ●            ●     /    \    ●
TBPHS +        /      \__                   /      \
0000h +-------/----------\__---------------/--------\__________________->
                      (sync causes a sharp drop to ~TBPHS here)

EPWMxSYNCI
      ____________________________|‾|_______________________________________

CTR = CMPB
      _________|‾|__________|‾|_____|‾|_______________|‾|_____________________

CTR = CMPA
      ______________|‾|__________|‾|__________|‾|__________|‾|______________

Legend:
- Levels: `TBPRD(value)`, `CMPA(value)`, `CMPB(value)`, `TBPHS(value)`, `0000h`.
- Dots (●) mark `TBCNT == CMPB/CMPA`; the dashed vertical markers align with the pulses on `CTR=CMPB` and `CTR=CMPA`.
- The single `EPWMxSYNCI` pulse causes the mid-cycle “kink” (phase reload) dropping the counter toward `TBPHS`.
```

#### 15.2.2.5 Action-Qualifier (AQ) Submodule

Figure 15-24 shows the action-qualifier (AQ) submodule (see shaded block) in the ePWM system. The action-qualifier submodule has the most important role in waveform construction and PWM generation. It decides which events are converted into various action types, thereby producing the required switched waveforms at the EPWMxA and EPWMxB outputs.

CTR = PRD Event CTR = 0Trigger Action **EPWMxINT** Interrupt CTR = CMPA and Qualifier **EPWMxSYNCI** controller CTR = PRD <sup>⊥</sup> Interrupt (AQ) CTR = CMPB CTR = 0 <sup>⊥</sup> Time-Base (ET) CTR\_Dir **FPWMxSYNCO** (TB) CTR Dir **EPWMxA** EPWMxA | Dead PWM-Trip Band choppei Zone CTR = CMPA <sup>⊥</sup> (DB) **GPIO** (PC) (TZ) Counter **EPWMxB** MUX Compare **EPWMxB** CTR = CMPB L (CC) CTR = 0TZ1 to TZn **EPWMxTZINT** Interrupt controller

Figure 15-24. Action-Qualifier Submodule

#### 15.2.2.5.1 Purpose of the Action-Qualifier Submodule

The action-qualifier submodule is responsible for the following:

- Qualifying and generating actions (set, clear, toggle) based on the following events:
  - CTR = PRD: Time-base counter equal to the period (TBCNT = TBPRD)
  - CTR = 0: Time-base counter equal to zero (TBCNT = 0000h)
  - CTR = CMPA: Time-base counter equal to the counter-compare A register (TBCNT = CMPA)
  - CTR = CMPB: Time-base counter equal to the counter-compare B register (TBCNT = CMPB)
- Managing priority when these events occur concurrently
- Providing independent control of events when the time-base counter is increasing and when it is decreasing.

#### 15.2.2.5.2 Controlling and Monitoring the Action-Qualifier Submodule

Table 15-15 lists the registers used to control and monitor the action-qualifier submodule.

Table 15-15. Action-Qualifier Submodule Registers
\nister Description Address Of

| Acronym | Register Description                                    | Address Offset | Shadowed |
|---------|---------------------------------------------------------|----------------|----------|
| AQCTLA  | Action-Qualifier Control Register For Output A (EPWMxA) | 16h            | No       |
| AQCTLB  | Action-Qualifier Control Register For Output B (EPWMxB) | 18h            | No       |
| AQSFRC  | Action-Qualifier Software Force Register                | 1Ah            | No       |
| AQCSFRC | Action-Qualifier Continuous Software Force              | 1Ch            | Yes      |

The action-qualifier submodule is based on event-driven logic. It can be thought of as a programmable cross switch with events at the input and actions at the output, all of which are software controlled via the set of registers shown in [Figure](#page-33-0) 15-25. The possible input events are summarized again in Table [15-16](#page-33-1).

```
Inputs (left)                                           Outputs (right)
----------------                                       -------------------
TBCLK  --------------------------------------------->  EPWMA
CTR = PRD  --(event)--------------------------------->
CTR = 0    --(event)--------------------------------->
CTR = CMPA --(event)--------------------------------->                   +----------------------+
CTR = CMPB --(event)--------------------------------->                   | Action-qualifier     |
CTR_dir  -------------------------------------------->                   |      (AQ) Module     |
                                                                         |                      |
                                                                         | +------------------+ |
                                                                         | | AQCTLA[15:0]     | |
                                                                         | | Action-qualifier | |
                                                                         | | control A        | |
                                                                         | +------------------+ |
                                                                         |                      |
                                                                         | +------------------+ |
                                                                         | | AQCTLB[15:0]     | |
                                                                         | | Action-qualifier | |
                                                                         | | control B        | |
                                                                         | +------------------+ |
                                                                         |                      |
                                                                         | +------------------+ |
                                                                         | | AQSFRC[15:0]     | |
                                                                         | | Action-qualifier | |
                                                                         | | S/W force        | |
                                                                         | +------------------+ |
                                                                         |                      |
                                                                         | +------------------+ |
                                                                         | | AQCSFRC[3:0]     | |
                                                                         | | (shadow)         | |
                                                                         | | continuous S/W   | |
                                                                         | | force            | |
                                                                         | +------------------+ |
                                                                         |          |           |
                                                                         |          v           |
                                                                         | +------------------+ |
                                                                         | | AQCSFRC[3:0]     | |
                                                                         | | (active)         | |
                                                                         | | continuous S/W   | |
                                                                         | | force            | |
                                                                         | +------------------+ |
                                                                         +----------------------+
                                                                                     |
                                                                                     +---------> EPWMB
```

**Figure 15-25. Action-Qualifier Submodule Inputs and Outputs**

**Table 15-16. Action-Qualifier Submodule Possible Input Events**

| Signal                | Description                                      | Registers Compared |
|-----------------------|--------------------------------------------------|--------------------|
| CTR = PRD             | Time-base counter equal to the period value      | TBCNT = TBPRD      |
| CTR = 0               | Time-base counter equal to zero                  | TBCNT = 0000h      |
| CTR = CMPA            | Time-base counter equal to the counter-compare A | TBCNT = CMPA       |
| CTR = CMPB            | Time-base counter equal to the counter-compare B | TBCNT = CMPB       |
| Software forced event | Asynchronous event initiated by software         |                    |

The software forced action is a useful asynchronous event. This control is handled by registers AQSFRC and AQCSFRC.

The action-qualifier submodule controls how the two outputs EPWMxA and EPWMxB behave when a particular event occurs. The event inputs to the action-qualifier submodule are further qualified by the counter direction (up or down). This allows for independent action on outputs on both the count-up and count-down phases.

The possible actions imposed on outputs EPWMxA and EPWMxB are:

- **Set High:** Set output EPWMxA or EPWMxB to a high level.
- **Clear Low:** Set output EPWMxA or EPWMxB to a low level.
- **Toggle:** If EPWMxA or EPWMxB is currently pulled high, then pull the output low. If EPWMxA or EPWMxB is currently pulled low, then pull the output high.
- **Do Nothing:** Keep outputs EPWMxA and EPWMxB at same level as currently set. Although the "Do Nothing" option prevents an event from causing an action on the EPWMxA and EPWMxB outputs, this event can still trigger interrupts. See the event-trigger submodule description in Section [15.2.2.9](#page-62-0) for details.

Actions are specified independently for either output (EPWMxA or EPWMxB). Any or all events can be configured to generate actions on a given output. For example, both CTR = CMPA and CTR = CMPB can operate on output EPWMxA. All qualifier actions are configured via the control registers found at the end of this section.

For clarity, the drawings in this chapter use a set of symbolic actions. These symbols are summarized in [Figure](#page-34-0) 15-26. Each symbol represents an action as a marker in time. Some actions are fixed in time (zero and period) while the CMPA and CMPB actions are moveable and their time positions are programmed via the counter-compare A and B registers, respectively. To turn off or disable an action, use the "Do Nothing option"; it is the default at reset.

**Figure 15-26. Possible Action-Qualifier Actions for EPWMxA and EPWMxB Outputs**

| S/W     | TB Counter equals: |           |           | Actions |            |
|---------|--------------------|-----------|-----------|---------|------------|
| force   | Zero               | Comp<br>A | Comp<br>B | Period  |            |
| SW      | Z                  | CA        | CB        | P       | Do Nothing |
| SW      | Z                  | CA        | CB        | P       | Clear Low  |
| SW      | Z                  | CA        | CB        | P       | Set High   |
| SW<br>T | Z<br>T             | CA<br>T   | CB<br>T   | P<br>T  | Toggle     |

#### *15.2.2.5.3 Action-Qualifier Event Priority*

It is possible for the ePWM action qualifier to receive more than one event at the same time. In this case events are assigned a priority by the hardware. The general rule is events occurring later in time have a higher priority and software forced events always have the highest priority. The event priority levels for updown-count mode are shown in Table [15-17](#page-35-0). A priority level of 1 is the highest priority and level 7 is the lowest. The priority changes slightly depending on the direction of TBCNT.

**Table 15-17. Action-Qualifier Event Priority for Up-Down-Count Mode**

| Priority Level | Event if TBCNT is Incrementing<br>TBCNT = 0 up to TBCNT = TBPRD | Event if TBCNT is Decrementing<br>TBCNT = TBPRD down to TBCNT = 1 |
|----------------|-----------------------------------------------------------------|-------------------------------------------------------------------|
| 1 (Highest)    | Software forced event                                           | Software forced event                                             |
| 2              | Counter equals CMPB on up-count (CBU)                           | Counter equals CMPB on down-count (CBD)                           |
| 3              | Counter equals CMPA on up-count (CAU)                           | Counter equals CMPA on down-count (CAD)                           |
| 4              | Counter equals zero                                             | Counter equals period (TBPRD)                                     |
| 5              | Counter equals CMPB on down-count (CBD) (1)                     | Counter equals CMPB on up-count (CBU) (1)                         |
| 6 (Lowest)     | Counter equals CMPA on down-count (CAD) (1)                     | Counter equals CMPA on up-count (CBU) (1)                         |

<sup>(1)</sup> To maintain symmetry for up-down-count mode, both up-events (CAU/CBU) and down-events (CAD/CBD) can be generated for TBPRD. Otherwise, up-events can occur only when the counter is incrementing and down-events can occur only when the counter is decrementing.

Table [15-18](#page-35-1) shows the action-qualifier priority for up-count mode. In this case, the counter direction is always defined as up and thus down-count events will never be taken.

**Table 15-18. Action-Qualifier Event Priority for Up-Count Mode**

| Priority Level | Event                                   |
|----------------|-----------------------------------------|
| 1 (Highest)    | Software forced event                   |
| 2              | Counter equal to period (TBPRD)         |
| 3              | Counter equal to CMPB on up-count (CBU) |
| 4              | Counter equal to CMPA on up-count (CAU) |
| 5 (Lowest)     | Counter equal to Zero                   |

Table [15-19](#page-35-2) shows the action-qualifier priority for down-count mode. In this case, the counter direction is always defined as down and thus up-count events will never be taken.

**Table 15-19. Action-Qualifier Event Priority for Down-Count Mode**

| Priority Level | Event                                     |
|----------------|-------------------------------------------|
| 1 (Highest)    | Software forced event                     |
| 2              | Counter equal to Zero                     |
| 3              | Counter equal to CMPB on down-count (CBD) |
| 4              | Counter equal to CMPA on down-count (CAD) |
| 5 (Lowest)     | Counter equal to period (TBPRD)           |

It is possible to set the compare value greater than the period. In this case the action will take place as shown in Table [15-20](#page-36-0).

**Table 15-20. Behavior if CMPA/CMPB is Greater than the Period**

| Counter Mode          | Compare on Up-Count Event CAU/CBU                                                                                       | Compare on Down-Count Event CAU/CBU                                                                                     |
|-----------------------|-------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------|
| Up-Count Mode         | If CMPA/CMPB ≤ TBPRD period, then the event<br>occurs on a compare match (TBCNT = CMPA or<br>CMPB).                     | Never occurs.                                                                                                           |
|                       | If CMPA/CMPB > TBPRD, then the event will not<br>occur.                                                                 |                                                                                                                         |
| Down-Count Mode       | Never occurs.                                                                                                           | If CMPA/CMPB < TBPRD, the event will occur on a<br>compare match (TBCNT = CMPA or CMPB).                                |
|                       |                                                                                                                         | If CMPA/CMPB ≥ TBPRD, the event will occur on a<br>period match (TBCNT = TBPRD).                                        |
| Up-Down-Count<br>Mode | If CMPA/CMPB < TBPRD and the counter is<br>incrementing, the event occurs on a compare match<br>(TBCNT = CMPA or CMPB). | If CMPA/CMPB < TBPRD and the counter is<br>decrementing, the event occurs on a compare match<br>(TBCNT = CMPA or CMPB). |
|                       | If CMPA/CMPB is ≥ TBPRD, the event will occur on a<br>period match (TBCNT = TBPRD).                                     | If CMPA/CMPB ≥ TBPRD, the event occurs on a<br>period match (TBCNT = TBPRD).                                            |

#### *15.2.2.5.4 Waveforms for Common Configurations*

**NOTE:** The waveforms in this chapter show the ePWMs behavior for a static compare register value. In a running system, the active compare registers (CMPA and CMPB) are typically updated from their respective shadow registers once every period. The user specifies when the update will take place; either when the time-base counter reaches zero or when the timebase counter reaches period. There are some cases when the action based on the new value can be delayed by one period or the action based on the old value can take effect for an extra period. Some PWM configurations avoid this situation. These include, but are not limited to, the following:

#### **Use up-down-count mode to generate a symmetric PWM:**

- If you load CMPA/CMPB on zero, then use CMPA/CMPB values greater than or equal to 1.
- If you load CMPA/CMPB on period, then use CMPA/CMPB values less than or equal to TBPRD - 1. This means there will always be a pulse of at least one TBCLK cycle in a PWM period which, when very short, tend to be ignored by the system.

#### **Use up-down-count mode to generate an asymmetric PWM:**

• To achieve 50%-0% asymmetric PWM use the following configuration: Load CMPA/CMPB on period and use the period action to clear the PWM and a compare-up action to set the PWM. Modulate the compare value from 0 to TBPRD to achieve 50%-0% PWM duty.

#### **When using up-count mode to generate an asymmetric PWM:**

• To achieve 0-100% asymmetric PWM use the following configuration: Load CMPA/CMPB on TBPRD. Use the Zero action to set the PWM and a compareup action to clear the PWM. Modulate the compare value from 0 to TBPRD+1 to achieve 0-100% PWM duty.

[Figure](#page-37-0) 15-27 shows how a symmetric PWM waveform can be generated using the up-down-count mode of the TBCNT. In this mode 0%-100% DC modulation is achieved by using equal compare matches on the up count and down count portions of the waveform. In the example shown, CMPA is used to make the comparison. When the counter is incrementing the CMPA match will pull the PWM output high. Likewise, when the counter is decrementing the compare match will pull the PWM signal low. When CMPA = 0, the PWM signal is low for the entire period giving the 0% duty waveform. When CMPA = TBPRD, the PWM signal is high achieving 100% duty.

When using this configuration in practice, if you load CMPA/CMPB on zero, then use CMPA/CMPB values greater than or equal to 1. If you load CMPA/CMPB on period, then use CMPA/CMPB values less than or equal to TBPRD-1. This means there will always be a pulse of at least one TBCLK cycle in a PWM period which, when very short, tend to be ignored by the system.

UP DOWN UP DOWN 2 0 3 4 1 2 3 1 2 0 3 4 1 2 0 3 1 TBCNT TBCTR Direction EPWMxA/EPWMxB Case 2: CMPA = 3, 25% Duty Case 3: CMPA = 2, 50% Duty Case 3: CMPA = 1, 75% Duty Case 4: CMPA = 0, 100% Duty Case 1: CMPA = 4, 0% Duty EPWMxA/EPWMxB EPWMxA/EPWMxB EPWMxA/EPWMxB EPWMxA/EPWMxB Mode: Up-Down Count TBPRD = 4 CAU = SET, CAD = CLEAR 0% - 100% Duty

**Figure 15-27. Up-Down-Count Mode Symmetrical Waveform**

The PWM waveforms in [Figure](#page-38-0) 15-28 through [Figure](#page-48-0) 15-33 show some common action-qualifier configurations. Some conventions used in the figures are as follows:

- TBPRD, CMPA, and CMPB refer to the value written in their respective registers. The active register, not the shadow register, is used by the hardware.
- CMPx, refers to either CMPA or CMPB.
- EPWMxA and EPWMxB refer to the output signals from ePWMx
- Up-Down means Count-up-and-down mode, Up means up-count mode and Dwn means down-count mode
- Sym = Symmetric, Asym = Asymmetric

Table [15-21](#page-39-0) and Table [15-22](#page-39-1) contains initialization and runtime register configurations for the waveforms in [Figure](#page-38-0) 15-28.

**Figure 15-28. Up, Single Edge Asymmetric Waveform, With Independent Modulation on EPWMxA and EPWMxB—Active High**

```
Figure 11 (AQ example) — Up-count TBCTR with AQ events

TBCNT (up-count, wraps at TBPRD)
          CB            CA                     CB            CA
          |             |                      |             |
TBPRD  ----+-------------+----------------------+-------------+------  (value)
       /                                                     /
      /                                                     /
     /                                                     /
    /                                                     /
   o-----------------------------------------------------o---------> t
   Z                                                     Z
   (wrap to 0)                                           (wrap to 0)

AQ event actions shown in the boxes:

For EPWMxA:
- Z : ↑  (set)
- P : X  (no action)
- CB: X  (no action)
- CA: ↓  (clear)

For EPWMxB:
- Z : ↑  (set)
- P : X  (no action)
- CB: ↓  (clear)
- CA: X  (no action)

Resulting outputs (relative timing):

time --->   Z      CB      CA      Z      CB      CA      Z

EPWMxA   ___|‾‾‾‾‾‾‾‾‾|_______|‾‾‾‾‾‾‾‾‾|_______|‾‾‾‾‾‾‾‾‾|___
             (set at Z)  (clear at CA)   (set at Z) (clear at CA)

EPWMxB   ___|‾‾‾‾|_____________|‾‾‾‾|_____________|‾‾‾‾|________
             (set at Z) (clear at CB)  (set at Z) (clear at CB)
```

- (1) PWM period = (TBPRD + 1 ) × TTBCLK
- (2) Duty modulation for EPWMxA is set by CMPA, and is active high (that is, high time duty proportional to CMPA).
- (3) Duty modulation for EPWMxB is set by CMPB and is active high (that is, high time duty proportional to CMPB).
- (4) The "Do Nothing" actions ( X ) are shown for completeness, but will not be shown on subsequent diagrams.
- (5) Actions at zero and period, although appearing to occur concurrently, are actually separated by one TBCLK period. TBCNT wraps from period to 0000h.

## **Table 15-21. EPWMx Initialization for [Figure](#page-38-0) 15-28**

| Register | Bit       | Value           | Comments                     |
|----------|-----------|-----------------|------------------------------|
| TBPRD    | TBPRD     | 600 (258h)      | Period = 601 TBCLK counts    |
| TBPHS    | TBPHS     | 0               | Clear Phase Register to 0    |
| TBCNT    | TBCNT     | 0               | Clear TB counter             |
| TBCTL    | CTRMODE   | TB_UP           |                              |
|          | PHSEN     | TB_DISABLE      | Phase loading disabled       |
|          | PRDLD     | TB_SHADOW       |                              |
|          | SYNCOSEL  | TB_SYNC_DISABLE |                              |
|          | HSPCLKDIV | TB_DIV1         | TBCLK = SYSCLK               |
|          | CLKDIV    | TB_DIV1         |                              |
| CMPA     | CMPA      | 350 (15Eh)      | Compare A = 350 TBCLK counts |
| CMPB     | CMPB      | 200 (C8h)       | Compare B = 200 TBCLK counts |
| CMPCTL   | SHDWAMODE | CC_SHADOW       |                              |
|          | SHDWBMODE | CC_SHADOW       |                              |
|          | LOADAMODE | CC_CTR_ZERO     | Load on CTR = 0              |
|          | LOADBMODE | CC_CTR_ZERO     | Load on CTR = 0              |
| AQCTLA   | ZRO       | AQ_SET          |                              |
|          | CAU       | AQ_CLEAR        |                              |
| AQCTLB   | ZRO       | AQ_SET          |                              |
|          | CBU       | AQ_CLEAR        |                              |

**Table 15-22. EPWMx Run Time Changes for [Figure](#page-38-0) 15-28**

| Register | Bit  | Value  | Comments                      |
|----------|------|--------|-------------------------------|
| CMPA     | CMPA | Duty1A | Adjust duty for output EPWM1A |
| CMPB     | CMPB | Duty1B | Adjust duty for output EPWM1B |

Table [15-23](#page-41-0) and Table [15-24](#page-41-1) contains initialization and runtime register configurations for the waveforms in [Figure](#page-40-0) 15-29.

**Figure 15-29. Up, Single Edge Asymmetric Waveform With Independent Modulation on EPWMxA and EPWMxB—Active Low**

```
Figure 5 (AQ example) — Up-count TBCTR with AQ events

TBCNT (up-count, wraps at TBPRD)
TBPRD  --------------------------------------------------------------  (value)
       /|                 /|                 /|
      / |                / |                / |
     /  |               /  |               /  |
    o   |              o   |              o   |---------------------> t
    P   |              P   |              P
        CB     CA          CB     CA

Event-action boxes in the figure:
- P : ↓  (clear)
- CA: ↑  (set)    (used on EPWMxA)
- CB: ↑  (set)    (used on EPWMxB)

Resulting outputs (relative timing):

time --->    P      CB      CA      P      CB      CA      P

EPWMxA    ____|______________|‾‾‾‾‾‾|______|______________|‾‾‾‾‾‾|____
          clear@P             set@CA clear@P             set@CA clear@P

EPWMxB    ____|______|‾‾‾‾‾‾‾‾‾‾‾‾‾‾|______|______|‾‾‾‾‾‾‾‾‾‾‾‾‾‾|____
          clear@P  set@CB             clear@P  set@CB             clear@P
```

- (1) PWM period = (TBPRD + 1 ) × TTBCLK
- (2) Duty modulation for EPWMxA is set by CMPA, and is active low (that is, the low time duty is proportional to CMPA).
- (3) Duty modulation for EPWMxB is set by CMPB and is active low (that is, the low time duty is proportional to CMPB).
- (4) The Do Nothing actions ( X ) are shown for completeness here, but will not be shown on subsequent diagrams.
- (5) Actions at zero and period, although appearing to occur concurrently, are actually separated by one TBCLK period. TBCNT wraps from period to 0000h.

## **Table 15-23. EPWMx Initialization for [Figure](#page-40-0) 15-29**

| Register | Bit       | Value           | Comments                     |  |  |
|----------|-----------|-----------------|------------------------------|--|--|
| TBPRD    | TBPRD     | 600 (258h)      | Period = 601 TBCLK counts    |  |  |
| TBPHS    | TBPHS     | 0               | Clear Phase Register to 0    |  |  |
| TBCNT    | TBCNT     | 0               | Clear TB counter             |  |  |
| TBCTL    | CTRMODE   | TB_UP           |                              |  |  |
|          | PHSEN     | TB_DISABLE      | Phase loading disabled       |  |  |
|          | PRDLD     | TB_SHADOW       |                              |  |  |
|          | SYNCOSEL  | TB_SYNC_DISABLE |                              |  |  |
|          | HSPCLKDIV | TB_DIV1         | TBCLK = SYSCLK               |  |  |
|          | CLKDIV    | TB_DIV1         |                              |  |  |
| CMPA     | CMPA      | 350 (15Eh)      | Compare A = 350 TBCLK counts |  |  |
| CMPB     | CMPB      | 200 (C8h)       | Compare B = 200 TBCLK counts |  |  |
| CMPCTL   | SHDWAMODE | CC_SHADOW       |                              |  |  |
|          | SHDWBMODE | CC_SHADOW       |                              |  |  |
|          | LOADAMODE | CC_CTR_ZERO     | Load on CTR = 0              |  |  |
|          | LOADBMODE | CC_CTR_ZERO     | Load on CTR = 0              |  |  |
| AQCTLA   | PRD       | AQ_CLEAR        |                              |  |  |
|          | CAU       | AQ_SET          |                              |  |  |
| AQCTLB   | PRD       | AQ_CLEAR        |                              |  |  |
|          | CBU       | AQ_SET          |                              |  |  |

### **Table 15-24. EPWMx Run Time Changes for [Figure](#page-40-0) 15-29**

| Register | Bit  | Value  | Comments                      |
|----------|------|--------|-------------------------------|
| CMPA     | CMPA | Duty1A | Adjust duty for output EPWM1A |
| CMPB     | CMPB | Duty1B | Adjust duty for output EPWM1B |

Table [15-25](#page-43-0) and Table [15-26](#page-43-1) contains initialization and runtime register configurations for the waveforms [Figure](#page-42-0) 15-30. Use the code in [Example](#page-15-0) 15-1 to define the headers.

**Figure 15-30. Up-Count, Pulse Placement Asymmetric Waveform With Independent Modulation on EPWMxA**

```
Figure 5 (AQ example) — Up-count TBCTR, EPWMxA uses CA/CB, EPWMxB uses ZT

TBCNT (up-count, wraps at TBPRD)
TBPRD  --------------------------------------------------------------  (value)
       /|                 /|                 /|
      / |                / |                / |
     /  |               /  |               /  |
    o   |              o   |              o   |---------------------> t
        CA      CB                 CA      CB

Event-action boxes shown:
- CA : ↑   (set)
- CB : ↓   (clear)
- ZT : (toggle at Zero)

Resulting outputs (relative timing):

time --->   Z(=0)   CA    CB    Z(=0)   CA    CB    Z(=0)

EPWMxA    _________|‾‾‾‾‾‾|___________ _________|‾‾‾‾‾‾|___________
                 set@CA   clear@CB               set@CA  clear@CB

EPWMxB    ___|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|___________________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
          toggle@Z(0)             toggle@Z(0)            toggle@Z(0)
```

- (1) PWM frequency = 1/( (TBPRD + 1 ) × TTBCLK )
- (2) Pulse can be placed anywhere within the PWM cycle (0000h TBPRD)
- (3) High time duty proportional to (CMPB CMPA)
- (4) EPWMxB can be used to generate a 50% duty square wave with frequency = 1/2 × ((TBPRD + 1) × TBCLK)

#### **Table 15-25. EPWMx Initialization for [Figure](#page-42-0) 15-30**

| Register | Bit       | Value           | Comments                     |  |  |
|----------|-----------|-----------------|------------------------------|--|--|
| TBPRD    | TBPRD     | 600 (258h)      | Period = 601 TBCLK counts    |  |  |
| TBPHS    | TBPHS     | 0               | Clear Phase Register to 0    |  |  |
| TBCNT    | TBCNT     | 0               | Clear TB counter             |  |  |
| TBCTL    | CTRMODE   | TB_UP           |                              |  |  |
|          | PHSEN     | TB_DISABLE      | Phase loading disabled       |  |  |
|          | PRDLD     | TB_SHADOW       |                              |  |  |
|          | SYNCOSEL  | TB_SYNC_DISABLE |                              |  |  |
|          | HSPCLKDIV | TB_DIV1         | TBCLK = SYSCLK               |  |  |
|          | CLKDIV    | TB_DIV1         |                              |  |  |
| CMPA     | CMPA      | 200 (C8h)       | Compare A = 200 TBCLK counts |  |  |
| CMPB     | CMPB      | 400 (190h)      | Compare B = 400 TBCLK counts |  |  |
| CMPCTL   | SHDWAMODE | CC_SHADOW       |                              |  |  |
|          | SHDWBMODE | CC_SHADOW       |                              |  |  |
|          | LOADAMODE | CC_CTR_ZERO     | Load on CTR = 0              |  |  |
|          | LOADBMODE | CC_CTR_ZERO     | Load on CTR = 0              |  |  |
| AQCTLA   | CAU       | AQ_SET          |                              |  |  |
|          | CBU       | AQ_CLEAR        |                              |  |  |
| AQCTLB   | ZRO       | AQ_TOGGLE       |                              |  |  |

### **Table 15-26. EPWMx Run Time Changes for [Figure](#page-42-0) 15-30**

| Register | Bit  | Value    | Comments                      |
|----------|------|----------|-------------------------------|
| CMPA     | CMPA | EdgePosA | Adjust duty for output EPWM1A |
| CMPB     | CMPB | EdgePosB | Adjust duty for output EPWM1B |

Table [15-27](#page-45-0) and Table [15-28](#page-45-1) contains initialization and runtime register configurations for the waveforms in [Figure](#page-44-0) 15-31. Use the code in [Example](#page-15-0) 15-1 to define the headers.

**Figure 15-31. Up-Down-Count, Dual Edge Symmetric Waveform, With Independent Modulation on EPWMxA and EPWMxB — Active Low**

```
Figure 5 (AQ example) — Up-Down count, EPWMxA uses CA (up/down), EPWMxB uses CB (up/down)

TBCNT (up-down triangle)
TBPRD  --------------------------------------------------------------  (value)
          / \                         / \
         /   \                       /   \
        /     \                     /     \
_______/       \___________________/       \______________---------> t
        CA↑   CA↓                  CA↑    CA↓
           CB↑ CB↓                    CB↑  CB↓

Event-action boxes shown:
- CA ↑ : set EPWMxA on CA (up-count compare)
- CA ↓ : clear EPWMxA on CA (down-count compare)
- CB ↑ : set EPWMxB on CB (up-count compare)
- CB ↓ : clear EPWMxB on CB (down-count compare)

Resulting outputs (relative timing):

time --->      CA↑     CB↑   CB↓    CA↓           CA↑    CB↑   CB↓    CA↓

EPWMxA      _________|‾‾‾‾‾‾‾‾‾‾‾|______________ _______|‾‾‾‾‾‾‾‾‾‾‾|_____
                 set@CA↑       clr@CA↓                set@CA↑      clr@CA↓

EPWMxB      ________________|‾‾‾‾|___________________ ____________|‾‾‾‾|____
                      set@CB↑ clr@CB↓                         set@CB↑ clr@CB↓
```

- (1) PWM period = 2 x TBPRD × TTBCLK
- (2) Duty modulation for EPWMxA is set by CMPA, and is active low (that is, the low time duty is proportional to CMPA).
- (3) Duty modulation for EPWMxB is set by CMPB and is active low (that is, the low time duty is proportional to CMPB).
- (4) Outputs EPWMxA and EPWMxB can drive independent power switches

#### **Table 15-27. EPWMx Initialization for [Figure](#page-44-0) 15-31**

| Register | Bit       | Value           | Comments                     |  |  |
|----------|-----------|-----------------|------------------------------|--|--|
| TBPRD    | TBPRD     | 600 (258h)      | Period = 601 TBCLK counts    |  |  |
| TBPHS    | TBPHS     | 0               | Clear Phase Register to 0    |  |  |
| TBCNT    | TBCNT     | 0               | Clear TB counter             |  |  |
| TBCTL    | CTRMODE   | TB_UPDOWN       |                              |  |  |
|          | PHSEN     | TB_DISABLE      | Phase loading disabled       |  |  |
|          | PRDLD     | TB_SHADOW       |                              |  |  |
|          | SYNCOSEL  | TB_SYNC_DISABLE |                              |  |  |
|          | HSPCLKDIV | TB_DIV1         | TBCLK = SYSCLK               |  |  |
|          | CLKDIV    | TB_DIV1         |                              |  |  |
| CMPA     | CMPA      | 400 (190h)      | Compare A = 400 TBCLK counts |  |  |
| CMPB     | CMPB      | 500 (1F4h)      | Compare B = 500 TBCLK counts |  |  |
| CMPCTL   | SHDWAMODE | CC_SHADOW       |                              |  |  |
|          | SHDWBMODE | CC_SHADOW       |                              |  |  |
|          | LOADAMODE | CC_CTR_ZERO     | Load on CTR = 0              |  |  |
|          | LOADBMODE | CC_CTR_ZERO     | Load on CTR = 0              |  |  |
| AQCTLA   | CAU       | AQ_SET          |                              |  |  |
|          | CAD       | AQ_CLEAR        |                              |  |  |
| AQCTLB   | CBU       | AQ_SET          |                              |  |  |
|          | CBD       | AQ_CLEAR        |                              |  |  |

### **Table 15-28. EPWMx Run Time Changes for [Figure](#page-44-0) 15-31**

| Register | Bit  | Value  | Comments                      |
|----------|------|--------|-------------------------------|
| CMPA     | CMPA | Duty1A | Adjust duty for output EPWM1A |
| CMPB     | CMPB | Duty1B | Adjust duty for output EPWM1B |

Table [15-29](#page-47-0) and Table [15-30](#page-47-1) contains initialization and runtime register configurations for the waveforms in [Figure](#page-46-0) 15-32. Use the code in [Example](#page-15-0) 15-1 to define the headers.

**Figure 15-32. Up-Down-Count, Dual Edge Symmetric Waveform, With Independent Modulation on EPWMxA and EPWMxB — Complementary**

```
Figure 5 (AQ example) — Up-Down count, EPWMxA uses CA (up/down), EPWMxB uses CB (down/up)

TBCNT (up-down triangle)
TBPRD  --------------------------------------------------------------  (value)
          / \                         / \
         /   \                       /   \
        /     \                     /     \
_______/       \___________________/       \______________---------> t
        CA↑   CA↓                  CA↑    CA↓
           CB↓ CB↑                    CB↓  CB↑

Event-action boxes shown:
- CA ↑ : set EPWMxA on CA (up-count compare)
- CA ↓ : clear EPWMxA on CA (down-count compare)
- CB ↓ : clear EPWMxB on CB (down-count compare)
- CB ↑ : set EPWMxB on CB (up-count compare)

Resulting outputs (relative timing):

time --->      CA↑      CA↓                CA↑      CA↓
EPWMxA      _________|‾‾‾‾‾‾‾‾‾‾‾|________ _______|‾‾‾‾‾‾‾‾‾‾‾|____
                 set@CA↑    clr@CA↓            set@CA↑    clr@CA↓

time --->          CB↓  CB↑                       CB↓  CB↑
EPWMxB      __________________|____|‾‾‾‾‾‾‾‾‾‾‾‾‾|____|‾‾‾‾‾‾‾‾
                     clr@CB↓ set@CB↑                 clr@CB↓ set@CB↑
```

- (1) PWM period = 2 × TBPRD × TTBCLK
- (2) Duty modulation for EPWMxA is set by CMPA, and is active low, i.e., low time duty proportional to CMPA
- (3) Duty modulation for EPWMxB is set by CMPB and is active high, i.e., high time duty proportional to CMPB
- (4) Outputs EPWMx can drive upper/lower (complementary) power switches
- (5) Dead-band = CMPB CMPA (fully programmable edge placement by software). Note the dead-band module is also available if the more classical edge delay method is required.

#### **Table 15-29. EPWMx Initialization for [Figure](#page-46-0) 15-32**

| Register | Bit       | Value           | Comments                     |
|----------|-----------|-----------------|------------------------------|
| TBPRD    | TBPRD     | 600 (258h)      | Period = 601 TBCLK counts    |
| TBPHS    | TBPHS     | 0               | Clear Phase Register to 0    |
| TBCNT    | TBCNT     | 0               | Clear TB counter             |
| TBCTL    | CTRMODE   | TB_UPDOWN       |                              |
|          | PHSEN     | TB_DISABLE      | Phase loading disabled       |
|          | PRDLD     | TB_SHADOW       |                              |
|          | SYNCOSEL  | TB_SYNC_DISABLE |                              |
|          | HSPCLKDIV | TB_DIV1         | TBCLK = SYSCLK               |
|          | CLKDIV    | TB_DIV1         |                              |
| CMPA     | CMPA      | 350 (15Eh)      | Compare A = 350 TBCLK counts |
| CMPB     | CMPB      | 400 (190h)      | Compare B = 400 TBCLK counts |
| CMPCTL   | SHDWAMODE | CC_SHADOW       |                              |
|          | SHDWBMODE | CC_SHADOW       |                              |
|          | LOADAMODE | CC_CTR_ZERO     | Load on CTR = 0              |
|          | LOADBMODE | CC_CTR_ZERO     | Load on CTR = 0              |
| AQCTLA   | CAU       | AQ_SET          |                              |
|          | CAD       | AQ_CLEAR        |                              |
| AQCTLB   | CBU       | AQ_CLEAR        |                              |
|          | CBD       | AQ_SET          |                              |

### **Table 15-30. EPWMx Run Time Changes for [Figure](#page-46-0) 15-32**

| Register | Bit  | Value  | Comments                      |
|----------|------|--------|-------------------------------|
| CMPA     | CMPA | Duty1A | Adjust duty for output EPWM1A |
| CMPB     | CMPB | Duty1B | Adjust duty for output EPWM1B |

Table [15-31](#page-49-0) and Table [15-32](#page-49-1) contains initialization and runtime register configurations for the waveforms in [Figure](#page-48-0) 15-33. Use the code in [Example](#page-15-0) 15-1 to define the headers.

**Figure 15-33. Up-Down-Count, Dual Edge Asymmetric Waveform, With Independent Modulation on EPWMxA—Active Low**

```
Figure 5 (AQ example) — Up-Down count with CA/CB actions (EPWMxA) and Z/P actions (EPWMxB)

TBCNT (up-down triangle; events marked by dashed lines)
        / \                        / \
       /   \                      /   \
______/     \____________________/     \__________________________-> t
   Z      CA↑    P        CB↓       Z      CA↑     P       CB↓

Event-action boxes shown:
EPWMxA:
- CA ↑ : set EPWMxA
- CB ↓ : clear EPWMxA

EPWMxB:
- Z ↓ : clear EPWMxB (at zero)
- P ↑ : set EPWMxB (at period)

Resulting outputs (relative timing):

time --->   Z     CA↑     P      CB↓     Z     CA↑     P      CB↓

EPWMxA    _______|‾‾‾‾‾‾‾‾‾|__________ _______|‾‾‾‾‾‾‾‾‾|__________
             set@CA↑  clr@CB↓             set@CA↑  clr@CB↓

EPWMxB    __|______________|‾‾‾‾‾‾‾‾‾‾‾|__|______________|‾‾‾‾‾‾‾‾‾
          clr@Z            set@P          clr@Z            set@P
```

- (1) PWM period = 2 × TBPRD × TBCLK
- (2) Rising edge and falling edge can be asymmetrically positioned within a PWM cycle. This allows for pulse placement techniques.
- (3) Duty modulation for EPWMxA is set by CMPA and CMPB.
- (4) Low time duty for EPWMxA is proportional to (CMPA + CMPB).
- (5) To change this example to active high, CMPA and CMPB actions need to be inverted (i.e., Set ! Clear and Clear Set).
- (6) Duty modulation for EPWMxB is fixed at 50% (utilizes spare action resources for EPWMxB)

## **Table 15-31. EPWMx Initialization for [Figure](#page-48-0) 15-33**

| Register | Bit       | Value           | Comments                     |  |  |
|----------|-----------|-----------------|------------------------------|--|--|
| TBPRD    | TBPRD     | 600 (258h)      | Period = 601 TBCLK counts    |  |  |
| TBPHS    | TBPHS     | 0               | Clear Phase Register to 0    |  |  |
| TBCNT    | TBCNT     | 0               | Clear TB counter             |  |  |
| TBCTL    | CTRMODE   | TB_UPDOWN       |                              |  |  |
|          | PHSEN     | TB_DISABLE      | Phase loading disabled       |  |  |
|          | PRDLD     | TB_SHADOW       |                              |  |  |
|          | SYNCOSEL  | TB_SYNC_DISABLE |                              |  |  |
|          | HSPCLKDIV | TB_DIV1         | TBCLK = SYSCLK               |  |  |
|          | CLKDIV    | TB_DIV1         |                              |  |  |
| CMPA     | CMPA      | 250 (FAh)       | Compare A = 250 TBCLK counts |  |  |
| CMPB     | CMPB      | 450 (1C2h)      | Compare B = 450 TBCLK counts |  |  |
| CMPCTL   | SHDWAMODE | CC_SHADOW       |                              |  |  |
|          | SHDWBMODE | CC_SHADOW       |                              |  |  |
|          | LOADAMODE | CC_CTR_ZERO     | Load on CTR = 0              |  |  |
|          | LOADBMODE | CC_CTR_ZERO     | Load on CTR = 0              |  |  |
| AQCTLA   | CAU       | AQ_SET          |                              |  |  |
|          | CBD       | AQ_CLEAR        |                              |  |  |
| AQCTLB   | ZRO       | AQ_CLEAR        |                              |  |  |
|          | PRD       | AQ_SET          |                              |  |  |

### **Table 15-32. EPWMx Run Time Changes for [Figure](#page-48-0) 15-33**

| Register | Bit  | Value    | Comments                      |
|----------|------|----------|-------------------------------|
| CMPA     | CMPA | EdgePosA | Adjust duty for output EPWM1A |
| CMPB     | CMPB | EdgePosB |                               |

#### 15.2.2.6 Dead-Band Generator (DB) Submodule

Figure 15-34 illustrates the dead-band generator submodule within the ePWM module.

Figure 15-34. Dead-Band Generator Submodule

```
EPWMxSYNCI  ---> +------------------+ <--- EPWMxSYNCO
                 |  Time-Base (TB)  |
                 +------------------+
                      |   |   |
                      |   |   +--> CTR_Dir ------------------------------+
                      |   +-------> CTR = 0 -------------------------+   |
                      +-----------> CTR = PRD ---------------------+ |   |
                                                                      |   |
                 +-----------------------+                            |   |
                 | Counter Compare (CC)  |                            |   |
                 +-----------------------+                            |   |
                      |            |                                  |   |
                      |            +--> CTR = CMPB ----------------+  |   |
                      +--> CTR = CMPA --------------------------+  |  |   |
                                                                   v  v   v
                                                           +----------------------+
                                                           | Action Qualifier (AQ)|
                                                           +----------------------+
                                                               |            |
                                                               |            |
                                                           EPWMxA         EPWMxB
                                                               |            |
                                                               v            v
                                                        +------------------------+
                                                        | Dead-Band (DB)         |
                                                        | (optional / dashed)    |
                                                        +------------------------+
                                                                 |
                                                                 v
                                                             +-------------+
                                                             |    PC       |  PWM-chopper
                                                             +-------------+
                                                                 |
                                                                 v
                                                             +-------------+  <--- TZ1 to TZn
                                                             |    TZ       |       (Trip inputs)
                                                             | Trip Zone   |
                                                             +-------------+
                                                               |       |
                                                               |       +----> EPWMxB -----> +----------+
                                                               +----------> EPWMxA ----->  | GPIO MUX  |
                                                                                          +----------+

Event/interrupt generation:
 CTR = PRD  --------------------+
 CTR = 0    --------------------+
 CTR = CMPA --------------------+----> +------------------------------+ ---> EPWMxINT ---> +---------------------+
 CTR = CMPB --------------------+      | Event Trigger & Interrupt (ET)|                  | Interrupt controller |
 CTR_Dir   ---------------------+      +------------------------------+                  +---------------------+

Trip-zone interrupt:
 CTR = 0 -------------------------------> (into TZ logic)
 EPWMxTZINT ----------------------------> +---------------------+
                                         | Interrupt controller |
                                         +---------------------+
```

#### 15.2.2.6.1 Purpose of the Dead-Band Submodule

The "Action-qualifier (AQ) Module" section discussed how it is possible to generate the required deadband by having full control over edge placement using both the CMPA and CMPB resources of the ePWM module. However, if the more classical edge delay-based dead-band with polarity control is required, then the dead-band generator submodule should be used.

The key functions of the dead-band generator submodule are:

- Generating appropriate signal pairs (EPWMxA and EPWMxB) with dead-band relationship from a single EPWMxA input
- Programming signal pairs for:
  - Active high (AH)
  - Active low (AL)
  - Active high complementary (AHC)
  - Active low complementary (ALC)
- Adding programmable delay to rising edges (RED)
- Adding programmable delay to falling edges (FED)
- Can be totally bypassed from the signal path (note dotted lines in diagram)

#### 15.2.2.6.2 Controlling and Monitoring the Dead-Band Submodule

The dead-band generator submodule operation is controlled and monitored via the following registers:

Table 15-33. Dead-Band Generator Submodule Registers

| Acronym | Register Description                        | Address Offset | Shadowed |
|---------|---------------------------------------------|----------------|----------|
| DBCTL   | Dead-Band Control Register                  | 1Eh            | No       |
| DBRED   | Dead-Band Rising Edge Delay Count Register  | 20h            | No       |
| DBFED   | Dead-Band Falling Edge Delay Count Register | 22h            | No       |

#### *15.2.2.6.3 Operational Highlights for the Dead-Band Generator Submodule*

The following sections provide the operational highlights.

The dead-band submodule has two groups of independent selection options as shown in [Figure](#page-51-0) 15-35.

- **Input Source Selection:** The input signals to the dead-band module are the EPWMxA and EPWMxB output signals from the action-qualifier. In this section they will be referred to as EPWMxA In and EPWMxB In. Using the DBCTL[IN\_MODE) control bits, the signal source for each delay, falling-edge or rising-edge, can be selected:
  - EPWMxA In is the source for both falling-edge and rising-edge delay. This is the default mode.
  - EPWMxA In is the source for falling-edge delay, EPWMxB In is the source for rising-edge delay.
  - EPWMxA In is the source for rising edge delay, EPWMxB In is the source for falling-edge delay.
  - EPWMxB In is the source for both falling-edge and rising-edge delay.
- **Output Mode Control:** The output mode is configured by way of the DBCTL[OUT\_MODE] bits. These bits determine if the falling-edge delay, rising-edge delay, neither, or both are applied to the input signals.
- **Polarity Control:** The polarity control (DBCTL[POLSEL]) allows you to specify whether the rising-edge delayed signal and/or the falling-edge delayed signal is to be inverted before being sent out of the dead-band submodule.

0 1 S2 1 0 S1 RED In Out Rising edge delay (10-bit counter) (10-bit counter) delay Falling edge In Out FED 1 0 S3 0 S0 1 EPWMxA EPWMxB DBCTL[POLSEL] DBCTL[OUT\_MODE] S5 DBCTL[IN\_MODE] 1 0 S4 0 1 EPWMxA in

**Figure 15-35. Configuration Options for the Dead-Band Generator Submodule**

EPWMxB in

Although all combinations are supported, not all are typical usage modes. Table [15-34](#page-52-0) lists some classical dead-band configurations. These modes assume that the DBCTL[IN\_MODE] is configured such that EPWMxA In is the source for both falling-edge and rising-edge delay. Enhanced, or non-traditional modes can be achieved by changing the input signal source. The modes shown in Table [15-34](#page-52-0) fall into the following categories:

- **Mode 1: Bypass both falling-edge delay (FED) and rising-edge delay (RED)** Allows you to fully disable the dead-band submodule from the PWM signal path.
- **Mode 2-5: Classical Dead-Band Polarity Settings** These represent typical polarity configurations that should address all the active high/low modes required by available industry power switch gate drivers. The waveforms for these typical cases are shown in [Figure](#page-53-0) 15-36. Note that to generate equivalent waveforms to [Figure](#page-53-0) 15-36, configure the action-qualifier submodule to generate the signal as shown for EPWMxA.
- **Mode 6: Bypass rising-edge-delay and Mode 7: Bypass falling-edge-delay** Finally the last two entries in Table [15-34](#page-52-0) show combinations where either the falling-edge-delay (FED) or rising-edgedelay (RED) blocks are bypassed.

**Table 15-34. Classical Dead-Band Operating Modes**

|      |                                                |        | DBCTL[POLSEL] |    | DBCTL[OUT_MODE] |
|------|------------------------------------------------|--------|---------------|----|-----------------|
| Mode | Mode Description (1)                           | S3     | S2            | S1 | S0              |
| 1    | EPWMxA and EPWMxB Passed Through (No Delay)    | x      | x             | 0  | 0               |
| 2    | Active High Complementary (AHC)                | 1      | 0             | 1  | 1               |
| 3    | Active Low Complementary (ALC)                 | 0      | 1             | 1  | 1               |
| 4    | Active High (AH)                               | 0      | 0             | 1  | 1               |
| 5    | Active Low (AL)                                | 1      | 1             | 1  | 1               |
| 6    | EPWMxA Out = EPWMxA In (No Delay)              | 0 or 1 | 0 or 1        |    | 1               |
|      | EPWMxB Out = EPWMxA In with Falling Edge Delay |        |               | 0  |                 |
| 7    | EPWMxA Out = EPWMxA In with Rising Edge Delay  | 0 or 1 | 0 or 1        | 1  | 0               |
|      | EPWMxB Out = EPWMxB In with No Delay           |        |               |    |                 |

<sup>(1)</sup> These are classical dead-band modes and assume that DBCTL[IN\_MODE] = 0,0. That is, EPWMxA in is the source for both the falling-edge and rising-edge delays. Enhanced, non-traditional modes can be achieved by changing the IN\_MODE configuration.

[Figure](#page-53-0) 15-36 shows waveforms for typical cases where 0% < duty < 100%.

**Figure 15-36. Dead-Band Waveforms for Typical Cases (0% < Duty < 100%)**

```
Figure 5 — Dead-band / complementary outputs (one period shown)

Time markers:
|---- period --------------------------------------------------------------->|
t0      tR           tF             t1
        ^            ^
        |            |
        |<-- RED --> |<-- FED -->|

Original (outA)
_____┌──────────────────────────┐___________________________________________
     │                          │
     └──────────────────────────┘

Rising Edge Delayed (RED)
_____┌──────────────────────────┐___________________________________________
     └─(rise shifted right by RED)─┘

Falling Edge Delayed (FED)
_____┌──────────────────────────────────────┐_______________________________
     │                                      │
     └────(fall shifted right by FED)───────┘

Active High Complementary (AHC)
_____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|_____________________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
      (complementary, active-high behavior with dead-band window)

Active Low Complementary (ALC)
_____┌──────────────────────────┐___________________________________________
     │                          │
_____|                          |____________________________________________
      (complementary, active-low behavior with dead-band window)

Active High (AH)
_____┌──────────────────────────┐___________________________________________
     │                          │
     └──────────────────────────┘

Active Low (AL)
_____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|_____________________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
```

The dead-band submodule supports independent values for rising-edge (RED) and falling-edge (FED) delays. The amount of delay is programmed using the DBRED and DBFED registers. These are 10-bit registers and their value represents the number of time-base clock, TBCLK, periods a signal edge is delayed by. For example, the formula to calculate falling-edge-delay and rising-edge-delay are:

FED = DBFED × TTBCLK

RED = DBRED × TTBCLK

Where TTBCLK is the period of TBCLK, the prescaled version of SYSCLKOUT.

#### 15.2.2.7 PWM-Chopper (PC) Submodule

Figure 15-37 illustrates the PWM-chopper (PC) submodule within the ePWM module. The PWM-chopper submodule allows a high-frequency carrier signal to modulate the PWM waveform generated by the action-qualifier and dead-band submodules. This capability is important if you need pulse transformer-based gate drivers to control the power switching elements.

> *[Diagram — signal/block labels not renderable in text format]*

Figure 15-37. PWM-Chopper Submodule

#### 15.2.2.7.1 Purpose of the PWM-Chopper Submodule

The key functions of the PWM-chopper submodule are:

- Programmable chopping (carrier) frequency
- Programmable pulse width of first pulse
- Programmable duty cycle of second and subsequent pulses
- Can be fully bypassed if not required

#### 15.2.2.7.2 Controlling the PWM-Chopper Submodule

The PWM-chopper submodule operation is controlled via the register in Table 15-35.

Table 15-35. PWM-Chopper Submodule Registers

| Acronym | Register Description         | Address Offset | Shadowed |
|---------|------------------------------|----------------|----------|
| PCCTL   | PWM-chopper Control Register | 3Ch            | No       |

#### *15.2.2.7.3 Operational Highlights for the PWM-Chopper Submodule*

[Figure](#page-55-0) 15-38 shows the operational details of the PWM-chopper submodule. The carrier clock is derived from SYSCLKOUT. Its frequency and duty cycle are controlled via the CHPFREQ and CHPDUTY bits in the PCCTL register. The one-shot block is a feature that provides a high energy first pulse to ensure hard and fast power switch turn on, while the subsequent pulses sustain pulses, ensuring the power switch remains on. The one-shot width is programmed via the OSHTWTH bits. The PWM-chopper submodule can be fully disabled (bypassed) via the CHPEN bit.

```
Figure 5 — PWM Chopper (PC) block diagram (text)

SYSCLKOUT ---> [/8] ----------------------------------------------+
                                                                 |
                                                                 v
                      +-----------------------------------+   +--------------------------+
EPWMxA -------------->| Start                             |   | Divider and duty control |
                      |        One shot                   |<--|  PCCTL[CHPFREQ]          |
                      | Clk                         OSHT  |   |  PCCTL[CHPDUTY]          |
                      | Pulse-width <--- PCCTL[OSHTWTH]   |   +--------------------------+
                      +-----------------------------------+                |
                                 |                                         | PSCLK
                                 v                                         v
                           +-----------+                             (feeds chopper gating)
                           |    OR     |<-------------------------------+
                           +-----------+                                |
                                 |                                      |
                                 v                                      |
                           +-----------+                                |
Bypass ------------------> |   AND     |----> PWMA_ch ------------------+
                           +-----------+

                                     +-------------------+
                                     |   MUX (Bypass)    |<---- PCCTL[CHPEN]
                                     |   0: Bypass       |
PWMA_ch ---------------------------->|   1: Chopped      |----> EPWMxA
                                     +-------------------+

                      +-----------------------------------+
EPWMxB -------------->| Clk                               |
                      | Pulse-width                       |
                      |        One shot              OSHT |
                      | Start       <--- PCCTL[OSHTWTH]   |
                      +-----------------------------------+
                                 |
                                 v
                           +-----------+
                           |    OR     |<-------------------------------+
                           +-----------+                                |
                                 |                                      |
                                 v                                      |
                           +-----------+                                |
Bypass ------------------> |   AND     |----> PWMB_ch ------------------+
                           +-----------+

                                     +-------------------+
                                     |   MUX (Bypass)    |<---- PCCTL[CHPEN]
                                     |   1: Chopped      |
PWMB_ch ---------------------------->|   0: Bypass       |----> EPWMxB
                                     +-------------------+
```

**Figure 15-38. PWM-Chopper Submodule Signals and Registers**

#### *15.2.2.7.4 Waveforms*

[Figure](#page-56-0) 15-39 shows simplified waveforms of the chopping action only; one-shot and duty-cycle control are not shown. Details of the one-shot and duty-cycle control are discussed in the following sections.

**Figure 15-39. Simple PWM-Chopper Submodule Waveforms Showing Chopping Action Only**

```
Figure 6 — PWM chopper example waveforms (conceptual)

time ---> |------ A high window ------|--gap--|------ A high window ------|--gap--|

EPWMxA (original)
        ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|______|‾‾‾‾‾‾‾‾‾‾‾‾‾‾|____________________

EPWMxB (original)
        ‾‾‾‾|__________________|‾‾‾‾‾‾|_____________________|‾‾‾‾‾‾‾‾‾‾‾‾‾

PSCLK (chopper clock)
        |_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_ ...

EPWMxA (after chopper)
        ______|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|___________|_|‾|_|‾|_|‾|_|‾|______
               (PSCLK pulses only while EPWMxA window is active)

EPWMxB (after chopper)
        |_|‾|_ ____________________ |_|‾|_|‾|_ __________________ |_|‾|_|‾|_
        (PSCLK pulses only inside the shown EPWMxB active windows)
```
#### *15.2.2.7.4.1 One-Shot Pulse*

The width of the first pulse can be programmed to any of 16 possible pulse width values. The width or period of the first pulse is given by:

$$T_{1stpulse} = T_{SYSCLKOUT} \times 8 \times OSHTWTH$$

Where TSYSCLKOUT is the period of the system clock (SYSCLKOUT) and OSHTWTH is the four control bits (value from 1 to 16)

[Figure](#page-56-1) 15-40 shows the first and subsequent sustaining pulses.

**Figure 15-40. PWM-Chopper Submodule Waveforms Showing the First Pulse and Subsequent Sustaining Pulses**

```
Figure 13 — One-shot (OSHT) pulse and sustaining chopper pulses

time ---> |------------------------------|------------------------------|

EPWMxA in
        ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|__________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾|____________
             ^ start OSHT pulse               ^ start OSHT pulse

        |_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_ ...

        ________|‾‾|___________________________|‾‾|____________________
                <--- prog. pulse width (OSHTWTH) --->

EPWMxA out
        ____|‾|_|‾|_|‾|_|‾|_|‾|_|‾|____________|‾|_|‾|_|‾|_|‾|_________
              \________ sustaining pulses ________/ \__ sustaining __/
```

#### *15.2.2.7.4.2 Duty Cycle Control*

Pulse transformer-based gate drive designs need to comprehend the magnetic properties or characteristics of the transformer and associated circuitry. Saturation is one such consideration. To assist the gate drive designer, the duty cycles of the second and subsequent pulses have been made programmable. These sustaining pulses ensure the correct drive strength and polarity is maintained on the power switch gate during the on period, and hence a programmable duty cycle allows a design to be tuned or optimized via software control.

[Figure](#page-57-0) 15-41 shows the duty cycle control that is possible by programming the CHPDUTY bits. One of seven possible duty ratios can be selected ranging from 12.5% to 87.5%.

**Figure 15-41. PWM-Chopper Submodule Waveforms Showing the Pulse Width (Duty Cycle) Control of Sustaining Pulses**

```
PSCLK and chopper duty examples (1/8 … 7/8)

PSCLK (reference clock)
... |_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_ ...
        <----- one PSCLK period ----->

Within one PSCLK Period, the duty can be programmed in 1/8 steps.
(Vertical guide labels in the figure: 12.5%, 25%, 37.5%, 50%, 62.5%, 75%, 87.5%)

time ---> |-------------------- PSCLK Period --------------------|

Duty 1/8   ____|‾|____________________________________________________|‾|____
Duty 2/8   ____|‾‾|___________________________________________________|‾‾|___
Duty 3/8   ____|‾‾‾|__________________________________________________|‾‾‾|__
Duty 4/8   ____|‾‾‾‾|_________________________________________________|‾‾‾‾|_
Duty 5/8   ____|‾‾‾‾‾|________________________________________________|‾‾‾‾‾|
Duty 6/8   ____|‾‾‾‾‾‾|_______________________________________________|‾‾‾‾‾‾|
Duty 7/8   ____|‾‾‾‾‾‾‾|______________________________________________|‾‾‾‾‾‾‾|
```
#### 15.2.2.8 Trip-Zone (TZ) Submodule

Figure 15-42 shows how the trip-zone (TZ) submodule fits within the ePWM module. Each ePWM module is connected to every  $\overline{TZ}$  signal that are sourced from the GPIO MUX. These signals indicates external fault or trip conditions, and the ePWM outputs can be programmed to respond accordingly when faults occur. See Section 15.1.2 to determine the number of trip-zone pins available for the device.

CTR = PRD Event CTR = 0Trigger Action Interrupt **EPWMxINT** CTR = CMPA and Qualifier controller **EPWMxSYNCI** CTR = PRD <sup>⊥</sup> (AQ) Interrupt CTR = CMPB Time-Base CTR =  $0^{1}$ (ET) CTR\_Dir **FPWMxSYNCO** (TB) CTR Dir **EPWMxA** EPWMxA H Dead PWM-Trip Band chopper Zone CTR = CMPA <sup>⊥</sup> (DB) **GPIO** (PC) (TZ) Counter **EPWMxB** MUX Compare **EPWMxB** CTR = CMPB (CC) TZ1 to TZn CTR = 0**EPWMxTZINT** Interrupt controller

Figure 15-42. Trip-Zone Submodule

#### 15.2.2.8.1 Purpose of the Trip-Zone Submodule

The key functions of the trip-zone submodule are:

- Trip inputs TZ1 to TZn can be flexibly mapped to any ePWM module.
- Upon a fault condition, outputs EPWMxA and EPWMxB can be forced to one of the following:
  - High
  - Low
  - High-impedance
  - No action taken
- Support for one-shot trip (OSHT) for major short circuits or over-current conditions.
- Support for cycle-by-cycle tripping (CBC) for current limiting operation.
- Each trip-zone input pin can be allocated to either one-shot or cycle-by-cycle operation.
- Interrupt generation is possible on any trip-zone pin.
- Software-forced tripping is also supported.
- The trip-zone submodule can be fully bypassed if it is not required.

#### *15.2.2.8.2 Controlling and Monitoring the Trip-Zone Submodule*

The trip-zone submodule operation is controlled and monitored through the following registers:

**Table 15-36. Trip-Zone Submodule Registers**

| Acronym | Register Description                | Address Offset | Shadowed |
|---------|-------------------------------------|----------------|----------|
| TZSEL   | Trip-Zone Select Register           | 24h            | No       |
| TZCTL   | Trip-Zone Control Register          | 28h            | No       |
| TZEINT  | Trip-Zone Enable Interrupt Register | 2Ah            | No       |
| TZFLG   | Trip-Zone Flag Register             | 2Ch            | No       |
| TZCLR   | Trip-Zone Clear Register            | 2Eh            | No       |
| TZFRC   | Trip-Zone Force Register            | 30h            | No       |

#### *15.2.2.8.3 Operational Highlights for the Trip-Zone Submodule*

The following sections describe the operational highlights and configuration options for the trip-zone submodule.

The trip-zone signals at pin TZ1 to TZn is an active-low input signal. When the pin goes low, it indicates that a trip event has occurred. Each ePWM module can be individually configured to ignore or use each of the trip-zone pins. Which trip-zone pins are used by a particular ePWM module is determined by the TZSEL register for that specific ePWM module. The trip-zone signal may or may not be synchronized to the system clock (SYSCLKOUT). A minimum of 1 SYSCLKOUT low pulse on the TZ n inputs is sufficient to trigger a fault condition in the ePWM module. The asynchronous trip makes sure that if clocks are missing for any reason, the outputs can still be tripped by a valid event present on the TZn inputs.

The TZ n input can be individually configured to provide either a cycle-by-cycle or one-shot trip event for a ePWM module. The configuration is determined by the TZSEL[CBCn] and TZSEL[OSHTn] bits (where n corresponds to the trip pin) respectively.

- **Cycle-by-Cycle (CBC):** When a cycle-by-cycle trip event occurs, the action specified in the TZCTL register is carried out immediately on the EPWMxA and/or EPWMxB output. Table [15-37](#page-60-0) lists the possible actions. In addition, the cycle-by-cycle trip event flag (TZFLG[CBC]) is set and a EPWMxTZINT interrupt is generated if it is enabled in the TZEINT register.
  - The specified condition on the pins is automatically cleared when the ePWM time-base counter reaches zero (TBCNT = 0000h) if the trip event is no longer present. Therefore, in this mode, the trip event is cleared or reset every PWM cycle. The TZFLG[CBC] flag bit will remain set until it is manually cleared by writing to the TZCLR[CBC] bit. If the cycle-by-cycle trip event is still present when the TZFLG[CBC] bit is cleared, then it will again be immediately set.
- **One-Shot (OSHT):** When a one-shot trip event occurs, the action specified in the TZCTL register is carried out immediately on the EPWMxA and/or EPWMxB output. Table [15-37](#page-60-0) lists the possible actions. In addition, the one-shot trip event flag (TZFLG[OST]) is set and a EPWMxTZINT interrupt is generated if it is enabled in the TZEINT register. The one-shot trip condition must be cleared manually by writing to the TZCLR[OST] bit.

The action taken when a trip event occurs can be configured individually for each of the ePWM output pins by way of the TZCTL[TZA] and TZCTL[TZB] register bits. One of four possible actions, shown in Table [15-37,](#page-60-0) can be taken on a trip event.

## **Table 15-37. Possible Actions On a Trip Event**

| TZCTL[TZA]<br>and/or<br>TZCTL[TZB] | EPWMxA<br>and/or<br>EPWMxB | Comment                                      |
|------------------------------------|----------------------------|----------------------------------------------|
| 0                                  | High-Impedance             | Tripped                                      |
| 1h                                 | Force to High State        | Tripped                                      |
| 2h                                 | Force to Low State         | Tripped                                      |
| 3h                                 | No Change                  | Do Nothing. No change is made to the output. |

#### *Example 15-2. Trip-Zone Configurations*

#### **Scenario A:**

A one-shot trip event on TZ1 pulls both EPWM1A, EPWM1B low and also forces EPWM2A and EPWM2B high.

- Configure the ePWM1 registers as follows:
  - TZSEL[OSHT1] = 1: enables TZ as a one-shot event source for ePWM1
  - TZCTL[TZA] = 2: EPWM1A will be forced low on a trip event.
  - TZCTL[TZB] = 2: EPWM1B will be forced low on a trip event.
- Configure the ePWM2 registers as follows:
  - TZSEL[OSHT1] = 1: enables TZ as a one-shot event source for ePWM2
  - TZCTL[TZA] = 1: EPWM2A will be forced high on a trip event.
  - TZCTL[TZB] = 1: EPWM2B will be forced high on a trip event.

#### **Scenario B:**

A cycle-by-cycle event on TZ5 pulls both EPWM1A, EPWM1B low.

A one-shot event on TZ1 or TZ6 puts EPWM2A into a high impedance state.

- Configure the ePWM1 registers as follows:
  - TZSEL[CBC5] = 1: enables TZ5 as a one-shot event source for ePWM1
  - TZCTL[TZA] = 2: EPWM1A will be forced low on a trip event.
  - TZCTL[TZB] = 2: EPWM1B will be forced low on a trip event.
- Configure the ePWM2 registers as follows:
  - TZSEL[OSHT1] = 1: enables TZ1 as a one-shot event source for ePWM2
  - TZSEL[OSHT6] = 1: enables TZ6 as a one-shot event source for ePWM1
  - TZCTL[TZA] = 0: EPWM1A will be put into a high-impedance state on a trip event.
  - TZCTL[TZB] = 3: EPWM1B will ignore the trip event.

#### *15.2.2.8.4 Generating Trip Event Interrupts*

[Figure](#page-61-0) 15-43 and [Figure](#page-61-1) 15-44 illustrate the trip-zone submodule control and interrupt logic, respectively.

> *[Diagram — signal/block labels not renderable in text format]*

Figure 15-44. Trip-Zone Submodule Interrupt Logic

```
Figure 5 — Trip-zone interrupt (TZINT) logic (CBC/OST sources)

                 +----------------------+
TZCLR[INT] ----->| Clear / Latch / Set  |----> TZFLG[INT]
                 +----------------------+
                           ^
                           |
EPWMxTZINT (to Interrupt controller) <--- +-------------------------------+
                                          | Generate interrupt pulse      |
                                          | when input = 1                |
                                          +-------------------------------+
                                                     ^
                                                     |
                                                     +----( OR )----+
                                                     |              |
                                                     |              |
                                             TZEINT[CBC]        TZEINT[OST]
                                                (enable)           (enable)
                                                     |              |
                                                     o--(gate)--+   o--(gate)--+
                                                                |               |
                                                                v               v
                 +----------------------+                +----------------------+
TZCLR[CBC] ----->| Clear / Latch / Set  |----> TZFLG[CBC]| Clear / Latch / Set  |<----- TZCLR[OST]
CBC trip event ->| (CBC flag latch)     |                | (OST flag latch)     |<- OSHT trip event
                 +----------------------+                +----------------------+
                                                            |
                                                            +----> TZFLG[OST]
```

TZn

TZSEL[OSHT1 to OSHTn]

Async Trip

TZFLG[OST]

Set

Clear

#### **15.2.2.9 Event-Trigger (ET) Submodule**

[Figure](#page-62-1) 15-45 shows the event-trigger (ET) submodule in the ePWM system. The event-trigger submodule manages the events generated by the time-base submodule and the counter-compare submodule to generate an interrupt to the CPU.

CTR = CMPB CTR = CMPA CTR\_Dir CTR = 0 CTR = PRD Dead Band (DB) Counter Compare (CC) Action Qualifier (AQ) EPWMA EPWMB CTR = CMPB CTR = 0 EPWMxINT EPWMxA EPWMxB TZ1 TZn to CTR = CMPA Time-Base (TB) CTR = PRD CTR = 0 CTR\_Dir EPWMxSYNCI EPWMxSYNCO EPWMxTZINT PWMchopper (PC) Event Trigger and Interrupt (ET) Trip Zone (TZ)

**Figure 15-45. Event-Trigger Submodule**

#### *15.2.2.9.1 Purpose of the Event-Trigger Submodule*

The key functions of the event-trigger submodule are:

- Receives event inputs generated by the time-base and counter-compare submodules
- Uses the time-base direction information for up/down event qualification
- Uses prescaling logic to issue interrupt requests at:
  - Every event
  - Every second event
  - Every third event
- Provides full visibility of event generation via event counters and flags

#### *15.2.2.9.2 Controlling and Monitoring the Event-Trigger Submodule*

The key registers used to configure the event-trigger submodule are shown in Table [15-38:](#page-62-2)

**Acronym Register Description Address Offset Shadowed** ETSEL Event-Trigger Selection Register 32h No ETPS Event-Trigger Prescale Register 34h No ETFLG Event-Trigger Flag Register 36h No ETCLR Event-Trigger Clear Register 38h No ETFRC Event-Trigger Force Register 3Ah No

**Table 15-38. Event-Trigger Submodule Registers**

#### *15.2.2.9.3 Operational Overview of the Event-Trigger Submodule*

The following sections describe the event-trigger submodule's operational highlights.

Each ePWM module has one interrupt request line connected to the interrupt controller as shown in [Figure](#page-63-0) 15-46.

**Figure 15-46. Event-Trigger Submodule Inter-Connectivity to Interrupt Controller**

```
+--------------+        EPWM1INT        +---------------------+
| EPWM1 module | ---------------------> |                     |
+--------------+                        |                     |
                                        |  Interrupt          |
+--------------+        EPWM2INT        |  controller         |
| EPWM2 module | ---------------------> |                     |
+--------------+                        |                     |
                                        |                     |
     ...                                +---------------------+
      :
+--------------+        EPWMxINT
| EPWMx module | ---------------------> (to Interrupt controller)
+--------------+
```

The event-trigger submodule monitors various event conditions (the left side inputs to event-trigger submodule shown in [Figure](#page-63-1) 15-47) and can be configured to prescale these events before issuing an Interrupt request. The event-trigger prescaling logic can issue Interrupt requests at:

- Every event
- Every second event
- Every third event

**Figure 15-47. Event-Trigger Submodule Showing Event Inputs and Prescaled Outputs**

```
Figure 13 — Event Trigger (ET) module (interrupt prescale / direction qualify)

Inputs (events)                          +--------------------------------------+
CTR = 0   ----(pulse)------------------->|                                      |
CTR = PRD ----(pulse)------------------->|   Event Trigger Module Logic         |----> [/n] ----> EPWMxINT ---> Interrupt controller
                                         |                                      |        ^  ^
CTR = CMPA ----(pulse)----+              |   Registers:                         |        |  |
CTR = CMPB ----(pulse)----+              |    - ETSEL reg                       |      count clear
                         v               |    - ETPS  reg                       |
                    +-----------+        |    - ETFLG reg                       |----> [/n]  (prescale/counter)
CTR_dir ----------->| Direction | ------>|    - ETCLR reg                       |         ^  ^
                    | qualifier |        |    - ETFRC reg                       |      count clear
                    +-----------+        |                                      |
        outputs (qualified events):      +--------------------------------------+
         - CTRU = CMPA
         - CTRD = CMPA
         - CTRU = CMPB
         - CTRD = CMPB

(Additional prescale paths shown in the figure: multiple [/n] blocks with count/clear driven by ET logic.)
```

- ETSEL—This selects which of the possible events will trigger an interrupt.
- ETPS—This programs the event prescaling options previously mentioned.
- ETFLG—These are flag bits indicating status of the selected and prescaled events.
- ETCLR—These bits allow you to clear the flag bits in the ETFLG register via software.
- ETFRC—These bits allow software forcing of an event. Useful for debugging or software intervention.

A more detailed look at how the various register bits interact with the Interrupt is shown in [Figure](#page-65-0) 15-48.

[Figure](#page-65-0) 15-48 shows the event-trigger's interrupt generation logic. The interrupt-period (ETPS[INTPRD]) bits specify the number of events required to cause an interrupt pulse to be generated. The choices available are:

- Do not generate an interrupt
- Generate an interrupt on every event
- Generate an interrupt on every second event
- Generate an interrupt on every third event

An interrupt cannot be generated on every fourth or more events.

Which event can cause an interrupt is configured by the interrupt selection (ETSEL[INTSEL]) bits. The event can be one of the following:

- Time-base counter equal to zero (TBCNT = 0000h).
- Time-base counter equal to period (TBCNT = TBPRD).
- Time-base counter equal to the compare A register (CMPA) when the timer is incrementing.
- Time-base counter equal to the compare A register (CMPA) when the timer is decrementing.
- Time-base counter equal to the compare B register (CMPB) when the timer is incrementing.
- Time-base counter equal to the compare B register (CMPB) when the timer is decrementing.

The number of events that have occurred can be read from the interrupt event counter (ETPS[INTCNT]) register bits. That is, when the specified event occurs the ETPS[INTCNT] bits are incremented until they reach the value specified by ETPS[INTPRD]. When ETPS[INTCNT] = ETPS[INTPRD] the counter stops counting and its output is set. The counter is only cleared when an interrupt is sent to the interrupt controller.

When ETPS[INTCNT] reaches ETPS[INTPRD], one of the following behaviors will occur:

- If interrupts are enabled, ETSEL[INTEN] = 1 and the interrupt flag is clear, ETFLG[INT] = 0, then an interrupt pulse is generated and the interrupt flag is set, ETFLG[INT] = 1, and the event counter is cleared ETPS[INTCNT] = 0. The counter will begin counting events again.
- If interrupts are disabled, ETSEL[INTEN] = 0, or the interrupt flag is set, ETFLG[INT] = 1, the counter stops counting events when it reaches the period value ETPS[INTCNT] = ETPS[INTPRD].
- If interrupts are enabled, but the interrupt flag is already set, then the counter will hold its output high until the ENTFLG[INT] flag is cleared. This allows for one interrupt to be pending while one is serviced.

Writing to the INTPRD bits will automatically clear the counter INTCNT = 0 and the counter output will be reset (so no interrupts are generated). Writing a 1 to the ETFRC[INT] bit will increment the event counter INTCNT. The counter will behave as described above when INTCNT = INTPRD. When INTPRD = 0, the counter is disabled and hence no events will be detected and the ETFRC[INT] bit is also ignored.

Note that the interrupts coming from the ePWM module are also used as DMA events. The interrupt registers should be used to enable and clear the current DMA event in order for the ePWM module to generate subsequent DMA events.

Latch Generate interrupt pulse when input = 1 2-bit Counter Set Clear 1 0 0 Clear CNT Inc CNT ETPS[INTCNT] ETPS[INTPRD] ETCLR[INT] EPWMxINT ETFRC[INT] ETSEL[INT] ETFLG[INT] ETSEL[INTSEL] 000 001 010 011 100 101 111 101 0 0 CTRU=CMPA CTRD=CMPA CTRU=CMPB CTRD=CMPB CTR=0 CTR=PRD

**Figure 15-48. Event-Trigger Interrupt Generator**

#### **15.2.2.10 High-Resolution PWM (HRPWM) Submodule**

[Figure](#page-66-1) 15-49 shows the high-resolution PWM (HRPWM) submodule in the ePWM system. Some devices include the high-resolution PWM submodule, see [Section](#page-3-0) 15.1.2 to determine which ePWM instances include this feature.

```
Figure 5 — ePWMx block overview (TB + CC + AQ + DB + PC + TZ + ET + Sync)

                        +------------------------------+
                        |         Time-base (TB)       |
                        |                              |
                        |  TBPRD shadow (16)           |
                        |  TBPRD active  (16)          |
                        |      |                       |
                        |      +--> (CTR = PRD pulse)  |
                        |                              |
                        |  Counter up/down (16-bit)    |
EPWMxSYNCI --->(OR)<----+  TBCNT active (16)            |
  TBCTL[SWFSYNC]        |   |     |                    |
 (software forced sync) |   |     +--> CTR = 0 pulse   |
                        |   +--------> CTR_Dir         |
                        |                              |
                        |  TBPHS active (24)           |
                        |   ^  (16)                    |
                        |   |                          |
                        |  TBPHSHR (8) -> Phase control|
                        +------------------------------+
                                   |
                                   | (feeds compare/AQ/ET timing)
                                   v

                 +----------------------------------+
                 |      Counter compare (CC)        |
                 |                                  |
                 |  CMPA active (24) / shadow (24)  |
                 |    +--> (CTR = CMPA pulse)       |
                 |  CMPAHR (8)                      |
                 |                                  |
                 |  CMPB active (16) / shadow (16)  |
                 |    +--> (CTR = CMPB pulse)       |
                 +----------------------------------+
                         |                 |
                         | CTR=CMPA        | CTR=CMPB
                         v                 v

                 +------------------------------+
                 |   Action qualifier (AQ)      |
                 |                              |
                 |  outputs: EPWMA, EPWMB       |
                 +------------------------------+
                     |                    |
                     | EPWMA              | EPWMB
                     v                    v
        (optional HiRes PWM (HRPWM) on EPWMA path)
           [HRPWM] (dashed block, on EPWMA)

                     +------------------+
                     | Dead band (DB)   |
                     +------------------+
                             |
                             v
                     +------------------+
                     | PWM chopper (PC) |
                     +------------------+
                             |
                             v
                     +------------------+<----- TZ1 to TZn
                     | Trip zone (TZ)   |
                     +------------------+
                       |          |             +------------------+
                       |          +-----------> | EPWMxTZINT        |
                       |                        +------------------+
                       +-----------------------> EPWMxA
                       +-----------------------> EPWMxB

Sync out path:
  sources --> +---------------------------+ ----> EPWMxSYNCO
              | Sync in/out select Mux    |
              |  CTR = 0 / CTR = CMPB /   |
              |  Disabled                 |
              +---------------------------+
                    ^ TBCTL[SYNCOSEL]

Event Trigger / Interrupt (ET):
  inputs: CTR = PRD, CTR = 0, CTR = CMPA, CTR = CMPB, CTR_Dir
           \_______________________________________________/
                            |
                            v
                  +---------------------------+ ----> EPWMxINT
                  | Event trigger & interrupt |
                  |            (ET)           |
                  +---------------------------+
```

**Figure 15-49. HRPWM System Interface**

#### *15.2.2.10.1 Purpose of the High-Resolution PWM Submodule*

The enhanced high-resolution pulse-width modulator (eHRPWM) extends the time resolution capabilities of the conventionally derived digital pulse-width modulator (PWM). HRPWM is typically used when PWM resolution falls below ~9-10 bits. The key features of HRPWM are:

- Extended time resolution capability
- Used in both duty cycle and phase-shift control methods
- Finer time granularity control or edge positioning using extensions to the Compare A and Phase registers
- Implemented using the A signal path of PWM, that is, on the EPWMxA output. EPWMxB output has conventional PWM capabilities

The ePWM peripheral is used to perform a function that is mathematically equivalent to a digital-to-analog converter (DAC). As shown in [Figure](#page-67-0) 15-50, the effective resolution for conventionally generated PWM is a function of PWM frequency (or period) and system clock frequency.

**Figure 15-50. Resolution Calculations for Conventionally Generated PWM**

```
Figure 11 — PWM period vs system clock tick (resolution)

      |<----------------------  T_PWM  ---------------------->|
      ___‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾_____
                        |  |  |
                        |  |  |  (edge can shift in SYSCLK steps)
                        v  v  v
t  ----------------------------------------------------------->

          |<-- T_SYSCLK -->|
          |<-- T_SYSCLK -->|

PWM resolution (%):   (F_PWM / F_SYSCLKOUT) × 100%
PWM resolution (bits): Log2( F_PWM / F_SYSCLKOUT )
```

If the required PWM operating frequency does not offer sufficient resolution in PWM mode, you may want to consider HRPWM. As an example of improved performance offered by HRPWM, Table [15-39](#page-67-1) shows resolution in bits for various PWM frequencies. Table [15-39](#page-67-1) values assume a MEP step size of 180 ps. See your device-specific data manual for typical and maximum performance specifications for the MEP.

**PWM Frequency (kHz) Regular Resolution (PWM) High Resolution (HRPWM) Bits % Bits %** 20 12.3 0.0 18.1 0.000 50 11.0 0.0 16.8 0.001 100 10.0 0.1 15.8 0.002 150 9.4 0.2 15.2 0.003 200 9.0 0.2 14.8 0.004 250 8.6 0.3 14.4 0.005 500 7.6 0.5 13.8 0.007 1000 6.6 1.0 12.4 0.018 1500 6.1 1.5 11.9 0.027 2000 5.6 2.0 11.4 0.036

**Table 15-39. Resolution for PWM and HRPWM**

Although each application may differ, typical low-frequency PWM operation (below 250 kHz) may not require HRPWM. HRPWM capability is most useful for high-frequency PWM requirements of power conversion topologies such as:

- Single-phase buck, boost, and flyback
- Multi-phase buck, boost, and flyback
- Phase-shifted full bridge
- Direct modulation of D-Class power amplifiers

#### *15.2.2.10.2 Architecture of the High-Resolution PWM Submodule*

The HRPWM is based on micro edge positioner (MEP) technology. MEP logic is capable of positioning an edge very finely by sub-dividing one coarse system clock of a conventional PWM generator. The time step accuracy is on the order of 150 ps. The HRPWM also has a self-check software diagnostics mode to check if the MEP logic is running optimally, under all operating conditions.

[Figure](#page-68-0) 15-51 shows the relationship between one coarse system clock and edge position in terms of MEP steps, which are controlled via an 8-bit field in the Compare A extension register (CMPAHR).

```
Figure 6 — HRPWM: coarse steps + MEP (fine) steps inside one coarse step

PWM waveform (one period)
|<---------------------- PWM period (N CPU cycles) ---------------------->|
 ____‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾_________________________________________
 |<----- PWM duty (0.0 .. 1.0 in Q15 format) ----->|

Time quantization (ticks)
| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
        ^---------------- number of coarse steps ----------------^
                       (integer part of PWMduty*PWMperiod)

Inside one coarse step:
  |<-- coarse step size -->|
  |.|.|.|.|.|.|.|.|         (MEP sub-steps)
   ^   ^
   |   +-- MEP step
   +------ MEP scale factor = number of MEP steps in one coarse step

Equations shown:
- Number of coarse steps = integer(PWMduty * PWMperiod)
- Number of MEP steps    = fraction(PWMduty * PWMperiod) * (MEPScaleFactor)

Register mapping shown:
- 16-bit CMPA register value   = number of coarse steps
- 16-bit CMPAHR register value = (number of MEP steps) << 8 + 0x180 (rounding)
```

**Figure 15-51. Operating Logic Using MEP**

A For MEP range and rounding adjustment.

To generate an HRPWM waveform, configure the TBM, CCM, and AQM registers as you would to generate a conventional PWM of a given frequency and polarity. The HRPWM works together with the TBM, CCM, and AQM registers to extend edge resolution, and should be configured accordingly. Although many programming combinations are possible, only a few are needed and practical.

#### *15.2.2.10.3 Controlling and Monitoring the High-Resolution PWM Submodule*

The MEP of the HRPWM is controlled by two extension registers, each 8-bits wide. These two HRPWM registers are concatenated with the 16-bit TBPHS and CMPA registers used to control PWM operation.

- TBPHSHR Time-Base Phase High-Resolution Register
- CMPAHR Counter-Compare A High-Resolution Register

Table [15-40](#page-68-1) lists the registers used to control and monitor the high-resolution PWM submodule.

| Acronym | Register Description               | Address Offset | Shadowed |
|---------|------------------------------------|----------------|----------|
| TBPHSHR | Extension Register for HRPWM Phase | 4h             | No       |
| CMPAHR  | Extension Register for HRPWM Duty  | 10h            | Yes      |
| HRCNFG  | HRPWM Configuration Register       | C0h            | No       |

**Table 15-40. HRPWM Submodule Registers**

#### *15.2.2.10.4 Configuring the High-Resolution PWM Submodule*

Once the ePWM has been configured to provide conventional PWM of a given frequency and polarity, the HRPWM is configured by programming the HRCNFG register located at offset address C0h. This register provides configuration options for the following key operating modes:

- **Edge Mode:** The MEP can be programmed to provide precise position control on the rising edge (RE), falling edge (FE), or both edges (BE) at the same time. FE and RE are used for power topologies requiring duty cycle control, while BE is used for topologies requiring phase shifting, for example, phase shifted full bridge.
- **Control Mode:** The MEP is programmed to be controlled either from the CMPAHR register (duty cycle control) or the TBPHSHR register (phase control). RE or FE control mode should be used with CMPAHR register. BE control mode should be used with TBPHSHR register.
- **Shadow Mode:** This mode provides the same shadowing (double buffering) option as in regular PWM mode. This option is valid only when operating from the CMPAHR register and should be chosen to be the same as the regular load option for the CMPA register. If TBPHSHR is used, then this option has no effect.

#### *15.2.2.10.5 Operational Highlights for the High-Resolution PWM Submodule*

The MEP logic is capable of placing an edge in one of 255 (8 bits) discrete time steps, each of which has a time resolution on the order of 150 ps. The MEP works with the TBM and CCM registers to be certain that time steps are optimally applied and that edge placement accuracy is maintained over a wide range of PWM frequencies, system clock frequencies and other operating conditions. Table [15-41](#page-69-0) shows the typical range of operating frequencies supported by the HRPWM.

**Table 15-41. Relationship Between MEP Steps, PWM Frequency and Resolution**

| System<br>(MHz) | MEP Steps Per<br>SYSCLKOUT(1) (2) (3) | PWM Minimum<br>(Hz)(4) | PWM Maximum<br>(MHz) | Resolution at<br>Maximum<br>(Bits)(5) |
|-----------------|---------------------------------------|------------------------|----------------------|---------------------------------------|
| 50.0            | 111                                   | 763                    | 2.50                 | 11.1                                  |
| 60.0            | 93                                    | 916                    | 3.00                 | 10.9                                  |
| 70.0            | 79                                    | 1068                   | 3.50                 | 10.6                                  |
| 80.0            | 69                                    | 1221                   | 4.00                 | 10.4                                  |
| 90.0            | 62                                    | 1373                   | 4.50                 | 10.3                                  |
| 100.0           | 56                                    | 1526                   | 5.00                 | 10.1                                  |

<sup>(1)</sup> System frequency = SYSCLKOUT, that is, CPU clock. TBCLK = SYSCLKOUT

<sup>(2)</sup> Table data based on a MEP time resolution of 180 ps (this is an example value)

<sup>(3)</sup> MEP steps applied = TSYSCLKOUT/180 ps in this example.

<sup>(4)</sup> PWM minimum frequency is based on a maximum period value, TBPRD = 65 535. PWM mode is asymmetrical up-count.

<sup>(5)</sup> Resolution in bits is given for the maximum PWM frequency stated.

#### *15.2.2.10.5.1 Edge Positioning*

In a typical power control loop (switch modes, digital motor control (DMC), uninterruptible power supply (UPS)), a digital controller (PID, 2pole/2zero, lag/lead, etc.) issues a duty command, usually expressed in a per unit or percentage terms.

In the following example, assume that for a particular operating point, the demanded duty cycle is 0.405 or 40.5% on-time and the required converter PWM frequency is 1.25 MHz. In conventional PWM generation with a system clock of 100 MHz, the duty cycle choices are in the vicinity of 40.5%. In [Figure](#page-70-0) 15-52, a compare value of 32 counts (duty = 40%) is the closest to 40.5% that you can attain. This is equivalent to an edge position of 320 ns instead of the desired 324 ns. This data is shown in Table [15-42](#page-70-1).

By utilizing the MEP, you can achieve an edge position much closer to the desired point of 324 ns. Table [15-42](#page-70-1) shows that in addition to the CMPA value, 22 steps of the MEP (CMPAHR register) will position the edge at 323.96 ns, resulting in almost zero error. In this example, it is assumed that the MEP has a step resolution of 180 ns.

```
Figure 7 — Example: duty placement with fine (10 ns) steps

|<------------------------------  T_pwm = 800 ns  ------------------------------>|
0                                                                            79
|---- coarse ticks ----| | | | | | | | | | | | | | | | | | | | | | | | | | | | |

Demanded duty = 40.5%  (dashed reference)
------------------------------·-----------------------------------------------

Time to demanded edge:
324 ns  (shown from start to the demanded edge marker)

Zoom around the edge (fine positioning):
                 30   31   32   33   34
                 |    |    |    |    |
                 :    :    :    :    :
                 <---- 10 ns steps ---->

Corresponding duty points (as labeled):
- 30  -> 37.5%
- 31  -> 38.8%
- 32  -> 40.0%
- 33  -> 41.3%
- 34  -> 42.5%

EPWM1A waveform (edge placed between coarse counts using fine steps):
EPWM1A  ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\_______________________________/‾‾‾‾‾‾‾
                           (falling edge adjusted with 10 ns steps)
```

**Figure 15-52. Required PWM Waveform for a Requested Duty = 40.5%**

**Table 15-42. CMPA vs Duty (left), and [CMPA:CMPAHR] vs Duty (right)**

| CMPA<br>(count)(1) (2) (3) | DUTY<br>(%) | High Time<br>(ns) | CMPA<br>(count) | CMPAHR<br>(count) | Duty<br>(%) | High Time<br>(ns) |
|----------------------------|-------------|-------------------|-----------------|-------------------|-------------|-------------------|
| 28                         | 35.0        | 280               | 32              | 18                | 40.405      | 323.24            |
| 29                         | 36.3        | 290               | 32              | 19                | 40.428      | 323.42            |
| 30                         | 37.5        | 300               | 32              | 20                | 40.450      | 323.60            |
| 31                         | 38.8        | 310               | 32              | 21                | 40.473      | 323.78            |
| 32                         | 40.0        | 320               | 32              | 22                | 40.495      | 323.96            |
| 33                         | 41.3        | 330               | 32              | 23                | 40.518      | 324.14            |
| 34                         | 42.5        | 340               | 32              | 24                | 40.540      | 324.32            |
|                            |             |                   | 32              | 25                | 40.563      | 324.50            |
| Required                   |             |                   | 32              | 26                | 40.585      | 324.68            |
| 32.40                      | 40.5        | 324               | 32              | 27                | 40.608      | 324.86            |

<sup>(1)</sup> System clock, SYSCLKOUT and TBCLK = 100 MHz, 10 ns

<sup>(2)</sup> For a PWM Period register value of 80 counts, PWM Period = 80 × 10 ns = 800 ns, PWM frequency = 1/800 ns = 1.25 MHz

<sup>(3)</sup> Assumed MEP step size for the above example = 180 ps

#### *15.2.2.10.5.2 Scaling Considerations*

The mechanics of how to position an edge precisely in time has been demonstrated using the resources of the standard (CMPA) and MEP (CMPAHR) registers. In a practical application, however, it is necessary to seamlessly provide the CPU a mapping function from a per-unit (fractional) duty cycle to a final integer (non-fractional) representation that is written to the [CMPA:CMPAHR] register combination.

To do this, first examine the scaling or mapping steps involved. It is common in control software to express duty cycle in a per-unit or percentage basis. This has the advantage of performing all needed math calculations without concern for the final absolute duty cycle, expressed in clock counts or high time in ns. Furthermore, it makes the code more transportable across multiple converter types running different PWM frequencies.

To implement the mapping scheme, a two-step scaling procedure is required.

Assumptions for this example:

System clock, SYSCLKOUT = 10 ns (100 MHz) PWM frequency = 1.25 MHz (1/800 ns)

Required PWM duty cycle, **PWMDuty** = 0.405 (40.5%)

PWM period in terms of coarse steps, = 80

**PWMperiod** (800 ns/10 ns)

Number of MEP steps per coarse step at

180 ps (10 ns/180 ps), **MEP\_SF**

Value to keep CMPAHR within the range

of 1-255 and fractional rounding constant (default value) = 180h

= 55

## **Step 1: Percentage Integer Duty value conversion for CMPA register**

CMPA register value = int(**PWMDuty** × **PWMperiod**); int means integer part

= int(0.405 × 80)

= int(32.4)

CMPA register value = 32 (20h)

#### **Step 2: Fractional value conversion for CMPAHR register**

CMPAHR register value = (frac(**PWMDuty** × **PWMperiod**) × **MEP\_SF**) << 8) +

180h; frac means fractional part

= (frac(32.4) × 55 <<8) + 180h; Shift is to move the

value as CMPAHR high byte

= ((0.4 × 55) <<8) + 180h

= (22 <<8) + 180h

= 22 × 256 + 180h ; Shifting left by 8 is the same

multiplying by 256.

= 5632 + 180h

= 1600h + 180h

CMPAHR value = 1780h; CMPAHR value = 1700h, lower 8 bits will be

ignored by hardware.

#### *15.2.2.10.5.3 Duty Cycle Range Limitation*

In high resolution mode, the MEP is not active for 100% of the PWM period. It becomes operational 3 SYSCLK cycles after the period starts.

Duty cycle range limitations are illustrated in [Figure](#page-72-0) 15-53. This limitation imposes a lower duty cycle limit on the MEP. For example, precision edge control is not available all the way down to 0% duty cycle. Although for the first 3 or 6 cycles, the HRPWM capabilities are not available, regular PWM duty control is still fully operational down to 0% duty. In most applications this should not be an issue as the controller regulation point is usually not designed to be close to 0% duty cycle.

**Figure 15-53. Low % Duty Cycle Range Limitation Example When PWM Frequency = 1 MHz**

```
Figure 7 — T_PWM and early-edge placement example

|<-------------------------------  T_PWM  ------------------------------->|
t0                                                                      t100
  0    3    6                                                     100

(TBCNT-style ramp over one PWM period)
   /
  /
 /
/_________________________________________________________________________
                                                                           \
                                                                            \ (wrap)

      ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
      |‾‾‾‾‾‾|
      |      |
______|      |_____________________________________________________________|‾‾‾
      0      6                                                           100
         (vertical markers shown at 0, 3, 6)
```

If the application demands HRPWM operation in the low percent duty cycle region, then the HRPWM can be configured to operate in count-down mode with the rising edge position (REP) controlled by the MEP. This is illustrated in [Figure](#page-72-1) 15-54. In this case low percent duty limitation is no longer an issue.

**Figure 15-54. High % Duty Cycle Range Limitation Example when PWM Frequency = 1 MHz**

```
Figure 10 — T_pwm with edge occurring at the end (near 100)

|<-------------------------------  T_pwm  ------------------------------->|
t0                                                                      t100
  0    3    6                                                     100

(TBCNT-style ramp over one PWM period; decreasing then wraps)
\_________________________________________________________________________
 \                                                                        |
  \                                                                       |
   \                                                                      |
    \                                                                     |
     \                                                                    |
      \                                                                   |
       \                                                                  |
        \                                                                 |
         \________________________________________________________________| (wrap)

_____________________________________________________________________|‾|_____
                                                                     100
(very narrow pulse at the end of the period)
```

#### *15.2.2.10.6 Scale Factor Optimizing Software (SFO)*

The micro edge positioner (MEP) logic is capable of placing an edge in one of 255 discrete time steps. As previously mentioned, the size of these steps is on the order of 150 ps. The MEP step size varies based on worst-case process parameters, operating temperature, and voltage. MEP step size increases with decreasing voltage and increasing temperature and decreases with increasing voltage and decreasing temperature. Applications that use the HRPWM feature should use the TI-supplied MEP scale factor optimizer (SFO) software functions. SFO functions help to dynamically determine the number of MEP steps per SYSCLKOUT period while the HRPWM is in operation.

To utilize the MEP capabilities effectively during the Q15 duty to [CMPA:CMPAHR] mapping function (see Section [15.2.2.10.5.2\)](#page-71-0), the correct value for the MEP scaling factor (MEP\_SF) needs to be known by the software. To accomplish this, each HRPWM module has built in self-check and diagnostics capabilities that can be used to determine the optimum MEP\_SF value for any operating condition. TI provides a Ccallable library containing two SFO functions that utilize this hardware and determine the optimum MEP\_SF. As such, MEP Control and Diagnostics registers are reserved for TI use.

Currently, there are two released versions of the SFO library - SFO\_TI\_Build.lib and SFO\_TI\_Build\_V5.lib. Versions 2, 3, and 4 were TI-Internal only. A high-level comparison table between the two library versions can be found in Section [15.2.2.10.6.1.](#page-73-0) A detailed description of the SFO\_TI\_Build.lib software functions follows in Section [15.2.2.10.6.2](#page-74-0). Information on the SFO\_TI\_Build\_V5.lib software functions, which support up to 16 HRPWM channels, can be found in Section [15.2.2.10.6.4](#page-78-0).

**NOTE:** For floating-point devices, when compiling application code for floating point (fpu32 mode), libraries utilized by the application code must also be compiled for floating point. The SFO\_TI\_Build\_fpu.lib and SFO\_TI\_Build\_V5\_fpu.lib are available as the floating-point compiled equivalents to the fixed-point SFO\_TI\_Build.lib and SFO\_TI\_Build\_V5.lib libraries. The SFO functions in the fpu-version libraries are C-code-compatible to their fixed-point equivalents.

#### *15.2.2.10.6.1 SFO Library Version Comparison*

Table [15-43](#page-73-1) includes a high-level comparison between SFO\_TI\_Build.lib and SFO\_TI\_Build\_V5.lib. A detailed description of SFO\_TI\_Build\_V5.lib follows the table, and more information on SFO\_TI\_Build.lib can be found in Section [15.2.2.10.6](#page-73-2).

**Table 15-43. SFO Library Version Comparison**

|                                                      | SYSCLK<br>Frequency | ePWM<br>Frequency | SFO_TI_Build.lib | SFO_TI_Build_V5.lib                   | Unit         |
|------------------------------------------------------|---------------------|-------------------|------------------|---------------------------------------|--------------|
| Maximum HRPWM<br>channels supported                  | -                   | -                 | Up to 4          | Up to 16                              | channels     |
| Total static variable<br>memory size                 | -                   | -                 | 220              | 79 (1 channel) to<br>192 (16 channel) | words        |
| MepEn runs on multiple<br>channels concurrently?     | -                   | -                 | Yes              | No                                    | -            |
| Error-checking?                                      | -                   | -                 | No               | Yes                                   | -            |
| Typical time required for                            | -                   | 400 kHz           | 3.26             | 1.5                                   | seconds      |
| MepEn to update<br>MEP_ScaleFactor on 1              | -                   | 1 MHz             | 1.308            | 0.6                                   | seconds      |
| channel if called repetitively                       | -                   | 2 MHz             | 0.66             | 0.3                                   | seconds      |
| without interrupts                                   | -                   | 3.33 MHz          | 0.396            | 0.18                                  | seconds      |
|                                                      | -                   | 200 MHz           | 0.066            | 0.03                                  | seconds      |
| Typical time required for                            | 100 MHz             | -                 | 0.83             | 1.86                                  | milliseconds |
| MepDis to update<br>MEP_ScaleFactor on 1             | 60 MHz              | -                 | 1.38             | 1.38                                  | milliseconds |
| channel if called repetitively<br>without interrupts | 50 MHz              | -                 | 1.66             | 1.66                                  | milliseconds |

#### *15.2.2.10.6.2 SFO\_TI\_Build Library Routines*

Table [15-44](#page-74-1) provides functional descriptions of the two SFO library routines in SFO\_TI\_Build.lib.

### **Table 15-44. SFO Library Routines**

| Function      | Description                                                                                                                                                                                                                                                                                                                      |  |  |  |
|---------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|--|
| SFO_MepDis(n) | Scale Factor Optimizer with MEP Disabled                                                                                                                                                                                                                                                                                         |  |  |  |
|               | This routine runs faster, as the calibration logic works when HRPWM capabilities are disabled; therefore,<br>HRPWM capabilities cannot be run concurrently when the ePWMn is being used.                                                                                                                                         |  |  |  |
|               | The function returns a value in the variable array:                                                                                                                                                                                                                                                                              |  |  |  |
|               | MEP_ScaleFactor[n](1)<br>= Number of MEP steps/SYSCLKOUT                                                                                                                                                                                                                                                                         |  |  |  |
|               | If TBCLK is not equal to SYSCLKOUT, then the returned value must be adjusted to reflect the correct<br>TBCLK:                                                                                                                                                                                                                    |  |  |  |
|               | = MEP_ScaleFactor[n] × (SYSCLKOUT/TBCLK)(1)<br>MEP steps per TBCLK                                                                                                                                                                                                                                                               |  |  |  |
|               | Constraints when using this function:                                                                                                                                                                                                                                                                                            |  |  |  |
|               | MEP diagnostics logic uses SYSCLKOUT not TBCLK and hence SYSCLKOUT restriction is an important<br>constraint. SFO_MepDis(n) function does not require a starting Scale Factor value.                                                                                                                                             |  |  |  |
|               | When to use:                                                                                                                                                                                                                                                                                                                     |  |  |  |
|               | If one of the ePWM modules is not used in HRPWM mode, then it can be dedicated to run the SFO<br>diagnostics for the modules that are running HRPWM mode. Here the single MEP_SF value obtained can be<br>applied to other ePWM modules. This assumes that all HRPWM module's MEP steps are similar but may<br>not be identical. |  |  |  |
|               | The ePWM module that is not active in HRPWM mode is still fully operational in conventional PWM mode<br>and can be used to drive PWM pins. The SFO function only makes use of the MEP diagnostics logic.                                                                                                                         |  |  |  |
|               | The other ePWM modules operating in HRPWM mode incur only a 3-cycle minimum duty limitation.                                                                                                                                                                                                                                     |  |  |  |
| SFO_MepEn(n)  | Scale Factor Optimizer with MEP Enabled                                                                                                                                                                                                                                                                                          |  |  |  |

This routine runs slower as the calibration logic is used concurrently while HRPWM capabilities are being used by the ePWM module.

The function returns a value in the variable array:

MEP\_ScaleFactor[n](1) = Number of MEP steps/SYSCLKOUT = Number of MEP steps/TBCLK

#### **Constraints when using this function:**

MEP diagnostics logic uses SYSCLKOUT not TBCLK and hence SYSCLKOUT restriction is an important constraint. SFO\_MepEn(n) function does require a starting Scale Factor value.MEP\_ScaleFactor[0] needs to be initialized to a typical MEP step size value.

**NOTE:** SFO\_MepEn(n) only supports the following HRPWM configuration:

- HRCNFG[HRLOAD] = 0 (load on CTR = 0)
- HRCNFG[EDGMODE] = 10 (falling edge MEP control)

#### **When to use:**

If the application requires all ePWM modules to have HRPWM capability (MEP is operational), then the SFO\_MepEn(n) function should run for each of the active ePWM modules with HRPWM capability.

- In the above case, a 6-cycle MEP inactivity zone exists at the start of the PWM period. See Section [15.2.2.10.5.3](#page-72-2) on duty cycle range limitation.
- If all ePWM modules are using the same TBCLK prescaler, then it is also possible to run the SFO\_MepEn(n) function for only one ePWM module and to use the SFO return value for the other modules. In this case only one ePWM module incurs the 6-cycle limitation, and remaining modules incur only a 3-cycle minimum duty limitation. See "Duty cycle limitation" section. This assumes that all HRPWM module's MEP steps are similar but may not be identical.

<sup>(1)</sup> n is the ePWM module number on which the SFO function operates.

Both routines can be run as background tasks in a slow loop requiring negligible CPU cycles. In most applications only one of these routines will be needed. However, if the application has free HRPWM resources then both the routines could be used. The repetition rate at which an SFO function needs to be executed depends on the applications operating environment. As with all digital CMOS devices temperature and supply voltage variations have an effect on MEP operation. However, in most applications these parameters vary slowly and therefore it is often sufficient to execute the SFO function once every 5 to 10 seconds or so. If more rapid variations are expected, then execution may have to be performed more frequently to match the application. Note, there is no high limit restriction on the SFO function repetition rate, hence it can execute as quickly as the background loop is capable.

While using HRPWM feature with no SFO diagnostics, HRPWM logic will not be active for the first 3 TBCLK cycles of the PWM period. While running the application in this configuration, if CMPA register value is less than 3 cycles, then its CMPAHR register must be cleared to zero. This would avoid any unexpected transitions on PWM signal.

However, if SFO diagnostic function SFO\_MepEn is used in the background, then HRPWM logic will not be active for the first 6 TBCLK cycles of PWM period. While using SFO\_MepEn function if CMPA register value is less than 6 cycles, then its CMPAHR register must be cleared to zero. This would avoid any unexpected transitions on PWM signal. Also note that the SFO\_MepDis function cannot be used concurrently with PWM signals with HRPWM enabled.

#### *15.2.2.10.6.3 SFO\_TI\_Build Software Usage*

Software library functions SFO\_MepEn(int n) and SFO\_MepDis(int n) calculate the MEP scale factor for up to four ePWM modules. The scale factor is an integer value in the range 1–255, and represents the number of micro step edge positions available for a system clock period. The scale factor value is returned in a global array of integer values called MEP\_ScaleFactor[x], where x is the maximum number of HRPWM channels for a device plus one. For example, see Table [15-45](#page-75-0).

**Table 15-45. Factor Values**

| Software function calls | Functional description                          | Updated Variable<br>MEP_ScaleFactor[5](1) |
|-------------------------|-------------------------------------------------|-------------------------------------------|
| SFO_MepDis(n)           |                                                 |                                           |
| SFO_MepDis(1);          | Returns the scale factor value to array index 1 | MEP_ScaleFactor[1]                        |
| SFO_MepDis(2);          | Returns the scale factor value to array index 2 | MEP_ScaleFactor[2]                        |
| SFO_MepDis(3);          | Returns the scale factor value to array index 3 | MEP_ScaleFactor[3]                        |
| SFO_MepDis(4);          | Returns the scale factor value to array index 4 | MEP_ScaleFactor[4]                        |
| SFO_MepEn(n)            |                                                 |                                           |
| SFO_MepEn(1);           | Returns the scale factor value to array index 1 | MEP_ScaleFactor[1]                        |
| SFO_MepEn(2);           | Returns the scale factor value to array index 2 | MEP_ScaleFactor[2]                        |
| SFO_MepEn(3);           | Returns the scale factor value to array index 3 | MEP_ScaleFactor[3]                        |
| SFO_MepEn(4);           | Returns the scale factor value to array index 4 | MEP_ScaleFactor[4]                        |

<sup>(1)</sup> MEP\_ScaleFactor[0] variable is a starting value and used by the SFO software functions internally

To use the HRPWM feature of the ePWMs it is recommended that the SFO functions be used as follows. For different devices that may have fewer HRPWM channels, modifications will be required in Step 1 and Step 2.

**NOTE:** The following example assumes there are four ePWM instances that contain the HRPWM submodule in the device. See [Section](#page-3-0) 15.1.2 to determine the number of ePWM instances that contain the HRPWM submodule.

#### **Step 1. Add Include Files**

The SFO.h file needs to be included as follows. This include file is mandatory while using the SFO library function. These include files are optional if customized header files are used in the end applications.

#### *Example 15-3. A Sample of How to Add Include Files*

```
#include "SFO.h" // SFO lib functions (needed for HRPWM)
```

#### **Step 2. Element Declaration**

Declare a 5-element array of integer variables as follows:

#### *Example 15-4. Declaring an Element*

```
int MEP_ScaleFactor[5] = {0,0,0,0,0}; // Scale factor values for ePWM1-4
int MEP_SF1, MEP_SF2, MEP_SF3, MEP_SF4
volatile struct EPWM_REGS *ePWM[] = {&EPwm1Regs, &EPwm1Regs, &EPwm2Regs, &EPwm3Regs, &EPwm4Regs};
```

## **Step 3. MEP\_ScaleFactor Initialization**

After power up, the SFO\_MepEn(n) function needs a starting Scale Factor value. This value can be conveniently determined by using one of the ePWM modules to run the SFO\_MepDis(n) function prior to configuring its PWM outputs for the application. SFO\_MepDis(n) function does not require a starting Scale Factor value.

As part of the one-time initialization code, include the following:

#### *Example 15-5. Initializing With a Scale Factor Value*

```
// MEP_ScaleFactor variables initialized using function SFO_MepDis
while (MEP_ScaleFactor[1] == 0) SFO_MepDis(1); //SFO for HRPWM1
while (MEP_ScaleFactor[2] == 0) SFO_MepDis(2); //SFO for HRPWM2
while (MEP_ScaleFactor[3] == 0) SFO_MepDis(3); //SFO for HRPWM3
while (MEP_ScaleFactor[4] == 0) SFO_MepDis(4); //SFO for HRPWM4
// Initialize a common seed variable MEP_ScaleFactor[0] // required for all SFO functions
MEP_ScaleFactor[0] = MEP_ScaleFactor[1]; //Common variable for SFOMepEN(n) function
```

#### **Step 4. Application Code**

While the application is running, fluctuations in both device temperature and supply voltage may be expected. To be sure that optimal Scale Factors are used for each ePWM module, the SFO function should be re-run periodically as part of a slower back-ground loop. Some examples of this are shown here.

## *Example 15-6. SFO Function Calls*

```
main()
{
   // User code
   // Case1: ePWM1,2,3,4 are running in HRPWM mode
       SFO_MepEn(1); // Each of these of function enables
       SFO_MepEn(2); // the respective MEP diagnostic logic
       SFO_MepEn(3); // and returns MEP Scale factor value
       SFO_MepEn(4);
       MEP_SF1 = MEP_ScaleFactor[1]; // used for ePWM1
       MEP_SF2 = MEP_ScaleFactor[2]; // used for ePWM2
       MEP_SF3 = MEP_ScaleFactor[3]; // used for ePWM3
       MEP_SF4 = MEP_ScaleFactor[4]; // used for ePWM4
   // Case2:ePWM1,2,3 only are running in HRPWM mode.
             One of the ePWM channel(as an example ePWM4) is used as for
             Scale factor calibration
  // Here minimum duty cycle limitation is 3 clock cycles.
  //
  // HRPWM 4 MEP diagnostics circuit is used to estimate the MEP steps
  // with the assumption that all HRPWM channels behave similarly
  // though may not be identical.
       SFO_MepDis(4); // MEP steps using ePWM4
       MEP_SF1 = MEP_ScaleFactor[4]; // used for ePWM1
       MEP_SF2 = MEP_SF1 // used for ePWM2
       MEP_SF3 = MEP_SF1 // used for ePWM3
       MEP_SF4 = MEP_SF1 // used for ePWM4
}
```

#### *15.2.2.10.6.4 SFO\_TI\_Build\_V5 Library Routines*

In SFO\_TI\_Build\_V5.lib, the diagnostic software has been optimized to use less memory, to minimize the calibration time, and to support up to 16 HRPWM channels. Table [15-46](#page-78-1) provides functional description of the two SFO library routines in SFO\_TI\_Build\_V5.lib.

#### **Table 15-46. SFO V5 Library Routines**

#### **Function Description**

#### **int SFO\_MepDis\_V5 (n) Scale Factor Optimizer V5 with MEP Disabled**

This routine is very similar to the SFO\_MepDis() routine in the original SFO library, but with one change. It now returns a 1 when MEP disabled calibration is complete, or a 0 while calibration is still running.

This function runs faster than the SFO\_MepEn \_V5() routine and cannot be used on an ePWM channel while HRPWM capabilities are enabled for that channel. If there is a spare ePWM channel available in the system, SFO\_MepDis\_V5() can be run for that channel, and the resulting MEP\_ScaleFactor[n] value can be copied into the MEP\_ScaleFactor[n] for all other channels.

Because the MEP step behavior in a particular piece of silicon is similar on all HRPWM channels with regard to temperature and voltage, using one dedicated HRPWM channel for calibration by calling the SFO\_MepDis\_V5 function will reduce software overhead.

The function returns a value in the variable array:

MEP\_ScaleFactor[n] = Number of MEP steps/SYSCLKOUT

If TBCLK is not equal to SYSCLKOUT, then the returned value must be adjusted to reflect the correct TBCLK:

MEP steps per TBCLK = MEP\_ScaleFactor[n] × (SYSCLKOUT/TBCLK)

#### **Constraints when using this function:**

- MEP diagnostics logic uses SYSCLKOUT and not TBCLK. Hence, the SYSCLKOUT restriction is an important constraint.
- If TBCLK does not equal SYSCLKOUT, the TBCLK frequency must be great enough so that MEP steps per TBCLK do not exceed 255. This is due to the restriction that there can be no more than 255 MEP steps in a coarse step.
- This function cannot be run on an ePWM channel with HRPWM capabilities enabled.

#### **Usage:**

- If one of the ePWM modules is running in normal ePWM mode, then it can be used to run the SFO diagnostics function. Here, the single MEP\_ScaleFactor value obtained for that channel can be copied and used as the MEP\_ScaleFactor for the other ePWM modules which are running HRPWM mode. This assumes that all HRPWM modules' MEP steps are similar but may not be identical.
- This routine returns a 1 when calibration is finished on the specified channel or a 0 if calibration is still running.
- The ePWM module that is not active in HRPWM mode is still fully operational in conventional PWM mode and can be used to drive PWM pins. The SFO function only makes use of the MEP diagnostics logic in the HRPWM circuitry.
- SFO\_MepDis\_V5(n) function does not require a starting Scale Factor value.
- The other ePWM modules operating in HRPWM mode incur only a 3-cycle minimum duty cycle limitation.
## **Table 15-46. SFO V5 Library Routines (continued)**

| Function<br>Description |
|-------------------------|
|-------------------------|

## **int SFO\_MepEn\_V5 (n) Scale Factor Optimizer V5 with MEP Enabled**

This function runs slower than the SFO\_MepDis\_V5() routine and runs SFO diagnostics on an ePWM channel with HRPWM capabilities enabled for that channel.

The function returns a value in the variable array:

MEP\_ScaleFactor[n] = Number of MEP steps/SYSCLKOUT = Number of MEP steps/TBCLK

#### **Constraints when using this function:**

• This routine must be run on one channel at a time and cannot be run on multiple channels concurrently. When it has finished updating the MEP\_ScaleFactor for a channel, it will return a 1. If it is still calibrating, it will return a 0. A background loop should exist in the user code which calls SFO\_MepEn\_V5(n) repetitively until it returns a 1. Then the function can be called for the next channel .

**NOTE:** Unlike the original SFO\_MepEn(n) routine, this routine cannot run on multiple channels concurrently.

> Do not call SFO\_MepEn\_V5(n) for another channel until the function returns a 1 for the current channel. Otherwise, the MEP\_ScaleFactor for both channels will become corrupted.

SFO\_MepEn\_V5(n) in *SFO\_TI\_Build\_V5.lib* supports only the following HRPWM configuration:

- HRCNFG[HRLOAD] = 0 (load on CTR = 0)
- HRCNFG[EDGMODE] = 10 (falling edge MEP control)

An upgraded version of SFO\_MepEn\_V5(n) in *SFO\_TI\_Build\_V5B.lib* supports all available HRPWM configurations. When using this version, the HRCNFG register must be initialized with the appropriate configuration after calling SFO\_MepDis\_V5(n) to seed the MEP\_ScaleFactor[n] and prior to calling SFO\_MepEn\_V5(n).

• MEP diagnostics logic uses SYSCLKOUT and not TBCLK. Hence, the SYSCLKOUT restriction is an important constraint.

#### **Usage:**

- After calling SFO\_MepDis(n) to seed MEP\_ScaleFactor[n], and prior to using the SFO\_MepEn\_V5(n) function in *SFO\_TI\_Build\_V5B.lib*, the HRCNFG register must be initialized with the desired HRPWM configuration. Otherwise, calibration will not be initiated, and calls to SFO\_MepEn\_V5(n) will continuously return zero.
- The SFO\_MepEn\_V5(n) function requires a starting scale factor value, MEP\_ScaleFactor[0]. MEP\_ScaleFactor[0] needs to be initialized to a typical MEP step size value. To do this, SFO\_MepDis\_V5(n) can be run on an ePWM channel while the HRPWM is disabled, and the resulting MEP\_ScaleFactor[n] value can be copied into MEP\_ScaleFactor[0].
- If there are drastic environmental changes to your system (i.e. temperature/voltage), it is generally a good idea to re-seed MEP\_ScaleFactor[0] with a new typical MEP step size value for the changed conditions.
- Because SFO\_MepEn\_V5(n) can be run on only one channel at a time, it is only recommended for systems where there are no spare HRPWM channels available, so SFO calibration must be performed on all channels with HRPWM capabilities enabled. In this case, a 6-cycle MEP inactivity zone exists at the start of each PWM period on all HRPWM channels. See Section [15.2.2.10.5.3](#page-72-2) on duty cycle range limitation.
- The function returns:
  - A 1 when it has finished SFO calibration for the current channel
  - A 0 when SFO diagnostics are still running for the channel
  - A 2 as an error indicator after calibration has completed if the resulting MEP\_ScaleFactor for
- (1) If SFO calibration must be run on multiple channels at a time while HRPWM capabilities are enabled, the previous version of the SFO library, SFO\_TI\_Build.lib, which uses more memory resources, can be used instead, and SFO\_MepEn(n) can run concurrently for up to 4 ePWM channels with HRPWM enabled.

#### **Table 15-46. SFO V5 Library Routines (continued)**

#### **Function Description**

the channel differs from the original MEP\_ScaleFactor[0] seed value by more than +/- 15. The function must be called repetitively before it will return a 1. This takes a longer time to complete than the SFO\_MepDis\_V5(n) calibration.

If it returns a 2, the MEP\_ScaleFactor for the channel has finished updating and is outside the typical drift range of MEP\_ScaleFactor[0] +/-15 even with large temperature and voltage variations. If the reason for the large difference between the seed and the channel scale factor is known and acceptable, the user may choose to ignore the return of 2, and treat it as a return value of 1, indicating that calibration is complete.

Otherwise, if the large difference is unexpected, there are steps to take to remedy the error:

- 1. Check your code to ensure SFO\_MepEn\_V5(n) is not being called on more than one channel at a time.
- 2. If the above is not effective, run SFO\_MepDis\_V5(n) again and re-seed Mep\_ScaleFactor[0].
- 3. If neither of the above 2 steps work, there may be a system problem. The application firmware should perform a shutdown or an appropriate recovery procedure.
- If all ePWM modules are using the same TBCLK prescaler, then it is possible to run the SFO\_MepEn\_V5(n) function for only one ePWM module and to use the MEP\_ScaleFactor value for that module for the other modules also. In this case only one ePWM module incurs the 6-cycle duty limitation, and the remaining modules incur only a 3-cycle minimum duty limitation. This assumes that all HRPWM modules' MEP steps are similar but may not be identical.

#### *15.2.2.10.6.5 SFO\_TI\_Build\_V5 Software Usage*

Software library functions int SFO\_MepEn\_V5(int n) and int SFO\_MepDis\_V5(int n) calculate the MEP scale factor for ePWMn modules, where n= the ePWM channel number. The scale factor value, which represents the number of micro-steps available in a system clock period, is returned in a global array of integer values called MEP\_ScaleFactor[x], where x is the maximum number of HRPWM channels for a device plus one. For example, if the maximum number of HRPWM channels for a device is 16, the scale factor array would be MEP\_ScaleFactor[17]. Both SFO\_MepEn\_V5 and SFO\_MepDis\_V5 themselves also return a 1 when calibration has completed, indicating the MEP\_ScaleFactor has been successfully updated for the channel, and a 0 when calibration is still on-going. A return of 2 represents an out-of-range error.

**Table 15-47. Software Functions**

| Software functional calls   | Functional Description                                                  |
|-----------------------------|-------------------------------------------------------------------------|
| int SFO_MepDis_V5(int n)(1) |                                                                         |
| status = SFO_MepDis_V5(1)   | The scale factor in MEP_ScaleFactor[1] is updated when status = 1       |
| status = SFO_MepDis_V5(2)   | The scale factor in MEP_ScaleFactor[2] is updated when status = 1       |
|                             |                                                                         |
| status = SFP_MepDis_V5(16)  | The scale factor in MEP_ScaleFactor[16] is updated when status = 1 or 2 |
| int SFO_MepEn_V5(int n)(1)  |                                                                         |
| status = SFO_MepEn_V5(1);   | The scale factor in MEP_ScaleFactor[1] is updated when status = 1 or 2  |
| status = SFO_MepEn_V5(2);   | The scale factor in MEP_ScaleFactor[2] is updated when status = 1 or 2  |
|                             |                                                                         |
| status = SFO_MepEn_V5(16);  | The scale factor in MEP_ScaleFactor[16] is updated when status = 1 or 2 |

<sup>(1)</sup> MEP\_ScaleFactor[0] is a starting seed value used by the SFO software functions internally.

To use the HRPWM feature of the ePWMs, it is recommended that the SFO functions in TI\_Build\_V5.lib be used as described here. For different devices that may have fewer HRPWM channels, modifications will be required in Step 1 and Step 2.

**NOTE:** The following example assumes there are four ePWM instances that contain the HRPWM submodule in the device. See [Section](#page-3-0) 15.1.2 to determine the number of ePWM instances that contain the HRPWM submodule.

#### **Step 1. Add Include Files**

The SFO\_V5.h file needs to be included as follows. This include file is mandatory when using the SFO V5 library functions.

## *Example 15-7. A Sample of How to Add Include Files*

#### **Step 2. Define Number of HRPWM Channels Used**

In the SFO\_V5.h file, the maximum number of HRPWM's used for a particular device must be defined. PWM\_CH must equal the number of HRPWM channels plus 1.

To save static variable memory, fewer than the maximum number of HRPWM channels may be defined with some caution. To do this, PWM\_CH can be set to the largest ePWM channel number plus 1. For instance, if only ePWM1A and ePWM2A channels are required as HRPWM channels, PWM\_CH can be set to 3. However, if only ePWM15A and ePWM16A channels are required as HRPWM channels, PWM\_CH must still be set to 17.

### *Example 15-8. Defining Number of HRPWM Channels Used (Plus 1)*

```
// SFO_V5.H
// NOTE: THIS IS A VERY IMPORTANT STEP. PWM_CH MUST BE DEFINED FIRST BEFORE
// BUILDING CODE.
#define PWM_CH 17 // Maximum of 16 HRPWM channels (16+1=17)
```

#### **Step 3. Element Declaration**

Declare an array of integer variables with a length equal to PWM\_CH, and an array of pointers to EPWM register structures. The array of pointers will include pointers for up to 16 EPWM register structures plus one dummy pointer in location EPWM[0] for a device with 16 EPWM channels. Likewise, it will include pointers for up to 4 EPWM register structures plus 1 for a device with 4 EPWM registers and up to 3 EPWM register structures plus 1 for a device with 3 EPWM registers.

#### *Example 15-9. Declaring Elements Required by SFO\_TI\_Build\_V5.lib*

```
int MEP_ScaleFactor[PWM_CH] = {0,0,0,0,0, // Scale factor values for ePWM 1-16
                                 0,0,0,0, // and MEP_ScaleFactor[0]
                                 0,0,0,0, // For fewer HRPWM channels, there
                                 0,0,0,0};// will be fewer zeros initialized.
// Declare a volatile array of pointers to EPWM Register structures.
// Only point to registers that exist. If a device has only 6 EPWMs (PWM_CH is 7),
// the array will include pointers for up to 6 EPWM register structures plus one
// dummy pointer in the ePWM[0] location.
volatile struct EPWM_REGS *ePWM[PWM_CH] {&EPwm1Regs, &EPwm1Regs, &EPwm2Regs,
&EPwm3Regs, &EPwm4Regs, &EPwm5Regs, &EPwm6Regs, &EPwm7Regs,
&EPwm8Regs, &EPwm9Regs, &EPwm10Regs, &EPwm11Regs, &EPwm12Regs,
&EPwm13Regs, &EPwm14Regs, &EPwm15Regs, &EPwm16Regs};
```

#### **Step 4. MEP\_ScaleFactor Initialization**

After power up, the SFO\_MepEn\_V5(n) function needs a typical scale factor starting seed value in MEP\_ScaleFactor[0]. This value can be conveniently determined using one of the ePWM modules to run the SFO\_MepDis\_V5(n) function prior to initializing the PWM settings for the application. The SFO\_MepDis\_V5(n) function does not require a starting scale factor value.

As part of the one-time initialization code, include the following:

#### *Example 15-10. Initialization With a Scale Factor Value*

```
// MEP_ScaleFactor variables initialized using function SFO_MepDis_V5
Uint16 I;
for (I=1; i<PWM_CH; I++) // For channels 1-16
{
   while (SFO_MepDis_V5(I)==0); // Calls MepDis until MEP_ScaleFactor updated
}
// Initialize MEP_ScaleFactor[0] with a typical MEP seed value
// required for SFO_MepEn_V5
   MEP_ScaleFactor[0] = MEP_ScaleFactor[1];
```

#### **Step 5. Application Code**

While the application is running, fluctuations in both device temperature and supply voltage may be expected. To be sure that optimal scale factors are used for each ePWM module, the SFO function should be re-run periodically as part of a slower background loop. Some examples of this are shown here.

## *Example 15-11. SFO Function Calls*

```
main()
{
  Uint16 current_ch = 1; // keeps track of current HRPWM channel being calibrated
  Uint16 status;
 // User code
 // Case 1: All ePWMs are running in HRPWM mode
 // Here, the minimum duty cycle limitation is 6 clock cycles.
      status = SFO_MepEn_V5(current_ch); // MepEn called here
      if (status ==1) // if MEP_ScaleFactor has been updated
      {
          current_ch++; // move on to the next channel
      }
      else if (status==2) // if MEP_ScaleFactor differs from
      { // MEP_ScaleFactor[0] seed by more than
           error(); // +/-15, flag an error
      }
      if (current_ch==PWM_CH) // if last channel has been reached
      {
          current_ch = 1; // go back to channel 1
      }
 // Case 2: all ePWMs except one are running in HRPWM mode.
 // One of the ePWM channels (ePWM16 in this example) is used
 // for SFO_MepDis_V5 scale factor calibration.
 // Here, the minimum duty cycle limitation is 3 clock cycles.
 // HRPWM16 diagnostics circuitry is used to estimate the MEP steps
 // with the assumption that all HRPWM channels behave similarly
 // though they may not be identical.
   while (SFO_MepDis_V5(16)==0); // wait until MEP_ScaleFactor[16] updated
   for (I=1; I<(PWM_CH-1); I++) // Update scale factors for ePWM 1-15.
   {
       MEP_ScaleFactor[i] = MEP_ScaleFactor[16];
```

#### **15.2.2.11 ePWM Behavior During Emulation**

To configure the ePWM to stop during emulation suspend events (for example, debugger breakpoints), set up the ePWM and the Debug Subsystem:

- 1. Set TBCTL.FREE\_SOFT= 0 or 1 (see register description for more details). This will allow the Suspend\_Control signal from the Debug Subsystem (Chapter 27) to stop and start the ePWM. Note that if FREE\_SOFT = 2 or 3, the Suspend\_Control signal is ignored and the ePWM is free running regardless of any debug suspend event. This FREE\_SOFT bit gives local control from a module perspective to gate the suspend signal coming from the Debug Subsystem.
- 2. Set the appropriate xxx\_Suspend\_Control register = 0x9, as described in Section 27.1.1.1, *Debug Suspend Support for Peripherals*. Choose the register appropriate to the peripheral you want to suspend during a suspend event.

### *15.2.3 Use Cases*

An ePWM module has all the local resources necessary to operate completely as a standalone module or to operate in synchronization with other identical ePWM modules.

#### **15.2.3.1 Overview of Multiple Modules**

Previously in this user's guide, all discussions have described the operation of a single module. To facilitate the understanding of multiple modules working together in a system, the ePWM module described in reference is represented by the more simplified block diagram shown in [Figure](#page-85-0) 15-55. This simplified ePWM block shows only the key resources needed to explain how a multiswitch power topology is controlled with multiple ePWM modules working together.

**Figure 15-55. Simplified ePWM Module**

```
Figure 8 — Sync/phase control (concept)

+--------------------------------------------------------+
|                     ePWMx module                       |
|                                                        |
|  Phase reg (Φ = 0°)                                    |
|      +----------------+                                |
|      |   Φ = 0°       |<---(EN switch)---+             |
|      +----------------+                  |             |
|                                          |             |
|                                   +------+------+ \    |
|  SyncIn (from top) -------------->|   SyncIn     | \   |
|                                   +------+------+  /o---+
|                                          |             |
|                                          v             |
|                                    (internal node)     |
|                                                        |
|  SyncOut select:                                       |
|     o  CTR = 0                                         |
|     o  CTR = CMPB                                      |
|     o  X (disabled)                                    |
|        \____(select switch)____/                       |
|                        |                               |
|                        v                               |
|                     SyncOut (bottom)                   |
+--------------------------------------------------------+

Outputs (right side):
  EPWMxA  --------------------------------------------->
  EPWMxB  --------------------------------------------->
```

#### **15.2.3.2 Key Configuration Capabilities**

The key configuration choices available to each module are as follows:

- Options for SyncIn
  - Load own counter with phase register on an incoming sync strobe—enable (EN) switch closed
  - Do nothing or ignore incoming sync strobe—enable switch open
  - Sync flow-through SyncOut connected to SyncIn
  - Master mode, provides a sync at PWM boundaries—SyncOut connected to CTR = PRD
  - Master mode, provides a sync at any programmable point in time—SyncOut connected to CTR = CMPB
  - Module is in standalone mode and provides No sync to other modules—SyncOut connected to X (disabled)
- Options for SyncOut
  - Sync flow-through SyncOut connected to SyncIn
  - Master mode, provides a sync at PWM boundaries—SyncOut connected to CTR = PRD
  - Master mode, provides a sync at any programmable point in time—SyncOut connected to CTR = CMPB
  - Module is in standalone mode and provides No sync to other modules—SyncOut connected to X (disabled)

For each choice of SyncOut, a module may also choose to load its own counter with a new phase value on a SyncIn strobe input or choose to ignore it, i.e., via the enable switch. Although various combinations are possible, the two most common—master module and slave module modes—are shown in [Figure](#page-86-0) 15- [56](#page-86-0).

**Figure 15-56. EPWM1 Configured as a Typical Master, EPWM2 Configured as a Slave**

```
Figure 19 — Master/Slave sync concept (external SyncIn optional)

                 Ext SyncIn (optional)
                         |
                         v
+---------------------------------------------------+
| Master (1)                                        |
|                                                   |
|  Phase reg (Φ = 0°)                               |
|      +----------------+                           |
|      |   Φ = 0°       |<---(EN switch)---+        |
|      +----------------+                  |        |
|                                         (•)-------+------->
|  SyncIn ------------------------------->(node)    |
|                                                   |
|  SyncOut select (mux):                            |
|     o  CTR = 0                                    |
|     o  CTR = CMPB                                 |
|     o  X (disabled)                               |
|                 |                                 |
|                 v                                 |
|              SyncOut  ----------------------------+----> (to slaves / chain)
|                                                   |
| Outputs:                                          |
|   EPWM1A  --------------------------------------->|
|   EPWM1B  --------------------------------------->|
+---------------------------------------------------+

Slave ePWM module (2) with SyncIn, phase enable, and SyncOut select

(from previous module's SyncOut) -------------------------------> SyncIn
                                                                  |
                                                                  v
+-----------------------------------------------------------------+
| Slave (2)                                                       |
|                                                                 |
|  Phase reg (Φ = 0°)                                             |
|     +----------------+                                          |
|     |   Φ = 0°       |<---(EN switch)---+                       |
|     +----------------+                  |                       |
|                                         (•) <---- SyncIn        |
|                                          |                      |
|                                          |                      |
|  SyncOut select (mux):                   |                      |
|     o  CTR = 0 --------------------------+                      |
|     o  CTR = CMPB -----------------------+-----> SyncOut -------+----> (to next module)
|     o  X (disabled)                      |                      |
|                                                                 |
| Outputs (right):                                                |
|   EPWM2A  -------------------------------------------------->   |
|   EPWM2B  -------------------------------------------------->   |
+-----------------------------------------------------------------+

(Left side of the picture also shows the previous module outputs EPWM1A/EPWM1B and its SyncOut feeding this slave's SyncIn.)
```

#### **15.2.3.3 Controlling Multiple Buck Converters With Independent Frequencies**

One of the simplest power converter topologies is the buck. A single ePWM module configured as a master can control two buck stages with the same PWM frequency. If independent frequency control is required for each buck converter, then one ePWM module must be allocated for each converter stage. [Figure](#page-87-0) 15-57 shows four buck stages, each running at independent frequencies. In this case, all four ePWM modules are configured as Masters and no synchronization is used. [Figure](#page-88-0) 15-58 shows the waveforms generated by the setup shown in [Figure](#page-87-0) 15-57; note that only three waveforms are shown, although there are four stages.

> *[Diagram — signal/block labels not renderable in text format]*

**Figure 15-57. Control of Four Buck Stages. Here FPWM1≠ FPWM2≠ FPWM3≠ FPWM4**

NOTE: Θ = X indicates value in phase register is a "don't care"

**Figure 15-58. Buck Waveforms for [Figure](#page-87-0) 15-57 (Note: Only three bucks shown here)**

```
Figure 4 — Examples of AQ actions (P and CA) for EPWM1A / EPWM2A / EPWM3A

Legend (as shown by symbols):
- P ↓  : clear output at Period event (P)
- P I  : Period marker (reference)
- CA ▲ : set output at Compare A event (CA)

----------------------------------------------------------------------
EPWM1A (example values marked: 700, 1200)

TBCNT (up-count with wrap at P)
P      CA(700)        P(1200)           CA            P
|        |              |                |             |
|/------●---------------●\---------------●-------------●\-----> t
        700            1200

EPWM1A output
___________|‾‾‾‾‾‾‾‾‾|___________________|‾‾‾‾‾‾‾‾‾|________
          set@CA      clr@P             set@CA      clr@P

----------------------------------------------------------------------
EPWM2A (example values marked: 700, 1400)

P      CA(700)                 P(1400)          CA
|        |                       |              |
|/------●------------------------●\-------------●\---------> t
        700                     1400

EPWM2A output
_____|‾‾‾‾‾‾‾‾‾‾‾‾‾|______________|‾‾‾‾‾‾‾‾‾‾‾‾‾|______
     clr@P   set@CA               clr@P   set@CA

----------------------------------------------------------------------
EPWM3A (example values marked: 500, 800; more frequent wraps)

CA     P      CA(500)   P(800)   CA      P      CA      P
|      |        |         |      |       |      |       |
/●\____●\_______●\________●\_____●\______●\_____●\______●\----> t
      500      800

EPWM3A output
_____|‾‾|_________|‾‾|_________|‾‾|_________|‾‾|____________________
     set@CA clr@P  set@CA clr@P  set@CA clr@P  set@CA clr@P
```
<sup>P</sup> Indicates this event triggers an interrupt <sup>I</sup>

#### **Table 15-48. EPWM1 Initialization for [Figure](#page-88-0) 15-58**

| Register | Bit       | Value           | Comments                   |
|----------|-----------|-----------------|----------------------------|
| TBPRD    | TBPRD     | 1200 (4B0h)     | Period = 1201 TBCLK counts |
| TBPHS    | TBPHS     | 0               | Clear Phase Register to 0  |
| TBCTL    | CTRMODE   | TB_UP           |                            |
|          | PHSEN     | TB_DISABLE      | Phase loading disabled     |
|          | PRDLD     | TB_SHADOW       |                            |
|          | SYNCOSEL  | TB_SYNC_DISABLE |                            |
| CMPCTL   | SHDWAMODE | CC_SHADOW       |                            |
|          | SHDWBMODE | CC_SHADOW       |                            |
|          | LOADAMODE | CC_CTR_ZERO     | Load on CTR = 0            |
|          | LOADBMODE | CC_CTR_ZERO     | Load on CTR = 0            |
| AQCTLA   | PRD       | AQ_CLEAR        |                            |
|          | CAU       | AQ_SET          |                            |

## **Table 15-49. EPWM2 Initialization for [Figure](#page-88-0) 15-58**

| Register | Bit       | Value           | Comments                   |
|----------|-----------|-----------------|----------------------------|
| TBPRD    | TBPRD     | 1400 (578h)     | Period = 1401 TBCLK counts |
| TBPHS    | TBPHS     | 0               | Clear Phase Register to 0  |
| TBCTL    | CTRMODE   | TB_UP           |                            |
|          | PHSEN     | TB_DISABLE      | Phase loading disabled     |
|          | PRDLD     | TB_SHADOW       |                            |
|          | SYNCOSEL  | TB_SYNC_DISABLE |                            |
| CMPCTL   | SHDWAMODE | CC_SHADOW       |                            |
|          | SHDWBMODE | CC_SHADOW       |                            |
|          | LOADAMODE | CC_CTR_ZERO     | Load on CTR = 0            |
|          | LOADBMODE | CC_CTR_ZERO     | Load on CTR = 0            |
| AQCTLA   | PRD       | AQ_CLEAR        |                            |
|          | CAU       | AQ_SET          |                            |

#### **Table 15-50. EPWM3 Initialization for [Figure](#page-88-0) 15-58**

| Register | Bit       | Value           | Comments                  |
|----------|-----------|-----------------|---------------------------|
| TBPRD    | TBPRD     | 800 (320h)      | Period = 801 TBCLK counts |
| TBPHS    | TBPHS     | 0               | Clear Phase Register to 0 |
| TBCTL    | CTRMODE   | TB_UP           |                           |
|          | PHSEN     | TB_DISABLE      | Phase loading disabled    |
|          | PRDLD     | TB_SHADOW       |                           |
|          | SYNCOSEL  | TB_SYNC_DISABLE |                           |
| CMPCTL   | SHDWAMODE | CC_SHADOW       |                           |
|          | SHDWBMODE | CC_SHADOW       |                           |
|          | LOADAMODE | CC_CTR_ZERO     | Load on CTR = 0           |
|          | LOADBMODE | CC_CTR_ZERO     | Load on CTR = 0           |
| AQCTLA   | PRD       | AQ_CLEAR        |                           |
|          | CAU       | AQ_SET          |                           |

#### *Example 15-12. Configuration for Example in [Figure](#page-88-0) 15-58*

```
// Run Time (Note: Example execution of one run-time instance)
//=========================================================
   EPwm1Regs.CMPA.half.CMPA = 700; // adjust duty for output EPWM1A
   EPwm2Regs.CMPA.half.CMPA = 700; // adjust duty for output EPWM2A
   EPwm3Regs.CMPA.half.CMPA = 500; // adjust duty for output EPWM3A
```

#### **15.2.3.4 Controlling Multiple Buck Converters With Same Frequencies**

If synchronization is a requirement, ePWM module 2 can be configured as a slave and can operate at integer multiple (N) frequencies of module 1. The sync signal from master to slave ensures these modules remain locked. [Figure](#page-90-0) 15-59 shows such a configuration; [Figure](#page-91-0) 15-60 shows the waveforms generated by the configuration.

CTR=0 CTR=CMPB X En Φ=0° SyncOut Phase reg Ext SyncIn (optional) EPWM1A EPWM1B SyncOut Phase reg CTR=CMPB CTR=0 X Φ=X En EPWM2B EPWM2A Slave Master Buck #1 Vin1 Vout1 EPWM1A Buck #2 Vin2 EPWM1B Vout2 Buck #4 Buck #3 Vin3 EPWM2B Vin4 EPWM2A Vout3 Vout4 SyncIn SyncIn **Figure 15-59. Control of Four Buck Stages. (Note: FPWM2 = N × FPWM1)**

200 400 600 400 200 300 500 300 500 EPWM1A EPWM1B EPWM2B EPWM2A Z I A CA P CA Z I Z I A CA P CA CB CB CB CB CA CA CA CA CB CB CB CB

**Figure 15-60. Buck Waveforms for [Figure](#page-90-0) 15-59 (Note: FPWM2 = FPWM1))**

## **Table 15-51. EPWM1 Initialization for [Figure](#page-90-0) 15-59**

| Register | Bit       | Value       | Comments                   |  |
|----------|-----------|-------------|----------------------------|--|
| TBPRD    | TBPRD     | 600 (258h)  | Period = 1200 TBCLK counts |  |
| TBPHS    | TBPHS     | 0           | Clear Phase Register to 0  |  |
| TBCTL    | CTRMODE   | TB_UPDOWN   |                            |  |
|          | PHSEN     | TB_DISABLE  | Phase loading disabled     |  |
|          | PRDLD     | TB_SHADOW   |                            |  |
|          | SYNCOSEL  | TB_CTR_ZERO | Sync down-stream module    |  |
| CMPCTL   | SHDWAMODE | CC_SHADOW   |                            |  |
|          | SHDWBMODE | CC_SHADOW   |                            |  |
|          | LOADAMODE | CC_CTR_ZERO | Load on CTR = 0            |  |
|          | LOADBMODE | CC_CTR_ZERO | Load on CTR = 0            |  |
| AQCTLA   | CAU       | AQ_SET      | Set actions for EPWM1A     |  |
|          | CAD       | AQ_CLEAR    |                            |  |
| AQCTLB   | CBU       | AQ_SET      | Set actions for EPWM1B     |  |
|          | CBD       | AQ_CLEAR    |                            |  |

#### **Table 15-52. EPWM2 Initialization for [Figure](#page-90-0) 15-59**

| Register | Bit       | Value       | Comments                   |
|----------|-----------|-------------|----------------------------|
| TBPRD    | TBPRD     | 600 (258h)  | Period = 1200 TBCLK counts |
| TBPHS    | TBPHS     | 0           | Clear Phase Register to 0  |
| TBCTL    | CTRMODE   | TB_UPDOWN   |                            |
|          | PHSEN     | TB_ENABLE   | Phase loading enabled      |
|          | PRDLD     | TB_SHADOW   |                            |
|          | SYNCOSEL  | TB_SYNC_IN  | Sync flow-through          |
| CMPCTL   | SHDWAMODE | CC_SHADOW   |                            |
|          | SHDWBMODE | CC_SHADOW   |                            |
|          | LOADAMODE | CC_CTR_ZERO | Load on CTR = 0            |
|          | LOADBMODE | CC_CTR_ZERO | Load on CTR = 0            |
| AQCTLA   | CAU       | AQ_SET      | Set actions for EPWM2A     |
|          | CAD       | AQ_CLEAR    |                            |
| AQCTLB   | CBU       | AQ_SET      | Set actions for EPWM2B     |
|          | CBD       | AQ_CLEAR    |                            |

#### *Example 15-13. Code Snippet for Configuration in [Figure](#page-90-0) 15-59*

```
// Run Time (Note: Example execution of one run-time instance)
//===========================================================
    EPwm1Regs.CMPA.half.CMPA = 400; // adjust duty for output EPWM1A
    EPwm1Regs.CMPB = 200; // adjust duty for output EPWM1B
    EPwm2Regs.CMPA.half.CMPA = 500; // adjust duty for output EPWM2A
    EPwm2Regs.CMPB = 300; // adjust duty for output EPWM2B
```

#### **15.2.3.5 Controlling Multiple Half H-Bridge (HHB) Converters**

Topologies that require control of multiple switching elements can also be addressed with these same ePWM modules. It is possible to control a Half-H bridge stage with a single ePWM module. This control can be extended to multiple stages. [Figure](#page-93-0) 15-61 shows control of two synchronized Half-H bridge stages where stage 2 can operate at integer multiple (N) frequencies of stage 1. [Figure](#page-94-0) 15-62 shows the waveforms generated by the configuration shown in [Figure](#page-93-0) 15-61.

Module 2 (slave) is configured for Sync flow-through; if required, this configuration allows for a third Half-H bridge to be controlled by PWM module 3 and also, most importantly, to remain in synchronization with master module 1.

```
Figure 6 — Master/Slave synchronized PWMs driving two power stages

                 Ext SyncIn (optional)
                         |
                         v
+-------------------------------+           +----------------------------------+
| Master ePWM (EPWM1)           |           | Power stage #1                   |
|                               |           |                                  |
| Phase reg (Φ = 0°), En        |           |   VDC_bus                        |
| SyncIn  -------------------+  |           |     |                            |
| SyncOut select:            |  |           |    [HS switch]  <--- EPWM1A      |
|   CTR = 0                  |  |           |     |                            |
|   CTR = CMPB               |  |           |     +---- transformer ----+      |
|   X (disabled)             |  |           |     |                     |      |
| SyncOut -------------------+--+---------> |    [LS switch]  <--- EPWM1B      |
|                               |           |     |                     |      |
| Outputs:                      |           |    GND                    |      |
|   EPWM1A ---------------------+---------->|               diode bridge + LC -> Vout1
|   EPWM1B ---------------------+---------->|                                  |
+-------------------------------+           +----------------------------------+

+-------------------------------+          +-----------------------------------+
| Slave ePWM (EPWM2)            |          | Power stage #2                    |
|                               |          |                                   |
| Phase reg (Φ = 0°), En        |          |   VDC_bus                         |
| SyncIn  <---------------------+----------|     |                             |
| SyncOut select:               |          |    [HS switch]  <--- EPWM2A       |
|   CTR = 0                     |          |     |                             |
|   CTR = CMPB                  |          |     +---- transformer ----+       |
|   X (disabled)                |          |     |                    |        |
| SyncOut ----------------------+--------->|    [LS switch]  <--- EPWM2B       |
|                               |          |     |                    |        |
| Outputs:                      |          |    GND                   |        |
|   EPWM2A ---------------------+--------->|               diode bridge + LC -> Vout2
|   EPWM2B ---------------------+--------->|                                   |
+-------------------------------+          +-----------------------------------+
```

**Figure 15-61. Control of Two Half-H Bridge Stages (FPWM2 = N × FPWM1)**

EPWM1A EPWM1B EPWM2A EPWM2B 600 200 400 400 200 250 <sup>500</sup> <sup>500</sup> 250 A Z CB CA Z I Z I Z I A Z CB CA Z A CA CB Pulse Center Z A CA CB A Z CB CA A Z CB CA A CA CB Z A CA CB Z

Pulse Center

**Figure 15-62. Half-H Bridge Waveforms for [Figure](#page-93-0) 15-61 (Note: Here FPWM2 = FPWM1 )**

#### **Table 15-53. EPWM1 Initialization for [Figure](#page-93-0) 15-61**

| Register | Bit       | Value       | Comments                   |  |
|----------|-----------|-------------|----------------------------|--|
| TBPRD    | TBPRD     | 600 (258h)  | Period = 1200 TBCLK counts |  |
| TBPHS    | TBPHS     | 0           | Clear Phase Register to 0  |  |
| TBCTL    | CTRMODE   | TB_UPDOWN   |                            |  |
|          | PHSEN     | TB_DISABLE  | Phase loading disabled     |  |
|          | PRDLD     | TB_SHADOW   |                            |  |
|          | SYNCOSEL  | TB_CTR_ZERO | Sync down-stream module    |  |
| CMPCTL   | SHDWAMODE | CC_SHADOW   |                            |  |
|          | SHDWBMODE | CC_SHADOW   |                            |  |
|          | LOADAMODE | CC_CTR_ZERO | Load on CTR = 0            |  |
|          | LOADBMODE | CC_CTR_ZERO | Load on CTR = 0            |  |
| AQCTLA   | ZRO       | AQ_SET      | Set actions for EPWM1A     |  |
|          | CAU       | AQ_CLEAR    |                            |  |
| AQCTLB   | ZRO       | AQ_CLEAR    | Set actions for EPWM1B     |  |
|          | CAD       | AQ_SET      |                            |  |

#### **Table 15-54. EPWM2 Initialization for [Figure](#page-93-0) 15-61**

| Register | Bit       | Value       | Comments                   |  |
|----------|-----------|-------------|----------------------------|--|
| TBPRD    | TBPRD     | 600 (258h)  | Period = 1200 TBCLK counts |  |
| TBPHS    | TBPHS     | 0           | Clear Phase Register to 0  |  |
| TBCTL    | CTRMODE   | TB_UPDOWN   |                            |  |
|          | PHSEN     | TB_ENABLE   | Phase loading enabled      |  |
|          | PRDLD     | TB_SHADOW   |                            |  |
|          | SYNCOSEL  | TB_SYNC_IN  | Sync flow-through          |  |
| CMPCTL   | SHDWAMODE | CC_SHADOW   |                            |  |
|          | SHDWBMODE | CC_SHADOW   |                            |  |
|          | LOADAMODE | CC_CTR_ZERO | Load on CTR = 0            |  |
|          | LOADBMODE | CC_CTR_ZERO | Load on CTR = 0            |  |
| AQCTLA   | ZRO       | AQ_SET      | Set actions for EPWM2A     |  |
|          | CAU       | AQ_CLEAR    |                            |  |
| AQCTLB   | ZRO       | AQ_CLEAR    | Set actions for EPWM2B     |  |
|          | CAD       | AQ_SET      |                            |  |

#### *Example 15-14. Code Snippet for Configuration in [Figure](#page-93-0) 15-61*

```
// Run Time (Note: Example execution of one run-time instance)
//===========================================================
    EPwm1Regs.CMPA.half.CMPA = 400; // adjust duty for output EPWM1A
    EPwm1Regs.CMPB = 200; // adjust duty for output EPWM1B
    EPwm2Regs.CMPA.half.CMPA = 500; // adjust duty for output EPWM2A
    EPwm2Regs.CMPB = 250; // adjust duty for output EPWM2B
```

#### **15.2.3.6 Controlling Dual 3-Phase Inverters for Motors (ACI and PMSM)**

The idea of multiple modules controlling a single power stage can be extended to the 3-phase Inverter case. In such a case, six switching elements can be controlled using three PWM modules, one for each leg of the inverter. Each leg must switch at the same frequency and all legs must be synchronized. A master + two slaves configuration can easily address this requirement. [Figure](#page-96-0) 15-63 shows how six PWM modules can control two independent 3-phase Inverters; each running a motor.

As in the cases shown in the previous sections, we have a choice of running each inverter at a different frequency (module 1 and module 4 are masters as in [Figure](#page-96-0) 15-63), or both inverters can be synchronized by using one master (module 1) and five slaves. In this case, the frequency of modules 4, 5, and 6 (all equal) can be integer multiples of the frequency for modules 1, 2, 3 (also all equal).

**Figure 15-63. Control of Dual 3-Phase Inverter Stages as Is Commonly Used in Motor Control**

**Figure 15-64. 3-Phase Inverter Waveforms for [Figure](#page-96-0) 15-63 (Only One Inverter Shown)**

```
Figure 4 — Three synchronized ePWM modules (up-down count) with CA events and dead-band (RED/FED)

Top ramp in each row = TBCNT (up-down triangle). Boxes mark events:
- ZI : sync at counter zero (sync-in marker)
- CA↑ : CA compare on up-count (set / rising edge source)
- PA  : peak/period point at top of triangle (TBPRD)
- CA↓ : CA compare on down-count (clear / falling edge source)
Dead-band markers:
- RED = rising edge delay
- FED = falling edge delay

================================================================================
EPWM1 (Φ = 0)
TBCNT:     ZI          CA↑      PA(800)      CA↓           ZI          CA↑   PA      CA↓
          |v|---------●---------▲-----------●-------------|v|---------●------▲------●---->

EPWM1A:    ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|____________________ ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|____
              ^RED (delayed rise)        (fall)             ^RED

EPWM1B:    ‾‾‾‾|_____________________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾ ‾‾‾‾|_____________|‾‾‾‾
                    ^FED (delayed fall)                              ^FED

(Values labeled on the triangle sides: 500, 800, 500)

================================================================================
EPWM2 (Φ2 = 0)
TBCNT:     ZI          CA↑        (600)       CA↓           ZI          CA↑        CA↓
          |v|---------●----------------------●-------------|v|---------●---------●----->

EPWM2A:    ____|‾‾‾‾‾‾‾‾‾‾|___________________________ ____|‾‾‾‾‾‾‾‾‾‾|__________
              ^RED                              (fall)          ^RED

EPWM2B:    ‾‾‾‾|___________________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾ ‾‾‾‾|_____________|‾‾‾‾
                    ^FED                                         ^FED

================================================================================
EPWM3 (Φ3 = 0)
TBCNT:     ZI        CA↑   CA↓               ZI        CA↑   CA↓
          |v|-------●-----●-----------------|v|-------●-----●------------------>

EPWM3A:    ____|‾‾|____________________________ ____|‾‾|________________________
            ^RED (delayed rise)   (early fall)      ^RED

EPWM3B:    ‾‾‾‾|_____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾ ‾‾‾‾|_____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
                ^FED                                         ^FED

(Values labeled on EPWM3 triangle: 700 and 700)
```
#### **Table 15-55. EPWM1 Initialization for [Figure](#page-96-0) 15-63**

| Register | Bit       | Value          | Comments                   |
|----------|-----------|----------------|----------------------------|
| TBPRD    | TBPRD     | 800 (320h)     | Period = 1600 TBCLK counts |
| TBPHS    | TBPHS     | 0              | Clear Phase Register to 0  |
| TBCTL    | CTRMODE   | TB_UPDOWN      |                            |
|          | PHSEN     | TB_DISABLE     | Phase loading disabled     |
|          | PRDLD     | TB_SHADOW      |                            |
|          | SYNCOSEL  | TB_CTR_ZERO    | Sync down-stream module    |
| CMPCTL   | SHDWAMODE | CC_SHADOW      |                            |
|          | SHDWBMODE | CC_SHADOW      |                            |
|          | LOADAMODE | CC_CTR_ZERO    | Load on CTR = 0            |
|          | LOADBMODE | CC_CTR_ZERO    | Load on CTR = 0            |
| AQCTLA   | CAU       | AQ_SET         | Set actions for EPWM1A     |
|          | CAD       | AQ_CLEAR       |                            |
| DBCTL    | MODE      | DB_FULL_ENABLE | Enable Dead-band module    |
|          | POLSEL    | DB_ACTV_HIC    | Active Hi complementary    |
| DBFED    | DBFED     | 50             | FED = 50 TBCLKs            |
|          | DBRED     | 50             | RED = 50 TBCLKs            |

#### **Table 15-56. EPWM2 Initialization for [Figure](#page-96-0) 15-63**

| Register | Bit       | Value          | Comments                   |
|----------|-----------|----------------|----------------------------|
| TBPRD    | TBPRD     | 800 (320h)     | Period = 1600 TBCLK counts |
| TBPHS    | TBPHS     | 0              | Clear Phase Register to 0  |
| TBCTL    | CTRMODE   | TB_UPDOWN      |                            |
|          | PHSEN     | TB_ENABLE      | Slave module               |
|          | PRDLD     | TB_SHADOW      |                            |
|          | SYNCOSEL  | TB_SYNC_IN     | Sync flow-through          |
| CMPCTL   | SHDWAMODE | CC_SHADOW      |                            |
|          | SHDWBMODE | CC_SHADOW      |                            |
|          | LOADAMODE | CC_CTR_ZERO    | Load on CTR = 0            |
|          | LOADBMODE | CC_CTR_ZERO    | Load on CTR = 0            |
| AQCTLA   | CAU       | AQ_SET         | Set actions for EPWM2A     |
|          | CAD       | AQ_CLEAR       |                            |
| DBCTL    | MODE      | DB_FULL_ENABLE | Enable Dead-band module    |
|          | POLSEL    | DB_ACTV_HIC    | Active Hi complementary    |
| DBFED    | DBFED     | 50             | FED = 50 TBCLKs            |
|          | DBRED     | 50             | RED = 50 TBCLKs            |

#### **Table 15-57. EPWM3 Initialization for [Figure](#page-96-0) 15-63**

| Register | Bit       | Value          | Comments                   |
|----------|-----------|----------------|----------------------------|
| TBPRD    | TBPRD     | 800 (320h)     | Period = 1600 TBCLK counts |
| TBPHS    | TBPHS     | 0              | Clear Phase Register to 0  |
| TBCTL    | CTRMODE   | TB_UPDOWN      |                            |
|          | PHSEN     | TB_ENABLE      | Slave module               |
|          | PRDLD     | TB_SHADOW      |                            |
|          | SYNCOSEL  | TB_SYNC_IN     | Sync flow-through          |
| CMPCTL   | SHDWAMODE | CC_SHADOW      |                            |
|          | SHDWBMODE | CC_SHADOW      |                            |
|          | LOADAMODE | CC_CTR_ZERO    | Load on CTR = 0            |
|          | LOADBMODE | CC_CTR_ZERO    | Load on CTR = 0            |
| AQCTLA   | CAU       | AQ_SET         | Set actions for EPWM3A     |
|          | CAD       | AQ_CLEAR       |                            |
| DBCTL    | MODE      | DB_FULL_ENABLE | Enable Dead-band module    |
|          | POLSEL    | DB_ACTV_HIC    | Active Hi complementary    |
| DBFED    | DBFED     | 50             | FED = 50 TBCLKs            |
|          | DBRED     | 50             | RED = 50 TBCLKs            |

## *Example 15-15. Code Snippet for Configuration in [Figure](#page-96-0) 15-63*

```
// Run Time (Note: Example execution of one run-time instance)
//=========================================================
EPwm1Regs.CMPA.half.CMPA = 500; // adjust duty for output EPWM1A
EPwm2Regs.CMPA.half.CMPA = 600; // adjust duty for output EPWM2A
EPwm3Regs.CMPA.half.CMPA = 700; // adjust duty for output EPWM3A
```

#### **15.2.3.7 Practical Applications Using Phase Control Between PWM Modules**

So far, none of the examples have made use of the phase register (TBPHS). It has either been set to zero or its value has been a don't care. However, by programming appropriate values into TBPHS, multiple PWM modules can address another class of power topologies that rely on phase relationship between legs (or stages) for correct operation. As described in the TB module section, a PWM module can be configured to allow a SyncIn pulse to cause the TBPHS register to be loaded into the TBCNT register. To illustrate this concept, [Figure](#page-100-0) 15-65 shows a master and slave module with a phase relationship of 120°, that is, the slave leads the master.

```

                 Ext SyncIn (optional)
                         |
                         v
+----------------------------------------------------+
| Master (1)                                         |
|                                                    |
|  Phase reg: Φ = 0°                                 |
|        +-----------+                               |
|        |  Φ = 0°   |<-- En switch --+              |
|        +-----------+                |              |
|                                     (•) <--- SyncIn|
|  SyncOut select (mux):               |             |
|     o  CTR = 0 ----------------------+             |
|     o  CTR = CMPB -------------------+----> SyncOut+----+
|     o  X (disabled)                                |    |
|                                                    |    |
| Outputs:                                           |    |
|   EPWM1A ----------------------------------------> |    |
|   EPWM1B ----------------------------------------> |    |
+----------------------------------------------------+    |
                                                          |
                                                          v
+----------------------------------------------------+
| Slave (2)                                          |
|                                                    |
|  Phase reg: Φ = 120°                               |
|        +------------+                              |
|        |  Φ = 120°  |<-- En switch --+             |
|        +------------+                |             |
|                                     (•) <--- SyncIn|
|  SyncOut select (mux):               |             |
|     o  CTR = 0 ----------------------+             |
|     o  CTR = CMPB -------------------+----> SyncOut ----> (to next)
|     o  X (disabled)                                |
|                                                    |
| Outputs:                                           |
|   EPWM2A ----------------------------------------> |
|   EPWM2B ----------------------------------------> |
+----------------------------------------------------+
```

**Figure 15-65. Configuring Two PWM Modules for Phase Control**

Figure 15-66 shows the associated timing waveforms for this configuration. Here, TBPRD = 600 for both master and slave. For the slave, TBPHS = 200 (200/600 × 360° = 120°). Whenever the master generates a SyncIn pulse (CTR = PRD), the value of TBPHS = 200 is loaded into the slave TBCNT register so the slave time-base is always leading the master's time-base by 120°.

**Figure 15-66. Timing Waveforms Associated With Phase Control Between 2 Modules**

#### **15.2.3.8 Controlling a 3-Phase Interleaved DC/DC Converter**

A popular power topology that makes use of phase-offset between modules is shown in [Figure](#page-1-0) 15-67. This system uses three PWM modules, with module 1 configured as the master. To work, the phase relationship between adjacent modules must be F = 120°. This is achieved by setting the slave TBPHS registers 2 and 3 with values of 1/3 and 2/3 of the period value, respectively. For example, if the period register is loaded with a value of 600 counts, then TBPHS (slave 2) = 200 and TBPHS (slave 3) = 400. Both slave modules are synchronized to the master 1 module.

This concept can be extended to four or more phases, by setting the TBPHS values appropriately. The following formula gives the TBPHS values for N phases:

$$TBPHS(N,M) = (TBPRD/N) \times (M - 1)$$

Where:

N = number of phases

M = PWM module number

For example, for the 3-phase case (N = 3), TBPRD = 600,

TBPHS(3,2) = (600/3) × (2 - 1) = 200 × 1 = 200 (Phase value for Slave module 2)

TBPHS(3,3) = (600/3) × (3 - 1) = 200 × 2 = 400 (Phase value for Slave module 3)

[Figure](#page-2-0) 15-68 shows the waveforms for the configuration in [Figure](#page-1-0) 15-67.

CTR=0 CTR=CMPB X En SyncOut Phase reg Ext SyncIn (optional) EPWM1A EPWM1B SyncOut Phase reg CTR=CMPB CTR=0 X En EPWM2B EPWM2A Slave Master EPWM1A SyncIn SyncIn EPWM1B CTR=0 CTR=CMPB SyncOut X EPWM3B Phase reg Slave En SyncIn EPWM3A 1 2 3 VIN EPWM2B EPWM2A EPWM3A EPWM3B <sup>V</sup>OUT Φ=0° Φ=120° Φ=240°

**Figure 15-67. Control of a 3-Phase Interleaved DC/DC Converter**

**Figure 15-68. 3-Phase Interleaved DC/DC Converter Waveforms for [Figure](#page-1-0) 15-67**

### **Table 15-58. EPWM1 Initialization for [Figure](#page-1-0) 15-67**

| Register | Bit       | Value          | Comments                  |
|----------|-----------|----------------|---------------------------|
| TBPRD    | TBPRD     | 450 (1C2h)     | Period = 900 TBCLK counts |
| TBPHS    | TBPHS     | 0              | Clear Phase Register to 0 |
| TBCTL    | CTRMODE   | TB_UPDOWN      |                           |
|          | PHSEN     | TB_DISABLE     | Phase loading disabled    |
|          | PRDLD     | TB_SHADOW      |                           |
|          | SYNCOSEL  | TB_CTR_ZERO    | Sync down-stream module   |
| CMPCTL   | SHDWAMODE | CC_SHADOW      |                           |
|          | SHDWBMODE | CC_SHADOW      |                           |
|          | LOADAMODE | CC_CTR_ZERO    | Load on CTR = 0           |
|          | LOADBMODE | CC_CTR_ZERO    | Load on CTR = 0           |
| AQCTLA   | CAU       | AQ_SET         | Set actions for EPWM1A    |
|          | CAD       | AQ_CLEAR       |                           |
| DBCTL    | MODE      | DB_FULL_ENABLE | Enable Dead-band module   |
|          | POLSEL    | DB_ACTV_HIC    | Active Hi complementary   |
| DBFED    | DBFED     | 20             | FED = 20 TBCLKs           |
|          | DBRED     | 20             | RED = 20 TBCLKs           |

### **Table 15-59. EPWM2 Initialization for [Figure](#page-1-0) 15-67**

| Register | Bit       | Value          | Comments                       |
|----------|-----------|----------------|--------------------------------|
| TBPRD    | TBPRD     | 450 (1C2h)     | Period = 900 TBCLK counts      |
| TBPHS    | TBPHS     | 300            | Phase = (300/900) × 360 = 120° |
| TBCTL    | CTRMODE   | TB_UPDOWN      |                                |
|          | PHSEN     | TB_ENABLE      | Slave module                   |
|          | PRDLD     | TB_SHADOW      |                                |
|          | SYNCOSEL  | TB_SYNC_IN     | Sync flow-through              |
|          | PHSDIR    | TB_DOWN        | Count DOWN on sync             |
| CMPCTL   | SHDWAMODE | CC_SHADOW      |                                |
|          | SHDWBMODE | CC_SHADOW      |                                |
|          | LOADAMODE | CC_CTR_ZERO    | Load on CTR = 0                |
|          | LOADBMODE | CC_CTR_ZERO    | Load on CTR = 0                |
| AQCTLA   | CAU       | AQ_SET         | Set actions for EPWM2A         |
|          | CAD       | AQ_CLEAR       |                                |
| DBCTL    | MODE      | DB_FULL_ENABLE | Enable Dead-band module        |
|          | POLSEL    | DB_ACTV_HIC    | Active Hi complementary        |
| DBFED    | DBFED     | 20             | FED = 20 TBCLKs                |
|          | DBRED     | 20             | RED = 20 TBCLKs                |

### **Table 15-60. EPWM3 Initialization for [Figure](#page-1-0) 15-67**

| Register | Bit       | Value          | Comments                       |
|----------|-----------|----------------|--------------------------------|
| TBPRD    | TBPRD     | 450 (1C2h)     | Period = 900 TBCLK counts      |
| TBPHS    | TBPHS     | 300            | Phase = (300/900) × 360 = 120° |
| TBCTL    | CTRMODE   | TB_UPDOWN      |                                |
|          | PHSEN     | TB_ENABLE      | Slave module                   |
|          | PRDLD     | TB_SHADOW      |                                |
|          | SYNCOSEL  | TB_SYNC_IN     | Sync flow-through              |
|          | PHSDIR    | TB_UP          | Count UP on sync               |
| CMPCTL   | SHDWAMODE | CC_SHADOW      |                                |
|          | SHDWBMODE | CC_SHADOW      |                                |
|          | LOADAMODE | CC_CTR_ZERO    | Load on CTR = 0                |
|          | LOADBMODE | CC_CTR_ZERO    | Load on CTR = 0                |
| AQCTLA   | CAU       | AQ_SET         | Set actions for EPWM3A         |
|          | CAD       | AQ_CLEAR       |                                |
| DBCTL    | MODE      | DB_FULL_ENABLE | Enable Dead-band module        |
|          | POLSEL    | DB_ACTV_HIC    | Active Hi complementary        |
| DBFED    | DBFED     | 20             | FED = 20 TBCLKs                |
|          | DBRED     | 20             | RED = 20 TBCLKs                |

*Example 15-16. Code Snippet for Configuration in [Figure](#page-1-0) 15-67*

```
// Run Time (Note: Example execution of one run-time instance)
//===========================================================
EPwm1Regs.CMPA.half.CMPA = 285; // adjust duty for output EPWM1A
EPwm2Regs.CMPA.half.CMPA = 285; // adjust duty for output EPWM2A
EPwm3Regs.CMPA.half.CMPA = 285; // adjust duty for output EPWM3A
```

#### **15.2.3.9 Controlling Zero Voltage Switched Full Bridge (ZVSFB) Converter**

The example given in [Figure](#page-5-0) 15-69 assumes a static or constant phase relationship between legs (modules). In such a case, control is achieved by modulating the duty cycle. It is also possible to dynamically change the phase value on a cycle-by-cycle basis. This feature lends itself to controlling a class of power topologies known as *phase-shifted full bridge*, or *zero voltage switched full bridge.* Here the controlled parameter is not duty cycle (this is kept constant at approximately 50 percent); instead it is the phase relationship between legs. Such a system can be implemented by allocating the resources of two PWM modules to control a single power stage, which in turn requires control of four switching elements. [Figure](#page-6-0) 15-70 shows a master/slave module combination synchronized together to control a full H-bridge. In this case, both master and slave modules are required to switch at the same PWM frequency. The phase is controlled by using the slave's phase register (TBPHS). The master's phase register is not used and therefore can be initialized to zero.

**Figure 15-69. Controlling a Full-H Bridge Stage (FPWM2 = FPWM1)**

Power phase EPWM1A EPWM1B RED 300 Φ2=variable TBPHS =(1200−Φ2) EPWM2A RED EPWM2B Power phase FED 200 600 1200 FED ZVS transition ZVS transition CA Z Z I Z I Z I Z CB A CB CA A Z Z CB A CA Z Z CB A CA

**Figure 15-70. ZVS Full-H Bridge Waveforms**

### **Table 15-61. EPWM1 Initialization for [Figure](#page-5-0) 15-69**

| Register | Bit       | Value          | Comments                   |
|----------|-----------|----------------|----------------------------|
| TBPRD    | TBPRD     | 1200 (4B0h)    | Period = 1201 TBCLK counts |
| TBPHS    | TBPHS     | 0              | Clear Phase Register to 0  |
| TBCTL    | CTRMODE   | TB_UP          |                            |
|          | PHSEN     | TB_DISABLE     | Phase loading disabled     |
|          | PRDLD     | TB_SHADOW      |                            |
|          | SYNCOSEL  | TB_CTR_ZERO    | Sync down-stream module    |
| CMPA     | CMPA      | 600 (258h)     | Set 50% duty for EPWM1A    |
| CMPCTL   | SHDWAMODE | CC_SHADOW      |                            |
|          | SHDWBMODE | CC_SHADOW      |                            |
|          | LOADAMODE | CC_CTR_ZERO    | Load on CTR = 0            |
|          | LOADBMODE | CC_CTR_ZERO    | Load on CTR = 0            |
| AQCTLA   | ZRO       | AQ_SET         | Set actions for EPWM1A     |
|          | CAU       | AQ_CLEAR       |                            |
| DBCTL    | MODE      | DB_FULL_ENABLE | Enable Dead-band module    |
|          | POLSEL    | DB_ACTV_HIC    | Active Hi complementary    |
| DBFED    | DBFED     | 50             | FED = 50 TBCLKs            |
|          | DBRED     | 70             | RED = 70 TBCLKs            |

#### **Table 15-62. EPWM2 Initialization for [Figure](#page-5-0) 15-69**

| Register | Bit       | Value          | Comments                   |
|----------|-----------|----------------|----------------------------|
| TBPRD    | TBPRD     | 1200 (4B0h)    | Period = 1201 TBCLK counts |
| TBPHS    | TBPHS     | 0              | Clear Phase Register to 0  |
| TBCTL    | CTRMODE   | TB_UP          |                            |
|          | PHSEN     | TB_ENABLE      | Slave module               |
|          | PRDLD     | TB_SHADOW      |                            |
|          | SYNCOSEL  | TB_SYNC_IN     | Sync flow-through          |
| CMPA     | CMPA      | 600 (258h)     | Set 50% duty for EPWM2A    |
| CMPCTL   | SHDWAMODE | CC_SHADOW      |                            |
|          | SHDWBMODE | CC_SHADOW      |                            |
|          | LOADAMODE | CC_CTR_ZERO    | Load on CTR = 0            |
|          | LOADBMODE | CC_CTR_ZERO    | Load on CTR = 0            |
| AQCTLA   | ZRO       | AQ_SET         | Set actions for EPWM2A     |
|          | CAU       | AQ_CLEAR       |                            |
| DBCTL    | MODE      | DB_FULL_ENABLE | Enable Dead-band module    |
|          | POLSEL    | DB_ACTV_HIC    | Active Hi complementary    |
| DBFED    | DBFED     | 30             | FED = 30 TBCLKs            |
|          | DBRED     | 40             | RED = 40 TBCLKs            |
|          |           |                |                            |

### *Example 15-17. Code Snippet for Configuration in [Figure](#page-5-0) 15-69*

```
// Run Time (Note: Example execution of one run-time instance)
//============================================================
EPwm2Regs.TBPHS = 1200-300; // Set Phase reg to 300/1200 * 360 = 90 deg
EPwm1Regs.DBFED = FED1_NewValue; // Update ZVS transition interval
EPwm1Regs.DBRED = RED1_NewValue; // Update ZVS transition interval
EPwm2Regs.DBFED = FED2_NewValue; // Update ZVS transition interval
```

### *Example 15-17. Code Snippet for Configuration in [Figure](#page-5-0) 15-69 (continued)*

EPwm2Regs.DBRED = RED2\_NewValue; // Update ZVS transition interval

### *15.2.4 EPWM Registers*

Table [15-63](#page-8-0) lists the memory-mapped registers for the EPWM. All register offset addresses not listed in Table [15-63](#page-8-0) should be considered as reserved locations and the register contents should not be modified.

## **Table 15-63. EPWM Registers**

| Offset | Acronym | Register Name                                              | Section           |
|--------|---------|------------------------------------------------------------|-------------------|
| 0h     | TBCTL   | Time-Base Control Register                                 | Section 15.2.4.1  |
| 2h     | TBSTS   | Time-Base Status Register                                  | Section 15.2.4.2  |
| 4h     | TBPHSHR | Extension for HRPWM Phase Register                         | Section 15.2.4.3  |
| 6h     | TBPHS   | Time-Base Phase Register                                   | Section 15.2.4.4  |
| 8h     | TBCNT   | Time-Base Counter Register                                 | Section 15.2.4.5  |
| Ah     | TBPRD   | Time-Base Period Register                                  | Section 15.2.4.6  |
| Eh     | CMPCTL  | Counter-Compare Control Register                           | Section 15.2.4.7  |
| 10h    | CMPAHR  | Extension for HRPWM Counter-Compare A Register             | Section 15.2.4.8  |
| 12h    | CMPA    | Counter-Compare A Register                                 | Section 15.2.4.9  |
| 14h    | CMPB    | Counter-Compare B Register                                 | Section 15.2.4.10 |
| 16h    | AQCTLA  | Action-Qualifier Control Register for Output A<br>(EPWMxA) | Section 15.2.4.11 |
| 18h    | AQCTLB  | Action-Qualifier Control Register for Output B<br>(EPWMxB) | Section 15.2.4.12 |
| 1Ah    | AQSFRC  | Action-Qualifier Software Force Register                   | Section 15.2.4.13 |
| 1Ch    | AQCSFRC | Action-Qualifier Continuous S/W Force Register Set         | Section 15.2.4.14 |
| 1Eh    | DBCTL   | Dead-Band Generator Control Register                       | Section 15.2.4.15 |
| 20h    | DBRED   | Dead-Band Generator Rising Edge Delay Count<br>Register    | Section 15.2.4.16 |
| 22h    | DBFED   | Dead-Band Generator Falling Edge Delay Count<br>Register   | Section 15.2.4.17 |
| 24h    | TZSEL   | Trip-Zone Select Register                                  | Section 15.2.4.18 |
| 28h    | TZCTL   | Trip-Zone Control Register                                 | Section 15.2.4.19 |
| 2Ah    | TZEINT  | Trip-Zone Enable Interrupt Register                        | Section 15.2.4.20 |
| 2Ch    | TZFLG   | Trip-Zone Flag Register                                    | Section 15.2.4.21 |
| 2Eh    | TZCLR   | Trip-Zone Clear Register                                   | Section 15.2.4.22 |
| 30h    | TZFRC   | Trip-Zone Force Register                                   | Section 15.2.4.23 |
| 32h    | ETSEL   | Event-Trigger Selection Register                           | Section 15.2.4.24 |
| 34h    | ETPS    | Event-Trigger Pre-Scale Register                           | Section 15.2.4.25 |
| 36h    | ETFLG   | Event-Trigger Flag Register                                | Section 15.2.4.26 |
| 38h    | ETCLR   | Event-Trigger Clear Register                               | Section 15.2.4.27 |
| 3Ah    | ETFRC   | Event-Trigger Force Register                               | Section 15.2.4.28 |
| 3Ch    | PCCTL   | PWM-Chopper Control Register                               | Section 15.2.4.29 |
| C0h    | HRCNFG  | HRPWM configuration register (HRCNFG)                      | Section 15.2.4.30 |

#### **15.2.4.1 TBCTL Register (offset = 0h) [reset = 0h]**

TBCTL is shown in [Figure](#page-9-1) 15-71 and described in Table [15-64.](#page-9-2)

#### **Figure 15-71. TBCTL Register**

| 15        | 14        | 13       | 12 | 11     | 10     | 9         | 8 |
|-----------|-----------|----------|----|--------|--------|-----------|---|
|           | FREE_SOFT | PHSDIR   |    | CLKDIV |        | HSPCLKDIV |   |
|           | R/W-0h    | R/W-0h   |    | R/W-0h |        | R/W-0h    |   |
| 7         | 6         | 5        | 4  | 3      | 2      | 1         | 0 |
| HSPCLKDIV | SWFSYNC   | SYNCOSEL |    | PRDLD  | PHSEN  | CTRMODE   |   |
| R/W-0h    | R/W-0h    | R/W-0h   |    | R/W-0h | R/W-0h | R/W-0h    |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-64. TBCTL Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|-------|-----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-14 | FREE_SOFT | R/W  | 0h    | Emulation Mode Bits.<br>These bits select the behavior of the ePWM time-base counter<br>during emulation suspend events.<br>Emulation debug events can be set up in the Debug Subsystem.<br>0h (R/W) = Stop after the next time-base counter increment or<br>decrement<br>1h (R/W) = Stop when counter completes a whole cycle. (a) Up<br>count mode - stop when the time-base counter = period (TBCNT =<br>TBPRD). (b) Down-count mode - stop when the time-base counter =<br>0000 (TBCNT = 0000h). (c) Up-down-count mode - stop when the<br>time-base counter = 0000 (TBCNT = 0000h).       |
|       |           |      |       | 2h (R/W) = Free run<br>3h (R/W) = Free run                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| 13    | PHSDIR    | R/W  | 0h    | Phase Direction Bit.<br>This bit is only used when the time-base counter is configured in the<br>up-down-count mode.<br>The PHSDIR bit indicates the direction the time-base counter<br>(TBCNT) will count after a synchronization event occurs and a new<br>phase value is loaded from the phase (TBPHS) register.<br>This is irrespective of the direction of the counter before the<br>synchronization event<br>In the up-count and down-count modes this bit is ignored.<br>0h (R/W) = Count down after the synchronization event.<br>1h (R/W) = Count up after the synchronization event. |
| 12-10 | CLKDIV    | R/W  | 0h    | Time-base Clock Prescale Bits.<br>These bits determine part of the time-base clock prescale value.<br>TBCLK = SYSCLKOUT/(HSPCLKDIV * CLKDIV)<br>0h (R/W) = /1 (default on reset)<br>1h (R/W) = /2<br>2h (R/W) = /4<br>3h (R/W) = /8<br>4h (R/W) = /16<br>5h (R/W) = /32<br>6h (R/W) = /64<br>7h (R/W) = /128                                                                                                                                                                                                                                                                                   |

**Table 15-64. TBCTL Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|-----|-----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 9-7 | HSPCLKDIV | R/W  | 0h    | High-Speed Time-base Clock Prescale Bits.<br>These bits determine part of the time-base clock prescale value.<br>TBCLK = SYSCLKOUT/(HSPCLKDIV * CLKDIV).<br>This divisor emulates the HSPCLK in the TMS320x281x system as<br>used on the Event Manager (EV) peripheral.<br>0h (R/W) = /1<br>1h (R/W) = /2 (default on reset)<br>2h (R/W) = /4<br>3h (R/W) = /6<br>4h (R/W) = /8<br>5h (R/W) = /10<br>6h (R/W) = /12<br>7h (R/W) = /14                                                                                                                    |
| 6   | SWFSYNC   | R/W  | 0h    | Software Forced Synchronization Pulse.<br>0h (R/W) = Writing a 0 has no effect and reads always return a 0.<br>1h (R/W) = Writing a 1 forces a one-time synchronization pulse to be<br>generated. This event is ORed with the EPWMxSYNCI input of the<br>ePWM module. SWFSYNC is valid (operates) only when<br>EPWMxSYNCI is selected by SYNCOSEL = 00.                                                                                                                                                                                                  |
| 5-4 | SYNCOSEL  | R/W  | 0h    | Synchronization Output Select.<br>These bits select the source of the EPWMxSYNCO signal.<br>0h (R/W) = EPWMxSYNC:<br>1h (R/W) = CTR = 0 - Time-base counter equal to zero (TBCNT =<br>0000h)<br>2h (R/W) = CTR = CMPB - Time-base counter equal to counter<br>compare B (TBCNT = CMPB)<br>3h (R/W) = Disable EPWMxSYNCO signal                                                                                                                                                                                                                           |
| 3   | PRDLD     | R/W  | 0h    | Active Period Register Load From Shadow Register Select<br>0h (R/W) = The period register (TBPRD) is loaded from its shadow<br>register when the time-base counter, TBCNT, is equal to zero. A<br>write or read to the TBPRD register accesses the shadow register.<br>1h (R/W) = Load the TBPRD register immediately without using a<br>shadow register. A write or read to the TBPRD register directly<br>accesses the active register.                                                                                                                |
| 2   | PHSEN     | R/W  | 0h    | Counter Register Load From Phase Register Enable<br>0h (R/W) = Do not load the time-base counter (TBCNT) from the<br>time-base phase register (TBPHS)<br>1h (R/W) = Load the time-base counter with the phase register when<br>an EPWMxSYNCI input signal occurs or when a software<br>synchronization is forced by the SWFSYNC bit.                                                                                                                                                                                                                     |
| 1-0 | CTRMODE   | R/W  | 0h    | Counter Mode.<br>The time-base counter mode is normally configured once and not<br>changed during normal operation.<br>If you change the mode of the counter, the change will take effect at<br>the next TBCLK edge and the current counter value shall increment<br>or decrement from the value before the mode change.<br>These bits set the time-base counter mode of operation as follows:<br>0h (R/W) = Up-count mode<br>1h (R/W) = Down-count mode<br>2h (R/W) = Up-down-count mode<br>3h (R/W) = Stop-freeze counter operation (default on reset) |

#### **15.2.4.2 TBSTS Register (offset = 2h) [reset = 0h]**

TBSTS is shown in [Figure](#page-11-1) 15-72 and described in Table [15-65](#page-11-2).

### **Figure 15-72. TBSTS Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-65. TBSTS Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                    |
|------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-3 | RESERVED | R    | 0h    |                                                                                                                                                                                                                |
| 2    | CTRMAX   |      | 0h    | Time-Base Counter Max Latched Status Bit.                                                                                                                                                                      |
|      |          |      |       | 0h (R/W) = Reading a 0 indicates the time-base counter never<br>reached its maximum value. Writing a 0 will have no effect.                                                                                    |
|      |          |      |       | 1h (R/W) = Reading a 1 on this bit indicates that the time-base<br>counter reached the max value 0xFFFF. Writing a 1 to this bit will<br>clear the latched event.                                              |
| 1    | SYNCI    | W1C  | 0h    | Input Synchronization Latched Status Bit.                                                                                                                                                                      |
|      |          |      |       | 0h (R/W) = Writing a 0 will have no effect. Reading a 0 indicates no<br>external synchronization event has occurred.                                                                                           |
|      |          |      |       | 1h (R/W) = Reading a 1 on this bit indicates that an external<br>synchronization event has occurred (EPWMxSYNCI). Writing a 1 to<br>this bit will clear the latched event.                                     |
| 0    | CTRDIR   | R    | 0h    | Time-Base Counter Direction Status Bit.<br>At reset, the counter is frozen, therefore, this bit has no meaning.<br>To make this bit meaningful, you must first set the appropriate mode<br>via TBCTL[CTRMODE]. |
|      |          |      |       | 0h (R/W) = Time-Base Counter is currently counting down.                                                                                                                                                       |
|      |          |      |       | 1h (R/W) = Time-Base Counter is currently counting up.                                                                                                                                                         |

#### **15.2.4.3 TBPHSHR Register (offset = 4h) [reset = 0h]**

TBPHSHR is shown in [Figure](#page-12-1) 15-73 and described in Table [15-66.](#page-12-2)

**Figure 15-73. TBPHSHR Register**

| 15     | 14       | 13 | 12   | 11 | 10 | 9 | 8 |  |  |  |
|--------|----------|----|------|----|----|---|---|--|--|--|
| TBPHSH |          |    |      |    |    |   |   |  |  |  |
|        | R/W-0h   |    |      |    |    |   |   |  |  |  |
| 7      | 6        | 5  | 4    | 3  | 2  | 1 | 0 |  |  |  |
|        | RESERVED |    |      |    |    |   |   |  |  |  |
|        |          |    | R-0h |    |    |   |   |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-66. TBPHSHR Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                          |
|------|----------|------|-------|--------------------------------------|
| 15-8 | TBPHSH   | R/W  | 0h    | Time-base phase high-resolution bits |
| 7-0  | RESERVED | R    | 0h    |                                      |

#### **15.2.4.4 TBPHS Register (offset = 6h) [reset = 0h]**

TBPHS is shown in [Figure](#page-13-1) 15-74 and described in Table [15-67](#page-13-2).

This register is only available on ePWM instances that include the high-resolution PWM (HRPWM) extension, otherwise, this location is reserved.

**Figure 15-74. TBPHS Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-67. TBPHS Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|------|-------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-0 | TBPHS | R/W  | 0h    | These bits set time-base counter phase of the selected ePWM<br>relative to the time-base that is supplying the synchronization input<br>signal.<br>(a) If TBCTL[PHSEN] = 0, then the synchronization event is ignored<br>and the time-base counter is not loaded with the phase.<br>(b) If TBCTL[PHSEN] = 1, then the time-base counter (TBCNT) will<br>be loaded with the phase (TBPHS) when a synchronization event<br>occurs.<br>The synchronization event can be initiated by the input<br>synchronization signal (EPWMxSYNCI) or by a software forced<br>synchronization. |

#### **15.2.4.5 TBCNT Register (offset = 8h) [reset = 0h]**

TBCNT is shown in [Figure](#page-14-1) 15-75 and described in Table [15-68](#page-14-2).

### **Figure 15-75. TBCNT Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 15-68. TBCNT Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                              |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-0 | TBCNT | R/W  | 0h    | Reading these bits gives the current time-base counter value.<br>Writing to these bits sets the current time-base counter value.<br>The update happens as soon as the write occurs.<br>The write is NOT synchronized to the time-base clock (TBCLK) and<br>the register is not shadowed. |

#### **15.2.4.6 TBPRD Register (offset = Ah) [reset = 0h]**

TBPRD is shown in [Figure](#page-15-1) 15-76 and described in Table [15-69.](#page-15-2)

### **Figure 15-76. TBPRD Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-69. TBPRD Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|------|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-0 | TBPRD | R/W  | 0h    | These bits determine the period of the time-base counter.<br>This sets the PWM frequency.<br>Shadowing of this register is enabled and disabled by the<br>TBCTL[PRDLD] bit.<br>By default this register is shadowed.<br>(a) If TBCTL[PRDLD] = 0, then the shadow is enabled and any write<br>or read will automatically go to the shadow register.<br>In this case, the active register will be loaded from the shadow<br>register when the time-base counter equals zero.<br>(b) If TBCTL[PRDLD] = 1, then the shadow is disabled and any write<br>or read will go directly to the active register, that is the register<br>actively controlling the hardware.<br>(c) The active and shadow registers share the same memory map<br>address. |

#### **15.2.4.7 CMPCTL Register (offset = Eh) [reset = 0h]**

CMPCTL is shown in [Figure](#page-16-1) 15-77 and described in Table [15-70](#page-16-2).

## **Figure 15-77. CMPCTL Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-70. CMPCTL Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|-------|-----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-10 | RESERVED  | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| 9     | SHDWBFULL | R    | 0h    | Counter-compare B (CMPB) Shadow Register Full Status Flag.<br>This bit self clears once a load-strobe occurs.<br>0h (R/W) = CMPB shadow FIFO not full yet<br>1h (R/W) = Indicates the CMPB shadow FIFO is full. A CPU write<br>will overwrite current shadow value.                                                                                                                                                                                      |
| 8     | SHDWAFULL | R    | 0h    | Counter-compare A (CMPA) Shadow Register Full Status Flag.<br>The flag bit is set when a 32 bit write to CMPA:CMPAHR register or<br>a 16 bit write to CMPA register is made.<br>A 16 bit write to CMPAHR register will not affect the flag.<br>This bit self clears once a load-strobe occurs.<br>0h (R/W) = CMPA shadow FIFO not full yet<br>1h (R/W) = Indicates the CMPA shadow FIFO is full, a CPU write will<br>overwrite the current shadow value. |
| 7     | RESERVED  | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| 6     | SHDWBMODE | R/W  | 0h    | Counter-compare B (CMPB) Register Operating Mode.<br>0h (R/W) = Shadow mode. Operates as a double buffer. All writes<br>via the CPU access the shadow register.<br>1h (R/W) = Immediate mode. Only the active compare B register is<br>used. All writes and reads directly access the active register for<br>immediate compare action.                                                                                                                   |
| 5     | RESERVED  | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| 4     | SHDWAMODE | R/W  | 0h    | Counter-compare A (CMPA) Register Operating Mode.<br>0h (R/W) = Shadow mode. Operates as a double buffer. All writes<br>via the CPU access the shadow register.<br>1h (R/W) = Immediate mode. Only the active compare register is<br>used. All writes and reads directly access the active register for<br>immediate compare action                                                                                                                      |
| 3-2   | LOADBMODE | R/W  | 0h    | Active Counter-Compare B (CMPB) Load From Shadow Select<br>Mode.<br>This bit has no effect in immediate mode (CMPCTL[SHDWBMODE]<br>= 1).<br>0h (R/W) = Load on CTR = 0 - Time-base counter equal to zero<br>(TBCNT = 0000h)<br>1h (R/W) = Load on CTR = PRD - Time-base counter equal to period<br>(TBCNT = TBPRD)<br>2h (R/W) = Load on either CTR = 0 or CTR = PRD<br>3h (R/W) = Freeze (no loads possible)                                            |

### **Table 15-70. CMPCTL Register Field Descriptions (continued)**

| Bit | Field     | Type | Reset | Description                                                                                                                              |
|-----|-----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------|
| 1-0 | LOADAMODE | R/W  | 0h    | Active Counter-Compare A (CMPA) Load From Shadow Select<br>Mode.<br>This bit has no effect in immediate mode (CMPCTL[SHDWAMODE]<br>= 1). |
|     |           |      |       | 0h (R/W) = Load on CTR = 0 - Time-base counter equal to zero<br>(TBCNT = 0000h)                                                          |
|     |           |      |       | 1h (R/W) = Load on CTR = PRD - Time-base counter equal to period<br>(TBCNT = TBPRD)                                                      |
|     |           |      |       | 2h (R/W) = Load on either CTR = 0 or CTR = PRD                                                                                           |
|     |           |      |       | 3h (R/W) = Freeze (no loads possible)                                                                                                    |

#### **15.2.4.8 CMPAHR Register (offset = 10h) [reset = 100h]**

CMPAHR is shown in [Figure](#page-18-1) 15-78 and described in Table [15-71](#page-18-2).

This register is only available on ePWM instances that include the high-resolution PWM (HRPWM) extension; otherwise, this location is reserved.

**Figure 15-78. CMPAHR Register**

| 15 | 14       | 13                         | 12 | 11 | 10 | 9 | 8 |  |  |
|----|----------|----------------------------|----|----|----|---|---|--|--|
|    | CMPAHR   |                            |    |    |    |   |   |  |  |
|    | R/W-1h   |                            |    |    |    |   |   |  |  |
| 7  | 6        | 5<br>4<br>3<br>2<br>1<br>0 |    |    |    |   |   |  |  |
|    | RESERVED |                            |    |    |    |   |   |  |  |
|    | R-0h     |                            |    |    |    |   |   |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-71. CMPAHR Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                               |
|------|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-8 | CMPAHR   | R/W  | 1h    | Compare A High-Resolution register bits for MEP step control.<br>A minimum value of 1h is needed to enable HRPWM capabilities.<br>Valid MEP range of operation<br>1-255h. |
| 7-0  | RESERVED | R    | 0h    |                                                                                                                                                                           |

#### **15.2.4.9 CMPA Register (offset = 12h) [reset = 0h]**

CMPA is shown in [Figure](#page-19-1) 15-79 and described in Table [15-72.](#page-19-2)

### **Figure 15-79. CMPA Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 15-72. CMPA Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-0 | CMPA  | R/W  | 0h    | The value in the active CMPA register is continuously compared to<br>the time-base counter (TBCNT).<br>When the values are equal, the counter-compare module generates<br>a "time-base counter equal to counter compare A" event.<br>This event is sent to the action-qualifier where it is qualified and<br>converted it into one or more actions.<br>These actions can be applied to either the EPWMxA or the<br>EPWMxB output depending on the configuration of the AQCTLA and<br>AQCTLB registers.<br>The actions that can be defined in the AQCTLA and AQCTLB<br>registers include the following.<br>(a) Do nothing the event is ignored.<br>(b) Clear - Pull the EPWMxA and/or EPWMxB signal low.<br>(c) Set - Pull the EPWMxA and/or EPWMxB signal high.<br>(d) Toggle the EPWMxA and/or EPWMxB signal.<br>Shadowing of this register is enabled and disabled by the<br>CMPCTL[SHDWAMODE] bit.<br>By default this register is shadowed.<br>(a) If CMPCTL[SHDWAMODE] = 0, then the shadow is enabled and<br>any write or read will automatically go to the shadow register.<br>In this case, the CMPCTL[LOADAMODE] bit field determines which<br>event will load the active register from the shadow register.<br>(b) Before a write, the CMPCTL[SHDWAFULL] bit can be read to<br>determine if the shadow register is currently full.<br>(c) If CMPCTL[SHDWAMODE] = 1, then the shadow register is<br>disabled and any write or read will go directly to the active register,<br>that is the register actively controlling the hardware.<br>(d) In either mode, the active and shadow registers share the same<br>memory map address. |

#### **15.2.4.10 CMPB Register (offset = 14h) [reset = 0h]**

CMPB is shown in [Figure](#page-20-1) 15-80 and described in Table [15-73.](#page-20-2)

### **Figure 15-80. CMPB Register**

| 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|---|--------|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    |   | CMPB   |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |   | R/W-0h |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 15-73. CMPB Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|------|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-0 | CMPB  | R/W  | 0h    | The value in the active CMPB register is continuously compared to<br>the time-base counter (TBCNT).<br>When the values are equal, the counter-compare module generates<br>a "time-base counter equal to counter compare B" event.<br>This event is sent to the action-qualifier where it is qualified and<br>converted it into one or more actions.<br>These actions can be applied to either the EPWMxA or the<br>EPWMxB output depending on the configuration of the AQCTLA and<br>AQCTLB registers.<br>The actions that can be defined in the AQCTLA and AQCTLB<br>registers include the following.<br>(a) Do nothing, the event is ignored.<br>(b) Clear - Pull the EPWMxA and/or EPWMxB signal low.<br>(c) Set - Pull the EPWMxA and/or EPWMxB signal high.<br>(d) Toggle the EPWMxA and/or EPWMxB signal.<br>Shadowing of this register is enabled and disabled by the<br>CMPCTL[SHDWBMODE] bit.<br>By default this register is shadowed.<br>(a) If CMPCTL[SHDWBMODE] = 0, then the shadow is enabled and<br>any write or read will automatically go to the shadow register.<br>In this case, the CMPCTL[LOADBMODE] bit field determines which<br>event will load the active register from the shadow register: (b)<br>Before a write, the CMPCTL[SHDWBFULL] bit can be read to<br>determine if the shadow register is currently full.<br>(c) If CMPCTL[SHDWBMODE] = 1, then the shadow register is<br>disabled and any write or read will go directly to the active register,<br>that is the register actively controlling the hardware.<br>(d) In either mode, the active and shadow registers share the same<br>memory map address. |

#### **15.2.4.11 AQCTLA Register (offset = 16h) [reset = 0h]**

AQCTLA is shown in [Figure](#page-21-1) 15-81 and described in Table [15-74](#page-21-2).

## **Figure 15-81. AQCTLA Register**

| 15 | 14       | 13 | 12     | 11     | 10 | 9      | 8 |
|----|----------|----|--------|--------|----|--------|---|
|    | RESERVED |    |        | CBD    |    | CBU    |   |
|    | R-0h     |    |        | R/W-0h |    | R/W-0h |   |
| 7  | 6        | 5  | 4      |        | 2  | 1      | 0 |
|    | CAD      |    | CAU    | PRD    |    | ZRO    |   |
|    | R/W-0h   |    | R/W-0h | R/W-0h |    | R/W-0h |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-74. AQCTLA Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                             |
|-------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-12 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 11-10 | CBD      | R/W  | 0h    | Action when the time-base counter equals the active CMPB register<br>and the counter is decrementing.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxA output low.<br>2h (R/W) = Set - force EPWMxA output high.<br>3h (R/W) = Toggle EPWMxA output - low output signal will be forced<br>high, and a high signal will be forced low.                                                                      |
| 9-8   | CBU      | R/W  | 0h    | Action when the counter equals the active CMPB register and the<br>counter is incrementing.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxA output low.<br>2h (R/W) = Set - force EPWMxA output high.<br>3h (R/W) = Toggle EPWMxA output - low output signal will be forced<br>high, and a high signal will be forced low.                                                                                |
| 7-6   | CAD      | R/W  | 0h    | Action when the counter equals the active CMPA register and the<br>counter is decrementing.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxA output low.<br>2h (R/W) = Set - force EPWMxA output high.<br>3h (R/W) = Toggle EPWMxA output - low output signal will be forced<br>high, and a high signal will be forced low.                                                                                |
| 5-4   | CAU      | R/W  | 0h    | Action when the counter equals the active CMPA register and the<br>counter is incrementing.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxA output low.<br>2h (R/W) = Set - force EPWMxA output high.<br>3h (R/W) = Toggle EPWMxA output - low output signal will be forced<br>high, and a high signal will be forced low.                                                                                |
| 3-2   | PRD      | R/W  | 0h    | Action when the counter equals the period.<br>Note: By definition, in count up-down mode when the counter equals<br>period the direction is defined as 0 or counting down.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxA output low.<br>2h (R/W) = Set - force EPWMxA output high.<br>3h (R/W) = Toggle EPWMxA output - low output signal will be forced<br>high, and a high signal will be forced low. |

**Table 15-74. AQCTLA Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                            |
|-----|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1-0 | ZRO   | R/W  | 0h    | Action when counter equals zero.<br>Note: By definition, in count up-down mode when the counter equals<br>0 the direction is defined as 1 or counting up.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxA output low.<br>2h (R/W) = Set - force EPWMxA output high.<br>3h (R/W) = Toggle EPWMxA output - low output signal will be forced<br>high, and a high signal will be forced low. |

#### **15.2.4.12 AQCTLB Register (offset = 18h) [reset = 0h]**

AQCTLB is shown in [Figure](#page-23-1) 15-82 and described in Table [15-75](#page-23-2).

### **Figure 15-82. AQCTLB Register**

| 15 | 14       | 13 | 12     | 11     | 10 | 9      | 8 |
|----|----------|----|--------|--------|----|--------|---|
|    | RESERVED |    |        | CBD    |    | CBU    |   |
|    | R-0h     |    |        | R/W-0h |    | R/W-0h |   |
| 7  | 6        | 5  | 4      | 3      | 2  | 1      | 0 |
|    | CAD      |    | CAU    | PRD    |    | ZRO    |   |
|    | R/W-0h   |    | R/W-0h | R/W-0h |    | R/W-0h |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-75. AQCTLB Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                             |
|-------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-12 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 11-10 | CBD      | R/W  | 0h    | Action when the counter equals the active CMPB register and the<br>counter is decrementing.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxB output low.<br>2h (R/W) = Set - force EPWMxB output high.<br>3h (R/W) = Toggle EPWMxB output - low output signal will be forced                                                                                                                               |
| 9-8   | CBU      | R/W  | 0h    | high, and a high signal will be forced low.<br>Action when the counter equals the active CMPB register and the<br>counter is incrementing.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxB output low.<br>2h (R/W) = Set - force EPWMxB output high.<br>3h (R/W) = Toggle EPWMxB output - low output signal will be forced<br>high, and a high signal will be forced low.                                 |
| 7-6   | CAD      | R/W  | 0h    | Action when the counter equals the active CMPA register and the<br>counter is decrementing.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxB output low.<br>2h (R/W) = Set - force EPWMxB output high.<br>3h (R/W) = Toggle EPWMxB output - low output signal will be forced<br>high, and a high signal will be forced low.                                                                                |
| 5-4   | CAU      | R/W  | 0h    | Action when the counter equals the active CMPA register and the<br>counter is incrementing.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxB output low.<br>2h (R/W) = Set - force EPWMxB output high.<br>3h (R/W) = Toggle EPWMxB output - low output signal will be forced<br>high, and a high signal will be forced low.                                                                                |
| 3-2   | PRD      | R/W  | 0h    | Action when the counter equals the period.<br>Note: By definition, in count up-down mode when the counter equals<br>period the direction is defined as 0 or counting down.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxB output low.<br>2h (R/W) = Set - force EPWMxB output high.<br>3h (R/W) = Toggle EPWMxB output - low output signal will be forced<br>high, and a high signal will be forced low. |

**Table 15-75. AQCTLB Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                            |
|-----|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1-0 | ZRO   | R/W  | 0h    | Action when counter equals zero.<br>Note: By definition, in count up-down mode when the counter equals<br>0 the direction is defined as 1 or counting up.<br>0h (R/W) = Do nothing (action disabled)<br>1h (R/W) = Clear - force EPWMxB output low.<br>2h (R/W) = Set - force EPWMxB output high.<br>3h (R/W) = Toggle EPWMxB output - low output signal will be forced<br>high, and a high signal will be forced low. |

#### **15.2.4.13 AQSFRC Register (offset = 1Ah) [reset = 0h]**

AQSFRC is shown in [Figure](#page-25-1) 15-83 and described in Table [15-76.](#page-25-2)

### **Figure 15-83. AQSFRC Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-76. AQSFRC Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                            |
|------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-8 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                        |
| 7-6  | RLDCSF   | R/W  | 0h    | AQCSFRC Active Register Reload From Shadow Options.<br>0h (R/W) = Load on event counter equals zero<br>1h (R/W) = Load on event counter equals period<br>2h (R/W) = Load on event counter equals zero or counter equals<br>period<br>3h (R/W) = Load immediately (the active register is directly accessed<br>by the CPU and is not loaded from the shadow register).                  |
| 5    | OTSFB    | R/W  | 0h    | One-Time Software Forced Event on Output B.<br>0h (R/W) = Writing a 0 (zero) has no effect. Always reads back a 0.<br>This bit is auto cleared once a write to this register is complete, that<br>is, a forced event is initiated. This is a one-shot forced event. It can<br>be overridden by another subsequent event on output B.<br>1h (R/W) = Initiates a single s/w forced event |
| 4-3  | ACTSFB   | R/W  | 0h    | Action when One-Time Software Force B Is invoked<br>0h (R/W) = Does nothing (action disabled)<br>1h (R/W) = Clear (low)<br>2h (R/W) = Set (high)<br>3h (R/W) = Toggle (Low -> High, High -> Low). Note: This action is<br>not qualified by counter direction (CNT_dir)                                                                                                                 |
| 2    | OTSFA    | R/W  | 0h    | One-Time Software Forced Event on Output A.<br>0h (R/W) = Writing a 0 (zero) has no effect. Always reads back a 0.<br>This bit is auto cleared once a write to this register is complete (that<br>is, a forced event is initiated).<br>1h (R/W) = Initiates a single software forced event.                                                                                            |
| 1-0  | ACTSFA   | R/W  | 0h    | Action When One-Time Software Force A Is Invoked.<br>0h (R/W) = Does nothing (action disabled).<br>1h (R/W) = Clear (low).<br>2h (R/W) = Set (high).<br>3h (R/W) = Toggle (Low -> High, High -> Low). Note: This action is<br>not qualified by counter direction (CNT_dir)                                                                                                             |

#### **15.2.4.14 AQCSFRC Register (offset = 1Ch) [reset = 0h]**

AQCSFRC is shown in [Figure](#page-26-1) 15-84 and described in Table [15-77.](#page-26-2)

## **Figure 15-84. AQCSFRC Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-77. AQCSFRC Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-4 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 3-2  | CSFB     | R/W  | 0h    | Continuous Software Force on Output B.<br>In immediate mode, a continuous force takes effect on the next<br>TBCLK edge.<br>In shadow mode, a continuous force takes effect on the next TBCLK<br>edge after a shadow load into the active register.<br>To configure shadow mode, use AQSFRC[RLDCSF].<br>0h (R/W) = Forcing disabled, that is, has no effect<br>1h (R/W) = Forces a continuous low on output B<br>2h (R/W) = Forces a continuous high on output B<br>3h (R/W) = Software forcing is disabled and has no effect |
| 1-0  | CSFA     | R/W  | 0h    | Continuous Software Force on Output A In immediate mode, a<br>continuous force takes effect on the next TBCLK edge.<br>In shadow mode, a continuous force takes effect on the next TBCLK<br>edge after a shadow load into the active register.<br>0h (R/W) = Forcing disabled, that is, has no effect<br>1h (R/W) = Forces a continuous low on output A<br>2h (R/W) = Forces a continuous high on output A<br>3h (R/W) = Software forcing is disabled and has no effect                                                      |

#### **15.2.4.15 DBCTL Register (offset = 1Eh) [reset = 0h]**

DBCTL is shown in [Figure](#page-27-1) 15-85 and described in Table [15-78.](#page-27-2)

### **Figure 15-85. DBCTL Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-78. DBCTL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
|------|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-6 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 5-4  | IN_MODE  | R/W  | 0h    | Dead Band Input Mode Control.<br>Bit 5 controls the S5 switch and bit 4 controls the S4 switch.<br>This allows you to select the input source to the falling-edge and<br>rising-edge delay.<br>To produce classical dead-band waveforms, the default is EPWMxA<br>In is the source for both falling and rising-edge delays.<br>0h (R/W) = EPWMxA In (from the action-qualifier) is the source for                                                                                                                               |
|      |          |      |       | both falling-edge and rising-edge delay.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|      |          |      |       | 1h (R/W) = EPWMxB In (from the action-qualifier) is the source for<br>rising-edge delayed signal. EPWMxA In (from the action-qualifier) is<br>the source for falling-edge delayed signal.                                                                                                                                                                                                                                                                                                                                       |
|      |          |      |       | 2h (R/W) = EPWMxA In (from the action-qualifier) is the source for<br>rising-edge delayed signal. EPWMxB In (from the action-qualifier) is<br>the source for falling-edge delayed signal.                                                                                                                                                                                                                                                                                                                                       |
|      |          |      |       | 3h (R/W) = EPWMxB In (from the action-qualifier) is the source for<br>both rising-edge delay and falling-edge delayed signal.                                                                                                                                                                                                                                                                                                                                                                                                   |
| 3-2  | POLSEL   | R/W  | 0h    | Polarity Select Control.<br>Bit 3 controls the S3 switch and bit 2 controls the S2 switch.<br>This allows you to selectively invert one of the delayed signals<br>before it is sent out of the dead-band submodule.<br>The following descriptions correspond to classical upper/lower switch<br>control as found in one leg of a digital motor control inverter.<br>These assume that DBCTL[OUT_MODE] = 1,1 and<br>DBCTL[IN_MODE] = 0,0.<br>Other enhanced modes are also possible, but not regarded as<br>typical usage modes. |
|      |          |      |       | 0h (R/W) = Active high (AH) mode. Neither EPWMxA nor EPWMxB<br>is inverted (default).                                                                                                                                                                                                                                                                                                                                                                                                                                           |
|      |          |      |       | 1h (R/W) = Active low complementary (ALC) mode. EPWMxA is<br>inverted.                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|      |          |      |       | 2h (R/W) = Active high complementary (AHC). EPWMxB is inverted.                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|      |          |      |       | 3h (R/W) = Active low (AL) mode. Both EPWMxA and EPWMxB are<br>inverted.                                                                                                                                                                                                                                                                                                                                                                                                                                                        |

### **Table 15-78. DBCTL Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                          |
|-----|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1-0 | OUT_MODE | R/W  | 0h    | Dead-band Output Mode Control.<br>Bit 1 controls the S1 switch and bit 0 controls the S0 switch.<br>This allows you to selectively enable or bypass the dead-band<br>generation for the falling-edge and rising-edge delay.                                                                          |
|     |          |      |       | 0h (R/W) = Dead-band generation is bypassed for both output<br>signals. In this mode, both the EPWMxA and EPWMxB output<br>signals from the action-qualifier are passed directly to the PWM<br>chopper submodule. In this mode, the POLSEL and IN_MODE bits<br>have no effect.                       |
|     |          |      |       | 1h (R/W) = Disable rising-edge delay. The EPWMxA signal from the<br>action-qualifier is passed straight through to the EPWMxA input of<br>the PWM-chopper submodule. The falling-edge delayed signal is<br>seen on output EPWMxB. The input signal for the delay is<br>determined by DBCTL[IN_MODE]. |
|     |          |      |       | 2h (R/W) = Disable falling-edge delay. The EPWMxB signal from the<br>action-qualifier is passed straight through to the EPWMxB input of<br>the PWM-chopper submodule. The rising-edge delayed signal is<br>seen on output EPWMxA. The input signal for the delay is<br>determined by DBCTL[IN_MODE]. |
|     |          |      |       | 3h (R/W) = Dead-band is fully enabled for both rising-edge delay on<br>output EPWMxA and falling-edge delay on output EPWMxB. The<br>input signal for the delay is determined by DBCTL[IN_MODE].                                                                                                     |

#### **15.2.4.16 DBRED Register (offset = 20h) [reset = 0h]**

DBRED is shown in [Figure](#page-29-1) 15-86 and described in Table [15-79](#page-29-2).

### **Figure 15-86. DBRED Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-79. DBRED Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                 |
|-------|----------|------|-------|---------------------------------------------|
| 15-10 | RESERVED | R    | 0h    |                                             |
| 9-0   | DEL      | R/W  | 0h    | Rising Edge Delay Count.<br>10 bit counter. |

#### **15.2.4.17 DBFED Register (offset = 22h) [reset = 0h]**

DBFED is shown in [Figure](#page-30-1) 15-87 and described in Table [15-80.](#page-30-2)

## **Figure 15-87. DBFED Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-80. DBFED Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                 |
|-------|----------|------|-------|---------------------------------------------|
| 15-10 | RESERVED | R    | 0h    |                                             |
| 9-0   | DEL      | R/W  | 0h    | Falling Edge Delay Count.<br>10 bit counter |

#### **15.2.4.18 TZSEL Register (offset = 24h) [reset = 0h]**

TZSEL is shown in [Figure](#page-31-1) 15-88 and described in Table [15-81](#page-31-2).

### **Figure 15-88. TZSEL Register**

| 15 | 14     | 13 | 12 | 11     | 10 | 9 | 8 |  |
|----|--------|----|----|--------|----|---|---|--|
|    |        |    |    | OSHTn  |    |   |   |  |
|    |        |    |    | R/W-0h |    |   |   |  |
| 7  | 6      | 5  | 4  | 3      | 2  | 1 | 0 |  |
|    |        |    |    | CBCn   |    |   |   |  |
|    | R/W-0h |    |    |        |    |   |   |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-81. TZSEL Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|------|-------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-8 | OSHTn | R/W  | 0h    | Trip-zone n (TZn) select.<br>One-Shot (OSHT) trip-zone enable/disable.<br>When any of the enabled pins go low, a one-shot trip event occurs<br>for this ePWM module.<br>When the event occurs, the action defined in the TZCTL register is<br>taken on the EPWMxA and EPWMxB outputs.<br>The one-shot trip condition remains latched until you clear the<br>condition via the TZCLR register.<br>0h (R/W) = Disable TZn as a one-shot trip source for this ePWM<br>module.<br>1h (R/W) = Enable TZn as a one-shot trip source for this ePWM<br>module. |
| 7-0  | CBCn  | R/W  | 0h    | Trip-zone n (TZn) select.<br>Cycle-by-Cycle (CBC) trip-zone enable/disable.<br>When any of the enabled pins go low, a cycle-by-cycle trip event<br>occurs for this ePWM module.<br>When the event occurs, the action defined in the TZCTL register is<br>taken on the EPWMxA and EPWMxB outputs.<br>A cycle-by-cycle trip condition is automatically cleared when the<br>time-base counter reaches zero.<br>0h (R/W) = Disable TZn as a CBC trip source for this ePWM module.<br>1h (R/W) = Enable TZn as a CBC trip source for this ePWM module.      |

#### **15.2.4.19 TZCTL Register (offset = 28h) [reset = 0h]**

TZCTL is shown in [Figure](#page-32-1) 15-89 and described in Table [15-82](#page-32-2).

### **Figure 15-89. TZCTL Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-82. TZCTL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                           |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-4 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                       |
| 3-2  | TZB      | R/W  | 0h    | When a trip event occurs the following action is taken on output<br>EPWMxB.<br>Which trip-zone pins can cause an event is defined in the TZSEL<br>register.<br>0h (R/W) = High impedance (EPWMxB = High-impedance state)<br>1h (R/W) = Force EPWMxB to a high state<br>2h (R/W) = Force EPWMxB to a low state<br>3h (R/W) = Do nothing, no action is taken on EPWMxB. |
| 1-0  | TZA      | R/W  | 0h    | When a trip event occurs the following action is taken on output<br>EPWMxA.<br>Which trip-zone pins can cause an event is defined in the TZSEL<br>register.<br>0h (R/W) = High impedance (EPWMxA = High-impedance state)<br>1h (R/W) = Force EPWMxA to a high state<br>2h (R/W) = Force EPWMxA to a low state<br>3h (R/W) = Do nothing, no action is taken on EPWMxA. |

#### **15.2.4.20 TZEINT Register (offset = 2Ah) [reset = 0h]**

TZEINT is shown in [Figure](#page-33-1) 15-90 and described in Table [15-83](#page-33-2).

### **Figure 15-90. TZEINT Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-83. TZEINT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                        |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-3 | RESERVED | R    | 0h    |                                                                                                                                                                                                                    |
| 2    | OST      | R/W  | 0h    | Trip-zone One-Shot Interrupt Enable<br>0h (R/W) = Disable one-shot interrupt generation<br>1h (R/W) = Enable Interrupt generation; a one-shot trip event will<br>cause a EPWMxTZINT interrupt.                     |
| 1    | CBC      | R/W  | 0h    | Trip-zone Cycle-by-Cycle Interrupt Enable<br>0h (R/W) = Disable cycle-by-cycle interrupt generation.<br>1h (R/W) = Enable interrupt generation; a cycle-by-cycle trip event<br>will cause an EPWMxTZINT interrupt. |
| 0    | RESERVED | R    | 0h    |                                                                                                                                                                                                                    |

#### **15.2.4.21 TZFLG Register (offset = 2Ch) [reset = 0h]**

TZFLG is shown in [Figure](#page-34-1) 15-91 and described in Table [15-84.](#page-34-2)

### **Figure 15-91. TZFLG Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-84. TZFLG Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-3 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 2    | OST      | R    | 0h    | Latched Status Flag for A One-Shot Trip Event.<br>0h (R/W) = No one-shot trip event has occurred.<br>1h (R/W) = Indicates a trip event has occurred on a pin selected as a<br>one-shot trip source. This bit is cleared by writing the appropriate<br>value to the TZCLR register.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 1    | CBC      | R    | 0h    | Latched Status Flag for Cycle-By-Cycle Trip Event<br>0h (R/W) = No cycle-by-cycle trip event has occurred.<br>1h (R/W) = Indicates a trip event has occurred on a pin selected as a<br>cycle-by-cycle trip source. The TZFLG[CBC] bit will remain set until it<br>is manually cleared by the user. If the cycle-by-cycle trip event is still<br>present when the CBC bit is cleared, then CBC will be immediately<br>set again. The specified condition on the pins is automatically<br>cleared when the ePWM time-base counter reaches zero (TBCNT =<br>0000h) if the trip condition is no longer present. The condition on the<br>pins is only cleared when the TBCNT = 0000h no matter where in<br>the cycle the CBC flag is cleared. This bit is cleared by writing the<br>appropriate value to the TZCLR register. |
| 0    | INT      | R    | 0h    | Latched Trip Interrupt Status Flag<br>0h (R/W) = Indicates no interrupt has been generated.<br>1h (R/W) = Indicates an EPWMxTZINT interrupt was generated<br>because of a trip condition. No further EPWMxTZINT interrupts will<br>be generated until this flag is cleared. If the interrupt flag is cleared<br>when either CBC or OST is set, then another interrupt pulse will be<br>generated. Clearing all flag bits will prevent further interrupts. This bit<br>is cleared by writing the appropriate value to the TZCLR register.                                                                                                                                                                                                                                                                                |

#### **15.2.4.22 TZCLR Register (offset = 2Eh) [reset = 0h]**

TZCLR is shown in [Figure](#page-35-1) 15-92 and described in Table [15-85](#page-35-2).

## **Figure 15-92. TZCLR Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-85. TZCLR Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-3 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 2    | OST      | R/W  | 0h    | Clear Flag for One-Shot Trip (OST) Latch<br>0h (R/W) = Has no effect. Always reads back a 0.<br>1h (R/W) = Clears this Trip (set) condition.                                                                                                                                                                                                                                                                                                     |
| 1    | CBC      | R/W  | 0h    | Clear Flag for Cycle-By-Cycle (CBC) Trip Latch<br>0h (R/W) = Has no effect. Always reads back a 0.<br>1h (R/W) = Clears this Trip (set) condition.                                                                                                                                                                                                                                                                                               |
| 0    | INT      | R/W  | 0h    | Global Interrupt Clear Flag<br>0h (R/W) = Has no effect. Always reads back a 0.<br>1h (R/W) = Clears the trip-interrupt flag for this ePWM module<br>(TZFLG[INT]). Note: No further EPWMxTZINT interrupts will be<br>generated until the flag is cleared. If the TZFLG[INT] bit is cleared<br>and any of the other flag bits are set, then another interrupt pulse will<br>be generated. Clearing all flag bits will prevent further interrupts. |

#### **15.2.4.23 TZFRC Register (offset = 30h) [reset = 0h]**

TZFRC is shown in [Figure](#page-36-1) 15-93 and described in Table [15-86.](#page-36-2)

## **Figure 15-93. TZFRC Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-86. TZFRC Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                   |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-3 | RESERVED | R    | 0h    |                                                                                                                                                                                               |
| 2    | OST      | R/W  | 0h    | Force a One-Shot Trip Event via Software<br>0h (R/W) = Writing of 0 is ignored. Always reads back a 0.<br>1h (R/W) = Forces a one-shot trip event and sets the TZFLG[OST]<br>bit.             |
| 1    | CBC      | R/W  | 0h    | Force a Cycle-by-Cycle Trip Event via Software<br>0h (R/W) = Writing of 0 is ignored. Always reads back a 0.<br>1h (R/W) = Forces a cycle-by-cycle trip event and sets the<br>TZFLG[CBC] bit. |
| 0    | RESERVED | R    | 0h    |                                                                                                                                                                                               |

#### **15.2.4.24 ETSEL Register (offset = 32h) [reset = 0h]**

ETSEL is shown in [Figure](#page-37-1) 15-94 and described in Table [15-87.](#page-37-2)

### **Figure 15-94. ETSEL Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-87. ETSEL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-4 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 3    | INTEN    | R/W  | 0h    | Enable ePWM Interrupt (EPWMx_INT) Generation<br>0h (R/W) = Disable EPWMx_INT generation<br>1h (R/W) = Enable EPWMx_INT generation                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| 2-0  | INTSEL   | R/W  | 0h    | ePWM Interrupt (EPWMx_INT) Selection Options<br>0h (R/W) = Reserved<br>1h (R/W) = Enable event time-base counter equal to zero. (TBCNT =<br>0000h)<br>2h (R/W) = Enable event time-base counter equal to period (TBCNT<br>= TBPRD)<br>3h (R/W) = Reserved<br>4h (R/W) = Enable event time-base counter equal to CMPA when<br>the timer is incrementing.<br>5h (R/W) = Enable event time-base counter equal to CMPA when<br>the timer is decrementing.<br>6h (R/W) = Enable event - time-base counter equal to CMPB when<br>the timer is incrementing.<br>7h (R/W) = Enable event - time-base counter equal to CMPB when<br>the timer is decrementing. |

#### **15.2.4.25 ETPS Register (offset = 34h) [reset = 0h]**

ETPS is shown in [Figure](#page-38-1) 15-95 and described in Table [15-88](#page-38-2).

### **Figure 15-95. ETPS Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-88. ETPS Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-4 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| 3-2  | INTCNT   | R    | 0h    | ePWM Interrupt Event (EPWMx_INT) Counter Register.<br>These bits indicate how many selected ETSEL[INTSEL] events have<br>occurred.<br>These bits are automatically cleared when an interrupt pulse is<br>generated.<br>If interrupts are disabled, ETSEL[INT] = 0 or the interrupt flag is set,<br>ETFLG[INT] = 1, the counter will stop counting events when it<br>reaches the period value ETPS[INTCNT] = ETPS[INTPRD].<br>0h (R/W) = No events have occurred.<br>1h (R/W) = 1 event has occurred.<br>2h (R/W) = 2 events have occurred.<br>3h (R/W) = 3 events have occurred.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 1-0  | INTPRD   | R/W  | 0h    | ePWM Interrupt (EPWMx_INT) Period Select.<br>These bits determine how many selected ETSEL[INTSEL] events<br>need to occur before an interrupt is generated.<br>To be generated, the interrupt must be enabled (ETSEL[INT] = 1).<br>If the interrupt status flag is set from a previous interrupt<br>(ETFLG[INT] = 1) then no interrupt will be generated until the flag is<br>cleared via the ETCLR[INT] bit.<br>This allows for one interrupt to be pending while another is still being<br>serviced.<br>Once the interrupt is generated, the ETPS[INTCNT] bits will<br>automatically be cleared.<br>Writing a INTPRD value that is the same as the current counter<br>value will trigger an interrupt if it is enabled and the status flag is<br>clear.<br>Writing a INTPRD value that is less than the current counter value<br>will result in an undefined state.<br>If a counter event occurs at the same instant as a new zero or non<br>zero INTPRD value is written, the counter is incremented.<br>0h (R/W) = Disable the interrupt event counter. No interrupt will be<br>generated and ETFRC[INT] is ignored.<br>1h (R/W) = Generate an interrupt on the first event INTCNT = 01<br>(first event)<br>2h (R/W) = Generate interrupt on ETPS[INTCNT] = 1,0 (second<br>event)<br>3h (R/W) = Generate interrupt on ETPS[INTCNT] = 1,1 (third event) |

#### **15.2.4.26 ETFLG Register (offset = 36h) [reset = 0h]**

ETFLG is shown in [Figure](#page-39-1) 15-96 and described in Table [15-89](#page-39-2).

### **Figure 15-96. ETFLG Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-89. ETFLG Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-1 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 0    | INT      | R    | 0h    | Latched ePWM Interrupt (EPWMx_INT) Status Flag<br>0h (R/W) = Indicates no event occurred<br>1h (R/W) = Indicates that an ePWMx interrupt (EWPMx_INT) was<br>generated. No further interrupts will be generated until the flag bit is<br>cleared. Up to one interrupt can be pending while the ETFLG[INT] bit<br>is still set. If an interrupt is pending, it will not be generated until after<br>the ETFLG[INT] bit is cleared. |

#### **15.2.4.27 ETCLR Register (offset = 38h) [reset = 0h]**

ETCLR is shown in [Figure](#page-40-1) 15-97 and described in Table [15-90.](#page-40-2)

## **Figure 15-97. ETCLR Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-90. ETCLR Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                      |
|------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-1 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                  |
| 0    | INT      | R    | 0h    | ePWM Interrupt (EPWMx_INT) Flag Clear Bit<br>0h (R/W) = Writing a 0 has no effect. Always reads back a 0.<br>1h (R/W) = Clears the ETFLG[INT] flag bit and enable further<br>interrupts pulses to be generated. NOTE: Interrupts can also used as<br>DMA events, and this will also enable further DMA events to be<br>generated |

#### **15.2.4.28 ETFRC Register (offset = 3Ah) [reset = 0h]**

ETFRC is shown in [Figure](#page-41-1) 15-98 and described in Table [15-91](#page-41-2).

#### **Figure 15-98. ETFRC Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-91. ETFRC Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                           |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-1 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                       |
| 0    | INT      | R    | 0h    | INT Force Bit.<br>The interrupt will only be generated if the event is enabled in the<br>ETSEL register.<br>The INT flag bit will be set regardless.<br>0h (R/W) = Writing 0 to this bit will be ignored. Always reads back a<br>0.<br>1h (R/W) = Generates an interrupt on EPWMxINT and set the INT<br>flag bit. This bit is used for test purposes. |

#### **15.2.4.29 PCCTL Register (offset = 3Ch) [reset = 0h]**

PCCTL is shown in [Figure](#page-42-1) 15-99 and described in Table [15-92.](#page-42-2)

### **Figure 15-99. PCCTL Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-92. PCCTL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                              |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-11 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                          |
| 10-8  | CHPDUTY  | R/W  | 0h    | Chopping Clock Duty Cycle<br>0h (R/W) = Duty = 1/8 (12.5%)<br>1h (R/W) = Duty = 2/8 (25.0%)<br>2h (R/W) = Duty = 3/8 (37.5%)<br>3h (R/W) = Duty = 4/8 (50.0%)<br>4h (R/W) = Duty = 5/8 (62.5%)<br>5h (R/W) = Duty = 6/8 (75.0%)<br>6h (R/W) = Duty = 7/8 (87.5%)<br>7h (R/W) = Reserved. |
| 7-5   | CHPFREQ  | R/W  | 0h    | Chopping Clock Frequency<br>0h (R/W) = Divide by 1 (no prescale).<br>1h (R/W) = Divide by 2.<br>2h (R/W) = Divide by 3.<br>3h (R/W) = Divide by 4.<br>4h (R/W) = Divide by 5.<br>5h (R/W) = Divide by 6.<br>6h (R/W) = Divide by 7.<br>7h (R/W) = Divide by 8.                           |
| 4-1   | OSHTWTH  | R/W  | 0h    | One-Shot Pulse Width<br>0h (R/W) = 1 - SYSCLKOUT/8 wide<br>1h (R/W) = 2 - SYSCLKOUT/8 wide<br>2h (R/W) = 3 - SYSCLKOUT/8 wide<br>3h (R/W) = 4 - SYSCLKOUT/8 wide<br>Fh (R/W) = 16 - SYSCLKOUT/8 wide                                                                                     |
| 0     | CHPEN    | R/W  | 0h    | PWM-chopping Enable<br>0h (R/W) = Disable (bypass) PWM chopping function<br>1h (R/W) = Enable chopping function                                                                                                                                                                          |

#### **15.2.4.30 HRCNFG Register (offset = C0h) [reset = 0h]**

HRCNFG is shown in Figure [15-100](#page-43-1) and described in Table [15-93](#page-43-2).

This register is only available on ePWM instances that include the high-resolution PWM (HRPWM) extension; otherwise, this location is reserved.

## **Figure 15-100. HRCNFG Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-93. HRCNFG Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-4 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| 3    | HRLOAD   | R/W  | 0h    | Shadow mode bit - Selects the time event that loads the CMPAHR<br>shadow value into the active register.<br>Note: Load mode selection is valid only if CTLMODE = 0 has been<br>selected.<br>You should select this event<br>to match the selection of the CMPA load mode<br>(CMPCTL[LOADMODE] bits) in the EPWM module as<br>follows:<br>0x<br>0: Load on CTR = 0<br>Time-base counter equal to zero (TBCNT = 0000h)<br>0x<br>1: Load on CTR = PRD<br>Time-base counter equal to period (TBCNT = TBPRD)<br>0x<br>2: Load on either CTR = 0 or CTR = PRD (should not be used with<br>HRPWM)<br>0x<br>3: Freeze (no loads possible should not be used with HRPWM)<br>0h (R/W) = CTR = PRD (counter equal period)<br>1h (R/W) = CTR = 0 (counter equals zero) |
| 2    | CTLMODE  | R/W  | 0h    | Control Mode Bits - Selects the register (CMP or TBPHS) that<br>controls the MEP.<br>0h (R/W) = CMPAHR(8) Register controls the edge position (this is<br>duty control mode). (default on reset)<br>1h (R/W) = TBPHSHR(8) Register controls the edge position (this is<br>phase control mode).                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| 1-0  | EDGMODE  | R/W  | 0h    | Edge Mode Bits - Selects the edge of the PWM that is controlled by<br>the micro-edge position (MEP) logic.<br>0h (R/W) = HRPWM capability is disabled (default on reset)<br>1h (R/W) = MEP control of rising edge<br>2h (R/W) = MEP control of falling edge<br>3h (R/W) = MEP control of both edges                                                                                                                                                                                                                                                                                                                                                                                                                                                        |

## **15.3 Enhanced Capture (eCAP) Module**

### *15.3.1 Introduction*

#### **15.3.1.1 Purpose of the Peripheral**

Uses for eCAP include:

- Sample rate measurements of audio inputs
- Speed measurements of rotating machinery (for example, toothed sprockets sensed via Hall sensors)
- Elapsed time measurements between position sensor pulses
- Period and duty cycle measurements of pulse train signals
- Decoding current or voltage amplitude derived from duty cycle encoded current/voltage sensors

#### **15.3.1.2 Features**

The eCAP module includes the following features:

- 32-bit time base counter
- 4-event time-stamp registers (each 32 bits)
- Edge polarity selection for up to four sequenced time-stamp capture events
- Interrupt on either of the four events
- Single shot capture of up to four event time-stamps
- Continuous mode capture of time-stamps in a four-deep circular buffer
- Absolute time-stamp capture
- Difference (Delta) mode time-stamp capture
- All above resources dedicated to a single input pin
- When not used in capture mode, the ECAP module can be configured as a single channel PWM output

### *15.3.2 Functional Description*

The eCAP module represents one complete capture channel that can be instantiated multiple times depending on the target device. In the context of this guide, one eCAP channel has the following independent key resources:

- Dedicated input capture pin
- 32-bit time base counter
- 4 × 32-bit time-stamp capture registers (CAP1-CAP4)
- 4-stage sequencer (Modulo4 counter) that is synchronized to external events, ECAP pin rising/falling edges.
- Independent edge polarity (rising/falling edge) selection for all 4 events
- Input capture signal prescaling (from 2-62)
- One-shot compare register (2 bits) to freeze captures after 1 to 4 time-stamp events
- Control for continuous time-stamp captures using a 4-deep circular buffer (CAP1-CAP4) scheme
- Interrupt capabilities on any of the 4 capture events

Multiple identical eCAP modules can be contained in a system as shown in Figure [15-101](#page-45-0). The number of modules is device-dependent and is based on target application needs. In this chapter, the letter x within a signal or module name is used to indicate a generic eCAP instance on a device.

**Figure 15-101. Multiple eCAP Modules**

#### **15.3.2.1 Capture and APWM Operating Mode**

You can use the eCAP module resources to implement a single-channel PWM generator (with 32 bit capabilities) when it is not being used for input captures. The counter operates in count-up mode, providing a time-base for asymmetrical pulse width modulation (PWM) waveforms. The CAP1 and CAP2 registers become the active period and compare registers, respectively, while CAP3 and CAP4 registers become the period and capture shadow registers, respectively. Figure [15-102](#page-46-0) is a high-level view of both the capture and auxiliary pulse-width modulator (APWM) modes of operation.

**Figure 15-102. Capture and APWM Modes of Operation**

- (1) A single pin is shared between CAP and APWM functions. In capture mode, it is an input; in APWM mode, it is an output.
- (2) In APWM mode, writing any value to CAP1/CAP2 active registers also writes the same value to the corresponding shadow registers CAP3/CAP4. This emulates immediate mode. Writing to the shadow registers CAP3/CAP4 invokes the shadow mode.

#### **15.3.2.2 Capture Mode Description**

Figure [15-103](#page-47-0) shows the various components that implement the capture function.

TSCTR (counter-32 bit) RST CAP1 (APRD active) LD CAP2 (ACMP active) LD CAP3 (APRD shadow) LD CAP4 (ACMP shadow) LD Continuous / Oneshot Capture Control LD1 LD2 LD3 LD4 32 32 PRD [0-31] CMP [0-31] CTR [0-31] ECAPx Interrupt Trigger and Flag control to Interrupt Controller 32 32 32 32 32 ACMP shadow Event Prescale CTRPHS (phase register-32 bit) SYNCOut SYNCIn Event qualifier Polarity select Polarity select Polarity select Polarity select PRDEQ CNTOVF 4 PWM compare logic CTR [0-31] PRD [0-31] CMP [0-31] CMPEQ PRDEQ OVF CTR\_OVF APWM mode Delta-mode SYNC 4 *Capture events* CEVT[1:4] APRD shadow 32 32 MODE SELECT ECCTL2[SYNCI\_EN, SYNCOSEL, SWSYNC] ECCTL2[CAP/APWM] *Edge Polarity Select* ECCTL1[CAPxPOL] ECCTL1[EVTPS] ECCTL1 [ CAPLDEN, CTRRSTx]

**Figure 15-103. Capture Function Diagram**

Registers: ECEINT, ECFLG, ECCLR, ECFRC

ECCTL2 [ RE-ARM, CONT/ONESHT, STOP\_WRAP]

#### *15.3.2.2.1 Event Prescaler*

An input capture signal (pulse train) can be prescaled by N = 2-62 (in multiples of 2) or can bypass the prescaler. This is useful when very high frequency signals are used as inputs. Figure [15-104](#page-48-0) shows a functional diagram and Figure [15-105](#page-48-1) shows the operation of the prescale function.

**Figure 15-104. Event Prescale Control**

(1) When a prescale value of 1 is chosen (ECCTL1[13:9] = 0000) the input capture signal by-passes the prescale logic completely.

#### *15.3.2.2.2 Edge Polarity Select and Qualifier*

- Four independent edge polarity (rising edge/falling edge) selection multiplexers are used, one for each capture event.
- Each edge (up to 4) is event qualified by the Modulo4 sequencer.
- The edge event is gated to its respective CAP*n* register by the Mod4 counter. The CAP*n* register is loaded on the falling edge.

#### *15.3.2.2.3 Continuous/One-Shot Control*

- The Mod4 (2 bit) counter is incremented via edge qualified events (CEVT1-CEVT4).
- The Mod4 counter continues counting (0->1->2->3->0) and wraps around unless stopped.
- A 2-bit stop register is used to compare the Mod4 counter output, and when equal stops the Mod4 counter and inhibits further loads of the CAP1-CAP4 registers. This occurs during one-shot operation.

The continuous/one-shot block (Figure [15-106](#page-49-0)) controls the start/stop and reset (zero) functions of the Mod4 counter via a mono-shot type of action that can be triggered by the stop-value comparator and rearmed via software control.

Once armed, the eCAP module waits for 1-4 (defined by stop-value) capture events before freezing both the Mod4 counter and contents of CAP1-4 registers (time-stamps).

Re-arming prepares the eCAP module for another capture sequence. Also re-arming clears (to zero) the Mod4 counter and permits loading of CAP1-4 registers again, providing the CAPLDEN bit is set.

In continuous mode, the Mod4 counter continues to run (0->1->2->3->0, the one-shot action is ignored, and capture values continue to be written to CAP1-4 in a circular buffer sequence.

**Figure 15-106. Continuous/One-shot Block Diagram**

#### *15.3.2.2.4 32-Bit Counter and Phase Control*

This counter (Figure [15-107\)](#page-50-0) provides the time-base for event captures, and is clocked via the system clock.

A phase register is provided to achieve synchronization with other counters, via a hardware and software forced sync. This is useful in APWM mode when a phase offset between modules is needed.

On any of the four event loads, an option to reset the 32-bit counter is given. This is useful for time difference capture. The 32-bit counter value is captured first, then it is reset to 0 by any of the LD1-LD4 signals.

**Figure 15-107. Counter and Synchronization Block Diagram**

#### *15.3.2.2.5 CAP1-CAP4 Registers*

These 32-bit registers are fed by the 32-bit counter timer bus, CTR[0-31] and are loaded (capture a timestamp) when their respective LD inputs are strobed.

Loading of the capture registers can be inhibited via control bit CAPLDEN. During one-shot operation, this bit is cleared (loading is inhibited) automatically when a stop condition occurs, StopValue = Mod4.

CAP1 and CAP2 registers become the active period and compare registers, respectively, in APWM mode.

CAP3 and CAP4 registers become the respective shadow registers (APRD and ACMP) for CAP1 and CAP2 during APWM operation.

#### *15.3.2.2.6 Interrupt Control*

An Interrupt can be generated on capture events (CEVT1-CEVT4, CNTOVF) or APWM events (PRDEQ, CMPEQ). See Figure [15-108.](#page-52-0)

A counter overflow event (FFFF FFFFh->0000 0000h) is also provided as an interrupt source (CNTOVF).

The capture events are edge and sequencer qualified (that is, ordered in time) by the polarity select and Mod4 gating, respectively.

One of these events can be selected as the interrupt source (from the eCAP*n* module) going to the interrupt controller.

Seven interrupt events (CEVT1, CEVT2, CEVT3, CEVT4, CNTOVF, PRDEQ, CMPEQ) can be generated. The interrupt enable register (ECEINT) is used to enable/disable individual interrupt event sources. The interrupt flag register (ECFLG) indicates if any interrupt event has been latched and contains the global interrupt flag bit (INT). An interrupt pulse is generated to the interrupt controller only if any of the interrupt events are enabled, the flag bit is 1, and the INT flag bit is 0. The interrupt service routine must clear the global interrupt flag bit and the serviced event via the interrupt clear register (ECCLR) before any other interrupt pulses are generated. You can force an interrupt event via the interrupt force register (ECFRC). This is useful for test purposes.

Note that the interrupts coming from the eCAP module are also used as DMA events. The interrupt registers should be used to enable and clear the current DMA event in order for the eCAP module to generate subsequent DMA events.

#### *15.3.2.2.7 Shadow Load and Lockout Control*

In capture mode, this logic inhibits (locks out) any shadow loading of CAP1 or CAP2 from APRD and ACMP registers, respectively.

In APWM mode, shadow loading is active and two choices are permitted:

- Immediate APRD or ACMP are transferred to CAP1 or CAP2 immediately upon writing a new value.
- On period equal, CTR[31:0] = PRD[31:0]

**NOTE:** The CEVT1, CEVT2, CEVT3, CEVT4 flags are only active in capture mode (ECCTL2[CAP/APWM == 0]). The PRDEQ, CMPEQ flags are only valid in APWM mode (ECCTL2[CAP/APWM == 1]). CNTOVF flag is valid in both modes.

**Figure 15-108. Interrupts in eCAP Module**

#### *15.3.2.2.8 APWM Mode Operation*

Main operating highlights of the APWM section:

- The time-stamp counter bus is made available for comparison via 2 digital (32-bit) comparators.
- When CAP1/2 registers are not used in capture mode, their contents can be used as Period and Compare values in APWM mode.
- Double buffering is achieved via shadow registers APRD and ACMP (CAP3/4). The shadow register contents are transferred over to CAP1/2 registers either immediately upon a write, or on a PRDEQ trigger.
- In APWM mode, writing to CAP1/CAP2 active registers will also write the same value to the corresponding shadow registers CAP3/CAP4. This emulates immediate mode. Writing to the shadow registers CAP3/CAP4 will invoke the shadow mode.
- During initialization, you must write to the active registers for both period and compare. This automatically copies the initial values into the shadow values. For subsequent compare updates, during run-time, you only need to use the shadow registers.

**Figure 15-109. PWM Waveform Details Of APWM Mode Operation**

#### The behavior of APWM active-high mode (APWMPOL == 0) is:

```
CMP = 0x00000000, output low for duration of period (0% duty)
CMP = 0x00000001, output high 1 cycle
CMP = 0x00000002, output high 2 cycles
CMP = PERIOD, output high except for 1 cycle (<100% duty)
CMP = PERIOD+1, output high for complete period (100% duty)
CMP > PERIOD+1, output high for complete period
```

#### The behavior of APWM active-low mode (APWMPOL == 1) is:

```
CMP = 0x00000000, output high for duration of period (0% duty)
CMP = 0x00000001, output low 1 cycle
CMP = 0x00000002, output low 2 cycles
CMP = PERIOD, output low except for 1 cycle (<100% duty)
CMP = PERIOD+1, output low for complete period (100% duty)
CMP > PERIOD+1, output low for complete period
```

### *15.3.3 Use Cases*

The following sections will provide Applications examples and code snippets to show how to configure and operate the eCAP module. For clarity and ease of use, below are useful #defines which will help in the understanding of the examples.

```
// ECCTL1 ( ECAP Control Reg 1)
//==========================
// CAPxPOL bits
#define EC_RISING 0x0
#define EC_FALLING 0x1
// CTRRSTx bits
#define EC_ABS_MODE 0x0
#define EC_DELTA_MODE 0x1
// PRESCALE bits
#define EC_BYPASS 0x0
#define EC_DIV1 0x0
#define EC_DIV2 0x1
#define EC_DIV4 0x2
#define EC_DIV6 0x3
#define EC_DIV8 0x4
#define EC_DIV10 0x5
// ECCTL2 ( ECAP Control Reg 2)
//==========================
// CONT/ONESHOT bit
#define EC_CONTINUOUS 0x0
#define EC_ONESHOT 0x1
// STOPVALUE bit
#define EC_EVENT1 0x0
#define EC_EVENT2 0x1
#define EC_EVENT3 0x2
#define EC_EVENT4 0x3
// RE-ARM bit
#define EC_ARM 0x1
// TSCTRSTOP bit
#define EC_FREEZE 0x0
#define EC_RUN 0x1
// SYNCO_SEL bit
#define EC_SYNCIN 0x0
#define EC_CTR_PRD 0x1
#define EC_SYNCO_DIS 0x2
// CAP/APWM mode bit
#define EC_CAP_MODE 0x0
#define EC_APWM_MODE 0x1
// APWMPOL bit
#define EC_ACTV_HI 0x0
#define EC_ACTV_LO 0x1
// Generic
#define EC_DISABLE 0x0
#define EC_ENABLE 0x1
#define EC_FORCE 0x1
```

#### **15.3.3.1 Absolute Time-Stamp Operation Rising Edge Trigger Example**

Figure [15-110](#page-55-0) shows an example of continuous capture operation (Mod4 counter wraps around). In this figure, TSCTR counts-up without resetting and capture events are qualified on the rising edge only, this gives period (and frequency) information.

On an event, the TSCTR contents (time-stamp) is first captured, then Mod4 counter is incremented to the next state. When the TSCTR reaches FFFF FFFFh (maximum value), it wraps around to 0000 0000h (not shown in Figure [15-110](#page-55-0)), if this occurs, the CNTOVF (counter overflow) flag is set, and an interrupt (if enabled) occurs, CNTOVF (counter overflow) Flag is set, and an Interrupt (if enabled) occurs. Captured time-stamps are valid at the point indicated by the diagram, after the 4th event, hence event CEVT4 can conveniently be used to trigger an interrupt and the CPU can read data from the CAP*n* registers.

**Figure 15-110. Capture Sequence for Absolute Time-Stamp, Rising Edge Detect**

### **Table 15-94. ECAP Initialization for CAP Mode Absolute Time, Rising Edge Trigger**

| Register | Bit         | Value         |
|----------|-------------|---------------|
| ECCTL1   | CAP1POL     | EC_RISING     |
| ECCTL1   | CAP2POL     | EC_RISING     |
| ECCTL1   | CAP3POL     | EC_RISING     |
| ECCTL1   | CAP4POL     | EC_RISING     |
| ECCTL1   | CTRRST1     | EC_ABS_MODE   |
| ECCTL1   | CTRRST2     | EC_ABS_MODE   |
| ECCTL1   | CTRRST3     | EC_ABS_MODE   |
| ECCTL1   | CTRRST4     | EC_ABS_MODE   |
| ECCTL1   | CAPLDEN     | EC_ENABLE     |
| ECCTL1   | PRESCALE    | EC_DIV1       |
| ECCTL2   | CAP_APWM    | EC_CAP_MODE   |
| ECCTL2   | CONT_ONESHT | EC_CONTINUOUS |
| ECCTL2   | SYNCO_SEL   | EC_SYNCO_DIS  |
| ECCTL2   | SYNCI_EN    | EC_DISABLE    |
| ECCTL2   | TSCTRSTOP   | EC_RUN        |

#### *Example 15-18. Code Snippet for CAP Mode Absolute Time, Rising Edge Trigger*

```
// Code snippet for CAP mode Absolute Time, Rising edge trigger
// Run Time ( e.g. CEVT4 triggered ISR call)
//==========================================
   TSt1 = ECAPxRegs.CAP1; // Fetch Time-Stamp captured at t1
   TSt2 = ECAPxRegs.CAP2; // Fetch Time-Stamp captured at t2
   TSt3 = ECAPxRegs.CAP3; // Fetch Time-Stamp captured at t3
   TSt4 = ECAPxRegs.CAP4; // Fetch Time-Stamp captured at t4
   Period1 = TSt2-TSt1; // Calculate 1st period
   Period2 = TSt3-TSt2; // Calculate 2nd period
   Period3 = TSt4-TSt3; // Calculate 3rd period
```

#### 15.3.3.2 Absolute Time-Stamp Operation Rising and Falling Edge Trigger Example

In Figure 15-111 the eCAP operating mode is almost the same as in the previous section except capture events are qualified as either rising or falling edge, this now gives both period and duty cycle information: Period1 =  $t_3 - t_1$ , Period2 =  $t_5 - t_3$ , ...etc. Duty Cycle1 (on-time %) =  $(t_2 - t_1)$  / Period1 x 100%, etc. Duty Cycle1 (off-time %) =  $(t_3 - t_2)$  / Period1 x 100%, etc.

Figure 15-111. Capture Sequence for Absolute Time-Stamp, Rising and Falling Edge Detect

### **Table 15-95. ECAP Initialization for CAP Mode Absolute Time, Rising and Falling Edge Trigger**

| Register | Bit         | Value         |
|----------|-------------|---------------|
| ECCTL1   | CAP1POL     | EC_RISING     |
| ECCTL1   | CAP2POL     | EC_FALLING    |
| ECCTL1   | CAP3POL     | EC_RISING     |
| ECCTL1   | CAP4POL     | EC_FALLING    |
| ECCTL1   | CTRRST1     | EC_ABS_MODE   |
| ECCTL1   | CTRRST2     | EC_ABS_MODE   |
| ECCTL1   | CTRRST3     | EC_ABS_MODE   |
| ECCTL1   | CTRRST4     | EC_ABS_MODE   |
| ECCTL1   | CAPLDEN     | EC_ENABLE     |
| ECCTL1   | PRESCALE    | EC_DIV1       |
| ECCTL2   | CAP_APWM    | EC_CAP_MODE   |
| ECCTL2   | CONT_ONESHT | EC_CONTINUOUS |
| ECCTL2   | SYNCO_SEL   | EC_SYNCO_DIS  |
| ECCTL2   | SYNCI_EN    | EC_DISABLE    |
| ECCTL2   | TSCTRSTOP   | EC_RUN        |

#### *Example 15-19. Code Snippet for CAP Mode Absolute Time, Rising and Falling Edge Trigger*

```
// Code snippet for CAP mode Absolute Time, Rising & Falling edge triggers
// Run Time ( e.g. CEVT4 triggered ISR call)
//==========================================
   TSt1 = ECAPxRegs.CAP1; // Fetch Time-Stamp captured at t1
   TSt2 = ECAPxRegs.CAP2; // Fetch Time-Stamp captured at t2
   TSt3 = ECAPxRegs.CAP3; // Fetch Time-Stamp captured at t3
   TSt4 = ECAPxRegs.CAP4; // Fetch Time-Stamp captured at t4
   Period1 = TSt3-TSt1; // Calculate 1st period
   DutyOnTime1 = TSt2-TSt1; // Calculate On time
   DutyOffTime1 = TSt3-TSt2; // Calculate Off time
```

#### 15.3.3.3 Time Difference (Delta) Operation Rising Edge Trigger Example

Figure 15-112 shows how the eCAP module can be used to collect Delta timing data from pulse train waveforms. Here Continuous Capture mode (TSCTR counts-up without resetting, and Mod4 counter wraps around) is used. In Delta-time mode, TSCTR is Reset back to Zero on every valid event. Here Capture events are qualified as Rising edge only. On an event, TSCTR contents (time-stamp) is captured first, and then TSCTR is reset to Zero. The Mod4 counter then increments to the next state. If TSCTR reaches FFFF FFFFh (maximum value), before the next event, it wraps around to 0000 0000h and continues, a CNTOVF (counter overflow) Flag is set, and an Interrupt (if enabled) occurs. The advantage of Delta-time Mode is that the CAPn contents directly give timing data without the need for CPU calculations: Period1 =  $T_1$ , Period2 =  $T_2$ ,...etc. As shown in Figure 15-112, the CEVT1 event is a good trigger point to read the timing data,  $T_1$ ,  $T_2$ ,  $T_3$ ,  $T_4$  are all valid here.

Figure 15-112. Capture Sequence for Delta Mode Time-Stamp, Rising Edge Detect

## **Table 15-96. ECAP Initialization for CAP Mode Delta Time, Rising Edge Trigger**

| Register | Bit         | Value         |
|----------|-------------|---------------|
| ECCTL1   | CAP1POL     | EC_RISING     |
| ECCTL1   | CAP2POL     | EC_RISING     |
| ECCTL1   | CAP3POL     | EC_RISING     |
| ECCTL1   | CAP4POL     | EC_RISING     |
| ECCTL1   | CTRRST1     | EC_DELTA_MODE |
| ECCTL1   | CTRRST2     | EC_DELTA_MODE |
| ECCTL1   | CTRRST3     | EC_DELTA_MODE |
| ECCTL1   | CTRRST4     | EC_DELTA_MODE |
| ECCTL1   | CAPLDEN     | EC_ENABLE     |
| ECCTL1   | PRESCALE    | EC_DIV1       |
| ECCTL2   | CAP_APWM    | EC_CAP_MODE   |
| ECCTL2   | CONT_ONESHT | EC_CONTINUOUS |
| ECCTL2   | SYNCO_SEL   | EC_SYNCO_DIS  |
| ECCTL2   | SYNCI_EN    | EC_DISABLE    |
| ECCTL2   | TSCTRSTOP   | EC_RUN        |

#### *Example 15-20. Code Snippet for CAP Mode Delta Time, Rising Edge Trigger*

```
// Code snippet for CAP mode Delta Time, Rising edge trigger
// Run Time ( e.g. CEVT1 triggered ISR call)
//==========================================
// Note: here Time-stamp directly represents the Period value.
   Period4 = ECAPxRegs.CAP1; // Fetch Time-Stamp captured at T1
   Period1 = ECAPxRegs.CAP2; // Fetch Time-Stamp captured at T2
   Period2 = ECAPxRegs.CAP3; // Fetch Time-Stamp captured at T3
   Period3 = ECAPxRegs.CAP4; // Fetch Time-Stamp captured at T4
```

#### 15.3.3.4 Time Difference (Delta) Operation Rising and Falling Edge Trigger Example

In Figure 15-113 the eCAP operating mode is almost the same as in previous section except Capture events are qualified as either Rising or Falling edge, this now gives both Period and Duty cycle information: Period1 =  $T_1 + T_2$ , Period2 =  $T_3 + T_4$ , ...etc Duty Cycle1 (on-time %) =  $T_1$  / Period1 x 100%, etc Duty Cycle1 (off-time %) =  $T_2$  / Period1 x 100%, etc

During initialization, you must write to the active registers for both period and compare. This will then automatically copy the init values into the shadow values. For subsequent compare updates, that is, during run-time, only the shadow registers must be used.

CEVT2 CEVT4 CEVT2 CEVT1 CEVT3 CEVT1 CEVT3 CEVT5 CAPx pin  $T_8$  $T_1$  $T_3$  $T_5$ **FFFFFFF**  $T_2$  $T_4$ CTR[0-31] 00000000 MOD4 0 2 3 0 1 2 3 0 CTR CAP1 XX CTR value at CEVT1  $t_4$ CAP2 XX  $t_1$ t<sub>5</sub> CAP3 XX $t_2$  $t_6$ XX  $t_3$  $t_7$ Polarity selection Capture registers [1-4]

Figure 15-113. Capture Sequence for Delta Mode Time-Stamp, Rising and Falling Edge Detect

### **Table 15-97. ECAP Initialization for CAP Mode Delta Time, Rising and Falling Edge Triggers**

| Register | Bit         | Value         |
|----------|-------------|---------------|
| ECCTL1   | CAP1POL     | EC_RISING     |
| ECCTL1   | CAP2POL     | EC_FALLING    |
| ECCTL1   | CAP3POL     | EC_RISING     |
| ECCTL1   | CAP4POL     | EC_FALLING    |
| ECCTL1   | CTRRST1     | EC_DELTA_MODE |
| ECCTL1   | CTRRST2     | EC_DELTA_MODE |
| ECCTL1   | CTRRST3     | EC_DELTA_MODE |
| ECCTL1   | CTRRST4     | EC_DELTA_MODE |
| ECCTL1   | CAPLDEN     | EC_ENABLE     |
| ECCTL1   | PRESCALE    | EC_DIV1       |
| ECCTL2   | CAP_APWM    | EC_CAP_MODE   |
| ECCTL2   | CONT_ONESHT | EC_CONTINUOUS |
| ECCTL2   | SYNCO_SEL   | EC_SYNCO_DIS  |
| ECCTL2   | SYNCI_EN    | EC_DISABLE    |
| ECCTL2   | TSCTRSTOP   | EC_RUN        |

#### *Example 15-21. Code Snippet for CAP Mode Delta Time, Rising and Falling Edge Triggers*

```
// Code snippet for CAP mode Delta Time, Rising and Falling edge triggers
// Run Time ( e.g. CEVT1 triggered ISR call)
//==========================================
// Note: here Time-stamp directly represents the Duty cycle values.
   DutyOnTime1 = ECAPxRegs.CAP2; // Fetch Time-Stamp captured at T2
   DutyOffTime1 = ECAPxRegs.CAP3; // Fetch Time-Stamp captured at T3
   DutyOnTime2 = ECAPxRegs.CAP4; // Fetch Time-Stamp captured at T4
   DutyOffTime2 = ECAPxRegs.CAP1; // Fetch Time-Stamp captured at T1
   Period1 = DutyOnTime1 + DutyOffTime1;
   Period2 = DutyOnTime2 + DutyOffTime2;
```

#### **15.3.3.5 Application of the APWM Mode**

#### *15.3.3.5.1 Simple PWM Generation (Independent Channel/s) Example*

In this example, the eCAP module is configured to operate as a PWM generator. Here a very simple single channel PWM waveform is generated from output pin APWM*n*. The PWM polarity is active high, which means that the compare value (CAP2 reg is now a compare register) represents the on-time (high level) of the period. Alternatively, if the APWMPOL bit is configured for active low, then the compare value represents the off-time.

**Figure 15-114. PWM Waveform Details of APWM Mode Operation**

### **Table 15-98. ECAP Initialization for APWM Mode**

| Register | Bit       | Value        |
|----------|-----------|--------------|
| CAP1     | CAP1      | 0x1000       |
| CTRPHS   | CTRPHS    | 0x0          |
| ECCTL2   | CAP_APWM  | EC_APWM_MODE |
| ECCTL2   | APWMPOL   | EC_ACTV_HI   |
| ECCTL2   | SYNCI_EN  | EC_DISABLE   |
| ECCTL2   | SYNCO_SEL | EC_SYNCO_DIS |
| ECCTL2   | TSCTRSTOP | EC_RUN       |

## *Example 15-22. Code Snippet for APWM Mode*

```
// Code snippet for APWM mode Example 1
// Run Time (Instant 1, e.g. ISR call)
//======================
  ECAPxRegs.CAP2 = 0x300; // Set Duty cycle i.e. compare value
// Run Time (Instant 2, e.g. another ISR call)
//======================
  ECAPxRegs.CAP2 = 0x500; // Set Duty cycle i.e. compare value
```

#### *15.3.3.5.2 Multichannel PWM Generation with Synchronization Example*

Figure [15-115](#page-65-0) takes advantage of the synchronization feature between eCAP modules. Here 4 independent PWM channels are required with different frequencies, but at integer multiples of each other to avoid "beat" frequencies. Hence one eCAP module is configured as the Master and the remaining 3 are Slaves all receiving their synch pulse (CTR = PRD) from the master. Note the Master is chosen to have the lower frequency (F1 = 1/20,000) requirement. Here Slave2 Freq = 2 × F1, Slave3 Freq = 4 × F1 and Slave4 Freq = 5 × F1. Note here values are in decimal notation. Also, only the APWM1 output waveform is shown.

**Figure 15-115. Multichannel PWM Example Using 4 eCAP Modules**

### **Table 15-99. ECAP1 Initialization for Multichannel PWM Generation with Synchronization**

| Register | Bit       | Value        |
|----------|-----------|--------------|
| CAP1     | CAP1      | 20000        |
| CTRPHS   | CTRPHS    | 0            |
| ECCTL2   | CAP_APWM  | EC_APWM_MODE |
| ECCTL2   | APWMPOL   | EC_ACTV_HI   |
| ECCTL2   | SYNCI_EN  | EC_DISABLE   |
| ECCTL2   | SYNCO_SEL | EC_CTR_PRD   |
| ECCTL2   | TSCTRSTOP | EC_RUN       |

**Table 15-100. ECAP2 Initialization for Multichannel PWM Generation with Synchronization**

| Register | Bit       | Value        |
|----------|-----------|--------------|
| CAP1     | CAP1      | 10000        |
| CTRPHS   | CTRPHS    | 0            |
| ECCTL2   | CAP_APWM  | EC_APWM_MODE |
| ECCTL2   | APWMPOL   | EC_ACTV_HI   |
| ECCTL2   | SYNCI_EN  | EC_ENABLE    |
| ECCTL2   | SYNCO_SEL | EC_SYNCI     |
| ECCTL2   | TSCTRSTOP | EC_RUN       |

**Table 15-101. ECAP3 Initialization for Multichannel PWM Generation with Synchronization**

| Register | Bit       | Value        |
|----------|-----------|--------------|
| CAP1     | CAP1      | 5000         |
| CTRPHS   | CTRPHS    | 0            |
| ECCTL2   | CAP_APWM  | EC_APWM_MODE |
| ECCTL2   | APWMPOL   | EC_ACTV_HI   |
| ECCTL2   | SYNCI_EN  | EC_ENABLE    |
| ECCTL2   | SYNCO_SEL | EC_SYNCI     |
| ECCTL2   | TSCTRSTOP | EC_RUN       |

**Table 15-102. ECAP4 Initialization for Multichannel PWM Generation with Synchronization**

| Register | Bit       | Value        |
|----------|-----------|--------------|
| CAP1     | CAP1      | 4000         |
| CTRPHS   | CTRPHS    | 0            |
| ECCTL2   | CAP_APWM  | EC_APWM_MODE |
| ECCTL2   | APWMPOL   | EC_ACTV_HI   |
| ECCTL2   | SYNCI_EN  | EC_ENABLE    |
| ECCTL2   | SYNCO_SEL | EC_SYNCO_DIS |
| ECCTL2   | TSCTRSTOP | EC_RUN       |

### *Example 15-23. Code Snippet for Multichannel PWM Generation with Synchronization*

```
// Code snippet for APWM mode Example 2
// Run Time (Note: Example execution of one run-time instant)
//============================================================
  ECAP1Regs.CAP2 = 7000; // Set Duty cycle i.e., compare value = 7000
  ECAP2Regs.CAP2 = 2000; // Set Duty cycle i.e., compare value = 2000
  ECAP3Regs.CAP2 = 550; // Set Duty cycle i.e., compare value = 550
  ECAP4Regs.CAP2 = 6500; // Set Duty cycle i.e., compare value = 6500
```

#### *15.3.3.5.3 Multichannel PWM Generation with Phase Control Example*

In Figure [15-116,](#page-68-0) the Phase control feature of the APWM mode is used to control a 3 phase Interleaved DC/DC converter topology. This topology requires each phase to be off-set by 120° from each other. Hence if "Leg" 1 (controlled by APWM1) is the reference Leg (or phase), that is, 0°, then Leg 2 need 120° off-set and Leg 3 needs 240° off-set. The waveforms in Figure [15-116](#page-68-0) show the timing relationship between each of the phases (Legs). Note eCAP1 module is the Master and issues a sync out pulse to the slaves (modules 2, 3) whenever TSCTR = Period value.

Comple− mentary and deadband logic APWM1 APWM2 logic deadband Comple− mentary and deadband logic APWM3 and mentary Comple− Vout APRD(1) APRD(1) 1200 700 SYNCO pulse (PRDEQ) APWM1 Ȉ2=120° Ȉ3=240° CTRPHS(2)=800 CTRPHS(3)=400 APWM2 APWM3 TSCTR

**Figure 15-116. Multiphase (channel) Interleaved PWM Example Using 3 eCAP Modules**

## **Table 15-103. ECAP1 Initialization for Multichannel PWM Generation with Phase Control**

| Register | Bit       | Value        |
|----------|-----------|--------------|
| CAP1     | CAP1      | 1200         |
| CTRPHS   | CTRPHS    | 0            |
| ECCTL2   | CAP_APWM  | EC_APWM_MODE |
| ECCTL2   | APWMPOL   | EC_ACTV_HI   |
| ECCTL2   | SYNCI_EN  | EC_DISABLE   |
| ECCTL2   | SYNCO_SEL | EC_CTR_PRD   |
| ECCTL2   | TSCTRSTOP | EC_RUN       |

**Table 15-104. ECAP2 Initialization for Multichannel PWM Generation with Phase Control**

| Register | Bit       | Value        |
|----------|-----------|--------------|
| CAP1     | CAP1      | 1200         |
| CTRPHS   | CTRPHS    | 800          |
| ECCTL2   | CAP_APWM  | EC_APWM_MODE |
| ECCTL2   | APWMPOL   | EC_ACTV_HI   |
| ECCTL2   | SYNCI_EN  | EC_ENABLE    |
| ECCTL2   | SYNCO_SEL | EC_SYNCI     |
| ECCTL2   | TSCTRSTOP | EC_RUN       |

**Table 15-105. ECAP3 Initialization for Multichannel PWM Generation with Phase Control**

| Register | Bit       | Value        |
|----------|-----------|--------------|
| CAP1     | CAP1      | 1200         |
| CTRPHS   | CTRPHS    | 400          |
| ECCTL2   | CAP_APWM  | EC_APWM_MODE |
| ECCTL2   | APWMPOL   | EC_ACTV_HI   |
| ECCTL2   | SYNCI_EN  | EC_ENABLE    |
| ECCTL2   | SYNCO_SEL | EC_SYNCO_DIS |
| ECCTL2   | TSCTRSTOP | EC_RUN       |

### *Example 15-24. Code Snippet for Multichannel PWM Generation with Phase Control*

```
// Code snippet for APWM mode Example 3
// Run Time (Note: Example execution of one run-time instant)
//============================================================
// All phases are set to the same duty cycle
 ECAP1Regs.CAP2 = 700; // Set Duty cycle i.e. compare value = 700
 ECAP2Regs.CAP2 = 700; // Set Duty cycle i.e. compare value = 700
 ECAP3Regs.CAP2 = 700; // Set Duty cycle i.e. compare value = 700
```

### *15.3.4 Registers*

All 32-bit registers are aligned on even address boundaries and are organized in little-endian mode. The 16 least-significant bits of a 32-bit register are located on lowest address (even address).

**NOTE:** In APWM mode, writing to CAP1/CAP2 active registers also writes the same value to the corresponding shadow registers CAP3/CAP4. This emulates immediate mode. Writing to the shadow registers CAP3/CAP4 invokes the shadow mode.

#### **15.3.4.1 ECAP Registers**

Table [15-106](#page-70-0) lists the memory-mapped registers for the ECAP. All register offset addresses not listed in Table [15-106](#page-70-0) should be considered as reserved locations and the register contents should not be modified.

**Table 15-106. ECAP Registers**

| Offset | Acronym | Register Name                       | Section                |
|--------|---------|-------------------------------------|------------------------|
| 0h     | TSCTR   | Time-Stamp Counter Register         | Section 15.3.4.1.1     |
| 4h     | CTRPHS  | Counter Phase Offset Value Register | Section 15.3.4.1.2     |
| 8h     | CAP1    | Capture 1 Register                  | Section 15.3.4.1.3     |
| Ch     | CAP2    | Capture 2 Register                  | Section 15.3.4.1.4     |
| 10h    | CAP3    | Capture 3 Register                  | Section 15.3.4.1.5     |
| 14h    | CAP4    | Capture 4 Register                  | Section 15.3.4.1.6     |
| 28h    | ECCTL1  | Capture Control Register 1          | Section 15.3.4.1.7     |
| 2Ah    | ECCTL2  | Capture Control Register 2          | Section 15.3.4.1.8     |
| 2Ch    | ECEINT  | Capture Interrupt Enable Register   | Section 15.3.4.1.9     |
| 2Eh    | ECFLG   | Capture Interrupt Flag Register     | Section<br>15.3.4.1.10 |
| 30h    | ECCLR   | Capture Interrupt Clear Register    | Section<br>15.3.4.1.11 |
| 32h    | ECFRC   | Capture Interrupt Force Register    | Section<br>15.3.4.1.12 |
| 5Ch    | REVID   | Revision ID Register                | Section 15.4.3.25      |

#### *15.3.4.1.1 TSCTR Register (offset = 0h) [reset = 0h]*

TSCTR is shown in Figure [15-117](#page-71-1) and described in Table [15-107.](#page-71-2)

### **Figure 15-117. TSCTR Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15     | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|--------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | TSCTR  |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-0h |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-107. TSCTR Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                          |
|------|-------|------|-------|----------------------------------------------------------------------|
| 31-0 | TSCTR | R/W  | 0h    | Active 32 bit counter register that is used as the capture time-base |

#### *15.3.4.1.2 CTRPHS Register (offset = 4h) [reset = 0h]*

CTRPHS is shown in Figure [15-118](#page-72-1) and described in Table [15-108](#page-72-2).

### **Figure 15-118. CTRPHS Register**

| 31 | 30     | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15     | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|--------|----|----|----|----|----|----|----|----|----|----|----|----|----|----|--------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    | CTRPHS |    |    |    |    |    |    |    |    |    |    |    |    |    |    |        |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    |        |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-0h |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 15-108. CTRPHS Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                         |
|------|--------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | CTRPHS | R/W  | 0h    | Counter phase value register that can be programmed for phase<br>lag/lead.<br>This register shadows TSCTR and is loaded into TSCTR upon either<br>a SYNCI event or S/W force via a control bit.<br>Used to achieve phase control synchronization with respect to other<br>eCAP and EPWM time-bases. |

#### *15.3.4.1.3 CAP1 Register (offset = 8h) [reset = 0h]*

CAP1 is shown in Figure [15-119](#page-73-1) and described in Table [15-109](#page-73-2).

### **Figure 15-119. CAP1 Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-109. CAP1 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                     |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | CAP1  | R/W  | 0h    | This register can be loaded (written) by the following.<br>(a) Time-Stamp (that is, counter value) during a capture event.<br>(b) Software may be useful for test purposes.<br>(c) APRD active register when used in APWM mode. |

#### *15.3.4.1.4 CAP2 Register (offset = Ch) [reset = 0h]*

CAP2 is shown in Figure [15-120](#page-74-1) and described in Table [15-110](#page-74-2).

### **Figure 15-120. CAP2 Register**

| 31 | 30   | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15     | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|------|----|----|----|----|----|----|----|----|----|----|----|----|----|----|--------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    | CAP2 |    |    |    |    |    |    |    |    |    |    |    |    |    |    |        |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    |      |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-0h |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 15-110. CAP2 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                     |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | CAP2  | R/W  | 0h    | This register can be loaded (written) by the following.<br>(a) Time-Stamp (that is, counter value) during a capture event.<br>(b) Software may be useful for test purposes.<br>(c) ACMP active register when used in APWM mode. |

#### *15.3.4.1.5 CAP3 Register (offset = 10h) [reset = 0h]*

CAP3 is shown in Figure [15-121](#page-75-1) and described in Table [15-111](#page-75-2).

### **Figure 15-121. CAP3 Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-111. CAP3 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                   |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | CAP3  | R/W  | 0h    | In CMP mode, this is a time-stamp capture register.<br>In APWM mode, this is the period shadow (APRD) register.<br>You update the PWM period value through this register.<br>In this mode, CAP3 shadows CAP1. |

#### *15.3.4.1.6 CAP4 Register (offset = 14h) [reset = 0h]*

CAP4 is shown in Figure [15-122](#page-76-1) and described in Table [15-112](#page-76-2).

### **Figure 15-122. CAP4 Register**

| 31 | 30   | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15     | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|------|----|----|----|----|----|----|----|----|----|----|----|----|----|----|--------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    | CAP4 |    |    |    |    |    |    |    |    |    |    |    |    |    |    |        |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    |      |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-0h |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 15-112. CAP4 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                     |
|------|-------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | CAP4  | R/W  | 0h    | In CMP mode, this is a time-stamp capture register.<br>In APWM mode, this is the compare shadow (ACMP) register.<br>You update the PWM compare value through this register.<br>In this mode, CAP4 shadows CAP2. |

#### *15.3.4.1.7 ECCTL1 Register (offset = 28h) [reset = 0h]*

ECCTL1 is shown in Figure [15-123](#page-77-1) and described in Table [15-113.](#page-77-2)

### **Figure 15-123. ECCTL1 Register**

| 15      | 14        | 13      | 12       | 11      | 10      | 9       | 8       |  |  |
|---------|-----------|---------|----------|---------|---------|---------|---------|--|--|
|         | FREE_SOFT |         | PRESCALE |         |         |         |         |  |  |
|         | R/W-0h    |         |          | R/W-0h  |         |         | R/W-0h  |  |  |
| 7       | 6         | 5       | 4        | 3       | 2       | 1       | 0       |  |  |
| CTRRST4 | CAP4POL   | CTRRST3 | CAP3POL  | CTRRST2 | CAP2POL | CTRRST1 | CAP1POL |  |  |
| R/W-0h  | R/W-0h    | R/W-0h  | R/W-0h   | R/W-0h  | R/W-0h  | R/W-0h  | R/W-0h  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-113. ECCTL1 Register Field Descriptions**

| Bit   | Field                | Type | Reset | Description                                                                                                                                                                                                                                                           |  |  |
|-------|----------------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|
| 15-14 | FREE_SOFT            | R/W  | 0h    | Emulation Control<br>0h = TSCTR counter stops immediately on emulation suspend.<br>1h = TSCTR counter runs until = 0.<br>2h = TSCTR counter is unaffected by emulation suspend (Run<br>Free).<br>3h = TSCTR counter is unaffected by emulation suspend (Run<br>Free). |  |  |
| 13-9  | PRESCALE             | R/W  | 0h    | Event Filter prescale select<br>0h = Divide by 1 (i.e,. no prescale, by-pass the prescaler)<br>1h = Divide by 2<br>2h = Divide by 4<br>3h = Divide by 6<br>4h = Divide by 8<br>5h = Divide by 10<br>1Eh = Divide by 60<br>1Fh = Divide by 62                          |  |  |
| 8     | CAPLDEN              | R/W  | 0h    | Enable Loading of CAP1 to CAP4 registers on a capture event<br>0h = Disable CAP1 through 4 register loads at capture event time.<br>1h = Enable CAP1-4 register loads at capture event time.                                                                          |  |  |
| 7     | CTRRST4              | R/W  | 0h    | Counter Reset on Capture Event 4<br>0h = Do not reset counter on Capture Event 4 (absolute time stamp<br>operation)<br>1h = Reset counter after Capture Event 4 time-stamp has been<br>captured (used in difference mode operation)                                   |  |  |
| 6     | CAP4POL              | R/W  | 0h    | Capture Event 4 Polarity select<br>0h = Capture Event 4 triggered on a rising edge (RE)<br>1h = Capture Event 4 triggered on a falling edge (FE)                                                                                                                      |  |  |
| 5     | CTRRST3              | R/W  | 0h    | Counter Reset on Capture Event 3<br>0h = Do not reset counter on Capture Event 3 (absolute time stamp)<br>1h = Reset counter after Event 3 time-stamp has been captured<br>(used in difference mode operation)                                                        |  |  |
| 4     | CAP3POL              | R/W  | 0h    | Capture Event 3 Polarity select<br>0h = Capture Event 3 triggered on a rising edge (RE)<br>1h = Capture Event 3 triggered on a falling edge (FE)                                                                                                                      |  |  |
| 3     | CTRRST2<br>R/W<br>0h |      |       | Counter Reset on Capture Event 2<br>0h = Do not reset counter on Capture Event 2 (absolute time stamp)<br>1h = Reset counter after Event 2 time-stamp has been captured<br>(used in difference mode operation)                                                        |  |  |

**Table 15-113. ECCTL1 Register Field Descriptions (continued)**

| Bit | Field   | Type | Reset | Description                                                                                                                                                                                                    |
|-----|---------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 2   | CAP2POL | R/W  | 0h    | Capture Event 2 Polarity select<br>0h = Capture Event 2 triggered on a rising edge (RE)<br>1h = Capture Event 2 triggered on a falling edge (FE)                                                               |
| 1   | CTRRST1 | R/W  | 0h    | Counter Reset on Capture Event 1<br>0h = Do not reset counter on Capture Event 1 (absolute time stamp)<br>1h = Reset counter after Event 1 time-stamp has been captured<br>(used in difference mode operation) |
| 0   | CAP1POL | R/W  | 0h    | Capture Event 1 Polarity select<br>0h = Capture Event 1 triggered on a rising edge (RE)<br>1h = Capture Event 1 triggered on a falling edge (FE)                                                               |

#### *15.3.4.1.8 ECCTL2 Register (offset = 2Ah) [reset = 6h]*

ECCTL2 is shown in Figure [15-124](#page-79-1) and described in Table [15-114.](#page-79-2)

### **Figure 15-124. ECCTL2 Register**

| 15        | 14 | 13       | 12        | 11     | 10      | 9         | 8               |
|-----------|----|----------|-----------|--------|---------|-----------|-----------------|
|           |    | RESERVED |           |        | APWMPOL | CAP_APWM  | SWSYNC          |
|           |    | R-0h     |           |        | R/W-0h  | R/W-0h    | R/W-0h          |
| 7         | 6  | 5        | 4         | 3      | 2       | 1         | 0               |
| SYNCO_SEL |    | SYNCI_EN | TSCTRSTOP | RE-ARM |         | STOP_WRAP | CONT_ONESH<br>T |
| R/W-0h    |    | R/W-0h   | R/W-0h    | R/W-0h |         | R/W-3h    | R/W-0h          |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-114. ECCTL2 Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|-------|-----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-11 | RESERVED  | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 10    | APWMPOL   | R/W  | 0h    | APWM output polarity select.<br>This is applicable only in APWM operating mode<br>0h = Output is active high (Compare value defines high time)<br>1h = Output is active low (Compare value defines low time)                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 9     | CAP_APWM  | R/W  | 0h    | CAP/APWM operating mode select<br>0h = ECAP module operates in capture mode. This mode forces the<br>following configuration. (a) Inhibits TSCTR resets via PRDEQ event.<br>(b) Inhibits shadow loads on CAP1 and 2 registers. (c) Permits user<br>to enable CAP1-4 register load. (d) ECAPn/APWMn pin operates as<br>a capture input.<br>1h = ECAP module operates in APWM mode. This mode forces the<br>following configuration. (a) Resets TSCTR on PRDEQ event (period<br>boundary). (b) Permits shadow loading on CAP1 and 2 registers. (c)<br>Disables loading of time-stamps into CAP1-4 registers. (d)<br>ECAPn/APWMn pin operates as a APWM output. |
| 8     | SWSYNC    | R/W  | 0h    | Software-forced Counter (TSCTR) Synchronizing.<br>This provides a convenient software method to synchronize some or<br>all ECAP time bases.<br>In APWM mode, the synchronizing can also be done via the PRDEQ<br>event.<br>Note: Selecting PRDEQ is meaningful only in APWM mode.<br>However, you can choose it in CAP mode if you find doing so useful.<br>0h = Writing a zero has no effect. Reading always returns a zero<br>1h = Writing a one forces a TSCTR shadow load of current ECAP<br>module and any ECAP modules down-stream providing the<br>SYNCO_SEL bits are 0,0. After writing a 1, this bit returns to a zero.                             |
| 7-6   | SYNCO_SEL | R/W  | 0h    | Sync-Out Select<br>0h = Select sync-in event to be the sync-out signal (pass through)<br>1h = Select PRDEQ event to be the sync-out signal<br>2h = Disable sync out signal<br>3h = Disable sync out signal                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 5     | SYNCI_EN  | R/W  | 0h    | Counter (TSCTR) Sync-In select mode<br>0h = Disable sync-in option<br>1h = Enable counter (TSCTR) to be loaded from CTRPHS register<br>upon either a SYNCI signal or a S/W force event.                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 4     | TSCTRSTOP | R/W  | 0h    | Time Stamp (TSCTR) Counter Stop (freeze) Control<br>0h = TSCTR stopped<br>1h = TSCTR free-running                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |

**Table 15-114. ECCTL2 Register Field Descriptions (continued)**

| Bit | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|-----|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 3   | RE-ARM      | R/W  | 0h    | One-Shot Re-Arming Control, that is, wait for stop trigger.<br>Note: The re-arm function is valid in one shot or continuous mode.<br>0h = Has no effect (reading always returns a 0)<br>1h = Arms the one-shot sequence as follows: 1) Resets the Mod4<br>counter to zero. 2) Unfreezes the Mod4 counter. 3) Enables capture                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 2-1 | STOP_WRAP   | R/W  | 3h    | register loads.<br>Stop value for one-shot mode.<br>This is the number (between 1 and 4) of captures allowed to occur<br>before the CAP (1 through 4) registers are frozen, that is, capture<br>sequence is stopped.<br>Wrap value for continuous mode.<br>This is the number (between 1 and 4) of the capture register in which<br>the circular buffer wraps around and starts again.<br>Notes: STOP_WRAP is compared to Mod4 counter and, when<br>equal, the following two actions occur.<br>(1) Mod4 counter is stopped (frozen), and (2) Capture register loads<br>are inhibited.<br>In one-shot mode, further interrupt events are blocked until re<br>armed.<br>0h = Stop after Capture Event 1 in one-shot mode. Wrap after<br>Capture Event 1 in continuous mode.<br>1h = Stop after Capture Event 2 in one-shot mode. Wrap after<br>Capture Event 2 in continuous mode.<br>2h = Stop after Capture Event 3 in one-shot mode. Wrap after<br>Capture Event 3 in continuous mode.<br>3h = Stop after Capture Event 4 in one-shot mode. Wrap after<br>Capture Event 4 in continuous mode. |
| 0   | CONT_ONESHT | R/W  | 0h    | Continuous or one-shot mode control (applicable only in capture<br>mode)<br>0h = Operate in continuous mode<br>1h = Operate in one-shot mode                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |

#### *15.3.4.1.9 ECEINT Register (offset = 2Ch) [reset = 0h]*

ECEINT is shown in Figure [15-125](#page-81-1) and described in Table [15-115.](#page-81-2)

The interrupt enable bits (CEVTn) block any of the selected events from generating an interrupt. Events will still be latched into the flag bit (ECFLG register) and can be forced or cleared via the ECFRC and ECCLR registers. The proper procedure for configuring peripheral modes and interrupts is: 1. Disable global interrupts. 2. Stop eCAP counter. 3. Disable eCAP interrupts. 4. Configure peripheral registers. 5. Clear spurious eCAP interrupt flags. 6. Enable eCAP interrupts. 7. Start eCAP counter. 8. Enable global interrupts.

### **Figure 15-125. ECEINT Register**

| 15     | 14       | 13     | 12     | 11     | 10     | 9      | 8        |  |  |  |  |
|--------|----------|--------|--------|--------|--------|--------|----------|--|--|--|--|
|        | RESERVED |        |        |        |        |        |          |  |  |  |  |
|        | R-0h     |        |        |        |        |        |          |  |  |  |  |
| 7      | 6        | 5      | 4      | 3      | 2      | 1      | 0        |  |  |  |  |
| CMPEQ  | PRDEQ    | CNTOVF | CEVT4  | CEVT3  | CEVT2  | CEVT1  | RESERVED |  |  |  |  |
| R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R-0h     |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 15-115. ECEINT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                         |  |  |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------|--|--|
| 15-8 | RESERVED | R    | 0h    |                                                                                                                                                     |  |  |
| 7    | CMPEQ    | R/W  | 0h    | Counter Equal Compare Interrupt Enable.<br>0h = Disable Compare Equal as an Interrupt source.<br>1h = Enable Compare Equal as an Interrupt source.  |  |  |
| 6    | PRDEQ    | R/W  | 0h    | Counter Equal Period Interrupt Enable.<br>0h = Disable Period Equal as an Interrupt source.<br>1h = Enable Period Equal as an Interrupt source.     |  |  |
| 5    | CNTOVF   | R/W  | 0h    | Counter Overflow Interrupt Enable.<br>0h = Disable counter Overflow as an Interrupt source.<br>1h = Enable counter Overflow as an Interrupt source. |  |  |
| 4    | CEVT4    | R/W  | 0h    | Capture Event 4 Interrupt Enable.<br>0h = Disable Capture Event 4 as an Interrupt source.<br>1h = Enable Capture Event 4 as an Interrupt source.    |  |  |
| 3    | CEVT3    | R/W  | 0h    | Capture Event 3 Interrupt Enable.<br>0h = Disable Capture Event 3 as an Interrupt source.<br>1h = Enable Capture Event 3 as an Interrupt source.    |  |  |
| 2    | CEVT2    | R/W  | 0h    | Capture Event 2 Interrupt Enable.<br>0h = Disable Capture Event 2 as an Interrupt source.<br>1h = Enable Capture Event 2 as an Interrupt source.    |  |  |
| 1    | CEVT1    | R/W  | 0h    | Capture Event 1 Interrupt Enable .<br>0h = Disable Capture Event 1 as an Interrupt source.<br>1h = Enable Capture Event 1 as an Interrupt source.   |  |  |
| 0    | RESERVED | R    | 0h    |                                                                                                                                                     |  |  |

#### *15.3.4.1.10 ECFLG Register (offset = 2Eh) [reset = 0h]*

ECFLG is shown in Figure [15-126](#page-82-1) and described in Table [15-116.](#page-82-2)

## **Figure 15-126. ECFLG Register**

| 15    | 14       | 13     | 12    | 11    | 10    | 9     | 8    |  |  |  |  |
|-------|----------|--------|-------|-------|-------|-------|------|--|--|--|--|
|       | RESERVED |        |       |       |       |       |      |  |  |  |  |
|       | R-0h     |        |       |       |       |       |      |  |  |  |  |
| 7     | 6        | 5      | 4     | 3     | 2     | 1     | 0    |  |  |  |  |
| CMPEQ | PRDEQ    | CNTOVF | CEVT4 | CEVT3 | CEVT2 | CEVT1 | INT  |  |  |  |  |
| R-0h  | R-0h     | R-0h   | R-0h  | R-0h  | R-0h  | R-0h  | R-0h |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-116. ECFLG Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                   |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------|
| 15-8 | RESERVED | R    | 0h    |                                                                                               |
| 7    | CMPEQ    | R    | 0h    | Compare Equal Compare Status Flag.<br>This flag is only active in APWM mode.                  |
|      |          |      |       | 0h = Indicates no event occurred                                                              |
|      |          |      |       | 1h = Indicates the counter (TSCTR) reached the compare register<br>value (ACMP)               |
| 6    | PRDEQ    | R    | 0h    | Counter Equal Period Status Flag.<br>This flag is only active in APWM mode.                   |
|      |          |      |       | 0h = Indicates no event occurred                                                              |
|      |          |      |       | 1h = Indicates the counter (TSCTR) reached the period register<br>value (APRD) and was reset. |
| 5    | CNTOVF   | R    | 0h    | Counter Overflow Status Flag.<br>This flag is active in CAP and APWM mode.                    |
|      |          |      |       | 0h = Indicates no event occurred.                                                             |
|      |          |      |       | 1h = Indicates the counter (TSCTR) has made the transition from<br>0xFFFFFFFF to 0x00000000   |
| 4    | CEVT4    | R    | 0h    | Capture Event 4 Status Flag This flag is only active in CAP mode.                             |
|      |          |      |       | 0h = Indicates no event occurred                                                              |
|      |          |      |       | 1h = Indicates the fourth event occurred at ECAPn pin                                         |
| 3    | CEVT3    | R    | 0h    | Capture Event 3 Status Flag.<br>This flag is active only in CAP mode.                         |
|      |          |      |       | 0h = Indicates no event occurred.                                                             |
|      |          |      |       | 1h = Indicates the third event occurred at ECAPn pin.                                         |
| 2    | CEVT2    | R    | 0h    | Capture Event 2 Status Flag.<br>This flag is only active in CAP mode.                         |
|      |          |      |       | 0h = Indicates no event occurred.                                                             |
|      |          |      |       | 1h = Indicates the second event occurred at ECAPn pin.                                        |
| 1    | CEVT1    | R    | 0h    | Capture Event 1 Status Flag.<br>This flag is only active in CAP mode.                         |
|      |          |      |       | 0h = Indicates no event occurred.                                                             |
|      |          |      |       | 1h = Indicates the first event occurred at ECAPn pin.                                         |
| 0    | INT      | R    | 0h    | Global Interrupt Status Flag                                                                  |
|      |          |      |       | 0h = Indicates no interrupt generated.                                                        |
|      |          |      |       | 1h = Indicates that an interrupt was generated.                                               |

#### *15.3.4.1.11 ECCLR Register (offset = 30h) [reset = 0h]*

ECCLR is shown in Figure [15-127](#page-83-1) and described in Table [15-117](#page-83-2).

### **Figure 15-127. ECCLR Register**

| 15     | 14       | 13     | 12     | 11     | 10     | 9      | 8      |  |  |  |  |
|--------|----------|--------|--------|--------|--------|--------|--------|--|--|--|--|
|        | RESERVED |        |        |        |        |        |        |  |  |  |  |
|        | R-0h     |        |        |        |        |        |        |  |  |  |  |
| 7      | 6        | 5      | 4      | 3      | 2      | 1      | 0      |  |  |  |  |
| CMPEQ  | PRDEQ    | CNTOVF | CEVT4  | CEVT3  | CEVT2  | CEVT1  | INT    |  |  |  |  |
| R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R/W-0h |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-117. ECCLR Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                            |  |  |
|------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|
| 15-8 | RESERVED | R    | 0h    |                                                                                                                                                                                                                        |  |  |
| 7    | CMPEQ    | R/W  | 0h    | Counter Equal Compare Status Flag<br>0h = Writing a 0 has no effect. Always reads back a 0<br>1h = Writing a 1 clears the CMPEQ flag condition                                                                         |  |  |
| 6    | PRDEQ    | R/W  | 0h    | Counter Equal Period Status Flag<br>0h = Writing a 0 has no effect. Always reads back a 0<br>1h = Writing a 1 clears the PRDEQ flag condition                                                                          |  |  |
| 5    | CNTOVF   | R/W  | 0h    | Counter Overflow Status Flag<br>0h = Writing a 0 has no effect. Always reads back a 0<br>1h = Writing a 1 clears the CNTOVF flag condition                                                                             |  |  |
| 4    | CEVT4    | R/W  | 0h    | Capture Event 4 Status Flag<br>0h = Writing a 0 has no effect. Always reads back a 0.<br>1h = Writing a 1 clears the CEVT3 flag condition.                                                                             |  |  |
| 3    | CEVT3    | R/W  | 0h    | Capture Event 3 Status Flag<br>0h = Writing a 0 has no effect. Always reads back a 0.<br>1h = Writing a 1 clears the CEVT3 flag condition.                                                                             |  |  |
| 2    | CEVT2    | R/W  | 0h    | Capture Event 2 Status Flag<br>0h = Writing a 0 has no effect. Always reads back a 0.<br>1h = Writing a 1 clears the CEVT2 flag condition.                                                                             |  |  |
| 1    | CEVT1    | R/W  | 0h    | Capture Event 1 Status Flag<br>0h = Writing a 0 has no effect. Always reads back a 0.<br>1h = Writing a 1 clears the CEVT1 flag condition.                                                                             |  |  |
| 0    | INT      | R/W  | 0h    | Global Interrupt Clear Flag<br>0h = Writing a 0 has no effect. Always reads back a 0.<br>1h = Writing a 1 clears the INT flag and enable further interrupts to<br>be generated if any of the event flags are set to 1. |  |  |

#### *15.3.4.1.12 ECFRC Register (offset = 32h) [reset = 0h]*

ECFRC is shown in Figure [15-128](#page-84-1) and described in Table [15-118.](#page-84-2)

## **Figure 15-128. ECFRC Register**

| 15     | 14       | 13     | 12     | 11     | 10     | 9      | 8        |  |  |  |  |
|--------|----------|--------|--------|--------|--------|--------|----------|--|--|--|--|
|        | RESERVED |        |        |        |        |        |          |  |  |  |  |
|        | R-0h     |        |        |        |        |        |          |  |  |  |  |
| 7      | 6        | 5      | 4      | 3      | 2      | 1      | 0        |  |  |  |  |
| CMPEQ  | PRDEQ    | CNTOVF | CEVT4  | CEVT3  | CEVT2  | CEVT1  | RESERVED |  |  |  |  |
| R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R-0h     |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-118. ECFRC Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                  |
|------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------|
| 15-8 | RESERVED | R    | 0h    |                                                                                                                              |
| 7    | CMPEQ    | R/W  | 0h    | Force Counter Equal Compare Interrupt<br>0h = No effect. Always reads back a 0.<br>1h = Writing a 1 sets the CMPEQ flag bit. |
| 6    | PRDEQ    | R/W  | 0h    | Force Counter Equal Period Interrupt<br>0h = No effect. Always reads back a 0.<br>1h = Writing a 1 sets the PRDEQ flag bit.  |
| 5    | CNTOVF   | R/W  | 0h    | Force Counter Overflow<br>0h = No effect. Always reads back a 0.<br>1h = Writing a 1 to this bit sets the CNTOVF flag bit.   |
| 4    | CEVT4    | R/W  | 0h    | Force Capture Event 4<br>0h = No effect. Always reads back a 0.<br>1h = Writing a 1 sets the CEVT4 flag bit                  |
| 3    | CEVT3    | R/W  | 0h    | Force Capture Event 3<br>0h = No effect. Always reads back a 0.<br>1h = Writing a 1 sets the CEVT3 flag bit                  |
| 2    | CEVT2    | R/W  | 0h    | Force Capture Event 2<br>0h = No effect. Always reads back a 0.<br>1h = Writing a 1 sets the CEVT2 flag bit.                 |
| 1    | CEVT1    | R/W  | 0h    | Always reads back a 0.<br>Force Capture Event 1<br>0h = No effect.<br>1h = Writing a 1 sets the CEVT1 flag bit.              |
| 0    | RESERVED | R    | 0h    |                                                                                                                              |

#### *15.3.4.1.13 REVID Register (offset = 5Ch) [reset = 44D22100h]*

REVID is shown in Figure [15-174](#page-133-1) and described in Table [15-146.](#page-133-2)

### **Figure 15-129. REVID Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-119. REVID Register Field Descriptions**

| Bit  | Field | Type | Reset     | Description  |
|------|-------|------|-----------|--------------|
| 31-0 | REV   | R    | 44D22100h | Revision ID. |

## **15.4 Enhanced Quadrature Encoder Pulse (eQEP) Module**

### *15.4.1 Introduction*

A single track of slots patterns the periphery of an incremental encoder disk, as shown in Figure [15-130](#page-86-0). These slots create an alternating pattern of dark and light lines. The disk count is defined as the number of dark/light line pairs that occur per revolution (lines per revolution). As a rule, a second track is added to generate a signal that occurs once per revolution (index signal: QEPI), which can be used to indicate an absolute position. Encoder manufacturers identify the index pulse using different terms such as index, marker, home position, and zero reference.

**Figure 15-130. Optical Encoder Disk**

To derive direction information, the lines on the disk are read out by two different photo-elements that "look" at the disk pattern with a mechanical shift of 1/4 the pitch of a line pair between them. This shift is realized with a reticle or mask that restricts the view of the photo-element to the desired part of the disk lines. As the disk rotates, the two photo-elements generate signals that are shifted 90 degrees out of phase from each other. These are commonly called the quadrature QEPA and QEPB signals. The clockwise direction for most encoders is defined as the QEPA channel going positive before the QEPB channel and vise versa as shown in Figure [15-131.](#page-87-0)

The encoder wheel typically makes one revolution for every revolution of the motor or the wheel may be at a geared rotation ratio with respect to the motor. Therefore, the frequency of the digital signal coming from the QEPA and QEPB outputs varies proportionally with the velocity of the motor. For example, a 2000-line encoder directly coupled to a motor running at 5000 revolutions per minute (rpm) results in a frequency of 166.6 KHz, so by measuring the frequency of either the QEPA or QEPB output, the processor can determine the velocity of the motor.

T0 0 1 2 3 4 5 6 7 N−6 N−5 N−4 N−3 N−2 N−1 0 QEPA QEPB QEPI Clockwise shaft rotation/forward movement Anti-clockwise shaft rotation/reverse movement 0 N−1 N−2 N−3 N−4 N−5 N−6 N−7 6 5 4 3 2 1 0 N−1 N−2 QEPA QEPB QEPI T0

**Figure 15-131. QEP Encoder Output Signal for Forward/Reverse Movement**

**Legend:** N = lines per revolution

Quadrature encoders from different manufacturers come with two forms of index pulse (gated index pulse or ungated index pulse) as shown in Figure [15-132.](#page-87-1) A nonstandard form of index pulse is ungated. In the ungated configuration, the index edges are not necessarily coincident with A and B signals. The gated index pulse is aligned to any of the four quadrature edges and width of the index pulse and can be equal to a quarter, half, or full period of the quadrature signal.

Some typical applications of shaft encoders include robotics and even computer input in the form of a mouse. Inside your mouse you can see where the mouse ball spins a pair of axles (a left/right, and an up/down axle). These axles are connected to optical shaft encoders that effectively tell the computer how fast and in what direction the mouse is moving.

**General Issues:** Estimating velocity from a digital position sensor is a cost-effective strategy in motor control. Two different first order approximations for velocity may be written as:

$$v(k) \approx \frac{x(k) - x(k-1)}{T} = \frac{\Delta X}{T}$$
 (1)

$$v(k) \approx \frac{X}{t(k) - t(k-1)} = \frac{X}{\Delta T}$$
(2)

where

v(k): Velocity at time instant k

x(k): Position at time instant k

x(k-1): Position at time instant k - 1

T: Fixed unit time or inverse of velocity calculation rate

ΔX: Incremental position movement in unit time

t(k): Time instant "k"

t(k-1): Time instant "k - 1"

X: Fixed unit position

ΔT: Incremental time elapsed for unit position movement.

[Equation](#page-88-0) 1 is the conventional approach to velocity estimation and it requires a time base to provide unit time event for velocity calculation. Unit time is basically the inverse of the velocity calculation rate.

The encoder count (position) is read once during each unit time event. The quantity [x(k) - x(k-1)] is formed by subtracting the previous reading from the current reading. Then the velocity estimate is computed by multiplying by the known constant 1/T (where T is the constant time between unit time events and is known in advance).

Estimation based on [Equation](#page-88-0) 1 has an inherent accuracy limit directly related to the resolution of the position sensor and the unit time period T. For example, consider a 500-line per revolution quadrature encoder with a velocity calculation rate of 400 Hz. When used for position the quadrature encoder gives a four-fold increase in resolution, in this case, 2000 counts per revolution. The minimum rotation that can be detected is therefore 0.0005 revolutions, which gives a velocity resolution of 12 rpm when sampled at 400 Hz. While this resolution may be satisfactory at moderate or high speeds, for example, 1% error at 1200 rpm, it would clearly prove inadequate at low speeds. In fact, at speeds below 12 rpm, the speed estimate would erroneously be zero much of the time.

At low speed, [Equation](#page-88-1) 2 provides a more accurate approach. It requires a position sensor that outputs a fixed interval pulse train, such as the aforementioned quadrature encoder. The width of each pulse is defined by motor speed for a given sensor resolution. [Equation](#page-88-1) 2 can be used to calculate motor speed by measuring the elapsed time between successive quadrature pulse edges. However, this method suffers from the opposite limitation, as does [Equation](#page-88-0) 1. A combination of relatively large motor speeds and high sensor resolution makes the time interval ΔT small, and thus more greatly influenced by the timer resolution. This can introduce considerable error into high-speed estimates.

For systems with a large speed range (that is, speed estimation is needed at both low and high speeds), one approach is to use [Equation](#page-88-1) 2 at low speed and have the DSP software switch over to [Equation](#page-88-0) 1 when the motor speed rises above some specified threshold.

### *15.4.2 Functional Description*

This section provides the eQEP inputs and functional description.

**NOTE:** Multiple identical eQEP modules can be contained in a system. The number of modules is device-dependent and is based on target application needs. In this document, the letter x within a signal or module name is used to indicate a generic eQEP instance on a device.

#### **15.4.2.1 EQEP Inputs**

The eQEP inputs include two pins for quadrature-clock mode or direction-count mode, an index (or 0 marker), and a strobe input.

- QEPA/XCLK and QEPB/XDIR: These two pins can be used in quadrature-clock mode or directioncount mode.
  - Quadrature-clock Mode: The eQEP encoders provide two square wave signals (A and B) 90 electrical degrees out of phase whose phase relationship is used to determine the direction of rotation of the input shaft and number of eQEP pulses from the index position to derive the relative position information. For forward or clockwise rotation, QEPA signal leads QEPB signal and vice versa. The quadrature decoder uses these two inputs to generate quadrature-clock and direction signals.
  - Direction-count Mode: In direction-count mode, direction and clock signals are provided directly from the external source. Some position encoders have this type of output instead of quadrature output. The QEPA pin provides the clock input and the QEPB pin provides the direction input.
- QEPI: Index or Zero Marker: The eQEP encoder uses an index signal to assign an absolute start position from which position information is incrementally encoded using quadrature pulses. This pin is connected to the index output of the eQEP encoder to optionally reset the position counter for each revolution. This signal can be used to initialize or latch the position counter on the occurrence of a desired event on the index pin.
- QEPS: Strobe Input: This general-purpose strobe signal can initialize or latch the position counter on the occurrence of a desired event on the strobe pin. This signal is typically connected to a sensor or limit switch to notify that the motor has reached a defined position.

#### **15.4.2.2 Functional Description**

The eQEP peripheral contains the following major functional units (as shown in Figure [15-133](#page-90-0)):

- Programmable input qualification for each pin (part of the GPIO MUX)
- Quadrature decoder unit (QDU)
- Position counter and control unit for position measurement (PCCU)
- Quadrature edge-capture unit for low-speed measurement (QCAP)
- Unit time base for speed/frequency measurement (UTIME)
- Watchdog timer for detecting stalls (QWDOG)

QWDTMR QWDPRD 16 UTIME QWDOG QUPRD QUTMR 32 UTOUT WDTOUT Quadrature capture unit (QCAP) QCPRDLAT QCTMRLAT 16 QFLG QEPSTS QEPCTL Registers used by multiple units QCLK QDIR QI QS PHE PCSOUT Quadrature decoder (QDU) QDECCTL 16 Position counter/ control unit QPOSLAT (PCCU) QPOSSLAT 32 QPOSILAT EQEPxAIN EQEPxBIN EQEPxIIN EQEPxIOUT EQEPxIOE EQEPxSIN EQEPxSOUT EQEPxSOE GPIO MUX EQEPxA/XCLK EQEPxB/XDIR EQEPxS EQEPxI QPOSCMP QEINT QFRC 32 QCLR QPOSCTL 32 16 QPOSCNT QPOSMAX QPOSINIT Interrupt Controller EQEPxINT Enhanced QEP (eQEP) peripheral System control registers QCTMR QCPRD 16 16 QCAPCTL EQEPxENCLK SYSCLKOUT Data bus To CPU

**Figure 15-133. Functional Block Diagram of the eQEP Peripheral**

#### **15.4.2.3 Quadrature Decoder Unit (QDU)**

Figure [15-134](#page-91-0) shows a functional block diagram of the QDU.

**Figure 15-134. Functional Block Diagram of Decoder Unit**

#### *15.4.2.3.1 Position Counter Input Modes*

Clock and direction input to position counter is selected using the QSRC bit in the eQEP decoder control register (QDECCTL), based on interface input requirement as follows:

- Quadrature-count mode
- Direction-count mode
- UP-count mode
- DOWN-count mode

#### *15.4.2.3.1.1 Quadrature Count Mode*

The quadrature decoder generates the direction and clock to the position counter in quadrature count mode.

**Direction Decoding—** The direction decoding logic of the eQEP circuit determines which one of the sequences (QEPA, QEPB) is the leading sequence and accordingly updates the direction information in the QDF bit in the eQEP status register (QEPSTS). Table [15-120](#page-92-0) and Figure [15-135](#page-93-0) show the direction decoding logic in truth table and state machine form. Both edges of the QEPA and QEPB signals are sensed to generate count pulses for the position counter. Therefore, the frequency of the clock generated by the eQEP logic is four times that of each input sequence. Figure [15-136](#page-93-1) shows the direction decoding and clock generation from the eQEP input signals.

**Phase Error Flag—** In normal operating conditions, quadrature inputs QEPA and QEPB will be 90 degrees out of phase. The phase error flag (PHE) is set in the QFLG register when edge transition is detected simultaneously on the QEPA and QEPB signals to optionally generate interrupts. State transitions marked by dashed lines in Figure [15-135](#page-93-0) are invalid transitions that generate a phase error.

**Count Multiplication—** The eQEP position counter provides 4x times the resolution of an input clock by generating a quadrature-clock (QCLK) on the rising/falling edges of both eQEP input clocks (QEPA and QEPB) as shown in Figure [15-136](#page-93-1).

**Reverse Count—** In normal quadrature count operation, QEPA input is fed to the QA input of the quadrature decoder and the QEPB input is fed to the QB input of the quadrature decoder. Reverse counting is enabled by setting the SWAP bit in the eQEP decoder control register (QDECCTL). This will swap the input to the quadrature decoder thereby reversing the counting direction.

|               | Table 15-120. Quadrature Decoder Truth Table |      |         |
|---------------|----------------------------------------------|------|---------|
| Previous Edge | Present Edge                                 | QDIR | QPOSCNT |

| Previous Edge | Present Edge | QDIR   | QPOSCNT                |
|---------------|--------------|--------|------------------------|
| QA↑           | QB↑          | UP     | Increment              |
|               | QB↓          | DOWN   | Decrement              |
|               | QA↓          | TOGGLE | Increment or Decrement |
| QA↓           | QB↓          | UP     | Increment              |
|               | QB↑          | DOWN   | Decrement              |
|               | QA↑          | TOGGLE | Increment or Decrement |
| QB↑           | QA↑          | DOWN   | Increment              |
|               | QA↓          | UP     | Decrement              |
|               | QB↓          | TOGGLE | Increment or Decrement |
| QB↓           | QA↓          | DOWN   | Increment              |
|               | QA↑          | UP     | Decrement              |
|               | QB↑          | TOGGLE | Increment or Decrement |

**Figure 15-135. Quadrature Decoder State Machine**

**Figure 15-136. Quadrature-clock and Direction Decoding**

#### *15.4.2.3.1.2 Direction-count Mode*

Some position encoders provide direction and clock outputs, instead of quadrature outputs. In such cases, direction-count mode can be used. QEPA input will provide the clock for position counter and the QEPB input will have the direction information. The position counter is incremented on every rising edge of a QEPA input when the direction input is high and decremented when the direction input is low.

#### *15.4.2.3.1.3 Up-Count Mode*

The counter direction signal is hard-wired for up count and the position counter is used to measure the frequency of the QEPA input. Setting of the XCR bit in the eQEP decoder control register (QDECCTL) enables clock generation to the position counter on both edges of the QEPA input, thereby increasing the measurement resolution by 2× factor.

#### *15.4.2.3.1.4 Down-Count Mode*

The counter direction signal is hardwired for a down count and the position counter is used to measure the frequency of the QEPA input. Setting of the XCR bit in the eQEP decoder control register (QDECCTL) enables clock generation to the position counter on both edges of a QEPA input, thereby increasing the measurement resolution by 2× factor.

#### *15.4.2.3.2 eQEP Input Polarity Selection*

Each eQEP input can be inverted using the in the eQEP decoder control register (QDECCTL[8:5]) control bits. As an example, setting of the QIP bit in QDECCTL inverts the index input.

#### *15.4.2.3.3 Position-Compare Sync Output*

The eQEP peripheral includes a position-compare unit that is used to generate the position-compare sync signal on compare match between the position counter register (QPOSCNT) and the position-compare register (QPOSCMP). This sync signal can be output using an index pin or strobe pin of the EQEP peripheral.

Setting the SOEN bit in the eQEP decoder control register (QDECCTL) enables the position-compare sync output and the SPSEL bit in QDECCTL selects either an eQEP index pin or an eQEP strobe pin.

#### **15.4.2.4 Position Counter and Control Unit (PCCU)**

The position counter and control unit provides two configuration registers (QEPCTL and QPOSCTL) for setting up position counter operational modes, position counter initialization/latch modes and positioncompare logic for sync signal generation.

#### *15.4.2.4.1 Position Counter Operating Modes*

Position counter data may be captured in different manners. In some systems, the position counter is accumulated continuously for multiple revolutions and the position counter value provides the position information with respect to the known reference. An example of this is the quadrature encoder mounted on the motor controlling the print head in the printer. Here the position counter is reset by moving the print head to the home position and then position counter provides absolute position information with respect to home position.

In other systems, the position counter is reset on every revolution using index pulse and position counter provides rotor angle with respect to index pulse position.

Position counter can be configured to operate in following four modes

- Position Counter Reset on Index Event
- Position Counter Reset on Maximum Position
- Position Counter Reset on the first Index Event
- Position Counter Reset on Unit Time Out Event (Frequency Measurement)

In all the above operating modes, position counter is reset to 0 on overflow and to QPOSMAX register value on underflow. Overflow occurs when the position counter counts up after QPOSMAX value. Underflow occurs when position counter counts down after "0". Interrupt flag is set to indicate overflow/underflow in QFLG register.

#### 15.4.2.4.1.1 Position Counter Reset on Index Event (QEPCTL[PCRM] = 00)

If the index event occurs during the forward movement, then position counter is reset to 0 on the next eQEP clock. If the index event occurs during the reverse movement, then the position counter is reset to the value in the QPOSMAX register on the next eQEP clock.

First index marker is defined as the quadrature edge following the first index edge. The eQEP peripheral records the occurrence of the first index marker (QEPSTS[FIMF]) and direction on the first index event marker (QEPSTS[FIDF]) in QEPSTS registers, it also remembers the quadrature edge on the first index marker so that same relative quadrature transition is used for index event reset operation.

For example, if the first reset operation occurs on the falling edge of QEPB during the forward direction, then all the subsequent reset must be aligned with the falling edge of QEPB for the forward rotation and on the rising edge of QEPB for the reverse rotation as shown in Figure 15-137.

The position-counter value is latched to the QPOSILAT register and direction information is recorded in the QEPSTS[QDLF] bit on every index event marker. The position-counter error flag (QEPSTS[PCEF]) and error interrupt flag (QFLG[PCE]) are set if the latched value is not equal to 0 or QPOSMAX. The position-counter error flag (QEPSTS[PCEF]) is updated on every index event marker and an interrupt flag (QFLG[PCE]) will be set on error that can be cleared only through software.

The index event latch configuration QEPCTL[IEL] bits are ignored in this mode and position counter error flag/interrupt flag are generated only in index event reset mode.

Figure 15-137. Position Counter Reset by Index Pulse for 1000 Line Encoder (QPOSMAX = 3999 or F9Fh)

#### *15.4.2.4.1.2 Position Counter Reset on Maximum Position (QEPCTL[PCRM]=01)*

If the position counter is equal to QPOSMAX, then the position counter is reset to 0 on the next eQEP clock for forward movement and position counter overflow flag is set. If the position counter is equal to ZERO, then the position counter is reset to QPOSMAX on the next QEP clock for reverse movement and position counter underflow flag is set. Figure [15-138](#page-96-0) shows the position counter reset operation in this mode.

First index marker is defined as the quadrature edge following the first index edge. The eQEP peripheral records the occurrence of the first index marker (QEPSTS[FIMF]) and direction on the first index event marker (QEPSTS[FIDF]) in the QEPSTS registers; it also remembers the quadrature edge on the first index marker so that the same relative quadrature transition is used for the software index marker (QEPCTL[IEL]=11).

#### *15.4.2.4.1.3 Position Counter Reset on the First Index Event (QEPCTL[PCRM] = 10)*

If the index event occurs during forward movement, then the position counter is reset to 0 on the next eQEP clock. If the index event occurs during the reverse movement, then the position counter is reset to the value in the QPOSMAX register on the next eQEP clock. Note that this is done only on the first occurrence and subsequently the position counter value is not reset on an index event; rather, it is reset based on maximum position as described in Section [15.4.2.4.1.2.](#page-96-1)

First index marker is defined as the quadrature edge following the first index edge. The eQEP peripheral records the occurrence of the first index marker (QEPSTS[FIMF]) and direction on the first index event marker (QEPSTS[FIDF]) in QEPSTS registers. It also remembers the quadrature edge on the first index marker so that same relative quadrature transition is used for software index marker (QEPCTL[IEL]=11).

#### *15.4.2.4.1.4 Position Counter Reset on Unit Time out Event (QEPCTL[PCRM] = 11)*

In this mode, the QPOSCNT value is latched to the QPOSLAT register and then the QPOSCNT is reset (to 0 or QPOSMAX, depending on the direction mode selected by QDECCTL[QSRC] bits on a unit time event). This is useful for frequency measurement.

#### *15.4.2.4.2 Position Counter Latch*

The eQEP index and strobe input can be configured to latch the position counter (QPOSCNT) into QPOSILAT and QPOSSLAT, respectively, on occurrence of a definite event on these pins.

#### *15.4.2.4.2.1 Index Event Latch*

In some applications, it may not be desirable to reset the position counter on every index event and instead it may be required to operate the position counter in full 32-bit mode (QEPCTL[PCRM] = 01 and QEPCTL[PCRM] = 10 modes).

In such cases, the eQEP position counter can be configured to latch on the following events and direction information is recorded in the QEPSTS[QDLF] bit on every index event marker.

- Latch on Rising edge (QEPCTL[IEL] = 01)
- Latch on Falling edge (QEPCTL[IEL] = 10)
- Latch on Index Event Marker (QEPCTL[IEL] = 11)

This is particularly useful as an error checking mechanism to check if the position counter accumulated the correct number of counts between index events. As an example, the 1000-line encoder must count 4000 times when moving in the same direction between the index events.

The index event latch interrupt flag (QFLG[IEL]) is set when the position counter is latched to the QPOSILAT register. The index event latch configuration bits (QEPCTZ[IEL]) are ignored when QEPCTL[PCRM] = 00.

- **Latch on Rising Edge (QEPCTL[IEL] = 01)—** The position counter value (QPOSCNT) is latched to the QPOSILAT register on every rising edge of an index input.
- **Latch on Falling Edge (QEPCTL[IEL] = 10)—** The position counter value (QPOSCNT) is latched to the QPOSILAT register on every falling edge of index input.
- **Latch on Index Event Marker/Software Index Marker (QEPCTL[IEL] = 11)—** The first index marker is defined as the quadrature edge following the first index edge. The eQEP peripheral records the occurrence of the first index marker (QEPSTS[FIMF]) and direction on the first index event marker (QEPSTS[FIDF]) in the QEPSTS registers. It also remembers the quadrature edge on the first index marker so that same relative quadrature transition is used for latching the position counter (QEPCTL[IEL] = 11).

Figure [15-139](#page-98-0) shows the position counter latch using an index event marker.

#### *15.4.2.4.2.2 Strobe Event Latch*

The position-counter value is latched to the QPOSSLAT register on the rising edge of the strobe input by clearing the QEPCTL[SEL] bit.

If the QEPCTL[SEL] bit is set, then the position counter value is latched to the QPOSSLAT register on the rising edge of the strobe input for forward direction and on the falling edge of the strobe input for reverse direction as shown in Figure [15-140](#page-99-0).

The strobe event latch interrupt flag (QFLG[SEL]) is set when the position counter is latched to the QPOSSLAT register.

#### *15.4.2.4.3 Position Counter Initialization*

The position counter can be initialized using following events:

- Index event
- Strobe event
- Software initialization

**Index Event Initialization (IEI)—** The QEPI index input can be used to trigger the initialization of the position counter at the rising or falling edge of the index input.

If the QEPCTL[IEI] bits are 10, then the position counter (QPOSCNT) is initialized with a value in the QPOSINIT register on the rising edge of strobe input for forward direction and on the falling edge of strobe input for reverse direction.

The index event initialization interrupt flag (QFLG[IEI]) is set when the position counter is initialized with a value in the QPOSINIT register.

**Strobe Event Initialization (SEI)—** If the QEPCTL[SEI] bits are 10, then the position counter is initialized with a value in the QPOSINIT register on the rising edge of strobe input.

If the QEPCTL[SEL] bits are 11, then the position counter (QPOSCNT) is initialized with a value in the QPOSINIT register on the rising edge of strobe input for forward direction and on the falling edge of strobe input for reverse direction.

The strobe event initialization interrupt flag (QFLG[SEI]) is set when the position counter is initialized with a value in the QPOSINIT register.

**Software Initialization (SWI)—** The position counter can be initialized in software by writing a 1 to the QEPCTL[SWI] bit, which will automatically be cleared after initialization.

#### *15.4.2.4.4 eQEP Position-compare Unit*

The eQEP peripheral includes a position-compare unit that is used to generate a sync output and/or interrupt on a position-compare match. Figure [15-141](#page-100-0) shows a diagram. The position-compare (QPOSCMP) register is shadowed and shadow mode can be enabled or disabled using the QPOSCTL[PSSHDW] bit. If the shadow mode is not enabled, the CPU writes directly to the active position compare register.

**Figure 15-141. eQEP Position-compare Unit**

In shadow mode, you can configure the position-compare unit (QPOSCTL[PCLOAD]) to load the shadow register value into the active register on the following events and to generate the position-compare ready (QFLG[PCR]) interrupt after loading.

- Load on compare match
- Load on position-counter zero event

The position-compare match (QFLG[PCM]) is set when the position-counter value (QPOSCNT) matches with the active position-compare register (QPOSCMP) and the position-compare sync output of the programmable pulse width is generated on compare match to trigger an external device.

For example, if QPOSCMP = 2, the position-compare unit generates a position-compare event on 1 to 2 transitions of the eQEP position counter for forward counting direction and on 3 to 2 transitions of the eQEP position counter for reverse counting direction (see Figure [15-142\)](#page-101-0).

shows the layout of the eQEP Position-Compare Control Register (QPOSCTL) and describes the QPOSCTL bit fields.

**Figure 15-142. eQEP Position-compare Event Generation Points**

The pulse stretcher logic in the position-compare unit generates a programmable position-compare sync pulse output on the position-compare match. In the event of a new position-compare match while a previous position-compare pulse is still active, then the pulse stretcher generates a pulse of specified duration from the new position-compare event as shown in Figure [15-143](#page-101-1).

**Figure 15-143. eQEP Position-compare Sync Output Pulse Stretcher**

#### **15.4.2.5 eQEP Edge Capture Unit**

The eQEP peripheral includes an integrated edge capture unit to measure the elapsed time between the unit position events as shown in Figure [15-144.](#page-103-0) This feature is typically used for low speed measurement using the following equation:

$$v(k) = \frac{X}{t(k) - t(k-1)} = \frac{X}{\Delta T}$$
(3)

where,

- X Unit position is defined by integer multiple of quadrature edges (see Figure [15-145\)](#page-103-1)
- ΔT Elapsed time between unit position events
- v(k) Velocity at time instant "k"

The eQEP capture timer (QCTMR) runs from prescaled SYSCLKOUT and the prescaler is programmed by the QCAPCTL[CCPS] bits. The capture timer (QCTMR) value is latched into the capture period register (QCPRD) on every unit position event and then the capture timer is reset, a flag is set in QEPSTS[UPEVNT] to indicate that new value is latched into the QCPRD register. Software can check this status flag before reading the period register for low speed measurement and clear the flag by writing 1.

Time measurement (ΔT) between unit position events will be correct if the following conditions are met:

- No more than 65,535 counts have occurred between unit position events.
- No direction change between unit position events.

The capture unit sets the eQEP overflow error flag (QEPSTS[COEF]) in the event of capture timer overflow between unit position events. If a direction change occurs between the unit position events, then an error flag is set in the status register (QEPSTS[CDEF]).

Capture Timer (QCTMR) and Capture period register (QCPRD) can be configured to latch on following events.

- CPU read of QPOSCNT register
- Unit time-out event

If the QEPCTL[QCLM] bit is cleared, then the capture timer and capture period values are latched into the QCTMRLAT and QCPRDLAT registers, respectively, when the CPU reads the position counter (QPOSCNT).

If the QEPCTL[QCLM] bit is set, then the position counter, capture timer, and capture period values are latched into the QPOSLAT, QCTMRLAT and QCPRDLAT registers, respectively, on unit time out.

Figure [15-146](#page-104-0) shows the capture unit operation along with the position counter.

**NOTE:** The QCAPCTL register should not be modified dynamically (such as switching CAPCLK prescaling mode from QCLK/4 to QCLK/8). The capture unit must be disabled before changing the prescaler.

Figure 15-144. eQEP Edge Capture Unit

N - Number of quadrature periods selected using QCAPCTL[UPPS] bits

Velocity Calculation Equations:

$$v(k) = \frac{x(k) - x(k-1)}{T} = \frac{\Delta X}{T} o$$
 (4)

where

v(k): Velocity at time instant k

x(k): Position at time instant k

x(k-1): Position at time instant k - 1

T: Fixed unit time or inverse of velocity calculation rate

ΔX: Incremental position movement in unit time

X: Fixed unit position

ΔT: Incremental time elapsed for unit position movement

t(k): Time instant "k"

t(k-1): Time instant "k - 1"

Unit time (T) and unit period (X) are configured using the QUPRD and QCAPCTL[UPPS] registers. Incremental position output and incremental time output is available in the QPOSLAT and QCPRDLAT registers.

| Parameter | Relevant Register to Configure or Read the Information                  |
|-----------|-------------------------------------------------------------------------|
| T         | Unit Period Register (QUPRD)                                            |
| ΔX        | Incremental Position = QPOSLAT(k) - QPOSLAT(K - 1)                      |
| X         | Fixed unit position defined by sensor resolution and ZCAPCTL[UPPS] bits |
| ΔT        | Capture Period Latch (QCPRDLAT)                                         |

#### **15.4.2.6 eQEP Watchdog**

The eQEP peripheral contains a 16-bit watchdog timer that monitors the quadrature-clock to indicate proper operation of the motion-control system. The eQEP watchdog timer is clocked from SYSCLKOUT/64 and the quadrate clock event (pulse) resets the watchdog timer. If no quadrature-clock event is detected until a period match (QWDPRD = QWDTMR), then the watchdog timer will time out and the watchdog interrupt flag will be set (QFLG[WTO]). The time-out value is programmable through the watchdog period register (QWDPRD).

**Figure 15-147. eQEP Watchdog Timer**

#### 15.4.2.7 Unit Timer Base

The eQEP peripheral includes a 32-bit timer (QUTMR) that is clocked by SYSCLKOUT to generate periodic interrupts for velocity calculations. The unit time out interrupt is set (QFLG[UTO]) when the unit timer (QUTMR) matches the unit period register (QUPRD).

The eQEP peripheral can be configured to latch the position counter, capture timer, and capture period values on a unit time out event so that latched values are used for velocity calculation as described in Section Section 15.4.2.5.

Figure 15-148. eQEP Unit Time Base

#### 15.4.2.8 eQEP Interrupt Structure

Figure 15-149 shows how the interrupt mechanism works in the EQEP module.

Figure 15-149. EQEP Interrupt Generation

Eleven interrupt events (PCE, PHE, QDC, WTO, PCU, PCO, PCR, PCM, SEL, IEL, and UTO) can be generated. The interrupt control register (QEINT) is used to enable/disable individual interrupt event sources. The interrupt flag register (QFLG) indicates if any interrupt event has been latched and contains the global interrupt flag bit (INT). An interrupt pulse is generated only to the interrupt controller if any of the interrupt events is enabled, the flag bit is 1 and the INT flag bit is 0. The interrupt service routine will need to clear the global interrupt flag bit and the serviced event, via the interrupt clear register (QCLR), before any other interrupt pulses are generated. You can force an interrupt event by way of the interrupt force register (QFRC), which is useful for test purposes.

Note that the interrupts coming from the eQEP module are also used as DMA events. The interrupt registers should be used to enable and clear the current DMA event in order for the eQEP module to generate subsequent DMA events.

### *15.4.3 EQEP Registers*

Table [15-121](#page-107-0) lists the memory-mapped registers for the EQEP. All register offset addresses not listed in Table [15-121](#page-107-0) should be considered as reserved locations and the register contents should not be modified.

**Table 15-121. EQEP Registers**

| Offset | Acronym  | Register Name                                 | Section           |
|--------|----------|-----------------------------------------------|-------------------|
| 0h     | QPOSCNT  | eQEP Position Counter Register                | Section 15.4.3.1  |
| 4h     | QPOSINIT | eQEP Position Counter Initialization Register | Section 15.4.3.2  |
| 8h     | QPOSMAX  | eQEP Maximum Position Count Register          | Section 15.4.3.3  |
| Ch     | QPOSCMP  | eQEP Position-Compare Register                | Section 15.4.3.4  |
| 10h    | QPOSILAT | eQEP Index Position Latch Register            | Section 15.4.3.5  |
| 14h    | QPOSSLAT | eQEP Strobe Position Latch Register           | Section 15.4.3.6  |
| 18h    | QPOSLAT  | eQEP Position Counter Latch Register          | Section 15.4.3.7  |
| 1Ch    | QUTMR    | eQEP Unit Timer Register                      | Section 15.4.3.8  |
| 20h    | QUPRD    | eQEP Unit Period Register                     | Section 15.4.3.9  |
| 24h    | QWDTMR   | eQEP Watchdog Timer Register                  | Section 15.4.3.10 |
| 26h    | QWDPRD   | eQEP Watchdog Period Register                 | Section 15.4.3.11 |
| 28h    | QDECCTL  | eQEP Decoder Control Register                 | Section 15.4.3.12 |
| 2Ah    | QEPCTL   | eQEP Control Register                         | Section 15.4.3.13 |
| 2Ch    | QCAPCTL  | eQEP Capture Control Register                 | Section 15.4.3.14 |
| 2Eh    | QPOSCTL  | eQEP Position-Compare Control Register        | Section 15.4.3.15 |
| 30h    | QEINT    | eQEP Interrupt Enable Register                | Section 15.4.3.16 |
| 32h    | QFLG     | eQEP Interrupt Flag Register                  | Section 15.4.3.17 |
| 34h    | QCLR     | eQEP Interrupt Clear Register                 | Section 15.4.3.18 |
| 36h    | QFRC     | eQEP Interrupt Force Register                 | Section 15.4.3.19 |
| 38h    | QEPSTS   | eQEP Status Register                          | Section 15.4.3.20 |
| 3Ah    | QCTMR    | eQEP Capture Timer Register                   | Section 15.4.3.21 |
| 3Ch    | QCPRD    | eQEP Capture Period Register                  | Section 15.4.3.22 |
| 3Eh    | QCTMRLAT | eQEP Capture Timer Latch Register             | Section 15.4.3.23 |
| 40h    | QCPRDLAT | eQEP Capture Period Latch Register            | Section 15.4.3.24 |
| 5Ch    | REVID    | eQEP Revision ID Register                     | Section 15.4.3.25 |

#### **15.4.3.1 QPOSCNT Register (offset = 0h) [reset = 0h]**

QPOSCNT is shown in Figure [15-150](#page-108-1) and described in Table [15-122.](#page-108-2)

### **Figure 15-150. QPOSCNT Register**

| 31 | 30     | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16      | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|--------|----|----|----|----|----|----|----|----|----|----|----|----|----|---------|----|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    |        |    |    |    |    |    |    |    |    |    |    |    |    |    | QPOSCNT |    |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    | R/W-0h |    |    |    |    |    |    |    |    |    |    |    |    |    |         |    |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-122. QPOSCNT Register Field Descriptions**

| Bit  | Field   | Type | Reset | Description                                                                                                                                                                                                                       |
|------|---------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | QPOSCNT | R/W  | 0h    | This 32 bit position counter register counts up/down on every eQEP<br>pulse based on direction input.<br>This counter acts as a position integrator whose count value is<br>proportional to position from a give reference point. |

#### **15.4.3.2 QPOSINIT Register (offset = 4h) [reset = 0h]**

QPOSINIT is shown in Figure [15-151](#page-109-1) and described in Table [15-123](#page-109-2).

### **Figure 15-151. QPOSINIT Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-123. QPOSINIT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                        |
|------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | QPOSINIT | R/W  | 0h    | This register contains the position value that is used to initialize the<br>position counter based on external strobe or index event.<br>The position counter can be initialized through software. |

#### **15.4.3.3 QPOSMAX Register (offset = 8h) [reset = 0h]**

QPOSMAX is shown in Figure [15-152](#page-110-1) and described in Table [15-124.](#page-110-2)

### **Figure 15-152. QPOSMAX Register**

| 31 |        | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15      | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|--------|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|---------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    |        |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | QPOSMAX |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    | R/W-0h |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |         |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 15-124. QPOSMAX Register Field Descriptions**

| Bit  | Field   | Type | Reset | Description                                                |
|------|---------|------|-------|------------------------------------------------------------|
| 31-0 | QPOSMAX | R/W  | 0h    | This register contains the maximum position counter value. |

#### **15.4.3.4 QPOSCMP Register (offset = Ch) [reset = 0h]**

QPOSCMP is shown in Figure [15-153](#page-111-1) and described in Table [15-125](#page-111-2).

### **Figure 15-153. QPOSCMP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-125. QPOSCMP Register Field Descriptions**

| Bit  | Field   | Type | Reset | Description                                                                                                                                                  |
|------|---------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | QPOSCMP | R/W  | 0h    | The position-compare value in this register is compared with the<br>position counter (QPOSCNT) to generate sync output and/or<br>interrupt on compare match. |

#### **15.4.3.5 QPOSILAT Register (offset = 10h) [reset = 0h]**

QPOSILAT is shown in Figure [15-154](#page-112-1) and described in Table [15-126](#page-112-2).

### **Figure 15-154. QPOSILAT Register**

| 31 | 30       | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----------|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    | QPOSILAT |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    | R-0h     |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-126. QPOSILAT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                       |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------|
| 31-0 | QPOSILAT | R    | 0h    | The position-counter value is latched into this register on an index<br>event as defined by the QEPCTL[IEL] bits. |

#### **15.4.3.6 QPOSSLAT Register (offset = 14h) [reset = 0h]**

QPOSSLAT is shown in Figure [15-155](#page-113-1) and described in Table [15-127](#page-113-2).

#### **Figure 15-155. QPOSSLAT Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-127. QPOSSLAT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                     |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------|
| 31-0 | QPOSSLAT | R    | 0h    | The position-counter value is latched into this register on strobe<br>event as defined by the QEPCTL[SEL] bits. |

#### **15.4.3.7 QPOSLAT Register (offset = 18h) [reset = 0h]**

QPOSLAT is shown in Figure [15-156](#page-114-1) and described in Table [15-128](#page-114-2).

### **Figure 15-156. QPOSLAT Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16      | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|---------|----|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | QPOSLAT |    |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R-0h    |    |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-128. QPOSLAT Register Field Descriptions**

| Bit  | Field   | Type | Reset | Description                                                                         |
|------|---------|------|-------|-------------------------------------------------------------------------------------|
| 31-0 | QPOSLAT | R    | 0h    | The position-counter value is latched into this register on unit time<br>out event. |

#### **15.4.3.8 QUTMR Register (offset = 1Ch) [reset = 0h]**

QUTMR is shown in Figure [15-157](#page-115-1) and described in Table [15-129.](#page-115-2)

### **Figure 15-157. QUTMR Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15     | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|--------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | QUTMR  |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-0h |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-129. QUTMR Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                    |
|------|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | QUTMR | R/W  | 0h    | This register acts as time base for unit time event generation.<br>When this timer value matches with unit time period value, unit time<br>event is generated. |

#### **15.4.3.9 QUPRD Register (offset = 20h) [reset = 0h]**

QUPRD is shown in Figure [15-158](#page-116-1) and described in Table [15-130](#page-116-2).

### **Figure 15-158. QUPRD Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15     | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|--------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | QUPRD  |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-0h |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 15-130. QUPRD Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                          |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | QUPRD | R/W  | 0h    | This register contains the period count for unit timer to generate<br>periodic unit time events to latch the eQEP position information at<br>periodic interval and optionally to generate interrupt. |

#### **15.4.3.10 QWDTMR Register (offset = 24h) [reset = 0h]**

QWDTMR is shown in Figure [15-159](#page-117-1) and described in Table [15-131](#page-117-2).

### **Figure 15-159. QWDTMR Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-131. QWDTMR Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                   |
|------|--------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-0 | QWDTMR | R/W  | 0h    | This register acts as time base for watch dog to detect motor stalls.<br>When this timer value matches with watch dog period value, watch<br>dog timeout interrupt is generated.<br>This register is reset upon edge transition in quadrature-clock<br>indicating the motion. |

#### **15.4.3.11 QWDPRD Register (offset = 26h) [reset = 0h]**

QWDPRD is shown in Figure [15-160](#page-118-1) and described in Table [15-132.](#page-118-2)

### **Figure 15-160. QWDPRD Register**

| 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7      | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|---|---|--------|---|---|---|---|---|---|---|
|    |    |    |    |    |    |   |   | QWDPRD |   |   |   |   |   |   |   |
|    |    |    |    |    |    |   |   | R/W-0h |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 15-132. QWDPRD Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                             |
|------|--------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-0 | QWDPRD | R/W  | 0h    | This register contains the time-out count for the eQEP peripheral<br>watch dog timer.<br>When the watchdog timer value matches the watchdog period value,<br>a watchdog timeout interrupt is generated. |

#### **15.4.3.12 QDECCTL Register (offset = 28h) [reset = 0h]**

QDECCTL is shown in Figure [15-161](#page-119-1) and described in Table [15-133](#page-119-2).

#### **Figure 15-161. QDECCTL Register**

| 15     | 14     | 13     | 12     | 11     | 10       | 9      | 8      |
|--------|--------|--------|--------|--------|----------|--------|--------|
|        | QSRC   | SOEN   | SPSEL  | XCR    | SWAP     | IGATE  | QAP    |
|        | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h |
| 7      | 6      | 5      | 4      | 3      | 2        | 1      | 0      |
| QBP    | QIP    | QSP    |        |        | RESERVED |        |        |
| R/W-0h | R/W-0h | R/W-0h |        |        | R-0h     |        |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-133. QDECCTL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                 |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-14 | QSRC     | R/W  | 0h    | Position-counter source selection.<br>0h = Quadrature count mode (QCLK = iCLK, QDIR = iDIR)<br>1h = Direction-count mode (QCLK = xCLK, QDIR = xDIR)<br>2h = UP count mode for frequency measurement (QCLK = xCLK,<br>QDIR = 1)<br>3h = DOWN count mode for frequency measurement (QCLK =<br>xCLK, QDIR = 0) |
| 13    | SOEN     | R/W  | 0h    | Sync output-enable<br>0h = Disable position-compare sync output<br>1h = Enable position-compare sync output                                                                                                                                                                                                 |
| 12    | SPSEL    | R/W  | 0h    | Sync output pin selection<br>0h = Index pin is used for sync output<br>1h = Strobe pin is used for sync output                                                                                                                                                                                              |
| 11    | XCR      | R/W  | 0h    | External clock rate<br>0h = 2x resolution: Count the rising/falling edge<br>1h = 1x resolution: Count the rising edge only                                                                                                                                                                                  |
| 10    | SWAP     | R/W  | 0h    | Swap quadrature clock inputs.<br>This swaps the input to the quadrature decoder, reversing the<br>counting direction.<br>0h = Quadrature-clock inputs are not swapped<br>1h = Quadrature-clock inputs are swapped                                                                                           |
| 9     | IGATE    | R/W  | 0h    | Index pulse gating option<br>0h = Disable gating of Index pulse<br>1h = Gate the index pin with strobe                                                                                                                                                                                                      |
| 8     | QAP      | R/W  | 0h    | QEPA input polarity<br>0h = No effect<br>1h = Negates QEPA input                                                                                                                                                                                                                                            |
| 7     | QBP      | R/W  | 0h    | QEPB input polarity<br>0h = No effect<br>1h = Negates QEPB input                                                                                                                                                                                                                                            |
| 6     | QIP      | R/W  | 0h    | QEPI input polarity<br>0h = No effect<br>1h = Negates QEPI input                                                                                                                                                                                                                                            |
| 5     | QSP      | R/W  | 0h    | QEPS input polarity<br>0h = No effect<br>1h = Negates QEPS input                                                                                                                                                                                                                                            |
| 4-0   | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                             |

#### **15.4.3.13 QEPCTL Register (offset = 2Ah) [reset = 0h]**

QEPCTL is shown in Figure [15-162](#page-120-1) and described in Table [15-134](#page-120-2).

## **Figure 15-162. QEPCTL Register**

| 15     | 14        | 13 | 12     | 11     | 10     | 9      | 8      |
|--------|-----------|----|--------|--------|--------|--------|--------|
|        | FREE_SOFT |    | PCRM   |        | SEI    |        | IEI    |
|        | R/W-0h    |    | R/W-0h |        | R/W-0h |        | R/W-0h |
| 7      | 6         | 5  | 4      | 3      | 2      | 1      | 0      |
| SWI    | SEL       |    | IEL    | PHEN   | QCLM   | UTE    | WDE    |
| R/W-0h | R/W-0h    |    | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-134. QEPCTL Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|-------|-----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-14 | FREE_SOFT | R/W  | 0h    | Emulation Control Bits.<br>In the values 0 through 3 listed below, x is different for the four<br>following behaviors.<br>QPOSCNT behavior, x refers to the Position counter.<br>QWDTMR behavior, x refers to the Watchdog counter.<br>QUTMR behavior, x refers to the Unit timer.<br>QCTMR behavior, x refers to the Capture timer.<br>0h = x stops immediately. For QPOSCNT behavior, the stop is on<br>emulation suspend.<br>1h = x continues to count until the rollover.<br>2h = x is unaffected by emulation suspend. |
| 13-12 | PCRM      | R/W  | 0h    | 3h = x is unaffected by emulation suspend.<br>Position counter reset mode<br>0h = Position counter reset on an index event<br>1h = Position counter reset on the maximum position<br>2h = Position counter reset on the first index event<br>3h = Position counter reset on a unit time event                                                                                                                                                                                                                               |
| 11-10 | SEI       | R/W  | 0h    | Strobe event initialization of position counter<br>0h = Does nothing (action disabled)<br>1h = Does nothing (action disabled)<br>2h = Initializes the position counter on rising edge of the QEPS<br>signal<br>3h = Clockwise Direction: Initializes the position counter on the rising<br>edge of QEPS strobe. Counter Clockwise Direction: Initializes the<br>position counter on the falling edge of QEPS strobe                                                                                                         |
| 9-8   | IEI       | R/W  | 0h    | Index event initialization of position counter<br>0h = Do nothing (action disabled)<br>1h = Do nothing (action disabled)<br>2h = Initializes the position counter on the rising edge of the QEPI<br>signal (QPOSCNT = QPOSINIT)<br>3h = Initializes the position counter on the falling edge of QEPI<br>signal (QPOSCNT = QPOSINIT)                                                                                                                                                                                         |
| 7     | SWI       | R/W  | 0h    | Software initialization of position counter<br>0h = Do nothing (action disabled)<br>1h = Initialize position counter, this bit is cleared automatically                                                                                                                                                                                                                                                                                                                                                                     |
| 6     | SEL       | R/W  | 0h    | Strobe event latch of position counter<br>0h = The position counter is latched on the rising edge of QEPS<br>strobe (QPOSSLAT = POSCCNT). Latching on the falling edge can<br>be done by inverting the strobe input using the QSP bit in the<br>QDECCTL register.<br>1h = Clockwise Direction: Position counter is latched on rising edge<br>of QEPS strobe. Counter Clockwise Direction: Position counter is<br>latched on falling edge of QEPS strobe.                                                                    |

**Table 15-134. QEPCTL Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|-----|-------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 5-4 | IEL   | R/W  | 0h    | Index event latch of position counter (software index marker)<br>0h = Reserved<br>1h = Latches position counter on rising edge of the index signal<br>2h = Latches position counter on falling edge of the index signal<br>3h = Software index marker. Latches the position counter and<br>quadrature direction flag on index event marker. The position<br>counter is latched to the QPOSILAT register and the direction flag is<br>latched in the QEPSTS[QDLF] bit. This mode is useful for software<br>index marking. |
| 3   | PHEN  | R/W  | 0h    | Quadrature position counter enable/software reset<br>0h = Reset the eQEP peripheral internal operating flags/read-only<br>registers. Control/configuration registers are not disturbed by a<br>software reset.<br>1h = eQEP position counter is enabled                                                                                                                                                                                                                                                                  |
| 2   | QCLM  | R/W  | 0h    | eQEP capture latch mode<br>0h = Latch on position counter read by CPU. Capture timer and<br>capture period values are latched into QCTMRLAT and QCPRDLAT<br>registers when CPU reads the QPOSCNT register.<br>1h = Latch on unit time out. Position counter, capture timer and<br>capture period values are latched into QPOSLAT, QCTMRLAT and<br>QCPRDLAT registers on unit time out.                                                                                                                                   |
| 1   | UTE   | R/W  | 0h    | eQEP unit timer enable<br>0h = Disable eQEP unit timer<br>1h = Enable unit timer                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 0   | WDE   | R/W  | 0h    | eQEP watchdog enable<br>0h = Disable the eQEP watchdog timer<br>1h = Enable the eQEP watchdog timer                                                                                                                                                                                                                                                                                                                                                                                                                      |

#### **15.4.3.14 QCAPCTL Register (offset = 2Ch) [reset = 0h]**

QCAPCTL is shown in Figure [15-163](#page-122-1) and described in Table [15-135](#page-122-2).

### **Figure 15-163. QCAPCTL Register**

| 15       | 14 | 13               | 12 | 11 | 10 | 9    | 8 |  |  |
|----------|----|------------------|----|----|----|------|---|--|--|
| CEN      |    | RESERVED         |    |    |    |      |   |  |  |
| R/W-0h   |    | R-0h             |    |    |    |      |   |  |  |
| 7        | 6  | 5                | 4  | 3  | 2  | 1    | 0 |  |  |
| RESERVED |    | CCPS             |    |    |    | UPPS |   |  |  |
| R-0h     |    | R/W-0h<br>R/W-0h |    |    |    |      |   |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-135. QCAPCTL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15   | CEN      | R/W  | 0h    | Enable eQEP capture<br>0h = eQEP capture unit is disabled<br>1h = eQEP capture unit is enabled                                                                                                                                                                                                                                                                                                                   |
| 14-7 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 6-4  | CCPS     | R/W  | 0h    | eQEP capture timer clock prescaler<br>0h = CAPCLK = SYSCLKOUT/1<br>1h = CAPCLK = SYSCLKOUT/2<br>2h = CAPCLK = SYSCLKOUT/4<br>3h = CAPCLK = SYSCLKOUT/8<br>4h = CAPCLK = SYSCLKOUT/16<br>5h = CAPCLK = SYSCLKOUT/32<br>6h = CAPCLK = SYSCLKOUT/64<br>7h = CAPCLK = SYSCLKOUT/128                                                                                                                                  |
| 3-0  | UPPS     | R/W  | 0h    | Unit position event prescaler<br>0h = UPEVNT = QCLK/1<br>1h = UPEVNT = QCLK/2<br>2h = UPEVNT = QCLK/4<br>3h = UPEVNT = QCLK/8<br>4h = UPEVNT = QCLK/16<br>5h = UPEVNT = QCLK/32<br>6h = UPEVNT = QCLK/64<br>7h = UPEVNT = QCLK/128<br>8h = UPEVNT = QCLK/256<br>9h = UPEVNT = QCLK/512<br>Ah = UPEVNT = QCLK/1024<br>Bh = UPEVNT = QCLK/2048<br>Ch = Reserved<br>Dh = Reserved<br>Eh = Reserved<br>Fh = Reserved |

#### **15.4.3.15 QPOSCTL Register (offset = 2Eh) [reset = 0h]**

QPOSCTL is shown in Figure [15-164](#page-123-1) and described in Table [15-136](#page-123-2).

### **Figure 15-164. QPOSCTL Register**

| 15     | 14     | 13     | 12     | 11     | 10 | 9 | 8 |  |  |  |
|--------|--------|--------|--------|--------|----|---|---|--|--|--|
| PCSHDW | PCLOAD | PCPOL  | PCE    | PCSPW  |    |   |   |  |  |  |
| R/W-0h | R/W-0h | R/W-0h | R/W-0h | R/W-0h |    |   |   |  |  |  |
| 7      | 6      | 5      | 4      | 3      | 2  | 1 | 0 |  |  |  |
|        |        |        |        | PCSPW  |    |   |   |  |  |  |
|        | R/W-0h |        |        |        |    |   |   |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-136. QPOSCTL Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                            |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15   | PCSHDW | R/W  | 0h    | Position-compare shadow enable<br>0h = Shadow disabled, load Immediate<br>1h = Shadow enabled                                                                                                                                                          |
| 14   | PCLOAD | R/W  | 0h    | Position-compare shadow load mode<br>0h = Load on QPOSCNT = 0<br>1h = Load when QPOSCNT = QPOSCMP                                                                                                                                                      |
| 13   | PCPOL  | R/W  | 0h    | Polarity of sync output<br>0h = Active HIGH pulse output<br>1h = Active LOW pulse output                                                                                                                                                               |
| 12   | PCE    | R/W  | 0h    | Position-compare enable/disable<br>0h = Disable position compare unit<br>1h = Enable position compare unit                                                                                                                                             |
| 11-0 | PCSPW  | R/W  | 0h    | Select-position-compare sync output pulse width<br>0h = 1 x 4 x SYSCLKOUT cycles<br>1h = 2 x 4 x SYSCLKOUT cycles<br>2h = 3 x 4 x SYSCLKOUT cycles to 4096 x 4 x SYSCLKOUT cycles<br>FFFh = 3 x 4 x SYSCLKOUT cycles to 4096 x 4 x SYSCLKOUT<br>cycles |

#### **15.4.3.16 QEINT Register (offset = 30h) [reset = 0h]**

QEINT is shown in Figure [15-165](#page-124-1) and described in Table [15-137.](#page-124-2)

## **Figure 15-165. QEINT Register**

| 15     | 14     | 13       | 12     | 11     | 10     | 9      | 8        |
|--------|--------|----------|--------|--------|--------|--------|----------|
|        |        | RESERVED |        | UTO    | IEL    | SEL    | PCM      |
|        |        | R-0h     |        | R/W-0h | R/W-0h | R/W-0h | R/W-0h   |
| 7      | 6      | 5        | 4      | 3      | 2      | 1      | 0        |
| PCR    | PCO    | PCU      | WTO    | QDC    | PHE    | PCE    | RESERVED |
| R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R-0h     |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-137. QEINT Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                             |
|-------|----------|------|-------|---------------------------------------------------------------------------------------------------------|
| 15-12 | RESERVED | R    | 0h    |                                                                                                         |
| 11    | UTO      | R/W  | 0h    | Unit time out interrupt enable<br>0h = Interrupt is disabled<br>1h = Interrupt is enabled               |
| 10    | IEL      | R/W  | 0h    | Index event latch interrupt enable<br>0h = Interrupt is disabled<br>1h = Interrupt is enabled           |
| 9     | SEL      | R/W  | 0h    | Strobe event latch interrupt enable<br>0h = Interrupt is disabled<br>1h = Interrupt is enabled          |
| 8     | PCM      | R/W  | 0h    | Position-compare match interrupt enable<br>0h = Interrupt is disabled<br>1h = Interrupt is enabled      |
| 7     | PCR      | R/W  | 0h    | Position-compare ready interrupt enable<br>0h = Interrupt is disabled<br>1h = Interrupt is enabled      |
| 6     | PCO      | R/W  | 0h    | Position counter overflow interrupt enable<br>0h = Interrupt is disabled<br>1h = Interrupt is enabled   |
| 5     | PCU      | R/W  | 0h    | Position counter underflow interrupt enable<br>0h = Interrupt is disabled<br>1h = Interrupt is enabled  |
| 4     | WTO      | R/W  | 0h    | Watchdog time out interrupt enable<br>0h = Interrupt is disabled<br>1h = Interrupt is enabled           |
| 3     | QDC      | R/W  | 0h    | Quadrature direction change interrupt enable<br>0h = Interrupt is disabled<br>1h = Interrupt is enabled |
| 2     | PHE      | R/W  | 0h    | Quadrature phase error interrupt enable<br>0h = Interrupt is disabled<br>1h = Interrupt is enabled      |
| 1     | PCE      | R/W  | 0h    | Position counter error interrupt enable<br>0h = Interrupt is disabled<br>1h = Interrupt is enabled      |
| 0     | RESERVED | R    | 0h    |                                                                                                         |

#### **15.4.3.17 QFLG Register (offset = 32h) [reset = 0h]**

QFLG is shown in Figure [15-166](#page-125-1) and described in Table [15-138](#page-125-2).

#### **Figure 15-166. QFLG Register**

| 15   | 14   | 13       | 12   | 11   | 10   | 9    | 8    |
|------|------|----------|------|------|------|------|------|
|      |      | RESERVED |      | UTO  | IEL  | SEL  | PCM  |
|      | R-0h |          |      |      | R-0h | R-0h | R-0h |
| 7    | 6    | 5        | 4    | 3    | 2    | 1    | 0    |
| PCR  | PCO  | PCU      | WTO  | QDC  | PHE  | PCE  | INT  |
| R-0h | R-0h | R-0h     | R-0h | R-0h | R-0h | R-0h | R-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-138. QFLG Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                           |
|-------|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-12 | RESERVED | R    | 0h    |                                                                                                                                                                                       |
| 11    | UTO      | R    | 0h    | Unit time out interrupt flag<br>0h = No interrupt generated<br>1h = Set by eQEP unit timer period match                                                                               |
| 10    | IEL      | R    | 0h    | Index event latch interrupt flag<br>0h = No interrupt generated<br>1h = This bit is set after latching the QPOSCNT to QPOSILAT                                                        |
| 9     | SEL      | R    | 0h    | Strobe event latch interrupt flag<br>0h = No interrupt generated<br>1h = This bit is set after latching the QPOSCNT to QPOSSLAT                                                       |
| 8     | PCM      | R    | 0h    | eQEP compare match event interrupt flag<br>0h = No interrupt generated<br>1h = This bit is set on position-compare match                                                              |
| 7     | PCR      | R    | 0h    | Position-compare ready interrupt flag<br>0h = No interrupt generated<br>1h = This bit is set after transferring the shadow register value to the<br>active position compare register. |
| 6     | PCO      | R    | 0h    | Position counter overflow interrupt flag<br>0h = No interrupt generated<br>1h = This bit is set on position counter overflow.                                                         |
| 5     | PCU      | R    | 0h    | Position counter underflow interrupt flag<br>0h = No interrupt generated<br>1h = This bit is set on position counter underflow.                                                       |
| 4     | WTO      | R    | 0h    | Watchdog timeout interrupt flag<br>0h = No interrupt generated<br>1h = Set by watch dog timeout                                                                                       |
| 3     | QDC      | R    | 0h    | Quadrature direction change interrupt flag<br>0h = No interrupt generated<br>1h = This bit is set during change of direction                                                          |
| 2     | PHE      | R    | 0h    | Quadrature phase error interrupt flag<br>0h = No interrupt generated<br>1h = Set on simultaneous transition of QEPA and QEPB                                                          |
| 1     | PCE      | R    | 0h    | Position counter error interrupt flag<br>0h = No interrupt generated<br>1h = Position counter error                                                                                   |
| 0     | INT      | R    | 0h    | Global interrupt status flag<br>0h = No interrupt generated<br>1h = Interrupt was generated                                                                                           |

#### **15.4.3.18 QCLR Register (offset = 34h) [reset = 0h]**

QCLR is shown in Figure [15-167](#page-126-1) and described in Table [15-139](#page-126-2).

## **Figure 15-167. QCLR Register**

| 15     | 14     | 13       | 12     | 11     | 10     | 9      | 8      |
|--------|--------|----------|--------|--------|--------|--------|--------|
|        |        | RESERVED |        | UTO    | IEL    | SEL    | PCM    |
|        |        | R-0h     |        | R/W-0h | R/W-0h | R/W-0h | R/W-0h |
| 7      | 6      | 5        | 4      | 3      | 2      | 1      | 0      |
| PCR    | PCO    | PCU      | WTO    | QDC    | PHE    | PCE    | INT    |
| R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-139. QCLR Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                      |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-12 | RESERVED | R    | 0h    |                                                                                                                                                                  |
| 11    | UTO      | R/W  | 0h    | Clear unit time out interrupt flag<br>0h = No effect<br>1h = Clears the interrupt flag                                                                           |
| 10    | IEL      | R/W  | 0h    | Clear index event latch interrupt flag<br>0h = No effect<br>1h = Clears the interrupt flag                                                                       |
| 9     | SEL      | R/W  | 0h    | Clear strobe event latch interrupt flag<br>0h = No effect<br>1h = Clears the interrupt flag                                                                      |
| 8     | PCM      | R/W  | 0h    | Clear eQEP compare match event interrupt flag<br>0h = No effect<br>1h = Clears the interrupt flag                                                                |
| 7     | PCR      | R/W  | 0h    | Clear position-compare ready interrupt flag<br>0h = No effect<br>1h = Clears the interrupt flag                                                                  |
| 6     | PCO      | R/W  | 0h    | Clear position counter overflow interrupt flag<br>0h = No effect<br>1h = Clears the interrupt flag                                                               |
| 5     | PCU      | R/W  | 0h    | Clear position counter underflow interrupt flag<br>0h = No effect<br>1h = Clears the interrupt flag                                                              |
| 4     | WTO      | R/W  | 0h    | Clear watchdog timeout interrupt flag<br>0h = No effect<br>1h = Clears the interrupt flag                                                                        |
| 3     | QDC      | R/W  | 0h    | Clear quadrature direction change interrupt flag<br>0h = No effect<br>1h = Clears the interrupt flag                                                             |
| 2     | PHE      | R/W  | 0h    | Clear quadrature phase error interrupt flag<br>0h = No effect<br>1h = Clears the interrupt flag                                                                  |
| 1     | PCE      | R/W  | 0h    | Clear position counter error interrupt flag<br>0h = No effect<br>1h = Clears the interrupt flag                                                                  |
| 0     | INT      | R/W  | 0h    | Global interrupt clear flag<br>0h = No effect<br>1h = Clears the interrupt flag and enables further interrupts to be<br>generated if an event flags is set to 1. |

#### **15.4.3.19 QFRC Register (offset = 36h) [reset = 0h]**

QFRC is shown in Figure [15-168](#page-127-1) and described in Table [15-140.](#page-127-2)

#### **Figure 15-168. QFRC Register**

| 15     | 14     | 13       | 12     | 11     | 10     | 9      | 8        |
|--------|--------|----------|--------|--------|--------|--------|----------|
|        |        | RESERVED |        | UTO    | IEL    | SEL    | PCM      |
|        |        | R-0h     |        | R/W-0h | R/W-0h | R/W-0h | R/W-0h   |
| 7      | 6      | 5        | 4      | 3      | 2      | 1      | 0        |
| PCR    | PCO    | PCU      | WTO    | QDC    | PHE    | PCE    | RESERVED |
| R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R-0h     |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-140. QFRC Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                               |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------|
| 15-12 | RESERVED | R    | 0h    |                                                                                           |
| 11    | UTO      | R/W  | 0h    | Force unit time out interrupt<br>0h = No effect<br>1h = Force the interrupt               |
| 10    | IEL      | R/W  | 0h    | Force index event latch interrupt<br>0h = No effect<br>1h = Force the interrupt           |
| 9     | SEL      | R/W  | 0h    | Force strobe event latch interrupt<br>0h = No effect<br>1h = Force the interrupt          |
| 8     | PCM      | R/W  | 0h    | Force position-compare match interrupt<br>0h = No effect<br>1h = Force the interrupt      |
| 7     | PCR      | R/W  | 0h    | Force position-compare ready interrupt<br>0h = No effect<br>1h = Force the interrupt      |
| 6     | PCO      | R/W  | 0h    | Force position counter overflow interrupt<br>0h = No effect<br>1h = Force the interrupt   |
| 5     | PCU      | R/W  | 0h    | Force position counter underflow interrupt<br>0h = No effect<br>1h = Force the interrupt  |
| 4     | WTO      | R/W  | 0h    | Force watchdog time out interrupt<br>0h = No effect<br>1h = Force the interrupt           |
| 3     | QDC      | R/W  | 0h    | Force quadrature direction change interrupt<br>0h = No effect<br>1h = Force the interrupt |
| 2     | PHE      | R/W  | 0h    | Force quadrature phase error interrupt<br>0h = No effect<br>1h = Force the interrupt      |
| 1     | PCE      | R/W  | 0h    | Force position counter error interrupt<br>0h = No effect<br>1h = Force the interrupt      |
| 0     | RESERVED | R    | 0h    |                                                                                           |

#### **15.4.3.20 QEPSTS Register (offset = 38h) [reset = 0h]**

QEPSTS is shown in Figure [15-169](#page-128-1) and described in Table [15-141.](#page-128-2)

## **Figure 15-169. QEPSTS Register**

| 15     | 14       | 13   | 12   | 11     | 10     | 9      | 8    |  |  |  |
|--------|----------|------|------|--------|--------|--------|------|--|--|--|
|        | RESERVED |      |      |        |        |        |      |  |  |  |
|        | R-0h     |      |      |        |        |        |      |  |  |  |
| 7      | 6        | 5    | 4    | 3      | 2      | 1      | 0    |  |  |  |
| UPEVNT | FDF      | QDF  | QDLF | COEF   | CDEF   | FIMF   | PCEF |  |  |  |
| R-0h   | R-0h     | R-0h | R-0h | R/W-0h | R/W-0h | R/W-0h | R-0h |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-141. QEPSTS Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                   |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-8 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                               |
| 7    | UPEVNT   | R    | 0h    | Unit position event flag<br>0h = No unit position event detected<br>1h = Unit position event detected. Write 1 to clear.                                                                                                                                                      |
| 6    | FDF      | R    | 0h    | Direction on the first index marker.<br>Status of the direction is latched on the first index event marker.<br>0h = Counter-clockwise rotation (or reverse movement) on the first<br>index event<br>1h = Clockwise rotation (or forward movement) on the first index<br>event |
| 5    | QDF      | R    | 0h    | Quadrature direction flag<br>0h = Counter-clockwise rotation (or reverse movement)<br>1h = Clockwise rotation (or forward movement)                                                                                                                                           |
| 4    | QDLF     | R    | 0h    | eQEP direction latch flag.<br>Status of direction is latched on every index event marker.<br>0h = Counter-clockwise rotation (or reverse movement) on index<br>event marker<br>1h = Clockwise rotation (or forward movement) on index event<br>marker                         |
| 3    | COEF     | R/W  | 0h    | Capture overflow error flag<br>0h = Sticky bit, cleared by writing 1<br>1h = Overflow occurred in eQEP Capture timer (QEPCTMR)                                                                                                                                                |
| 2    | CDEF     | R/W  | 0h    | Capture direction error flag<br>0h = Sticky bit, cleared by writing 1<br>1h = Direction change occurred between the capture position event.                                                                                                                                   |
| 1    | FIMF     | R/W  | 0h    | First index marker flag<br>0h = Sticky bit, cleared by writing 1<br>1h = Set by first occurrence of index pulse                                                                                                                                                               |
| 0    | PCEF     | R    | 0h    | Position counter error flag.<br>This bit is not sticky and it is updated for every index event.<br>0h = No error occurred during the last index transition.<br>1h = Position counter error                                                                                    |

#### **15.4.3.21 QCTMR Register (offset = 3Ah) [reset = 0h]**

QCTMR is shown in Figure [15-170](#page-129-1) and described in Table [15-142.](#page-129-2)

### **Figure 15-170. QCTMR Register**

| 15     | 14    | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|--------|-------|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|        | QCTMR |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
| R/W-0h |       |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-142. QCTMR Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                             |
|------|-------|------|-------|---------------------------------------------------------|
| 15-0 | QCTMR | R/W  | 0h    | This register provides time base for edge capture unit. |

#### **15.4.3.22 QCPRD Register (offset = 3Ch) [reset = 0h]**

QCPRD is shown in Figure [15-171](#page-130-1) and described in Table [15-143](#page-130-2).

### **Figure 15-171. QCPRD Register**

|        | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|--------|----|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
| QCPRD  |    |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
| R/W-0h |    |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-143. QCPRD Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                    |
|------|-------|------|-------|------------------------------------------------------------------------------------------------|
| 15-0 | QCPRD | R/W  | 0h    | This register holds the period count value between the last<br>successive eQEP position events |

#### **15.4.3.23 QCTMRLAT Register (offset = 3Eh) [reset = 0h]**

QCTMRLAT is shown in Figure [15-172](#page-131-1) and described in Table [15-144](#page-131-2).

#### **Figure 15-172. QCTMRLAT Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 15-144. QCTMRLAT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                         |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-0 | QCTMRLAT | R    | 0h    | The eQEP capture timer value can be latched into this register on<br>two events, that is, unit timeout event, reading the eQEP position<br>counter. |

#### **15.4.3.24 QCPRDLAT Register (offset = 40h) [reset = 0h]**

QCPRDLAT is shown in Figure [15-173](#page-132-1) and described in Table [15-145.](#page-132-2)

### **Figure 15-173. QCPRDLAT Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-145. QCPRDLAT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                      |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-0 | QCPRDLAT | R/W  | 0h    | eQEP capture period value can be latched into this register on two<br>events, that is, unit timeout event, reading the eQEP position<br>counter. |

#### **15.4.3.25 REVID Register (offset = 5Ch) [reset = 44D31103h]**

REVID is shown in Figure [15-174](#page-133-1) and described in Table [15-146.](#page-133-2)

### **Figure 15-174. REVID Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 15-146. REVID Register Field Descriptions**

| Bit  | Field | Type | Reset     | Description      |
|------|-------|------|-----------|------------------|
| 31-0 | REV   | R    | 44D31103h | eQEP revision ID |
