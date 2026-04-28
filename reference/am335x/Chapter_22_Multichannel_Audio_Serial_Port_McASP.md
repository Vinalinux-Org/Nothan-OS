# 22 Multichannel Audio Serial Port (McASP)

This chapter describes the McASP of the device.

**Topic** ........................................................................................................................... **Page**

| 22.1 | Introduction           | 4653 |
|------|------------------------|------|
| 22.2 | Integration            | 4655 |
| 22.3 | Functional Description | 4657 |
| 22.4 | Registers              | 4710 |
|      |                        |      |
## **22.1 Introduction**

### *22.1.1 Purpose of the Peripheral*

The multichannel audio serial port (McASP) functions as a general-purpose audio serial port optimized for the needs of multichannel audio applications. The McASP is useful for time-division multiplexed (TDM) stream, Inter-Integrated Sound (I2S) protocols, and intercomponent digital audio interface transmission (DIT). The McASP consists of transmit and receive sections that may operate synchronized, or completely independently with separate master clocks, bit clocks, and frame syncs, and using different transmit modes with different bit-stream formats. The McASP module also includes serializers that can be individually enabled to either transmit or receive.

### *22.1.2 Features*

Features of the McASP include:

- Two independent clock generator modules for transmit and receive.
  - Clocking flexibility allows the McASP to receive and transmit at different rates. For example, the McASP can receive data at 48 kHz but output up-sampled data at 96 kHz or 192 kHz.
- Independent transmit and receive modules, each includes:
  - Programmable clock and frame sync generator.
  - TDM streams from 2 to 32, and 384 time slots.
  - Support for time slot sizes of 8, 12, 16, 20, 24, 28, and 32 bits.
  - Data formatter for bit manipulation.
- Individually assignable serial data pins (up to 6 pins).
- Glueless connection to audio analog-to-digital converters (ADC), digital-to-analog converters (DAC), codec, digital audio interface receiver (DIR), and S/PDIF transmit physical layer components.
- Wide variety of I2S and similar bit-stream format.
- Integrated digital audio interface transmitter (DIT) supports (up to 10 transmit pins):
  - S/PDIF, IEC60958-1, AES-3 formats.
  - Enhanced channel status/user data RAM.
- 384-slot TDM with external digital audio interface receiver (DIR) device.
  - For DIR reception, an external DIR receiver integrated circuit should be used with I2S output format and connected to the McASP receive section.
- Extensive error checking and recovery.
  - Transmit underruns and receiver overruns due to the system not meeting real-time requirements.
  - Early or late frame sync in TDM mode.
  - Out-of-range high-frequency master clock for both transmit and receive.
  - External error signal coming into the AMUTEIN input.
  - DMA error due to incorrect programming.
### *22.1.3 Protocols Supported*

The McASP supports a wide variety of protocols.

- Transmit section supports:
  - Wide variety of I2S and similar bit-stream formats.
  - TDM streams from 2 to 32 time slots.
  - S/PDIF, IEC60958-1, AES-3 formats.
- Receive section supports:
  - Wide variety of I2S and similar bit-stream formats.
  - TDM streams from 2 to 32 time slots.
  - TDM stream of 384 time slots specifically designed for easy interface to external digital interface receiver (DIR) device transmitting DIR frames to McASP using the I2S protocol (one time slot for each DIR subframe).

The transmit and receive sections may each be individually programmed to support the following options on the basic serial protocol:

- Programmable clock and frame sync polarity (rising or falling edge): ACLKR/X, AHCLKR/X, and AFSR/X.
- Slot length (number of bits per time slot): 8, 12, 16, 20, 24, 28, 32 bits supported.
- Word length (bits per word): 8, 12, 16, 20, 24, 28, 32 bits; always less than or equal to the time slot length.
- First-bit data delay: 0, 1, 2 bit clocks.
- Left/right alignment of word inside slot.
- Bit order: MSB first or LSB first.
- Bit mask/pad/rotate function.
  - Automatically aligns data internally in either Q31 or integer formats.
  - Automatically masks nonsignificant bits (sets to 0, 1, or extends value of another bit).

In DIT mode for McASP, additional features of the transmitter are:

- Transmit-only mode 384 time slots (subframe) per frame.
- Bi-phase encoded 3.3 V output.
- Support for consumer and professional applications.
- Channel status RAM (384 bits).
- User data RAM (384 bits).
- Separate valid bit (V) for subframe A, B.

In I2S mode, the transmit and receive sections can support simultaneous transfers on up to all serial data pins operating as 192 kHz stereo channels.

In DIT mode, the transmitter can support a 192 kHz frame rate (stereo) on up to all serial data pins simultaneously (note that the internal bit clock for DIT runs two times faster than the equivalent bit clock for I2S mode, due to the need to generate Biphase Mark Encoded Data).

### *22.1.4 Unsupported McASP Features*

The unsupported McASP features in this device include the following.

**Table 22-1. Unsupported McASP Features**

| Feature  | Reason                     |
|----------|----------------------------|
| AXR[9:4] | Signals are not pinned out |
| AMUTE    | Not connected              |
| AMUTEIN  | Not connected              |
## 22.2 Integration

The device contains two instantiations of the McASP subsystem: McASP0 and McASP1. The McASP subsystem includes a McASP peripheral, and transmit/receive buffers.

Each McASP is configured with four serializers.

McASP McASPx Pads Subsystem AHCLKX MCAx AHCLK MCAx\_ACLKX L3 Slow ACLKX DMA/Data AFSX MCAx\_AFSX Interconnect L4 Peripheral AHCLKR MCAx\_AHCLKR CFG Interface ACLKR Interconnect MCAx ACLKR AFSR MCAx\_AFSR AXRO MCAx\_AXR0 MPU Subsystem AXR1 x\_intr\_pend MCAx\_AXR1 and PRU-ICSS AXR2 MCAx\_AXR2 r\_intr\_pend Interrupts AXR3 MCAx\_AXR3 AXR[9:4] AXEVT0 xevent drea **EDMA** AREVT0 revent\_dreq **AMUTE PRCM** AMUTEIN MCASP\_FCLK CLK\_M\_OSC aux\_clk

Figure 22-1. McASP0-1 Integration

### 22.2.1 McASP Connectivity Attributes

The general connectivity attributes for the McASP modules are summarized in Table 22-2

Attributes Type Power Domain Peripheral Domain Clock Domain PD\_PER\_L3S\_GCLK (OCP Clock) PD\_PER\_MCASP\_FCLK (Aux Clock) Reset Signals PER DOM RST N Idle/Wakeup Signals Smart Idle Interrupt Requests 1 Transmit Interrupt per instance x\_intr\_pend - to MPU Subsystem (MCATXINTx) and PRU-ICSS (mcasp\_x\_intr\_pend) 1 Receive Interrupt r\_intr\_pend - to MPU Subsystem (MCARXINTx) and PRU-ICSS (mcasp\_r\_intr\_pend) 2 DMA requests per instance to EDMA (Transmit: AXEVTx, DMA Requests Receive: AREVTx) Physical Address L3 Slow slave port (data) L4 Peripheral slave port (CFG)

Table 22-2. McASP Connectivity Attributes
### *22.2.2 McASP Clock and Reset Management*

The McASP module uses functional clocks either generated internally (master mode) or supplied from its serial interface (slave mode). The internal interconnect interface clock is used for the module internal OCP interface. Internal registers select the source of the functional clocks and the applied divider ratio.

**Table 22-3. McASP Clock Signals**

| Clock Signal               | Max Freq  | Reference / Source | Comments                       |
|----------------------------|-----------|--------------------|--------------------------------|
| ocp_clk<br>Interface clock | 100 MHz   | CORE_CLKOUTM4 / 2  | pd_per_l3s_gclk<br>From PRCM   |
| auxclk<br>Functional clock | 26 MHz(1) | CLK_M_OSC          | pd_per_mcasp_fclk<br>From PRCM |

<sup>(1)</sup> The maximum frequency may change in certain low power modes. For more timing information for different operating performance points (OPPs), see the device datasheet *AM335x Sitara Processors* (literature number [SPRS717\)](http://www.ti.com/lit/pdf/SPRS717).

### *22.2.3 McASP Pin List*

The McASP external interface signals are shown in [Table](#page-4-0) 22-4.

### **Table 22-4. McASP Pin List**

| Pin              | Type | Description                |
|------------------|------|----------------------------|
| McASPx_AXR[3:0]  | I/O  | Audio transmit/receive pin |
| McASPx_ACLKX(1)  | I/O  | Transmit clock             |
| McASPx_FSX(1)    | I/O  | Frame synch for transmit   |
| McASPx_AHCLKX(1) | I/O  | High speed transmit clock  |
| McASPx_ACLKR(1)  | I/O  | Receive clock              |
| McASPx_FSR(1)    | I/O  | Frame synch for receive    |
| McASPx_AHCLKR(1) | I/O  | High speed receive clock   |

<sup>(1)</sup> These signals are also used as inputs to re-time or sync data. The associated CONF\_<module>\_<pin>\_RXACTIVE bit for these signals must be set to 1 to enable the inputs back to the module. It is also recommended to place a 33-ohm resistor in series (close to the processor) on each of these signals to avoid signal reflections.
## **22.3 Functional Description**

### *22.3.1 Overview*

[Figure](#page-6-0) 22-2 shows the major blocks of the McASP. The McASP has independent receive/transmit clock generators and frame sync generators, error-checking logic, and up to four serial data pins. See the device-specific data manual for the number of data pins available on your device.

All the McASP pins on the device may be individually programmed as general-purpose I/O (GPIO) if they are not used for serial port functions.

The McASP includes the following pins:

- Serializers;
  - Data pins AXRn: Up to four pins.
- Transmit clock generator:
  - AHCLKX: McASP transmit high-frequency master clock.
  - ACLKX: McASP transmit bit clock.
- Transmit Frame Sync Generator;
  - AFSX: McASP transmit frame sync or left/right clock (LRCLK).
- Receive clock generator;
  - AHCLKR: McASP receive high-frequency master clock.
  - ACLKR: McASP receive bit clock.
- Receive Frame Sync Generator;
  - AFSR: McASP receive frame sync or left/right clock (LRCLK).
- Mute in/out;
  - AMUTEIN: McASP mute input (from external device).
  - AMUTE: McASP mute output.
  - Data pins AXRn.
### 22.3.2 Functional Block Diagram

Figure 22-2 shows the major blocks of the McASP. The McASP has independent receive/transmit clock generators and frame sync generators.


- A McASP has 6 serial data pins.
- B AMUTEIN is not a dedicated McASP pin, but typically comes from one of the external interrupt pins.
#### **22.3.2.1 System Level Connections**

[Figure](#page-7-0) 22-3 through [Figure](#page-8-0) 22-7 show examples of McASP usage in digital audio encoder/decoder systems.


**Figure 22-3. McASP to Parallel 2-Channel DACs**


DVD player Stereo I2S **PWM** Digital Coaxial/ generator amp optical S/PDIF encoded **PWM** Digital McASP generator amp I2S DIR TX **PWM** Digital generator amp **PWM** Digital generator amp

Figure 22-5. McASP to Digital Amplifier

Figure 22-6. McASP as Digital Audio Encoder


Figure 22-7. McASP as 16 Channel Digital Processor

### *22.3.3 Industry Standard Compliance Statement*

The McASP supports the following industry standard interfaces.

#### **22.3.3.1 TDM Format**

The McASP transmitter and receiver support the multichannel, synchronous time-division-multiplexed (TDM) format via the TDM transfer mode. Within this transfer mode, a wide variety of serial data formats are supported, including formats compatible with devices using the Inter-Integrated Sound (I2S) protocol. This section briefly discusses the TDM format and the I2S protocol.

#### *22.3.3.1.1 TDM Format*

The TDM format is typically used when communicating between integrated circuit devices on the same printed circuit board or on another printed circuit board within the same piece of equipment. For example, the TDM format is used to transfer data between the processor and one or more analog-to-digital converter (ADC), digital-to-analog converter (DAC), or S/PDIF receiver (DIR) devices.

The TDM format consists of three components in a basic synchronous serial transfer: the clock, the data, and the frame sync. In a TDM transfer, all data bits (AXRn) are synchronous to the serial clock (ACLKX or ACLKR). The data bits are grouped into words and slots (as defined in [Section](#page-13-0) 22.3.4). The "slots" are also commonly referred to as "time slots" or "channels" in TDM terminology. A frame consists of multiple slots (or channels). Each TDM frame is defined by the frame sync signal (AFSX or AFSR). Data transfer is continuous and periodic, since the TDM format is most commonly used to communicate with data converters that operate at a fixed sample rate.

There are no delays between slots. The last bit of slot N is followed immediately on the next serial clock cycle with the first bit of slot N + 1, and the last bit of the last slot is followed immediately on the next serial clock with the first bit of the first slot. However, the frame sync may be offset from the first bit of the first slot with a 0, 1, or 2-cycle delay.

It is required that the transmitter and receiver in the system agree on the number of bits per slot, since the determination of a slot boundary is not made by the frame sync signal (although the frame sync marks the beginning of slot 0 and the beginning of a new frame).

[Figure](#page-9-0) 22-8 shows the TDM format. [Figure](#page-10-0) 22-9 shows the different bit delays from the frame sync.


**Figure 22-8. TDM Format–6 Channel TDM Example**

A FS duration of slot is shown. FS duration of single bit is also supported.

- A FS duration of slot is shown. FS duration of single bit is also supported.
- B Last bit of last slot of previous frame. No gap between this bit and the first bit of slot 0 is allowed.

In a typical audio system, one frame of data is transferred during each data converter sample period  $f_s$ . To support multiple channels, the choices are to either include more time slots per frame (thus operating with a higher bit clock rate), or to use additional data pins to transfer the same number of channels (thus operating with a slower bit clock rate).

For example, a particular six channel DAC may be designed to transfer over a single serial data pin AXRn as shown in Figure 22-8. In this case the serial clock must run fast enough to transfer a total of 6 channels within each frame period. Alternatively, a similar six channel DAC may be designed to use three serial data pins AXR[0,1,2], transferring two channels of data on each pin during each sample period. In the latter case, if the sample period remains the same, the serial clock can run three times slower than the former case. The McASP is flexible enough to support either type of DAC.

#### 22.3.3.1.2 Inter-Integrated Sound (I2S) Format

The inter-integrated sound (I2S) format is used extensively in audio interfaces. The TDM transfer mode of the McASP supports the I2S format when configured to 2 slots per frame.

I2S format is specifically designed to transfer a stereo channel (left and right) over a single data pin AXRn. "Slots" are also commonly referred to as "channels". The frame width duration in the I2S format is the same as the slot size. The frame signal is also referred to as "word select" in the I2S format. Figure 22-10 shows the I2S protocol.

The McASP supports transfer of multiple stereo channels over multiple AXRn pins.


A 1 to 6 data pins may be supported.
#### 22.3.3.2 S/PDIF Coding Format

The McASP transmitter supports the S/PDIF format with 3.3V biphase-mark encoded output. The S/PDIF format is supported by the digital audio interface transmit (DIT) transfer mode of the McASP. This section briefly discusses the S/PDIF coding format.

#### 22.3.3.2.1 Biphase-Mark Code (BMC)

In S/PDIF format, the digital signal is coded using the biphase-mark code (BMC). The clock, frame, and data are embedded in only one signal—the data pin AXRn. In the BMC system, each data bit is encoded into two logical states (00, 01, 10, or 11) at the pin. These two logical states form a cell. The duration of the cell, which equals to the duration of the data bit, is called a time interval. A logical 1 is represented by two transitions of the signal within a time interval, which corresponds to a cell with logical states 01 or 10. A logical 0 is represented by one transition within a time interval, which corresponds to a cell with logical states 00 or 11. In addition, the logical level at the start of a cell is inverted from the level at the end of the previous cell. Figure 22-11 and Table 22-5 show how data is encoded to the BMC format.

As shown in Figure 22-11, the frequency of the clock is twice the unencoded data bit rate. In addition, the clock is always programmed to  $128 \times f_s$ , where  $f_s$  is the sample rate (see Section 22.3.3.2.3 for details on how this clock rate is derived based on the S/PDIF format). The device receiving in S/PDIF format can recover the clock and frame information from the BMC signal.


Figure 22-11. Biphase-Mark Code (BMC)

Table 22-5. Biphase-Mark Encoder

| Data (Unencoded) | Previous State at Pin<br>AXRn | BMC-Encoded Cell Output at AXRn |
|------------------|-------------------------------|---------------------------------|
| 0                | 0                             | 11                              |
| 0                | 1                             | 00                              |
| 1                | 0                             | 10                              |
| 1                | 1                             | 01                              |
#### *22.3.3.2.2 Subframe Format*

Every audio sample transmitted in a subframe consists of 32 S/PDIF time intervals (or cells), numbered from 0 to 31. [Figure](#page-12-0) 22-12 shows a subframe.

- **Time intervals 0-3** carry one of the three permitted preambles to signify the type of audio sample in the current subframe. The preamble is *not* encoded in BMC format, and therefore the preamble code can contain more than two consecutive 0 or 1 logical states in a row. See [Table](#page-12-1) 22-6.
- **Time intervals 4-27** carry the audio sample word in linear 2s-complement representation. The mostsignificant bit (MSB) is carried by time interval 27. When a 24-bit coding range is used, the leastsignificant bit (LSB) is in time interval 4. When a 20-bit coding range is used, time intervals 8-27 carry the audio sample word with the LSB in time interval 8. Time intervals 4-7 may be used for other applications and are designated auxiliary sample bits.
- If the source provides fewer bits than the interface allows (either 20 or 24), the unused LSBs are set to logical 0. For a nonlinear PCM audio application or a data application, the main data field may carry any other information.
- **Time interval 28** carries the validity bit (V) associated with the main data field in the subframe.
- **Time interval 29** carries the user data channel (U) associated with the main data field in the subframe.
- **Time interval 30** carries the channel status information (C) associated with the main data field in the subframe. The channel status indicates if the data in the subframe is digital audio or some other type of data.
- • **Time interval 31** carries a parity bit (P) such that time intervals 4-31 carry an even number of 1s and an even number of 0s (even parity). As shown in [Table](#page-12-1) 22-6, the preambles (time intervals 0-3) are also defined with even parity.


**Figure 22-12. S/PDIF Subframe Format**

**Table 22-6. Preamble Codes**

| Preamble Code(1) | Previous Logical State | Logical States on pin AXRn(2) | Description                     |
|------------------|------------------------|-------------------------------|---------------------------------|
| B (or Z)         | 0                      | 1110 1000                     | Start of a block and subframe 1 |
| M (or X)         | 0                      | 1110 0010                     | Subframe 1                      |
| W (or Y)         | 0                      | 1110 0100                     | Subframe 2                      |

<sup>(1)</sup> Historically, preamble codes are referred to as B, M, W. For use in professional applications, preambles are referred to as Z, X, Y, respectively.

As shown in [Table](#page-12-1) 22-6, the McASP DIT only generates one polarity of preambles and it assumes the previous logical state to be 0. This is because the McASP assures an even-polarity encoding scheme when transmitting in DIT mode. If an underrun condition occurs, the DIT resynchronizes to the correct logic level on the AXRn pin before continuing with the next transmission.

<sup>(2)</sup> The preamble is not BMC encoded. Each logical state is synchronized to the serial clock. These 8 logical states make up time slots (cells) 0 to 3 in the S/PDIF stream.
#### 22.3.3.2.3 Frame Format

An S/PDIF frame is composed of two subframes (Figure 22-13). For linear coded audio applications, the rate of frame transmission normally corresponds exactly to the source sampling frequency  $f_s$ . The S/PDIF format clock rate is therefore 128 ×  $f_s$  (128 = 32 cells/subframe × 2 clocks/cell × 2 subframes/sample). For example, for an S/PDIF stream at a 192 kHz sampling frequency, the serial clock is 128 × 192 kHz = 24.58 MHz.

In 2-channel operation mode, the samples taken from both channels are transmitted by time multiplexing in consecutive subframes. Both subframes contain valid data. The first subframe (**left** or **A** channel in stereophonic operation and **primary** channel in monophonic operation) normally starts with preamble M. However, the preamble of the first subframe changes to preamble B once every 192 frames to identify the start of the block structure used to organize the channel status information. The second subframe (**right** or **B** channel in stereophonic operation and **secondary** channel in monophonic operation) always starts with preamble W.

In single-channel operation mode in a professional application, the frame format is the same as in the 2-channel mode. Data is carried in the first subframe and may be duplicated in the second subframe. If the second subframe is not carrying duplicate data, cell 28 (validity bit) is set to logical 1.


Figure 22-13. S/PDIF Frame Format

### 22.3.4 Definition of Terms

Slot

The serial bit stream transmitted or received by the McASP is a long sequence of 1s and 0s, either output or input on one of the audio transmit/receive pins (AXRn). However, the sequence has a hierarchical organization that can be described in terms of frames of data, slots, words, and bits.

A basic synchronous serial interface consists of three important components: clock, frame sync, and data. Figure 22-14 shows two of the three basic components—the clock (ACLK) and the data (AXRn). Figure 22-14 does not specify whether the clock is for transmit (ACLKX) or receive (ACLKR) because the definitions of terms apply to both receive and transmit interfaces. In operation, the transmitter uses ACLKX as the serial clock, and the receiver uses ACLKR as the serial clock. Optionally, the receiver can use ACLKX as the serial clock when the transmitter and receiver of the McASP are configured to operate synchronously.

Bit A bit is the smallest entity in the serial data stream. The beginning and end of each bit is marked by an edge of the serial clock. The duration of a bit is a serial clock period. A 1 is represented by a logic high on the AXRn pin for the entire duration of the bit. A 0 is represented by a logic low on the AXRn pin for the entire duration of the bit.

Word A word is a group of bits that make up the data being transferred between the processor and the external device. Figure 22-14 shows an 8-bit word.

A slot consists of the bits that make up the word, and may consist of additional bits used to pad the word to a convenient number of bits for the interface between the processor and the external device. In Figure 22-14, the audio data consists of only 8 bits of useful data (8-bit word), but it is padded with 4 zeros (12-bit slot) to satisfy the desired protocol in interfacing to an external device. Within a slot, the bits may be shifted in/out of the McASP on the AXRn pin either MSB or LSB first. When the word size is smaller than the slot size, the word may be aligned to the left (beginning) of the slot or to the right (end) of the slot. The additional bits in the slot not belonging to the word may be padded with 0, 1, or with one of the bits (the MSB or the LSB typically) from the data word. These options are shown in Figure 22-15.
Figure 22-14. Definition of Bit, Word, and Slot


- (1) b7:b0 bits. Bits b7 to b0 form a word
- (2) P pad bits. Bits b7 to b0, together with the four pad bits, form a slot.
- (3) In this example, the data is transmitted MSB first, left aligned.

Figure 22-15. Bit Order and Word Alignment Within a Slot Examples

|     |                                 |   |   |   |   |      | •     | Tim      | ie |   |    |    |     |
|-----|---------------------------------|---|---|---|---|------|-------|----------|----|---|----|----|-----|
| Bit | 0                               | 1 | 2 | 3 | 4 | 5    | 6     | 7        | 8  | 9 | 10 | 11 |     |
|     | 1                               | 0 | 0 | 0 | 0 | 1    | 1     | 1        | 0  | 0 | 0  | 0  | (a) |
|     | 0                               | 1 | 2 | 3 | 4 | 5    | 6     | 7        | 8  | 9 | 10 | 11 |     |
|     | 0                               | 0 | 0 | 0 | 1 | 0    | 0     | 0        | 0  | 1 | 1  | 1  | (b) |
|     | 0                               | 1 | 2 | 3 | 4 | 5    | 6     | 7        | 8  | 9 | 10 | 11 |     |
|     | 1                               | 1 | 1 | 0 | 0 | 0    | 0     | 1        | 0  | 0 | 0  | 0  | (c) |
|     | 0                               | 1 | 2 | 3 | 4 | 5    | 6     | 7        | 8  | 9 | 10 | 11 | , , |
|     | 0                               | 0 | 0 | 0 | 1 | 1    | 1     | 0        | 0  | 0 | 0  | 1  | (d) |
|     |                                 |   |   |   | l | l    |       |          |    |   |    |    | ()  |
|     | 0                               | 1 | 2 | 3 | 4 | 5    | 6     | 7        | 8  | 9 | 10 | 11 | (0) |
|     | I                               | U | U | 0 | U | ı    |       | <u> </u> |    | ı | I  | ı  | (e) |
|     | 0                               | 1 | 2 | 3 | 4 | 5    | 6     | 7        | 8  | 9 | 10 | 11 |     |
|     | 1                               | 1 | 1 | 1 | 1 | 0    | 0     | 0        | 0  | 1 | 1  | 1  | (f) |
|     | 0                               | 1 | 2 | 3 | 4 | 5    | 6     | 7        | 8  | 9 | 10 | 11 |     |
|     | 1                               | 1 | 1 | 0 | 0 | 0    | 0     | 1        | 1  | 1 | 1  | 1  | (g) |
|     | 0                               | 1 | 2 | 3 | 4 | 5    | 6     | 7        | 8  | 9 | 10 | 11 |     |
|     | 1                               | 1 | 1 | 1 | 1 | 1    | 1     | 0        | 0  | 0 | 0  | 1  | (h) |
|     | 0                               | 1 | 2 | 3 | 4 | 5    | 6     | 7        | 8  | 9 | 10 | 11 |     |
|     | 1                               | 1 | 1 | 0 | 0 | 0    | 0     | 0        | 0  | 0 | 0  | 0  | (i) |
|     |                                 |   |   |   |   |      |       |          |    |   |    |    | (-) |
|     | 0                               | 1 | 2 | 3 | 4 | 5    | 6     | 7        | 8  | 9 | 10 |    | (1) |
|     | 0                               | 0 | 0 | 0 | 0 | 1    | 1     | 0        | 0  | 0 | 0  | 1  | (j) |
|     | 8-bit word                      |   |   |   |   |      |       |          |    |   |    |    |     |
|     | S-bit word                      |   |   |   |   |      |       |          |    |   |    |    |     |
|     |                                 |   |   |   | 1 | 2-bi | it sl | ot       |    |   |    |    |     |
| 1   | 1 Unshaded: bit belongs to word |   |   |   |   |      |       |          |    | 1 |    |    |     |

- (a) 87h as 8-bit word, 12-bit slot, left align, MSB first, pad zeros
- (b) 87h as 8-bit word, 12-bit slot, right align, MSB first, pad zeros
- (c) 87h as 8-bit word, 12-bit slot, left align, LSB first, pad zeros
- (d) 87h as 8-bit word, 12-bit slot, right align, LSB first, pad zeros
- (e) 87h as 8-bit word, 12-bit slot, left align, MSB first, pad with bit 7
- (f) 87h as 8-bit word, 12-bit slot, right align, MSB first, pad with bit 4
- (g) 87h as 8-bit word, 12-bit slot, left align, LSB first, pad with bit 7
- (h) 87h as 8-bit word, 12-bit slot, right align, LSB first, pad with bit 4
- (i) 07h as 8-bit word, 12-bit slot, left align, LSB first, pad with bit 7
- 86h as 8-bit word, 12-bit slot, right align, LSB first, pad with bit 4
- 1 Shaded: bit is a pad bit
The third basic element of a synchronous serial interface is the frame synchronization signal, also referred to as frame sync in this document.

**Frame** A frame contains one or multiple slots, as determined by the desired protocol. [Figure](#page-15-0) 22-16 shows an example frame of data and the frame definitions. [Figure](#page-15-0) 22-16 does not specify whether the frame sync (FS) is for transmit (AFSX) or receive (AFSR) because the definitions of terms apply to both receive and transmit interfaces. In operation, the transmitter uses AFSX and the receiver uses AFSR. Optionally, the receiver can use AFSX as the frame sync when the transmitter and receiver of the McASP are configured to operate synchronously.

This section only shows the generic definition of the frame sync. See [Section](#page-9-1) 22.3.3 and Section [22.3.8.1](#page-20-0) for details on the frame sync formats required for the different transfer modes and protocols (burst mode, TDM mode and I2S format, DIT mode and S/PDIF format).

Slot 0 Slot 1 Frame sync width FS AXRn

Frame

**Figure 22-16. Definition of Frame and Frame Sync Width**

(1) In this example, there are two slots in a frame, and FS duration of slot length is shown.

Slot

Other terms used throughout the document:

| TDM                  | Time-division multiplexed. See Section 22.3.3.1 for details on the TDM protocol.                                                                                        |
|----------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| DIR                  | Digital audio interface receive. The McASP does not natively support receiving in the S/PDIF format. The<br>McASP supports I2S format output by an external DIR device. |
| DIT                  | Digital audio interface transmit. The McASP supports transmitting in S/PDIF format on up to all data pins<br>configured as outputs.                                     |
| I2S                  | Inter-Integrated Sound protocol, commonly used on audio interfaces. The McASP supports the I2S protocol as<br>part of the TDM mode (when configured as a 2-slot frame). |
| Slot or<br>Time Slot | For TDM format, the term time slot is interchangeable with the term slot defined in this section. For DIT format, a<br>McASP time slot corresponds to a DIT subframe.   |

### *22.3.5 Clock and Frame Sync Generators*

The McASP clock generators are able to produce two independent clock zones: transmit and receive clock zones. The serial clock generators may be programmed independently for the transmit section and the receive section, and may be completely asynchronous to each other. The serial clock (clock at the bit rate) may be sourced:

- Internally by passing through two clock dividers off the internal clock source (AUXCLK).
- Externally directly from ACLKR/X pin.
- Mixed an external high-frequency clock is input to the McASP on either the AHCLKX or AHCLKR pins, and divided down to produce the bit rate clock.

In the internal/mixed cases, the bit rate clock is generated internally and should be driven out on the ACLKX (for transmit) or ACLKR (for receive) pins. In the internal case, an internally-generated highfrequency clock may be driven out onto the AHCLKX or AHCLKR pins to serve as a reference clock for other components in the system.

The McASP requires a minimum of a bit clock and a frame sync to operate, and provides the capability to reference these clocks from an external high-frequency master clock. In DIT mode, it is possible to use only internally-generated clocks and frame syncs.
#### **22.3.5.1 Transmit Clock**

The transmit bit clock, ACLKX, ([Figure](#page-16-0) 22-17) may be either externally sourced from the ACLKX pin or internally generated, as selected by the CLKXM bit. If internally generated (CLKXM = 1), the clock is divided down by a programmable bit clock divider (CLKXDIV) from the transmit high-frequency master clock (AHCLKX).

Internally, the McASP always shifts transmit data at the rising edge of the internal transmit clock, XCLK, [\(Figure](#page-16-0) 22-17). The CLKXP mux determines if ACLKX needs to be inverted to become XCLK. If CLKXP = 0, the CLKXP mux directly passes ACLKX to XCLK. As a result, the McASP shifts transmit data at the rising edge of ACLKX. If CLKXP = 1, the CLKX mux passes the inverted version of ACLKX to XCLK. As a result, the McASP shifts transmit data at the falling edge of ACLKX.

The transmit high-frequency master clock, AHCLKX, may be either externally sourced from the AHCLKX pin or internally generated, as selected by the HCLKXM bit. If internally generated (HCLKXM = 1), the clock is divided down by a programmable high clock divider (HCLKXDIV) from McASP internal clock source AUXCLK. The transmit high-frequency master clock may be (but is not required to be) output on the AHCLKX pin where it is available to other devices in the system.

The transmit clock configuration is controlled by the following registers:

- ACLKXCTL.
- • AHCLKXCTL.


**Figure 22-17. Transmit Clock Generator Block Diagram**
#### 22.3.5.2 Receive Clock

The receiver also has the option to operate synchronously from the ACLKX and AFSX signals. This is achieved when the ASYNC bit in the transmit clock control register (ACLKXCTL) is cleared to 0 (see Figure 22-18). The receiver may be configured with different polarity (CLKRP) and frame sync data delay options from those options of the transmitter.

The receive clock configuration is controlled by the following registers:

- ACLKRCTL.
- AHCLKRCTL.

Figure 22-18. Receive Clock Generator Block Diagram

#### 22.3.5.3 Frame Sync Generator

There are two different modes for frame sync: burst and TDM. A block diagram of the frame sync generator is shown in Figure 22-19. The frame sync options are programmed by the receive and transmit frame sync control registers (AFSRCTL and AFSXCTL). The options are:

- Internally-generated or externally-generated.
- Frame sync polarity: rising edge or falling edge.
- · Frame sync width: single bit or single word.
- Bit delay: 0, 1, or 2 cycles before the first data bit.

The transmit frame sync pin is AFSX and the receive frame sync pin is AFSR. A typical usage for these pins is to carry the left/right clock (LRCLK) signal when transmitting and receiving stereo data.

Regardless if the AFSX/AFSR is internally generated or externally sourced, the polarity of AFSX/AFSR is determined by FSXP/FSRP, respectively, to be either rising or falling edge. If FSXP/FSRP = 0, the frame sync polarity is rising edge. If FSRP/FSRP = 1, the frame sync polarity is falling edge.


Figure 22-19. Frame Sync Generator Block Diagram
#### **22.3.5.4 Clocking Examples**

Some examples of processes using the McASP clocking and frame flexibility are:

- Receive data from a DVD at 48 kHz, but output up-sampled or decoded audio at 96 kHz or 192 kHz. This could be accomplished by inputting a high-frequency master clock (for example, 512 × receive FS), receiving with an internally-generated bit clock ratio of divide-by-8, and transmitting with an internally-generated bit clock ratio of divide-by-4 or divide-by-2.
- Transmit/receive data based on one sample rate (for example, 44.1 kHz), and transmit/receive data at a different sample rate (for example, 48 kHz).

#### **22.3.5.5 Crystal Considerations**

When choosing a high frequency input crystal for the device, consider the maximum functional clock frequency of the McASP module versus the maximum external receive/transmit clock AHCLKR/AHCLKX, especially when planning to generate the clocks internally. For example, if the maximum transmit clock AHCLKX is 25 MHz, a master oscillator CLK\_M\_OSC of 26 MHz requires a divide-by-2. However, master oscillator of 25 MHz enables full performance of the transmit clock.

### *22.3.6 Signal Descriptions*

The signals used on the McASP audio interface are listed in [Table](#page-19-0) 22-7.

|         |       | Device Reset |                                                 |
|---------|-------|--------------|-------------------------------------------------|
| Pin     | I/O/Z | (RESET = 0)  | Description                                     |
|         |       |              | Transmitter Control                             |
| AHCLKX  | I/O/Z | Input        | Transmit high-frequency master clock            |
| AFSX    | I/O/Z | Input        | Transmit frame sync or left/right clock (LRCLK) |
| ACLKX   | I/O/Z | Input        | Transmit bit clock                              |
|         |       |              | Receiver Control                                |
| AHCLKR  | I/O/Z | Input        | Receive high-frequency master clock             |
| AFSR    | I/O/Z | Input        | Receive frame sync or left/right clock (LRCLK)  |
| ACLKR   | I/O/Z | Input        | Receive bit clock                               |
|         |       |              | Mute                                            |
| AMUTE   | I/O/Z | Input        | Mute output                                     |
| AMUTEIN | I/O/Z | Input        | Mute input                                      |
|         |       |              | Data                                            |
| AXRn    | I/O/Z | Input        | TX/RX data pins                                 |

**Table 22-7. McASP Interface Signals**

### *22.3.7 Pin Multiplexing*

The McASP signals share pins with other processor functions. For detailed information on the McASP pin multiplexing and configuration, see the pin multiplexing information in the device-specific data manual.
### *22.3.8 Transfer Modes*

#### **22.3.8.1 Burst Transfer Mode**

The McASP supports a burst transfer mode, which is useful for nonaudio data such as passing control information between two processors. Burst transfer mode uses a synchronous serial format similar to the TDM mode. The frame sync generation is not periodic or time-driven as in TDM mode, but data driven, and the frame sync is generated for each data word transferred.

When operating in burst frame sync mode [\(Figure](#page-20-1) 22-20), as specified for transmit (XMOD = 0 in AFSXCTL) and receive (RMOD = 0 in AFSRCTL), one slot is shifted for each active edge of the frame sync signal that is recognized. Additional clocks after the slot and before the next frame sync edge are ignored.

In burst frame sync mode, the frame sync delay may be specified as 0, 1, or 2 serial clock cycles. This is the delay between the frame sync active edge and the start of the slot. The frame sync signal lasts for a single bit clock duration (FRWID = 0 in AFSRCTL, FXWID = 0 in AFSXCTL).

For transmit, when generating the transmit frame sync internally, the frame sync begins when the previous transmission has completed and when all the XBUF[n] (for every serializer set to operate as a transmitter) has been updated with new data.

For receive, when generating the receive frame sync internally, frame sync begins when the previous transmission has completed and when all the RBUF[n] (for every serializer set to operate as a receiver) has been read.


**Figure 22-20. Burst Frame Sync Mode**

The control registers must be configured as follows for the burst transfer mode. The burst mode specific bit fields are in bold face:

- PFUNC: The clock, frame, data pins must be configured for McASP function.
- PDIR: The clock, frame, data pins must be configured to the direction desired.
- PDOUT, PDIN, PDSET, PDCLR: Not applicable. Leave at default.
- GBLCTL: Follow the initialization sequence in Section [22.3.12.2](#page-49-0) to configure this register.
- AMUTE: Not applicable. Leave at default.
- DLBCTL: If loopback mode is desired, configure this register according to Section [22.3.10.5](#page-47-0), otherwise leave this register at default.
- DITCTL: DITEN must be left at default 0 to select non-DIT mode. Leave the register at default.
- RMASK/XMASK: Mask desired bits according to Section [22.3.9.2](#page-27-0) and Section [22.3.10.3](#page-37-0).
- RFMT/XFMT: Program all fields according to data format desired. See Section [22.3.10.3](#page-37-0).
- AFSRCTL/AFSXCTL: Clear **RMOD/XMOD** bits to 0 to indicate burst mode. Clear **FRWID/FXWID** bits to 0 for single bit frame sync duration. Configure other fields as desired.
- ACLKRCTL/ACLKXCTL: Program all fields according to bit clock desired. See [Section](#page-15-1) 22.3.5.
- AHCLKRCTL/AHCLKXCTL: Program all fields according to high-frequency clock desired. See [Section](#page-15-1) 22.3.5.
- RTDM/XTDM: Program RTDMS0/XTDMS0 to 1 to indicate one active slot only. Leave other fields at default.
- RINTCTL/XINTCTL: Program all fields according to interrupts desired.
- RCLKCHK/XCLKCHK: Not applicable. Leave at default.
- SRCTLn: Program SRMOD to inactive/transmitter/receiver as desired. DISMOD is not applicable and should be left at default.
- DITCSRA[n], DITCSRB[n], DITUDRA[n], DITUDRB[n]: Not applicable. Leave at default.

#### **22.3.8.2 Time-Division Multiplexed (TDM) Transfer Mode**

The McASP time-division multiplexed (TDM) transfer mode supports the TDM format discussed in Section [22.3.3.1](#page-9-2).

Transmitting data in the TDM transfer mode requires a minimum set of pins:

- ACLKX transmit bit clock.
- AFSX transmit frame sync (or commonly called left/right clock).
- One or more serial data pins, AXRn, whose serializers have been configured to transmit.

The transmitter has the option to receive the ACLKX bit clock as an input, or to generate the ACLKX bit clock by dividing down the AHCLKX high-frequency master clock. The transmitter can either generate AHCLKX internally or receive AHCLKX as an input. See Section [22.3.5.1](#page-16-1).

Similarly, to receive data in the TDM transfer mode requires a minimum set of pins:

- ACLKR receive bit clock.
- AFSR receive frame sync (or commonly called left/right clock).
- One or more serial data pins, AXRn, whose serializers have been configured to receive.

The receiver has the option to receive the ACLKR bit clock as an input or to generate the ACLKR bit clock by dividing down the AHCLKR high-frequency master clock. The receiver can either generate AHCLKR internally or receive AHCLKR as an input. See Section [22.3.5.2](#page-17-1) and Section [22.3.5.3](#page-18-1).

The control registers must be configured as follows for the TDM mode. The TDM mode specific bit fields are in bold face:

- PFUNC: The clock, frame, data pins must be configured for McASP function.
- PDIR: The clock, frame, data pins must be configured to the direction desired.
- PDOUT, PDIN, PDSET, PDCLR: Not applicable. Leave at default.
- GBLCTL: Follow the initialization sequence in Section [22.3.12.2](#page-49-0) to configure this register.
- AMUTE: Program all fields according to mute control desired.
- DLBCTL: If loopback mode is desired, configure this register according to Section [22.3.10.5](#page-47-0), otherwise leave this register at default.
- DITCTL: DITEN must be left at default 0 to select TDM mode. Leave the register at default.
- RMASK/XMASK: Mask desired bits according to Section [22.3.9.2](#page-27-0) and Section [22.3.10.3](#page-37-0).
- RFMT/XFMT: Program all fields according to data format desired. See Section [22.3.10.3](#page-37-0).
- AFSRCTL/AFSXCTL: Set **RMOD/XMOD** bits to 2-32 for TDM mode. Configure other fields as desired.
- ACLKRCTL/ACLKXCTL: Program all fields according to bit clock desired. See [Section](#page-15-1) 22.3.5.
- AHCLKRCTL/AHCLKXCTL: Program all fields according to high-frequency clock desired. See [Section](#page-15-1) 22.3.5.
- RTDM/XTDM: Program all fields according to the time slot characteristics desired.
- RINTCTL/XINTCTL: Program all fields according to interrupts desired.
- RCLKCHK/XCLKCHK: Program all fields according to clock checking desired.
- SRCTLn: Program all fields according to serializer operation desired.
- DITCSRA[n], DITCSRB[n], DITUDRA[n], DITUDRB[n]: Not applicable. Leave at default.
#### *22.3.8.2.1 TDM Time Slots*

TDM mode on the McASP can extend to support multiprocessor applications, with up to 32 time slots per frame. For each of the time slots, the McASP may be configured to participate or to be inactive by configuring XTDM and/or RTDM (this allows multiple processors to communicate on the same TDM serial bus).

The TDM sequencer (separate ones for transmit and receive) functions in this mode. The TDM sequencer counts the slots beginning with the frame sync. For each slot, the TDM sequencer checks the respective bit in either XTDM or RTDM to determine if the McASP should transmit/receive in that time slot.

If the transmit/receive bit is active, the McASP functions normally during that time slot; otherwise, the McASP is inactive during that time slot; no update to the buffer occurs, and no event is generated. Transmit pins are automatically set to a high-impedance state, 0, or 1 during that slot, as determined by bit DISMOD in SRCTL[n].

[Figure](#page-22-0) 22-21 shows when the transmit DMA event AXEVT is generated. See Section [22.3.10.1.1](#page-31-0) for details on data ready and the initialization period indication. The transmit DMA event for an active time slot (slot N) is generated during the previous time slot (slot N - 1), regardless if the previous time slot (slot N - 1) is active or inactive.

During an active transmit time slot (slot N), if the next time slot (slot N + 1) is configured to be active, the copy from XRBUF[n] to XRSR[n] generates the DMA event for time slot N + 1. If the next time slot (slot N + 1) is configured to be inactive, then the DMA event will be delayed to time slot M - 1. In this case, slot M is the next active time slot. The DMA event for time slot M is generated during the first bit time of slot M - 1.

The receive DMA request generation does not need this capability, since the receive DMA event is generated after data is received in the buffer (looks back in time). If a time slot is disabled, then no data is copied to the buffer for that time slot and no DMA event is generated.

Slot 0 Slot 1 Slot N−1 Slot M Slot N+1 EDMA event for slot 0 EDMA event for slot 1 EDMA event for slot N − 1 EDMA event for slot N EDMA event for slot N + 1 Slot 1 Slot 2 Slot N Slot N EDMA event for slot 2 EDMA event for slot M Active slot Inactive slot Slot N−2 Slot 0 Slot M−1 Initialization period(A) EDMA event for slot N + 2 Initialization period(A)

**Figure 22-21. Transmit DMA Event (AXEVT) Generation in TDM Time Slots**

A See Section [22.3.12.2,](#page-49-0) step 7a.
#### *22.3.8.2.2 Special 384 Slot TDM Mode for Connection to External DIR*

The McASP receiver also supports a 384 time slot TDM mode (DIR mode), to support S/PDIF, AES-3, IEC-60958 receiver ICs whose natural block (block corresponds to McASP frame) size is 384 samples. The advantage to using the 384 time slot TDM mode is that interrupts may be generated synchronous to the S/PDIF, AES-3, IEC-60958, such as the last slot interrupt.

The receive TDM time slot register (RTDM) should be programmed to all 1s during reception of a DIR block. Other TDM functionalities (for example, inactive slots) are not supported (only the slot counter counts the 384 subframes in a block).

To receive data in the DIR mode, the following pins are typically needed:

- ACLKR receive bit clock.
- AFSR receive frame sync (or commonly called left/right clock). In this mode, AFSR should be connected to a DIR which outputs a start of block signal, instead of LRCLK.
- One or more serial data pins, AXRn, whose serializers have been configured to receive.

For this special DIR mode, the control registers can be configured just as for TDM mode, except set RMOD in AFSRCTL to 384 to receive 384 time slots.

#### **22.3.8.3 Digital Audio Interface Transmit (DIT) Transfer Mode**

In addition to the TDM and burst transfer modes, which are suitable for transmitting audio data between ICs inside the same system, the digital audio interface transmit (DIT) transfer mode of the McASP also supports transmission of audio data in the S/PDIF, AES-3, or IEC-60958 format. These formats are designed to carry audio data between different systems through an optical or coaxial cable. The DIT mode only applies to serializers configured as transmitters, not receivers. See Section [22.3.3.2](#page-11-2) for a description of the S/PDIF format.

#### *22.3.8.3.1 Transmit DIT Encoding*

The McASP operation in DIT mode is basically identical to the 2 time slot TDM mode, but the data transmitted is output as a biphase mark encoded bit stream, with preamble, channel status, user data, validity, and parity automatically stuffed into the bit stream by the McASP. The McASP includes separate validity bits for even/odd subframes and two 384-bit RAM modules to hold channel status and user data bits.

The transmit TDM time slot register (XTDM) should be programmed to all 1s during DIT mode. TDM functionality is not supported in DIT mode, except that the TDM slot counter counts the DIT subframes.

To transmit data in the DIT mode, the following pins are typically needed:

- AHCLKX transmit high-frequency master clock.
- One or more serial data pins, AXRn, whose serializers have been configured to transmit.

AHCLKX is optional (the internal clock source may be used instead), but if used as a reference, the processor provides a clock check circuit that continually monitors the AHCLKX input for stability.

If the McASP is configured to transmit in the DIT mode on more than one serial data pin, the bit streams on all pins will be synchronized. In addition, although they will carry unique audio data, they will carry the same channel status, user data, and validity information.

The actual 24-bit audio data must always be in bit positions 23-0 after passing through the first three stages of the transmit format unit.
For left-aligned Q31 data, the following transmit format unit settings process the data into right aligned 24 bit audio data ready for transmission:

- XROT = 010 (rotate right by 8 bits).
- XRVRS = 0 (no bit reversal, LSB first).
- XMASK = FFFF FF00h-FFFF 0000h (depending upon whether 24, 23, 22, 21, 20, 19, 18, 17, or 16 valid audio data bits are present).
- XPAD = 00 (pad extra bits with 0).

For right-aligned data, the following transmit format unit settings process the data into right aligned 24-bit audio data ready for transmission:

- XROT = 000 (rotate right by 0 bits).
- XRVRS = 0 (no bit reversal, LSB first).
- XMASK = 00FF FFFFh to 0000 FFFFh (depending upon whether 24, 23, 22, 21, 20, 19, 18, 17, or 16 valid audio data bits are present).
- XPAD = 00 (pad extra bits with 0).

#### *22.3.8.3.2 Transmit DIT Clock and Frame Sync Generation*

The DIT transmitter only works in the following configuration:

- In transmit frame control register (AFSXCTL):
  - Internally-generated transmit frame sync, FSXM = 1.
  - Rising-edge frame sync, FSXP = 0.
  - Bit-width frame sync, FXWID = 0.
  - 384-slot TDM, XMOD = 1 1000 0000b.
- In transmit clock control register (ACLKXCTL), ASYNC = 1.
- In transmit bitstream format register (XFMT), XSSZ = 1111 (32-bit slot size).

All combinations of AHCLKX and ACLKX are supported.

This is a summary of the register configurations required for DIT mode. The DIT mode specific bit fields are in bold face:

- PFUNC: The data pins must be configured for McASP function. If AHCLKX is used, it must also be configured for McASP function.
- PDIR: The data pins must be configured as outputs. If AHCLKX is used as an input reference, it should be configured as input. If internal clock source AUXCLK is used as the reference clock, it may be output on the AHCLKX pin by configuring AHCLKX as an output.
- PDOUT, PDIN, PDSET, PDCLR: Not applicable for DIT operation. Leave at default.
- GBLCTL: Follow the initialization sequence in Section [22.3.12.2](#page-49-0) to configure this register.
- AMUTE: Program all fields according to mute control desired.
- DLBCTL: Not applicable. Loopback is not supported for DIT mode. Leave at default.
- DITCTL: **DITEN** bit must be set to 1 to enable DIT mode. Configure other bits as desired.
- RMASK: Not applicable. Leave at default.
- RFMT: Not applicable. Leave at default.
- AFSRCTL: Not applicable. Leave at default.
- ACLKRCTL: Not applicable. Leave at default.
- AHCLKRCTL: Not applicable. Leave at default.
- RTDM: Not applicable. Leave at default.
- RINTCTL: Not applicable. Leave at default.
- RCLKCHK: Not applicable. Leave at default.
- **XMASK**: Mask desired bits according to the discussion in this section, depending upon left-aligned or right-aligned internal data.
• **XFMT: XDATDLY** = 0. **XRVRS** = 0. **XPAD** = 0. **XPBIT** = default (not applicable). **XSSZ** = Fh (32-bit slot). XBUSEL = configured as desired. **XROT** bit is configured according to the discussion in this section, either 0 or 8-bit rotate.

- **AFSXCTL**: Configure the bits according to the discussion in this section.
- **ACLKXCTL**: **ASYNC** = 1. Program CLKXDIV bits to obtain the bit clock rate desired. Configure CLKXP and CLKXM bits as desired, because CLKX is not actually used in the DIT protocol.
- **AHCLKXCTL**: Program all fields according to high-frequency clock desired.
- **XTDM**: Set to FFFF FFFFh for all active slots for DIT transfers.
- XINTCTL: Program all fields according to interrupts desired.
- XCLKCHK: Program all fields according to clock checking desired.
- SRCTLn: Set **SRMOD** = 1 (transmitter) for the DIT pins. DISMOD field is don't care for DIT mode.
- **DITCSRA[n], DITCSRB[n]**: Program the channel status bits as desired.
- **DITUDRA[n], DITUDRB[n]**: Program the user data bits as desired.

#### *22.3.8.3.3 DIT Channel Status and User Data Register Files*

The channel status registers (DITCSRA*n* and DITCSRB*n*) and user data registers (DITUDRA*n* and DITUDRB*n*) are not double buffered. Typically the programmer uses one of the synchronizing interrupts, such as last slot, to create an event at a safe time so the register may be updated. In addition, the CPU reads the transmit TDM slot counter to determine which word of the register is being used.

It is a requirement that the software avoid writing to the word of user data and channel status that are being used to encode the current time slot; otherwise, it will be indeterminate whether the old or new data is used to encode the bitstream.

The DIT subframe format is defined in Section [22.3.3.2.2.](#page-12-2) The channel status information (C) and User Data (U) are defined in these DIT control registers:

- DITCSRA0 to DITCSRA5: The 192 bits in these six registers contain the channel status information for the LEFT channel within each frame.
- DITCSRB0 to DITCSRB5: The 192 bits in these six registers contain the channel status information for the RIGHT channel within each frame.
- DITUDRA0 to DITUDRA5: The 192 bits in these six registers contain the user data information for the LEFT channel within each frame.
- DITUDRB0 to DITUDRB5: The 192 bits in these six registers contain the user data information for the RIGHT channel within each frame.

The S/PDIF block format is shown in [Figure](#page-13-2) 22-13. There are 192 frames within a block (frame 0 to frame 191). Within each frame there are two subframes (subframe 1 and 2 for left and right channels, respectively). The channel status and user data information sent on each subframe is summarized in [Table](#page-26-0) 22-8.
**Table 22-8. Channel Status and User Data for Each DIT Block**

| Frame | Subframe                                          | Preamble | Channel Status defined in: | User Data defined in: |
|-------|---------------------------------------------------|----------|----------------------------|-----------------------|
|       | Defined by DITCSRA0, DITCSRB0, DITUDRA0, DITUDRB0 |          |                            |                       |
| 0     | 1 (L)                                             | B        | DITCSRA0[0]                | DITUDRA0[0]           |
| 0     | 2 (R)                                             | W        | DITCSRB0[0]                | DITUDRB0[0]           |
| 1     | 1 (L)                                             | M        | DITCSRA0[1]                | DITUDRA0[1]           |
| 1     | 2 (R)                                             | W        | DITCSRB0[1]                | DITUDRB0[1]           |
| 2     | 1 (L)                                             | M        | DITCSRA0[2]                | DITUDRA0[2]           |
| 2     | 2 (R)                                             | W        | DITCSRB0[2]                | DITUDRB0[2]           |
| …     | …                                                 | …        | …                          | …                     |
| 31    | 1 (L)                                             | M        | DITCSRA0[31]               | DITUDRA0[31]          |
| 31    | 2 (R)                                             | W        | DITCSRB0[31]               | DITUDRB0[31]          |
|       | Defined by DITCSRA1, DITCSRB1, DITUDRA1, DITUDRB1 |          |                            |                       |
| 32    | 1 (L)                                             | M        | DITCSRA1[0]                | DITUDRA1[0]           |
| 32    | 2 (R)                                             | W        | DITCSRB1[0]                | DITUDRB1[0]           |
| …     | …                                                 | …        | …                          | …                     |
| 63    | 1 (L)                                             | M        | DITCSRA1[31]               | DITUDRA1[31]          |
| 63    | 2 (R)                                             | W        | DITCSRB1[31]               | DITUDRB1[31]          |
|       | Defined by DITCSRA2, DITCSRB2, DITUDRA2, DITUDRB2 |          |                            |                       |
| 64    | 1 (L)                                             | M        | DITCSRA2[0]                | DITUDRA2[0]           |
| 64    | 2 (R)                                             | W        | DITCSRB2[0]                | DITUDRB2[0]           |
| …     | …                                                 | …        | …                          | …                     |
| 95    | 1 (L)                                             | M        | DITCSRA2[31]               | DITUDRA2[31]          |
| 95    | 2 (R)                                             | W        | DITCSRB2[31]               | DITUDRB2[31]          |
|       | Defined by DITCSRA3, DITCSRB3, DITUDRA3, DITUDRB3 |          |                            |                       |
| 96    | 1 (L)                                             | M        | DITCSRA3[0]                | DITUDRA3[0]           |
| 96    | 2 (R)                                             | W        | DITCSRB3[0]                | DITUDRB3[0]           |
| …     | …                                                 | …        | …                          | …                     |
| 127   | 1 (L)                                             | M        | DITCSRA3[31]               | DITUDRA3[31]          |
| 127   | 2 (R)                                             | W        | DITCSRB3[31]               | DITUDRB3[31]          |
|       | Defined by DITCSRA4, DITCSRB4, DITUDRA4, DITUDRB4 |          |                            |                       |
| 128   | 1 (L)                                             | M        | DITCSRA4[0]                | DITUDRA4[0]           |
| 128   | 2 (R)                                             | W        | DITCSRB4[0]                | DITUDRB4[0]           |
| …     | …                                                 | …        | …                          | …                     |
| 159   | 1 (L)                                             | M        | DITCSRA4[31]               | DITUDRA4[31]          |
| 159   | 2 (R)                                             | W        | DITCSRB4[31]               | DITUDRB4[31]          |
|       | Defined by DITCSRA5, DITCSRB5, DITUDRA5, DITUDRB5 |          |                            |                       |
| 160   | 1 (L)                                             | M        | DITCSRA5[0]                | DITUDRA5[0]           |
| 160   | 2 (R)                                             | W        | DITCSRB5[0]                | DITUDRB5[0]           |
| …     | …                                                 | …        | …                          | …                     |
| 191   | 1 (L)                                             | M        | DITCSRA5[31]               | DITUDRA5[31]          |
| 191   | 2 (R)                                             | W        | DITCSRB5[31]               | DITUDRB5[31]          |
### *22.3.9 General Architecture*

#### **22.3.9.1 Serializers**

[Figure](#page-27-1) 22-22 shows the block diagram of the serializer and its interface to other units within the McASP. The serializers take care of shifting serial data in and out of the McASP. Each serializer consists of a shift register (XRSR), a data buffer (XRBUF), a control register (SRCTL), and logic to support the data alignment options of the McASP. For each serializer, there is a dedicated serial data pin (AXRn) and a dedicated control register (SRCTL[n]). The control register allows the serializer to be configured as a transmitter, receiver, or as inactive. When configured as a transmitter the serializer shifts out data to the serial data pin AXRn. When configured as a receiver, the serializer shifts in data from the AXRn pin. The serializer is clocked from the transmit/receive section clock (ACLKX/ACLKR) if configured to transmit/receive respectively.

All serializers that are configured to transmit operate in lock-step. Similarly, all serializers that are configured to receive also operate in lock-step. This means that at most there are two zones per McASP, one for transmit and one for receive.

**Figure 22-22. Individual Serializer and Connections Within McASP**


For receive, data is shifted in through the AXRn pin to the shift register XRSR. Once the entire slot of data is collected in the XRSR, the data is copied to the data buffer XRBUF. The data is now ready to be read by the processor through the RBUF register, which is an alias of the XRBUF for receive. When the processor reads from the RBUF, the McASP passes the data from RBUF through the receive format unit and returns the formatted data to the processor.

For transmit, the processor services the McASP by writing data into the XBUF register, which is an alias of the XRBUF for transmit. The data automatically passes through the transmit format unit before actually reaching the XRBUF register in the serializer. The data is then copied from XRBUF to XRSR, and shifted out from the AXRn synchronously to the serial clock.

In DIT mode, in addition to the data, the serializer shifts out other DIT-specific information accordingly (preamble, user data, etc.).

The serializer configuration is controlled by SRCTL[n].

#### **22.3.9.2 Format Unit**

The McASP has two data formatting units, one for transmit and one for receive. These units automatically remap the data bits within the transmitted and received words between a natural format for the processor (such as a Q31 representation) and the required format for the external serial device (such as "I2S format"). During the remapping process, the format unit also can mask off certain bits or perform sign extension.

Since all transmitters share the same data formatting unit, the McASP only supports one transmit format at a time. For example, the McASP will not transmit in "I2S format" on serializer 0, while transmitting "Left Justified" on serializer 1. Likewise, the receiver section of the McASP only supports one data format at a time, and this format applies to all receiving serializers. However, the McASP can transmit in one format while receiving in a completely different format.
This formatting unit consists of three stages:

- Bit mask and pad (masks off bits, performs sign extension)
- Rotate right (aligns data within word)
- Bit reversal (selects between MSB first or LSB first)

[Figure](#page-28-0) 22-23 shows a block diagram of the receive formatting unit, and [Figure](#page-28-1) 22-24 shows the transmit formatting unit. Note that the order in which data flows through the three stages is different between the transmit and receive formatting units.

Bit mask/pad RMASK RPAD Bus (configuration bus or data port) 32 RROT 32 Programmable rotate by: 0, 4, 8, 12, 16, 20, 24, 28 32 Bit reverse RRVRS 32 Parallel read from XRBUF[n] RPBIT

**Figure 22-23. Receive Format Unit**


The bit mask and pad stage includes a full 32-bit mask register, allowing selected individual bits to either pass through the stage unchanged, or be masked off. The bit mask and pad then pad the value of the masked off bits by inserting either a 0, a 1, or one of the original 32 bits as the pad value. The last option allows for sign-extension when the sign bit is selected to pad the remaining bits.

The rotate right stage performs bitwise rotation by a multiple of 4 bits (between 0 and 28 bits), programmable by the (R/X)FMT register. Note that this is a rotation process, not a shifting process, so bit 0 gets shifted back into bit 31 during the rotation.

The bit reversal stage either passes all 32 bits directly through, or swaps them. This allows for either MSB or LSB first data formats. If bit reversal is not enabled, then the McASP will naturally transmit and receive in an LSB first order.

Finally, note that the (R/X)DATDLY bits in (R/X)FMT also determine the data format. For example, the difference between I2S format and left-justified is determined by the delay between the frame sync edge and the first data bit of a given time slot. For I2S format, (R/X)DATDLY should be set to a 1-bit delay, whereas for left-justified format, it should be set to a 0-bit delay.

The combination of all the options in (R/X)FMT means that the McASP supports a wide variety of data formats, both on the serial data lines, and in the internal processor representation.

Section [22.3.10.3](#page-37-0) provides more detail and specific examples. The examples use internal representation in integer and Q31 notation, but other fractional notations are also possible.

#### **22.3.9.3 State Machine**

The receive and transmit sections have independent state machines. Each state machine controls the interactions between the various units in the respective section. In addition, the state machine keeps track of error conditions and serial port status.

No serial transfers can occur until the respective state machine is released from reset. See initialization sequence for details [\(Section](#page-48-0) 22.3.12).

The receive state machine is controlled by the RFMT register, and it reports the McASP status and error conditions in the RSTAT register. Similarly, the transmit state machine is controlled by the XFMT register, and it reports the McASP status and error conditions in the XSTAT register.

#### **22.3.9.4 TDM Sequencer**

There are separate TDM sequencers for the transmit section and the receive section. Each TDM sequencer keeps track of the slot count. In addition, the TDM sequencer checks the bits of (R/X)TDM and determines if the McASP should receive/transmit in that time slot.

If the McASP should participate (transmit/receive bit is active) in the time slot, the McASP functions normally. If the McASP should not participate (transmit/receive bit is inactive) in the time slot, no transfers between the XRBUF and XRSR registers in the serializer would occur during that time slot. In addition, the serializers programmed as transmitters place their data output pins in a predetermined state (logic low, high, or high impedance) as programmed by each serializer control register (SRCTL). Refer also to Section [22.3.8.2](#page-21-0) for details on how DMA event or interrupt generations are handled during inactive time slots in TDM mode.

The receive TDM sequencer is controlled by register RTDM and reports current receive slot to RSLOT. The transmit TDM sequencer is controlled by register XTDM and reports current transmit slot to XSLOT.

#### **22.3.9.5 Clock Check Circuit**

A common source of error in audio systems is a serial clock failure due to instabilities in the off-chip DIR circuit. To detect a clock error quickly, a clock-check circuit is included in the McASP for both transmit and receive clocks, since both may be sourced from off chip.

The clock check circuit can detect and recover from transmit and receive clock failures. See Section [22.3.10.4.6](#page-43-0) for implementation and programming details.
#### **22.3.9.6 Pin Function Control**

All McASP pins except AMUTEIN are bidirectional input/output pins. In addition, these bidirectional pins function either as McASP or general-purpose I/O (GPIO) pins. The following registers control the pin functions:

- Pin function register (PFUNC): selects pin to function as McASP or GPIO.
- Pin direction register (PDIR): selects pin to be input or output.
- Pin data input register (PDIN): shows data input at the pin.
- Pin data output register (PDOUT): data to be output at the pin if the pin is configured as GPIO output (PFUNC[n] = 1 and PDIR[n] = 1). Not applicable when the pin is configured as McASP pin (PFUNC[n] = 0).
- Pin data set register (PDSET): alias of PDOUT. Writing a 1 to PDSET[n] sets the respective PDOUT[n] to 1. Writing a 0 has no effect. Applicable only when the pin is configured as GPIO output (PFUNC[n] = 1 and PDIR[n] = 1).
- Pin data clear register (PDCLR): alias of PDOUT. Writing a 1 to PDCLR[n] clears the respective PDOUT[n] to 0. Writing a 0 has no effect. Applicable only when the pin is configured as GPIO output (PFUNC[n] = 1 and PDIR[n] = 1).

See the register descriptions in [Section](#page-58-0) 22.4 for details on the mapping of each McASP pin to the register bits. [Figure](#page-30-0) 22-25 shows the pin control block diagram.


**Figure 22-25. McASP I/O Pin Control Block Diagram**

#### *22.3.9.6.1 McASP Pin Control-Transmit and Receive*

You must correctly set the McASP GPIO registers PFUNC and PDIR, even when McASP pins are used for their serial port (non-GPIO) function.

Serial port functions include:

- Clock pins (ACLKX, ACLKR, AHCLKX, AHCLKR, AFSX, AFSR) used as clock inputs and outputs.
- Serializer data pins (AXRn) used to transmit or receive.
- AMUTE used as a mute output signal.

When using these pins in their serial port function, you must clear PFUNC[n] to 0 for each pin.

Also, certain outputs require PDIR[n] = 1, such as clock pins used as clock outputs, serializer data pins used to transmit, and AMUTE used as mute output.

Clock inputs and serializers configured to receive must have PDIR[n] = 0.

PFUNC and PDIR do not control the AMUTEIN signal, it is usually tied to a device level interrupt pin (consult device datasheet). If used as a mute input, this pin needs to be configured as an input in the appropriate peripheral.
Finally, there is an important advantage to having separate control of pin direction (by PDIR), and the choice of internal versus external clocking (by CLKRM/CLKXM). Depending on the specific device and usage, you might select an external clock (CLKRM = 0), while enabling the internal clock divider, and the clock pin as an output in the PDIR register (PDIR[ACLKR] = 1). In this case, the bit clock is an output (PDIR[ACLKR] = 1) and, therefore, routed to the ACLKR pin. However, because CLKRM = 0, the bit clock is then routed back to the McASP module as an "external" clock source. This may result in less skew between the clock inside the McASP and the clock in the external device, thus producing more balanced setup and hold times for a particular system. As a result, this may allow a higher serial clock rate interface.

### *22.3.10 Operation*

This section discusses the operation of the McASP.

#### **22.3.10.1 Data Transmission and Reception**

The processor services the McASP by writing data to the XBUF register(s) for transmit operations, and by reading data from the RBUF register(s) for receive operations. The McASP sets status flag and notifies the processor whenever data is ready to be serviced. Section [22.3.10.1.1](#page-31-0) discusses data ready status in detail.

The XBUF and RBUF registers can be accessed through one of the two peripheral ports of the device:

- The data port (DAT): This port is dedicated for data transfers on the device.
- The configuration bus (CFG): This port is used for both data transfers and peripheral configuration control on the device.

Section [22.3.10.1.2](#page-34-0) and Section [22.3.10.1.3](#page-34-1) discuss how to perform transfers through the data port and the configuration bus.

Either the CPU or the DMA can be used to service the McASP through any of these two peripheral ports. The CPU and DMA usages are discussed in Section [22.3.10.1.4](#page-34-2) and Section [22.3.14.2](#page-55-0).

#### *22.3.10.1.1 Data Ready Status and Event/Interrupt Generation*

#### *22.3.10.1.1.1 Transmit Data Ready*

The transmit data ready flag XDATA bit in the XSTAT register reflects the status of the XBUF register. The XDATA flag is set when data is transferred from the XRBUF[n] buffers to the XRSR[n] shift registers, indicating that the XBUF is empty and ready to accept new data from the processor. This flag is cleared when the XDATA bit is written with a 1, or when all the serializers configured as transmitters are written by the processor.

Whenever XDATA is set, an DMA event AXEVT is automatically generated to notify the DMA of the XBUF empty status. An interrupt AXINT is also generated if XDATA interrupt is enabled in the XINTCTL register (See Section [22.3.13.2](#page-53-0) for details).

For DMA requests, the McASP does not require XSTAT to be read between DMA events. This means that even if XSTAT already has the XDATA flag set to 1 from a previous request, the next transfer triggers another DMA request.

Since all serializers act in lockstep, only one DMA event is generated to indicate that all active transmit serializers are ready to be written to with new data.

[Figure](#page-32-0) 22-26 shows the timing details of when AXEVT is generated at the McASP boundary. In this example, as soon as the last bit (bit A0) of Word A is transmitted, the McASP sets the XDATA flag and generates an AXEVT event. However, it takes up to 5 McASP system clocks (AXEVT Latency) before AXEVT is active at the McASP boundary. Upon AXEVT, the processor can begin servicing the McASP by writing Word C into the XBUF (Processor Service Time). The processor must write Word C into the XBUF no later than the setup time required by the McASP (Setup Time).

The maximum Processor Service Time [\(Figure](#page-32-0) 22-26) can be calculated as:

Processor Service Time = Time Slot - AXEVT Latency - Setup Time

**Figure 22-26. Processor Service Time Upon Transmit DMA Event (AXEVT)**

A Refer to the device-specific data manual for the McASP system clock source. This is not the same as AUXCLK.

*Example 22-1. Processor Service Time Calculation for Transmit DMA Event (AXEVT)*

The following is an example to show how to calculate Processor Service Time. Assume the following setup:

• McASP transmits in I2S format at 192 kHz frame rate. Assume slot size is 32 bit.

With the above setup, we obtain the following parameters corresponding to [Figure](#page-32-0) 22-26:

- Calculation of McASP system clock cycle:
  - System functional clock = 26 MHz
  - Therefore, McASP system clock cycle = 1/26MHz = 38.5 ns.
- Calculation of ACLKX clock cycle:
  - This example has two 32-bit slots per frame, for a total of 64 bits per frame.
  - ACLKX clock cycle is (1/192 kHz)/64 = 81.4 ns.
- Time Slot between AXEVT events:
  - For I2S format, McASP generates two AXEVT events per 192 kHz frame.
  - Therefore, Time Slot between AXEVT events is (1/192 kHz)/2 = 2604 ns.
- AXEVT Latency:
  - = 5 McASP system clocks
  - = 38.5 ns × 5 = 192.5 ns
- Setup Time
  - = 3 McASP system clocks + 4 ACLKX cycles
  - = (38.5 ns × 3) + (81.4 ns × 4)
  - = 441.1 ns
- Processor Service Time
  - = Time Slot AXEVT Latency Setup Time
  - = 2604 ns 441.1 ns 192.5 ns
  - = 1970.4 ns
#### *22.3.10.1.1.2 Receive Data Ready*

Similarly, the receive data ready flag RDATA bit in the RSTAT reflects the status of the RBUF register. The RDATA flag is set when data is transferred from the XRSR[n] shift registers to the XRBUF[n] buffers, indicating that the RBUF contains received data and is ready to have the processor read the data. This flag is cleared when the RDATA bit is written with a 1, or when all the serializers configured as receivers are read.

Whenever RDATA is set, an DMA event AREVT is automatically generated to notify the DMA of the RBUF ready status. An interrupt ARINT is also generated if RDATA interrupt is enabled in the RINTCTL register (See Section [22.3.13.3](#page-53-1) for details).

For DMA requests, the McASP does not require RSTAT to be read between DMA events. This means that even if RSTAT already has the RDATA flag set to 1 from a previous request, the next transfer triggers another DMA request.

Since all serializers act in lockstep, only one DMA event is generated to indicate that all active receive serializers are ready to receive new data.

[Figure](#page-33-0) 22-27 shows the timing details of when AREVT is generated at the McASP boundary. In this example, as soon as the last bit (bit A0) of Word A is received, the McASP sets the RDATA flag and generates an AREVT event. However, it takes up to 5 McASP system clocks (AREVT Latency) before AREVT is active at the McASP boundary. Upon AREVT, the processor can begin servicing the McASP by reading Word A from the RBUF (Processor Service Time). The processor must read Word A from the XBUF no later than the setup time required by the McASP (Setup Time).

The maximum Processor Service Time [\(Figure](#page-33-0) 22-27) can be calculated as: Processor Service Time = Time Slot - AREVT Latency - Setup Time

The Processor Service Time calculation for receive is similar to the calculation for transmit. See [Example](#page-32-1) 22-1 for Processor Service Time calculation using transmit as an example.


**Figure 22-27. Processor Service Time Upon Receive DMA Event (AREVT)**

A The device uses SYSCLK2 as the McASP system clock source.
#### *22.3.10.1.2 Transfers Through the Data Port (DAT)*

**NOTE:** To perform internal transfers through the data port, clear XBUSEL/RBUSEL bit to 0 in the

respective XFMT/RFMT registers. Failure to do so will result in software malfunction.

Typically, you will access the McASP XRBUF registers through the data port. To access through the data port, simply have the CPU or DMA access the XRBUF through its data port location. Through the data port, the DMA/CPU can service all the serializers through a single address. The McASP automatically cycles through the appropriate serializers.

For transmit operations through the data port, the DMA/CPU should write to the same XBUF data port address to service all of the active transmit serializers. In addition, the DMA/CPU should write to the XBUF for all active transmit serializers in incremental (although not necessarily consecutive) order. For example, if serializers 0, 4, and 5, are set up as active transmitters, the DMA/CPU should write to the XBUF data port address four times with data for serializers 0, 4, and 5, upon each transmit data ready event. This exact servicing order must be followed so that data appears in the appropriate serializers.

Similarly, for receive operations through the data port, the DMA/CPU should read from the same RBUF data port address to service all of the active receive serializers. In addition, reads from the active receive serializers through the data port return data in incremental (although not necessarily consecutive) order. For example, if serializers 1, 2, and 3, are set up as active receivers, the DMA/CPU should read from the RBUF data port address four times to obtain data for serializers 1, 2, and 3, in this exact order, upon each receive data ready event.

When transmitting, the DMA/CPU must write data to each serializer configured as "active" and "transmit" within each time slot. Failure to do so results in a buffer underrun condition (Section [22.3.10.4.2\)](#page-41-0). Similarly, when receiving, data must be read from each serializer configured as "active" and "receive" within each time slot. Failure to do results in a buffer overrun condition (Section [22.3.10.4.3](#page-42-0)).

To perform internal transfers through the data port, clear XBUSEL/RBUSEL bit to 0 in the respective XFMT/RFMT registers.

#### *22.3.10.1.3 Transfers Through the Configuration Bus (CFG)*

**NOTE:** To perform internal transfers through the configuration bus, set XBUSEL/RBUSEL bit to 1 in the respective XFMT/RFMT registers. Failure to do so will result in software malfunction.

In this method, the DMA/CPU accesses the XRBUF registers through the configuration bus address. The exact XRBUF register address for any particular serializer is determined by adding the offset for that particular serializer to the base address for the particular McASP. XRBUF for the serializers configured as transmitters is given the name XBUF*n*. For example, the XRBUF associated with transmit serializer 2 is named XBUF2. Similarly, XRBUF for the serializers configured as receivers is given the name RBUF*n*.

Accessing the XRBUF registers through the data port is different because the CPU/DMA only needs to access one single address. When accessing through the configuration bus, the CPU/DMA must provide the exact XBUF*n* or RBUF*n* address for each access.

When transmitting, DMA/CPU must write data to each serializer configured as "active" and "transmit" within each time slot. Failure to do so results in a buffer underrun condition (Section [22.3.10.4.2\)](#page-41-0). Similarly when receiving, data must be read from each serializer configured as "active" and "receive" within each time slot. Failure to do results in a buffer overrun condition (Section [22.3.10.4.3](#page-42-0)).

To perform internal transfers through the configuration bus, set XBUSEL/RBUSEL bit to 1 in the respective XFMT/RFMT registers.

#### *22.3.10.1.4 Using the CPU for McASP Servicing*

The CPU can be used to service the McASP through interrupt (upon AXINT/ARINT interrupts) or through polling the XDATA bit in the XSTAT register. As discussed in Section [22.3.10.1.2](#page-34-0) and Section [22.3.10.1.3,](#page-34-1) the CPU can access through either the data port or through the configuration bus.
To use the CPU to service the McASP through interrupts, the XDATA/RDATA bit must be enabled in the respective XINTCTL/RINTCTL registers, to generate interrupts AXINT/ARINT to the CPU upon data ready.

#### **22.3.10.2 McASP Audio FIFO (AFIFO)**

The AFIFO contains two FIFOs: one Read FIFO (RFIFO), and one Write FIFO (WFIFO). To ensure backward compatibility with existing software, both the Read and Write FIFOs are disabled by default. See [Figure](#page-35-0) 22-28 for a high-level block diagram of the AFIFO.

The AFIFO may be enabled/disabled and configured via the WFIFOCTL and RFIFOCTL registers. Note that if the Read or Write FIFO is to be enabled, it must be enabled prior to initializing the receive/transmit section of the McASP (see Section [22.3.12.2](#page-49-0) for details).


**Figure 22-28. McASP Audio FIFO (AFIFO) Block Diagram**

#### *22.3.10.2.1 AFIFO Data Transmission*

When the Write FIFO is disabled, transmit DMA requests pass through directly from the McASP to the host/DMA controller. Whether the WFIFO is enabled or disabled, the McASP generates transmit DMA requests as needed; the AFIFO is "invisible" to the McASP.

When the Write FIFO is enabled, transmit DMA requests from the McASP are sent to the AFIFO, which in turn generates transmit DMA requests to the host/DMA controller.

If the Write FIFO is enabled, upon a transmit DMA request from the McASP, the WFIFO writes *WNUMDMA* 32-bit words to the McASP if and when there are at least *WNUMDMA* words in the Write FIFO. If there are not, the WFIFO waits until this condition has been satisfied. At that point, it writes *WNUMDMA* words to the McASP. (See description for WFIFOCTL.WNUMDMA.)

If the host CPU writes to the Write FIFO, independent of a transmit DMA request, the WFIFO will accept host writes until full. After this point, excess data will be discarded.

Note that when the WFIFO is first enabled, it will immediately issue a transmit DMA request to the host. This is because it begins in an empty state, and is therefore ready to accept data.
#### *22.3.10.2.1.1 Transmit DMA Event Pacer*

The AFIFO may be configured to delay making a transmit DMA request to the host until the Write FIFO has enough space for a specified number of words. In this situation, the number of transmit DMA requests to the host or DMA controller is reduced.

If the Write FIFO has space to accept *WNUMEVT* 32-bit words, it generates a transmit DMA request to the host and then waits for a response. Once *WNUMEVT* words have been written to the FIFO, it checks again to see if there is space for *WNUMEVT* 32-bit words. If there is space, it generates another transmit DMA request to the host, and so on. In this fashion, the Write FIFO will attempt to stay filled.

Note that if transmit DMA event pacing is desired, WFIFOCTL.WNUMEVT should be set to a non-zero integer multiple of the value in WFIFOCTL.WNUMDMA. If transmit DMA event pacing is not desired, then the value in WFIFOCTL.WNUMEVT should be set equal to the value in WFIFOCTL.WNUMDMA.

#### *22.3.10.2.2 AFIFO Data Reception*

When the Read FIFO is disabled, receive DMA requests pass through directly from McASP to the host/DMA controller. Whether the RFIFO is enabled or disabled, the McASP generates receive DMA requests as needed; the AFIFO is "invisible" to the McASP.

When the Read FIFO is enabled, receive DMA requests from the McASP are sent to the AFIFO, which in turn generates receive DMA requests to the host/DMA controller.

If the Read FIFO is enabled and the McASP makes a receive DMA request, the RFIFO reads *RNUMDMA* 32-bit words from the McASP, if and when the RFIFO has space for *RNUMDMA* words. If it does not, the RFIFO waits until this condition has been satisfied; at that point, it reads *RNUMDMA* words from the McASP. (See description for RFIFOCTL.RNUMDMA.)

If the host CPU reads the Read FIFO, independent of a receive DMA request, and the RFIFO at that time contains less than *RNUMEVT* words, those words will be read correctly, emptying the FIFO.

#### *22.3.10.2.2.1 Receive DMA Event Pacer*

The AFIFO may be configured to delay making a receive DMA request to the host until the Read FIFO contains a specified number of words. In this situation, the number of receive DMA requests to the host or DMA controller is reduced.

If the Read FIFO contains at least *RNUMEVT* 32-bit words, it generates a receive DMA request to the host and then waits for a response. Once *RNUMEVT* 32-bit words have been read from the RFIFO, the RFIFO checks again to see if it contains at least another *RNUMEVT* words. If it does, it generates another receive DMA request to the host, and so on. In this fashion, the Read FIFO will attempt to stay empty.

Note that if receive DMA event pacing is desired, RFIFOCTL.RNUMEVT should be set to a non-zero integer multiple of the value in RFIFOCTL.RNUMDMA. If receive DMA event pacing is not desired, then the value in RFIFOCTL.RNUMEVT should be set equal to the value in RFIFOCTL.RNUMDMA.

#### *22.3.10.2.3 Arbitration Between Transmit and Receive DMA Requests*

If both the WFIFO and the RFIFO are enabled and a transmit DMA request and receive DMA request occur simultaneously, priority is given to the transmit DMA request. Once a transfer is in progress, it is allowed to complete.

If only the WFIFO is enabled and a transmit DMA request and receive DMA request occur simultaneously, priority is given to the transmit DMA request. Once a transfer is in progress, it is allowed to complete.

If only the RFIFO is enabled and a transmit DMA request and receive DMA request occur simultaneously, priority is given to the receive DMA request. Once a transfer is in progress, it is allowed to complete.
#### **22.3.10.3 Formatter**

#### *22.3.10.3.1 Transmit Bit Stream Data Alignment*

The McASP transmitter supports serial formats of:

- Slot (or Time slot) size = 8, 12, 16, 20, 24, 28, 32 bits.
- Word size ≤ Slot size.
- Alignment: when more bits/slot than bits/words, then:
  - Left aligned = word shifted first, remaining bits are pad.
  - Right aligned = pad bits are shifted first, word occupies the last bits in slot.
- Order: order of bits shifted out:
  - MSB: most-significant bit of word is shifted out first, last bit is LSB.
  - LSB: least-significant bit of word is shifted out last, last bit is MSB.

Hardware support for these serial formats comes from the programmable options in the transmit bitstream format register (XFMT):

- XRVRS: bit reverse (1) or no bit reverse (0).
- XROT: rotate right by 0, 4, 8, 12, 16, 20, 24, or 28 bits.
- XSSZ: transmit slot size of 8, 12, 16, 20, 24, 28, or 32 bits.

XSSZ should always be programmed to match the slot size of the serial stream. The word size is not directly programmed into the McASP, but rather is used to determine the rotation needed in the XROT field.

[Table](#page-37-1) 22-9 and [Figure](#page-38-0) 22-29 show the XRVRS and XROT fields for each serial format and for both integer and Q31 fractional internal representations.

This discussion assumes that all slot size (SLOT in [Table](#page-37-1) 22-9) and word size (WORD in [Table](#page-37-1) 22-9) options are multiples of 4, since the transmit rotate right unit only supports rotation by multiples of 4. However, the bit mask/pad unit does allow for any number of significant digits. For example, a Q31 number may have 19 significant digits (word) and be transmitted in a 24-bit slot; this would be formatted as a word size of 20 bits and a slot size of 24 bits. However, it is possible to set the bit mask unit to only pass the 19 most-significant digits (program the mask value to FFFF E000h). The digits that are not significant can be set to a selected pad value, which can be any one of the significant digits, a fixed value of 0, or a fixed value of 1.

The transmit bit mask/pad unit operates on data as an initial step of the transmit format unit (see [Figure](#page-28-1) 22-24), and the data is aligned in the same representation as it is written to the transmitter by the processor (typically Q31 or integer).

|              |                  | Bit Stream    | Internal Numeric | XFMT Bit    |       |
|--------------|------------------|---------------|------------------|-------------|-------|
| Figure 22-29 | Bit Stream Order | Alignment     | Representation   | XROT(1)     | XRVRS |
| (a)(2)       | MSB first        | Left aligned  | Q31 fraction     | 0           | 1     |
| (b)          | MSB first        | Right aligned | Q31 fraction     | SLOT - WORD | 1     |
| (c)          | LSB first        | Left aligned  | Q31 fraction     | 32 - WORD   | 0     |
| (d)          | LSB first        | Right aligned | Q31 fraction     | 32 - SLOT   | 0     |
| (e)(2)       | MSB first        | Left aligned  | Integer          | WORD        | 1     |
| (f)          | MSB first        | Right aligned | Integer          | SLOT        | 1     |
| (g)          | LSB first        | Left aligned  | Integer          | 0           | 0     |

(h) LSB first Right aligned Integer (32 - (SLOT - WORD)) % 32 0

**Table 22-9. Transmit Bitstream Data Alignment**

<sup>(1)</sup> WORD = Word size rounded up to the nearest multiple of 4; SLOT = slot size; % = modulo operator

<sup>(2)</sup> To transmit in I2S format, use MSB first, left aligned, and also select XDATDLY = 01 (1 bit delay)
#### **Figure 22-29. Data Flow Through Transmit Format Unit, Illustrated**

#### *22.3.10.3.2 Receive Bit Stream Data Alignment*

The McASP receiver supports serial formats of:

- Slot or time slot size = 8, 12, 16, 20, 24, 28, 32 bits.
- Word size ≤ Slot size.
- Alignment when more bits/slot than bits/words, then:
  - Left aligned = word shifted first, remaining bits are pad.
  - Right aligned = pad bits are shifted first, word occupies the last bits in slot.
- Order of bits shifted out:
  - MSB: most-significant bit of word is shifted out first, last bit is LSB.
  - LSB: least-significant bit of word is shifted out last, last bit is MSB.

Hardware support for these serial formats comes from the programmable options in the receive bitstream format register (RFMT):

- RRVRS: bit reverse (1) or no bit reverse (0).
- RROT: rotate right by 0, 4, 8, 12, 16, 20, 24, or 28 bits.
- RSSZ: receive slot size of 8, 12, 16, 20, 24, 28, or 32 bits.

RSSZ should always be programmed to match the slot size of the serial stream. The word size is not directly programmed into the McASP, but rather is used to determine the rotation needed in the RROT field.

Table [22-10](#page-39-0) and [Figure](#page-40-0) 22-30 show the RRVRS and RROT fields for each serial format and for both integer and Q31 fractional internal representations.

This discussion assumes that all slot size and word size options are multiples of 4; since the receive rotate right unit only supports rotation by multiples of 4. However, the bit mask/pad unit does allow for any number of significant digits. For example, a Q31 number may have 19 significant digits (word) and be transmitted in a 24-bit slot; this would be formatted as a word size of 20 bits and a slot size of 24 bits. However, it is possible to set the bit mask unit to only pass the 19 most-significant digits (program the mask value to FFFF E000h). The digits that are not significant can be set to a selected pad value, which can be any one of the significant digits, a fixed value of 0, or a fixed value of 1.

The receive bit mask/pad unit operates on data as the final step of the receive format unit (see [Figure](#page-28-0) 22- [23](#page-28-0)), and the data is aligned in the same representation as it is read from the receiver by the processor (typically Q31 or integer).

**Table 22-10. Receive Bitstream Data Alignment**

|              |                  | Bit Stream    | Internal Numeric | RFMT Bit                  |       |
|--------------|------------------|---------------|------------------|---------------------------|-------|
| Figure 22-30 | Bit Stream Order | Alignment     | Representation   | RROT(1)                   | RRVRS |
| (a)(2)       | MSB first        | Left aligned  | Q31 fraction     | SLOT                      | 1     |
| (b)          | MSB first        | Right aligned | Q31 fraction     | WORD                      | 1     |
| (c)          | LSB first        | Left aligned  | Q31 fraction     | (32 - (SLOT - WORD)) % 32 | 0     |
| (d)          | LSB first        | Right aligned | Q31 fraction     | 0                         | 0     |
| (e)(2)       | MSB first        | Left aligned  | Integer          | SLOT - WORD               | 1     |
| (f)          | MSB first        | Right aligned | Integer          | 0                         | 1     |
| (g)          | LSB first        | Left aligned  | Integer          | 32 - SLOT                 | 0     |
| (h)          | LSB first        | Right aligned | Integer          | 32 - WORD                 | 0     |

<sup>(1)</sup> WORD = Word size rounded up to the nearest multiple of 4; SLOT = slot size; % = modulo operator

<sup>(2)</sup> To transmit in I2S format, select MSB first, left aligned, and also select RDATDLY = 01 (1 bit delay)
#### **Figure 22-30. Data Flow Through Receive Format Unit, Illustrated**

#### **22.3.10.4 Error Handling and Management**

To support the design of a robust audio system, the McASP includes error-checking capability for the serial protocol, data underrun, and data overrun. In addition, the McASP includes a timer that continually measures the high-frequency master clock every 32 AHCLKX/AHCLKR clock cycles. The timer value can be read to get a measurement of the clock frequency and has a minimum and maximum range setting that can set an error flag if the master clock goes out of a specified range.

Upon the detection of any one or more errors (software selectable), or the assertion of the AMUTEIN input pin, the AMUTE output pin may be asserted to a high or low level to immediately mute the audio output. In addition, an interrupt may be generated if desired, based on any one or more of the error sources.

#### *22.3.10.4.1 Unexpected Frame Sync Error*

An unexpected frame sync occurs when:

- In burst mode, when the next active edge of the frame sync occurs early such that the current slot will not be completed by the time the next slot is scheduled to begin.
- In TDM mode, a further constraint is that the frame sync must occur exactly during the correct bit clock (not a cycle earlier or later) and only before slot 0. An unexpected frame sync occurs if this condition is not met.

When an unexpected frame sync occurs, there are two possible actions depending upon when the unexpected frame sync occurs:

- 1. Early: An early unexpected frame sync occurs when the McASP is in the process of completing the current frame and a new frame sync is detected (not including overlap that occurs due to a 1 or 2 bit frame sync delay). When an early unexpected frame sync occurs:
  - Error interrupt flag is set (XSYNCERR, if an unexpected transmit frame sync occurs; RSYNCERR, if an unexpected receive frame sync occurs).
  - Current frame is not resynchronized. The number of bits in the current frame is completed. The next frame sync, which occurs after the current frame is completed, will be resynchronized.
- 2. Late: A late unexpected frame sync occurs when there is a gap or delay between the last bit of the previous frame and the first bit of the next frame. When a late unexpected frame sync occurs (as soon as the gap is detected):
  - Error interrupt flag is set (XSYNCERR, if an unexpected transmit frame sync occurs; RSYNCERR, if an unexpected receive frame sync occurs).
  - Resynchronization occurs upon the arrival of the next frame sync.

Late frame sync is detected the same way in both burst mode and TDM mode; however, in burst mode, late frame sync is not meaningful and its interrupt enable should not be set.

#### *22.3.10.4.2 Buffer Underrun Error - Transmitter*

A buffer underrun can only occur for serializers programmed to be transmitters. A buffer underrun occurs when the serializer is instructed by the transmit state machine to transfer data from XRBUF[n] to XRSR[n], but XRBUF[n] has not yet been written with new data since the last time the transfer occurred. When this occurs, the transmit state machine sets the XUNDRN flag.

An underrun is checked only once per time slot. The XUNDRN flag is set when an underrun condition occurs. Once set, the XUNDRN flag remains set until the processor explicitly writes a 1 to the XUNDRN bit to clear the XUNDRN bit.

In DIT mode, a pair of BMC zeros is shifted out when an underrun occurs (four bit times at 128 × fs). By shifting out a pair of zeros, a clock may be recovered on the receiver. To recover, reset the McASP and start again with the proper initialization.

In TDM mode, during an underrun case, a long stream of zeros are shifted out causing the DACs to mute. To recover, reset the McASP and start again with the proper initialization.
#### *22.3.10.4.3 Buffer Overrun Error - Receiver*

A buffer overrun can only occur for serializers programmed to be receivers. A buffer overrun occurs when the serializer is instructed to transfer data from XRSR[n] to XRBUF[n], but XRBUF[n] has not yet been read by either the DMA or the processor. When this occurs, the receiver state machine sets the ROVRN flag. However, the individual serializer writes over the data in the XRBUF[n] register (destroying the previous sample) and continues shifting.

An overrun is checked only once per time slot. The ROVRN flag is set when an overrun condition occurs. It is possible that an overrun occurs on one time slot but then the processor catches up and does not cause an overrun on the following time slots. However, once the ROVRN flag is set, it remains set until the processor explicitly writes a 1 to the ROVRN bit to clear the ROVRN bit.

#### *22.3.10.4.4 DMA Error - Transmitter*

A transmit DMA error, as indicated by the XDMAERR flag in the XSTAT register, occurs when the DMA (or CPU) writes more words to the DAT port of the McASP than it should. For each DMA event, the DMA should write exactly as many words as there are serializers enabled as transmitters.

XDMAERR indicates that the DMA (or CPU) wrote too many words to the McASP for a given transmit DMA event. Writing too few words results in a transmit underrun error setting XUNDRN in XSTAT.

While XDMAERR occurs infrequently, an occurrence indicates a serious loss of synchronization between the McASP and the DMA or CPU. You should reinitialize both the McASP transmitter and the DMA to resynchronize them.

#### *22.3.10.4.5 DMA Error - Receiver*

A receive DMA error, as indicated by the RDMAERR flag in the RSTAT register, occurs when the DMA (or CPU) reads more words from the DAT port of the McASP than it should. For each DMA event, the DMA should read exactly as many words as there are serializers enabled as receivers.

RDMAERR indicates that the DMA (or CPU) read too many words from the McASP for a given receive DMA event. Reading too few words results in a receiver overrun error setting ROVRN in RSTAT.

While RDMAERR occurs infrequently, an occurrence indicates a serious loss of synchronization between the McASP and the DMA or CPU. You should reinitialize both the McASP receiver and the DMA to resynchronize them.
#### *22.3.10.4.6 Clock Failure Detection*

#### *22.3.10.4.6.1 Clock-Failure Check Startup*

It is expected, initially, that the clock-failure circuits will generate an error until at least one measurement has been taken. Therefore, the clock failure interrupts, clock switch, and mute functions should not immediately be enabled, but be enabled only after a specific startup procedure. The startup procedure is:

- 1. For the transmit clock failure check:
  - a. Configure transmit clock failure detect logic (XMIN, XMAX, XPS) in the transmit clock check control register (XCLKCHK).
  - b. Clear transmit clock failure flag (XCKFAIL) in the transmit status register (XSTAT).
  - c. Wait until first measurement is taken (> 32 AHCLKX clock periods).
  - d. Verify no clock failure is detected.
  - e. Repeat steps b–d until clock is running and is no longer issuing clock failure errors.
  - f. After the transmit clock is measured and falls within the acceptable range, the following may be enabled:
    - i. transmit clock failure interrupt enable bit (XCKFAIL) in the transmitter interrupt control register (XINTCTL).
    - ii. transmit clock failure detect autoswitch enable bit (XCKFAILSW) in the transmit clock check control register (XCLKCHK).
    - iii. mute option (XCKFAIL) in the mute control register (AMUTE).
- 2. For the receive clock failure check:
  - a. Configure receive clock failure detect logic (RMIN, RMAX, RPS) in the receive clock check control register (RCLKCHK).
  - b. Clear receive clock failure flag (RCKFAIL) in the receive status register (RSTAT).
  - c. Wait until first measurement is taken (> 32 AHCLKR clock periods).
  - d. Verify no clock failure is detected.
  - e. Repeat steps b–d until clock is running and is no longer issuing clock failure errors.
  - f. After the receive clock is measured and falls within the acceptable range, the following may be enabled:
    - i. receive clock failure interrupt enable bit (RCKFAIL) in the receiver interrupt control register (RINTCTL).
    - ii. mute option (RCKFAIL) in the mute control register (AMUTE).
#### 22.3.10.4.6.2 Transmit Clock Failure Check and Recovery

The transmit clock failure check circuit (Figure 22-31) works off both the internal McASP system clock and the external high-frequency serial clock (AHCLKX). It continually counts the number of system clocks for every 32 high rate serial clock (AHCLKX) periods, and stores the count in XCNT of the transmit clock check control register (XCLKCHK) every 32 high rate serial clock cycles.

The logic compares the count against a user-defined minimum allowable boundary (XMIN), and automatically flags an interrupt (XCKFAIL in XSTST) when an out-of-range condition occurs. An out-of-range minimum condition occurs when the count is smaller than XMIN. The logic continually compares the current count (from the running system clock counter) against the maximum allowable boundary (XMAX). This is in case the external clock completely stops, so that the counter value is not copied to XCNT. An out-of-range maximum condition occurs when the count is greater than XMAX. Note that the XMIN and XMAX fields are 8-bit unsigned values, and the comparison is performed using unsigned arithmetic.

An out-of-range count may indicate either that an unstable clock was detected, or that the audio source has changed and a new sample rate is being used.

In order for the transmit clock failure check circuit to operate correctly, the high-frequency serial clock divider must be taken out of reset regardless if AHCLKX is internally generated or externally sourced.


Figure 22-31. Transmit Clock Failure Detection Circuit Block Diagram

A Refer to device data manual for the McASP system clock source. This is not the same as AUXCLK.
The following actions are taken if a clock failure is detected:

1. Transmit clock failure flag (XCKFAIL) in XSTAT is set. This causes an interrupt if transmit clock failure interrupt enable bit (XCKFAIL) in XINTCTL is set.

In addition (only supported for DIT mode), if the transmit clock failure detect autoswitch enable bit (XCKFAILSW) in XCLKCHK is set, the following additional steps are taken to change the clock source from external to internal:

- 1. High-frequency transmit clock source bit (HCLKXM) in AHCLKXCTL is set to 1 and internal serial clock divider is selected. However, AHCLKX pin direction does not change to an output while XCKFAIL is set.
- 2. The internal clock divider is reset, so that the next clock it produces is a full period. However, the transmit clock divide ratio bits (HCLKXDIV) in AHCLKXCTL are not affected, so the internal clock divider generates clocks at the rate configured.
- 3. The transmit section is reset for a single serial clock period.
- 4. The transmit section is released from reset and attempts to begin transmitting. If data is available, it begins transmitting immediately; otherwise, it enters the underrun state. An initial underrun is certain to occur, the pattern 1100 (BMC zeroes) should be shifted out initially.

To change back to an external clock, take the following actions:

- 1. Wait for the external clock to stabilize again. This can be checked by polling the transmit clock count (XCNT) in XCLKCHK.
- 2. Reset the transmit section according to the startup procedure in Section [22.3.10.4.6.1.](#page-43-1)
#### *22.3.10.4.6.3 Receive Clock Failure Check and Recovery*

The receive clock failure check circuit ([Figure](#page-46-0) 22-32) works off both the internal McASP system clock and the external high-frequency serial clock (AHCLKR). It continually counts the number of system clocks for every 32 high rate serial clock (AHCLKR) periods, and stores the count in RCNT of the receive clock check control register (RCLKCHK) every 32 high rate serial clock cycles.

The logic compares the count against a user-defined minimum allowable boundary (RMIN) and automatically flags an interrupt (RCKFAIL in RSTAT) when an out-of-range condition occurs. An out-ofrange minimum condition occurs when the count is smaller than RMIN. The logic continually compares the current count (from the running system clock counter) against the maximum allowable boundary (RMAX). This is in case the external clock completely stops, so that the counter value is not copied to RCNT. An out-of-range maximum condition occurs when the count is greater than RMAX. Note that the RMIN and RMAX fields are 8-bit unsigned values, and the comparison is performed using unsigned arithmetic.

An out-of-range count may indicate either that an unstable clock was detected or that the audio source has changed and a new sample rate is being used.

In order for the receive clock failure check circuit to operate correctly, the high-frequency serial clock divider must be taken out of reset regardless if AHCLKR is internally generated or externally sourced.


**Figure 22-32. Receive Clock Failure Detection Circuit Block Diagram**

A Refer to device data manual for the McASP system clock source. This is not the same as AUXCLK.
#### **22.3.10.5 Loopback Modes**

The McASP features a digital loopback mode (DLB) that allows testing of the McASP code in TDM mode with a single processor device. In loopback mode, output of the transmit serializers is connected internally to the input of the receive serializers. Therefore, you can check the receive data against the transmit data to ensure that the McASP settings are correct. Digital loopback mode applies to TDM mode only (2 to 32 slots in a frame). It does not apply to DIT mode (XMOD = 180h) or burst mode (XMOD = 0).

[Figure](#page-47-1) 22-33 shows the basic logical connection of the serializers in loopback mode. Two types of loopback connections are possible, selected by the ORD bit in the digital loopback control register (DLBCTL) as follows:

- ORD = 0: Outputs of odd serializers are connected to inputs of even serializers. If this mode is selected, you should configure odd serializers to be transmitters and even serializers to be receivers.
- ORD = 1: Outputs of even serializers are connected to inputs of odd serializers. If this mode is selected, you should configure even serializers to be transmitters and odd serializers to be receivers.

Data can be externally visible at the I/O pin of the transmit serializer if the pin is configured as a McASP output pin by setting the corresponding PFUNC bit to 0 and PDIR bit to 1.

In loopback mode, the transmit clock and frame sync are used by both the transmit and receive sections of the McASP. The transmit and receive sections operate synchronously. This is achieved by setting the MODE bit of the DLBCTL register to 01b and the ASYNC bit of the ACLKXCTL register to 0.


**Figure 22-33. Serializers in Loopback Mode**

and

odd transmit)

even transmit)
#### *22.3.10.5.1 Loopback Mode Configurations*

This is a summary of the settings required for digital loopback mode for TDM format:

- The DLBEN bit in DLBCTL must be set to 1 to enable loopback mode.
- The MODE bits in DLBCTL must be set to 01b for both the transmit and receive sections to use the transmit clock and frame sync generator.
- The ORD bit in DLBCTL must be programmed appropriately to select odd or even serializers to be transmitters or receivers. The corresponding serializers must be configured accordingly.
- The ASYNC bit in ACLKXCTL must be cleared to 0 to ensure synchronous transmit and receive operations.
- RMOD field in AFSRCTL and XMOD field in AFSXCTL must be set to 2h to 20h to indicate TDM mode. Loopback mode does not apply to DIT or burst mode.

### *22.3.11 Reset Considerations*

The McASP has two reset sources: software reset and hardware reset.

#### **22.3.11.1 Software Reset Considerations**

The transmitter and receiver portions of the McASP may be put in reset through the global control register (GBLCTL). Note that a valid serial clock must be supplied to the desired portion of the McASP (transmit and/or receive) in order to assert the software reset bits in GBLCTL. see Section [22.3.12.2](#page-49-0) for details on how to ensure reset has occurred.

The entire McASP module may also be reset through the Power and Sleep Controller (PSC). Note that from the McASP perspective, this reset appears as a hardware reset to the entire module.

#### **22.3.11.2 Hardware Reset Considerations**

When the McASP is reset due to device reset, the entire serial port (including the transmitter and receiver state machines, and other registers) is reset.

### *22.3.12 Setup and Initialization*

This section discusses steps necessary to use the McASP module.

#### **22.3.12.1 Considerations When Using a McASP**

The following is a list of things to be considered for systems using a McASP:

#### *22.3.12.1.1 Clocks*

For each receive and transmit section:

- External or internal generated bit clock and high frequency clock?
- If internally generated, what is the bit clock speed and the high frequency clock speed?
- Clock polarity?
- External or internal generated frame sync?
- If internally generated, what is frame sync speed?
- Frame sync polarity?
- Frame sync width?
- Transmit and receive sync or asynchronous?

#### *22.3.12.1.2 Data Pins*

For each pin of each McASP:

- McASP or GPIO?
- Input or output?
#### *22.3.12.1.3 Data Format*

For each transmit and receive data:

- Internal numeric representation (integer, Q31 fraction)?
- I2S or DIT (transmit only)?
- Time slot delay (0, 1, or 2 bit)?
- Alignment (left or right)?
- Order (MSB first, LSB first)?
- Pad (if yes, pad with what value)?
- Slot size?
- Rotate?
- Mask?

#### *22.3.12.1.4 Data Transfers*

• Internal: DMA or CPU?

• External: TDM or burst?

• Bus: configuration bus (CFG) or data port (DAT)?

#### **22.3.12.2 Transmit/Receive Section Initialization**

You must follow the following steps to properly configure the McASP. If external clocks are used, they should be present prior to the following initialization steps.

- 1. Reset McASP to default values by setting GBLCTL = 0.
- 2. Configure all McASP registers except GBLCTL in the following order:
  - a. Power Idle SYSCONFIG: PWRIDLESYSCONFIG.
  - b. Receive registers: RMASK, RFMT, AFSRCTL, ACLKRCTL, AHCLKRCTL, RTDM, RINTCTL, RCLKCHK. If external clocks AHCLKR and/or ACLKR are used, they must be running already for proper synchronization of the GBLCTL register.
  - c. Transmit registers: XMASK, XFMT, AFSXCTL, ACLKXCTL, AHCLKXCTL, XTDM, XINTCTL, XCLKCHK. If external clocks AHCLKX and/or ACLKX are used, they must be running already for proper synchronization of the GBLCTL register.
  - d. Serializer registers: SRCTL[n].
  - e. Global registers: Registers PFUNC, PDIR, DITCTL, DLBCTL, AMUTE. Note that PDIR should only be programmed after the clocks and frames are set up in the steps above. This is because the moment a clock pin is configured as an output in PDIR, the clock pin starts toggling at the rate defined in the corresponding clock control register. Therefore you must ensure that the clock control register is configured appropriately before you set the pin to be an output. A similar argument applies to the frame sync pins. Also note that the reset state for the transmit highfrequency clock divide register (HCLKXDIV) is divide-by-1, and the divide-by-1 clocks are not gated by the transmit high-frequency clock divider reset enable (XHCLKRST).
  - f. DIT registers: For DIT mode operation, set up registers DITCSRA[n], DITCSRB[n], DITUDRA[n], and DITUDRB[n].
- 3. Start the respective high-frequency serial clocks AHCLKX and/or AHCLKR. This step is necessary even if external high-frequency serial clocks are used:
  - a. Take the respective internal high-frequency serial clock divider(s) out of reset by setting the RHCLKRST bit for the receiver and/or the XHCLKRST bit for the transmitter in GBLCTL. All other bits in GBLCTL should be held at 0.
  - b. Read back from GBLCTL to ensure the bit(s) to which you wrote are successfully latched in GBLCTL before you proceed.
- 4. Start the respective serial clocks ACLKX and/or ACLKR. This step can be skipped if external serial clocks are used and they are running:
  - a. Take the respective internal serial clock divider(s) out of reset by setting the RCLKRST bit for the receiver and/or the XCLKRST bit for the transmitter in GBLCTL. All other bits in GBLCTL should be left at the previous state.
  - b. Read back from GBLCTL to ensure the bit(s) to which you wrote are successfully latched in GBLCTL before you proceed.
- 5. Setup data acquisition as required:
  - a. If DMA is used to service the McASP, set up data acquisition as desired and start the DMA in this step, before the McASP is taken out of reset.
  - b. If CPU interrupt is used to service the McASP, enable the transmit and/ or receive interrupt as required.
  - c. If CPU polling is used to service the McASP, no action is required in this step.
- 6. Activate serializers.
  - a. Before starting, clear the respective transmitter and receiver status registers by writing XSTAT = FFFFh and RSTAT = FFFFh.
  - b. Take the respective serializers out of reset by setting the RSRCLR bit for the receiver and/or the XSRCLR bit for the transmitter in GBLCTL. All other bits in GBLCTL should be left at the previous state.
  - c. Read back from GBLCTL to ensure the bit(s) to which you wrote are successfully latched in GBLCTL before you proceed.
- 7. Verify that all transmit buffers are serviced. Skip this step if the transmitter is not used. Also, skip this step if time slot 0 is selected as inactive (special cases, see [Figure](#page-22-0) 22-21, second waveform). As soon as the transmit serializer is taken out of reset, XDATA in the XSTAT register is set, indicating that XBUF is empty and ready to be serviced. The XDATA status causes an DMA event AXEVT to be generated, and can cause an interrupt AXINT to be generated if it is enabled in the XINTCTL register.
  - a. If DMA is used to service the McASP, the DMA automatically services the McASP upon receiving AXEVT. Before proceeding in this step, you should verify that the XDATA bit in the XSTAT is cleared to 0, indicating that all transmit buffers are already serviced by the DMA.
  - b. If CPU interrupt is used to service the McASP, interrupt service routine is entered upon the AXINT interrupt. The interrupt service routine should service the XBUF registers. Before proceeding in this step, you should verify that the XDATA bit in XSTAT is cleared to 0, indicating that all transmit buffers are already serviced by the CPU.
  - c. If CPU polling is used to service the McASP, the XBUF registers should be written to in this step.
- 8. Release state machines from reset.
  - I. Take the respective state machine(s) out of reset by setting the RSMRST bit for the receiver and/or the XSMRST bit for the transmitter in GBLCTL. All other bits in GBLCTL should be left at the previous state.
  - II. Read back from GBLCTL to ensure the bit(s) to which you wrote are successfully latched in GBLCTL before you proceed.
- 9. Release frame sync generators from reset. Note that it is necessary to release the internal frame sync generators from reset, even if an external frame sync is being used, because the frame sync error detection logic is built into the frame sync generator.
  - a. Take the respective frame sync generator(s) out of reset by setting the RFRST bit for the receiver, and/or the XFRST bit for the transmitter in GBLCTL. All other bits in GBLCTL should be left at the previous state.
  - b. Read back from GBLCTL to ensure the bit(s) to which you wrote are successfully latched in GBLCTL before you proceed.
- 10. Upon the first frame sync signal, McASP transfers begin. The McASP synchronizes to an edge on the frame sync pin, not the level on the frame sync pin. This makes it easy to release the state machine and frame sync generators from reset.
  - a. For example, if you configure the McASP for a rising edge transmit frame sync, then you do not need to wait for a low level on the frame sync pin before releasing the McASP transmitter state
machine and frame sync generators from reset.
#### **22.3.12.3 Separate Transmit and Receive Initialization**

In many cases, it is desirable to separately initialize the McASP transmitter and receiver. For example, you may delay the initialization of the transmitter until the type of data coming in on the receiver is recognized. Or a change in the incoming data stream on the receiver may necessitate a reinitialization of the transmitter.

In this case, you may still follow the sequence outlined in Section [22.3.12.2,](#page-49-0) but use it for each section (transmit, receive) individually. The GBLCTL register is aliased to RGBLCTL and XGBLCTL to facilitate separate initialization of transmit and receive sections.

#### **22.3.12.4 Importance of Reading Back GBLCTL**

In Section [22.3.12.2](#page-49-0), steps 3b, 4b, 6c, 8b, and 9b state that GBLCTL should be read back until the bits that were written are successfully latched. This is important, because the transmitter and receiver state machines run off of the respective bit clocks, which are typically about tens to hundreds of times slower than the processor's internal bus clock. Therefore, it takes many cycles between when the processor writes to GBLCTL (or RGBLCTL and XGBLCTL), and when the McASP actually recognizes the write operation. If you skip this step, then the McASP may never see the reset bits in the global control registers get asserted and de-asserted; resulting in an uninitialized McASP.

Therefore, the logic in McASP has been implemented such that once the processor writes GBLCTL, RGBLCTL, or XGBLCTL, the resulting write is not visible by reading back GBLCTL until the McASP has recognized the change. This typically requires two bit clocks plus two processor bus clocks to occur.

Also, if the bit clocks can be completely stopped, any software that polls GBLCTL should be implemented with a time-out. If GBLCTL does not have a time-out, and the bit clock stops, the changes written to GBLCTL will not be reflected until the bit clock restarts.

Finally, please note that while RGBLCTL and XGBLCTL allow separate changing of the receive and transmit halves of GBLCTL, they also immediately reflect the updated value (useful for debug purposes). Only GBLCTL can be used for the read back step.

#### **22.3.12.5 Synchronous Transmit and Receive Operation (ASYNC = 0)**

When ASYNC = 0 in ACLKXCTL, the transmit and receive sections operate synchronously from the transmit section clock and transmit frame sync signals ([Figure](#page-16-0) 22-17). The receive section may have a different (but compatible in terms of slot size) data format.

When ASYNC = 0, the receive frame sync generator is internally disabled. If the AFSX pin is configured as an output, it serves as the frame sync signal for both transmit and receive. The AFSR pin should not be used because the transmit frame sync generator output, which is used by both the transmitter and the receiver when ASYNC = 0, is not propagated to the AFSR pin [\(Figure](#page-18-0) 22-19).

When ASYNC = 0, the transmit and receive sections must share some common settings, since they both use the same clock and frame sync signals:

- DITEN = 0 in DITCTL (TDM mode is enabled).
- The total number of bits per frame must be the same (that is, RSSZ × RMOD must equal XSSZ × XMOD).
- Both transmit and receive should either be specified as burst or TDM mode, but not mixed.
- The settings in ACLKRCTL are irrelevant.
- FSXM must match FSRM.
- FXWID must match FRWID.

For all other settings, the transmit and receive sections may be programmed independently.

#### **22.3.12.6 Asynchronous Transmit and Receive Operation (ASYNC = 1)**

When ASYNC = 1 in ACLKXCTL, the transmit and receive sections operate completely independently and have separate clock and frame sync signals ([Figure](#page-16-0) 22-17, [Figure](#page-17-0) 22-18, and [Figure](#page-18-0) 22-19). The events generated by each section come asynchronously.
### *22.3.13 Interrupts*

#### **22.3.13.1 Interrupt Multiplexing**

The processor includes an interrupt controller (INTC) to manage CPU interrupts. The INTC maps the device events to 12 CPU interrupts. The McASP generates 4 interrupts to the processor.

The event inputs can be routed to 12 maskable interrupts to the CPU (INT[15:4]). The INTC interrupt selector allows the McASP system events to be routed to any of the 12 CPU interrupt inputs. Furthermore, the INTC provides status flags and allows the combination of events, as shown in [Figure](#page-53-2) 22-34. You must properly configure the INTC by enabling, masking, and routing the McASP system events to the desired CPU interrupts.

**Figure 22-34. Interrupt Multiplexing**


#### **22.3.13.2 Transmit Data Ready Interrupt**

The transmit data ready interrupt (XDATA) is generated if XDATA is 1 in the XSTAT register and XDATA is also enabled in XINTCTL. Section [22.3.10.1.1](#page-31-0) provides details on when XDATA is set in the XSTAT register.

A transmit start of frame interrupt (XSTAFRM) is triggered by the recognition of transmit frame sync. A transmit last slot interrupt (XLAST) is a qualified version of the data ready interrupt (XDATA). It has the same behavior as the data ready interrupt, but is further qualified by having the data requested belonging to the last slot (the slot that just ended was next-to-last TDM slot, current slot is last slot).

#### **22.3.13.3 Receive Data Ready Interrupt**

The receive data ready interrupt (RDATA) is generated if RDATA is 1 in the RSTAT register and RDATA is also enabled in RINTCTL. Section [22.3.10.1.2](#page-34-0) provides details on when RDATA is set in the RSTAT register.

A receiver start of frame interrupt (RSTAFRM) is triggered by the recognition of a receiver frame sync. A receiver last slot interrupt (RLAST) is a qualified version of the data ready interrupt (RDATA). It has the same behavior as the data ready interrupt, but is further qualified by having the data in the buffer come from the last TDM time slot (the slot that just ended was last TDM slot).

#### **22.3.13.4 Error Interrupts**

Upon detection, the following error conditions generate interrupt flags:

- In the receive status register (RSTAT):
  - Receiver overrun (ROVRN).
  - Unexpected receive frame sync (RSYNCERR).
  - Receive clock failure (RCKFAIL).
  - Receive DMA error (RDMAERR).
- In the transmit status register (XSTAT):
  - Transmit underrun (XUNDRN).
  - Unexpected transmit frame sync (XSYNCERR).
  - Transmit clock failure (XCKFAIL).
  - Transmit DMA error (XDMAERR).

Each interrupt source also has a corresponding enable bit in the receive interrupt control register (RINTCTL) and transmit interrupt control register (XINTCTL). If the enable bit is set in RINTCTL or XINTCTL, an interrupt is requested when the interrupt flag is set in RSTAT or XSTAT. If the enable bit is not set, no interrupt request is generated. However, the interrupt flag may be polled.

#### 22.3.13.5 Audio Mute (AMUTE) Function

The McASP includes an automatic audio mute function (Figure 22-35) that asserts in hardware the AMUTE pin to a preprogrammed output state, as selected by the MUTEN bit in the audio mute control register (AMUTE). The AMUTE pin is asserted when one of the interrupt flags is set or an external device issues an error signal on the AMUTEIN input. Typically, the AMUTEIN input is shared with a device interrupt pin (for example EXT\_INT4).

The AMUTEIN input allows the on-chip logic to consider a mute input from other devices in the system, so that all errors may be considered. The AMUTEIN input has a programmable polarity to allow it to adapt to different devices, as selected by the INPOL bit in AMUTE, and it must be enabled explicitly.

In addition to the external AMUTEIN input, the AMUTE pin output may be asserted when one of the error interrupt flags is set and its mute function is enabled in AMUTE.

When one or more of the errors is detected and enabled, the AMUTE pin is driven to an active state that is selected by MUTEN in AMUTE. The active polarity of the AMUTE pin is programmable by MUTEN (and the inactive polarity is the opposite of the active polarity). The AMUTE pin remains driven active until software clears all the error interrupt flags that are enabled to mute, and until the AMUTEIN is inactive.


Figure 22-35. Audio Mute (AMUTE) Block Diagram
#### **22.3.13.6 Multiple Interrupts**

This only applies to interrupts and not to DMA requests. The following terms are defined:

- **Active Interrupt Request:** a flag in RSTAT or XSTAT is set and the interrupt is enabled in RINTCTL or XINTCTL.
- **Outstanding Interrupt Request:** An interrupt request has been issued on one of the McASP transmit/receive interrupt ports, but that request has not yet been serviced.
- **Serviced:** The CPU writes to RSTAT or XSTAT to clear one or more of the active interrupt request flags.

The first interrupt request to become active for the transmitter with the interrupt flag set in XSTAT and the interrupt enabled in XINTCTL generates a request on the McASP transmit interrupt port AXINT.

If more than one interrupt request becomes active in the same cycle, a single interrupt request is generated on the McASP transmit interrupt port. Subsequent interrupt requests that become active while the first interrupt request is outstanding do not immediately generate a new request pulse on the McASP transmit interrupt port.

The transmit interrupt is serviced with the CPU writing to XSTAT. If any interrupt requests are active after the write, a new request is generated on the McASP transmit interrupt port.

The receiver operates in a similar way, but using RSTAT, RINTCTL, and the McASP receive interrupt port ARINT.

One outstanding interrupt request is allowed on each port, so a transmit and a receive interrupt request may both be outstanding at the same time.

### *22.3.14 EDMA Event Support*

#### **22.3.14.1 EDMA Events**

There are 6 EDMA events.

#### **22.3.14.2 Using the DMA for McASP Servicing**

The most typical scenario is to use the DMA to service the McASP through the data port, although the DMA can also service the McASP through the configuration bus. Two possibilities exist for using the DMA events to service the McASP:

- 1. Use **AXEVT/AREVT**: Triggered upon each XDATA/RDATA transition from 0 to 1.
- 2. Use **AXEVTO/AREVTO** and **AXEVTE/AREVTE**: Alternating AXEVT/AREVT events for odd/even slots. Upon AXEVT/AREVT, AXEVTO/AREVTO is triggered if the event is for an odd channel, and AXEVTE/AREVTE is triggered if the event is for an even channel.

**NOTE:** Check the device-specific data manual to see if AXEVTO/AREVTO and AXEVTE/AREVTE are supported. These are optional.

[Figure](#page-56-0) 22-36 and [Figure](#page-56-1) 22-37 show an example audio system with six audio channels (LF, RF, LS, RS, C, and LFE) transmitted from three AXRn pins on the McASP. [Figure](#page-56-0) 22-36 and [Figure](#page-56-1) 22-37 show when events AXEVT, AXEVTO, and AXEVTE are triggered. [Figure](#page-56-0) 22-36 and [Figure](#page-56-1) 22-37 also apply for the receive audio channels and show when events AREVT, AREVTO, and AREVTE are triggered.

You can either use the DMA to service the McASP upon events AXEVT and AREVT [\(Figure](#page-56-0) 22-36) or upon events AXEVTO, AREVTO, AXEVTE, and AREVTE ([Figure](#page-56-1) 22-37).

**Figure 22-36. DMA Events in an Audio Example–Two Events (Scenario 1)**

**Figure 22-37. DMA Events in an Audio Example–Four Events (Scenario 2)**


In scenario 1 [\(Figure](#page-56-0) 22-36), a DMA event AXEVT/AREVT is triggered on each time slot. In the example, AXEVT is triggered for each of the transmit audio channel time slot (Time slot for channels LF, LS, and C; and time slot for channels RF, RS, LFE). Similarly, AREVT is triggered for each of the receive audio channel time slot. Scenario 1 allows for the use of a single DMA to transmit all audio channels, and a single DMA to receive all audio channels.

In scenario 2 [\(Figure](#page-56-1) 22-37), two alternating DMA events are triggered for each time slot. In the example, AXEVTE (even) is triggered for the time slot for the even audio channels (LF, LS, C) and AXEVTO (odd) is triggered for the time slot for the odd audio channels (RF, RS, LFE). AXEVTO and AXEVTE alternate in time. The same is true in the receive direction with the use of AREVTO and AREVTE. This scenario allows for the use of two DMA channels (odd and even) to transmit all audio channels, and two DMA channels to receive all audio channels.
Here are some guidelines on using the different DMA events:

• Either use AXEVT, or the combination of AXEVTO and AXEVTE, to service the McASP. Never use all three at the same time. Similarly for receive, either use AREVT, or the combination of AREVTO and AREVTE.

• The McASP generates transmit DMA events independently from receive DMA events; therefore, separate schemes can be used for transmit and receive DMA. For example, scenario 1 could be used for the transmit data (AXEVT) and scenario 2 could be used for the receive data (AREVTO, AREVTE), and conversely.

Note the difference between DMA event generation and the CPU interrupt generation. DMA events are generated automatically upon data ready; whereas CPU interrupt generation needs to be enabled in the XINTCTL/RINTCTL register.

In [Figure](#page-56-1) 22-37, scenario 2, each transmit DMA request is for data in the next time slot, while each receive DMA request is for data in the previous time slot. For example, [Figure](#page-57-0) 22-38 shows a circled AXEVTE event for an even time slot transmit DMA request. The transmitter always requests a DMA transfer for data it will need to transmit during the next time slot. So, in this example, the circled event AXEVTE is a request for data for samples LF2, LS2, and C2.

On the other hand, the circled AREVTE event is an even time slot receive DMA request. The receiver always requests a DMA transfer for data it received during the previous time slot. In this example, the circled event AREVTE is a request for samples LF1, LS1, and C1.


**Figure 22-38. DMA Events in an Audio Example**

### *22.3.15 Power Management*

The McASP can be placed in reduced power modes to conserve power during periods of low activity.

### *22.3.16 Emulation Considerations*

**NOTE:** The receive buffer registers (RBUF*n*) and transmit buffer registers (XBUF*n*) should not be accessed by the emulator when the McASP is running. Such an access will cause the RRDY/XRDY bit in the serializer control register *n* (SRCTL*n*) to be updated.

The McASP does not support the emulation suspend.
## **22.4 Registers**

### *22.4.1 MCASP Registers*

Table [22-11](#page-58-1) lists the memory-mapped registers for the MCASP. All register offset addresses not listed in Table [22-11](#page-58-1) should be considered as reserved locations and the register contents should not be modified.

**Table 22-11. MCASP Registers**

| Offset          | Acronym                | Register Name                                                       | Section           |
|-----------------|------------------------|---------------------------------------------------------------------|-------------------|
| 0h              | REV                    | Revision Identification Register                                    | Section 22.4.1.1  |
| 4h              | PWRIDLESYSCONFIG       | Power Idle SYSCONFIG Register                                       | Section 22.4.1.2  |
| 10h             | PFUNC                  | Pin Function Register                                               | Section 22.4.1.3  |
| 14h             | PDIR                   | Pin Direction Register                                              | Section 22.4.1.4  |
| 18h             | PDOUT                  | Pin Data Output Register                                            | Section 22.4.1.5  |
| 1Ch             | PDIN                   | Pin Data Input Register                                             | Section 22.4.1.6  |
| 20h             | PDCLR                  | Pin Data Clear Register                                             | Section 22.4.1.7  |
| 44h             | GBLCTL                 | Global Control Register                                             | Section 22.4.1.8  |
| 48h             | AMUTE                  | Audio Mute Control Register                                         | Section 22.4.1.9  |
| 4Ch             | DLBCTL                 | Digital Loopback Control Register                                   | Section 22.4.1.10 |
| 50h             | DITCTL                 | DIT Mode Control Register                                           | Section 22.4.1.11 |
| 60h             | RGBLCTL                | Receiver Global Control Register                                    | Section 22.4.1.12 |
| 64h             | RMASK                  | Receive Format Unit Bit Mask Register                               | Section 22.4.1.13 |
| 68h             | RFMT                   | Receive Bit Stream Format Register                                  | Section 22.4.1.14 |
| 6Ch             | AFSRCTL                | Receive Frame Sync Control Register                                 | Section 22.4.1.15 |
| 70h             | ACLKRCTL               | Receive Clock Control Register                                      | Section 22.4.1.16 |
| 74h             | AHCLKRCTL              | Receive High-Frequency Clock Control Register                       | Section 22.4.1.17 |
| 78h             | RTDM                   | Receive TDM Time Slot 0-31 Register                                 | Section 22.4.1.18 |
| 7Ch             | RINTCTL                | Receiver Interrupt Control Register                                 | Section 22.4.1.19 |
| 80h             | RSTAT                  | Receiver Status Register                                            | Section 22.4.1.20 |
| 84h             | RSLOT                  | Current Receive TDM Time Slot Register                              | Section 22.4.1.21 |
| 88h             | RCLKCHK                | Receive Clock Check Control Register                                | Section 22.4.1.22 |
| 8Ch             | REVTCTL                | Receiver DMA Event Control Register                                 | Section 22.4.1.23 |
| A0h             | XGBLCTL                | Transmitter Global Control Register                                 | Section 22.4.1.24 |
| A4h             | XMASK                  | Transmit Format Unit Bit Mask Register                              | Section 22.4.1.25 |
| A8h             | XFMT                   | Transmit Bit Stream Format Register                                 | Section 22.4.1.26 |
| ACh             | AFSXCTL                | Transmit Frame Sync Control Register                                | Section 22.4.1.27 |
| B0h             | ACLKXCTL               | Transmit Clock Control Register                                     | Section 22.4.1.28 |
| B4h             | AHCLKXCTL              | Transmit High-Frequency Clock Control Register                      | Section 22.4.1.29 |
| B8h             | XTDM                   | Transmit TDM Time Slot 0-31 Register                                | Section 22.4.1.30 |
| BCh             | XINTCTL                | Transmitter Interrupt Control Register                              | Section 22.4.1.31 |
| C0h             | XSTAT                  | Transmitter Status Register                                         | Section 22.4.1.32 |
| C4h             | XSLOT                  | Current Transmit TDM Time Slot Register                             | Section 22.4.1.33 |
| C8h             | XCLKCHK                | Transmit Clock Check Control Register                               | Section 22.4.1.34 |
| CCh             | XEVTCTL                | Transmitter DMA Event Control Register                              | Section 22.4.1.35 |
| 100h to<br>114h | DITCSRA_0 to DITCSRA_5 | Left (Even TDM Time Slot) Channel Status Registers<br>(DIT Mode)    | Section 22.4.1.36 |
| 118h to<br>12Ch | DITCSRB_0 to DITCSRB_5 | Right (Odd TDM Time Slot) Channel Status Registers<br>(DIT Mode)    | Section 22.4.1.37 |
| 130h to<br>144h | DITUDRA_0 to DITUDRA_5 | Left (Even TDM Time Slot) Channel User Data<br>Registers (DIT Mode) | Section 22.4.1.38 |
| 148h to         | DITUDRB_0 to DITUDRB_5 | Right (Odd TDM Time Slot) Channel User Data<br>Registers (DIT Mode) | Section 22.4.1.39 |
#### **Table 22-11. MCASP Registers (continued)**

| Offset          | Acronym            | Register Name                            | Section           |
|-----------------|--------------------|------------------------------------------|-------------------|
| 180h to<br>194h | SRCTL_0 to SRCTL_5 | Serializer Control Registers             | Section 22.4.1.40 |
| 200h to<br>214h | XBUF_0 to XBUF_5   | Transmit Buffer Register for Serializers | Section 22.4.1.41 |
| 280h to<br>294h | RBUF_0 to RBUF_5   | Receive Buffer Register for Serializers  | Section 22.4.1.42 |
| 1000h           | WFIFOCTL           | Write FIFO Control Register              | Section 22.4.1.43 |
| 1004h           | WFIFOSTS           | Write FIFO Status Register               | Section 22.4.1.44 |
| 1008h           | RFIFOCTL           | Read FIFO Control Register               | Section 22.4.1.45 |
| 100Ch           | RFIFOSTS           | Read FIFO Status Register                | Section 22.4.1.46 |
#### **22.4.1.1 REV Register (offset = 0h) [reset = 44307B02h]**

REV is shown in [Figure](#page-60-1) 22-39 and described in Table [22-12.](#page-60-2)

The revision identification register (REV) contains identification data for the peripheral.

### **Figure 22-39. REV Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-12. REV Register Field Descriptions**

| Bit  | Field | Type | Reset     | Description                        |
|------|-------|------|-----------|------------------------------------|
| 31-0 | REV   | R    | 44307B02h | Identifies revision of peripheral. |
#### **22.4.1.2 PWRIDLESYSCONFIG Register (offset = 4h) [reset = 2h]**

PWRIDLESYSCONFIG is shown in [Figure](#page-61-1) 22-40 and described in Table [22-13.](#page-61-2)

**Figure 22-40. PWRIDLESYSCONFIG Register**

| 31       | 30 | 29    | 28 | 27       | 26 | 25       | 24
|----------|----|-------|----|----------|----|----------|----|--|--
| RESERVED |    |       |    |          |    |          |
| R-0h     |    |       |    |          |    |          |
| 23       | 22 | 21    | 20 | 19       | 18 | 17       | 16
| RESERVED |    |       |    |          |    |          |
| R-0h     |    |       |    |          |    |          |
| 15       | 14 | 13    | 12 | 11       | 10 | 9        | 8
|          |    |       |    | RESERVED |    |          |
|          |    |       |    | R-0h     |    |          |
| 7        | 6  | 5     | 4  | 3        | 2  | 1        | 0
| RESERVED |    | Other |    |          |    | IDLEMODE |
| R-0h     |    |       |    | R/W-0h   |    | R/W-2h   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 22-13. PWRIDLESYSCONFIG Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                         |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--
| 31-6 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                     |
| 5-2  | Other    | R/W  | 0h    | Reserved for future programming.                                                                                                                                                                                                                    |
| 1-0  | IDLEMODE | R/W  | 2h    | Power management Configuration of the local target state<br>management mode.<br>By definition, target can handle read/write transaction as long as it is<br>out of IDLE state.                                                                      |
|      |          |      |       | 0h = Force-idle mode: local target's idle state follows (acknowledges)<br>the system's idle requests unconditionally, i.e. regardless of the IP<br>module's internal requirements. Backup mode, for debug only.                                     |
|      |          |      |       | 1h = No-idle mode: local target never enters idle state. Backup<br>mode, for debug only.                                                                                                                                                            |
|      |          |      |       | 2h = Smart-idle mode: local target's idle state eventually follows<br>(acknowledges) the system's idle requests, depending on the IP<br>module's internal requirements. IP module shall not generate (IRQ<br>or DMA-request-related) wakeup events. |
|      |          |      |       | 3h = Reserved.                                                                                                                                                                                                                                      |
#### **22.4.1.3 PFUNC Register (offset = 10h) [reset = 0h]**

PFUNC is shown in [Figure](#page-62-1) 22-41 and described in Table [22-14.](#page-62-2)

The pin function register (PFUNC) specifies the function of AXRn, ACLKX, AHCLKX, AFSX, ACLKR, AHCLKR, and AFSR pins as either a McASP pin or a general-purpose input/output (GPIO) pin. CAUTION: Writing a value other than 0 to reserved bits in this register may cause improper device operation.

**Figure 22-41. PFUNC Register**

| 31       | 30       | 29     | 28     | 27       | 26     | 25     | 24
|----------|----------|--------|--------|----------|--------|--------|----------|--|--
| AFSR     | AHCLKR   | ACLKR  | AFSX   | AHCLKX   | ACLKX  | AMUTE  | RESERVED
| R/W-0h   | R/W-0h   | R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R-0h
| 23       | 22       | 21     | 20     | 19       | 18     | 17     | 16
|          | RESERVED |        |        |          |        |        |
|          | R-0h     |        |        |          |        |        |
| 15       | 14       | 13     | 12     | 11       | 10     | 9      | 8
|          |          |        |        | RESERVED |        |        |
|          |          |        |        | R-0h     |        |        |
| 7        | 6        | 5      | 4      | 3        | 2      | 1      | 0
| RESERVED |          |        |        | AXR      |        |        |
| R-0h     |          |        |        |          | R/W-0h |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-14. PFUNC Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------|--|--
| 31   | AFSR     | R/W  | 0h    | Determines if AFSR pin functions as McASP or GPIO.<br>0h = Pin functions as McASP pin.<br>1h = Pin functions as GPIO pin.
| 30   | AHCLKR   | R/W  | 0h    | Determines if AHCLKR pin functions as McASP or GPIO.<br>0h = Pin functions as McASP pin.<br>1h = Pin functions as GPIO pin.
| 29   | ACLKR    | R/W  | 0h    | Determines if ACLKR pin functions as McASP or GPIO.<br>0h = Pin functions as McASP pin.<br>1h = Pin functions as GPIO pin.
| 28   | AFSX     | R/W  | 0h    | Determines if AFSX pin functions as McASP or GPIO.<br>0h = Pin functions as McASP pin.<br>1h = Pin functions as GPIO pin.
| 27   | AHCLKX   | R/W  | 0h    | Determines if AHCLKX pin functions as McASP or GPIO.<br>0h = Pin functions as McASP pin.<br>1h = Pin functions as GPIO pin.
| 26   | ACLKX    | R/W  | 0h    | Determines if ACLKX pin functions as McASP or GPIO.<br>0h = Pin functions as McASP pin.<br>1h = Pin functions as GPIO pin.
| 25   | AMUTE    | R/W  | 0h    | Determines if AMUTE pin functions as McASP or GPIO.<br>0h = Pin functions as McASP pin.<br>1h = Pin functions as GPIO pin.
| 24-4 | RESERVED | R    | 0h    |
| 3-0  | AXR      | R/W  | 0h    | Determines if AXRn pin functions as McASP or GPIO.<br>0h = Pin functions as McASP pin.<br>1h = Pin functions as GPIO pin.
#### **22.4.1.4 PDIR Register (offset = 14h) [reset = 0h]**

PDIR is shown in [Figure](#page-63-1) 22-42 and described in Table [22-15.](#page-63-2)

The pin direction register (PDIR) specifies the direction of AXRn, ACLKX, AHCLKX, AFSX, ACLKR, AHCLKR, and AFSR pins as either an input or an output pin. Regardless of the pin function register (PFUNC) setting, each PDIR bit must be set to 1 for the specified pin to be enabled as an output and each PDIR bit must be cleared to 0 for the specified pin to be an input. For example, if the McASP is configured to use an internally-generated bit clock and the clock is to be driven out to the system, the PFUNC bit must be cleared to 0 (McASP function) and the PDIR bit must be set to 1 (an output). When AXRn is configured to transmit, the PFUNC bit must be cleared to 0 (McASP function) and the PDIR bit must be set to 1 (an output). Similarly, when AXRn is configured to receive, the PFUNC bit must be cleared to 0 (McASP function) and the PDIR bit must be cleared to 0 (an input). CAUTION: Writing a value other than 0 to reserved bits in this register may cause improper device operation.

**Figure 22-42. PDIR Register**

| 31     | 30       | 29     | 28     | 27       | 26     | 25     | 24
|--------|----------|--------|--------|----------|--------|--------|----------|--|--
| AFSR   | AHCLKR   | ACLKR  | AFSX   | AHCLKX   | ACLKX  | AMUTE  | RESERVED
| R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R-0h
| 23     | 22       | 21     | 20     | 19       | 18     | 17     | 16
|        | RESERVED |        |        |          |        |        |
|        | R-0h     |        |        |          |        |        |
| 15     | 14       | 13     | 12     | 11       | 10     | 9      | 8
|        |          |        |        | RESERVED |        |        |
|        |          |        |        | R-0h     |        |        |
| 7      | 6        | 5      | 4      | 3        | 2      | 1      | 0
|        | RESERVED |        |        |          | AXR    |        |
| R-0h   |          |        |        | R/W-0h   |        |        |
|        |          |        |        |          |        |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-15. PDIR Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description
|------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------|--|--
| 31   | AFSR     | R/W  | 0h    | Determines if AFSR pin functions as an input or output.<br>0h = Pin functions as input.<br>1h = Pin functions as output.
| 30   | AHCLKR   | R/W  | 0h    | Determines if AHCLKR pin functions as an input or output.<br>0h = Pin functions as input.<br>1h = Pin functions as output.
| 29   | ACLKR    | R/W  | 0h    | Determines if ACLKR pin functions as an input or output.<br>0h = Pin functions as input.<br>1h = Pin functions as output.
| 28   | AFSX     | R/W  | 0h    | Determines if AFSX pin functions as an input or output.<br>0h = Pin functions as input.<br>1h = Pin functions as output.
| 27   | AHCLKX   | R/W  | 0h    | Determines if AHCLKX pin functions as an input or output.<br>0h = Pin functions as input.<br>1h = Pin functions as output.
| 26   | ACLKX    | R/W  | 0h    | Determines if ACLKX pin functions as an input or output.<br>0h = Pin functions as input.<br>1h = Pin functions as output.
| 25   | AMUTE    | R/W  | 0h    | Determines if AMUTE pin functions as an input or output.<br>0h = Pin functions as input.<br>1h = Pin functions as output.
| 24-4 | RESERVED | R    | 0h    |
**Table 22-15. PDIR Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                                             |
|-----|-------|------|-------|---------------------------------------------------------|
| 3-0 | AXR   | R/W  | 0h    | Determines if AXRn pin functions as an input or output. |
|     |       |      |       | 0h = Pin functions as input.                            |
|     |       |      |       | 1h = Pin functions as output.                           |
#### **22.4.1.5 PDOUT Register (offset = 18h) [reset = 0h]**

PDOUT is shown in [Figure](#page-65-1) 22-43 and described in Table [22-16](#page-65-2).

The pin data output register (PDOUT) holds a value for data out at all times, and may be read back at all times. The value held by PDOUT is not affected by writing to PDIR and PFUNC. However, the data value in PDOUT is driven out onto the McASP pin only if the corresponding bit in PFUNC is set to 1 (GPIO function) and the corresponding bit in PDIR is set to 1 (output). When reading data, returns the corresponding bit value in PDOUT[n], does not return input from I/O pin; when writing data, writes to the corresponding PDOUT[n] bit. PDOUT has these aliases or alternate addresses: PDSET When written to at this address, writing a 1 to a bit in PDSET sets the corresponding bit in PDOUT to 1; writing a 0 has no effect and keeps the bits in PDOUT unchanged. PDCLR When written to at this address, writing a 1 to a bit in PDCLR clears the corresponding bit in PDOUT to 0; writing a 0 has no effect and keeps the bits in PDOUT unchanged. There is only one set of data out bits, PDOUT[31-0]. The other registers, PDSET and PDCLR, are just different addresses for the same control bits, with different behaviors during writes. CAUTION: Writing a value other than 0 to reserved bits in this register may cause improper device operation.

**Figure 22-43. PDOUT Register**

| 31     | 30       | 29     | 28     | 27       | 26     | 25     | 24       |
|--------|----------|--------|--------|----------|--------|--------|----------|--|--|--
| AFSR   | AHCLKR   | ACLKR  | AFSX   | AHCLKX   | ACLKX  | AMUTE  | RESERVED |
| R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R-0h     |
| 23     | 22       | 21     | 20     | 19       | 18     | 17     | 16       |
|        | RESERVED |        |        |          |        |        |          |
|        | R-0h     |        |        |          |        |        |          |
| 15     | 14       | 13     | 12     | 11       | 10     | 9      | 8        |
|        |          |        |        | RESERVED |        |        |          |
|        |          |        |        | R-0h     |        |        |          |
| 7      | 6        | 5      | 4      | 3        | 2      | 1      | 0        |
|        | RESERVED |        |        | AXR      |        |        |          |
| R-0h   |          |        |        | R/W-0h   |        |        |          |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-16. PDOUT Register Field Descriptions**

| Bit | Field  | Type | Reset | Description                                                                                               |
|-----|--------|------|-------|-----------------------------------------------------------------------------------------------------------|
| 31  | AFSR   | R/W  | 0h    | Determines drive on AFSR output pin when the corresponding<br>PFUNC[31] and PDIR[31] bits are set to 1.   |
|     |        |      |       | 0h = Pin drives low.                                                                                      |
|     |        |      |       | 1h = Pin drives high.                                                                                     |
| 30  | AHCLKR | R/W  | 0h    | Determines drive on AHCLKR output pin when the corresponding<br>PFUNC[30] and PDIR[30] bits are set to 1. |
|     |        |      |       | 0h = Pin drives low.                                                                                      |
|     |        |      |       | 1h = Pin drives high.                                                                                     |
| 29  | ACLKR  | R/W  | 0h    | Determines drive on ACLKR output pin when the corresponding<br>PFUNC[29] and PDIR[29] bits are set to 1.  |
|     |        |      |       | 0h = Pin drives low.                                                                                      |
|     |        |      |       | 1h = Pin drives high.                                                                                     |
| 28  | AFSX   | R/W  | 0h    | Determines drive on AFSX output pin when the corresponding<br>PFUNC[28] and PDIR[28] bits are set to 1.   |
|     |        |      |       | 0h = Pin drives low.                                                                                      |
|     |        |      |       | 1h = Pin drives high.                                                                                     |
| 27  | AHCLKX | R/W  | 0h    | Determines drive on AHCLKX output pin when the corresponding<br>PFUNC[27] and PDIR[27] bits are set to 1. |
|     |        |      |       | 0h = Pin drives low.                                                                                      |
|     |        |      |       | 1h = Pin drives high.                                                                                     |
**Table 22-16. PDOUT Register Field Descriptions (continued)**

| Bit  | Field    | Type | Reset | Description                                                                                                                                               |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------|
| 26   | ACLKX    | R/W  | 0h    | Determines drive on ACLKX output pin when the corresponding<br>PFUNC[26] and PDIR[26] bits are set to 1.<br>0h = Pin drives low.<br>1h = Pin drives high. |
| 25   | AMUTE    | R/W  | 0h    | Determines drive on AMUTE output pin when the corresponding<br>PFUNC[25] and PDIR[25] bits are set to 1.<br>0h = Pin drives low.<br>1h = Pin drives high. |
| 24-4 | RESERVED | R    | 0h    |                                                                                                                                                           |
| 3-0  | AXR      | R/W  | 0h    | Determines drive on AXR[n] output pin when the corresponding<br>PFUNC[n] and PDIR[n] bits are set to 1.<br>0h = Pin drives low.<br>1h = Pin drives high.  |
#### **22.4.1.6 PDIN Register (offset = 1Ch) [reset = 0h]**

PDIN is shown in [Figure](#page-67-1) 22-44 and described in Table [22-17.](#page-67-2)

The pin data input register (PDIN) holds the I/O pin state of each of the McASP pins. PDIN allows the actual value of the pin to be read, regardless of the state of PFUNC and PDIR. The value after reset for registers 1 through 15 and 24 through 31 depends on how the pins are being driven. CAUTION: Writing a value other than 0 to reserved bits in this register may cause improper device operation.

**Figure 22-44. PDIN Register**

| 31     | 30       | 29     | 28     | 27       | 26     | 25     | 24       |
|--------|----------|--------|--------|----------|--------|--------|----------|--|--|--
| AFSR   | AHCLKR   | ACLKR  | AFSX   | AHCLKX   | ACLKX  | AMUTE  | RESERVED |
| R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R-0h     |
| 23     | 22       | 21     | 20     | 19       | 18     | 17     | 16       |
|        | RESERVED |        |        |          |        |        |          |
|        | R-0h     |        |        |          |        |        |          |
| 15     | 14       | 13     | 12     | 11       | 10     | 9      | 8        |
|        |          |        |        | RESERVED |        |        |          |
|        |          |        |        | R-0h     |        |        |          |
| 7      | 6        | 5      | 4      | 3        | 2      | 1      | 0        |
|        | RESERVED |        |        |          | AXR    |        |          |
| R-0h   |          |        |        | R/W-0h   |        |        |          |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-17. PDIN Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                     |
|------|----------|------|-------|---------------------------------------------------------------------------------|
| 31   | AFSR     | R/W  | 0h    | Logic level on AFSR pin.<br>0h = Pin is logic low.<br>1h = Pin is logic high.   |
| 30   | AHCLKR   | R/W  | 0h    | Logic level on AHCLKR pin.<br>0h = Pin is logic low.<br>1h = Pin is logic high. |
| 29   | ACLKR    | R/W  | 0h    | Logic level on ACLKR pin.<br>0h = Pin is logic low.<br>1h = Pin is logic high.  |
| 28   | AFSX     | R/W  | 0h    | Logic level on AFSX pin.<br>0h = Pin is logic low.<br>1h = Pin is logic high.   |
| 27   | AHCLKX   | R/W  | 0h    | Logic level on AHCLKX pin.<br>0h = Pin is logic low.<br>1h = Pin is logic high. |
| 26   | ACLKX    | R/W  | 0h    | Logic level on ACLKX pin.<br>0h = Pin is logic low.<br>1h = Pin is logic high.  |
| 25   | AMUTE    | R/W  | 0h    | Logic level on AMUTE pin.<br>0h = Pin is logic low.<br>1h = Pin is logic high.  |
| 24-4 | RESERVED | R    | 0h    |                                                                                 |
| 3-0  | AXR      | R/W  | 0h    | Logic level on AXR[n] pin.<br>0h = Pin is logic low.<br>1h = Pin is logic high. |
#### **22.4.1.7 PDCLR Register (offset = 20h) [reset = 0h]**

PDCLR is shown in [Figure](#page-68-1) 22-45 and described in Table [22-18.](#page-68-2)

The pin data clear register (PDCLR) is an alias of the pin data output register (PDOUT) for writes only. Writing a 1 to the PDCLR bit clears the corresponding bit in PDOUT and, if PFUNC = 1 (GPIO function) and PDIR = 1 (output), drives a logic low on the pin. PDCLR is useful for a multitasking system because it allows you to clear to a logic low only the desired pin(s) within a system without affecting other I/O pins controlled by the same McASP. CAUTION: Writing a value other than 0 to reserved bits in this register may cause improper device operation.

### **Figure 22-45. PDCLR Register**

| 31     | 30       | 29     | 28     | 27       | 26     | 25     | 24
|--------|----------|--------|--------|----------|--------|--------|----------|--|--
| AFSR   | AHCLKR   | ACLKR  | AFSX   | AHCLKX   | ACLKX  | AMUTE  | RESERVED
| R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R/W-0h   | R/W-0h | R/W-0h | R-0h
| 23     | 22       | 21     | 20     | 19       | 18     | 17     | 16
|        | RESERVED |        |        |          |        |        |
|        | R-0h     |        |        |          |        |        |
| 15     | 14       | 13     | 12     | 11       | 10     | 9      | 8
|        |          |        |        | RESERVED |        |        |
|        |          |        |        | R-0h     |        |        |
| 7      | 6        | 5      | 4      | 3        | 2      | 1      | 0
|        | RESERVED |        |        |          | AXR    |        |
| R-0h   |          |        |        | R/W-0h   |        |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-18. PDCLR Register Field Descriptions**

| Bit | Field  | Type | Reset | Description                                                                                                                                                                                          |
|-----|--------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31  | AFSR   | R/W  | 0h    | Allows the corresponding AFSR bit in PDOUT to be cleared to a<br>logic low without affecting other I/O pins controlled by the same port.<br>0h = No effect.<br>1h = PDOUT[31] bit is cleared to 0.   |
| 30  | AHCLKR | R/W  | 0h    | Allows the corresponding AHCLKR bit in PDOUT to be cleared to a<br>logic low without affecting other I/O pins controlled by the same port.<br>0h = No effect.<br>1h = PDOUT[30] bit is cleared to 0. |
| 29  | ACLKR  | R/W  | 0h    | Allows the corresponding ACLKR bit in PDOUT to be cleared to a<br>logic low without affecting other I/O pins controlled by the same port.<br>0h = No effect.<br>1h = PDOUT[29] bit is cleared to 0.  |
| 28  | AFSX   | R/W  | 0h    | Allows the corresponding AFSX bit in PDOUT to be cleared to a<br>logic low without affecting other I/O pins controlled by the same port.<br>0h = No effect.<br>1h = PDOUT[28] bit is cleared to 0.   |
| 27  | AHCLKX | R/W  | 0h    | Allows the corresponding AHCLKX bit in PDOUT to be cleared to a<br>logic low without affecting other I/O pins controlled by the same port.<br>0h = No effect.<br>1h = PDOUT[27] bit is cleared to 0. |
| 26  | ACLKX  | R/W  | 0h    | Allows the corresponding ACLKX bit in PDOUT to be cleared to a<br>logic low without affecting other I/O pins controlled by the same port.<br>0h = No effect.<br>1h = PDOUT[26] bit is cleared to 0.  |
**Table 22-18. PDCLR Register Field Descriptions (continued)**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                         |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 25   | AMUTE    | R/W  | 0h    | Allows the corresponding AMUTE bit in PDOUT to be cleared to a<br>logic low without affecting other I/O pins controlled by the same port.<br>0h = No effect.<br>1h = PDOUT[25] bit is cleared to 0. |
| 24-4 | RESERVED | R    | 0h    |                                                                                                                                                                                                     |
| 3-0  | AXR      | R/W  | 0h    | Allows the corresponding AXR[n] bit in PDOUT to be cleared to a<br>logic low without affecting other I/O pins controlled by the same port.<br>0h = No effect.<br>1h = PDOUT[n] bit is cleared to 0. |
#### **22.4.1.8 GBLCTL Register (offset = 44h) [reset = 0h]**

GBLCTL is shown in [Figure](#page-70-1) 22-46 and described in Table [22-19](#page-70-2).

The global control register (GBLCTL) provides initialization of the transmit and receive sections. The bit fields in GBLCTL are synchronized and latched by the corresponding clocks (ACLKX for bits 12-8 and ACLKR for bits 4-0). Before GBLCTL is programmed, you must ensure that serial clocks are running. If the corresponding external serial clocks, ACLKX and ACLKR, are not yet running, you should select the internal serial clock source in AHCLKXCTL, AHCLKRCTL, ACLKXCTL, and ACLKRCTL before GBLCTL is programmed. Also, after programming any bits in GBLCTL you should not proceed until you have read back from GBLCTL and verified that the bits are latched in GBLCTL.

**Figure 22-46. GBLCTL Register**

| 31 | 30       | 29 | 28     | 27     | 26     | 25       | 24      |
|----|----------|----|--------|--------|--------|----------|---------|--
|    | RESERVED |    |        |        |        |          |         |
|    |          |    |        | R-0h   |        |          |         |
| 23 | 22       | 21 | 20     | 19     | 18     | 17       | 16      |
|    | RESERVED |    |        |        |        |          |         |
|    | R-0h     |    |        |        |        |          |         |
| 15 | 14       | 13 | 12     | 11     | 10     | 9        | 8       |
|    | RESERVED |    | XFRST  | XSMRST | XSRCLR | XHCLKRST | XCLKRST |
|    | R-0h     |    | R/W-0h | R/W-0h | R/W-0h | R/W-0h   | R/W-0h  |
| 7  | 6        | 5  | 4      | 3      | 2      | 1        | 0       |
|    | RESERVED |    | RFRST  | RSMRST | RSRCLR | RHCLKRST | RCLKRST |
|    | R-0h     |    | R/W-0h | R/W-0h | R/W-0h | R/W-0h   | R/W-0h  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-19. GBLCTL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-13 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| 12    | XFRST    | R/W  | 0h    | Transmit frame sync generator reset enable bit.<br>0h = Transmit frame sync generator is reset.<br>1h = Transmit frame sync generator is active. When released from<br>reset, the transmit frame sync generator begins counting serial<br>clocks and generating frame sync as programmed.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 11    | XSMRST   | R/W  | 0h    | Transmit state machine reset enable bit.<br>0h = Transmit state machine is held in reset. AXRn pin state: If<br>PFUNC[n] = 0 and PDIR[n] = 1; then the serializer drives the AXRn<br>pin to the state specified for inactive time slot (as determined by<br>DISMOD bits in SRCTL).<br>1h = Transmit state machine is released from reset. When released<br>from reset, the transmit state machine immediately transfers data<br>from XRBUF[n] to XRSR[n]. The transmit state machine sets the<br>underrun flag (XUNDRN) in XSTAT, if XRBUF[n] have not been<br>preloaded with data before reset is released. The transmit state<br>machine also immediately begins detecting frame sync and is ready<br>to transmit. Transmit TDM time slot begins at slot 0 after reset is<br>released. |
| 10    | XSRCLR   | R/W  | 0h    | Transmit serializer clear enable bit.<br>By clearing then setting this bit, the transmit buffer is flushed to an<br>empty state (XDATA = 1).<br>If XSMRST = 1, XSRCLR = 1, XDATA = 1, and XBUF is not loaded<br>with new data before the start of the next active time slot, an<br>underrun will occur.<br>0h = Transmit serializers are cleared.<br>1h = Transmit serializers are active. When the transmit serializers<br>are first taken out of reset (XSRCLR changes from 0 to 1), the<br>transmit data ready bit (XDATA) in XSTAT is set to indicate XBUF is<br>ready to be written.                                                                                                                                                                                                |
**Table 22-19. GBLCTL Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset                                                                                                                                                                                                                                                                                                                                             | Description
|-----|----------|------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| 9   | XHCLKRST | R/W  | 0h                                                                                                                                                                                                                                                                                                                                                | Transmit high-frequency clock divider reset enable bit.<br>0h = Transmit high-frequency clock divider is held in reset and<br>passes through its input as divide-by-1.<br>1h = Transmit high-frequency clock divider is running.
| 8   | XCLKRST  | R/W  | 0h<br>Transmit clock divider reset enable bit.<br>0h = Transmit clock divider is held in reset. When the clock divider is<br>in reset, it passes through a divide-by-1 of its input.<br>1h = Transmit clock divider is running.                                                                                                                   |
| 7-5 | RESERVED | R    | 0h                                                                                                                                                                                                                                                                                                                                                |
| 4   | RFRST    | R/W  | 0h                                                                                                                                                                                                                                                                                                                                                | Receive frame sync generator reset enable bit.<br>0h = Receive frame sync generator is reset.<br>1h = Receive frame sync generator is active. When released from<br>reset, the receive frame sync generator begins counting serial clocks<br>and generating frame sync as programmed.
| 3   | RSMRST   | R/W  | 0h<br>Receive state machine reset enable bit.<br>0h = Receive state machine is held in reset.<br>1h = Receive state machine is released from reset. When released<br>from reset, the receive state machine immediately begins detecting<br>frame sync and is ready to receive. Receive TDM time slot begins at<br>slot 0 after reset is released. |
| 2   | RSRCLR   | R/W  | 0h                                                                                                                                                                                                                                                                                                                                                | Receive serializer clear enable bit.<br>By clearing then setting this bit, the receive buffer is flushed.<br>0h = Receive serializers are cleared.<br>1h = Receive serializers are active.
| 1   | RHCLKRST | R/W  | 0h                                                                                                                                                                                                                                                                                                                                                | Receive high-frequency clock divider reset enable bit.<br>0h = Receive high-frequency clock divider is held in reset and<br>passes through its input as divide-by-1.<br>1h = Receive high-frequency clock divider is running.
| 0   | RCLKRST  | R/W  | 0h                                                                                                                                                                                                                                                                                                                                                | Receive high-frequency clock divider reset enable bit.<br>0h = Receive clock divider is held in reset. When the clock divider is<br>in reset, it passes through a divide-by-1 of its input.<br>1h = Receive clock divider is running.
#### **22.4.1.9 AMUTE Register (offset = 48h) [reset = 0h]**

AMUTE is shown in [Figure](#page-72-1) 22-47 and described in Table [22-20](#page-72-2).

The audio mute control register (AMUTE) controls the McASP audio mute (AMUTE) output pin. The value after reset for register 4 depends on how the pins are being driven.

**Figure 22-47. AMUTE Register**

| 31       | 30       | 29     | 28      | 27      | 26      | 25      | 24       |
|----------|----------|--------|---------|---------|---------|---------|----------|--
|          | RESERVED |        |         |         |         |         |          |
|          | R-0h     |        |         |         |         |         |          |
| 23       | 22       | 21     | 20      | 19      | 18      | 17      | 16       |
|          | RESERVED |        |         |         |         |         |          |
|          | R-0h     |        |         |         |         |         |          |
| 15       | 14       | 13     | 12      | 11      | 10      | 9       | 8        |
|          | RESERVED |        | XDMAERR | RDMAERR | XCKFAIL | RCKFAIL | XSYNCERR |
|          | R-0h     |        | R/W-0h  | R/W-0h  | R/W-0h  | R/W-0h  | R/W-0h   |
| 7        | 6        | 5      | 4       | 3       | 2       | 1       | 0        |
| RSYNCERR | XUNDRN   | ROVRN  | INSTAT  | INEN    | INPOL   |         | MUTEN    |
| R/W-0h   | R/W-0h   | R/W-0h | R-0h    | R/W-0h  | R/W-0h  |         | R/W-0h   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-20. AMUTE Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                         |
|-------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-13 | RESERVED | R    | 0h    |                                                                                                                                                     |
| 12    | XDMAERR  | R/W  | 0h    | If transmit DMA error (XDMAERR), drive AMUTE active enable bit.                                                                                     |
|       |          |      |       | 0h = Drive is disabled. Detection of transmit DMA error is ignored by<br>AMUTE.                                                                     |
|       |          |      |       | 1h = Drive is enabled (active). Upon detection of transmit DMA error,<br>AMUTE is active and is driven according to MUTEN bit.                      |
| 11    | RDMAERR  | R/W  | 0h    | If receive DMA error (RDMAERR), drive AMUTE active enable bit.                                                                                      |
|       |          |      |       | 0h = Drive is disabled. Detection of receive DMA error is ignored by<br>AMUTE.                                                                      |
|       |          |      |       | 1h = Drive is enabled (active). Upon detection of receive DMA error,<br>AMUTE is active and is driven according to MUTEN bit.                       |
| 10    | XCKFAIL  | R/W  | 0h    | If transmit clock failure (XCKFAIL), drive AMUTE active enable bit.                                                                                 |
|       |          |      |       | 0h = Drive is disabled. Detection of transmit clock failure is ignored<br>by AMUTE.                                                                 |
|       |          |      |       | 1h = Drive is enabled (active). Upon detection of transmit clock<br>failure, AMUTE is active and is driven according to MUTEN bit                   |
| 9     | RCKFAIL  | R/W  | 0h    | If receive clock failure (RCKFAIL), drive AMUTE active enable bit.                                                                                  |
|       |          |      |       | 0h = Drive is disabled. Detection of receive clock failure is ignored by<br>AMUTE.                                                                  |
|       |          |      |       | 1h = Drive is enabled (active). Upon detection of receive clock<br>failure, AMUTE is active and is driven according to MUTEN bit.                   |
| 8     | XSYNCERR | R/W  | 0h    | If unexpected transmit frame sync error (XSYNCERR), drive AMUTE<br>active enable bit.                                                               |
|       |          |      |       | 0h = Drive is disabled. Detection of unexpected transmit frame sync<br>error is ignored by AMUTE.                                                   |
|       |          |      |       | 1h = Drive is enabled (active). Upon detection of unexpected<br>transmit frame sync error, AMUTE is active and is driven according<br>to MUTEN bit. |
#### **Table 22-20. AMUTE Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset                                                                                                                                                                                                                                                                                                | Description                                                                                                                                                                                                                                                                                                                                    |
|-----|----------|------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7   | RSYNCERR | R/W  | 0h                                                                                                                                                                                                                                                                                                   | If unexpected receive frame sync error (RSYNCERR), drive AMUTE<br>active enable bit.<br>0h = Drive is disabled. Detection of unexpected receive frame sync<br>error is ignored by AMUTE.<br>1h = Drive is enabled (active). Upon detection of unexpected receive<br>frame sync error, AMUTE is active and is driven according to<br>MUTEN bit. |
| 6   | XUNDRN   | R/W  | 0h                                                                                                                                                                                                                                                                                                   | If transmit underrun error (XUNDRN), drive AMUTE active enable<br>bit.<br>0h = Drive is disabled. Detection of transmit underrun error is<br>ignored by AMUTE.<br>1h = Drive is enabled (active). Upon detection of transmit underrun<br>error, AMUTE is active and is driven according to MUTEN bit.                                          |
| 5   | ROVRN    | R/W  | 0h<br>If receiver overrun error (ROVRN), drive AMUTE active enable bit.<br>0h = Drive is disabled. Detection of receiver overrun error is ignored<br>by AMUTE.<br>1h = Drive is enabled (active). Upon detection of receiver overrun<br>error, AMUTE is active and is driven according to MUTEN bit. |                                                                                                                                                                                                                                                                                                                                                |
| 4   | INSTAT   | R    | 0h                                                                                                                                                                                                                                                                                                   | Determines drive on AXRn pin when PFUNC[n] and PDIR[n] bits are<br>set to 1.<br>0h = AMUTEIN pin is inactive.<br>1h = AMUTEIN pin is active. Audio mute in error is detected.                                                                                                                                                                  |
| 3   | INEN     | R/W  | 0h                                                                                                                                                                                                                                                                                                   | Drive AMUTE active when AMUTEIN error is active (INSTAT = 1).<br>0h = Drive is disabled. AMUTEIN is ignored by AMUTE.<br>1h = Drive is enabled (active). INSTAT = 1 drives AMUTE active.                                                                                                                                                       |
| 2   | INPOL    | R/W  | 0h                                                                                                                                                                                                                                                                                                   | Audio mute in (AMUTEIN) polarity select bit.<br>0h = Polarity is active high. A high on AMUTEIN sets INSTAT to 1.<br>1h = Polarity is active low. A low on AM UTEIN sets INSTAT to 1.                                                                                                                                                          |
| 1-0 | MUTEN    | R/W  | 0h                                                                                                                                                                                                                                                                                                   | AMUTE pin enable bit (unless overridden by GPIO registers).<br>0h = AMUTE pin is disabled, pin goes to tri-state condition.<br>1h = AMUTE pin is driven high if error is detected.<br>2h = AMUTE pin is driven low if error is detected.<br>3h = Reserved                                                                                      |
#### **22.4.1.10 DLBCTL Register (offset = 4Ch) [reset = 0h]**

DLBCTL is shown in [Figure](#page-74-1) 22-48 and described in Table [22-21](#page-74-2).

The digital loopback control register (DLBCTL) controls the internal loopback settings of the McASP in TDM mode.

**Figure 22-48. DLBCTL Register**

| 31   | 30       | 29 | 28       | 27 | 26     | 25     | 24
|------|----------|----|----------|----|--------|--------|--------
|      |          |    | RESERVED |    |        |        |
|      | R-0h     |    |          |    |        |        |
| 23   | 22       | 21 | 20       | 19 | 18     | 17     | 16
|      | RESERVED |    |          |    |        |        |
| R-0h |          |    |          |    |        |        |
| 15   | 14       | 13 | 12       | 11 | 10     | 9      | 8
|      |          |    | RESERVED |    |        |        |
|      |          |    | R-0h     |    |        |        |
| 7    | 6        | 5  | 4        | 3  | 2      | 1      | 0
|      | RESERVED |    |          |    | MODE   | ORD    | DLBEN
| R-0h |          |    |          |    | R/W-0h | R/W-0h | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 22-21. DLBCTL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                            |
|------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-4 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                        |
| 3-2  | MODE     | R/W  | 0h    | Loopback generator mode bits.<br>Applies only when loopback mode is enabled (DLBEN = 1).<br>0h = Default and reserved on loopback mode (DLBEN = 1). When in<br>non-loopback mode (DLBEN = 0), MODE should be left at default<br>(00). When in loopback mode (DLBEN = 1), MODE = 00 is reserved<br>and is not applicable.<br>1h = Transmit clock and frame sync generators used by both |
|      |          |      |       | transmit and receive sections. When in loopback mode (DLBEN = 1),<br>MODE must be 01.<br>2h = Reserved.<br>3h = Reserved.                                                                                                                                                                                                                                                              |
| 1    | ORD      | R/W  | 0h    | Loopback order bit when loopback mode is enabled (DLBEN = 1).<br>0h = Odd serializers N + 1 transmit to even serializers N that<br>receive. The corresponding serializers must be programmed<br>properly.<br>1h = Even serializers N transmit to odd serializers N + 1 that receive.<br>The corresponding serializers must be programmed properly.                                     |
| 0    | DLBEN    | R/W  | 0h    | Loopback mode enable bit.<br>0h = Loopback mode is disabled.<br>1h = Loopback mode is enabled.                                                                                                                                                                                                                                                                                         |
#### **22.4.1.11 DITCTL Register (offset = 50h) [reset = 0h]**

DITCTL is shown in [Figure](#page-75-1) 22-49 and described in Table [22-22](#page-75-2).

The DIT mode control register (DITCTL) controls DIT operations of the McASP.

**Figure 22-49. DITCTL Register**

| 31       | 30       | 29 | 28 | 27       | 26     | 25       | 24
|----------|----------|----|----|----------|--------|----------|--------
|          |          |    |    | RESERVED |        |          |
|          | R-0h     |    |    |          |        |          |
| 23       | 22       | 21 | 20 | 19       | 18     | 17       | 16
| RESERVED |          |    |    |          |        |          |
| R-0h     |          |    |    |          |        |          |
| 15       | 14       | 13 | 12 | 11       | 10     | 9        | 8
|          |          |    |    | RESERVED |        |          |
|          |          |    |    | R-0h     |        |          |
| 7        | 6        | 5  | 4  | 3        | 2      | 1        | 0
|          | RESERVED |    |    |          | VA     | RESERVED | DITEN
|          | R-0h     |    |    | R/W-0h   | R/W-0h | R-0h     | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-22. DITCTL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                           |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-4 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                       |
| 3    | VB       | R/W  | 0h    | Valid bit for odd time slots (DIT right subframe).<br>0h = V bit is 0 during odd DIT subframes.<br>1h = V bit is 1 during odd DIT subframes.                                                                                                                                                                                                                                                          |
| 2    | VA       | R/W  | 0h    | Valid bit for even time slots (DIT left subframe).<br>0h = V bit is 0 during even DIT subframes.<br>1h = V bit is 1 during even DIT subframes.                                                                                                                                                                                                                                                        |
| 1    | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                       |
| 0    | DITEN    | R/W  | 0h    | DIT mode enable bit.<br>DITEN should only be changed while the XSMRST bit in GBLCTL is<br>in reset (and for startup, XSRCLR also in reset).<br>However, it is not necessary to reset the XCLKRST or XHCLKRST<br>bits in GBLCTL to change DITEN.<br>0h = DIT mode is disabled. Transmitter operates in TDM or burst<br>mode.<br>1h = DIT mode is enabled. Transmitter operates in DIT encoded<br>mode. |
#### **22.4.1.12 RGBLCTL Register (offset = 60h) [reset = 0h]**

RGBLCTL is shown in [Figure](#page-76-1) 22-50 and described in Table [22-23](#page-76-2).

Alias of the global control register (GBLCTL). Writing to the receiver global control register (RGBLCTL) affects only the receive bits of GBLCTL (bits 4-0). Reads from RGBLCTL return the value of GBLCTL. RGBLCTL allows the receiver to be reset independently from the transmitter.

**Figure 22-50. RGBLCTL Register**

| 31   | 30       | 29 | 28     | 27       | 26     | 25       | 24
|------|----------|----|--------|----------|--------|----------|---------
|      |          |    |        | RESERVED |        |          |
|      | R-0h     |    |        |          |        |          |
| 23   | 22       | 21 | 20     | 19       | 18     | 17       | 16
|      | RESERVED |    |        |          |        |          |
| R-0h |          |    |        |          |        |          |
| 15   | 14       | 13 | 12     | 11       | 10     | 9        | 8
|      | RESERVED |    | XFRST  | XSMRST   | XSRCLR | XHCLKRST | XCLKRST
|      | R-0h     |    | R-0h   | R-0h     | R-0h   | R-0h     | R-0h
| 7    | 6        | 5  | 4      | 3        | 2      | 1        | 0
|      | RESERVED |    | RFRST  | RSMRST   | RSRCLR | RHCLKRST | RCLKRST
|      | R-0h     |    | R/W-0h | R/W-0h   | R/W-0h | R/W-0h   | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-23. RGBLCTL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                            |
|-------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-13 | RESERVED | R    | 0h    |                                                                                                                                                                                                        |
| 12    | XFRST    | R    | 0h    | Transmit frame sync generator reset enable bit.<br>A read of this bit returns the XFRST bit value of GBLCTL.<br>Writes have no effect.                                                                 |
| 11    | XSMRST   | R    | 0h    | Transmit state machine reset enable bit.<br>A read of this bit returns the XSMRST bit value of GBLCTL.<br>Writes have no effect.                                                                       |
| 10    | XSRCLR   | R    | 0h    | Transmit serializer clear enable bit.<br>A read of this bit returns the XSRCLR bit value of GBLCTL.<br>Writes have no effect.                                                                          |
| 9     | XHCLKRST | R    | 0h    | Transmit high-frequency clock divider reset enable bit.<br>A read of this bit returns the XHCLKRST bit value of GBLCTL.<br>Writes have no effect.                                                      |
| 8     | XCLKRST  | R    | 0h    | Transmit clock divider reset enable bit.<br>A read of this bit returns the XCLKRST bit value of GBLCTL.<br>Writes have no effect.                                                                      |
| 7-5   | RESERVED | R    | 0h    |                                                                                                                                                                                                        |
| 4     | RFRST    | R/W  | 0h    | Receive frame sync generator reset enable bit.<br>A write to this bit affects the RFRST bit of GBLCTL.<br>0h = Receive frame sync generator is reset.<br>1h = Receive frame sync generator is active.  |
| 3     | RSMRST   | R/W  | 0h    | Receive state machine reset enable bit.<br>A write to this bit affects the RSMRST bit of GBLCTL.<br>0h = Receive state machine is held in reset.<br>1h = Receive state machine is released from reset. |
| 2     | RSRCLR   | R/W  | 0h    | Receive serializer clear enable bit.<br>A write to this bit affects the RSRCLR bit of GBLCTL.<br>0h = Receive serializers are cleared.<br>1h = Receive serializers are active.                         |
#### **Table 22-23. RGBLCTL Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset | Description                                                                                                                                                                                 |
|-----|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1   | RHCLKRST | R/W  | 0h    | Receive high-frequency clock divider reset enable bit.<br>A write to this bit affects the RHCLKRST bit of GBLCTL.                                                                           |
|     |          |      |       | 0h = Receive high-frequency clock divider is held in reset and<br>passes through its input as divide-by-1.                                                                                  |
|     |          |      |       | 1h = Receive high-frequency clock divider is running.                                                                                                                                       |
| 0   | RCLKRST  | R/W  | 0h    | Receive clock divider reset enable bit.<br>A write to this bit affects the RCLKRST bit of GBLCTL.<br>0h = Receive clock divider is held in reset.<br>1h = Receive clock divider is running. |
#### **22.4.1.13 RMASK Register (offset = 64h) [reset = 0h]**

RMASK is shown in [Figure](#page-78-1) 22-51 and described in Table [22-24.](#page-78-2)

The receive format unit bit mask register (RMASK) determines which bits of the received data are masked off and padded with a known value before being read by the CPU or DMA.

**Figure 22-51. RMASK Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-24. RMASK Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                |
|------|-------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RMASK | R/W  | 0h    | Receive data mask n enable bit.                                                                                                                                                            |
|      |       |      |       | 0h = Corresponding bit of receive data (after passing through<br>reverse and rotate units) is masked out and then padded with the<br>selected bit pad value (RPAD and RPBIT bits in RFMT). |
|      |       |      |       | 1h = Corresponding bit of receive data (after passing through<br>reverse and rotate units) is returned to CPU or DMA.                                                                      |
#### **22.4.1.14 RFMT Register (offset = 68h) [reset = 0h]**

RFMT is shown in [Figure](#page-79-1) 22-52 and described in Table [22-25.](#page-79-2)

The receive bit stream format register (RFMT) configures the receive data format.

#### **Figure 22-52. RFMT Register**

| 31     | 30       | 29 | 28 | 27     | 26     | 25     | 24 |
|--------|----------|----|----|--------|--------|--------|----|--
|        | RESERVED |    |    |        |        |        |    |
|        | R-0h     |    |    |        |        |        |    |
| 23     | 22       | 21 | 20 | 19     | 18     | 17     | 16 |
|        | RESERVED |    |    |        |        |        |    |
| R-0h   |          |    |    |        |        | R/W-0h |    |
| 15     | 14       | 13 | 12 | 11     | 10     | 9      | 8  |
| RRVRS  | RPAD     |    |    | RPBIT  |        |        |    |
| R/W-0h | R/W-0h   |    |    |        | R/W-0h |        |    |
| 7      | 6        | 5  | 4  | 3      | 2      | 1      | 0  |
|        | RSSZ     |    |    |        |        | RROT   |    |
|        | R/W-0h   |    |    | R/W-0h |        | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-25. RFMT Register Field Descriptions**

| Bit   | Field    | Type | Reset                                                                                                                                                                                                                                                                                            | Description                                                                                                                                                                                                                                                                                                                                                                                              |
|-------|----------|------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-18 | RESERVED | R    | 0h                                                                                                                                                                                                                                                                                               |                                                                                                                                                                                                                                                                                                                                                                                                          |
| 17-16 | RDATDLY  | R/W  | 0h                                                                                                                                                                                                                                                                                               | Receive bit delay.<br>0h = 0-bit delay. The first receive data bit, AXRn, occurs in same<br>ACLKR cycle as the receive frame sync (AFSR).<br>1h = 1-bit delay. The first receive data bit, AXRn, occurs one ACLKR<br>cycle after the receive frame sync (AFSR).<br>2h = 2-bit delay. The first receive data bit, AXRn, occurs two ACLKR<br>cycles after the receive frame sync (AFSR).<br>3h = Reserved. |
| 15    | RRVRS    | R/W  | 0h                                                                                                                                                                                                                                                                                               | Receive serial bitstream order.<br>0h = Bitstream is LSB first. No bit reversal is performed in receive<br>format bit reverse unit.<br>1h = Bitstream is MSB first. Bit reversal is performed in receive<br>format bit reverse unit.                                                                                                                                                                     |
| 14-13 | RPAD     | R/W  | 0h<br>Pad value for extra bits in slot not belonging to the word.<br>This field only applies to bits when RMASK[n] = 0.<br>0h = Pad extra bits with 0.<br>1h = Pad extra bits with 1.<br>2h = Pad extra bits with one of the bits from the word as specified by<br>RPBIT bits.<br>3h = Reserved. |                                                                                                                                                                                                                                                                                                                                                                                                          |
| 12-8  | RPBIT    | R/W  | 0h                                                                                                                                                                                                                                                                                               | RPBIT value determines which bit (as read by the CPU or DMA from<br>RBUF[n]) is used to pad the extra bits.<br>This field only applies when RPAD = 2h.<br>0h = Pad with bit 0 value.<br>1h = Pad with bit 1 to bit 31 value from 1h to 1Fh.                                                                                                                                                              |
**Table 22-25. RFMT Register Field Descriptions (continued)**

| Bit | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                             |
|-----|--------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7-4 | RSSZ   | R/W  | 0h    | Receive slot size.<br>0h = Reseved.<br>1h = Reserved.<br>2h = Reserved.<br>3h = Slot size is 8 bits.<br>4h = Reserved<br>5h = Slot size is 12 bits.<br>6h = Reserved<br>7h = Slot size is 16 bits.<br>8h = Reserved<br>9h = Slot size is 20 bits.<br>Ah = Reserved<br>Bh = Slot size is 24 bits<br>Ch = Reserved<br>Dh = Slot size is 28 bits.<br>Eh = Reserved<br>Fh = Slot size is 32 bits.           |
| 3   | RBUSEL | R/W  | 0h    | Selects whether reads from serializer buffer XRBUF[n] originate from<br>the configuration bus (CFG) or the data (DAT) port.<br>0h = Reads from XRBUF[n] originate on data port. Reads from<br>XRBUF[n] on configuration bus are ignored.<br>1h = Reads from XRBUF[n] originate on configuration bus. Reads<br>from XRBUF[n] on data port are ignored.                                                   |
| 2-0 | RROT   | R/W  | 0h    | Right-rotation value for receive rotate right format unit.<br>0h = Rotate right by 0 (no rotation).<br>1h = Rotate right by 4 bit positions.<br>2h = Rotate right by 8 bit positions.<br>3h = Rotate right by 12 bit positions.<br>4h = Rotate right by 16 bit positions.<br>5h = Rotate right by 20 bit positions.<br>6h = Rotate right by 24 bit positions.<br>7h = Rotate right by 28 bit positions. |
#### **22.4.1.15 AFSRCTL Register (offset = 6Ch) [reset = 0h]**

AFSRCTL is shown in [Figure](#page-81-1) 22-53 and described in Table [22-26.](#page-81-2)

The receive frame sync control register (AFSRCTL) configures the receive frame sync (AFSR).

**Figure 22-53. AFSRCTL Register**

| 30       | 29 | 28     | 27       | 26                         | 25     | 24     |
|----------|----|--------|----------|----------------------------|--------|--------|--
|          |    |        |          |                            |        |        |
| R-0h     |    |        |          |                            |        |        |
| 22       | 21 | 20     | 19       | 18                         | 17     | 16     |
| RESERVED |    |        |          |                            |        |        |
| R-0h     |    |        |          |                            |        |        |
| 14       | 13 | 12     | 11       | 10                         | 9      | 8      |
|          |    |        |          |                            |        |        |
|          |    |        |          |                            |        |        |
| 6        | 5  | 4      | 3        | 2                          | 1      | 0      |
| RESERVED |    | FRWID  | RESERVED |                            | FSRM   | FSRP   |
|          |    | R/W-0h | R-0h     |                            | R/W-0h | R/W-0h |
|          |    | R-0h   |          | RESERVEDRMOD<br>R/W-0h |        |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-26. AFSRCTL Register Field Descriptions**

| Bit   | Field        | Type      | Reset | Description                                                                                                           |
|-------|--------------|-----------|-------|-----------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED     | R         | 0h    |                                                                                                                       |
| 15-7  | RMOD         | R/W0h |       | Receive frame sync mode select bits.<br>1FFh = Reserved from 181h to 1FFh.                                            |
|       |              |           |       | 0h = Burst mode.                                                                                                      |
|       |              |           |       | 1h = Reserved.                                                                                                        |
|       |              |           |       | 2h = 2-slot TDM (I2S mode) to 32-slot TDM from 2h to 20h.                                                             |
|       |              |           |       | 21h = Reserved from 21h to 17Fh.                                                                                      |
|       |              |           |       | 180h = 384-slot TDM (external DIR IC inputting 384-slot DIR frames<br>to McASP over I2S interface).                   |
|       |              |           |       | 181h = Reserved from 181h to 1FFh.                                                                                    |
| 6-5   | RESERVED     | R         | 0h    |                                                                                                                       |
| 4     | FRWIDR/W |           | 0h    | Receive frame sync width select bit indicates the width of the receive<br>frame sync (AFSR) during its active period. |
|       |              |           |       | 0h = Single bit.                                                                                                      |
|       |              |           |       | 1h = Single word.                                                                                                     |
| 3-2   | RESERVED     | R         | 0h    |                                                                                                                       |
| 1     | FSRM         | R/W       | 0h    | Receive frame sync generation select bit.                                                                             |
|       |              |           |       | 0h = Externally-generated receive frame sync.                                                                         |
|       |              |           |       | 1h = Internally-generated receive frame sync.                                                                         |
| 0     | FSRP         | R/W       | 0h    | Receive frame sync polarity select bit.                                                                               |
|       |              |           |       | 0h = A rising edge on receive frame sync (AFSR) indicates the<br>beginning of a frame.                                |
|       |              |           |       | 1h = A falling edge on receive frame sync (AFSR) indicates the<br>beginning of a frame.                               |
#### **22.4.1.16 ACLKRCTL Register (offset = 70h) [reset = 0h]**

ACLKRCTL is shown in [Figure](#page-82-1) 22-54 and described in Table [22-27](#page-82-2).

The receive clock control register (ACLKRCTL) configures the receive bit clock (ACLKR) and the receive clock generator.

#### **Figure 22-54. ACLKRCTL Register**

| 31     | 30       | 29     | 28       | 27 | 26      | 25 | 24 |
|--------|----------|--------|----------|----|---------|----|----|--
|        | RESERVED |        |          |    |         |    |    |
|        | R-0h     |        |          |    |         |    |    |
| 23     | 22       | 21     | 20       | 19 | 18      | 17 | 16 |
|        | RESERVED |        |          |    |         |    |    |
| R-0h   |          |        |          |    |         |    |    |
| 15     | 14       | 13     | 12       | 11 | 10      | 9  | 8  |
|        |          |        | RESERVED |    |         |    |    |
|        |          |        | R-0h     |    |         |    |    |
| 7      | 6        | 5      | 4        | 3  | 2       | 1  | 0  |
| CLKRP  | RESERVED | CLKRM  |          |    | CLKRDIV |    |    |
| R/W-0h | R-0h     | R/W-0h |          |    | R/W-0h  |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-27. ACLKRCTL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
|------|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| 7    | CLKRP    | R/W  | 0h    | Receive bitstream clock polarity select bit.<br>0h = Falling edge. Receiver samples data on the falling edge of the<br>serial clock, so the external transmitter driving this receiver must shift<br>data out on the rising edge of the serial clock.<br>1h = Rising edge. Receiver samples data on the rising edge of the<br>serial clock, so the external transmitter driving this receiver must shift<br>data out on the falling edge of the serial clock. |
| 6    | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| 5    | CLKRM    | R/W  | 0h    | Receive bit clock source bit.<br>Note that this bit does not have any effect, if ACLKXCTL.ASYNC =<br>0.<br>0h = External receive clock source from ACLKR pin.<br>1h = Internal receive clock source from output of programmable bit<br>clock divider.                                                                                                                                                                                                         |
| 4-0  | CLKRDIV  | R/W  | 0h    | Receive bit clock divide ratio bits determine the divide-down ratio<br>from AHCLKR to ACLKR.<br>Note that this bit does not have any effect, if ACLKXCTL.ASYNC =<br>0.<br>0h = Divide-by-1.<br>1h = Divide-by-2.<br>2h = Divide-by-3 to divide-by-32 from 2h to 1Fh.                                                                                                                                                                                          |
#### **22.4.1.17 AHCLKRCTL Register (offset = 74h) [reset = 0h]**

AHCLKRCTL is shown in [Figure](#page-83-1) 22-55 and described in Table [22-28.](#page-83-2)

The receive high-frequency clock control register (AHCLKRCTL) configures the receive high-frequency master clock (AHCLKR) and the receive clock generator.

#### **Figure 22-55. AHCLKRCTL Register**

| 31       | 30       | 29       | 28 | 27       | 26     | 25 | 24
|----------|----------|----------|----|----------|--------|----|----
| RESERVED |          |          |    |          |        |    |
| R-0h     |          |          |    |          |        |    |
| 23       | 22       | 21       | 20 | 19       | 18     | 17 | 16
|          | RESERVED |          |    |          |        |    |
| R-0h     |          |          |    |          |        |    |
| 15       | 14       | 13       | 12 | 11       | 10     | 9  | 8
| HCLKRM   | HCLKRP   | RESERVED |    | HCLKRDIV |        |    |
| R/W-0h   | R/W-0h   | R-0h     |    |          | R/W-0h |    |
| 7        | 6        | 5        | 4  | 3        | 2      | 1  | 0
|          |          |          |    | HCLKRDIV |        |    |
|          |          |          |    | R/W-0h   |        |    |
|          |          |          |    |          |        |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-28. AHCLKRCTL Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                                                                                                                                                                                                                |
|-------|-----------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED        | R    | 0h    |                                                                                                                                                                                                                                                                                                            |
| 15    | HCLKRM          | R/W  | 0h    | Receive high-frequency clock source bit.<br>0h = External receive high-frequency clock source from AHCLKR                                                                                                                                                                                                  |
|       |                 |      |       | pin.                                                                                                                                                                                                                                                                                                       |
|       |                 |      |       | 1h = Internal receive high-frequency clock source from output of<br>programmable high clock divider.                                                                                                                                                                                                       |
| 14    | HCLKRP          | R/W  | 0h    | Receive bitstream high-frequency clock polarity select bit.                                                                                                                                                                                                                                                |
|       |                 |      |       | 0h = AHCLKR is not inverted before programmable bit clock divider.<br>In the special case where the receive bit clock (ACLKR) is internally<br>generated and the programmable bit clock divider is set to divide-by<br>1 (CLKRDIV = 0 in ACLKRCTL), AHCLKR is directly passed through<br>to the ACLKR pin. |
|       |                 |      |       | 1h = AHCLKR is inverted before programmable bit clock divider. In<br>the special case where the receive bit clock (ACLKR) is internally<br>generated and the programmable bit clock divider is set to divide-by<br>1 (CLKRDIV = 0 in ACLKRCTL), AHCLKR is directly passed through<br>to the ACLKR pin.     |
| 13-12 | RESERVED        | R    | 0h    |                                                                                                                                                                                                                                                                                                            |
| 11-0  | HCLKRDIVR/W |      | 0h    | Receive high-frequency clock divide ratio bits determine the divide<br>down ratio from AUXCLK to AHCLKR.                                                                                                                                                                                                   |
|       |                 |      |       | 0h = Divide-by-1.                                                                                                                                                                                                                                                                                          |
|       |                 |      |       | 1h = Divide-by-2.                                                                                                                                                                                                                                                                                          |
|       |                 |      |       | 2h = Divide-by-3 to divide-by-4096 from 2h to FFFh.                                                                                                                                                                                                                                                        |
#### **22.4.1.18 RTDM Register (offset = 78h) [reset = 0h]**

RTDM is shown in [Figure](#page-84-1) 22-56 and described in Table [22-29.](#page-84-2)

The receive TDM time slot register (RTDM) specifies which TDM time slot the receiver is active.

### **Figure 22-56. RTDM Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-29. RTDM Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                  |
|------|-------|------|-------|--------------------------------------------------------------------------------------------------------------|
| 31-0 | RTDMS | R/W  | 0h    | Receiver mode during TDM time slot n.                                                                        |
|      |       |      |       | 0h = Receive TDM time slot n is inactive. The receive serializer does<br>not shift in data during this slot. |
|      |       |      |       | 1h = Receive TDM time slot n is active. The receive serializer shifts<br>in data during this slot.           |
#### **22.4.1.19 RINTCTL Register (offset = 7Ch) [reset = 0h]**

RINTCTL is shown in [Figure](#page-85-1) 22-57 and described in Table [22-30](#page-85-2).

The receiver interrupt control register (RINTCTL) controls generation of the McASP receive interrupt (RINT). When the register bit(s) is set to 1, the occurrence of the enabled McASP condition(s) generates RINT. See the RSTAT register for a description of the interrupt conditions.

**Figure 22-57. RINTCTL Register**

| 31      | 30       | 29     | 28     | 27       | 26      | 25       | 24     |
|---------|----------|--------|--------|----------|---------|----------|--------|--
|         | RESERVED |        |        |          |         |          |        |
|         | R-0h     |        |        |          |         |          |        |
| 23      | 22       | 21     | 20     | 19       | 18      | 17       | 16     |
|         | RESERVED |        |        |          |         |          |        |
| R-0h    |          |        |        |          |         |          |        |
| 15      | 14       | 13     | 12     | 11       | 10      | 9        | 8      |
|         |          |        |        | RESERVED |         |          |        |
|         |          |        |        | R-0h     |         |          |        |
| 7       | 6        | 5      | 4      | 3        | 2       | 1        | 0      |
| RSTAFRM | RESERVED | RDATA  | RLAST  | RDMAERR  | RCKFAIL | RSYNCERR | ROVRN  |
| R/W-0h  | R-0h     | R/W-0h | R/W-0h | R/W-0h   | R/W-0h  | R/W-0h   | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-30. RINTCTL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                           |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------|
| 31-8 | RESERVED | R    | 0h    |                                                                                                                       |
| 7    | RSTAFRM  | R/W  | 0h    | Receive start of frame interrupt enable bit.                                                                          |
|      |          |      |       | 0h = Interrupt is disabled. A receive start of frame interrupt does not<br>generate a McASP receive interrupt (RINT). |
|      |          |      |       | 1h = Interrupt is enabled. A receive start of frame interrupt generates<br>a McASP receive interrupt (RINT).          |
| 6    | RESERVED | R    | 0h    |                                                                                                                       |
| 5    | RDATA    | R/W  | 0h    | Receive data ready interrupt enable bit.                                                                              |
|      |          |      |       | 0h = Interrupt is disabled. A receive data ready interrupt does not<br>generate a McASP receive interrupt (RINT).     |
|      |          |      |       | 1h = Interrupt is enabled. A receive data ready interrupt generates a<br>McASP receive interrupt (RINT).              |
| 4    | RLAST    | R/W  | 0h    | Receive last slot interrupt enable bit.                                                                               |
|      |          |      |       | 0h = Interrupt is disabled. A receive last slot interrupt does not<br>generate a McASP receive interrupt (RINT).      |
|      |          |      |       | 1h = Interrupt is enabled. A receive last slot interrupt generates a<br>McASP receive interrupt (RINT).               |
| 3    | RDMAERR  | R/W  | 0h    | Receive DMA error interrupt enable bit.                                                                               |
|      |          |      |       | 0h = Interrupt is disabled. A receive DMA error interrupt does not<br>generate a McASP receive interrupt (RINT).      |
|      |          |      |       | 1h = Interrupt is enabled. A receive DMA error interrupt generates a<br>McASP receive interrupt (RINT).               |
| 2    | RCKFAIL  | R/W  | 0h    | Receive clock failure interrupt enable bit.                                                                           |
|      |          |      |       | 0h = Interrupt is disabled. A receive clock failure interrupt does not<br>generate a McASP receive interrupt (RINT).  |
|      |          |      |       | 1h = Interrupt is enabled. A receive clock failure interrupt generates<br>a McASP receive interrupt (RINT).           |
**Table 22-30. RINTCTL Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset | Description                                                                                                                   |
|-----|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------|
| 1   | RSYNCERR | R/W  | 0h    | Unexpected receive frame sync interrupt enable bit.                                                                           |
|     |          |      |       | 0h = Interrupt is disabled. An unexpected receive frame sync<br>interrupt does not generate a McASP receive interrupt (RINT). |
|     |          |      |       | 1h = Interrupt is enabled. An unexpected receive frame sync<br>interrupt generates a McASP receive interrupt (RINT).          |
| 0   | ROVRN    | R/W  | 0h    | Receiver overrun interrupt enable bit.                                                                                        |
|     |          |      |       | 0h = Interrupt is disabled. A receiver overrun interrupt does not<br>generate a McASP receive interrupt (RINT).               |
|     |          |      |       | 1h = Interrupt is enabled. A receiver overrun interrupt generates a<br>McASP receive interrupt (RINT).                        |
#### **22.4.1.20 RSTAT Register (offset = 80h) [reset = 0h]**

RSTAT is shown in [Figure](#page-87-1) 22-58 and described in Table [22-31.](#page-87-2)

The receiver status register (RSTAT) provides the receiver status and receive TDM time slot number. If the McASP logic attempts to set an interrupt flag in the same cycle that the CPU writes to the flag to clear it, the McASP logic has priority and the flag remains set. This also causes a new interrupt request to be generated.

**Figure 22-58. RSTAT Register**

| 31       | 30       | 29       | 28       | 27       | 26       | 25       | 24
|----------|----------|----------|----------|----------|----------|----------|----------|--|--|--|--
|          |          |          |          | RESERVED |          |          |
|          | R-0h     |          |          |          |          |          |
| 23       | 22       | 21       | 20       | 19       | 18       | 17       | 16
|          |          |          |          | RESERVED |          |          |
| R-0h     |          |          |          |          |          |          |
| 15       | 14       | 13       | 12       | 11       | 10       | 9        | 8
|          |          |          | RESERVED |          |          |          | RERR
|          |          |          | R-0h     |          |          |          | R/W-0h
| 7        | 6        | 5        | 4        | 3        | 2        | 1        | 0
| RDMAERR  | RSTAFRM  | RDATA    | RLAST    | RTDMSLOT | RCKFAIL  | RSYNCERR | ROVRN
| R/W1C-0h | R/W1C-0h | R/W1C-0h | R/W1C-0h | R-0h     | R/W1C-0h | R/W1C-0h | R/W1C-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-31. RSTAT Register Field Descriptions**

| Bit  | Field    | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|------|----------|-------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-9 | RESERVED | R     | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 8    | RERR     | R/W   | 0h    | RERR bit always returns a logic-OR of: ROVRN OR RSYNCERROR RCKFAIL OR RDMAERR.<br>Allows a single bit to be checked to determine if a receiver error<br>interrupt has occurred.<br>0h = No errors have occurred.<br>1h = An error has occurred.                                                                                                                                                                                            |
| 7    | RDMAERR  | R/W1C | 0h    | Receive DMA error flag.<br>RDMAERR is set when the CPU or DMA reads more serializers<br>through the data port in a given time slot than were programmed as<br>receivers.<br>Causes a receive interrupt (RINT), if this bit is set and RDMAERR in<br>RINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 to this bit has no effect.<br>0h = Receive DMA error did not occur.<br>1h = Receive DMA error did occur. |
| 6    | RSTAFRM  | R/W1C | 0h    | Receive start of frame flag.<br>Causes a receive interrupt (RINT), if this bit is set and RSTAFRM in<br>RINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 to this bit has no effect.<br>0h = No new receive frame sync (AFSR) is detected.<br>1h = A new receive frame sync (AFSR) is detected.                                                                                                                |
| 5    | RDATA    | R/W1C | 0h    | Receive data ready flag.<br>Causes a receive interrupt (RINT), if this bit is set and RDATA in<br>RINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 to this bit has no effect.<br>0h = No new data in RBUF.<br>1h = Data is transferred from XRSR to RBUF and ready to be<br>serviced by the CPU or DMA. When RDATA is set, it always causes<br>a DMA event (AREVT).                                           |
**Table 22-31. RSTAT Register Field Descriptions (continued)**

| Bit | Field    | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-----|----------|-------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 4   | RLAST    | R/W1C | 0h    | Receive last slot flag.<br>RLAST is set along with RDATA, if the current slot is the last slot in<br>a frame.<br>Causes a receive interrupt (RINT), if this bit is set and RLAST in<br>RINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 to this bit has no effect.<br>0h = Current slot is not the last slot in a frame.<br>1h = Current slot is the last slot in a frame. RDATA is also set.                                    |
| 3   | RTDMSLOT | R     | 0h    | Returns the LSB of RSLOT.<br>Allows a single read of RSTAT to determine whether the current<br>TDM time slot is even or odd.<br>0h = Current TDM time slot is odd.<br>1h = Current TDM time slot is even.                                                                                                                                                                                                                                                         |
| 2   | RCKFAIL  | R/W1C | 0h    | Receive clock failure flag.<br>RCKFAIL is set when the receive clock failure detection circuit<br>reports an error.<br>Causes a receive interrupt (RINT), if this bit is set and RCKFAIL in<br>RINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 to this bit has no effect.<br>0h = Receive clock failure did not occur.<br>1h = Receive clock failure did occur.                                                                 |
| 1   | RSYNCERR | R/W1C | 0h    | Unexpected receive frame sync flag.<br>RSYNCERR is set when a new receive frame sync (AFSR) occurs<br>before it is expected.<br>Causes a receive interrupt (RINT), if this bit is set and RSYNCERR<br>in RINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 to this bit has no effect.<br>0h = Unexpected receive frame sync did not occur.<br>1h = Unexpected receive frame sync did occur.                                       |
| 0   | ROVRN    | R/W1C | 0h    | Receiver overrun flag.<br>ROVRN is set when the receive serializer is instructed to transfer<br>data from XRSR to RBUF, but the former data in RBUF has not yet<br>been read by the CPU or DMA.<br>Causes a receive interrupt (RINT), if this bit is set and ROVRN in<br>RINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 to this bit has no effect.<br>0h = Receiver overrun did not occur.<br>1h = Receiver overrun did occur. |
#### **22.4.1.21 RSLOT Register (offset = 84h) [reset = 0h]**

RSLOT is shown in [Figure](#page-89-1) 22-59 and described in Table [22-32](#page-89-2).

The current receive TDM time slot register (RSLOT) indicates the current time slot for the receive data frame.

**Figure 22-59. RSLOT Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-32. RSLOT Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                             |
|------|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-9 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                         |
| 8-0  | RSLOTCNT | R    | 0h    | 0-17Fh = Current receive time slot count.<br>Legal values: 0 to 383 (17Fh).<br>TDM function is not supported for > 32 time slots.<br>However, TDM time slot counter may count to 383 when used to<br>receive a DIR block (transferred over TDM format). |
#### **22.4.1.22 RCLKCHK Register (offset = 88h) [reset = 0h]**

RCLKCHK is shown in [Figure](#page-90-1) 22-60 and described in Table [22-33.](#page-90-2)

The receive clock check control register (RCLKCHK) configures the receive clock failure detection circuit.

### **Figure 22-60. RCLKCHK Register**

| 31   | 30     | 29 | 28 | 27 | 26 | 25 | 24 | 23              | 22 | 21   | 20 | 19     | 18 | 17     | 16 |
|------|--------|----|----|----|----|----|----|-----------------|----|------|----|--------|----|--------|----|
| RCNT |        |    |    |    |    |    |    |                 |    |      |    | RMAX   |    |        |    |
|      | R-0h   |    |    |    |    |    |    |                 |    |      |    | R/W-0h |    |        |    |
| 15   | 14     | 13 | 12 | 11 | 10 | 9  | 8  | 7               | 6  | 5    | 4  | 3      | 2  | 1      | 0  |
| RMIN |        |    |    |    |    |    |    | RESERVEDRPS |    |      |    |        |    |        |    |
|      | R/W-0h |    |    |    |    |    |    |                 |    | R-0h |    |        |    | R/W-0h |    |
|      |        |    |    |    |    |    |    |                 |    |      |    |        |    |        |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 22-33. RCLKCHK Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description
|-------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--
| 31-24 | RCNT     | R    | 0h    | Receive clock count value (from previous measurement).<br>The clock circuit continually counts the number of system clocks for<br>every 32 receive high-frequency master clock (AHCLKR) signals,<br>and stores the count in RCNT until the next measurement is taken.
| 23-16 | RMAX     | R/W  | 0h    | Receive clock maximum boundary.<br>This 8 bit unsigned value sets the maximum allowed boundary for<br>the clock check counter after 32 receive high-frequency master clock<br>(AHCLKR) signals have been received.<br>If the current counter value is greater than RMAX after counting 32AHCLKR signals, RCKFAIL in RSTAT is set.<br>The comparison is performed using unsigned arithmetic.
| 15-8  | RMIN     | R/W  | 0h    | Receive clock minimum boundary.<br>This 8 bit unsigned value sets the minimum allowed boundary for the<br>clock check counter after 32 receive high-frequency master clock<br>(AHCLKR) signals have been received.<br>If RCNT is less than RMIN after counting 32 AHCLKR signals,<br>RCKFAIL in RSTAT is set.<br>The comparison is performed using unsigned arithmetic.
| 7-4   | RESERVED | R    | 0h    |
| 3-0   | RPS      | R/W  | 0h    | Receive clock check prescaler value.<br>0h = McASP system clock divided by 1.<br>1h = McASP system clock divided by 2.<br>2h = McASP system clock divided by 4.<br>3h = McASP system clock divided by 8.<br>4h = McASP system clock divided by 16.<br>5h = McASP system clock divided by 32.<br>6h = McASP system clock divided by 64.<br>7h = McASP system clock divided by 128.<br>8h = McASP system clock divided by 256.<br>9h = Reserved from 9h to Fh.
#### **22.4.1.23 REVTCTL Register (offset = 8Ch) [reset = 0h]**

REVTCTL is shown in [Figure](#page-91-1) 22-61 and described in Table [22-34.](#page-91-2)

The receiver DMA event control register (REVTCTL) contains a disable bit for the receiver DMA event. Note for device-specific registers: Accessing REVTCTL not implemented on a specific device may cause improper operation.

**Figure 22-61. REVTCTL Register**

| 31       | 30   | 29 | 28       | 27 | 26 | 25 | 24     |
|----------|------|----|----------|----|----|----|--------|--|--|--
| RESERVED |      |    |          |    |    |    |        |
|          | R-0h |    |          |    |    |    |        |
| 23       | 22   | 21 | 20       | 19 | 18 | 17 | 16     |
| RESERVED |      |    |          |    |    |    |        |
| R-0h     |      |    |          |    |    |    |        |
| 15       | 14   | 13 | 12       | 11 | 10 | 9  | 8      |
|          |      |    | RESERVED |    |    |    |        |
|          |      |    | R-0h     |    |    |    |        |
| 7        | 6    | 5  | 4        | 3  | 2  | 1  | 0      |
| RESERVED |      |    |          |    |    |    |        |
|          |      |    | R-0h     |    |    |    | R/W-0h |
|          |      |    |          |    |    |    |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-34. REVTCTL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                        |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | RESERVED | R    | 0h    |                                                                                                                                                                    |
| 0    | RDATDMA  | R/W  | 0h    | Receive data DMA request enable bit.<br>If writing to this bit, always write the default value of 0.<br>0h = Receive data DMA request is enabled.<br>1h = Reserved |
#### **22.4.1.24 XGBLCTL Register (offset = A0h) [reset = 0h]**

XGBLCTL is shown in [Figure](#page-92-1) 22-62 and described in Table [22-35.](#page-92-2)

Alias of the global control register (GBLCTL). Writing to the transmitter global control register (XGBLCTL) affects only the transmit bits of GBLCTL (bits 12-8). Reads from XGBLCTL return the value of GBLCTL. XGBLCTL allows the transmitter to be reset independently from the receiver. See the GBLCTL register for a detailed description of GBLCTL.

**Figure 22-62. XGBLCTL Register**

| 31   | 30                                                           | 29 | 28     | 27       | 26     | 25       | 24
|------|--------------------------------------------------------------|----|--------|----------|--------|----------|---------|--|--
|      |                                                              |    |        | RESERVED |        |          |
| R-0h |                                                              |    |        |          |        |          |
| 23   | 22                                                           | 21 | 20     | 19       | 18     | 17       | 16
|      |                                                              |    |        | RESERVED |        |          |
| R-0h |                                                              |    |        |          |        |          |
| 15   | 14                                                           | 13 | 12     | 11       | 10     | 9        | 8
|      | RESERVED                                                     |    | XFRST  | XSMRST   | XSRCLR | XHCLKRST | XCLKRST
|      | R-0h                                                         |    | R/W-0h | R/W-0h   | R/W-0h | R/W-0h   | R/W-0h
| 7    | 6                                                            | 5  | 4      | 3        | 2      | 1        | 0
|      | RESERVEDRFRST<br>RSMRSTRSRCLR<br>RHCLKRSTRCLKRST |    |        |          |        |          |
|      | R-0h                                                         |    | R-0h   | R-0h     | R-0h   | R-0h     | R-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-35. XGBLCTL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                           |
|-------|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------|--
| 31-13 | RESERVED | R    | 0h    |                                                                                                                                       |
| 12    | XFRST    | R/W  | 0h    | Transmit frame sync generator reset enable bit.<br>A write to this bit affects the XFRST bit of GBLCTL.                               |
|       |          |      |       | 0h = Transmit frame sync generator is reset.                                                                                          |
|       |          |      |       | 1h = Transmit frame sync generator is active.                                                                                         |
| 11    | XSMRST   | R/W  | 0h    | Transmit state machine reset enable bit.<br>A write to this bit affects the XSMRST bit of GBLCTL.                                     |
|       |          |      |       | 0h = Transmit state machine is held in reset.                                                                                         |
|       |          |      |       | 1h = Transmit state machine is released from reset.                                                                                   |
| 10    | XSRCLR   | R/W  | 0h    | Transmit serializer clear enable bit.<br>A write to this bit affects the XSRCLR bit of GBLCTL.                                        |
|       |          |      |       | 0h = Transmit serializers are cleared.                                                                                                |
|       |          |      |       | 1h = Transmit serializers are active.                                                                                                 |
| 9     | XHCLKRST | R/W  | 0h    | Transmit high-frequency clock divider reset enable bit.<br>A write to this bit affects the XHCLKRST bit of GBLCTL.                    |
|       |          |      |       | 0h = Transmit high-frequency clock divider is held in reset.                                                                          |
|       |          |      |       | 1h = Transmit high-frequency clock divider is running.                                                                                |
| 8     | XCLKRST  | R/W  | 0h    | Transmit clock divider reset enable bit.<br>A write to this bit affects the XCLKRST bit of GBLCTL.                                    |
|       |          |      |       | 0h = Transmit clock divider is held in reset.                                                                                         |
|       |          |      |       | 1h = Transmit clock divider is running.                                                                                               |
| 7-5   | RESERVED | R    | 0h    |                                                                                                                                       |
| 4     | RFRST    | R    | 0h    | Receive frame sync generator reset enable bit.<br>A read of this bit returns the RFRST bit value of GBLCTL.<br>Writes have no effect. |
| 3     | RSMRST   | R    | 0h    | Receive state machine reset enable bit.<br>A read of this bit returns the RSMRST bit value of GBLCTL.<br>Writes have no effect.       |
| 2     | RSRCLR   | R    | 0h    | Receive serializer clear enable bit.<br>A read of this bit returns the RSRSCLR bit value of GBLCTL.<br>Writes have no effect.         |
**Table 22-35. XGBLCTL Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset | Description                                                                                                                                      |
|-----|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------|
| 1   | RHCLKRST | R    | 0h    | Receive high-frequency clock divider reset enable bit.<br>A read of this bit returns the RHCLKRST bit value of GBLCTL.<br>Writes have no effect. |
| 0   | RCLKRST  | R    | 0h    | Receive clock divider reset enable bit.<br>A read of this bit returns the RCLKRST bit value of GBLCTL.<br>Writes have no effect.                 |
#### **22.4.1.25 XMASK Register (offset = A4h) [reset = 0h]**

XMASK is shown in [Figure](#page-94-1) 22-63 and described in Table [22-36](#page-94-2).

The transmit format unit bit mask register (XMASK) determines which bits of the transmitted data are masked off and padded with a known value before being shifted out the McASP.

**Figure 22-63. XMASK Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-36. XMASK Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                        |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | XMASK | R/W  | 0h    | Transmit data mask n enable bit.<br>0h = Corresponding bit of transmit data (before passing through<br>reverse and rotate units) is masked out and then padded with the<br>selected bit pad value (XPAD and XPBIT bits in XFMT), which is<br>transmitted out the McASP in place of the original bit.<br>1h = Corresponding bit of transmit data (before passing through<br>reverse and rotate units) is transmitted out the McASP. |
#### **22.4.1.26 XFMT Register (offset = A8h) [reset = 0h]**

XFMT is shown in [Figure](#page-95-1) 22-64 and described in Table [22-37](#page-95-2).

The transmit bit stream format register (XFMT) configures the transmit data format.

#### **Figure 22-64. XFMT Register**

| 31     | 30     | 29     | 28      | 27       | 26     | 25     | 24
|--------|--------|--------|---------|----------|--------|--------|----|--|--|--|--
|        |        |        |         | RESERVED |        |        |
|        | R-0h   |        |         |          |        |        |
| 23     | 22     | 21     | 20      | 19       | 18     | 17     | 16
|        |        |        | XDATDLY |          |        |        |
|        |        | R/W-0h |         |          |        |        |
| 15     | 14     | 13     | 12      | 11       | 10     | 9      | 8
| XRVRS  | XPAD   |        |         |          | XPBIT  |        |
| R/W-0h | R/W-0h |        |         |          | R/W-0h |        |
| 7      | 6      | 5      | 4       | 3        | 2      | 1      | 0
|        | XSSZ   |        |         | XBUSEL   |        | XROT   |
|        | R/W-0h |        |         | R/W-0h   |        | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-37. XFMT Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                             |
|-------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-18 | RESERVED | R    | 0h    |                                                                                                                                                                         |
| 17-16 | XDATDLY  | R/W  | 0h    | Transmit sync bit delay.                                                                                                                                                |
|       |          |      |       | 0h = 0-bit delay. The first transmit data bit, AXRn, occurs in same<br>ACLKX cycle as the transmit frame sync (AFSX).                                                   |
|       |          |      |       | 1h = 1-bit delay. The first transmit data bit, AXRn, occurs one<br>ACLKX cycle after the transmit frame sync (AFSX).                                                    |
|       |          |      |       | 2h = 2-bit delay. The first transmit data bit, AXRn, occurs two ACLKX<br>cycles after the transmit frame sync (AFSX).<br>3h = Reserved.                                 |
| 15    | XRVRS    | R/W  | 0h    | Transmit serial bitstream order.                                                                                                                                        |
|       |          |      |       | 0h = Bitstream is LSB first. No bit reversal is performed in transmit<br>format bit reverse unit.                                                                       |
|       |          |      |       | 1h = Bitstream is MSB first. Bit reversal is performed in transmit<br>format bit reverse unit.                                                                          |
| 14-13 | XPAD     | R/W  | 0h    | Pad value for extra bits in slot not belonging to word defined by<br>XMASK.                                                                                             |
|       |          |      |       | This field only applies to bits when XMASK[n] = 0.                                                                                                                      |
|       |          |      |       | 0h = Pad extra bits with 0.                                                                                                                                             |
|       |          |      |       | 1h = Pad extra bits with 1.<br>2h = Pad extra bits with one of the bits from the word as specified by                                                                   |
|       |          |      |       | XPBIT bits.                                                                                                                                                             |
|       |          |      |       | 3h = Reserved.                                                                                                                                                          |
| 12-8  | XPBIT    | R/W  | 0h    | XPBIT value determines which bit (as written by the CPU or DMA to<br>XBUF[n]) is used to pad the extra bits before shifting.<br>This field only applies when XPAD = 2h. |
|       |          |      |       | 0h = Pad with bit 0 value.                                                                                                                                              |
|       |          |      |       | 1h = Pad with bit 1 to bit 31 value from 1h to 1Fh.                                                                                                                     |
**Table 22-37. XFMT Register Field Descriptions (continued)**

| Bit | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                            |
|-----|--------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 7-4 | XSSZ   | R/W  | 0h    | Transmit slot size.<br>0h = Reserved.<br>1h = Reserved.<br>2h = Reserved.<br>3h = Slot size is 8 bits.<br>4h = Reserved.<br>5h = Slot size is 12 bits.<br>6h = Reserved.<br>7h = Slot size is 16 bits.<br>8h = Reserved.<br>9h = Slot size is 20 bits.<br>Ah = Reserved.<br>Bh = Slot size is 24 bits.<br>Ch = Reserved.<br>Dh = Slot size is 28 bits.<br>Eh = Reserved.<br>Fh = Slot size is 32 bits.                                 |
| 3   | XBUSEL | R/W  | 0h    | Selects whether writes to serializer buffer XRBUF[n] originate from<br>the configuration bus (CFG) or the data (DAT) port.<br>0h = Writes to XRBUF[n] originate from the data port. Writes to<br>XRBUF[n] from the configuration bus are ignored with no effect to<br>the McASP.<br>1h = Writes to XRBUF[n] originate from the configuration bus. Writes<br>to XRBUF[n] from the data port are ignored with no effect to the<br>McASP. |
| 2-0 | XROT   | R/W  | 0h    | Right-rotation value for transmit rotate right format unit.<br>0h = Rotate right by 0 (no rotation).<br>1h = Rotate right by 4 bit positions.<br>2h = Rotate right by 8 bit positions.<br>3h = Rotate right by 12 bit positions.<br>4h = Rotate right by 16 bit positions.<br>5h = Rotate right by 20 bit positions.<br>6h = Rotate right by 24 bit positions.<br>7h = Rotate right by 28 bit positions.                               |
#### **22.4.1.27 AFSXCTL Register (offset = ACh) [reset = 0h]**

AFSXCTL is shown in [Figure](#page-97-1) 22-65 and described in Table [22-38](#page-97-2).

The transmit frame sync control register (AFSXCTL) configures the transmit frame sync (AFSX).

**Figure 22-65. AFSXCTL Register**

| 31       | 30       | 29 | 28     | 27       | 26   | 25     | 24     |
|----------|----------|----|--------|----------|------|--------|--------|--
|          | RESERVED |    |        |          |      |        |        |
|          | R-0h     |    |        |          |      |        |        |
| 23       | 22       | 21 | 20     | 19       | 18   | 17     | 16     |
| RESERVED |          |    |        |          |      |        |        |
| R-0h     |          |    |        |          |      |        |        |
| 15       | 14       | 13 | 12     | 11       | 10   | 9      | 8      |
|          |          |    |        | XMOD     |      |        |        |
|          |          |    |        | R/W-0h   |      |        |        |
| 7        | 6        | 5  | 4      | 3        | 2    | 1      | 0      |
| XMOD     | RESERVED |    | FXWID  | RESERVED |      | FSXM   | FSXP   |
| R/W-0h   | R-0h     |    | R/W-0h |          | R-0h | R/W-0h | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-38. AFSXCTL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                           |
|-------|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                       |
| 15-7  | XMOD     | R/W  | 0h    | Transmit frame sync mode select bits.<br>1FFh = Reserved from 181h to 1FFh.<br>0h = Burst mode.<br>1h = Reserved.<br>2h = 2-slot TDM (I2S mode) to 32-slot TDM from 2h to 20h.<br>21h = Reserved from 21h to 17Fh.<br>180h = 384-slot DIT mode.<br>181h = Reserved from 181h to 1FFh. |
| 6-5   | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                       |
| 4     | FXWID    | R/W  | 0h    | Transmit frame sync width select bit indicates the width of the<br>transmit frame sync (AFSX) during its active period.<br>0h = Single bit.<br>1h = Single word.                                                                                                                      |
| 3-2   | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                       |
| 1     | FSXM     | R/W  | 0h    | Transmit frame sync generation select bit.<br>0h = Externally-generated transmit frame sync.<br>1h = Internally-generated transmit frame sync.                                                                                                                                        |
| 0     | FSXP     | R/W  | 0h    | Transmit frame sync polarity select bit.<br>0h = A rising edge on transmit frame sync (AFSX) indicates the<br>beginning of a frame.<br>1h = A falling edge on transmit frame sync (AFSX) indicates the<br>beginning of a frame.                                                       |
#### **22.4.1.28 ACLKXCTL Register (offset = B0h) [reset = 60h]**

ACLKXCTL is shown in [Figure](#page-98-1) 22-66 and described in Table [22-39.](#page-98-2)

The transmit clock control register (ACLKXCTL) configures the transmit bit clock (ACLKX) and the transmit clock generator.

#### **Figure 22-66. ACLKXCTL Register**

| 31     | 30       | 29     | 28 | 27       | 26      | 25 | 24 |
|--------|----------|--------|----|----------|---------|----|----|--
|        | RESERVED |        |    |          |         |    |    |
|        | R-0h     |        |    |          |         |    |    |
| 23     | 22       | 21     | 20 | 19       | 18      | 17 | 16 |
|        | RESERVED |        |    |          |         |    |    |
|        | R-0h     |        |    |          |         |    |    |
| 15     | 14       | 13     | 12 | 11       | 10      | 9  | 8  |
|        |          |        |    | RESERVED |         |    |    |
|        |          |        |    | R-0h     |         |    |    |
| 7      | 6        | 5      | 4  | 3        | 2       | 1  | 0  |
| CLKXP  | ASYNC    | CLKXM  |    |          | CLKXDIV |    |    |
| R/W-0h | R/W-1h   | R/W-1h |    |          | R/W-0h  |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-39. ACLKXCTL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                     |
|------|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | RESERVED | R    | 0h    |                                                                                                                                                                                 |
| 7    | CLKXP    | R/W  | 0h    | Transmit bitstream clock polarity select bit.                                                                                                                                   |
|      |          |      |       | 0h = Rising edge. External receiver samples data on the falling edge<br>of the serial clock, so the transmitter must shift data out on the rising<br>edge of the serial clock.  |
|      |          |      |       | 1h = Falling edge. External receiver samples data on the rising edge<br>of the serial clock, so the transmitter must shift data out on the falling<br>edge of the serial clock. |
| 6    | ASYNC    | R/W  | 1h    | Transmit/receive operation asynchronous enable bit.                                                                                                                             |
|      |          |      |       | 0h = Synchronous. Transmit clock and frame sync provides the<br>source for both the transmit and receive sections.                                                              |
|      |          |      |       | 1h = Asynchronous. Separate clock and frame sync used by transmit<br>and receive sections.                                                                                      |
| 5    | CLKXM    | R/W  | 1h    | Transmit bit clock source bit.                                                                                                                                                  |
|      |          |      |       | 0h = External transmit clock source from ACLKX pin.                                                                                                                             |
|      |          |      |       | 1h = Internal transmit clock source from output of programmable bit<br>clock divider.                                                                                           |
| 4-0  | CLKXDIV  | R/W  | 0h    | Transmit bit clock divide ratio bits determine the divide-down ratio<br>from AHCLKX to ACLKX.                                                                                   |
|      |          |      |       | 0h = Divide-by-1.                                                                                                                                                               |
|      |          |      |       | 1h = Divide-by-2.                                                                                                                                                               |
|      |          |      |       | 2h = Divide-by-3 to divide-by-32 from 2h to 1Fh.                                                                                                                                |
#### **22.4.1.29 AHCLKXCTL Register (offset = B4h) [reset = 0h]**

AHCLKXCTL is shown in [Figure](#page-99-1) 22-67 and described in Table [22-40](#page-99-2).

The transmit high-frequency clock control register (AHCLKXCTL) configures the transmit high-frequency master clock (AHCLKX) and the transmit clock generator.

**Figure 22-67. AHCLKXCTL Register**

| 31     | 30       | 29       | 28 | 27       | 26     | 25 | 24 |
|--------|----------|----------|----|----------|--------|----|----|--
|        | RESERVED |          |    |          |        |    |    |
| R-0h   |          |          |    |          |        |    |    |
| 23     | 22       | 21       | 20 | 19       | 18     | 17 | 16 |
|        | RESERVED |          |    |          |        |    |    |
| R-0h   |          |          |    |          |        |    |    |
| 15     | 14       | 13       | 12 | 11       | 10     | 9  | 8  |
| HCLKXM | HCLKXP   | RESERVED |    | HCLKXDIV |        |    |    |
| R/W-0h | R/W-0h   | R-0h     |    |          | R/W-0h |    |    |
| 7      | 6        | 5        | 4  | 3        | 2      | 1  | 0  |
|        | HCLKXDIV |          |    |          |        |    |    |
|        |          |          |    | R/W-0h   |        |    |    |
|        |          |          |    |          |        |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-40. AHCLKXCTL Register Field Descriptions**

| Bit   | Field    | Type | Reset                                                                                                                                                                                                                              | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|-------|----------|------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h                                                                                                                                                                                                                                 |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 15    | HCLKXM   | R/W  | 0h<br>Transmit high-frequency clock source bit.<br>0h = External transmit high-frequency clock source from AHCLKX<br>pin.<br>1h = Internal transmit high-frequency clock source from output of<br>programmable high clock divider. |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 14    | HCLKXP   | R/W  | 0h                                                                                                                                                                                                                                 | Transmit bitstream high-frequency clock polarity select bit.<br>0h = AHCLKX is not inverted before programmable bit clock divider.<br>In the special case where the transmit bit clock (ACLKX) is internally<br>generated and the programmable bit clock divider is set to divide-by<br>1 (CLKXDIV = 0 in ACLKXCTL), AHCLKX is directly passed through<br>to the ACLKX pin.<br>1h = AHCLKX is inverted before programmable bit clock divider. In<br>the special case where the transmit bit clock (ACLKX) is internally<br>generated and the programmable bit clock divider is set to divide-by<br>1 (CLKXDIV = 0 in ACLKXCTL), AHCLKX is directly passed through<br>to the ACLKX pin. |
| 13-12 | RESERVED | R    | 0h                                                                                                                                                                                                                                 |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 11-0  | HCLKXDIV | R/W  | 0h                                                                                                                                                                                                                                 | Transmit high-frequency clock divide ratio bits determine the divide<br>down ratio from AUXCLK to AHCLKX.<br>0h = Divide-by-1.<br>1h = Divide-by-2.<br>2h = Divide-by-3 to divide-by-4096 from 2h to FFFh.                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
#### **22.4.1.30 XTDM Register (offset = B8h) [reset = 0h]**

XTDM is shown in [Figure](#page-100-1) 22-68 and described in Table [22-41](#page-100-2).

The transmit TDM time slot register (XTDM) specifies in which TDM time slot the transmitter is active. TDM time slot counter range is extended to 384 slots (to support SPDIF blocks of 384 subframes). XTDM operates modulo 32, that is, XTDMS specifies the TDM activity for time slots 0, 32, 64, 96, 128, and so on.

#### **Figure 22-68. XTDM Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 22-41. XTDM Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                   |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | XTDMS | R/W  | 0h    | Transmitter mode during TDM time slot n.                                                                                                                      |
|      |       |      |       | 0h = Transmit TDM time slot n is inactive. The transmit serializer<br>does not shift out data during this slot.                                               |
|      |       |      |       | 1h = Transmit TDM time slot n is active. The transmit serializer shifts<br>out data during this slot according to the serializer control register<br>(SRCTL). |
#### **22.4.1.31 XINTCTL Register (offset = BCh) [reset = 0h]**

XINTCTL is shown in [Figure](#page-101-1) 22-69 and described in Table [22-42.](#page-101-2)

The transmitter interrupt control register (XINTCTL) controls generation of the McASP transmit interrupt (XINT). When the register bit(s) is set to 1, the occurrence of the enabled McASP condition(s) generates XINT. See the XSTAT register for a description of the interrupt conditions.

**Figure 22-69. XINTCTL Register**

| 31      | 30       | 29     | 28     | 27       | 26      | 25       | 24
|---------|----------|--------|--------|----------|---------|----------|--------|--|--
|         | RESERVED |        |        |          |         |          |
|         | R-0h     |        |        |          |         |          |
| 23      | 22       | 21     | 20     | 19       | 18      | 17       | 16
|         | RESERVED |        |        |          |         |          |
| R-0h    |          |        |        |          |         |          |
| 15      | 14       | 13     | 12     | 11       | 10      | 9        | 8
|         |          |        |        | RESERVED |         |          |
|         |          |        |        | R-0h     |         |          |
| 7       | 6        | 5      | 4      | 3        | 2       | 1        | 0
| XSTAFRM | RESERVED | XDATA  | XLAST  | XDMAERR  | XCKFAIL | XSYNCERR | XUNDRN
| R/W-0h  | R-0h     | R/W-0h | R/W-0h | R/W-0h   | R/W-0h  | R/W-0h   | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-42. XINTCTL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                    |
|------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | RESERVED | R    | 0h    |                                                                                                                                                                |
| 7    | XSTAFRM  | R/W  | 0h    | Transmit start of frame interrupt enable bit.                                                                                                                  |
|      |          |      |       | 0h = Interrupt is disabled. A transmit start of frame interrupt does not<br>generate a McASP transmit interrupt (XINT).                                        |
|      |          |      |       | 1h = Interrupt is enabled. A transmit start of frame interrupt<br>generates a McASP transmit interrupt (XINT).                                                 |
| 6    | RESERVED | R    | 0h    |                                                                                                                                                                |
| 5    | XDATA    | R/W  | 0h    | Transmit data ready interrupt enable bit.                                                                                                                      |
|      |          |      |       | 0h = Interrupt is disabled. A transmit data ready interrupt does not<br>generate a McASP transmit interrupt (XINT).                                            |
|      |          |      |       | 1h = Interrupt is enabled. A transmit data ready interrupt generates a<br>McASP transmit interrupt (XINT).                                                     |
| 4    | XLAST    | R/W  | 0h    | Transmit last slot interrupt enable bit.<br>0h = Interrupt is disabled. A transmit last slot interrupt does not<br>generate a McASP transmit interrupt (XINT). |
|      |          |      |       | 1h = Interrupt is enabled. A transmit last slot interrupt generates a<br>McASP transmit interrupt (XINT).                                                      |
| 3    | XDMAERR  | R/W  | 0h    | Transmit DMA error interrupt enable bit.                                                                                                                       |
|      |          |      |       | 0h = Interrupt is disabled. A transmit DMA error interrupt does not<br>generate a McASP transmit interrupt (XINT).                                             |
|      |          |      |       | 1h = Interrupt is enabled. A transmit DMA error interrupt generates a<br>McASP transmit interrupt (XINT).                                                      |
| 2    | XCKFAIL  | R/W  | 0h    | Transmit clock failure interrupt enable bit.                                                                                                                   |
|      |          |      |       | 0h = Interrupt is disabled. A transmit clock failure interrupt does not<br>generate a McASP transmit interrupt (XINT).                                         |
|      |          |      |       | 1h = Interrupt is enabled. A transmit clock failure interrupt generates<br>a McASP transmit interrupt (XINT).                                                  |
**Table 22-42. XINTCTL Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset | Description                                                                                                                     |
|-----|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------|
| 1   | XSYNCERR | R/W  | 0h    | Unexpected transmit frame sync interrupt enable bit.                                                                            |
|     |          |      |       | 0h = Interrupt is disabled. An unexpected transmit frame sync<br>interrupt does not generate a McASP transmit interrupt (XINT). |
|     |          |      |       | 1h = Interrupt is enabled. An unexpected transmit frame sync<br>interrupt generates a McASP transmit interrupt (XINT).          |
| 0   | XUNDRN   | R/W  | 0h    | Transmitter underrun interrupt enable bit.                                                                                      |
|     |          |      |       | 0h = Interrupt is disabled. A transmitter underrun interrupt does not<br>generate a McASP transmit interrupt (XINT).            |
|     |          |      |       | 1h = Interrupt is enabled. A transmitter underrun interrupt generates<br>a McASP transmit interrupt (XINT).                     |
#### **22.4.1.32 XSTAT Register (offset = C0h) [reset = 0h]**

XSTAT is shown in [Figure](#page-103-1) 22-70 and described in Table [22-43](#page-103-2).

The transmitter status register (XSTAT) provides the transmitter status and transmit TDM time slot number. If the McASP logic attempts to set an interrupt flag in the same cycle that the CPU writes to the flag to clear it, the McASP logic has priority and the flag remains set. This also causes a new interrupt request to be generated.

**Figure 22-70. XSTAT Register**

| 31       | 30       | 29       | 28       | 27       | 26       | 25       | 24
|----------|----------|----------|----------|----------|----------|----------|----------|--|--
|          | RESERVED |          |          |          |          |          |
|          | R-0h     |          |          |          |          |          |
| 23       | 22       | 21       | 20       | 19       | 18       | 17       | 16
|          | RESERVED |          |          |          |          |          |
| R-0h     |          |          |          |          |          |          |
| 15       | 14       | 13       | 12       | 11       | 10       | 9        | 8
|          |          |          | RESERVED |          |          |          | XERR
|          |          |          | R-0h     |          |          |          | R/W-0h
| 7        | 6        | 5        | 4        | 3        | 2        | 1        | 0
| XDMAERR  | XSTAFRM  | XDATA    | XLAST    | XTDMSLOT | XCKFAIL  | XSYNCERR | XUNDRN
| R/W1C-0h | R/W1C-0h | R/W1C-0h | R/W1C-0h | R-0h     | R/W1C-0h | R/W1C-0h | R/W1C-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-43. XSTAT Register Field Descriptions**

| Bit  | Field    | Type  | Reset | Description
|------|----------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--
| 31-9 | RESERVED | R     | 0h    |
| 8    | XERR     | R/W   | 0h    | XERR bit always returns a logic-OR of: XUNDRN OR XSYNCERROR XCKFAIL OR XDMAERR.<br>Allows a single bit to be checked to determine if a transmitter error<br>interrupt has occurred.<br>0h = No errors have occurred.<br>1h = An error has occurred.
| 7    | XDMAERR  | R/W1C | 0h    | Transmit DMA error flag.<br>XDMAERR is set when the CPU or DMA writes more serializers<br>through the data port in a given time slot than were programmed as<br>transmitters.<br>Causes a transmit interrupt (XINT), if this bit is set and XDMAERR in<br>XINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 has no effect.<br>0h = Transmit DMA error did not occur.<br>1h = Transmit DMA error did occur.
| 6    | XSTAFRM  | R/W1C | 0h    | Transmit start of frame flag.<br>Causes a transmit interrupt (XINT), if this bit is set and XSTAFRM in<br>XINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 has no effect.<br>0h = No new transmit frame sync (AFSX) is detected.<br>1h = A new transmit frame sync (AFSX) is detected.
| 5    | XDATA    | R/W1C | 0h    | Transmit data ready flag.<br>Causes a transmit interrupt (XINT), if this bit is set and XDATA in<br>XINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 has no effect.<br>0h = XBUF is written and is full.<br>1h = Data is copied from XBUF to XRSR. XBUF is empty and ready<br>to be written. XDATA is also set when the transmit serializers are<br>taken out of reset. When XDATA is set, it always causes a DMA<br>event (AXEVT).
**Table 22-43. XSTAT Register Field Descriptions (continued)**

| Bit | Field    | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|-----|----------|-------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 4   | XLAST    | R/W1C | 0h    | Transmit last slot flag.<br>XLAST is set along with XDATA, if the current slot is the last slot in a<br>frame.<br>Causes a transmit interrupt (XINT), if this bit is set and XLAST in<br>XINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 has no effect.<br>0h = Current slot is not the last slot in a frame.<br>1h = Current slot is the last slot in a frame. XDATA is also set.                                                                                                                                                         |
| 3   | XTDMSLOT | R     | 0h    | Returns the LSB of XSLOT.<br>Allows a single read of XSTAT to determine whether the current<br>TDM time slot is even or odd.<br>0h = Current TDM time slot is odd.<br>1h = Current TDM time slot is even.                                                                                                                                                                                                                                                                                                                                                                    |
| 2   | XCKFAIL  | R/W1C | 0h    | Transmit clock failure flag.<br>XCKFAIL is set when the transmit clock failure detection circuit<br>reports an error.<br>Causes a transmit interrupt (XINT), if this bit is set and XCKFAIL in<br>XINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 has no effect.<br>0h = Transmit clock failure did not occur.<br>1h = Transmit clock failure did occur.                                                                                                                                                                                   |
| 1   | XSYNCERR | R/W1C | 0h    | Unexpected transmit frame sync flag.<br>XSYNCERR is set when a new transmit frame sync (AFSX) occurs<br>before it is expected.<br>Causes a transmit interrupt (XINT), if this bit is set and XSYNCERR<br>in XINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 has no effect.<br>0h = Unexpected transmit frame sync did not occur.<br>1h = Unexpected transmit frame sync did occur.                                                                                                                                                         |
| 0   | XUNDRN   | R/W1C | 0h    | Transmitter underrun flag.<br>XUNDRN is set when the transmit serializer is instructed to transfer<br>data from XBUF to XRSR, but XBUF has not yet been serviced with<br>new data since the last transfer.<br>Causes a transmit interrupt (XINT), if this bit is set and XUNDRN in<br>XINTCTL is set.<br>This bit is cleared by writing a 1 to this bit.<br>Writing a 0 has no effect.<br>0h = Transmitter underrun did not occur.<br>1h = Transmitter underrun did occur. For details on McASP action<br>upon underrun conditions, see Buffer Underrun Error - Transmitter. |
#### **22.4.1.33 XSLOT Register (offset = C4h) [reset = 0h]**

XSLOT is shown in [Figure](#page-105-1) 22-71 and described in Table [22-44.](#page-105-2)

The current transmit TDM time slot register (XSLOT) indicates the current time slot for the transmit data frame.

**Figure 22-71. XSLOT Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20       | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4        | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|---|---|---|---|---|----------|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | XSLOTCNT |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | R-0h     |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-44. XSLOT Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                            |
|-------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                        |
| 9-0   | XSLOTCNT | R    | 0h    | Current transmit time slot count.<br>Legal values: 0 to 383 (17Fh).<br>During reset, this counter value is 383 so the next count value, which<br>is used to encode the first DIT group of data, will be 0 and encodes<br>the B preamble.<br>TDM function is not supported for >32 time slots.<br>However, TDM time slot counter may count to 383 when used to<br>transmit a DIT block. |
#### **22.4.1.34 XCLKCHK Register (offset = C8h) [reset = 0h]**

XCLKCHK is shown in [Figure](#page-106-1) 22-72 and described in Table [22-45](#page-106-2).

The transmit clock check control register (XCLKCHK) configures the transmit clock failure detection circuit.

### **Figure 22-72. XCLKCHK Register**

| 31 | 30 | 29 | 28 | 27     | 26 | 25 | 24 | 23 | 22 | 21       | 20 | 19     | 18 | 17     | 16 |
|----|----|----|----|--------|----|----|----|----|----|----------|----|--------|----|--------|----|
|    |    |    |    | XCNT   |    |    |    |    |    |          |    | XMAX   |    |        |    |
|    |    |    |    | R-0h   |    |    |    |    |    |          |    | R/W-0h |    |        |    |
| 15 | 14 | 13 | 12 | 11     | 10 | 9  | 8  | 7  | 6  | 5        | 4  | 3      | 2  | 1      | 0  |
|    |    |    |    | XMIN   |    |    |    |    |    | RESERVED |    |        |    | XPS    |    |
|    |    |    |    | R/W-0h |    |    |    |    |    | R-0h     |    |        |    | R/W-0h |    |
|    |    |    |    |        |    |    |    |    |    |          |    |        |    |        |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 22-45. XCLKCHK Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                       |
|-------|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | XCNT     | R    | 0h    | Transmit clock count value (from previous measurement).<br>The clock circuit continually counts the number of system clocks for<br>every 32 transmit high-frequency master clock (AHCLKX) signals,<br>and stores the count in XCNT until the next measurement is taken.                                                                                                                           |
| 23-16 | XMAX     | R/W  | 0h    | Transmit clock maximum boundary.<br>This 8 bit unsigned value sets the maximum allowed boundary for<br>the clock check counter after 32 transmit high-frequency master<br>clock (AHCLKX) signals have been received.<br>If the current counter value is greater than XMAX after counting 32AHCLKX signals, XCKFAIL in XSTAT is set.<br>The comparison is performed using unsigned arithmetic. |
| 15-8  | XMIN     | R/W  | 0h    | Transmit clock minimum boundary.<br>This 8 bit unsigned value sets the minimum allowed boundary for the<br>clock check counter after 32 transmit high-frequency master clock<br>(AHCLKX) signals have been received.<br>If XCNT is less than XMIN after counting 32 AHCLKX signals,<br>XCKFAIL in XSTAT is set.<br>The comparison is performed using unsigned arithmetic.                         |
| 7-4   | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                   |
| 3-0   | XPS      | R/W  | 0h    | Transmit clock check prescaler value.<br>Fh = Reserved from 9h to Fh.                                                                                                                                                                                                                                                                                                                             |
|       |          |      |       | 0h = McASP system clock divided by 1.                                                                                                                                                                                                                                                                                                                                                             |
|       |          |      |       | 1h = McASP system clock divided by 2.                                                                                                                                                                                                                                                                                                                                                             |
|       |          |      |       | 2h = McASP system clock divided by 4.                                                                                                                                                                                                                                                                                                                                                             |
|       |          |      |       | 3h = McASP system clock divided by 8.                                                                                                                                                                                                                                                                                                                                                             |
|       |          |      |       | 4h = McASP system clock divided by 16.                                                                                                                                                                                                                                                                                                                                                            |
|       |          |      |       | 5h = McASP system clock divided by 32.                                                                                                                                                                                                                                                                                                                                                            |
|       |          |      |       | 6h = McASP system clock divided by 64.                                                                                                                                                                                                                                                                                                                                                            |
|       |          |      |       | 7h = McASP system clock divided by 128.                                                                                                                                                                                                                                                                                                                                                           |
|       |          |      |       | 8h = McASP system clock divided by 256.<br>9h = Reserved from 9h to Fh.                                                                                                                                                                                                                                                                                                                           |
|       |          |      |       |                                                                                                                                                                                                                                                                                                                                                                                                   |
#### **22.4.1.35 XEVTCTL Register (offset = CCh) [reset = 0h]**

XEVTCTL is shown in [Figure](#page-107-1) 22-73 and described in Table [22-46](#page-107-2).

The transmitter DMA event control register (XEVTCTL) contains a disable bit for the transmit DMA event. Note for device-specific registers: Accessing XEVTCTL not implemented on a specific device may cause improper device operation.

**Figure 22-73. XEVTCTL Register**

| 31 | 30 | 29 | 28       | 27       | 26 | 25 | 24      |
|----|----|----|----------|----------|----|----|---------|
|    |    |    |          | RESERVED |    |    |         |
|    |    |    |          | R-0h     |    |    |         |
| 23 | 22 | 21 | 20       | 19       | 18 | 17 | 16      |
|    |    |    |          | RESERVED |    |    |         |
|    |    |    |          | R-0h     |    |    |         |
| 15 | 14 | 13 | 12       | 11       | 10 | 9  | 8       |
|    |    |    |          | RESERVED |    |    |         |
|    |    |    |          | R-0h     |    |    |         |
| 7  | 6  | 5  | 4        | 3        | 2  | 1  | 0       |
|    |    |    | RESERVED |          |    |    | XDATDMA |
|    |    |    | R-0h     |          |    |    | R/W-0h  |
|    |    |    |          |          |    |    |         |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-46. XEVTCTL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                          |
|------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | RESERVED | R    | 0h    |                                                                                                                                                                      |
| 0    | XDATDMA  | R/W  | 0h    | Transmit data DMA request enable bit.<br>If writing to this bit, always write the default value of 0.<br>0h = Transmit data DMA request is enabled.<br>1h = Reserved |
#### **22.4.1.36 DITCSRA\_0 to DITCSRA\_5 Register (offset = 100h to 114h) [reset = 0h]**

DITCSRA\_0 to DITCSRA\_5 is shown in [Figure](#page-108-1) 22-74 and described in Table [22-47](#page-108-2).

The DIT left channel status registers (DITCSRA) provide the status of each left channel (even TDM time slot). Each of the six 32-bit registers can store 192 bits of channel status data for a complete block of transmission. The DIT reuses the same data for the next block. It is your responsibility to update the register file in time, if a different set of data need to be sent.

### **Figure 22-74. DITCSRA\_0 to DITCSRA\_5 Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-47. DITCSRA\_0 to DITCSRA\_5 Register Field Descriptions**

| Bit  | Field   | Type | Reset | Description                        |
|------|---------|------|-------|------------------------------------|
| 31-0 | DITCSRA | R/W  | 0h    | DIT left channel status registers. |
#### **22.4.1.37 DITCSRB\_0 to DITCSRB\_5 Register (offset = 118h to 12Ch) [reset = 0h]**

DITCSRB\_0 to DITCSRB\_5 is shown in [Figure](#page-109-1) 22-75 and described in Table [22-48](#page-109-2).

The DIT right channel status registers (DITCSRB) provide the status of each right channel (odd TDM time slot). Each of the six 32-bit registers can store 192 bits of channel status data for a complete block of transmission. The DIT reuses the same data for the next block. It is your responsibility to update the register file in time, if a different set of data need to be sent.

#### **Figure 22-75. DITCSRB\_0 to DITCSRB\_5 Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15      | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|---------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | DITCSRB |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-0h  |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-48. DITCSRB\_0 to DITCSRB\_5 Register Field Descriptions**

| Bit  | Field   | Type | Reset | Description                         |
|------|---------|------|-------|-------------------------------------|
| 31-0 | DITCSRB | R/W  | 0h    | DIT right channel status registers. |
#### **22.4.1.38 DITUDRA\_0 to DITUDRA\_5 Register (offset = 130h to 144h) [reset = 0h]**

DITUDRA\_0 to DITUDRA\_5 is shown in [Figure](#page-110-1) 22-76 and described in Table [22-49.](#page-110-2)

The DIT left channel user data registers (DITUDRA) provides the user data of each left channel (even TDM time slot). Each of the six 32-bit registers can store 192 bits of user data for a complete block of transmission. The DIT reuses the same data for the next block. It is your responsibility to update the register in time, if a different set of data need to be sent.

### **Figure 22-76. DITUDRA\_0 to DITUDRA\_5 Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-49. DITUDRA\_0 to DITUDRA\_5 Register Field Descriptions**

| Bit  | Field   | Type | Reset | Description                           |
|------|---------|------|-------|---------------------------------------|
| 31-0 | DITUDRA | R/W  | 0h    | DIT left channel user data registers. |
#### **22.4.1.39 DITUDRB\_0 to DITUDRB\_5 Register (offset = 148h to 15Ch) [reset = 0h]**

DITUDRB\_0 to DITUDRB\_5 is shown in [Figure](#page-111-1) 22-77 and described in Table [22-50.](#page-111-2)

The DIT right channel user data registers (DITUDRB) provides the user data of each right channel (odd TDM time slot). Each of the six 32-bit registers can store 192 bits of user data for a complete block of transmission. The DIT reuses the same data for the next block. It is your responsibility to update the register in time, if a different set of data need to be sent.

#### **Figure 22-77. DITUDRB\_0 to DITUDRB\_5 Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15      | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|---------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | DITUDRB |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-0h  |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-50. DITUDRB\_0 to DITUDRB\_5 Register Field Descriptions**

| Bit  | Field   | Type | Reset | Description                            |
|------|---------|------|-------|----------------------------------------|
| 31-0 | DITUDRB | R/W  | 0h    | DIT right channel user data registers. |
#### **22.4.1.40 SRCTL\_0 to SRCTL\_5 Register (offset = 180h to 194h) [reset = 0h]**

SRCTL\_0 to SRCTL\_5 is shown in [Figure](#page-112-1) 22-78 and described in Table [22-51](#page-112-2).

Each serializer on the McASP has a serializer control register (SRCTL). There are up to 16 serializers per McASP. Note for device-specific registers: Accessing SRCTLn not implemented on a specific device may cause improper device operation.

**Figure 22-78. SRCTL\_0 to SRCTL\_5 Register**

| 31       | 30   | 29   | 28       | 27 | 26     | 25     | 24 |
|----------|------|------|----------|----|--------|--------|----|
|          |      |      | RESERVED |    |        |        |    |
|          |      |      | R-0h     |    |        |        |    |
| 23       | 22   | 21   | 20       | 19 | 18     | 17     | 16 |
|          |      |      | RESERVED |    |        |        |    |
|          |      |      | R-0h     |    |        |        |    |
| 15       | 14   | 13   | 12       | 11 | 10     | 9      | 8  |
|          |      |      | RESERVED |    |        |        |    |
|          |      |      | R-0h     |    |        |        |    |
| 7        | 6    | 5    | 4        | 3  | 2      | 1      | 0  |
| RESERVED |      | RRDY | XRDY     |    | DISMOD | SRMOD  |    |
|          | R-0h | R-0h | R-0h     |    | R/W-0h | R/W-0h |    |
|          |      |      |          |    |        |        |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-51. SRCTL\_0 to SRCTL\_5 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-6 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 5    | RRDY     | R    | 0h    | Receive buffer ready bit.<br>RRDY indicates the current receive buffer state.<br>Always reads 0 when programmed as a transmitter or as inactive.<br>If SRMOD bit is set to receive (2h), RRDY switches from 0 to 1<br>whenever data is transferred from XRSR to RBUF.                                                                                                                                                                                                                                                                                                                                                                                                 |
|      |          |      |       | 0h = Receive buffer (RBUF) is empty.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|      |          |      |       | 1h = Receive buffer (RBUF) contains data and needs to be read<br>before the start of the next time slot or a receiver overrun occurs.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 4    | XRDY     | R    | 0h    | Transmit buffer ready bit.<br>XRDY indicates the current transmit buffer state.<br>Always reads 0 when programmed as a receiver or as inactive.<br>If SRMOD bit is set to transmit (1h), XRDY switches from 0 to 1<br>when XSRCLR in GBLCTL is switched from 0 to 1 to indicate an<br>empty transmitter.<br>XRDY remains set until XSRCLR is forced to 0, data is written to the<br>corresponding transmit buffer, or SRMOD bit is changed to receive<br>(2h) or inactive (0).<br>0h = Transmit buffer (XBUF) contains data.<br>1h = Transmit buffer (XBUF) is empty and needs to be written before<br>the start of the next time slot or a transmit underrun occurs. |
| 3-2  | DISMOD   | R/W  | 0h    | Serializer pin drive mode bit.<br>Drive on pin when in inactive TDM slot of transmit mode or when<br>serializer is inactive.<br>This field only applies if the pin is configured as a McASP pin<br>(PFUNC = 0).<br>0h = Drive on pin is 3-state.<br>1h = Reserved.<br>2h = Drive on pin is logic low.<br>3h = Drive on pin is logic high.                                                                                                                                                                                                                                                                                                                             |
**Table 22-51. SRCTL\_0 to SRCTL\_5 Register Field Descriptions (continued)**

| Bit | Field | Type | Reset | Description                     |
|-----|-------|------|-------|---------------------------------|
| 1-0 | SRMOD | R/W  | 0h    | Serializer mode bit.            |
|     |       |      |       | 0h = Serializer is inactive.    |
|     |       |      |       | 1h = Serializer is transmitter. |
|     |       |      |       | 2h = Serializer is receiver.    |
|     |       |      |       | 3h = Reserved.                  |
#### **22.4.1.41 XBUF\_0 to XBUF\_5 Register (offset = 200h to 214h) [reset = 0h]**

XBUF\_0 to XBUF\_5 is shown in [Figure](#page-114-1) 22-79 and described in Table [22-52.](#page-114-2)

The transmit buffers for the serializers (XBUF) hold data from the transmit format unit. For transmit operations, the XBUF is an alias of the XRBUF in the serializer. Accessing XBUF registers not implemented on a specific device may cause improper device operation.

**Figure 22-79. XBUF\_0 to XBUF\_5 Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 22-52. XBUF\_0 to XBUF\_5 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                       |
|------|-------|------|-------|-----------------------------------|
| 31-0 | XBUF  | R/W  | 0h    | Transmit buffers for serializers. |
#### **22.4.1.42 RBUF\_0 to RBUF\_5 Register (offset = 280h to 294h) [reset = 0h]**

RBUF\_0 to RBUF\_5 is shown in [Figure](#page-115-1) 22-80 and described in Table [22-53](#page-115-2).

The receive buffers for the serializers (RBUF) hold data from the serializer before the data goes to the receive format unit. For receive operations, the RBUF is an alias of the XRBUF in the serializer. Accessing XBUF registers not implemented on a specific device may cause improper device operation.

**Figure 22-80. RBUF\_0 to RBUF\_5 Register**


LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-53. RBUF\_0 to RBUF\_5 Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                      |
|------|-------|------|-------|----------------------------------|
| 31-0 | RBUF  | R/W  | 0h    | Receive buffers for serializers. |
#### **22.4.1.43 WFIFOCTL Register (offset = 1000h) [reset = 0h]**

WFIFOCTL is shown in [Figure](#page-116-1) 22-81 and described in Table [22-54](#page-116-2).

The WNUMEVT and WNUMDMA values must be set prior to enabling the Write FIFO. If the Write FIFO is to be enabled, it must be enabled prior to taking the McASP out of reset

**Figure 22-81. WFIFOCTL Register**

| 31 | 30 | 29 | 28       | 27       | 26 | 25 | 24     |
|----|----|----|----------|----------|----|----|--------|
|    |    |    |          | RESERVED |    |    |        |
|    |    |    |          | R-0h     |    |    |        |
| 23 | 22 | 21 | 20       | 19       | 18 | 17 | 16     |
|    |    |    | RESERVED |          |    |    | WENA   |
|    |    |    | R-0h     |          |    |    | R/W-0h |
| 15 | 14 | 13 | 12       | 11       | 10 | 9  | 8      |
|    |    |    |          | WNUMEVT  |    |    |        |
|    |    |    |          | R/W-0h   |    |    |        |
| 7  | 6  | 5  | 4        | 3        | 2  | 1  | 0      |
|    |    |    |          | WNUMDMA  |    |    |        |
|    |    |    |          | R/W-0h   |    |    |        |
|    |    |    |          |          |    |    |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 22-54. WFIFOCTL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-17 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 16    | WENA     | R/W  | 0h    | Write FIFO enable bit.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|       |          |      |       | 0h = Write FIFO is disabled. The WLVL bit in the Write FIFO status<br>register (WFIFOSTS) is reset to 0 and pointers are initialized, that is,<br>the Write FIFO is flushed.                                                                                                                                                                                                                                                                                                                                                                                                      |
|       |          |      |       | 1h = Write FIFO is enabled. If Write FIFO is to be enabled, it must<br>be enabled prior to taking McASP out of reset.                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| 15-8  | WNUMEVT  | R/W  | 0h    | Write word count per DMA event (32 bit).<br>When the Write FIFO has space for at least WNUMEVT words of<br>data, then an AXEVT (transmit DMA event) is generated to the<br>host/DMA controller.<br>This value should be set to a non-zero integer multiple of the number<br>of serializers enabled as transmitters.<br>This value must be set prior to enabling the Write FIFO.<br>40h = 3 to 64 words from 3h to 40h.<br>FFh = Reserved from 41h to FFh.<br>0h = 0 words<br>1h = 1 word<br>2h = 2 words<br>3h = 3 to 64 words from 3h to 40h.<br>41h = Reserved from 41h to FFh. |
| 7-0   | WNUMDMA  | R/W  | 0h    | Write word count per transfer (32 bit words).<br>Upon a transmit DMA event from the McASP, WNUMDMA words are<br>transferred from the Write FIFO to the McASP.<br>This value must equal the number of McASP serializers used as<br>transmitters.<br>This value must be set prior to enabling the Write FIFO.<br>FFh = Reserved from 11h to FFh.<br>0h = 0 words<br>1h = 1 word<br>2h = 2 words<br>3h = 3-16 words from 3h to 10h.<br>11h = Reserved from 11h to FFh.                                                                                                               |
#### **22.4.1.44 WFIFOSTS Register (offset = 1004h) [reset = 0h]**

WFIFOSTS is shown in [Figure](#page-117-1) 22-82 and described in Table [22-55.](#page-117-2)

### **Figure 22-82. WFIFOSTS Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20       | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4    | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|---|---|---|---|---|------|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | WLVL |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | R-0h |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 22-55. WFIFOSTS Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                  |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                              |
| 7-0  | WLVL     | R    | 0h    | Write level (read-only).<br>Number of 32 bit words currently in the Write FIFO.<br>40h = 3 to 64 words currently in Write FIFO from 3h to 40h.<br>FFh = Reserved from 41h to FFh.<br>0h = 0 words currently in Write FIFO.<br>1h = 1 word currently in Write FIFO.<br>2h = 2 words currently in Write FIFO.<br>3h = 3 to 64 words currently in Write FIFO from 3h to 40h.<br>41h = Reserved from 41h to FFh. |
#### **22.4.1.45 RFIFOCTL Register (offset = 1008h) [reset = 0h]**

RFIFOCTL is shown in [Figure](#page-118-1) 22-83 and described in Table [22-56.](#page-118-2)

The RNUMEVT and RNUMDMA values must be set prior to enabling the Read FIFO. If the Read FIFO is to be enabled, it must be enabled prior to taking the McASP out of reset

**Figure 22-83. RFIFOCTL Register**

| 31 | 30 | 29 | 28       | 27       | 26 | 25 | 24     |
|----|----|----|----------|----------|----|----|--------|
|    |    |    |          | RESERVED |    |    |        |
|    |    |    |          | R-0h     |    |    |        |
| 23 | 22 | 21 | 20       | 19       | 18 | 17 | 16     |
|    |    |    | RESERVED |          |    |    | RENA   |
|    |    |    | R-0h     |          |    |    | R/W-0h |
| 15 | 14 | 13 | 12       | 11       | 10 | 9  | 8      |
|    |    |    |          | RNUMEVT  |    |    |        |
|    |    |    |          | R/W-0h   |    |    |        |
| 7  | 6  | 5  | 4        | 3        | 2  | 1  | 0      |
|    |    |    |          | RNUMDMA  |    |    |        |
|    |    |    |          | R/W-0h   |    |    |        |
|    |    |    |          |          |    |    |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-56. RFIFOCTL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|-------|----------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-17 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 16    | RENA     | R/W  | 0h    | Read FIFO enable bit.<br>0h = Read FIFO is disabled. The RLVL bit in the Read FIFO status<br>register (RFIFOSTS) is reset to 0 and pointers are initialized, that is,<br>the Read FIFO is flushed.<br>1h = Read FIFO is enabled. If Read FIFO is to be enabled, it must<br>be enabled prior to taking McASP out of reset.                                                                                                                                                                                                                                            |
| 15-8  | RNUMEVT  | R/W  | 0h    | Read word count per DMA event (32 bit).<br>When the Read FIFO contains at least RNUMEVT words of data,<br>then an AREVT (receive DMA event) is generated to the host/DMA<br>controller.<br>This value should be set to a non-zero integer multiple of the number<br>of serializers enabled as receivers.<br>This value must be set prior to enabling the Read FIFO.<br>40h = 3 to 64 words from 3h to 40h.<br>FFh = Reserved from 41h = FFh.<br>0h = 0 words<br>1h = 1 word<br>2h = 2 words<br>3h = 3 to 64 words from 3h to 40h.<br>41h = Reserved from 41h to FFh. |
| 7-0   | RNUMDMA  | R/W  | 0h    | Read word count per transfer (32 bit words).<br>Upon a receive DMA event from the McASP, the Read FIFO reads<br>RNUMDMA words from the McASP.<br>This value must equal the number of McASP serializers used as<br>receivers.<br>This value must be set prior to enabling the Read FIFO.<br>10h = 3 to 16 words from 3h to 10h.<br>FFh = Reserved from 11h to FFh.<br>0h = 0 words<br>1h = 1 word<br>2h = 2 words<br>3h = 3 to 16 words from 3h to 10h.<br>11h = Reserved from 11h to FFh.                                                                            |
#### **22.4.1.46 RFIFOSTS Register (offset = 100Ch) [reset = 0h]**

RFIFOSTS is shown in [Figure](#page-119-1) 22-84 and described in Table [22-57.](#page-119-2)

### **Figure 22-84. RFIFOSTS Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20       | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4    | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|---|---|---|---|---|------|---|---|---|---|
|    |    |    |    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | RLVL |   |   |   |   |
|    |    |    |    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |   |   |   |   |   | R-0h |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 22-57. RFIFOSTS Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                           |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                       |
| 7-0  | RLVL     | R    | 0h    | Read level (read-only).<br>Number of 32 bit words currently in the Read FIFO.<br>40h = 3 to 64 words currently in Read FIFO from 3h to 40h.<br>FFh = Reserved from 41h to FFh.<br>0h = 0 words currently in Read FIFO.<br>1h = 1 word currently in Read FIFO.<br>2h = 2 words currently in Read FIFO.<br>3h = 3 to 64 words currently in Read FIFO from 3h to 40h.<br>41h = Reserved from 41h to FFh. |
