# 13 LCD Controller

This chapter describes the LCD controller of the device.

| 13.1 | Introduction           | 1921 |
|------|------------------------|------|
| 13.2 | Integration            | 1923 |
| 13.3 | Functional Description | 1925 |
| 13.4 | Programming Model      | 1952 |
| 13.5 | Registers              | 1959 |
|      |                        |      |

## **13.1 Introduction**

### *13.1.1 Purpose of the Peripheral*

The LCD controller consists of two independent controllers, the Raster Controller and the LCD Interface Display Driver (LIDD) controller. Each controller operates independently from the other and only one of them is active at any given time.

- The Raster Controller handles the synchronous LCD interface. It provides timing and data for constant graphics refresh to a passive display. It supports a wide variety of monochrome and full-color display types and sizes by use of programmable timing controls, a built-in palette, and a gray-scale/serializer. Graphics data is processed and stored in frame buffers. A frame buffer is a contiguous memory block in the system. A built-in DMA engine supplies the graphics data to the Raster engine which, in turn, outputs to the external LCD device.
- The LIDD Controller supports the asynchronous LCD interface. It provides full-timing programmability of control signals (CS, WE, OE, ALE) and output data.

[Figure](#page-94-1) 13-1 shows the LCD controller details. The raster and LIDD Controllers are responsible for generating the correct external timing. The DMA engine provides a constant flow of data from the frame buffer(s) to the external LCD panel via the Raster and LIDD Controllers. In addition, CPU access is provided to read and write registers.

The solid, thick lines in [Figure](#page-94-1) 13-1 indicate the data path. The Raster Controller's data path is fairly complicated, for a thorough description of the Raster Controller data path, see [Section](#page-108-0) 13.3.6.

```
Figure (page_94 Figure_9) – LCD block diagram (ASCII)

                        LCD_CLK
                          |
                          v
              +----------------------------------------------+
              |                 LCD block                    |
              |                                              |
              |   +-----------+                              |
              |   | Input FIFO|<-------------------+         |
              |   +-----+-----+                    |         |
              |         |                          |         |
              |         v                          |         |
              |   +-----------+                    |         |
              |   | Palette   |<----------------+  |         |
              |   | RAM       |                 |  |         |
              |   +-----+-----+                 |  |         |
              |         |                       |  |         |
              |         v                       |  |         |
              |   +-------------------+         |  |         |
              |   | Gray-scaler /     |<--------+  |         |
              |   | serializer        |            |         |
              |   +---------+---------+            |         |
              |             |                      |         |
              |             v                      |         |
              |        +----------+                |         |
              |        | Output   |                |         |
              |        | FIFO     |                |         |
              |        +----+-----+                |         |
              |             |                      |         |
              |      (STN path)                    |         |
              |             v                      |         |
              |          +-----+                   |         |
              | TFT ---->|MUX  |<----- Output FIFO |         |
              |          +--+--+                   |         |
              |             |                      |         |
              |             v                      |         |
              |          +-----+                   |         |
              |          |MUX  |---------------------------> LCD_D[23:0]
              |          +-----+                   |         |
              |             ^                      |         |
              |             | (select source)      |         |
              |        +----+-----+                |         |
              |        | Raster   |<---------------+         |
              |        | controller|                         |
              |        +----+-----+                          |
              |             |                                |
              |             v                                |
              |        +-----------+                         |
              |        | LIDD      |--------------------+    |
              |        | controller|                    |    |
              |        +-----+-----+                    |    |
              |              \                          v    |
              |               \                      +-----+ |
              |                \-------------------->| MUX |----> LCD_VSYNC
              |                                       +-----+     LCD_HSYNC
              |                                                   LCD_PCLK
              |                                                   LCD_AC_ENB_CS
              |                                                   LCD_MCLK
              |                                              |
              |                +-----------+                 |
              |                | Registers |<-----------------+
              |                +-----------+
              +----------------------------------------------+

      +-------------------+
      |     DMA block     |
CPU<->| DMA control regs  |
r/w   +---------+---------+
                |
                v
             +-----+
             | DMA |-------------------------------> (into LCD block datapath)
             +-----+
```

**Figure 13-1. LCD Controller**

### *13.1.2 Features*

General features of the LCD Controller include:

- Supports up to 24-bit data output; 8 bits-per-pixel (RGB).
- Supports up to 2048x2048 resolution.
- Integrated DMA engine to pull data from the external frame buffer without burdening the processor via interrupts or a firmware timer.
- 512 word deep internal FIFO with programmable threshold values.
- Character Based Panels
  - Supports 2 Character Panels (CS0 and CS1) with independent and programmable bus timing parameters when in asynchronous Hitachi, Motorola and Intel modes
  - Supports 1 Character Panel (CS0) with programmable bus timing parameters when in synchronous Motorola and Intel modes
  - Can be used as a generic 16 bit address/data interleaved MPU bus master with no external stall
- Passive Matrix LCD Panels
  - Panel types including STN, DSTN, and C-DSTN
  - AC Bias Control
- Active Matrix LCD Panels
  - Panel types including TN TFT
- OLED Panels
  - Passive Matrix (PM OLED) with frame buffer and controller IC inside the Panel
  - Active Matrix (AM OLED)

## 13.2 Integration

The device includes an LCD Controller that reads display data from external memory and drives several different types of LCD displays. The LCD Controller integration is shown in Figure 13-2.

```
(page_96 Figure_4) – LCD Controller system view (ASCII)

                    +----------------------------+
L3Fast Interconnect <->                          |
L4Peripheral Interconnect <->   LCD Controller   |---- lcd_cp ---------> LCD_PCLK
                    |   (DMA Master Interface)   |---- lcd_pixel[15:0]-> LCD_DATA[15:0]
                    |   (CFG Interface)          |---- lcd_pixel[23:16]->LCD_DATA[23:16]
                    |                            |---- lcd_lp ---------> LCD_HSYNC
                    |                            |---- lcd_fp ---------> LCD_VSYNC
                    |                            |---- lcd_mclk -------> LCD_MEMORY_CLK
                    |                            |
                    |   intr_pend --------------->|--> MPU Subsystem Interrupts
                    |                            |
                    |   lcd_int_clk              |
                    |   lcd_clk  <---------------+  (input clock)
                    +-------------^--------------+
                                  |
                                  |  LCD_CLK
Disp PLL CLKOUT ---> +------+     |
                     | PRCM |-----+
                     +------+

Right-side group: "LCDC Pads"
- LCD_PCLK
- LCD_DATA[15:0]
- LCD_DATA[23:16]
- LCD_HSYNC
- LCD_VSYNC
- LCD_MEMORY_CLK
```

Figure 13-2. LCD Controller Integration

### 13.2.1 LCD Controller Connectivity Attributes

The general connectivity attributes for the LCDC subsystems are shown in Table 13-1.

Table 13-1. LCD Controller Connectivity Attributes

| Attributes          | Type                                                                                                          |
|---------------------|---------------------------------------------------------------------------------------------------------------|
| Power Domain        | Peripheral Domain                                                                                             |
| Clock Domain        | PD_PER_LCD_L3_GCLK (OCP Master Clock) PD_PER_LCD_L3_GCLK (OCP Slave Clock) PD_PER_LCD_GCLK (Functional Clock) |
| Reset Signals       | PER_DOM_RST_N                                                                                                 |
| Idle/Wakeup Signals | Standby<br>Smart Idle                                                                                         |
| Interrupt Requests  | 1 Interrupt to MPU Subsystem (LCDCINT)                                                                        |
| DMA Requests        | None                                                                                                          |
| Physical Address    | L4 Peripheral Slave Port                                                                                      |

### *13.2.2 LCD Controller Clock and Reset Management*

The LCDC module uses the following functional and OCP interface clocks. The L4 Slave interface runs at half the frequency of the L3 Master interface but uses the same clock. The clock is divided within the LCDC through the l4\_clkdiv input using the pd\_per\_lcd\_l4s\_gclk\_ien signal from the PRCM. The functional clock comes from the Display PLL. When the Display PLL is in bypass mode, its output is sourced by either CORE\_CLKOUTM6 or PER\_CLKOUTM2.

**Table 13-2. LCD Controller Clock Signals**

| Clock Signal                     | Max Freq | Reference / Source                     | Comments                        |
|----------------------------------|----------|----------------------------------------|---------------------------------|
| l3_clk<br>Master Interface Clock | 200 MHz  | CORE_CLKOUTM4                          | pd_per_lcd_l3_gclk<br>From PRCM |
| l4_clk<br>Slave Interface Clock  | 100 MHz  | CORE_CLKOUTM4<br>(divided within LCDC) | pd_per_lcd_l3_gclk<br>From PRCM |
| lcd_clk<br>Functional Clock      | 200 MHz  | Display PLL CLKOUT                     | pd_per_lcd_gclk<br>From PRCM    |

### *13.2.3 LCD Controller Pin List*

The LCD Controller external interface signals are shown in [Table](#page-97-0) 13-3.

**Table 13-3. LCD Controller Pin List**

| Pin               | Type | Description                                                                                                                 |
|-------------------|------|-----------------------------------------------------------------------------------------------------------------------------|
| lcd_cp            | O    | Pixel Clock in Raster model Read Strobe<br>or Read/Write Strobe in LIDD mode                                                |
| lcd_pixel_i[15:0] | I    | LCD Data Bus input (for LIDD mode only)                                                                                     |
| lcd_pixel_o[23:0] | O    | LCD Data Bus output                                                                                                         |
| lcd_lp            | O    | Line Clock or HSYNC in Raster mode;<br>Write Strobe or Direction bit in LIDD mode                                           |
| lcd_fp            | O    | Frame Clock or VSYNC in Raster mode;<br>Address Latch Enable in LIDD mode                                                   |
| lcd_ac            | O    | AC bias or Latch Enable in Raster mode;<br>Primary Chip Select/Primary Enable in<br>LIDD MPU/Hitachi mode                   |
| lcd_mclk          | O    | N/A in Raster mode; Memory<br>Clock/Secondary chip Select/Secondary<br>Enable in LIDD Synchronous/Async<br>MPU/Hitachi mode |

## 13.3 Functional Description

### 13.3.1 Clocking

This section details the various clocks and signals. Figure 13-3 shows input and output LCD controller clocks.

Figure 13-3. Input and Output Clocks

```
(page_98 Figure_6)

          Pixel Clock derived from LCD_CLK
+-------------------+  ------------------------->  +-----------+
|  LCD Controller   |  HSYNC / Line Clock ------>  |  Display  |
|                   |  VSYNC / Frame Clock ----->  |           |
+-------------------+                              +-----------+
        ^
        |
      LCD_CLK
```

#### 13.3.1.1 Pixel Clock (LCD\_PCLK)

The pixel clock (LCD\_PCLK) frequency is derived from LCD\_CLK, the reference clock to this LCD module (see Figure 13-3). The pixel clock is used by the LCD display to clock the pixel data into the line shift register.

$$\mathsf{LCD\_PCLK} = \frac{\mathsf{LCD\_CLK}}{\mathsf{CLKDIV}}$$

where CLKDIV is a field in the LCD\_CTRL register and should not be 0 or 1.

- Passive (STN) mode. LCD\_PCLK only transitions when valid data is available for output. It does not transition when the horizontal clock (HSYNC) is asserted or during wait state insertion.
- Active (TFT) mode. LCD\_PCLK continuously toggles as long as the Raster Controller is enabled.

#### **13.3.1.2 Horizontal Clock (LCD\_HSYNC)**

LCD\_HSYNC toggles after all pixels in a horizontal line have been transmitted to the LCD and a programmable number of pixel clock wait states has elapsed both at the beginning and end of each line.

The RASTER\_TIMING\_0 register fully defines the behavior of this signal.

LCD\_HSYNC can be programmed to be synchronized with the rising or falling edge of LCD\_PCLK. The configuration field is bits 24 and 25 in the RASTER\_TIMING\_2 register.

**Active (TFT) mode:** The horizontal clock or the line clock is also used by TFT displays as the horizontal synchronization signal (LCD\_HSYNC).

The timings of the horizontal clock (line clock) pins are programmable to support:

- Delay insertion both at the beginning and end of each line.
- Line clock polarity.
- Line clock pulse width, driven on rising or falling edge of pixel clock.

#### **13.3.1.3 Vertical Clock (LCD\_VSYNC)**

LCD\_VSYNC toggles after all lines in a frame have been transmitted to the LCD and a programmable number of line clock cycles has elapsed both at the beginning and end of each frame.

The RASTER\_TIMING\_1 register fully defines the behavior of this signal.

LCD\_VSYNC can be programmed to be synchronized with the rising or falling edge of LCD\_PCLK. The configuration field is bits 24 and 25 in the RASTER\_TIMING\_2 register.

- **Passive (STN) mode.** The vertical, or frame, clock toggles during the first line of the screen.
- **Active (TFT) mode.** The vertical, or frame, clock is also used by TFT displays as the vertical synchronization signal (LCD\_VSYNC).

The timings of the vertical clock pins are programmable to support:

- Delay insertion both at the beginning and end of each frame
- Frame clock polarity

#### **13.3.1.4 LCD\_AC\_BIAS\_EN**

- **Passive (STN) mode.** To prevent a dc charge within the screen pixels, the power and ground supplies of the display are periodically switched. The Raster Controller signals the LCD to switch the polarity by toggling this pin (LCD\_AC\_BIAS\_EN).
- **Active (TFT) mode.** This signal acts as an output enable (OE) signal. It is used to signal the external LCD that the data is valid on the data bus (LCD\_DATA).

### *13.3.2 LCD External I/O Signals*

[Table](#page-100-0) 13-4 shows the details of the LCD controller external signals.

#### **Table 13-4. LCD External I/O Signals**

| Signal         | Type         | Description                                                                                                                                                                                                                                                                                                                                                                   |
|----------------|--------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| LCD_VSYNC      | OUT          | Raster controller: Frame clock the LCD uses to signal the start of a new frame of pixels.<br>Also used by TFT displays as the vertical synchronization signal.                                                                                                                                                                                                                |
|                |              | LIDD character: Register select (RS) or address latch enable (ALE)                                                                                                                                                                                                                                                                                                            |
|                |              | LIDD graphics: Address bit 0 (A0) or command/data select (C/D)                                                                                                                                                                                                                                                                                                                |
| LCD_HSYNC      | OUT          | Raster controller: Line clock the LCD uses to signal the end of a line of pixels that transfers<br>line data from the shift register to the screen and to increment the line pointer(s). Also used<br>by TFT displays as the horizontal synchronization signal.                                                                                                               |
|                |              | LIDD character: not used.                                                                                                                                                                                                                                                                                                                                                     |
|                |              | LIDD graphics:                                                                                                                                                                                                                                                                                                                                                                |
|                |              | •<br>6800 mode = read or write enable                                                                                                                                                                                                                                                                                                                                         |
|                |              | •<br>8080 mode = not write strobe                                                                                                                                                                                                                                                                                                                                             |
| LCD_PCLK       | OUT          | Raster controller: Pixel clock the LCD uses to clock the pixel data into the line shift register.<br>In passive mode, the pixel clock transitions only when valid data is available on the data<br>lines. In active mode, the pixel clock transitions continuously, and the ac-bias pin is used<br>as an output enable to signal when data is available on the LCD pin.       |
|                |              | LIDD character: not used.                                                                                                                                                                                                                                                                                                                                                     |
|                |              | LIDD graphics:                                                                                                                                                                                                                                                                                                                                                                |
|                |              | •<br>6800 mode = enable strobe                                                                                                                                                                                                                                                                                                                                                |
|                |              | •<br>8080 mode = not read strobe                                                                                                                                                                                                                                                                                                                                              |
| LCD_AC_BIAS_EN | OUT          | Raster controller: ac-bias used to signal the LCD to switch the polarity of the power<br>supplies to the row and column axis of the screen to counteract DC offset. Used in TFT<br>mode as the output enable to signal when data is latched from the data pins using the pixel<br>clock.                                                                                      |
|                |              | LIDD character: Primary enable strobe                                                                                                                                                                                                                                                                                                                                         |
|                |              | LIDD graphics: Chip select 0 (CS0)                                                                                                                                                                                                                                                                                                                                            |
| LCD_MCLK       | OUT          | Raster controller: not used.                                                                                                                                                                                                                                                                                                                                                  |
|                |              | LIDD character: Secondary enable strobe                                                                                                                                                                                                                                                                                                                                       |
|                |              | LIDD graphics: Chip select 1 (CS1)                                                                                                                                                                                                                                                                                                                                            |
| LCD_D[23:0]    | Raster: OUT  | LCD data bus, providing a 4-, 8-, 16- or 24-bit data path.                                                                                                                                                                                                                                                                                                                    |
|                |              | Raster controller: For monochrome displays, each signal represents a pixel; for passive<br>color displays, groupings of three signals represent one pixel (red, green, and blue).<br>LCD_DATA[3:0] is used for monochrome displays of 2, 4, and 8 BPP; LCD_DATA[7:0] is<br>used for color STN displays and LCD_DATA[15:0] or LCD_DATA[23:0] is used for active<br>(TFT) mode. |
|                | LIDD: OUT/IN | LIDD character: LCD_DATA[15:0] Read and write the command and data registers.                                                                                                                                                                                                                                                                                                 |
|                |              |                                                                                                                                                                                                                                                                                                                                                                               |

### *13.3.3 Pin Mapping and Color Assignments*

Due to a silicon bug, pin mapping for the data signals for RGB888 and RGB565 are not as designed. Refer to the AM335x Silicon Errata [\(SPRZ360\)](http://www.ti.com/lit/pdf/SPRZ360) for proper pin mapping and color assignments when using these modes with an LCD panel.

### *13.3.4 DMA Engine*

The DMA engine provides the capability to output graphics data to constantly refresh LCDs, without burdening the CPU, via interrupts or a firmware timer. It operates on one or two frame buffers, which are set up during initialization. Using two frame buffers (ping-pong buffers) enables the simultaneous operation of outputting the current video frame to the external display and updating the next video frame. The ping-pong buffering approach is preferred in most applications.

When the Raster Controller is used, the DMA engine reads data from a frame buffer and writes it to the input FIFO (as shown in [Figure](#page-94-1) 13-1). The Raster Controller requests data from the FIFO for frame refresh; as a result, the DMA's job is to ensure that the FIFO is always kept full.

When the LIDD Controller is used, the DMA engine accesses the LIDD Controller's address and/or data registers.

To program the DMA engine, configure the following registers, as shown in [Table](#page-101-0) 13-5.

registers will not be used.) LCDDMA\_FB1\_CEILING

| Register           | Configuration                                                          |
|--------------------|------------------------------------------------------------------------|
| LCDDMA_CTRL        | Configure DMA data format                                              |
| LCDDMA_FB0_BASE    | Configure frame buffer 0                                               |
| LCDDMA_FB0_CEILING |                                                                        |
| LCDDMA_FB1_BASE    | Configure frame buffer 1. (If only one frame buffer is used, these two |

**Table 13-5. Register Configuration for DMA Engine Programming**

In addition, the LIDD\_CTRL register (for LIDD Controller) or the RASTER\_CTRL register (for Raster Controller) should also be configured appropriately, along with all the timing registers.

To enable DMA transfers, the LIDD\_DMA\_EN bit (in the LIDD\_CTRL register) or the LCDEN bit (in the RASTER\_CTRL register) should be written with 1.

**CAUTION**

Writes to RAM will fail when they are in the vicinity of where the DMA Engine is reading. Thus, for screen updates to occur smoothly, rendering must be done to a buffer that is not currently in use by the DMA engine. The between-frames period (immediately after the end-of-frame interrupts) works well to BLIT a "back buffer" (double-buffering) to the "front buffer" where DMA reads next. This works well, and enables application firmware to let the DMA engine continue reading from the same frame buffer. Alternatively, the location of where the DMA engine is reading from can be changed to an alternate (preprepared) frame buffer. When doing so at the end-of-frame interrupt, by the time the ISR knows it is an EOF0 interrupt (for example), the DMA engine has already read its BASE and CEILING addresses for the next frame (FrameBuffer1). So the DMA BASE and CEILING addresses that can be changed safely at this point would be for FrameBuffer0 (the same buffer as the frame that just completed). Then the DMA engine starts using these addresses on the next ! frame. If FrameBuffer1 BASE and CEILING are also updated, those take effect the next time the DMA engine reads them (after 2 frames).

#### **13.3.4.1 Interrupts**

Interrupts in this LCD module are related to DMA engine operation. Four registers are used to control and monitor the interrupts:

- The IRQENABLE\_SET register allows the user to enable any of the interrupt sources.
- The IRQENABLE\_CLEAR register allows the user to disable interrupts sources.
- The IRQSTATUS\_RAW register collects all the interrupt status information.

• The IRQSTATUS register collects the interrupt status information for all enabled interrupts. Any interrupt source not enabled in the IRQENABLE\_SET register is masked out.

#### *13.3.4.1.1 LIDD Mode*

When operating in LIDD mode, the DMA engine generates one interrupt signal every time the specified frame buffer has been transferred completely.

- The DONE bit in the LIDD\_CTRL register specifies if the interrupt signal is delivered to the system interrupt controller, which in turn may or may not generate an interrupt to CPU.
- The EOF1, EOF0, and DONE bits in the IRQSTATUS\_RAW register reflect the interrupt signal, regardless of being delivered to the system interrupt controller or not.

#### *13.3.4.1.2 Raster Mode*

When operating in Raster mode, the DMA engine can generate the interrupts in the following scenarios:

- 1. **Output FIFO under-run**. This occurs when the DMA engine cannot keep up with the data rate consumed by the LCD (which is determined by the LCD\_PCLK.) This is likely due to a system memory throughput issue or an incorrect LCD\_PCLK setting. The FUF bit in IRQSTATUS\_RAW is set when this error occurs. This bit is cleared by writing a 1 to the FUF bit in the IRQSTATUS register.
- 2. **Frame synchronization lost**. This error happens when the DMA engine attempts to read what it believes to be the first word of the video buffer but it cannot be recognized as such. This could be caused by an invalid frame buffer address or an invalid BPP value (for more details, see Section [13.3.6.2](#page-110-0)). The SYNC bit in the IRQSTATUS\_RAW register is set when such an error is detected. This bit is cleared by writing a 1 to the SYNC bit in the IRQSTATUS register.
- 3. **Palette loaded**. When using palette-only or palette+data modes, the PL bit in the IRQSTATUS\_RAW register will be set when the palette portion of a DMA transfer has been loaded into palette RAM. This interrupt can be cleared by writing a '1' to the PL bit in the IRQSTATUS register.
- 4. **AC bias transition**. If the ACB\_I bit in the RASTER\_TIMING\_2 register is programmed with a nonzero value, an internal counter will be loaded with this value and starts to decrement each time LCD\_AC\_BIAS\_EN (AC-bias signal) switches its state. When the counter reaches zero, the ACB bit in the IRQSTATUS\_RAW register is set, which will deliver an interrupt signal to the system interrupt controller (if the interrupt is enabled.) The counter reloads the value in field ACB\_I, but does not start to decrement until the ACB bit is cleared by writing 1 to this bit in the IRQSTATUS register.
- 5. **Frame transfer completed**. When one frame of data is transferred completely, the DONE bit in the IRQSTATUS\_RAW register is set. Note that the EOF0 and EOF1 bits in the IRQSTATUS\_RAW register will be set accordingly. This bit is cleared by writing a 1 to the corresponding interrupt in the IRQSTATUS register.

Note that the interrupt enable bits are in the IRQENABLE\_SET register. The corresponding enable bit must be set in order to generate an interrupt to the CPU. However, the IRQSTATUS\_RAW register reflects the interrupt signal regardless of the interrupt enable bits settings.

#### *13.3.4.1.3 Interrupt Handling*

See Chapter 6, *Interrupts*, for information about LCD interrupt number to CPU. The interrupt service routine needs to determine the interrupt source by examining the IRQSTATUS\_RAW register and clearing the interrupt properly.

### *13.3.5 LIDD Controller*

The LIDD Controller is designed to support LCD panels with a memory-mapped interface. The types of displays range from low-end character monochrome LCD panels to high-end TFT smart LCD panels.

LIDD mode (and the use of this logic) is enabled by clearing the MODESEL bit in the LCD control register (LCD\_CTRL).

LIDD Controller operation is summarized as follows:

• During initialization, the LCD LIDD CS0/CS1 configuration registers (LIDD\_CS0\_CONF and LIDD\_CS1\_CONF) are configured to match the requirements of the LCD panel being used.

• During normal operation, the CPU writes display data to the LCD data registers (LIDD\_CS0\_DATA and LIDD\_CS1\_DATA). The LIDD interface converts the CPU write into the proper signal transition sequence for the display, as programmed earlier. Note that the first CPU write should send the beginning address of the update to the LCD panel and the subsequent writes update data at display locations starting from the first address and continuing sequentially. Note that DMA may be used instead of CPU.

• The LIDD Controller is also capable of reading back status or data from the LCD panel, if the latter has this capability. This is set up and activated in a similar manner to the write function described above.

**NOTE:** If an LCD panel is not used, this interface can be used to control any MCU-like peripheral.

See your device-specific data manual to check the LIDD features supported by the LCD controller.

[Table](#page-103-0) 13-6 describes how the signals are used to interface external LCD modules, which are configured by the LIDD\_CTRL register.

**Table 13-6. LIDD I/O Name Map**

| Display Type                       | Interface<br>Type | Data<br>Bits | LIDD_CTRL<br>[2:0] | I/O Name       | Display I/O<br>Name | Comment                                     |
|------------------------------------|-------------------|--------------|--------------------|----------------|---------------------|---------------------------------------------|
| Character<br>Display               | HD44780<br>Type   | 4            | 100                | LCD_DATA[7:4]  | DATA[7:4]           | Data Bus (length defined by<br>Instruction) |
|                                    |                   |              |                    | LCD_AC_BIAS_EN | E (or E0)           | Enable Strobe (first display)               |
|                                    |                   |              |                    | LCD_HSYNC      | R/W                 | Read/Write                                  |
|                                    |                   |              |                    | LCD_VSYNC      | RS                  | Register Select (Data/not<br>Instruction)   |
|                                    |                   |              |                    | LCD_MCLK       | E1                  | Enable Strobe (second display<br>optional)  |
| Character<br>Display               | HD44780<br>Type   | 8            | 100                | LCD_DATA[7:0]  | DATA[7:0]           | Data Bus (length defined by<br>Instruction) |
|                                    |                   |              |                    | LCD_AC_BIAS_EN | E (or E0)           | Enable Strobe (first display)               |
|                                    |                   |              |                    | LCD_HSYNC      | R/W                 | Read/Write                                  |
|                                    |                   |              |                    | LCD_VSYNC      | RS                  | Register Select (Data/not<br>Instruction)   |
|                                    |                   |              |                    | LCD_MCLK       | E1                  | Enable Strobe (second display<br>optional)  |
| Micro Interface<br>Graphic Display | 6800<br>Family    | Up to<br>16  | 001                | LCD_DATA[15:0] | DATA[7:0]           | Data Bus (16 bits always<br>available)      |
|                                    |                   |              |                    | LCD_PCLK       | E                   | Enable Clock                                |
|                                    |                   |              |                    | LCD_HSYNC      | R/W                 | Read/Write                                  |
|                                    |                   |              |                    | LCD_VSYNC      | A0                  | Address/Data Select                         |
|                                    |                   |              |                    | LCD_AC_BIAS_EN | CS (or CS0)         | Chip Select (first display)                 |
|                                    |                   |              |                    | LCD_MCLK       | CS1                 | Chip Select (second display<br>optional)    |
|                                    |                   |              | 000                | LCD_MCLK       | None                | Synchronous Clock (optional)                |
| Micro Interface<br>Graphic Display | 8080<br>Family    | Up to<br>16  | 011                | LCD_DATA[15:0] | DATA[7:0]           | Data Bus (16 bits always<br>available)      |
|                                    |                   |              |                    | LCD_PCLK       | RD                  | Read Strobe                                 |
|                                    |                   |              |                    | LCD_HSYNC      | WR                  | Write Strobe                                |
|                                    |                   |              |                    | LCD_VSYNC      | A0                  | Address/Data Select                         |
|                                    |                   |              |                    | LCD_AC_BIAS_EN | CS (or CS0)         | Chip Select (first display)                 |
|                                    |                   |              |                    | LCD_MCLK       | CS1                 | Chip Select (second display<br>optional)    |
|                                    |                   |              | 010                | LCD_MCLK       | None                | Synchronous Clock (optional)                |

The timing parameters are defined by the LIDD\_CS0\_CONF and LIDD\_CS1\_CONF registers, which are described in and .

The timing configuration is based on an internal reference clock, MCLK. The MCLK is generated out of LCD\_CLK, which is determined by the CLKDIV bit in the LCD\_CTRL register.

$$\begin{split} & \text{MCLK} = \text{LCD\_CLK} \text{ when CLKDIV} = 0. \\ & \text{MCLK} = \frac{\text{LCD\_CLK}}{\text{CLKDIV}} \text{ when CLKDIV} \neq 0. \end{split}$$

See your device-specific data manual for the timing configurations supported by the LCD controller.

#### **13.3.5.1 LIDD Controller Timing**

**Figure 13-4. LIDD Mode HD44780 Write Timing Diagram**

```
(page_104 Figure_8) – LIDD write/read timing (ASCII)

time --> |---- W_SU ----|--- W_STROBE ---|-- W_HOLD --|CS_DLY|  ...  |---- R_SU ----|--- R_STROBE ---|-- R_HOLD --|CS_DLY|

MCLK(in)   _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_  ...  _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_

LCD_DATA
 [7:0]     =======  Write data  =====================      ZZZ      =======  Rd data  =======      ZZZ     ===== Data[7:0] ===

LCD_PCLK   ----------------------------------------------------------------------  (not used)

LCD_VSYNC  ------------------------------\____________________/--------------------  (RS)

LCD_HSYNC  ---------\____________________/-----------------------------------------  (R/W)

LCD_AC_BIAS_EN
LCD_MCLK   _____________/‾‾‾‾‾‾‾‾‾‾\__________________________/‾‾‾‾‾‾‾‾\___________  (E0/E1)

Right-side labels (as shown):
- LCD_VSYNC : RS
- LCD_HSYNC : R/W
- LCD_AC_BIAS_EN / LCD_MCLK : E0 / E1
```

**Figure 13-5. LIDD Mode HD44780 Read Timing Diagram**

```
(page_104 Figure_10) – Read then Write-instruction timing (ASCII)

time --> |---- R_SU ----|--- R_STROBE ---|-- R_HOLD --|CS_DLY| ... |---- W_SU ----|--- W_STROBE ---|-- W_HOLD --|CS_DLY|

MCLK(in)   _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_  ...  _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_

LCD_DATA
 [7:0]     =======  Rd data  =======      ZZZ      =======  Write instruction  ===================  ZZZ  === Data[7:0] ===

LCD_PCLK   ----------------------------------------------------------------------  (not used)

LCD_VSYNC  ------------------------------\______________________________/---------  (RS)

LCD_HSYNC  ------------------------------\______________________________/---------  (R/W)

LCD_AC_BIAS_EN
LCD_MCLK   _____________/‾‾‾‾‾‾‾‾\______________________/‾‾‾‾‾‾‾‾\______________  (E0/E1)

Right-side labels (as shown):
- LCD_VSYNC : RS
- LCD_HSYNC : R/W
- LCD_AC_BIAS_EN / LCD_MCLK : E0 / E1
```

#### **Figure 13-6. LIDD Mode 6800 Write Timing Diagram**

```
(page_105 Figure_3) – 16-bit: write address then write data (ASCII)

time --> |---- W_SU ----|--- W_STROBE ---|-- W_HOLD --|CS_DLY| ... |---- W_SU ----|--- W_STROBE ---|-- W_HOLD --|CS_DLY|

MCLK(in)      _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_  ...  _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_      (clock)

LCD_DATA
 [15:0]       =======  Write address  =====================  ZZZ  =======  Write data  =====================  ZZZ  == Data[15:0]==

LCD_AC_BIAS_EN
LCD_MCLK      ____________/‾‾‾‾‾‾‾‾‾‾\______________ ... __________/‾‾‾‾‾‾‾‾‾‾\______________              (CS0/CS1)

LCD_VSYNC     --------------\______________________/----------- ... --------------\______________________/-----              (A0)

LCD_HSYNC     --------------\______________________/----------- ... --------------\______________________/-----              (R/W)

LCD_PCLK       _____________/‾‾‾‾‾‾‾‾‾‾\________________ ... ____________/‾‾‾‾‾‾‾‾‾‾\________________          (E)

Right-side labels (as shown):
- LCD_AC_BIAS_EN / LCD_MCLK : CS0 / CS1
- LCD_VSYNC                : A0
- LCD_HSYNC                : R/W
- LCD_PCLK                 : E
```

#### **Figure 13-7. LIDD Mode 6800 Read Timing Diagram**

```
(page_106 Figure_3) – 16-bit: write address then read data (ASCII)

time --> |---- W_SU ----|--- W_STROBE ---|-- W_HOLD --|CS_DLY| ... |---- R_SU ----|--- R_STROBE ---|-- R_HOLD --|CS_DLY|

MCLK(in)      _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_  ...  _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_      (clock)

LCD_DATA
 [15:0]       =======  Write address  =====================  ZZZ  =======  Rd data  =======  ZZZ  == Data[15:0] ==

LCD_AC_BIAS_EN
LCD_MCLK      ____________/‾‾‾‾‾‾‾‾‾‾\______________ ... ____________/‾‾‾‾‾‾‾‾‾‾\______________              (CS0/CS1)

LCD_VSYNC     --------------\______________________/----------- ... -------------------------------------------              (A0)

LCD_HSYNC     --------------\______________________/----------- ... -------------------------------------------              (R/W)

LCD_PCLK       _____________/‾‾‾‾‾‾‾‾‾‾\________________ ... _____________\______/________________________________           (E)

Right-side labels (as shown):
- LCD_AC_BIAS_EN / LCD_MCLK : CS0 / CS1
- LCD_VSYNC                : A0
- LCD_HSYNC                : R/W
- LCD_PCLK                 : E
```

**Figure 13-8. LIDD Mode 6800 Status Timing Diagram**

```
(page_106 Figure_5) – 16-bit: read Data then read Status (ASCII)

time --> |---- R_SU ----|--- R_STROBE ---|-- R_HOLD --|CS_DLY| ... |---- R_SU ----|--- R_STROBE ---|-- R_HOLD --|CS_DLY|

MCLK(in)      _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_  ...  _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_      (clock)

LCD_DATA
 [15:0]       =======  rd Data  =======  ZZZ  == Data[15:0] == ... =======  rd Status  =======  ZZZ  == Data[15:0] ==

LCD_AC_BIAS_EN
LCD_MCLK      ____________/‾‾‾‾‾‾‾‾‾‾\______________ ... ____________/‾‾‾‾‾‾‾‾‾‾\______________              (CS0/CS1)

LCD_VSYNC     ------------------------------(stable)---------------- ... ---------\______________________/------              (A0)

LCD_HSYNC     ------------------------------(stable)---------------- ... --------------------------------------              (R/W)

LCD_PCLK       _____________/‾‾‾‾‾‾‾‾‾‾\________________ ... ____________/‾‾‾‾‾‾‾‾‾‾\________________          (E)

Right-side labels (as shown):
- LCD_AC_BIAS_EN / LCD_MCLK : CS0 / CS1
- LCD_VSYNC                : A0
- LCD_HSYNC                : R/W
- LCD_PCLK                 : E
```

**Figure 13-9. LIDD Mode 8080 Write Timing Diagram**

```
(page_106 Figure_7) – 16-bit write: address phase then data phase (ASCII)

time --> |---- W_SU ----|--- W_STROBE ---|-- W_HOLD --|CS_DLY| ... |---- W_SU ----|--- W_STROBE ---|-- W_HOLD --|CS_DLY|

MCLK(in)      _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_  ...  _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_      (clock)

LCD_DATA
 [15:0]       =======  Write address  =====================  ZZZ  =======  Write data  =====================  ZZZ  == Data[15:0]==

LCD_AC_BIAS_EN
LCD_MCLK      ____________/‾‾‾‾‾‾‾‾‾‾\______________ ... __________/‾‾‾‾‾‾‾‾‾‾\______________              (CS0/CS1)

LCD_VSYNC     --------------\______________________/----------- ... --------------\______________________/-----              (A0)

LCD_HSYNC     --------------\______________________/----------- ... --------------\______________________/-----              (R/W)

LCD_PCLK       _____________/‾‾‾‾‾‾‾‾‾‾\________________ ... ____________/‾‾‾‾‾‾‾‾‾‾\________________          (RD)

Right-side labels (as shown):
- LCD_AC_BIAS_EN / LCD_MCLK : CS0 / CS1
- LCD_VSYNC                : A0
- LCD_HSYNC                : R/W
- LCD_PCLK                 : RD
```

### **Figure 13-10. LIDD Mode 8080 Read Timing Diagram**

```
(page_107 Figure_3) – 16-bit: write address then read data (ASCII)

time --> |---- W_SU ----|--- W_STROBE ---|-- W_HOLD --|CS_DLY| ... |---- R_SU ----|--- R_STROBE ---|-- R_HOLD --|CS_DLY|

MCLK(in)      _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_  ...  _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_      (clock)

LCD_DATA
 [15:0]       =======  Write address  =====================  ZZZ  =======  Rd data  =======  ZZZ  == Data[15:0] ==

LCD_AC_BIAS_EN
LCD_MCLK      ____________/‾‾‾‾‾‾‾‾‾‾\______________ ... ____________/‾‾‾‾‾‾‾‾‾‾\______________              (CS0/CS1)

LCD_VSYNC     --------------\______________________/----------- ... -------------------------------------------              (A0)

LCD_HSYNC     --------------------\__________/----------------- ... --------------\______________________/-----              (R/W)
             (goes low during addr phase)                           (low during read phase)

LCD_PCLK       ________________________________ ... _____________\______/_____________________________________           (RD)

Right-side labels (as shown):
- LCD_AC_BIAS_EN / LCD_MCLK : CS0 / CS1
- LCD_VSYNC                : A0
- LCD_HSYNC                : R/W
- LCD_PCLK                 : RD
```

**Figure 13-11. LIDD Mode 8080 Status Timing Diagram**

```
(page_107 Figure_5) – 16-bit: read Data then read Status (ASCII)

time --> |---- R_SU ----|--- R_STROBE ---|-- R_HOLD --|CS_DLY| ... |---- R_SU ----|--- R_STROBE ---|-- R_HOLD --|CS_DLY|

MCLK(in)      _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_  ...  _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_      (clock)

LCD_DATA
 [15:0]       =======  rd Data  =======  ZZZ  == Data[15:0] == ... =======  rd Status  =======  ZZZ  == Data[15:0] ==

LCD_AC_BIAS_EN
LCD_MCLK      ____________/‾‾‾‾‾‾‾‾‾‾\______________ ... ____________/‾‾‾‾‾‾‾‾‾‾\______________              (CS0/CS1)

LCD_VSYNC     ------------------------------(stable)---------------- ... ---------\______________________/------              (A0)

LCD_HSYNC     ------------------------------(stable)---------------- ... --------------------------------------              (R/W)

LCD_PCLK      _________\______/________________________ ... _________\______/___________________________        (RD)
              (RD pulse during read strobe)                    (RD pulse during read strobe)

Right-side labels (as shown):
- LCD_AC_BIAS_EN / LCD_MCLK : CS0 / CS1
- LCD_VSYNC                : A0
- LCD_HSYNC                : R/W
- LCD_PCLK                 : RD
```

**NOTE:** The CS\_DELAY in above figures is same as bit field TA in LCDLIDDCS0CONFIG.

### *13.3.6 Raster Controller*

Raster mode (and the use of this logic) is enabled by setting the MODESEL bit in the LCD control register (LCD\_CTRL). [Table](#page-108-1) 13-7 shows the active external signals when this mode is active.

**Table 13-7. Operation Modes Supported by Raster Controller**

| Interface           | Data Bus<br>Width | Register Bits<br>RASTER_CTRL[9, 7, 1] | Signal Name    | Description                  |
|---------------------|-------------------|---------------------------------------|----------------|------------------------------|
| Passive (STN) Mono  | 4                 | 001                                   | LCD_DATA[3:0]  | Data bus                     |
| 4-bit               |                   |                                       | LCD_PCLK       | Pixel clock                  |
|                     |                   |                                       | LCD_HSYNC      | Horizontal clock(Line Clock) |
|                     |                   |                                       | LCD_VSYNC      | Vertical clock (Frame Clock) |
|                     |                   |                                       | LCD_AC_BIAS_EN | AC Bias                      |
|                     |                   |                                       | LCD_MCLK       | Not used                     |
| Passive (STN) Mono  | 8                 | 101                                   | LCD_DATA[7:0]  | Data bus                     |
| 8-bit               |                   |                                       | LCD_PCLK       | Pixel clock                  |
|                     |                   |                                       | LCD_HSYNC      | Horizontal clock(Line Clock) |
|                     |                   |                                       | LCD_VSYNC      | Vertical clock (Frame Clock) |
|                     |                   |                                       | LCD_AC_BIAS_EN | AC Bias                      |
|                     |                   |                                       | LCD_MCLK       | Not used                     |
| Passive (STN) Color | 8                 | 100                                   | LCD_DATA[7:0]  | Data bus                     |
|                     |                   |                                       | LCD_PCLK       | Pixel clock                  |
|                     |                   |                                       | LCD_HSYNC      | Horizontal clock(Line Clock) |
|                     |                   |                                       | LCD_VSYNC      | Vertical clock (Frame Clock) |
|                     |                   |                                       | LCD_AC_BIAS_EN | AC Bias                      |
|                     |                   |                                       | LCD_MCLK       | Not used                     |
| Active (TFT) Color  | 16                | x10                                   | LCD_DATA[15:0] | Data bus                     |
|                     |                   |                                       | LCD_PCLK       | Pixel clock                  |
|                     |                   |                                       | LCD_HSYNC      | Horizontal clock(Line Clock) |
|                     |                   |                                       | LCD_VSYNC      | Vertical clock (Frame Clock) |
|                     |                   |                                       | LCD_AC_BIAS_EN | Output enable                |
|                     |                   |                                       | LCD_MCLK       | Not used                     |

#### **13.3.6.1 Logical Data Path**

The block diagram of the Raster Controller is shown in [Figure](#page-94-1) 13-1. [Figure](#page-109-0) 13-12 illustrates its logical data path for various operation modes (passive (STN) versus active (TFT), various BPP size).

#### [Figure](#page-109-0) 13-12 shows that:

- The gray-scaler/serializer and output FIFO blocks are bypassed in active (TFT) modes.
- • The palette is bypassed in both 12- and 16-BPP modes.

```
Data path (STN vs TFT)

             +---------------------------+
             | Data source (frame buffers)|
             +-------------+-------------+
                           |
                           v
                    +-------------+
                    |  Input FIFO |
                    +------+------+
                           |
              +------------+------------+
              |                         |
              v                         v
        +-----------+             +-----------+
        | STN       |             | TFT       |
        | (passive) |             | (active)  |
        +-----+-----+             +-----+-----+
              |                         |
      +-------+--------+         +------+--------+
      |                |         |               |
      v                v         v               v
  12,16 BPP       1,2,4,8 BPP  1,2,4,8 BPP    16,24 BPP
     |                |            |             |
     |                v            v             |
     |          +-----------+  +-----------+     |
     |          | Palette   |  | Palette   |     |
     |          +-----+-----+  +-----+-----+     |
     |                |            |             |
     +----------------+            +-------------+
              |                         |
              v                         v
     +---------------------+   +---------------------+
     | Gray-scaler/        |   | Gray-scaler/        |
     | serializer          |   | serializer          |
     +----------+----------+   +----------+----------+
                |                         |
                v                         v
           +-----------+             +-----------+
           | Output    |             | Output    |
           | FIFO      |             | FIFO      |
           +-----+-----+             +-----+-----+
                 \                       /
                  \                     /
                   v                   v
                 +-------------------------+
                 |       Output pins       |
                 +-------------------------+

```

**Figure 13-12. Logical Data Path for Raster Controller**

In summary:

- The display image is stored in frame buffers.
- The built-in DMA engine constantly transfers the data stored in the frame buffers to the Input FIFO.
- The Raster Controller relays data to the external pins according to the specified format.

The remainder of this section describes the functioning blocks in [Figure](#page-109-0) 13-12, including frame buffers, palette, and gray-scaler/serializer. Their operation and programming techniques are covered in detail. The output format is also described in Section [13.3.6.5](#page-117-0).

#### **13.3.6.2 Frame Buffer**

A frame buffer is a contiguous memory block, storing enough data to fill a full LCD screen. For this device, external memory needs to be used for the frame buffer. For specific details on which external memory interface (EMIF) controller can be accessed by the LCD controller, see your device-specific data manual. The data in the frame buffer consists of pixel values as well as a look-up palette. [Figure](#page-110-1) 13-13 shows the frame buffer structure.

### **Figure 13-13. Frame Buffer Structure**

#### **1, 2, 4, 12, 16, 24 BPP Modes**

| Palette  | Pixel Data |
|----------|------------|
| 32 bytes | x bytes    |

#### **8 BPP Mode**

| Palette   | Pixel Data |
|-----------|------------|
| 512 bytes | x bytes    |

**NOTE:**

- 8-BPP mode uses the first 512 bytes in the frame buffer as the palette while the other modes use 32 bytes.
- 12-, 16-, and 24-BPP modes do not need a palette; i.e., the pixel data is the desired RGB value. However, the first 32 bytes are still considered a palette. The first entry should be 4000h (bit 14 is 1) while the remaining entries must be filled with 0. (For details, see [Table](#page-111-0) 13-8.)
- Each entry in a palette occupies 2 bytes. As a result, 8-BPP mode palette has 256 color entries while the other palettes have up to 16 color entries.
- 4-BPP mode uses up the all the 16 entries in a palette.
- 1-BPP mode uses the first 2 entries in a palette while 2-BPP mode uses the first 4 entries. The remaining entries are not used and must be filled with 0.
- In 12- and 16-BPP modes, pixel data is RGB data. For all the other modes, pixel data is actually an index of the palette entry.

**Table 13-8. Bits-Per-Pixel Encoding for Palette Entry 0 Buffer**

| Bit   | Name | Value | Description(1) (2) (3)                                              |
|-------|------|-------|---------------------------------------------------------------------|
| 14-12 | BPP  |       | Bits-per-pixel.                                                     |
|       |      | 000   | 1 BPP                                                               |
|       |      | 001   | 2 BPP                                                               |
|       |      | 010   | 4 BPP                                                               |
|       |      | 011   | 8 BPP                                                               |
|       |      | 1xx   | 12 BPP in passive mode (TFT_STN = 0 and STN_565 = 0 in RASTER_CTRL) |
|       |      |       | 16 BPP in passive mode (TFT_STN = 0 and STN_565 = 1 in RASTER_CTRL) |
|       |      |       | 16 BPP in active mode (LCDTFT = 1 and TFT24 = 0 in RASTER_CTRL)     |
|       |      |       | 24 BPP in active mode (LCDTFT = 1 and TFT24 = 1 in RASTER_CTRL)     |

<sup>(1)</sup> Eight 1-bit pixels, four 2-bit pixels, and two 4-bit pixels are packed into each byte, and 12-bit pixels are right justified on (16-bit) word boundaries (in the same format as palette entry).

The equations shown in [Table](#page-111-1) 13-9 are used to calculate the total frame buffer size (in bytes) based on varying pixel size encoding and screen sizes.

[Figure](#page-112-0) 13-14 and [Figure](#page-113-0) 13-15 show more detail of the palette entry organization.

**Table 13-9. Frame Buffer Size According to BPP**

| BPP   | Frame Buffer Size          |
|-------|----------------------------|
| 1     | 32 + (Lines × Columns)/8   |
| 2     | 32 + (Lines × Columns)/4   |
| 4     | 32 + (Lines × Columns)/2   |
| 8     | 512 + (Lines × Columns)    |
| 12/16 | 32 + 2 × (Lines × Columns) |

<sup>(2)</sup> For STN565, see the 16 BPP STN mode bit ().

<sup>(3)</sup> For Raw Data (12/16/24 bpp) framebuffers, no Palette lookup is employed therefore PALMODE = 0x10 in RASTER\_CTRL.

**Figure 13-14. 16-Entry Palette/Buffer Format (1, 2, 4, 12, 16 BPP)**

| Individual Palette Entry |        |    |            |    |                    |         |   |   |   |   |           |   |   |          |   |   |
|--------------------------|--------|----|------------|----|--------------------|---------|---|---|---|---|-----------|---|---|----------|---|---|
| Bit                      | 15     | 14 | 13         | 12 | 11                 | 10      | 9 | 8 | 7 | 6 | 5         | 4 | 3 | 2        | 1 | 0 |
| Color                    | Unused |    | (A)<br>BPP |    |                    | Red (R) |   |   |   |   | Green (G) |   |   | Blue (B) |   |   |
| Bit                      | 15     | 14 | 13         | 12 | 11                 | 10      | 9 | 8 | 7 | 6 | 5         | 4 | 3 | 2        | 1 | 0 |
| Mono                     | Unused |    | BPP(A)     |    | Unused<br>Mono (M) |         |   |   |   |   |           |   |   |          |   |   |

| Bit        | 15              | 16-Entry Palette Buffer | 0 |
|------------|-----------------|-------------------------|---|
| Base + 0   |                 | Palette Entry 0         |   |
| Base + 2   | Palette Entry 1 |                         |   |
|            |                 |                         |   |
|            |                 |                         |   |
| Base + 1Ch |                 | Palette Entry 14        |   |
| Base + 1Eh |                 | Palette Entry 15        |   |

A. Bits-per-pixels (BPP) is only contained within the first palette entry (palette entry 0).

### **Figure 13-15. 256-Entry Palette/Buffer Format (8 BPP)**

```
(page_113 Figure_3) – Individual Palette Entry (16-bit)

Bit:   15    14 13 12     11 10  9  8      7  6  5  4      3  2  1  0
      +-----+-----------+-------------+-------------+-------------+
Color |Unused|  BPP(A)   |   Red (R)   | Green (G)   |  Blue (B)   |
      +-----+-----------+-------------+-------------+-------------+
       [15]   [14:12]      [11:8]         [7:4]         [3:0]

      +-----+-----------+---------------------------+-------------+
Mono  |Unused|  BPP(A)   |          Unused           |  Mono (M)   |
      +-----+-----------+---------------------------+-------------+
       [15]   [14:12]              [11:4]              [3:0]
```

```
(page_113 Figure_4) – 256-Entry Palette Buffer (each entry is 16-bit: bit 15..0)

Base + 00h   +--------------------------------------------------------+
            |                  Palette Entry 0                        |
            +--------------------------------------------------------+

Base + 02h   +--------------------------------------------------------+
            |                  Palette Entry 1                        |
            +--------------------------------------------------------+

            :                         ...                             :
            :                         ...                             :

Base + FCh   +--------------------------------------------------------+
            |                  Palette Entry 254                      |
            +--------------------------------------------------------+

Base + FEh   +--------------------------------------------------------+
            |                  Palette Entry 255                      |
            +--------------------------------------------------------+
```

A. Bits-per-pixels (BPP) is only contained within the first palette entry (palette entry 0).

Bits 12, 13, and 14 of the first palette entry select the number of bits-per-pixel to be used in the following frame and thus the number of palette RAM entries. The palette entry is used by the Raster Controller to correctly unpack pixel data.

[Figure](#page-113-1) 13-16 through [Figure](#page-115-0) 13-21 show the memory organization within the frame buffer for each pixel encoding size.

**Figure 13-16. 16-BPP Data Memory Organization (TFT Mode Only)—Little Endian**

| Bit           | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---------------|----|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
| 16 bits/pixel |    |    | R  |    |    |    |   | G |   |   |   |   |   | B |   |   |

```
(page_113 Figure_10) – Pixel buffer (16-bit per pixel)

Base         +--------------------------------------------------------+
             |                       Pixel 0                          |
             +--------------------------------------------------------+

Base + 02h    +--------------------------------------------------------+
             |                       Pixel 1                          |
             +--------------------------------------------------------+
```

#### **Figure 13-17. 12-BPP Data Memory Organization—Little Endian**

| Bit           | 15 | 14     | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---------------|----|--------|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
| 12 bits/pixel |    | Unused |    |    |    | R  |   |   |   | G |   |   |   | B |   |   |

```
(page_114 Figure_4) – Pixel buffer (16-bit per pixel)

Base         +--------------------------------------------------------+
             |                       Pixel 0                          |
             +--------------------------------------------------------+

Base + 02h    +--------------------------------------------------------+
             |                       Pixel 1                          |
             +--------------------------------------------------------+
```

Unused [15-12] bits are filled with zeroes in TFT mode.

**Figure 13-18. 8-BPP Data Memory Organization**

```
(page_114 Figure_7) – 8 bits/pixel

Bit:     7  6  5  4  3  2  1  0
        +-----------------------+
8 bpp   |       Data[7:0]       |
        +-----------------------+
```

```
(page_114 Figure_8) – 8 bpp pixel buffer (1 byte per pixel)

Base        +--------------------------------------------------------+
            |                       Pixel 0                          |
            +--------------------------------------------------------+

Base + 01h   +--------------------------------------------------------+
            |                       Pixel 1                          |
            +--------------------------------------------------------+

Base + 02h   +--------------------------------------------------------+
            |                       Pixel 2                          |
            +--------------------------------------------------------+
```

**Figure 13-19. 4-BPP Data Memory Organization**

| Bit          | 3 | 2         | 1 | 0 |
|--------------|---|-----------|---|---|
| 4 bits/pixel |   | Data[3:0] |   |   |

```
(page_114 Figure_11) – 4 bpp pixel buffer (2 pixels per byte)

              +-------------+-------------+
Base          |   Pixel 0    |   Pixel 1  |
              +-------------+-------------+
Base + 01h    |   Pixel 2    |   Pixel 3  |
              +-------------+-------------+
Base + 02h    |   Pixel 4    |   Pixel 5  |
              +-------------+-------------+
```

#### **Figure 13-20. 2-BPP Data Memory Organization**

| Bit      | 7 | 6       | 5       | 4       | 3       | 2       | 1 | 0       |  |          |  |          |
|----------|---|---------|---------|---------|---------|---------|---|---------|--|----------|--|----------|
| Base     |   | Pixel 0 | Pixel 1 |         | Pixel 2 |         |   | Pixel 3 |  |          |  |          |
| Base + 1 |   | Pixel 4 |         | Pixel 5 |         | Pixel 6 |   | Pixel 7 |  |          |  |          |
| Base + 2 |   | Pixel 8 | Pixel 9 |         |         |         |   |         |  | Pixel 10 |  | Pixel 11 |

### **Figure 13-21. 1-BPP Data Memory Organization**

| Bit      | 7  | 6  | 5   | 4   | 3   | 2   | 1   | 0   |
|----------|----|----|-----|-----|-----|-----|-----|-----|
| Base     | P0 | P1 | P2  | P3  | P4  | P5  | P6  | P7  |
| Base + 1 | P8 | P9 | P10 | P11 | P12 | P13 | P14 | P15 |

#### **13.3.6.3 Palette**

As explained in the previous section, the pixel data is an index of palette entry (when palette is used). The number of colors supported is given by 2 number of BPP . However, due to a limitation of the grayscaler/serializer block, fewer grayscales or colors may be supported.

The PLM field (in RASTER\_CTRL) affects the palette loading:

- If PLM is 00b (palette-plus-data mode) or 01b (palette-only mode), the palette is loaded by the DMA engine at the very beginning, which is followed by the loading of pixel data.
- If PLM is 10b (data-only mode), the palette is not loaded. Instead, the DMA engine loads the pixel data immediately.

#### **13.3.6.4 Gray-Scaler/Serializer**

#### *13.3.6.4.1 Passive (STN) Mode*

Once a palette entry is selected from the look-up palette by the pixel data, its content is sent to the grayscaler/serializer. If it is monochrome data, it is encoded as 4 bits. If it is color data, it is encoded as 4 bits (Red), 4 bits (Green), and 4 bits (Blue).

These 4-bit values are used to select one of the 16 intensity levels, as shown in Table [13-10.](#page-116-0) A patented algorithm is used during this processing to provide an optimized intensity value that matches the eye's visual perception of color/gray gradations.

#### *13.3.6.4.2 Active (TFT) Mode*

The gray-scaler/serializer is bypassed.

**Table 13-10. Color/Grayscale Intensities and Modulation Rates**

| Dither Value<br>(4-Bit Value from Palette) | Intensity<br>(0% is White) | Modulation Rate<br>(Ratio of ON to ON+OFF Pixels) |
|--------------------------------------------|----------------------------|---------------------------------------------------|
| 0000                                       | 0.0%                       | 0                                                 |
| 0001                                       | 14.3%                      | 1/7                                               |
| 0010                                       | 20.0%                      | 1/5                                               |
| 0011                                       | 25%                        | 1/4                                               |
| 0100                                       | 33.3%                      | 3/9                                               |
| 0101                                       | 40.0%                      | 2/5                                               |
| 0110                                       | 44.4%                      | 4/9                                               |
| 0111                                       | 50.0%                      | 1/2                                               |
| 1000                                       | 55.6%                      | 5/9                                               |
| 1001                                       | 60.0%                      | 3/5                                               |
| 1010                                       | 66.6%                      | 6/9                                               |
| 1011                                       | 75%                        | 3/4                                               |
| 1100                                       | 80.0%                      | 4/5                                               |
| 1101                                       | 85.7%                      | 6/7                                               |
| 1110                                       | 93.3%                      | 14/15                                             |
| 1111                                       | 100.0%                     | 1                                                 |

#### *13.3.6.4.3 Summary of Color Depth*

**Table 13-11. Number of Colors/Shades of Gray Available on Screen**

| Number of |                                                                                                                    | Passive Mode (LCDTFT = 0)                                   | Active Mode (LCDTFT = 1)                                     |
|-----------|--------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------|--------------------------------------------------------------|
| BPP       | Monochrome (LCDBW = 1)                                                                                             | Color (LCDBW = 0)                                           | Color Only (LCDBW = 0)                                       |
| 1         | 2 palette entries to select within<br>15 grayscales                                                                | 2 palette entries to select within<br>3375 possible colors  | 2 palette entries to select within<br>4096 possible colors   |
| 2         | 4 palette entries to select within<br>15 grayscales                                                                | 4 palette entries to select within<br>3375 possible colors  | 4 palette entries to select within<br>4096 possible colors   |
| 4         | 16 palette entries to select within<br>15 grayscales                                                               | 16 palette entries to select within<br>3375 possible colors | 16 palette entries to select within<br>4096 possible colors  |
| 8         | Not relevant since it would consist in<br>256 palette entries to select within<br>15 grayscales, but exists anyway | 256 palette entries to select<br>3375 possible colors       | 256 palette entries to select within<br>4096 possible colors |
| 12        | x                                                                                                                  | 3375 possible colors                                        | 4096 possible colors                                         |
| 16        | x                                                                                                                  | 3375 possible colors (STN_565 = 1)                          | Up to 65536 possible colors                                  |
| 24        | X                                                                                                                  | X                                                           | Up to 16.7 million colors                                    |

#### **13.3.6.5 Output Format**

#### *13.3.6.5.1 Passive (STN) Mode*

As shown in [Figure](#page-109-0) 13-12, the pixel data stored in frame buffers go through palette (if applicable) and gray-scaler/serializer before reaching the Output FIFO. As a result, it is likely that the data fed to the Output FIFO is numerically different from the data in the frame buffers. (However, they represent the same color or grayscale.)

The output FIFO formats the received data according to display modes (see [Table](#page-108-1) 13-7). [Figure](#page-117-1) 13-22 shows the actual data output on the external pins.

#### *13.3.6.5.2 Active (TFT) Mode*

As shown in [Figure](#page-109-0) 13-12, the gray-scaler/serializer and output FIFO are bypassed in active (TFT) mode. Namely, at each pixel clock, one pixel data (16 bits) is output to the external LCD.

```
(page_117 Figure_8) – MONO8B mapping to LCD output pins

MONO8B = 0                                Pixel data bus: [3:0]
--------------------------------         ------------------------
LCD controller output pins
  Pixel data pin 0  <----  Pix1
  Pixel data pin 1  <----  Pix2
  Pixel data pin 2  <----  Pix3
  Pixel data pin 3  <----  Pix4

Pixel clock:  __|‾‾|__|‾‾|__

MONO8B = 1                                Pixel data bus: [7:0]
--------------------------------         ------------------------
LCD controller output pins
  Pixel data pin 0  <----  Pix1
  Pixel data pin 1  <----  Pix2
  Pixel data pin 2  <----  Pix3
  Pixel data pin 3  <----  Pix4
  Pixel data pin 4  <----  Pix5
  Pixel data pin 5  <----  Pix6
  Pixel data pin 6  <----  Pix7

Pixel clock:  __|‾‾|__|‾‾|__
```

**Figure 13-22. Monochrome and Color Output**

```
(page_117 Figure_11) – Color: mapping Pixel data pins [7:0] over time

Each column = one pixel-clock time slot (Pixel data[7:0] driven in parallel)

                 |  Slot 0   |  Slot 1   |  Slot 2   |
------------------------------------------------------
Pixel data pin 7  | (Pix1)R   | (Pix3)B   | (Pix6)G   |
Pixel data pin 6  | (Pix1)G   | (Pix4)R   | (Pix6)B   |
Pixel data pin 5  | (Pix1)B   | (Pix4)G   | (Pix7)R   |
Pixel data pin 4  | (Pix2)R   | (Pix4)B   | (Pix7)G   |
Pixel data pin 3  | (Pix2)G   | (Pix5)R   | (Pix7)B   |
Pixel data pin 2  | (Pix2)B   | (Pix5)G   | (Pix8)R   |
Pixel data pin 1  | (Pix3)R   | (Pix5)B   | (Pix8)G   |
Pixel data pin 0  | (Pix3)G   | (Pix6)R   | (Pix8)B   |
------------------------------------------------------
Bus: Pixel data[7:0]

Pixel clock:  __|‾‾|__|‾‾|__|‾‾|__
```

#### **13.3.6.6 Subpicture Feature**

A feature exists in the LCD to cover either the top or lower portion of the display with a default color. This feature is called a subpicture and is illustrated in [Figure](#page-118-0) 13-23. Subpictures are only allowed for Active Matrix mode (cfg\_lcdtft = '1').

Subpictures reduce the bandwith to the DDR since lines containing default pixel data are not read from memory. For example, suppose the panel has 100 lines of which 50 are default pixel data lines. Then, only 50 lines of data are DMAed from DDR for this subpicture setup. That is, the cfg\_fbx\_base and cfg\_fbx\_ceiling registers only encompass 50 lines of data instead of 100.

**Figure 13-23. Example of Subpicture**

The subpicture feature is enabled when the spen MMR control bit is set to '1'. The hols bit, when set to '0,' puts the Default Pixel Data lines at the top of the screen and the active video lines at the bottom of the screen.

When the hols bit is set to '1,' Active video lines are at the top of the screen and Default Pixel Data lines are at the bottom of the screen. The hols bit behavior is shown in [Figure](#page-118-1) 13-24.

**Figure 13-24. Subpicture HOLS Bit**

The lines per panel threshold (LPPT) bitfield defines the number of lines at the bottom of the picture for both hols = '1' or '0'. LPPT is an encoded value in the range {0:2047} used to represent the number of lines in the range {1:2048}.

**Figure 13-25. Raster Mode Display Format**

Data pixels (from 1 to P)

```
(page_119 Figure_4) – LCD mapping (pixels by column P and line L)

          left edge (first columns)                          right edge (last columns)
+--------------------------------------------------------------------------------------+
| (1,1) (2,1) (3,1) ------------------------------------------------> (P-2,1)(P-1,1)(P,1)
| (1,2) (2,2) ------------------------------------------------------>        (P-1,2)(P,2)
| (1,3) ------------------------------------------------------------>               (P,3)
|                                                                                      |
|                                      LCD                                             |
|                                                                                      |
| (1,L-2) ---------------------------------------------------------->               (P,L-2)
| (1,L-1) (2,L-1) -------------------------------------------------->        (P-1,L-1)(P,L-1)
| (1,L)  (2,L)  (3,L) ----------------------------------------------> (P-2,L)(P-1,L)(P,L)
+--------------------------------------------------------------------------------------+
^
|
Data lines (from 1 to L)
```

#### 13.3.6.7 Raster Controller Timing

Figure 13-26. Raster Mode Passive (STN) Timing Diagram

```
(page_120 Figure_4) – Passive STN timing (overview + zoom)

OVERVIEW (frame/lines)
time -->
            <--- VBP --->                 Lines per panel (LPP)                   <--- VFP --->
            VBP=0  VFP=0  VSW=1..64                                        VBP=0  VFP=0  VSW=1..64

LCD_HSYNC   |‾|_|‾|_|‾|_|‾|_|‾|_ ... _|‾|_|‾|_|‾|_|‾|_|‾|_ ... _|‾|_
             <----------- line time ----------->         ...        <--- line time --->

LCD_VSYNC   ____|‾‾‾‾‾‾|___________________________________________ ... ___________|‾‾‾‾‾‾|____
                (VSW)                 (active lines 1..LPP)                   (VSW)

LCD_DATA
 [15:0]     [L-1,P-1][L-1,P][1,1][1,2][1,3]...[1,P] ... [L, P-2][L,P-1][L,P] ...
            (example labels shown in figure: (line, pixel))

LCD_AC_BIAS_EN
            ____|‾‾‾‾‾‾‾‾‾‾|______________ ... _____________|‾‾‾‾‾‾‾‾‾‾|____
                 <---- AC bias frequency ACB (0..255) ---->   <--- ACB --->

ZOOM (one line boundary: end of line 5 -> start of line 6)
time -->
Line 5 data region (PPL pixels)     HFP            HSW              HBP          Line 6 data region (PPL pixels)
PPL = 16*(1..1024)               (1..256)       (1..64)          (1..256)

LCD_HSYNC   ________________________/‾‾‾‾‾‾‾‾‾‾\______________________________________________/‾‾‾...
                                  (HSW pulse)

LCD_PCLK    |‾|_|‾|_|‾|_ ... |‾|_|___________(no pixel clocks during porch/sync)___________|‾|_|‾|_ ... |‾|_|
            <--- pixel clocks for PPL --->                                        <--- pixel clocks for PPL --->

LCD_DATA
 [15:0]     [1,5][2,5]...[P,5]  (don’t care/blank) (don’t care/blank) (don’t care/blank) [1,6][2,6]...[P,6]
```

### **Figure 13-27. Raster Mode Active (TFT) Timing Diagram**

```
(page_121 Figure_3) – Active TFT timing (overview + zoom)

OVERVIEW (frame/vertical)
time -->
<-- VSW -->   <-- VBP -->            Lines per panel (LPP)             <-- VFP -->   <-- VSW -->
VSW=1..64     VBP=0..255                 (1..1024)                    VFP=0..255     (1..64)

LCD_HSYNC   |‾|_|‾|_|‾|_ ... (line pulses continue through frame) ... |‾|_|‾|_ ...

LCD_VSYNC   ____|‾‾‾‾‾‾‾‾‾‾|___________________________ ... __________________|‾‾‾‾‾‾|____
            (VSW pulse width)        (VBP + active LPP + VFP timing)          (next frame)

LCD_DATA
 [15:0]     (during active lines: pixels like [1,1],[1,2]...[P,1] ... [1,L]...[P,L])
            (blank/ignored during porches/sync)

LCD_AC_BIAS_EN
            ____|‾‾‾‾‾‾‾‾‾‾|____________ ... _____________|‾‾‾‾‾‾‾‾‾‾|____   Enable
                 <--- AC bias frequency ACB (0..255) --->   <--- ACB --->

ZOOM (horizontal timing within a line: end line 1 -> start line 2)
time -->
Line 1 data region (PPL pixels)     HFP            HSW              HBP          Line 2 data region (PPL pixels)
PPL = 16*(1..1024)               (1..256)       (1..64)          (1..256)

LCD_HSYNC   ________________________/‾‾‾‾‾‾‾‾‾‾\______________________________________________/‾‾‾...
                                  (HSW pulse)

LCD_PCLK    |‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_  (continuous CLK)

LCD_DATA
 [15:0]     [1,1][2,1]...[P,1]  (blank)        (blank)           (blank)        [1,2][2,2]...[P,2]
```

### *13.3.7 Interrupt Conditions*

#### **13.3.7.1 Highlander 0.8 Interrupts**

#### *13.3.7.1.1 Highlander Interrupt Basics*

The interrupt mechanism is Highlander 0.8-compliant and relies on the ipgvmodirq IP Generic. The ipgvmodirq module supports hardware-initiated interrupts, each of which can also be individually triggered by software. An interrupt mask function allows each interrupt to be masked or enabled. The software can read all of the raw interrupts or only those that are unmasked.

All pending interrupts in the LCD module must be serviced by the Host's Interrupt Service Routine before it exits.

The Interrupt Module registers are described in the following table.

**Table 13-12. Highlander 0.8 Interrupt Module Control Registers**

| Address Offset | Name  | Description                      |
|----------------|-------|----------------------------------|
| 0x58           | Reg22 | Interrupt Raw Status Register    |
| 0x5C           | Reg23 | Interrupt Masked Status Register |
| 0x60           | Reg24 | Interrupt Enable Set (Unmask)    |
| 0x64           | Reg25 | Interrupt Enable Clear (Mask)    |
| 0x68           | Reg26 | End of Interrupt Indicator       |

#### *13.3.7.1.2 Raw Status Register*

Interrupts are associated with a bit position. For instance, Hardware Interrupt 0 is physically connected to bit 0 of the interrupt controller and all Sets, Clears, and Masks to this interrupt will reference the Bit 0 location of the interrupt vector. Likewise, Hardware Interrupt 1 is referenced by bit 1 of the interrupt vector, and so on.

The Host CPU can see all the interrupts that have been set, regardless of the interrupt mask, by reading Reg22, the Raw Status Register.

If the Host CPU writes a '1' to a bit position in Reg 22, it will do a software set for the interrupt associated with that bit position.

#### *13.3.7.1.3 Masked Status Register*

The Masked Status Register contains all the pending interrupts that are unmasked (enabled). The Interrupt Service Routine should read this register to determine which interrupts must be serviced.

#### *13.3.7.1.4 Interrupt Enable Set Register*

To unmask an interrupt, the Host CPU writes a '1' to the appropriate bit position of the Enable Set (Unmask) register.

#### *13.3.7.1.5 Interrupt Enable Clear Register*

To mask an interrupt, the Host CPU writes a '1' to the appropriate bit position of the Enable Clear (Mask) register.

#### *13.3.7.1.6 End of Interrupt Register*

The ipgvmodirq module supports level or pulse interrupts to the CPU. For pulse interrupts, the Host must write to an end-of-interrupt (EOI), memory-mapped address to indicate that the Interrupt Service Routine has completed and is exiting. Any pending interrupts that have not been serviced will trigger another interrupt pulse to the Host CPU.

#### **13.3.7.2 Interrupt Sources**

#### *13.3.7.2.1 Overview of Interrupt Sources*

The interrupt sources include:

- DMA End of Frame 0
- DMA End of Frame 1
- Palette Loaded
- FIFO Underflow
- AC Bias Count
- Sync Lost
- Recurrent Frame Done
- LIDD or Raster Frame Done

#### *13.3.7.2.1.1 DMA End of Frame 0 and End of Frame 1 Interrupt*

The DMA End of Frame 0 and End of Frame 1 interrupts are triggered when the DMA module has completed transferring the contents of a frame buffer bounded by cfg\_fb0\_base/cfg\_bf0\_ceil or cfg\_fb1\_base/cfg\_fb1\_ceil.

#### *13.3.7.2.1.2 Palette Loaded Interrupt*

When cfg\_palmode is set to Palette-only or Palette+data, the Palette Loaded interrupt is triggered when the palette portion of the DMA transfer has been stored in the Palette RAM.

#### *13.3.7.2.1.3 FIFO Underflow Interrupt*

The FIFO Underflow interrupt is triggered when the real-time output needs to send a value for pixel data but one cannot be found in the FIFO.

#### *13.3.7.2.1.4 AC Bias Count Interrupt*

For Passive Matrix displays, a count can be kept of the number of times the AC Bias line toggles. Once the specified number of transitions has been seen, the AC Bias Count interrupt is triggered. The module will not post any further interrupts or keep counting AC Bias transitions until the interrupt has been cleared.

#### *13.3.7.2.1.5 Sync Lost Interrupt*

When the DMA module reads a frame buffer and stores it in the FIFO, it sets a start frame and an end frame indicator embedded with the data. On retrieving the data from the FIFO in the lcd\_clk domain, the Sync Lost interrupt is triggered if the start indicator is not found at the first pixel of a new frame.

#### *13.3.7.2.1.6 Recurrent Frame Done Interrupt*

In raster mode, the Recurrent Frame Done interrupt is triggered each time a complete frame has been sent to the interface pins.

#### *13.3.7.2.1.7 LIDD or Raster Frame Done Interrupt*

In LIDD DMA mode, a frame buffer of data is sent. When the frame buffer has completed, the LIDD Frame Done interrupt is triggered. In order to do another LIDD DMA, the DMA engine must be disabled and then re-enabled.

In Raster mode, the interrupt is triggered after cfg\_lcden is set to '0' and after the last frame is sent to the pins. After the Raster mode DMA is running, the interrupt occurs only once after the module is disabled.

### *13.3.8 DMA*

DDR access is handled internally by the DMA module. For Character Displays, the DMA module can transfer a sequence of data transactions from the DDR to LCD panel. By using the DMA instead of the Host CPU, the Host will not be stalled waiting for the slow external peripheral to complete.

For Passive and Active Matrix displays, the DMA is used to read frame buffers with associated palette information from DDR. The DMA parses the frame buffer according to the frame buffer type and supplies the raster processing chain with Palette information and pixel data as needed.

### *13.3.9 Power Management*

Power management within the DSS can be accomplished in several ways:

- 1. L4 OCP MConnect/SConnect can disable the internal L4 clock network.
- 2. L3 OCP MConnect/Sconnect can disable the internal L3 clock network.
- 3. Within the Clock Control register, there are clock enable registers to disable the clock networks to all major internal functional paths.
- 4. Power Compiler clock gates are automatically instantiated within datapaths to minimize active power.

Items 1 and 2 are accomplished using the standard IDLE (for L4) and STANDBY (for L3) IPGeneric modules. When these modules are instructed to disable clocks for the internal L3 or L4 (MMR) clock domains, the internal clock networks will be shut down. This shutdown applies to the external clock pins l3\_clk and l4\_clk.

All other internal clock domains (Item 3) can only be shut down by writing the appropriate register bit within the Clock Enable register. This software clock control applies to all other clock inputs.

Power Compiler clock gating is done automatically as a function of the design. There is no special control required for this operation.

Because the LCD normally drives displays, and because all video is sourced from the L3 clock domain, shutting down the L3 domain using the IPGenerics can cause undesirable display effects. In most circumstances, it will be necessary to hardware/software reset the LCD module after such an event has occurred.

## **13.4 Programming Model**

### *13.4.1 LCD Character Displays*

#### **13.4.1.1 Configuration Registers, Setup, and Settings**

#### *13.4.1.1.1 Configuration Registers*

Set the following to appropriate values for the target LCD character panel:

- cfg\_cs1\_e1\_pol
- cfg\_cs0\_e0\_pol
- ws\_dir\_pol
- cfg\_rs\_en\_pol
- cfg\_alepol

cfg\_lidd\_mode\_sel[2:0] defines the type of CPU bus that will be used in interfacing with the LCD character panel. Note that the clocked bus styles only support a single panel using CS0 since the clock pin takes a device pin that is otherwise used for CS1.

Set the following to appropriate bus timing parameters for the target LCD character panel:

- cfg\_w\_su
- cfg\_w\_strobe
- cfg\_w\_hold, cfg\_r\_su
- cfg\_r\_strobe
- cfg\_r\_hold
- cfg\_ta

A set of bus timing parameters are individually available for CS0 and CS1 such that the bus transactions can be customized for each of the two supported LCD character displays.

#### *13.4.1.1.2 Defining Panel Commands and Panel Data*

In the Hitachi interface mode used for the example panel, whether the Character Panel understands a data transfer as Command or Data depends on the state of the REGSEL input pin. Writing to the cfg\_adr\_indx register will output a Command transfer. Writing to the cfg\_data register will result in a Data transfer.

Functionally, the ALE (lcd\_fp pin) from the LCD controller is tied to the REGSEL input of the character panel.

For example, to send byte 0xAB as a command to the previously described character panel, the CPU would write 0x00AB to the adr\_indx register. To send byte 0xAB as data, the CPU would write 0x00AB to the data register.

#### **13.4.1.2 CPU Initiated Data Bus Transactions**

#### *13.4.1.2.1 Initiating Data Bus Transactions*

Writing to cfg\_cs0\_data will initiate a write transfer to the CS0 panel. Reading from cfg\_cs0\_data will initiate a read transfer from the CS0 panel.

Writing to cfg\_cs1\_data will initiate a write transfer to the CS1 panel. Reading from cfg\_cs1\_data will initiate a read transfer from the CS0 panel.

**NOTE:** Writes to CS1 translate to valid bus transactions only if cfg\_lidd\_mode\_sel[2:0] is configured for an asynchronous mode.

#### **13.4.1.3 DMA Initiated Data Bus Transactions for LIDD**

#### *13.4.1.3.1 DMA Overview for MPU Bus Output*

Writing a long sequence of data to the Character Display Panel will ensure that the CPU will be occupied for a long time. However, the DMA module supports a mode in which this sequence of data elements can first be written in DRAM by the CPU.

The DMA can read this sequence of commands or data from the DRAM and send it to the LCD Interface Display Driver (LIDD) module such that each data element becomes a write bus transaction to the external Character Panel/MPU Bus. The data bus write transaction can target either CS0 or CS1 and use the appropriate bus timing parameters.

Functionally, in this DMA LIDD mode, the DMA module sends the sequence of data to the LIDD module by acting as another CPU.

The DMA can only perform write bus transactions. It cannot read from the external character panel a series of data elements and store them in the DRAM.

When the LIDD module is controlled by the DMA module by setting cfg\_lidd\_dma\_en = '1', CPU reads or writes to cfg\_adr\_index and cfg\_data are not allowed.

The fb0\_base and fb0\_ceil registers define the address boundary of data elements to be sent out the character display by the DMA engine. Setting cfg\_lidd\_dma\_en from '0' to '1' will initiate the DMA as if a virtual CPU is reading data from the DDR and writing the values to Reg6 or Reg9. cfg\_dma\_cs0\_cs1 determines whether the virtual CPU writes to Reg6 (CS0) or Reg7 (CS1).

**NOTE:** Writes to CS1 translate to valid bus transactions only if cfg\_lidd\_mode\_sel[2:0] is configured for an asynchronous mode.

The DMA module requires the start and end DDR addresses to be on word-aligned byte addresses. The MPU/LIDD bus is a halfword (16 bit) output, so both the upper and lower halfwords of the DDR memory will be sent out. Thus, the number of data elements sent to the LIDD by the DMA must always result in an even number of bus MPU bus transactions. In other words, a transfer of three 32-bit words from DDR will result in six 16-bit bus transactions.

#### *13.4.1.3.2 MCU/LIDD DMA Setup: Example Pseudo Code*

Suppose we want to send by DMA a section of DDR memory from byte address 0x4 to byte address 0x3C to the MCU bus using chip select 0. The pseudo code for such an operation is listed below.

```
// Enable Clocks
wr 006C 0000_0007
// LCD Control Register
wr 0004 0000_8000 // set clock divisor
// LIDD Control Register
wr 000C 0000_000C // set output bus polarities and lidd_mode_sel
// LIDD CS0 Register
wr 0010 0822_1044 // set bus timing parameters for CS0
// DMA Control Register
wr 0040 0000_0030 // set DMA parameters like burst size, memory layout
// DMA FB0 Base Register
wr 0044 0000_0004 // DMA start byte address
// DMA FB0 Ceiling Register
wr 0048 0000_003C // DMA end byte address
// LIDD Control Register - enable DMA
wr 000C 0000_010C // Flip LIDD DMA enable bit
```

Once the DMA completes sending data to the Async FIFO, the Eof0 interrupt will occur. The Done interrupt will occur when the last word is written out the MPU bus.

The CPU must bring cfg\_lidd\_dma\_en low before the CPU can directly initiate MPU bus transactions or for the DMA module to start again.

#### **13.4.1.4 Passive Matrix**

#### *13.4.1.4.1 Monochrome Bitrate Awareness*

In a mostly testbench related note, care must be taken when configuring the module for Passive Matrix (cfg\_lcdtft = '0') monochrome (cfg\_lcdbw = '1') modes. In passive matrix mode, the Blue component of the Grayscaler output is used as the quantized value for each scan order pixel.

When cfg\_mono8b='1', eight pixel values must be sent through the grayscaler before one 8-bit output is ready. This output data represents the passive matrix output states for eight pixels.

Likewise, when cfg\_mono8b='0', four pixel values must be sent through the grayscaler before one 4-bit output is ready. This output data represents the passive matrix output states for four pixels.

The problem arises when the output clock is fast (cfg\_clkdiv=0x2). The data path must output a value every two system clocks. However, it takes four or eight system clocks to generate a data element to be output. In this case, the LCD module returns an underflow interrupt.

In practice, such a situation does not occur because passive matrix panels are slow by design.

### *13.4.2 Active Matrix Displays*

#### **13.4.2.1 Interfacing to Dual LVDS Transmitters**

The pixel clock rate for HD-sized pictures is approximately 148.5 MHz. At this speed, the LVDS link requires a double-wide data bus for transferring the even and odd pixels at half the pixel rate. The LCD Controller outputs one pixel per pixel clock cycle. Some LVDS transmitters accept a high speed, single pixel input and output to dual LVDS drivers, in which case external glue logic is unnecessary. For those LVDS transmitters that require the even and odd pixel to enter the LVDS transmitter at half the pixel clock rate, external logic is required.

### *13.4.3 System Interaction*

#### **13.4.3.1 DMA End of Frame Interrupts**

The LCD module works with the DMA such that data is fetched from DDR and sent to a FIFO memory. The DMA module does this fetching independently of the logic on the output side of the FIFO.

For LIDD mode DMA, the module fetches frame buffer 0. When the last word of frame buffer 0 is stored in the FIFO memory, the Eof0 interrupt is triggered (if cfg\_eof\_inten='1') and the DMA stops. The CPU has to set cfg\_lidd\_dma\_en='0', followed by a cfg\_lidd\_dma\_en='1', before the next burst from frame buffer 0 is read from DDR.

For Raster mode DMA, the module fetches frame buffer 0. When the last word of frame buffer 0 is stored in the FIFO memory, the Eof0 interrupt is triggered (if cfg\_eof\_inten='1') but the DMA does not stop. The DMA module ping pongs immediately to frame buffer 1 if cfg\_frame\_mode='1'. Otherwise, the DMA fetches the frame buffer 0 address range from DDR. When the DMA module fetches frame buffer 1, and the last word of frame buffer 1 is stored in the FIFO memory, the Eof1 interrupt is triggered (if cfg\_eof\_inten='1'). This pattern would repeat.

### *13.4.4 Palette Lookup*

For Active Matrix and Passive Matrix modes, the 12-bit Palette RAM Lookup can be used. For Active Matrix (cfg\_lcdtft = '1'), palette lookup is enabled when cfg\_tft24 = '0' and the bpp field in the Palette RAM is set to "000," "001," "010," or "011" (1, 2, 4, or 8 bpp). Palette lookup cannot used when the bpp field is set to "100" (12/16 bpp).

For Passive Matrix (cfg\_lcdtft = '0'), palette lookup is enabled when the bpp field in the Palette RAM is set to "000," "001," "010," or "011" (1, 2, 4, or 8 bpp). Palette lookup cannot be used when the bpp field is set to "100" (12/16 bpp).

Palette lookup scenarios are illustrated in [Figure](#page-129-0) 13-28.

When the bpp encoding is set to 1 bpp, each bit in a 16-bit frame buffer halfword is used to index the two bottom locations of the palette RAM. Suppose the frame buffer bit value is '0', this '0' indicates that the address 0 entry in the Palette RAM should be read. If the frame buffer bit value is '1,' the address 1 entry in the Palette RAM is used. The resulting 12-bit output from the Palette RAM is the quantized pixel value of a 4-bit per color component quantized pixel value.

When the bpp encoding is set to 2 bpp, every two bits in a 16 bit frame buffer halfword is used to index the bottom 4 locations of the palette RAM. Suppose the frame buffer bit value is "00." This "00" indicates that the address 0 entry in the Palette RAM should be read. If the frame buffer bit value is "01," the address 1 entry in the Palette RAM is used. When the frame buffer bit value is "10," the address 2 entry in the Palette RAM is read. Finally, if the frame buffer bit value is "11," the address 3 entry in the Palette RAM is read. The resulting 12 bit output from the Palette RAM is the quantized pixel value of the 4 bit per color component.

The 4 bpp encoding allows every four bits from a frame buffer halfword to address 16 entries in the Palette RAM.

The 8 bpp encoding enables every byte from a frame buffer halfword to address one of the 256 entries in the Palette RAM.

#### **Figure 13-28. Palette Lookup Examples**

```
Palette RAM selection examples (output: 12 bits)

1 BPP Example
-------------
1 bpp pixel value selects 1 of 2 palette locations
      +-------------------+
      | Palette RAM       |
      | [0]               |  <-- selected by bit 0 = 0
      | [1]               |  <-- selected by bit 0 = 1
      | ...               |
      | 256 entries       |
      +-------------------+
            -> 12 bits

2 BPP Example
-------------
2 bpp pixel value selects 1 of 4 palette locations
      +-------------------+
      | Palette RAM       |
      | [0]               |
      | [1]               |
      | [2]               |
      | [3]               |  <-- selected by bits [1:0]
      | ...               |
      | 256 entries       |
      +-------------------+
            -> 12 bits

4 BPP Example
-------------
4 bpp pixel value selects 1 of 16 palette locations
      +-------------------+
      | Palette RAM       |
      | [0] .. [15]       |  <-- selected by bits [3:0]
      | ...               |
      | 256 entries       |
      +-------------------+
            -> 12 bits

8 BPP Example
-------------
8 bpp pixel value selects 1 of 256 palette locations
      +-------------------+
      | Palette RAM       |
      | [0] .. [255]      |  <-- selected by bits [7:0]
      | 256 entries       |
      +-------------------+
            -> 12 bits
```

A 16-bit halfword is read from the DDR frame buffer. This halfword can be byte lane and halfword swapped using the DMA configuration values cfg\_byte\_swap and cfg\_bigendian. This section will deal with the frame buffer data as it is returned post swapped from the DMA module.

The DMA module actually outputs a 32-bit word. The Palette Lookup logic uses the lower halfword first, followed by the upper halfword. The cfg\_rdorder and cfg\_nibmode registers determine the raster read ordering of the frame buffer data to be sent to the palette lookup table.

There are precedence rules for the hardware as it parses each 16-bit word from the frame buffer.

- 1. If cfg\_rdorder = '0', the data halfword is parsed from the least significant bit to the most significant bit.
- 2. Else, if cfg\_nibmode = '1', the data halfword is parsed byte swapped with the scan order going from the most significant bit of each byte to the least significant bit of each byte.
- 3. Otherwise, the data halfword is parsed from the most significant bit to the least significant bit.

The bitwise scan order for each halfword fetched from the frame buffer is shown in the following lists. The bitfields returned are used to determine the addressing of the Palette RAM.

#### **Frame buffer halfword scan order for 1 bpp**

- 1. If cfg\_rdorder = 0, scan order is [0] [1] [2] [3] [4] [5] [6] [7] [8] [9] [10] [11][ 12] [13] [14] [15]
- 2. Else if cfg\_nibmode=1, scan order is [7] [6] [5] [4] [3] [2] [1] [0] [15] [14] [13] [12] [11] [10] [9] [8]
- 3. Otherwise, scan order is [15] [14] [13] [12] [11] [10] [9] [8] [7] [6] [5] [4] [3] [2] [1] [0]

### **Frame buffer halfword scan order for 2 bpp**

- 1. If cfg\_rdorder = 0, scan order is [1:0] [3:2] [5:4] [7:6] [9:8] [11:10][ 13:12] [15:14]
- 2. Else if cfg\_nibmode=1, scan order is [7:6] [5:4] [3:2] [1:0] [15:14] [13:12] [11:10] [9:8]
- 3. Otherwise, scan order is [15:14] [13:12] [11:10] [9:8] [7:6] [5:4] [3:2] [1:0]

#### **Frame buffer halfword scan order for 4 bpp**

- 1. If cfg\_rdorder = 0, scan order is [3:0] [7:4] [11:8] [15:12]
- 2. Else if cfg\_nibmode=1, scan order is [7:4] [3:0] [15:12] [11:8]
- 3. Otherwise, scan order is [15:12] [11:8] [7:4] [3:0]

### **Frame buffer halfword scan order for 8 bpp**

- 1. If cfg\_rdorder = 0, scan order is [7:0] [15:8]
- 2. Else if cfg\_nibmode=1, scan order is [7:0] [15:8]
- 3. Otherwise, scan order is [15:8] [7:0]

### *13.4.5 Test Logic*

### *13.4.6 Disable and Software Reset Sequence*

In Raster Modes, the module must be disabled before applying a software reset. When cfg\_lcden is set to '0' to disable the module, the output continues to the end of the current frame.

The Done interrupt will trigger once the frame is complete. The software reset can then be applied to the module.

The software reset will clear all the frame information in the FIFO. Upon a restart, the L3 DMA will fetch from the fb0\_base address.

### To summarize:

- Set cfg\_lcden='0'.
- Wait for the Done interrupt.
- Set the software reset bits high (cfg\_main\_rst='1' or [cfg\_dma\_rst='1' and cfg\_core\_rst='1']) for several cycles.
- Set the resets back low.
- Set cfg\_lcden='1'.

The disable and reset sequence must be done in this order to properly operate the LCD module and the EMIF.

### *13.4.7 Precedence Order for Determining Frame Buffer Type*

The precedence order for determining frame buffer type is specified as follows:

```
If (cfg_lcdtft == 1) // active matrix
    If (cfg_tft24 == 1) // 24 bpp
        If (cfg_tft24_unpacked == 1)
            4 pixels in 4 words
        else
            4 pixels in 3 words
    else // 1/2/4/8/12/16 bpp
        if (bpp[2] == 1)
            12/16 bpp data
        else
            if (bpp == 0)
                1 bpp data
            else if (bpp == 1)
                2 bpp data
            else if (bpp == 2)
                4 bpp data
            else // if (bpp == 3)
                8 bpp data
else // passive matrix
    if (bpp[2] == 1)
        12/16 bpp data
    else
        if (bpp == 0)
            1 bpp data
        else if (bpp == 1)
            2 bpp data
        else if (bpp == 2)
            4 bpp data
        else // if (bpp == 3)
            8 bpp data
```

## **13.5 Registers**

### *13.5.1 LCD Registers*

Table [13-13](#page-132-1) lists the memory-mapped registers for the LCD. All register offset addresses not listed in Table [13-13](#page-132-1) should be considered as reserved locations and the register contents should not be modified.

**Table 13-13. LCD Registers**

| Offset | Acronym            | Register Name | Section           |
|--------|--------------------|---------------|-------------------|
| 0h     | PID                |               | Section 13.5.1.1  |
| 4h     | CTRL               |               | Section 13.5.1.2  |
| Ch     | LIDD_CTRL          |               | Section 13.5.1.3  |
| 10h    | LIDD_CS0_CONF      |               | Section 13.5.1.4  |
| 14h    | LIDD_CS0_ADDR      |               | Section 13.5.1.5  |
| 18h    | LIDD_CS0_DATA      |               | Section 13.5.1.6  |
| 1Ch    | LIDD_CS1_CONF      |               | Section 13.5.1.7  |
| 20h    | LIDD_CS1_ADDR      |               | Section 13.5.1.8  |
| 24h    | LIDD_CS1_DATA      |               | Section 13.5.1.9  |
| 28h    | RASTER_CTRL        |               | Section 13.5.1.10 |
| 2Ch    | RASTER_TIMING_0    |               | Section 13.5.1.11 |
| 30h    | RASTER_TIMING_1    |               | Section 13.5.1.12 |
| 34h    | RASTER_TIMING_2    |               | Section 13.5.1.13 |
| 38h    | RASTER_SUBPANEL    |               | Section 13.5.1.14 |
| 3Ch    | RASTER_SUBPANEL2   |               | Section 13.5.1.15 |
| 40h    | LCDDMA_CTRL        |               | Section 13.5.1.16 |
| 44h    | LCDDMA_FB0_BASE    |               | Section 13.5.1.17 |
| 48h    | LCDDMA_FB0_CEILING |               | Section 13.5.1.18 |
| 4Ch    | LCDDMA_FB1_BASE    |               | Section 13.5.1.19 |
| 50h    | LCDDMA_FB1_CEILING |               | Section 13.5.1.20 |
| 54h    | SYSCONFIG          |               | Section 13.5.1.21 |
| 58h    | IRQSTATUS_RAW      |               | Section 13.5.1.22 |
| 5Ch    | IRQSTATUS          |               | Section 13.5.1.23 |
| 60h    | IRQENABLE_SET      |               | Section 13.5.1.24 |
| 64h    | IRQENABLE_CLEAR    |               | Section 13.5.1.25 |
| 6Ch    | CLKC_ENABLE        |               | Section 13.5.1.26 |
| 70h    | CLKC_RESET         |               | Section 13.5.1.27 |

#### **13.5.1.1 PID Register (offset = 0h) [reset = 0h]**

PID is shown in [Figure](#page-133-1) 13-29 and described in Table [13-14.](#page-133-2)

#### **Figure 13-29. PID Register**

| 31     | 30     | 29   | 28       | 27   | 26    | 25    | 24 |  |  |
|--------|--------|------|----------|------|-------|-------|----|--|--|
| scheme |        |      | RESERVED | func |       |       |    |  |  |
|        | R-0h   |      | R-0h     | R-0h |       |       |    |  |  |
| 23     | 22     | 21   | 20       | 19   | 18    | 17    | 16 |  |  |
|        |        |      | func     |      |       |       |    |  |  |
|        |        |      | R-0h     |      |       |       |    |  |  |
| 15     | 14     | 13   | 12       | 11   | 10    | 9     | 8  |  |  |
|        |        | rtl  |          |      |       | major |    |  |  |
|        |        | R-0h |          |      |       | R-0h  |    |  |  |
| 7      | 6      | 5    | 4        | 3    | 2     | 1     | 0  |  |  |
|        | custom |      |          |      | minor |       |    |  |  |
|        | R-0h   |      |          |      | R-0h  |       |    |  |  |
|        |        |      |          |      |       |       |    |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-14. PID Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                              |
|-------|----------|------|-------|--------------------------------------------------------------------------|
| 31-30 | scheme   | R    | 0h    | The scheme of the register used.<br>This field indicates the 3.5 Method. |
| 29-28 | RESERVED | R    | 0h    |                                                                          |
| 27-16 | func     | R    | 0h    | The function of the module being used.                                   |
| 15-11 | rtl      | R    | 0h    | The Release number for this IP.                                          |
| 10-8  | major    | R    | 0h    | Major Release Number                                                     |
| 7-6   | custom   | R    | 0h    | Custom IP                                                                |
| 5-0   | minor    | R    | 0h    | Minor Release Number                                                     |

#### **13.5.1.2 CTRL Register (offset = 4h) [reset = 0h]**

CTRL is shown in [Figure](#page-134-1) 13-30 and described in Table [13-15](#page-134-2).

**Figure 13-30. CTRL Register**

| 31 | 30 | 29 | 28       | 27       | 26 | 25                     | 24      |
|----|----|----|----------|----------|----|------------------------|---------|
|    |    |    |          | RESERVED |    |                        |         |
|    |    |    |          | R-0h     |    |                        |         |
| 23 | 22 | 21 | 20       | 19       | 18 | 17                     | 16      |
|    |    |    |          | RESERVED |    |                        |         |
|    |    |    |          | R-0h     |    |                        |         |
| 15 | 14 | 13 | 12       | 11       | 10 | 9                      | 8       |
|    |    |    |          | clkdiv   |    |                        |         |
|    |    |    |          | R/W-0h   |    |                        |         |
| 7  | 6  | 5  | 4        | 3        | 2  | 1                      | 0       |
|    |    |    | RESERVED |          |    | auto_uflow_rest<br>art | modesel |
|    |    |    | R/W-0h   |          |    | R/W-0h                 | R/W-0h  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 13-15. CTRL Register Field Descriptions**

| Bit   | Field              | Type | Reset | Description                                                                                                                                                                                                                                                                                |
|-------|--------------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED           | R    | 0h    |                                                                                                                                                                                                                                                                                            |
| 15-8  | clkdiv             | R/W  | 0h    | Clock divisor.<br>Raster mode: Values of 2 through 255 are permitted and resulting<br>pixel clock is lcd_clk/2 through lcd_clk/255.<br>LIDD mode: Values of 0 through 255 are permitted with resulting<br>MCLK of lcd_clk/1 through lcd_clk/255 where both 0 and 1 result in<br>lcd_clk/1. |
| 7-2   | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                                                            |
| 1     | auto_uflow_restart | R/W  | 0h    | 0 = On an underflow, the software has to restart the module.<br>1 = On an underflow, the hardware will restart on the next frame.                                                                                                                                                          |
| 0     | modesel            | R/W  | 0h    | LCD Mode select.<br>0 = LCD Controller in LIDD Mode.<br>1 = LCD Controller in Raster Mode.                                                                                                                                                                                                 |

#### **13.5.1.3 LIDD\_CTRL Register (offset = Ch) [reset = 0h]**

LIDD\_CTRL is shown in [Figure](#page-135-1) 13-31 and described in Table [13-16.](#page-135-2)

**Figure 13-31. LIDD\_CTRL Register**

| 31         | 30         | 29         | 28        | 27       | 26 | 25            | 24          |
|------------|------------|------------|-----------|----------|----|---------------|-------------|
|            |            |            |           | RESERVED |    |               |             |
|            |            |            |           | R-0h     |    |               |             |
| 23         | 22         | 21         | 20        | 19       | 18 | 17            | 16          |
|            |            |            |           | RESERVED |    |               |             |
|            |            |            |           | R-0h     |    |               |             |
| 15         | 14         | 13         | 12        | 11       | 10 | 9             | 8           |
|            |            | RESERVED   |           |          |    | dma_cs0_cs1   | lidd_dma_en |
|            |            | R-0h       |           |          |    | R/W-0h        | R/W-0h      |
| 7          | 6          | 5          | 4         | 3        | 2  | 1             | 0           |
| cs1_e1_pol | cs0_e0_pol | ws_dir_pol | rs_en_pol | alepol   |    | lidd_mode_sel |             |
| R/W-0h     | R/W-0h     | R/W-0h     | R/W-0h    | R/W-0h   |    | R/W-0h        |             |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-16. LIDD\_CTRL Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                  |
|-------|-------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED    | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 9     | dma_cs0_cs1 | R/W  | 0h    | CS0/CS1 Select for LIDD DMA writes.<br>0 = DMA writes to LIDD CS0.<br>1 = DMA writes for LIDD CS1.                                                                                                                                                                                                                                                                                                                           |
| 8     | lidd_dma_en | R/W  | 0h    | LIDD DMA Enable.<br>0 = Deactivate DMA control of LIDD interface.<br>DMA control is released upon completion of transfer of the current<br>frame of data (LIDD Frame Done) after this bit is cleared.<br>The MPU has direct read/write access to the panel in this mode.<br>1 = Activate DMA to drive LIDD interface to support streaming data<br>to smart panels.<br>The MPU cannot access the panel directly in this mode. |
| 7     | cs1_e1_pol  | R/W  | 0h    | Chip Select 1/Enable 1 (Secondary) Polarity Control.<br>0 = Do Not Invert Chip Select 1/Enable 1.<br>Chip Select 1 is active low by default.<br>Enable 1 is active high by default.<br>1 = Invert Chip Select 1/Enable 1.                                                                                                                                                                                                    |
| 6     | cs0_e0_pol  | R/W  | 0h    | Chip Select 0/Enable 0 (Secondary) Polarity Control.<br>0 = Do Not Invert Chip Select 0/Enable 0.<br>Chip Select 0 is active low by default.<br>Enable 0 is active high by default.<br>1 = Invert Chip Select 0/Enable 0.                                                                                                                                                                                                    |
| 5     | ws_dir_pol  | R/W  | 0h    | Write Strobe/Direction Polarity Control.<br>0 = Do Not Invert Write Strobe/Direction.<br>Write Strobe/Direction is active low/write low by default.<br>1 = Invert Write Strobe/Direction.                                                                                                                                                                                                                                    |
| 4     | rs_en_pol   | R/W  | 0h    | Read Strobe/Direction Polarity Control.<br>0 = Do Not Invert Read Strobe/Direction.<br>Read Strobe/Direction is active low/write low by default.<br>1 = Invert Read Strobe/Direction.                                                                                                                                                                                                                                        |
| 3     | alepol      | R/W  | 0h    | Address Latch Enable (ALE) Polarity Control.<br>0 = Do Not Invert ALE.<br>ALE is active low by default.<br>1 = Invert.                                                                                                                                                                                                                                                                                                       |

**Table 13-16. LIDD\_CTRL Register Field Descriptions (continued)**

| Bit | Field         | Type | Reset | Description                                                                                                                                                                                                                                         |
|-----|---------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 2-0 | lidd_mode_sel | R/W  | 0h    | LIDD Mode Select.<br>Selects type of LCD display interface for the LIDD to drive.<br>000b = Sync MPU68.<br>001b = Async MPU68.<br>010b = Sync MPU80.<br>011b = Async MPU80.<br>100b = Hitachi (Async).<br>101b = N/A.<br>110b = N/A.<br>111b = N/A. |

#### **13.5.1.4 LIDD\_CS0\_CONF Register (offset = 10h) [reset = 0h]**

LIDD\_CS0\_CONF is shown in [Figure](#page-137-1) 13-32 and described in Table [13-17.](#page-137-2)

### **Figure 13-32. LIDD\_CS0\_CONF Register**

| 31 | 30 | 29     | 28 | 27 | 26 | 25 | 24       | 23       | 22 | 21 | 20 | 19     | 18     | 17 | 16        |
|----|----|--------|----|----|----|----|----------|----------|----|----|----|--------|--------|----|-----------|
|    |    | w_su   |    |    |    |    |          | w_strobe |    |    |    |        | w_hold |    | r_su      |
|    |    | R/W-0h |    |    |    |    |          | R/W-0h   |    |    |    |        | R/W-0h |    | R/W<br>0h |
| 15 | 14 | 13     | 12 | 11 | 10 | 9  | 8        | 7        | 6  | 5  | 4  | 3      | 2      | 1  | 0         |
|    |    | r_su   |    |    |    |    | r_strobe |          |    |    |    | r_hold |        |    | ta        |
|    |    | R/W-0h |    |    |    |    | R/W-0h   |          |    |    |    | R/W-0h |        |    | R/W-0h    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-17. LIDD\_CS0\_CONF Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                     |
|-------|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-27 | w_su     | R/W  | 0h    | Write Strobe Set-Up cycles.<br>When performing a write access, this field defines the number of<br>memclk cycles that Data Bus/Pad Output Enable, the Direction bit,<br>and Chip Select 0 have to be ready before the Write Strobe is<br>asserted.<br>The minimum value is 0x0. |
| 26-21 | w_strobe | R/W  | 0h    | Write Strobe Duration cycles.<br>Field value defines the number of memclk cycles for which the Write<br>Strobe is held active when performing a write access.<br>The minimum value is 0x1.                                                                                      |
| 20-17 | w_hold   | R/W  | 0h    | Write Strobe Hold cycles.<br>Field value defines the number of memclk cycles for which Data<br>Bus/Pas Output Enable, ALE, the Direction bit, and Chip Select 0<br>are held after the Write Strobe is de-asserted when performing write<br>access.<br>The minimum value is 0x1. |
| 16-12 | r_su     | R/W  | 0h    | Read Strobe Set-Up cycles.<br>When performing a read access, this field defines the number of<br>memclk cycles that Data Bus/Pad Output Enable, the Direction bit,<br>and Chip Select 0 have to be ready before the Read Strobe is<br>asserted.                                 |
| 11-6  | r_strobe | R/W  | 0h    | Read Strobe Duration cycles.<br>Field value defines the number of memclk cycles for which the Read<br>Strobe is held active when performing a read access.<br>The minimum value is 0x1.                                                                                         |
| 5-2   | r_hold   | R/W  | 0h    | Read Strobe Hold cycles.<br>Field value defines the number of memclk cycles for which Data<br>Bus/Pad Output Enable, the Direction bit, and Chip Select 0 are held<br>after the Read Strobe is deasserted when performing a read access.<br>The minimum value is 0x1.           |
| 1-0   | ta       | R/W  | 0h    | Field value defines the number of memclk (ta+1) cycles between the<br>end of one CS0 device access and the start of another CS0 device<br>access unless the two accesses are both Reads.<br>In this case, this delay is not incurred.<br>The minimum value is 0x0.              |

#### **13.5.1.5 LIDD\_CS0\_ADDR Register (offset = 14h) [reset = 0h]**

LIDD\_CS0\_ADDR is shown in [Figure](#page-138-1) 13-33 and described in Table [13-18.](#page-138-2)

### **Figure 13-33. LIDD\_CS0\_ADDR Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23       | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7        | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|----|----|----|---|---|----------|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   | adr_indx |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   | R/W-0h   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-18. LIDD\_CS0\_ADDR Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 15-0  | adr_indx | R/W  | 0h    | The LCD Controller supports a shared Address/Data output bus.<br>A write to this register would initiate a bus write transaction.<br>A read from this register would initiate a bus read transaction.<br>CPU reads and writes to this register are not permitted if the LIDD<br>module is in DMA mode (cfg_lidd_dma_en = 1).<br>If the LIDD is being used as a generic bus interface, writing to this<br>register can store adr_indx to an external transparent latch holding a<br>16-bit address.<br>If the LIDD is being used to interface with a character based LCD<br>panel in configuration mode, reading and writing to this register can<br>be used to access the command instruction area of the panel. |

#### **13.5.1.6 LIDD\_CS0\_DATA Register (offset = 18h) [reset = 0h]**

LIDD\_CS0\_DATA is shown in [Figure](#page-139-1) 13-34 and described in Table [13-19](#page-139-2).

### **Figure 13-34. LIDD\_CS0\_DATA Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23       | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|----|----|----|---|--------|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |    |    |    |   | data   |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |    |    |    |   | R/W-0h |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 13-19. LIDD\_CS0\_DATA Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 15-0  | data     | R/W  | 0h    | The LCD Controller supports a shared Address/Data output bus.<br>A write to this register would initiate a bus write transaction.<br>A read from this register would initiate a bus read transaction.<br>CPU reads and writes to this register are not permitted if the LIDD<br>module is in DMA mode (cfg_lidd_dma_en = 1).<br>If the LIDD is being used as a generic bus interface, writing to this<br>register can store adr_indx to an external transparent latch holding a<br>16-bit address.<br>If the LIDD is being used to interface with a character based LCD<br>panel in configuration mode, reading and writing to this register can<br>be used to access the command instruction area of the panel. |

#### **13.5.1.7 LIDD\_CS1\_CONF Register (offset = 1Ch) [reset = 0h]**

LIDD\_CS1\_CONF is shown in [Figure](#page-140-1) 13-35 and described in Table [13-20.](#page-140-2)

#### **Figure 13-35. LIDD\_CS1\_CONF Register**

| 31 | 30 | 29     | 28 | 27 | 26 | 25 | 24       | 23       | 22 | 21 | 20 | 19     | 18     | 17 | 16        |
|----|----|--------|----|----|----|----|----------|----------|----|----|----|--------|--------|----|-----------|
|    |    | w_su   |    |    |    |    |          | w_strobe |    |    |    |        | w_hold |    | r_su      |
|    |    | R/W-0h |    |    |    |    |          | R/W-0h   |    |    |    |        | R/W-0h |    | R/W<br>0h |
| 15 | 14 | 13     | 12 | 11 | 10 | 9  | 8        | 7        | 6  | 5  | 4  | 3      | 2      | 1  | 0         |
|    |    | r_su   |    |    |    |    | r_strobe |          |    |    |    | r_hold |        |    | ta        |
|    |    | R/W-0h |    |    |    |    | R/W-0h   |          |    |    |    | R/W-0h |        |    | R/W-0h    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 13-20. LIDD\_CS1\_CONF Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                       |
|-------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-27 | w_su     | R/W  | 0h    | Write Strobe Set-Up cycles.<br>When performing a write access, this field defines the number of<br>memclk cycles that Data Bus/Pad Output Enable, , the Direction bit,<br>and Chip Select 1 have to be ready before the Write Strobe is<br>asserted.<br>The minimum value is 0x0. |
| 26-21 | w_strobe | R/W  | 0h    | Write Strobe Duration cycles.<br>Field value defines the number of memclk cycles for which the Write<br>Strobe is held active when performing a write access.<br>The minimum value is 0x1.                                                                                        |
| 20-17 | w_hold   | R/W  | 0h    | Write Strobe Hold cycles.<br>Field value defines the number of memclk cycles for which Data<br>Bus/Pad Output Enable, ALE, the Direction bit, and Chip Select 1<br>are held after the Write Strobe is deasserted when performing a<br>write access.<br>The minimum value is 0x1.  |
| 16-12 | r_su     | R/W  | 0h    | Read Strobe Set-Up cycles.<br>When performing a read access, this field defines the number of<br>memclk cycles that Data Bus/Pad Output Enable, , the Direction bit,<br>and Chip Select 1 have to be ready before the Read Strobe is<br>asserted.<br>The minimum value is 0x0.    |
| 11-6  | r_strobe | R/W  | 0h    | Read Strobe Duration cycles.<br>Field value defines the number of memclk cycles for which the Read<br>Strobe is held active when performing a read access.<br>The minimum value is 0x1.                                                                                           |
| 5-2   | r_hold   | R/W  | 0h    | Read Strobe Hold cycles.<br>Field value defines the number of memclk cycles for which Data<br>Bus/Pad Output Enable, , the Direction bit, and Chip Select 1 are<br>held after the Read Strobe is deasserted when performing a read<br>access.<br>The minimum value is 0x1.        |
| 1-0   | ta       | R/W  | 0h    | Field value defines the number of memclk (ta+1) cycles between the<br>end of one CS1 device access and the start of another CS1 device<br>access unless the two accesses are both Reads.<br>In this case, this delay is not incurred.<br>The minimum value is 0x0.                |

#### **13.5.1.8 LIDD\_CS1\_ADDR Register (offset = 20h) [reset = 0h]**

LIDD\_CS1\_ADDR is shown in [Figure](#page-141-1) 13-36 and described in Table [13-21.](#page-141-2)

#### **Figure 13-36. LIDD\_CS1\_ADDR Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23       | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7        | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|----|----|----|---|---|----------|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   | adr_indx |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   | R/W-0h   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-21. LIDD\_CS1\_ADDR Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 15-0  | adr_indx | R/W  | 0h    | The LCD Controller supports a shared Address/Data output bus.<br>A write to this register would initiate a bus write transaction.<br>A read from this register would initiate a bus read transaction.<br>CPU reads and writes to this register are not permitted if the LIDD<br>module is in DMA mode (cfg_lidd_dma_en = 1).<br>If the LIDD is being used as a generic bus interface, writing to this<br>register can store adr_indx to an external transparent latch holding a<br>16-bit address.<br>If the LIDD is being used to interface with a character based LCD<br>panel in configuration mode, reading and writing to this register can<br>be used to access the command instruction area of the panel. |

#### **13.5.1.9 LIDD\_CS1\_DATA Register (offset = 24h) [reset = 0h]**

LIDD\_CS1\_DATA is shown in [Figure](#page-142-1) 13-37 and described in Table [13-22](#page-142-2).

### **Figure 13-37. LIDD\_CS1\_DATA Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23       | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|----------|----|----|----|----|----|----|----|----|----|----|----|----|----|---|--------|---|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    | RESERVED |    |    |    |    |    |    |    |    |    |    |    |    |    |   | data   |   |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    | R-0h     |    |    |    |    |    |    |    |    |    |    |    |    |    |   | R/W-0h |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 13-22. LIDD\_CS1\_DATA Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 15-0  | data     | R/W  | 0h    | The LCD Controller supports a shared Address/Data output bus.<br>A write to this register would initiate a bus write transaction.<br>A read from this register would initiate a bus read transaction.<br>CPU reads and writes to this register are not permitted if the LIDD<br>module is in DMA mode (cfg_lidd_dma_en = 1).<br>If the LIDD is being used as a generic bus interface, writing to this<br>register can store adr_indx to an external transparent latch holding a<br>16-bit address.<br>If the LIDD is being used to interface with a character based LCD<br>panel in configuration mode, reading and writing to this register can<br>be used to access the command instruction area of the panel. |

#### **13.5.1.10 RASTER\_CTRL Register (offset = 28h) [reset = 0h]**

RASTER\_CTRL is shown in [Figure](#page-143-1) 13-38 and described in Table [13-23](#page-143-2).

#### **Figure 13-38. RASTER\_CTRL Register**

| 31     | 30      | 29       | 28       | 27 | 26            | 25     | 24      |
|--------|---------|----------|----------|----|---------------|--------|---------|
|        |         | RESERVED |          |    | tft24unpacked | tft24  | stn565  |
|        |         | R/W-0h   |          |    | R/W-0h        | R/W-0h | R/W-0h  |
| 23     | 22      | 21       | 20       | 19 | 18            | 17     | 16      |
| tftmap | nibmode |          | palmode  |    | reqdly        |        |         |
| R/W-0h | R/W-0h  |          | R/W-0h   |    | R/W-0h        |        |         |
| 15     | 14      | 13       | 12       | 11 | 10            | 9      | 8       |
|        |         | reqdly   |          |    | RESERVED      | mono8b | rdorder |
|        |         | R/W-0h   |          |    | R/W-0h        | R/W-0h | R/W-0h  |
| 7      | 6       | 5        | 4        | 3  | 2             | 1      | 0       |
| lcdtft |         |          | RESERVED |    |               | lcdbw  | lcden   |
| R/W-0h |         |          | R/W-0h   |    |               | R/W-0h | R/W-0h  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-23. RASTER\_CTRL Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|-------|---------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-27 | RESERVED      | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 26    | tft24unpacked | R/W  | 0h    | 24 bit Mode Packing.<br>Only used when cfg_tft<br>24=1 and cfg_lcdtft=1.<br>0 =<br>24-bit pixels are packed into 32 bit boundaries, which means 4 pixels<br>are saved in every three words.<br>Word<br>0: pix1<br>[7:0], pix0<br>[23:0].<br>Word<br>1: pix2<br>[15:0], pix1<br>[23:8].<br>Word<br>2: pix3<br>[23:0], pix2<br>[23:16].<br>1 =<br>24-bit pixels are stored unpacked in with the uppermost byte unused.<br>Word<br>0: Unused<br>[7:0], pix0<br>[23:0].<br>Word<br>1: Unused<br>[7:0], pix1<br>[23:0].<br>Word<br>2: Unused<br>[7:0], pix2<br>[23:0].<br>Word<br>3: Unused<br>[7:0], pix3<br>[23:0]. |
| 25    | tft24         | R/W  | 0h    | 24 bit mode.<br>0 = off<br>1 = on (<br>24-bit data in active mode).<br>The format of the framebuffer data depends on cfg_tft24unpacked.                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |

**Table 13-23. RASTER\_CTRL Register Field Descriptions (continued)**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 24    | stn565   | R/W  | 0h    | Passive Matrix Mode only (cfg_lcdtft='0') and 16 bpp raw data<br>framebuffers (bpp = '00').<br>If the bpp field in the framebuffer palette header is '00' (12/16/24 bpp<br>source), then the DDR contains raw data and the palette lookup is<br>bypassed.<br>Only for this case, this bit selects whether the framebuffer format is<br>16 bpp 565 or 12 bpp.<br>The Grayscaler can only take 12 bits per pixel.<br>The framebuffer data is 16 bits per pixel 565 when stn565 is set to<br>'1' and only the 4 most significant bits of each color component are<br>sent to the Grayscaler input.<br>0 = Framebuffer is 12 bpp packed in bits<br>[11:0].<br>1 = Framebuffer is 16 bpp 565.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 23    | tftmap   | R/W  | 0h    | TFT Mode Alternate Signal Mapping for Palettized framebuffer.<br>Must be '0' for all 12/16/24 bpp raw data formats.<br>Can only be '1' for 1/2/4/8 bpp Palette Lookup data.<br>Valid only in Active Matrix mode when cfg_lcdtft='1'.<br>0 = 4 bits per component output data for 1, 2, 4, and 8 bpp modes<br>will be right aligned on lcd_pins (<br>11:0).<br>1 = 4 bits per component output data for 1, 2, 4, and 8 bpp will be<br>converted to 5,6,5, format and use pins (<br>15:0) R3 R2 R1 R0 R3 G3 G2 G1 G0 G3 G2 B3 B2 B1 B0 B3                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 22    | nibmode  | R/W  | 0h    | Nibble Mode.<br>This bit is used to determine palette indexing and is used in<br>conjunction with cfg_rdorder.<br>0 = Nibble mode is disabled.<br>1 = Nibble mode is enabled.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| 21-20 | palmode  | R/W  | 0h    | Palette Loading Mode.<br>00 = Palette and data loading, reset value.<br>01 = Palette loading only.<br>10 = Data loading only For Raw Data (12/16/24 bpp) framebuffers,<br>no palette lookup is employed.<br>Thus, these framebuffers should use the data-only loading mode.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 19-12 | reqdly   | R/W  | 0h    | Palette Loading Delay When loading the Palette from DDR, palette<br>data is burst into the internal Palette SRAM from the Async FIFO.<br>1-,<br>2-, and<br>4-bit per pixel framebuffer encodings use a fixed<br>16-word entry palette residing above the video data.<br>The 8 bit per pixel framebuffer encoding uses a<br>256-word entry palette residing above the video data.<br>Likewise, 12, 16, and 24 bit per pixel framebuffer encodings also<br>define a<br>256-word entry palette even though these encodings will not do a full<br>bit-depth palette lookup.<br>However, the<br>256-word palette entry must still be read from DDR as a framebuffer<br>is fetched.<br>Bursting in 256 words in sequential lcd_clk cycles may cause the<br>Async FIFO to underflow depending on the SOC DDR burst<br>bandwidth.<br>This<br>8-bit reqdly parameter pauses reading of the Palette data from the<br>Async FIFO between each burst of 16 words.<br>The delay is in terms of lcd_clk (system clock) cycles.<br>Value (<br>0-255) used to specify the number of system clock cycles that<br>should be paused between bursts of 16 word reads from the Async<br>FIFO while loading the Palette SRAM.<br>Programming reqdly=00h disables this pause when loading the<br>palette table. |
| 11-10 | RESERVED | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |

**Table 13-23. RASTER\_CTRL Register Field Descriptions (continued)**

| Bit | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-----|----------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|     |          |      |       |                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 9   | mono8b   | R/W  | 0h    | Mono 8 bit.<br>0 = lcd_pixel_o<br>[3:0] is used to output four pixel values to the panel each pixel clock<br>transition.<br>1 = lcd_pixel_o<br>[7:0] is used to output eight pixel values to the panel each pixel clock<br>transition.<br>This bit is ignored in all other modes.                                                                                                                                                                 |
| 8   | rdorder  | R/W  | 0h    | Raster Data Order Select.<br>0 = The frame buffer parsing for Palette Data lookup is from Bit 0 to<br>bit 31 of the input word from the DMA output.<br>1 = The fame buffer parsing for Palette Data lookup is from Bit 31 to<br>Bit 0 of the input word from the DMA output.<br>This bit has no effect on raw data framebuffers (12/16/24 bpp).<br>This bit is used to determine palette indexing and is used in<br>conjunction with cfg_nibmode. |
| 7   | lcdtft   | R/W  | 0h    | 0 = Passive or display operation enabled, dither logic is enabled.<br>1 = Active or display operation enabled, external palette and DAC<br>required, dither logic bypassed, pin timing changes to support<br>continuous pixel clock, output enable, vsync, and hsync.                                                                                                                                                                             |
| 6-2 | RESERVED | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 1   | lcdbw    | R/W  | 0h    | Only Applies for Passive Matrix Panels LCD Monochrome.<br>0 = Color operation enabled.<br>1 = Monochrome operation enabled.                                                                                                                                                                                                                                                                                                                       |
| 0   | lcden    | R/W  | 0h    | LCD Controller Enable.<br>0 = LCD controller disabled.<br>1 = LCD controller enabled.                                                                                                                                                                                                                                                                                                                                                             |

#### **13.5.1.11 RASTER\_TIMING\_0 Register (offset = 2Ch) [reset = 0h]**

RASTER\_TIMING\_0 is shown in [Figure](#page-146-1) 13-39 and described in Table [13-24](#page-146-2).

**Figure 13-39. RASTER\_TIMING\_0 Register**

| 31 | 30 | 29     | 28     | 27     | 26 | 25       | 24     |
|----|----|--------|--------|--------|----|----------|--------|
|    |    |        |        | hbp    |    |          |        |
|    |    |        |        | R/W-0h |    |          |        |
| 23 | 22 | 21     | 20     | 19     | 18 | 17       | 16     |
|    |    |        |        | hfp    |    |          |        |
|    |    |        |        | R/W-0h |    |          |        |
| 15 | 14 | 13     | 12     | 11     | 10 | 9        | 8      |
|    |    |        | hsw    |        |    |          | ppllsb |
|    |    |        | R/W-0h |        |    |          | R/W-0h |
| 7  | 6  | 5      | 4      | 3      | 2  | 1        | 0      |
|    |    | ppllsb |        | pplmsb |    | RESERVED |        |
|    |    | R/W-0h |        | R/W-0h |    | R-0h     |        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 13-24. RASTER\_TIMING\_0 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|-------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | hbp      | R/W  | 0h    | Horizontal Back Porch Lowbits Bits 7 to 0 of the horizontal back<br>porch field.<br>Encoded value (from<br>1-1024) used to specify the number of pixel clock periods to add to<br>the beginning of a line transmission before the first set of pixels is<br>output to the display (programmed value plus 1).<br>Note that pixel clock is held in its inactive state during the beginning<br>of the line wait period in passive display mode, and is permitted to<br>transition in active display mode. |
| 23-16 | hfp      | R/W  | 0h    | Horizontal Front Porch Lowbits Encoded value (from 1 to 1024) used<br>to specify the number of pixel clock periods to add to the end of a<br>line transmission before line clock is asserted (programmed value<br>plus 1).<br>Note that pixel clock is held in its inactive state during the end of line<br>wait period in passive display mode, and is permitted to transition in<br>active display mode.                                                                                             |
| 15-10 | hsw      | R/W  | 0h    | Horizontal Sync Pulse Width Lowbits Bits 5 to 0 of the horizontal<br>sync pulse width field.<br>Encoded value (from<br>1-1024) used to specify the number of pixel clock periods to pulse<br>the line clock at the end of each line (programmed value plus 1).<br>Note that pixel clock is held in its inactive state during the generation<br>of line clock in passive display mode, and is permitted to transition in<br>active display mode.                                                        |
| 9-4   | ppllsb   | R/W  | 0h    | Pixels-per-line LSB<br>[9:4].<br>Encoded LSB value (from<br>1-1024) used to specify the number of pixels contained within each<br>line on the LCD display (programmed to value minus one).<br>PPL = 11'b{pplmsb, ppllsb, 4'b1111} + 1 ex: pplmsb=1'b1,<br>pppllsb=6'0001000 PPL = 11'b{1, 000100, 1111}+<br>1 = 1104d (decimal) pixels per line.<br>In other words, PPL = 16*({pplmsb, ppllsb}+1).                                                                                                     |
| 3     | pplmsb   | R/W  | 0h    | Pixels-per-line MSB[10].<br>Needed in order to support up to 2048 ppl.                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 2-0   | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |

#### **13.5.1.12 RASTER\_TIMING\_1 Register (offset = 30h) [reset = 0h]**

RASTER\_TIMING\_1 is shown in [Figure](#page-147-1) 13-40 and described in Table [13-25](#page-147-2).

#### **Figure 13-40. RASTER\_TIMING\_1 Register**

| 31 | 30 | 29 | 28     | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20     | 19 | 18 | 17 | 16 | 15 | 14 | 13     | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4      | 3 | 2 | 1 | 0 |
|----|----|----|--------|----|----|----|----|----|----|----|--------|----|----|----|----|----|----|--------|----|----|----|---|---|---|---|---|--------|---|---|---|---|
|    |    |    | vbp    |    |    |    |    |    |    |    | vfp    |    |    |    |    |    |    | vsw    |    |    |    |   |   |   |   |   | lpp    |   |   |   |   |
|    |    |    | R/W-0h |    |    |    |    |    |    |    | R/W-0h |    |    |    |    |    |    | R/W-0h |    |    |    |   |   |   |   |   | R/W-0h |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-25. RASTER\_TIMING\_1 Register Field Descriptions**

| Bit   | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|-------|-------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | vbp   | R/W  | 0h    | Vertical Back Porch Value (from<br>0-255) use to specify the number of line clock periods to add to the<br>beginning of a frame before the first set of pixels is output to the<br>display.<br>Note that line clock transitions during the insertion of the extra line<br>clock periods.                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 23-16 | vfp   | R/W  | 0h    | Vertical Front Porch Value (from<br>0-255) used to specify the number of line clock periods to add to the<br>end of each frame.<br>Note that the line clock transitions during the insertion of the extra<br>line clock periods.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 15-10 | vsw   | R/W  | 0h    | Vertical Sync Width Pulse In active mode (lcdtft=1), encoded value<br>(from<br>1-64) used to specify the number of line clock periods to set the<br>lcd_fp pin active at the end of each frame after the (vfp) period<br>elapses.<br>The number of clock cycles is programmed value plus one.<br>The frame clock is used as the VSYNC signal in active mode.<br>In passive mode (lcdtft=0), encoded value (from<br>1-64) used to specify the number of extra line clock periods to insert<br>after the vertical front porch (vfp) period has elapsed.<br>Note that the width of lcd_fp is not affected by vsw in passive mode<br>and line clock transitions during the insertion of the extra line clock<br>periods (programmed value plus one). |
| 9-0   | lpp   | R/W  | 0h    | Lines Per Panel Encoded value (programmed value range of {<br>0:2047} represents an actual range of {<br>1:2048}) used to specify the number of lines per panel.<br>It represents the total number of lines on the LCD (programmed<br>value plus one).<br>Bit 10 of this field is in RASTER_TIMING_2.                                                                                                                                                                                                                                                                                                                                                                                                                                            |

#### **13.5.1.13 RASTER\_TIMING\_2 Register (offset = 34h) [reset = 0h]**

RASTER\_TIMING\_2 is shown in [Figure](#page-148-1) 13-41 and described in Table [13-26](#page-148-2).

**Figure 13-41. RASTER\_TIMING\_2 Register**

| 31       | 30       | 29     | 28           | 27     | 26       | 25           | 24       |
|----------|----------|--------|--------------|--------|----------|--------------|----------|
| RESERVED |          |        | hsw_highbits |        | lpp_b10  | phsvs_on_off | phsvs_rf |
| R-0h     |          |        | R/W-0h       |        | R/W-0h   | R/W-0h       | R/W-0h   |
| 23       | 22       | 21     | 20           | 19     | 18       | 17           | 16       |
| ieo      | ipc      | ihs    | ivs          |        |          | acbi         |          |
| R/W-0h   | R/W-0h   | R/W-0h | R/W-0h       |        |          | R/W-0h       |          |
| 15       | 14       | 13     | 12           | 11     | 10       | 9            | 8        |
|          |          |        |              | acb    |          |              |          |
|          |          |        |              | R/W-0h |          |              |          |
| 7        | 6        | 5      | 4            | 3      | 2        | 1            | 0        |
|          | RESERVED |        | hbp_highbits |        | RESERVED | hfp_highbits |          |
|          | R-0h     |        | R/W-0h       |        | R-0h     | R/W-0h       |          |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 13-26. RASTER\_TIMING\_2 Register Field Descriptions**

| Bit   | Field        | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
|-------|--------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31    | RESERVED     | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| 30-27 | hsw_highbits | R/W  | 0h    | Bits 9 to 6 of the horizontal sync width field.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| 26    | lpp_b10      | R/W  | 0h    | Lines Per Panel Bit 10.<br>Bit 10 of the lpp field in RASTER_TIMING_1.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 25    | phsvs_on_off | R/W  | 0h    | Hsync/Vsync Pixel Clock Control On/Off.<br>0 = lcd_lp and lcd_fp are driven on opposite edges of pixel clock<br>than the lcd_pixel_o.<br>1 = lcd_lp and lcd_fp are driven according to bit 24.<br>Note that this bit MUST be set to '0' for Passive Matrix displays.<br>The edge timing is fixed.                                                                                                                                                                                                                                                                                                                                                                   |
| 24    | phsvs_rf     | R/W  | 0h    | Program HSYNC/VSYNC Rise or Fall.<br>0 = lcd_lp and lcd_fp are driven on the rising edge of pixel clock (bit<br>25 must be set to 1).<br>1 = lcd_lp and lcd_fp are driven on the falling edge of pixel clock (bit<br>25 must be set to 1).                                                                                                                                                                                                                                                                                                                                                                                                                          |
| 23    | ieo          | R/W  | 0h    | Invert Output Enable.<br>0 = lcd_ac pin is active high in active display mode.<br>1 = lcd_ac pin is active low in active display mode.<br>Active display mode: data driven out of the LCD's data lines on<br>programmed pixel clock edge where AC-bias is active.<br>Note that ieo is ignored in passive display mode.                                                                                                                                                                                                                                                                                                                                              |
| 22    | ipc          | R/W  | 0h    | Invert Pixel Clock.<br>0 = Data is driven on the LCD's data lines on the rising edge of<br>lcd_cp.<br>1 = Data is driven on the LCD's data lines in the falling edge of<br>lcd_cp.<br>For Active Matrix output (cfg_lcdtft='1'), the Output Pixel Clock is a<br>free running clock in that it transitions in horizontal blanking<br>(including horizontal front porch, horizontal back porch) areas and all<br>vertical blanking times.<br>For Passive Matrix output (cfg_lcdtft='0'), the Output Pixel Clock on<br>occurs when an output data value is written.<br>It is in a return-to-zero state when cfg_ipc='0' and a return-to-one<br>state when cfg_ipc='1'. |
| 21    | ihs          | R/W  | 0h    | Invert Hsync.<br>0 = lcd_lp pin is active high and inactive low.<br>1 = lcd_lp pin is active low and inactive high.<br>Active and passive mode: horizontal sync pulse/line clock active<br>between lines, after the end of line wait period                                                                                                                                                                                                                                                                                                                                                                                                                         |

**Table 13-26. RASTER\_TIMING\_2 Register Field Descriptions (continued)**

| Bit   | Field        | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                    |
|-------|--------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 20    | ivs          | R/W  | 0h    | Invert Vsync.<br>0 = lcd_fp pin is active high and inactive low.<br>1 = lcd_fp pin is active low and inactive high.<br>Active mode: vertical sync pulse active between frames, after end of<br>frame wait period.<br>Passive mode: frame clock active during first line of each frame.                                                                         |
| 19-16 | acbi         | R/W  | 0h    | AC Bias Pins Transitions per Interrupt.<br>Value (from 0 to 15) used to specify the number of AC Bias pin<br>transitions to count before setting the line count status (lcs) bit,<br>signaling an interrupt request.<br>Counter frozen when lcd is set, and is restarted when lcs is cleared<br>by software.<br>This function is disabled when acbi = b'0000'. |
| 15-8  | acb          | R/W  | 0h    | AC Bias Pin Frequency.<br>Value (from<br>0-255) used to specify the number of line clocks to count before<br>transitioning the AC Bias pin.<br>This pin is used to periodically invert the polarity of the power supply<br>to prevent DC charge build-up within the display.<br>acb = Number of line clocks/toggle of the lcd_ac pin.                          |
| 7-6   | RESERVED     | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                |
| 5-4   | hbp_highbits | R/W  | 0h    | Bits<br>9:8 of the horizontal back porch field.                                                                                                                                                                                                                                                                                                                |
| 3-2   | RESERVED     | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                |
| 1-0   | hfp_highbits | R/W  | 0h    | Bits<br>9:8 of the horizontal front porch field.                                                                                                                                                                                                                                                                                                               |

#### **13.5.1.14 RASTER\_SUBPANEL Register (offset = 38h) [reset = 0h]**

RASTER\_SUBPANEL is shown in [Figure](#page-150-1) 13-42 and described in Table [13-27.](#page-150-2)

#### **Figure 13-42. RASTER\_SUBPANEL Register**

| 31     | 30       | 29     | 28               | 27     | 26 | 25 | 24 |  |  |
|--------|----------|--------|------------------|--------|----|----|----|--|--|
| spen   | RESERVED | hols   | RESERVED<br>lppt |        |    |    |    |  |  |
| R/W-0h | R-0h     | R/W-0h | R/W-0h<br>R/W-0h |        |    |    |    |  |  |
| 23     | 22       | 21     | 20               | 19     | 18 | 17 | 16 |  |  |
|        | lppt     |        |                  |        |    |    |    |  |  |
| R/W-0h |          |        |                  |        |    |    |    |  |  |
| 15     | 14       | 13     | 12               | 11     | 10 | 9  | 8  |  |  |
|        |          |        |                  | dpdlsb |    |    |    |  |  |
|        |          |        |                  | R/W-0h |    |    |    |  |  |
| 7      | 6        | 5      | 4                | 3      | 2  | 1  | 0  |  |  |
|        |          |        |                  | dpdlsb |    |    |    |  |  |
|        |          |        |                  | R/W-0h |    |    |    |  |  |
|        |          |        |                  |        |    |    |    |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-27. RASTER\_SUBPANEL Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------|----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31    | spen     | R/W  | 0h    | Sub Panel Enable.<br>0 = function disabled.<br>1 = sub-panel function mode enabled.                                                                                                                                                                                                                                                                                                                                                |
| 30    | RESERVED | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 29    | hols     | R/W  | 0h    | High or Low Signal This field indicates the position of the sub-panel<br>based on the LPPT value.<br>0 = Default Pixel Data lines are at the top of the screen and the<br>active video lines are at the bottom of the screen.<br>1 = Active video lines are at the top of the screen and Default Pixel<br>Data lines are at the bottom of the screen.                                                                              |
| 28-26 | RESERVED | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 25-16 | lppt     | R/W  | 0h    | Line Per Panel Threshold Encoded value (programmed value range<br>of {<br>0:2047} represents an actual range of {<br>1:2048}) used to specify the number of lines on the bottom part of<br>the panel.<br>Bit10 of this field is in RASTER_SUBPANEL2.<br>Hols determines whether Default Pixel Data is on the top (hols=''0')<br>or on the bottom (hols='1').<br>Lppt defines the number of lines on the bottom part of the output. |
| 15-0  | dpdlsb   | R/W  | 0h    | Default Pixel Data LSB<br>[15:0] DPD defines the default value of the pixel data sent to the<br>panel for the lines until LPPT is reach or after passing LPPT.                                                                                                                                                                                                                                                                     |

#### **13.5.1.15 RASTER\_SUBPANEL2 Register (offset = 3Ch) [reset = 0h]**

RASTER\_SUBPANEL2 is shown in [Figure](#page-151-1) 13-43 and described in Table [13-28](#page-151-2).

**Figure 13-43. RASTER\_SUBPANEL2 Register**

| 31       | 30       | 29 | 28       | 27     | 26 | 25 | 24       |  |  |  |
|----------|----------|----|----------|--------|----|----|----------|--|--|--|
|          | RESERVED |    |          |        |    |    |          |  |  |  |
|          | R-0h     |    |          |        |    |    |          |  |  |  |
| 23       | 22       | 21 | 20       | 19     | 18 | 17 | 16       |  |  |  |
| RESERVED |          |    |          |        |    |    |          |  |  |  |
|          | R-0h     |    |          |        |    |    |          |  |  |  |
| 15       | 14       | 13 | 12       | 11     | 10 | 9  | 8        |  |  |  |
|          |          |    | RESERVED |        |    |    | lppt_b10 |  |  |  |
|          |          |    | R-0h     |        |    |    | R/W-0h   |  |  |  |
| 7        | 6        | 5  | 4        | 3      | 2  | 1  | 0        |  |  |  |
|          |          |    |          | dpdmsb |    |    |          |  |  |  |
|          |          |    |          | R/W-0h |    |    |          |  |  |  |
|          |          |    |          |        |    |    |          |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 13-28. RASTER\_SUBPANEL2 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                           |
|------|----------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-9 | RESERVED | R    | 0h    |                                                                                                                                                                       |
| 8    | lppt_b10 | R/W  | 0h    | Lines Per Panel Threshold Bit 10 This register is Bit 10 of the lppt<br>field in RASTER_SUBPANEL.                                                                     |
| 7-0  | dpdmsb   | R/W  | 0h    | Default Pixel Data MSB<br>[23:16] DPD defines the default value of the pixel data sent to the<br>panel for the lines until LPPT is reached or after passing the LPPT. |

#### **13.5.1.16 LCDDMA\_CTRL Register (offset = 40h) [reset = 0h]**

LCDDMA\_CTRL is shown in [Figure](#page-152-1) 13-44 and described in Table [13-29.](#page-152-2)

#### **Figure 13-44. LCDDMA\_CTRL Register**

| 31       | 30       | 29         | 28              | 27        | 26            | 25        | 24         |  |  |
|----------|----------|------------|-----------------|-----------|---------------|-----------|------------|--|--|
|          | RESERVED |            |                 |           |               |           |            |  |  |
|          | R/W-0h   |            |                 |           |               |           |            |  |  |
| 23       | 22       | 21         | 20              | 19        | 18            | 17        | 16         |  |  |
|          |          | RESERVED   | dma_master_prio |           |               |           |            |  |  |
|          |          | R/W-0h     |                 |           | R/W-0h        |           |            |  |  |
| 15       | 14       | 13         | 12              | 11        | 10            | 9         | 8          |  |  |
|          |          | RESERVED   |                 |           | th_fifo_ready |           |            |  |  |
|          |          | R/W-0h     |                 |           |               | R/W-0h    |            |  |  |
| 7        | 6        | 5          | 4               | 3         | 2             | 1         | 0          |  |  |
| RESERVED |          | burst_size |                 | byte_swap | RESERVED      | bigendian | frame_mode |  |  |
| R/W-0h   | R/W-0h   |            |                 | R/W-0h    | R-0h          | R/W-0h    | R/W-0h     |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-29. LCDDMA\_CTRL Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
|-------|-----------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-19 | RESERVED        | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 18-16 | dma_master_prio | R/W  | 0h    | Priority for the L3 OCP Master Bus.<br>000b = Highest priority.<br>111b = Lowest priority.                                                                                                                                                                                                                                                                                                                                                                                              |
| 15-11 | RESERVED        | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 10-8  | th_fifo_ready   | R/W  | 0h    | DMA FIFO threshold.<br>The DMA FIFO becomes ready when the number of words specified<br>by this register from the frame buffer have been loaded.<br>000b = 8.<br>001b = 16.<br>010b = 32.<br>011b = 64.<br>100b = 128.<br>101b = 256.<br>110b = 512.<br>111b = Reserved.                                                                                                                                                                                                                |
| 7     | RESERVED        | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 6-4   | burst_size      | R/W  | 0h    | Burst Size setting for DMA transfers (all DMA transfers are 32 bits<br>wide).<br>000b = burst size of 1.<br>001b = burst size of 2.<br>010b = burst size of 4.<br>011b = burst size of 8.<br>100b = burst size of 16.<br>101b = N/A.<br>110b = N/A.<br>111b = N/A.<br>burst_size cannot be changed once the DMA is enabled in LIDD or<br>Raster modes.<br>In this case, the DMA must be disabled and the Done Interrupt must<br>occur before the value in this register can be changed. |
| 3     | byte_swap       | R/W  | 0h    | This bit controls the bytelane ordering of the data on the output of<br>the DMA module.<br>It works in conjunction with the big-endian bit.<br>See the big-endian description for configuration guidelines.                                                                                                                                                                                                                                                                             |
| 2     | RESERVED        | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
|       |                 |      |       |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |

**Table 13-29. LCDDMA\_CTRL Register Field Descriptions (continued)**

| Bit | Field      | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                               |
|-----|------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1   | bigendian  | R/W  | 0h    | Big Endian Enable.<br>Use this bit when the processor is operating in Big Endian mode<br>AND writes to the frame buffer(s) are less than 32 bits wide.<br>Only in this scenario do we need to change the byte alignment for<br>data coming into the FIFO from the frame buffer(s).<br>0 = Big Endian data reordering disabled.<br>1 = Big Endian data reordering enabled. |
| 0   | frame_mode | R/W  | 0h    | Frame Mode.<br>0 = one frame buffer (FB0 only) used.<br>1 = two frame buffers used DMA ping-pongs between FB0 and FB1<br>in this mode.                                                                                                                                                                                                                                    |

#### **13.5.1.17 LCDDMA\_FB0\_BASE Register (offset = 44h) [reset = 0h]**

LCDDMA\_FB0\_BASE is shown in [Figure](#page-154-1) 13-45 and described in Table [13-30](#page-154-2).

#### **Figure 13-45. LCDDMA\_FB0\_BASE Register**

| 31     | 30       | 29       | 28 | 27       | 26 | 25 | 24       |  |  |
|--------|----------|----------|----|----------|----|----|----------|--|--|
|        | fb0_base |          |    |          |    |    |          |  |  |
| R/W-0h |          |          |    |          |    |    |          |  |  |
| 23     | 22       | 21       | 20 | 19       | 18 | 17 | 16       |  |  |
|        |          |          |    | fb0_base |    |    |          |  |  |
| R/W-0h |          |          |    |          |    |    |          |  |  |
| 15     | 14       | 13       | 12 | 11       | 10 | 9  | 8        |  |  |
|        |          |          |    | fb0_base |    |    |          |  |  |
|        |          |          |    | R/W-0h   |    |    |          |  |  |
| 7      | 6        | 5        | 4  | 3        | 2  | 1  | 0        |  |  |
|        |          | fb0_base |    |          |    |    | RESERVED |  |  |
|        |          |          |    | R-0h     |    |    |          |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 13-30. LCDDMA\_FB0\_BASE Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                          |
|------|----------|------|-------|--------------------------------------|
| 31-2 | fb0_base | R/W  | 0h    | Frame Buffer 0 Base Address pointer. |
| 1-0  | RESERVED | R    | 0h    |                                      |

#### **13.5.1.18 LCDDMA\_FB0\_CEILING Register (offset = 48h) [reset = 0h]**

LCDDMA\_FB0\_CEILING is shown in [Figure](#page-155-1) 13-46 and described in Table [13-31.](#page-155-2)

**Figure 13-46. LCDDMA\_FB0\_CEILING Register**

| 31 | 30       | 29 | 28 | 27       | 26 | 25 | 24       |
|----|----------|----|----|----------|----|----|----------|
|    |          |    |    | fb0_ceil |    |    |          |
|    |          |    |    | R/W-0h   |    |    |          |
| 23 | 22       | 21 | 20 | 19       | 18 | 17 | 16       |
|    |          |    |    | fb0_ceil |    |    |          |
|    |          |    |    | R/W-0h   |    |    |          |
| 15 | 14       | 13 | 12 | 11       | 10 | 9  | 8        |
|    |          |    |    | fb0_ceil |    |    |          |
|    |          |    |    | R/W-0h   |    |    |          |
| 7  | 6        | 5  | 4  | 3        | 2  | 1  | 0        |
|    | fb0_ceil |    |    |          |    |    | RESERVED |
|    |          |    |    | R-0h     |    |    |          |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-31. LCDDMA\_FB0\_CEILING Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                             |
|------|----------|------|-------|-----------------------------------------|
| 31-2 | fb0_ceil | R/W  | 0h    | Frame Buffer 0 Ceiling Address pointer. |
| 1-0  | RESERVED | R    | 0h    |                                         |

#### **13.5.1.19 LCDDMA\_FB1\_BASE Register (offset = 4Ch) [reset = 0h]**

LCDDMA\_FB1\_BASE is shown in [Figure](#page-156-1) 13-47 and described in Table [13-32](#page-156-2).

#### **Figure 13-47. LCDDMA\_FB1\_BASE Register**

| 31     | 30 | 29 | 28       | 27       | 26 | 25 | 24       |  |
|--------|----|----|----------|----------|----|----|----------|--|
|        |    |    |          | fb1_base |    |    |          |  |
| R/W-0h |    |    |          |          |    |    |          |  |
| 23     | 22 | 21 | 20       | 19       | 18 | 17 | 16       |  |
|        |    |    |          | fb1_base |    |    |          |  |
| R/W-0h |    |    |          |          |    |    |          |  |
| 15     | 14 | 13 | 12       | 11       | 10 | 9  | 8        |  |
|        |    |    |          | fb1_base |    |    |          |  |
|        |    |    |          | R/W-0h   |    |    |          |  |
| 7      | 6  | 5  | 4        | 3        | 2  | 1  | 0        |  |
|        |    |    | fb1_base |          |    |    | RESERVED |  |
|        |    |    |          | R-0h     |    |    |          |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 13-32. LCDDMA\_FB1\_BASE Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                          |
|------|----------|------|-------|--------------------------------------|
| 31-2 | fb1_base | R/W  | 0h    | Frame Buffer 1 Base Address pointer. |
| 1-0  | RESERVED | R    | 0h    |                                      |

#### **13.5.1.20 LCDDMA\_FB1\_CEILING Register (offset = 50h) [reset = 0h]**

LCDDMA\_FB1\_CEILING is shown in [Figure](#page-157-1) 13-48 and described in Table [13-33.](#page-157-2)

**Figure 13-48. LCDDMA\_FB1\_CEILING Register**

| 31     | 30       | 29 | 28 | 27       | 26 | 25 | 24       |  |  |
|--------|----------|----|----|----------|----|----|----------|--|--|
|        |          |    |    | fb1_ceil |    |    |          |  |  |
| R/W-0h |          |    |    |          |    |    |          |  |  |
| 23     | 22       | 21 | 20 | 19       | 18 | 17 | 16       |  |  |
|        |          |    |    | fb1_ceil |    |    |          |  |  |
|        |          |    |    | R/W-0h   |    |    |          |  |  |
| 15     | 14       | 13 | 12 | 11       | 10 | 9  | 8        |  |  |
|        |          |    |    | fb1_ceil |    |    |          |  |  |
|        |          |    |    | R/W-0h   |    |    |          |  |  |
| 7      | 6        | 5  | 4  | 3        | 2  | 1  | 0        |  |  |
|        | fb1_ceil |    |    |          |    |    | RESERVED |  |  |
| R/W-0h |          |    |    |          |    |    | R-0h     |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-33. LCDDMA\_FB1\_CEILING Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                             |
|------|----------|------|-------|-----------------------------------------|
| 31-2 | fb1_ceil | R/W  | 0h    | Frame Buffer 1 Ceiling Address pointer. |
| 1-0  | RESERVED | R    | 0h    |                                         |

#### **13.5.1.21 SYSCONFIG Register (offset = 54h) [reset = 0h]**

SYSCONFIG is shown in Figure 15-4 and described in Table 15-7.

#### **Figure 13-49. SYSCONFIG Register**

| 31       | 30                      | 29     | 28       | 27       | 26       | 25     | 24 |  |  |  |
|----------|-------------------------|--------|----------|----------|----------|--------|----|--|--|--|
| RESERVED |                         |        |          |          |          |        |    |  |  |  |
| R-0h     |                         |        |          |          |          |        |    |  |  |  |
| 23       | 22                      | 21     | 20       | 19       | 18       | 17     | 16 |  |  |  |
|          | RESERVED                |        |          |          |          |        |    |  |  |  |
| R-0h     |                         |        |          |          |          |        |    |  |  |  |
| 15       | 14                      | 13     | 12       | 11       | 10       | 9      | 8  |  |  |  |
|          |                         |        | RESERVED |          |          |        |    |  |  |  |
|          |                         |        | R-0h     |          |          |        |    |  |  |  |
| 7        | 6                       | 5      | 4        | 3        | 2        | 1      | 0  |  |  |  |
|          | RESERVED<br>standbymode |        |          | idlemode | RESERVED |        |    |  |  |  |
|          | R-0h                    | R/W-0h |          | R/W-0h   |          | R/W-0h |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 13-34. SYSCONFIG Register Field Descriptions**

| Bit  | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
|------|-------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-6 | RESERVED    | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 5-4  | standbymode | R/W  | 0h    | Configuration of the local initiator state management mode.<br>By definition, initiator may generate read/write transaction as long as<br>it is out of STANDBY state.<br>0 = Force-standby mode: local initiator is unconditionally placed in<br>standby state.<br>Backup mode, for debug only.<br>1 = No-standby mode: local initiator is unconditionally placed out of<br>standby state.<br>Backup mode, for debug only.<br>2 = Smart-standby mode: local initiator standby status depends on<br>local conditions, that is, the module's functional requirement from the<br>initiator.<br>IP module shall not generate (initiator-related) wakeup events.<br>3 = Reserved.                                                                          |
| 3-2  | idlemode    | R/W  | 0h    | Configuration of the local target state management mode.<br>By definition, target can handle read/write transaction as long as it is<br>out of IDLE state.<br>0 = Force-idle mode: local target's idle state follows (acknowledges)<br>the system's idle requests unconditionally, i.e.<br>regardless of the IP module's internal requirements.<br>Backup mode, for debug only.<br>1 = No-idle mode: local target never enters idle state.<br>Backup mode, for debug only.<br>2 = Smart-idle mode: local target's idle state eventually follows<br>(acknowledges) the system's idle requests, depending on the IP<br>module's internal requirements.<br>IP module shall not generate (IRQ- or DMA-request-related) wakeup<br>events.<br>3 = Reserved. |
| 1-0  | RESERVED    | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |

#### **13.5.1.22 IRQSTATUS\_RAW Register (offset = 58h) [reset = 0h]**

IRQSTATUS\_RAW is shown in Figure 20-9 and described in Table 20-14.

#### **Figure 13-50. IRQSTATUS\_RAW Register**

| 31       | 30         | 29          | 28       | 27          | 26           | 25                                | 24           |  |  |  |  |
|----------|------------|-------------|----------|-------------|--------------|-----------------------------------|--------------|--|--|--|--|
|          | RESERVED   |             |          |             |              |                                   |              |  |  |  |  |
| R/W-0h   |            |             |          |             |              |                                   |              |  |  |  |  |
| 23       | 22         | 21          | 20       | 19          | 18           | 17                                | 16           |  |  |  |  |
|          | RESERVED   |             |          |             |              |                                   |              |  |  |  |  |
|          | R/W-0h     |             |          |             |              |                                   |              |  |  |  |  |
| 15       | 14         | 13          | 12       | 11          | 10           | 9                                 | 8            |  |  |  |  |
|          |            |             | RESERVED |             |              | eof1_raw_set                      | eof0_raw_set |  |  |  |  |
|          |            |             | R/W-0h   |             |              | R/W-0h                            | R/W-0h       |  |  |  |  |
| 7        | 6          | 5           | 4        | 3           | 2            | 1                                 | 0            |  |  |  |  |
| RESERVED | pl_raw_set | fuf_raw_set | RESERVED | acb_raw_set | sync_raw_set | recurrent_raster<br>_done_raw_set | done_raw_set |  |  |  |  |
| R/W-0h   | R/W-0h     | R/W-0h      | R-0h     | R/W-0h      | R/W-0h       | R/W-0h                            | R/W-0h       |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 13-35. IRQSTATUS\_RAW Register Field Descriptions**

| Bit   | Field        | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                             |  |  |  |
|-------|--------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|--|
| 31-10 | RESERVED     | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                         |  |  |  |
| 9     | eof1_raw_set | R/W  | 0h    | DMA End-of-Frame 1 Raw Interrupt Status and Set Read indicates<br>raw status.<br>0 = Inactive.<br>1 = Active.<br>Writing 1 will set status.<br>Writing 0 has no effect.                                                                                                                                                                                 |  |  |  |
| 8     | eof0_raw_set | R/W  | 0h    | DMA End-of-Frame 0 Raw Interrupt Status and Set Read indicates<br>raw status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will set status.<br>Writing 0 has no effect.                                                                                                                                                                                 |  |  |  |
| 7     | RESERVED     | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                         |  |  |  |
| 6     | pl_raw_set   | R/W  | 0h    | DMA Palette Loaded Raw Interrupt Status and Set Read indicates<br>raw status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will set status.<br>Writing 0 has no effect.                                                                                                                                                                                 |  |  |  |
| 5     | fuf_raw_set  | R/W  | 0h    | DMA FIFO Underflow Raw Interrupt Status and Set LCD dithering<br>logic not supplying data to FIFO at a sufficient rate, FIFO has<br>completely emptied and data pin driver logic has attempted to take<br>added data from FIFO.<br>Read indicates raw status.<br>0 = Inactive.<br>1 = Active.<br>Writing 1 will set status.<br>Writing 0 has no effect. |  |  |  |
| 4     | RESERVED     | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                         |  |  |  |

**Table 13-35. IRQSTATUS\_RAW Register Field Descriptions (continued)**

| Bit | Field                             | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|-----|-----------------------------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 3   | acb_raw_set                       | R/W  | 0h    | For Passive Matrix Panels Only AC Bias Count Raw Interrupt Status<br>and Set AC bias transition counter has decremented to zero,<br>indicating that the lcd_ac_o line has transitioned the number of times<br>which is specified by the acbi control bit-field.<br>The counter is reloaded with the value in acbi but it is disabled until<br>the user clears ABC.<br>Read indicates raw status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will set status.<br>Writing 0 has no effect. |
| 2   | sync_raw_set                      | R/W  | 0h    | Frame Synchronization Lost Raw Interrupt Status and Set Read<br>indicates raw status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will set status.<br>Writing 0 has no effect.                                                                                                                                                                                                                                                                                                            |
| 1   | recurrent_raster_done_ra<br>w_set | R/W  | 0h    | Raster Mode Frame Done Interrupt.<br>Read indicates raw status.<br>0 = Inactive.<br>1 = Active.<br>Writing 1 will set status.<br>Writing 0 has no effect.                                                                                                                                                                                                                                                                                                                                  |
| 0   | done_raw_set                      | R/W  | 0h    | Raster or LIDD Frame Done (shared, depends on whether Raster or<br>LIDD mode enabled).<br>Raw Interrupt Status and Set Read indicates raw status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will set status.<br>Writing 0 has no effect.                                                                                                                                                                                                                                                |

#### **13.5.1.23 IRQSTATUS Register (offset = 5Ch) [reset = 0h]**

IRQSTATUS is shown in Figure 20-10 and described in Table 20-15.

#### **Figure 13-51. IRQSTATUS Register**

| 31       | 30        | 29         | 28       | 27         | 26          | 25                               | 24          |  |  |  |  |
|----------|-----------|------------|----------|------------|-------------|----------------------------------|-------------|--|--|--|--|
|          | RESERVED  |            |          |            |             |                                  |             |  |  |  |  |
|          | R/W-0h    |            |          |            |             |                                  |             |  |  |  |  |
| 23       | 22        | 21         | 20       | 19         | 18          | 17                               | 16          |  |  |  |  |
|          |           |            |          | RESERVED   |             |                                  |             |  |  |  |  |
|          | R/W-0h    |            |          |            |             |                                  |             |  |  |  |  |
| 15       | 14        | 13         | 10       | 9          | 8           |                                  |             |  |  |  |  |
|          |           |            | RESERVED |            |             | eof1_en_clr                      | eof0_en_clr |  |  |  |  |
|          |           |            | R/W-0h   |            |             | R/W-0h                           | R/W-0h      |  |  |  |  |
| 7        | 6         | 5          | 4        | 3          | 2           | 1                                | 0           |  |  |  |  |
| RESERVED | pl_en_clr | fuf_en_clr | RESERVED | acb_en_clr | sync_en_clr | recurrent_raster<br>_done_en_clr | done_en_clr |  |  |  |  |
| R/W-0h   | R/W-0h    | R/W-0h     | R/W-0h   | R/W-0h     | R/W-0h      | R/W-0h                           | R/W-0h      |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 13-36. IRQSTATUS Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                     |
|-------|-------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                 |
| 9     | eof1_en_clr | R/W  | 0h    | DMA End-of-Frame 1 Enabled Interrupt and Clear Read indicates<br>enabled (masked) status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                 |
| 8     | eof0_en_clr | R/W  | 0h    | DMA End-of-Frame 0 Enabled Interrupt and Clear Read indicates<br>enabled (masked) status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                 |
| 7     | RESERVED    | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                 |
| 6     | pl_en_clr   | R/W  | 0h    | DMA Palette Loaded Enabled Interrupt and Clear Read indicates<br>enabled (masked) status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                 |
| 5     | fuf_en_clr  | R/W  | 0h    | DMA FIFO Underflow Enabled Interrupt and Clear LCD dithering<br>logic not supplying data to FIFO at a sufficient rate, FIFO has<br>completely emptied and data pin driver logic has attempted to take<br>added data from FIFO.<br>Read indicates enabled (masked) status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect. |
| 4     | RESERVED    | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                 |

**Table 13-36. IRQSTATUS Register Field Descriptions (continued)**

| Bit | Field                            | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|-----|----------------------------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 3   | acb_en_clr                       | R/W  | 0h    | For Passive Matrix Panels Only.<br>AC Bias Count Enabled Interrupt and Clear AC bias transition<br>counter has decremented to zero, indicating that the lcd_ac_o line<br>has transitioned the number of times which is specified by the acbi<br>control bit-field.<br>The counter is reloaded with the value in acbi but it is disabled until<br>the user clears ABC.<br>Read indicates enabled (masked) status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect. |
| 2   | sync_en_clr                      | R/W  | 0h    | Frame Synchronization Lost Enabled Interrupt and Clear Read<br>indicates enabled status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                                                                                                                                                         |
| 1   | recurrent_raster_done_en<br>_clr | R/W  | 0h    | Raster Frame Done Interrupt.<br>Read indicates enabled status.<br>0 = Inactive.<br>1 = Active.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                                                                                                                                                                                   |
| 0   | done_en_clr                      | R/W  | 0h    | Raster or LIDD Frame Done (shared, depends on whether Raster or<br>LIDD mode enabled) Enabled Interrupt and Clear Read indicates<br>enabled status.<br>0 = inactive.<br>1 = active.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                                                                                              |

#### **13.5.1.24 IRQENABLE\_SET Register (offset = 60h) [reset = 0h]**

IRQENABLE\_SET is shown in Figure 20-11 and described in Table 20-16.

### **Figure 13-52. IRQENABLE\_SET Register**

| 31       | 30        | 29         | 28       | 27         | 26          | 25                               | 24          |  |  |  |  |
|----------|-----------|------------|----------|------------|-------------|----------------------------------|-------------|--|--|--|--|
|          | RESERVED  |            |          |            |             |                                  |             |  |  |  |  |
|          | R/W-0h    |            |          |            |             |                                  |             |  |  |  |  |
| 23       | 22        | 21         | 20       | 19         | 18          | 17                               | 16          |  |  |  |  |
|          | RESERVED  |            |          |            |             |                                  |             |  |  |  |  |
|          | R/W-0h    |            |          |            |             |                                  |             |  |  |  |  |
| 15       | 14        | 13         | 12       | 11         | 10          | 9                                | 8           |  |  |  |  |
|          |           |            | RESERVED |            |             | eof1_en_set                      | eof0_en_set |  |  |  |  |
|          |           |            | R/W-0h   |            |             | R/W-0h                           | R/W-0h      |  |  |  |  |
| 7        | 6         | 5          | 4        | 3          | 2           | 1                                | 0           |  |  |  |  |
| RESERVED | pl_en_set | fuf_en_set | RESERVED | acb_en_set | sync_en_set | recurrent_raster<br>_done_en_set | done_en_set |  |  |  |  |
| R/W-0h   | R/W-0h    | R/W-0h     | R-0h     | R/W-0h     | R/W-0h      | R/W-0h                           | R/W-0h      |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-37. IRQENABLE\_SET Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                           |  |  |
|-------|-------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|
| 31-10 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                       |  |  |
| 9     | eof1_en_set | R/W  | 0h    | DMA End-of-Frame 1 Interrupt Enable Set Read indicates enabled<br>(mask) status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will set interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                 |  |  |
| 8     | eof0_en_set | R/W  | 0h    | DMA End-of-Frame 0 Interrupt Enable Set Read indicates enabled<br>(mask) status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will set interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                 |  |  |
| 7     | RESERVED    | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                       |  |  |
| 6     | pl_en_set   | R/W  | 0h    | DMA Palette Loaded Interrupt Enable Set Read indicates enabled<br>(mask) status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will set interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                 |  |  |
| 5     | fuf_en_set  | R/W  | 0h    | DMA FIFO Underflow Interrupt Enable Set LCD dithering logic not<br>supplying data to FIFO at a sufficient rate, FIFO has completely<br>emptied and data pin driver logic has attempted to take added data<br>from FIFO.<br>Read indicates enabled (mask) status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will set interrupt enable.<br>Writing 0 has no effect. |  |  |
| 4     | RESERVED    | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                       |  |  |

**Table 13-37. IRQENABLE\_SET Register Field Descriptions (continued)**

| Bit | Field                            | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|-----|----------------------------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 3   | acb_en_set                       | R/W  | 0h    | For Passive Matrix Panels Only AC Bias Count Interrupt Enable Set<br>AC bias transition counter has decremented to zero, indicating that<br>the lcd_ac_o line has transitioned the number of times which is<br>specified by the acbi control bit-field.<br>The counter is reloaded with the value in acbi but it is disabled until<br>the user clears ABC.<br>Read indicates enabled (mask) status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will set interrupt enable.<br>Writing 0 has no effect. |
| 2   | sync_en_set                      | R/W  | 0h    | Frame Synchronization Lost Interrupt Enable Set Read indicates<br>enabled (mask) status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will set interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                                                                                                                                            |
| 1   | recurrent_raster_done_en<br>_set | R/W  | 0h    | Raster Done Interrupt Enable Set.<br>Read indicates enabled (mask) status.<br>0 = Disabled.<br>1 = Enabled.<br>Writing 1 will set interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                                                                                                                                                          |
| 0   | done_en_set                      | R/W  | 0h    | Raster or LIDD Frame Done (shared, depends on whether Raster or<br>LIDD mode enabled) Interrupt Enable Set Read indicates enabled<br>(mask) status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will set interrupt enable.                                                                                                                                                                                                                                                                             |

#### **13.5.1.25 IRQENABLE\_CLEAR Register (offset = 64h) [reset = 0h]**

IRQENABLE\_CLEAR is shown in [Figure](#page-165-1) 13-53 and described in Table [13-38](#page-165-2).

#### **Figure 13-53. IRQENABLE\_CLEAR Register**

| 31       | 30        | 29         | 28       | 27         | 26          | 25                               | 24          |  |  |  |  |
|----------|-----------|------------|----------|------------|-------------|----------------------------------|-------------|--|--|--|--|
|          | RESERVED  |            |          |            |             |                                  |             |  |  |  |  |
| R/W-0h   |           |            |          |            |             |                                  |             |  |  |  |  |
| 23       | 22        | 21         | 20       | 19         | 18          | 17                               | 16          |  |  |  |  |
|          |           |            |          | RESERVED   |             |                                  |             |  |  |  |  |
|          | R/W-0h    |            |          |            |             |                                  |             |  |  |  |  |
| 15       | 14        | 13         | 12       | 11         | 10          | 9                                | 8           |  |  |  |  |
|          |           |            | RESERVED |            |             | eof1_en_clr                      | eof0_en_clr |  |  |  |  |
|          |           |            | R/W-0h   |            |             | R/W-0h                           | R/W-0h      |  |  |  |  |
| 7        | 6         | 5          | 4        | 3          | 2           | 1                                | 0           |  |  |  |  |
| RESERVED | pl_en_clr | fuf_en_clr | RESERVED | acb_en_clr | sync_en_clr | recurrent_raster<br>_done_en_clr | done_en_clr |  |  |  |  |
| R/W-0h   | R/W-0h    | R/W-0h     | R/W-0h   | R/W-0h     | R/W-0h      | R/W-0h                           | R/W-0h      |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-38. IRQENABLE\_CLEAR Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                         |
|-------|-------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-10 | RESERVED    | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                     |
| 9     | eof1_en_clr | R/W  | 0h    | DMA End-of-Frame 1 Interrupt Enable Clear.<br>Read indicates enabled status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                 |
| 8     | eof0_en_clr | R/W  | 0h    | DMA End-of-Frame 0 Interrupt Enable Clear.<br>Read indicates enabled status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                 |
| 7     | RESERVED    | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                     |
| 6     | pl_en_clr   | R/W  | 0h    | DMA Palette Loaded Interrupt Enable Clear.<br>Read indicates enabled status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                 |
| 5     | fuf_en_clr  | R/W  | 0h    | DMA FIFO Underflow Interrupt Enable Clear.<br>LCD dithering logic not supplying data to FIFO at a sufficient rate,<br>FIFO has completely emptied and data pin driver logic has<br>attempted to take added data from FIFO.<br>Read indicates enabled status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect. |
| 4     | RESERVED    | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                     |

**Table 13-38. IRQENABLE\_CLEAR Register Field Descriptions (continued)**

| Bit | Field                            | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
|-----|----------------------------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 3   | acb_en_clr                       | R/W  | 0h    | For Passive Matrix Panels Only.<br>AC Bias Count Interrupt Enable Clear AC bias transition counter has<br>decremented to zero, indicating that the lcd_ac_o line has<br>transitioned the number of times which is specified by the acbi<br>control bit-field.<br>The counter is reloaded with the value in acbi but it is disabled until<br>the user clears ABC.<br>Read indicates enabled status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect. |
| 2   | sync_en_clr                      | R/W  | 0h    | Frame Synchronization Lost Interrupt Enable Clear Read indicates<br>enabled status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                                                                                                                                                |
| 1   | recurrent_raster_done_en<br>_clr | R/W  | 0h    | Raster Done Interrupt Enable Clear.<br>Read indicates enabled status.<br>0 = Disabled.<br>1 = Enabled.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                                                                                                                                                              |
| 0   | done_en_clr                      | R/W  | 0h    | Raster or LIDD Frame Done (shared, depends on whether Raster or<br>LIDD mode enabled) Interrupt Enable Clear.<br>Read indicates enabled status.<br>0 = disabled.<br>1 = enabled.<br>Writing 1 will clear interrupt enable.<br>Writing 0 has no effect.                                                                                                                                                                                                                                                    |

#### **13.5.1.26 CLKC\_ENABLE Register (offset = 6Ch) [reset = 0h]**

CLKC\_ENABLE is shown in [Figure](#page-167-1) 13-54 and described in Table [13-39.](#page-167-2)

#### **Figure 13-54. CLKC\_ENABLE Register**

| 31       | 30 | 29       | 28         | 27          | 26          | 25     | 24     |
|----------|----|----------|------------|-------------|-------------|--------|--------|
|          |    |          |            | RESERVED    |             |        |        |
| R-0h     |    |          |            |             |             |        |        |
| 23       | 22 | 21       | 20         | 19          | 18          | 17     | 16     |
|          |    |          |            | RESERVED    |             |        |        |
| R-0h     |    |          |            |             |             |        |        |
| 15       | 14 | 13       | 12         | 11          | 10          | 9      | 8      |
| RESERVED |    |          |            |             |             |        |        |
| R-0h     |    |          |            |             |             |        |        |
| 7        | 6  | 5        | 4          | 3           | 2           | 1      | 0      |
|          |    | RESERVED | dma_clk_en | lidd_clk_en | core_clk_en |        |        |
| R-0h     |    |          |            |             | R/W-0h      | R/W-0h | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 13-39. CLKC\_ENABLE Register Field Descriptions**

| Bit  | Field       | Type | Reset | Description                                                                                                                                                        |
|------|-------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-3 | RESERVED    | R    | 0h    |                                                                                                                                                                    |
| 2    | dma_clk_en  | R/W  | 0h    | Software Clock Enable for the DMA submodule.<br>The DMA submodule runs on the L3 Clock domain.                                                                     |
| 1    | lidd_clk_en | R/W  | 0h    | Software Clock Enable for the LIDD submodule (character displays).<br>The LIDD submodule runs on the System Clock (lcd_clk) domain                                 |
| 0    | core_clk_en | R/W  | 0h    | Software Clock Enable for the Core, which encompasses the Raster<br>Active Matrix and Passive Matrix logic.<br>The Core runs on the System Clock (lcd_clk) domain. |

#### **13.5.1.27 CLKC\_RESET Register (offset = 70h) [reset = 0h]**

CLKC\_RESET is shown in [Figure](#page-168-1) 13-55 and described in Table [13-40.](#page-168-2)

#### **Figure 13-55. CLKC\_RESET Register**

| 31       | 30       | 29 | 28 | 27       | 26      | 25       | 24       |
|----------|----------|----|----|----------|---------|----------|----------|
|          |          |    |    | RESERVED |         |          |          |
| R/W-0h   |          |    |    |          |         |          |          |
| 23       | 22       | 21 | 20 | 19       | 18      | 17       | 16       |
|          |          |    |    | RESERVED |         |          |          |
| R/W-0h   |          |    |    |          |         |          |          |
| 15       | 14       | 13 | 12 | 11       | 10      | 9        | 8        |
| RESERVED |          |    |    |          |         |          |          |
| R/W-0h   |          |    |    |          |         |          |          |
| 7        | 6        | 5  | 4  | 3        | 2       | 1        | 0        |
|          | RESERVED |    |    | main_rst | dma_rst | lidd_rst | core_rst |
| R/W-0h   |          |    |    | R/W-0h   | R/W-0h  | R/W-0h   | R/W-0h   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 13-40. CLKC\_RESET Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                     |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-4 | RESERVED | R/W  | 0h    |                                                                                                                                                 |
| 3    | main_rst | R/W  | 0h    | Software Reset for the entire LCD module.<br>1 = Reset Enable.<br>0 = Reset Disable.                                                            |
| 2    | dma_rst  | R/W  | 0h    | Software Reset for the DMA submodule.<br>1 = Reset Enable.<br>0 = Reset Disable.                                                                |
| 1    | lidd_rst | R/W  | 0h    | Software Reset for the LIDD submodule (character displays).<br>1 = Reset Enable.<br>0 = Reset Disable.                                          |
| 0    | core_rst | R/W  | 0h    | Software Reset for the Core, which encompasses the Raster Active<br>Matrix and Passive Matrix logic.<br>1 = Reset Enable.<br>0 = Reset Disable. |
