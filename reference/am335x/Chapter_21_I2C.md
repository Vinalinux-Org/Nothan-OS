# 21 I2C

This chapter describes the I2C of the device.

| Topic |                        | Page |
|-------|------------------------|------|
| 21.1  | Introduction           | 4584 |
| 21.2  | Integration            | 4585 |
| 21.3  | Functional Description | 4587 |
| 21.4  | I2C Registers          | 4601 |

*I2C*
## **21.1 Introduction**

The multi-master I2C peripheral provides an interface between a CPU and any I2C-bus-compatible device that connects via the I2C serial bus. External components attached to the I2C bus can serially transmit/receive up to 8-bit data to/from the CPU device through the two-wire I2C interface.

The I2C bus is a multi-master bus. The I2C controller does support the multi-master mode that allows more than one device capable of controlling the bus to be connected to it. Each I2C device is recognized by a unique address and can operate as either transmitter or receiver, according to the function of the device. In addition to being a transmitter or receiver, a device connected to the I2C bus can also be considered as master or slave when performing data transfers. Note that a master device is the device which initiates a data transfer on the bus and generates the clock signals to permit that transfer. During this transfer, any device addressed by this master is considered a slave.

### *21.1.1 I2C Features*

The general features of the I2C controller are:

- Compliant with Philips I2C specification version 2.1
- Supports standard mode (up to 100K bits/s) and fast mode (up to 400K bits/s).
- Multimaster transmitter/slave receiver mode
- Multimaster receiver/slave transmitter mode
- Combined master transmit/receive and receive/transmit modes
- 7-bit and 10-bit device addressing modes
- Built-in 32-byte FIFO for buffered read or writes in each module
- Programmable clock generation
- Two DMA channels, one interrupt line

### *21.1.2 Unsupported I2C Features*

The I2C module features not supported in this device are shown in [Table](#page-1-1) 21-1.

**Table 21-1. Unsupported I2C Features**

| Feature                         | Reason                     |
|---------------------------------|----------------------------|
| SCCB Protocol                   | SCCB signal not pinned out |
| High Speed (3.4 MBPS) operation | Not supported              |
## 21.2 Integration

This device includes three instantiations of the I2C module. This peripheral implements the multi-master I2C bus which allows serial transfer of 8-bit data to/from other I2C master/slave devices through a two-wire interface. There are three I2C modules instantiations called I2C0, I2C1, and I2C2. The I2C0 module is located in the Wake-up power domain. Figure 21-1 and Figure 21-2 show examples of a system with multiple I2C-compatible devices.


Figure 21-1. I2C0 Integration and Bus Application


Figure 21-2. I2C(1-2) Integration and Bus Application

### 21.2.1 I2C Connectivity Attributes

The general connectivity attributes for the I2C module are shown in Table 21-2 and Table 21-3.

**Attributes** Type Power Domain Wakeup Domain PD\_WKUP\_L4\_WKUP\_GCLK (Interface/OCP) Clock Domain PD\_WKUP\_I2C0\_GFCLK (Func) WKUP\_DOM\_RST\_N Reset Signals Idle/Wakeup Signals Smart Idle / Wakeup Interrupt Requests 1 interrupt to MPU Subsystem (I2C0INT), PRU-ICSS, and WakeM3 DMA Requests 2 DMA requests to EDMA (I2CTXEVT0, I2CRXEVT0) Physical Address L4 Wakeup slave port

Table 21-2. I2C0 Connectivity Attributes
### **Table 21-3. I2C(1–2) Connectivity Attributes**

| Attributes          | Type                                                          |
|---------------------|---------------------------------------------------------------|
| Power Domain        | Peripheral Domain                                             |
| Clock Domain        | PD_PER_L4LS_GCLK (Interface/OCP)<br>PD_PER_I2C_FCLK (Func)    |
| Reset Signals       | PER_DOM_RST_N                                                 |
| Idle/Wakeup Signals | Smart Idle                                                    |
| Interrupt Requests  | 1 interrupt per instance to MPU Subsystem (I2C1INT, I2C2INT)  |
| DMA Requests        | 2 DMA requests per instance to EDMA (I2CTXEVTx,<br>I2CRXEVTx) |
| Physical Address    | L4 Peripheral slave port                                      |

### *21.2.2 I2C Clock and Reset Management*

The I2C controllers have separate bus interface and functional clocks. During power-down mode, the I2C*x*\_SCL and I2C*x*\_SDA are configured as inputs.

**Table 21-4. I2C Clock Signals**

| Clock Signal                 | Max Freq           | Reference / Source     | Comments
|------------------------------|--------------------|------------------------|-----------------------------------|--|--|--|--|--|--
|                              | I2C0 Clock Signals |                        |
| PIOCPCLKInterface clock  | MHz                |                        | pd_wkup_l4_wkup_gclk<br>From PRCM
| PISYSCLKFunctional clock | 48 MHz             | PER_CLKOUTM2 / 4       | pd_wkup_i2c0_gfclk<br>From PRCM
|                              |                    | I2C(1-2) Clock Signals |
| PIOCPCLKInterface clock  | 100 MHz            | CORE_CLKOUTM4 / 2      | pd_per_l4ls_gclk<br>From PRCM
| PISYSCLKFunctional clock | 48 MHz             | PER_CLKOUTM2 / 4       | pd_per_ic2_fclk<br>From PRCM

### *21.2.3 I2C Pin List*

The external signals (I2C*x*\_SDA, I2C*x*\_SCL) on the device use standard LVCMOS I/Os and may not meet full compliance with the I2C specifications for Fast-mode devices for slope control and input filtering (spike suppression) to improve the EMC behavior.

**Table 21-5. I2C Pin List**

| Pin      | Type    | Description                   |
|----------|---------|-------------------------------|
| I2Cx_SCL | I/OD(1) | I2C serial clock (open drain) |
| I2Cx_SDA | I/OD    | I2C serial data (open drain)  |

<sup>(1)</sup> These signals are also used as inputs to re-time or sync data. The associated CONF\_<module>\_<pin>\_RXACTIVE bit for these signals must be set to 1 to enable the inputs back to the module. It is also recommended to place a 33-ohm resistor in series (close to the processor) on each of these signals to avoid signal reflections.
## **21.3 Functional Description**

### *21.3.1 Functional Block Diagram*

[Figure](#page-4-1) 21-3 shows an example of a system with multiple I2C compatible devices in which the I2C serial ports are all connected together for a two-way transfer from one device to other devices.


**Figure 21-3. I2C Functional Block Diagram**

The I2C peripheral consists of the following primary blocks:

- A serial interface: one data pin (I2C\_SDA) and one clock pin (I2C\_SCL).
- Data registers to temporarily hold receive data and transmit data traveling between the I2C\_SDA pin and the CPU or the DMA controller.
- Control and status registers
- A peripheral data bus interface to enable the CPU and the DMA controller to access the I2C peripheral registers.
- A clock synchronizer to synchronize the I2C input clock (from the processor clock generator) and the clock on the I2C\_SCL pin, and to synchronize data transfers with masters of different clock speeds.
- A prescaler to divide down the input clock that is driven to the I2C peripheral
- A noise filter on each of the two pins, I2C\_SDA and I2C\_SCL
- An arbitrator to handle arbitration between the I2C peripheral (when it is a master) and another master
- Interrupt generation logic, so that an interrupt can be sent to the CPU
- DMA event generation logic to send an interrupt to the CPU upon reception and data transmission of data.

### *21.3.2 I2C Master/Slave Contoller Signals*

Data is communicated to devices interfacing with the I2C via the serial data line (SDA) and the serial clock line (SCL). These two wires can carry information between a device and others connected to the I2C bus. Both SDA and SCL are bi-directional pins. They must be connected to a positive supply voltage via a pullup resistor. When the bus is free, both pins are high. The driver of these two pins has an open drain to perform the required wired-AND function.

An example of multiple I2C modules that are connected for a two-way transfer from one device to other devices is shown in [Figure](#page-5-0) 21-4.
TI device I2C I 2C EPROM I 2C I2C TI device VDD Pull-up resistors Serial data (I2Cx\_SDA) Serial clock (I2Cx\_SCL) controller

**Figure 21-4. Multiple I2C Modules Connected**

**Table 21-6. Signal Pads**

|         |                           | I2C Mode                                                           |
|---------|---------------------------|--------------------------------------------------------------------|
| Name    | Default Operating<br>Mode | Description                                                        |
| I2C_SCL | In/ Out                   | I2C serial CLK line                                                |
|         |                           | Open-drain output buffer. Requires external pull-up resistor (Rp). |
| I2C_SDA | In/ Out                   | I2C serial data line                                               |
|         |                           | Open-drain output buffer. Requires external pull-up resistor (Rp). |

### *21.3.3 I2C Reset*

The I2C module can be reset in the following three ways:

- A system reset (PIRSTNA = 0). A device reset causes the system reset. All registers are reset to power up reset values.
- A software reset by setting the SRST bit in the I2C\_SYSC register. This bit has exactly the same action on the module logic as the system bus reset. All registers are reset to power up reset values.
- The I2C\_EN bit in the I2C\_CON register can be used to hold the I2C module in reset. When the system bus reset is removed (PIRSTNA = 1), I2C\_EN = 0 keeps the functional part of I2C module in reset state and all configuration registers can be accessed. I2C\_EN = 0 does not reset the registers to power up reset values.

**Table 21-7. Reset State of I2C Signals**

|     |           |                | I2C Reset      |
|-----|-----------|----------------|----------------|
| Pin | I/O/Z (1) | System Reset   | (I2C_EN = 0)   |
| SDA | I/O/Z     | High impedance | High impedance |
| SCL | I/O/Z     | High impedance | High impedance |

<sup>(1)</sup> I = Input, O = Outpu, Z = High impedance
### *21.3.4 Data Validity*

The data on the SDA line must be stable during the high period of the clock. The high and low states of the data line can only change when the clock signal on the SCL line is LOW.

**Figure 21-5. Bit Transfer on the I2C Bus**


*I2C*
### 21.3.5 START & STOP Conditions

The I2C module generates START and STOP conditions when it is configured as a master.

- START condition is a high-to-low transition on the SDA line while SCL is high.
- STOP condition is a low-to-high transition on the SDA line while SCL is high.
- The bus is considered to be busy after the START condition (BB = 1) and free after the STOP condition (BB = 0).


Figure 21-6. Start and Stop Condition Events

### 21.3.6 I2C Operation

#### 21.3.6.1 Serial Data Formats

The I2C controller operates in 8-bit word data format (byte write access supported for the last access). Each byte put on the SDA line is 8 bits long. The number of bytes that can be transmitted or received is restricted by the value programmed in the DCOUNT register. The data is transferred with the most significant bit (MSB) first. Each byte is followed by an acknowledge bit from the I2C module if it is in receiver mode.


The I2C module supports two data formats, as shown in Figure 21-8:

- 7-bit/10-bit addressing format
- 7-bit/10-bit addressing format with repeated start condition

The first byte after a start condition (S) always consists of 8 bits. In the acknowledge mode, an extra bit dedicated for acknowledgment is inserted after each byte. In the addressing formats with 7-bit addresses, the first byte is composed of 7 MSB slave address bits and 1 LSB R/nW bit. In the addressing formats with 10-bit addresses, the first byte is composed of 7 MSB slave address bits, such as 11110XX, where XX is the two MSB of the 10-bit addresses, and 1 LSB R/nW bit, which is 0 in this case.
The least significant R/nW of the address byte indicates the direction of transmission of the following data bytes. If R/nW is 0, the master writes data into the selected slave; if it is 1, the master reads data out of the slave.

#### Figure 21-8. I2C Data Transfer Formats


7-Bit Addressing Format


10-Bit Addressing Format


7-Bit Addressing Format With Repeated START Condition

#### 21.3.6.2 Master Transmitter

In this mode, data assembled in one of the previously described data formats is shifted out on the serial data line SDA in synch with the self-generated clock pulses on the serial clock line SCL. The clock pulses are inhibited and SCL held low when the intervention of the processor is required (XUDF) after a byte has been transmitted.

#### 21.3.6.3 Master Receiver

This mode can only be entered from the master transmitter mode. With either of the address formats (Figure 21-8 (a), (b), and (c)), the master receiver is entered after the slave address byte and bit R/W\_ has been transmitted, if R/W\_ is high. Serial data bits received on bus line SDA are shifted in synch with the self-generated clock pulses on SCL. The clock pulses are inhibited and SCL held low when the intervention of the processor is required (ROVR) after a byte has been transmitted. At the end of a transfer, it generates the stop condition.

#### 21.3.6.4 Slave Transmitter

This mode can only be entered from the slave receiver mode. With either of the address formats (Figure 21-8 (a), (b), and (c)), the slave transmitter is entered if the slave address byte is the same as its own address and bit R/W\_ has been transmitted, if R/W\_ is high. The slave transmitter shifts the serial data out on the data line SDA in synch with the clock pulses that are generated by the master device. It does not generate the clock but it can hold clock line SCL low while intervention of the CPU is required (XUDF).

#### 21.3.6.5 Slave Receiver

In this mode, serial data bits received on the bus line SDA are shifted-in in synch with the clock pulses on SCL that are generated by the master device. It does not generate the clock but it can hold clock line SCL low while intervention of the CPU is required (ROVR) following the reception of a byte.
### *21.3.7 Arbitration*

If two or more master transmitters start a transmission on the same bus almost simultaneously, an arbitration procedure is invoked. The arbitration procedure uses the data presented on the serial bus by the competing transmitters. When a transmitter senses that a high signal it has presented on the bus has been overruled by a low signal, it switches to the slave receiver mode, sets the arbitration lost (AL) flag, and generates the arbitration lost interrupt. [Figure](#page-9-0) 21-9 shows the arbitration procedure between two devices. The arbitration procedure gives priority to the device that transmits the serial data stream with the lowest binary value. Should two or more devices send identical first bytes, arbitration continues on the subsequent bytes.


**Figure 21-9. Arbitration Procedure Between Two Master Transmitters**

### *21.3.8 I2C Clock Generation and I2C Clock Synchronization*

Under normal conditions, only one master device generates the clock signal, SCL. During the arbitration procedure, however, there are two or more master devices and the clock must be synchronized so that the data output can be compared. The wired-AND property of the clock line means that a device that first generates a low period of the clock line overrules the other devices. At this high/low transition, the clock generators of the other devices are forced to start generation of their own low period. The clock line is then held low by the device with the longest low period, while the other devices that finish their low periods must wait for the clock line to be released before starting their high periods. A synchronized signal on the clock line is thus obtained, where the slowest device determines the length of the low period and the fastest the length of the high period.

If a device pulls down the clock line for a longer time, the result is that all clock generators must enter the WAIT-state. In this way a slave can slow down a fast master and the slow device can create enough time to store a received byte or to prepare a byte to be transmitted (Clock Stretching). [Figure](#page-10-0) 21-10 illustrates the clock synchronization.

**Note:** If the SCL or SDA lines are stuck low, the Bus Clear operation is supported. If the clock line (SCL) is stuck low, the preferred procedure is to reset the bus using the hardware reset signal if your I2C devices have hardware reset inputs. If the I2C devices do not have hardware reset inputs, cycle power to the devices to activate the mandatory internal power-on reset (POR) circuit. If the data line (SDA) is stuck low, the master should send nine clock pulses. The device that held the bus low should release it sometime within those nine clocks. If not, use the hardware reset or cycle power to clear the bus.

Figure 21-10. Synchronization of Two I2C Clock Generators

### 21.3.9 Prescaler (SCLK/ICLK)

The I2C module is operated with a functional clock (SCLK) frequency that can be in a range of 12-100 MHz, according to I2C mode that must be used (an internal ~24 MHz clock (ICLK) is recommended in case of F/S operation mode e). Note that the frequency of the functional clock influences directly the I2C bus performance and timings.

The internal clock used for I2C logic - ICLK - is generated via the I2C prescaler block. The prescaler consists of a 4-bit register - I2C \_PSC, and is used to divide the system clock (SCLK) to obtain the internal required clock for the I2C module.

### 21.3.10 Noise Filter

The noise filter is used to suppress any noise that is 50 ns or less, in the case of F/S mode of operation. It is designed to suppress noise with one ICLK. The noise filter is always one ICLK cycle, regardless of the bus speed. For FS mode (prescaler = 4, ICLK = 24 MHz), the maximum width of the suppressed spikes is 41.6 ns. To ensure a correct filtering, the prescaler must be programmed accordingly.

### 21.3.11 I2C Interrupts

The I2C module generates 12 types of interrupt: addressed as slave, bus free (stop condition detected), access error, start condition, arbitration-lost, noacknowledge, general call, registers-ready-for-access, receive and transmit data, receive and transmit draining. These 12 interrupts are accompanied with 12 interrupt masks and flags defined in the I2C\_IRQENABLE\_SET and respectively I2C\_IRQSTATUS\_RAW registers. Note that all these 12 interrupt events are sharing the same hardware interrupt line.

- Addressed As Slave interrupt (AAS) is generated to inform the Local Host that an external master addressed the module as a slave. When this interrupt occurs, the CPU can check the I2C\_ACTOA status register to check which of the 4 own addresses was used by the external master to access the module.
- Bus Free interrupt (BF) is generated to inform the Local Host that the I2C bus became free (when a Stop Condition is detected on the bus) and the module can initiate his own I2C transaction.
- Start Condition interrupt (STC) is generated after the module being in idle mode have detected (synchronously or asynchronously) a possible Start Condition on the bus (signalized with WakeUp).
- Access Error interrupt (AERR) is generated if a Data read access is performed while RX FIFO is empty or a Data write access is performed while TX FIFO is full.
- Arbitration lost interrupt (AL) is generated when the I2C arbitration procedure is lost.
- No-acknowledge interrupt (NACK) is generated when the master I2C does not receive acknowledge from the receiver.
- General call interrupt (GC) is generated when the device detects the address of all zeros (8 bits).
- Registers-ready-for-access interrupt (ARDY) is generated by the I2C when the previously programmed address, data, and command have been performed and the status bits have been updated. This
interrupt is used to let the CPU know that the I2C registers are ready for access.

- Receive interrupt/status (RRDY) is generated when there is received data ready to be read by the CPU from the I2C\_DATA register (see the FIFO Management subsection for a complete description of required conditions for interrupt generation). The CPU can alternatively poll this bit to read the received data from the I2C\_DATA register.
- Transmit interrupt/status (XRDY) is generated when the CPU needs to put more data in the I2C\_DATA register after the transmitted data has been shifted out on the SDA pin (see the FIFO Management subsection for a complete description of required conditions for interrupt generation). The CPU can alternatively poll this bit to write the next transmitted data into the I2C\_DATA register.
- Receive draining interrupt (RDR) is generated when the transfer length is not a multiple of threshold value, to inform the CPU that it can read the amount of data left to be transferred and to enable the draining mechanism (see Section [21.3.14.4](#page-16-0), *Draining Feature*, for additional details).
- Transmit draining interrupt (XDR) is generated when the transfer length is not a multiple of threshold value, to inform the CPU that it can read the amount of data left to be written and to enable the draining mechanism (see Section [21.3.14.4](#page-16-0), *Draining Feature*, for additional details).

When the interrupt signal is activated, the Local Host must read the I2C\_IRQSTATUS\_RAW register to define the type of the interrupt, process the request, and then write into these registers the correct value to clear the interrupt flag.

### *21.3.12 DMA Events*

The I2C module can generate two DMA requests events, read (I2C\_DMA\_RX) and write (I2C\_DMA\_TX) that can be used by the DMA controller to synchronously read received data from the I2C\_DATA or write transmitted data to the I2C\_DATA register. The DMA read and write requests are generated in a similar manner as RRDY and XRDY, respectively.

The I2C DMA request signals (I2C\_DMA\_TX and I2C\_DMA\_RX) are activated according to the FIFO Management subsection.

### *21.3.13 Interrupt and DMA Events*

I2C has two DMA channels (Tx and Rx).

I2C has one interrupt line for all the interrupt requests.

For the event and interrupt numbers, see the device-specific datasheet.

### *21.3.14 FIFO Management*

The I2C module implements two internal 32-bytes FIFOs with dual clock for RX and TX modes. The depth of the FIFOs can be configured at integration via a generic parameter which will also be reflected in I2C\_IRQSTATUS\_RAW.FIFODEPTH register.

#### **21.3.14.1 FIFO Interrupt Mode Operation**

In FIFO interrupt mode (relevant interrupts enabled via I2C\_IRQENABLE\_SET register), the processor is informed of the status of the receiver and transmitter by an interrupt signal. These interrupts are raised when receive/transmit FIFO threshold (defined by I2C\_BUF.TXTRSH or I2C\_BUF.RXTRSH) are reached; the interrupt signals instruct the Local Host to transfer data to the destination (from the I2C module in receive mode and/or from any source to the I2C FIFO in transmit mode).

[Figure](#page-12-0) 21-11 and [Figure](#page-12-1) 21-12, respectively, illustrate receive and transmit operations from FIFO management point of view.

**Figure 21-11. Receive FIFO Interrupt Request Generation**

Note that in [Figure](#page-12-0) 21-11, the RRDY Condition illustrates that the condition for generating a RRDY interrupt is achieved. The interrupt request is generated when this signal is active, and it can be cleared only by the CPU by writing a 1 in the corresponding interrupt flag. If the condition is still present after clearing the previous interrupt, another interrupt request will be generated.

In receive mode, RRDY interrupt is not generated until the FIFO reaches its receive threshold. Once low, the interrupt can only be de-asserted when the Local Host has handled enough bytes to make the FIFO level below threshold. For each interrupt, the Local Host can be configured to read an amount of bytes equal with the value of the RX FIFO threshold + 1.


**Figure 21-12. Transmit FIFO Interrupt Request Generation**

Note that in [Figure](#page-12-1) 21-12, the XRDY Condition illustrates that the condition for generating a XRDY interrupt is achieved. The interrupt request is generated when this condition is achieved (when TX FIFO is empty, or the TX FIFO threshold is not reached and there are still data bytes to be transferred in the TX FIFO), and it can be cleared only by the CPU by writing a 1 in the corresponding interrupt flag after transmitting the configured number of bytes. If the condition is still present after clearing the previous interrupt, another interrupt request will be generated.

Note that in interrupt mode, the module offers two options for the CPU application to handle the interrupts:

• When detecting an interrupt request (XRDY or RRDY type), the CPU can write/read one data byte to/from the FIFO and then clear the interrupt. The module will not reassert the interrupt until the

*I2C*
interrupt condition is not met.

• When detecting an interrupt request (XRDY or RRDY type), the CPU can be programmed to write/read the amount of data bytes specified by the corresponding FIFO threshold (I2C\_BUF.TXTRSH + 1 or I2C\_BUF.RXTRSH + 1). In this case, the interrupt condition will be cleared and the next interrupt will be asserted again when the XRDY or RRDY condition will be again met.

If the second interrupt serving approach is used, an additional mechanism (draining feature) is implemented for the case when the transfer length is not a multiple of FIFO threshold (see Section [21.3.14.4](#page-16-0), *Draining Feature*).

In slave TX mode, the draining feature cannot be used, since the transfer length is not known at the configuration time, and the external master can end the transfer at any point by not acknowledging one data byte.

#### **21.3.14.2 FIFO Polling Mode Operation**

In FIFO polled mode (I2C\_IRQENABLE\_SET.XRDY\_IE and I2C\_IRQENABLE\_SET.RRDY\_IE disabled and DMA disabled), the status of the module (receiver or transmitter) can be checked by polling the XRDY and RRDY status registers (I2C\_IRQSTATUS\_RAW) (RDR and XDR can also be polled if draining feature must be used). The XRDY and RRDY flags are accurately reflecting the interrupt conditions mentioned in Interrupt Mode. This mode is an alternative to the FIFO interrupt mode of operation, where the status of the receiver and transmitter is automatically known by means of interrupts sent to the CPU.

#### **21.3.14.3 FIFO DMA Mode Operation**

In receive mode, a DMA request is generated as soon as the receive FIFO exceeds its threshold level defined in the threshold level register (I2C\_BUF.RXTRSH +1). This request should be de-asserted when the number of bytes defined by the threshold level has been read by the DMA, by setting the I2C\_DMARXENABLE\_CLR.DMARX\_ENABLE\_CLEAR field.


**Figure 21-13. Receive FIFO DMA Request Generation**

In transmit mode, a DMA request is automatically asserted when the transmit FIFO is empty. This request should be de-asserted when the number of bytes defined by the number in the threshold register (I2C\_BUF.TXTHRS+1) has been written in the FIFO by the DMA, by setting the I2C\_DMATXENABLE\_CLR. DMATX\_ENABLE\_CLEAR field. If an insufficient number of characters are written, then the DMA request will remain active. [Figure](#page-14-0) 21-14 and [Figure](#page-14-1) 21-15 illustrate DMA TX transfers with different values for TXTRSH.
**Figure 21-14. Transmit FIFO DMA Request Generation (High Threshold)**


**Figure 21-15. Transmit FIFO DMA Request Generation (Low Threshold)**


Note that also in DMA mode it is possible to have a transfer whose length is not multiple of the configured FIFO threshold. In this case, the DMA draining feature is also used for transferring the additional bytes of the transfer (see Section [21.3.14.4](#page-16-0), *Draining Feature*, for additional details).

According to the desired operation mode, the programmer must set the FIFO thresholds according to the following table (note that only the interface/OCP side thresholds can be programmed; the I2C side thresholds are default equals to 1). Note that the thresholds must be set consistent with the DMA channel length.

In I2C Slave RX Mode, the Local Host can program the RX threshold with the desired value, and use the FIFO draining feature at the end of the I2C transfer to extract from the FIFO the remaining bytes if the threshold is not reached (see Section [21.3.14.4,](#page-16-0) *Draining Feature*, for additional details).

*I2C*
Note that in I2C Slave TX Mode, the TX FIFO threshold should be set to 1 (I2C\_BUF.TXTRSH=0, default value), since the length of the transfer may not be known at configuration time. In this way, the interrupt (or accordingly, DMA) requests will be generated for each byte requested by the remote I2C master to be transferred over the I2C bus. This configuration will prevent the I2C core to request additional data from the CPU or from the DMA controller (using IRQ or DMA), data that will eventually not be extracted from the FIFO by the external master (which can use not acknowledge at any time to end the transfer). If the TX threshold is not set to 1, the module will generate an interrupt or assert a DMA only when the external master requests a byte and the FIFO is empty. However, in this case the TX FIFO will require to be cleared at the end of the transfer.

The I2C module offers the possibility to the user to clear the RX or TX FIFO. This is achieved through I2C\_BUF.RXFIFO\_CLR and I2C\_BUF.TXFIFO\_CLR registers, which act like software reset for the FIFOs. In DMA mode, these bits will also reset the DMA state machines.

The FIFO clearing feature can be used when the module is configured as a transmitter, the external receiver responds with a NACK in the middle of the transfer, and there is still data in TX FIFO waiting to be transferred.

On the Functional (I2C) domain, the thresholds can always be considered equal to 1. This means that the I2C Core can start transferring data on the I2C bus whenever it has data in the FIFOs (FIFO is not empty).
#### **21.3.14.4 Draining Feature**

The Draining Feature is implemented by the I2C core for handling the end of the transfers whose length is not a multiple of FIFO threshold value, and offers the possibility to transfer the remaining amount of bytes (since the threshold is not reached).

Note that this feature prevents the CPU or the DMA controller to attempt more FIFO accesses than necessary (for example, to generate at the end of a transfer a DMA RX request having in the FIFO less bytes than the configured DMA transfer length). Otherwise, an Access Error interrupt will be generated (see I2C\_IRQSTATUS\_RAW.AERR interrupt).

The Draining mechanism will generate an interrupt (I2C\_IRQSTATUS\_RAW.RDR or I2C\_IRQSTATUS\_RAW.XDR) at the end of the transfer informing the CPU that it needs to check the amount of data left to be transferred (I2C\_BUFSTAT.TXSTAT or RXSTAT) and to enable the Draining Feature of the DMA controller if DMA mode is enabled (by re-configuring the DMA transfer length according to this value), or perform only the required number of data accesses, if DMA mode is disabled.

In receiving mode (master or slave), if the RX FIFO threshold is not reached but the transfer was ended on the I2C bus and there is still data left in the FIFO (less than the threshold), the receive draining interrupt (I2C\_IRQSTATUS\_RAW.RDR) will be asserted to inform the local host that it can read the amount of data in the FIFO (I2C\_BUFSTAT.RXSTAT). The CPU will perform a number of data read accesses equal with RXSTAT value (if interrupt or polling mode) or re-configure the DMA controller with the required value in order to drain the FIFO.

In master transmit mode, if the TX FIFO threshold is not reached but the amount of data remaining to be written in the FIFO is less than TXTRSH, the transmit draining interrupt (I2C\_IRQSTATUS\_RAW.XDR) will be asserted to inform the local host that it can read the amount of data remained to be written in the TX FIFO (I2C\_BUFSTAT.TXSTAT). The CPU will need to write the required number of data bytes (specified by TXSTAT value) or re-configure the DMA controller with the required value in order to transfer the last bytes to the FIFO.

Note that in master mode, the CPU can alternatively skip the checking of TXSTAT and RXSTAT values since it can obtain internally this information (by computing DATACOUNT modulo TX/RXTHRSH).

The draining feature is disabled by default, and it can be enabled using I2C\_IRQENABLE\_SET.XDR\_IE or I2C\_IRQENABLE\_SET.RDR\_IE registers (default disabled) only for the transfers with length not equal with the threshold value.

### *21.3.15 How to Program I2C*

#### **21.3.15.1 Module Configuration Before Enabling the Module**

- 1. Program the prescaler to obtain an approximately 12-MHz I2C module clock (I2C\_PSC = x; this value is to be calculated and is dependent on the System clock frequency).
- 2. Program the I2C clock to obtain 100 Kbps or 400 Kbps (SCLL = x and SCLH = x; these values are to be calculated and are dependent on the System clock frequency).
- 3. Configure its own address (I2C\_OA = x) only in case of I2C operating mode (F/S mode).
- 4. Take the I2C module out of reset (I2C\_CON:I2C\_EN = 1).

#### **21.3.15.2 Initialization Procedure**

- 1. Configure the I2C mode register (I2C\_CON) bits.
- 2. Enable interrupt masks (I2C\_IRQENABLE\_SET), if using interrupt for transmit/receive data.
- 3. Enable the DMA (I2C\_BUF and I2C\_DMA/RX/TX/ENABLE\_SET) and program the DMA controller) only in case of I2C operating mode (F/S mode), if using DMA for transmit/receive data.

#### **21.3.15.3 Configure Slave Address and DATA Counter Registers**

In master mode, configure the slave address (I2C\_SA = x) and the number of byte associated with the transfer (I2C\_CNT = x).
#### **21.3.15.4 Initiate a Transfer**

Poll the bus busy (BB) bit in the I2C status register (I2C\_IRQSTATUS\_RAW). If it is cleared to 0 (bus not busy), configure START/STOP (I2C\_CON: STT / I2C\_CON: STP condition to initiate a transfer) - only in case of I2C operating mode (F/S mode).

#### **21.3.15.5 Receive Data**

Poll the receive data ready interrupt flag bit (RRDY) in the I2C status register (I2C\_IRQSTATUS\_RAW), use the RRDY interrupt (I2C\_IRQENABLE\_SET.RRDY\_IE set) or use the DMA RX (I2C\_BUF.RDMA\_EN set together with I2C\_DMARXENABLE\_SET) to read the received data in the data receive register (I2C\_DATA). Use draining feature (I2C\_IRQSTATUS\_RAW.RDR enabled by I2C\_IRQENABLE\_SET.RDR\_IE)) if the transfer length is not equal with FIFO threshold.

#### **21.3.15.6 Transmit Data**

Poll the transmit data ready interrupt flag bit (XRDY) in the I2C status register (I2C\_IRQSTATUS\_RAW), use the XRDY interrupt (I2C\_IRQENABLE\_SET.XRDY\_IE set) or use the DMA TX (I2C\_BUF.XDMA\_EN set together with I2C\_DMATXENABLE\_SET) to write data into the data transmit register (I2C\_DATA). Use draining feature (I2C\_IRQSTATUS\_RAW.XDR enabled by I2C\_IRQENABLE\_SET.XDR\_IE)) if the transfer length is not equal with FIFO threshold.

### *21.3.16 I2C Behavior During Emulation*

To configure the I2C to stop during emulation suspend events (for example, debugger breakpoints), set up the I2C and the Debug Subsystem:

- 1. Set I2C\_SYSTEST.FREE=0. This will allow the Suspend\_Control signal from the Debug Subsystem (Chapter 27) to stop and start the I2C. Note that if FREE=1, the Suspend\_Control signal is ignored and the I2C is free running regardless of any debug suspend event. This FREE bit gives local control from a module perspective to gate the suspend signal coming from the Debug Subsystem.
- 2. Set the appropriate xxx\_Suspend\_Control register = 0x9, as described in Section 27.1.1.1, *Debug Suspend Support for Peripherals*. Choose the register appropriate to the peripheral you want to suspend during a suspend event.
## **21.4 I2C Registers**

**NOTE:** All bits defined as reserved must be written by software with 0s, for preserving future compatibility. When read, any reserved bit returns 0. Also, note that it is good software practice to use complete mask patterns for setting or testing individually bit fields within a register.

### *21.4.1 I2C Registers*

[Table](#page-18-1) 21-8 lists the memory-mapped registers for the I2C. All register offset addresses not listed in [Table](#page-18-1) 21-8 should be considered as reserved locations and the register contents should not be modified.

### **Table 21-8. I2C Registers**

| Offset | Acronym             | Register Name                         | Section           |
|--------|---------------------|---------------------------------------|-------------------|
| 0h     | I2C_REVNB_LO        | Module Revision Register (low bytes)  | Section 21.4.1.1  |
| 4h     | I2C_REVNB_HI        | Module Revision Register (high bytes) | Section 21.4.1.2  |
| 10h    | I2C_SYSC            | System Configuration Register         | Section 21.4.1.3  |
| 24h    | I2C_IRQSTATUS_RAW   | I2C Status Raw Register               | Section 21.4.1.4  |
| 28h    | I2C_IRQSTATUS       | I2C Status Register                   | Section 21.4.1.5  |
| 2Ch    | I2C_IRQENABLE_SET   | I2C Interrupt Enable Set Register     | Section 21.4.1.6  |
| 30h    | I2C_IRQENABLE_CLR   | I2C Interrupt Enable Clear Register   | Section 21.4.1.7  |
| 34h    | I2C_WE              | I2C Wakeup Enable Register            | Section 21.4.1.8  |
| 38h    | I2C_DMARXENABLE_SET | Receive DMA Enable Set Register       | Section 21.4.1.9  |
| 3Ch    | I2C_DMATXENABLE_SET | Transmit DMA Enable Set Register      | Section 21.4.1.10 |
| 40h    | I2C_DMARXENABLE_CLR | Receive DMA Enable Clear Register     | Section 21.4.1.11 |
| 44h    | I2C_DMATXENABLE_CLR | Transmit DMA Enable Clear Register    | Section 21.4.1.12 |
| 48h    | I2C_DMARXWAKE_EN    | Receive DMA Wakeup Register           | Section 21.4.1.13 |
| 4Ch    | I2C_DMATXWAKE_EN    | Transmit DMA Wakeup Register          | Section 21.4.1.14 |
| 90h    | I2C_SYSS            | System Status Register                | Section 21.4.1.15 |
| 94h    | I2C_BUF             | Buffer Configuration Register         | Section 21.4.1.16 |
| 98h    | I2C_CNT             | Data Counter Register                 | Section 21.4.1.17 |
| 9Ch    | I2C_DATA            | Data Access Register                  | Section 21.4.1.18 |
| A4h    | I2C_CON             | I2C Configuration Register            | Section 21.4.1.19 |
| A8h    | I2C_OA              | I2C Own Address Register              | Section 21.4.1.20 |
| ACh    | I2C_SA              | I2C Slave Address Register            | Section 21.4.1.21 |
| B0h    | I2C_PSC             | I2C Clock Prescaler Register          | Section 21.4.1.22 |
| B4h    | I2C_SCLL            | I2C SCL Low Time Register             | Section 21.4.1.23 |
| B8h    | I2C_SCLH            | I2C SCL High Time Register            | Section 21.4.1.24 |
| BCh    | I2C_SYSTEST         | System Test Register                  | Section 21.4.1.25 |
| C0h    | I2C_BUFSTAT         | I2C Buffer Status Register            | Section 21.4.1.26 |
| C4h    | I2C_OA1             | I2C Own Address 1 Register            | Section 21.4.1.27 |
| C8h    | I2C_OA2             | I2C Own Address 2 Register            | Section 21.4.1.28 |
| CCh    | I2C_OA3             | I2C Own Address 3 Register            | Section 21.4.1.29 |
| D0h    | I2C_ACTOA           | Active Own Address Register           | Section 21.4.1.30 |
| D4h    | I2C_SBLOCK          | I2C Clock Blocking Enable Register    | Section 21.4.1.31 |
|        |                     |                                       |                   |
#### **21.4.1.1 I2C\_REVNB\_LO Register (offset = 0h) [reset = 0h]**

I2C\_REVNB\_LO is shown in [Figure](#page-19-1) 21-16 and described in [Table](#page-19-2) 21-9.

This read-only register contains the hard-coded revision number of the module. A write to this register has no effect. I2C controller with interrupt using interrupt vector register (I2C\_IV) is revision 1.x. I2C controller with interrupt using status register bits (I2C\_IRQSTATUS\_RAW) is revision 2.x.

**Figure 21-16. I2C\_REVNB\_LO Register**

|          | 31 | 30 | 29   | 28 | 27 | 26 | 25    | 24 | 23 | 22     | 21 | 20 | 19 | 18    | 17 | 16 |
|----------|----|----|------|----|----|----|-------|----|----|--------|----|----|----|-------|----|----|
| RESERVED |    |    |      |    |    |    |       |    |    |        |    |    |    |       |    |    |
| R-0h     |    |    |      |    |    |    |       |    |    |        |    |    |    |       |    |    |
|          | 15 | 14 | 13   | 12 | 11 | 10 | 9     | 8  | 7  | 6      | 5  | 4  | 3  | 2     | 1  | 0  |
|          |    |    | RTL  |    |    |    | MAJOR |    |    | CUSTOM |    |    |    | MINOR |    |    |
|          |    |    | R-0h |    |    |    | R-0h  |    |    | R-0h   |    |    |    | R-0h  |    |    |
|          |    |    |      |    |    |    |       |    |    |        |    |    |    |       |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-9. I2C\_REVNB\_LO Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                       |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                   |
| 15-11 | RTL      | R    | 0h    | RTL version.                                                                                                                                                      |
| 10-8  | MAJOR    | R    | 0h    | Major Revision.<br>This field changes when there is a major feature change.<br>This field does not change due to bug fix, or minor feature change.                |
| 7-6   | CUSTOM   | R    | 0h    | Indicates a special version for a particular device.<br>Consequence of use may avoid use of standard Chip Support<br>Library (CSL) / Drivers.<br>0 if non-custom. |
| 5-0   | MINOR    | R    | 0h    | Minor Revision This field changes when features are scaled up or<br>down.<br>This field does not change due to bug fix, or major feature change.                  |
#### **21.4.1.2 I2C\_REVNB\_HI Register (offset = 4h) [reset = 0h]**

I2C\_REVNB\_HI is shown in [Figure](#page-20-1) 21-17 and described in Table [21-10](#page-20-2).

A reset has no effect on the value returned.

### **Figure 21-17. I2C\_REVNB\_HI Register**

| 31       | 30       | 29 | 28       | 27   | 26   | 25 | 24
|----------|----------|----|----------|------|------|----|----|--|--
| RESERVED |          |    |          |      |      |    |
|          | R-0h     |    |          |      |      |    |
| 23       | 22       | 21 | 20       | 19   | 18   | 17 | 16
|          | RESERVED |    |          |      |      |    |
|          | R-0h     |    |          |      |      |    |
| 15       | 14       | 13 | 12       | 11   | 10   | 9  | 8
| SCHEME   |          |    | RESERVED | FUNC |      |    |
| R-0h     |          |    | R-0h     |      | R-0h |    |
| 7        | 6        | 5  | 4        | 3    | 2    | 1  | 0
|          |          |    | FUNC     |      |      |    |
|          |          |    | R-0h     |      |      |    |
|          |          |    |          |      |      |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 21-10. I2C\_REVNB\_HI Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                |
|-------|----------|------|-------|--------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                            |
| 15-14 | SCHEME   | R    | 0h    | Used to distinguish between old Scheme and current.<br>Spare bit to encode future schemes. |
| 13-12 | RESERVED | R    | 0h    |                                                                                            |
| 11-0  | FUNC     | R    | 0h    | Function: Indicates a software compatible module family                                    |

*I2C*
#### **21.4.1.3 I2C\_SYSC Register (offset = 10h) [reset = 0h]**

I2C\_SYSC is shown in [Figure](#page-21-1) 21-18 and described in Table [21-11](#page-21-2).

This register allows controlling various parameters of the peripheral interface.

**Figure 21-18. I2C\_SYSC Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 21-11. I2C\_SYSC Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|-------|-------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED    | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 9-8   | CLKACTIVITY | R/W  | 0h    | Clock Activity selection bits.<br>Those bits (one bit for each clock signal present on the boundary of<br>the module) are set to 1 to disable external clock gating mechanism<br>in Idle Mode.<br>Values after reset are low (for both 2 bits).<br>Note: If the System (functional) Clock is cut-off, the module will<br>assert a WakeUp event when it asynchronously detects a Start<br>Condition on the I2C Bus.<br>Note that in this case the first transfer will not be taken into account<br>by the module (NACK will be detected by the external master).<br>0h = Both clocks can be cut off<br>1h = Only Interface/OCP clock must be kept active; system clock<br>can be cut off<br>2h = Only system clock must be kept active; Interface/OCP clock<br>can be cut off<br>3h = Both clocks must be kept active |
| 7-5   | RESERVED    | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 4-3   | IDLEMODE    | R/W  | 0h    | Idle Mode selection bits.<br>These two bits are used to select one of the idle mode operation<br>mechanisms.<br>Value after reset is 00 (Force Idle).<br>1h = No Idle mode<br>2h = Smart Idle mode<br>3h = Smart-idle wakeup. Available only on I2C0.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 2     | ENAWAKEUP   | R/W  | 0h    | Enable Wakeup control bit.<br>When this bit is set to 1, the module enables its own wakeup<br>mechanism.<br>Value after reset is low.<br>0h = Wakeup mechanism is disabled<br>1h = Wakeup mechanism is enabled                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
### **Table 21-11. I2C\_SYSC Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                               |
|-----|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1   | SRST     | R/W  | 0h    | SoftReset bit.<br>When this bit is set to 1, entire module is reset as for the hardware<br>reset.<br>This bit is automatically cleared to 0 by the core and it is only reset<br>by the hardware reset.<br>During reads, it always returns 0.<br>Value after reset is low.<br>0h = Normal mode<br>1h = The module is reset                                                 |
| 0   | AUTOIDLE | R/W  | 0h    | Autoidle bit.<br>When this bit is set to 1, the module activates its own idle mode<br>mechanism.<br>By evaluating its internal state, the module can decide to gate part of<br>his internal clock tree in order to improve the overall power<br>consumption.<br>Value after reset is high.<br>0h = Auto Idle mechanism is disabled<br>1h = Auto Idle mechanism is enabled |

*I2C*
#### **21.4.1.4 I2C\_IRQSTATUS\_RAW Register (offset = 24h) [reset = 0h]**

I2C\_IRQSTATUS\_RAW is shown in [Figure](#page-23-1) 21-19 and described in Table [21-12](#page-23-2).

This register provides core status information for interrupt handling, showing all active events (enabled and not enabled). The fields are read-write. Writing a 1 to a bit will set it to 1, that is, trigger the IRQ (mostly for debug). Writing a 0 will have no effect, that is, the register value will not be modified. Only enabled, active events will trigger an actual interrupt request on the IRQ output line.

**Figure 21-19. I2C\_IRQSTATUS\_RAW Register**

| 31       | 30       | 29     | 28     | 27       | 26     | 25     | 24
|----------|----------|--------|--------|----------|--------|--------|--------|--|--
|          | RESERVED |        |        |          |        |        |
|          | R-0h     |        |        |          |        |        |
| 23       | 22       | 21     | 20     | 19       | 18     | 17     | 16
|          |          |        |        | RESERVED |        |        |
|          |          |        |        | R-0h     |        |        |
| 15       | 14       | 13     | 12     | 11       | 10     | 9      | 8
| RESERVED | XDR      | RDR    | BB     | ROVR     | XUDF   | AAS    | BF
| R-0h     | R/W-0h   | R/W-0h | R-0h   | R/W-0h   | R/W-0h | R/W-0h | R/W-0h
| 7        | 6        | 5      | 4      | 3        | 2      | 1      | 0
| AERR     | STC      | GC     | XRDY   | RRDY     | ARDY   | NACK   | AL
| R/W-0h   | R/W-0h   | R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-12. I2C\_IRQSTATUS\_RAW Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-15 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 14    | XDR      | R/W  | 0h    | Transmit draining IRQ status.<br>I2C Master Transmit mode only.<br>This read/clear only bit is set to 1 when the module is configured as<br>a master transmitter, the TX FIFO level is below the configured<br>threshold (TXTRSH) and the amount of data still to be transferred is<br>less than TXTRSH.<br>When this bit is set to 1 by the core, CPU must read the<br>I2C_BUFSTAT.TXSTAT register in order to check the amount of<br>data that need to be written in the TX FIFO.<br>Then, according to the mode set (DMA or interrupt), the CPU can<br>enable the DMA draining feature of the DMA controller with the<br>number of data bytes to be transferred (I2C_BUFSTAT.TXSTAT), or<br>generate write data accesses according to this value (IRQ mode).<br>The interrupt needs to be cleared after the DMA controller was<br>reconfigured (if DMA mode enabled), or before generating data<br>accesses to the FIFO (if IRQ mode enabled).<br>If the corresponding interrupt was enabled, an interrupt is signaled to<br>the local host.<br>The CPU can also poll this bit.<br>For more details about TDR generation, refer to the FIFOManagement subsection.<br>The CPU can only clear this bit by writing a 1 into this register.<br>A write 0 has no effect.<br>Value after reset is low.<br>0h = Transmit draining inactive<br>1h = Transmit draining enabled |
### **Table 21-12. I2C\_IRQSTATUS\_RAW Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|-----|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 13  | RDR   | R/W  | 0h    | Receive draining IRQ status.<br>I2C Receive mode only.<br>This read/clear only bit is set to 1 when the module is configured as<br>a receiver, a stop condition was received on the bus and the RXFIFO level is below the configured threshold (RXTRSH).<br>When this bit is set to 1 by the core, CPU must read the<br>I2C_BUFSTAT.RXSTAT register in order to check the amount of<br>data left to be transferred from the FIFO.<br>Then, according to the mode set (DMA or interrupt), the CPU needs<br>to enable the draining feature of the DMA controller with the number<br>of data bytes to be transferred (I2C_BUFSTAT.RXSTAT), or<br>generate read data accesses according to this value (IRQ mode).<br>The interrupt needs to be cleared after the DMA controller was<br>reconfigured (if DMA mode enabled), or before generating data<br>accesses to the FIFO (if IRQ mode enabled).<br>If the corresponding interrupt was enabled, an interrupt is signaled to<br>the local host.<br>The CPU can also poll this bit.<br>For more details about RDR generation, refer to the FIFOManagement subsection.<br>The CPU can only clear this bit by writing a 1 into this register.<br>A write 0 has no effect.<br>Value after reset is low.<br>0h = Receive draining inactive<br>1h = Receive draining enabled |
| 12  | BB    | R    | 0h    | This read-only bit indicates the state of the serial bus.<br>In slave mode, on reception of a start condition, the device sets BB<br>to 1.<br>BB is cleared to 0 after reception of a stop condition.<br>In master mode, the software controls BB.<br>To start a transmission with a start condition, MST, TRX, and STT<br>must be set to 1 in the I2C_CON register.<br>To end a transmission with a stop condition, STP must be set to 1 in<br>the I2C_CON register.<br>When BB = 1 and STT = 1, a restart condition is generated.<br>Value after reset is low.<br>0h = Bus is free<br>1h = Bus is occupied                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 11  | ROVR  | R/W  | 0h    | Receive overrun status.<br>Writing into this bit has no effect.<br>I2C receive mode only.<br>This read-only bit indicates whether the receiver has experienced<br>overrun.<br>Overrun occurs when the shift register is full and the receive FIFO is<br>full.<br>An overrun condition does not result in a data loss<br>the peripheral is just holding the bus (low on SCL) and prevents<br>other bytes from being received.<br>ROVR is set to 1 when the I2C has recognized an overrun.<br>ROVR is clear when reading I2C_DATA register, or when resetting<br>the I2C (I2C_CON:I2C_EN = 0).<br>Value after reset is low.<br>0h = Normal operation<br>1h = Receiver overrun                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
### **Table 21-12. I2C\_IRQSTATUS\_RAW Register Field Descriptions (continued)**

| Field | Type        | Reset      | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-------|-------------|------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|       |             |            | Transmit underflow status.<br>Writing into this bit has no effect.<br>I2C transmit mode only.<br>This read-only bit indicates whether the transmitter has experienced<br>underflow.<br>In master transmit mode, underflow occurs when the shift register is<br>empty, the transmit FIFO is empty, and there are still some bytes to<br>transmit (DCOUNT 0).<br>In slave transmit mode, underflow occurs when the shift register is<br>empty, the transmit FIFO is empty, and there are still some bytes to<br>transmit (read request from external I2C master).<br>XUDF is set to 1 when the I2C has recognized an underflow.<br>The core holds the line till the underflow cause has disappeared.<br>XUDF is clear when writing I2C_DATA register or resetting the I2C<br>(I2C_CON:I2C_EN = 0).<br>Value after reset is low.<br>0h = Normal operation<br>1h = Transmit underflow |
|       |             |            |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
|       |             |            | Address recognized as slave IRQ status.<br>I2C mode only.<br>This read only bit is set to 1 by the device when it has recognized its<br>own slave address (or one of the alternative own addresses), or an<br>address of all zeros (8 bits).<br>When this bit is set to 1 by the core, an interrupt is signaled to the<br>local host if the interrupt was enabled.<br>This bit can be cleared in 2 ways: One way is if the interrupt was<br>enabled, it will be cleared by writing 1 into this register (writing 0 has<br>no effect).<br>The other way is if the interrupt was not enabled, the AAS bit is reset<br>to 0 by restart or stop.<br>Value after reset is low.<br>0h = No action                                                                                                                                                                                       |
|       |             |            | 1h = Address recognized                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| BF    | R/W         | 0h         | I2C mode only.<br>This read only bit is set to 1 by the device when the I2C bus became<br>free (after a transfer is ended on the bus stop condition detected).<br>This interrupt informs the Local Host that it can initiate its own I2C<br>transfer on the bus.<br>When this bit is set to 1 by the core, an interrupt is signaled to the<br>local host if the interrupt was enabled.<br>The CPU can only clear this bit by writing a 1 into this register.<br>Writing 0 has no effect.<br>Value after reset is low.                                                                                                                                                                                                                                                                                                                                                             |
|       |             |            | 0h = No action                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|       |             |            | 1h = Bus Free                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| AERR  | R/W         | 0h         | Access Error IRQ status.<br>I2C mode only.<br>This read/clear only bit is set to 1 by the device if an Interface/OCP<br>write access is performed to I2C_DATA while the TX FIFO is full or if<br>an Interface/OCP read access is performed to the I2C_DATA while<br>the RX FIFO is empty.<br>Note that, when the RX FIFO is empty, a read access will return to<br>the previous read data value.<br>When the TX FIFO is full, a write access is ignored.<br>In both events, the FIFO pointers will not be updated.<br>When this bit is set to 1 by the core, an interrupt is signaled to the<br>local host if the interrupt was enabled.<br>The CPU can only clear this bit by writing a 1 into this register.<br>Writing 0 has no effect.<br>Value after reset is low.<br>0h = No action<br>1h = Access Error                                                                    |
|       | XUDFAAS | R/WR/W | 0h<br>0h                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
### **Table 21-12. I2C\_IRQSTATUS\_RAW Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|-----|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 6   | STC   | R/W  | 0h    | Start Condition IRQ status.<br>I2C mode only.<br>This read/clear only bit is set to 1 by the device if previously the<br>module was in idle mode and a start condition was asynchronously<br>detected on the I2C Bus and signalized with an Wakeup (if the<br>I2C_SYSC.ClockActivity allows the system clock to be cut-off).<br>When the Active Mode will be restored and the interrupt generated,<br>this bit will indicate the reason of the wakeup.<br>Note<br>1: The corresponding interrupt for this bit should be enabled only if<br>the module was configured to allow the possibility of cutting-off the<br>system clock while in Idle State (I2C_SYSC.ClockActivity = 00 or<br>01).<br>Note<br>2: The first transfer (corresponding to the detected start condition)<br>will be lost (not taken into account by the module) and it will be used<br>only for generating the WakeUp enable for restoring the Active Mode<br>of the module.<br>On the I2C line, the external master which generated the transfer will<br>detect this behavior as a not acknowledge to the address phase and<br>will possibly restart the transfer.<br>The CPU can only clear this bit by writing a 1 into this register.<br>Writing 0 has no effect.<br>Value after reset is low.<br>0h = No action<br>1h = Start Condition detected |
| 5   | GC    | R/W  | 0h    | General call IRQ status.<br>Set to '1' by core when General call address detected and interrupt<br>signaled to MPUSS.<br>Write '1' to clear.<br>I2C mode only.<br>This read/clear only bit is set to 1 by the device if it detects the<br>address of all zeros (8 bits) (general call).<br>When this bit is set to 1 by the core, an interrupt is signaled to the<br>local host if the interrupt was enabled.<br>The CPU can only clear this bit by writing a 1 into this register.<br>Writing 0 has no effect.<br>Note: When this bit is set to 1, AAS also reads as 1.<br>Value after reset is low.<br>0h = No general call detected<br>1h = General call address detected                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |

*I2C*
**Table 21-12. I2C\_IRQSTATUS\_RAW Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
|-----|-------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 4   | XRDY  | R/W  | 0h    | Transmit data ready IRQ status.<br>Set to '1' by core when transmitter and when new data is requested.<br>When set to '1' by core, an interrupt is signaled to MPUSS.<br>Write '1' to clear.<br>Transmit mode only (I2C mode).<br>This read/clear only bit (XRDY) is set to 1 when the I2C peripheral is<br>a master or slave transmitter, the CPU needs to send data through<br>the I2C bus, and the module (transmitter) requires new data to be<br>served.<br>Note that a master transmitter requests new data if the FIFO TX<br>level is below the threshold (TXTRSH) and the required amount of<br>data remained to be transmitted (I2C_BUFSTAT.TXSTAT) is greater<br>than the threshold.<br>A slave transmitter requests new data when the FIFO TX level is<br>below the threshold (if TXTRSH > 1), or anytime there is a read<br>request from external master (for each acknowledge received from<br>the master), if TXTRSH = 1.<br>When this bit is set to 1 by the core, an interrupt is signaled to the<br>local host if the interrupt was enabled.<br>The CPU can also poll this bit (refer to the FIFO Management<br>subsection for details about XRDY generation).<br>The CPU can only clear this bit by writing a 1 into this register.<br>Writing 0 has no effect.<br>Note: If the DMA transmit mode is enabled (I2C_BUF.XDMA_EN is<br>set, together with I2C_DMATXENABLE_SET), this bit is forced to 0<br>and no interrupt will be generated<br>instead, a DMA TX request to the main DMA controller of the system<br>is generated.<br>Value after reset is low.<br>0h = Transmission ongoing<br>1h = Transmit data ready |
| 3   | RRDY  | R/W  | 0h    | Receive mode only (I2C mode).<br>This read/clear only RRDY is set to 1 when the RX FIFO level is<br>above the configured threshold (RXTRSH).<br>When this bit is set to 1 by the core, CPU is able to read new data<br>from the I2C_DATA register.<br>If the corresponding interrupt was enabled, an interrupt is signaled to<br>the local host.<br>The CPU to read the received data in I2C_DATA register can also<br>poll this bit (refer to the FIFO Management subsection for details<br>about RRDY generation).<br>The CPU can only clear this bit by writing a 1 into this register.<br>A write 0 has no effect.<br>If the DMA receive mode is enabled (I2C_BUF.RDMA_EN is set,<br>together with I2C_DMARXENABLE_SET), this bit is forced to 0 and<br>no interrupt will be generated<br>instead a DMA RX request to the main DMA controller of the system<br>is generated.<br>Value after reset is low.<br>0h = Receive FIFO threshold not reached<br>1h = Receive data ready for read (RX FIFO threshold reached)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
**Table 21-12. I2C\_IRQSTATUS\_RAW Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|-----|-------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|     |       |      |       |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| 2   | ARDY  | R/W  | 0h    | I2C mode only.<br>This read/clear only bit, when set to 1, indicates that the previously<br>programmed data and command (receive or transmit, master or<br>slave) has been performed and status bit has been updated.<br>The CPU uses this flag to let it know that the I2C registers are ready<br>to be accessed again.<br>The CPU can only clear this bit by writing a 1 into this register.<br>A write 0 has no effect.<br>Mode: I2C Master transmit, Others: STP = 1, ARDY Set Condition:<br>DCOUNT = 0.<br>Mode: I2C Master receive, Others: STP = 1, ARDY Set Condition:<br>DCOUNT = 0 and receiver FIFO empty Mode: I2C Master transmit,<br>Others: STP = 0, ARDY Set Condition: DCOUNT passed 0 Mode:<br>I2C Master receive, Others: STP = 0, ARDY Set Condition:<br>DCOUNT passed 0 and receiver FIFO empty Mode: I2C Master<br>transmit, Others: n/a, ARDY Set Condition: Stop or restart condition<br>received from master Mode: I2C Slave receive, Others: n/a, ARDYSet Condition: Stop or restart condition and receiver FIFO empty<br>Value after reset is low.<br>0h = No action<br>1h = Access ready |
| 1   | NACK  | R/W  | 0h    | No acknowledgment IRQ status.<br>Bit is set when No Acknowledge has been received, an interrupt is                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|     |       |      |       | signaled to MPUSS.<br>Write '1' to clear this bit.<br>I2C mode only.<br>The read/clear only No Acknowledge flag bit is set when the<br>hardware detects No Acknowledge has been received.<br>When a NACK event occurs on the bus, this bit is set to 1, the core<br>automatically ends the transfer and clears the MST/STP bits in the<br>I2C_CON register and the I2C becomes a slave.<br>Clearing the FIFOs from remaining data might be required.<br>The CPU can only clear this bit by writing a 1 into this register.<br>Writing 0 has no effect.<br>Value after reset is low.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|     |       |      |       | 0h = Normal operation<br>1h = Not Acknowledge detected                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 0   | AL    | R/W  | 0h    | Arbitration lost IRQ status.<br>This bit is automatically set by the hardware when it loses the<br>Arbitration in master transmit mode, an interrupt is signaled to<br>MPUSS.<br>During reads, it always returns 0.<br>I2C mode only.<br>The read/clear only Arbitration Lost flag bit is set to 1 when the<br>device (configured in master mode) detects it has lost an arbitration<br>(in Address Phase).<br>This happens when two or more masters initiate a transfer on the<br>I2C bus almost simultaneously or when the I2C attempts to start a<br>transfer while BB (bus busy) is 1.<br>When this is set to 1 due to arbitration lost, the core automatically<br>clears the MST/STP bits in the I2C_CON register and the I2C<br>becomes a slave receiver.<br>The CPU can only clear this bit by writing a 1 to this register.<br>Writing 0 has no effect.<br>Value after reset is low.<br>0h = Normal operation<br>1h = Arbitration lost detected                                                                                                                                                                  |
#### **21.4.1.5 I2C\_IRQSTATUS Register (offset = 28h) [reset = 0h]**

I2C\_IRQSTATUS is shown in [Figure](#page-29-1) 21-20 and described in Table [21-13](#page-29-2).

This register provides core status information for interrupt handling, showing all active and enabled events and masking the others. The fields are read-write. Writing a 1 to a bit will clear it to 0, that is, clear the IRQ. Writing a 0 will have no effect, that is, the register value will not be modified. Only enabled, active events will trigger an actual interrupt request on the IRQ output line. For all the internal fields of the I2C\_IRQSTATUS register, the descriptions given in the I2C\_IRQSTATUS\_RAW subsection are valid.

**Figure 21-20. I2C\_IRQSTATUS Register**

| 31       | 30       | 29     | 28     | 27       | 26     | 25     | 24
|----------|----------|--------|--------|----------|--------|--------|--------|--|--
|          | RESERVED |        |        |          |        |        |
|          | R-0h     |        |        |          |        |        |
| 23       | 22       | 21     | 20     | 19       | 18     | 17     | 16
|          |          |        |        | RESERVED |        |        |
|          | R-0h     |        |        |          |        |        |
| 15       | 14       | 13     | 12     | 11       | 10     | 9      | 8
| RESERVED | XDR      | RDR    | BB     | ROVR     | XUDF   | AAS    | BF
| R-0h     | R/W-0h   | R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h
| 7        | 6        | 5      | 4      | 3        | 2      | 1      | 0
| AERR     | STC      | GC     | XRDY   | RRDY     | ARDY   | NACK   | AL
| R/W-0h   | R/W-0h   | R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-13. I2C\_IRQSTATUS Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                    |
|-------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------|
| 31-15 | RESERVED | R    | 0h    |                                                                                                                                |
| 14    | XDR      | R/W  | 0h    | Transmit draining IRQ enabled status.<br>0h = Transmit draining inactive<br>1h = Transmit draining enabled                     |
| 13    | RDR      | R/W  | 0h    | Receive draining IRQ enabled status.<br>0h = Receive draining inactive<br>1h = Receive draining enabled                        |
| 12    | BB       | R/W  | 0h    | Bus busy enabled status.<br>Writing into this bit has no effect.<br>0h = Bus is free<br>1h = Bus is occupied                   |
| 11    | ROVR     | R/W  | 0h    | Receive overrun enabled status.<br>Writing into this bit has no effect.<br>0h = Normal operation<br>1h = Receiver overrun      |
| 10    | XUDF     | R/W  | 0h    | Transmit underflow enabled status.<br>Writing into this bit has no effect.<br>0h = Normal operation<br>1h = Transmit underflow |
| 9     | AAS      | R/W  | 0h    | Address recognized as slave IRQ enabled status.<br>0h = No action<br>1h = Address recognized                                   |
| 8     | BF       | R/W  | 0h    | Bus Free IRQ enabled status.<br>0h = No action<br>1h = Bus free                                                                |
| 7     | AERR     | R/W  | 0h    | Access Error IRQ enabled status.<br>0h = No action<br>1h = Access error                                                        |
### **Table 21-13. I2C\_IRQSTATUS Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                            |
|-----|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 6   | STC   | R/W  | 0h    | Start Condition IRQ enabled status.<br>0h = No action<br>1h = Start condition detected                                                                                                                                                                                                 |
| 5   | GC    | R/W  | 0h    | General call IRQ enabled status.<br>Set to '1' by core when General call address detected and interrupt<br>signaled to MPUSS.<br>Write '1' to clear.<br>0h = No general call detected                                                                                                  |
|     |       |      |       | 1h = General call address detected                                                                                                                                                                                                                                                     |
| 4   | XRDY  | R/W  | 0h    | Transmit data ready IRQ enabled status.<br>Set to '1' by core when transmitter and when new data is requested.<br>When set to '1' by core, an interrupt is signaled to MPUSS.<br>Write '1' to clear.                                                                                   |
|     |       |      |       | 0h = Transmission ongoing<br>1h = Transmit data ready                                                                                                                                                                                                                                  |
| 3   | RRDY  | R/W  | 0h    | Receive data ready IRQ enabled status.<br>Set to '1' by core when receiver mode, a new data is able to be read.<br>When set to '1' by core, an interrupt is signaled to MPUSS.<br>Write '1' to clear.                                                                                  |
|     |       |      |       | 0h = No data available                                                                                                                                                                                                                                                                 |
|     |       |      |       | 1h = Receive data available                                                                                                                                                                                                                                                            |
| 2   | ARDY  | R/W  | 0h    | Register access ready IRQ enabled status.<br>When set to '1' it indicates that previous access has been performed<br>and registers are ready to be accessed again.<br>An interrupt is signaled to MPUSS.<br>Write '1' to clear.                                                        |
|     |       |      |       | 0h = Module busy                                                                                                                                                                                                                                                                       |
|     |       |      |       | 1h = Access ready                                                                                                                                                                                                                                                                      |
| 1   | NACK  | R/W  | 0h    | No acknowledgment IRQ enabled status.<br>Bit is set when No Acknowledge has been received, an interrupt is<br>signaled to MPUSS.<br>Write '1' to clear this bit.                                                                                                                       |
|     |       |      |       | 0h = Normal operation                                                                                                                                                                                                                                                                  |
|     |       |      |       | 1h = Not Acknowledge detected                                                                                                                                                                                                                                                          |
| 0   | AL    | R/W  | 0h    | Arbitration lost IRQ enabled status.<br>This bit is automatically set by the hardware when it loses the<br>Arbitration in master transmit mode, an interrupt is signaled to<br>MPUSS.<br>During reads, it always returns 0.<br>0h = Normal operation<br>1h = Arbitration lost detected |
#### **21.4.1.6 I2C\_IRQENABLE\_SET Register (offset = 2Ch) [reset = 0h]**

I2C\_IRQENABLE\_SET is shown in [Figure](#page-31-1) 21-21 and described in Table [21-14](#page-31-2).

All 1-bit fields enable a specific interrupt event to trigger an interrupt request. Writing a 1 to a bit will enable the field. Writing a 0 will have no effect, that is, the register value will not be modified. For all the internal fields of the I2C\_IRQENABLE\_SET register, the descriptions given in the I2C\_IRQSTATUS\_RAW subsection are valid.

**Figure 21-21. I2C\_IRQENABLE\_SET Register**

| 31       | 30     | 29     | 28       | 27      | 26      | 25      | 24     |
|----------|--------|--------|----------|---------|---------|---------|--------|--
| RESERVED |        |        |          |         |         |         |        |
|          | R-0h   |        |          |         |         |         |        |
| 23       | 22     | 21     | 20       | 19      | 18      | 17      | 16     |
|          |        |        | RESERVED |         |         |         |        |
|          | R-0h   |        |          |         |         |         |        |
| 15       | 14     | 13     | 12       | 11      | 10      | 9       | 8      |
| RESERVED | XDR_IE | RDR_IE | RESERVED | ROVR    | XUDF    | AAS_IE  | BF_IE  |
| R-0h     | R/W-0h | R/W-0h | R-0h     | R/W-0h  | R/W-0h  | R/W-0h  | R/W-0h |
| 7        | 6      | 5      | 4        | 3       | 2       | 1       | 0      |
| AERR_IE  | STC_IE | GC_IE  | XRDY_IE  | RRDY_IE | ARDY_IE | NACK_IE | AL_IE  |
| R/W-0h   | R/W-0h | R/W-0h | R/W-0h   | R/W-0h  | R/W-0h  | R/W-0h  | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-14. I2C\_IRQENABLE\_SET Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                           |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-15 | RESERVED | R    | 0h    |                                                                                                                                                                                                       |
| 14    | XDR_IE   | R/W  | 0h    | Transmit draining interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[XDR].<br>0h = Transmit draining interrupt disabled<br>1h = Transmit draining interrupt enabled    |
| 13    | RDR_IE   | R/W  | 0h    | Receive draining interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[RDR].<br>0h = Receive draining interrupt disabled<br>1h = Receive draining interrupt enabled       |
| 12    | RESERVED | R    | 0h    |                                                                                                                                                                                                       |
| 11    | ROVR     | R/W  | 0h    | Receive overrun enable set.<br>0h = Receive overrun interrupt disabled<br>1h = Receive draining interrupt enabled                                                                                     |
| 10    | XUDF     | R/W  | 0h    | Transmit underflow enable set.<br>0h = Transmit underflow interrupt disabled<br>1h = Transmit underflow interrupt enabled                                                                             |
| 9     | AAS_IE   | R/W  | 0h    | Addressed as slave interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[AAS].<br>0h = Addressed as slave interrupt disabled<br>1h = Addressed as slave interrupt enabled |
| 8     | BF_IE    | R/W  | 0h    | Bus free interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[BF].<br>0h = Bus free interrupt disabled<br>1h = Bus free interrupt enabled                                |
| 7     | AERR_IE  | R/W  | 0h    | Access error interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[AERR].<br>0h = Access error interrupt disabled<br>1h = Access error interrupt enabled                  |
### **Table 21-14. I2C\_IRQENABLE\_SET Register Field Descriptions (continued)**

| Bit | Field   | Type | Reset | Description                                                                                                                                                                                                     |
|-----|---------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 6   | STC_IE  | R/W  | 0h    | Start condition interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[STC].<br>0h = Start condition interrupt disabled<br>1h = Start condition interrupt enabled                    |
| 5   | GC_IE   | R/W  | 0h    | General call interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[GC].<br>0h = General call interrupt disabled<br>1h = General call interrupt enabled                              |
| 4   | XRDY_IE | R/W  | 0h    | Transmit data ready interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[XRDY].<br>0h = Transmit data ready interrupt disabled<br>1h = Transmit data ready interrupt enabled       |
| 3   | RRDY_IE | R/W  | 0h    | Receive data ready interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[RRDY].<br>0h = Receive data ready interrupt disabled<br>1h = Receive data ready interrupt enabled          |
| 2   | ARDY_IE | R/W  | 0h    | Register access ready interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[ARDY].<br>0h = Register access ready interrupt disabled<br>1h = Register access ready interrupt enabled |
| 1   | NACK_IE | R/W  | 0h    | No acknowledgment interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[NACK].<br>0h = Not Acknowledge interrupt disabled<br>1h = Not Acknowledge interrupt enabled                 |
| 0   | AL_IE   | R/W  | 0h    | Arbitration lost interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[AL].<br>0h = Arbitration lost interrupt disabled<br>1h = Arbitration lost interrupt enabled                  |
#### **21.4.1.7 I2C\_IRQENABLE\_CLR Register (offset = 30h) [reset = 0h]**

I2C\_IRQENABLE\_CLR is shown in [Figure](#page-33-1) 21-22 and described in Table [21-15.](#page-33-2)

All 1-bit fields clear a specific interrupt event. Writing a 1 to a bit will disable the interrupt field. Writing a 0 will have no effect, that is, the register value will not be modified. For all the internal fields of the I2C\_IRQENABLE\_CLR register, the descriptions given in the I2C\_IRQSTATUS\_RAW subsection are valid.

**Figure 21-22. I2C\_IRQENABLE\_CLR Register**

| 31       | 30     | 29     | 28       | 27      | 26      | 25      | 24     |
|----------|--------|--------|----------|---------|---------|---------|--------|--
| RESERVED |        |        |          |         |         |         |        |
|          | R-0h   |        |          |         |         |         |        |
| 23       | 22     | 21     | 20       | 19      | 18      | 17      | 16     |
|          |        |        | RESERVED |         |         |         |        |
|          | R-0h   |        |          |         |         |         |        |
| 15       | 14     | 13     | 12       | 11      | 10      | 9       | 8      |
| RESERVED | XDR_IE | RDR_IE | RESERVED | ROVR    | XUDF    | AAS_IE  | BF_IE  |
| R-0h     | R/W-0h | R/W-0h | R-0h     | R/W-0h  | R/W-0h  | R/W-0h  | R/W-0h |
| 7        | 6      | 5      | 4        | 3       | 2       | 1       | 0      |
| AERR_IE  | STC_IE | GC_IE  | XRDY_IE  | RRDY_IE | ARDY_IE | NACK_IE | AL_IE  |
| R/W-0h   | R/W-0h | R/W-0h | R/W-0h   | R/W-0h  | R/W-0h  | R/W-0h  | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-15. I2C\_IRQENABLE\_CLR Register Field Descriptions**

| Bit   | Field    | Type | Reset                                                                                                                                                                                                   | Description                                                                                                                                                                                             |
|-------|----------|------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-15 | RESERVED | R    | 0h                                                                                                                                                                                                      |                                                                                                                                                                                                         |
| 14    | XDR_IE   | R/W  | 0h                                                                                                                                                                                                      | Transmit draining interrupt enable clear.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[XDR].<br>0h = Transmit draining interrupt disabled<br>1h = Transmit draining interrupt enabled    |
| 13    | RDR_IE   | R/W  | 0h<br>Receive draining interrupt enable clear.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[RDR].<br>0h = Receive draining interrupt disabled<br>1h = Receive draining interrupt enabled |                                                                                                                                                                                                         |
| 12    | RESERVED | R    | 0h                                                                                                                                                                                                      |                                                                                                                                                                                                         |
| 11    | ROVR     | R/W  | 0h                                                                                                                                                                                                      | Receive overrun enable clear.<br>0h = Receive overrun interrupt disabled<br>1h = Receive draining interrupt enabled                                                                                     |
| 10    | XUDF     | R/W  | 0h                                                                                                                                                                                                      | Transmit underflow enable clear.<br>0h = Transmit underflow interrupt disabled<br>1h = Transmit underflow interrupt enabled                                                                             |
| 9     | AAS_IE   | R/W  | 0h                                                                                                                                                                                                      | Addressed as slave interrupt enable clear.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[AAS].<br>0h = Addressed as slave interrupt disabled<br>1h = Addressed as slave interrupt enabled |
| 8     | BF_IE    | R/W  | 0h                                                                                                                                                                                                      | Bus Free interrupt enable clear.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[BF].<br>0h = Bus free interrupt disabled<br>1h = Bus free interrupt enabled                                |
| 7     | AERR_IE  | R/W  | 0h                                                                                                                                                                                                      | Access error interrupt enable clear.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[AERR].<br>0h = Access error interrupt disabled<br>1h = Access error interrupt enabled                  |
### **Table 21-15. I2C\_IRQENABLE\_CLR Register Field Descriptions (continued)**

| Bit | Field   | Type | Reset | Description                                                                                                                                                                                                       |
|-----|---------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 6   | STC_IE  | R/W  | 0h    | Start condition interrupt enable clear.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[STC].<br>0h = Start condition interrupt disabled<br>1h = Start condition interrupt enabled                    |
| 5   | GC_IE   | R/W  | 0h    | General call interrupt enable clear.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[GC].<br>0h = General call interrupt disabled<br>1h = General call interrupt enabled                              |
| 4   | XRDY_IE | R/W  | 0h    | Transmit data ready interrupt enable clear.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[XRDY].<br>0h = Transmit data ready interrupt disabled<br>1h = Transmit data ready interrupt enabled       |
| 3   | RRDY_IE | R/W  | 0h    | Receive data ready interrupt enable set.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[RRDY]<br>0h = Receive data ready interrupt disabled<br>1h = Receive data ready interrupt enabled             |
| 2   | ARDY_IE | R/W  | 0h    | Register access ready interrupt enable clear.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[ARDY].<br>0h = Register access ready interrupt disabled<br>1h = Register access ready interrupt enabled |
| 1   | NACK_IE | R/W  | 0h    | No acknowledgment interrupt enable clear.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[NACK].<br>0h = Not Acknowledge interrupt disabled<br>1h = Not Acknowledge interrupt enabled                 |
| 0   | AL_IE   | R/W  | 0h    | Arbitration lost interrupt enable clear.<br>Mask or unmask the interrupt signaled by bit in I2C_STAT[AL].<br>0h = Arbitration lost interrupt disabled<br>1h = Arbitration lost interrupt enabled                  |
#### **21.4.1.8 I2C\_WE Register (offset = 34h) [reset = 0h]**

I2C\_WE is shown in [Figure](#page-35-1) 21-23 and described in Table [21-16](#page-35-2).

Every 1-bit field in the I2C\_WE register enables a specific (synchronous) IRQ request source to generate an asynchronous wakeup (on the appropriate swakeup line). When a bit location is set to 1 by the local host, a wakeup is signaled to the local host if the corresponding event is captured by the core of the I2C controller. Value after reset is low (all bits). There is no need for an Access Error WakeUp event, since this event occurs only when the module is in Active Mode (for Interface/OCP accesses to FIFO) and is signaled by an interrupt. With the exception of Start Condition WakeUp, which is asynchronously detected when the Functional clock is turned-off, all the other WakeUp events require the Functional (System) clock to be enabled.

**Figure 21-23. I2C\_WE Register**

| 31       | 30       | 29     | 28       | 27      | 26      | 25      | 24
|----------|----------|--------|----------|---------|---------|---------|--------
|          | RESERVED |        |          |         |         |         |
|          | R-0h     |        |          |         |         |         |
| 23       | 22       | 21     | 20       | 19      | 18      | 17      | 16
|          | RESERVED |        |          |         |         |         |
|          | R-0h     |        |          |         |         |         |
| 15       | 14       | 13     | 12       | 11      | 10      | 9       | 8
| RESERVED | XDR_WE   | RDR_WE | RESERVED | ROVR_WE | XUDF_WE | AAS_WE  | BF_WE
| R-0h     | R/W-0h   | R/W-0h | R-0h     | R/W-0h  | R/W-0h  | R/W-0h  | R/W-0h
| 7        | 6        | 5      | 4        | 3       | 2       | 1       | 0
| RESERVED | STC_WE   | GC_WE  | RESERVED | DRDY_WE | ARDY_WE | NACK_WE | AL_WE
| R-0h     | R/W-0h   | R/W-0h | R-0h     | R/W-0h  | R/W-0h  | R/W-0h  | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 21-16. I2C\_WE Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
|-------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-15 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 14    | XDR_WE   | R/W  | 0h    | Transmit draining wakeup enable.<br>This read/write bit is used to enable or disable wakeup signal<br>generation when I2C module is in idle mode, the TX FIFO level is<br>below the threshold and the amount of data left to be transferred is<br>less than TXTRSH value.<br>This allows for the module to inform the CPU that it can check the<br>amount of data to be written to the FIFO.<br>0h = Transmit draining wakeup disabled<br>1h = Transmit draining wakeup enabled                           |
| 13    | RDR_WE   | R/W  | 0h    | Receive draining wakeup enable.<br>This read/write bit is used to enable or disable wakeup signal<br>generation when I2C is in idle mode, configured as a receiver, and it<br>has detected a stop condition on the bus but the RX FIFO threshold<br>is not reached (but the FIFO is not empty).<br>This allows for the module to inform the CPU that it can check the<br>amount of data to be transferred from the FIFO.<br>0h = Receive draining wakeup disabled<br>1h = Receive draining wakeup enabled |
| 12    | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 11    | ROVR_WE  | R/W  | 0h    | Receive overrun wakeup enable<br>0h = Receive overrun wakeup disabled<br>1h = Receive overrun wakeup enabled                                                                                                                                                                                                                                                                                                                                                                                              |
| 10    | XUDF_WE  | R/W  | 0h    | Transmit underflow wakeup enable<br>0h = Transmit underflow wakeup disabled<br>1h = Transmit underflow wakeup enabled                                                                                                                                                                                                                                                                                                                                                                                     |
### **Table 21-16. I2C\_WE Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
|-----|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 9   | AAS_WE   | R/W  | 0h    | Address as slave IRQ wakeup enable.<br>This read/write bit is used to enable or disable wakeup signal<br>generation when I2C module is in idle mode, and external master<br>addresses the I2C module as a slave.<br>This allows for the module to inform the CPU that it can check which<br>of the own addresses was used by the external master to access the<br>I2C core.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
|     |          |      |       | 0h = Addressed as slave wakeup disabled<br>1h = Addressed as slave wakeup enabled                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 8   | BF_WE    | R/W  | 0h    | Bus free IRQ wakeup enable.<br>This read/write bit is used to enable or disable wakeup signal<br>generation when I2C module is in idle mode and the I2C bus<br>became free.<br>This allows for the module to inform the CPU that it can initiate its<br>own transfer on the I2C line.<br>0h = Bus Free wakeup disabled                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|     |          |      |       | 1h = Bus Free wakeup enabled                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 7   | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| 6   | STC_WE   | R/W  | 0h    | Start condition IRQ wakeup set.<br>This read/write bit is used to enable or disable wakeup signal<br>generation when I2C module is in idle mode (with the functional<br>clock inactive) and a possible start condition is detected on the I2C<br>line.<br>The STC WakeUp is generated only if the I2C_SYSC.ClockActivity<br>field indicates that the functional clock can be disabled.<br>Note that if the functional clock is not active, the start condition is<br>asynchronously detected (no filtering and synchronization is used).<br>For this reason, it is possible that the signalized start condition to be<br>a glitch.<br>If the functional clock cannot be disabled (I2C_SYSC.ClockActivity =<br>10 or 11), the programmer should not enable this wakeup, since the<br>module has other synchronously detected WakeUp event that might<br>be used to exit from idle mode, only if the detected transfer is<br>accessing the I2C module.<br>0h = Start condition wakeup disabled<br>1h = Start condition wakeup enabled |
| 5   | GC_WE    | R/W  | 0h    | General call IRQ wakeup enable.<br>This read/write bit is used to enable or disable wakeup signal<br>generation when I2C module is in idle mode and a general call is<br>received on I2C line.<br>0h = General call wakeup disabled<br>1h = General call wakeup enabled                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| 4   | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| 3   | DRDY_WE  | R/W  | 0h    | Receive/Transmit data ready IRQ wakeup enable.<br>This read/write bit is used to enable or disable wakeup signal<br>generation when I2C module is involved into a long transfer and no<br>more registers accesses are performed on the interface (for example<br>module are set in F/S I2C master transmitter mode and FIFO is full).<br>If in the middle of such a transaction, the FIFO buffer needs more<br>data to be transferred, CPU must be informed to write (in case of<br>transmitter mode) or read (if receiver mode) in/from the FIFO.<br>0h = Transmit/receive data ready wakeup disabled<br>1h = Transmit/receive data ready wakeup enabled                                                                                                                                                                                                                                                                                                                                                                           |
| 2   | ARDY_WE  | R/W  | 0h    | Register access ready IRQ wakeup enable.<br>This read/write bit is used to enable or disable wakeup signal<br>generation when I2C module is involved into a long transfer and no<br>more registers accesses are performed on the interface (for example<br>the module is set in F/S I2C master transmitter mode and FIFO is<br>full).<br>If the current transaction is finished, the module needs to inform<br>CPU about transmission completion.<br>0h = Register access ready wakeup disabled<br>1h = Register access ready wakeup enabled                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
**Table 21-16. I2C\_WE Register Field Descriptions (continued)**

| Bit | Field   | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|-----|---------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1   | NACK_WE | R/W  | 0h    | No acknowledgment IRQ wakeup enable.<br>This read/write bit is used to enable or disable wakeup signal<br>generation when I2C module is involved into a long transfer and no<br>more registers accesses are performed on the interface (for example<br>the module is set in F/S I2C master transmitter mode and FIFO is<br>full).<br>If in the middle of such of a transaction a Not Acknowledgment event<br>is raised, the module needs to inform CPU about transmission error.<br>0h = Not Acknowledge wakeup disabled<br>1h = Not Acknowledge wakeup enabled                                                                                                                                                                                                                                                                                                                                                          |
| 0   | AL_WE   | R/W  | 0h    | Arbitration lost IRQ wakeup enable.<br>This read/write bit is used to enable or disable wakeup signal<br>generation when I2C module is configured as a master and it loses<br>the arbitration.<br>This wake up is very useful when the module is configured as a<br>master transmitter, all the necessary data is provided in the FIFO Tx,<br>STT is enabled and the module enters in Idle Mode.<br>If the module loses the arbitration, an Arbitration Lost event is raised<br>and the module needs to inform CPU about transmission error.<br>Note: The AL wakeup must be enabled only for multimaster<br>communication.<br>If the AL_WE is not enabled and the scenario described above<br>occurs, the module will not be able to inform the CPU about the state<br>of the transfer and it will be blocked in an undetermined state.<br>0h = Arbitration lost wakeup disabled<br>1h = Arbitration lost wakeup enabled |
#### **21.4.1.9 I2C\_DMARXENABLE\_SET Register (offset = 38h) [reset = 0h]**

I2C\_DMARXENABLE\_SET is shown in [Figure](#page-38-1) 21-24 and described in Table [21-17.](#page-38-2)

The 1-bit field enables a receive DMA request. Writing a 1 to this field will set it to 1. Writing a 0 will have no effect, that is, the register value is not modified. Note that the I2C\_BUF.RDMA\_EN field is the global (slave) DMA enabler, and that it is disabled by default. The I2C\_BUF.RDMA\_EN field should also be set to 1 to enable a receive DMA request.

**Figure 21-24. I2C\_DMARXENABLE\_SET Register**

| 31   | 30       | 29 | 28   | 27       | 26 | 25 | 24
|------|----------|----|------|----------|----|----|--------
|      | RESERVED |    |      |          |    |    |
| R-0h |          |    |      |          |    |    |
| 23   | 22       | 21 | 20   | 19       | 18 | 17 | 16
|      |          |    |      | RESERVED |    |    |
|      | R-0h     |    |      |          |    |    |
| 15   | 14       | 13 | 12   | 11       | 10 | 9  | 8
|      |          |    |      | RESERVED |    |    |
|      |          |    |      | R-0h     |    |    |
| 7    | 6        | 5  | 4    | 3        | 2  | 1  | 0
|      | RESERVED |    |      |          |    |    |
|      |          |    | R-0h |          |    |    | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 21-17. I2C\_DMARXENABLE\_SET Register Field Descriptions**

| Bit  | Field            | Type | Reset | Description                     |
|------|------------------|------|-------|---------------------------------|
| 31-1 | RESERVED         | R    | 0h    |                                 |
| 0    | DMARX_ENABLE_SET | R/W  | 0h    | Receive DMA channel enable set. |

*I2C*
#### **21.4.1.10 I2C\_DMATXENABLE\_SET Register (offset = 3Ch) [reset = 0h]**

I2C\_DMATXENABLE\_SET is shown in [Figure](#page-39-1) 21-25 and described in Table [21-18.](#page-39-2)

The 1-bit field enables a transmit DMA request. Writing a 1 to this field will set it to 1. Writing a 0 will have no effect, that is, the register value is not modified. Note that the I2C\_BUF.XDMA\_EN field is the global (slave) DMA enabler, and that it is disabled by default. The I2C\_BUF.XDMA\_EN field should also be set to 1 to enable a transmit DMA request.

**Figure 21-25. I2C\_DMATXENABLE\_SET Register**

| 31   | 30       | 29 | 28       | 27 | 26 | 25 | 24     |
|------|----------|----|----------|----|----|----|--------|--
|      | RESERVED |    |          |    |    |    |        |
| R-0h |          |    |          |    |    |    |        |
| 23   | 22       | 21 | 20       | 19 | 18 | 17 | 16     |
|      |          |    | RESERVED |    |    |    |        |
| R-0h |          |    |          |    |    |    |        |
| 15   | 14       | 13 | 12       | 11 | 10 | 9  | 8      |
|      |          |    | RESERVED |    |    |    |        |
|      |          |    | R-0h     |    |    |    |        |
| 7    | 6        | 5  | 4        | 3  | 2  | 1  | 0      |
|      | RESERVED |    |          |    |    |    |        |
|      |          |    | R-0h     |    |    |    | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 21-18. I2C\_DMATXENABLE\_SET Register Field Descriptions**

| Bit  | Field              | Type | Reset | Description                      |
|------|--------------------|------|-------|----------------------------------|
| 31-1 | RESERVED           | R    | 0h    |                                  |
| 0    | DMATX_TRANSMIT_SET | R/W  | 0h    | Transmit DMA channel enable set. |
#### **21.4.1.11 I2C\_DMARXENABLE\_CLR Register (offset = 40h) [reset = 0h]**

I2C\_DMARXENABLE\_CLR is shown in [Figure](#page-40-1) 21-26 and described in Table [21-19](#page-40-2).

The 1-bit field disables a receive DMA request. Writing a 1 to a bit will clear it to 0. Another result of setting to 1 the DMARX\_ENABLE\_CLEAR field, is the reset of the DMA RX request and wakeup lines. Writing a 0 will have no effect, that is, the register value is not modified.

**Figure 21-26. I2C\_DMARXENABLE\_CLR Register**

| 31       | 30       | 29 | 28       | 27 | 26 | 25 | 24     |
|----------|----------|----|----------|----|----|----|--------|--
| RESERVED |          |    |          |    |    |    |        |
|          | R-0h     |    |          |    |    |    |        |
| 23       | 22       | 21 | 20       | 19 | 18 | 17 | 16     |
|          |          |    | RESERVED |    |    |    |        |
| R-0h     |          |    |          |    |    |    |        |
| 15       | 14       | 13 | 12       | 11 | 10 | 9  | 8      |
|          |          |    | RESERVED |    |    |    |        |
|          |          |    | R-0h     |    |    |    |        |
| 7        | 6        | 5  | 4        | 3  | 2  | 1  | 0      |
|          | RESERVED |    |          |    |    |    |        |
|          |          |    | R-0h     |    |    |    | R/W-0h |
|          |          |    |          |    |    |    |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-19. I2C\_DMARXENABLE\_CLR Register Field Descriptions**

| Bit  | Field                  | Type | Reset | Description                       |
|------|------------------------|------|-------|-----------------------------------|
| 31-1 | RESERVED               | R    | 0h    |                                   |
| 0    | DMARX_ENABLE_CLEAR | R/W  | 0h    | Receive DMA channel enable clear. |
#### **21.4.1.12 I2C\_DMATXENABLE\_CLR Register (offset = 44h) [reset = 0h]**

I2C\_DMATXENABLE\_CLR is shown in [Figure](#page-41-1) 21-27 and described in Table [21-20](#page-41-2).

The 1-bit field disables a transmit DMA request. Writing a 1 to a bit will clear it to 0. Another result of setting to 1 the DMATX\_ENABLE\_CLEAR field, is the reset of the DMA TX request and wakeup lines. Writing a 0 will have no effect, that is, the register value is not modified.

**Figure 21-27. I2C\_DMATXENABLE\_CLR Register**

| 31   | 30       | 29 | 28       | 27 | 26 | 25 | 24
|------|----------|----|----------|----|----|----|--------
|      |          |    | RESERVED |    |    |    |
| R-0h |          |    |          |    |    |    |
| 23   | 22       | 21 | 20       | 19 | 18 | 17 | 16
|      |          |    | RESERVED |    |    |    |
|      |          |    | R-0h     |    |    |    |
| 15   | 14       | 13 | 12       | 11 | 10 | 9  | 8
|      |          |    | RESERVED |    |    |    |
|      |          |    | R-0h     |    |    |    |
| 7    | 6        | 5  | 4        | 3  | 2  | 1  | 0
|      | RESERVED |    |          |    |    |    |
|      |          |    | R-0h     |    |    |    | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-20. I2C\_DMATXENABLE\_CLR Register Field Descriptions**

| Bit  | Field                  | Type | Reset | Description                       |
|------|------------------------|------|-------|-----------------------------------|
| 31-1 | RESERVED               | R    | 0h    |                                   |
| 0    | DMARX_ENABLE_CLEAR | R/W  | 0h    | Receive DMA channel enable clear. |
#### **21.4.1.13 I2C\_DMARXWAKE\_EN Register (offset = 48h) [reset = 0h]**

I2C\_DMARXWAKE\_EN is shown in [Figure](#page-42-1) 21-28 and described in Table [21-21](#page-42-2).

All 1-bit fields enable a specific (synchronous) DMA request source to generate an asynchronous wakeup (on the appropriate swakeup line). Note that the I2C\_SYSC.ENAWAKEUP field is the global (slave) wakeup enabler, and that it is disabled by default.

**Figure 21-28. I2C\_DMARXWAKE\_EN Register**

| 30                                                                                                                          | 29     | 28       | 27     | 26               | 25     | 24
|-----------------------------------------------------------------------------------------------------------------------------|--------|----------|--------|------------------|--------|--------|--|--|--|--
|                                                                                                                             |        |          |        |                  |        |
|                                                                                                                             |        |          |        |                  |        |
| 22                                                                                                                          | 21     | 20       | 19     | 18               | 17     | 16
|                                                                                                                             |        |          |        |                  |        |
|                                                                                                                             |        |          |        |                  |        |
| RESERVEDR-0h<br>1413-8RESERVED<br>XDRRDR<br>RESERVEDROVR<br>XUDFAAS<br>BF |        |          |        |                  |        |
|                                                                                                                             |        |          |        |                  |        |
| R/W-0h                                                                                                                      | R/W-0h | R-0h     | R/W-0h | R/W-0h           | R/W-0h | R/W-0h
| 6                                                                                                                           | 5      | 4        | 3      | 2                | 1      | 0
| STC                                                                                                                         | GC     | RESERVED | DRDY   | ARDY             | NACK   | AL
| R/W-0h                                                                                                                      | R/W-0h | R-0h     | R/W-0h | R/W-0h           | R/W-0h | R/W-0h
|                                                                                                                             |        |          |        | RESERVEDR-0h |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-21. I2C\_DMARXWAKE\_EN Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                 |
|-------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------|
| 31-15 | RESERVED | R    | 0h    |                                                                                                                             |
| 14    | XDR      | R/W  | 0h    | Transmit draining wakeup set.<br>0h = Transmit draining interrupt disabled<br>1h = Transmit draining interrupt enabled      |
| 13    | RDR      | R/W  | 0h    | Receive draining wakeup set.<br>0h = Receive draining interrupt disabled<br>1h = Receive draining interrupt enabled         |
| 12    | RESERVED | R    | 0h    |                                                                                                                             |
| 11    | ROVR     | R/W  | 0h    | Receive overrun wakeup set.<br>0h = Receive overrun interrupt disabled<br>1h = Receive draining interrupt enabled           |
| 10    | XUDF     | R/W  | 0h    | Transmit underflow wakeup set.<br>0h = Transmit underflow interrupt disabled<br>1h = Transmit underflow interrupt enabled   |
| 9     | AAS      | R/W  | 0h    | Address as slave IRQ wakeup set.<br>0h = Addressed as slave interrupt disabled<br>1h = Addressed as slave interrupt enabled |
| 8     | BF       | R/W  | 0h    | Bus free IRQ wakeup set.<br>0h = Bus free wakeup disabled<br>1h = Bus free wakeup enabled                                   |
| 7     | RESERVED | R    | 0h    |                                                                                                                             |
| 6     | STC      | R/W  | 0h    | Start condition IRQ wakeup set.<br>0h = Start condition wakeup disabled<br>1h = Start condition wakeup enabled              |
| 5     | GC       | R/W  | 0h    | General call IRQ wakeup set.<br>0h = General call wakeup disabled<br>1h = General call wakeup enabled                       |
| 4     | RESERVED | R    | 0h    |                                                                                                                             |
**Table 21-21. I2C\_DMARXWAKE\_EN Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                        |
|-----|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 3   | DRDY  | R/W  | 0h    | Receive/transmit data ready IRQ wakeup set.<br>0h = Transmit/receive data ready wakeup disabled<br>1h = Transmit/receive data ready wakeup enabled |
| 2   | ARDY  | R/W  | 0h    | Register access ready IRQ wakeup set.<br>0h = Register access ready wakeup disabled<br>1h = Register access ready wakeup enabled                   |
| 1   | NACK  | R/W  | 0h    | No acknowledgment IRQ wakeup set.<br>0h = Not Acknowledge wakeup disabled<br>1h = Not Acknowledge wakeup enabled                                   |
| 0   | AL    | R/W  | 0h    | Arbitration lost IRQ wakeup set.<br>0h = Arbitration lost wakeup disabled<br>1h = Arbitration lost wakeup enabled                                  |
#### **21.4.1.14 I2C\_DMATXWAKE\_EN Register (offset = 4Ch) [reset = 0h]**

I2C\_DMATXWAKE\_EN is shown in [Figure](#page-44-1) 21-29 and described in Table [21-22](#page-44-2).

All 1-bit fields enable a specific (synchronous) DMA request source to generate an asynchronous wakeup (on the appropriate swakeup line). Note that the I2C\_SYSC.ENAWAKEUP field is the global (slave) wakeup enabler, and that it is disabled by default.

**Figure 21-29. I2C\_DMATXWAKE\_EN Register**

| 31       | 30                                   | 29                                                                                                               | 28       | 27     | 26     | 25     | 24
|----------|--------------------------------------|------------------------------------------------------------------------------------------------------------------|----------|--------|--------|--------|--------|--|--|--|--|--|--
|          |                                      |                                                                                                                  | RESERVED |        |        |        |
|          |                                      |                                                                                                                  |          |        |        |        |
| 23       | 22                                   | 21                                                                                                               | 20       | 19     | 18     | 17     | 16
|          |                                      |                                                                                                                  |          |        |        |        |
|          | R-0h                                 |                                                                                                                  |          |        |        |        |
| 15       | 1413-8 |                                                                                                                  |          |        |        |        |
| RESERVED | XDR                                  | R-0h<br>RESERVEDRDR<br>RESERVEDROVR<br>XUDFAAS<br>BFR/W-0h<br>R-0h<br>R/W-0h<br>R/W-0h<br>R/W-0h |          |        |        |        |
| R-0h     | R/W-0h                               |                                                                                                                  |          |        |        |        | R/W-0h
| 7        | 6                                    | 5                                                                                                                | 4        | 3      | 2      | 1      | 0
| RESERVED | STC                                  | GC                                                                                                               | RESERVED | DRDY   | ARDY   | NACK   | AL
| R-0h     | R/W-0h                               | R/W-0h                                                                                                           | R-0h     | R/W-0h | R/W-0h | R/W-0h | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 21-22. I2C\_DMATXWAKE\_EN Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                 |
|-------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------|
| 31-15 | RESERVED | R    | 0h    |                                                                                                                             |
| 14    | XDR      | R/W  | 0h    | Transmit draining wakeup set.<br>0h = Transmit draining interrupt disabled<br>1h = Transmit draining interrupt enabled      |
| 13    | RDR      | R/W  | 0h    | Receive draining wakeup set.<br>0h = Receive draining interrupt disabled<br>1h = Receive draining interrupt enabled         |
| 12    | RESERVED | R    | 0h    |                                                                                                                             |
| 11    | ROVR     | R/W  | 0h    | Receive overrun wakeup set.<br>0h = Receive overrun interrupt disabled<br>1h = Receive draining interrupt enabled           |
| 10    | XUDF     | R/W  | 0h    | Transmit underflow wakeup set.<br>0h = Transmit underflow interrupt disabled<br>1h = Transmit underflow interrupt enabled   |
| 9     | AAS      | R/W  | 0h    | Address as slave IRQ wakeup set.<br>0h = Addressed as slave interrupt disabled<br>1h = Addressed as slave interrupt enabled |
| 8     | BF       | R/W  | 0h    | Bus free IRQ wakeup set.<br>0h = Bus free wakeup disabled<br>1h = Bus free wakeup enabled                                   |
| 7     | RESERVED | R    | 0h    |                                                                                                                             |
| 6     | STC      | R/W  | 0h    | Start condition IRQ wakeup set.<br>0h = Start condition wakeup disabled<br>1h = Start condition wakeup enabled              |
| 5     | GC       | R/W  | 0h    | General call IRQ wakeup set.<br>0h = General call wakeup disabled<br>1h = General call wakeup enabled                       |
| 4     | RESERVED | R    | 0h    |                                                                                                                             |
**Table 21-22. I2C\_DMATXWAKE\_EN Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                        |
|-----|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 3   | DRDY  | R/W  | 0h    | Receive/transmit data ready IRQ wakeup set.<br>0h = Transmit/receive data ready wakeup disabled<br>1h = Transmit/receive data ready wakeup enabled |
| 2   | ARDY  | R/W  | 0h    | Register access ready IRQ wakeup set.<br>0h = Register access ready wakeup disabled<br>1h = Register access ready wakeup enabled                   |
| 1   | NACK  | R/W  | 0h    | No acknowledgment IRQ wakeup set.<br>0h = Not Acknowledge wakeup disabled<br>1h = Not Acknowledge wakeup enabled                                   |
| 0   | AL    | R/W  | 0h    | Arbitration lost IRQ wakeup set.<br>0h = Arbitration lost wakeup disabled<br>1h = Arbitration lost wakeup enabled                                  |
#### **21.4.1.15 I2C\_SYSS Register (offset = 90h) [reset = 0h]**

I2C\_SYSS is shown in [Figure](#page-46-1) 21-30 and described in Table [21-23.](#page-46-2)

### **Figure 21-30. I2C\_SYSS Register**

| 31 | 30 | 29                                                 | 28       | 27       | 26 | 25 | 24     |
|----|----|----------------------------------------------------|----------|----------|----|----|--------|--|--|--
|    |    |                                                    |          | RESERVED |    |    |        |
|    |    |                                                    |          | R-0h     |    |    |        |
| 23 | 22 | 21                                                 | 20       | 19       | 18 | 17 | 16     |
|    |    |                                                    |          |          |    |    |        |
|    |    | RESERVEDR-0h<br>1312-98 |          |          |    |    |        |
| 15 | 14 |                                                    |          |          |    |    |        |
|    |    |                                                    |          | RESERVED |    |    |        |
|    |    |                                                    |          | R-0h     |    |    |        |
| 7  | 6  | 5                                                  | 4        | 3        | 2  | 1  | 0      |
|    |    |                                                    | RESERVED |          |    |    | RDONE  |
|    |    |                                                    | R-0h     |          |    |    | R/W-0h |
|    |    |                                                    |          |          |    |    |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-23. I2C\_SYSS Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                        |
|------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                    |
| 0    | RDONE    | R/W  | 0h    | Reset done bit.<br>This read-only bit indicates the state of the reset in case of hardware<br>reset, global software reset (I2C_SYSC.SRST) or partial software<br>reset (I2C_CON.I2C_EN).<br>The module must receive all its clocks before it can grant a reset<br>completed status.<br>Value after reset is low.<br>0h = Internal module reset in ongoing<br>1h = Reset completed |

*I2C*
#### **21.4.1.16 I2C\_BUF Register (offset = 94h) [reset = 0h]**

I2C\_BUF is shown in [Figure](#page-47-1) 21-31 and described in Table [21-24](#page-47-2).

This read/write register enables DMA transfers and allows the configuration of FIFO thresholds for the FIFO management (see the FIFO Management subsection).

**Figure 21-31. I2C\_BUF Register**

| 3130-2524                             |   |   |   |   |                                      |
|--------------------------------------------------------------------------|---|---|---|---|--------------------------------------|----|--|--|--|--|--|--
| RESERVEDR-0h<br>2221-1817<br>RESERVEDR-0h |   |   |   |   |                                      |
|                                                                          |   |   |   |   |                                      |
|                                                                          |   |   |   |   |                                      |
|                                                                          |   |   |   |   |                                      | 16
|                                                                          |   |   |   |   |                                      |
|                                                                          |   |   |   |   |                                      |
| 1413-8                                     |   |   |   |   |                                      |
| RXFIFO_CLR                                                               |   |   |   |   |                                      |
| R/W-0h                                                                   |   |   |   |   |                                      |
| 6                                                                        | 5 | 4 | 3 | 2 | 1                                    | 0
| TXFIFO_CLR                                                               |   |   |   |   |                                      |
| R/W-0h                                                                   |   |   |   |   |                                      |
|                                                                          |   |   |   |   | RXTRSHR/W-0h<br>TXTRSHR/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 21-24. I2C\_BUF Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
|-------|------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED   | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 15    | RDMA_EN    | R/W  | 0h    | Receive DMA channel enable.<br>When this bit is set to 1, the receive DMA channel is enabled and<br>the receive data ready status bit (I2C_IRQSTATUS_RAW: RRDY) is<br>forced to 0 by the core.<br>Value after reset is low.<br>0h = Receive DMA channel disabled<br>1h = Receive DMA channel enabled                                                                                                                                                                                                                                                                                                                                                                                      |
| 14    | RXFIFO_CLR | R/W  | 0h    | Receive FIFO clear.<br>When set, receive FIFO is cleared (hardware reset for RX FIFO<br>generated).<br>This bit is automatically reset by the hardware.<br>During reads, it always returns 0.<br>Value after reset is low.<br>0h = Normal mode<br>1h = Rx FIFO is reset                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 13-8  | RXTRSH     | R/W  | 0h    | Threshold value for FIFO buffer in RX mode.<br>The receive threshold value is used to specify the trigger level for<br>data receive transfers.<br>The value is specified from the Interface/OCP point of view.<br>Value after reset is 00h.<br>For the FIFO management description, see the FIFO Management<br>subsection.<br>Note<br>1: programmed threshold cannot exceed the actual depth of the<br>FIFO.<br>Note<br>2: the threshold must not be changed while a transfer is in progress<br>(after STT was configured or after the module was addressed as a<br>slave).<br>0h = Receive Threshold value = 11h = Receive Threshold value = 23Fh = Receive Threshold value = 64 |
**Table 21-24. I2C\_BUF Register Field Descriptions (continued)**

| Bit | Field      | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|-----|------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | XDMA_EN    | R/W  | 0h    | Transmit DMA channel enable.<br>When this bit is set to 1, the transmit DMA channel is enabled and<br>the transmit data ready status (I2C_IRQSTATUS_RAW: XRDY) bit is<br>forced to 0 by the core.<br>Value after reset is low.<br>0h = Transmit DMA channel disabled<br>1h = Transmit DMA channel enabled                                                                                                                                                                                                                                                                                |
| 6   | TXFIFO_CLR | R/W  | 0h    | Transmit FIFO clear.<br>When set, transmit FIFO is cleared (hardware reset for TX FIFO).<br>This bit is automatically reset by the hardware.<br>During reads, it always returns 0.<br>Value after reset is low.<br>0h = Normal mode<br>1h = Tx FIFO is reset                                                                                                                                                                                                                                                                                                                             |
| 5-0 | TXTRSH     | R/W  | 0h    | Threshold value for FIFO buffer in TX mode.<br>The Transmit Threshold value is used to specify the trigger level for<br>data transfers.<br>The value is specified from the OCP point of view.<br>Value after reset is 00h Note<br>1: programmed threshold cannot exceed the actual depth of the<br>FIFO.<br>Note<br>2: the threshold must not be changed while a transfer is in progress<br>(after STT was configured or after the module was addressed as a<br>slave).<br>0h = Transmit Threshold value = 11h = Transmit Threshold value = 23Fh = Transmit Threshold value = 64 |

*I2C*
#### **21.4.1.17 I2C\_CNT Register (offset = 98h) [reset = 0h]**

I2C\_CNT is shown in [Figure](#page-49-1) 21-32 and described in Table [21-25.](#page-49-2)

CAUTION: During an active transfer phase (between STT having been set to 1 and reception of ARDY), no modification must be done in this register. Changing it may result in an unpredictable behavior. This read/write register is used to control the numbers of bytes in the I2C data payload.

**Figure 21-32. I2C\_CNT Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23       | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|----|----|----|---|--------|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |    |    |    |   | DCOUNT |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |    |    |    |   | R/W-0h |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-25. I2C\_CNT Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 15-0  | DCOUNT   | R/W  | 0h    | Data count.<br>I2C Master Mode only (receive or transmit<br>F/S).<br>This<br>16-bit countdown counter decrements by 1 for every byte received or<br>sent through the I2C interface.<br>A write initializes DCOUNT to a saved initial value.<br>A read returns the number of bytes that are yet to be received or<br>sent.<br>A read into DCOUNT returns the initial value only before a start<br>condition and after a stop condition.<br>When DCOUNT reaches 0, the core generates a stop condition if a<br>stop condition was specified (I2C_CON.STP = 1) and the ARDY<br>status flag is set to 1 in the I2C_IRQSTATUS_RAW register.<br>Note that DCOUNT must not be reconfigured after I2C_CON.STT<br>was enabled and before ARDY is received.<br>Note<br>1: In case of I2C mode of operation, if I2C_CON.STP = 0, then the<br>I2C asserts SCL = 0 when DCOUNT reaches 0.<br>The CPU can then reprogram DCOUNT to a new value and resume<br>sending or receiving data with a new start condition (restart).<br>This process repeats until the CPU sets to 1 the I2C_CON.STP bit.<br>The ARDY flag is set each time DCOUNT reaches 0 and DCOUNT<br>is reloaded to its initial value.<br>Values after reset are low (all 16 bits).<br>Note<br>2: Since for DCOUNT = 0, the transfer length is 65536, the module<br>does not allow the possibility to initiate zero data bytes transfers.<br>0h = Data counter = 65536 bytes (216)<br>1h = Data counter = 1 bytes<br>FFFFh = Data counter = 65535 bytes (216 - 1) |
#### **21.4.1.18 I2C\_DATA Register (offset = 9Ch) [reset = 0h]**

I2C\_DATA is shown in [Figure](#page-50-1) 21-33 and described in Table [21-26.](#page-50-2)

This register is the entry point for the local host to read data from or write data to the FIFO buffer.

### **Figure 21-33. I2C\_DATA Register**

| 31               |      | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15     | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|------------------|------|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|--------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
| RESERVEDDATA |      |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |        |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|                  | R-0h |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-0h |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 21-26. I2C\_DATA Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 7-0  | DATA     | R/W  | 0h    | Transmit/Receive data FIFO endpoint.<br>When read, this register contains the received I2C data.<br>When written, this register contains the byte value to transmit over<br>the I2C data.<br>In SYSTEST loop back mode (I2C_SYSTEST: TMODE = 11), this<br>register is also the entry/receive point for the data.<br>Values after reset are unknown (all<br>8-bits).<br>Note: A read access, when the buffer is empty, returns the previous<br>read data value.<br>A write access, when the buffer is full, is ignored.<br>In both events, the FIFO pointers are not updated and an Access<br>Error (AERR) Interrupt is generated. |
#### **21.4.1.19 I2C\_CON Register (offset = A4h) [reset = 0h]**

I2C\_CON is shown in [Figure](#page-51-1) 21-34 and described in Table [21-27](#page-51-2).

During an active transfer phase (between STT having been set to 1 and reception of ARDY), no modification must be done in this register (except STP enable). Changing it may result in an unpredictable behavior.

**Figure 21-34. I2C\_CON Register**

| 31     | 30       | 29     | 28     | 27       | 26       | 25     | 24     |
|--------|----------|--------|--------|----------|----------|--------|--------|
|        |          |        |        | RESERVED |          |        |        |
|        |          |        |        | R-0h     |          |        |        |
| 23     | 22       | 21     | 20     | 19       | 18       | 17     | 16     |
|        |          |        |        | RESERVED |          |        |        |
|        |          |        |        | R-0h     |          |        |        |
| 15     | 14       | 13     | 12     | 11       | 10       | 9      | 8      |
| I2C_EN | RESERVED |        | OPMODE | STB      | MST      | TRX    | XSA    |
| R/W-0h | R-0h     |        | R/W-0h | R/W-0h   | R/W-0h   | R/W-0h | R/W-0h |
| 7      | 6        | 5      | 4      | 3        | 2        | 1      | 0      |
| XOA0   | XOA1     | XOA2   | XOA3   |          | RESERVED | STP    | STT    |
| R/W-0h | R/W-0h   | R/W-0h | R/W-0h |          | R-0h     | R/W-0h | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-27. I2C\_CON Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|-------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 15    | I2C_EN   | R/W  | 0h    | I2C module enable.<br>When this bit is cleared to 0, the I2C controller is not enabled and<br>reset.<br>When 0, receive and transmit FIFOs are cleared and all status bits<br>are set to their default values.<br>All configuration registers (I2C_IRQENABLE_SET,<br>I2C_IRQWAKE_SET, I2C_BUF, I2C_CNT, I2C_CON, I2C_OA,<br>I2C_SA, I2C_PSC, I2C_SCLL and I2C_SCLH) are not reset, they<br>keep their initial values and can be accessed.<br>The CPU must set this bit to 1 for normal operation.<br>Value after reset is low. |
|       |          |      |       | 0h = Controller in reset. FIFO are cleared and status bits are set to<br>their default value.<br>1h = Module enabled                                                                                                                                                                                                                                                                                                                                                                                                           |
| 14    | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 13-12 | OPMODE   | R/W  | 0h    | Operation mode selection.<br>These two bits select module operation mode.<br>Value after reset is 00.<br>0h = I2C Fast/Standard mode<br>1h = Reserved<br>2h = Reserved<br>3h = Reserved                                                                                                                                                                                                                                                                                                                                        |
| 11    | STB      | R/W  | 0h    | Start byte mode (I2C master mode only).<br>The start byte mode bit is set to 1 by the CPU to configure the I2C in<br>start byte mode (I2C_SA = 0000 0001).<br>See the Philips I2C spec for more details [1].<br>Value after reset is low.<br>0h = Normal mode<br>1h = Start byte mode                                                                                                                                                                                                                                          |
### **Table 21-27. I2C\_CON Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|-----|-------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 10  | MST   | R/W  | 0h    | Master/slave mode (I2C mode only).<br>When this bit is cleared, the I2C controller is in the slave mode and<br>the serial clock (SCL) is received from the master device.<br>When this bit is set, the I2C controller is in the master mode and<br>generates the serial clock.<br>Note: This bit is automatically cleared at the end of the transfer on a<br>detected stop condition, in case of arbitration lost or when the<br>module is configured as a master but addressed as a slave by an<br>external master.<br>Value after reset is low.<br>0h = Slave mode<br>1h = Master mode                                                                                                                                                                                 |
| 9   | TRX   | R/W  | 0h    | Transmitter/receiver mode (i2C master mode only).<br>When this bit is cleared, the I2C controller is in the receiver mode<br>and data on data line SDA is shifted into the receiver FIFO and can<br>be read from I2C_DATA register.<br>When this bit is set, the I2C controller is in the transmitter mode and<br>the data written in the transmitter FIFO via I2C_DATA is shifted out<br>on data line SDA.<br>Value after reset is low.<br>The operating modes are defined as follows: MST = 0, TRX = x,<br>Operating Mode = Slave receiver.<br>MST = 0, TRX = x, Operating Mode = Slave transmitter.<br>MST = 1, TRX = 0, Operating Modes = Master receiver.<br>MST = 1, TRX = 1, Operating Modes = Master transmitter.<br>0h = Receiver mode<br>1h = Transmitter mode |
| 8   | XSA   | R/W  | 0h    | Expand slave address.<br>(I2C mode only).<br>When set, this bit expands the slave address to<br>10-bit.<br>Value after reset is low.<br>0h = 7-bit address mode<br>1h = 10-bit address mode                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 7   | XOA0  | R/W  | 0h    | Expand own address 0.<br>(I2C mode only).<br>When set, this bit expands the base own address (OA0) to<br>10-bit.<br>Value after reset is low.<br>0h = 7-bit address mode<br>1h = 10-bit address mode                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| 6   | XOA1  | R/W  | 0h    | Expand own address 1.<br>(I2C mode only).<br>When set, this bit expands the first alternative own address (OA1) to<br>10-bit.<br>Value after reset is low.<br>0h = 7-bit address mode<br>1h = 10-bit address mode                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 5   | XOA2  | R/W  | 0h    | Expand own address 2.<br>(I2C mode only).<br>When set, this bit expands the second alternative own address<br>(OA2) to<br>10-bit.<br>Value after reset is low.<br>0h = 7-bit address mode. (I2C mode only).<br>1h = 10-bit address mode                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 4   | XOA3  | R/W  | 0h    | Expand own address 3.<br>When set, this bit expands the third alternative own address (OA3)<br>to<br>10-bit.<br>Value after reset is low.<br>0h = 7-bit address mode<br>1h = 10-bit address mode                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
**Table 21-27. I2C\_CON Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
|-----|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 3-2 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 1   | STP      | R/W  | 0h    | Stop condition (I2C master mode only).<br>This bit can be set to a 1 by the CPU to generate a stop condition.<br>It is reset to 0 by the hardware after the stop condition has been<br>generated.<br>The stop condition is generated when DCOUNT passes 0.<br>When this bit is not set to 1 before the end of the transfer (DCOUNT<br>= 0), the stop condition is not generated and the SCL line is hold to 0<br>by the master, which can re-start a new transfer by setting the STT<br>bit to 1.<br>Value after reset is low                                                                                                                                                                                                                                                                         |
|     |          |      |       | 0h = No action or stop condition detected                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| 0   | STT      | R/W  | 0h    | 1h = Stop condition queried<br>Start condition (I2C master mode only).<br>This bit can be set to a 1 by the CPU to generate a start condition.<br>It is reset to 0 by the hardware after the start condition has been<br>generated.<br>The start/stop bits can be configured to generate different transfer<br>formats.<br>Value after reset is low.<br>Note: DCOUNT is data count value in I2C_CNT register.<br>STT = 1, STP = 0, Conditions = Start, Bus Activities = S-A-D.<br>STT = 0, STP = 1, Conditions = Stop, Bus Activities = P.<br>STT = 1, STP = 1, Conditions = Start-Stop (DCOUNT=n), Bus<br>Activities = S-A-D(n)D-P.<br>STT = 1, STP = 0, Conditions = Start (DCOUNT=n), Bus Activities =<br>S-A-D(n)D.<br>0h = No action or start condition detected<br>1h = Start condition queried |
#### **21.4.1.20 I2C\_OA Register (offset = A8h) [reset = 0h]**

I2C\_OA is shown in [Figure](#page-54-1) 21-35 and described in Table [21-28.](#page-54-2)

CAUTION: During an active transfer phase (between STT having been set to 1 and reception of ARDY), no modification must be done in this register. Changing it may result in an unpredictable behavior. This register is used to specify the module s base I2C 7-bit or 10-bit address (base own address).

**Figure 21-35. I2C\_OA Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20       | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4      | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|---|---|---|---|---|--------|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | OA     |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | R/W-0h |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-28. I2C\_OA Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                        |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                    |
| 9-0   | OA       | R/W  | 0h    | Own address.<br>This field specifies either: A10-bit address coded on OA<br>[9:0] when XOA (Expand Own Address, I2C_CON[7]) is set to 1.<br>or A7-bit address coded on OA<br>[6:0] when XOA (Expand Own Address, I2C_CON[7]) is cleared to 0.<br>In this case, OA<br>[9:7] bits must be cleared to 000 by application software.<br>Value after reset is low (all 10 bits). |
#### **21.4.1.21 I2C\_SA Register (offset = ACh) [reset = 0h]**

I2C\_SA is shown in [Figure](#page-55-1) 21-36 and described in Table [21-29.](#page-55-2)

CAUTION: During an active transfer phase (between STT having been set to 1 and reception of ARDY), no modification must be done in this register. Changing it may result in an unpredictable behavior. This register is used to specify the addressed I2C module 7-bit or 10-bit address (slave address).

**Figure 21-36. I2C\_SA Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-29. I2C\_SA Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                 |
|-------|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                             |
| 9-0   | SA       | R/W  | 0h    | Slave address.<br>This field specifies either: A10-bit address coded on SA<br>[9:0] when XSA (Expand Slave Address, I2C_CON[8]) is set to 1.<br>or A7-bit address coded on SA<br>[6:0] when XSA (Expand Slave Address, I2C_CON[8]) is cleared to<br>0.<br>In this case, SA<br>[9:7] bits must be cleared to 000 by application software.<br>Value after reset is low (all 10 bits). |
#### **21.4.1.22 I2C\_PSC Register (offset = B0h) [reset = 0h]**

I2C\_PSC is shown in [Figure](#page-56-1) 21-37 and described in Table [21-30.](#page-56-2)

CAUTION: During an active mode (I2C\_EN bit in I2C\_CON register is set to 1), no modification must be done in this register. Changing it may result in an unpredictable behavior. This register is used to specify the internal clocking of the I2C peripheral core.

**Figure 21-37. I2C\_PSC Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20       | 19   | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4      | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----------|------|----|----|----|----|----|----|----|----|----|---|---|---|---|---|--------|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    | RESERVED |      |    |    |    |    |    |    |    |    |    |   |   |   |   |   | PSC    |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    |          | R-0h |    |    |    |    |    |    |    |    |    |   |   |   |   |   | R/W-0h |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-30. I2C\_PSC Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 7-0  | PSC      | R/W  | 0h    | Fast/Standard mode prescale sampling clock divider value.<br>The core uses this<br>8-bit value to divide the system clock (SCLK) and generates its own<br>internal sampling clock (ICLK) for Fast and Standard operation<br>modes.<br>The core logic is sampled at the clock rate of the system clock for<br>the module divided by (PSC + 1).<br>Value after reset is low (all 8 bits).<br>0h = Divide by 11h = Divide by 2FFh = Divide by 256 |
#### **21.4.1.23 I2C\_SCLL Register (offset = B4h) [reset = 0h]**

I2C\_SCLL is shown in [Figure](#page-57-1) 21-38 and described in Table [21-31](#page-57-2).

CAUTION: During an active mode (I2C\_EN bit in I2C\_CON register is set to 1), no modification must be done in this register. Changing it may result in an unpredictable behavior. This register is used to determine the SCL low time value when master.

### **Figure 21-38. I2C\_SCLL Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20       | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4      | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|---|---|---|---|---|--------|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | SCLL   |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | R/W-0h |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-31. I2C\_SCLL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                    |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                |
| 7-0  | SCLL     | R/W  | 0h    | Fast/Standard mode SCL low time.<br>I2C master mode only, (FS).<br>This<br>8-bit value is used to generate the SCL low time value (tLOW) when<br>the peripheral is operated in master mode.<br>tLOW = (SCLL + 7) * ICLK time period, Value after reset is low (all 8<br>bits). |
#### **21.4.1.24 I2C\_SCLH Register (offset = B8h) [reset = 0h]**

I2C\_SCLH is shown in [Figure](#page-58-1) 21-39 and described in Table [21-32.](#page-58-2)

CAUTION: During an active mode (I2C\_EN bit in I2C\_CON register is set to 1), no modification must be done in this register. Changing it may result in an unpredictable behavior. This register is used to determine the SCL high time value when master.

**Figure 21-39. I2C\_SCLH Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19       | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4      | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|---|---|---|---|---|--------|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |   |   |   |   |   | SCLH   |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |   |   |   |   |   | R/W-0h |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-32. I2C\_SCLH Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                         |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                     |
| 7-0  | SCLH     | R/W  | 0h    | Fast/Standard mode SCL low time.<br>I2C master mode only, (FS).<br>This<br>8-bit value is used to generate the SCL high time value (tHIGH)<br>when the peripheral is operated in master mode.<br>- tHIGH = (SCLH + 5) * ICLK time period.<br>Value after reset is low (all 8 bits). |
#### **21.4.1.25 I2C\_SYSTEST Register (offset = BCh) [reset = 0h]**

I2C\_SYSTEST is shown in [Figure](#page-59-1) 21-40 and described in Table [21-33.](#page-59-2)

CAUTION: Never enable this register for normal I2C operation This register is used to facilitate systemlevel tests by overriding some of the standard functional features of the peripheral. It allows testing of SCL counters, controlling the signals that connect to I/O pins, or creating digital loop-back for self-test when the module is configured in system test (SYSTEST) mode. It also provides stop/non-stop functionality in the debug mode.

### **Figure 21-40. I2C\_SYSTEST Register**

| 31         | 30         | 29         | 28       | 27     | 26       | 25    | 24         |
|------------|------------|------------|----------|--------|----------|-------|------------|
|            |            |            | RESERVED |        |          |       |            |
|            |            |            | R-0h     |        |          |       |            |
| 23         | 22         | 21         | 20       | 19     | 18       | 17    | 16         |
|            |            |            | RESERVED |        |          |       |            |
|            |            |            | R-0h     |        |          |       |            |
| 15         | 14         | 13         | 12       | 11     | 10       | 9     | 8          |
| ST_EN      | FREE       | TMODE      |          | SSB    | RESERVED |       | SCL_I_FUNC |
| R/W-0h     | R/W-0h     | R/W-0h     |          | R/W-0h | R-0h     |       | R-0h       |
| 7          | 6          | 5          | 4        | 3      | 2        | 1     | 0          |
| SCL_O_FUNC | SDA_I_FUNC | SDA_O_FUNC | RESERVED | SCL_I  | SCL_O    | SDA_I | SDA_O      |
| R-0h       | R-0h       | R-0h       | R-0h     | R-0h   | R-0h     | R-0h  | R-0h       |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-33. I2C\_SYSTEST Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 15    | ST_EN    | R/W  | 0h    | System test enable.<br>This bit must be set to 1 to permit other system test registers bits to<br>be set.<br>Value after reset is low.<br>0h = Normal mode. All others bits in register are read only.<br>1h = System test enabled. Permit other system test registers bits to<br>be set.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| 14    | FREE     | R/W  | 0h    | Free running mode (on breakpoint).<br>This bit is used to determine the state of the I2C controller when a<br>breakpoint is encountered in the HLL debugger.<br>Note: This bit can be set independently of ST_EN value.<br>FREE =<br>0: the I2C controller stops immediately after completion of the on<br>going bit transfer.<br>Stopping the transfer is achieved by forcing the SCL line low.<br>Note that in this case there will be no status register updates.<br>FREE =<br>1: the I2C interface runs free.<br>When Suspend indication will be asserted, there will be no accesses<br>on the OCP Interface (the CPU is in debug mode) and consequently<br>the FIFOs will reach full/empty state (according to RX or TX modes)<br>and the I2C SDA line will be kept low.<br>Note that the status registers will be updated, but no DMA, IRQ or<br>WakeUp will be generated.<br>The status registers likely to be updated in this mode are:<br>I2C_IRQSTATUS_RAW.XRDY, I2C_IRQSTATUS_RAW.RRDY,<br>I2C_IRQSTATUS_RAW.XUDF, I2C_IRQSTATUS_RAW.ROVR,<br>I2C_IRQSTATUS_RAW.ARDY and I2C_IRQSTATUS_RAW.NACK.<br>Value after reset is low.<br>0h = Stop mode (on breakpoint condition). If Master mode, it stops<br>after completion of the on-going bit transfer. In slave mode, it stops<br>during the phase transfer when 1 byte is completely<br>transmitted/received.<br>1h = Free running mode |

*I2C*
### **Table 21-33. I2C\_SYSTEST Register Field Descriptions (continued)**

| Bit   | Field      | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
|-------|------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 13-12 | TMODE      | R/W  | 0h    | Test mode select.<br>In normal functional mode (ST_EN = 0), these bits are don't care.<br>They are always read as 00 and a write is ignored.<br>In system test mode (ST_EN = 1), these bits can be set according to<br>the following table to permit various system tests.<br>Values after reset are low (2 bits).<br>SCL counter test mode: in this mode, the SCL pin is driven with a<br>permanent clock as if mastered with the parameters set in the<br>I2C_PSC, I2C_SCLL, and I2C_SCLH registers.<br>Loop back mode: in the master transmit mode only, data transmitted<br>out of the I2C_DATA register (write action) is received in the same<br>I2C_DATA register via an internal path through the FIFO buffer.<br>The DMA and interrupt requests are normally generated if enabled.<br>SDA/SCL IO mode: in this mode, the SCL IO and SDA IO are<br>controlled via the I2C_SYSTEST<br>[5:0] register bits.<br>0h = Functional mode (default)<br>1h = Reserved<br>2h = Test of SCL counters (SCLL, SCLH, PSC). SCL provides a<br>permanent clock with master mode.<br>3h = Loop back mode select + SDA/SCL IO mode select |
| 11    | SSB        | R/W  | 0h    | Set status bits.<br>Writing 1 into this bit also sets the 6 read/clear-only status bits<br>contained in I2C_IRQSTATUS_RAW register (bits<br>5:0) to 1.<br>Writing 0 into this bit doesn't clear status bits that are already set<br>only writing 1 into a set status bit can clear it (see<br>I2C_IRQSTATUS_RAW operation).<br>This bit must be cleared prior attempting to clear a status bit.<br>Value after reset is low.<br>0h = No action.<br>1h = Set all interrupt status bits to 1.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| 10-9  | RESERVED   | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 8     | SCL_I_FUNC | R    | 0h    | SCL line input value (functional mode).<br>This read-only bit returns the logical state taken by the SCL line<br>(either 1 or 0).<br>It is active both in functional and test mode.<br>Value after reset is low.<br>0h (R) = Read 0 from SCL line<br>1h (R) = Read 1 from SCL line                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 7     | SCL_O_FUNC | R    | 0h    | SCL line output value (functional mode).<br>This read-only bit returns the value driven by the module on the SCL<br>line (either 1 or 0).<br>It is active both in functional and test mode.<br>Value after reset is low.<br>0h (R) = Driven 0 on SCL line<br>1h (R) = Driven 1 on SCL line                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 6     | SDA_I_FUNC | R    | 0h    | SDA line input value (functional mode).<br>This read-only bit returns the logical state taken by the SDA line<br>(either 1 or 0).<br>It is active both in functional and test mode.<br>Value after reset is low.<br>0h (R) = Read 0 from SDA line<br>1h (R) = Read 1 from SDA line                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 5     | SDA_O_FUNC | R    | 0h    | SDA line output value (functional mode).<br>This read-only bit returns the value driven by the module on the SDA<br>line (either 1 or 0).<br>It is active both in functional and test mode.<br>Value after reset is low.<br>0h (R) = Driven 0 to SDA line<br>1h (R) = Driven 1 to SDA line                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 4     | RESERVED   | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
**Table 21-33. I2C\_SYSTEST Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                            |
|-----|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 3   | SCL_I | R    | 0h    | SCL line sense input value.<br>In normal functional mode (ST_EN = 0), this read-only bit always<br>reads 0.<br>In system test mode (ST_EN = 1 and TMODE = 11), this read-only<br>bit returns the logical state taken by the SCL line (either 1 or 0).<br>Value after reset is low.<br>0h (R) = Read 0 from SCL line<br>1h (R) = Read 1 from SCL line                                                                                   |
| 2   | SCL_O | R    | 0h    | SCL line drive output value.<br>In normal functional mode (ST_EN = 0), this bit is don't care.<br>It always reads 0 and a write is ignored.<br>In system test mode (ST_EN = 1 and TMODE = 11), a 0 forces a<br>low level on the SCL line and a 1 puts the I2C output driver to a<br>high-impedance state.<br>Value after reset is low.<br>0h (R) = Forces 0 on the SCL data line<br>1h (R) = SCL output driver in high-impedance state |
| 1   | SDA_I | R    | 0h    | SDA line sense input value.<br>In normal functional mode (ST_EN = 0), this read-only bit always<br>reads 0.<br>In system test mode (ST_EN = 1 and TMODE = 11), this read-only<br>bit returns the logical state taken by the SDA line (either 1 or 0).<br>Value after reset is low.<br>0h (R) = Read 0 from SDA line<br>1h (R) = Read 1 from SDA line                                                                                   |
| 0   | SDA_O | R    | 0h    | SDA line drive output value.<br>In normal functional mode (ST_EN = 0), this bit is don't care.<br>It reads as 0 and a write is ignored.<br>In system test mode (ST_EN = 1 and TMODE = 11), a 0 forces a<br>low level on the SDA line and a 1 puts the I2C output driver to a<br>high-impedance state.<br>Value after reset is low.<br>0h = Write 0 to SDA line<br>1h = Write 1 to SDA line                                             |
#### **21.4.1.26 I2C\_BUFSTAT Register (offset = C0h) [reset = 0h]**

I2C\_BUFSTAT is shown in [Figure](#page-62-1) 21-41 and described in Table [21-34.](#page-62-2)

This read-only register reflects the status of the internal buffers for the FIFO management (see the FIFO Management subsection).

**Figure 21-41. I2C\_BUFSTAT Register**

| 31        | 30 | 29 | 28 | 27       | 26 | 25 | 24 |
|-----------|----|----|----|----------|----|----|----|
|           |    |    |    | RESERVED |    |    |    |
|           |    |    |    | R-0h     |    |    |    |
| 23        | 22 | 21 | 20 | 19       | 18 | 17 | 16 |
|           |    |    |    | RESERVED |    |    |    |
|           |    |    |    | R-0h     |    |    |    |
| 15        | 14 | 13 | 12 | 11       | 10 | 9  | 8  |
| FIFODEPTH |    |    |    | RXSTAT   |    |    |    |
| R-2h      |    |    |    | R-0h     |    |    |    |
| 7         | 6  | 5  | 4  | 3        | 2  | 1  | 0  |
| RESERVED  |    |    |    | TXSTAT   |    |    |    |
| R-0h      |    |    |    | R-0h     |    |    |    |
|           |    |    |    |          |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 21-34. I2C\_BUFSTAT Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description                                                                                                                                                                                                                                                                                                        |
|-------|-----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED  | R    | 0h    |                                                                                                                                                                                                                                                                                                                    |
| 15-14 | FIFODEPTH | R    | 2h    | Internal FIFO buffers depth.<br>This read-only bit indicates the internal FIFO buffer depth.<br>Value after reset is given by the boundary module generic<br>parameter.<br>0h = 8-bytes FIFO1h = 16-bytes FIFO2h = 32-bytes FIFO3h = 64-bytes FIFO                                                     |
| 13-8  | RXSTAT    | R    | 0h    | RX buffer status.<br>This read-only field indicates the number of bytes to be transferred<br>from the FIFO at the end of the I2C transfer (when RDR is asserted).<br>It corresponds to the level indication of the RX FIFO (number of<br>written locations).<br>Value after reset is 0.                            |
| 7-6   | RESERVED  | R    | 0h    |                                                                                                                                                                                                                                                                                                                    |
| 5-0   | TXSTAT    | R    | 0h    | TX buffer status.<br>This read-only field indicates the number of data bytes still left to be<br>written in the TX FIFO (it s equal with the initial value of<br>I2C_CNT.DCOUNT minus the number of data bytes already written<br>in the TX FIFO through the OCP Interface).<br>Value after reset is equal with 0. |
#### **21.4.1.27 I2C\_OA1 Register (offset = C4h) [reset = 0h]**

I2C\_OA1 is shown in [Figure](#page-63-1) 21-42 and described in Table [21-35](#page-63-2).

CAUTION: During an active transfer phase (between STT has been set to 1 and receiving of ARDY), no modification must be done in this register. Changing it may result in an unpredictable behavior. This register is used to specify the first alternative I2C 7-bit or 10-bit address (own address 1 - OA1).

**Figure 21-42. I2C\_OA1 Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20       | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4      | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|---|---|---|---|---|--------|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | OA1    |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | R/W-0h |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-35. I2C\_OA1 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                  |
|-------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                              |
| 9-0   | OA1      | R/W  | 0h    | Own address 1.<br>This field specifies either: A10-bit address coded on OA1<br>[9:0] when XOA1 (Expand Own Address<br>1 - XOA1, I2C_CON[6]) is set to 1.<br>A7-bit address coded on OA1<br>[6:0] when XOA1 (Expand Own Address 1 XOA1, I2C_CON[6]) is<br>cleared to 0.<br>In this case, OA1<br>[9:7] bits must be cleared to 000 by application software.<br>Value after reset is low (all 10 bits). |
#### **21.4.1.28 I2C\_OA2 Register (offset = C8h) [reset = 0h]**

I2C\_OA2 is shown in [Figure](#page-64-1) 21-43 and described in Table [21-36](#page-64-2).

CAUTION: During an active transfer phase (between STT has been set to 1 and receiving of ARDY), no modification must be done in this register. Changing it may result in an unpredictable behavior. This register is used to specify the first alternative I2C 7-bit or 10-bit address (own address 2 - OA2).

**Figure 21-43. I2C\_OA2 Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20       | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4      | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|---|---|---|---|---|--------|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | OA2    |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | R/W-0h |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-36. I2C\_OA2 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                  |
|-------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                              |
| 9-0   | OA2      | R/W  | 0h    | Own address 2.<br>This field specifies either: A10-bit address coded on OA2<br>[9:0] when XOA1 (Expand Own Address<br>2 - XOA2, I2C_CON[5]) is set to 1.<br>A7-bit address coded on OA2<br>[6:0] when XOA2 (Expand Own Address 2 XOA2, I2C_CON[5]) is<br>cleared to 0.<br>In this case, OA2<br>[9:7] bits must be cleared to 000 by application software.<br>Value after reset is low (all 10 bits). |
#### **21.4.1.29 I2C\_OA3 Register (offset = CCh) [reset = 0h]**

I2C\_OA3 is shown in [Figure](#page-65-1) 21-44 and described in Table [21-37](#page-65-2).

CAUTION: During an active transfer phase (between STT has been set to 1 and receiving of ARDY), no modification must be done in this register. Changing it may result in an unpredictable behavior. This register is used to specify the first alternative I2C 7-bit or 10-bit address (own address 3 - OA3).

**Figure 21-44. I2C\_OA3 Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20       | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4      | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|---|---|---|---|---|--------|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | OA3    |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | R/W-0h |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-37. I2C\_OA3 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                  |
|-------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                              |
| 9-0   | OA3      | R/W  | 0h    | Own address 2.<br>This field specifies either: A10-bit address coded on OA3<br>[9:0] when XOA3 (Expand Own Address<br>3 - XOA3, I2C_CON[4]) is set to 1.<br>A7-bit address coded on OA3<br>[6:0] when XOA1 (Expand Own Address 3 XOA3, I2C_CON[4]) is<br>cleared to 0.<br>In this case, OA3<br>[9:7] bits must be cleared to 000 by application software.<br>Value after reset is low (all 10 bits). |
#### **21.4.1.30 I2C\_ACTOA Register (offset = D0h) [reset = 0h]**

I2C\_ACTOA is shown in [Figure](#page-66-1) 21-45 and described in Table [21-38.](#page-66-2)

This read-only register is used to indicate which one of the module s four own addresses the external master used when addressing the module. The CPU can read this register when the AAS indication was activated. The indication is cleared at the end of the transfer.

**Figure 21-45. I2C\_ACTOA Register**

| 31 | 30       | 29 | 28 | 27       | 26      | 25      | 24      |
|----|----------|----|----|----------|---------|---------|---------|
|    |          |    |    | RESERVED |         |         |         |
|    |          |    |    | R-0h     |         |         |         |
| 23 | 22       | 21 | 20 | 19       | 18      | 17      | 16      |
|    |          |    |    | RESERVED |         |         |         |
|    |          |    |    | R-0h     |         |         |         |
| 15 | 14       | 13 | 12 | 11       | 10      | 9       | 8       |
|    |          |    |    | RESERVED |         |         |         |
|    |          |    |    | R-0h     |         |         |         |
| 7  | 6        | 5  | 4  | 3        | 2       | 1       | 0       |
|    | RESERVED |    |    | OA3_ACT  | OA2_ACT | OA1_ACT | OA0_ACT |
|    | R-0h     |    |    | R-0h     | R-0h    | R-0h    | R-0h    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 21-38. I2C\_ACTOA Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                        |
|------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-4 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                    |
| 3    | OA3_ACT  | R    | 0h    | Own address 3 active.<br>When a bit location is set to 1 by the core, it signalizes to the Local<br>Host that an external master using the corresponding own address<br>addressed the module.<br>Value after reset is low.<br>0h = Own address inactive<br>1h = Own address active |
| 2    | OA2_ACT  | R    | 0h    | Own address 2 active.<br>When a bit location is set to 1 by the core, it signalizes to the Local<br>Host that an external master using the corresponding own address<br>addressed the module.<br>Value after reset is low.<br>0h = Own address inactive<br>1h = Own address active |
| 1    | OA1_ACT  | R    | 0h    | Own address 1 active.<br>When a bit location is set to 1 by the core, it signalizes to the Local<br>Host that an external master using the corresponding own address<br>addressed the module.<br>Value after reset is low.<br>0h = Own address inactive<br>1h = Own address active |
| 0    | OA0_ACT  | R    | 0h    | Own address 0 active.<br>When a bit location is set to 1 by the core, it signalizes to the Local<br>Host that an external master using the corresponding own address<br>addressed the module.<br>Value after reset is low.<br>0h = Own address inactive<br>1h = Own address active |
#### **21.4.1.31 I2C\_SBLOCK Register (offset = D4h) [reset = 0h]**

I2C\_SBLOCK is shown in [Figure](#page-67-1) 21-46 and described in Table [21-39.](#page-67-2)

This read/write register controls the automatic blocking of I2C clock feature in slave mode. It is used for the Local Host to configure for which of the 4 own addresses, the core must block the I2C clock (keep SCL line low) right after the Address Phase, when it is addressed as a slave.

**Figure 21-46. I2C\_SBLOCK Register**

| 31       | 30 | 29 | 28 | 27     | 26     | 25     | 24     |
|----------|----|----|----|--------|--------|--------|--------|--
| RESERVED |    |    |    |        |        |        |        |
| R-0h     |    |    |    |        |        |        |        |
| 23       | 22 | 21 | 20 | 19     | 18     | 17     | 16     |
| RESERVED |    |    |    |        |        |        |        |
| R-0h     |    |    |    |        |        |        |        |
| 15       | 14 | 13 | 12 | 11     | 10     | 9      | 8      |
| RESERVED |    |    |    |        |        |        |        |
| R-0h     |    |    |    |        |        |        |        |
| 7        | 6  | 5  | 4  | 3      | 2      | 1      | 0      |
| RESERVED |    |    |    | OA3_EN | OA2_EN | OA1_EN | OA0_EN |
| R-0h     |    |    |    | R/W-0h | R/W-0h | R/W-0h | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 21-39. I2C\_SBLOCK Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-4 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 3    | OA3_EN   | R/W  | 0h    | Enable I2C clock blocking for own address 3.<br>When the CPU sets a bit location to 1, if an external master using<br>the corresponding own address addresses the core, the core will<br>block the I2C clock right after the address phase.<br>For releasing the I2C clock the CPU must write 0 in the<br>corresponding field.<br>Value after reset is low.<br>0h = I2C clock released<br>1h = I2C clock blocked |
| 2    | OA2_EN   | R/W  | 0h    | Enable I2C clock blocking for own address 2.<br>When the CPU sets a bit location to 1, if an external master using<br>the corresponding own address addresses the core, the core will<br>block the I2C clock right after the address phase.<br>For releasing the I2C clock the CPU must write 0 in the<br>corresponding field.<br>Value after reset is low.<br>0h = I2C clock released<br>1h = I2C clock blocked |
| 1    | OA1_EN   | R/W  | 0h    | Enable I2C clock blocking for own address 1.<br>When the CPU sets a bit location to 1, if an external master using<br>the corresponding own address addresses the core, the core will<br>block the I2C clock right after the address phase.<br>For releasing the I2C clock the CPU must write 0 in the<br>corresponding field.<br>Value after reset is low.<br>0h = I2C clock released<br>1h = I2C clock blocked |
**Table 21-39. I2C\_SBLOCK Register Field Descriptions (continued)**

| Bit | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                      |
|-----|--------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0   | OA0_EN | R/W  | 0h    | Enable I2C clock blocking for own address 0.<br>When the CPU sets a bit location to 1, if an external master using<br>the corresponding own address addresses the core, the core will<br>block the I2C clock right after the address phase.<br>For releasing the I2C clock the CPU must write 0 in the<br>corresponding field.<br>Value after reset is low.<br>0h = I2C clock released<br>1h = I2C clock blocked |

*I2C*
