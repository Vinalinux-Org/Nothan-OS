# 7 Memory Subsystem

## Overview

The AM335x Memory Subsystem contains three main components:
- **GPMC (General-Purpose Memory Controller)** - External memory interface
- **EMIF (External Memory Interface)** - DDR2/DDR3/mDDR SDRAM controller
- **ELM (Error Location Module)** - BCH error correction for NAND flash

---

## 1. EMIF (External Memory Interface)

## 1.1 Introduction

The EMIF provides interface to external DDR2, DDR3, and mDDR (LPDDR1) SDRAM devices.

**Key Features:**
- Supports DDR2, DDR3, and LPDDR1 (mDDR) SDRAM
- 16-bit data bus width
- Maximum DDR3 data rate: 400 MHz (800 MT/s)
- Maximum DDR2 data rate: 266 MHz (533 MT/s)
- Self-refresh mode support
- Power management features
- Hardware leveling support (DDR3)
- Programmable timing parameters

## 1.2 Clock Configuration

DDR clock frequency formula:
```
DDR_CLK = (DDR PLL input clock × multiplier) / ((pre-divider + 1) × post-divider)
```

## 1.3 Architecture Components

**Main Components:**
- DDR2/3/mDDR Memory Controller
- Command Macro
- Data Macro (2 macros for 16-bit interface)
- VTP Controller Macro
- DQS Gate IOs

**Controller FIFOs:**
- Command FIFO
- Write Data FIFO
- Return Command FIFO
- Register Read Data FIFO
- SDRAM Read Data FIFO

## 1.4 Address Mapping

The EMIF maps logical addresses to SDRAM row, column, and bank addresses based on configuration fields:

**Configuration Fields:**
- `IBANK` - Number of internal banks (1/2/4/8)
- `EBANK` - Number of chip selects (1/2)
- `PAGESIZE` - Column address bits (8/9/10/11 bits)
- `RSIZE` - Row address bits (9-16 bits)
- `REG_IBANK_POS` - Internal bank position (0-3)
- `REG_EBANK_POS` - External bank position (0-1)

**Address Mapping Examples:**

When `REG_IBANK_POS=0` and `REG_EBANK_POS=0`:
```
[Row Address | Chip Select | Bank Address | Column Address]
```

Maximum interleaving: 16 banks (8 internal × 2 chip selects)

## 1.5 Performance Management

**Command Scheduling:**
- Reorders commands for maximum efficiency
- Maintains data coherency
- Priority-based arbitration
- Class of Service (COS) support

**Refresh Scheduling:**

Three urgency levels:
- **Refresh May** - Backlog count > 0
- **Refresh Release** - Backlog count > 4
- **Refresh Must** - Backlog count > 7 (highest priority)

**Performance Counters:**

Configurable counters to monitor:
- Total SDRAM accesses
- Total activations
- Read/write counts
- FIFO full cycles
- Priority elevations
- Bus utilization

## 1.6 Power Management Modes

**Clock Stop Mode (LPDDR1 only):**
- Stops clocks after idle period
- Clock automatically restarts on access

**Self-Refresh Mode:**
- SDRAM maintains data with internal refresh
- Lowest power state while preserving data
- Programmable entry timer

**Power-Down Mode:**
- Active or precharge power-down
- Clocks continue running
- Faster wake-up than self-refresh

**Deep Power-Down Mode (LPDDR1 only):**
- Data lost
- Requires re-initialization on exit

## 1.7 SDRAM Initialization Sequences

**DDR2 Initialization Steps:**
1. Drive CKE low
2. Wait 16 refresh intervals
3. Issue PRECHARGE all
4. Load mode registers (EMR2, EMR3, EMR1, MR)
5. DLL reset
6. Issue refreshes
7. OCD calibration
8. Enter idle state

**DDR3 Initialization Steps:**
1. Drive CKE low
2. Wait 16 refresh intervals
3. Load mode registers (EMR2, EMR3, EMR1, MR)
4. ZQ calibration (ZQCL)
5. Auto refresh
6. Enter idle state

**LPDDR1 Initialization Steps:**
1. Drive CKE high, issue NOPs
2. Wait 16 refresh intervals
3. PRECHARGE all
4. Issue 2 auto refreshes
5. Load mode registers (MR, EMR)
6. Enter idle state

## 1.8 DDR3 Leveling

**Write Leveling:**
- Aligns DQS with CLK at SDRAM
- Uses special mode in DDR3
- Can be triggered by software

**Read DQS Gate Training:**
- Determines when to enable DQS sampling
- Compensates for board delays

**Read Data Eye Training:**
- Optimizes read data sampling point
- Maximizes timing margins

**Leveling Modes:**
- **Full Leveling** - Complete calibration (triggered by software)
- **Incremental Leveling** - Periodic updates during operation

## 1.9 Key EMIF Registers

#### SDRAM_CONFIG (0x08)
Defines SDRAM type and geometry:
- `SDRAM_TYPE` [31:29] - 0=DDR1, 1=LPDDR1, 2=DDR2, 3=DDR3
- `IBANK_POS` [28:27] - Internal bank position
- `DDR_TERM` [26:24] - Termination resistor value
- `CWL` [17:16] - CAS write latency (DDR3)
- `NARROW_MODE` [15:14] - Data bus width
- `CL` [13:10] - CAS latency
- `ROWSIZE` [9:7] - Number of row bits
- `IBANK` [6:4] - Number of internal banks
- `PAGESIZE` [2:0] - Page size

#### SDRAM_REF_CTRL (0x10)
Controls refresh operation:
- `INITREF_DIS` [31] - Disable init and refresh
- `SRT` [29] - Self-refresh temperature range
- `ASR` [28] - Auto self-refresh (DDR3)
- `PASR` [26:24] - Partial array self-refresh
- `REFRESH_RATE` [15:0] - Refresh interval

#### SDRAM_TIM_1 (0x18)
Primary timing parameters:
- `T_RP` [28:25] - Precharge to activate/refresh
- `T_RCD` [24:21] - Activate to read/write
- `T_WR` [20:17] - Write recovery
- `T_RAS` [16:12] - Activate to precharge
- `T_RC` [11:6] - Activate to activate
- `T_RRD` [5:3] - Activate to activate (different bank)
- `T_WTR` [2:0] - Write to read

#### SDRAM_TIM_2 (0x20)
Secondary timing parameters:
- `T_XP` [30:28] - Power-down exit timing
- `T_XSNR` [24:16] - Self-refresh exit to non-read
- `T_XSRD` [15:6] - Self-refresh exit to read
- `T_RTP` [5:3] - Read to precharge
- `T_CKE` [2:0] - CKE minimum pulse width

#### SDRAM_TIM_3 (0x28)
Additional timing:
- `T_PDLL_UL` [31:28] - PHY DLL unlock time
- `ZQ_ZQCS` [20:15] - ZQ calibration short
- `T_RFC` [12:4] - Refresh to activate/refresh
- `T_RAS_MAX` [3:0] - Maximum activate to precharge

#### DDR_PHY_CTRL_1 (0xE4)
PHY configuration:
- `PHY_ENABLE_DYNAMIC_PWRDN` [20] - Dynamic power down
- `PHY_RST_N` [15] - PHY reset
- `PHY_IDLE_LOCAL_ODT` [13:12] - ODT when idle
- `PHY_RD_LOCAL_ODT` [9:8] - ODT during read
- `READ_LATENCY` [4:0] - Read data latency

#### PWR_MGMT_CTRL (0x38)
Power management:
- `PD_TIM` [15:12] - Power-down timer
- `DPD_EN` [11] - Deep power-down enable
- `LP_MODE` [10:8] - Low power mode (1=Clock Stop, 2=Self-Refresh, 4=Power-Down)
- `SR_TIM` [7:4] - Self-refresh timer
- `CS_TIM` [3:0] - Clock stop timer

---

## 2. DDR PHY Registers

## 2.1 Command Macros (CMD0/1/2)

**CMD_REG_PHY_CTRL_SLAVE_RATIO_0:**
- Controls address/command launch timing
- `CMD_SLAVE_RATIO` [9:0] - Delay in 1/256th clock cycle units
- Default: 0x80 (128/256 = 0.5 cycle)

**CMD_REG_PHY_DLL_LOCK_DIFF_0:**
- DLL lock tolerance
- Default: 0x4
- Do not modify

**CMD_REG_PHY_INVERT_CLKOUT_0:**
- `INVERT_CLK_SEL` [0] - Inverts DRAM clock polarity
  - 0 = Normal
  - 1 = Inverted

## 2.2 Data Macros (DATA0/1)

**DATA_REG_PHY_RD_DQS_SLAVE_RATIO_0:**
- Read DQS timing
- `RD_DQS_SLAVE_RATIO_CS0` [9:0]
- Default: 0x40

**DATA_REG_PHY_WR_DQS_SLAVE_RATIO_0:**
- Write DQS timing
- `WR_DQS_SLAVE_RATIO_CS0` [9:0]

**DATA_REG_PHY_WRLVL_INIT_RATIO_0:**
- Write leveling initial value
- Used when `WRLVL_INIT_MODE_SEL` = 1

**DATA_REG_PHY_GATELVL_INIT_RATIO_0:**
- Gate training initial value
- Used when `GATELVL_INIT_MODE_SEL` = 1

**DATA_REG_PHY_FIFO_WE_SLAVE_RATIO_0:**
- DQS gate timing
- `FIFO_WE_SLAVE_RATIO_CS0` [9:0]

**DATA_REG_PHY_WR_DATA_SLAVE_RATIO_0:**
- Write data timing relative to DQS
- Default: 0x40

**DATA_REG_PHY_DQ_OFFSET_0:**
- DQ offset from DQS
- Default: 0x40 (90 degree shift)

**DATA_REG_PHY_USE_RANK0_DELAYS:**
- `PHY_USE_RANK0_DELAYS_0` [0]
  - 0 = Each rank uses own delays (DDR3)
  - 1 = All ranks use rank 0 delays (DDR2/mDDR)

**DATA_REG_PHY_DLL_LOCK_DIFF_0:**
- DLL lock tolerance
- Default: 0x4

---

## 3. ELM (Error Location Module)

## 3.1 Introduction

The ELM performs BCH (Bose-Chaudhuri-Hocquenghem) error location for NAND flash error correction. It works in conjunction with the GPMC which computes syndrome polynomials.

**Key Features:**
- 4-bit, 8-bit, and 16-bit error correction
- 8 simultaneous processing contexts
- Page mode and continuous mode operation
- Automatic interrupt generation

## 3.2 Operation Modes

**Continuous Mode:**
- Each syndrome processed independently
- Interrupt per syndrome completion
- Results retrieved individually

**Page Mode:**
- Multiple syndromes grouped as a page
- Single interrupt when all complete
- Must process entire page before next

## 3.3 Processing Flow

**Initialization:**
1. Set error correction level (`ECC_BCH_LEVEL`)
2. Set buffer size (`ECC_SIZE`)
3. Configure mode (page or continuous)
4. Enable appropriate interrupts

**Processing:**
1. Write syndrome fragments 0-5 in any order
2. Write syndrome fragment 6 last (sets `SYNDROME_VALID`)
3. ELM processes automatically
4. Interrupt generated on completion
5. Read results from status and location registers

**Error Location:**
- Errors reported as bit positions in buffer
- First error in `ERROR_LOCATION_0`
- Second error in `ERROR_LOCATION_1`
- Up to 16 error locations supported

## 3.4 Key ELM Registers

#### ELM_SYSCONFIG (0x10)
- `SOFTRESET` [1] - Software reset
- `SIDLEMODE` [4:3] - Idle mode (0=Force, 1=No, 2=Smart)
- `AUTOGATING` [0] - Clock gating enable

#### ELM_LOCATION_CONFIG (0x20)
- `ECC_SIZE` [26:16] - Buffer size in nibbles (4-bit units)
- `ECC_BCH_LEVEL` [1:0] - Error correction level
  - 0 = 4-bit
  - 1 = 8-bit
  - 2 = 16-bit

#### ELM_PAGE_CTRL (0x80)
- `SECTOR_[7:0]` - Defines which syndromes are in page
- All 0 = Continuous mode
- Any 1 = Page mode

#### ELM_IRQSTATUS (0x18)
- `PAGE_VALID` [8] - Full page complete (page mode)
- `LOC_VALID_[7:0]` - Individual syndrome complete

#### ELM_IRQENABLE (0x1C)
- `PAGE_MASK` [8] - Enable page interrupt
- `LOCATION_MASK_[7:0]` - Enable syndrome interrupts

#### ELM_SYNDROME_FRAGMENT_[0-6]_i (0x400+)
Syndrome polynomial input (i = 0 to 7):
- Fragments 0-5: 32 bits each
- Fragment 6: Includes `SYNDROME_VALID` [16] bit

#### ELM_LOCATION_STATUS_i (0x800+)
Processing results (i = 0 to 7):
- `ECC_CORRECTABLE` [8] - 1 = success, 0 = too many errors
- `ECC_NB_ERRORS` [4:0] - Number of errors found

#### ELM_ERROR_LOCATION_[0-15]_i (0x880+)
Error bit positions (i = 0 to 7):
- `ECC_ERROR_LOCATION` [12:0] - Bit address in buffer
- Read only first `ECC_NB_ERRORS` registers

## 3.5 BCH Algorithm

**Syndrome Polynomial:**
- Computed by GPMC during NAND read
- Represents error signature for 512-byte block
- Non-zero syndrome indicates errors present

**Error Location:**
- ELM solves syndrome equations
- Outputs bit positions of errors
- Software must flip indicated bits to correct data

**Buffer Addressing:**
For 16-bit NAND interface, bit positions are mapped:
- First word at address 0 contains bits 4223-4208
- Within word: bits ordered [7:0], then [15:8]
- Must map error location to actual buffer position

## 3.6 Use Case Example

**8-bit Correction, Continuous Mode:**

Configuration:
- Buffer size: 528 bytes (512 data + 16 spare)
- Correction level: 8-bit
- Mode: Continuous

Syndrome polynomial (from GPMC):
```
P = 0x0A16ABE115E44F767BFB0D0980
```

Results:
- 4 errors detected
- Locations: 431, 1062, 1909, 3452 (decimal bit addresses)

Software must:
1. Map bit addresses to byte/bit positions
2. Read affected bytes
3. Flip error bits
4. Write corrected data

---

## 4. Programming Guidelines

## 4.1 EMIF Configuration Sequence

**Basic DDR3 Setup:**

1. Configure PRCM to enable EMIF clock
2. Configure DDR PHY control registers
3. Set timing parameters (`SDRAM_TIM_1/2/3`)
4. Set refresh rate (`SDRAM_REF_CTRL`)
5. Configure `SDRAM_CONFIG` (triggers initialization)
6. Wait for PHY ready (`STATUS.PHY_DLL_READY`)
7. Perform leveling if required
8. Enable power management if desired

**DDR3 Timing Calculation Example:**

For DDR3-1600 (800 MHz, 1.25ns cycle):
```
T_RP = ceil(13.75ns / 1.25ns) - 1 = 10
T_RCD = ceil(13.75ns / 1.25ns) - 1 = 10
T_WR = ceil(15ns / 1.25ns) - 1 = 11
T_RAS = ceil(35ns / 1.25ns) - 1 = 27
T_RC = ceil(48.75ns / 1.25ns) - 1 = 38
T_RRD = ceil(7.5ns / 1.25ns) - 1 = 5
T_WTR = ceil(7.5ns / 1.25ns) - 1 = 5
```

## 4.2 DDR PHY Configuration

**Typical Values for DDR3-800:**

Command Macros:
```
CMD_SLAVE_RATIO = 0x80 (default)
```

Data Macros:
```
RD_DQS_SLAVE_RATIO = 0x40
WR_DATA_SLAVE_RATIO = 0x80
FIFO_WE_SLAVE_RATIO = determined by leveling
```

**Leveling Procedure:**

1. Enable leveling: `RDWRLVL_EN = 1`
2. Trigger full leveling: `RDWRLVLFULL_START = 1`
3. Wait for completion
4. Read results from PHY registers
5. Enable incremental leveling for runtime tracking

## 4.3 ELM Usage Pattern

**Continuous Mode:**

```
1. Reset ELM
2. Set ECC_BCH_LEVEL and ECC_SIZE
3. Clear ELM_PAGE_CTRL (all 0)
4. Enable LOCATION_MASK interrupts
5. For each syndrome:
   - Write SYNDROME_FRAGMENT_0-5
   - Write SYNDROME_FRAGMENT_6 (sets VALID)
   - Wait for LOC_VALID_i interrupt
   - Read LOCATION_STATUS_i
   - If correctable, read ERROR_LOCATION_*_i
   - Clear LOC_VALID_i interrupt
```

**Page Mode:**

```
1. Reset ELM
2. Set ECC_BCH_LEVEL and ECC_SIZE
3. Set ELM_PAGE_CTRL for desired sectors
4. Enable PAGE_MASK interrupt only
5. Write all syndrome polynomials
6. Wait for PAGE_VALID interrupt
7. Read all LOCATION_STATUS and ERROR_LOCATION
8. Clear all interrupt bits with single write
```

## 4.4 Common Pitfalls

**EMIF:**
- Not waiting for PHY ready before access
- Incorrect timing calculations for clock frequency
- Forgetting to enable module clocks in PRCM
- Using wrong IBANK_POS/EBANK_POS for topology

**ELM:**
- Reading error locations before checking ECC_CORRECTABLE
- Not clearing syndrome VALID bit before re-use
- Reading more ERROR_LOCATION registers than ECC_NB_ERRORS
- Mixing page and continuous mode settings

---

## 5. Integration Notes

## 5.1 Clock Dependencies

**EMIF:**
- Functional clock: DDR PLL output
- Interface clock: L3 clock
- Both must be enabled in PRCM

**ELM:**
- Single clock domain: L4_PER
- Maximum frequency: 100 MHz

## 5.2 Interrupt Routing

**EMIF:**
- Single interrupt line to MPU
- Must check IRQSTATUS to determine source

**ELM:**
- Single interrupt line to MPU  
- Multiple sources flagged in IRQSTATUS

## 5.3 Power Domains

Both EMIF and ELM are in peripheral power domain:
- Can be powered off to save power
- Must save/restore context if powering off
- EMIF supports save/restore mode for retention

## 5.4 Boot Considerations

**ROM Code:**
- May configure EMIF for boot from external memory
- Check existing configuration before modifying
- Some registers may be locked after boot

**U-Boot/Bootloader:**
- Typically reconfigures EMIF for optimal performance
- May enable leveling and power management
- Consider compatibility when making changes

---

## 6. Performance Optimization

## 6.1 EMIF Optimization

**Bank Interleaving:**
- Use IBANK_POS=0 for maximum interleaving
- Allows up to 16 banks open simultaneously
- Best for random access patterns

**Command Scheduling:**
- Use Class of Service to prioritize traffic
- Set appropriate COS_COUNT values
- Avoid starvation with PR_OLD_COUNT

**Read/Write Balance:**
- Adjust RD_THRSH and WR_THRSH
- Lower values = more frequent switching
- Higher values = better burst efficiency

**Power vs Performance:**
- Self-refresh has highest latency
- Power-down medium latency
- Clock stop lowest latency (LPDDR1)

## 6.2 PHY Tuning

**Read Latency:**
- Use minimum value: CL + 2
- Reduces read latency
- Must be at least 2 cycles more than CL

**ODT Settings:**
- Match to board impedance
- Full/half Thevenin termination options
- Dynamic power-down saves power

**Leveling Intervals:**
- Faster incremental leveling = better tracking
- More frequent = higher overhead
- Balance based on temperature variation

---

## 7. Register Quick Reference

### EMIF Base Address: 0x4C00_0000

| Offset | Register | Key Fields |
|--------|----------|-----------|
| 0x08 | SDRAM_CONFIG | SDRAM_TYPE, CL, ROWSIZE, IBANK, PAGESIZE |
| 0x10 | SDRAM_REF_CTRL | REFRESH_RATE, PASR, SRT |
| 0x18 | SDRAM_TIM_1 | T_RP, T_RCD, T_WR, T_RAS, T_RC |
| 0x20 | SDRAM_TIM_2 | T_XP, T_XSNR, T_XSRD, T_RTP, T_CKE |
| 0x28 | SDRAM_TIM_3 | T_RFC, ZQ_ZQCS |
| 0x38 | PWR_MGMT_CTRL | LP_MODE, SR_TIM, PD_TIM |
| 0xE4 | DDR_PHY_CTRL_1 | READ_LATENCY, PHY_RD_LOCAL_ODT |
| 0xDC | RDWR_LVL_CTRL | Leveling control |

### DDR PHY Base Address: 0x44E1_2000

| Offset | Register | Purpose |
|--------|----------|---------|
| 0x01C | CMD0_SLAVE_RATIO | Command 0 timing |
| 0x050 | CMD1_SLAVE_RATIO | Command 1 timing |
| 0x084 | CMD2_SLAVE_RATIO | Command 2 timing |
| 0x0C8 | DATA0_RD_DQS_RATIO | Data 0 read timing |
| 0x0DC | DATA0_WR_DQS_RATIO | Data 0 write DQS |
| 0x108 | DATA0_FIFO_WE_RATIO | Data 0 gate timing |
| 0x120 | DATA0_WR_DATA_RATIO | Data 0 write data |
| 0x16C | DATA1_RD_DQS_RATIO | Data 1 read timing |
| 0x1AC | DATA1_FIFO_WE_RATIO | Data 1 gate timing |

### ELM Base Address: 0x4808_0000

| Offset | Register | Purpose |
|--------|----------|---------|
| 0x10 | ELM_SYSCONFIG | Module configuration |
| 0x18 | ELM_IRQSTATUS | Interrupt status |
| 0x1C | ELM_IRQENABLE | Interrupt enable |
| 0x20 | ELM_LOCATION_CONFIG | ECC level, buffer size |
| 0x80 | ELM_PAGE_CTRL | Page mode definition |
| 0x400+ | SYNDROME_FRAGMENT | Input syndromes (8 sets) |
| 0x800+ | LOCATION_STATUS | Results (8 sets) |
| 0x880+ | ERROR_LOCATION | Error positions (8×16) |

---

## 8. Debugging Tips

## 8.1 EMIF Issues

**No Memory Access:**
- Check PRCM clock enables
- Verify PHY_DLL_READY status
- Confirm correct SDRAM_TYPE setting
- Check pin muxing for DDR signals

**Data Corruption:**
- Verify timing parameters for speed grade
- Check board trace lengths and impedance
- Run leveling procedure
- Verify VTP calibration

**Performance Problems:**
- Check performance counters
- Look for FIFO full conditions
- Verify command scheduling settings
- Check for command starvation

## 8.2 ELM Issues

**No Interrupt:**
- Verify SYNDROME_VALID written last
- Check interrupt enable settings
- Confirm mode (page vs continuous)
- Ensure syndrome is non-zero

**Uncorrectable Errors:**
- Check ECC_BCH_LEVEL matches GPMC
- Verify ECC_SIZE is correct
- Confirm syndrome polynomial format
- Check for hardware errors in NAND

**Wrong Error Locations:**
- Verify bit position to address mapping
- Check byte/word ordering
- Confirm buffer size calculation
- Validate against known error patterns

---

## Appendix A: Timing Parameter Ranges

### DDR3 AC Timing Parameters (ns)

| Parameter | -800E | -1066F | -1333H | -1600K |
|-----------|-------|--------|--------|--------|
| tCK | 2.5 | 1.875 | 1.5 | 1.25 |
| tRCD | 13.75 | 13.125 | 13.5 | 13.75 |
| tRP | 13.75 | 13.125 | 13.5 | 13.75 |
| tRAS | 37.5 | 36 | 36 | 35 |
| tRC | 50 | 49.125 | 49.5 | 48.75 |
| tRRD | 10 | 7.5 | 6 | 7.5 |
| tWR | 15 | 15 | 15 | 15 |
| tWTR | 7.5 | 7.5 | 7.5 | 7.5 |
| tRTP | 7.5 | 7.5 | 7.5 | 7.5 |
| tFAW | 50 | 50 | 45 | 40 |
| tRFC | varies by density |

### DDR2 AC Timing Parameters (ns)

| Parameter | -533 | -667 | -800 |
|-----------|------|------|------|
| tCK | 3.75 | 3.0 | 2.5 |
| tRCD | 15 | 15 | 15 |
| tRP | 15 | 15 | 15 |
| tRAS | 40 | 40 | 45 |
| tRC | 55 | 55 | 60 |
| tRRD | 10 | 10 | 10 |
| tWR | 15 | 15 | 15 |
| tWTR | 7.5 | 7.5 | 10 |
| tRTP | 7.5 | 7.5 | 7.5 |

---

## Appendix B: Common Configurations

### Configuration 1: DDR3-1600, 4Gb (512MB), x16

```
SDRAM_CONFIG:
  SDRAM_TYPE = 3 (DDR3)
  IBANK = 3 (8 banks)
  PAGESIZE = 2 (10 column bits, 1KB page)
  ROWSIZE = 6 (15 row bits)
  NARROW_MODE = 1 (16-bit)
  CL = 6 (CL=7, encoded as 0110b)
  CWL = 1 (CWL=6)

Timing @ 400MHz (2.5ns):
  T_RP = 10, T_RCD = 10, T_WR = 11
  T_RAS = 27, T_RC = 38, T_RRD = 5
  T_WTR = 5, T_RTP = 2

Refresh:
  REFRESH_RATE = 0x61A (7.8us refresh @ 400MHz)
```

### Configuration 2: DDR2-800, 2Gb (256MB), x16

```
SDRAM_CONFIG:
  SDRAM_TYPE = 2 (DDR2)
  IBANK = 2 (4 banks)
  PAGESIZE = 2 (10 column bits)
  ROWSIZE = 4 (13 row bits)
  NARROW_MODE = 1 (16-bit)
  CL = 4 (CL=4)

Timing @ 266MHz (3.75ns):
  T_RP = 3, T_RCD = 3, T_WR = 3
  T_RAS = 10, T_RC = 14, T_RRD = 2
  T_WTR = 1, T_RTP = 1

Refresh:
  REFRESH_RATE = 0x50F (7.8us refresh @ 266MHz)
```

---

## Section 9: Complete ELM Register Specifications

## 9.1 ELM Register Map Overview

The ELM module contains 8 processing contexts (0-7), each with identical register structures for syndrome input and error location output.

**Base Address:** 0x4808_0000

**Register Organization:**
- Configuration registers: 0x000 - 0x0FF
- Syndrome input registers: 0x400 - 0x5FF (8 contexts × 7 fragments each)
- Status registers: 0x800 - 0xF00 (8 contexts)
- Error location output registers: 0x880 - 0xFBC (8 contexts × 16 locations each)

## 9.2 Syndrome Fragment Registers (All Contexts)

Each of the 8 processing contexts has 7 syndrome fragment registers to input the complete 208-bit syndrome polynomial.

**Context 0 Syndrome Fragments:**
- `ELM_SYNDROME_FRAGMENT_0_0` @ 0x400: Bits [31:0]
- `ELM_SYNDROME_FRAGMENT_1_0` @ 0x404: Bits [63:32]
- `ELM_SYNDROME_FRAGMENT_2_0` @ 0x408: Bits [95:64]
- `ELM_SYNDROME_FRAGMENT_3_0` @ 0x40C: Bits [127:96]
- `ELM_SYNDROME_FRAGMENT_4_0` @ 0x410: Bits [159:128]
- `ELM_SYNDROME_FRAGMENT_5_0` @ 0x414: Bits [191:160]
- `ELM_SYNDROME_FRAGMENT_6_0` @ 0x418: Bits [207:192] + SYNDROME_VALID

**Context 1-7 Follow Same Pattern:**
- Context 1: Base offset 0x440
- Context 2: Base offset 0x480
- Context 3: Base offset 0x4C0
- Context 4: Base offset 0x500
- Context 5: Base offset 0x540
- Context 6: Base offset 0x580
- Context 7: Base offset 0x5C0

**Each context adds 0x40 to base offset**

**SYNDROME_FRAGMENT_6_x Special Fields:**
```
Bits [31:17] - Reserved
Bit  [16]    - SYNDROME_VALID (R/W)
               0 = Do not process this syndrome
               1 = Process this syndrome (triggers computation)
Bits [15:0]  - SYNDROME_6 (syndrome bits 192-207)
```

**Critical Note:** Writing SYNDROME_VALID=1 in fragment 6 initiates the error location computation. Always write fragments 0-5 first, then fragment 6 last.

## 9.3 Error Location Status Registers

Each context has one status register indicating processing results.

**Register Addresses:**
- `ELM_LOCATION_STATUS_0` @ 0x800 (Context 0)
- `ELM_LOCATION_STATUS_1` @ 0x900 (Context 1)
- `ELM_LOCATION_STATUS_2` @ 0xA00 (Context 2)
- `ELM_LOCATION_STATUS_3` @ 0xB00 (Context 3)
- `ELM_LOCATION_STATUS_4` @ 0xC00 (Context 4)
- `ELM_LOCATION_STATUS_5` @ 0xD00 (Context 5)
- `ELM_LOCATION_STATUS_6` @ 0xE00 (Context 6)
- `ELM_LOCATION_STATUS_7` @ 0xF00 (Context 7)

**Field Definitions:**
```
Bits [31:9]  - Reserved
Bit  [8]     - ECC_CORRECTABLE (Read-only)
               0 = Error location failed, results invalid
               1 = All errors successfully located
Bits [7:5]   - Reserved
Bits [4:0]   - ECC_NB_ERRORS (Read-only)
               Number of errors detected and located (0-31)
               Valid only if ECC_CORRECTABLE = 1
```

**Usage Flow:**
1. Write syndrome fragments 0-6 (with SYNDROME_VALID=1 in fragment 6)
2. Wait for interrupt or poll LOCATION_STATUS
3. Read ECC_CORRECTABLE to verify success
4. Read ECC_NB_ERRORS to get error count
5. Read ERROR_LOCATION registers for bit positions

## 9.4 Error Location Registers (Complete Map)

Each context provides up to 16 error location registers for BCH-16 mode.

**Context 0 Error Locations:**
- `ELM_ERROR_LOCATION_0_0` @ 0x880
- `ELM_ERROR_LOCATION_1_0` @ 0x884
- `ELM_ERROR_LOCATION_2_0` @ 0x888
- `ELM_ERROR_LOCATION_3_0` @ 0x88C
- `ELM_ERROR_LOCATION_4_0` @ 0x890
- `ELM_ERROR_LOCATION_5_0` @ 0x894
- `ELM_ERROR_LOCATION_6_0` @ 0x898
- `ELM_ERROR_LOCATION_7_0` @ 0x89C
- `ELM_ERROR_LOCATION_8_0` @ 0x8A0
- `ELM_ERROR_LOCATION_9_0` @ 0x8A4
- `ELM_ERROR_LOCATION_10_0` @ 0x8A8
- `ELM_ERROR_LOCATION_11_0` @ 0x8AC
- `ELM_ERROR_LOCATION_12_0` @ 0x8B0
- `ELM_ERROR_LOCATION_13_0` @ 0x8B4
- `ELM_ERROR_LOCATION_14_0` @ 0x8B8
- `ELM_ERROR_LOCATION_15_0` @ 0x8BC

**Context 1-7 Base Offsets:**
- Context 1: 0x980 (spacing +0x100 from context 0)
- Context 2: 0xA80
- Context 3: 0xB80
- Context 4: 0xC80
- Context 5: 0xD80
- Context 6: 0xE80
- Context 7: 0xF80

**Register Format (all ERROR_LOCATION registers):**
```
Bits [31:13] - Reserved
Bits [12:0]  - ECC_ERROR_LOCATION
               Error bit position (0 to 0x1FFF = 8191)
               Indicates which bit in the data+ECC needs correction
```

**Reading Error Locations:**
- Only read up to ECC_NB_ERRORS registers
- Higher-numbered registers contain invalid data if fewer errors exist
- Bit position 0 = LSB of first byte in the data buffer

## 9.5 Register Access Patterns

**Syndrome Writing (Continuous Mode):**
```c
// Context 0 example
volatile uint32_t *elm_base = 0x48080000;

// Write syndrome polynomial (208 bits total)
elm_base[0x400/4] = syndrome_bits_0_31;
elm_base[0x404/4] = syndrome_bits_32_63;
elm_base[0x408/4] = syndrome_bits_64_95;
elm_base[0x40C/4] = syndrome_bits_96_127;
elm_base[0x410/4] = syndrome_bits_128_159;
elm_base[0x414/4] = syndrome_bits_160_191;
elm_base[0x418/4] = syndrome_bits_192_207 | (1 << 16); // Set VALID bit

// Wait for completion interrupt
wait_for_elm_interrupt();

// Read status
uint32_t status = elm_base[0x800/4];
if (status & (1 << 8)) { // ECC_CORRECTABLE
    uint32_t num_errors = status & 0x1F;
    
    // Read error locations
    for (int i = 0; i < num_errors; i++) {
        uint32_t error_loc = elm_base[(0x880 + i*4)/4];
        uint32_t bit_pos = error_loc & 0x1FFF;
        // Correct bit at position bit_pos
    }
}
```

**Page Mode Access (8 contexts):**
```c
// Configure page mode
elm_base[0x80/4] |= (1 << 0); // PAGE_CTRL = 1

// Write all 8 syndromes
for (int ctx = 0; ctx < 8; ctx++) {
    uint32_t base = 0x400 + (ctx * 0x40);
    elm_base[(base + 0x00)/4] = syndrome[ctx].bits_0_31;
    elm_base[(base + 0x04)/4] = syndrome[ctx].bits_32_63;
    elm_base[(base + 0x08)/4] = syndrome[ctx].bits_64_95;
    elm_base[(base + 0x0C)/4] = syndrome[ctx].bits_96_127;
    elm_base[(base + 0x10)/4] = syndrome[ctx].bits_128_159;
    elm_base[(base + 0x14)/4] = syndrome[ctx].bits_160_191;
    elm_base[(base + 0x18)/4] = syndrome[ctx].bits_192_207 | (1 << 16);
}

// Wait for PAGE_MASK interrupt
wait_for_page_complete();

// Read all results
for (int ctx = 0; ctx < 8; ctx++) {
    uint32_t status_addr = 0x800 + (ctx * 0x100);
    uint32_t status = elm_base[status_addr/4];
    
    if (status & (1 << 8)) {
        uint32_t num_errors = status & 0x1F;
        uint32_t error_base = 0x880 + (ctx * 0x100);
        
        for (int i = 0; i < num_errors; i++) {
            uint32_t error_loc = elm_base[(error_base + i*4)/4];
            // Process error at (error_loc & 0x1FFF)
        }
    }
}
```

## 9.6 Memory Map Summary

**Complete ELM Address Space (0x4808_0000 - 0x4808_0FFF):**

```
0x000 - 0x0FF: Configuration Registers
  0x000: REVISION
  0x010: SYSCONFIG
  0x014: SYSSTATUS
  0x018: IRQSTATUS
  0x01C: IRQENABLE
  0x020: LOCATION_CONFIG
  0x024: PAGE_CTRL
  0x080-0x08C: SYNDROME_FRAGMENT_[0-6]_[0-7] config

0x400 - 0x43F: Context 0 Syndrome Input
  0x400-0x418: SYNDROME_FRAGMENT_[0-6]_0

0x440 - 0x47F: Context 1 Syndrome Input
  0x440-0x458: SYNDROME_FRAGMENT_[0-6]_1

0x480 - 0x4BF: Context 2 Syndrome Input
0x4C0 - 0x4FF: Context 3 Syndrome Input
0x500 - 0x53F: Context 4 Syndrome Input
0x540 - 0x57F: Context 5 Syndrome Input
0x580 - 0x5BF: Context 6 Syndrome Input
0x5C0 - 0x5FF: Context 7 Syndrome Input

0x800 - 0x8FF: Context 0 Results
  0x800: LOCATION_STATUS_0
  0x880-0x8BC: ERROR_LOCATION_[0-15]_0

0x900 - 0x9FF: Context 1 Results
0xA00 - 0xAFF: Context 2 Results
0xB00 - 0xBFF: Context 3 Results
0xC00 - 0xCFF: Context 4 Results
0xD00 - 0xDFF: Context 5 Results
0xE00 - 0xEFF: Context 6 Results
0xF00 - 0xFFF: Context 7 Results
```

## 9.7 Practical Examples

**Example 1: Single Error Correction (BCH-4)**
```c
// 512 byte sector with 4-bit correction
// Syndrome polynomial computed by GPMC ECC engine

uint32_t syndrome[7];
// ... compute syndrome from read data ...

// Write to ELM context 0
write_syndrome_fragment(0, 0, syndrome[0]);
write_syndrome_fragment(0, 1, syndrome[1]);
write_syndrome_fragment(0, 2, syndrome[2]);
write_syndrome_fragment(0, 3, syndrome[3]);
write_syndrome_fragment(0, 4, syndrome[4]);
write_syndrome_fragment(0, 5, syndrome[5]);
write_syndrome_fragment(0, 6, syndrome[6] | VALID_BIT);

// Poll or wait for interrupt
while (!(read_irq_status() & (1 << 0)));

// Check results
uint32_t status = read_location_status(0);
if (status & ECC_CORRECTABLE) {
    uint32_t errors = status & 0x1F;
    for (int i = 0; i < errors; i++) {
        uint32_t bit_pos = read_error_location(0, i);
        flip_bit(data_buffer, bit_pos);
    }
} else {
    // Uncorrectable error
    handle_ecc_failure();
}
```

**Example 2: NAND Page with 8 Sectors (Page Mode)**
```c
// Read 4KB NAND page (8 × 512 byte sectors)
// Each sector has independent BCH-8 ECC

uint32_t syndromes[8][7];
// ... compute all 8 syndrome polynomials ...

// Enable page mode
enable_page_mode();

// Write all 8 syndromes
for (int ctx = 0; ctx < 8; ctx++) {
    for (int frag = 0; frag < 6; frag++) {
        write_syndrome_fragment(ctx, frag, syndromes[ctx][frag]);
    }
    write_syndrome_fragment(ctx, 6, syndromes[ctx][6] | VALID_BIT);
}

// Wait for all processing complete
wait_for_page_mask_irq();

// Check all sectors
int uncorrectable = 0;
for (int ctx = 0; ctx < 8; ctx++) {
    uint32_t status = read_location_status(ctx);
    if (status & ECC_CORRECTABLE) {
        uint32_t errors = status & 0x1F;
        for (int i = 0; i < errors; i++) {
            uint32_t bit_pos = read_error_location(ctx, i);
            flip_bit(&page_data[ctx * 512], bit_pos);
        }
    } else {
        uncorrectable |= (1 << ctx);
    }
}

if (uncorrectable) {
    // Some sectors have uncorrectable errors
    handle_page_failure(uncorrectable);
}
```

## 9.8 Timing Considerations

**Processing Time:**
- BCH-4: ~2-4 μs per syndrome
- BCH-8: ~4-8 μs per syndrome
- BCH-16: ~8-16 μs per syndrome

**Interrupt Latency:**
- Continuous mode: Interrupt per syndrome completion
- Page mode: Single interrupt after all 8 contexts complete
- Use page mode for better throughput with multiple sectors

**Performance Optimization:**
- Pre-compute syndromes in CPU/DMA during read
- Use page mode for multi-sector NAND pages
- Pipeline: While ELM processes, start next read
- Clear interrupts promptly to avoid missing events

---

*Document Version: 1.1*
*Based on: AM335x TRM SPRUH73Q*
*Target: AI Training Reference*
*Updated: Complete ELM register specifications added*
