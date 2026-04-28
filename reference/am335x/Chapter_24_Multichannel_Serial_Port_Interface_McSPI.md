# 24 Multichannel Serial Port Interface (McSPI)

This chapter describes the McSPI of the device.

**Topic** ........................................................................................................................... **Page**

| 24.1 | Introduction           | 4886 |
|------|------------------------|------|
| 24.2 | Integration            | 4887 |
| 24.3 | Functional Description | 4889 |
| 24.4 | McSPI Registers        | 4923 |
|      |                        |      |
## **24.1 Introduction**

This document is intended to provide programmers with a functional presentation of the Master/Slave Multichannel Serial Port Interface (McSPI) module. It also provides a register description and a module configuration example.

McSPI is a general-purpose receive/transmit master/slave controller that can interface with up to four slave external devices or one single external master. It allows a duplex, synchronous, serial communication between a CPU and SPI compliant external devices (Slaves and Masters).

### *24.1.1 McSPI Features*

The general features of the SPI controller are:

- Buffered receive/transmit data register per channel (1 word deep)
- Multiple SPI word access with one channel using a FIFO
- Two DMA requests per channel, one interrupt line
- Single interrupt line, for multiple interrupt source events
- Serial link interface supports:
  - Full duplex / Half duplex
  - Multi-channel master or single channel slave operations
  - Programmable 1-32 bit transmit/receive shift operations.
  - Wide selection of SPI word lengths continuous from 4 to 32 bits
- Up to four SPI channels
- SPI word Transmit / Receive slot assignment based on round robin arbitration
- SPI configuration per channel (clock definition, enable polarity and word width)
- Clock generation supports:
  - Programmable master clock generation (operating from fixed 48-MHz functional clock input)
  - Selectable clock phase and clock polarity per chip select.

### *24.1.2 Unsupported McSPI Features*

This device supports only two chip selects per module. Module wakeup during slave mode operation is not supported, as noted in *McSPI Clock and Reset Management*.

**Table 24-1. Unsupported McSPI Features**

| Feature                     | Reason                                        |
|-----------------------------|-----------------------------------------------|
| Chip selects 2 and 3        | Not pinned out                                |
| Slave mode wakeup           | SWAKEUP not connected                         |
| Retention during power down | Module not synthesized with retention enabled |
## 24.2 Integration

This device includes two instantiations of McSPI: SPI0 and SPI1. The McSPI module is a general-purpose receive/transmit master/slave controller that can interface with either up to four slave external devices or one single external master. Figure 24-1 shows the example of a system with multiple external slave SPI compatible devices and Figure 24-2 shows the example of a system with an external master.


Figure 24-1. SPI Master Application


Figure 24-2. SPI Slave Application
### *24.2.1 McSPI Connectivity Attributes*

The general connectivity attributes for the McSPI module are shown in [Table](#page-3-0) 24-2.

### **Table 24-2. McSPI Connectivity Attributes**

| Attributes          | Type                                                                                                                                                                                                |
|---------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Power Domain        | Peripheral Domain                                                                                                                                                                                   |
| Clock Domain        | PD_PER_L4LS_GCLK (Interface/OCP)<br>PD_PER_SPI_GCLK (Func)                                                                                                                                          |
| Reset Signals       | PER_DOM_RST_N                                                                                                                                                                                       |
| Idle/Wakeup Signals | Smart Idle                                                                                                                                                                                          |
| Interrupt Requests  | 1 interrupt to MPU subsystem and PRU-ICSS (McSPI0INT)<br>1 interrupt to MPU subsystem only (McSPI1INT)                                                                                              |
| DMA Requests        | 4 DMA requests per instance to EDMA<br>•<br>1 RX request for CS0 (SPIREVT0)<br>•<br>1 TX request for CS0 (SPIXEVT0)<br>•<br>1 RX request for CS1 (SPIREVT1)<br>•<br>1 TX request for CS1 (SPIXEVT1) |
| Physical Address    | L4 Peripheral slave port                                                                                                                                                                            |

### *24.2.2 McSPI Clock and Reset Management*

The SPI module clocks can be woken up in two manners: by the SPI module itself using the SWAKEUP signal (refer to the module functional spec for detailed conditions), or directly from an external SPI master device by detecting an active low level on its chip select input pin (CS0n) using a GPIO attached to that device pin. Neither of these methods is supported on the device.

**Table 24-3. McSPI Clock Signals**

| Clock Signal                  | Max Freq | Reference / Source | Comments                      |
|-------------------------------|----------|--------------------|-------------------------------|
| CLKInterface clock        | 100 MHz  | CORE_CLKOUTM4 / 2  | pd_per_l4ls_gclk<br>From PRCM |
| CLKSPIREFFunctional clock | 48 MHz   | PER_CLKOUTM2 / 4   | pd_per_spi_gclk<br>From PRCM  |

### *24.2.3 McSPI Pin List*

The McSPI interface pins are summarized in [Table](#page-3-1) 24-4.

**Table 24-4. McSPI Pin List**

| Pin       | Type   | Description                                                            |
|-----------|--------|------------------------------------------------------------------------|
| SPIx_SCLK | I/O(1) | SPI serial clock (output when master,<br>input when slave)             |
| SPIx_D0   | I/O    | Can be configured as either input or<br>output (MOSI or MISO)          |
| SPIx_D1   | I/O    | Can be configured as either input or<br>output (MOSI or MISO)          |
| SPIx_CS0  | I/O    | SPI chip select 0 output when master,<br>input when slave (active low) |
| SPIx_CS1  | I/O    | SPI chip select 1 output when master,<br>input when slave (active low) |

<sup>(1)</sup> These signals are also used as inputs to re-time or sync data. The associated CONF\_<module>\_<pin>\_RXACTIVE bit for these signals must be set to 1 to enable the inputs back to the module. It is also recommended to place a 33-ohm resistor in series (close to the processor) on each of these signals to avoid signal reflections.
## **24.3 Functional Description**

### *24.3.1 SPI Transmission*

This section describes the transmissions supported by McSPI. The SPI protocol is a synchronous protocol that allows a master device to initiate serial communication with a slave device. Data is exchanged between these devices. A slave select line (SPIEN) can be used to allow selection of an individual slave SPI device. Slave devices that are not selected do not interfere with SPI bus activities. Connected to multiple external devices, McSPI exchanges data with a single SPI device at a time through two main modes:

- Two data pins interface mode. (See Section [24.3.1.1](#page-5-0))
- Single data pin interface mode (recommended for half-duplex transmission). (See Section [24.3.1.2](#page-5-1))

The flexibility of McSPI allows exchanging data with several formats through programmable parameters described in Section [24.3.1.3.](#page-7-0)
#### **24.3.1.1 Two Data Pins Interface Mode**

The two data pins interface mode, allows a full duplex SPI transmission where data is transmitted (shifted out serially) and received (shifted in serially) simultaneously on separate data lines SPIDAT [0] and SPIDAT [1]. Data leaving the master exits on transmit serial data line also known as MOSI: MasterOutSlaveIn. Data leaving the slave exits on the receive data line also known as MISO: MasterInSlaveOut.

McSPI has a unified SPI port control: SPIDAT [1:0] can be independently configured as receive or transmit lines. The user has the responsibility to program which data line to use and in which direction (receive or transmit), according to the external slave/master connection.

The serial clock (SPICLK) synchronizes shifting and sampling of the information on the two serial data lines (SPIDAT [1:0]). Each time a bit is transferred out from the Master, one bit is transferred in from Slave.

[Figure](#page-5-2) 24-3 shows an example of a full duplex system with a Master device on the left and a Slave device on the right. After 8 cycles of the serial clock SPICLK, the WordA has been transferred from the master to the slave. At the same time, the 8-bit WordB has been transferred from the slave to the master.

When referring to the master device, the control block transmits the clock SPICLK and the enable signal SPIEN (optional, see , *McSPI\_MODULCTRL*).


**Figure 24-3. SPI Full-Duplex Transmission**

#### **24.3.1.2 Single Data Pin Interface Mode**

In single data pin interface mode, under software control, a single data line is used to alternatively transmit and receive data (Half duplex transmission).

McSPI has a unified SPI port control: SPIDAT [1:0] can be independently configured as receive or transmit lines. The user has the responsibility to program which data line to use and in which direction (receive or transmit), according to the external slave/master connection.

As for a full duplex transmission, the serial clock (SPICLK) synchronizes shifting and sampling of the information on the single serial data line.
#### *24.3.1.2.1 Example With a Receive-Only Slave*

[Figure](#page-6-0) 24-4 shows a half duplex system with a Master device on the left and a receive-only Slave device on the right. Each time a bit is transferred out from the Master, one bit is transferred in the Slave. After 8 cycles of the serial clock SPICLK, the 8-bit WordA has been transferred from the master to the slave.


**Figure 24-4. SPI Half-Duplex Transmission (Receive-only Slave)**

#### *24.3.1.2.2 Example With a Transmit-Only Slave*

[Figure](#page-6-1) 24-5 shows a half duplex system with a Master device on the left and a transmit-only Slave device on the right. Each time a bit is transferred out from the Slave, one bit is transferred in the Master. After 8 cycles of the serial clock SPICLK, the 8-bit WordA has been transferred from the slave to the master.


**Figure 24-5. SPI Half-Duplex Transmission (Transmit-Only Slave)**

**After 8**

**WordC**

**WordB After 8**
#### **24.3.1.3 Transfer Formats**

This section describes the transfer formats supported by McSPI.

The flexibility of McSPI allows setting the parameters of the SPI transfer:

- SPI word length
- SPI enable generation programmable
- SPI enable assertion
- SPI enable polarity
- SPI clock frequency
- SPI clock phase
- SPI clock polarity

The consistency between SPI word length, clock phase and clock polarity of the master SPI device and the communicating slave device remains under software responsibility.

#### *24.3.1.3.1 Programmable Word Length*

McSPI supports any SPI word from 4 to 32 bits long.

The SPI word length can be changed between transmissions to allow a master device to communicate with peripheral slaves having different requirements.

#### *24.3.1.3.2 Programmable SPI Enable Generation*

McSPI is able to generate or not generate SPI enable. If management of chip select is de-asserted, a point-to-point connection is mandatory. Only a single master of a slave device can be connected to the SPI bus.

#### *24.3.1.3.3 Programmable SPI Enable (SPIEN)*

The polarity of the SPIEN signals is programmable. SPIEN signals can be active high or low.

The assertion of the SPIEN signals is programmable. SPIEN signals can be manually asserted or automatically asserted.

Two consecutive words for two different slave devices may go along with active SPIEN signals with different polarity.

#### *24.3.1.3.4 Programmable SPI Clock (SPICLK)*

The phase and the polarity of the SPI serial clock are programmable when McSPI is a SPI master device or a SPI slave device. The baud rate of the SPI serial clock is programmable when McSPI is a SPI master.

When McSPI is operating as a slave, the serial clock SPICLK is an input from the master.

#### *24.3.1.3.5 Bit Rate*

In Master Mode, an internal reference clock CLKSPIREF is used as an input of a programmable divider to generate bit rate of the serial clock SPICLK. Granularity of this clock divider can be changed.
#### *24.3.1.3.6 Polarity and Phase*

McSPI supports four sub-modes of the SPI format transfer that depend on the polarity (POL) and the phase (PHA) of the SPI serial clock (SPICLK). [Table](#page-8-0) 24-5 and [Figure](#page-8-1) 24-6 show a summary of the four sub-modes. Software selects one of four combinations of serial clock phase and polarity.

Two consecutive SPI words for two different slave devices may go along with active SPICLK signal with different phase and polarity.

**Table 24-5. Phase and Polarity Combinations**

| Polarity (POL) | Phase (PHA) | SPI Mode | Comments                                                    |
|----------------|-------------|----------|-------------------------------------------------------------|
| 0              | 0           | mode0    | SPICLK active high and sampling occurs on the rising edge.  |
| 0              | 1           | mode1    | SPICLK active high and sampling occurs on the falling edge. |
| 1              | 0           | mode2    | SPICLK active low and sampling occurs on the falling edge.  |
| 1              | 1           | mode3    | SPICLK active low and sampling occurs on the rising edge.   |

**Figure 24-6. Phase and Polarity Combinations**

#### *24.3.1.3.7 Transfer Format With PHA = 0*

This section describes the concept of a SPI transmission with the SPI mode0 and the SPI mode2.

In the transfer format with PHA = 0, SPIEN is activated a half cycle of SPICLK ahead of the first SPICLK edge.

In both master and slave modes, McSPI drives the data lines at the time of SPIEN is asserted.

Each data frame is transmitted starting with the MSB. At the extremity of both SPI data lines, the first bit of SPI word is valid a half-cycle of SPICLK after the SPIEN assertion.

Therefore, the first edge of the SPICLK line is used by the master to sample the first data bit sent by the slave. On the same edge, the first data bit sent by the master is sampled by the slave.

On the next SPICLK edge, the received data bit is shifted into the shift register, and a new data bit is transmitted on the serial data line.

This process continues for a total of pulses on the SPICLK line defined by the SPI word length programmed in the master device, with data being latched on odd numbered edges and shifted on even numbered edges.

[Figure](#page-9-0) 24-7 is a timing diagram of a SPI transfer for the SPI mode0 and the SPI mode2, when McSPI is master or slave, with the frequency of SPICLK equals to the frequency of CLKSPIREF. It should not be used as a replacement for SPI timing information and requirements detailed in the data manual.

When McSPI is in slave mode, if the SPIEN line is not de-asserted between successive transmissions then the content of the Transmitter register is not transmitted, instead the last received SPI word is transmitted.

In master mode, the SPIEN line must be negated and reasserted between each successive SPI word. This is because the slave select pin freezes the data in its shift register and does not allow it to be altered if PHA bit equals 0.

In 3-pin mode without using the SPIEN signal, the controller provides the same waveform but with SPIEN forced to low state. In slave mode, SPIEN is useless.


**Figure 24-7. Full Duplex Single Transfer Format with PHA = 0**
#### *24.3.1.3.8 Transfer Format With PHA = 1*

This section describes SPI full duplex transmission with the SPI mode1 and the SPI mode3.

In the transfer format with PHA = 1, SPIEN is activated a delay (tLead) ahead of the first SPICLK edge.

In both master and slave modes, McSPI drives the data lines on the first SPICLK edge.

Each data frame is transmitted starting with the MSB. At the extremity of both SPI data lines, the first bit of SPI word is valid on the next SPICLK edge, a half-cycle later of SPICLK. It is the sampling edge for both the master and slave.

When the third edge occurs, the received data bit is shifted into the shift register. The next data bit of the master is provided to the serial input pin of the slave.

This process continues for a total of pulses on the SPICLK line defined by the word length programmed in the master device, with data being latched on even numbered edges and shifted on odd numbered edges.

[Figure](#page-10-0) 24-8 is a timing diagram of a SPI transfer for the SPI mode1 and the SPI mode3, when McSPI is master or slave, with the frequency of SPICLK equals to the frequency of CLKSPIREF. It should not be used as a replacement for SPI timing information and requirements detailed in the data manual.

The SPIEN line may remain active between successive transfers. In 3-pin mode without using the SPIEN signal, the controller provides the same waveform but with SPIEN forced to low state. In slave mode SPIEN is useless.


**Figure 24-8. Full Duplex Single Transfer Format With PHA = 1**
### *24.3.2 Master Mode*

McSPI is in master mode when the bit MS of the register MCSPI\_MODULCTRL is cleared.

In master mode McSPI supports multi-channel communication with up to 4 independent SPI communication channel contexts. McSPI initiates a data transfer on the data lines (SPIDAT [1:0]) and generates clock (SPICLK) and control signals (SPIEN) to a single SPI slave device at a time.

#### **24.3.2.1 Dedicated Resources Per Channel**

In the following sections, the letter "i" indicates the channel number that can be 0, 1, 2 or 3. Each channel has the following dedicated resources:

- Its own channel enable, programmable with the bit EN of the register MCSPI\_CH(i)CTRL. Disabling the channel, outside data word transmission, remains under user responsibility.
- Its own transmitter register MCSPI\_TX on top of the common shift register. If the transmitter register is empty, the status bit TXS of the register MCSPI\_CH(i)STAT is set.
- Its own receiver register MCSPI\_RX on top of the common shift register. If the receiver register is full, the status bit RXS of the register MCSPI\_CH(i)STAT is set.
- A fixed SPI ENABLE line allocation (SPIEN[i] port for channel "i"), SPI enable management is optional.
- Its own communication configuration with the following parameters via the register MCSPI\_CH(i)CONF
  - Transmit/Receive modes, programmable with the bit TRM.
  - Interface mode (Two data pins or Single data pin) and data pins assignment, both programmable with the bits IS and DPE.
  - SPI word length, programmable with the bits WL.
  - SPIEN polarity, programmable with the bit EPOL.
  - SPIEN kept active between words, programmable with the bit FORCE.
  - Turbo mode, programmable with the bit TURBO.
  - SPICLK frequency, programmable with the bit CLKD, the granularity of clock division can be changed using CLKG bit, the clock ratio is then concatenated with MCSPI\_CH(i)CTRL[EXTCLK] value.
  - SPICLK polarity, programmable with the bit POL
  - SPICLK phase, programmable with the bit PHA.
  - Start bit polarity, programmable with the bit SBPOL
  - Use a FIFO Buffer or not (see the following note), programmable with FFER and FFEW, depending on transfer mode, (MCSPI\_CH(i)CONF[TRM]).
- Two DMA requests events, read and write, to synchronize read/write accesses of the DMA controller with the activity of McSPI. The DMA requests are enabled with the bits DMAR and DMAW.
- Three interrupts events

**Note:** When more than one channel has an FIFO enable bit field (FFER or FFEW) set, the FIFO will not be used on any channel. Software must ensure that only one enabled channel is configured to use the FIFO buffer.

The transfers will use the latest loaded parameters of the register MCSPI\_CH(i)CONF.

The configuration parameters SPIEN polarity, Turbo mode, SPICLK phase and SPICLK polarity can be loaded in the MCSPI\_CH(i)CONF register only when the channel is disabled. The user has the responsibility to change the other parameters of the MCSPI\_CH(i)CONF register when no transfer occurs on the SPI interface.
#### **24.3.2.2 Interrupt Events in Master Mode**

In master mode, the interrupt events related to the transmitter register state are TX\_empty and TX\_underflow. The interrupt event related to the receiver register state is RX\_full.

#### *24.3.2.2.1 TX\_empty*

The event TX\_empty is activated when a channel is enabled and its transmitter register becomes empty (transient event). Enabling channel automatically raises this event, except for the Master receive only mode. (See Section [24.3.2.5](#page-14-0)). When the FIFO buffer is enabled (MCSPI\_CH(i)CONF[FFEW] set to 1), the TX\_empty is asserted as soon as there is enough space in the buffer to write a number of bytes defined by MCSPI\_XFERLEVEL[AEL].

Transmitter register must be loaded to remove the source of the interrupt and the TX\_empty interrupt status bit must be cleared for interrupt line de-assertion (if event enabled as interrupt source) . (See [Section](#page-32-0) 24.3.4).

When FIFO is enabled, no new TX\_empty event will be asserted as soon as CPU has not performed the number of writes into the transmitter register defined by MCSPI\_XFERLEVEL[AEL]. It is the responsibility of CPU to perform the right number of writes.

#### *24.3.2.2.2 TX\_underflow*

The event TX\_underflow is activated when the channel is enabled and if the transmitter register or FIFO is empty (not updated with new data) at the time of shift register assignment.

The TX\_underflow is a harmless warning in master mode.

To avoid having TX\_underflow event at the beginning of a transmission, the event TX\_underflow is not activated when no data has been loaded into the transmitter register since channel has been enabled.

To avoid having a TX\_underflow event, the Transmit Register (MCSPI\_TX(i)) should be loaded as infrequently as possible.

TX\_underflow interrupt status bit must be cleared for interrupt line de-assertion (if event enable as interrupt source).

**Note:** When more than one channel has an FIFO enable bit field (FFER or FFEW) set, the FIFO will not be used on any channel. Software must ensure that only one enabled channel is configured to use the FIFO buffer.

#### *24.3.2.2.3 RX\_ full*

The event RX\_full is activated when channel is enabled and receiver register becomes filled (transient event). When FIFO buffer is enabled (MCSPI\_CH(i)CONF[FFER] set to 1), the RX\_full is asserted when the number of bytes in the buffer equals the level defined by MCSPI\_XFERLEVEL[AFL].

Receiver register must be read to remove source of interrupt and RX\_full interrupt status bit must be cleared for interrupt line de-assertion (if event enabled as interrupt source).

When the FIFO is enabled, no new RX\_FULL event will be asserted once the CPU has read the number of bytes defined by MCSPI\_XFERLEVEL[AFL]. It is the responsibility of the CPU to perform the correct number of read operations.

#### *24.3.2.2.4 End of Word Count*

The event end of word (EOW) count is activated when channel is enabled and configured to use the builtin FIFO. This interrupt is raised when the controller had performed the number of transfers defined in the MCSPI\_XFERLEVEL[WCNT] register. If the value was programmed to 0000h, the counter is not enabled and this interrupt is not generated.

The EOW count interrupt also indicates that the SPI transfer has halted on the channel using the FIFO buffer.

The EOW interrupt status bit must be cleared for interrupt line de-assertion (if event enable as interrupt source).
#### **24.3.2.3 Master Transmit and Receive Mode**

This mode is programmable per channel (bit TRM of register MCSPI\_CH(i)CONF).

The channel access to the shift registers, for transmission/reception, is based on its transmitter and receiver register state and round robin arbitration.

The channel that meets the rules below is included in the round robin list of already active channels scheduled for transmission and/or reception. The arbiter skips the channel that does not meet the rules and search for the next following enabled channel, in rotation.

**Rule 1:** Only enabled channels (bit EN of the register MCSPI\_CH(i)CTRL), can be scheduled for transmission and/or reception.

**Rule 2:** An enabled channel can be scheduled if its transmitter register is not empty (bit TXS of the register MCSPI\_CH(i)STAT) or its FIFO is not empty when the buffer is used for the corresponding channel (bit FFE of the register MCSPI\_CH(i)STAT) at the time of shift register assignment. If the transmitter register or FIFO is empty, at the time of shift register assignment, the event TX\_underflow is activated and the next enabled channel with new data to transmit is scheduled. (See also transmit only mode).

**Rule 3:** An enabled channel can be scheduled if its receive register is not full (bit RXS of the register MCSPI\_CH(i)STAT)) or its FIFO is not full when the buffer is used for the corresponding channel (bit FFF of the register MCSPI\_CH(i)STAT) at the time of shift register assignment. (See also receive only mode). Therefore the receiver register of FIFO cannot be overwritten. The RX\_overflow bit, in the MCSPI\_IRQSTATUS register is never set in this mode.

On completion of SPI word transfer (bit EOT of the register MCSPI\_CH(i)STAT is set) the updated transmitter register for the next scheduled channel is loaded into the shift register. This bit is meaningless when using the Buffer for this channel. The serialization (transmit and receive) starts according to the channel communication configuration. On serialization completion the received data is transferred to the channel receive register.

The built-in FIFO is available in this mode and if configured in one data direction, transmit or receive, then the FIFO is seen as a unique 64-byte buffer. If configured in both data directions, transmit and receive, then the FIFO is split into two separate 32-byte buffers with their own address space management. In this last case, the definition of AEL and AFL levels is based on 32 bytes and is under CPU responsibility.

#### **24.3.2.4 Master Transmit-Only Mode**

This mode eliminates the need for the CPU to read the receiver register (minimizing data movement) when only transmission is meaningful.

The master transmit only mode is programmable per channel (bits TRM of the register MCSPI\_CH(i)CONF).

In master transmit only mode, transmission starts after data is loaded into the transmitter register.

**Rule 1** and **Rule 2**, defined above, are applicable in this mode.

**Rule 3**, defined above, is not applicable: In master transmit only mode, the receiver register or FIFO state "full" does not prevent transmission, and the receiver register is always overwritten with the new SPI word. This event in the receiver register is not significant when only transmission is meaningful. So, the RX\_overflow bit, in the MCSPI\_IRQSTATUS register is never set in this mode.

The McSPI module automatically disables the RX\_full interrupt status. The corresponding interrupt request and DMA Read request are not generated in master transmit only mode.

The status of the serialization completion is given by the bit EOT of the register MCSPI\_CH(i)STAT. This bit is meaningless when using the Buffer for this channel.

The built-in FIFO is available in this mode and can be configured with FFEW bit field in the MCSPI\_CH(i)CONF register, then the FIFO is seen as a unique 64-byte buffer.
#### **24.3.2.5 Master Receive-Only Mode**

This mode eliminates the need for the CPU to refill the transmitter register (minimizing data movement) when only reception is meaningful.

The master receive mode is programmable per channel (bits TRM of the register MCSPI\_CH(i)CONF).

The master receive only mode enables channel scheduling only on empty state of the receiver register.

**Rule 1** and **Rule 3**, defined above, are applicable in this mode.

**Rule 2**, defined above, is not applicable: In master receive only mode, after the first loading of the transmitter register of the enabled channel, the transmitter register state is maintained as full. The content of the transmitter register is always loaded into the shift register, at the time of shift register assignment. So, after the first loading of the transmitter register, the bits TX\_empty and TX\_underflow, in the MCSPI\_IRQSTATUS register are never set in this mode.

The status of the serialization completion is given by the bit EOT of the register MCSPI\_CH(i)STAT. The bit RX\_full in the MCSPI\_IRQSTATUS register is set when a received data is loaded from the shift register to the receiver register. This bit is meaningless when using the Buffer for this channel.

The built-in FIFO is available in this mode and can be configured with FFER bit field in the MCSPI\_CH(i)CONF register, then the FIFO is seen as a unique 64-byte buffer.

#### **24.3.2.6 Single-Channel Master Mode**

When the SPI is configured as a master device with a single enabled channel, the assertion of the SPIM\_CSX signal can be controlled in two different ways:

- In 3 pin mode : MCSPI\_MODULCTRL[1] PIN34 and MCSPI\_MODULCTRL[0] SINGLE bit are set to 1, the controller transmit SPI word as soon as transmit register or FIFO is not empty.
- In 4 pin mode : MCSPI\_MODULCTRL[1] PIN34 bit is cleared to 0 and MCSPI\_MODULCTRL[0] SINGLE bit is set to 1, SPIEN assertion/deassertion controlled by Software. (See Section [24.3.2.6.1](#page-14-1)) using the MCSPI\_CH(i)CONF[20] FORCE bit.

#### *24.3.2.6.1 Programming Tips When Switching to Another Channel*

When a single channel is enabled and data transfer is ongoing:

- Wait for completion of the SPI word transfer (bit EOT of the register MCSPI\_CH(i)STAT is set) before disabling the current channel and enabling a different channel.
- Disable the current channel first, and then enable the other channel.

#### *24.3.2.6.2 Keep SPIEN Active Mode (Force SPIEN)*

Continuous transfers are manually allowed by keeping the SPIEN signal active for successive SPI words transfer. Several sequences (configuration/enable/disable of the channel) can be run without deactivating the SPIEN line. This mode is supported by all channels and any master sequence can be used (transmitreceive, transmit-only, receive-only).

Keeping the SPIEN active mode is supported when:

- A single channel is used (bit MCSPI\_MODULCTRL[Single] is set to 1).
- Transfer parameters of the transfer are loaded in the configuration register (MCSPI\_CH(i)CONF) in the appropriate channel.

The state of the SPIEN signal is programmable.

- Writing 1 into the bit FORCE of the register MCSPI\_CH(i)CONF drives high the SPIEN line when MCSPI\_CH(i)CONF[EPOL] is set to zero, and drives it low when MCSPI\_CH(i)CONF[EPOL] is set.
- Writing 0 into the bit FORCE of the register MCSPI\_CH(i)CONF drives low the SPIEN line when MCSPI\_CH(i)CONF[EPOL] is set to zero, and drives it high when MCSPI\_CH(i)CONF[EPOL] is set.
- A single channel is enabled (MCSPI\_CH(i)CTRL[En] set to 1) . The first enabled channel activates the SPIEN line.

Once the channel is enabled, the SPIEN signal is activated with the programmed polarity.
As in multi-channel master mode, the start of the transfer depends on the status of the transmitter register, the status of the receiver register and the mode defined by the bits TRM in the configuration register (transmit only, receive only or transmit and receive) of the enabled channel.

The status of the serialization completion of each SPI word is given by the bit EOT of the register MCSPI\_CH(i)STAT. The bit RX\_full in the MCSPI\_IRQSTATUS register is set when a received data is loaded from the shift register to the receiver register.

A change in the configuration parameters is propagated directly on the SPI interface. If the SPIEN signal is activated the user must insure that the configuration is changed only between SPI words, in order to avoid corrupting the current transfer.

**NOTE:** The SPIEN polarity, the SPICLK phase and SPICLK polarity must not be modified when the SPIEN signal is activated. The Transmit/Receive mode, programmable with the bit TRM can be modified only when the channel is disabled. The channel can be disabled and enabled while the SPIEN signal is activated.

The delay between SPI words that requires the connected SPI slave device to switch from one configuration (transmit only for instance) to another (receive only for instance) must be handled under software responsibility.

At the end of the last SPI word, the channel must be deactivated (MCSPI\_CH(i)CTRL[En] is cleared to 0) and the SPIEN can be forced to its inactive state (MCSPI\_CH(i)CONF[Force]).

[Figure](#page-15-0) 24-9 and [Figure](#page-15-1) 24-10 show successive transfers with SPIEN kept active low with a different configuration for each SPI word in respectively single data pin interface mode and two data pins interface mode. The arrows indicate when the channel is disabled before a change in the configuration parameters and enabled again.

**Figure 24-9. Continuous Transfers With SPIEN Maintained Active (Single-Data-Pin Interface Mode)**


**Figure 24-10. Continuous Transfers With SPIEN Maintained Active (Dual-Data-Pin Interface Mode)**

**NOTE:** The turbo mode is also supported for the Keep SPIEN active mode when the following conditions are met:

- A single channel will be explicitly used (bit MCSPI\_MODULCTRL[Single] is set to 1).
- The turbo mode is enabled in the configuration of the channel (bit Turbo of the register MCSPI\_CH(i)CONF).

#### *24.3.2.6.3 Turbo Mode*

The purpose of the Turbo mode is to improve the throughput of the SPI interface when a single channel is enabled, by allowing transfers until the shift register and the receiver register are full.

This mode is programmable per channel (bit Turbo of the register MCSPI\_CH(i)CONF). When several channels are enabled, the bit Turbo of the registers MCSPI\_CH(i)CONF has no effect, and the channel access to the shift registers remains as described in Section [24.3.2.3.](#page-13-0)

In Turbo mode, **Rule 1** and **Rule 2** defined in Section [24.3.2.3](#page-13-0) are applicable but Rule 3 is not applicable. An enabled channel can be scheduled if its receive register is full (bit RXS of the register MCSPI\_CH(i)STAT) at the time of shift register assignment until the shift register is full.

In Turbo mode, **Rule 1** and **Rule 2** defined in Section [24.3.2.3](#page-13-0) are applicable but Rule 3 is not applicable. An enabled channel can be scheduled if its receive register is full (bit RXS of the register MCSPI\_CH(i)STAT) at the time of shift register assignment until the shift register is full.

The receiver register cannot be overwritten in Turbo mode. In consequence the RX\_overflow bit, in MCSPI\_IRQSTATUS register is never set in this mode.

#### **24.3.2.7 Start Bit Mode**

The purpose of the start bit mode is to add an extended bit before the SPI word transmission specified by word length WL. This feature is only available in master mode.

This mode is programmable per channel using the start bit enable (SBE) bit of the register MCSPI\_CH(i)CONF).

The polarity of the extended bit is programmable per channel and it indicates whether the next SPI word must be handled as a command when SBPOL is cleared to 0 or as a data or a parameter when SBPOL is set to 1. Moreover start bit polarity SBPOL can be changed dynamically during start bit mode transfer without disabling the channel for reconfiguration, in this case you have the responsibility to configure the SBPOL bit before writing the SPI word to be transmitted in TX register.

The start bit mode could be used at the same time as turbo mode and/or manual chip select mode. In this case only one channel could be used, no round-robin arbitration is possible.

#### **24.3.2.8 Chip-Select Timing Control**

The chip select timing control is only available in master mode with automatic chip select generation (FORCE bit field is cleared to 0), to add a programmable delay between chip select assertion and first clock edge or chip select removal and last clock edge. The option is available only in 4 pin mode MCSPI\_MODULCTRL[1] PIN34 is cleared to 0.

This mode is programmable per channel (bit TCS of the register MCSPI\_CH(i)CONF). [Figure](#page-18-0) 24-12 shows the chip-select SPIEN timing control.


**Figure 24-12. Chip-Select SPIEN Timing Controls**

**NOTE:** Because of the design implementation for transfers using a clock divider ratio set to 1 (clock bypassed), a half cycle must be added to the value between chip-select assertion and the first clock edge with PHA = 1 or between chip-select removal and the last clock edge with PHA = 0.

With an odd clock divider ratio which occurs when granularity is one clock cycle, that means that MCSPI\_CH(i)CONF[CLKG] is set to 1 and MCSPI\_CH(i)CONF[CLKD] has an even value, the clock duty cycle is not 50%, then one of the high level or low level duration is selected to be added to TCS delay.

[Table](#page-18-1) 24-6 summarizes all delays between chip select and first (setup) or last (hold) clock edge.

In 3-pin mode this option is useless, the chip select SPIEN is forced to low state.

**Clock Ratio Fratio Clock Phase PHA Chip Select ↔ Clock Edge Delay Setup Hold** 1 0 T\_ref × (TCS + ½) T\_ref × (TCS + 1) 1 T\_ref × (TCS + 1) T\_ref × (TCS + ½) Even ≥ 2 x T\_ref × Fratio × (TCS + ½) T\_ref × Fratio × (TCS + ½) Odd ≥ 3 (only with MCSPI\_CH(i)CONF[CLK G] set to 1 0 T\_ref × [{Fratio × TCS) + (Fratio + ½)] T\_ref × [{Fratio × TCS) + (Fratio + ½)] 1 T\_ref × [{Fratio × TCS) + (Fratio - ½)] T\_ref × [{Fratio × TCS) + (Fratio - ½)]

**Table 24-6. Chip Select ↔ Clock Edge Delay Depending on Configuration**

T\_ref = CLKSPIREF period in ns. Fratio = SPI clock division ratio

The clock divider ratio depends on divider granularity MCSPI\_CH(i)CONF[CLKG]:

- MCSPI\_CH(i)CONF[CLKG] = 0 : granularity is power of two. Fratio = 2 MCSPI\_CH(i)CONF[CLKD]
- MCSPI\_CH(i)CONF[CLKG] = 1 : granularity is one cycle. Fratio = MCSPI\_CH(i)CTRL[EXTCLK] × MCSPI\_CH(i)CONF[CLKD] + 1
#### 24.3.2.9 Clock Ratio Granularity

By default the clock division ratio is defined by the register MCSPI\_CH(i)CONF[CLKD] with power of two granularity leading to a clock division in range 1 to 32768, in this case the duty cycle is always 50%. With bit MCSPI\_CH(i)CONF[CLKG] the clock division granularity can be changed to one clock cycle, in that case the register MCSPI\_CH(i)CTRL[EXTCLK] is concatenated with MCSPI\_CH(i)CONF[CLKD] to give a 12-bit width division ratio in range 1 to 4096.

When granularity is one clock cycle (MCSPI\_CH(i)CONF[CLKG] set to 1), for odd value of clock ratio the clock duty cycle is kept to 50-50 using falling edge of clock reference CLKSPIREF.

Table 24-7. CLKSPIO High/Low Time Computation

| Clock Ratio F <sub>ratio</sub> | CLKSPIO High Time            | CLKSPIO Low Time                |
|--------------------------------|------------------------------|---------------------------------|
| 1                              | T <sub>high</sub> _ref       | T <sub>low</sub> _ref           |
| Even ≥ 2                       | $t_ref \times (F_{ratio}/2)$ | t_ref × (F <sub>ratio</sub> /2) |
| Odd ≥ 3                        | $t_ref \times (F_{ratio}/2)$ | $t_ref \times (F_{ratio}/2)$    |

 $T_ref = CLKSPIREF$  period in ns.  $T_{low}ref = CLKSPIREF$  high Time period in ns.  $T_{low}ref = CLKSPIREF$  low Time period in ns.  $F_{ratio} = SPI$  clock division ratio

 $F_{ratio} = MCSPI\_CH(i)CTRL[EXTCLK] \times MCSPI\_CH(i)CONF[CLKD] + 1$ 

For odd ratio value the duty cycle is calculated as below:

Duty\_cycle = ½

Granularity examples: With a clock source frequency of 48 MHz:

**Table 24-8. Clock Granularity Examples** 

| MCSPI_CH<br>(i)CTRL | MCSPI_CH<br>(i)CONF | MCSPI_CH<br>(i)CONF |             | MCSPI_CH<br>(i)CONF | MCSPI_CH<br>(i)CONF | Thigh | Tlow  | Tperiod | Duty  | Fout  |
|---------------------|---------------------|---------------------|-------------|---------------------|---------------------|-------|-------|---------|-------|-------|
| EXTCLK              | CLKD                | CLKG                | $F_{ratio}$ | PHA                 | POL                 | (ns)  | (ns)  | (ns)    | Cycle | (MHz) |
| Χ                   | 0                   | 0                   | 1           | Χ                   | Χ                   | 10.4  | 10.4  | 20.8    | 50-50 | 48    |
| Χ                   | 1                   | 0                   | 2           | Χ                   | Χ                   | 20.8  | 20.8  | 41.6    | 50-50 | 24    |
| X                   | 2                   | 0                   | 4           | X | X | 41.6  | 41.6  | 83.2    | 50-50 | 12    |
| X                   | 3                   | 0                   | 8           | X | X | 83.2  | 83.2  | 166.4   | 50-50 | 6     |
| 0                   | 0                   | 1                   | 1           | X | X | 10.4  | 10.4  | 20.8    | 50-50 | 48    |
| 0                   | 1                   | 1                   | 2           | X | X | 20.8  | 20.8  | 41.6    | 50-50 | 24    |
| 0                   | 2                   | 1                   | 3           | 1                   | 0                   | 31,2  | 31,2  | 62.4    | 50-50 | 16    |
| 0                   | 2                   | 1                   | 3           | 1                   | 1                   | 31,2  | 31,2  | 62.4    | 50-50 | 16    |
| 0                   | 3                   | 1                   | 4           | X | X | 41.6  | 41.6  | 83.2    | 50-50 | 12    |
| 5                   | 0                   | 1                   | 81          | 1                   | 0                   | 842,4 | 842,4 | 1684.8  | 50-50 | 0.592 |
| 5                   | 7                   | 1                   | 88          | X | X | 915.2 | 915.2 | 1830.4  | 50-50 | 0.545 |
#### **24.3.2.10 FIFO Buffer Management**

The McSPI controller has a built-in 64-byte buffer in order to unload DMA or interrupt handler and improve data throughput.

This buffer can be used by only one channel and is selected by setting MCSPI\_CH(i)CONF[FFER] and/or MCSPI\_CH(i)CONF[FFEW] to 1.

If several channels are selected and several FIFO enable bit fields set to 1, the controller forces the buffer to be disabled for all channels. It is the responsibility of the driver to enable the buffer for only one channel.

The buffer can be used in the modes defined below:

- Master or Slave mode.
- Transmit only, Receive only or Transmit/Receive mode.
- Single channel or turbo mode, or in normal round robin mode. In round robin mode the buffer is used by only one channel.
- All word length MCSPI\_CH(i)CONF[WL] are supported.

Two levels AEL and AFL located in MCSPI\_XFERLEVEL register rule the buffer management. The granularity of these levels is one byte, then it is not aligned with SPI word length. It is the responsibility of the driver to set these values as a multiple of SPI word length defined in MCSPI\_CH(i)CONF[WL]. The number of byte written in the FIFO depends on word length (see [Table](#page-20-0) 24-9).

**Table 24-9. FIFO Writes, Word Length Relationship**

|                                    |            | SPI Word Length WL |              |
|------------------------------------|------------|--------------------|--------------|
|                                    | 3 ≤ WL ≤ 7 | 8 ≤ WL ≤ 15        | 16 ≤ WL ≤ 31 |
| Number of byte written in the FIFO | 1 byte     | 2 bytes            | 4 bytes      |

#### *24.3.2.10.1 Split FIFO*

The FIFO can be split into two part when module is configured in transmit/receive mode MCSPI\_CH(i)CONF[TRM] is cleared to 0 and MCSPI\_CH(i)CONF[FFER] and MCSPI\_CH(i)CONF[FFEW] asserted. Then system can access a 32-byte depth FIFO per direction.

The FIFO buffer pointers are reset when the corresponding channel is enabled or FIFO configuration changes.
**Figure 24-13. Transmit/Receive Mode With No FIFO Used**


**Configuration:**

**MCSPI\_CH(i)CONF[TRM]=0x0 Transmit/receive mode MCSPI\_CH(i)CONF[FFRE]=0x0 FIFO disabled on receive path MCSPI\_CH(i)CONF[FFWE]=0x0 FIFO disabled on transmit path**

**Figure 24-14. Transmit/Receive Mode With Only Receive FIFO Enabled**


**Configuration:**

**MCSPI\_CH(i)CONF[TRM]=0x0 Transmit/receive mode MCSPI\_CH(i)CONF[FFRE]=0x1 FIFO enabled on receive path MCSPI\_CH(i)CONF[FFWE]=0x0 FIFO disabled on transmit path**
**Figure 24-15. Transmit/Receive Mode With Only Transmit FIFO Used**


**Configuration:**

**MCSPI\_CH(i)CONF[TRM]=0x0 Transmit/receive mode MCSPI\_CH(i)CONF[FFRE]=0x0 FIFO disabled on receive path MCSPI\_CH(i)CONF[FFWE]=0x1 FIFO enabled on transmit path**

**Figure 24-16. Transmit/Receive Mode With Both FIFO Direction Used**


**Configuration:**

**MCSPI\_CH(i)CONF[TRM]=0x0 Transmit/receive mode MCSPI\_CH(i)CONF[FFRE]=0x1 FIFO enabled on receive path MCSPI\_CH(i)CONF[FFWE]=0x0 FIFO disabled on transmit path**
**Figure 24-17. Transmit-Only Mode With FIFO Used**


**Configuration: MCSPI\_CH(i)CONF[TRM]=0x2 Transmit only mode MCSPI\_CH(i)CONF[FFRE]=0x1 FIFO enabled on transmit path MCSPI\_CH(i)CONF[FFWE] not applicable**

**Figure 24-18. Receive-Only Mode With FIFO Used**


**Configuration: MCSPI\_CH(i)CONF[TRM]=012 Receive only mode**

**MCSPI\_CH(i)CONF[FFRE]=0x1 FIFO enabled on receive path**

**MCSPI\_CH(i)CONF[FFWE] not applicable**
#### *24.3.2.10.2 Buffer Almost Full*

The bit field MCSPI\_XFERLEVEL[AFL] is needed when the buffer is used to receive SPI word from a slave (MCSPI\_CH(i)CONF[FFER] must be set to 1). It defines the almost full buffer status.

When FIFO pointer reaches this level an interrupt or a DMA request is sent to the CPU to enable system to read AFL+1 bytes from receive register. Be careful AFL+1 must correspond to a multiple value of MCSPI\_CH(i)CONF[WL].

When DMA is used, the request is de-asserted after the first receive register read.

No new request will be asserted until the system has performed the correct number of read operations from the buffer.

**Figure 24-19. Buffer Almost Full Level (AFL)**


**<sup>\*</sup> non-DMA mode only. In DMA mode, the DMA RX request is asserted to its active level under identical conditions.**

**.**

**NOTE:** SPI\_IRQSTATUS register bits are not available in DMA mode. In DMA mode, the SPIm\_DMA\_RX*n* request is asserted on the same conditions as the SPI\_IRQSTATUS RX*n*\_FULL flag.
#### *24.3.2.10.3 Buffer Almost Empty*

The bitfield MCSPI\_XFERLEVEL[AEL] is needed when the buffer is used to transmit SPI word to a slave (MCSPI\_CH(i)CONF[FFEW]must be set to 1). It defines the almost empty buffer status.

When FIFO pointer has not reached this level an interrupt or a DMA request is sent to the CPU to enable system to write AEL+1 bytes to transmit register. Be careful AEL+1 must correspond to a multiple value of MCSPI\_CH(i)CONF[WL].

When DMA is used, the request is de-asserted after the first transmit register write.

No new request will be asserted until the system has performed the correct number of write operations.


**<sup>\*</sup> non-DMA mode only. In DMA mode, the DMA TX request is asserted to its active level under identical conditions.**

#### *24.3.2.10.4 End of Transfer Management*

**.**

When the FIFO buffer is enabled for a channel, the user should configure the MCSPI\_XFERLEVEL register, the AEL and AFL levels, and, especially, the WCNT bit field to define the number of SPI word to be transferred using the FIFO. This should be done before enabling the channel.

This counter allows the controller to stop the transfer correctly after a defined number of SPI words have been transferred. If WNCT is cleared to 0, the counter is not used and the user must stop the transfer manually by disabling the channel, in this case the user doesn't know how many SPI transfers have been done. For receive transfer, software shall poll the corresponding FFE bit field and read the Receive register to empty the FIFO buffer.

When End Of Word count interrupt is generated, the user can disable the channel and poll on MCSPI\_CH(i)STAT[FFE] register to know if SPI word is still there in FIFO buffer and read last words.
#### *24.3.2.10.5 Multiple SPI Word Access*

The CPU has the ability to perform multiple SPI word access to the receive or transmit registers within a single 32-bit OCP access by setting the bit field MCSPI\_MODULCTRL[MOA] to '1' under specific conditions:

- The channel selected has the FIFO enable.
- Only FIFO sense enabled support the kind of access.
- The bit field MCSPI\_MODULCTRL[MOA] is set to 1
- Only 32-bit OCP access and data width can be performed to receive or transmit registers, for other kind of access the CPU must de-assert MCSPI\_MODULCTRL[MOA] bit fields.
- The Level MCSPI\_XFERLEVEL[AEL] and MCSPI\_XFERLEVEL[AFL] must be 32-bit aligned , it means that AEL[0] = AEL[1] = 1 or AFL[0] = AFL[1] = 1.
- If MCSPI\_XFERLEVEL[WCNT] is used it must be configured according to SPI word length.
- The word length of SPI words allows to perform multiple SPI access, that means that MCSPI\_CH(i)CONF[WL] < 16.

Number of SPI word access depending on SPI word length:

- 3 ≤ WL ≤ 7, SPI word length smaller or equal to byte length, four SPI words accessed per 32-bit OCP read/write. If word count is used (MCSPI\_XFERLEVEL[WCNT]), set the bit field to WCNT[0]=WCNT[1]=0.
- 8 ≤ WL ≤ 15, SPI word length greater than byte or equal to 16-bit length, two SPI words accessed per 32-bit OCP read/write. If word count is used (MCSPI\_XFERLEVEL[WCNT]), set the bit field to WCNT[0]= 0.
- 16 ≤ WL multiple SPI word access not applicable.

#### **24.3.2.11 First SPI Word Delayed**

The McSPI controller has the ability to delay the first SPI word transfer to give time for system to complete some parallel processes or fill the FIFO in order to improve transfer bandwidth. This delay is applied only on first SPI word after SPI channel enabled and first write in Transmit register. It is based on output clock frequency.

This option is meaningful in master mode and single channel mode, MCSPI\_MODULCTRL[SINGLE] = 1.


Few delay values are available: No delay, 4/8/16/32 SPI cycles.

Its accuracy is half cycle in clock bypass mode and depends on clock polarity and phase.
#### **24.3.2.12 3- or 4-Pin Mode**

External SPI bus interface can be configured to use a restricted set of pins using the bit field MCSPI\_MODULCTRL[PIN34] and depending on targeted application:

- If MCSPI\_MODULCTRL[PIN34] is cleared to 0 (default value) the controller is in 4-pin mode using the SPI pins SPICLK, SOMI, SIMO and chip enable CS.
- If MCSPI\_MODULCTRL[PIN34] is set to 1 the controller is in 3-pin mode using the SPI pins SPICLK, SOMI and SIMO.

In 3-pin mode it is mandatory to put the controller in single channel master mode (MCSPI\_MODULCTRL[SINGLE] asserted) and to connect only one SPI device on the bus.


**Figure 24-22. 3-Pin Mode System Overview**

In 3-pin mode all options related to chip select management are useless:

- MCSPI\_CHxCONF[EPOL]
- MCSPI\_CHxCONF[TCS0]
- MCSPI\_CHxCONF[FORCE]

The chip select pin SPIEN is forced to '0' in this mode.
### *24.3.3 Slave Mode*

McSPI is in slave mode when the bit MS of the register MCSPI\_MODULCTRL is set.

In slave mode, McSPI can be connected to up to 4 external SPI master devices. McSPI handles transactions with a single SPI master device at a time.

In slave mode, McSPI initiates data transfer on the data lines (SPIDAT[1;0]) when it receives an SPI clock (SPICLK) from the external SPI master device.

The controller is able to work with or without a chip select SPIEN depending on MCSPI\_MODULCTRL[PIN34] bit setting. It also supports transfers without a dead cycle between two successive words.

#### **24.3.3.1 Dedicated Resources**

In slave mode, enabling a channel that is not channel 0 has no effect. Only channel 0 can be enabled. The channel 0, in slave mode has the following resources:

- Its own channel enable, programmable with the bit EN of the register MCSPI\_CH0CTRL. This channel should be enabled before transmission and reception. Disabling the channel, outside data word transmission, remains under user responsibility.
- Any of the 4 ports SPIEN[3:0] can be used as a slave SPI device enable. This is programmable with the bits SPIENSLV of the register MCSPI\_CH0CONF.
- Its own transmitter register MCSPI\_TX on top of the common shift register. If the transmitter register is empty, the status bit TXS of the register MCSPI\_CH0STAT is set. When McSPI is selected by an external master (active signal on the SPIEN port assigned to channel 0), the transmitter register content of channel0 is always loaded in shift register whether it has been updated or not. The transmitter register should be loaded before McSPI is selected by a master.
- Its own receiver register MCSPI\_RX on top of the common shift register. If the receiver register is full, the status bit RXS of the register MCSPI\_CH0STAT is set.

**NOTE:** The transmitter register and receiver registers of the other channels are not used. Read from or Write in the registers of a channel other than 0 has no effect.

- Its own communication configuration with the following parameters via the register MCSPI\_CH0CONF:
  - Transmit/Receive modes, programmable with the bit TRM.
  - Interface mode (Two data pins or Single data pin) and data pins assignment, both programmable with the bits IS and DPE.
  - SPI word length, programmable with the bits WL.
  - SPIEN polarity, programmable with the bit EPOL.
  - SPICLK polarity, programmable with the bit POL.
  - SPICLK phase, programmable with the bit PHA.
  - Use a FIFO buffer or not, programmable with FFER and FFEW, depending on transfer mode TRM.

The SPICLK frequency of a transfer is controlled by the external SPI master connected to McSPI. The bits CLKD0 of the MCSPI\_CH0CONF register are not used in slave mode.

**NOTE:** The configuration of the channel can be loaded in the MCSPI\_CH0CONF register only when the channel is disabled.

- Two DMA requests events, read and write, to synchronize read/write accesses of the DMA controller with the activity of McSPI. The DMA requests are enabled with the bits DMAR and DMAW of the MCSPI\_CH0CONF register.
- Four interrupts events.

[Figure](#page-29-0) 24-23 shows an example of four slaves wired on a single master device.
Device spim\_cs0 spim\_somi spim\_simo spim\_clk SIMO SIMO SIMO SIMO SCLK SCLK SCLK SCLK SOMI SOMI SOMI SOMI CS CS CS CS Generic SPI master device Generic SPI slave device Generic SPI slave device Generic SPI slave device McSPI module m

**Figure 24-23. Example of SPI Slave with One Master and Multiple Slave Devices on Channel 0**

#### **24.3.3.2 Interrupt Events in Slave Mode**

The interrupt events related to the transmitter register state are TX\_empty and TX\_underflow. The interrupt events related to the receiver register state are RX\_full and RX\_overflow.

#### *24.3.3.2.1 TX\_EMPTY*

The event TX\_empty is activated when the channel is enabled and its transmitter register becomes empty. Enabling channel automatically raises this event. When FIFO buffer is enabled (MCSPI\_CH(i)CONF[FFEW] set to 1), the TX\_empty is asserted as soon as there is enough space in buffer to write a number of byte defined by MCSPI\_XFERLEVEL[AEL].

Transmitter register must be load to remove source of interrupt and TX\_empty interrupt status bit must be cleared for interrupt line de-assertion (if event enable as interrupt source).

When FIFO is enabled, no new TX\_empty event will be asserted unless the host performs the number of writes to the transmitter register defined by MCSPI\_XFERLEVEL[AEL]. It is the responsibility of the Local Host to perform the right number of writes.

108-017
#### *24.3.3.2.2 TX\_UNDERFLOW*

The event TX\_underflow is activated when channel is enabled and if the transmitter register or FIFO (if use of buffer is enabled) is empty (not updated with new data) when an external master device starts a data transfer with McSPI (transmit and receive).

When the FIFO is enabled, the data read while the underflow flag is set will not be the last word written to the FIFO.

The TX\_underflow indicates an error (data loss) in slave mode.

To avoid having TX\_underflow event at the beginning of a transmission, the event TX\_underflow is not activated when no data has been loaded into the transmitter register since channel has been enabled.

TX\_underflow interrupt status bit must be cleared for interrupt line de-assertion (if event enable as interrupt source).

#### *24.3.3.2.3 RX\_FULL*

The event RX\_FULL is activated when channel is enabled and receiver becomes filled (transient event). When FIFO buffer is enabled (MCSPI\_CH(i)CONF[FFER] set to 1), the RX\_FULL is asserted as soon as there is a number of bytes holds in buffer to read defined by MCSPI\_XFERLEVEL[AFL].

Receiver register must be read to remove source of interrupt and RX\_full interrupt status bit must be cleared for interrupt line de-assertion (if event enable as interrupt source).

When FIFO is enabled, no new RX\_FULL event will be asserted unless the host has performed the number of reads from the receive register defined by MCSPI\_XFERLEVEL[AFL]. It is the responsibility of Local Host to perform the right number of reads.

#### *24.3.3.2.4 RX\_OVERFLOW*

The RX0\_OVERFLOW event is activated in slave mode in either transmit-and-receive or receive-only mode, when a channel is enabled and the SPI\_RX*n* register or FIFO is full when a new SPI word is received. The SPI\_RX*n* register is always overwritten with the new SPI word. If the FIFO is enabled, data within the FIFO is overwritten, it must be considered as corrupted. The RX0\_OVERFLOW event should not appear in slave mode using the FIFO.

The RX0\_OVERFLOW indicates an error (data loss) in slave mode.

The SPI\_IRQSTATUS[3] RX0\_OVERFLOW interrupt status bit must be cleared for interrupt line deassertion (if the event is enabled as the interrupt source).

#### *24.3.3.2.5 End of Word Count*

The event end of word (EOW) count is activated when channel is enabled and configured to use the builtin FIFO. This interrupt is raised when the controller had performed the number of transfer defined in MCSPI\_XFERLEVEL[WCNT] register. If the value was programmed to 0000h, the counter is not enabled and this interrupt is not generated.

The EOW count interrupt also indicates that the SPI transfer has halted on the channel using the FIFO buffer.

The EOW interrupt status bit must be cleared for interrupt line de-assertion (if event enable as interrupt source).

#### **24.3.3.3 Slave Transmit-and-Receive Mode**

The slave transmit and receive mode is programmable (TRM bit cleared to 0 in the register MCSPI\_CH(i)CONF).

After the channel is enabled, transmission and reception proceeds with interrupt and DMA request events.

In slave transmit and receive mode, transmitter register should be loaded before McSPI is selected by an external SPI master device.
Transmitter register or FIFO (if enabled) content is always loaded into the shift register whether it has been updated or not. The event TX\_underflow is activated accordingly, and does not prevent transmission.

On completion of SPI word transfer (bit EOT of the register MCSPI\_CH(i)STAT is set) the received data is transferred to the channel receive register. This bit is meaningless when using the Buffer for this channel.

The built-in FIFO is available in this mode and can be configured in one data direction, transmit or receive, then the FIFO is seen as a unique 64-byte buffer. It can also be configured in both data directions, transmit and receive, then the FIFO is split into two separate 32-byte buffers with their own address space management.

#### **24.3.3.4 Slave Receive-Only Mode**

The slave receive-only mode is programmable (MCSPI\_CH(i)CONF[TRM] set to 01).

In receive-only mode, the transmitter register should be loaded before McSPI is selected by an external SPI master device. Transmitter register or FIFO (if enabled) content is always loaded into the shift register whether it has been updated or not. The event TX\_underflow is activated accordingly, and does not prevent transmission.

When an SPI word transfer completes (the MCSPI\_CH(i)STAT[EOT] bit (with *I* = 0) is set to 1), the received data is transferred to the channel receive register.

To use McSPI as a slave receive-only device with MCSPI\_CH(i)CONF[TRM]=00, the user has the responsibility to disable the TX\_empty and TX\_underflow interrupts and DMA write requests due to the transmitter register state.

On completion of SPI word transfer (bit EOT of the register MCSPI\_CH(i)STAT is set) the received data is transferred to the channel receive register. This bit is meaningless when using the Buffer for this channel. The built-in FIFO is available in this mode and can be configured with FFER bit field in the MCSPI\_CH(i)CONF register, then the FIFO is seen as a unique 64-byte buffer.

[Figure](#page-31-0) 24-24 shows an example of a half-duplex system with a master device on the left and a receiveonly slave device on the right. Each time one bit transfers out from the master, one bit transfers in to the slave. If WordA is 8 bits, then after eight cycles of the serial clock spim\_clk, WordA transfers from the master to the slave.


**Figure 24-24. SPI Half-Duplex Transmission (Receive-Only Slave)**

108-032
#### **24.3.3.5 Slave Transmit-Only Mode**

The slave transmit-only mode is programmable (MCSPI\_CH(i)CONF[TRM] set to 10). This mode eliminates the need for the CPU to read the receiver register (minimizing data movement) when only transmission is meaningful.

To use McSPI as a slave transmit-only device with MCSPI\_CH(i)CONF[TRM]=10, the user should disable the RX\_full and RX\_overflow interrupts and DMA read requests due to the receiver register state.

On completion of SPI word transfer the bit EOT of the register MCSPI\_CH(i)STAT is set. This bit is meaningless when using the Buffer for this channel.

The built-in FIFO is available in this mode and can be configured with FFER bit field in the MCSPI\_CH(i)CONF register, then the FIFO is seen as a unique 64-byte buffer.

[Figure](#page-32-1) 24-25 shows a half-duplex system with a master device on the left and a transmit-only slave device on the right. Each time a bit transfers out from the slave device, one bit transfers in the master. If WordB is 8-bits, then after eight cycles of the serial clock spim\_clk, WordB transfers from the slave to the master.


**Figure 24-25. SPI Half-Duplex Transmission (Transmit-Only Slave)**

### *24.3.4 Interrupts*

According to its transmitter register state and its receiver register state each channel can issue interrupt events if they are enabled.

The interrupt events are listed in the Section [24.3.2.2](#page-12-0) and in Section [24.3.3.2](#page-29-1).

Each interrupt event has a status bit, in the MCSPI\_IRQSTATUS register, which indicates service is required, and an interrupt enable bit, in the MCSPI\_IRQENABLE register, which enables the status to generate hardware interrupt requests.

When an interrupt occurs and it is later masked (IRQENABLE), the interrupt line is not asserted again even if the interrupt source has not been serviced.

McSPI supports interrupt driven operation and polling.
#### **24.3.4.1 Interrupt-Driven Operation**

Alternatively, an interrupt enable bit, in the MCSPI\_IRQENABLE register, can be set to enable each of the events to generate interrupt requests when the corresponding event occurs. Status bits are automatically set by hardware logic conditions.

When an event occurs (the single interrupt line is asserted), the CPU must:

- Read the MCSPI\_IRQSTATUS register to identify which event occurred,
- Read the receiver register that corresponds to the event in order to remove the source of an RX\_full event, or write into the transmitter register that corresponds to the event in order to remove the source of a TX\_empty event. No action is needed to remove the source of the events TX\_underflow and RX\_overflow.
- Write a 1 into the corresponding bit of MCSPI\_IRQSTATUS register to clear the interrupt status, and release the interrupt line.

The interrupt status bit should always be reset after the channel is enabled and before the event is enabled as an interrupt source.

#### **24.3.4.2 Polling**

When the interrupt capability of an event is disabled in the MCSPI\_IRQENABLE register, the interrupt line is not asserted and:

- The status bits in the MCSPI\_IRQSTATUS register can be polled by software to detect when the corresponding event occurs.
- Once the expected event occurs, CPU must read the receiver register that corresponds to the event in order to remove the source of an RX\_full event, or write into the transmitter register that corresponds to the event in order to remove the source of a TX\_empty event. No action is needed to remove the source of the events TX\_underflow and RX\_overflow.
- Writing a 1 into the corresponding bit of MCSPI\_IRQSTATUS register clears the interrupt status and does not affect the interrupt line state.

### *24.3.5 DMA Requests*

McSPI can be interfaced with a DMA controller. At system level, the advantage is to free the local host of the data transfers.

According to its transmitter register state, its receiver register state or FIFO level (if use of buffer for the channel) each channel can issue DMA requests if they are enabled.

The DMA requests need to be disabled in order to get TX and RX interrupts, in order to define either the end of the transfer or the transfer of the last words for the modes listed below:

- Master transmit-only
- Master normal receive-only mode
- Master turbo receive-only mode
- Slave transmit-only

There are two DMA request lines per channel. The management of DMA requests differ according to use of FIFO buffer or not.

#### **24.3.5.1 FIFO Buffer Disabled**

The DMA Read request line is asserted when the channel is enabled and a new data is available in the receive register of the channel. DMA Read request can be individually masked with the bit DMAR of the register MCSPI\_CH(i)CONF. The DMA Read request line is de-asserted on read completion of the receive register of the channel.

The DMA Write request line is asserted when the channel is enabled and the transmitter register of the channel is empty. DMA Write request can be individually masked with the bit DMAW of the register MCSPI\_CH(i)CONF. The DMA Write request line is de-asserted on load completion of the transmitter register of the channel.
Only one SPI word can be transmitted/received per OCP bus access to write/read the transmit or receive register.

#### **24.3.5.2 FIFO Buffer Enabled**

The DMA Read request line is asserted when the channel is enabled and a number of bytes defined in MCSPI\_XFERLEVEL[AFL] bit field is hold in FIFO buffer for the receive register of the channel. DMA Read request can be individually masked with the bit DMAR of the register MCSPI\_CH(i)CONF. The DMA Read request line is de-asserted on the first SPI word read completion of the receive register of the channel. No new DMA request will be asserted again as soon as user has not performed the right number of read accesses defined by MCSPI\_XFERLEVEL[AFL] it is under user responsibility.

The DMA Write request line is asserted when the channel is enabled and the number of bytes hold in FIFO buffer is below the level defined by the MCSPI\_XFERLEVEL[AEL] bit field. DMA Write request can be individually masked with the bit DMAW of the register MCSPI\_CH(i)CONF. The DMA Write request line is de-asserted on load completion of the first SPI word in the transmitter register of the channel. No new DMA request will be asserted again as soon as user has not performed the right number of write accesses defined by MCSPI\_XFERLEVEL[AEL] it is under user responsibility.

Only one SPI word can be transmitted/received per OCP bus access to write/read the transmit or receive FIFO.

#### **24.3.5.3 DMA 256-Bit Aligned Addresses**

The controller has two registers, MCSPI\_DAFTX and MCSPI\_DAFRX, used only with an enabled channel which manages the FIFO to be compliant the a DMA handler providing only 256-bit aligned addresses.

This features is activated when the bit field MCSPI\_MODULCTRL[FDDA] is set to '1' and only one enabled channel have its bit field MCSPI\_CH(i)CONF[FFEW] or MCSPI\_CH(i)CONF[FFER] enabled.

In this case the registers MCSPI\_TX(i) and MCSPI\_RX(i) are not used and data is managed through registers MCSPI\_DAFTX and MCSPI\_DAFRX.

### *24.3.6 Emulation Mode*

The MReqDebug input differentiates a regular access of a processor (application access), from an emulator access.

Application access: MReqDebug = 0

In functional mode, the consequences of a read of a receiver register MCSPI\_RX(i) are the following:

- The source of an RX(i)\_Full event in the MCSPI\_IRQSTATUS register is removed, if it was enabled in the MCSPI\_IRQENABLE register.
- The RX(i)S status bit in the MCSPI\_IRQSTATUS register is cleared.
- In master mode, depending on the round robin arbitration, and the transmitter register state, the channel may access to the shift register for transmission/reception.

Emulator access: MReqDebug = 1

In emulation mode, McSPI behavior is the same as in functional mode but a read of a receiver register MCSPI\_RX(i) is not intrusive:

- MCSPI\_RX(i) is still considered as not read. When the FIFO buffer is enabled, pointers are not updated.
- The source of an RX(i)\_Full event in the MCSPI\_IRQSTATUS register is not removed. The RX(i)S status bit in the MCSPI\_CH(i)STAT register is held steady.

In emulation mode, as in functional mode, based on the ongoing data transfers, the status bits of the MCSPI\_CH(i)STAT register may be optionally updated, the interrupt and DMA request lines may be optionally asserted.
### *24.3.7 Power Saving Management*

Independently of the module operational modes (Transmit and/or Receive), two modes of operations are defined from a power management perspective: normal and idle modes.

The two modes are temporally fully exclusive.

#### **24.3.7.1 Normal Mode**

Both the Interface, or OCP, clock and SPI clock (CLKSPIREF) provided to McSPI must be active for both master and slave modes. The auto-gating of the module OCP clock and SPI clock occurs when the following conditions are met:

- The bit AutoIdle of the register MCSPI\_SYSCONFIG is set.
- In master mode, there is no data to transmit or receive in all channels.
- In slave mode, the SPI is not selected by the external SPI master device and no OCP accesses.

Autogating of the module OCP clock and SPI clock stops when the following conditions are met:

- In master mode, an OCP access occurs.
- In slave mode, an OCP access occurs or McSPI is selected by an external SPI master device.

#### **24.3.7.2 Idle Mode**

The OCP clock and SPI clock provided to McSPI may be switched off on system power manager request and switched back on module request.

McSPI is compliant with the power management handshaking protocol: idle request from the system power manager, idle acknowledgement from McSPI.

The idle acknowledgement in response to an idle request from the system power manager varies according to a programmable mode in the MCSPI\_SYSCONFIG register: No idle mode, force idle mode, and smart idle mode.

- When programmed for no idle mode (the bit SIdleMode of the register MCSPI\_SYSCONFIG is set to "01"), the module ignores the system power manager request, and behaves normally, as if the request was not asserted.
- When programmed for smart idle mode (the bit SIdleMode of the register MCSPI\_SYSCONFIG is set to "10"), the module acknowledges the system power manager request according to its internal state.
- When programmed for force idle mode (the bit SIdleMode of the register MCSPI\_SYSCONFIG is set to "00"), the module acknowledges the system power manager request unconditionally.

The OCP clock will be optionally switched off, during the smart idle mode period, if the bit ClockActivity of the register MCSPI\_SYSCONFIG is set.

The SPI clock will be optionally switched off, during the smart idle mode period, if the second bit ClockActivity of the register MCSPI\_SYSCONFIG is set.

McSPI assumes that both clocks may be switched off whatever the value set in the field ClockActivity of the register MCSPI\_SYSCONFIG.

#### *24.3.7.2.1 Transitions from Normal Mode to Smart-Idle Mode*

The module detects an idle request when the synchronous signal IdleReq is asserted.

When IdleReq is asserted, any access to the module will generate an error as long as the OCP clock is alive.

When configured as a slave device, McSPI responds to the idle request by asserting the SIdleAck signal (idle acknowledgement) only after completion of the current transfer (SPIEN slave selection signal deasserted by the external master) and if interrupt or DMA request lines are not asserted.

As a master device, McSPI responds to the idle request by asserting the SIdleAck signal (idle acknowledgement) only after completion of all the channel data transfers and if interrupt or DMA request lines are not asserted.
As long as SIdleAck is not asserted, if an event occurs, the module can still generate an interrupt or a DMA request after IdleReq assertion. In this case, the module ignores the idle request and SIdleAck will not get asserted: The system power manager will abort the power mode transition procedure. It is then the responsibility of the system to de-assert IdleReq before attempting to access the module.

When SIdleAck is asserted, the module does not assert any new interrupt or DMA request.

#### *24.3.7.2.2 Transition From Smart-Idle Mode to Normal Mode*

McSPI detects the end of the idle period when the idle request signal (IdleReq) is deasserted.

Upon IdleReq de-assertion, the module switches back to normal mode and de-asserts SIdleAck signal. The module is fully operational.

#### *24.3.7.2.3 Force-Idle Mode*

Force-idle mode is enabled as follows:

• The bit SIdleMode of the register MCSPI\_SYSCONFIG is cleared to "00" (Force Idle).

The force idle mode is an idle mode where McSPI responds unconditionally to the idle request by asserting the SIdleAck signal and by deasserting unconditionally the interrupt and DMA request lines if asserted.

The transition from normal mode to idle mode does not affect the interrupt event bits of the MCSPI\_IRQSTATUS register.

In force-idle mode, the module is supposed to be disabled at that time, so the interrupt and DMA request lines are likely deasserted. OCP clock and SPI clock provided to McSPI can be switched off.

An idle request during an SPI data transfer can lead to an unexpected and unpredictable result, and is under software responsibility.

Any access to the module in force idle mode will generate an error as long as the OCP clock is alive and IdleReq is asserted.

The module exits the force idle mode when:

• The idle request signal (IdleReq) is de-asserted.

Upon IdleReq de-assertion, the module switches back to normal mode and de-asserts SIdleAck signal. The module is fully operational. The interrupt and DMA request lines are optionally asserted a clock cycle later.

### *24.3.8 System Test Mode*

McSPI is in system test mode (SYSTEST) when the bit System\_Test of the register MCSPI\_MODULCTRL is set.

The SYSTEST mode is used to check in a very simple manner the correctness of the system interconnect either internally to interrupt handler, or power manager, or externally to SPI I/Os.

I/O verification can be performed in SYSTEST mode by toggling the outputs and capturing the logic state of the inputs. (See MCSPI\_SYST register definition in )

### *24.3.9 Reset*

#### **24.3.9.1 Internal Reset Monitoring**

The module is reset by the hardware when an active-low reset signal, synchronous to the OCP interface clock is asserted on the input pin RESETN.

This hardware reset signal has a global reset action on the module. All configuration registers and all state machines are reset, in all clock domains.

Additionally, the module can be reset by software through the bit SoftReset of the register MCSPI\_SYSCONFIG. This bit has exactly the same action on the module logic as the hardware RESETN signal. The register MCSPI\_SYSCONFIG is not sensitive to software reset. The SoftReset control bit is active high. The bit is automatically reset to 0 by the hardware.
A global ResetDone status bit is provided in the status register MCSPI\_SYSSTATUS. This bit is set to 1 when all the different clock domains resets (OCP domain and SPI domains) have been released (logical AND).

The global ResetDone status bit can be monitored by the software to check if the module is ready-to-use following a reset (either hardware or software).

The clock CLKSPIREF must be provided to the module, in order to allow the ResetDone status bit to be set.

When used in slave mode, the clock CLKSPIREF is needed only during the reset phase. The clock CLKSPIREF can be switched off after the ResetDone status is set.

### *24.3.10 Access to Data Registers*

This section details the supported data accesses (read or write) from/to the data receiver registers MCSPI\_RX(i) and data transmitter registers MCSPI\_TX(i).

Supported access:

McSPI supports only one SPI word per register (receiver or transmitter) and does not support successive 8-bit or 16-bit accesses for a single SPI word.

The SPI word received is always right justified on LSbit of the 32bit register MCSPI\_RX(i), and the SPI word to transmit is always right justified on LSbit of the 32bit register MCSPI\_TX(i).

The upper bits, above SPI word length, are ignored and the content of the data registers is not reset between the SPI data transfers.

The coherence between the number of bits of the SPI Word, the number of bits of the access and the enabled byte remains under the user's responsibility. Only aligned accesses are supported.

In Master mode, data should not be written in the transmit register when the channel is disabled.

### *24.3.11 Programming Aid*

#### **24.3.11.1 Module Initialization**

- Hard or soft reset.
- Read MCSPI\_SYSSTATUS.
- Check if reset is done.
- Module configuration: (a) Write into MCSPI\_MODULCTRL (b) Write into MCSPI\_SYSCONFIG.
- Before the ResetDone bit is set, the clocks CLK and CLKSPIREF must be provided to the module.
- To avoid hazardous behavior, it is advised to reset the module before changing from MASTER mode to SLAVE mode or from SLAVE mode to MASTER mode.

#### **24.3.11.2 Common Transfer Sequence**

McSPI module allows the transfer of one or several words, according to different modes:

- MASTER, MASTER Turbo, SLAVE
- TRANSMIT RECEIVE, TRANSMIT ONLY, RECEIVE ONLY
- Write and Read requests: Interrupts, DMA
- SPIEN lines assertion/deassertion: automatic, manual

For all these flows, the host process contains the main process and the interrupt routines. The interrupt routines are called on the interrupt signals or by an internal call if the module is used in polling mode.

In multi-channel master mode, the flows of different channels can be run simultaneously.

#### **24.3.11.3 Main Program**

• Interrupt Initialization: (a) Reset status bits in MCSPI\_IRQSTATUS (b) Enable interrupts in MCSPI\_IRQENA.
- Channel Configuration: Write MCSPI\_CH(i)CONF.
- Start the channel: Write 0000 0001h in MCSPI\_CH(i)CTRL.
- First write request: TX empty Generate DMA write event/ polling TX empty flag by CPU to write First transmit word into MCSPI\_TX(i).
- End of transfer: Stop the channel by writing 0000 0000h in MCSPI\_CH(i)CTRL

The end of transfer depends on the transfer mode.

In multi-channel master mode, be careful not to overwrite the bits of other channels when initializing MCSPI\_IRQSTATUS and MCSPI\_IRQENABLE.

### *24.3.12 Interrupt and DMA Events*

McSPI has two DMA requests (Rx and Tx) per channel. It also has one interrupt line for all the interrupt requests.

## **24.4 McSPI Registers**

### *24.4.1 SPI Registers*

Table [24-10](#page-38-1) lists the memory-mapped registers for the SPI. All register offset addresses not listed in Table [24-10](#page-38-1) should be considered as reserved locations and the register contents should not be modified.

|  |  |  | Table 24-10. SPI Registers |
|--|--|--|----------------------------|
|--|--|--|----------------------------|

| Offset | Acronym         | Register Name                                 | Section           |
|--------|-----------------|-----------------------------------------------|-------------------|
| 0h     | MCSPI_REVISION  | McSPI revision register                       | Section 24.4.1.1  |
| 110h   | MCSPI_SYSCONFIG | McSPI system configuration register           | Section 24.4.1.2  |
| 114h   | MCSPI_SYSSTATUS | McSPI system status register                  | Section 24.4.1.3  |
| 118h   | MCSPI_IRQSTATUS | McSPI interrupt status register               | Section 24.4.1.4  |
| 11Ch   | MCSPI_IRQENABLE | McSPI interrupt enable register               | Section 24.4.1.5  |
| 124h   | MCSPI_SYST      | McSPI system register                         | Section 24.4.1.6  |
| 128h   | MCSPI_MODULCTRL | McSPI module control register                 | Section 24.4.1.7  |
| 12Ch   | MCSPI_CH0CONF   | McSPI channel 0 configuration register        | Section 24.4.1.8  |
| 130h   | MCSPI_CH0STAT   | McSPI channel 0 status register               | Section 24.4.1.9  |
| 134h   | MCSPI_CH0CTRL   | McSPI channel 0 control register              | Section 24.4.1.10 |
| 138h   | MCSPI_TX0       | McSPI channel 0 FIFO transmit buffer register | Section 24.4.1.11 |
| 13Ch   | MCSPI_RX0       | McSPI channel 0 FIFO receive buffer register  | Section 24.4.1.12 |
| 140h   | MCSPI_CH1CONF   | McSPI channel 1 configuration register        | Section 24.4.1.13 |
| 144h   | MCSPI_CH1STAT   | McSPI channel 1 status register               | Section 24.4.1.14 |
| 148h   | MCSPI_CH1CTRL   | McSPI channel 1 control register              | Section 24.4.1.15 |
| 14Ch   | MCSPI_TX1       | McSPI channel 1 FIFO transmit buffer register | Section 24.4.1.16 |
| 150h   | MCSPI_RX1       | McSPI channel 1 FIFO receive buffer register  | Section 24.4.1.17 |
| 154h   | MCSPI_CH2CONF   | McSPI channel 2 configuration register        | Section 24.4.1.18 |
| 158h   | MCSPI_CH2STAT   | McSPI channel 2 status register               | Section 24.4.1.19 |
| 15Ch   | MCSPI_CH2CTRL   | McSPI channel 2 control register              | Section 24.4.1.20 |
| 160h   | MCSPI_TX2       | McSPI channel 2 FIFO transmit buffer register | Section 24.4.1.21 |
| 164h   | MCSPI_RX2       | McSPI channel 2 FIFO receive buffer register  | Section 24.4.1.22 |
| 168h   | MCSPI_CH3CONF   | McSPI channel 3 configuration register        | Section 24.4.1.23 |
| 16Ch   | MCSPI_CH3STAT   | McSPI channel 3 status register               | Section 24.4.1.24 |
| 170h   | MCSPI_CH3CTRL   | McSPI channel 3 control register              | Section 24.4.1.25 |
| 174h   | MCSPI_TX3       | McSPI channel 3 FIFO transmit buffer register | Section 24.4.1.26 |
| 178h   | MCSPI_RX3       | McSPI channel 3 FIFO receive buffer register  | Section 24.4.1.27 |
| 17Ch   | MCSPI_XFERLEVEL | McSPI transfer levels register                | Section 24.4.1.28 |
### **Table 24-10. SPI Registers (continued)**

| Offset | Acronym     | Register Name                                          | Section           |
|--------|-------------|--------------------------------------------------------|-------------------|
| 180h   | MCSPI_DAFTX | McSPI DMA address aligned FIFO transmitter<br>register | Section 24.4.1.29 |
| 1A0h   | MCSPI_DAFRX | McSPI DMA address aligned FIFO receiver register       | Section 24.4.1.30 |
#### **24.4.1.1 MCSPI\_REVISION Register (offset = 0h) [reset = 300000h]**

MCSPI\_REVISION is shown in [Figure](#page-40-1) 24-26 and described in Table [24-11](#page-40-2).

The McSPI system configuration register (MCSPI\_REVISION) allows control of various parameters of the module interface. It is not sensitive to software reset.

**Figure 24-26. MCSPI\_REVISION Register**

| 31     | 30     | 29       | 28    | 27 | 26      | 25      | 24 |
|--------|--------|----------|-------|----|---------|---------|----|
| SCHEME |        | RESERVED |       |    |         | FUNC    |    |
| R-0h   |        | R-0h     |       |    |         | R-30h   |    |
| 23     | 22     | 21       | 20    | 19 | 18      | 17      | 16 |
|        |        |          | FUNC  |    |         |         |    |
|        |        |          | R-30h |    |         |         |    |
| 15     | 14     | 13       | 12    | 11 | 10      | 9       | 8  |
|        |        | R_RTL    |       |    |         | X_MAJOR |    |
|        |        | R-0h     |       |    |         | R-0h    |    |
| 7      | 6      | 5        | 4     | 3  | 2       | 1       | 0  |
|        | CUSTOM |          |       |    | Y_MINOR |         |    |
|        | R/W-0h |          |       |    | R/W-0h  |         |    |
|        |        |          |       |    |         |         |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 24-11. MCSPI\_REVISION Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                             |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-30 | SCHEME   | R    | 0h    | Used to distinguish between old scheme and current.                                                                                                                     |
|       |          |      |       | 0h = Legacy scheme.                                                                                                                                                     |
|       |          |      |       | 1h = Revision 0.8 scheme.                                                                                                                                               |
| 29-28 | RESERVED | R    | 0h    |                                                                                                                                                                         |
| 27-16 | FUNC     | R    | 30h   | Function indicates a software compatible module family.<br>If there is no level of software compatibility a new Func number (and<br>hence REVISION) should be assigned. |
| 15-11 | R_RTL    | R    | 0h    | R_RTL bit.                                                                                                                                                              |
| 10-8  | X_MAJOR  | R    | 0h    | X_MAJOR bit.                                                                                                                                                            |
| 7-6   | CUSTOM   | R/W  | 0h    | CUSTOM bit.                                                                                                                                                             |
| 5-0   | Y_MINOR  | R/W  | 0h    | Y_MINOR bit.                                                                                                                                                            |
#### **24.4.1.2 MCSPI\_SYSCONFIG Register (offset = 110h) [reset = 0h]**

MCSPI\_SYSCONFIG is shown in [Figure](#page-41-1) 24-27 and described in Table [24-12](#page-41-2).

The McSPI system configuration register (MCSPI\_SYSCONFIG) allows control of various parameters of the module interface. It is not sensitive to software reset.

**Figure 24-27. MCSPI\_SYSCONFIG Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 24-12. MCSPI\_SYSCONFIG Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|-------|---------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 9-8   | CLOCKACTIVITY | R/W  | 0h    | Clocks activity during wake-up mode period.<br>0h = OCP and Functional clocks may be switched off.<br>1h = OCP clock is maintained. Functional clock may be switched-off.<br>2h = Functional clock is maintained. OCP clock may be switched-off.<br>3h = OCP and Functional clocks are maintained.                                                                                                                                                                                           |
| 7-5   | RESERVED      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 4-3   | SIDLEMODE     | R/W  | 0h    | Power management.<br>0h = If an idle request is detected, the McSPI acknowledges it<br>unconditionally and goes in Inactive mode. Interrupt, DMA requests<br>are unconditionally de-asserted.<br>1h = If an idle request is detected, the request is ignored and keeps<br>on behaving normally.<br>2h = Smart-idle mode: local target's idle state eventually follows<br>(acknowledges) the system's idle requests, depending on the IP<br>module's internal requirements.<br>3h = Reserved. |
| 2     | RESERVED      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 1     | SOFTRESET     | R/W  | 0h    | Software reset.<br>During reads it always returns 0.<br>0h = (write) Normal mode.<br>1h = (write) Set this bit to 1 to trigger a module reset. The bit is<br>automatically reset by the hardware.                                                                                                                                                                                                                                                                                            |
| 0     | AUTOIDLE      | R/W  | 0h    | Internal OCP Clock gating strategy.<br>0h = OCP clock is free-running.<br>1h = Automatic OCP clock gating strategy is applied, based on the<br>OCP interface activity.                                                                                                                                                                                                                                                                                                                       |
#### **24.4.1.3 MCSPI\_SYSSTATUS Register (offset = 114h) [reset = 0h]**

MCSPI\_SYSSTATUS is shown in [Figure](#page-42-1) 24-28 and described in Table [24-13.](#page-42-2)

The McSPI system status register (MCSPI\_SYSSTATUS) provides status information about the module excluding the interrupt status information.

**Figure 24-28. MCSPI\_SYSSTATUS Register**

| 31       | 30       | 29 | 28 | 27       | 26 | 25        | 24   |
|----------|----------|----|----|----------|----|-----------|------|--
|          | RESERVED |    |    |          |    |           |      |
|          |          |    |    | R-0h     |    |           |      |
| 23       | 22       | 21 | 20 | 19       | 18 | 17        | 16   |
|          | RESERVED |    |    |          |    |           |      |
| R-0h     |          |    |    |          |    |           |      |
| 15       | 14       | 13 | 12 | 11       | 10 | 9         | 8    |
|          |          |    |    | RESERVED |    |           |      |
|          |          |    |    | R-0h     |    |           |      |
| 7        | 6        | 5  | 4  | 3        | 2  | 1         | 0    |
| RESERVED |          |    |    |          |    | RESETDONE |      |
| R-0h     |          |    |    |          |    |           | R-0h |
|          |          |    |    |          |    |           |      |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 24-13. MCSPI\_SYSSTATUS Register Field Descriptions**

| Bit  | Field     | Type | Reset                                                                      | Description          |
|------|-----------|------|----------------------------------------------------------------------------|----------------------|
| 31-1 | RESERVED  | R    | 0h                                                                         |                      |
| 0    | RESETDONE | R    | 0h<br>Internal Reset Monitoring.<br>0h = Internal module reset is on-going |                      |
|      |           |      |                                                                            | 1h = Reset completed |
#### **24.4.1.4 MCSPI\_IRQSTATUS Register (offset = 118h) [reset = 0h]**

MCSPI\_IRQSTATUS is shown in [Figure](#page-43-1) 24-29 and described in Table [24-14.](#page-43-2)

The McSPI interrupt status register (MCSPI\_IRQSTATUS) regroups all the status of the module internal events that can generate an interrupt.

**Figure 24-29. MCSPI\_IRQSTATUS Register**

| 31       | 30                     | 29                | 28        | 27               | 26       | 25                | 24        |
|----------|------------------------|-------------------|-----------|------------------|----------|-------------------|-----------|
|          |                        |                   |           | RESERVED         |          |                   |           |
|          |                        |                   |           | R-0h             |          |                   |           |
| 23       | 22                     | 21                | 20        | 19               | 18       | 17                | 16        |
|          |                        | RESERVED          |           |                  |          | EOW               | RESERVED  |
|          | R-0h<br>R/W-0h<br>R-0h |                   |           |                  |          |                   |           |
| 15       | 14                     | 13                | 12        | 11               | 10       | 9                 | 8         |
| RESERVED | RX3_FULL               | TX3_UNDERFLOW | TX3_EMPTY | RESERVED         | RX2_FULL | TX2_UNDERFLOW | TX2_EMPTY |
| R-0h     | R/W-0h                 | R/W-0h            | R/W-0h    | R-0h             | R/W-0h   | R/W-0h            | R/W-0h    |
| 7        | 6                      | 5                 | 4         | 3                | 2        | 1                 | 0         |
| RESERVED | RX1_FULL               | TX1_UNDERFLOW | TX1_EMPTY | RX0_OVERFLOW | RX0_FULL | TX0_UNDERFLOW | TX0_EMPTY |
| R-0h     | R/W-0h                 | R/W-0h            | R/W-0h    | R/W-0h           | R/W-0h   | R/W-0h            | R/W-0h    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-14. MCSPI\_IRQSTATUS Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-------|---------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-18 | RESERVED      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 17    | EOW           | R/W  | 0h    | End of word (EOW) count event when a channel is enabled using<br>the FIFO buffer and the channel has sent the number of McSPI<br>words defined by the MCSPI_XFERLEVEL[WCNT].<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending.                                                                                                                                             |
| 16    | RESERVED      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 15    | RESERVED      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 14    | RX3_FULL      | R/W  | 0h    | Receiver register is full or almost full.<br>Only when Channel 3 is enabled.<br>This bit indicate FIFO almost full status when built-in FIFO is used<br>for receive register (MCSPI_CH3CONF[FFE3R] is set).<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending.                                                                                                              |
| 13    | TX3_UNDERFLOW | R/W  | 0h    | Transmitter register underflow.<br>Only when Channel 3 is enabled.<br>The transmitter register is empty (not updated by Host or DMA with<br>new data) before its time slot assignment.<br>Exception: No TX_underflow event when no data has been loaded<br>into the transmitter register since channel has been enabled.<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending. |
### **Table 24-14. MCSPI\_IRQSTATUS Register Field Descriptions (continued)**

| Bit | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                              |
|-----|---------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 12  | TX3_EMPTY     | R/W  | 0h    | Transmitter register is empty or almost empty.<br>This bit indicate FIFO almost full status when built-in FIFO is used<br>for transmit register (MCSPI_CH3CONF[FFE3W] is set).<br>Note: Enabling the channel automatically raises this event.<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset. |
|     |               |      |       | 1h (R) = Event is pending.                                                                                                                                                                                                                                                                                                                               |
| 11  | RESERVED      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                          |
| 10  | RX2_FULL      | R/W  | 0h    | Receiver register full or almost full.<br>Channel 2 This bit indicate FIFO almost full status when built-in<br>FIFO is used for receive register (MCSPI_CH3CONF[FFE2R] is set).<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending.                                 |
| 9   | TX2_UNDERFLOW | R/W  | 0h    | Transmitter register underflow.<br>Channel 20h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending.                                                                                                                                                                    |
| 8   | TX2_EMPTY     | R/W  | 0h    | Transmitter register empty or almost empty.<br>Channel 2.<br>This bit indicate FIFO almost full status when built-in FIFO is used<br>for transmit register (MCSPI_CH3CONF[FFE2W] is set).<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending.                       |
| 7   | RESERVED      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                          |
| 6   | RX1_FULL      | R/W  | 0h    | Receiver register full or almost full.<br>Channel 1.<br>This bit indicate FIFO almost full status when built-in FIFO is use for<br>receive register (MCSPI_CH3CONF[FFE1R] is set).<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending.                              |
| 5   | TX1_UNDERFLOW | R/W  | 0h    | Transmitter register underflow.<br>Channel 1.<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending.                                                                                                                                                                   |
| 4   | TX1_EMPTY     | R/W  | 0h    | Transmitter register empty or almost empty.<br>Channel 1.<br>This bit indicate FIFO almost full status when built-in FIFO is use for<br>transmit register (MCSPI_CH3CONF[FFE1W] is set).<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending.                        |
### **Table 24-14. MCSPI\_IRQSTATUS Register Field Descriptions (continued)**

| Bit | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                       |
|-----|---------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 3   | RX0_OVERFLOW  | R/W  | 0h    | Receiver register overflow (slave mode only).<br>Channel 0.<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending.                                                                                                                              |
| 2   | RX0_FULL      | R/W  | 0h    | Receiver register full or almost full.<br>Channel 0.<br>Receiver register full or almost full.<br>Channel 00h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending.                                                                              |
| 1   | TX0_UNDERFLOW | R/W  | 0h    | Transmitter register underflow.<br>Channel 0.<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending.                                                                                                                                            |
| 0   | TX0_EMPTY     | R/W  | 0h    | Transmitter register empty or almost empty.<br>Channel 0.<br>This bit indicate FIFO almost full status when built-in FIFO is use for<br>transmit register (MCSPI_CH3CONF[FFE0W] is set).<br>0h (W) = Event status bit is unchanged.<br>0h (R) = Event false.<br>1h (W) = Event status bit is reset.<br>1h (R) = Event is pending. |
#### **24.4.1.5 MCSPI\_IRQENABLE Register (offset = 11Ch) [reset = 0h]**

MCSPI\_IRQENABLE is shown in [Figure](#page-46-1) 24-30 and described in Table [24-15.](#page-46-2)

This McSPI interrupt enable register (MCSPI\_IRQENABLE) enables/disables the module internal sources of interrupt, on an event-by-event basis.

**Figure 24-30. MCSPI\_IRQENABLE Register**

| 31             | 30                 | 29                      | 28                  | 27                     | 26                 | 25                      | 24                  |
|----------------|--------------------|-------------------------|---------------------|------------------------|--------------------|-------------------------|---------------------|--
|                |                    |                         | RESERVED            |                        |                    |                         |                     |
|                | R-0h               |                         |                     |                        |                    |                         |                     |
| 23             | 22                 | 21                      | 20                  | 19                     | 18                 | 17                      | 16                  |
|                |                    | RESERVED                |                     |                        |                    | EOWKE                   | RESERVED            |
| R-0h<br>R/W-0h |                    |                         |                     |                        |                    |                         | R-0h                |
| 15             | 14                 | 13                      | 12                  | 11                     | 10                 | 9                       | 8                   |
| RESERVED       | RX3_FULLENABLE | TX3_UNDERFLOWENABLE | TX3_EMPTYENABLE | RESERVED               | RX2_FULLENABLE | TX2_UNDERFLOWENABLE | TX2_EMPTYENABLE |
| R-0h           | R/W-0h             | R/W-0h                  | R/W-0h              | R-0h                   | R/W-0h             | R/W-0h                  | R/W-0h              |
| 7              | 6                  | 5                       | 4                   | 3                      | 2                  | 1                       | 0                   |
| RESERVED       | RX1_FULLENABLE | TX1_UNDERFLOWENABLE | TX1_EMPTYENABLE | RX0_OVERFLOWENABLE | RX0_FULLENABLE | TX0_UNDERFLOWENABLE | TX0_EMPTYENABLE |
| R-0h           | R/W-0h             | R/W-0h                  | R/W-0h              | R/W-0h                 | R/W-0h             | R/W-0h                  | R/W-0h              |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-15. MCSPI\_IRQENABLE Register Field Descriptions**

| Bit   | Field                   | Type | Reset | Description                                                                           |
|-------|-------------------------|------|-------|---------------------------------------------------------------------------------------|
| 31-18 | RESERVED                | R    | 0h    |                                                                                       |
| 17    | EOWKE                   | R/W  | 0h    | End of word count interrupt enable.                                                   |
|       |                         |      |       | 0h = Interrupt is disabled.                                                           |
|       |                         |      |       | 1h = Interrupt is enabled.                                                            |
| 16    | RESERVED                | R    | 0h    |                                                                                       |
| 15    | RESERVED                | R    | 0h    |                                                                                       |
| 14    | RX3_FULLENABLE          | R/W  | 0h    | MCSPI_RX3 receiver register full or almost full interrupt enable<br>(channel 3).      |
|       |                         |      |       | 0h = Interrupt is disabled.                                                           |
|       |                         |      |       | 1h = Interrupt is enabled.                                                            |
| 13    | TX3_UNDERFLOWENABLE | R/W  | 0h    | MCSPI_TX3 transmitter register underflow interrupt enable (channel<br>3).             |
|       |                         |      |       | 0h = Interrupt is disabled.                                                           |
|       |                         |      |       | 1h = Interrupt is enabled.                                                            |
| 12    | TX3_EMPTYENABLE         | R/W  | 0h    | MCSPI_TX3 transmitter register empty or almost empty interrupt<br>enable (channel 3). |
|       |                         |      |       | 0h = Interrupt is disabled.                                                           |
|       |                         |      |       | 1h = Interrupt is enabled.                                                            |
| 11    | RESERVED                | R    | 0h    |                                                                                       |
| 10    | RX2_FULLENABLE          | R/W  | 0h    | MCSPI_RX2 receiver register full or almost full interrupt enable<br>(channel 2).      |
|       |                         |      |       | 0h = Interrupt is disabled.                                                           |
|       |                         |      |       | 1h = Interrupt is enabled.                                                            |
| 9     | TX2_UNDERFLOWENABLE | R/W  | 0h    | MCSPI_TX2 transmitter register underflow interrupt enable (channel<br>2).             |
|       |                         |      |       | 0h = Interrupt is disabled.                                                           |
|       |                         |      |       | 1h = Interrupt is enabled.                                                            |
**Table 24-15. MCSPI\_IRQENABLE Register Field Descriptions (continued)**

| Bit | Field                   | Type | Reset | Description                                                                                                                                        |
|-----|-------------------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 8   | TX2_EMPTYENABLE         | R/W  | 0h    | MCSPI_TX2 transmitter register empty or almost empty interrupt<br>enable (channel 2).<br>0h = Interrupt is disabled.<br>1h = Interrupt is enabled. |
| 7   | RESERVED                | R    | 0h    |                                                                                                                                                    |
| 6   | RX1_FULLENABLE          | R/W  | 0h    | MCSPI_RX1 receiver register full or almost full interrupt enable<br>(channel 1)<br>0h = Interrupt is disabled.<br>1h = Interrupt is enabled.       |
| 5   | TX1_UNDERFLOWENABLE | R/W  | 0h    | MCSPI_TX1 transmitter register underflow interrupt enable (channel<br>1).<br>0h = Interrupt is disabled.<br>1h = Interrupt is enabled.             |
| 4   | TX1_EMPTYENABLE         | R/W  | 0h    | MCSPI_TX1 transmitter register empty or almost empty interrupt<br>enable (channel 1).<br>0h = Interrupt is disabled.<br>1h = Interrupt is enabled. |
| 3   | RX0_OVERFLOWENABLE  | R/W  | 0h    | MCSPI_RX0 receivier register overflow interrupt enable (channel 0).<br>0h = Interrupt is disabled.<br>1h = Interrupt is enabled.                   |
| 2   | RX0_FULLENABLE          | R/W  | 0h    | MCSPI_RX0 receiver register full or almost full interrupt enable<br>(channel 0).<br>0h = Interrupt is disabled.<br>1h = Interrupt is enabled.      |
| 1   | TX0_UNDERFLOWENABLE | R/W  | 0h    | MCSPI_TX0 transmitter register underflow interrupt enable (channel<br>0).<br>0h = Interrupt is disabled.<br>1h = Interrupt is enabled.             |
| 0   | TX0_EMPTYENABLE         | R/W  | 0h    | MCSPI_TX0 transmitter register empty or almost empty interrupt<br>enable (channel 0).<br>0h = Interrupt is disabled.<br>1h = Interrupt is enabled. |
#### **24.4.1.6 MCSPI\_SYST Register (offset = 124h) [reset = 0h]**

MCSPI\_SYST is shown in [Figure](#page-48-1) 24-31 and described in Table [24-16](#page-48-2).

This McSPI system register (MCSPI\_SYST) is used to configure the system interconnect either internally to the peripheral bus or externally to the device I/O pads, when the module is configured in the system test (SYSTEST) mode.

**Figure 24-31. MCSPI\_SYST Register**

| 31       | 30       | 29       | 28       | 27       | 26       | 25         | 24         |
|----------|----------|----------|----------|----------|----------|------------|------------|--
|          | RESERVED |          |          |          |          |            |            |
|          | R-0h     |          |          |          |          |            |            |
| 23       | 22       | 21       | 20       | 19       | 18       | 17         | 16         |
|          |          |          |          | RESERVED |          |            |            |
|          | R-0h     |          |          |          |          |            |            |
| 15       | 14       | 13       | 12       | 11       | 10       | 9          | 8          |
|          |          | RESERVED |          | SSB      | SPIENDIR | SPIDATDIR1 | SPIDATDIR0 |
|          |          | R-0h     |          | R/W-0h   | R/W-0h   | R/W-0h     | R/W-0h     |
| 7        | 6        | 5        | 4        | 3        | 2        | 1          | 0          |
| RESERVED | SPICLK   | SPIDAT_1 | SPIDAT_0 | SPIEN_3  | SPIEN_2  | SPIEN_1    | SPIEN_0    |
| R-0h     | R/W-0h   | R/W-0h   | R/W-0h   | R/W-0h   | R/W-0h   | R/W-0h     | R/W-0h     |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 24-16. MCSPI\_SYST Register Field Descriptions**

| Bit   | Field      | Type | Reset | Description                                                                                                                                                                            |
|-------|------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-12 | RESERVED   | R    | 0h    |                                                                                                                                                                                        |
| 11    | SSB        | R/W  | 0h    | Set status bit.<br>This bit must be cleared prior attempting to clear a status bit of the<br>MCSPI_ IRQSTATUS register.                                                                |
|       |            |      |       | 0h = No action. Writing 0 does not clear already set status bits. This<br>bit must be cleared prior attempting to clear a status bit of the<br>MCSPI_IRQSTATUS register.               |
|       |            |      |       | 1h = Writing 1 sets to 1 all status bits contained in the<br>MCSPI_IRQSTATUS register. Writing 1 into this bit sets to 1 all<br>status bits contained in the MCSPI_IRQSTATUS register. |
| 10    | SPIENDIR   | R/W  | 0h    | Sets the direction of the SPIEN<br>[3:0] lines and SPICLK line.                                                                                                                        |
|       |            |      |       | 0h = Output (as in master mode).                                                                                                                                                       |
|       |            |      |       | 1h = Input (as in slave mode).                                                                                                                                                         |
| 9     | SPIDATDIR1 | R/W  | 0h    | Sets the direction of the SPIDAT[1].                                                                                                                                                   |
|       |            |      |       | 0h = Output                                                                                                                                                                            |
|       |            |      |       | 1h = Input                                                                                                                                                                             |
| 8     | SPIDATDIR0 | R/W  | 0h    | Sets the direction of the SPIDAT[0].                                                                                                                                                   |
|       |            |      |       | 0h = Output                                                                                                                                                                            |
|       |            |      |       | 1h = Input                                                                                                                                                                             |
| 7     | RESERVED   | R    | 0h    |                                                                                                                                                                                        |
| 6     | SPICLK     | R/W  | 0h    | SPICLK line (signal data value)                                                                                                                                                        |
|       |            |      |       | 0h = If MCSPI_SYST[SPIENDIR] = 1 (input mode direction), this bit<br>returns the value on the CLKSPI line (high or low), and a write into<br>this bit has no effect.                   |
|       |            |      |       | 1h = If MCSPI_SYST[SPIENDIR] = 0 (output mode direction), the<br>CLKSPI line is driven high or low according to the value written into<br>this register.                               |
**Table 24-16. MCSPI\_SYST Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                      |
|-----|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 5   | SPIDAT_1 | R/W  | 0h    | SPIDAT[1] line (signal data value)<br>0h = If MCSPI_SYST[SPIDATDIR1] = 0 (output mode direction), the<br>SPIDAT[1] line is driven high or low according to the value written<br>into this register.<br>1h = If MCSPI_SYST[SPIDATDIR1] = 1 (input mode direction), this<br>bit returns the value on the SPIDAT[1] line (high or low), and a write<br>into this bit has no effect. |
| 4   | SPIDAT_0 | R/W  | 0h    | SPIDAT[0] line (signal data value)<br>0h = If MCSPI_SYST[SPIDATDIR0] = 0 (output mode direction), the<br>SPIDAT[0] line is driven high or low according to the value written<br>into this register.<br>1h = If MCSPI_SYST[SPIDATDIR0] = 1 (input mode direction), this<br>bit returns the value on the SPIDAT[0] line (high or low), and a write<br>into this bit has no effect. |
| 3   | SPIEN_3  | R/W  | 0h    | SPIEN[3] line (signal data value)<br>0h = If MCSPI_SYST[SPIENDIR] = 0 (output mode direction), the<br>SPIENT[3] line is driven high or low according to the value written<br>into this register.<br>1h = If MCSPI_SYST[SPIENDIR] = 1 (input mode direction), this bit<br>returns the value on the SPIEN[3] line (high or low), and a write into<br>this bit has no effect.       |
| 2   | SPIEN_2  | R/W  | 0h    | SPIEN[2] line (signal data value)<br>0h = If MCSPI_SYST[SPIENDIR] = 0 (output mode direction), the<br>SPIENT[2] line is driven high or low according to the value written<br>into this register.<br>1h = If MCSPI_SYST[SPIENDIR] = 1 (input mode direction), this bit<br>returns the value on the SPIEN[2] line (high or low), and a write into<br>this bit has no effect.       |
| 1   | SPIEN_1  | R/W  | 0h    | SPIEN[1] line (signal data value)<br>0h = If MCSPI_SYST[SPIENDIR] = 0 (output mode direction), the<br>SPIENT[1] line is driven high or low according to the value written<br>into this register.<br>1h = If MCSPI_SYST[SPIENDIR] = 1 (input mode direction), this bit<br>returns the value on the SPIEN[1] line (high or low), and a write into<br>this bit has no effect.       |
| 0   | SPIEN_0  | R/W  | 0h    | SPIEN[0] line (signal data value)<br>0h = If MCSPI_SYST[SPIENDIR] = 0 (output mode direction), the<br>SPIENT[0] line is driven high or low according to the value written<br>into this register.<br>1h = If MCSPI_SYST[SPIENDIR] = 1 (input mode direction), this bit<br>returns the value on the SPIEN[0] line (high or low), and a write into<br>this bit has no effect.       |
#### **24.4.1.7 MCSPI\_MODULCTRL Register (offset = 128h) [reset = 0x0000\_0004h]**

MCSPI\_MODULCTRL is shown in [Figure](#page-50-1) 24-32 and described in Table [24-17.](#page-50-2)

This McSPI module control register (MCSPI\_MODULCTRL) is used to configure the serial port interface.

**Figure 24-32. MCSPI\_MODULCTRL Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-17. MCSPI\_MODULCTRL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-9 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| 8    | FDAA     | R/W  | 0h    | FIFO DMA Address 256 bit aligned.<br>This register is used when a FIFO is managed by the module and<br>DMA connected to the controller provides only 256 bit aligned<br>address.<br>If this bit is set the enabled channel which uses the FIFO has its<br>datas managed through MCSPI_DAFTX and MCSPI_DAFRX<br>registers instead of MCSPI_TX(i) and MCSPI_RX(i) registers.<br>0h = FIFO data managed by MCSPI_TX(i) and MCSPI_RX(i)<br>registers.<br>1h = FIFO data managed by MCSPI_DAFTX and MCSPI_DAFRX<br>registers.                                                                                                                                                                                   |
| 7    | MOA      | R/W  | 0h    | Multiple word ocp access.<br>This register can only be used when a channel is enabled using a<br>FIFO.<br>It allows the system to perform multiple SPI word access for a single<br>32 bit OCP word access.<br>This is possible for WL less than 16.<br>0h = Multiple word access disabled<br>1h = Multiple word access enabled with FIFO                                                                                                                                                                                                                                                                                                                                                                   |
| 6-4  | INITDLY  | R/W  | 0h    | Initial SPI delay for first transfer.<br>This register is an option only available in SINGLE master mode,<br>The controller waits for a delay to transmit the first SPI word after<br>channel enabled and corresponding TX register filled.<br>This delay is based on SPI output frequency clock, No clock output<br>provided to the boundary and chip select is not active in 4 pin mode<br>within this period.<br>0h = No delay for first SPI transfer<br>1h = The controller wait 4 SPI bus clock<br>2h = The controller wait 8 SPI bus clock<br>3h = The controller wait 16 SPI bus clock<br>4h = The controller wait 32 SPI bus clock<br>5h = Reserved from 5h to Fh.<br>Fh = Reserved from 5h to Fh. |
### **Table 24-17. MCSPI\_MODULCTRL Register Field Descriptions (continued)**

| Bit | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                                                                               |
|-----|-------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 3   | SYSTEM_TEST | R/W  | 0h    | Enables the system test mode<br>0h = Functional mode<br>1h = System test mode (SYSTEST)                                                                                                                                                                                                                                                   |
| 2   | MS          | R/W  | 1h    | Master/ Slave<br>0h = Master - The module generates the SPICLK and SPIEN[3:0]<br>1h = Slave - The module receives the SPICLK and SPIEN[3:0]                                                                                                                                                                                               |
| 1   | PIN34       | R/W  | 0h    | Pin mode selection.<br>This register is used to configure the SPI pin mode, in master or<br>slave mode.<br>If asserted the controller only use SIMO,SOMI and SPICLK clock pin<br>for SPI transfers.<br>0h = SPIEN is used as a chip select.<br>1h = SPIEN is not used. In this mode all related option to chip select<br>have no meaning. |
| 0   | SINGLE      | R/W  | 0h    | Single channel / Multi Channel (master mode only).<br>0h = More than one channel will be used in master mode.<br>1h = Only one channel will be used in master mode. This bit must be<br>set in Force SPIEN mode.<br>For slave mode, set this bit to 0                                                                                     |
#### **24.4.1.8 MCSPI\_CH0CONF Register (offset = 12Ch) [reset = 0x0006\_0000h]**

MCSPI\_CH0CONF is shown in [Figure](#page-52-1) 24-33 and described in Table [24-18.](#page-52-2)

The McSPI channel 0 configuration register (MCSPI\_CH0CONF) is used to configure channel 0.

**Figure 24-33. MCSPI\_CH0CONF Register**

| 31     | 30       | 29       | 28     | 27     | 26     | 25     | 24     |
|--------|----------|----------|--------|--------|--------|--------|--------|
|        | RESERVED | CLKG     |        | FFEW   |        | TCS    |        |
|        | R-0h     | R/W-0h   | R/W-0h | R/W-0h |        | R/W-0h | R/W-0h |
| 23     | 22       | 21       | 20     | 19     | 18     | 17     | 16     |
| SBE    |          | SPIENSLV | FORCE  | TURBO  | IS     | DPE1   | DPE0   |
| R/W-0h |          | R/W-0h   | R/W-0h |        | R/W-1h | R/W-1h | R/W-0h |
| 15     | 14       | 13       | 12     | 11     | 10     | 9      | 8      |
| DMAR   | DMAW     |          | TRM    | WL     |        |        |        |
| R/W-0h | R/W-0h   |          | R/W-0h |        |        | R/W-0h |        |
| 7      | 6        | 5        | 4      | 3      | 2      | 1      | 0      |
| WL     | EPOL     |          |        | CLKD   |        | POL    | PHA    |
| R/W-0h | R/W-0h   |          |        | R/W-0h |        | R/W-0h | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-18. MCSPI\_CH0CONF Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|-------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-30 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 29    | CLKG     | R/W  | 0h    | Clock divider granularity.<br>This register defines the granularity of channel clock divider: power<br>of two or one clock cycle granularity.<br>When this bit is set the register MCSPI_CHCTRL[EXTCLK] must be<br>configured to reach a maximum of 4096 clock divider ratio.<br>Then The clock divider ratio is a concatenation of<br>MCSPI_CHCONF[CLKD] and MCSPI_CHCTRL[EXTCLK] values.<br>0h = Clock granularity of power of 21h = 1 clock cycle granularity |
| 28    | FFER     | R/W  | 0h    | FIFO enabled for receive.<br>Only one channel can have this bit set.<br>0h = The buffer is not used to receive data.<br>1h = The buffer is used to receive data.                                                                                                                                                                                                                                                                                                     |
| 27    | FFEW     | R/W  | 0h    | FIFO enabled for transmit.<br>Only one channel can have this bit set.<br>0h = The buffer is not used to transmit data.<br>1h = The buffer is used to transmit data.                                                                                                                                                                                                                                                                                                  |
| 26-25 | TCS      | R/W  | 0h    | Chip select time control.<br>These two bits define the number of interface clock cycles between<br>CS toggling and first or last edge of SPI clock.<br>0h = 0.5 clock cycles<br>1h = 1.5 clock cycles<br>2h = 2.5 clock cycles<br>3h = 3.5 clock cycles                                                                                                                                                                                                              |
| 24    | SBPOL    | R/W  | 0h    | Start bit polarity.<br>0h = Start bit polarity is held to 0 during SPI transfer.<br>1h = Start bit polarity is held to 1 during SPI transfer.                                                                                                                                                                                                                                                                                                                        |
| 23    | SBE      | R/W  | 0h    | Start bit enable for SPI transfer.<br>0h = Default SPI transfer length as specified by WL bit field.<br>1h = Start bit D/CX added before SPI transfer. Polarity is defined by<br>MCSPI_CH0CONF[SBPOL].                                                                                                                                                                                                                                                               |
### **Table 24-18. MCSPI\_CH0CONF Register Field Descriptions (continued)**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                              |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 22-21 | SPIENSLV | R/W  | 0h    | Channel 0 only and slave mode only: SPI slave select signal<br>detection.<br>Reserved bits (read returns 0) for other cases.<br>0h = Detection enabled only on SPIEN[0]<br>1h = Detection enabled only on SPIEN[1]<br>2h = Detection enabled only on SPIEN[2]<br>3h = Detection enabled only on SPIEN[3]                                                                                 |
| 20    | FORCE    | R/W  | 0h    | Manual SPIEN assertion to keep SPIEN active between SPI words<br>(single channel master mode only).<br>0h = Writing 0 into this bit drives low the SPIEN line when<br>MCSPI_CHCONF0[EPOL]=0, and drives it high when<br>MCSPI_CHCONF0[EPOL]=1.<br>1h = Writing 1 into this bit drives high the SPIEN line when<br>MCSPI_CHCONF0[EPOL]=0, and drives it low when<br>MCSPI_CHCONF0[EPOL]=1 |
| 19    | TURBO    | R/W  | 0h    | Turbo mode.<br>0h = Turbo is deactivated (recommended for single SPI word<br>transfer).<br>1h = Turbo is activated to maximize the throughput for multi-SPI<br>word transfers.                                                                                                                                                                                                           |
| 18    | IS       | R/W  | 1h    | Input select<br>0h = Data line 0 (SPIDAT[0]) selected for reception.<br>1h = Data line 1 (SPIDAT[1]) selected for reception.                                                                                                                                                                                                                                                             |
| 17    | DPE1     | R/W  | 1h    | Transmission enable for data line 1 (SPIDATAGZEN[1])<br>0h = Data line 1 (SPIDAT[1]) selected for transmission<br>1h = No transmission on data line 1 (SPIDAT[1])                                                                                                                                                                                                                        |
| 16    | DPE0     | R/W  | 0h    | Transmission enable for data line 0 (SPIDATAGZEN[0])<br>0h = Data line 0 (SPIDAT[0]) selected for transmission<br>1h = No transmission on data line 0 (SPIDAT[0])                                                                                                                                                                                                                        |
| 15    | DMAR     | R/W  | 0h    | DMA read request.<br>The DMA read request line is asserted when the channel is enabled<br>and new data is available in the receive register of the channel.<br>The DMA read request line is deasserted on read completion of the<br>receive register of the channel.<br>0h = DMA read request is disabled.<br>1h = DMA read request is enabled.                                          |
| 14    | DMAW     | R/W  | 0h    | DMA write request.<br>The DMA write request line is asserted when the channel is enabled<br>and the MCSPI_TX0 register of the channel is empty.<br>The DMA write request line is deasserted on load completion of the<br>MCSPI_TX0 register of the channel.<br>0h = DMA write request is disabled.<br>1h = DMA write request is enabled.                                                 |
| 13-12 | TRM      | R/W  | 0h    | Transmit/receive modes.<br>0h = Transmit and receive mode<br>1h = Receive-only mode<br>2h = Transmit-only mode<br>3h = Reserved                                                                                                                                                                                                                                                          |
**Table 24-18. MCSPI\_CH0CONF Register Field Descriptions (continued)**

| Bit  | Field | Type | Reset | Description                                      |
|------|-------|------|-------|--------------------------------------------------|
| 11-7 | WL    | R/W  | 0h    | SPI word length.                                 |
|      |       |      |       | 0h = Reserved                                    |
|      |       |      |       | 1h = Reserved                                    |
|      |       |      |       | 2h = Reserved                                    |
|      |       |      |       | 3h = The SPI word is 4-bits long.                |
|      |       |      |       | 4h = The SPI word is 5-bits long                 |
|      |       |      |       | 5h = The SPI word is 6-bits long                 |
|      |       |      |       | 6h = The SPI word is 7-bits long                 |
|      |       |      |       | 7h = The SPI word is 8-bits long                 |
|      |       |      |       | 8h = The SPI word is 9-bits long                 |
|      |       |      |       | 9h = The SPI word is 10-bits long                |
|      |       |      |       | Ah = The SPI word is 11-bits long                |
|      |       |      |       | Bh = The SPI word is 12-bits long                |
|      |       |      |       | Ch = The SPI word is 13-bits long                |
|      |       |      |       | Dh = The SPI word is 14-bits long                |
|      |       |      |       | Eh = The SPI word is 15-bits long                |
|      |       |      |       | Fh = The SPI word is 16-bits long                |
|      |       |      |       | 10h = The SPI word is 17-bits long               |
|      |       |      |       | 11h = The SPI word is 18-bits long               |
|      |       |      |       | 12h = The SPI word is 19-bits long               |
|      |       |      |       | 13h = The SPI word is 20-bits long               |
|      |       |      |       | 14h = The SPI word is 21-bits long               |
|      |       |      |       | 15h = The SPI word is 22-bits long               |
|      |       |      |       | 16h = The SPI word is 23-bits long               |
|      |       |      |       | 17h = The SPI word is 24-bits long               |
|      |       |      |       | 18h = The SPI word is 25-bits long               |
|      |       |      |       | 19h = The SPI word is 26-bits long               |
|      |       |      |       | 1Ah = The SPI word is 27-bits long               |
|      |       |      |       | 1Bh = The SPI word is 28-bits long               |
|      |       |      |       | 1Ch = The SPI word is 29-bits long               |
|      |       |      |       | 1Dh = The SPI word is 30-bits long               |
|      |       |      |       | 1Eh = The SPI word is 31-bits long               |
|      |       |      |       | 1Fh = The SPI word is 32-bits long               |
| 6    | EPOL  | R/W  | 0h    | SPIEN polarity                                   |
|      |       |      |       | 0h = SPIEN is held high during the active state. |
|      |       |      |       | 1h = SPIEN is held low during the active state.  |
### **Table 24-18. MCSPI\_CH0CONF Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
|-----|-------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 5-2 | CLKD  | R/W  | 0h    | Frequency divider for SPICLK (only when the module is a Master<br>SPI device).<br>A programmable clock divider divides the SPI reference clock<br>(CLKSPIREF) with a 4 bit value, and results in a new clock SPICLK<br>available to shift-in and shift-out data.<br>By default the clock divider ratio has a power of two granularity when<br>MCSPI_CHCONF[CLKG] is cleared, Otherwise this register is the 4LSB bit of a 12 bit register concatenated with clock divider extension<br>MCSPI_CHCTRL[EXTCLK] register.<br>The value description below defines the clock ratio when<br>MCSPI_CHCONF[CLKG] is cleared to 0.<br>0h = Divide by 1.<br>1h = Divide by 2.<br>2h = Divide by 4.<br>3h = Divide by 8.<br>4h = Divide by 16.<br>5h = Divide by 32.<br>6h = Divide by 64.<br>7h = Divide by 128.<br>8h = Divide by 256.<br>9h = Divide by 512.<br>Ah = Divide by 1024.<br>Bh = Divide by 2048.<br>Ch = Divide by 4096.<br>Dh = Divide by 8192.<br>Eh = Divide by 16384.<br>Fh = Divide by 32768. |
| 1   | POL   | R/W  | 0h    | SPICLK polarity<br>0h = SPICLK is held high during the active state<br>1h = SPICLK is held low during the active state                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 0   | PHA   | R/W  | 0h    | SPICLK phase<br>0h = Data are latched on odd numbered edges of SPICLK1h = Data are latched on even numbered edges of SPICLK                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
#### **24.4.1.9 MCSPI\_CH0STAT Register (offset = 130h) [reset = 0h]**

MCSPI\_CH0STAT is shown in [Figure](#page-56-1) 24-34 and described in Table [24-19.](#page-56-2)

The McSPI channel 0 status register (MCSPI\_CH0STAT) provides status information about the McSPI channel 0 FIFO transmit buffer register (MCSPI\_TX0) and the McSPI channel 0 FIFO receive buffer register (MCSPI\_RX0) of channel 0.

**Figure 24-34. MCSPI\_CH0STAT Register**

| 31       | 30    | 29    | 28    | 27       | 26   | 25   | 24
|----------|-------|-------|-------|----------|------|------|------
|          |       |       |       | RESERVED |      |      |
|          | R-0h  |       |       |          |      |      |
| 23       | 22    | 21    | 20    | 19       | 18   | 17   | 16
|          |       |       |       | RESERVED |      |      |
| R-0h     |       |       |       |          |      |      |
| 15       | 14    | 13    | 12    | 11       | 10   | 9    | 8
|          |       |       |       | RESERVED |      |      |
|          |       |       |       | R-0h     |      |      |
| 7        | 6     | 5     | 4     | 3        | 2    | 1    | 0
| RESERVED | RXFFF | RXFFE | TXFFF | TXFFE    | EOT  | TXS  | RXS
| R-0h     | R-0h  | R-0h  | R-0h  | R-0h     | R-0h | R-0h | R-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 24-19. MCSPI\_CH0STAT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                    |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-7 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 6    | RXFFF    | R    | 0h    | Channel 0 FIFO receive buffer full status.<br>0h = FIFO receive buffer is not full.<br>1h = FIFO receive buffer is full.                                                                                                                                                                                                                                                                                                       |
| 5    | RXFFE    | R    | 0h    | Channel 0 FIFO receive buffer empty status.<br>0h = FIFO receive buffer is not empty.<br>1h = FIFO receive buffer is empty.                                                                                                                                                                                                                                                                                                    |
| 4    | TXFFF    | R    | 0h    | Channel 0 FIFO transmit buffer full status.<br>0h = FIFO transmit buffer is not full.<br>1h = FIFO transmit buffer is full.                                                                                                                                                                                                                                                                                                    |
| 3    | TXFFE    | R    | 0h    | Channel 0 FIFO transmit buffer empty status.<br>0h = FIFO transmit buffer is not empty.<br>1h = FIFO transmit buffer is empty.                                                                                                                                                                                                                                                                                                 |
| 2    | EOT      | R    | 0h    | Channel 0 end-of-transfer status.<br>The definitions of beginning and end of transfer vary with master<br>versus slave and the transfer format (transmit/receive mode, turbo<br>mode).<br>0h = This flag is automatically cleared when the shift register is<br>loaded with the data from the transmitter register (beginning of<br>transfer).<br>1h = This flag is automatically set to one at the end of an SPI<br>transfer. |
| 1    | TXS      | R    | 0h    | Channel 0 transmitter register status.<br>The bit is cleared when the host writes the most significant byte of<br>the SPI word in the MCSPI_TX0 register.<br>The bit is set when enabling the channel 0 , and also when the SPI<br>word is transferred from the MCSPI_TX0 register to the shift register.<br>0h = Register is full.<br>1h = Register is empty.                                                                 |
**Table 24-19. MCSPI\_CH0STAT Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                      |
|-----|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0   | RXS   | R    | 0h    | Channel 0 receiver register status.<br>The bit is cleared when enabling the channel i, and also when the<br>host reads the most significant byte of the received SPI word from<br>the MCSPI_RX0 register.<br>The bit is set when the received SPI word is transferred from the<br>shift register to the MCSPI_RX0 register.<br>0h = Register is empty.<br>1h = Register is full. |
#### **24.4.1.10 MCSPI\_CH0CTRL Register (offset = 134h) [reset = 0h]**

MCSPI\_CH0CTRL is shown in [Figure](#page-58-1) 24-35 and described in Table [24-20](#page-58-2).

### **Figure 24-35. MCSPI\_CH0CTRL Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 24-20. MCSPI\_CH0CTRL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                             |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                         |
| 15-8  | EXTCLK   | R/W  | 0h    | Clock ratio extension.<br>Used to concatenate with the CLKD bit field in MCSPI_CH0CONF<br>for clock ratio only when granularity is 1 clock cycle (CLKG bit in<br>MCSPI_CH0CONF set to 1).<br>Then the maximum value reached is a 4096 clock divider ratio.<br>0h = Clock ratio is CLKD + 11h = Clock ratio is CLKD + 1 + 16FFh = Clock ratio is CLKD + 1 + 4080 |
| 7-1   | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                         |
| 0     | EN       | R/W  | 0h    | Channel 0 enable.<br>0h = Channel 0 is not active.<br>1h = Channel 0 is active.                                                                                                                                                                                                                                                                                         |
#### **24.4.1.11 MCSPI\_TX0 Register (offset = 138h) [reset = 0h]**

MCSPI\_TX0 is shown in [Figure](#page-59-1) 24-36 and described in Table [24-21.](#page-59-2)

The McSPI channel FIFO transmit buffer register (MCSPI\_TXx) contains a single McSPI word to transmit through the serial link. Little endian host access SPI 8-bit word on 0; big endian host accesses on 3h. SPI words are transferred with MSB first. Refer to [Section](#page-37-0) 24.3.10 for more information.

### **Figure 24-36. MCSPI\_TX0 Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-21. MCSPI\_TX0 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                 |
|------|-------|------|-------|-----------------------------|
| 31-0 | TDATA | R/W  | 0h    | Channel 0 data to transmit. |
#### **24.4.1.12 MCSPI\_RX0 Register (offset = 13Ch) [reset = 0h]**

MCSPI\_RX0 is shown in [Figure](#page-60-1) 24-37 and described in Table [24-22.](#page-60-2)

The McSPI channel 0 FIFO receive buffer register (MCSPI\_RX0) contains a single McSPI word received through the serial link. Little endian host access SPI 8 bit word on 0; big endian host accesses on 3h. Refer to [Section](#page-37-0) 24.3.10 for more information.

### **Figure 24-37. MCSPI\_RX0 Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-22. MCSPI\_RX0 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description              |
|------|-------|------|-------|--------------------------|
| 31-0 | RDATA | R    | 0h    | Channel 0 received data. |
#### **24.4.1.13 MCSPI\_CH1CONF Register (offset = 140h) [reset = 0x0006\_0000h]**

MCSPI\_CH1CONF is shown in [Figure](#page-61-1) 24-38 and described in Table [24-23.](#page-61-2)

The McSPI channel 1 configuration register (MCSPI\_CH1CONF) is used to configure channel 1.

**Figure 24-38. MCSPI\_CH1CONF Register**

| 31     | 30       | 29       | 28     | 27     | 26     | 25     | 24     |
|--------|----------|----------|--------|--------|--------|--------|--------|
|        | RESERVED | CLKG     | FFER   | FFEW   | TCS    |        | SBPOL  |
|        | R-0h     | R/W-0h   | R/W-0h | R/W-0h | R/W-0h |        | R/W-0h |
| 23     | 22       | 21       | 20     | 19     | 18     | 17     | 16     |
| SBE    |          | SPIENSLV | FORCE  | TURBO  | IS     | DPE1   | DPE0   |
| R/W-0h |          | R/W-0h   | R/W-0h |        | R/W-0h | R/W-0h | R/W-0h |
| 15     | 14       | 13       | 12     | 11     | 10     | 9      | 8      |
| DMAR   | DMAW     |          | TRM    | WL     |        |        |        |
| R/W-0h | R/W-0h   |          | R/W-0h |        | R/W-0h |        |        |
| 7      | 6        | 5        | 4      | 3      | 2      | 1      | 0      |
| WL     | EPOL     |          |        | CLKD   |        | POL    | PHA    |
| R/W-0h | R/W-0h   |          |        | R/W-0h |        | R/W-0h | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-23. MCSPI\_CH1CONF Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|-------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-30 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 29    | CLKG     | R/W  | 0h    | Clock divider granularity.<br>This register defines the granularity of channel clock divider: power<br>of two or one clock cycle granularity.<br>When this bit is set the register MCSPI_CHCTRL[EXTCLK] must be<br>configured to reach a maximum of 4096 clock divider ratio.<br>Then The clock divider ratio is a concatenation of<br>MCSPI_CHCONF[CLKD] and MCSPI_CHCTRL[EXTCLK] values.<br>0h = Clock granularity of power of 21h = 1 clock cycle granularity |
| 28    | FFER     | R/W  | 0h    | FIFO enabled for receive.<br>Only one channel can have this bit set.<br>0h = The buffer is not used to receive data.<br>1h = The buffer is used to receive data.                                                                                                                                                                                                                                                                                                     |
| 27    | FFEW     | R/W  | 0h    | FIFO enabled for transmit.<br>Only one channel can have this bit set.<br>0h = The buffer is not used to transmit data.<br>1h = The buffer is used to transmit data.                                                                                                                                                                                                                                                                                                  |
| 26-25 | TCS      | R/W  | 0h    | Chip select time control.<br>These two bits define the number of interface clock cycles between<br>CS toggling and first or last edge of SPI clock.<br>0h = 0.5 clock cycles<br>1h = 1.5 clock cycles<br>2h = 2.5 clock cycles<br>3h = 3.5 clock cycles                                                                                                                                                                                                              |
| 24    | SBPOL    | R/W  | 0h    | Start bit polarity.<br>0h = Start bit polarity is held to 0 during SPI transfer.<br>1h = Start bit polarity is held to 1 during SPI transfer.                                                                                                                                                                                                                                                                                                                        |
| 23    | SBE      | R/W  | 0h    | Start bit enable for SPI transfer.<br>0h = Default SPI transfer length as specified by WL bit field.<br>1h = Start bit D/CX added before SPI transfer. Polarity is defined by<br>MCSPI_CH1CONF[SBPOL].                                                                                                                                                                                                                                                               |
### **Table 24-23. MCSPI\_CH1CONF Register Field Descriptions (continued)**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                              |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 22-21 | SPIENSLV | R/W  | 0h    | Channel 0 only and slave mode only: SPI slave select signal<br>detection.<br>Reserved bits (read returns 0) for other cases.<br>0h = Detection enabled only on SPIEN[0]<br>1h = Detection enabled only on SPIEN[1]<br>2h = Detection enabled only on SPIEN[2]<br>3h = Detection enabled only on SPIEN[3]                                                                                 |
| 20    | FORCE    | R/W  | 0h    | Manual SPIEN assertion to keep SPIEN active between SPI words<br>(single channel master mode only).<br>0h = Writing 0 into this bit drives low the SPIEN line when<br>MCSPI_CHCONF1[EPOL]=0, and drives it high when<br>MCSPI_CHCONF1[EPOL]=1.<br>1h = Writing 1 into this bit drives high the SPIEN line when<br>MCSPI_CHCONF1[EPOL]=0, and drives it low when<br>MCSPI_CHCONF1[EPOL]=1 |
| 19    | TURBO    | R/W  | 0h    | Turbo mode.<br>0h = Turbo is deactivated (recommended for single SPI word<br>transfer).<br>1h = Turbo is activated to maximize the throughput for multi-SPI<br>word transfers.                                                                                                                                                                                                           |
| 18    | IS       | R/W  | 0h    | Input select<br>0h = Data line 0 (SPIDAT[0]) selected for reception.<br>1h = Data line 1 (SPIDAT[1]) selected for reception.                                                                                                                                                                                                                                                             |
| 17    | DPE1     | R/W  | 0h    | Transmission enable for data line 1 (SPIDATAGZEN[1])<br>0h = Data line 1 (SPIDAT[1]) selected for transmission<br>1h = No transmission on data line 1 (SPIDAT[1])                                                                                                                                                                                                                        |
| 16    | DPE0     | R/W  | 0h    | Transmission enable for data line 0 (SPIDATAGZEN[0])<br>0h = Data line 0 (SPIDAT[0]) selected for transmission<br>1h = No transmission on data line 0 (SPIDAT[0])                                                                                                                                                                                                                        |
| 15    | DMAR     | R/W  | 0h    | DMA read request.<br>The DMA read request line is asserted when the channel is enabled<br>and new data is available in the receive register of the channel.<br>The DMA read request line is deasserted on read completion of the<br>receive register of the channel.<br>0h = DMA read request is disabled.<br>1h = DMA read request is enabled.                                          |
| 14    | DMAW     | R/W  | 0h    | DMA write request.<br>The DMA write request line is asserted when the channel is enabled<br>and the MCSPI_TX1 register of the channel is empty.<br>The DMA write request line is deasserted on load completion of the<br>MCSPI_TX1 register of the channel.<br>0h = DMA write request is disabled.<br>1h = DMA write request is enabled.                                                 |
| 13-12 | TRM      | R/W  | 0h    | Transmit/receive modes.<br>0h = Transmit and receive mode<br>1h = Receive-only mode<br>2h = Transmit-only mode<br>3h = Reserved                                                                                                                                                                                                                                                          |
**Table 24-23. MCSPI\_CH1CONF Register Field Descriptions (continued)**

| Bit  | Field | Type | Reset | Description                                      |
|------|-------|------|-------|--------------------------------------------------|
| 11-7 | WL    | R/W  | 0h    | SPI word length.                                 |
|      |       |      |       | 0h = Reserved                                    |
|      |       |      |       | 1h = Reserved                                    |
|      |       |      |       | 2h = Reserved                                    |
|      |       |      |       | 3h = The SPI word is 4-bits long.                |
|      |       |      |       | 4h = The SPI word is 5-bits long                 |
|      |       |      |       | 5h = The SPI word is 6-bits long                 |
|      |       |      |       | 6h = The SPI word is 7-bits long                 |
|      |       |      |       | 7h = The SPI word is 8-bits long                 |
|      |       |      |       | 8h = The SPI word is 9-bits long                 |
|      |       |      |       | 9h = The SPI word is 10-bits long                |
|      |       |      |       | Ah = The SPI word is 11-bits long                |
|      |       |      |       | Bh = The SPI word is 12-bits long                |
|      |       |      |       | Ch = The SPI word is 13-bits long                |
|      |       |      |       | Dh = The SPI word is 14-bits long                |
|      |       |      |       | Eh = The SPI word is 15-bits long                |
|      |       |      |       | Fh = The SPI word is 16-bits long                |
|      |       |      |       | 10h = The SPI word is 17-bits long               |
|      |       |      |       | 11h = The SPI word is 18-bits long               |
|      |       |      |       | 12h = The SPI word is 19-bits long               |
|      |       |      |       | 13h = The SPI word is 20-bits long               |
|      |       |      |       | 14h = The SPI word is 21-bits long               |
|      |       |      |       | 15h = The SPI word is 22-bits long               |
|      |       |      |       | 16h = The SPI word is 23-bits long               |
|      |       |      |       | 17h = The SPI word is 24-bits long               |
|      |       |      |       | 18h = The SPI word is 25-bits long               |
|      |       |      |       | 19h = The SPI word is 26-bits long               |
|      |       |      |       | 1Ah = The SPI word is 27-bits long               |
|      |       |      |       | 1Bh = The SPI word is 28-bits long               |
|      |       |      |       | 1Ch = The SPI word is 29-bits long               |
|      |       |      |       | 1Dh = The SPI word is 30-bits long               |
|      |       |      |       | 1Eh = The SPI word is 31-bits long               |
|      |       |      |       | 1Fh = The SPI word is 32-bits long               |
| 6    | EPOL  | R/W  | 0h    | SPIEN polarity                                   |
|      |       |      |       | 0h = SPIEN is held high during the active state. |
|      |       |      |       | 1h = SPIEN is held low during the active state.  |
### **Table 24-23. MCSPI\_CH1CONF Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
|-----|-------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 5-2 | CLKD  | R/W  | 0h    | Frequency divider for SPICLK (only when the module is a Master<br>SPI device).<br>A programmable clock divider divides the SPI reference clock<br>(CLKSPIREF) with a 4 bit value, and results in a new clock SPICLK<br>available to shift-in and shift-out data.<br>By default the clock divider ratio has a power of two granularity when<br>MCSPI_CHCONF[CLKG] is cleared, Otherwise this register is the 4LSB bit of a 12 bit register concatenated with clock divider extension<br>MCSPI_CHCTRL[EXTCLK] register.<br>The value description below defines the clock ratio when<br>MCSPI_CHCONF[CLKG] is cleared to 0.<br>0h = Divide by 1.<br>1h = Divide by 2.<br>2h = Divide by 4.<br>3h = Divide by 8.<br>4h = Divide by 16.<br>5h = Divide by 32.<br>6h = Divide by 64.<br>7h = Divide by 128.<br>8h = Divide by 256.<br>9h = Divide by 512.<br>Ah = Divide by 1024.<br>Bh = Divide by 2048.<br>Ch = Divide by 4096.<br>Dh = Divide by 8192.<br>Eh = Divide by 16384.<br>Fh = Divide by 32768. |
| 1   | POL   | R/W  | 0h    | SPICLK polarity<br>0h = SPICLK is held high during the active state<br>1h = SPICLK is held low during the active state                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 0   | PHA   | R/W  | 0h    | SPICLK phase<br>0h = Data are latched on odd numbered edges of SPICLK1h = Data are latched on even numbered edges of SPICLK                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
#### **24.4.1.14 MCSPI\_CH1STAT Register (offset = 144h) [reset = 0h]**

MCSPI\_CH1STAT is shown in [Figure](#page-65-1) 24-39 and described in Table [24-24.](#page-65-2)

The McSPI channel 1 status register (MCSPI\_CH1STAT) provides status information about the McSPI channel 1 FIFO transmit buffer register (MCSPI\_TX1) and the McSPI channel 1 FIFO receive buffer register (MCSPI\_RX1) of channel 1.

**Figure 24-39. MCSPI\_CH1STAT Register**

| 31       | 30    | 29    | 28    | 27       | 26   | 25   | 24   |
|----------|-------|-------|-------|----------|------|------|------|
|          |       |       |       | RESERVED |      |      |      |
|          |       |       |       | R-0h     |      |      |      |
| 23       | 22    | 21    | 20    | 19       | 18   | 17   | 16   |
|          |       |       |       | RESERVED |      |      |      |
|          |       |       |       | R-0h     |      |      |      |
| 15       | 14    | 13    | 12    | 11       | 10   | 9    | 8    |
|          |       |       |       | RESERVED |      |      |      |
|          |       |       |       | R-0h     |      |      |      |
| 7        | 6     | 5     | 4     | 3        | 2    | 1    | 0    |
| RESERVED | RXFFF | RXFFE | TXFFF | TXFFE    | EOT  | TXS  | RXS  |
| R-0h     | R-0h  | R-0h  | R-0h  | R-0h     | R-0h | R-0h | R-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-24. MCSPI\_CH1STAT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                    |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-7 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 6    | RXFFF    | R    | 0h    | Channel 1 FIFO receive buffer full status.<br>0h = FIFO receive buffer is not full.<br>1h = FIFO receive buffer is full.                                                                                                                                                                                                                                                                                                       |
| 5    | RXFFE    | R    | 0h    | Channel 1 FIFO receive buffer empty status.<br>0h = FIFO receive buffer is not empty.<br>1h = FIFO receive buffer is empty.                                                                                                                                                                                                                                                                                                    |
| 4    | TXFFF    | R    | 0h    | Channel 1 FIFO transmit buffer full status.<br>0h = FIFO transmit buffer is not full.<br>1h = FIFO transmit buffer is full.                                                                                                                                                                                                                                                                                                    |
| 3    | TXFFE    | R    | 0h    | Channel 1 FIFO transmit buffer empty status.<br>0h = FIFO transmit buffer is not empty.<br>1h = FIFO transmit buffer is empty.                                                                                                                                                                                                                                                                                                 |
| 2    | EOT      | R    | 0h    | Channel 1 end-of-transfer status.<br>The definitions of beginning and end of transfer vary with master<br>versus slave and the transfer format (transmit/receive mode, turbo<br>mode).<br>0h = This flag is automatically cleared when the shift register is<br>loaded with the data from the transmitter register (beginning of<br>transfer).<br>1h = This flag is automatically set to one at the end of an SPI<br>transfer. |
| 1    | TXS      | R    | 0h    | Channel 1 transmitter register status.<br>The bit is cleared when the host writes the most significant byte of<br>the SPI word in the MCSPI_TX1 register.<br>The bit is set when enabling the channel 1 , and also when the SPI<br>word is transferred from the MCSPI_TX1 register to the shift register.<br>0h = Register is full.<br>1h = Register is empty.                                                                 |
### **Table 24-24. MCSPI\_CH1STAT Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                      |
|-----|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0   | RXS   | R    | 0h    | Channel 1 receiver register status.<br>The bit is cleared when enabling the channel i, and also when the<br>host reads the most significant byte of the received SPI word from<br>the MCSPI_RX1 register.<br>The bit is set when the received SPI word is transferred from the<br>shift register to the MCSPI_RX1 register.<br>0h = Register is empty.<br>1h = Register is full. |
#### **24.4.1.15 MCSPI\_CH1CTRL Register (offset = 148h) [reset = 0h]**

MCSPI\_CH1CTRL is shown in [Figure](#page-67-1) 24-40 and described in Table [24-25](#page-67-2).

### **Figure 24-40. MCSPI\_CH1CTRL Register**

| 31 | 30 | 29 | 28     | 27 | 26 | 25 | 24 | 23       | 22 | 21 | 20       | 19 | 18 | 17 | 16        |
|----|----|----|--------|----|----|----|----|----------|----|----|----------|----|----|----|-----------|
|    |    |    |        |    |    |    |    | RESERVED |    |    |          |    |    |    |           |
|    |    |    |        |    |    |    |    | R-0h     |    |    |          |    |    |    |           |
| 15 | 14 | 13 | 12     | 11 | 10 | 9  | 8  | 7        | 6  | 5  | 4        | 3  | 2  | 1  | 0         |
|    |    |    | EXTCLK |    |    |    |    |          |    |    | RESERVED |    |    |    | EN        |
|    |    |    | R/W-0h |    |    |    |    |          |    |    | R-0h     |    |    |    | R/W0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-25. MCSPI\_CH1CTRL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                             |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                         |
| 15-8  | EXTCLK   | R/W  | 0h    | Clock ratio extension.<br>Used to concatenate with the CLKD bit field in MCSPI_CH1CONF<br>for clock ratio only when granularity is 1 clock cycle (CLKG bit in<br>MCSPI_CH1CONF set to 1).<br>Then the maximum value reached is a 4096 clock divider ratio.<br>0h = Clock ratio is CLKD + 11h = Clock ratio is CLKD + 1 + 16FFh = Clock ratio is CLKD + 1 + 4080 |
| 7-1   | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                         |
| 0     | EN       | R/W  | 0h    | Channel 1 enable.<br>0h = Channel 1 is not active.<br>1h = Channel 1 is active.                                                                                                                                                                                                                                                                                         |
#### **24.4.1.16 MCSPI\_TX1 Register (offset = 14Ch) [reset = 0h]**

MCSPI\_TX1 is shown in [Figure](#page-68-1) 24-41 and described in Table [24-26.](#page-68-2)

The McSPI channel FIFO transmit buffer register (MCSPI\_TXx) contains a single McSPI word to transmit through the serial link. Little endian host access SPI 8-bit word on 0; big endian host accesses on 3h. SPI words are transferred with MSB first. Refer to [Section](#page-37-0) 24.3.10 for more information.

**Figure 24-41. MCSPI\_TX1 Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15     | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|--------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | TDATA  |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-0h |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-26. MCSPI\_TX1 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                 |
|------|-------|------|-------|-----------------------------|
| 31-0 | TDATA | R/W  | 0h    | Channel 1 data to transmit. |
#### **24.4.1.17 MCSPI\_RX1 Register (offset = 150h) [reset = 0h]**

MCSPI\_RX1 is shown in [Figure](#page-69-1) 24-42 and described in Table [24-27.](#page-69-2)

The McSPI channel 1 FIFO receive buffer register (MCSPI\_RX1) contains a single McSPI word received through the serial link. Little endian host access SPI 8 bit word on 0; big endian host accesses on 3h. Refer to [Section](#page-37-0) 24.3.10 for more information.

### **Figure 24-42. MCSPI\_RX1 Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-27. MCSPI\_RX1 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description              |
|------|-------|------|-------|--------------------------|
| 31-0 | RDATA | R    | 0h    | Channel 1 received data. |
#### **24.4.1.18 MCSPI\_CH2CONF Register (offset = 154h) [reset = 0x0006\_0000h]**

MCSPI\_CH2CONF is shown in [Figure](#page-70-1) 24-43 and described in Table [24-28.](#page-70-2)

The McSPI channel 2 configuration register (MCSPI\_CH2CONF) is used to configure channel 2.

**Figure 24-43. MCSPI\_CH2CONF Register**

| 31     | 30       | 29       | 28     | 27     | 26     | 25     | 24     |
|--------|----------|----------|--------|--------|--------|--------|--------|
|        | RESERVED | CLKG     | FFER   | FFEW   | TCS    |        | SBPOL  |
|        | R-0h     | R/W-0h   | R/W-0h | R/W-0h | R/W-0h |        | R/W-0h |
| 23     | 22       | 21       | 20     | 19     | 18     | 17     | 16     |
| SBE    |          | SPIENSLV | FORCE  | TURBO  | IS     | DPE1   | DPE0   |
| R/W-0h |          | R/W-0h   | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R/W-0h |
| 15     | 14       | 13       | 12     | 11     | 10     | 9      | 8      |
| DMAR   | DMAW     |          | TRM    |        | WL     |        |        |
| R/W-0h | R/W-0h   |          | R/W-0h |        | R/W-0h |        |        |
| 7      | 6        | 5        | 4      | 3      | 2      | 1      | 0      |
| WL     | EPOL     |          |        | CLKD   |        | POL    | PHA    |
| R/W-0h | R/W-0h   |          |        | R/W-0h |        | R/W-0h | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 24-28. MCSPI\_CH2CONF Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|-------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-30 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 29    | CLKG     | R/W  | 0h    | Clock divider granularity.<br>This register defines the granularity of channel clock divider: power<br>of two or one clock cycle granularity.<br>When this bit is set the register MCSPI_CHCTRL[EXTCLK] must be<br>configured to reach a maximum of 4096 clock divider ratio.<br>Then The clock divider ratio is a concatenation of<br>MCSPI_CHCONF[CLKD] and MCSPI_CHCTRL[EXTCLK] values.<br>0h = Clock granularity of power of 21h = 1 clock cycle granularity |
| 28    | FFER     | R/W  | 0h    | FIFO enabled for receive.<br>Only one channel can have this bit set.<br>0h = The buffer is not used to receive data.<br>1h = The buffer is used to receive data.                                                                                                                                                                                                                                                                                                     |
| 27    | FFEW     | R/W  | 0h    | FIFO enabled for transmit.<br>Only one channel can have this bit set.<br>0h = The buffer is not used to transmit data.<br>1h = The buffer is used to transmit data.                                                                                                                                                                                                                                                                                                  |
| 26-25 | TCS      | R/W  | 0h    | Chip select time control.<br>These two bits define the number of interface clock cycles between<br>CS toggling and first or last edge of SPI clock.<br>0h = 0.5 clock cycles<br>1h = 1.5 clock cycles<br>2h = 2.5 clock cycles<br>3h = 3.5 clock cycles                                                                                                                                                                                                              |
| 24    | SBPOL    | R/W  | 0h    | Start bit polarity.<br>0h = Start bit polarity is held to 0 during SPI transfer.<br>1h = Start bit polarity is held to 1 during SPI transfer.                                                                                                                                                                                                                                                                                                                        |
| 23    | SBE      | R/W  | 0h    | Start bit enable for SPI transfer.<br>0h = Default SPI transfer length as specified by WL bit field.<br>1h = Start bit D/CX added before SPI transfer. Polarity is defined by<br>MCSPI_CH2CONF[SBPOL].                                                                                                                                                                                                                                                               |
### **Table 24-28. MCSPI\_CH2CONF Register Field Descriptions (continued)**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                              |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 22-21 | SPIENSLV | R/W  | 0h    | Channel 0 only and slave mode only: SPI slave select signal<br>detection.<br>Reserved bits (read returns 0) for other cases.<br>0h = Detection enabled only on SPIEN[0]<br>1h = Detection enabled only on SPIEN[1]<br>2h = Detection enabled only on SPIEN[2]<br>3h = Detection enabled only on SPIEN[3]                                                                                 |
| 20    | FORCE    | R/W  | 0h    | Manual SPIEN assertion to keep SPIEN active between SPI words<br>(single channel master mode only).<br>0h = Writing 0 into this bit drives the SPIEN line when<br>MCSPI_CHCONF2[EPOL]=0, and drives it high when<br>MCSPI_CHCONF2[EPOL]=1.<br>1h = Writing 1 into this bit drives low high the SPIEN line when<br>MCSPI_CHCONF2[EPOL]=0, and drives it low when<br>MCSPI_CHCONF2[EPOL]=1 |
| 19    | TURBO    | R/W  | 0h    | Turbo mode.<br>0h = Turbo is deactivated (recommended for single SPI word<br>transfer).<br>1h = Turbo is activated to maximize the throughput for multi-SPI<br>word transfers.                                                                                                                                                                                                           |
| 18    | IS       | R/W  | 0h    | Input select<br>0h = Data line 0 (SPIDAT[0]) selected for reception.<br>1h = Data line 1 (SPIDAT[1]) selected for reception.                                                                                                                                                                                                                                                             |
| 17    | DPE1     | R/W  | 0h    | Transmission enable for data line 1 (SPIDATAGZEN[1])<br>0h = Data line 1 (SPIDAT[1]) selected for transmission<br>1h = No transmission on data line 1 (SPIDAT[1])                                                                                                                                                                                                                        |
| 16    | DPE0     | R/W  | 0h    | Transmission enable for data line 0 (SPIDATAGZEN[0])<br>0h = Data line 0 (SPIDAT[0]) selected for transmission<br>1h = No transmission on data line 0 (SPIDAT[0])                                                                                                                                                                                                                        |
| 15    | DMAR     | R/W  | 0h    | DMA read request.<br>The DMA read request line is asserted when the channel is enabled<br>and new data is available in the receive register of the channel.<br>The DMA read request line is deasserted on read completion of the<br>receive register of the channel.<br>0h = DMA read request is disabled.<br>1h = DMA read request is enabled.                                          |
| 14    | DMAW     | R/W  | 0h    | DMA write request.<br>The DMA write request line is asserted when the channel is enabled<br>and the MCSPI_TX2 register of the channel is empty.<br>The DMA write request line is deasserted on load completion of the<br>MCSPI_TX2 register of the channel.<br>0h = DMA write request is disabled.<br>1h = DMA write request is enabled.                                                 |
| 13-12 | TRM      | R/W  | 0h    | Transmit/receive modes.<br>0h = Transmit and receive mode<br>1h = Receive-only mode<br>2h = Transmit-only mode<br>3h = Reserved                                                                                                                                                                                                                                                          |
**Table 24-28. MCSPI\_CH2CONF Register Field Descriptions (continued)**

| Bit  | Field | Type | Reset | Description                                      |
|------|-------|------|-------|--------------------------------------------------|
| 11-7 | WL    | R/W  | 0h    | SPI word length.                                 |
|      |       |      |       | 0h = Reserved                                    |
|      |       |      |       | 1h = Reserved                                    |
|      |       |      |       | 2h = Reserved                                    |
|      |       |      |       | 3h = The SPI word is 4-bits long.                |
|      |       |      |       | 4h = The SPI word is 5-bits long                 |
|      |       |      |       | 5h = The SPI word is 6-bits long                 |
|      |       |      |       | 6h = The SPI word is 7-bits long                 |
|      |       |      |       | 7h = The SPI word is 8-bits long                 |
|      |       |      |       | 8h = The SPI word is 9-bits long                 |
|      |       |      |       | 9h = The SPI word is 10-bits long                |
|      |       |      |       | Ah = The SPI word is 11-bits long                |
|      |       |      |       | Bh = The SPI word is 12-bits long                |
|      |       |      |       | Ch = The SPI word is 13-bits long                |
|      |       |      |       | Dh = The SPI word is 14-bits long                |
|      |       |      |       | Eh = The SPI word is 15-bits long                |
|      |       |      |       | Fh = The SPI word is 16-bits long                |
|      |       |      |       | 10h = The SPI word is 17-bits long               |
|      |       |      |       | 11h = The SPI word is 18-bits long               |
|      |       |      |       | 12h = The SPI word is 19-bits long               |
|      |       |      |       | 13h = The SPI word is 20-bits long               |
|      |       |      |       | 14h = The SPI word is 21-bits long               |
|      |       |      |       | 15h = The SPI word is 22-bits long               |
|      |       |      |       | 16h = The SPI word is 23-bits long               |
|      |       |      |       | 17h = The SPI word is 24-bits long               |
|      |       |      |       | 18h = The SPI word is 25-bits long               |
|      |       |      |       | 19h = The SPI word is 26-bits long               |
|      |       |      |       | 1Ah = The SPI word is 27-bits long               |
|      |       |      |       | 1Bh = The SPI word is 28-bits long               |
|      |       |      |       | 1Ch = The SPI word is 29-bits long               |
|      |       |      |       | 1Dh = The SPI word is 30-bits long               |
|      |       |      |       | 1Eh = The SPI word is 31-bits long               |
|      |       |      |       | 1Fh = The SPI word is 32-bits long               |
| 6    | EPOL  | R/W  | 0h    | SPIEN polarity                                   |
|      |       |      |       | 0h = SPIEN is held high during the active state. |
|      |       |      |       | 1h = SPIEN is held low during the active state.  |
**Table 24-28. MCSPI\_CH2CONF Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
|-----|-------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 5-2 | CLKD  | R/W  | 0h    | Frequency divider for SPICLK (only when the module is a Master<br>SPI device).<br>A programmable clock divider divides the SPI reference clock<br>(CLKSPIREF) with a 4 bit value, and results in a new clock SPICLK<br>available to shift-in and shift-out data.<br>By default the clock divider ratio has a power of two granularity when<br>MCSPI_CHCONF[CLKG] is cleared, Otherwise this register is the 4LSB bit of a 12 bit register concatenated with clock divider extension<br>MCSPI_CHCTRL[EXTCLK] register.<br>The value description below defines the clock ratio when<br>MCSPI_CHCONF[CLKG] is cleared to 0.<br>0h = Divide by 1.<br>1h = Divide by 2.<br>2h = Divide by 4.<br>3h = Divide by 8.<br>4h = Divide by 16.<br>5h = Divide by 32.<br>6h = Divide by 64.<br>7h = Divide by 128.<br>8h = Divide by 256.<br>9h = Divide by 512.<br>Ah = Divide by 1024.<br>Bh = Divide by 2048.<br>Ch = Divide by 4096.<br>Dh = Divide by 8192.<br>Eh = Divide by 16384.<br>Fh = Divide by 32768. |
| 1   | POL   | R/W  | 0h    | SPICLK polarity<br>0h = SPICLK is held high during the active state<br>1h = SPICLK is held low during the active state                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 0   | PHA   | R/W  | 0h    | SPICLK phase<br>0h = Data are latched on odd numbered edges of SPICLK1h = Data are latched on even numbered edges of SPICLK                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
#### **24.4.1.19 MCSPI\_CH2STAT Register (offset = 158h) [reset = 0h]**

MCSPI\_CH2STAT is shown in [Figure](#page-74-1) 24-44 and described in Table [24-29.](#page-74-2)

The McSPI channel 2 status register (MCSPI\_CH2STAT) provides status information about the McSPI channel 2 FIFO transmit buffer register (MCSPI\_TX2) and the McSPI channel 2 FIFO receive buffer register (MCSPI\_RX2) of channel 2.

**Figure 24-44. MCSPI\_CH2STAT Register**

| 31       | 30       | 29    | 28    | 27       | 26   | 25   | 24
|----------|----------|-------|-------|----------|------|------|------|--|--
|          | RESERVED |       |       |          |      |      |
|          | R-0h     |       |       |          |      |      |
| 23       | 22       | 21    | 20    | 19       | 18   | 17   | 16
|          |          |       |       | RESERVED |      |      |
|          | R-0h     |       |       |          |      |      |
| 15       | 14       | 13    | 12    | 11       | 10   | 9    | 8
|          |          |       |       | RESERVED |      |      |
|          |          |       |       | R-0h     |      |      |
| 7        | 6        | 5     | 4     | 3        | 2    | 1    | 0
| RESERVED | RXFFF    | RXFFE | TXFFF | TXFFE    | EOT  | TXS  | RXS
| R-0h     | R-0h     | R-0h  | R-0h  | R-0h     | R-0h | R-0h | R-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 24-29. MCSPI\_CH2STAT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| 31-7 | RESERVED | R    | 0h    |
| 6    | RXFFF    | R    | 0h    | Channel 2 FIFO receive buffer full status.<br>0h = FIFO receive buffer is not full.<br>1h = FIFO receive buffer is full.
| 5    | RXFFE    | R    | 0h    | Channel 2 FIFO receive buffer empty status.<br>0h = FIFO receive buffer is not empty.<br>1h = FIFO receive buffer is empty.<br>Channel 2 FIFO transmit buffer full status.
| 4    | TXFFF    | R    | 0h    | 0h = FIFO transmit buffer is not full.<br>1h = FIFO transmit buffer is full.
| 3    | TXFFE    | R    | 0h    | Channel 2 FIFO transmit buffer empty status.<br>0h = FIFO transmit buffer is not empty.<br>1h = FIFO transmit buffer is empty.
| 2    | EOT      | R    | 0h    | Channel 2 end-of-transfer status.<br>The definitions of beginning and end of transfer vary with master<br>versus slave and the transfer format (transmit/receive mode, turbo<br>mode).<br>0h = This flag is automatically cleared when the shift register is<br>loaded with the data from the transmitter register (beginning of<br>transfer).<br>1h = This flag is automatically set to one at the end of an SPI<br>transfer.
| 1    | TXS      | R    | 0h    | Channel 2 transmitter register status.<br>The bit is cleared when the host writes the most significant byte of<br>the SPI word in the MCSPI_TX2 register.<br>The bit is set when enabling the channel 2 , and also when the SPI<br>word is transferred from the MCSPI_TX2 register to the shift register.<br>0h = Register is full.<br>1h = Register is empty.
### **Table 24-29. MCSPI\_CH2STAT Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                      |
|-----|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0   | RXS   | R    | 0h    | Channel 2 receiver register status.<br>The bit is cleared when enabling the channel i, and also when the<br>host reads the most significant byte of the received SPI word from<br>the MCSPI_RX2 register.<br>The bit is set when the received SPI word is transferred from the<br>shift register to the MCSPI_RX2 register.<br>0h = Register is empty.<br>1h = Register is full. |
#### **24.4.1.20 MCSPI\_CH2CTRL Register (offset = 15Ch) [reset = 0h]**

MCSPI\_CH2CTRL is shown in [Figure](#page-76-1) 24-45 and described in Table [24-30](#page-76-2).

### **Figure 24-45. MCSPI\_CH2CTRL Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 24-30. MCSPI\_CH2CTRL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                             |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                         |
| 15-8  | EXTCLK   | R/W  | 0h    | Clock ratio extension.<br>Used to concatenate with the CLKD bit field in MCSPI_CH2CONF<br>for clock ratio only when granularity is 1 clock cycle (CLKG bit in<br>MCSPI_CH2CONF set to 1).<br>Then the maximum value reached is a 4096 clock divider ratio.<br>0h = Clock ratio is CLKD + 11h = Clock ratio is CLKD + 1 + 16FFh = Clock ratio is CLKD + 1 + 4080 |
| 7-1   | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                         |
| 0     | EN       | R/W  | 0h    | Channel 2 enable.<br>0h = Channel 2 is not active.<br>1h = Channel 2 is active.                                                                                                                                                                                                                                                                                         |
#### **24.4.1.21 MCSPI\_TX2 Register (offset = 160h) [reset = 0h]**

MCSPI\_TX2 is shown in [Figure](#page-77-1) 24-46 and described in Table [24-31.](#page-77-2)

The McSPI channel FIFO transmit buffer register (MCSPI\_TXx) contains a single McSPI word to transmit through the serial link. Little endian host access SPI 8-bit word on 0; big endian host accesses on 3h. SPI words are transferred with MSB first. Refer to [Section](#page-37-0) 24.3.10 for more information.

**Figure 24-46. MCSPI\_TX2 Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-31. MCSPI\_TX2 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                 |
|------|-------|------|-------|-----------------------------|
| 31-0 | TDATA | R/W  | 0h    | Channel 2 data to transmit. |
#### **24.4.1.22 MCSPI\_RX2 Register (offset = 164h) [reset = 0h]**

MCSPI\_RX2 is shown in [Figure](#page-78-1) 24-47 and described in Table [24-32.](#page-78-2)

The McSPI channel 2 FIFO receive buffer register (MCSPI\_RX2) contains a single McSPI word received through the serial link. Little endian host access SPI 8 bit word on 0; big endian host accesses on 3h. Refer to [Section](#page-37-0) 24.3.10 for more information.

**Figure 24-47. MCSPI\_RX2 Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-32. MCSPI\_RX2 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description              |
|------|-------|------|-------|--------------------------|
| 31-0 | RDATA | R    | 0h    | Channel 2 received data. |
#### **24.4.1.23 MCSPI\_CH3CONF Register (offset = 168h) [reset = 0x0006\_0000h]**

MCSPI\_CH3CONF is shown in [Figure](#page-79-1) 24-48 and described in Table [24-33.](#page-79-2)

The McSPI channel 3 configuration register (MCSPI\_CH3CONF) is used to configure channel 3.

**Figure 24-48. MCSPI\_CH3CONF Register**

| 31     | 30               | 29       | 28     | 27     | 26     | 25     | 24     |
|--------|------------------|----------|--------|--------|--------|--------|--------|
|        | RESERVEDCLKG |          | FFER   | FFEW   | TCS    |        | SBPOL  |
|        | R-0h             | R/W-0h   | R/W-0h | R/W-0h | R/W-0h |        | R/W-0h |
| 23     | 22               | 21       | 20     | 19     | 18     | 17     | 16     |
| SBE    |                  | SPIENSLV | FORCE  | TURBO  | IS     | DPE1   | DPE0   |
| R/W-0h |                  | R/W-0h   | R/W-0h | R/W-0h | R/W-0h | R/W-0h | R/W-0h |
| 15     | 14               | 13       | 12     | 11     | 10     | 9      | 8      |
| DMAR   | DMAW             |          | TRM    | WL     |        |        |        |
| R/W-0h | R/W-0h           |          | R/W-0h |        | R/W-0h |        |        |
| 7      | 6                | 5        | 4      | 3      | 2      | 1      | 0      |
| WL     | EPOL             |          |        | CLKD   |        | POL    | PHA    |
| R/W-0h | R/W-0h           |          |        | R/W-0h |        | R/W-0h | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-33. MCSPI\_CH3CONF Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|-------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-30 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 29    | CLKG     | R/W  | 0h    | Clock divider granularity.<br>This register defines the granularity of channel clock divider: power<br>of two or one clock cycle granularity.<br>When this bit is set the register MCSPI_CHCTRL[EXTCLK] must be<br>configured to reach a maximum of 4096 clock divider ratio.<br>Then The clock divider ratio is a concatenation of<br>MCSPI_CHCONF[CLKD] and MCSPI_CHCTRL[EXTCLK] values.<br>0h = Clock granularity of power of 21h = 1 clock cycle granularity |
| 28    | FFER     | R/W  | 0h    | FIFO enabled for receive.<br>Only one channel can have this bit set.<br>0h = The buffer is not used to receive data.<br>1h = The buffer is used to receive data.                                                                                                                                                                                                                                                                                                     |
| 27    | FFEW     | R/W  | 0h    | FIFO enabled for transmit.<br>Only one channel can have this bit set.<br>0h = The buffer is not used to transmit data.<br>1h = The buffer is used to transmit data.                                                                                                                                                                                                                                                                                                  |
| 26-25 | TCS      | R/W  | 0h    | Chip select time control.<br>These two bits define the number of interface clock cycles between<br>CS toggling and first or last edge of SPI clock.<br>0h = 0.5 clock cycles<br>1h = 1.5 clock cycles<br>2h = 2.5 clock cycles<br>3h = 3.5 clock cycles                                                                                                                                                                                                              |
| 24    | SBPOL    | R/W  | 0h    | Start bit polarity.<br>0h = Start bit polarity is held to 0 during SPI transfer.<br>1h = Start bit polarity is held to 1 during SPI transfer.                                                                                                                                                                                                                                                                                                                        |
| 23    | SBE      | R/W  | 0h    | Start bit enable for SPI transfer.<br>0h = Default SPI transfer length as specified by WL bit field.<br>1h = Start bit D/CX added before SPI transfer. Polarity is defined by<br>MCSPI_CH3CONF[SBPOL].                                                                                                                                                                                                                                                               |
**Table 24-33. MCSPI\_CH3CONF Register Field Descriptions (continued)**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                         |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--
| 22-21 | SPIENSLV | R/W  | 0h    | Channel 0 only and slave mode only: SPI slave select signal<br>detection.<br>Reserved bits (read returns 0) for other cases.<br>0h = Detection enabled only on SPIEN[0]<br>1h = Detection enabled only on SPIEN[1]<br>2h = Detection enabled only on SPIEN[2]                                                                                                                                                                       |
| 20    | FORCE    | R/W  | 0h    | 3h = Detection enabled only on SPIEN[3]<br>Manual SPIEN assertion to keep SPIEN active between SPI words<br>(single channel master mode only).<br>0h = Writing 0 into this bit drives low the SPIEN line when<br>MCSPI_CHCONF3[EPOL]=0, and drives it high when<br>MCSPI_CHCONF3[EPOL]=1.<br>1h = Writing 1 into this bit drives high the SPIEN line when<br>MCSPI_CHCONF3[EPOL]=0, and drives it low when<br>MCSPI_CHCONF3[EPOL]=1 |
| 19    | TURBO    | R/W  | 0h    | Turbo mode.<br>0h = Turbo is deactivated (recommended for single SPI word<br>transfer).<br>1h = Turbo is activated to maximize the throughput for multi-SPI<br>word transfers.                                                                                                                                                                                                                                                      |
| 18    | IS       | R/W  | 0h    | Input select<br>0h = Data line 0 (SPIDAT[0]) selected for reception.<br>1h = Data line 1 (SPIDAT[1]) selected for reception.                                                                                                                                                                                                                                                                                                        |
| 17    | DPE1     | R/W  | 0h    | Transmission enable for data line 1 (SPIDATAGZEN[1])<br>0h = Data line 1 (SPIDAT[1]) selected for transmission<br>1h = No transmission on data line 1 (SPIDAT[1])                                                                                                                                                                                                                                                                   |
| 16    | DPE0     | R/W  | 0h    | Transmission enable for data line 0 (SPIDATAGZEN[0])<br>0h = Data line 0 (SPIDAT[0]) selected for transmission<br>1h = No transmission on data line 0 (SPIDAT[0])                                                                                                                                                                                                                                                                   |
| 15    | DMAR     | R/W  | 0h    | DMA read request.<br>The DMA read request line is asserted when the channel is enabled<br>and new data is available in the receive register of the channel.<br>The DMA read request line is deasserted on read completion of the<br>receive register of the channel.<br>0h = DMA read request is disabled.<br>1h = DMA read request is enabled.                                                                                     |
| 14    | DMAW     | R/W  | 0h    | DMA write request.<br>The DMA write request line is asserted when the channel is enabled<br>and the MCSPI_TX3 register of the channel is empty.<br>The DMA write request line is deasserted on load completion of the<br>MCSPI_TX3 register of the channel.<br>0h = DMA write request is disabled.<br>1h = DMA write request is enabled.                                                                                            |
| 13-12 | TRM      | R/W  | 0h    | Transmit/receive modes.<br>0h = Transmit and receive mode<br>1h = Receive-only mode<br>2h = Transmit-only mode<br>3h = Reserved                                                                                                                                                                                                                                                                                                     |
**Table 24-33. MCSPI\_CH3CONF Register Field Descriptions (continued)**

| Bit  | Field | Type | Reset | Description                                      |
|------|-------|------|-------|--------------------------------------------------|
| 11-7 | WL    | R/W  | 0h    | SPI word length.                                 |
|      |       |      |       | 0h = Reserved                                    |
|      |       |      |       | 1h = Reserved                                    |
|      |       |      |       | 2h = Reserved                                    |
|      |       |      |       | 3h = The SPI word is 4-bits long.                |
|      |       |      |       | 4h = The SPI word is 5-bits long                 |
|      |       |      |       | 5h = The SPI word is 6-bits long                 |
|      |       |      |       | 6h = The SPI word is 7-bits long                 |
|      |       |      |       | 7h = The SPI word is 8-bits long                 |
|      |       |      |       | 8h = The SPI word is 9-bits long                 |
|      |       |      |       | 9h = The SPI word is 10-bits long                |
|      |       |      |       | Ah = The SPI word is 11-bits long                |
|      |       |      |       | Bh = The SPI word is 12-bits long                |
|      |       |      |       | Ch = The SPI word is 13-bits long                |
|      |       |      |       | Dh = The SPI word is 14-bits long                |
|      |       |      |       | Eh = The SPI word is 15-bits long                |
|      |       |      |       | Fh = The SPI word is 16-bits long                |
|      |       |      |       | 10h = The SPI word is 17-bits long               |
|      |       |      |       | 11h = The SPI word is 18-bits long               |
|      |       |      |       | 12h = The SPI word is 19-bits long               |
|      |       |      |       | 13h = The SPI word is 20-bits long               |
|      |       |      |       | 14h = The SPI word is 21-bits long               |
|      |       |      |       | 15h = The SPI word is 22-bits long               |
|      |       |      |       | 16h = The SPI word is 23-bits long               |
|      |       |      |       | 17h = The SPI word is 24-bits long               |
|      |       |      |       | 18h = The SPI word is 25-bits long               |
|      |       |      |       | 19h = The SPI word is 26-bits long               |
|      |       |      |       | 1Ah = The SPI word is 27-bits long               |
|      |       |      |       | 1Bh = The SPI word is 28-bits long               |
|      |       |      |       | 1Ch = The SPI word is 29-bits long               |
|      |       |      |       | 1Dh = The SPI word is 30-bits long               |
|      |       |      |       | 1Eh = The SPI word is 31-bits long               |
|      |       |      |       | 1Fh = The SPI word is 32-bits long               |
| 6    | EPOL  | R/W  | 0h    | SPIEN polarity                                   |
|      |       |      |       | 0h = SPIEN is held high during the active state. |
|      |       |      |       | 1h = SPIEN is held low during the active state.  |
### **Table 24-33. MCSPI\_CH3CONF Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
|-----|-------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 5-2 | CLKD  | R/W  | 0h    | Frequency divider for SPICLK (only when the module is a Master<br>SPI device).<br>A programmable clock divider divides the SPI reference clock<br>(CLKSPIREF) with a 4 bit value, and results in a new clock SPICLK<br>available to shift-in and shift-out data.<br>By default the clock divider ratio has a power of two granularity when<br>MCSPI_CHCONF[CLKG] is cleared, Otherwise this register is the 4LSB bit of a 12 bit register concatenated with clock divider extension<br>MCSPI_CHCTRL[EXTCLK] register.<br>The value description below defines the clock ratio when<br>MCSPI_CHCONF[CLKG] is cleared to 0.<br>0h = Divide by 1.<br>1h = Divide by 2.<br>2h = Divide by 4.<br>3h = Divide by 8.<br>4h = Divide by 16.<br>5h = Divide by 32.<br>6h = Divide by 64.<br>7h = Divide by 128.<br>8h = Divide by 256.<br>9h = Divide by 512.<br>Ah = Divide by 1024.<br>Bh = Divide by 2048.<br>Ch = Divide by 4096.<br>Dh = Divide by 8192.<br>Eh = Divide by 16384.<br>Fh = Divide by 32768. |
| 1   | POL   | R/W  | 0h    | SPICLK polarity<br>0h = SPICLK is held high during the active state<br>1h = SPICLK is held low during the active state                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 0   | PHA   | R/W  | 0h    | SPICLK phase<br>0h = Data are latched on odd numbered edges of SPICLK1h = Data are latched on even numbered edges of SPICLK                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
#### **24.4.1.24 MCSPI\_CH3STAT Register (offset = 16Ch) [reset = 0h]**

MCSPI\_CH3STAT is shown in [Figure](#page-83-1) 24-49 and described in Table [24-34.](#page-83-2)

The McSPI channel 3 status register (MCSPI\_CH3STAT) provides status information about the McSPI channel 3 FIFO transmit buffer register (MCSPI\_TX3) and the McSPI channel 3 FIFO receive buffer register (MCSPI\_RX3) of channel 3.

**Figure 24-49. MCSPI\_CH3STAT Register**

| 31       | 30       | 29    | 28       | 27    | 26   | 25   | 24
|----------|----------|-------|----------|-------|------|------|------|--|--
|          | RESERVED |       |          |       |      |      |
|          | R-0h     |       |          |       |      |      |
| 23       | 22       | 21    | 20       | 19    | 18   | 17   | 16
|          |          |       | RESERVED |       |      |      |
|          | R-0h     |       |          |       |      |      |
| 15       | 14       | 13    | 12       | 11    | 10   | 9    | 8
|          |          |       | RESERVED |       |      |      |
|          |          |       | R-0h     |       |      |      |
| 7        | 6        | 5     | 4        | 3     | 2    | 1    | 0
| RESERVED | RXFFF    | RXFFE | TXFFF    | TXFFE | EOT  | TXS  | RXS
| R-0h     | R-0h     | R-0h  | R-0h     | R-0h  | R-0h | R-0h | R-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-34. MCSPI\_CH3STAT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                    |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-7 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 6    | RXFFF    | R    | 0h    | Channel 3 FIFO receive buffer full status.<br>0h = FIFO receive buffer is not full.<br>1h = FIFO receive buffer is full.                                                                                                                                                                                                                                                                                                       |
| 5    | RXFFE    | R    | 0h    | Channel 3 FIFO receive buffer empty status.<br>0h = FIFO receive buffer is not empty.<br>1h = FIFO receive buffer is empty.                                                                                                                                                                                                                                                                                                    |
| 4    | TXFFF    | R    | 0h    | Channel 3 FIFO transmit buffer full status.<br>0h = FIFO transmit buffer is not full.<br>1h = FIFO transmit buffer is full.                                                                                                                                                                                                                                                                                                    |
| 3    | TXFFE    | R    | 0h    | Channel 3 FIFO transmit buffer empty status.<br>0h = FIFO transmit buffer is not empty.<br>1h = FIFO transmit buffer is empty.                                                                                                                                                                                                                                                                                                 |
| 2    | EOT      | R    | 0h    | Channel 3 end-of-transfer status.<br>The definitions of beginning and end of transfer vary with master<br>versus slave and the transfer format (transmit/receive mode, turbo<br>mode).<br>0h = This flag is automatically cleared when the shift register is<br>loaded with the data from the transmitter register (beginning of<br>transfer).<br>1h = This flag is automatically set to one at the end of an SPI<br>transfer. |
| 1    | TXS      | R    | 0h    | Channel 3 transmitter register status.<br>The bit is cleared when the host writes the most significant byte of<br>the SPI word in the MCSPI_TX3 register.<br>The bit is set when enabling the channel 3 , and also when the SPI<br>word is transferred from the MCSPI_TX3 register to the shift register.<br>0h = Register is full.<br>1h = Register is empty.                                                                 |
### **Table 24-34. MCSPI\_CH3STAT Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                      |
|-----|-------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0   | RXS   | R    | 0h    | Channel 3 receiver register status.<br>The bit is cleared when enabling the channel i, and also when the<br>host reads the most significant byte of the received SPI word from<br>the MCSPI_RX3 register.<br>The bit is set when the received SPI word is transferred from the<br>shift register to the MCSPI_RX3 register.<br>0h = Register is empty.<br>1h = Register is full. |
#### **24.4.1.25 MCSPI\_CH3CTRL Register (offset = 170h) [reset = 0h]**

MCSPI\_CH3CTRL is shown in [Figure](#page-85-1) 24-50 and described in Table [24-35](#page-85-2).

### **Figure 24-50. MCSPI\_CH3CTRL Register**

| 31 | 30 | 29 | 28 | 27     | 26 | 25 | 24 | 23       | 22 | 21 | 20       | 19 | 18 | 17 | 16        |
|----|----|----|----|--------|----|----|----|----------|----|----|----------|----|----|----|-----------|
|    |    |    |    |        |    |    |    | RESERVED |    |    |          |    |    |    |           |
|    |    |    |    |        |    |    |    | R-0h     |    |    |          |    |    |    |           |
| 15 | 14 | 13 | 12 | 11     | 10 | 9  | 8  | 7        | 6  | 5  | 4        | 3  | 2  | 1  | 0         |
|    |    |    |    | EXTCLK |    |    |    |          |    |    | RESERVED |    |    |    | EN        |
|    |    |    |    | R/W-0h |    |    |    |          |    |    | R-0h     |    |    |    | R/W0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 24-35. MCSPI\_CH3CTRL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                             |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                         |
| 15-8  | EXTCLK   | R/W  | 0h    | Clock ratio extension.<br>Used to concatenate with the CLKD bit field in MCSPI_CH3CONF<br>for clock ratio only when granularity is 1 clock cycle (CLKG bit in<br>MCSPI_CH3CONF set to 1).<br>Then the maximum value reached is a 4096 clock divider ratio.<br>0h = Clock ratio is CLKD + 11h = Clock ratio is CLKD + 1 + 16FFh = Clock ratio is CLKD + 1 + 4080 |
| 7-1   | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                         |
| 0     | EN       | R/W  | 0h    | Channel 3 enable.<br>0h = Channel 3 is not active.<br>1h = Channel 3 is active.                                                                                                                                                                                                                                                                                         |
#### **24.4.1.26 MCSPI\_TX3 Register (offset = 174h) [reset = 0h]**

MCSPI\_TX3 is shown in [Figure](#page-86-1) 24-51 and described in Table [24-36.](#page-86-2)

The McSPI channel FIFO transmit buffer register (MCSPI\_TXx) contains a single McSPI word to transmit through the serial link. Little endian host access SPI 8-bit word on 0; big endian host accesses on 3h. SPI words are transferred with MSB first. Refer to [Section](#page-37-0) 24.3.10 for more information.

**Figure 24-51. MCSPI\_TX3 Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-36. MCSPI\_TX3 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                 |
|------|-------|------|-------|-----------------------------|--|--|--
| 31-0 | TDATA | R/W  | 0h    | Channel 3 data to transmit. |
#### **24.4.1.27 MCSPI\_RX3 Register (offset = 178h) [reset = 0h]**

MCSPI\_RX3 is shown in [Figure](#page-87-1) 24-52 and described in Table [24-37.](#page-87-2)

The McSPI channel 3 FIFO receive buffer register (MCSPI\_RX3) contains a single McSPI word received through the serial link. Little endian host access SPI 8 bit word on 0; big endian host accesses on 3h. Refer to [Section](#page-37-0) 24.3.10 for more information.

### **Figure 24-52. MCSPI\_RX3 Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-37. MCSPI\_RX3 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description              |
|------|-------|------|-------|--------------------------|--|--|--
| 31-0 | RDATA | R    | 0h    | Channel 3 received data. |
#### **24.4.1.28 MCSPI\_XFERLEVEL Register (offset = 17Ch) [reset = 0h]**

MCSPI\_XFERLEVEL is shown in [Figure](#page-88-1) 24-53 and described in Table [24-38](#page-88-2).

The McSPI transfer levels register (MCSPI\_XFERLEVEL) provides the transfer levels needed while using the FIFO buffer during transfer.

**Figure 24-53. MCSPI\_XFERLEVEL Register**

| 31 | 30     | 29 | 28 | 27 | 26 | 25 | 24 | 23   | 22 | 21 | 20     | 19 | 18 | 17 | 16 | 15     | 14 | 13 | 12  | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4   | 3 | 2 | 1 | 0 |
|----|--------|----|----|----|----|----|----|------|----|----|--------|----|----|----|----|--------|----|----|-----|----|----|---|---|---|---|---|-----|---|---|---|---|
|    |        |    |    |    |    |    |    | WCNT |    |    |        |    |    |    |    |        |    |    | AFL |    |    |   |   |   |   |   | AEL |   |   |   |   |
|    | R/W-0h |    |    |    |    |    |    |      |    |    | R/W-0h |    |    |    |    | R/W-0h |    |    |     |    |    |   |   |   |   |   |     |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-38. MCSPI\_XFERLEVEL Register Field Descriptions**

| Bit   | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                   |
|-------|-------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | WCNT  | R/W  | 0h    | SPI word counter.<br>Holds the programmable value of the number of SPI words to be<br>transferred on the channel that is using the FIFO buffer.<br>When the transfer has started, a read back of this register returns<br>the current SPI word transfer index.<br>0h = Counter not used<br>1h = 1 SPI word<br>FFFEh = 65534 SPI word<br>FFFFh = 65535 SPI word                                |
| 15-8  | AFL   | R/W  | 0h    | Buffer almost full.<br>Holds the programmable almost full level value used to determine<br>almost full buffer condition.<br>If you want an interrupt or a DMA read request to be issued during a<br>receive operation when the data buffer holds at least n bytes, then<br>the buffer MCSPI_MODULCTRL[AFL] must be set with n - 1.<br>0h = 1 byte<br>1h = 2 bytes<br>FFh = 256 bytes          |
| 7-0   | AEL   | R/W  | 0h    | Buffer almost empty.<br>Holds the programmable almost empty level value used to determine<br>almost empty buffer condition.<br>If you want an interrupt or a DMA write request to be issued during a<br>transmit operation when the data buffer is able to receive n bytes,<br>then the buffer MCSPI_MODULCTRL[AEL] must be set with n - 1.<br>0h = 1 byte<br>1h = 2 bytes<br>FFh = 256 bytes |
#### **24.4.1.29 MCSPI\_DAFTX Register (offset = 180h) [reset = 0h]**

MCSPI\_DAFTX is shown in [Figure](#page-89-1) 24-54 and described in Table [24-39](#page-89-2).

The McSPI DMA address aligned FIFO transmitter register (MCSPI\_DAFTX) contains the SPI words to transmit on the serial link when FIFO is used and the DMA address is aligned on 256 bit. This register is an image of one of the MCSPI\_TX(i) registers corresponding to the channel which have its FIFO enabled. The SPI words are transferred with MSB first. See Chapter Access to data registers for the list of supported accesses.

**Figure 24-54. MCSPI\_DAFTX Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-39. MCSPI\_DAFTX Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                     |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | DAFTDATA | R/W  | 0h    | FIFO Data to transmit with DMA 256 bit aligned address.<br>This register is used only when MCSPI_MODULCTRL[FDAA] is set<br>to 1, and only one of the MCSPI_CH(i)CONF[FFEW] of enabled<br>channels is set.<br>Without these conditions, any access to this register will return a null<br>value. |
#### **24.4.1.30 MCSPI\_DAFRX Register (offset = 1A0h) [reset = 0h]**

MCSPI\_DAFRX is shown in [Figure](#page-90-1) 24-55 and described in Table [24-40](#page-90-2).

The McSPI DMA address aligned FIFO receiver register (MCSPI\_DAFRX) contains the SPI words to received on the serial link when FIFO used and DMA address is aligned on 256 bit. This register is an image of one of MCSPI\_RX(i) register corresponding to the channel which have its FIFO enabled.

**Figure 24-55. MCSPI\_DAFRX Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 24-40. MCSPI\_DAFRX Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                  |
|------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | DAFRDATA | R    | 0h    | FIFO Received Data with DMA 256 bit aligned address.<br>This register is used only when MCSPI_MODULCTRL[FDAA] is set<br>to 1, and only one of the MCSPI_CH(i)CONF[FFER] of enabled<br>channels is set.<br>Without these conditions, any access to this register will return a null<br>value. |
