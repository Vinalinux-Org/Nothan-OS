# 18 Multimedia Card (MMC)

## Overview

The AM335x device contains three instances of the Multimedia Card (MMC), Secure Digital (SD), and Secure Digital I/O (SDIO) high-speed interface module (MMCHS). The controller provides an interface to MMC, SD memory cards, or SDIO cards.

## Key Features

### MMCHS Features
- Built-in 1024-byte buffer for read or write operations
- Two DMA channels, one interrupt line
- 96-MHz functional clock source input
- Data transfer rates:
  - Up to 384 Mbit/sec (48 MByte/sec) in MMC mode 8-bit data transfer
  - Up to 192 Mbit/sec (24 MByte/sec) in High-Speed SD mode 4-bit data transfer
  - Up to 24 Mbit/sec (3 MByte/sec) in Default SD mode 1-bit data transfer
- Support for SDA 3.0 Part A2 programming model
- Full compliance with:
  - MMC command/response sets (MMC standard specification v4.3)
  - SD command/response sets (SD Physical Layer specification v2.00)
  - SDIO command/response sets (SD part E1 specification v2.00)

### Unsupported Features
- MMC Out-of-band interrupts (MMC_OBI input tied low)
- Master DMA operation (disabled through synthesis parameter)
- Card Supply Control for MMCHS1-2 (signal not pinned out)
- Dual Data Rate (DDR) mode (timing not supported on AM335x)

## System Integration

### Connectivity Attributes
- Power Domain: Peripheral Domain
- Clock Domain: 
  - PD_PER_L4LS_GCLK (OCP)
  - PD_PER_MMC_FCLK (Functional)
  - CLK_32KHZ (Debounce)
- Reset Signals: PER_DOM_RST_N
- Idle/Wakeup: Smart Idle mode
- Interrupts: 1 interrupt per instance to MPU Subsystem (MMCSDxINT)
- DMA Requests: 2 DMA requests per instance to EDMA (SDTXEVTx, SDRXEVTx)

### Clock Signals

| Clock Signal | Max Frequency | Source | Description |
|--------------|---------------|--------|-------------|
| CLK (Interface) | 100 MHz | CORE_CLKOUTM4 / 2 | pd_per_l4ls_gclk from PRCM |
| CLKADPI (Functional) | 96 MHz | PER_CLKOUTM2 / 2 | pd_per_mmc_fclk from PRCM |
| CLK32 (Debounce) | 32.768 KHz | CLK_24 / 732.4219 | clk_32KHz from PRCM |

**Note:** Maximum MMC_CLK signal frequency is 48 MHz.

## Pin Configuration

### MMCHS Pin List

| Pin | Type | Description |
|-----|------|-------------|
| MMCx_CLK | I/O | MMC/SD serial clock output (requires RXACTIVE bit set) |
| MMCx_CMD | I/O | MMC/SD command signal |
| MMCx_DAT0 | I/O | MMC/SD data signal |
| MMCx_DAT1 | I/O | MMC/SD data signal, SDIO interrupt input |
| MMCx_DAT2 | I/O | MMC/SD data signal, SDIO read wait output |
| MMCx_DAT[7:3] | I/O | MMC/SD data signals |
| MMCx_POW | O | MMC/SD power supply control (MMCHS 0 only) |
| MMCx_SDCD | I | SD card detect (from connector) |
| MMCx_SDWP | I | SD write protect (from connector) |
| MMCx_OBI | I | MMC out of band interrupt |

### DAT Line Direction for Data Transfer Modes

| Line | MMC/SD 1-bit | MMC/SD 4-bit | MMC/SD 8-bit | SDIO 1-bit | SDIO 4-bit |
|------|--------------|--------------|--------------|------------|------------|
| DAT0 | I/O | I/O | I/O | I/O | I/O |
| DAT1 | Hi-Z | I/O | I/O | Input (interrupt) | I/O or Input |
| DAT2 | Hi-Z | I/O | I/O | I/O (read wait) | I/O or Output |
| DAT3 | Hi-Z | I/O | I/O | Hi-Z | I/O |
| DAT[7:4] | Hi-Z | Hi-Z | I/O | Hi-Z | Hi-Z |

## Functional Description

### Protocol and Data Format

The bus protocol between the MMC/SD/SDIO host controller and the card is message-based with three main components:

#### Message Components
1. **Command**: Starts an operation, transferred serially from host to card on mmc_cmd line
2. **Response**: Answer to a command, sent from card to host on mmc_cmd line
3. **Data**: Transferred between host and card using DATA lines
4. **Busy**: mmc_dat0 signal maintained low while card is programming data
5. **CRC Status**: CRC result sent by card through mmc_dat0 line during write transfer

### Data Transfer Types

#### Sequential Operation (MMC Only)
- Continuous data stream for 1-bit transfer
- Terminates when stop command follows on mmc_cmd line
- Stream commands supported only by MMC cards

#### Block-Oriented Operation
- Sends data block plus CRC bits
- Available for all card types
- Terminates when stop command follows or block count reached

### Data Format Coding Schemes

#### Command Token Format (48 bits)
- Start bit: 0
- Transmitter bit: 1 (host command)
- Command index: 6 bits (0-63)
- Argument: 32 bits
- CRC checksum: 7 bits
- End bit: 1

#### Response Token Format
- **48-bit Response (R1, R3, R4, R5, R6)**
  - Start bit: 0
  - Transmitter bit: 0 (card response)
  - Content varies by response type
  - CRC checksum: 7 bits
  - End bit: 1

- **136-bit Response (R2)**
  - Extended response for card identification

#### Response Type Configuration

| Response Type | RSP_TYPE | Index Check (CICE) | CRC Check (CCCE) | Name |
|---------------|----------|-------------------|------------------|------|
| No Response | 00 | 0 | 0 | - |
| R2 | 01 | 0 | 1 | R2 |
| R3/R4 | 10 | 0 | 0 | R3, R4 |
| R1/R5/R6/R7 | 10 | 1 | 1 | R1, R6, R5, R7 |
| R1b/R5b | 11 | 1 | 1 | R1b, R5b |

## Power Management

### Operating Modes

#### Normal Mode
- Auto-gating of interface and functional clocks when:
  - AUTOIDLE bit is set to 1
  - No transaction on MMC interface
- Clock gating stops when:
  - Register access occurs through interconnect
  - Wake-up event occurs (SDIO card interrupt)
  - Transaction on MMC/SD/SDIO interface starts

#### Idle Mode
Controlled by SIDLEMODE bit field:
- **Force-idle (0)**: Unconditional acknowledgment to power manager request
- **No-idle (1)**: Ignores power manager request
- **Smart-idle (2)**: Acknowledges based on internal state
- **Reserved (3)**: Not used

### Wake-Up Configuration
Wake-up events can be enabled for:
- SDIO card interrupt (IWE bit)
- Card insertion (INS bit)
- Card removal (REM bit)
- Out-of-band interrupt (OBWE bit)

## Reset Mechanisms

### Hardware Reset
- Global reset action on entire module
- Resets all configuration registers and state machines
- RESETDONE bit indicates completion

### Software Reset
- Triggered by SOFTRESET bit
- Same action as hardware reset except:
  - Debounce logic not affected
  - SD_PSTATE, SD_CAPA, SD_CUR_CAPA registers preserved
- RESETDONE bit indicates completion

### Partial Software Resets
- **SRD bit**: Resets data transfer state machines
- **SRC bit**: Resets command transfer state machines

**Important:** If clock inputs are not present, software reset will not complete.

## Interrupt System

### Interrupt Structure
Each interrupt has three control bits:
1. **Status bit** (SD_STAT): Automatically updated when event occurs
2. **Interrupt enable** (SD_IE): Enables/disables status update
3. **Signal enable** (SD_ISE): Enables/disables interrupt transmission

### Key Interrupt Events

| Event | Flag | Description |
|-------|------|-------------|
| Command Complete | CC | Command execution completed |
| Transfer Complete | TC | Data transfer completed |
| Block Gap Event | BGE | Transaction stopped at block gap |
| DMA Interrupt | DMA | DMA transfer event |
| Buffer Write Ready | BWR | Buffer ready for write data |
| Buffer Read Ready | BRR | Buffer ready for read data |
| Card Interrupt | CIRQ | SDIO card interrupt (special clearing) |
| Card Insertion | CINS | Card inserted |
| Card Removal | CREM | Card removed |

### Error Interrupts

| Error | Flag | Description |
|-------|------|-------------|
| Command Timeout | CTO | No response within 64 clock cycles |
| Command CRC Error | CCRC | CRC7 error in command response |
| Command End Bit Error | CEB | Incorrect end bit in response |
| Command Index Error | CIE | Response index mismatch |
| Data Timeout | DTO | Data timeout error |
| Data CRC Error | DCRC | CRC16 error in data phase |
| Data End Bit Error | DEB | Incorrect end bit in data |
| Auto CMD12 Error | ACE | Auto CMD12 error occurred |
| ADMA Error | ADMAE | ADMA transfer error |
| Card Error | CERR | Card status error |
| Bad Access | BADA | Invalid buffer access |

### Special Interrupt Handling

**CIRQ (Card Interrupt):**
- Cannot be cleared by writing 1
- Must disable CIRQ_ENABLE bit
- Clear interrupt source in SDIO card CCCR register
- Re-enable CIRQ_ENABLE after servicing

**ERRI (Error Interrupt):**
- Automatically set when any error bit (SD_STAT[31:16]) is set
- Automatically cleared when all error bits are cleared
- Cannot be cleared directly

## DMA Operations

### DMA Slave Mode
The controller operates as slave on DMA transactions with two separate requests:
- SDMAWREQN (write request)
- SDMARREQN (read request)

#### DMA Receive Mode
- Request asserted when complete block written to buffer
- Block size specified by BLEN field
- Request deasserted after first word read from buffer
- One request per block
- Buffer never overflows (clock stopped if needed)

#### DMA Transmit Mode
- Request asserted when block space available in buffer
- Request deasserted after first word written to buffer
- One request per block
- Transfer size = BLEN buffer size

### ADMA2 Support
- 32-bit Address ADMA2 mode available
- Controlled by DMA_MnS and DMAS bits
- Descriptor-based transfer mechanism
- Error status available in ADMA Error Status register

## Buffer Management

### Data Buffer Features
- 1024-byte internal buffer
- Prefetch register for faster read access
- Post-write buffer for write operations
- 32-bit SD_DATA register access point
- Double buffering support for blocks ≤ 512 bytes

### Buffer Operating Modes

#### Double Buffering (Block size ≤ MEM_SIZE/2)
- Two data transfers can occur simultaneously
- Ping-pong operation between buffer portions
- Read from one portion while writing to another

#### Single Buffering (Block size > MEM_SIZE/2)
- Only one data transfer at a time
- Entire buffer used as single portion

### Memory Size and Block Length Relationship

| Memory Size | Max Block Length | Double Buffering | Single Buffering |
|-------------|------------------|------------------|------------------|
| 512 bytes | 512 | N/A | BLEN ≤ 512 |
| 1024 bytes | 1024 | BLEN ≤ 512 | 512 < BLEN ≤ 1024 |
| 2048 bytes | 2048 | BLEN ≤ 1024 | 1024 < BLEN ≤ 2048 |
| 4096 bytes | 2048 | BLEN ≤ 2048 | N/A |

### Buffer Access Rules
- **Read access**: Allowed only when BRE bit = 1, else BADA error
- **Write access**: Allowed only when BWE bit = 1, else BADA error
- **Access sequence**: Little endian format required
- **Direction**: DDIR bit must be configured before transfer

## Transfer Control

### Auto CMD12 Feature
- Automatically issues CMD12 after last block
- Enabled by ACEN bit
- For known number of blocks (MMC/SD only)
- Timing specifications for SDR50/SDR104 cards
- Response stored in SD_RSP76 register

### Stop at Block Gap
- Enabled by SBGR bit
- Holds transfer at block boundary
- Allows CMD12 transmission during pause
- Continue with CR bit or suspend/resume sequence

### Transfer Stop Methods

| Transfer Type | Before Boundary | At Boundary End |
|---------------|-----------------|-----------------|
| Single Block | Auto complete | Auto complete |
| Multi-Block (finite) | Send CMD12 or CMD52 | Auto CMD12 or stop at gap |
| Multi-Block (infinite) | Send CMD12 or CMD52 | Set SBGR, send CMD52 |

## Signal Generation and Timing

### Output Signal Generation Modes

#### Falling Edge Generation (Default)
- HSPE bit = 0
- Maximizes hold timings
- CMD and DAT driven on falling edge of MMC clock

#### Rising Edge Generation (High Speed)
- HSPE bit = 1
- Increases setup timings
- Enables higher bus frequency
- Required for SDR transfers
- **Do not use in DDR mode**

### Auto CMD12 Timing Constraints

#### Write Transfer
- Margin (Ncrc): 2 to 8 cycles for SDR50/SDR104
- Auto CMD12 end bit must arrive after CRC status end bit
- Host controller has 18 clock cycle margin

#### Read Transfer
- Gap timing: 4 cycles between successive blocks (UHS cards)
- Host follows left border case timing
- Independent of bus configuration (DDR/standard, 1/4/8-bit)

## Boot Mode Operation

### Boot Mode Using CMD0
Configuration steps:
1. Set BOOT_CF0 = 0
2. Set BOOT_ACK if acknowledge expected
3. Configure MMCHS_BLK with block length and count
4. Set MMCHS_SYSCTL[DTO] for timeout
5. Write correct argument to MMCHS_ARG
6. Start CMD0 transfer with DP=1, DDIR=1, MSBS=1, BCE=1

Sequence:
- Minimum 74 clocks after power stable
- CMD0 sent with boot argument
- Optional boot status received
- Data blocks transferred
- CMD0/reset to exit boot mode
- Minimum 56 clocks before next command

### Boot Mode with CMD Line Held Low
Configuration steps:
1. Set BOOT_CF0 = 1 and BOOT_ACK if acknowledge expected
2. Configure MMCHS_BLK with block length and count
3. Set MMCHS_SYSCTL[DTO] for timeout
4. Write to MMCHS_CMD with DP=1, DDIR=1, MSBS=1, BCE=1

Exit:
- Clear BOOT_CF0 to release CMD line
- Enables card to exit boot state

## Error Detection and Handling

### Command Errors

| Error Type | Detection | Recovery |
|------------|-----------|----------|
| Timeout (CTO) | No response within 64 cycles | Retry or abort |
| CRC Error (CCRC) | CRC7 mismatch | Can indicate bus conflict if with CTO |
| End Bit Error (CEB) | End bit = 0 | Check bus integrity |
| Index Error (CIE) | Response index mismatch | Command protocol error |
| Card Error (CERR) | Error bits in R1/R1b/R5/R6 | Check card status |

### Data Errors

| Error Type | Conditions | Handling |
|------------|-----------|----------|
| Timeout (DTO) | Busy timeout (R1b/R5b) | Software reset required |
| | Busy after write CRC | Use SRD bit |
| | Write CRC status timeout | |
| | Read data timeout | |
| | Boot acknowledge timeout | |
| CRC Error (DCRC) | CRC16 error in data | Retransmit block |
| | Invalid CRC status token | |
| End Bit Error (DEB) | End bit = 0 in data | Check bus integrity |

### Error Status Flags (CC and TC)

| Error | CC Value | TC Value | Notes |
|-------|----------|----------|-------|
| BADA | No dependency | No dependency | Related to register access |
| CERR | 1 | - | CC set upon error |
| DEB | - | 1 | TC set upon error |
| DCRC | - | 1 | TC set upon error |
| DTO | 0 | 0 | Mutually exclusive with TC |
| CIE | 1 | - | CC set upon error |
| CEB | 1 | - | CC set upon error |
| CCRC | 1 or 0 | - | With CTO = no CC (bus conflict) |
| CTO | 0 | - | Mutually exclusive with CC |

## Initialization Procedures

### Global Initialization Requirements

Before using MMC/SD/SDIO module:

1. **PRCM**: Enable module interface and functional clocks
2. **Control Module**: Configure pad muxing
3. **MPU INTC** (optional): Enable interrupts from SD module
4. **EDMA** (optional): Configure DMA channels
5. **Interconnect** (optional): Configure interconnect settings

### MMC/SD/SDIO Controller Initialization

#### Step 1: Enable Clocks
Enable OCP and CLKADPI clocks in PRCM module registers

#### Step 2: Software Reset
1. Set SD_SYSCONFIG[SOFTRESET] = 1
2. Poll SD_SYSSTATUS[RESETDONE] until = 1

#### Step 3: Set Capabilities
Write to SD_CAPA[26:24] and SD_CUR_CAPA[23:0] before driver starts

#### Step 4: Wake-Up Configuration
1. Configure SD_SYSCONFIG[ENAWAKEUP] if necessary
2. Enable wake-up events on SD_HCTL[IWE] (SDIO only)
3. Enable card interrupt SD_IE[CIRQENABLE] if necessary

#### Step 5: Bus Configuration
1. Write SD_HCTL (SDVS, SDBP, DTW) for voltage and data bus width
2. Verify SDBP = 1 (power on successful)
3. Set SD_SYSCTL[ICE] = 1 to enable internal clock
4. Configure SD_SYSCTL[CLKD] for clock divider
5. Poll SD_SYSCTL[ICS] until = 1 (clock stable)
6. Write SD_SYSCONFIG (CLOCKACTIVITY, SIDLEMODE, AUTOIDLE)
7. Write SD_CON for specific transfers (OD, DW8, CEATA)

### Card Identification and Selection

#### Initialization Stream
1. Set SD_CON[INIT] = 1
2. Write 0x00000000 to SD_CMD
3. Poll SD_STAT[CC] until = 1
4. Clear SD_STAT[CC] = 1
5. Set SD_CON[INIT] = 0
6. Wait 1 ms
7. Clear SD_STAT (write 0xFFFFFFFF)
8. Change clock frequency

#### Card Type Detection
1. Send CMD0
2. Send CMD8 (SD 2.0+)
   - If timeout: Not SD 2.0
   - If success: SD 2.0+ card
3. Send CMD5 (SDIO)
   - If timeout: Not SDIO
   - If success: SDIO card
4. Send CMD1 (MMC)
   - If success: MMC card

#### Card Selection
1. Send ACMD41 (SD) or CMD1 (MMC) until not busy
2. Send CMD2 to get CID
3. Send CMD3 to get RCA
4. Send CMD7 to select card

## Register Programming

### Critical Programming Notes

1. **Access Width**: Only 32-bit accesses allowed (16-bit and 8-bit corrupt registers)
2. **SD_DATA Register**: Sequential and contiguous access required
3. **Little Endian**: Least significant byte must be accessed first
4. **Transfer Active**: Some registers cannot be modified during transfers

### Register Map

| Offset | Register | Description |
|--------|----------|-------------|
| 0x110 | SD_SYSCONFIG | System Configuration |
| 0x114 | SD_SYSSTATUS | System Status |
| 0x124 | SD_CSRE | Card Status Response Error |
| 0x128 | SD_SYSTEST | System Test |
| 0x12C | SD_CON | Configuration |
| 0x130 | SD_PWCNT | Power Counter |
| 0x200 | SD_SDMASA | SDMA System Address |
| 0x204 | SD_BLK | Transfer Length Configuration |
| 0x208 | SD_ARG | Command Argument |
| 0x20C | SD_CMD | Command and Transfer Mode |
| 0x210 | SD_RSP10 | Command Response [31:0] |
| 0x214 | SD_RSP32 | Command Response [63:32] |
| 0x218 | SD_RSP54 | Command Response [95:64] |
| 0x21C | SD_RSP76 | Command Response [127:96] |
| 0x220 | SD_DATA | Data Register |
| 0x224 | SD_PSTATE | Present State |
| 0x228 | SD_HCTL | Host Control |
| 0x22C | SD_SYSCTL | System Control |
| 0x230 | SD_STAT | Interrupt Status |
| 0x234 | SD_IE | Interrupt Enable |
| 0x238 | SD_ISE | Interrupt Signal Enable |
| 0x23C | SD_AC12 | Auto CMD12 Error Status |
| 0x240 | SD_CAPA | Capabilities |
| 0x248 | SD_CUR_CAPA | Maximum Current Capabilities |
| 0x250 | SD_FE | Force Event |
| 0x254 | SD_ADMAES | ADMA Error Status |
| 0x258 | SD_ADMASAL | ADMA System Address Low |
| 0x25C | SD_ADMASAH | ADMA System Address High |
| 0x2FC | SD_REV | Versions |

## Detailed Register Descriptions

### SD_SYSCONFIG (Offset 0x110)
System Configuration Register - Controls OCP interface parameters

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-14 | RESERVED | R | 0 | Reserved |
| 13-12 | RESERVED | R | 0 | Reserved |
| 11-10 | RESERVED | R | 0 | Reserved |
| 9-8 | CLOCKACTIVITY | R/W | 0 | Clock activity during wake-up: 0=Both off, 1=Interface on, 2=Functional on, 3=Both on |
| 7-5 | RESERVED | R | 0 | Reserved |
| 4-3 | SIDLEMODE | R/W | 0 | Power management: 0=Force-idle, 1=No-idle, 2=Smart-idle, 3=Reserved |
| 2 | ENAWAKEUP | R/W | 0 | Wake-up capability enable: 0=Disabled, 1=Enabled |
| 1 | SOFTRESET | R/W | 0 | Software reset: Write 1 to trigger reset |
| 0 | AUTOIDLE | R/W | 0 | Internal clock gating: 0=Free-running, 1=Automatic gating |

### SD_SYSSTATUS (Offset 0x114)
System Status Register - Module status information

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-1 | RESERVED | R | 0 | Reserved |
| 0 | RESETDONE | R | 0 | Reset monitoring: 0=Reset ongoing, 1=Reset completed |

### SD_CSRE (Offset 0x124)
Card Status Response Error - Enable card error detection

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | CSRE | R/W | 0 | Card status response error enable bits [31:0] |

### SD_CON (Offset 0x12C)
Configuration Register - Card functional mode and features

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-22 | RESERVED | R | 0 | Reserved |
| 21 | SDMA_LnE | R/W | 0 | DMA request: 0=Edge sensitive, 1=Level sensitive |
| 20 | DMA_MnS | R/W | 0 | DMA mode: 0=Slave, 1=Master (not available) |
| 19 | DDR | R/W | 0 | Dual data rate: 0=Standard, 1=DDR (not supported, always 0) |
| 18 | BOOT_CF0 | R/W | 0 | Boot mode CMD line force: 0=Normal, 1=Force CMD to 0 |
| 17 | BOOT_ACK | R/W | 0 | Boot acknowledge: 0=No ack, 1=Expect boot status |
| 16 | CLKEXTFREE | R/W | 0 | External clock free running: 0=Cut off, 1=Maintain |
| 15 | PADEN | R/W | 0 | PAD power control: 0=Auto, 1=Force active |
| 14-13 | RESERVED | R | 0 | Reserved |
| 12 | CEATA | R/W | 0 | CE-ATA mode: 0=Standard, 1=CE-ATA |
| 11 | CTPL | R/W | 0 | DAT1 power: 0=Disable all, 1=Keep DAT1 for interrupt |
| 10-9 | DVAL | R/W | 0 | Debounce filter: 0=33us, 1=231us, 2=1ms, 3=8.4ms |
| 8 | WPP | R/W | 0 | Write protect polarity: 0=Active high, 1=Active low |
| 7 | CDP | R/W | 0 | Card detect polarity: 0=Active high, 1=Active low |
| 6 | MIT | R/W | 0 | MMC interrupt timeout: 0=Enabled, 1=Disabled |
| 5 | DW8 | R/W | 0 | 8-bit mode: 0=1/4-bit, 1=8-bit |
| 4 | MODE | R/W | 0 | Mode select: 0=Functional, 1=SYSTEST |
| 3 | STR | R/W | 0 | Stream command: 0=Block, 1=Stream |
| 2 | HR | R/W | 0 | Host response: 0=Normal, 1=Generate response |
| 1 | INIT | R/W | 0 | Initialization: 0=Normal, 1=Send init sequence |
| 0 | OD | R/W | 0 | Open drain: 0=Push-pull, 1=Open drain |

### SD_BLK (Offset 0x204)
Block Configuration - Block size and count

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-16 | NBLK | R/W | 0 | Number of blocks: 0=Stop, 1-65535=Block count |
| 15-12 | RESERVED | R | 0 | Reserved |
| 11-0 | BLEN | R/W | 0 | Block length in bytes: 0=No transfer, 1-2048=Size in bytes |

### SD_ARG (Offset 0x208)
Command Argument - 32-bit command argument

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | ARG | R/W | 0 | Command argument bits [31:0] |

### SD_CMD (Offset 0x20C)
Command Register - Command configuration and transfer mode

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-30 | RESERVED | R | 0 | Reserved |
| 29-24 | INDX | R/W | 0 | Command index: 0-63 (CMD0-CMD63) |
| 23-22 | CMD_TYPE | R/W | 0 | Command type: 0=Normal, 1=Suspend, 2=Resume, 3=Abort |
| 21 | DP | R/W | 0 | Data present: 0=No data, 1=Data transfer |
| 20 | CICE | R/W | 0 | Command index check: 0=Disabled, 1=Enabled |
| 19 | CCCE | R/W | 0 | Command CRC check: 0=Disabled, 1=Enabled |
| 18 | RESERVED | R | 0 | Reserved |
| 17-16 | RSP_TYPE | R/W | 0 | Response type: 0=None, 1=136-bit, 2=48-bit, 3=48-bit+busy |
| 15-6 | RESERVED | R | 0 | Reserved |
| 5 | MSBS | R/W | 0 | Multi/Single block: 0=Single, 1=Multiple |
| 4 | DDIR | R/W | 0 | Data direction: 0=Write, 1=Read |
| 3 | RESERVED | R | 0 | Reserved |
| 2 | ACEN | R/W | 0 | Auto CMD12 enable: 0=Disabled, 1=Enabled |
| 1 | BCE | R/W | 0 | Block count enable: 0=Disabled, 1=Enabled |
| 0 | DE | R/W | 0 | DMA enable: 0=Disabled, 1=Enabled |

### SD_RSP10 (Offset 0x210)
Response Register [31:0]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-16 | RSP1 | R | 0 | Response bits [31:16] |
| 15-0 | RSP0 | R | 0 | Response bits [15:0] |

### SD_DATA (Offset 0x220)
Data Buffer Access - 32-bit data register

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | DATA | R/W | 0 | Data register - buffer access point |

**Important:** 
- Read only when BRE=1, write only when BWE=1
- Sequential access required (little endian)
- Buffer size: 1024 bytes (256 x 32-bit words)

### SD_PSTATE (Offset 0x224)
Present State - Controller status (Read-only)

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-25 | RESERVED | R | 0 | Reserved |
| 24 | CLEV | R | 0 | CMD line level: 0=Low, 1=High |
| 23-20 | DLEV | R | 0 | DAT[3:0] line levels |
| 19 | WP | R | 0 | Write protect: Reflects SDWP pin |
| 18 | CDPL | R | 0 | Card detect pin level: Inverse of SDCD |
| 17 | CSS | R | 0 | Card state stable: 0=Debouncing, 1=Stable |
| 16 | CINS | R | 0 | Card inserted: Debounced SDCD value |
| 15-12 | RESERVED | R | 0 | Reserved |
| 11 | BRE | R | 0 | Buffer read enable: 0=No data, 1=Data ready |
| 10 | BWE | R | 0 | Buffer write enable: 0=No space, 1=Space available |
| 9 | RTA | R | 0 | Read transfer active: 0=No transfer, 1=Reading |
| 8 | WTA | R | 0 | Write transfer active: 0=No transfer, 1=Writing |
| 7-3 | RESERVED | R | 0 | Reserved |
| 2 | DLA | R | 0 | DAT line active: 0=Inactive, 1=Active |
| 1 | DATI | R | 0 | Command inhibit DAT: 0=Can issue, 1=Inhibited |
| 0 | CMDI | R | 0 | Command inhibit CMD: 0=Can issue, 1=Inhibited |

### SD_HCTL (Offset 0x228)
Host Control - Power, voltage, and transfer parameters

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-28 | RESERVED | R | 0 | Reserved |
| 27 | OBWE | R/W | 0 | Out-of-band wake enable |
| 26 | REM | R/W | 0 | Card removal wake enable |
| 25 | INS | R/W | 0 | Card insertion wake enable |
| 24 | IWE | R/W | 0 | Card interrupt wake enable |
| 23-20 | RESERVED | R | 0 | Reserved |
| 19 | IBG | R/W | 0 | Interrupt at block gap: 0=Disabled, 1=Enabled |
| 18 | RWC | R/W | 0 | Read wait control: 0=Disabled, 1=Enabled |
| 17 | CR | R/W | 0 | Continue request: Write 1 to restart |
| 16 | SBGR | R/W | 0 | Stop at block gap: 0=Transfer, 1=Stop |
| 15-12 | RESERVED | R | 0 | Reserved |
| 11-9 | SDVS | R/W | 0 | SD bus voltage: 5=1.8V, 6=3.0V, 7=3.3V |
| 8 | SDBP | R/W | 0 | SD bus power: 0=Off, 1=On |
| 7 | CDSS | R/W | 0 | Card detect source: 0=SDCD pin, 1=Test level |
| 6 | CDTL | R/W | 0 | Card detect test level: 0=No card, 1=Card |
| 5 | RESERVED | R | 0 | Reserved |
| 4-3 | DMAS | R/W | 0 | DMA select: 0-1=Reserved, 2=ADMA2, 3=Reserved |
| 2 | HSPE | R/W | 0 | High speed enable: 0=Normal, 1=High speed |
| 1 | DTW | R/W | 0 | Data transfer width: 0=1-bit, 1=4-bit |
| 0 | RESERVED | R | 0 | Reserved |

### SD_SYSCTL (Offset 0x22C)
System Control - Reset, clock, and timeout control

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-27 | RESERVED | R | 0 | Reserved |
| 26 | SRD | R/W | 0 | Software reset DAT line: Write 1, auto-clears |
| 25 | SRC | R/W | 0 | Software reset CMD line: Write 1, auto-clears |
| 24 | SRA | R/W | 0 | Software reset all: Write 1, auto-clears |
| 23-20 | RESERVED | R | 0 | Reserved |
| 19-16 | DTO | R/W | 0 | Data timeout: 0=TCF*2^13, 1=TCF*2^14, ..., 14=TCF*2^27 |
| 15-6 | CLKD | R/W | 0 | Clock divider: 0-1=Bypass, 2-1023=Divide by N |
| 5-3 | RESERVED | R | 0 | Reserved |
| 2 | CEN | R/W | 0 | Clock enable: 0=Disabled, 1=Enabled |
| 1 | ICS | R | 0 | Internal clock stable: 0=Unstable, 1=Stable |
| 0 | ICE | R/W | 0 | Internal clock enable: 0=Stopped, 1=Oscillating |

### SD_STAT (Offset 0x230)
Interrupt Status - Event flags (Write 1 to clear)

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-30 | RESERVED | R | 0 | Reserved |
| 29 | BADA | R/W | 0 | Bad access to data space |
| 28 | CERR | R/W | 0 | Card error |
| 27-26 | RESERVED | R | 0 | Reserved |
| 25 | ADMAE | R/W | 0 | ADMA error |
| 24 | ACE | R/W | 0 | Auto CMD12 error |
| 23 | RESERVED | R | 0 | Reserved |
| 22 | DEB | R/W | 0 | Data end bit error |
| 21 | DCRC | R/W | 0 | Data CRC error |
| 20 | DTO | R/W | 0 | Data timeout error |
| 19 | CIE | R/W | 0 | Command index error |
| 18 | CEB | R/W | 0 | Command end bit error |
| 17 | CCRC | R/W | 0 | Command CRC error |
| 16 | CTO | R/W | 0 | Command timeout error |
| 15 | ERRI | R | 0 | Error interrupt (auto-set when any error bit set) |
| 14-11 | RESERVED | R | 0 | Reserved |
| 10 | BSR | R/W | 0 | Boot status received |
| 9 | OBI | R | 0 | Out-of-band interrupt |
| 8 | CIRQ | R | 0 | Card interrupt (special clearing required) |
| 7 | CREM | R/W | 0 | Card removal |
| 6 | CINS | R/W | 0 | Card insertion |
| 5 | BRR | R/W | 0 | Buffer read ready |
| 4 | BWR | R/W | 0 | Buffer write ready |
| 3 | DMA | R/W | 0 | DMA interrupt |
| 2 | BGE | R/W | 0 | Block gap event |
| 1 | TC | R/W | 0 | Transfer complete |
| 0 | CC | R/W | 0 | Command complete |

### SD_IE (Offset 0x234)
Interrupt Enable - Enable/disable module to set status bits, event-by-event basis.
SD_IE[31:16] = Error Interrupt Status Enable. SD_IE[15:0] = Normal Interrupt Status Enable.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-30 | RESERVED | R | 0 | Reserved |
| 29 | BADA_ENABLE | R/W | 0 | Bad access to data space interrupt enable: 0=Masked, 1=Enabled |
| 28 | CERR_ENABLE | R/W | 0 | Card error interrupt enable: 0=Masked, 1=Enabled |
| 27-26 | RESERVED | R | 0 | Reserved |
| 25 | ADMA_ENABLE | R/W | 0 | ADMA error interrupt enable: 0=Masked, 1=Enabled |
| 24 | ACE_ENABLE | R/W | 0 | Auto CMD12 error interrupt enable: 0=Masked, 1=Enabled |
| 23 | RESERVED | R | 0 | Reserved |
| 22 | DEB_ENABLE | R/W | 0 | Data end bit error interrupt enable: 0=Masked, 1=Enabled |
| 21 | DCRC_ENABLE | R/W | 0 | Data CRC error interrupt enable: 0=Masked, 1=Enabled |
| 20 | DTO_ENABLE | R/W | 0 | Data timeout error interrupt enable: 0=Masked, 1=Enabled |
| 19 | CIE_ENABLE | R/W | 0 | Command index error interrupt enable: 0=Masked, 1=Enabled |
| 18 | CEB_ENABLE | R/W | 0 | Command end bit error interrupt enable: 0=Masked, 1=Enabled |
| 17 | CCRC_ENABLE | R/W | 0 | Command CRC error interrupt enable: 0=Masked, 1=Enabled |
| 16 | CTO_ENABLE | R/W | 0 | Command timeout error interrupt enable: 0=Masked, 1=Enabled |
| 15 | NULL | R | 0 | Fixed to 0. Host driver shall control error interrupts using the Error Interrupt Signal Enable register (SD_ISE). Writes ignored. |
| 14-11 | RESERVED | R | 0 | Reserved |
| 10 | BSR_ENABLE | R/W | 0 | Boot Status interrupt enable. Write ignored when SD_CON[BOOT]=0: 0=Masked, 1=Enabled |
| 9 | OBI_ENABLE | R/W | 0 | Out-of-band interrupt enable. Write ignored when SD_CON[14]=0: 0=Masked, 1=Enabled |
| 8 | CIRQ_ENABLE | R/W | 0 | Card interrupt enable. Clearing this bit also clears corresponding status bit. Must be set to 1 when entering smart idle mode to enable wake-up detection. In 1-bit mode, if interrupt routine does not remove source in SDIO card, status bit reasserts when this bit set to 1 again: 0=Masked, 1=Enabled |
| 7 | CREM_ENABLE | R/W | 0 | Card Removal interrupt enable. Must be set to 1 in smart idle mode for wake-up: 0=Masked, 1=Enabled |
| 6 | CINS_ENABLE | R/W | 0 | Card Insertion interrupt enable. Must be set to 1 in smart idle mode for wake-up: 0=Masked, 1=Enabled |
| 5 | BRR_ENABLE | R/W | 0 | Buffer read ready interrupt enable: 0=Masked, 1=Enabled |
| 4 | BWR_ENABLE | R/W | 0 | Buffer write ready interrupt enable: 0=Masked, 1=Enabled |
| 3 | DMA_ENABLE | R/W | 0 | DMA interrupt enable: 0=Masked, 1=Enabled |
| 2 | BGE_ENABLE | R/W | 0 | Block gap event interrupt enable: 0=Masked, 1=Enabled |
| 1 | TC_ENABLE | R/W | 0 | Transfer completed interrupt enable: 0=Masked, 1=Enabled |
| 0 | CC_ENABLE | R/W | 0 | Command completed interrupt enable: 0=Masked, 1=Enabled |

### SD_ISE (Offset 0x238)
Interrupt Signal Enable - Enable/disable interrupt signal transmission, event-by-event basis.
SD_ISE[31:16] = Error Interrupt Signal Enable. SD_ISE[15:0] = Normal Interrupt Signal Enable.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-30 | RESERVED | R | 0 | Reserved |
| 29 | BADA_SIGEN | R/W | 0 | Bad access to data space interrupt signal enable: 0=Masked, 1=Enabled |
| 28 | CERR_SIGEN | R/W | 0 | Card error interrupt signal enable: 0=Masked, 1=Enabled |
| 27-26 | RESERVED | R | 0 | Reserved |
| 25 | ADMA_SIGEN | R/W | 0 | ADMA error signal enable: 0=Masked, 1=Enabled |
| 24 | ACE_SIGEN | R/W | 0 | Auto CMD12 error signal enable: 0=Masked, 1=Enabled |
| 23 | RESERVED | R | 0 | Reserved |
| 22 | DEB_SIGEN | R/W | 0 | Data end bit error signal enable: 0=Masked, 1=Enabled |
| 21 | DCRC_SIGEN | R/W | 0 | Data CRC error signal enable: 0=Masked, 1=Enabled |
| 20 | DTO_SIGEN | R/W | 0 | Data timeout error signal enable. 0=Masked (host controller provides clock to card until data sent or transfer aborted), 1=Enabled |
| 19 | CIE_SIGEN | R/W | 0 | Command index error signal enable: 0=Masked, 1=Enabled |
| 18 | CEB_SIGEN | R/W | 0 | Command end bit error signal enable: 0=Masked, 1=Enabled |
| 17 | CCRC_SIGEN | R/W | 0 | Command CRC error signal enable: 0=Masked, 1=Enabled |
| 16 | CTO_SIGEN | R/W | 0 | Command timeout error signal enable: 0=Masked, 1=Enabled |
| 15 | NULL | R | 0 | Fixed to 0. Host driver shall control error interrupts using this Error Interrupt Signal Enable register. Writes ignored. |
| 14-11 | RESERVED | R | 0 | Reserved |
| 10 | BSR_SIGEN | R/W | 0 | Boot Status signal enable. Write ignored when SD_CON[BOOT]=0: 0=Masked, 1=Enabled |
| 9 | OBI_SIGEN | R/W | 0 | Out-of-band interrupt signal enable. Write ignored when SD_CON[14]=0: 0=Masked, 1=Enabled |
| 8 | CIRQ_SIGEN | R/W | 0 | Card interrupt signal enable. Clearing this bit also clears corresponding status bit. Must be set to 1 when entering smart idle mode. In 1-bit mode, status bit reasserts if interrupt source not cleared in SDIO card: 0=Masked, 1=Enabled |
| 7 | CREM_SIGEN | R/W | 0 | Card Removal signal enable. Must be set to 1 in smart idle mode: 0=Masked, 1=Enabled |
| 6 | CINS_SIGEN | R/W | 0 | Card Insertion signal enable. Must be set to 1 in smart idle mode: 0=Masked, 1=Enabled |
| 5 | BRR_SIGEN | R/W | 0 | Buffer read ready signal enable: 0=Masked, 1=Enabled |
| 4 | BWR_SIGEN | R/W | 0 | Buffer write ready signal enable: 0=Masked, 1=Enabled |
| 3 | DMA_SIGEN | R/W | 0 | DMA signal enable: 0=Masked, 1=Enabled |
| 2 | BGE_SIGEN | R/W | 0 | Block gap event signal enable: 0=Masked, 1=Enabled |
| 1 | TC_SIGEN | R/W | 0 | Transfer completed signal enable: 0=Masked, 1=Enabled |
| 0 | CC_SIGEN | R/W | 0 | Command completed signal enable: 0=Masked, 1=Enabled |

### SD_PWCNT (Offset 0x130)
Power Counter - Delay between PAD power activation and command transfer

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-16 | RESERVED | R | 0 | Reserved |
| 15-0 | PWRCNT | R/W | 0 | Power counter: delay between PADACTIVE pin assertion and command issued. 0=No additional delay, 1=TCF delay (1 card clock period), 2=TCF*2, ..., 0xFFFE=TCF*65534, 0xFFFF=TCF*65535 |

### SD_SDMASA (Offset 0x200)
SDMA System Address - System memory address for SDMA transfer

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | SDMA_SYSADDR | R | 0 | System memory address for SDMA transfer. When host controller stops SDMA, this register points to next contiguous data position. Accessible only when no transaction executing. Reads during transfer may return invalid value. Host Driver must initialize before starting SDMA. After SDMA stops, next address is readable here. SDMA transfer waits at every boundary specified by Host SDMA Buffer Boundary in Block Size register. Host Controller generates DMA Interrupt to request Host Driver to update. When most upper byte (003h) is written, Host Controller restarts SDMA. ADMA does not use this register. |

### SD_SYSTEST (Offset 0x128)
System Test - Controls signals connected to I/O pins in SYSTEST mode (SD_CON[MODE]=1) for boundary connectivity verification. In SYSTEST mode, write to SD_CMD will NOT start a transfer. Buffer behaves as a stack (push/pop). BLEN and NBLK still needed to generate BWR/BRR interrupts.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-17 | RESERVED | R | 0 | Reserved |
| 16 | OBI | R/W | 0 | Out-of-band interrupt data value: 0=pin driven low, 1=pin driven high |
| 15 | SDCD | R/W | 0 | Card detect input signal data value: 0=pin driven low, 1=pin driven high |
| 14 | SDWP | R/W | 0 | Write protect input signal data value: 0=SDWP driven low, 1=driven high |
| 13 | WAKD | R/W | 0 | Wake request output signal value: W 0=SWAKEUP driven low, W 1=SWAKEUP driven high |
| 12 | SSB | R/W | 0 | Set status bit. Must be cleared before clearing any SD_STAT bit. W 0=Clear SSB (does not clear status bits), W 1=Force all SD_STAT bits to 1 if corresponding SD_ISE bit is set |
| 11 | D7D | R/W | 0 | DAT7 I/O signal value. W in output mode: 0=drive low, 1=drive high. R in input mode: returns DAT7 line value |
| 10 | D6D | R/W | 0 | DAT6 I/O signal value. W in output mode: 0=drive low, 1=drive high. R in input mode: returns DAT6 line value |
| 9 | D5D | R/W | 0 | DAT5 I/O signal value. W in output mode: 0=drive low, 1=drive high. R in input mode: returns DAT5 line value |
| 8 | D4D | R/W | 0 | DAT4 I/O signal value. W in output mode: 0=drive low, 1=drive high. R in input mode: returns DAT4 line value |
| 7 | D3D | R/W | 0 | DAT3 I/O signal value. W in output mode: 0=drive low, 1=drive high. R in input mode: returns DAT3 line value |
| 6 | D2D | R/W | 0 | DAT2 I/O signal value. W in output mode: 0=drive low, 1=drive high. R in input mode: returns DAT2 line value |
| 5 | D1D | R/W | 0 | DAT1 I/O signal value. W in output mode: 0=drive low, 1=drive high. R in input mode: returns DAT1 line value |
| 4 | D0D | R/W | 0 | DAT0 I/O signal value. W in output mode: 0=drive low, 1=drive high. R in input mode: returns DAT0 line value |
| 3 | DDIR | R/W | 0 | Control of DAT[7:0] pins direction: W 0=DAT lines are outputs (host to card), W 1=DAT lines are inputs (card to host) |
| 2 | CDAT | R/W | 0 | CMD I/O signal data value. W in output mode: 0=drive low, 1=drive high. R in input mode: returns CMD line value |
| 1 | CDIR | R/W | 0 | Control of CMD pin direction: W 0=CMD is output (host to card), W 1=CMD is input (card to host) |
| 0 | MCKD | R/W | 0 | MMC clock output signal data value: W 0=output clock driven low, W 1=output clock driven high |

### SD_RSP32 (Offset 0x214)
Command Response [63:32] - Bits [63:32] of command response type R2

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-16 | RSP3 | R | 0 | Command Response [63:48] |
| 15-0 | RSP2 | R | 0 | Command Response [47:32] |

### SD_RSP54 (Offset 0x218)
Command Response [95:64] - Bits [95:64] of command response type R2

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-16 | RSP5 | R | 0 | Command Response [95:80] |
| 15-0 | RSP4 | R | 0 | Command Response [79:64] |

### SD_RSP76 (Offset 0x21C)
Command Response [127:96] - Bits [127:96] of command response type R2. Also stores Auto CMD12 response.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-16 | RSP7 | R | 0 | Command Response [127:112] |
| 15-0 | RSP6 | R | 0 | Command Response [111:96] |


Auto CMD12 Error Status - Auto CMD12 error information

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-8 | RESERVED | R | 0 | Reserved |
| 7 | CNI | R | 0 | Command not issued: 0=Issued, 1=Not issued |
| 6-5 | RESERVED | R | 0 | Reserved |
| 4 | ACIE | R | 0 | Auto CMD12 index error |
| 3 | ACEB | R | 0 | Auto CMD12 end bit error |
| 2 | ACCE | R | 0 | Auto CMD12 CRC error |
| 1 | ACTO | R | 0 | Auto CMD12 timeout error |
| 0 | ACNE | R | 0 | Auto CMD12 not executed |

### SD_CAPA (Offset 0x240)
Capabilities - Host controller capabilities (Can be initialized at boot)

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-29 | RESERVED | R | 0 | Reserved |
| 28 | BUS_64BIT | R/W | 0 | 64-bit system bus: 0=32-bit, 1=64-bit |
| 27 | RESERVED | R | 0 | Reserved |
| 26 | VS18 | R/W | 0 | 1.8V support: 0=Not supported, 1=Supported |
| 25 | VS30 | R/W | 0 | 3.0V support: 0=Not supported, 1=Supported |
| 24 | VS33 | R/W | 0 | 3.3V support: 0=Not supported, 1=Supported |
| 23 | SRS | R | 0 | Suspend/resume support |
| 22 | DS | R | 0 | DMA support: 0=Not supported, 1=Supported |
| 21 | HSS | R | 0 | High speed support: 0=Not supported, 1=Supported |
| 20 | RESERVED | R | 0 | Reserved |
| 19 | AD2S | R | 0 | ADMA2 support: 0=Not supported, 1=Supported |
| 18 | RESERVED | R | 0 | Reserved |
| 17-16 | MBL | R | 0 | Max block length: 0=512, 1=1024, 2=2048 bytes |
| 15-14 | RESERVED | R | 0 | Reserved |
| 13-8 | BCF | R | - | Base clock frequency for SD clock (in MHz) |
| 7 | TCU | R | 0 | Timeout clock unit: 0=kHz, 1=MHz |
| 6 | RESERVED | R | 0 | Reserved |
| 5-0 | TCF | R | 0 | Timeout clock frequency |

### SD_CUR_CAPA (Offset 0x248)
Current Capabilities - Maximum current for each voltage

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-24 | RESERVED | R | 0 | Reserved |
| 23-16 | CUR_1V8 | R/W | 0 | Maximum current for 1.8V |
| 15-8 | CUR_3V0 | R/W | 0 | Maximum current for 3.0V |
| 7-0 | CUR_3V3 | R/W | 0 | Maximum current for 3.3V |

### SD_FE (Offset 0x250)
Force Event - Not a physically implemented register. Writing to this address is reflected in the Error Interrupt Status register (SD_STAT) if the corresponding bit in Error Interrupt Status Enable (SD_IE) is set. Used for testing interrupt handling.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-30 | RESERVED | R | 0 | Reserved |
| 29 | FE_BADA | W | 0 | Force Event: Bad access to data space. 0=No effect, 1=Interrupt forced |
| 28 | FE_CERR | W | 0 | Force Event: Card error. 0=No effect, 1=Interrupt forced |
| 27-26 | RESERVED | R | 0 | Reserved |
| 25 | FE_ADMAE | W | 0 | Force Event: ADMA error. 0=No effect, 1=Interrupt forced |
| 24 | FE_ACE | W | 0 | Force Event: Auto CMD12 error. 0=No effect, 1=Interrupt forced |
| 23 | RESERVED | R | 0 | Reserved |
| 22 | FE_DEB | W | 0 | Force Event: Data End Bit error. 0=No effect, 1=Interrupt forced |
| 21 | FE_DCRC | W | 0 | Force Event: Data CRC error. 0=No effect, 1=Interrupt forced |
| 20 | FE_DTO | W | 0 | Force Event: Data timeout error. 0=No effect, 1=Interrupt forced |
| 19 | FE_CIE | W | 0 | Force Event: Command index error. 0=No effect, 1=Interrupt forced |
| 18 | FE_CEB | W | 0 | Force Event: Command end bit error. 0=No effect, 1=Interrupt forced |
| 17 | FE_CCRC | W | 0 | Force Event: Command CRC error. 0=No effect, 1=Interrupt forced |
| 16 | FE_CTO | W | 0 | Force Event: Command Timeout error. 0=No effect, 1=Interrupt forced |
| 15-8 | RESERVED | R | 0 | Reserved |
| 7 | FE_CNI | W | 0 | Force Event: Command not issued by Auto CMD12 error. 0=No effect, 1=Interrupt forced |
| 6-5 | RESERVED | R | 0 | Reserved |
| 4 | FE_ACIE | W | 0 | Force Event: Auto CMD12 index error. 0=No effect, 1=Interrupt forced |
| 3 | FE_ACEB | W | 0 | Force Event: Auto CMD12 end bit error. 0=No effect, 1=Interrupt forced |
| 2 | FE_ACCE | W | 0 | Force Event: Auto CMD12 CRC error. 0=No effect, 1=Interrupt forced |
| 1 | FE_ACTO | W | 0 | Force Event: Auto CMD12 timeout error. 0=No effect, 1=Interrupt forced |
| 0 | FE_ACNE | W | 0 | Force Event: Auto CMD12 not executed. 0=No effect, 1=Interrupt forced |

### SD_ADMAES (Offset 0x254)
ADMA Error Status - ADMA state when an ADMA Error Interrupt occurred

When an ADMA Error Interrupt occurs, the ADMA Error States field holds the ADMA state and SD_ADMASAL holds the address around the error descriptor. Recovery: ST_STOP = previous location in ADMASAL is error descriptor; ST_FDS = current location is error descriptor; ST_TFR = previous location is error descriptor. Host Controller generates ADMA Error Interrupt when it detects invalid descriptor data (Valid=0) at ST_FDS state.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-3 | RESERVED | R | 0 | Reserved |
| 2 | LME | W | 0 | ADMA Length Mismatch Error: total data length in Descriptor table differs from Block Count x Block Length, or total data length not divisible by block length. 0=No error, 1=Error |
| 1-0 | AES | R/W | 0 | ADMA Error State. 0h=ST_STOP (Stop DMA, previous location is error descriptor), 1h=ST_FDS (Fetch Descriptor, current location is error descriptor), 2h=Never set (not used), 3h=ST_TFR (Transfer Data, next of error descriptor) |

### SD_ADMASAL (Offset 0x258)
ADMA System Address Low - Byte address of executing Descriptor table command (lower 32 bits). At start of ADMA, Host Driver sets start address of Descriptor table here. ADMA2 ignores lower 2 bits (assumes 00b). Host Driver shall program Descriptor Table on 32-bit boundary.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | ADMA_A32B | R/W | 0 | ADMA system address lower 32 bits. ADMA increments this register pointing to next line when fetching a Descriptor line. When ADMA Error Interrupt is generated, this register holds valid Descriptor address depending on ADMA state. |

### SD_ADMASAH (Offset 0x25C)
ADMA System Address High - Upper 32 bits of ADMA system address (for 64-bit address mode)

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-0 | ADMA_A32B | R/W | 0 | ADMA system address upper 32 bits |


Revision - Version information

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-24 | VREV | R | 0x31 | Vendor version: 0x31 = v3.1 |
| 23-16 | SREV | R | 0x01 | Spec version: 0x01 = SD Host Spec v1.0 |
| 15-1 | RESERVED | R | 0 | Reserved |
| 0 | SIS | R | 0 | Slot interrupt status |

### SD_REV (Offset 0x2FC)
Revision - Version information. Reset value: 0x31010000. SD_REV[31:16] = Host Controller Version. SD_REV[15:0] = Slot Interrupt Status.

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 31-24 | VREV | R | 0x31 | Vendor Version Number. Bits[7:4]=major revision, bits[3:0]=minor revision. Example: 10h=v1.0, 21h=v2.1. Reset value=0x31 (v3.1) |
| 23-16 | SREV | R | 0x01 | Specification Version Number. 0x00=SD Host Spec v1.0. Reset value=0x01 |
| 15-1 | RESERVED | R | 0 | Reserved |
| 0 | SIS | R | 0 | Slot Interrupt Status: inverted state of interrupt signal. Deasserted by power-on reset or SD_SYSCTL[24] SRA |

### Important Register Behaviors

#### SD_STAT Special Cases
- **CIRQ**: Cannot be cleared by writing 1; disable CIRQ_ENABLE first
- **ERRI**: Auto-cleared when all error bits cleared
- **Write to clear**: Most status bits cleared by writing 1

#### SD_PSTATE Read-Only Status
All bits are read-only and reflect current controller state:
- Command/data line levels
- Transfer active status
- Buffer ready status
- Card insertion status
- Write protect status

#### SD_CMD Transfer Trigger
Writing to upper byte (CMD index) starts transfer immediately

#### SD_CAPA and SD_CUR_CAPA Initialization
These registers can be written during boot/initialization to match system capabilities. After initialization, host driver should not modify them.

## CE-ATA Support

### Command Completion Signal
For CE-ATA cards (SD_CON[CEATA] = 1):
- Command completion detection enabled with ACEN = 1
- Command completion disable token: CMD line held low 47 cycles + 1
- Generate by: SD_CON[HR] = 1, SD_ARG = 0, SD_CMD = 0x00000000

### Critical Timing Cases
When CCSD sent during last data block:
1. **CCS before CCSD**: CIRQ generated, then CCSD sent, then CC generated
2. **CCS during CCSD**: No CIRQ (no conflict), only CC generated
3. **CCS without CCSD**: Only CIRQ generated

## Best Practices

### Clock Management
1. Stop clock before changing frequency
2. Wait for ICS = 1 before enabling output clock (CEN)
3. Use AUTOIDLE for power optimization
4. Configure CLOCKACTIVITY based on idle requirements

### Error Recovery
1. After timeout: Use SRA or SRD software reset
2. Buffer errors: Check BRE/BWE before SD_DATA access
3. Command errors: Check PSTATE[CMDI] before issuing new command
4. Line conflicts: Use SRC to reset command line, SRD for data line

### Interrupt Handling
1. Check ERRI first for efficient error detection
2. Service CIRQ by disabling interrupt, clearing card source, re-enabling
3. Don't clear interrupt status before servicing source
4. Use both IE and ISE for proper interrupt control

### DMA Configuration
1. Set DE bit with command to trigger initial request
2. Configure block size (BLEN) before transfer
3. Use appropriate DMA mode (slave only for AM335x)
4. Handle one request per block

### Transfer Optimization
1. Use Auto CMD12 for multi-block transfers
2. Enable double buffering for blocks ≤ 512 bytes
3. Use stop at block gap for controlled suspension
4. Configure timeout appropriately for card type

### Power Management
1. Use Smart-idle mode for wake-up capability
2. Enable specific wake-up sources (card interrupt, insertion/removal)
3. Configure ENAWAKEUP before entering idle
4. Ensure CLOCKACTIVITY matches PRCM settings

## Common Use Cases

### High-Speed SD Card Read
```
1. Initialize controller and card
2. Set HSPE = 1 for high-speed mode
3. Configure CLKD for desired frequency (up to 48 MHz)
4. Set BLEN and NBLK for transfer size
5. Enable Auto CMD12 (ACEN = 1)
6. Configure DMA or buffer interrupts
7. Issue CMD18 (multi-block read)
8. Handle BRR interrupts or DMA requests
9. Wait for TC interrupt
```

### SDIO Card Interrupt Handling
```
1. Set CTPL = 1 to maintain DAT1 power
2. Enable CIRQ_ENABLE in SD_IE
3. Enable IWE for wake-up capability
4. Enable CIRQ_SIGEN in SD_ISE
5. Wait for CIRQ interrupt
6. Disable CIRQ_ENABLE
7. Service card interrupt source
8. Re-enable CIRQ_ENABLE
```

### MMC Boot Operation
```
1. Configure BOOT_CF0 and BOOT_ACK
2. Set block length and count
3. Configure timeout
4. Issue boot command (CMD0 or hold CMD low)
5. Wait for boot status (optional)
6. Handle data transfer
7. Exit boot mode (CMD0 or clear BOOT_CF0)
```

## Performance Considerations

### Maximum Throughput Factors
1. Clock frequency (up to 48 MHz)
2. Bus width (1-bit, 4-bit, 8-bit)
3. High-speed mode enable
4. DMA vs CPU transfers
5. Block size optimization
6. Double buffering utilization

### Latency Optimization
1. Use DMA to offload CPU
2. Enable Auto CMD12 for multi-block
3. Minimize interrupt handling overhead
4. Use smart-idle for quick wake-up
5. Pre-configure frequently used settings

### Power Optimization
1. Use AUTOIDLE for automatic clock gating
2. Configure CLOCKACTIVITY appropriately
3. Enter smart-idle when inactive
4. Disable unused wake-up sources
5. Use minimum required clock frequency

## Troubleshooting Guide

### Common Issues

**Transfer timeout:**
- Check card insertion and power
- Verify clock configuration
- Check DTO timeout setting
- Ensure card supports command

**CRC errors:**
- Check signal integrity
- Verify bus termination
- Reduce clock frequency
- Check for electrical noise

**Buffer access errors (BADA):**
- Check BRE/BWE before SD_DATA access
- Verify transfer direction (DDIR)
- Ensure proper buffer management

**Card not detected:**
- Check SDCD pin connection
- Verify debounce settings
- Check card detect polarity (CDP)
- Verify power supply

**Interrupt not firing:**
- Check IE and ISE both enabled
- Verify interrupt routing
- Check interrupt controller configuration
- Ensure event actually occurred

**DMA not working:**
- Verify DE bit set in command
- Check DMA request routing
- Ensure EDMA properly configured
- Verify buffer alignment

## Reference Information

### Key Acronyms
- ACMD: Application-Specific Command
- ADMA: Advanced DMA
- BCE: Block Count Enable
- BLEN: Block Length
- CCS: Command Completion Signal
- CID: Card Identification
- CMD: Command
- CRC: Cyclic Redundancy Check
- DAT: Data
- DDR: Dual Data Rate
- DMA: Direct Memory Access
- DTO: Data Timeout
- EDMA: Enhanced DMA
- MMC: MultiMedia Card
- NBLK: Number of Blocks
- OCP: Open Core Protocol
- PRCM: Power, Reset, and Clock Management
- RCA: Relative Card Address
- SCR: SD Configuration Register
- SD: Secure Digital
- SDIO: Secure Digital Input/Output
- SDHC: Secure Digital High Capacity

### Specification References
- MMC System Specification v4.3
- SD Physical Layer Specification v2.00
- SDIO Card Specification Part E1 v2.00
- SD Host Controller Standard Specification Part A2 v2.00
- CE-ATA Specification

### Register Base Addresses
- MMCHS0: 0x48060000 (supports MMC_POW pin for power switch control)
- MMCHS1: 0x481D8000
- MMCHS2: 0x47810000

**Note:** Register offsets (e.g., 0x110, 0x200...) are relative to the base address of each MMCHS instance.

---

*This reference document is based on AM335x Technical Reference Manual SPRUH73Q. For complete register descriptions and detailed specifications, refer to the official TI documentation.*
