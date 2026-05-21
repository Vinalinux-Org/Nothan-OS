
# Control Module

This chapter describes the control module of the device.

---

## 9.1 Introduction

The control module includes status and control logic not addressed within the peripherals or the rest of the device infrastructure. This module provides interface to control the following areas of the device:

- Functional I/O multiplexing
- Emulation controls
- Device control and status
- DDR PHY control and IO control registers
- EDMA event multiplexing control registers

**Note:** For writing to the control module registers, the MPU will need to be in privileged mode of operation and writes will not work from user mode.

---

## 9.2 Functional Description

### 9.2.1 Control Module Initialization

The control module responds only to the internal POR and device type. At power on, reset values for the registers define the safe state for the device. In the initialization mode, only modules to be used at boot time are associated with the pads. Other module inputs are internally tied and output pads are turned off. After POR, software sets the pad functional multiplexing and configuration registers to the desired values according to the requested device configuration.

General-purpose (GP) devices include features that are inaccessible or unavailable. These inaccessible registers define the default or fixed device configuration or behavior.

The CONTROL_STATUS[7:0] SYS_BOOT bit field reflects the state of the sys_boot pins captured at POR in the PRCM module.

### 9.2.2 Pad Control Registers

The Pad Control Registers are 32-bit registers to control the signal muxing and other aspects of each I/O pad. After POR, software must set the pad functional multiplexing and configuration registers to the desired values according to the requested device configuration. The configuration is controlled by pads or by a group of pads. Each configurable pin has its own configuration register for pullup/down control and for the assignment to a given module.

The following table shows the generic Pad Control Register Description.

**Table 9-1. Pad Control Register Field Descriptions**

| Bit  | Field       | Value | Description                                                                             |  |
|------|-------------|-------|-----------------------------------------------------------------------------------------|--|
| 31-7 | Reserved    |       | Reserved. Read returns 0.                                                               |  |
| 6    | SLEWCTRL    |       | Select between faster or slower slew rate.                                              |  |
|      |             | 0     | Fast                                                                                    |  |
|      |             | 1     | Slow(1)                                                                                 |  |
| 5    | RXACTIVE    |       | Input enable value for the Pad. Set to 0 for output only. Set to 1 for input or output. |  |
|      |             | 0     | Receiver disabled                                                                       |  |
|      |             | 1     | Receiver enabled                                                                        |  |
| 4    | PULLTYPESEL |       | Pad pullup/pulldown type selection                                                      |  |
|      |             | 0     | Pulldown selected                                                                       |  |
|      |             | 1     | Pullup selected                                                                         |  |
| 3    | PULLUDEN    |       | Pad Pullup/pulldown enable                                                              |  |
|      |             | 0     | Pullup/pulldown enabled.                                                                |  |
|      |             | 1     | Pullup/pulldown disabled.                                                               |  |
| 2-0  | MUXMODE     |       | Pad functional signal mux select                                                        |  |

<sup>(1)</sup> Some peripherals do not support slow slew rate. To determine which interfaces support each slew rate, see *AM335x Sitara Processors* (literature number [SPRS717\)](http://www.ti.com/lit/pdf/SPRS717).

#### 9.2.2.1 Mode Selection

The MUXMODE field in the pad control registers defines the multiplexing mode applied to the pad. Modes are referred to by their decimal (from 0 to 7) or binary (from 0b000 to 0b111) representation. For most pads, the reset value for the MUXMODE field in the registers is 0b111. The exceptions are pads to be used at boot time to transfer data from selected peripherals to the external flash memory.

**Table 9-2. Mode Selection**

| MUXMODE | Selected Mode         |
|---------|-----------------------|
| 000b    | Primary Mode = Mode 0 |
| 001b    | Mode 1                |
| 010b    | Mode 2                |
| 011b    | Mode 3                |
| 100b    | Mode 4                |
| 101b    | Mode 5                |
| 110b    | Mode 6                |
| 111b    | Mode 7                |

Mode 0 is the primary mode. When mode 0 is set, the function mapped to the pin corresponds to the name of the pin. Mode 1 to mode 7 are possible modes for alternate functions. On each pin, some modes are used effectively for alternate functions, while other modes are unused and correspond to no functional configuration.

**CAUTION

The multiplexer controlling the signal mode selection is not a glitch-free structure. Thus, it is possible to see the signal glitch for a few nanoseconds during the MUXMODE change. The user must ensure a glitch does not cause contention or negatively impact an external device connected to the pad.**

#### 9.2.2.2 Pull Selection

There is no automatic gating control to ensure that internal weak pull- down/pull up resistors on a pad are disconnected whenever the pad is configured as output. If a pad is always configured in output mode, it is recommended for user software to disable any internal pull resistor tied to it, to avoid unnecessary consumption. The following table summarizes the various possible combinations of PULLTYPESEL and PULLUDEN fields of PAD control register.

**Table 9-3. Pull Selection**

| PULL TYPE   | Pin Behavior |                                     |
|-------------|--------------|-------------------------------------|
| PULLTYPESEL | PULLUDENABLE |                                     |
| 0b          | 0b           | Pulldown selected and activated     |
| 0b          | 1b           | Pulldown selected but not activated |
| 1b          | 0b           | Pullup selected and activated       |
| 1b          | 1b           | Pullup selected but not activated   |

#### 9.2.2.3 RX Active

The RXACTIVE bit is used to enable and disable the input buffer. This control can be used to help with power leakage or device isolation through the I/O. The characteristic of the signal is ultimately dictated by the mux mode the pad is put into.

### 9.2.3 EDMA Event Multiplexing

The device has more DMA events than can be accommodated by the TPCC's maximum number of events, which is 64. To overcome the device has one crossbar at the top level. This module will multiplex the extra events with all of the direct mapped events. Mux control registers are defined in the Control Module to select the event to be routed to the TPCC. Direct mapped event is the default (mux selection set to '0').

For every EDMA event there is a cross bar implemented in the design as shown in the figure.The direct mapped event/interrupt will be always connected to Mux input[0], The additional events will be connected to Mux input[1], Mux input[2].etc as defined in EDMA event table. The Mux selection value is programmed into the corresponding TPCC_EVT_MUX_n register. The EVT_MUX value can take a value from 1 to 32. Other values are reserved. By default the MUX_selection value is written to 0, which means the direct mapped event is connected to the Event output.

When the additional event is selected through the Cross bar programming the direct mapped event cannot be used.

For example, when TINT0 (Timer Interrupt 0) event, which is not directly mapped to the DMA event source needs to be connected to EDMA channel no 24 (which is directly mapped to SDTXEVT0 event). The user has to program the EVT_MUX_24 field in TPCC_EVT_MUX_24_27 register to 22 (value corresponding to TINT0 interrupt in crossbar mapping). When this is set, TINT0 interrupt event will trigger the channel 24.

Please note that once this is set. The SDTXEVT0 can no longer be handled by EDMA. The user has to allocate the correct DMA event number for crossbar mapped events so that there is no compromise on the channel allocation for the used event numbers.

### 9.2.4 Device Control and Status

#### 9.2.4.1 Control and Boot Status

The device configuration is set during power on or hardware reset (PORz sequence) by the configuration input pins (SYSBOOT[15:0]).The CONTROL_STATUS register reflects the system boot and the device type configuration values as sampled when the power-on reset (PORz) signal is asserted. The Configuration input pins are sampled continuously during the PORz active period and the final sampled value prior to the last rising edge is latched in the register. The CONTROL_STATUS register gives the status of the device boot process.

#### 9.2.4.2 Interprocessor Communication

The control module has the IPC_MSG_REG (7:0) registers which is for sharing messages between Cortex M3 and the Cortex A8 MPU. The M3 TX end of event (M3_TXEV_EOI) register provides the mechanism to clear/enable the TX Event from Cortex M3 to Cortex A8 MPU Subsystem. See the M3_TXEV_EOI register description for further detail.

See Section 8.1.4.6, *Functional Sequencing for Power Management with Cortex M3*, for specific information on how the IPC_MSG_REG registers are used to communicate with the Cortex-M3 firmware.

#### 9.2.4.3 Initiator Priority Control

The control module provides the registers to control the bus interconnect priority and the EMIF priority.

##### 9.2.4.3.1 Initiator Priority Control for Interconnect

The INIT_PRIORITY_n register controls the infrastructure priority at the bus interconnects. This can be used for dynamic priority escalation. There are bit fields that control the interconnect priority for each bus initiator. By default all the initiators are given equal priority and the allocation is done on a round robin basis.

The priority can take a value from 0 to 3. The following table gives the valid set of priority values.

| Interconnect Priority Value | Remarks         |
|-----------------------------|-----------------|
| 00                          | Low priority    |
| 01                          | Medium priority |
| 10                          | Reserved        |
| 11                          | High priority   |

**Table 9-4. Interconnect Priority Values**

##### 9.2.4.3.2 Initiator Priority at EMIF

The MREQPRIO register provides an interface to change the access priorities for the various masters accessing the EMIF(DDR). Software can make use of this register to set the requestor priorities for required EMIF arbitration. The EMIF priority can take a value from 000b to 111b where 000b will be the highest priority and 111b will be lowest priority.

#### 9.2.4.4 Peripheral Control and Status

##### 9.2.4.4.1 USB Control and Status

The USB_CTRLn and USB_STSn registers reflect the Control and Status of the USB instances. The USB IO lines can be used as UART TX and RX lines the USB Control register bit field GPIOMODE has settings that configures the USB lines as GPIO lines. The other USB PHY control settings for controlling the OTG settings and PHY are part of the USB_CTRLn register.

The USB_STSn register gives the status of the USB PHY module. See the USB_STSn register description for further details.

See Section 16.1.4, *USB GPIO Details*, for more information.

##### 9.2.4.4.2 USB Charger Detect

Each USB PHY contains circuitry which can automatically detect the presence of a charger attached to the USB port. The charger detection circuitry is compliant to the Battery Charging Specification Revision 1.1 from the USB Implementers Forum, which can be found at [www.usb.org](http://www.usb.org). See this document for more details on USB charger implementation.

##### 9.2.4.4.2.1 Features

The charger detection circuitry of each PHY has the following features:

- Contains a state machine which can automatically detect the presence of a Charging Downstream Port or a Dedicated Charging Port (see the Battery Charging Specification for the definition of these terms)
- Outputs a charger enable signal (3.3 V level active high CMOS driver) when a charger is present.
- Allows you to enable/disable the circuitry to save power
- The detection circuitry requires only a 3.3-V supply to be present to operate.
- The charger detection also has a manual mode which allows the user to implement the battery charging specification in software.

##### 9.2.4.4.2.2 Operation

The control module gives the following interface to control the automatic charger detection circuitry:

- USB_CTRLx.CDET_EXTCTL: Turns the automatic detection on/off. Keep this bit 0 to keep the automatic detection on. Changing this to 1 enables the manual mode.
- USB_CTRLx.CHGDET_RSTRT: Restarts the charger detection state machine. To initiate the charger detection, change this bit from 1 to 0. If this bit is 1, the charger enable output (CE) is disabled.
- USB_CTRLx.CHGDET_DIS: Enables/disables the charger detection circuitry. Keep this bit 0 to keep this charger detection enabled. Setting this bit to 1 will power down the charger detection circuitry.
- USB_CTRLx.CM_PWRDN: Powers up/down the PHY which contains the charger detection circuitry. Clear this bit to 0 to enable power to the PHY.

To start the charger detection during normal operation, ensure that the PHY and charger are enabled and the automatic detection is turned on. Then, initiate a charger detection cycle by transitioning CHGDET_RSTRT from 1 to 0. If a Charging Downstream Port or a Dedicated Charging Port is detected, the charger enable signal (USBx_CE) will be driven high and remain high until the charger is disabled by either CHGDET_DIS = 1 or CHGDET_RSTRT=1. If the port remains unconnected after intiating the charger detect cycle, it will continue the detection until a charger is detected or an error condition occurs. Note that USBx_CE is not an open drain output.

To disable the charger after successful detection, you must disable the charger detect circuitry with CHGDET_DIS or CHGDET_RSTRT, even if the charger is physically disconnected.

**Figure 9-1. USB Charger Detection**

Charger detection can be automatically started with no power to the rest of AM335x. If VDDA3P3V_USBx is present, via an LDO powered by VBUS connected to a host, the charger detection state machine will automatically start and perform detection. If a charger is detected, USBx_CE will be driven high, otherwise it will be driven low.

The charger detection circuitry performs the following steps of the Battery Charging specification v1.1:

- 1. VBUS Detect
- 2. Data Contact Detect
- 3. Primary Detection

Secondary Detection (to distinguish between a Charging Downstream Port and a Dedicated Charging Port) is a newly added feature of the v1.2 spec and is not implemented in the charger detection state machine.

**NOTE:** The USBx_CE output will only operate when the corresponding USBx_ID pin is grounded (indicating USB host mode). The USBx_CE output does not operate in peripheral mode (when USBx_ID is floating).

##### 9.2.4.4.3 Ethernet MII Mode Selection

The control module provides a mechanism to select the Mode of operation of Ethernet MII interface. The GMII_SEL register has register bit fields to select the MII/RMII/RGMII modes, clock sources, and delay mode.

##### 9.2.4.4.4 Ethernet Module Reset Isolation Control

This feature allows the device to undergo a warm reset without disrupting the switch or traffic being routed through the switch during the reset condition. The CPSW Reset Isolation register (RESET_ISO) has an ISO_CONTROL field which controls the reset isolation feature.

If the reset isolation is enabled, any warm reset source will be blocked to the EMAC switch. If the EMAC reset isolation is NOT active (default state), then the warm reset sources are allowed to propagate as normal including to the EMAC Switch module (both reset inputs to the IP). All cold or POR resets will always propagate to the EMAC switch module as normal.

When RESET_ISO is enabled, the following registers will not be disturbed by a warm reset:

- GMII_SEL
- CONF_GPMC_A[11:0]
- CONF_GPMC_WAIT0
- CONF_GPMC_WPN
- CONF_GPMC_BEN1
- CONF_MII1_COL
- CONF_MII1_CRS
- CONF_MII1_RX_ER
- CONF_MII1_TX_EN
- CONF_MII1_RX_DV
- CONF_MII1_TXD[3:0]
- CONF_MII1_TX_CLK
- CONF_MII1_RX_CLK
- CONF_MII1_RXD[3:0]
- CONF_RMII1_REF_CLK
- CONF_MDIO
- CONF_MDC

##### 9.2.4.4.5 Timer/eCAP Event Capture Control

The Timer 5, 6, 7 events and the eCAP0, 1, 2 events can be selected using the TIMER_EVT_CAPTURE and ECAP_EVT_CAPTURE registers. The following table lists the available sources for those events.

**Table 9-5. Available Sources for Timer[5–7] and eCAP[0–2] Events**

| Event No. | Source module                                  | Interrupt Name/Pin |
|-----------|------------------------------------------------|--------------------|
|           | For Timer 5 MUX input from IO signal<br>TIMER5 | TIMER5 IO pin      |
|           | For Timer 6 MUX input from IO signal<br>TIMER6 | TIMER6 IO pin      |
| 0         | For Timer 7 MUX input from IO signal<br>TIMER7 | TIMER7 IO pin      |
|           | For eCAP 0 MUX input from IO signal<br>eCAP0   | eCAP0 IO pin       |
|           | For eCAP 1 MUX input from IO signal<br>eCAP1   | eCAP1 IO pin       |
|           | For eCAP 2 MUX input from IO signal<br>eCAP2   | eCAP2 IO pin       |
| 1         | UART0                                          | UART0INT           |
| 2         | UART1                                          | UART1INT           |
| 3         | UART2                                          | UART2INT           |
| 4         | UART3                                          | UART3INT           |
| 5         | UART4                                          | UART4INT           |
| 6         | UART5                                          | UART5INT           |
| 7         | 3PGSW                                          | 3PGSWRXTHR0        |
| 8         | 3PGSW                                          | 3PGSWRXINT0        |
| 9         | 3PGSW                                          | 3PGSWTXINT0        |
| 10        | 3PGSW                                          | 3PGSWMISC0         |
| 11        | McASP0                                         | MCATXINT0          |
| 12        | McASP0                                         | MCARXINT0          |
| 13        | McASP1                                         | MCATXINT1          |
| 14        | McASP1                                         | MCARXINT1          |
| 15        | Reserved                                       | Reserved           |
| 16        | Reserved                                       | Reserved           |
| 17        | GPIO 0                                         | GPIOINT0A          |
| 18        | GPIO 0                                         | GPIOINT0B          |
| 19        | GPIO 1                                         | GPIOINT1A          |
| 20        | GPIO 1                                         | GPIOINT1B          |
| 21        | GPIO 2                                         | GPIOINT2A          |
| 22        | GPIO 2                                         | GPIOINT2B          |
| 23        | GPIO 3                                         | GPIOINT3A          |
| 24        | GPIO 3                                         | GPIOINT3B          |
| 25        | DCAN0                                          | DCAN0_INT0         |
| 26        | DCAN0                                          | DCAN0_INT1         |
| 27        | DCAN0                                          | DCAN0_PARITY       |
| 28        | DCAN1                                          | DCAN1_INT0         |
| 29        | DCAN1                                          | DCAN1_INT1         |
|           |                                                |                    |

##### 9.2.4.4.6 ADC Capture Control

The following chip level events can be connected through the software-controlled multiplexer to the TSC_ADC module.

- 1. PRU-ICSS Host Event 0
- 2. Timer 4 Event
- 3. Timer 5 Event
- 4. Timer 6 Event
- 5. Timer 7 Event

This pin is the external hardware trigger to start the ADC channel conversion. The ADC_EVT_CAPT register needs to programmed to select the proper source for this conversion.

Table 9-6 contains the value to be programmed in the selection mux.

**Table 9-6. Selection Mux Values**

| ADC_EVENT_SEL Value | ADC External event selected |
|---------------------|-----------------------------|
| 000                 | PRU-ICSS Host Event 0       |
| 001                 | Timer 4 Event               |
| 010                 | Timer 5 Event               |
| 011                 | Timer 6 Event               |
| 100                 | Timer 7 Event               |
| 101-111             | Reserved                    |

##### 9.2.4.4.7 SRAM LDO Control

The device incorporates two instances of the SRAM LDO (VSLDO) module. One of these LDOs powers the ARM internal SRAM and the other powers the OCMC SRAMs. In the SMA2 register, the VSLDO_CORE_AUTO_RAMP_EN bit, when set, allows the VSLDO, which powers the OCMC SRAMs, to be put into retention during deepsleep and enable lower power consumption. Since the VSLDO is shared between WKUP M3 memories and CORE memories, the VSLDO has to be brought out of retention on any wakeup event. This bit allows this functionality and should be set to allow proper sleep/wakeup operation during Standby and DeepSleep modes. Similar functionality is not necessary for the LDO powering the ARM internal SRAM. It can be put in retention mode using PRM_LDO_SRAM_MPU_CTRL.

### 9.2.5 DDR PHY

**Table 9-7. DDR Slew Rate Control Settings(1)(2)**

| sr1 | sr0 | Slew Rate Level |
|-----|-----|-----------------|
| 0   | 0   | Fastest         |
| 1   | 0   | Fast            |
| 0   | 1   | Slow            |
| 1   | 1   | Slowest         |

<sup>(1)</sup> These values are programmed in the following registers: ddr_cmd0_ioctrl, ddr_cmd1_ioctrl, ddr_cmd2_ioctrl, ddr_data0_ioctrl, ddr_data1_ioctrl.

**Table 9-8. DDR Impedance Control Settings(1)(2)(3)**

| I2 | I1 | I0 | Output Impedance<br>(Ron) | Drive Strength<br> IOH ,  IOL | Example:<br>Ron for Rext =<br>49.9 ohms | Example:<br> IOH ,  IOL  for Rext =<br>49.9 ohms |
|----|----|----|---------------------------|-------------------------------|-----------------------------------------|--------------------------------------------------|
| 0  | 0  | 0  | 1.6*Rext                  | 0.625*Iout                    | 80 ohms                                 | 5 mA                                             |
| 0  | 0  | 1  | 1.33*Rext                 | 0.75*Iout                     | 67 ohms                                 | 6 mA                                             |
| 0  | 1  | 0  | 1.14*Rext                 | 0.875*Iout                    | 57 ohms                                 | 7 mA                                             |
| 0  | 1  | 1  | Rext                      | Iout                          | 50 ohms                                 | 8 mA                                             |
| 1  | 0  | 0  | 0.88*Rext                 | 1.125*Iout                    | 44 ohms                                 | 9 mA                                             |
| 1  | 0  | 1  | 0.8*Rext                  | 1.250*Iout                    | 40 ohms                                 | 10 mA                                            |
| 1  | 1  | 0  | 0.73*Rext                 | 1.375*Iout                    | 36 ohms                                 | 11 mA                                            |
| 1  | 1  | 1  | 0.67*Rext                 | 1.5*Iout                      | 33 ohms                                 | 12 mA                                            |

<sup>(1)</sup> These values are programmed in the following registers: ddr_cmd0_ioctrl, ddr_cmd1_ioctrl, ddr_cmd2_ioctrl, ddr_data0_ioctrl, ddr_data1_ioctrl.

#### 9.2.5.1 DDR PHY to IO Pin Mapping

The following table describes the DDR PHY to IO pin mapping.

**Table 9-9. DDR PHY to IO Pin Mapping**

| Macro Pin | CMD0    | CMD1     | CMD2       | DATA0     | DATA1     |
|-----------|---------|----------|------------|-----------|-----------|
| 0         | ddr_ba2 | Unconn   | ddr_cke    | ddr_d8    | ddr_d0    |
| 1         | ddr_wen | ddr_a15  | ddr_resetn | ddr_d9    | ddr_d1    |
| 2         | ddr_ba0 | ddr_a2   | ddr_odt    | ddr_d10   | ddr_d2    |
| 3         | ddr_a5  | ddr_a12  | Unconn     | ddr_d11   | ddr_d3    |
| 4         | ddr_ck  | ddr_a7   | ddr_a14    | ddr_d12   | ddr_d4    |
| 5         | ddr_ckn | ddr_ba1  | ddr_a13    | ddr_d13   | ddr_d5    |
| 6         | ddr_a3  | ddr_a10  | ddr_csn0   | ddr_d14   | ddr_d6    |
| 7         | ddr_a4  | ddr_a0   | Unconn     | ddr_d15   | ddr_d7    |
| 8         | ddr_a8  | ddr_a11  | ddr_a1     | ddr_dqm1  | ddr_dqm0  |
| 9         | ddr_a9  | ddr_casn | Unconn     | ddr_dqs1  | ddr_dqs0  |
| 10        | ddr_a6  | ddr_rasn | Unconn     | ddr_dqsn1 | ddr_dqsn0 |

<sup>(2)</sup> Values for DDR_CMDx_IOCTRL.io_config_sr_clk should be programmed to the same value.

<sup>(2)</sup> Values for DDR_CMDx_IOCTRL.io_config_i_clk should be programmed to the same value.

<sup>(3)</sup> Rext is the external VTP compensation resistor connected to DDR_VTP terminal.

---

## 9.3 Registers

### 9.3.1 CONTROL_MODULE Registers

Table 9-10 lists the memory-mapped registers for the CONTROL_MODULE. All other register offset addresses not listed in Table 9-10 should be considered as reserved locations and the register contents should not be modified.

**Table 9-10. CONTROL_MODULE REGISTERS**

| Offset | Acronym                   | Register Description | Section          |
|--------|---------------------------|----------------------|------------------|
| 0h     | control_revision          |                      | Section 9.3.1.1  |
| 4h     | control_hwinfo            |                      | Section 9.3.1.2  |
| 10h    | control_sysconfig         |                      | Section 9.3.1.3  |
| 40h    | control_status            |                      | Section 9.3.1.4  |
| 110h   | control_emif_sdram_config |                      | Section 9.3.1.5  |
| 428h   | core_sldo_ctrl            |                      | Section 9.3.1.6  |
| 42Ch   | mpu_sldo_ctrl             |                      | Section 9.3.1.7  |
| 444h   | clk32kdivratio_ctrl       |                      | Section 9.3.1.8  |
| 448h   | bandgap_ctrl              |                      | Section 9.3.1.9  |
| 44Ch   | bandgap_trim              |                      | Section 9.3.1.10 |
| 458h   | pll_clkinpulow_ctrl       |                      | Section 9.3.1.11 |
| 468h   | mosc_ctrl                 |                      | Section 9.3.1.12 |
| 470h   | deepsleep_ctrl            |                      | Section 9.3.1.13 |
| 50Ch   | dpll_pwr_sw_status        |                      | Section 9.3.1.14 |
| 600h   | device_id                 |                      | Section 9.3.1.15 |
| 604h   | dev_feature               |                      | Section 9.3.1.16 |
| 608h   | init_priority_0           |                      | Section 9.3.1.17 |
| 60Ch   | init_priority_1           |                      | Section 9.3.1.18 |
| 614h   | tptc_cfg                  |                      | Section 9.3.1.19 |
| 620h   | usb_ctrl0                 |                      | Section 9.3.1.20 |
| 624h   | usb_sts0                  |                      | Section 9.3.1.21 |
| 628h   | usb_ctrl1                 |                      | Section 9.3.1.22 |
| 62Ch   | usb_sts1                  |                      | Section 9.3.1.23 |
| 630h   | mac_id0_lo                |                      | Section 9.3.1.24 |
| 634h   | mac_id0_hi                |                      | Section 9.3.1.25 |
| 638h   | mac_id1_lo                |                      | Section 9.3.1.26 |
| 63Ch   | mac_id1_hi                |                      | Section 9.3.1.27 |
| 644h   | dcan_raminit              |                      | Section 9.3.1.28 |
| 648h   | usb_wkup_ctrl             |                      | Section 9.3.1.29 |
| 650h   | gmii_sel                  |                      | Section 9.3.1.30 |
| 664h   | pwmss_ctrl                |                      | Section 9.3.1.31 |
| 670h   | mreqprio_0                |                      | Section 9.3.1.32 |
| 674h   | mreqprio_1                |                      | Section 9.3.1.33 |
| 690h   | hw_event_sel_grp1         |                      | Section 9.3.1.34 |
| 694h   | hw_event_sel_grp2         |                      | Section 9.3.1.35 |
| 698h   | hw_event_sel_grp3         |                      | Section 9.3.1.36 |
| 69Ch   | hw_event_sel_grp4         |                      | Section 9.3.1.37 |
| 6A0h   | smrt_ctrl                 |                      | Section 9.3.1.38 |
| 6A4h   | mpuss_hw_debug_sel        |                      | Section 9.3.1.39 |
| 6A8h   | mpuss_hw_dbg_info         |                      | Section 9.3.1.40 |
| 770h   | vdd_mpu_opp_050           |                      | Section 9.3.1.41 |

| Offset | Acronym            | Register Description                                                                                                                                                                              | Section          |
|--------|--------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------|
| 774h   | vdd_mpu_opp_100    |                                                                                                                                                                                                   | Section 9.3.1.42 |
| 778h   | vdd_mpu_opp_120    |                                                                                                                                                                                                   | Section 9.3.1.43 |
| 77Ch   | vdd_mpu_opp_turbo  |                                                                                                                                                                                                   | Section 9.3.1.44 |
| 7B8h   | vdd_core_opp_050   |                                                                                                                                                                                                   | Section 9.3.1.45 |
| 7BCh   | vdd_core_opp_100   |                                                                                                                                                                                                   | Section 9.3.1.46 |
| 7D0h   | bb_scale           |                                                                                                                                                                                                   | Section 9.3.1.47 |
| 7F4h   | usb_vid_pid        |                                                                                                                                                                                                   | Section 9.3.1.48 |
| 7FCh   | efuse_sma          |                                                                                                                                                                                                   | Section 9.3.1.49 |
| 800h   | conf_gpmc_ad0      | See the device datasheet for information on default pin<br>mux configurations. Note that the device ROM may<br>change the default pin mux for certain pins based on the<br>SYSBOOT mode settings. | Section 9.3.1.50 |
| 804h   | conf_gpmc_ad1      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 808h   | conf_gpmc_ad2      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 80Ch   | conf_gpmc_ad3      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 810h   | conf_gpmc_ad4      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 814h   | conf_gpmc_ad5      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 818h   | conf_gpmc_ad6      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 81Ch   | conf_gpmc_ad7      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 820h   | conf_gpmc_ad8      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 824h   | conf_gpmc_ad9      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 828h   | conf_gpmc_ad10     |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 82Ch   | conf_gpmc_ad11     |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 830h   | conf_gpmc_ad12     |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 834h   | conf_gpmc_ad13     |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 838h   | conf_gpmc_ad14     |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 83Ch   | conf_gpmc_ad15     |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 840h   | conf_gpmc_a0       |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 844h   | conf_gpmc_a1       |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 848h   | conf_gpmc_a2       |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 84Ch   | conf_gpmc_a3       |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 850h   | conf_gpmc_a4       |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 854h   | conf_gpmc_a5       |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 858h   | conf_gpmc_a6       |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 85Ch   | conf_gpmc_a7       |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 860h   | conf_gpmc_a8       |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 864h   | conf_gpmc_a9       |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 868h   | conf_gpmc_a10      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 86Ch   | conf_gpmc_a11      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 870h   | conf_gpmc_wait0    |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 874h   | conf_gpmc_wpn      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 878h   | conf_gpmc_ben1     |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 87Ch   | conf_gpmc_csn0     |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 880h   | conf_gpmc_csn1     |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 884h   | conf_gpmc_csn2     |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 888h   | conf_gpmc_csn3     |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 88Ch   | conf_gpmc_clk      |                                                                                                                                                                                                   | Section 9.3.1.50 |
| 890h   | conf_gpmc_advn_ale |                                                                                                                                                                                                   | Section 9.3.1.50 |

| Offset       | Acronym                          | Register Description | Section                              |
|--------------|----------------------------------|----------------------|--------------------------------------|
| 894h         | conf_gpmc_oen_ren                |                      | Section 9.3.1.50                     |
| 898h         | conf_gpmc_wen                    |                      | Section 9.3.1.50                     |
| 89Ch         | conf_gpmc_ben0_cle               |                      | Section 9.3.1.50                     |
| 8A0h         | conf_lcd_data0                   |                      | Section 9.3.1.50                     |
| 8A4h         | conf_lcd_data1                   |                      | Section 9.3.1.50                     |
| 8A8h         | conf_lcd_data2                   |                      | Section 9.3.1.50                     |
| 8ACh         | conf_lcd_data3                   |                      | Section 9.3.1.50                     |
| 8B0h         | conf_lcd_data4                   |                      | Section 9.3.1.50                     |
| 8B4h         | conf_lcd_data5                   |                      | Section 9.3.1.50                     |
| 8B8h         | conf_lcd_data6                   |                      | Section 9.3.1.50                     |
| 8BCh         | conf_lcd_data7                   |                      | Section 9.3.1.50                     |
| 8C0h         | conf_lcd_data8                   |                      | Section 9.3.1.50                     |
| 8C4h         | conf_lcd_data9                   |                      | Section 9.3.1.50                     |
| 8C8h         | conf_lcd_data10                  |                      | Section 9.3.1.50                     |
| 8CCh         | conf_lcd_data11                  |                      | Section 9.3.1.50                     |
| 8D0h         | conf_lcd_data12                  |                      | Section 9.3.1.50                     |
| 8D4h         | conf_lcd_data13                  |                      | Section 9.3.1.50                     |
| 8D8h         | conf_lcd_data14                  |                      | Section 9.3.1.50                     |
| 8DCh         | conf_lcd_data15                  |                      | Section 9.3.1.50                     |
| 8E0h         | conf_lcd_vsync                   |                      | Section 9.3.1.50                     |
| 8E4h         | conf_lcd_hsync                   |                      | Section 9.3.1.50                     |
| 8E8h         | conf_lcd_pclk                    |                      | Section 9.3.1.50                     |
| 8ECh         | conf_lcd_ac_bias_en              |                      | Section 9.3.1.50                     |
| 8F0h         | conf_mmc0_dat3                   |                      | Section 9.3.1.50                     |
| 8F4h         | conf_mmc0_dat2                   |                      | Section 9.3.1.50                     |
| 8F8h         | conf_mmc0_dat1                   |                      | Section 9.3.1.50                     |
| 8FCh         | conf_mmc0_dat0                   |                      | Section 9.3.1.50                     |
| 900h         | conf_mmc0_clk                    |                      | Section 9.3.1.50                     |
| 904h         | conf_mmc0_cmd                    |                      | Section 9.3.1.50                     |
| 908h         | conf_mii1_col                    |                      | Section 9.3.1.50                     |
| 90Ch         | conf_mii1_crs                    |                      | Section 9.3.1.50                     |
| 910h         | conf_mii1_rx_er                  |                      | Section 9.3.1.50                     |
| 914h         | conf_mii1_tx_en                  |                      | Section 9.3.1.50                     |
| 918h         | conf_mii1_rx_dv                  |                      | Section 9.3.1.50                     |
| 91Ch         | conf_mii1_txd3                   |                      | Section 9.3.1.50                     |
| 920h         | conf_mii1_txd2                   |                      | Section 9.3.1.50                     |
| 924h         | conf_mii1_txd1                   |                      | Section 9.3.1.50                     |
| 928h         | conf_mii1_txd0                   |                      | Section 9.3.1.50                     |
| 92Ch         | conf_mii1_tx_clk                 |                      | Section 9.3.1.50                     |
| 930h         | conf_mii1_rx_clk                 |                      | Section 9.3.1.50                     |
| 934h         | conf_mii1_rxd3                   |                      | Section 9.3.1.50                     |
| 938h         | conf_mii1_rxd2                   |                      | Section 9.3.1.50                     |
| 93Ch<br>940h | conf_mii1_rxd1<br>conf_mii1_rxd0 |                      | Section 9.3.1.50<br>Section 9.3.1.50 |
| 944h         | conf_rmii1_ref_clk               |                      | Section 9.3.1.50                     |
| 948h         | conf_mdio                        |                      | Section 9.3.1.50                     |
| 94Ch         | conf_mdc                         |                      | Section 9.3.1.50                     |
|              |                                  |                      |                                      |

| Offset | Acronym                | Register Description | Section          |
|--------|------------------------|----------------------|------------------|
| 950h   | conf_spi0_sclk         |                      | Section 9.3.1.50 |
| 954h   | conf_spi0_d0           |                      | Section 9.3.1.50 |
| 958h   | conf_spi0_d1           |                      | Section 9.3.1.50 |
| 95Ch   | conf_spi0_cs0          |                      | Section 9.3.1.50 |
| 960h   | conf_spi0_cs1          |                      | Section 9.3.1.50 |
| 964h   | conf_ecap0_in_pwm0_out |                      | Section 9.3.1.50 |
| 968h   | conf_uart0_ctsn        |                      | Section 9.3.1.50 |
| 96Ch   | conf_uart0_rtsn        |                      | Section 9.3.1.50 |
| 970h   | conf_uart0_rxd         |                      | Section 9.3.1.50 |
| 974h   | conf_uart0_txd         |                      | Section 9.3.1.50 |
| 978h   | conf_uart1_ctsn        |                      | Section 9.3.1.50 |
| 97Ch   | conf_uart1_rtsn        |                      | Section 9.3.1.50 |
| 980h   | conf_uart1_rxd         |                      | Section 9.3.1.50 |
| 984h   | conf_uart1_txd         |                      | Section 9.3.1.50 |
| 988h   | conf_i2c0_sda          |                      | Section 9.3.1.50 |
| 98Ch   | conf_i2c0_scl          |                      | Section 9.3.1.50 |
| 990h   | conf_mcasp0_aclkx      |                      | Section 9.3.1.50 |
| 994h   | conf_mcasp0_fsx        |                      | Section 9.3.1.50 |
| 998h   | conf_mcasp0_axr0       |                      | Section 9.3.1.50 |
| 99Ch   | conf_mcasp0_ahclkr     |                      | Section 9.3.1.50 |
| 9A0h   | conf_mcasp0_aclkr      |                      | Section 9.3.1.50 |
| 9A4h   | conf_mcasp0_fsr        |                      | Section 9.3.1.50 |
| 9A8h   | conf_mcasp0_axr1       |                      | Section 9.3.1.50 |
| 9ACh   | conf_mcasp0_ahclkx     |                      | Section 9.3.1.50 |
| 9B0h   | conf_xdma_event_intr0  |                      | Section 9.3.1.50 |
| 9B4h   | conf_xdma_event_intr1  |                      | Section 9.3.1.50 |
| 9B8h   | conf_warmrstn          |                      | Section 9.3.1.50 |
| 9C0h   | conf_nnmi              |                      | Section 9.3.1.50 |
| 9D0h   | conf_tms               |                      | Section 9.3.1.50 |
| 9D4h   | conf_tdi               |                      | Section 9.3.1.50 |
| 9D8h   | conf_tdo               |                      | Section 9.3.1.50 |
| 9DCh   | conf_tck               |                      | Section 9.3.1.50 |
| 9E0h   | conf_trstn             |                      | Section 9.3.1.50 |
| 9E4h   | conf_emu0              |                      | Section 9.3.1.50 |
| 9E8h   | conf_emu1              |                      | Section 9.3.1.50 |
| 9F8h   | conf_rtc_pwronrstn     |                      | Section 9.3.1.50 |
| 9FCh   | conf_pmic_power_en     |                      | Section 9.3.1.50 |
| A00h   | conf_ext_wakeup        |                      | Section 9.3.1.50 |
| A1Ch   | conf_usb0_drvvbus      |                      | Section 9.3.1.50 |
| A34h   | conf_usb1_drvvbus      |                      | Section 9.3.1.50 |
| E00h   | cqdetect_status        |                      | Section 9.3.1.51 |
| E04h   | ddr_io_ctrl            |                      | Section 9.3.1.52 |
| E0Ch   | vtp_ctrl               |                      | Section 9.3.1.53 |
| E14h   | vref_ctrl              |                      | Section 9.3.1.54 |
| F90h   | tpcc_evt_mux_0_3       |                      | Section 9.3.1.55 |
| F94h   | tpcc_evt_mux_4_7       |                      | Section 9.3.1.56 |
| F98h   | tpcc_evt_mux_8_11      |                      | Section 9.3.1.57 |

| Offset | Acronym            | Register Description | Section          |
|--------|--------------------|----------------------|------------------|
| F9Ch   | tpcc_evt_mux_12_15 |                      | Section 9.3.1.58 |
| FA0h   | tpcc_evt_mux_16_19 |                      | Section 9.3.1.59 |
| FA4h   | tpcc_evt_mux_20_23 |                      | Section 9.3.1.60 |
| FA8h   | tpcc_evt_mux_24_27 |                      | Section 9.3.1.61 |
| FACh   | tpcc_evt_mux_28_31 |                      | Section 9.3.1.62 |
| FB0h   | tpcc_evt_mux_32_35 |                      | Section 9.3.1.63 |
| FB4h   | tpcc_evt_mux_36_39 |                      | Section 9.3.1.64 |
| FB8h   | tpcc_evt_mux_40_43 |                      | Section 9.3.1.65 |
| FBCh   | tpcc_evt_mux_44_47 |                      | Section 9.3.1.66 |
| FC0h   | tpcc_evt_mux_48_51 |                      | Section 9.3.1.67 |
| FC4h   | tpcc_evt_mux_52_55 |                      | Section 9.3.1.68 |
| FC8h   | tpcc_evt_mux_56_59 |                      | Section 9.3.1.69 |
| FCCh   | tpcc_evt_mux_60_63 |                      | Section 9.3.1.70 |
| FD0h   | timer_evt_capt     |                      | Section 9.3.1.71 |
| FD4h   | ecap_evt_capt      |                      | Section 9.3.1.72 |
| FD8h   | adc_evt_capt       |                      | Section 9.3.1.73 |
| 1000h  | reset_iso          |                      | Section 9.3.1.74 |
| 1318h  | dpll_pwr_sw_ctrl   |                      | Section 9.3.1.75 |
| 131Ch  | ddr_cke_ctrl       |                      | Section 9.3.1.76 |
| 1320h  | sma2               |                      | Section 9.3.1.77 |
| 1324h  | m3_txev_eoi        |                      | Section 9.3.1.78 |
| 1328h  | ipc_msg_reg0       |                      | Section 9.3.1.79 |
| 132Ch  | ipc_msg_reg1       |                      | Section 9.3.1.80 |
| 1330h  | ipc_msg_reg2       |                      | Section 9.3.1.81 |
| 1334h  | ipc_msg_reg3       |                      | Section 9.3.1.82 |
| 1338h  | ipc_msg_reg4       |                      | Section 9.3.1.83 |
| 133Ch  | ipc_msg_reg5       |                      | Section 9.3.1.84 |
| 1340h  | ipc_msg_reg6       |                      | Section 9.3.1.85 |
| 1344h  | ipc_msg_reg7       |                      | Section 9.3.1.86 |
| 1404h  | ddr_cmd0_ioctrl    |                      | Section 9.3.1.87 |
| 1408h  | ddr_cmd1_ioctrl    |                      | Section 9.3.1.88 |
| 140Ch  | ddr_cmd2_ioctrl    |                      | Section 9.3.1.89 |
| 1440h  | ddr_data0_ioctrl   |                      | Section 9.3.1.90 |
| 1444h  | ddr_data1_ioctrl   |                      | Section 9.3.1.91 |

#### 9.3.1.1 control_revision Register (offset = 0h) [reset = 0h]

control_revision is described in Table 9-11.

**Table 9-11. control_revision Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                          |  |  |
|-------|---------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|
| 31-30 | ip_rev_scheme | R    | 0h    | 01 - New Scheme                                                                                                                                                                      |  |  |
| 29-28 | Reserved      | R    | 0h    |                                                                                                                                                                                      |  |  |
| 27-16 | ip_rev_func   | R    | 0h    | Function indicates a software compatible module family.<br>If there is no level of software compatibility a new Func number (and<br>hence REVISION) should be assigned.              |  |  |
| 15-11 | ip_rev_rtl    | R    | 0h    | RTL Version (R).                                                                                                                                                                     |  |  |
| 10-8  | ip_rev_major  | R    | 0h    | Major Revision (X).                                                                                                                                                                  |  |  |
| 7-6   | ip_rev_custom | R    | 0h    | Indicates a special version for a particular device. Consequence of<br>use may avoid use of standard Chip Support Library (CSL) / Drivers<br>-<br>00: Non custom (standard) revision |  |  |
| 5-0   | ip_rev_minor  | R    | 0h    | Minor Revision (Y).                                                                                                                                                                  |  |  |

#### 9.3.1.2 control_hwinfo Register (offset = 4h) [reset = 0h]

control_hwinfo is described in Table 9-12.

**Table 9-12. control_hwinfo Register Field Descriptions**

| Bit  | Field     | Type | Reset | Description         |
|------|-----------|------|-------|---------------------|
| 31-0 | ip_hwinfo | R    | 0h    | IP Module dependent |

#### 9.3.1.3 control_sysconfig Register (offset = 10h) [reset = 0h]

control_sysconfig is described in Table 9-13.

**Table 9-13. control_sysconfig Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                    |
|------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-6 | Reserved | R    | 0h    |                                                                                                                                                                                                                |
| 5-4  | standby  | R    | 0h    | Configure local initiator state management<br>00: Force Standby<br>01: No Standby Mode<br>10: Smart Standby<br>11: Smart Standby wakeup capable<br>Reserved in Control Module since it has no local initiator. |
| 3-2  | idlemode | R/W  | 0h    | Configure local target state management<br>00: Force Idle<br>01: No Idle<br>10: Smart Idle<br>11: Smart Idle wakeup capable                                                                                    |
| 1    | freeemu  | R    | 0h    | Sensitivity to Emulation suspend input.<br>0: Module is sensitive to EMU suspend<br>1: Module not sensitive to EMU suspend                                                                                     |
| 0    | Reserved | R    | 0h    |                                                                                                                                                                                                                |

#### 9.3.1.4 control_status Register (offset = 40h) [reset = 0h]

control_status is described in Table 9-14.

**Table 9-14. control_status Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                            |  |  |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|
| 31-24 | Reserved | R    | 0h    |                                                                                                                                                                        |  |  |
| 23-22 | sysboot1 | R    | X     | Used to select crystal clock frequency.<br>See SYSBOOT Configuration Pins.<br>Reset value is from SYSBOOT[15:14].                                                      |  |  |
| 21-20 | testmd   | R    | X     | Set to 00b.<br>See SYSBOOT Configuration Pins for more information.<br>Reset value is from SYSBOOT[13:12].                                                             |  |  |
| 19-18 | admux    | R    | X     | GPMC CS0 Default Address Muxing<br>00: No Addr/Data Muxing<br>01: Addr/Addr/Data Muxing<br>10: Addr/Data Muxing<br>11: Reserved<br>Reset value is from SYSBOOT[11:10]. |  |  |
| 17    | waiten   | R    | X     | GPMC CS0 Default Wait Enable<br>0: Ignore WAIT input<br>1: Use WAIT input<br>See SYSBOOT Configuration Pins for more information.<br>Reset value is from SYSBOOT[9].   |  |  |
| 16    | bw       | R    | X     | GPMC CS0 Default Bus Width<br>0: 8-bit data bus<br>1: 16-bit data bus<br>See SYSBOOT Configuration Pins for more information.<br>Reset value is from SYSBOOT[8].       |  |  |
| 15-11 | Reserved | R    | 0h    |                                                                                                                                                                        |  |  |
| 10-8  | devtype  | R    | 11b   | 000: Reserved<br>001: Reserved<br>010: Reserved<br>011: General Purpose (GP) Device<br>111: Reserved                                                                   |  |  |
| 7-0   | sysboot0 | R    | X     | Selected boot mode.<br>See SYSBOOT Configuration Pins for more information.<br>Reset value is from SYSBOOT[7:0].                                                       |  |  |

#### 9.3.1.5 control_emif_sdram_config Register (offset = 110h) [reset = 0h]

The CONTROL_EMIF_SDRAM_CONFIG register exports SDRAM configuration information to the EMIF after resuming from low power scenarios.

This register should be loaded with the same value as SDRAM_CONFIG during DDR initialization. control_emif_sdram_config is described in Table 9-15.

**Table 9-15. control_emif_sdram_config Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|-------|------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-29 | SDRAM_TYPE | R/W  | 0h    | SDRAM Type selection<br>000 – Reserved<br>001 – LPDDR1<br>010 – DDR2<br>011 – DDR3<br>100 – Reserved<br>101 – Reserved<br>110 – Reserved<br>111 – Reserved                                                                                                                                                                                                                                                                                                                                                   |
| 28-27 | IBANK_POS  | R/W  | 0h    | Internal bank position.<br>00 - All Bank Address bits assigned from OCP address above<br>column address bits.<br>01 – Bank Address bits [1:0] assigned from OCP address above<br>column address bits and bit [2] from OCP address bits above row<br>address bits.<br>10 – Bank Address bit [0] assigned from OCP address above<br>column address bits and bit [2:1] from OCP address bits above row<br>address bits.<br>11 – All Bank Address bits assigned from OCP address bits above<br>row address bits. |
| 26-24 | DDR_TERM   | R/W  | 0h    | DDR2 and DDR3 termination resistor value. Set to 0 to disable<br>termination.<br>For DDR2, set to 1 for 75 ohm, set to 2 for 150 ohm, and set to 3 for<br>50 ohm.<br>For DDR3, set to 1 for RZQ/4, set to 2 for RZQ/2, set to 3 for RZQ/6,<br>set to 4 for RZQ/12, and set to 5 for RZQ/8.<br>All other values are reserved.                                                                                                                                                                                 |
| 23    | DDR2_DDQS  | R    | 0h    | Reserved. Defaults to 0 for single ended DQS. For differential<br>operation, SDRAM_CONFIG register in the EMIF module must be<br>written.                                                                                                                                                                                                                                                                                                                                                                    |

**Table 9-15. control_emif_sdram_config Register Field Descriptions (continued)**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                |  |  |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|
| 22-21 | DYN_ODT     | R/W  | 0h    | DDR3 Dynamic ODT.<br>Set to 0 to turn off dynamic ODT.<br>Set to 1 for RZQ/4 and set to 2 for RZQ/2.<br>All other values are reserved.                                                                                                                                                                                                                                                                     |  |  |
| 20    | Reserved    | R    | 0h    | Reserved. Read returns 0.                                                                                                                                                                                                                                                                                                                                                                                  |  |  |
| 19-18 | SDRAM_DRIVE | R/W  | 0h    | SDRAM drive strength.<br>For DDR2, set to 0 for normal, and set to 1 for weak drive strength.<br>For DDR3, set to 0 for RZQ/6 and set to 1 for RZQ/7.<br>For LPDDR1, set to 0 for full, set to 1 for 1/2, set to 2 for 1/4, and set<br>to 3 for 1/8 drive strength.<br>All other values are reserved.                                                                                                      |  |  |
| 17-16 | CWL         | R/W  | 0h    | DDR3 CAS Write latency. Value of 0, 1, 2, and 3 (CAS write latency<br>of 5, 6, 7, and 8) are supported. Use the lowest value supported for<br>best performance. All other values are reserved.                                                                                                                                                                                                             |  |  |
| 15-14 | NARROW_MODE | R/W  | 0h    | SDRAM data bus width. Set to 0 for 32-bit and set to 1 for 16-bit. All<br>other values are reserved.                                                                                                                                                                                                                                                                                                       |  |  |
| 13-10 | CL          | R/W  | 0h    | CAS Latency. The value of this field defines the CAS latency to be<br>used when accessing connected SDRAM devices. Value of 2, 3, 4,<br>and 5 (CAS latency of 2, 3, 4, and 5) are supported for DDR2. Value<br>of 2, 4, 6, 8, 10, 12, and 14 (CAS latency of 5, 6, 7, 8, 9, 10, and 11)<br>are supported for DDR3. All other values are reserved.                                                          |  |  |
| 9-7   | ROWSIZE     | R/W  | 0h    | Row Size. Defines the number of row address bits of connected<br>SDRAM devices. Set to 0 for 9 row bits, set to 1 for 10 row bits, set<br>to 2 for 11 row bits, set to 3 for 12 row bits, set to 4 for 13 row bits,<br>set to 5 for 14 row bits, set to 6 for 15 row bits, and set to 7 for 16<br>row bits. This field is only used when ibank_pos field in SDRAM<br>Config register is set to 1, 2, or 3. |  |  |
| 6-4   | IBANK       | R/W  | 0h    | Internal Bank setup. Defines number of banks inside connected<br>SDRAM devices. Set to 0 for 1 bank, set to 1 for 2 banks, set to 2 for<br>4 banks, and set to 3 for 8 banks. All other values are reserved.                                                                                                                                                                                               |  |  |
| 3     | EBANK       | R/W  | 0h    | External chip select setup. Defines whether SDRAM accesses will<br>use 1 or 2 chip select lines. Set to 0 to use pad_cs_o_n[0] only. Set<br>to 1 to use pad_cs_o_n[1:0].                                                                                                                                                                                                                                   |  |  |
| 2-0   | PAGESIZE    | R/W  | 0h    | Page Size. Defines the internal page size of connected SDRAM<br>devices. Set to 0 for 256-word page (8 column bits), set to 1 for 512-<br>word page (9 column bits), set to 2 for 1024-word page (10 column<br>bits), and set to 3 for 2048-word page (11 column bits). All other<br>values are reserved.                                                                                                  |  |  |

#### 9.3.1.6 core_sldo_ctrl Register (offset = 428h) [reset = 0h]

core_sldo_ctrl is described in Table 9-16.

**Table 9-16. core_sldo_ctrl Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description |
|-------|----------|------|-------|-------------|
| 31-26 | Reserved | R/W  | 0h    |             |
| 25-16 | vset     | R/W  | 0h    | Trims VDDAR |
| 15-0  | Reserved | R/W  | 0h    |             |

#### 9.3.1.7 mpu_sldo_ctrl Register (offset = 42Ch) [reset = 0h]

mpu_sldo_ctrl is described in Table 9-17.

**Table 9-17. mpu_sldo_ctrl Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description |
|-------|----------|------|-------|-------------|
| 31-26 | Reserved | R/W  | 0h    |             |
| 25-16 | vset     | R/W  | 0h    | Trims VDDAR |
| 15-0  | Reserved | R/W  | 0h    |             |

#### 9.3.1.8 clk32kdivratio_ctrl Register (offset = 444h) [reset = 0h]

clk32kdivratio_ctrl is described in Table 9-18.

**Table 9-18. clk32kdivratio_ctrl Register Field Descriptions**

| Bit  | Field          | Type | Reset | Description                                                                                                               |
|------|----------------|------|-------|---------------------------------------------------------------------------------------------------------------------------|
| 31-1 | Reserved       | R    | 0h    |                                                                                                                           |
| 0    | clkdivopp50_en | R/W  | 0h    | 0 : OPP100 operation, use ratio for 24MHz to 32KHz division<br>1 : OPP50 operation, use ratio for 12MHz to 32KHz division |

#### 9.3.1.9 bandgap_ctrl Register (offset = 448h) [reset = 0h]

bandgap_ctrl is described in Table 9-19.

**Table 9-19. bandgap_ctrl Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31-16 | Reserved | R    | 0h    |                                                                                      |
| 15-8  | dtemp    | R    | 0h    | Temperature data from ADC.<br>To be used when end of conversion (EOCZ) is 0.         |
| 7     | cbiassel | R/W  | 0h    | 0: Select bandgap voltage as reference<br>1: Select resistor divider as reference    |
| 6     | bgroff   | R/W  | 0h    | 0: Normal operation<br>1: Bandgap is OFF (OFF Mode)                                  |
| 5     | tmpsoff  | R/W  | 0h    | 0: Normal operation<br>1: Temperature sensor is off and thermal shutdown in OFF mode |
| 4     | soc      | R/W  | 0h    | ADC start of conversion.<br>Transition to high starts a new ADC conversion cycle.    |
| 3     | clrz     | R/W  | 0h    | 0: Resets the digital outputs                                                        |
| 2     | contconv | R/W  | 0h    | 0: ADC single conversion mode<br>1: ADC continuous conversion mode                   |
| 1     | ecoz     | R    | 0h    | ADC end of conversion<br>0: End of conversion<br>1: Conversion in progress           |
| 0     | tshut    | R    | 0h    | 0: Normal operation<br>1: Thermal shutdown event (greater than 147C)                 |

#### 9.3.1.10 bandgap_trim Register (offset = 44Ch) [reset = 0h]

bandgap_trim is described in Table 9-20.

**Table 9-20. bandgap_trim Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description                        |
|-------|-----------|------|-------|------------------------------------|
| 31-24 | dtrbgapc  | R/W  | 0h    | trim the output voltage of bandgap |
| 23-16 | dtrbgapv  | R/W  | 0h    | trim the output voltage of bandgap |
| 15-8  | dtrtemps  | R/W  | 0h    | trim the temperature sensor        |
| 7-0   | dtrtempsc | R/W  | 0h    | trim the temperature sensor        |

#### 9.3.1.11 pll_clkinpulow_ctrl Register (offset = 458h) [reset = 0h]

pll_clkinpulow_ctrl is described in Table 9-21.

**Table 9-21. pll_clkinpulow_ctrl Register Field Descriptions**

| Bit  | Field                   | Type | Reset | Description                                                                                   |
|------|-------------------------|------|-------|-----------------------------------------------------------------------------------------------|
| 31-3 | Reserved                | R    | 0h    |                                                                                               |
| 2    | ddr_pll_clkinpulow_sel  | R/W  | 0h    | 0 : Select CORE_CLKOUT_M6 clock as CLKINPULOW<br>1 : Select PER_CLKOUT_M2 clock as CLKINPULOW |
| 1    | disp_pll_clkinpulow_sel | R/W  | 0h    | 0 : Select CORE_CLKOUT_M6 clock as CLKINPULOW<br>1 : Select PER_CLKOUT_M2 clock as CLKINPULOW |
| 0    | mpu_dpll_clkinpulow_sel | R/W  | 0h    | 0 : Select CORE_CLKOUT_M6 clock as CLKINPULOW<br>1 : Select PER_CLKOUT_M2 clock as CLKINPULOW |

#### 9.3.1.12 mosc_ctrl Register (offset = 468h) [reset = 0h]

mosc_ctrl is described in Table 9-22.

**Table 9-22. mosc_ctrl Register Field Descriptions**

| Bit  | Field     | Type | Reset | Description                                                                                                                                                                        |
|------|-----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | Reserved  | R    | 0h    |                                                                                                                                                                                    |
| 0    | resselect | R/W  | 0h    | 0: Enable 1M ohm internal resistor (connected between XTALIN and<br>XTALOUT).<br>1: Disable 1M ohm internal resistor (bias resistor needs to be<br>provided externally to device). |

#### 9.3.1.13 deepsleep_ctrl Register (offset = 470h) [reset = 0h]

deepsleep_ctrl is described in Table 9-23.

**Table 9-23. deepsleep_ctrl Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                        |
|-------|----------|------|-------|----------------------------------------------------------------------------------------------------|
| 31-18 | Reserved | R    | 0h    |                                                                                                    |
| 17    | dsenable | R/W  | 0h    | Deep sleep enable<br>0: Normal operation<br>1: Master oscillator output is gated                   |
| 16    | Reserved | R    | 0h    |                                                                                                    |
| 15-0  | dscount  | R/W  | 0h    | Programmable count of how many CLK_M_OSC clocks needs to be<br>seen before exiting deep sleep mode |

#### 9.3.1.14 dpll_pwr_sw_status (offset = 50Ch) [reset = 0h]

dpll_pwr_sw_status is described in Table 9-24.

**Table 9-24. dpll_pwr_sw_status Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                        |
|-------|---------------|------|-------|--------------------------------------------------------------------|
| 31-26 | Reserved      | R    | 0h    |                                                                    |
| 25    | pgoodout_ddr  | R    | 0h    | Power Good status for DDR DPLL<br>0: Power Fault<br>1: Power Good  |
| 24    | ponout_ddr    | R    | 0h    | Power Enable status for DDR DPLL<br>0: Disabled<br>1: Enabled      |
| 23-18 | Reserved      | R    | 0h    |                                                                    |
| 17    | pgoodout_disp | R    | 0h    | Power Good status for DISP DPLL<br>0: Power Fault<br>1: Power Good |
| 16    | ponout_disp   | R    | 0h    | Power Enable status for DISP DPLL<br>0: Disabled<br>1: Enabled     |
| 15-10 | Reserved      | R    | 0h    |                                                                    |
| 9     | pgoodout_per  | R    | 0h    | Power Good status for PER DPLL<br>0: Power Fault<br>1: Power Good  |
| 8     | ponout_per    | R    | 0h    | Power Enable status for PER DPLL<br>0: Disabled<br>1: Enabled      |
| 7-0   | Reserved      | R    | 0h    |                                                                    |

#### 9.3.1.15 device_id Register (offset = 600h) [reset = 0x]

device_id is described in Table 9-25.

**Table 9-25. device_id Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                 |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-28 | devrev   | R    | 0h    | Device revision.<br>0000b - Silicon Revision 1.0<br>0001b - Silicon Revision 2.0<br>0010b - Silicon Revision 2.1<br>See device errata for detailed information on functionality in each<br>device revision. |
|       |          |      |       | Reset value is revision-dependent.                                                                                                                                                                          |
| 27-12 | partnum  | R    | B944h | Device part number (unique JTAG ID)                                                                                                                                                                         |
| 11-1  | mfgr     | R    | 017h  | Manufacturer's JTAG ID                                                                                                                                                                                      |
| 0     | Reserved | R    | 0h    |                                                                                                                                                                                                             |

#### 9.3.1.16 dev_feature Register (offset = 604h) [reset = 0h]

dev_feature is described in Table 9-26.

**Table 9-26. dev_feature Register Field Descriptions**

| Bit  | Field            | Type | Reset | Description                                                                     |
|------|------------------|------|-------|---------------------------------------------------------------------------------|
| 31-0 | dev_feature_bits | R    | 0h    | Device-dependent, See Device Feature Comparison table in device<br>data manual. |

#### 9.3.1.17 init_priority_0 Register (offset = 608h) [reset = 0h]

init_priority_0 is described in Table 9-27.

**Table 9-27. init_priority_0 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                          |
|-------|----------|------|-------|--------------------------------------|
| 31-28 | Reserved | R    | 0h    |                                      |
| 27-26 | tcwr2    | R/W  | 0h    | TPTC 2 Write Port initiator priority |
| 25-24 | tcrd2    | R/W  | 0h    | TPTC 2 Read Port initiator priority  |
| 23-22 | tcwr1    | R/W  | 0h    | TPTC 1 Write Port initiator priority |
| 21-20 | tcrd1    | R/W  | 0h    | TPTC 1 Read Port initiator priority  |
| 19-18 | tcwr0    | R/W  | 0h    | TPTC 0 Write Port initiator priority |
| 17-16 | tcrd0    | R/W  | 0h    | TPTC 0 Read Port initiator priority  |
| 15-14 | p1500    | R/W  | 0h    | P1500 Port Initiator priority        |
| 13-8  | Reserved | R    | 0h    |                                      |
| 7-6   | mmu      | R/W  | 0h    | System MMU initiator priority        |
| 5-4   | pru_icss | R/W  | 0h    | PRU-ICSS initiator priority          |
| 3-2   | Reserved | R    | 0h    |                                      |
| 1-0   | host_arm | R/W  | 0h    | Host Cortex A8 initiator priority    |

#### 9.3.1.18 init_priority_1 Register (offset = 60Ch) [reset = 0h]

init_priority_1 is described in Table 9-28.

**Table 9-28. init_priority_1 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                          |
|-------|----------|------|-------|--------------------------------------|
| 31-26 | Reserved | R    | 0h    |                                      |
| 25-24 | debug    | R/W  | 0h    | Debug Subsystem initiator priority   |
| 23-22 | lcd      | R/W  | 0h    | LCD initiator priority               |
| 21-20 | sgx      | R/W  | 0h    | SGX initiator priority               |
| 19-18 | Reserved | R    | 0h    |                                      |
| 17-16 | Reserved | R    | 0h    |                                      |
| 15-8  | Reserved | R    | 0h    |                                      |
| 7-6   | usb_qmgr | R/W  | 0h    | USB Queue Manager initiator priority |
| 5-4   | usb_dma  | R/W  | 0h    | USB DMA port initiator priority      |
| 3-2   | Reserved | R    | 0h    |                                      |
| 1-0   | cpsw     | R/W  | 0h    | CPSW initiator priority              |

#### 9.3.1.19 tptc_cfg Register (offset = 614h) [reset = 0h]

tptc_cfg is described in Table 9-29.

**Table 9-29. tptc_cfg Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                           |
|------|----------|------|-------|---------------------------------------------------------------------------------------|
| 31-6 | Reserved | R    | 0h    |                                                                                       |
| 5-4  | tc2dbs   | R/W  | 0h    | TPTC2 Default Burst Size<br>00: 16 byte<br>01: 32 byte<br>10: 64 byte<br>11: 128 byte |
| 3-2  | tc1dbs   | R/W  | 0h    | TPTC1 Default Burst Size<br>00: 16 byte<br>01: 32 byte<br>10: 64 byte<br>11: 128 byte |
| 1-0  | tc0dbs   | R/W  | 0h    | TPTC0 Default Burst Size<br>00: 16 byte<br>01: 32 byte<br>10: 64 byte<br>11: 128 byte |

#### 9.3.1.20 usb_ctrl0 Register (offset = 620h) [reset = 0h]

usb_ctrl0 is described in Table 9-30.

**Table 9-30. usb_ctrl0 Register Field Descriptions**

| Bit   | Field            | Type | Reset | Description                                                                                                                     |
|-------|------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | Reserved         | R/W  | 3Ch   | Reserved. Any writes to this register must keep these bits set to<br>0x3C.                                                      |
| 23    | datapolarity_inv | R/W  | 0h    | Data Polarity Invert:<br>0: DP/DM (normal polarity matching port definition)<br>1: DM/DP (inverted polarity of port definition) |
| 22    | Reserved         | R    | 0h    |                                                                                                                                 |
| 21    | Reserved         | R    | 0h    |                                                                                                                                 |
| 20    | otgsessenden     | R/W  | 0h    | Session End Detect Enable<br>0: Disable Session End Comparator<br>1: Turns on Session End Comparator                            |
| 19    | otgvdet_en       | R/W  | 0h    | VBUS Detect Enable<br>0: Disable VBUS Detect Enable<br>1: Turns on all comparators except Session End comparator                |
| 18    | dmgpio_pd        | R/W  | 0h    | Pulldown on DM in GPIO Mode<br>0: Enables pulldown<br>1: Disables pulldown                                                      |
| 17    | dpgpio_pd        | R/W  | 0h    | Pulldown on DP in GPIO Mode<br>0: Enables pulldown<br>1: Disables pulldown                                                      |
| 16    | Reserved         | R/W  | 0h    |                                                                                                                                 |
| 15    | Reserved         | R/W  | 0h    |                                                                                                                                 |
| 14    | gpio_sig_cross   | R/W  | 0h    | UART TX -> DM<br>UART RX -> DP                                                                                                  |
| 13    | gpio_sig_inv     | R/W  | 0h    | UART TX -> Invert -> DP<br>UART RX -> Invert -> DM                                                                              |
| 12    | gpiomode         | R/W  | 0h    | GPIO Mode<br>0: USB Mode<br>1: GPIO Mode (UART Mode)                                                                            |
| 11    | Reserved         | R/W  | 0h    |                                                                                                                                 |
| 10    | cdet_extctl      | R/W  | 0h    | Bypass the charger detection state machine<br>0: Charger detection on<br>1: Charger detection is bypassed                       |
| 9     | dppullup         | R/W  | 0h    | Pull-up on DP line<br>0: No effect<br>1: Enable pull-up on DP line                                                              |

**Table 9-30. usb_ctrl0 Register Field Descriptions (continued)**

| Bit | Field        | Type | Reset | Description                                                                |
|-----|--------------|------|-------|----------------------------------------------------------------------------|
| 8   | dmpulldn     | R/W  | 0h    | Pull-down on DM line<br>0: No effect<br>1: Enable pull-down on DM line     |
| 7   | chgvsrc_en   | R/W  | 0h    | Enable VSRC on DP line (Host Charger case)                                 |
| 6   | chgisink_en  | R/W  | 0h    | Enable ISINK on DM line (Host Charger case)                                |
| 5   | sinkondp     | R/W  | 0h    | Sink on DP<br>0: Sink on DM<br>1: Sink on DP                               |
| 4   | srcondm      | R/W  | 0h    | Source on DM<br>0: Source on DP<br>1: Source on DM                         |
| 3   | chgdet_rstrt | R/W  | 0h    | Restart Charger Detect                                                     |
| 2   | chgdet_dis   | R/W  | 0h    | Charger Detect Disable<br>0: Enable<br>1: Disable                          |
| 1   | otg_pwrdn    | R/W  | 0h    | Power down the USB OTG PHY<br>0: PHY in normal mode<br>1: PHY Powered down |
| 0   | cm_pwrdn     | R/W  | 0h    | Power down the USB CM PHY<br>0: PHY in normal mode<br>1: PHY Powered down  |

#### 9.3.1.21 usb_sts0 Register (offset = 624h) [reset = 0h]

usb_sts0 is described in Table 9-31.

**Table 9-31. usb_sts0 Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|------|--------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved     | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 7-5  | chgdetsts    | R    | 0h    | Charge Detection Status<br>000: Wait State (When a D+WPU and D-15K are connected, it<br>enters into this state and will remain in this state unless it enters into<br>other state)<br>001: No Contact<br>010: PS/2<br>011: Unknown error<br>100: Dedicated charger(valid if CE is HIGH)<br>101: HOST charger (valid if CE is HIGH)<br>110: PC<br>111: Interrupt (if any of the pullup is enabled, charger detect routine<br>gets interrupted and will restart from the beginning if the same is<br>disabled) |
| 4    | cdet_dmdet   | R    | 0h    | DM Comparator Output                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 3    | cdet_dpdet   | R    | 0h    | DP Comparator Output                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 2    | cdet_datadet | R    | 0h    | Charger Comparator Output                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 1    | chgdetect    | R    | 0h    | Charger Detection Status<br>0: Charger was no detected<br>1: Charger was detected                                                                                                                                                                                                                                                                                                                                                                                                                            |
| 0    | chgdetdone   | R    | 0h    | Charger Detection Protocol Done                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |

#### 9.3.1.22 usb_ctrl1 Register (offset = 628h) [reset = 0h]

usb_ctrl1 is described in Table 9-32.

**Table 9-32. usb_ctrl1 Register Field Descriptions**

| Bit   | Field            | Type | Reset | Description                                                                                                                     |
|-------|------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | Reserved         | R/W  | 3Ch   | Reserved. Any writes to this register must keep these bits set to<br>0x3C.                                                      |
| 23    | datapolarity_inv | R/W  | 0h    | Data Polarity Invert:<br>0: DP/DM (normal polarity matching port definition)<br>1: DM/DP (inverted polarity of port definition) |
| 22    | Reserved         | R    | 0h    |                                                                                                                                 |
| 21    | Reserved         | R    | 0h    |                                                                                                                                 |
| 20    | otgsessenden     | R/W  | 0h    | Session End Detect Enable<br>0: Disable Session End Comparator<br>1: Turns on Session End Comparator                            |
| 19    | otgvdet_en       | R/W  | 0h    | VBUS Detect Enable<br>0: Disable VBUS Detect Enable<br>1: Turns on all comparators except Session End comparator                |
| 18    | dmgpio_pd        | R/W  | 0h    | Pulldown on DM in GPIO Mode<br>0: Enables pulldown<br>1: Disables pulldown                                                      |
| 17    | dpgpio_pd        | R/W  | 0h    | Pulldown on DP in GPIO Mode<br>0: Enables pulldown<br>1: Disables pulldown                                                      |
| 16    | Reserved         | R/W  | 0h    |                                                                                                                                 |
| 15    | Reserved         | R/W  | 0h    |                                                                                                                                 |
| 14    | gpio_sig_cross   | R/W  | 0h    | UART TX -> DM<br>UART RX -> DP                                                                                                  |
| 13    | gpio_sig_inv     | R/W  | 0h    | UART TX -> INV -> DP<br>UART RX -> INV -> DM                                                                                    |
| 12    | gpiomode         | R/W  | 0h    | GPIO Mode<br>0: USB Mode<br>1: GPIO Mode (UART)                                                                                 |
| 11    | Reserved         | R/W  | 0h    |                                                                                                                                 |
| 10    | cdet_extctl      | R/W  | 0h    | Bypass the charger detection state machine<br>0: Charger detection on<br>1: Charger detection is bypassed                       |
| 9     | dppullup         | R/W  | 0h    | Pull-up on DP line<br>0: No effect<br>1: Enable pull-up on DP line                                                              |

**Table 9-32. usb_ctrl1 Register Field Descriptions (continued)**

| Bit | Field        | Type | Reset                                                                            | Description                                                               |
|-----|--------------|------|----------------------------------------------------------------------------------|---------------------------------------------------------------------------|
| 8   | dmpulldn     | R/W  | 0h                                                                               | Pull-down on DM line<br>0: No effect<br>1: Enable pull-down on DM line    |
| 7   | chgvsrc_en   | R/W  | 0h                                                                               | Enable VSRC on DP line (Host Charger case)                                |
| 6   | chgisink_en  | R/W  | 0h                                                                               | Enable ISINK on DM line (Host Charger case)                               |
| 5   | sinkondp     | R/W  | 0h                                                                               | Sink on DP<br>0: Sink on DM<br>1: Sink on DP                              |
| 4   | srcondm      | R/W  | 0h<br>Source on DM<br>0: Source on DP<br>1: Source on DM                         |                                                                           |
| 3   | chgdet_rstrt | R/W  | 0h                                                                               | Restart Charger Detect                                                    |
| 2   | chgdet_dis   | R/W  | 0h<br>Charger Detect Disable<br>0: Enable<br>1: Disable                          |                                                                           |
| 1   | otg_pwrdn    | R/W  | 0h<br>Power down the USB OTG PHY<br>0: PHY in normal mode<br>1: PHY Powered down |                                                                           |
| 0   | cm_pwrdn     | R/W  | 0h                                                                               | Power down the USB CM PHY<br>1: PHY Powered down<br>0: PHY in normal mode |

#### 9.3.1.23 usb_sts1 Register (offset = 62Ch) [reset = 0h]

usb_sts1 is described in Table 9-33.

**Table 9-33. usb_sts1 Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|------|--------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved     | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 7-5  | chgdetsts    | R    | 0h    | Charge Detection Status<br>000: Wait State (When a D+WPU and D-15K are connected, it<br>enters into this state and will remain in this state unless it enters into<br>other state)<br>001: No Contact<br>010: PS/2<br>011: Unknown error<br>100: Dedicated charger(valid if CE is HIGH)<br>101: HOST charger (valid if CE is HIGH)<br>110: PC<br>111: Interrupt (if any of the pullup is enabled, charger detect routine<br>gets interrupted and will restart from the beginning if the same is<br>disabled) |
| 4    | cdet_dmdet   | R    | 0h    | DM Comparator Output                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 3    | cdet_dpdet   | R    | 0h    | DP Comparator Output                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 2    | cdet_datadet | R    | 0h    | Charger Comparator Output                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 1    | chgdetect    | R    | 0h    | Charger Detection Status<br>0: Charger was no detected<br>1: Charger was detected                                                                                                                                                                                                                                                                                                                                                                                                                            |
| 0    | chgdetdone   | R    | 0h    | Charger Detection Protocol Done                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |

#### 9.3.1.24 mac_id0_lo Register (offset = 630h) [reset = 0h]

mac_id0_lo is described in Table 9-34.

**Table 9-34. mac_id0_lo Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                               |
|-------|---------------|------|-------|-----------------------------------------------------------|
| 31-16 | Reserved      | R    | 0h    |                                                           |
| 15-8  | macaddr_47_40 | R    | 0h    | MAC0 Address - Byte 5<br>Reset value is device-dependent. |
| 7-0   | macaddr_39_32 | R    | 0h    | MAC0 Address - Byte 4<br>Reset value is device-dependent. |

#### 9.3.1.25 mac_id0_hi Register (offset = 634h) [reset = 0h]

mac_id0_hi is described in Table 9-35.

**Table 9-35. mac_id0_hi Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                               |
|-------|---------------|------|-------|-----------------------------------------------------------|
| 31-24 | macaddr_31_24 | R    | 0h    | MAC0 Address - Byte 3<br>Reset value is device-dependent. |
| 23-16 | macaddr_23_16 | R    | 0h    | MAC0 Address - Byte 2<br>Reset value is device-dependent. |
| 15-8  | macaddr_15_8  | R    | 0h    | MAC0 Address - Byte 1<br>Reset value is device-dependent. |
| 7-0   | macaddr_7_0   | R    | 0h    | MAC0 Address - Byte 0<br>Reset value is device-dependent. |

#### 9.3.1.26 mac_id1_lo Register (offset = 638h) [reset = 0h]

mac_id1_lo is described in Table 9-36.

**Table 9-36. mac_id1_lo Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                               |
|-------|---------------|------|-------|-----------------------------------------------------------|
| 31-16 | Reserved      | R    | 0h    |                                                           |
| 15-8  | macaddr_47_40 | R    | 0h    | MAC1 Address - Byte 5<br>Reset value is device-dependent. |
| 7-0   | macaddr_39_32 | R    | 0h    | MAC1 Address - Byte 4<br>Reset value is device-dependent. |

#### 9.3.1.27 mac_id1_hi Register (offset = 63Ch) [reset = 0h]

mac_id1_hi is described in Table 9-37.

**Table 9-37. mac_id1_hi Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                               |
|-------|---------------|------|-------|-----------------------------------------------------------|
| 31-24 | macaddr_31_24 | R    | 0h    | MAC1 Address - Byte 3<br>Reset value is device-dependent. |
| 23-16 | macaddr_23_16 | R    | 0h    | MAC1 Address - Byte 2<br>Reset value is device-dependent. |
| 15-8  | macaddr_15_8  | R    | 0h    | MAC1 Address - Byte 1<br>Reset value is device-dependent. |
| 7-0   | macaddr_7_0   | R    | 0h    | MAC1 Address - Byte 0<br>Reset value is device-dependent. |

#### 9.3.1.28 dcan_raminit Register (offset = 644h) [reset = 0h]

dcan_raminit is described in Table 9-38.

**Table 9-38. dcan_raminit Register Field Descriptions**

| Bit   | Field               | Type      | Reset | Description                                                                      |
|-------|---------------------|-----------|-------|----------------------------------------------------------------------------------|
| 31-10 | Reserved            | R         | 0h    |                                                                                  |
| 9     | dcan1_raminit_done  | R/W1toClr | 0h    | 0: DCAN1 RAM Initialization NOT complete<br>1: DCAN1 RAM Initialization complete |
| 8     | dcan0_raminit_done  | R/W1toClr | 0h    | 0: DCAN0 RAM Initialization NOT complete<br>1: DCAN0 RAM Initialization complete |
| 7-2   | Reserved            | R         | 0h    |                                                                                  |
| 1     | dcan1_raminit_start | R/W       | 0h    | A transition from 0 to 1 will start DCAN1 RAM initialization sequence.           |
| 0     | dcan0_raminit_start | R/W       | 0h    | A transition from 0 to 1 will start DCAN0 RAM initialization sequence.           |

#### 9.3.1.29 usb_wkup_ctrl Register (offset = 648h) [reset = 0h]

usb_wkup_ctrl is described in Table 9-39.

**Table 9-39. usb_wkup_ctrl Register Field Descriptions**

| Bit  | Field     | Type | Reset | Description                                                  |
|------|-----------|------|-------|--------------------------------------------------------------|
| 31-9 | Reserved  | R    | 0h    |                                                              |
| 8    | phy1_wuen | R/W  | 0h    | PHY1 Wakeup Enable.<br>Write to 1 enables WKUP from USB PHY1 |
| 7-1  | Reserved  | R    | 0h    |                                                              |
| 0    | phy0_wuen | R/W  | 0h    | PHY0 Wakeup Enable.<br>Write to 1 enables WKUP from USB PHY0 |

#### 9.3.1.30 gmii_sel Register (offset = 650h) [reset = 0h]

gmii_sel is described in Table 9-40.

**Table 9-40. gmii_sel Register Field Descriptions**

| Bit  | Field           | Type | Reset | Description                                                                                                                                                                                                                                                                                                    |
|------|-----------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved        | R    | 0h    |                                                                                                                                                                                                                                                                                                                |
| 7    | rmii2_io_clk_en | R/W  | 1h    | 0: RMII Reference Clock Output mode. Enable RMII clock to be<br>sourced from PLL.<br>1: RMII Reference Clock Input mode. Enable RMII clock to be<br>sourced from chip pin.<br>See "Silicon Revision Functional Differences and Enhancements" for<br>differences in operation based on AM335x silicon revision. |
| 6    | rmii1_io_clk_en | R/W  | 1h    | 0: RMII Reference Clock Output mode. Enable RMII clock to be<br>sourced from PLL<br>1: RMII Reference Clock Input mode. Enable RMII clock to be<br>sourced from chip pin<br>See "Silicon Revision Functional Differences and Enhancements" for<br>differences in operation based on AM335x silicon revision.   |
| 5    | rgmii2_idmode   | R/W  | 1h    | RGMII2 Internal Delay Mode<br>0: Reserved<br>1: No Internal Delay<br>See "Silicon Revision Functional Differences and Enhancements" for<br>differences in operation based on AM335x silicon revision.                                                                                                          |
| 4    | rgmii1_idmode   | R/W  | 1h    | RGMII1 Internal Delay Mode<br>0: Reserved<br>1: No Internal Delay<br>See "Silicon Revision Functional Differences and Enhancements" for<br>differences in operation based on AM335x silicon revision.                                                                                                          |
| 3-2  | gmii2_sel       | R/W  | 0h    | 00: Port2 GMII/MII Mode<br>01: Port2 RMII Mode<br>10: Port2 RGMII Mode<br>11: Not Used                                                                                                                                                                                                                         |
| 1-0  | gmii1_sel       | R/W  | 0h    | 00: Port1 GMII/MII Mode<br>01: Port1 RMII Mode<br>10: Port1 RGMII Mode<br>11: Not Used                                                                                                                                                                                                                         |

#### 9.3.1.31 pwmss_ctrl Register (offset = 664h) [reset = 0h]

pwmss_ctrl is described in Table 9-41.

**Table 9-41. pwmss_ctrl Register Field Descriptions**

| Bit  | Field          | Type | Reset | Description                      |
|------|----------------|------|-------|----------------------------------|
| 31-3 | Reserved       | R    | 0h    |                                  |
| 2    | pwmss2_tbclken | R/W  | 0h    | Timebase clock enable for PWMSS2 |
| 1    | pwmss1_tbclken | R/W  | 0h    | Timebase clock enable for PWMSS1 |
| 0    | pwmss0_tbclken | R/W  | 0h    | Timebase clock enable for PWMSS0 |

#### 9.3.1.32 mreqprio_0 Register (offset = 670h) [reset = 0h]

mreqprio_0 is described in Table 9-42.

**Table 9-42. mreqprio_0 Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                            |
|-------|---------------|------|-------|--------------------------------------------------------|
| 31    | Reserved      | R    | 0h    |                                                        |
| 30-28 | sgx           | R/W  | 0h    | MReqPriority for SGX Initiator OCP Interface           |
| 27    | Reserved      | R    | 0h    |                                                        |
| 26-24 | usb1          | R/W  | 0h    | MReqPriority for USB1 Initiator OCP Interface          |
| 23    | Reserved      | R    | 0h    |                                                        |
| 22-20 | usb0          | R/W  | 0h    | MReqPriority for USB0 Initiator OCP Interface          |
| 19    | Reserved      | R    | 0h    |                                                        |
| 18-16 | cpsw          | R/W  | 0h    | MReqPriority for CPSW Initiator OCP Interface          |
| 15    | Reserved      | R    | 0h    |                                                        |
| 14-12 | Reserved      | R    | 0h    |                                                        |
| 11    | Reserved      | R    | 0h    |                                                        |
| 10-8  | pru_icss_pru0 | R/W  | 0h    | MReqPriority for PRU-ICSS PRU0 Initiator OCP Interface |
| 7     | Reserved      | R    | 0h    |                                                        |
| 6-4   | sab_init1     | R/W  | 0h    | MReqPriority for MPU Initiator 1 OCP Interface         |
| 3     | Reserved      | R    | 0h    |                                                        |
| 2-0   | sab_init0     | R/W  | 0h    | MReqPriority for MPU Initiator 0 OCP Interface         |

#### 9.3.1.33 mreqprio_1 Register (offset = 674h) [reset = 0h]

mreqprio_1 is described in Table 9-43.

**Table 9-43. mreqprio_1 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-0 | Reserved | R    | 0h    |             |

#### 9.3.1.34 hw_event_sel_grp1 Register (offset = 690h) [reset = 0h]

hw_event_sel_grp1 is described in Table 9-44.

**Table 9-44. hw_event_sel_grp1 Register Field Descriptions**

| Bit   | Field  | Type | Reset | Description                         |
|-------|--------|------|-------|-------------------------------------|
| 31-24 | event4 | R/W  | 0h    | Select 4th trace event from group 1 |
| 23-16 | event3 | R/W  | 0h    | Select 3rd trace event from group 1 |
| 15-8  | event2 | R/W  | 0h    | Select 2nd trace event from group 1 |
| 7-0   | event1 | R/W  | 0h    | Select 1st trace event from group 1 |

#### 9.3.1.35 hw_event_sel_grp2 Register (offset = 694h) [reset = 0h]

hw_event_sel_grp2 is described in Table 9-45.

**Table 9-45. hw_event_sel_grp2 Register Field Descriptions**

| Bit   | Field  | Type | Reset | Description                         |
|-------|--------|------|-------|-------------------------------------|
| 31-24 | event8 | R/W  | 0h    | Select 8th trace event from group 2 |
| 23-16 | event7 | R/W  | 0h    | Select 7th trace event from group 2 |
| 15-8  | event6 | R/W  | 0h    | Select 6th trace event from group 2 |
| 7-0   | event5 | R/W  | 0h    | Select 5th trace event from group 2 |

#### 9.3.1.36 hw_event_sel_grp3 Register (offset = 698h) [reset = 0h]

hw_event_sel_grp3 is described in Table 9-46.

**Table 9-46. hw_event_sel_grp3 Register Field Descriptions**

| Bit   | Field   | Type | Reset | Description                          |
|-------|---------|------|-------|--------------------------------------|
| 31-24 | event12 | R/W  | 0h    | Select 12th trace event from group 3 |
| 23-16 | event11 | R/W  | 0h    | Select 11th trace event from group 3 |
| 15-8  | event10 | R/W  | 0h    | Select 10th trace event from group 3 |
| 7-0   | event9  | R/W  | 0h    | Select 9th trace event from group 3  |

#### 9.3.1.37 hw_event_sel_grp4 Register (offset = 69Ch) [reset = 0h]

hw_event_sel_grp4 is described in Table 9-47.

**Table 9-47. hw_event_sel_grp4 Register Field Descriptions**

| Bit   | Field   | Type | Reset | Description                          |
|-------|---------|------|-------|--------------------------------------|
| 31-24 | event16 | R/W  | 0h    | Select 16th trace event from group 4 |
| 23-16 | event15 | R/W  | 0h    | Select 15th trace event from group 4 |
| 15-8  | event14 | R/W  | 0h    | Select 14th trace event from group 4 |
| 7-0   | event13 | R/W  | 0h    | Select 13th trace event from group 4 |

#### 9.3.1.38 smrt_ctrl Register (offset = 6A0h) [reset = 0h]

smrt_ctrl is described in Table 9-48.

**Table 9-48. smrt_ctrl Register Field Descriptions**

| Bit  | Field     | Type | Reset | Description                                                                                            |
|------|-----------|------|-------|--------------------------------------------------------------------------------------------------------|
| 31-2 | Reserved  | R    | 0h    |                                                                                                        |
| 1    | sr1_sleep | R/W  | 0h    | 0: Disable sensor (SRSLEEP on sensor driven to 1)<br>1: Enable sensor (SRSLEEP on sensor driven to 0). |
| 0    | sr0_sleep | R/W  | 0h    | 0: Disable sensor (SRSLEEP on sensor driven to 1)<br>1: Enable sensor (SRSLEEP on sensor driven to 0). |

#### 9.3.1.39 mpuss_hw_debug_sel Register (offset = 6A4h) [reset = 0h]

mpuss_hw_debug_sel is described in Table 9-49.

**Table 9-49. mpuss_hw_debug_sel Register Field Descriptions**

| Bit   | Field          | Type | Reset | Description                                                                                                                                                                                                                                                                                     |
|-------|----------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | Reserved       | R    | 0h    |                                                                                                                                                                                                                                                                                                 |
| 9     | hw_dbg_gate_en | R/W  | 0h    | To save power input to MPUSS_HW_DBG_INFO is gated off to all<br>zeros when HW_DBG_GATE_EN bit is low.<br>0: Debug info gated off<br>1: Debug info not gated off                                                                                                                                 |
| 8     | Reserved       | R    | 0h    |                                                                                                                                                                                                                                                                                                 |
| 7-4   | Reserved       | R    | 0h    |                                                                                                                                                                                                                                                                                                 |
| 3-0   | hw_dbg_sel     | R/W  | 0h    | Selects which Group of signals are sent out to the<br>MODENA_HW_DBG_INFO register. Please see MPU functional<br>spec for more details<br>0000: Group 0<br>0001: Group 1<br>0010: Group 2<br>0011: Group 3<br>0100: Group 4<br>0101: Group 5<br>0110: Group 6<br>0111: Group 7<br>1xxx: Reserved |

#### 9.3.1.40 mpuss_hw_dbg_info Register (offset = 6A8h) [reset = 0h]

mpuss_hw_dbg_info is described in Table 9-50.

**Table 9-50. mpuss_hw_dbg_info Register Field Descriptions**

| Bit  | Field       | Type | Reset | Description                   |
|------|-------------|------|-------|-------------------------------|
| 31-0 | hw_dbg_info | R    | 0h    | Hardware Debug Info from MPU. |

#### 9.3.1.41 vdd_mpu_opp_050 Register (offset = 770h) [reset = 0h]

vdd_mpu_opp_050 is described in Table 9-51.

**Table 9-51. vdd_mpu_opp_050 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                    |
|-------|----------|------|-------|--------------------------------------------------------------------------------|
| 31-24 | Reserved | R    | 0h    |                                                                                |
| 23-0  | ntarget  | R    | 0h    | Ntarget value for MPU Voltage domain OPP50<br>Reset value is device-dependent. |

#### 9.3.1.42 vdd_mpu_opp_100 Register (offset = 774h) [reset = 0h]

vdd_mpu_opp_100 is described in Table 9-52.

**Table 9-52. vdd_mpu_opp_100 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                     |
|-------|----------|------|-------|---------------------------------------------------------------------------------|
| 31-24 | Reserved | R    | 0h    |                                                                                 |
| 23-0  | ntarget  | R    | 0h    | Ntarget value for MPU Voltage domain OPP100<br>Reset value is device-dependent. |

#### 9.3.1.43 vdd_mpu_opp_120 Register (offset = 778h) [reset = 0h]

vdd_mpu_opp_120 is described in Table 9-53.

**Table 9-53. vdd_mpu_opp_120 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                     |
|-------|----------|------|-------|---------------------------------------------------------------------------------|
| 31-24 | Reserved | R    | 0h    |                                                                                 |
| 23-0  | ntarget  | R    | 0h    | Ntarget value for MPU Voltage domain OPP120<br>Reset value is device-dependent. |

#### 9.3.1.44 vdd_mpu_opp_turbo Register (offset = 77Ch) [reset = 0h]

vdd_mpu_opp_turbo is described in Table 9-54.

**Table 9-54. vdd_mpu_opp_turbo Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                       |
|-------|----------|------|-------|-----------------------------------------------------------------------------------|
| 31-24 | Reserved | R    | 0h    |                                                                                   |
| 23-0  | ntarget  | R    | 0h    | Ntarget value for MPU Voltage domain OPPTURBO<br>Reset value is device-dependent. |

#### 9.3.1.45 vdd_core_opp_050 Register (offset = 7B8h) [reset = 0h]

vdd_core_opp_050 is described in Table 9-55.

**Table 9-55. vdd_core_opp_050 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                     |
|-------|----------|------|-------|---------------------------------------------------------------------------------|
| 31-24 | Reserved | R    | 0h    |                                                                                 |
| 23-0  | ntarget  | R    | 0h    | Ntarget value for CORE Voltage domain OPP50<br>Reset value is device-dependent. |

#### 9.3.1.46 vdd_core_opp_100 Register (offset = 7BCh) [reset = 0h]

vdd_core_opp_100 is described in Table 9-56.

**Table 9-56. vdd_core_opp_100 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                      |
|-------|----------|------|-------|----------------------------------------------------------------------------------|
| 31-24 | Reserved | R    | 0h    |                                                                                  |
| 23-0  | ntarget  | R    | 0h    | Ntarget value for CORE Voltage domain OPP100<br>Reset value is device-dependent. |

#### 9.3.1.47 bb_scale Register (offset = 7D0h) [reset = 0h]

bb_scale is described in Table 9-57.

**Table 9-57. bb_scale Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                              |
|-------|----------|------|-------|------------------------------------------|
| 31-12 | Reserved | R    | 0h    |                                          |
| 11-8  | scale    | R    | 0h    | Dynamic core voltage scaling for class 0 |
| 7-2   | Reserved | R    | 0h    |                                          |
| 1-0   | bbias    | R    | 0h    | BBIAS value from Efuse                   |

#### 9.3.1.48 usb_vid_pid Register (offset = 7F4h) [reset = 4516141h]

usb_vid_pid is described in Table 9-58.

**Table 9-58. usb_vid_pid Register Field Descriptions**

| Bit   | Field   | Type | Reset  | Description    |
|-------|---------|------|--------|----------------|
| 31-16 | usb_vid | R    | 0x451  | USB Vendor ID  |
| 15-0  | usb_pid | R    | 0x6141 | USB Product ID |

#### 9.3.1.49 efuse_sma Register (offset = 7FCh) [reset = 0h]

This register describes the device's ARM maximum frequency capabilities and package type. Note that this register is only applicable in PG2.x. The contents of this register is not applicable in PG1.0 devices.

efuse_sma is described in Table 9-59.

**Table 9-59. efuse_sma Register Field Descriptions**

| Bit   | Field            | Type | Reset                | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
|-------|------------------|------|----------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-18 | Reserved         | R    |                      | These bits are undefined and contents can vary from device to<br>device.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 17-16 | package_type     | R    | Package<br>dependent | Designates the Package type of the device (PG2.x only).<br>00b - Undefined<br>01b - ZCZ Package<br>10b - ZCE Package<br>11b - Reserved                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 15-13 | Reserved         | R    |                      | These bits are undefined and contents can vary from device to<br>device.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 12-0  | arm_mpu_max_freq | R    | Device<br>dependent  | Designates the ARM MPU Maximum Frequency supported by the<br>device (PG2.x only).<br>There are also voltage requirements that accompany each frequency<br>(OPPs).<br>See the device specific data manual for this information and for<br>information on device variants.<br>0x1FEF - 300 MHz ARM MPU Maximum (ZCZ Package only)<br>0x1FAF - 600 MHz ARM MPU Maximum (ZCZ Package only)<br>0x1F2F - 720 MHz ARM MPU Maximum (ZCZ Package only)<br>0x1E2F - 800 MHz ARM MPU Maximum (ZCZ Package only)<br>0x1C2F - 1 GHz ARM MPU Maximum (ZCZ Package only)<br>0x1FDF - 300 MHz ARM MPU Maximum (ZCE Package only)<br>0x1F9F - 600 MHz ARM MPU Maximum (ZCE Package only)<br>All other values are reserved. |

#### 9.3.1.50 conf_<module>_<pin> Register (offset = 800h–A34h)

See the device datasheet for information on default pin mux configurations. Note that the device ROM may change the default pin mux for certain pins based on the SYSBOOT mode settings.

See Table 9-10, *Control Module Registers Table*, for the full list of offsets for each module/pin configuration.

conf_<module>_<pin> is described in Table 9-60.

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 9-60. conf_<module>_<pin> Register Field Descriptions**

| Bit   | Field                                              | Type | Reset | Description                                                                                                              |
|-------|----------------------------------------------------|------|-------|--------------------------------------------------------------------------------------------------------------------------|
| 31-20 | Reserved                                           | R    | 0h    |                                                                                                                          |
| 19-7  | Reserved                                           | R    | 0h    |                                                                                                                          |
| 6     | conf_ <module>_<pin>_sle<br/>wctrl</pin></module>  | R/W  | X     | Select between faster or slower slew rate<br>0: Fast<br>1: Slow<br>Reset value is pad-dependent.                         |
| 5     | conf_ <module>_<pin>_rx<br/>active</pin></module>  | R/W  | 1h    | Input enable value for the PAD<br>0: Receiver disabled<br>1: Receiver enabled                                            |
| 4     | conf_ <module>_<pin>_pu<br/>typesel</pin></module> | R/W  | X     | Pad pullup/pulldown type selection<br>0: Pulldown selected<br>1: Pullup selected<br>Reset value is pad-dependent.        |
| 3     | conf_ <module>_<pin>_pu<br/>den</pin></module>     | R/W  | X     | Pad pullup/pulldown enable<br>0: Pullup/pulldown enabled<br>1: Pullup/pulldown disabled<br>Reset value is pad-dependent. |
| 2-0   | conf_ <module>_<pin>_m<br/>mode</pin></module>     | R/W  | X     | Pad functional signal mux select.<br>Reset value is pad-dependent.                                                       |

#### 9.3.1.51 cqdetect_status Register (offset = E00h) [reset = 0h]

cqdetect_status is described in Table 9-61.

**Table 9-61. cqdetect_status Register Field Descriptions**

| Bit   | Field          | Type | Reset | Description                                  |
|-------|----------------|------|-------|----------------------------------------------|
| 31-22 | Reserved       | R    | 0h    |                                              |
| 21    | Reserved       | R    | 0h    |                                              |
| 20    | Reserved       | R    | 0h    |                                              |
| 19    | Reserved       | R    | 0h    |                                              |
| 18    | Reserved       | R    | 0h    |                                              |
| 17    | Reserved       | R    | 0h    |                                              |
| 16    | Reserved       | R    | 0h    |                                              |
| 15-14 | Reserved       | R    | 0h    |                                              |
| 13    | cqerr_general  | R    | 0h    | CQDetect Mode Error Status                   |
| 12    | cqerr_gemac_b  | R    | 0h    | CQDetect Mode Error Status                   |
| 11    | cqerr_gemac_a  | R    | 0h    | CQDetect Mode Error Status                   |
| 10    | cqerr_mmcsd_b  | R    | 0h    | CQDetect Mode Error Status                   |
| 9     | cqerr_mmcsd_a  | R    | 0h    | CQDetect Mode Error Status                   |
| 8     | cqerr_gpmc     | R    | 0h    | CQDetect Mode Error Status                   |
| 7-6   | Reserved       | R    | 0h    |                                              |
| 5     | cqstat_general | R    | 0h    | 1: IOs are 3.3V mode<br>0: IOs are 1.8V mode |
| 4     | cqstat_gemac_b | R    | 0h    | 1: IOs are 3.3V mode<br>0: IOs are 1.8V mode |
| 3     | cqstat_gemac_a | R    | 0h    | 1: IOs are 3.3V mode<br>0: IOs are 1.8V mode |
| 2     | cqstat_mmcsd_b | R    | 0h    | 1: IOs are 3.3V mode<br>0: IOs are 1.8V mode |
| 1     | cqstat_mmcsd_a | R    | 0h    | 1: IOs are 3.3V mode<br>0: IOs are 1.8V mode |
| 0     | cqstat_gpmc    | R    | 0h    | 1: IOs are 3.3V mode<br>0: IOs are 1.8V mode |

#### 9.3.1.52 ddr_io_ctrl Register (offset = E04h) [reset = 0h]

ddr_io_ctrl is described in Table 9-62.

**Table 9-62. ddr_io_ctrl Register Field Descriptions**

| Bit  | Field             | Type | Reset | Description                                                                                                                                                                                                                      |
|------|-------------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31   | ddr3_rst_def_val  | R/W  | 0h    | DDR3 reset default value                                                                                                                                                                                                         |
| 30   | ddr_wuclk_disable | R/W  | 0h    | Disables the slow clock to WUCLKIN and ISOCLKIN of DDR emif<br>SS and IOs (required for proper initialization, after which clock could<br>be shut off).<br>0 = free running SLOW (32k) clock<br>1 = clock is synchronously gated |
| 29   | Reserved          | R    | 0h    |                                                                                                                                                                                                                                  |
| 28   | mddr_sel          | R/W  | 0h    | 0: IOs set for DDR2/DDR3 (STL mode)<br>1: IOs set for mDDR (CMOS mode)                                                                                                                                                           |
| 27-0 | Reserved          | R/W  | 0h    |                                                                                                                                                                                                                                  |

#### 9.3.1.53 vtp_ctrl Register (offset = E0Ch) [reset = 0h]

vtp_ctrl is described in Table 9-63.

**Table 9-63. vtp_ctrl Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-23 | Reserved | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 22-16 | pcin     | R    | 1h    | Default/reset values of 'P' for the VTP controller.<br>See "Silicon Revision Functional Differences and Enhancements" for<br>differences in operation based on AM335x silicon revision.                                                                                                                                                                                                                                                                                            |
| 15    | Reserved | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 14-8  | ncin     | R    | 1h    | Default/reset values of 'N' for the VTP controller.<br>See "Silicon Revision Functional Differences and Enhancements" for<br>differences in operation based on AM335x silicon revision.                                                                                                                                                                                                                                                                                            |
| 7     | Reserved | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 6     | enable   | R/W  | 0h    | 0: VTP macro in bypass mode. P and N are driven from PCIN and<br>NCIN.<br>1: Dynamic VTP compensation mode                                                                                                                                                                                                                                                                                                                                                                         |
| 5     | ready    | R    | 0h    | 0: Training sequence is not complete<br>1: Training sequence is complete                                                                                                                                                                                                                                                                                                                                                                                                           |
| 4     | lock     | R/W  | 0h    | 0: Normal operation dynamic update<br>1: freeze dynamic update, pwrdn controller                                                                                                                                                                                                                                                                                                                                                                                                   |
| 3-1   | filter   | R/W  | 11h   | Digital filter bits to prevent the controller from making excessive<br>number of changes.<br>000: Filter off<br>001: Update on two consecutive update requests<br>010: Update on three consecutive update requests<br>011: Update on four consecutive update requests<br>100: Update on five consecutive update requests<br>101: Update on six consecutive update requests<br>110: Update on seven consecutive update requests<br>111: Update on eight consecutive update requests |
| 0     | clrz     | R/W  | 0h    | clears flops, start count again, after low going pulse                                                                                                                                                                                                                                                                                                                                                                                                                             |

#### 9.3.1.54 vref_ctrl Register (offset = E14h) [reset = 0h]

vref_ctrl is described in Table 9-64.

**Table 9-64. vref_ctrl Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                    |
|------|---------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-5 | Reserved      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                |
| 4-3  | ddr_vref_ccap | R/W  | 0h    | select for coupling cap for DDR<br>00 : No capacitor connected<br>01 : Capacitor between BIAS2 and VSS<br>10 : Capacitor between BIAS2 and VDDS<br>11: Capacitor between BIAS2 and VSS andamp<br>Capacitor between BIAS2 and VDDS                                                                                                                              |
| 2-1  | ddr_vref_tap  | R/W  | 0h    | select for int ref for DDR<br>00 : Pad/Bias2 connected to internal reference VDDS/2 for 2uA<br>current load<br>01 : Pad/Bias2 connected to internal reference VDDS/2 for 4uA<br>current load<br>10 : Pad/Bias2 connected to internal reference VDDS/2 for 6uA<br>current load<br>11 : Pad/Bias2 connected to internal reference VDDS/2 for 8uA<br>current load |
| 0    | ddr_vref_en   | R/W  | 0h    | active high internal reference enable for DDR                                                                                                                                                                                                                                                                                                                  |

#### 9.3.1.55 tpcc_evt_mux_0_3 Register (offset = F90h) [reset = 0h]

tpcc_evt_mux_0_3 is described in Table 9-65.

**Table 9-65. tpcc_evt_mux_0_3 Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description                            |
|-------|-----------|------|-------|----------------------------------------|
| 31-30 | Reserved  | R    | 0h    |                                        |
| 29-24 | evt_mux_3 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 3 |
| 23-22 | Reserved  | R    | 0h    |                                        |
| 21-16 | evt_mux_2 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 2 |
| 15-14 | Reserved  | R    | 0h    |                                        |
| 13-8  | evt_mux_1 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 1 |
| 7-6   | Reserved  | R    | 0h    |                                        |
| 5-0   | evt_mux_0 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 0 |

#### 9.3.1.56 tpcc_evt_mux_4_7 Register (offset = F94h) [reset = 0h]

tpcc_evt_mux_4_7 is described in Table 9-66.

**Table 9-66. tpcc_evt_mux_4_7 Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description                            |
|-------|-----------|------|-------|----------------------------------------|
| 31-30 | Reserved  | R    | 0h    |                                        |
| 29-24 | evt_mux_7 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 7 |
| 23-22 | Reserved  | R    | 0h    |                                        |
| 21-16 | evt_mux_6 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 6 |
| 15-14 | Reserved  | R    | 0h    |                                        |
| 13-8  | evt_mux_5 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 5 |
| 7-6   | Reserved  | R    | 0h    |                                        |
| 5-0   | evt_mux_4 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 4 |

#### 9.3.1.57 tpcc_evt_mux_8_11 Register (offset = F98h) [reset = 0h]

tpcc_evt_mux_8_11 is described in Table 9-67.

**Table 9-67. tpcc_evt_mux_8_11 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_11 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 11 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_10 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 10 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_9  | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 9  |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_8  | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 8  |

#### 9.3.1.58 tpcc_evt_mux_12_15 Register (offset = F9Ch) [reset = 0h]

tpcc_evt_mux_12_15 is described in Table 9-68.

**Table 9-68. tpcc_evt_mux_12_15 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_15 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 15 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_14 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 14 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_13 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 13 |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_12 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 12 |

#### 9.3.1.59 tpcc_evt_mux_16_19 Register (offset = FA0h) [reset = 0h]

tpcc_evt_mux_16_19 is described in Table 9-69.

**Table 9-69. tpcc_evt_mux_16_19 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_19 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 19 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_18 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 18 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_17 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 17 |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_16 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 16 |

#### 9.3.1.60 tpcc_evt_mux_20_23 Register (offset = FA4h) [reset = 0h]

tpcc_evt_mux_20_23 is described in Table 9-70.

**Table 9-70. tpcc_evt_mux_20_23 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_23 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 23 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_22 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 22 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_21 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 21 |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_20 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 20 |

#### 9.3.1.61 tpcc_evt_mux_24_27 Register (offset = FA8h) [reset = 0h]

tpcc_evt_mux_24_27 is described in Table 9-71.

**Table 9-71. tpcc_evt_mux_24_27 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_27 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 27 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_26 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 26 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_25 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 25 |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_24 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 24 |

#### 9.3.1.62 tpcc_evt_mux_28_31 Register (offset = FACh) [reset = 0h]

tpcc_evt_mux_28_31 is described in Table 9-72.

**Table 9-72. tpcc_evt_mux_28_31 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_31 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 31 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_30 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 30 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_29 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 29 |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_28 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 28 |

#### 9.3.1.63 tpcc_evt_mux_32_35 Register (offset = FB0h) [reset = 0h]

tpcc_evt_mux_32_35 is described in Table 9-73.

**Table 9-73. tpcc_evt_mux_32_35 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_35 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 35 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_34 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 34 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_33 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 33 |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_32 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 32 |

#### 9.3.1.64 tpcc_evt_mux_36_39 Register (offset = FB4h) [reset = 0h]

tpcc_evt_mux_36_39 is described in Table 9-74.

**Table 9-74. tpcc_evt_mux_36_39 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_39 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 39 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_38 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 38 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_37 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 37 |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_36 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 36 |

#### 9.3.1.65 tpcc_evt_mux_40_43 Register (offset = FB8h) [reset = 0h]

tpcc_evt_mux_40_43 is described in Table 9-75.

**Table 9-75. tpcc_evt_mux_40_43 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_43 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 43 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_42 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 42 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_41 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 41 |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_40 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 40 |

#### 9.3.1.66 tpcc_evt_mux_44_47 Register (offset = FBCh) [reset = 0h]

tpcc_evt_mux_44_47 is described in Table 9-76.

**Table 9-76. tpcc_evt_mux_44_47 Register Field Descriptions**

| Bit   | Field      | Type | Reset                                         | Description                             |
|-------|------------|------|-----------------------------------------------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h                                            |                                         |
| 29-24 | evt_mux_47 | R/W  | 0h<br>Selects 1 of 64 inputs for DMA event 47 |                                         |
| 23-22 | Reserved   | R    | 0h                                            |                                         |
| 21-16 | evt_mux_46 | R/W  | 0h                                            | Selects 1 of 64 inputs for DMA event 46 |
| 15-14 | Reserved   | R    | 0h                                            |                                         |
| 13-8  | evt_mux_45 | R/W  | 0h                                            | Selects 1 of 64 inputs for DMA event 45 |
| 7-6   | Reserved   | R    | 0h                                            |                                         |
| 5-0   | evt_mux_44 | R/W  | 0h                                            | Selects 1 of 64 inputs for DMA event 44 |

#### 9.3.1.67 tpcc_evt_mux_48_51 Register (offset = FC0h) [reset = 0h]

tpcc_evt_mux_48_51 is described in Table 9-77.

**Table 9-77. tpcc_evt_mux_48_51 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_51 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 51 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_50 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 50 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_49 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 49 |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_48 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 48 |

#### 9.3.1.68 tpcc_evt_mux_52_55 Register (offset = FC4h) [reset = 0h]

tpcc_evt_mux_52_55 is described in Table 9-78.

**Table 9-78. tpcc_evt_mux_52_55 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_55 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 55 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_54 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 54 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_53 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 53 |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_52 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 52 |

#### 9.3.1.69 tpcc_evt_mux_56_59 Register (offset = FC8h) [reset = 0h]

tpcc_evt_mux_56_59 is described in Table 9-79.

**Table 9-79. tpcc_evt_mux_56_59 Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                             |
|-------|------------|------|-------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h    |                                         |
| 29-24 | evt_mux_59 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 59 |
| 23-22 | Reserved   | R    | 0h    |                                         |
| 21-16 | evt_mux_58 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 58 |
| 15-14 | Reserved   | R    | 0h    |                                         |
| 13-8  | evt_mux_57 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 57 |
| 7-6   | Reserved   | R    | 0h    |                                         |
| 5-0   | evt_mux_56 | R/W  | 0h    | Selects 1 of 64 inputs for DMA event 56 |

#### 9.3.1.70 tpcc_evt_mux_60_63 Register (offset = FCCh) [reset = 0h]

tpcc_evt_mux_60_63 is described in Table 9-80.

**Table 9-80. tpcc_evt_mux_60_63 Register Field Descriptions**

| Bit   | Field      | Type | Reset                                         | Description                             |
|-------|------------|------|-----------------------------------------------|-----------------------------------------|
| 31-30 | Reserved   | R    | 0h                                            |                                         |
| 29-24 | evt_mux_63 | R/W  | 0h<br>Selects 1 of 64 inputs for DMA event 63 |                                         |
| 23-22 | Reserved   | R    | 0h                                            |                                         |
| 21-16 | evt_mux_62 | R/W  | 0h                                            | Selects 1 of 64 inputs for DMA event 62 |
| 15-14 | Reserved   | R    | 0h                                            |                                         |
| 13-8  | evt_mux_61 | R/W  | 0h                                            | Selects 1 of 64 inputs for DMA event 61 |
| 7-6   | Reserved   | R    | 0h                                            |                                         |
| 5-0   | evt_mux_60 | R/W  | 0h                                            | Selects 1 of 64 inputs for DMA event 60 |

#### 9.3.1.71 timer_evt_capt Register (offset = FD0h) [reset = 0h]

timer_evt_capt is described in Table 9-81.

**Table 9-81. timer_evt_capt Register Field Descriptions**

| Bit   | Field          | Type | Reset                           | Description               |
|-------|----------------|------|---------------------------------|---------------------------|
| 31-21 | Reserved       | R    | 0h                              |                           |
| 20-16 | timer7_evtcapt | R/W  | 0h<br>Timer 7 event capture mux |                           |
| 15-13 | Reserved       | R    | 0h                              |                           |
| 12-8  | timer6_evtcapt | R/W  | 0h                              | Timer 6 event capture mux |
| 7-5   | Reserved       | R    | 0h                              |                           |
| 4-0   | timer5_evtcapt | R/W  | 0h                              | Timer 5 event capture mux |

#### 9.3.1.72 ecap_evt_capt Register (offset = FD4h) [reset = 0h]

ecap_evt_capt is described in Table 9-82.

**Table 9-82. ecap_evt_capt Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description             |
|-------|---------------|------|-------|-------------------------|
| 31-21 | Reserved      | R    | 0h    |                         |
| 20-16 | ecap2_evtcapt | R/W  | 0h    | ECAP2 event capture mux |
| 15-13 | Reserved      | R    | 0h    |                         |
| 12-8  | ecap1_evtcapt | R/W  | 0h    | ECAP1 event capture mux |
| 7-5   | Reserved      | R    | 0h    |                         |
| 4-0   | ecap0_evtcapt | R/W  | 0h    | ECAP0 event capture mux |

#### 9.3.1.73 adc_evt_capt Register (offset = FD8h) [reset = 0h]

adc_evt_capt is described in Table 9-83.

**Table 9-83. adc_evt_capt Register Field Descriptions**

| Bit  | Field       | Type | Reset | Description             |
|------|-------------|------|-------|-------------------------|
| 31-4 | Reserved    | R    | 0h    |                         |
| 3-0  | adc_evtcapt | R/W  | 0h    | ECAP0 event capture mux |

#### 9.3.1.74 reset_iso Register (offset = 1000h) [reset = 0h]

reset_iso is described in Table 9-84.

**Table 9-84. reset_iso Register Field Descriptions**

| Bit  | Field       | Type | Reset | Description                                                            |
|------|-------------|------|-------|------------------------------------------------------------------------|
| 31-1 | Reserved    | R    | 0h    |                                                                        |
| 0    | iso_control | R/W  | 0h    | 0 : Ethernet Switch is not isolated<br>1 : Ethernet Switch is isolated |

#### 9.3.1.75 dpll_pwr_sw_ctrl Register (offset = 1318h) [reset = 0h]

The DPLL_PWR_SW_CTRL register, in conjunction with the DPLL_PWR_SW_STATUS register, can be used to power off the digital power domain of the 3 DPLLS – DDR, DISP, PER to save leakage power in deep-sleep power modes. This register gives control over the power switch signals of the individual DPLLS.

A specific sequence has to be followed while programming the RET, PONIN, PGOODIN, ISO and RESET signals to put the PLLs in to low power mode and bring it out of low power mode.

In normal operating mode, the PRCM controls the RESET of the DPLLS. The RET, PONIN, PGOODIN and ISO are tied off. An over-ride bit is provided in this register SW_CTRL_\*_RESET, which when set allows S/W to control the RESET, RET, PONIN, PGOODIN and ISO of the DPLLs to enable entry/exit into DPLL low power modes.

dpll_pwr_sw_ctrl is described in Table 9-85.

**Table 9-85. dpll_pwr_sw_ctrl Register Field Descriptions**

| Bit | Field            | Type | Reset | Description                                                                                                                                                                                                                            |
|-----|------------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31  | sw_ctrl_ddr_pll  | R/W  | 0h    | Enable software control over DDR DPLL RET, RESET, ISO,<br>PGOODIN, PONIN for power savings.<br>0: PRCM controls the DPLL reset, RET = 0, ISO = 0, PGOODIN = 1,<br>PONIN = 1.                                                           |
|     |                  |      |       | 1: Controlled by corresponding bits in this register.                                                                                                                                                                                  |
| 30  | Reserved         | R    | 0h    |                                                                                                                                                                                                                                        |
| 29  | isoscan_ddr      | R/W  | 0h    | Drives ISOSCAN of DDR PLL.                                                                                                                                                                                                             |
| 28  | ret_ddr          | R/W  | 0h    | Drives RET signal of DDR PLL.                                                                                                                                                                                                          |
| 27  | reset_ddr        | R/W  | 0h    | Drives RESET of DDR DPLL.                                                                                                                                                                                                              |
| 26  | iso_ddr          | R/W  | 0h    | Drives ISO of DDR DPLL.                                                                                                                                                                                                                |
| 25  | pgoodin_ddr      | R/W  | 1h    | Drives PGOODIN of DDR DPLL.                                                                                                                                                                                                            |
| 24  | ponin_ddr        | R/W  | 1h    | Drives PONIN of DDR DPLL.                                                                                                                                                                                                              |
| 23  | sw_ctrl_disp_pll | R/W  | 0h    | Enable software control over DISP DPLL RET, RESET, ISO,<br>PGOODIN, PONIN for power savings.<br>0: PRCM controls the DPLL reset, RET = 0, ISO = 0, PGOODIN = 1,<br>PONIN = 1.<br>1: Controlled by corresponding bits in this register. |
| 22  | Reserved         | R    | 0h    |                                                                                                                                                                                                                                        |
| 21  | isoscan_disp     | R/W  | 0h    | Drives ISOSCAN of DISP PLL.                                                                                                                                                                                                            |
| 20  | ret_disp         | R/W  | 0h    | Drives RET of DISP DPLL.                                                                                                                                                                                                               |

**Table 9-85. dpll_pwr_sw_ctrl Register Field Descriptions (continued)**

| Bit | Field            | Type | Reset | Description                                                                                 |
|-----|------------------|------|-------|---------------------------------------------------------------------------------------------|
| 19  | reset_disp       | R/W  | 0h    | Drives RESET of DISP DPLL.                                                                  |
| 18  | iso_disp         | R/W  | 0h    | Drives ISO of DISP DPLL.                                                                    |
| 17  | pgoodin_disp     | R/W  | 1h    | Drives PGOODIN of DISP DPLL.                                                                |
| 16  | ponin_disp       | R/W  | 1h    | Drives PONIN of DISP DPLL.                                                                  |
| 15  | sw_ctrl_per_dpll | R/W  | 0h    | Enable software control over PER DPLL RET, RESET, ISO,<br>PGOODIN, PONIN for power savings. |
|     |                  |      |       | 0: PRCM controls the DPLL reset, RET = 0, ISO = 0, PGOODIN = 1,<br>PONIN = 1.               |
|     |                  |      |       | 1: Controlled by corresponding bits in this register.                                       |
| 14  | Reserved         | R    | 0h    |                                                                                             |
| 13  | isoscan_per      | R/W  | 0h    | Drives ISOSCAN of PER PLL.                                                                  |
| 12  | ret_per          | R/W  | 0h    | Drives RET of PER DPLL.                                                                     |
| 11  | reset_per        | R/W  | 0h    | Drives RESET signal of PER DPLL.                                                            |
| 10  | iso_per          | R/W  | 0h    | Drives ISO signal of PER DPLL.                                                              |
| 9   | pgoodin_per      | R/W  | 1h    | Drives PGOODIN signal of PER DPLL.                                                          |
| 8   | ponin_per        | R/W  | 1h    | Drives PONIN signal of PER DPLL.                                                            |
| 7-0 | Reserved         | R    | 0h    |                                                                                             |

#### 9.3.1.76 ddr_cke_ctrl Register (offset = 131Ch) [reset = 0h]

ddr_cke_ctrl is described in Table 9-86.

**Table 9-86. ddr_cke_ctrl Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                                                                                                                                                                                                                       |
|------|--------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | Reserved     | R    | 0h    |                                                                                                                                                                                                                                   |
| 0    | ddr_cke_ctrl | R/W  | 0h    | CKE from EMIF/DDRPHY is ANDed with this bit.<br>0: CKE to memories gated off to zero. External DRAM memories will<br>not able to register DDR commands from device<br>1: Normal operation. CKE is now controlled by EMIF/DDR PHY. |

#### 9.3.1.77 sma2 Register (offset = 1320h) [reset = 0h]

sma2 is described in Table 9-87.

**Table 9-87. sma2 Register Field Descriptions**

| Bit  | Field                       | Type | Reset | Description                                                                                                 |
|------|-----------------------------|------|-------|-------------------------------------------------------------------------------------------------------------|
| 31-2 | Reserved                    | R    | 0h    |                                                                                                             |
| 1    | vsldo_core_auto_ramp_en R/W |      | 0h    | 0: PRCM controls VSLDO.<br>1: Allows hardware to bring VSLDO out of retention on wakeup from<br>deep-sleep. |
| 0    | rmii2_crs_dv_mode_sel       | R/W  | 0h    | 0: Select MMC2_DAT7 on GPMC_A9 pin in MODE3.<br>1: Select RMII2_CRS_DV on GPMC_A9 pin in MODE3.             |

#### 9.3.1.78 m3_txev_eoi Register (offset = 1324h) [reset = 0h]

m3_txev_eoi is described in Table 9-88.

**Table 9-88. m3_txev_eoi Register Field Descriptions**

| Bit  | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|------|-------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | Reserved    | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 0    | m3_txev_eoi | R/W  | 0h    | TXEV (Event) from M3 processor is a pulse signal connected as<br>intertupt to MPU IRQ(78) Since MPU expects level signals.<br>The TXEV pulse from M3 is converted to a level in glue logic.<br>The logic works as follows:<br>-On a 0-1 transition on TXEV, the IRQ[78] is set.<br>-For clearing the interrupt, S/W must do the following:<br>S/W must clear the IRQ[78] by writing a 1 to M3_TXEV_EOI bit in<br>this registe<br>This bit is sticky and for re-arming the IRQ[78], S/W must write a 0 to<br>this field in the ISR |

#### 9.3.1.79 ipc_msg_reg0 Register (offset = 1328h) [reset = 0h]

ipc_msg_reg0 is described in Table 9-89. This register is typically used for messaging between Cortex A8 and CortexM3 (WKUP).

See the section "Functional Sequencing for Power Management with Cortex M3" for specific information on how the IPC_MSG_REG registers are used to communicate with the Cortex-M3 firmware.

**Table 9-89. ipc_msg_reg0 Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                        |
|------|--------------|------|-------|------------------------------------|
| 31-0 | ipc_msg_reg0 | R/W  | 0h    | Inter Processor Messaging Register |

#### 9.3.1.80 ipc_msg_reg1 Register (offset = 132Ch) [reset = 0h]

ipc_msg_reg1 is described in Table 9-90. This register is typically used for messaging between Cortex A8 and CortexM3 (WKUP).

See the section "Functional Sequencing for Power Management with Cortex M3" for specific information on how the IPC_MSG_REG registers are used to communicate with the Cortex-M3 firmware.

**Table 9-90. ipc_msg_reg1 Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                        |
|------|--------------|------|-------|------------------------------------|
| 31-0 | ipc_msg_reg1 | R/W  | 0h    | Inter Processor Messaging Register |

#### 9.3.1.81 ipc_msg_reg2 Register (offset = 1330h) [reset = 0h]

ipc_msg_reg2 is described in Table 9-91. This register is typically used for messaging between Cortex A8 and CortexM3 (WKUP).

See the section "Functional Sequencing for Power Management with Cortex M3" for specific information on how the IPC_MSG_REG registers are used to communicate with the Cortex-M3 firmware.

**Table 9-91. ipc_msg_reg2 Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                        |
|------|--------------|------|-------|------------------------------------|
| 31-0 | ipc_msg_reg2 | R/W  | 0h    | Inter Processor Messaging Register |

#### 9.3.1.82 ipc_msg_reg3 Register (offset = 1334h) [reset = 0h]

ipc_msg_reg3 is described in Table 9-92. This register is typically used for messaging between Cortex A8 and CortexM3 (WKUP).

See the section "Functional Sequencing for Power Management with Cortex M3" for specific information on how the IPC_MSG_REG registers are used to communicate with the Cortex-M3 firmware.

**Table 9-92. ipc_msg_reg3 Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                        |
|------|--------------|------|-------|------------------------------------|
| 31-0 | ipc_msg_reg3 | R/W  | 0h    | Inter Processor Messaging Register |

#### 9.3.1.83 ipc_msg_reg4 Register (offset = 1338h) [reset = 0h]

ipc_msg_reg4 is described in Table 9-93. This register is typically used for messaging between Cortex A8 and CortexM3 (WKUP).

See the section "Functional Sequencing for Power Management with Cortex M3" for specific information on how the IPC_MSG_REG registers are used to communicate with the Cortex-M3 firmware.

**Table 9-93. ipc_msg_reg4 Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                        |
|------|--------------|------|-------|------------------------------------|
| 31-0 | ipc_msg_reg4 | R/W  | 0h    | Inter Processor Messaging Register |

#### 9.3.1.84 ipc_msg_reg5 Register (offset = 133Ch) [reset = 0h]

ipc_msg_reg5 is described in Table 9-94. This register is typically used for messaging between Cortex A8 and CortexM3 (WKUP).

See the section "Functional Sequencing for Power Management with Cortex M3" for specific information on how the IPC_MSG_REG registers are used to communicate with the Cortex-M3 firmware.

**Table 9-94. ipc_msg_reg5 Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                        |
|------|--------------|------|-------|------------------------------------|
| 31-0 | ipc_msg_reg5 | R/W  | 0h    | Inter Processor Messaging Register |

#### 9.3.1.85 ipc_msg_reg6 Register (offset = 1340h) [reset = 0h]

ipc_msg_reg6 is described in Table 9-95. This register is typically used for messaging between Cortex A8 and CortexM3 (WKUP).

See the section "Functional Sequencing for Power Management with Cortex M3" for specific information on how the IPC_MSG_REG registers are used to communicate with the Cortex-M3 firmware.

**Table 9-95. ipc_msg_reg6 Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                        |
|------|--------------|------|-------|------------------------------------|
| 31-0 | ipc_msg_reg6 | R/W  | 0h    | Inter Processor Messaging Register |

#### 9.3.1.86 ipc_msg_reg7 Register (offset = 1344h) [reset = 0h]

ipc_msg_reg7 is described in Table 9-96. This register is typically used for messaging between Cortex A8 and CortexM3 (WKUP).

See the section "Functional Sequencing for Power Management with Cortex M3" for specific information on how the IPC_MSG_REG registers are used to communicate with the Cortex-M3 firmware.

**Table 9-96. ipc_msg_reg7 Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                        |
|------|--------------|------|-------|------------------------------------|
| 31-0 | ipc_msg_reg7 | R/W  | 0h    | Inter Processor Messaging Register |

#### 9.3.1.87 ddr_cmd0_ioctrl Register (offset = 1404h) [reset = 0h]

ddr_cmd0_ioctrl is described in Table 9-97.

**Table 9-97. ddr_cmd0_ioctrl Register Field Descriptions**

| Bit   | Field            | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-------|------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-21 | io_config_gp_wd1 | R/W  | 0h    | There are 2 bits per IO: io_config_gp_wd1 and io_config_gp_wd0.<br>For example:<br>macro pin 0: WD1 is bit 21, WD0 is bit 10<br>macro pin 1: WD1 is bit 22, WD0 is bit 11<br><br>macro pin 10: WD1 is bit 31, WD0 is bit 20<br>See the DDR PHY to IO Pin Mapping table in the Control Module<br>Functional Description section for a mapping of macro bits to I/Os.<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled |
| 20-10 | io_config_gp_wd0 | R/W  | 0h    | There are 2 bits per IO: io_config_gp_wd1 and io_config_gp_wd0.<br>For example:<br>macro pin 0: WD1 is bit 21, WD0 is bit 10<br>macro pin 1: WD1 is bit 22, WD0 is bit 11<br><br>macro pin 10: WD1 is bit 31, WD0 is bit 20<br>See the DDR PHY to IO Pin Mapping table in the Control Module<br>Functional Description section for a mapping of macro bits to I/Os.<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled |
| 9-8   | io_config_sr_clk | R/W  | 0h    | 2 bit to program clock IO Pads (DDR_CK/DDR_CKN) output slew<br>rate.<br>These connect as SR1, SR0 to the corresponding DDR IO buffer.<br>See the DDR Slew Rate Control Settings table in the Control Module<br>Functional Description section for a definition of these bits.                                                                                                                                                                                                                     |
| 7-5   | io_config_i_clk  | R/W  | 0h    | 3-bit configuration input to program clock IO pads<br>(DDR_CK/DDR_CKN) output impedance.<br>These connect as I2, I1, I0 to the corresponding DDR IO buffer.<br>See the DDR Impedance Control Settings table in the Control<br>Module Functional Description section for a definition of these bits.                                                                                                                                                                                               |

**Table 9-97. ddr_cmd0_ioctrl Register Field Descriptions (continued)**

| Bit | Field        | Type | Reset | Description                                                                                                                                                                                                                                                                   |
|-----|--------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 4-3 | io_config_sr | R/W  | 0h    | 2 bit to program addr/cmd IO Pads output slew rate.<br>These connect as SR1, SR0 to the corresponding DDR IO buffer.<br>See the DDR Slew Rate Control Settings table in the Control Module<br>Functional Description section for a definition of these bits.                  |
| 2-0 | io_config_i  | R/W  | 0h    | 3-bit configuration input to program addr/cmd IO output impedance.<br>These connect as I2, I1, I0 to the corresponding DDR IO buffer.<br>See the DDR Impedance Control Settings table in the Control<br>Module Functional Description section for a definition of these bits. |

#### 9.3.1.88 ddr_cmd1_ioctrl Register (offset = 1408h) [reset = 0h]

ddr_cmd1_ioctrl is described in Table 9-98.

**Table 9-98. ddr_cmd1_ioctrl Register Field Descriptions**

| Bit   | Field            | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-------|------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-21 | io_config_gp_wd1 | R/W  | 0h    | There are 2 bits per IO: io_config_gp_wd1 and io_config_gp_wd0.<br>For example:<br>macro pin 0: WD1 is bit 21, WD0 is bit 10<br>macro pin 1: WD1 is bit 22, WD0 is bit 11<br><br>macro pin 10: WD1 is bit 31, WD0 is bit 20<br>See the DDR PHY to IO Pin Mapping table in the Control Module<br>Functional Description section for a mapping of macro bits to I/Os.<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled |
| 20-10 | io_config_gp_wd0 | R/W  | 0h    | There are 2 bits per IO: io_config_gp_wd1 and io_config_gp_wd0.<br>For example:<br>macro pin 0: WD1 is bit 21, WD0 is bit 10<br>macro pin 1: WD1 is bit 22, WD0 is bit 11<br><br>macro pin 10: WD1 is bit 31, WD0 is bit 20<br>See the DDR PHY to IO Pin Mapping table in the Control Module<br>Functional Description section for a mapping of macro bits to I/Os.<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled |
| 9-8   | io_config_sr_clk | R/W  | 0h    | Only ddr_cmd0_ioctrl[9:8] are used to control io_config_sr_clk.                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 7-5   | io_config_i_clk  | R/W  | 0h    | Only ddr_cmd0_ioctrl[7:5] are used to control io_config_i_clk.                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 4-3   | io_config_sr     | R/W  | 0h    | 2 bit to program addr/cmd IO Pads output slew rate.<br>These connect as SR1, SR0 to the corresponding DDR IO buffer.<br>See the DDR Slew Rate Control Settings table in the Control Module<br>Functional Description section for a definition of these bits.                                                                                                                                                                                                                                      |
| 2-0   | io_config_i      | R/W  | 0h    | 3-bit configuration input to program addr/cmd IO output impedance.<br>These connect as I2, I1, I0 to the corresponding DDR IO buffer.<br>See the DDR Impedance Control Settings table in the Control<br>Module Functional Description section for a definition of these bits.                                                                                                                                                                                                                     |

#### 9.3.1.89 ddr_cmd2_ioctrl Register (offset = 140Ch) [reset = 0h]

ddr_cmd2_ioctrl is described in Table 9-99.

**Table 9-99. ddr_cmd2_ioctrl Register Field Descriptions**

| Bit   | Field            | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-------|------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-21 | io_config_gp_wd1 | R/W  | 0h    | There are 2 bits per IO: io_config_gp_wd1 and io_config_gp_wd0.<br>For example:<br>macro pin 0: WD1 is bit 21, WD0 is bit 10<br>macro pin 1: WD1 is bit 22, WD0 is bit 11<br><br>macro pin 10: WD1 is bit 31, WD0 is bit 20<br>See the DDR PHY to IO Pin Mapping table in the Control Module<br>Functional Description section for a mapping of macro bits to I/Os.<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled |
| 20-10 | io_config_gp_wd0 | R/W  | 0h    | There are 2 bits per IO: io_config_gp_wd1 and io_config_gp_wd0.<br>For example:<br>macro pin 0: WD1 is bit 21, WD0 is bit 10<br>macro pin 1: WD1 is bit 22, WD0 is bit 11<br><br>macro pin 10: WD1 is bit 31, WD0 is bit 20<br>See the DDR PHY to IO Pin Mapping table in the Control Module<br>Functional Description section for a mapping of macro bits to I/Os.<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled |
| 9-8   | io_config_sr_clk | R/W  | 0h    | Only ddr_cmd0_ioctrl[9:8] are used to control io_config_sr_clk.                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 7-5   | io_config_i_clk  | R/W  | 0h    | Only ddr_cmd0_ioctrl[7:5] are used to control io_config_i_clk.                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 4-3   | io_config_sr     | R/W  | 0h    | 2 bit to program addr/cmd IO Pads output slew rate.<br>These connect as SR1, SR0 to the corresponding DDR IO buffer.<br>See the DDR Slew Rate Control Settings table in the Control Module<br>Functional Description section for a definition of these bits.                                                                                                                                                                                                                                      |
| 2-0   | io_config_i      | R/W  | 0h    | 3-bit configuration input to program addr/cmd IO output impedance.<br>These connect as I2, I1, I0 to the corresponding DDR IO buffer.<br>See the DDR Impedance Control Settings table in the Control<br>Module Functional Description section for a definition of these bits.                                                                                                                                                                                                                     |

#### 9.3.1.90 ddr_data0_ioctrl Register (offset = 1440h) [reset = 0h]

ddr_data0_ioctrl is described in Table 9-100.

**Table 9-100. ddr_data0_ioctrl Register Field Descriptions**

| Bit   | Field             | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|-------|-------------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-30 | Reserved          | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 29    | io_config_wd1_dqs | R/W  | 0h    | Input that selects pullup or pulldown for DDR_DQS0 and<br>DDR_DQSn0.<br>Used with io_config_wd0_dqs to define pullup/pulldown according to<br>the following:<br>WD1: WD0<br>00b: Pullup/Pulldown disabled for both DDR_DQS0 and<br>DDR_DQSn0<br>01b: Enable weak pullup for DDR_DQS0 and weak pulldown for<br>DDR_DQSn0<br>10b: Enable weak pulldown for DDR_DQS0 and weak pullup for<br>DDR_DQSn0<br>11b: Weak keeper enabled for both DDR_DQS0 and DDR_DQSn0                                                                                   |
| 28    | io_config_wd1_dm  | R/W  | 0h    | Input that selects pullup or pulldown for DM.<br>Used with io_config_wd0_dm to define pullup/pulldown according to<br>the following:<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled                                                                                                                                                                                                                                                                               |
| 27-20 | io_config_wd1_dq  | R/W  | 0h    | Input that selects pullup or pulldown for DQ.<br>There are 2 bits per IO: io_config_wd1_dq and io_config_wd0_dq.<br>For example:<br>macro pin 0: WD1 is bit 20 WD0 is bit 10<br>macro pin 1: WD1 is bit 21, WD0 is bit 11<br><br>macro pin 7: WD1 is bit 27, WD0 is bit 17<br>See the DDR PHY to IO Pin Mapping table in the Control Module<br>Functional Description section for a mapping of macro bits to I/Os.<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled |

**Table 9-100. ddr_data0_ioctrl Register Field Descriptions (continued)**

| Bit   | Field             | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-------|-------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 19    | io_config_wd0_dqs | R/W  | 0h    | Input that selects pullup or pulldown for DDR_DQS0 and<br>DDR_DQSn0.<br>Used with io_config_wd1_dqs to define pullup/pulldown according to<br>the following:<br>WD1:WD0<br>00b: Pullup/Pulldown disabled for both DDR_DQS0 and<br>DDR_DQSn0<br>01b: Enable weak pullup for DDR_DQS0 and weak pulldown for<br>DDR_DQSn0<br>10b: Enable weak pulldown for DDR_DQS0 and weak pullup for<br>DDR_DQSn0<br>11b: Weak keeper enabled for both DDR_DQS0 and DDR_DQSn0                                                                                     |
| 18    | io_config_wd0_dm  | R/W  | 0h    | Input that selects pullup or pulldown for DM.<br>Used with io_config_wd1_dm to define pullup/pulldown according to<br>the following:<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled                                                                                                                                                                                                                                                                                |
| 17-10 | io_config_wd0_dq  | R/W  | 0h    | Input that selects pullup or pulldown for DQ.<br>There are 2 bits per IO: io_config_wd1_dq and io_config_wd0_dq.<br>For example:<br>macro pin 0: WD1 is bit 20, WD0 is bit 10<br>macro pin 1: WD1 is bit 21, WD0 is bit 11<br><br>macro pin 7: WD1 is bit 27, WD0 is bit 17<br>See the DDR PHY to IO Pin Mapping table in the Control Module<br>Functional Description section for a mapping of macro bits to I/Os.<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled |
| 9-8   | io_config_sr_clk  | R/W  | 0h    | 2 bit to program clock IO Pads (DDR_DQS/DDR_DQSn) output slew<br>rate.<br>These connect as SR1, SR0 of the corresponding IO buffer.<br>See the DDR Slew Rate Control Settings table in the Control Module<br>Functional Description section for a definition of these bits.                                                                                                                                                                                                                                                                       |
| 7-5   | io_config_i_clk   | R/W  | 0h    | 3-bit configuration input to program clock IO pads<br>(DDR_DQS/DDR_DQSn) output impedance.<br>These connect as I2, I1, I0 of the corresponding buffer.<br>See the DDR Impedance Control Settings table in the Control<br>Module Functional Description section for a definition of these bits.                                                                                                                                                                                                                                                    |
| 4-3   | io_config_sr      | R/W  | 0h    | 2 bit to program data IO Pads output slew rate.<br>These connect as SR1, SR0 of the corresponding IO buffer.<br>See the DDR Slew Rate Control Settings table in the Control Module<br>Functional Description section for a definition of these bits.                                                                                                                                                                                                                                                                                              |
| 2-0   | io_config_i       | R/W  | 0h    | 3-bit configuration input to program data IO output impedance.<br>These connect as I2, I1, I0 of the corresponding IO buffer.<br>See the DDR Impedance Control Settings table in the Control<br>Module Functional Description section for a definition of these bits.                                                                                                                                                                                                                                                                             |

#### 9.3.1.91 ddr_data1_ioctrl Register (offset = 1444h) [reset = 0h]

ddr_data1_ioctrl is described in Table 9-101.

**Table 9-101. ddr_data1_ioctrl Register Field Descriptions**

| Bit   | Field             | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-------|-------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-30 | Reserved          | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 29    | io_config_wd1_dqs | R/W  | 0h    | Input that selects pullup or pulldown for DDR_DQS1 and<br>DDR_DQSn1.<br>Used with io_config_wd0_dqs to define pullup/pulldown according to<br>the following:<br>WD1:WD0<br>00b: Pullup/Pulldown disabled for both DDR_DQS1 and<br>DDR_DQSn1<br>01b: Enable weak pullup for DDR_DQS1 and weak pulldown for<br>DDR_DQSn1<br>10b: Enable weak pulldown for DDR_DQS1 and weak pullup for<br>DDR_DQSn1<br>11b: Weak keeper enabled for both DDR_DQS1 and DDR_DQSn1                                                                                     |
| 28    | io_config_wd1_dm  | R/W  | 0h    | Input that selects pullup or pulldown for DM.<br>Used with io_config_wd0_dm to define pullup/pulldown according to<br>the following:<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled                                                                                                                                                                                                                                                                                |
| 27-20 | io_config_wd1_dq  | R/W  | 0h    | Input that selects pullup or pulldown for DQ.<br>There are 2 bits per IO: io_config_wd1_dq and io_config_wd0_dq.<br>For example:<br>macro pin 0: WD1 is bit 20, WD0 is bit 10<br>macro pin 1: WD1 is bit 21, WD0 is bit 11<br><br>macro pin 7: WD1 is bit 27, WD0 is bit 17<br>See the DDR PHY to IO Pin Mapping table in the Control Module<br>Functional Description section for a mapping of macro bits to I/Os.<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled |

**Table 9-101. ddr_data1_ioctrl Register Field Descriptions (continued)**

| Bit   | Field             | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-------|-------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 19    | io_config_wd0_dqs | R/W  | 0h    | Input that selects pullup or pulldown for DDR_DQS1 and<br>DDR_DQSn1.<br>Used with io_config_wd1_dqs to define pullup/pulldown according to<br>the following:<br>WD1:WD0<br>00b: Pullup/Pulldown disabled for both DDR_DQS1 and<br>DDR_DQSn1<br>01b: Enable weak pullup for DDR_DQS1 and weak pulldown for<br>DDR_DQSn1<br>10b: Enable weak pulldown for DDR_DQS1 and weak pullup for<br>DDR_DQSn1<br>11b: Weak keeper enabled for both DDR_DQS1 and DDR_DQSn1                                                                                     |
| 18    | io_config_wd0_dm  | R/W  | 0h    | Input that selects pullup or pulldown for DM.<br>Used with io_config_wd1_dm to define pullup/pulldown according to<br>the following:<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled                                                                                                                                                                                                                                                                                |
| 17-10 | io_config_wd0_dq  | R/W  | 0h    | Input that selects pullup or pulldown for DQ.<br>There are 2 bits per IO: io_config_wd1_dq and io_config_wd0_dq.<br>For example:<br>macro pin 0: WD1 is bit 20, WD0 is bit 10<br>macro pin 1: WD1 is bit 21, WD0 is bit 11<br><br>macro pin 7: WD1 is bit 27, WD0 is bit 17<br>See the DDR PHY to IO Pin Mapping table in the Control Module<br>Functional Description section for a mapping of macro bits to I/Os.<br>WD1:WD0<br>00: Pullup/Pulldown disabled<br>01: Weak pullup enabled<br>10: Weak pulldown enabled<br>11: Weak keeper enabled |
| 9-8   | io_config_sr_clk  | R/W  | 0h    | 2 bit to program clock IO Pads (DDR_DQS/DDR_DQSn) output slew<br>rate.<br>These connect as SR1, SR0 of the corresponding IO buffer.<br>See the DDR Slew Rate Control Settings table in the Control Module<br>Functional Description section for a definition of these bits.                                                                                                                                                                                                                                                                       |
| 7-5   | io_config_i_clk   | R/W  | 0h    | 3-bit configuration input to program clock IO pads<br>(DDR_DQS/DDR_DQSn) output impedance.<br>These connect as I2, I1, I0 of the corresponding buffer.<br>See the DDR Impedance Control Settings table in the Control<br>Module Functional Description section for a definition of these bits.                                                                                                                                                                                                                                                    |
| 4-3   | io_config_sr      | R/W  | 0h    | 2 bit to program data IO Pads output slew rate.<br>These connect as SR1, SR0 of the corresponding IO buffer.<br>See the DDR Slew Rate Control Settings table in the Control Module<br>Functional Description section for a definition of these bits.                                                                                                                                                                                                                                                                                              |
| 2-0   | io_config_i       | R/W  | 0h    | 3-bit configuration input to program data IO output impedance.<br>These connect as I2, I1, I0 of the corresponding IO buffer.<br>See the DDR Impedance Control Settings table in the Control<br>Module Functional Description section for a definition of these bits.                                                                                                                                                                                                                                                                             |