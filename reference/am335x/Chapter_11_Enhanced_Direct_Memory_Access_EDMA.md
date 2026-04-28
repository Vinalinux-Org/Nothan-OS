# 11 Enhanced Direct Memory Access (EDMA)



This chapter describes the EDMA of the device.



## 11.1 Introduction

The enhanced direct memory access (EDMA3) controller’s primary purpose is to service user-
programmed data transfers between two memory-mapped slave endpoints on the device.
Typical usage includes, but is not limited to the following:
• Servicing software-driven paging transfers (e.g., transfers from external memory, such as DDR2 to
internal device memory).
• Servicing event-driven peripherals, such as a serial port.
• Performing sorting or sub-frame extraction of various data structures.
• Offloading data transfers from the main device CPU(s).
The EDMA3 controller consists of two principal blocks:
• EDMA3 channel controller (EDMA3CC).
• EDMA3 transfer controller(s) (EDMA3TC).
The EDMA3 channel controller serves as the user interface for the EDMA3 controller. The EDMA3CC
includes parameter RAM (PaRAM), channel control registers, and interrupt control registers. The
EDMA3CC serves to prioritize incoming software requests or events from peripherals and submits transfer
requests (TRs) to the transfer controller.
The EDMA3 transfer controllers are slaves to the EDMA3 channel controller that is responsible for data
movement. The transfer controller issues read/write commands to the source and destination addresses
that are programmed for a given transfer. The operation is transparent to user.


### 11.1.1 EDMA3 Controller Block Diagram

Figure 11-1 shows a block diagram for the EDMA3 controller.


**Figure 11-1. EDMA3 Controller Block Diagram**

Transfer
controllers
MMR
Channel controller                                                         access
Read/write
TC0        commands
To/from       DMA/QDMA                                   Transfer                                           and data
Event
EDMA3          channel                     PaRAM          request                 EDMA3TC0_
queues
programmer          logic                                   submission                  ERRINT


MMR
access
Completion                                                                                     Read/write
EDMA3CC_ERRINT                                                     Completion                               TC2        commands
and error
EDMA3CC_INT[7:0]         interrupt                                  detection                                          and data
EDMA3CC_MPINT           logic                                                             EDMA3TC2_
ERRINT


### 11.1.2 Third-Party Channel Controller (TPCC) Overview


#### 11.1.2.1 TPCC Features

The general features of the TPCC module are:
• Up to 64 DMA Channels
– Channels triggered by:
• Event Synchronization
• Manual Synchronization (CPU write to ‘Event Set Register’)
• Chain Synchronization (completion of one transfer chains to the next)
– Parameterizable support for programmable DMA Channel to PaRAM mapping
• Up to 8 QDMA Channels
– QDMA Channels are triggered automatically upon writing to PaRAM
– Support for programmable QDMA Channel to PaRAM mapping
• Up to 64 Event Inputs
• Up to 8 Interrupt outputs for multi-core support
• Up to 256 PaRAM entries
– Each PaRAM entry can be used as DMA Entry (up to 64), QDMA Entry (up to 8), or Link Entry
(remaining)
• 8 Priority Levels for mapping CC/TC priority relative to priority of other masters in the system.
• Up to 3 Event Queues
• 16 Event Entries per Event Queue
• Supports three-transfer dimensions
– A-synchronized transfers—one dimension serviced per event
– AB-synchronized transfers—two dimensions serviced per event
– Independent Indexes on Source and Destination
– Does not support direct submission of 3D transfer to TC
– Chaining feature allows 3D transfer based on single event
• Increment and FIFO transfer addressing modes (TC feature)
• Linking mechanism allows automatic PaRAM Entry update
• Transfer Completion Signaling between TC and CC for Chaining and Interrupt generation.
• Programmable assignment of Priority to TC channel.
• Proxied Memory Protection for TR submission
• Parameterizable support for Active Memory Protection for accesses to PaRAM and registers.
• Queue Watermarking
• Missed Event Detection
• Error and status recording to facilitate debug
• Single Clock domain for all interfaces
• Parameterizable number of Write Completion interfaces (up to 8) (set to number of TC Channels)
• AET Event generation


#### 11.1.2.2 Unsupported TPCC Features

This device does not support AET event generation because output is not connected.
This device does not use the global completion interrupt. Only regional completion interrupts are
supported.
The channel controller only supports 4 memory protection regions 0-3.
The channel controller only supports 4 shadow regions 0-3.


Only 2 region completion interrupts are connected at the system level. See Table 10-1 for more
information.
Only 256 PaRAM entries are supported.
Only 3 event queues are supported.


### 11.1.3 Third-Party Transfer Controller (TPTC) Overview


#### 11.1.3.1 TPTC Features

The TPTC module includes the following features:
• Up to eight independent channels
• External event control use model (TPCC)
• Read and Write Master ports per Channel 64- or 128-bit configuration.
• Parameterizable FIFO size
• Up to four in-flight Transfer Requests
• Proxied Memory protection for data transfers
• Programmable Priority levels (up to 8)
• Background programmation capability
• Supports 2-dimensional transfers with independent indexes on Source and Destination.
• Support for increment or FIFO-mode transfers
• Interrupt and error support
• Single clock domain for all interfaces


#### 11.1.3.2 Unsupported TPTC Features

TPTCx supports 512-byte FIFO size.


## 11.2 Integration


### 11.2.1 Third-Party Channel Controller (TPCC) Integration

This device uses the TPCC peripheral to provide control over its third-party transfer channels (TPTCs).
TPCC Integration shows the integration of the TPCC module.

TPCC Integration


TPCC
L3 Fast                                   TR Interface
CFG Slave      Master                    To TPTCs
Interconnect
mpint_pend_po                            Fr. TPTC0
Host ARM                      errint_pend_po     Completion
Interrupts                                           Ports             Fr. TPTC1
int_pend_po0
Fr. TPTC2
PRU-ICSS Interrupts                 int_pend_po1
int_pend_po[3:2]
intg_pend_po
aet_po
Crossbar


From Event
Event


Sources                          event_pi[63:0]
(up to 128)                   64


#### 11.2.1.1 TPCC Connectivity Attributes

The general connectivity attributes of the TPCC are summarized in Table 11-1.


**Table 11-1. TPCC Connectivity Attributes**

Attributes                                                                Type
Power domain                                                              Peripheral Domain
Clock domain                                                              PD_PER_L3_GCLK
Reset signals                                                             PER_DOM_RST_N
Idle/Wakeup signals                                                       Smart Idle
Interrupt request                                                         4 Regional Completion Interrupts:
int_pend_po0 (EDMACOMPINT) – to MPU Subsystem
int_pend_po1 (tpcc_int_pend_po1) – to PRU-ICSS
Int_pend_po[3:2] - unused
Error Interrupt:
errint_po (EDMAERRINT) – to MPU Subsystem
Memory Protection Error Interrupt:
mpint_p0 (EDMAMPERR) – to MPU Subsystem
DMA request                                                               none
Physical address                                                          L3 Fast slave port


#### 11.2.1.2 TPCC Clock and Reset Management

The TPCC operates from a single clock and runs at the L3_Fast clock rate.


**Table 11-2. TPCC Clock Signals**

Clock Signal                       Max Freq                               Reference / Source            Comments
tpcc_clk_pi                        200 MHz                                CORE_CLKOUTM4                 pd_per_l3_gclk
Interface / Functional clock                                                                            From PRCM


#### 11.2.1.3 TPCC Pin List

The TPCC module does not include any external interface pins.


### 11.2.2 Third-Party Transfer Controller (TPTC) Integration

This device uses the three TPTC peripherals (TC0–TC2; TC3 is not supported) to perform EDMA
transfers between slave peripherals. The submission of transfer requests to the TPTCs is controlled by the
TPCC. TPTC Integration shows the integration of the TPTC modules

TPTC Integration

Device
TPTC
L3 Fast             CFG Slave                                        L3 Fast
Master Read
Interconnect                                                              Interconnect
128

L3 Fast
Master Write               Interconnect
Control                                                           128
dbs_pi[1:0]
Module

MPU Subsystem
erint_pend_po          Completion
PRU-ICSS                                                                  To TPCC
Port
Interrupts
int_pend_po

bigendian_pi


#### 11.2.2.1 TPTC Connectivity Attributes

The general connectivity attributes for the TPTCs are shown in Table 11-3.


**Table 11-3. TPTC Connectivity Attributes**

Attributes                                                           Type
Power domain                                                         Peripheral Domain
Clock domain                                                         PD_PER_L3_GCLK
Reset signals                                                        PER_DOM_RST_N
Idle/Wakeup signals                                                  Standby
Smart Idle
Interrupt request                                                    Error interrupt per instance
erint_pend_po (TCERRINTx) – to MPU Subsystem and PRU-
ICSS (tptc_erint_pend_po, TPTC0 only)
DMA request                                                          none
Physical address                                                     L3 Fast slave port


#### 11.2.2.2 TPTC Clock and Reset Management

The TPTC operates from a single clock and runs at the L3_Fast clock rate.


**Table 11-4. TPTC Clock Signals**

Clock Signal                      Max Freq                           Reference / Source                Comments
tptc_clk_pi                       200 MHz                            CORE_CLKOUTM4                     pd_per_l3_gclk
Interface / Functional clock                                                                           From PRCM


#### 11.2.2.3 TPTC Pin List

The TPTC module does not include any external interface pins.


## 11.3 Functional Description

This chapter discusses the architecture of the EDMA3 controller.


### 11.3.1 Functional Overview


#### 11.3.1.1 EDMA3 Channel Controller (EDMA3CC)

Figure 11-2 shows a functional block diagram of the EDMA3 channel controller (EDMA3CC).
The main blocks of the EDMA3CC are as follows:
• Parameter RAM (PaRAM): The PaRAM maintains parameter sets for channel and reload parameter
sets. You must write the PaRAM with the transfer context for the desired channels and link parameter
sets. EDMA3CC processes sets based on a trigger event and submits a transfer request (TR) to the
transfer controller.
• EDMA3 event and interrupt processing registers: Allows mapping of events to parameter sets,
enable/disable events, enable/disable interrupt conditions, and clearing interrupts.
• Completion detection: The completion detect block detects completion of transfers by the EDMA3TC
and/or slave peripherals. You can optionally use completion of transfers to chain trigger new transfers
or to assert interrupts.
• Event queues: Event queues form the interface between the event detection logic and the transfer
request submission logic.
• Memory protection registers: Memory protection registers define the accesses (privilege level and
requestor(s)) that are allowed to access the DMA channel shadow region view(s) and regions of
PaRAM.
Other functions include the following:
• Region registers: Region registers allow DMA resources (DMA channels and interrupts) to be assigned
to unique regions that different EDMA3 programmers own (for example, ARM).
• Debug registers: Debug registers allow debug visibility by providing registers to read the queue status,
controller status, and missed event status.
The EDMA3CC includes two channel types: DMA channels (64 channels) and QDMA channels (8
channels).
Each channel is associated with a given event queue/transfer controller and with a given PaRAM set. The
main thing that differentiates a DMA channel from a QDMA channel is the method that the system uses to
trigger transfers. See Section 11.3.4.


**Figure 11-2. EDMA3 Channel Controller (EDMA3CC) Block Diagram**

From peripherals/external events                                                                             EDMA3
E63 E1 E0                                                                                            channel
controller
Event queues                                              PaRAM
15              0                                    Parameter
Event


64:1 priority encoder
register                                                                  Queue 0                                          set 0


Transfer request process submit
Event     (ER/ERH)                                                                                                                  Parameter
trigger      Event                                                                                                                     set 1
enable          64


Channel mapping
15              0
register
(EER/EERH)                                                                  Queue 1
To
TC(S)
Event
Manual         set    64                                                         15              0
trigger      register
(ESR/ESRH)                                                                  Queue 2                                        Parameter
64                                                                                                      set 254
Chain                                                                            Queue bypass                                       Parameter
trigger     Chained                                                                                                                   set 255
event
8:1 priority encoder


register


Early completion
(CER/CERH)


QDMA
event              8
register
(QER)
Completion
QDMA trigger
interface

To chained event register (CER/CERH)                                        Completion                                                                                        From
detection                                                                                        EDMA3TC0


Memory                       Error                                       Completion                                                                                                       From
protection                  detection                                      interrupt                                                                                                       EDMA3TC2


EDMA3CC_INT[7:0]

EDMA3CC_              Read/                                                                                                          EDMA3CC_ERRINT
MPINT              write to/
from CPU


A trigger event is needed to initiate a transfer. A trigger event may be due to an external event, manual
write to the event set register, or chained event for DMA channels. QDMA channels auto-trigger when a
write to the trigger word that you program occurs on the associated PaRAM set. All such trigger events
are logged into appropriate registers upon recognition.
Once a trigger event is recognized, the appropriate event gets queued in the EDMA3CC event queue. The
assignment of each DMA/QDMA channel to an event queue is programmable. Each queue is 16 events
deep; therefore, you can queue up to 16 events (on a single queue) in the EDMA3CC at a time. Additional
pending events that are mapped to a full queue are queued when the event queue space becomes
available. See Section 11.3.11.
If events on different channels are detected simultaneously, the events are queued based on a fixed
priority arbitration scheme with the DMA channels being higher priority events than the QDMA channels.
Among the two groups of channels, the lowest-numbered channel is the highest priority.


Each event in the event queue is processed in FIFO order. When the head of the queue is reached, the
PaRAM associated with that channel is read to determine the transfer details. The TR submission logic
evaluates the validity of the TR and is responsible for submitting a valid transfer request (TR) to the
appropriate EDMA3TC (based on the event queue to the EDMA3TC association, Q0 goes to TC0 , Q1
goes to TC1, and Q2 goes to TC2). For more information, refer to Section 11.3.3.
The EDMA3TC receives the request and is responsible for data movement, as specified in the transfer
request packet (TRP), other necessary tasks like buffering, and ensuring transfers are carried out in an
optimal fashion wherever possible. For more information on EDMA3TC, refer to Section 11.3.1.2.
If you have decided to receive an interrupt or to chain to another channel on completion of the current
transfer, the EDMA3TC signals completion to the EDMA3CC completion detection logic when the transfer
is complete. You can alternately choose to trigger completion when a TR leaves the EDMA3CC boundary,
rather than wait for all of the data transfers to complete. Based on the setting of the EDMA3CC interrupt
registers, the completion interrupt generation logic is responsible for generating EDMA3CC completion
interrupts to the CPU. For more information, refer to Section 11.3.5.
Additionally, the EDMA3CC also has an error detection logic that causes an error interrupt generation on
various error conditions (like missed events, exceeding event queue thresholds, etc.). For more
information on error interrupts, refer to Section 11.3.9.4.


#### 11.3.1.2 EDMA3 Transfer Controller (EDMA3TC)

Section 11.3.9.4 shows a functional block diagram of the EDMA3 transfer controller (EDMA3TC).


**Figure 11-3. EDMA3 Transfer Controller (EDMA3TC) Block Diagram**

EDMA3TCn


SRC active                     Read


controller
register set
Read
command
register set


Read data
Program


Transfer request
submission
SRC                         Write
To completion
controller


command
Write


detection logic
in EDMA3CC
Write data
Destination FIFO
EDMA3TCn_ERRINT                                 register set


The main blocks of the EDMA3TC are:
• DMA program register set: The DMA program register set stores the transfer requests received from
the EDMA3 channel controller (EDMA3CC).
• DMA source active register set: The DMA source active register set stores the context for the DMA
transfer request currently in progress in the read controller.
• Read controller: The read controller issues read commands to the source address.
• Destination FIFO register set: The destination (DST) FIFO register set stores the context for the DMA
transfer request(s) currently in progress in the write controller.
• Write controller: The write controller issues write commands/write data to the destination slave.
• Data FIFO: The data FIFO exists for holding temporary in-flight data.
• Completion interface: The completion interface sends completion codes to the EDMA3CC when a
transfer completes, and generates interrupts and chained events (also, see Section 11.3.1.1 for more
information on transfer completion reporting).


When the EDMA3TC is idle and receives its first TR, DMA program register set receives the TR, where it
transitions to the DMA source active set and the destination FIFO register set immediately. The second
TR (if pending from EDMA3CC) is loaded into the DMA program set, ensuring it can start as soon as
possible when the active transfer completes. As soon as the current active set is exhausted, the TR is
loaded from the DMA program register set into the DMA source active register set as well as to the
appropriate entry in the destination FIFO register set.
The read controller issues read commands governed by the rules of command fragmentation and
optimization. These are issued only when the data FIFO has space available for the data read. When
sufficient data is in the data FIFO, the write controller starts issuing a write command again following the
rules for command fragmentation and optimization. For more information on command fragmentation and
optimization, refer to Section 11.3.12.1.1.
Depending on the number of entries, the read controller can process up to two or four transfer requests
ahead of the destination subject to the amount of free data FIFO.


### 11.3.2 Types of EDMA3 Transfers

An EDMA3 transfer is always defined in terms of three dimensions. Figure 11-4 shows the three
dimensions used by EDMA3 transfers. These three dimensions are defined as:
• 1st Dimension or Array (A): The 1st dimension in a transfer consists of ACNT contiguous bytes.
• 2nd Dimension or Frame (B): The 2nd dimension in a transfer consists of BCNT arrays of ACNT bytes.
Each array transfer in the 2nd dimension is separated from each other by an index programmed using
SRCBIDX or DSTBIDX.
• 3rd Dimension or Block (C): The 3rd dimension in a transfer consists of CCNT frames of BCNT arrays
of ACNT bytes. Each transfer in the 3rd dimension is separated from the previous by an index
programmed using SRCCIDX or DSTCIDX.
Note that the reference point for the index depends on the synchronization type. The amount of data
transferred upon receipt of a trigger/synchronization event is controlled by the synchronization types
(SYNCDIM bit in OPT). Of the three dimensions, only two synchronization types are supported: A-
synchronized transfers and AB-synchronized transfers.


**Figure 11-4. Definition of ACNT, BCNT, and CCNT**

ACNT bytes in
Array/1st dimension


Frame 0       Array 1              Array 2                   Array BCNT


Frame 1       Array 1              Array 2                   Array BCNT            CCNT frames in
Block/3rd dimmension


Frame CCNT         Array 1              Array 2                   Array BCNT


BCNT arrays in Frame/2nd dimmension


#### 11.3.2.1 A-Synchronized Transfers

In an A-synchronized transfer, each EDMA3 sync event initiates the transfer of the 1st dimension of ACNT
bytes, or one array of ACNT bytes. In other words, each event/TR packet conveys the transfer information
for one array only. Thus, BCNT × CCNT events are needed to completely service a PaRAM set.
Arrays are always separated by SRCBIDX and DSTBIDX, as shown in Figure 11-5, where the start
address of Array N is equal to the start address of Array N – 1 plus source (SRC) or destination (DST)
BIDX.
Frames are always separated by SRCCIDX and DSTCIDX. For A-synchronized transfers, after the frame
is exhausted, the address is updated by adding SRCCIDX/DSTCIDX to the beginning address of the last
array in the frame. As in Figure 11-5, SRCCIDX/DSTCIDX is the difference between the start of Frame 0
Array 3 to the start of Frame 1 Array 0.
Figure 11-5 shows an A-synchronized transfer of 3 (CCNT) frames of 4 (BCNT) arrays of n (ACNT) bytes.
In this example, a total of 12 sync events (BCNT × CCNT) exhaust a PaRAM set. See Section 11.3.3.6 for
details on parameter set updates.


**Figure 11-5. A-Synchronized Transfers (ACNT = n, BCNT = 4, CCNT = 3)**

(SRC|DST)          (SRC|DST)            (SRC|DST)            Each array submit
as one TR
BIDX                BIDX                 BIDX
Frame 0   Array 0            Array 1              Array 2               Array 3

(SRC|DST)
CIDX

(SRC|DST)          (SRC|DST)            (SRC|DST)
BIDX                BIDX                 BIDX
Frame 1   Array 0            Array 1              Array 2               Array 3

(SRC|DST)
CIDX
(SRC|DST)          (SRC|DST)           (SRC|DST)
BIDX                BIDX                 BIDX
Frame 2   Array 0            Array 1              Array 2               Array 3


#### 11.3.2.2 AB-Synchronized Transfers

In a AB-synchronized transfer, each EDMA3 sync event initiates the transfer of 2 dimensions or one
frame. In other words, each event/TR packet conveys information for one entire frame of BCNT arrays of
ACNT bytes. Thus, CCNT events are needed to completely service a PaRAM set.
Arrays are always separated by SRCBIDX and DSTBIDX as shown in Figure 11-6. Frames are always
separated by SRCCIDX and DSTCIDX.
Note that for AB-synchronized transfers, after a TR for the frame is submitted, the address update is to
add SRCCIDX/DSTCIDX to the beginning address of the beginning array in the frame. This is different
from A-synchronized transfers where the address is updated by adding SRCCIDX/DSTCIDX to the start
address of the last array in the frame. See Section 11.3.3.6 for details on parameter set updates.
Figure 11-6 shows an AB-synchronized transfer of 3 (CCNT) frames of 4 (BCNT) arrays of n (ACNT)
bytes. In this example, a total of 3 sync events (CCNT) exhaust a PaRAM set; that is, a total of 3 transfers
of 4 arrays each completes the transfer.


**Figure 11-6. AB-Synchronized Transfers (ACNT = n, BCNT = 4, CCNT = 3)**

AB_Sync transfer

(SRC|DST)         (SRC|DST)           (SRC|DST)           Each array submit
as one TR
BIDX               BIDX               BIDX
Frame 0      Array 0           Array 1             Array 2            Array 3

(SRC|DST)
CIDX
(SRC|DST)        (SRC|DST)           (SRC|DST)
BIDX               BIDX               BIDX
Frame 1      Array 0           Array 1             Array 2            Array 3


(SRC|DST)
CIDX
(SRC|DST)        (SRC|DST)           (SRC|DST)
BIDX               BIDX               BIDX
Frame 2      Array 0           Array 1             Array 2            Array 3


NOTE: ABC-synchronized transfers are not directly supported. But can be logically achieved by
chaining between multiple AB-synchronized transfers.


### 11.3.3 Parameter RAM (PaRAM)

The EDMA3 controller is a RAM-based architecture. The transfer context (source/destination addresses,
count, indexes, etc.) for DMA or QDMA channels is programmed in a parameter RAM table within
EDMA3CC, referred to as PaRAM. The PaRAM table is segmented into multiple PaRAM sets. Each
PaRAM set includes eight four-byte PaRAM set entries (32-bytes total per PaRAM set), which includes
typical DMA transfer parameters such as source address, destination address, transfer counts, indexes,
options, etc.
The PaRAM structure supports flexible ping-pong, circular buffering, channel chaining, and auto-reloading
(linking).
The contents of the PaRAM include the following:
• 256 PaRAM sets
• 64 channels that are direct mapped and can be used as link or QDMA sets if not used for DMA
channels
• 64 channels remain for link or QDMA sets


By default, all channels map to PaRAM set to 0. These should be remapped before use. For more
information, see (DCHMAP registers) and (QCHMAP registers).


**Table 11-5. EDMA3 Parameter RAM Contents**

PaRAM Set Number                           Address                               Parameters
0               EDMA Base Address + 4000h to EDMA Base                  PaRAM set 0
Address + 401Fh
1               EDMA Base Address + 4020h to EDMA Base                  PaRAM set 1
Address + 403Fh
2               EDMA Base Address + 4040h to EDMA Base                  PaRAM set 2
Address + 405Fh
3               EDMA Base Address + 4060h to EDMA Base                  PaRAM set 3
Address + 407Fh
4               EDMA Base Address + 4080h to EDMA Base                  PaRAM set 4
Address + 409Fh
5               EDMA Base Address + 40A0h to EDMA Base                  PaRAM set 5
Address + 40BFh
6               EDMA Base Address + 40C0h to EDMA Base                  PaRAM set 6
Address + 40DFh
7               EDMA Base Address + 40E0h to EDMA Base                  PaRAM set 7
Address + 40FFh
8               EDMA Base Address + 4100h to EDMA Base                  PaRAM set 8
Address + 411Fh
9               EDMA Base Address + 4120h to EDMA Base                  PaRAM set 9
Address + 413Fh
...                                    ...                                   ...
63              EDMA Base Address + 47E0h to EDMA Base                  PaRAM set 63
Address + 47FFh
64              EDMA Base Address + 4800h to EDMA Base                  PaRAM set 64
Address + 481Fh
65              EDMA Base Address + 4820h to EDMA Base                  PaRAM set 65
Address + 483Fh
...                                    ...                                   ...
254             EDMA Base Address + 5FC0h to EDMA Base                 PaRAM set 254
Address + 5FDFh
255             EDMA Base Address + 5FE0h to EDMA Base                 PaRAM set 255
Address + 5FFFh


#### 11.3.3.1 PaRAM

Each parameter set of PaRAM is organized into eight 32-bit words or 32 bytes, as shown in Figure 11-7
and described in Table 11-6. Each PaRAM set consists of 16-bit and 32-bit parameters.


**Figure 11-7. PaRAM Set**

Byte          Set                                                                                  Byte address
address          #              PaRAM                                            PaRAM set               offset

EDMA Base Address + 4000h    0        Parameter set 0                                            OPT                +0h
EDMA Base Address + 4020h    1        Parameter set 1                                            SRC                +4h
EDMA Base Address + 4040h    2        Parameter set 2                                     BCNT          ACNT        +8h
EDMA Base Address + 4060h    3        Parameter set 3                                            DST                +Ch
DSTBIDX           SRCBIDX      +10h
BCNTRLD             LINK       +14h
DSTCIDX           SRCCIDX      +18h
Rsvd          CCNT        +1Ch


EDMA Base Address + 5FC0    254      Parameter set 254
EDMA Base Address + 5FE0    255      Parameter set 255


**Table 11-6. EDMA3 Channel Parameter Description**

Offset Address
(bytes)       Acronym               Parameter                             Description
0h          OPT                   Channel Options                       Transfer configuration options
4h          SRC                   Channel Source Address                The byte address from which data is transferred
8h (1)       ACNT                  Count for 1st Dimension               Unsigned value specifying the number of contiguous bytes
within an array (first dimension of the transfer). Valid values
range from 1 to 65 535.
BCNT                  Count for 2nd Dimension               Unsigned value specifying the number of arrays in a frame,
where an array is ACNT bytes. Valid values range from 1 to
65 535.
Ch          DST                   Channel Destination Address           The byte address to which data is transferred
10h (1)       SRCBIDX               Source BCNT Index                     Signed value specifying the byte address offset between
source arrays within a frame (2nd dimension). Valid values
range from –32 768 and 32 767.
DSTBIDX               Destination BCNT Index                Signed value specifying the byte address offset between
destination arrays within a frame (2nd dimension). Valid
values range from –32 768 and 32 767.
14h (1)       LINK                  Link Address                          The PaRAM address containing the PaRAM set to be linked
(copied from) when the current PaRAM set is exhausted. A
value of FFFFh specifies a null link.
BCNTRLD               BCNT Reload                           The count value used to reload BCNT when BCNT
decrements to 0 (TR is submitted for the last array in 2nd
dimension). Only relevant in A-synchronized transfers.
18h (1)       SRCCIDX               Source CCNT Index                     Signed value specifying the byte address offset between
frames within a block (3rd dimension). Valid values range
from –32 768 and 32 767.
A-synchronized transfers: The byte address offset from the
beginning of the last source array in a frame to the
beginning of the first source array in the next frame.
AB-synchronized transfers: The byte address offset from the
beginning of the first source array in a frame to the
beginning of the first source array in the next frame.
DSTCIDX               Destination CCNT index                Signed value specifying the byte address offset between
frames within a block (3rd dimension). Valid values range
from –32 768 and 32 767.
A-synchronized transfers: The byte address offset from the
beginning of the last destination array in a frame to the
beginning of the first destination array in the next frame.
AB-synchronized transfers: The byte address offset from the
beginning of the first destination array in a frame to the
beginning of the first destination array in the next frame.
1Ch          CCNT                  Count for 3rd Dimension               Unsigned value specifying the number of frames in a block,
where a frame is BCNT arrays of ACNT bytes. Valid values
range from 1 to 65 535.
RSVD                  Reserved                              Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
(1)
If OPT, SRC, or DST is the trigger word for a QDMA transfer, then doing a 32-bit access is required for that field. TI also recommends
performing only 32-bit accesses on the parameter RAM for best code compatibility. For example, switching the endianness of the
processor swaps addresses of the 16-bit fields, but 32-bit accesses avoid the issue entirely.


#### 11.3.3.2 EDMA3 Channel PaRAM Set Entry Fields


##### 11.3.3.2.1 Channel Options Parameter (OPT)

The channel options parameter (OPT) is shown in Figure 11-8 and described in Table 11-7.


**Figure 11-8. Channel Options Parameter (OPT)**

31      30         28     27                     24       23             22              21        20         19            18        17         16
PRIV      Reserved              PRIVID                  ITCCHEN         TCCHEN      ITCINTEN      TCINTEN           Reserved                TCC
R-0          R-0                   R-0                  R/W-0          R/W-0           R/W-0      R/W-0             R/W-0                 R/W-0

15                 12     11         10           8        7                                        4          3              2       1           0
TCC              TCCMOD            FWID                                Reserved                     STATIC      SYNCDIM      DAM      SAM
E
R/W-0             R/W-0            R/W-0                                R/W-0                        R/W-0        R/W-0      R/W-0    R/W-0


**Table 11-7. Channel Options Parameters (OPT) Field Descriptions**

Bit     Field             Value         Description
31      PRIV                            Privilege level (supervisor versus user) for the host/CPU/DMA that programmed this PaRAM set. This
value is set with the EDMA3 master’s privilege value when any part of the PaRAM set is written.
0         User level privilege.
1         Supervisor level privilege.
30-28     Reserved              0         Reserved. Always write 0 to this bit; writes of 1 to this bit are not supported and attempts to do so may
result in undefined behavior.
27-24     PRIVID            0-Fh          Privilege identification for the external host/CPU/DMA that programmed this PaRAM set. This value is
set with the EDMA3 master’s privilege identification value when any part of the PaRAM set is written.
23      ITCCHEN                         Intermediate transfer completion chaining enable.
0         Intermediate transfer complete chaining is disabled.
1         Intermediate transfer complete chaining is enabled.
When enabled, the chained event register (CER/CERH) bit is set on every intermediate chained transfer
completion (upon completion of every intermediate TR in the PaRAM set, except the final TR in the
PaRAM set). The bit (position) set in CER or CERH is the TCC value specified.
22      TCCHEN                          Transfer complete chaining enable.
0         Transfer complete chaining is disabled.
1         Transfer complete chaining is enabled.
When enabled, the chained event register (CER/CERH) bit is set on final chained transfer completion
(upon completion of the final TR in the PaRAM set). The bit (position) set in CER or CERH is the TCC
value specified.
21      ITCINTEN                        Intermediate transfer completion interrupt enable.
0         Intermediate transfer complete interrupt is disabled.
1         Intermediate transfer complete interrupt is enabled.
When enabled, the interrupt pending register (IPR / IPRH) bit is set on every intermediate transfer
completion (upon completion of every intermediate TR in the PaRAM set, except the final TR in the
PaRAM set). The bit (position) set in IPR or IPRH is the TCC value specified. To generate a completion
interrupt to the CPU, the corresponding IER [TCC] / IERH [TCC] bit must be set.
20      TCINTEN                         Transfer complete interrupt enable.
0         Transfer complete interrupt is disabled.
1         Transfer complete interrupt is enabled.
When enabled, the interrupt pending register (IPR / IPRH) bit is set on transfer completion (upon
completion of the final TR in the PaRAM set). The bit (position) set in IPR or IPRH is the TCC value
specified. To generate a completion interrupt to the CPU, the corresponding IER[TCC] / IERH [TCC] bit
must be set.
19-18     Reserved              0         Reserved. Always write 0 to this bit; writes of 1 to this bit are not supported and attempts to do so may
result in undefined behavior.


**Table 11-7. Channel Options Parameters (OPT) Field Descriptions (continued)**

Bit     Field         Value   Description
17-12     TCC           0-3Fh   Transfer complete code. This 6-bit code sets the relevant bit in the chaining enable register (CER [TCC]
/CERH [TCC]) for chaining or in the interrupt pending register (IPR [TCC] / IPRH [TCC]) for interrupts.
11      TCCMODE               Transfer complete code mode. Indicates the point at which a transfer is considered completed for
chaining and interrupt generation.
0     Normal completion: A transfer is considered completed after the data has been transferred.
1     Early completion: A transfer is considered completed after the EDMA3CC submits a TR to the
EDMA3TC. TC may still be transferring data when the interrupt/chain is triggered.
10-8     FWID           0-7h   FIFO Width. Applies if either SAM or DAM is set to constant addressing mode.
0     FIFO width is 8-bit.
1h    FIFO width is 16-bit.
2h    FIFO width is 32-bit.
3h    FIFO width is 64-bit.
4h    FIFO width is 128-bit.
5h    FIFO width is 256-bit.
6h-7h   Reserved.
7-4      Reserved        0     Reserved. Always write 0 to this bit; writes of 1 to this bit are not supported and attempts to do so may
result in undefined behavior.
3       STATIC                Static set.
0     Set is not static. The PaRAM set is updated or linked after a TR is submitted. A value of 0 should be
used for DMA channels and for non-final transfers in a linked list of QDMA transfers.
1     Set is static. The PaRAM set is not updated or linked after a TR is submitted. A value of 1 should be
used for isolated QDMA transfers or for the final transfer in a linked list of QDMA transfers.
2       SYNCDIM               Transfer synchronization dimension.
0     A-synchronized. Each event triggers the transfer of a single array of ACNT bytes.
1     AB-synchronized. Each event triggers the transfer of BCNT arrays of ACNT bytes.
1       DAM                   Destination address mode.
0     Increment (INCR) mode. Destination addressing within an array increments. Destination is not a FIFO.
1     Constant addressing (CONST) mode. Destination addressing within an array wraps around upon
reaching FIFO width.
0       SAM                   Source address mode.
0     Increment (INCR) mode. Source addressing within an array increments. Source is not a FIFO.
1     Constant addressing (CONST) mode. Source addressing within an array wraps around upon reaching
FIFO width.


##### 11.3.3.2.2 Channel Source Address (SRC)

The 32-bit source address parameter specifies the starting byte address of the source. For SAM in
increment mode, there are no alignment restrictions imposed by EDMA3. For SAM in constant addressing
mode, you must program the source address to be aligned to a 256-bit aligned address (5 LSBs of
address must be 0). The EDMA3TC will signal an error, if this rule is violated. See Section 11.3.12.3 for
additional details.


##### 11.3.3.2.3 Channel Destination Address (DST)

The 32-bit destination address parameter specifies the starting byte address of the destination. For DAM
in increment mode, there are no alignment restrictions imposed by EDMA3. For DAM in constant
addressing mode, you must program the destination address to be aligned to a 256-bit aligned address (5
LSBs of address must be 0). The EDMA3TC will signal an error, if this rule is violated. See
Section 11.3.12.3 for additional details.


##### 11.3.3.2.4 Count for 1st Dimension (ACNT)

ACNT represents the number of bytes within the 1st dimension of a transfer. ACNT is a 16-bit unsigned
value with valid values between 0 and 65 535. Therefore, the maximum number of bytes in an array is
65 535 bytes (64K – 1 bytes). ACNT must be greater than or equal to 1 for a TR to be submitted to
EDMA3TC. A transfer with ACNT equal to 0 is considered either a null or dummy transfer. A dummy or
null transfer generates a completion code depending on the settings of the completion bit fields in OPT.
See Section 11.3.3.5 and Section 11.3.5.3 for details on dummy/null completion conditions.


##### 11.3.3.2.5 Count for 2nd Dimension (BCNT)

BCNT is a 16-bit unsigned value that specifies the number of arrays of length ACNT. For normal
operation, valid values for BCNT are between 1 and 65 535. Therefore, the maximum number of arrays in
a frame is 65 535 (64K – 1 arrays). A transfer with BCNT equal to 0 is considered either a null or dummy
transfer. A dummy or null transfer generates a completion code depending on the settings of the
completion bit fields in OPT.
See Section 11.3.3.5 and Section 11.3.5.3 for details on dummy/null completion conditions.


##### 11.3.3.2.6 Count for 3rd Dimension (CCNT)

CCNT is a 16-bit unsigned value that specifies the number of frames in a block. Valid values for CCNT are
between 1 and 65 535. Therefore, the maximum number of frames in a block is 65 535 (64K – 1 frames).
A transfer with CCNT equal to 0 is considered either a null or dummy transfer. A dummy or null transfer
generates a completion code depending on the settings of the completion bit fields in OPT.
A CCNT value of 0 is considered either a null or dummy transfer. See Section 11.3.3.5 and
Section 11.3.5.3 for details on dummy/null completion conditions.


##### 11.3.3.2.7 BCNT Reload (BCNTRLD)

BCNTRLD is a 16-bit unsigned value used to reload the BCNT field once the last array in the
2nd dimension is transferred. This field is only used for A-synchronized transfers. In this case, the
EDMA3CC decrements the BCNT value by 1 on each TR submission. When BCNT reaches 0, the
EDMA3CC decrements CCNT and uses the BCNTRLD value to reinitialize the BCNT value.
For AB-synchronized transfers, the EDMA3CC submits the BCNT in the TR and the EDMA3TC
decrements BCNT appropriately. For AB-synchronized transfers, BCNTRLD is not used.


##### 11.3.3.2.8 Source B Index (SRCBIDX)

SRCBIDX is a 16-bit signed value (2s complement) used for source address modification between each
array in the 2nd dimension. Valid values for SRCBIDX are between –32 768 and 32 767. It provides a
byte address offset from the beginning of the source array to the beginning of the next source array. It
applies to both A-synchronized and AB-synchronized transfers. Some examples:
• SRCBIDX = 0000h (0): no address offset from the beginning of an array to the beginning of the next
array. All arrays are fixed to the same beginning address.
• SRCBIDX = 0003h (+3): the address offset from the beginning of an array to the beginning of the next
array in a frame is 3 bytes. For example, if the current array begins at address 1000h, the next array
begins at 1003h.
• SRCBIDX = FFFFh (–1): the address offset from the beginning of an array to the beginning of the next
array in a frame is –1 byte. For example, if the current array begins at address 5054h, the next array
begins at 5053h.


##### 11.3.3.2.9 Destination B Index (DSTBIDX)

DSTBIDX is a 16-bit signed value (2s complement) used for destination address modification between
each array in the 2nd dimension. Valid values for DSTBIDX are between –32 768 and 32 767. It provides
a byte address offset from the beginning of the destination array to the beginning of the next destination
array within the current frame. It applies to both A-synchronized and AB-synchronized transfers. See
SRCBIDX for examples.


##### 11.3.3.2.10 Source C Index (SRCCIDX)

SRCCIDX is a 16-bit signed value (2s complement) used for source address modification in the
3rd dimension. Valid values for SRCCIDX are between –32 768 and 32 767. It provides a byte address
offset from the beginning of the current array (pointed to by SRC address) to the beginning of the first
source array in the next frame. It applies to both A-synchronized and AB-synchronized transfers. Note that
when SRCCIDX is applied, the current array in an A-synchronized transfer is the last array in the frame
(Figure 11-5), while the current array in an AB-synchronized transfer is the first array in the frame
(Figure 11-6).


##### 11.3.3.2.11 Destination C Index (DSTCIDX)

DSTCIDX is a 16-bit signed value (2s complement) used for destination address modification in the
3rd dimension. Valid values are between –32 768 and 32 767. It provides a byte address offset from the
beginning of the current array (pointed to by DST address) to the beginning of the first destination array
TR in the next frame. It applies to both A-synchronized and AB-synchronized transfers. Note that when
DSTCIDX is applied, the current array in an A-synchronized transfer is the last array in the frame
(Figure 11-5), while the current array in a AB-synchronized transfer is the first array in the frame
(Figure 11-6).


##### 11.3.3.2.12 Link Address (LINK)

The EDMA3CC provides a mechanism, called linking, to reload the current PaRAM set upon its natural
termination (that is, after the count fields are decremented to 0) with a new PaRAM set. The 16-bit
parameter LINK specifies the byte address offset in the PaRAM from which the EDMA3CC loads/reloads
the next PaRAM set during linking.
You must program the link address to point to a valid aligned 32-byte PaRAM set. The 5 LSBs of the LINK
field should be cleared to 0.
The EDMA3CC ignores the upper 2 bits of the LINK entry, allowing the programmer the flexibility of
programming the link address as either an absolute/literal byte address or use the PaRAM-base-relative
offset address. Therefore, if you make use of the literal address with a range from 4000h to 7FFFh, it will
be treated as a PaRAM-base-relative value of 0000h to 3FFFh.
You should make sure to program the LINK field correctly, so that link update is requested from a PaRAM
address that falls in the range of the available PaRAM addresses on the device.
A LINK value of FFFFh is referred to as a NULL link that should cause the EDMA3CC to perform an
internal write of 0 to all entries of the current PaRAM set, except for the LINK field that is set to FFFFh.
Also, see Section 11.3.5 for details on terminating a transfer.


#### 11.3.3.3 Null PaRAM Set

A null PaRAM set is defined as a PaRAM set where all count fields (ACNT, BCNT, and CCNT) are
cleared to 0. If a PaRAM set associated with a channel is a NULL set, then when serviced by the
EDMA3CC, the bit corresponding to the channel is set in the associated event missed register (EMR,
EMRH, or QEMR). This bit remains set in the associated secondary event register (SER, SERH, or
QSER). This implies that any future events on the same channel are ignored by the EDMA3CC and you
are required to clear the bit in SER, SERH, or QSER for the channel. This is considered an error
condition, since events are not expected on a channel that is configured as a null transfer. See and for
more information on the SER and EMR registers, respectively.


#### 11.3.3.4 Dummy PaRAM Set

A dummy PaRAM set is defined as a PaRAM set where at least one of the count fields (ACNT, BCNT, or
CCNT) is cleared to 0 and at least one of the count fields is nonzero.
If a PaRAM set associated with a channel is a dummy set, then when serviced by the EDMA3CC, it will
not set the bit corresponding to the channel (DMA/QDMA) in the event missed register (EMR, EMRH, or
QEMR) and the secondary event register (SER, SERH, or QSER) bit gets cleared similar to a normal
transfer. Future events on that channel are serviced. A dummy transfer is a legal transfer of 0 bytes. For
more information, see the SER and EMR registers.


#### 11.3.3.5 Dummy Versus Null Transfer Comparison

There are some differences in the way the EDMA3CC logic treats a dummy versus a null transfer request.
A null transfer request is an error condition, but a dummy transfer is a legal transfer of 0 bytes. A null
transfer causes an error bit (En) in EMR to get set and the En bit in SER remains set, essentially
preventing any further transfers on that channel without clearing the associated error registers.
Table 11-8 summarizes the conditions and effects of null and dummy transfer requests.


**Table 11-8. Dummy and Null Transfer Request**

Feature                                                                                    Null TR           Dummy TR
EMR/EMRH/QEMR is set                                                                        Yes                  No
SER/SERH/QSER remains set                                                                   Yes                  No
Link update (STATIC = 0 in OPT)                                                             Yes                  Yes
QER is set                                                                                  Yes                  Yes
IPR/IPRH CER/CERH is set using early completion                                             Yes                  Yes


#### 11.3.3.6 Parameter Set Updates

When a TR is submitted for a given DMA/QDMA channel and its corresponding PaRAM set, the
EDMA3CC is responsible for updating the PaRAM set in anticipation of the next trigger event. For events
that are not final, this includes address and count updates; for final events, this includes the link update.
The specific PaRAM set entries that are updated depend on the channel’s synchronization type (A-
synchronized or B-synchronized) and the current state of the PaRAM set. A B-update refers to the
decrementing of BCNT in the case of A-synchronized transfers after the submission of successive TRs. A
C-update refers to the decrementing of CCNT in the case of A-synchronized transfers after BCNT TRs for
ACNT byte transfers have submitted. For AB-synchronized transfers, a C-update refers to the
decrementing of CCNT after submission of every transfer request.
See Table 11-9 for details and conditions on the parameter updates. A link update occurs when the
PaRAM set is exhausted, as described in Section 11.3.3.7.
After the TR is read from the PaRAM (and is in process of being submitted to EDMA3TC), the following
fields are updated if needed:
• A-synchronized: BCNT, CCNT, SRC, DST.
• AB-synchronized: CCNT, SRC, DST.
The following fields are not updated (except for during linking, where all fields are overwritten by the link
PaRAM set):
• A-synchronized: ACNT, BCNTRLD, SRCBIDX, DSTBIDX, SRCCIDX, DSTCIDX, OPT, LINK.
• AB-synchronized: ACNT, BCNT, BCNTRLD, SRCBIDX, DSTBIDX, SRCCIDX, DSTCIDX, OPT, LINK.


Note that PaRAM updates only pertain to the information that is needed to properly submit the next
transfer request to the EDMA3TC. Updates that occur while data is moved within a transfer request are
tracked within the transfer controller, and is detailed in Section 11.3.12. For A-synchronized transfers, the
EDMA3CC always submits a TRP for ACNT bytes (BCNT = 1 and CCNT = 1). For AB-synchronized
transfers, the EDMA3CC always submits a TRP for ACNT bytes of BCNT arrays (CCNT = 1). The
EDMA3TC is responsible for updating source and destination addresses within the array based on ACNT
and FWID (in OPT). For AB-synchronized transfers, the EDMA3TC is also responsible to update source
and destination addresses between arrays based on SRCBIDX and DSTBIDX.
Table 11-9 shows the details of parameter updates that occur within EDMA3CC for A-synchronized and
AB-synchronized transfers.


**Table 11-9. Parameter Updates in EDMA3CC (for Non-Null, Non-Dummy PaRAM Set)**

A-Synchronized Transfer                                     AB-Synchronized Transfer
B-Update          C-Update           Link Update            B-Update         C-Update        Link Update
BCNT == 1 &&         BCNT == 1 &&
Condition:            BCNT > 1         CCNT > 1            CCNT == 1                 N/A           CCNT > 1        CCNT == 1
SRC                  += SRCBIDX       += SRCCIDX        = Link.SRC              in EDMA3TC        += SRCCIDX    = Link.SRC
DST                  += DSTBIDX       += DSTCIDX        = Link.DST              in EDMA3TC        += DSTCIDX    = Link.DST
ACNT                    None              None          = Link.ACNT                 None             None       = Link.ACNT
BCNT                     –= 1          = BCNTRLD        = Link.BCNT             in EDMA3TC            N/A       = Link.BCNT
CCNT                    None               –= 1         = Link.CCNT             in EDMA3TC            –=1       = Link.CCNT
SRCBIDX                 None              None          = Link.SRCBIDX          in EDMA3TC           None       = Link.SRCBIDX
DSTBIDX                 None              None          = Link.DSTBIDX              None             None       = Link.DSTBIDX
SRCCIDX                 None              None          = Link.SRCBIDX          in EDMA3TC           None       = Link.SRCBIDX
DSTCIDX                 None              None          = Link.DSTBIDX              None             None       = Link.DSTBIDX
LINK                    None              None          = Link.LINK                 None             None       = Link.LINK
BCNTRLD                 None              None          = Link.BCNTRLD              None             None       = Link.BCNTRLD
(1)
OPT                     None              None          = LINK.OPT                  None             None       = LINK.OPT
(1)
In all cases, no updates occur if OPT.STATIC == 1 for the current PaRAM set.


NOTE: The EDMA3CC includes no special hardware to detect when an indexed address update
calculation overflows/underflows. The address update will wrap across boundaries as
programmed by the user. You should ensure that no transfer is allowed to cross internal port
boundaries between peripherals. A single TR must target a single source/destination slave
endpoint.


#### 11.3.3.7 Linking Transfers

The EDMA3CC provides a mechanism known as linking, which allows the entire PaRAM set to be
reloaded from a location within the PaRAM memory map (for both DMA and QDMA channels). Linking is
especially useful for maintaining ping-pong buffers, circular buffering, and repetitive/continuous transfers
with no CPU intervention. Upon completion of a transfer, the current transfer parameters are reloaded with
the parameter set pointed that the 16-bit link address field of the current parameter set points to. Linking
only occurs when the STATIC bit in OPT is cleared.

NOTE: You should always link a transfer (EDMA3 or QDMA) to another useful transfer. If you must
terminate a transfer, then you should link the transfer to a NULL parameter set. See
Section 11.3.3.3.

The link update occurs after the current PaRAM set event parameters have been exhausted. An event's
parameters are exhausted when the EDMA3 channel controller has submitted all of the transfers that are
associated with the PaRAM set.
A link update occurs for null and dummy transfers depending on the state of the STATIC bit in OPT and
the LINK field. In both cases (null or dummy), if the value of LINK is FFFFh, then a null PaRAM set (with
all 0s and LINK set to FFFFh) is written to the current PaRAM set. Similarly, if LINK is set to a value other
than FFFFh, then the appropriate PaRAM location that LINK points to is copied to the current PaRAM set.
Once the channel completion conditions are met for an event, the transfer parameters that are located at
the link address are loaded into the current DMA or QDMA channel’s associated parameter set. This
indicates that the EDMA3CC reads the entire set (eight words) from the PaRAM set specified by LINK and
writes all eight words to the PaRAM set that is associated with the current channel. Figure 11-9 shows an
example of a linked transfer.
Any PaRAM set in the PaRAM can be used as a link/reload parameter set. The PaRAM sets associated
with peripheral synchronization events (see Section 11.3.6) should only be used for linking if the
corresponding events are disabled.
If a PaRAM set location is defined as a QDMA channel PaRAM set (by QCHMAPn), then copying the link
PaRAM set into the current QDMA channel PaRAM set is recognized as a trigger event. It is latched in
QER because a write to the trigger word was performed. You can use this feature to create a linked list of
transfers using a single QDMA channel and multiple PaRAM sets. See Section 11.3.4.2.
Linking to itself replicates the behavior of auto-initialization, thus facilitating the use of circular buffering
and repetitive transfers. After an EDMA3 channel exhausts its current PaRAM set, it reloads all of the
parameter set entries from another PaRAM set, which is initialized with values that are identical to the
original PaRAM set. Figure 11-9 shows an example of a linked to self transfer. Here, the PaRAM set 255
has the link field pointing to the address of parameter set 255 (linked to self).

NOTE: If the STATIC bit in OPT is set for a PaRAM set, then link updates are not performed.


#### 11.3.3.8 Constant Addressing Mode Transfers/Alignment Issues

If either SAM or DAM is set (constant addressing mode), then the source or destination address must be
aligned to a 256-bit aligned address, respectively, and the corresponding BIDX should be an even multiple
of 32 bytes (256 bits). The EDMA3CC does not recognize errors here, but the EDMA3TC asserts an error
if this is not true. See Section 11.3.12.3.

NOTE: The constant addressing (CONST) mode has limited applicability. The EDMA3 should be
configured for the constant addressing mode (SAM/DAM = 1) only if the transfer source or
destination (on-chip memory, off-chip memory controllers, slave peripherals) support the
constant addressing mode. See the device-specific data manual and/or peripheral user`s
guide to verify if the constant addressing mode is supported. If the constant addressing
mode is not supported, the similar logical transfer can be achieved using the increment
(INCR) mode (SAM/DAM =0) by appropriately programming the count and indices values.


#### 11.3.3.9 Element Size

The EDMA3 controller does not use element-size and element-indexing. Instead, all transfers are defined
in terms of all three dimensions: ACNT, BCNT, and CCNT. An element-indexed transfer is logically
achieved by programming ACNT to the size of the element and BCNT to the number of elements that
need to be transferred. For example, if you have 16-bit audio data and 256 audio samples that must be
transferred to a serial port, you can only do this by programming the ACNT = 2 (2 bytes) and BCNT = 256.


**Figure 11-9. Linked Transfer**

(a)    At initialization                                                                                    PaRAM set 3
Byte                     Set                                                                 OPT X
address                     #                 PaRAM                                            SRC X
EDMA Base Address + 4000h            0          Parameter set 0                             BCNT X               ACNT X
EDMA Base Address + 4020h            1          Parameter set 1                                     DST X
EDMA Base Address + 4040h            2          Parameter set 2                           DSTBIDX X      SRCBIDX X
EDMA Base Address + 4060h            3          Parameter set 3                          BCNTRLD X             Link X=5FE0h
DSTCIDX X            SRCCIDX X
Rsvd               CCNT X

PaRAM set 255
OPT Y
EDMA Base Address + 5FC0h              254        Parameter set 254
SRC Y
EDMA Base Address + 5FE0h              255        Parameter set 255
BCNT Y               ACNT Y
DST Y
DSTBIDX Y      SRCBIDX Y
BCNTRLD Y           Link Y=FFFFh
DSTCIDX Y            SRCCIDX Y
Rsvd               CCNT Y
(b)    After completion of PaRAM set 3
(link update)
PaRAM set 3
Byte                     Set                                                                 OPT Y
address                     #                 PaRAM
SRC Y
EDMA Base Address + 4000h            0          Parameter set 0                             BCNT Y               ACNT Y
EDMA Base Address + 4020h            1          Parameter set 1                                     DST Y
EDMA Base Address + 4040h            2          Parameter set 2                           DSTBIDX Y      SRCBIDX Y
EDMA Base Address + 4060h            3          Parameter set 3                          BCNTRLD Y           Link Y=FFFFh
DSTCIDX Y            SRCCIDX Y
Rsvd               CCNT Y
Link
PaRAM set 255
copy
EDMA Base Address + 5FC0h          254        Parameter set 254                                         OPT Y
EDMA Base Address + 5FE0h          255        Parameter set 255                                         SRC Y
BCNT Y               ACNT Y
DST Y
DSTBIDX Y      SRCBIDX Y
(c)    After completion of PaRAM set 51                  1                                     BCNTRLD Y           Link Y=FFFFh
(link to null set)                                                                       DSTCIDX Y            SRCCIDX Y
Byte                     Set                                                      Rsvd               CCNT Y
address                     #                 PaRAM
EDMA Base Address + 4000h           0          Parameter set 0
EDMA Base Address + 4020h           1          Parameter set 1                         PaRAM set 3 (Null PaRAM set)
EDMA Base Address + 4040h           2          Parameter set 2                                           0h
EDMA Base Address + 4060h           3          Parameter set 3                                           0h
0h                 0h
0h
0h                 0h
0h             Link=FFFFh
EDMA Base Address + 5FC0h          254        Parameter set 254                                  0h                 0h
EDMA Base Address + 5FE0h          255        Parameter set 255                                  0h                 0h


**Figure 11-10. Link-to-Self Transfer**

(a)    At initialization                                                                PaRAM set 3
Byte               Set                                                          OPT X
address               #               PaRAM                                       SRC X
EDMA Base Address + 4000h         0          Parameter set 0                      BCNT X              ACNT X
EDMA Base Address + 4020h         1          Parameter set 1                              DST X
EDMA Base Address + 4040h         2          Parameter set 2                    DSTBIDX X      SRCBIDX X
EDMA Base Address + 4060h         3          Parameter set 3                    BCNTRLD X         Link=5FE0h
DSTCIDX X         SRCCIDX X
Rsvd            CCNT X

PaRAM set 255
OPT X
EDMA Base Address + 5FC0h 254               Parameter set 254
SRC X
EDMA Base Address + 5FE0h 255               Parameter set 255
BCNT X              ACNT X
DST X
DSTBIDX Y      SRCBIDX X
BCNTRLD X         Link=5FE0h
DSTCIDX X         SRCCIDX X
Rsvd            CCNT X
(b)    After completion of PaRAM set 3
(link update)
PaRAM set 3
Byte               Set                                                          OPT X
address               #               PaRAM
SRC X
EDMA Base Address + 4000h         0          Parameter set 0                      BCNT X              ACNT X
EDMA Base Address + 4020h         1          Parameter set 1                              DST X
EDMA Base Address + 4040h         2          Parameter set 2                    DSTBIDX X      SRCBIDX X
EDMA Base Address + 4060h         3          Parameter set 3                    BCNTRLD X         Link=5FE0h
DSTCIDX X         SRCCIDX X
Rsvd            CCNT X
Link
PaRAM set 255
update
EDMA Base Address + 5FC0h 254               Parameter set 254                                 OPT X
EDMA Base Address + 5FE0h       255         Parameter set 255                                 SRC X
BCNT X              ACNT X
DST X
DSTBIDX X      SRCBIDX X
(c)    After completion of PaRAM set 127                                           BCNTRLD X         Link=5FE0h
(link to self)                                                              DSTCIDX X         SRCCIDX X
Byte               Set                                               Rsvd            CCNT X
address               #               PaRAM
EDMA Base Address + 4000h      0          Parameter set 0
EDMA Base Address + 4020h      1          Parameter set 1                            PaRAM set 3
EDMA Base Address + 4040h      2          Parameter set 2                                  OPT X
EDMA Base Address + 4060h      3          Parameter set 3                                  SRC X
BCNT X              ACNT X
DST X
DSTBIDX X      SRCBIDX X
BCNTRLD X         Link=5FE0h
EDMA Base Address + 5FC0h 254               Parameter set 254                   DSTCIDX X         SRCCIDX X
EDMA Base Address + 5FE0h 255               Parameter set 255                      Rsvd            CCNT X


### 11.3.4 Initiating a DMA Transfer

There are multiple ways to initiate a programmed data transfer using the EDMA3 channel controller.
Transfers on DMA channels are initiated by three sources.
They are listed as follows:
• Event-triggered transfer request (this is the more typical usage of EDMA3): A peripheral, system, or
externally-generated event triggers a transfer request.
• Manually-triggered transfer request:The CPU to manually triggers a transfer by writing a 1 to the


corresponding bit in the event set register (ESR/ESRH).
•   Chain-triggered transfer request: A transfer is triggered on the completion of another transfer or sub-
transfer.
Transfers on QDMA channels are initiated by two sources. They are as follows:
• Auto-triggered transfer request: Writing to the programmed trigger word triggers a transfer.
• Link-triggered transfer requests: Writing to the trigger word triggers the transfer when linking occurs.


#### 11.3.4.1 DMA Channel


##### 11.3.4.1.1 Event-Triggered Transfer Request

When an event is asserted from a peripheral or device pins, it gets latched in the corresponding bit of the
event register (ER.En = 1). If the corresponding event in the event enable register (EER) is enabled
(EER.En = 1), then the EDMA3CC prioritizes and queues the event in the appropriate event queue. When
the event reaches the head of the queue, it is evaluated for submission as a transfer request to the
transfer controller.
If the PaRAM set is valid (not a NULL set), then a transfer request packet (TRP) is submitted to the
EDMA3TC and the En bit in ER is cleared. At this point, a new event can be safely received by the
EDMA3CC.
If the PaRAM set associated with the channel is a NULL set (see Section 11.3.3.3), then no transfer
request (TR) is submitted and the corresponding En bit in ER is cleared and simultaneously the
corresponding channel bit is set in the event miss register (EMR.En = 1) to indicate that the event was
discarded due to a null TR being serviced. Good programming practices should include cleaning the event
missed error before re-triggering the DMA channel.
When an event is received, the corresponding event bit in the event register is set (ER.En = 1), regardless
of the state of EER.En. If the event is disabled when an external event is received (ER.En = 1 and
EER.En = 0), the ER.En bit remains set. If the event is subsequently enabled (EER.En = 1), then the
pending event is processed by the EDMA3CC and the TR is processed/submitted, after which the ER.En
bit is cleared.
If an event is being processed (prioritized or is in the event queue) and another sync event is received for
the same channel prior to the original being cleared (ER.En != 0), then the second event is registered as a
missed event in the corresponding bit of the event missed register (EMR.En = 1).
See Section 9.2.3, EDMA Event Multiplexing, for a description of how DMA events map to the EDMA
event crossbar. See Section 11.3.20, EDMA Events, for a table of direct and crossbar mapped EDMA
events.


##### 11.3.4.1.2 Manually Triggered Transfer Request

The CPU or any EDMA programmer initiates a DMA transfer by writing to the event set register (ESR).
Writing a 1 to an event bit in the ESR results in the event being prioritized/queued in the appropriate event
queue, regardless of the state of the EER.En bit. When the event reaches the head of the queue, it is
evaluated for submission as a transfer request to the transfer controller.
As in the event-triggered transfers, if the PaRAM set associated with the channel is valid (it is not a null
set) then the TR is submitted to the associated EDMA3TC and the channel can be triggered again.
If the PaRAM set associated with the channel is a NULL set (see Section 11.3.3.3), then no transfer
request (TR) is submitted and the corresponding En bit in ER is cleared and simultaneously the
corresponding channel bit is set in the event miss register (EMR.En = 1) to indicate that the event was
discarded due to a null TR being serviced. Good programming practices should include clearing the event
missed error before re-triggering the DMA channel.
If an event is being processed (prioritized or is in the event queue) and the same channel is manually set
by a write to the corresponding channel bit of the event set register (ESR.En = 1) prior to the original
being cleared (ESR.En = 0), then the second event is registered as a missed event in the corresponding
bit of the event missed register (EMR.En = 1).


##### 11.3.4.1.3 Chain-Triggered Transfer Request

Chaining is a mechanism by which the completion of one transfer automatically sets the event for another
channel. When a chained completion code is detected, the value of which is dictated by the transfer
completion code (TCC[5:0] in OPT of the PaRAM set associated with the channel), it results in the
corresponding bit in the chained event register (CER) to be set (CER.E[TCC] = 1).
Once a bit is set in CER, the EDMA3CC prioritizes and queues the event in the appropriate event queue.
When the event reaches the head of the queue, it is evaluated for submission as a transfer request to the
transfer controller.
As in the event-triggered transfers, if the PaRAM set associated with the channel is valid (it is not a null
set) then the TR is submitted to the associated EDMA3TC and the channel can be triggered again.
If the PaRAM set associated with the channel is a NULL set (see Section 11.3.3.3), then no transfer
request (TR) is submitted and the corresponding En bit in CER is cleared and simultaneously the
corresponding channel bit is set in the event miss register (EMR.En = 1) to indicate that the event was
discarded due to a null TR being serviced. In this case, the error condition must be cleared by you before
the DMA channel can be re-triggered. Good programming practices might include clearing the event
missed error before re-triggering the DMA channel.
If a chaining event is being processed (prioritized or queued) and another chained event is received for
the same channel prior to the original being cleared (CER.En != 0), then the second chained event is
registered as a missed event in the corresponding channel bit of the event missed register (EMR.En = 1).

NOTE: Chained event registers, event registers, and event set registers operate independently. An
event (En) can be triggered by any of the trigger sources (event-triggered, manually-
triggered, or chain-triggered).


#### 11.3.4.2 QDMA Channels


##### 11.3.4.2.1 Auto-triggered and Link-Triggered Transfer Request

QDMA-based transfer requests are issued when a QDMA event gets latched in the QDMA event register
(QER.En = 1). A bit corresponding to a QDMA channel is set in the QDMA event register (QER) when the
following occurs:
• A CPU (or any EDMA3 programmer) write occurs to a PaRAM address that is defined as a QDMA
channel trigger word (programmed in the QDMA channel mapping register (QCHMAPn)) for the
particular QDMA channel and the QDMA channel is enabled via the QDMA event enable register
(QEER.En = 1).
• EDMA3CC performs a link update on a PaRAM set address that is configured as a QDMA channel
(matches QCHMAPn settings) and the corresponding channel is enabled via the QDMA event enable
register (QEER.En = 1).
Once a bit is set in QER, the EDMA3CC prioritizes and queues the event in the appropriate event queue.
When the event reaches the head of the queue, it is evaluated for submission as a transfer request to the
transfer controller.
As in the event-triggered transfers, if the PaRAM set associated with the channel is valid (it is not a null
set) then the TR is submitted to the associated EDMA3TC and the channel can be triggered again.
If a bit is already set in QER (QER.En = 1) and a second QDMA event for the same QDMA channel
occurs prior to the original being cleared, the second QDMA event gets captured in the QDMA event miss
register (QEMR.En = 1).


#### 11.3.4.3 Comparison Between DMA and QDMA Channels

The primary difference between DMA and QDMA channels is the event/channel synchronization. QDMA
events are either auto-triggered or link triggered. auto-triggering allows QDMA channels to be triggered by
CPU(s) with a minimum number of linear writes to PaRAM. Link triggering allows a linked list of transfers
to be executed, using a single QDMA PaRAM set and multiple link PaRAM sets.


A QDMA transfer is triggered when a CPU (or other EDMA3 programmer) writes to the trigger word of the
QDMA channel parameter set (auto-triggered) or when the EDMA3CC performs a link update on a
PaRAM set that has been mapped to a QDMA channel (link triggered). Note that for CPU triggered
(manually triggered) DMA channels, in addition to writing to the PaRAM set, it is required to write to the
event set register (ESR) to kick-off the transfer.
QDMA channels are typically for cases where a single event will accomplish a complete transfer since the
CPU (or EDMA3 programmer) must reprogram some portion of the QDMA PaRAM set in order to re-
trigger the channel. In other words, QDMA transfers are programmed with BCNT = CCNT = 1 for A-
synchronized transfers, and CCNT = 1 for AB-synchronized transfers.
Additionally, since linking is also supported (if STATIC = 0 in OPT) for QDMA transfers, it allows you to
initiate a linked list of QDMAs, so when EDMA3CC copies over a link PaRAM set (including the write to
the trigger word), the current PaRAM set mapped to the QDMA channel will automatically be recognized
as a valid QDMA event and initiate another set of transfers as specified by the linked set.


### 11.3.5 Completion of a DMA Transfer

A parameter set for a given channel is complete when the required number of transfer requests is
submitted (based on receiving the number of synchronization events). The expected number of TRs for a
non-null/non-dummy transfer is shown in Table 11-10 for both synchronization types along with state of
the PaRAM set prior to the final TR being submitted. When the counts (BCNT and/or CCNT) are this
value, the next TR results in a:
• Final chaining or interrupt codes to be sent by the transfer controllers (instead of intermediate).
• Link updates (linking to either null or another valid link set).


**Table 11-10. Expected Number of Transfers for Non-Null Transfer**

Sync Mode           Counts at time 0       Total # Transfers                               Counts prior to final TR
A-synchronized      ACNT                   (BCNT × CCNT ) TRs of ACNT bytes each           BCNT == 1 && CCNT == 1
BCNT
CCNT
AB-synchronized     ACNT                   CCNT TRs for ACNT × BCNT bytes each             CCNT == 1
BCNT
CCNT


You must program the PaRAM OPT field with a specific transfer completion code (TCC) along with the
other OPT fields (TCCHEN, TCINTEN, ITCCHEN, and ITCINTEN bits) to indicate whether the completion
code is to be used for generating a chained event or/and for generating an interrupt upon completion of a
transfer.
The specific TCC value (6-bit binary value) programmed dictates which of the 64-bits in the chain event
register (CER[TCC]) and/or interrupt pending register (IPR[TCC]) is set.
You can also selectively program whether the transfer controller sends back completion codes on
completion of the final transfer request (TR) of a parameter set (TCCHEN or TCINTEN), for all but the
final transfer request (TR) of a parameter set (ITCCHEN or ITCINTEN), or for all TRs of a parameter set
(both). See Section 11.3.8 for details on chaining (intermediate/final chaining) and Section 11.3.9 for
details on intermediate/final interrupt completion.
A completion detection interface exists between the EDMA3 channel controller and transfer controller(s).
This interface sends back information from the transfer controller to the channel controller to indicate that
a specific transfer is completed. Completion of a transfer is used for generating chained events and/or
generating interrupts to the CPU(s).
All DMA/QDMA PaRAM sets must also specify a link address value. For repetitive transfers such as ping-
pong buffers, the link address value should point to another predefined PaRAM set. Alternatively, a non-
repetitive transfer should set the link address value to the null link value. The null link value is defined as
FFFFh. See Section 11.3.3.7 for more details.


NOTE: Any incoming events that are mapped to a null PaRAM set results in an error condition. The
error condition should be cleared before the corresponding channel is used again. See
Section 11.3.3.5.

There are three ways the EDMA3CC gets updated/informed about a transfer completion: normal
completion, early completion, and dummy/null completion. This applies to both chained events and
completion interrupt generation.


#### 11.3.5.1 Normal Completion

In normal completion mode (TCCMODE = 0 in OPT), the transfer or sub-transfer is considered to be
complete when the EDMA3 channel controller receives the completion codes from the EDMA3 transfer
controller. In this mode, the completion code to the channel controller is posted by the transfer controller
after it receives a signal from the destination peripheral. Normal completion is typically used to generate
an interrupt to inform the CPU that a set of data is ready for processing.


#### 11.3.5.2 Early Completion

In early completion mode (TCCMODE = 1 in OPT), the transfer is considered to be complete when the
EDMA3 channel controller submits the transfer request (TR) to the EDMA3 transfer controller. In this
mode, the channel controller generates the completion code internally. Early completion is typically useful
for chaining, as it allows subsequent transfers to be chained-triggered while the previous transfer is still in
progress within the transfer controller, maximizing the overall throughput of the set of the transfers.


#### 11.3.5.3 Dummy or Null Completion

This is a variation of early completion. Dummy or null completion is associated with a dummy set
(Section 11.3.3.4) or null set (Section 11.3.3.3). In both cases, the EDMA3 channel controller does not
submit the associated transfer request to the EDMA3 transfer controller(s). However, if the set
(dummy/null) has the OPT field programmed to return completion code (intermediate/final
interrupt/chaining completion), then it will set the appropriate bits in the interrupt pending registers
(IPR/IPRH) or chained event register (CER/CERH). The internal early completion path is used by the
channel controller to return the completion codes internally (that is, EDMA3CC generates the completion
code).


### 11.3.6 Event, Channel, and PaRAM Mapping

Several of the 64 DMA channels are tied to a specific hardware event, thus allowing events from device
peripherals or external hardware to trigger transfers. A DMA channel typically requests a data transfer
when it receives its event (apart from manually-triggered, chain-triggered, and other transfers). The
amount of data transferred per synchronization event depends on the channel’s configuration (ACNT,
BCNT, CCNT, etc.) and the synchronization type (A-synchronized or AB-synchronized).
The association of an event to a channel is fixed, each DMA channel has one specific event associated
with it. See Section 9.2.3, EDMA Event Multiplexing, for a description of how DMA events map to the
EDMA event crossbar. See Section 11.3.20, EDMA Events, for a table of direct and crossbar mapped
EDMA events.
In an application, if a channel does not use the associated synchronization event or if it does not have an
associated synchronization event (unused), that channel can be used for manually-triggered or chained-
triggered transfers, for linking/reloading, or as a QDMA channel.


#### 11.3.6.1 DMA Channel to PaRAM Mapping

The mapping between the DMA channel numbers and the PaRAM sets is programmable (see Table 11-
5). The DMA channel mapping registers (DCHMAPn) in the EDMA3CC provide programmability that
allows the DMA channels to be mapped to any of the PaRAM sets in the PaRAM memory map. Figure 11-
11 illustrates the use of DCHMAP. There is one DCHMAP register per channel.


**Figure 11-11. DMA Channel and QDMA Channel to PaRAM Mapping**

DCHMAPn                                                            QCHMAPn
31                  14 13              5     4           0    31                    14 13                   5 4             2 1 0
00 0000 0000 0000 0000       PAENTRY             00000         00 0000 0000 0000 0000          PAENTRY        TR WORD 00


Byte
Byte               Set                                                                        address
address              #            PaRAM                                  PaRAM set              offset
EDMA Base Address 4000h        0         Parameter set 0                              OPT                 +0h
EDMA Base Address 4020h        1         Parameter set 1                              SRC                 +4h
EDMA Base Address 4040h        2         Parameter set 2                       BCNT         ACNT          +8h
EDMA Base Address 4060h        3         Parameter set 3                              DST                 +Ch
DSTBIDX       SRCBIDX          +10h
BCNTRLD           LINK          +14h
DSTCIDX       SRCCIDX          +18h
Reserved         CCNT
+1Ch


EDMA Base Address 5FC0h       254       Parameter set 254
EDMA Base Address 5FE0h       255       Parameter set 255


#### 11.3.6.2 QDMA Channel to PaRAM Mapping

The mapping between the QDMA channels and the PaRAM sets is programmable. The QDMA channel
mapping register (QCHMAP) in the EDMA3CC allows you to map the QDMA channels to any of the
PaRAM sets in the PaRAM memory map. Figure 11-12 illustrates the use of QCHMAP.
Additionally, QCHMAP allows you to program the trigger word in the PaRAM set for the QDMA channel. A
trigger word is one of the eight words in the PaRAM set. For a QDMA transfer to occur, a valid TR
synchronization event for EDMA3CC is a write to the trigger word in the PaRAM set pointed to by
QCHMAP for a particular QDMA channel. By default, QDMA channels are mapped to PaRAM set 0. You
must appropriately re-map PaRAM set 0 before you use it.


**Figure 11-12. QDMA Channel to PaRAM Mapping**

31                                             14 13                               5 4              2 1       0
PAENTR Y                    TR WORD
QCHMAPn                0000 0000 0000 00                                                                           00
00 0000 01 1                    1 11

Byte           Set
address           #             PaRAM                                                                Byte
EDMA Base Address + 4000h     0          Parameter set 0                                                        address
EDMA Base Address + 4020h     1          Parameter set 1                                                         offset
PaRAM set
EDMA Base Address + 4040h     2          Parameter set 2
EDMA Base Address + 4060h     3          Parameter set 3                                        OPT               +0h
SRC               +4h
BCNT            ACNT      +8h
DST               +Ch
DSTBIDX          SRCBIDX     +10h
BCNTRLD            LINK      +14h
DSTCIDX          SRCCIDX     +18h
Rsvd            CCNT       +1Ch


EDMA Base Address + 5FC0h 254           Parameter set 254
EDMA Base Address + 5FE0h 255           Parameter set 255


### 11.3.7 EDMA3 Channel Controller Regions

The EDMA3 channel controller divides its address space into eight regions. Individual channel resources
are assigned to a specific region, where each region is typically assigned to a specific EDMA programmer.
You can design the application software to use regions or to ignore them altogether. You can use active
memory protection in conjunction with regions so that only a specific EDMA programmer (for example,
privilege identification) or privilege level (for example, user vs. supervisor) is allowed access to a given
region, and thus to a given DMA or QDMA channel. This allows robust system-level DMA code where
each EDMA programmer only modifies the state of the assigned resources. Memory protection is
described in Section 11.3.10.


#### 11.3.7.1 Region Overview

The EDMA3 channel controller memory-mapped registers are divided in three main categories:
1. Global registers
2. Global region channel registers
3. Shadow region channel registers
The global registers are located at a single/fixed location in the EDMA3CC memory map. These registers
control EDMA3 resource mapping and provide debug visibility and error tracking information.
The channel registers (including DMA, QDMA, and interrupt registers) are accessible via the global
channel region address range, or in the shadow n channel region address range(s). For example, the
event enable register (EER) is visible at the global address of EDMA Base Address + 1020h or region
addresses of EDMA Base Address + 2020h for region 0, EDMA Base Address + 2220h for region 1, …
EDMA Base Address + 2E20h for region 7.
The DMA region access enable registers (DRAEm) and the QDMA region access enable registers
(QRAEn) control the underlying control register bits that are accessible via the shadow region address
space (except for IEVALn). Table 11-11 lists the registers in the shadow region memory map. See the
EDMA3CC memory map () for the complete global and shadow region memory maps. Figure 11-13
illustrates the conceptual view of the regions.


**Table 11-11. Shadow Region Registers**

DRAEm                     DRAEHm                     QRAEn
ER                      ERH                        QER
ECR                      ECRH                      QEER
ESR                      ESRH                     QEECR
CER                      CERH                     QEESR
EER                      EERH
EECR                    EECRH
EESR                    EESRH
SER                      SERH
SECR                    SECRH
IER                     IERH
IECR                    IECRH
IESR                    IESRH
IPR                     IPRH
ICR                      ICRH
Register not affected by DRAE\DRAEH
IEVAL


**Figure 11-13. Shadow Region Registers**

Shadow region 0                           Physical register

ER, ERH                                                         EDMA
Access address                                                             ER, ERH            Base
DRAE0/                    ECR, ECRH           Address + 1000h
EDMA Base Address + 2000h
DRAE0H                    ESR, ESRH
EDMA Base Address + 2094h
QSECR              QRAE0                    CER, CERH
except IEV AL
EER, EERH
IEVAL
EECR, EECRH
Shadow region 0                           EESR, EESRH
registers                              SER, SERH
SECR, SECRH

IER, IERH
IECR,
IESR,
IPR,
ICR,
IEVAL,

QER
ER, ERH                                           QEER
Access address                                 DRAE7/                          QEECR
EDMA Base Address + 2E00h                                 DRAE7H                          QEESR
EDMA Base Address + 2E94h               QSECR              QRAE7                          QSER          EDMA
QSECR         Base
IEVAL                                                          Address + 1094h
Shadow region 7
registers


**Table 11-12. EDMA Shadow Regions**

EDMA Address Offset                                                    Description
0x2000-0x2094                                          Shadow Region 0 Channel Registers
0x2200-0x2294                                          Shadow Region 1 Channel Registers


1598Enhanced Direct Memory Access (EDMA)                                            SPRUH73Q – October 2011 – Revised December 2019


**Table 11-12. EDMA Shadow Regions (continued)**

EDMA Address Offset                                                    Description
0x2400-0x2494                                          Shadow Region 2 Channel Registers
0x2600-0x2694                                          Shadow Region 3 Channel Registers
0x2800-0x2894                                     Shadow Region 4-7 Channel Registers unused


#### 11.3.7.2 Channel Controller Regions

There are eight EDMA3 shadow regions (and associated memory maps). Associated with each shadow
region are a set of registers defining which channels and interrupt completion codes belong to that region.
These registers are user-programmed per region to assign ownership of the DMA/QDMA channels to a
region.
• DRAEm and DRAEHm: One register pair exists for each of the shadow regions. The number of bits in
each register pair matches the number of DMA channels (64 DMA channels). These registers need to
be programmed to assign ownership of DMA channels and interrupt (or TCC codes) to the respective
region. Accesses to DMA and interrupt registers via the shadow region address view are filtered
through the DRAE/DRAEH pair. A value of 1 in the corresponding DRAE(H) bit implies that the
corresponding DMA/interrupt channel is accessible; a value of 0 in the corresponding DRAE(H) bit
forces writes to be discarded and returns a value of 0 for reads.
• QRAEn: One register exists for every region. The number of bits in each register matches the number
of QDMA channels (4 QDMA channels). These registers must be programmed to assign ownership of
QDMA channels to the respective region. To enable a channel in a shadow region using shadow
region 0 QEER, the respective bit in QRAE must be set or writing into QEESR will not have the desired
effect.
• MPPAn and MPPAG: One register exists for every region. This register defines the privilege level,
requestor, and types of accesses allowed to a region's memory-mapped registers.
It is typical for an application to have a unique assignment of QDMA/DMA channels (and, therefore, a
given bit position) to a given region.
The use of shadow regions allows for restricted access to EDMA3 resources (DMA channels, QDMA
channels, TCC, interrupts) by tasks in a system by setting or clearing bits in the DRAE/ORAE registers. If
exclusive access to any given channel / TCC code is required for a region, then only that region's
DRAE/ORAE should have the associated bit set.

Example 11-1. Resource Pool Division Across Two Regions

This example illustrates a judicious resource pool division across two regions, assuming region 0 must be
allocated 16 DMA channels (0-15) and 1 QDMA channel (0) and 32 TCC codes (0-15 and 48-63). Region 1
needs to be allocated 16 DMA channels (16-32) and the remaining 7 QDMA channels (1-7) and TCC codes
(16-47). DRAE should be equal to the OR of the bits that are required for the DMA channels and the TCC
codes:
Region 0: DRAEH, DRAE = 0xFFFF0000, 0x0000FFFF QRAE = 0x0000001
Region 1: DRAEH, DRAE = 0x0000FFFF, 0xFFFF0000 QRAE = 0x00000FE


#### 11.3.7.3 Region Interrupts

In addition to the EDMA3CC global completion interrupt, there is an additional completion interrupt line
that is associated with every shadow region. Along with the interrupt enable register (IER), DRAE acts as
a secondary interrupt enable for the respective shadow region interrupts. See Section 11.3.9 for more
information.


### 11.3.8 Chaining EDMA3 Channels

The channel chaining capability for the EDMA3 allows the completion of an EDMA3 channel transfer to
trigger another EDMA3 channel transfer. The purpose is to allow you the ability to chain several events
through one event occurrence.
Chaining is different from linking (Section 11.3.3.7). The EDMA3 link feature reloads the current channel
parameter set with the linked parameter set. The EDMA3 chaining feature does not modify or update any
channel parameter set; it provides a synchronization event to the chained channel (see Section 11.3.4.1.3
for chain-triggered transfer requests).
Chaining is achieved at either final transfer completion or intermediate transfer completion, or both, of the
current channel. Consider a channel m (DMA/QDMA) required to chain to channel n. Channel number n
(0-63) needs to be programmed into the TCC bit of channel m channel options parameter (OPT) set.
• If final transfer completion chaining (TCCHEN = 1 in OPT) is enabled, the chain-triggered event occurs
after the submission of the last transfer request of channel m is either submitted or completed
(depending on early or normal completion).
• If intermediate transfer completion chaining (ITCCHEN = 1 in OPT) is enabled, the chain-triggered
event occurs after every transfer request, except the last of channel m is either submitted or completed
(depending on early or normal completion).
• If both final and intermediate transfer completion chaining (TCCHEN = 1 and ITCCHEN = 1 in OPT)
are enabled, then the chain-trigger event occurs after every transfer request is submitted or completed
(depending on early or normal completion).
Table 11-13 illustrates the number of chain event triggers occurring in different synchronized scenarios.
Consider channel 31 programmed with ACNT = 3, BCNT = 4, CCNT = 5, and TCC = 30.


**Table 11-13. Chain Event Triggers**

(Number of chained event triggers on channel 30)
Options                                A-Synchronized                                  AB-Synchronized
TCCHEN = 1, ITCCHEN = 0                1 (Owing to the last TR)                        1 (Owing to the last TR)
TCCHEN = 0, ITCCHEN = 1                19 (Owing to all but the last TR)               4 (Owing to all but the last TR)
TCCHEN = 1, ITCCHEN = 1                20 (Owing to a total of 20 TRs)                 5 (Owing to a total of 5 TRs)


### 11.3.9 EDMA3 Interrupts

The EDMA3 interrupts are divided into 2 categories: transfer completion interrupts and error interrupts.
There are nine region interrupts, eight shadow regions and one global region. The transfer completion
interrupts are listed in Table 11-14. The transfer completion interrupts and the error interrupts from the
transfer controllers are all routed to the ARM interrupt controllers.


**Table 11-14. EDMA3 Transfer Completion Interrupts**

Name                       Description
EDMA3CC_INT0               EDMA3CC Transfer Completion Interrupt Shadow Region 0
EDMA3CC_INT1               EDMA3CC Transfer Completion Interrupt Shadow Region 1
EDMA3CC_INT2               EDMA3CC Transfer Completion Interrupt Shadow Region 2
EDMA3CC_INT3               EDMA3CC Transfer Completion Interrupt Shadow Region 3
EDMA3CC_INT4               EDMA3CC Transfer Completion Interrupt Shadow Region 4
EDMA3CC_INT5               EDMA3CC Transfer Completion Interrupt Shadow Region 5
EDMA3CC_INT6               EDMA3CC Transfer Completion Interrupt Shadow Region 6
EDMA3CC_INT7               EDMA3CC Transfer Completion Interrupt Shadow Region 7


**Table 11-15. EDMA3 Error Interrupts**

Name                            Description
EDMA3CC_ERRINT                  EDMA3CC Error Interrupt
EDMA3CC_MPINT                   EDMA3CC Memory Protection Interrupt
EDMA3TC0_ERRINT                 TC0 Error Interrupt
EDMA3TC1_ERRINT                 TC1 Error Interrupt
EDMA3TC2_ERRINT                 TC2 Error Interrupt


#### 11.3.9.1 Transfer Completion Interrupts

The EDMA3CC is responsible for generating transfer completion interrupts to the CPU(s) (and other
EDMA3 masters). The EDMA3 generates a single completion interrupt per shadow region, as well as one
for the global region on behalf of all 64 channels. The various control registers and bit fields facilitate
EDMA3 interrupt generation.
The software architecture should either use the global interrupt or the shadow interrupts, but not both.
The transfer completion code (TCC) value is directly mapped to the bits of the interrupt pending register
(IPR/IPRH). For example, if TCC = 10 0001b, IPRH[1] is set after transfer completion, and results in
interrupt generation to the CPU(s) if the completion interrupt is enabled for the CPU. See
Section 11.3.9.1.1 for details on enabling EDMA3 transfer completion interrupts.
When a completion code is returned (as a result of early or normal completions), the corresponding bit in
IPR/IPRH is set if transfer completion interrupt (final/intermediate) is enabled in the channel options
parameter (OPT) for a PaRAM set associated with the transfer.


**Table 11-16. Transfer Complete Code (TCC) to EDMA3CC Interrupt Mapping**

TCC Bits in OPT                                                     TCC Bits in OPT
IPR Bit Set                                                    IPRH Bit Set (1)
(TCINTEN/ITCINTEN = 1)                                              (TCINTEN/ITCINTEN = 1)
0                               IPR0                                20h                      IPR32/IPRH0
1                               IPR1                                21h                      IPR33/IPRH1
2h                              IPR2                                22h                      IPR34/IPRH2
3h                              IPR3                                23h                      IPR35/IPRH3
4h                              IPR4                                24h                      IPR36/IPRH4
...                               ...                                ...                           ...
1Eh                               IPR30                               3Eh                     IPR62/IPRH30
1Fh                               IPR31                               3Fh                     IPR63/IPRH31
(1)
Bit fields IPR[32-63] correspond to bits 0 to 31 in IPRH, respectively.


You can program the transfer completion code (TCC) to any value for a DMA/QDMA channel. A direct
relation between the channel number and the transfer completion code value does not need to exist. This
allows multiple channels having the same transfer completion code value to cause a CPU to execute the
same interrupt service routine (ISR) for different channels.
If the channel is used in the context of a shadow region and you intend for the shadow region interrupt to
be asserted, then ensure that the bit corresponding to the TCC code is enabled in IER/IERH and in the
corresponding shadow region's DMA region access registers (DRAE/DRAEH).
You can enable Interrupt generation at either final transfer completion or intermediate transfer completion,
or both. Consider channel m as an example.
• If the final transfer interrupt (TCCINT = 1 in OPT) is enabled, the interrupt occurs after the last transfer
request of channel m is either submitted or completed (depending on early or normal completion).
• If the intermediate transfer interrupt (ITCCINT = 1 in OPT) is enabled, the interrupt occurs after every
transfer request, except the last TR of channel m is either submitted or completed (depending on early
or normal completion).
• If both final and intermediate transfer completion interrupts (TCCINT = 1, and ITCCINT = 1 in OPT) are

enabled, then the interrupt occurs after every transfer request is submitted or completed (depending on
early or normal completion).
Table 11-17 shows the number of interrupts that occur in different synchronized scenarios. Consider
channel 31, programmed with ACNT = 3, BCNT = 4, CCNT = 5, and TCC = 30.


**Table 11-17. Number of Interrupts**

Options                                A-Synchronized                                  AB-Synchronized
TCINTEN = 1, ITCINTEN = 0              1 (Last TR)                                     1 (Last TR)
TCINTEN = 0, ITCINTEN = 1              19 (All but the last TR)                        4 (All but the last TR)
TCINTEN = 1, ITCINTEN = 1              20 (All TRs)                                    5 (All TRs)


##### 11.3.9.1.1 Enabling Transfer Completion Interrupts

For the EDMA3 channel controller to assert a transfer completion to the external environment, the
interrupts must be enabled in the EDMA3CC. This is in addition to setting up the TCINTEN and ITCINTEN
bits in OPT of the associated PaRAM set.
The EDMA3 channel controller has interrupt enable registers (IER/IERH) and each bit location in
IER/IERH serves as a primary enable for the corresponding interrupt pending registers (IPR/IPRH).
All of the interrupt registers (IER, IESR, IECR, and IPR) are either manipulated from the global DMA
channel region, or by the DMA channel shadow regions. The shadow regions provide a view to the same
set of physical registers that are in the global region.
The EDMA3 channel controller has a hierarchical completion interrupt scheme that uses a single set of
interrupt pending registers (IPR/IPRH) and single set of interrupt enable registers (IER/IERH). The
programmable DMA region access enable registers (DRAE/DRAEH) provides a second level of interrupt
masking. The global region interrupt output is gated based on the enable mask that is provided by
IER/IERH. see Figure 11-14
The region interrupt outputs are gated by IER and the specific DRAE/DRAEH associated with the region.
See Figure 11-14.


**Figure 11-14. Interrupt Diagram**

Interrupt pending
register (IPR)

X                  1    0

Interrupt
enable                             DMA region                                       DMA region
register                         access enable 1                                  access enable 7
(IER)                              (DRAE1)                                          (DRAE7)

X                  1    0            X              1      0                          X              1      0


IEVAL0.EVAL                      IEVAL1.EVAL                                     IEVAL7.EVAL


Eval                                Eval                                             Eval
pulse                               pulse                                            pulse


EDMA3CC_INT0                       EDMA3CC_INT1                                       EDMA3CC_INT7


For the EDMA3CC to generate the transfer completion interrupts that are associated with each shadow
region, the following conditions must be true:
• EDMA3CC_INT0: (IPR.E0 & IER.E0 & DRAE0.E0) | (IPR.E1 & IER.E1 & DRAE0.E1) | …|(IPRH.E63 &
IERH.E63 & DRAHE0.E63)
• EDMA3CC_INT1: (IPR.E0 & IER.E0 & DRAE1.E0) | (IPR.E1 & IER.E1 & DRAE1.E1) | …| (IPRH.E63
& IERH.E63 & DRAHE1.E63)
• EDMA3CC_INT2 : (IPR.E0 & IER.E0 & DRAE2.E0) | (IPR.E1 & IER.E1 & DRAE2.E1) | …|(IPRH.E63
& IERH.E63 & DRAHE2.E63)....
• Up to EDMA3CC_INT7 : (IPR.E0 & IER.E0 & DRAE7.E0) | (IPR.E1 & IER.E1 & DRAE7.E1)
| …|(IPRH.E63 & IERH.E63 & DRAEH7.E63)

NOTE: The DRAE/DRAEH for all regions are expected to be set up at system initialization and to
remain static for an extended period of time. The interrupt enable registers should be used
for dynamic enable/disable of individual interrupts.
Because there is no relation between the TCC value and the DMA/QDMA channel, it is
possible, for example, for DMA channel 0 to have the OPT.TCC = 63 in its associated
PaRAM set. This would mean that if a transfer completion interrupt is enabled
(OPT.TCINTEN or OPT.ITCINTEN is set), then based on the TCC value, IPRH.E63 is set up
on completion. For proper channel operations and interrupt generation using the shadow
region map, you must program the DRAE/DRAEH that is associated with the shadow region
to have read/write access to both bit 0 (corresponding to channel 0) and bit 63
(corresponding to IPRH bit that is set upon completion).


##### 11.3.9.1.2 Clearing Transfer Completion Interrupts

Transfer completion interrupts that are latched to the interrupt pending registers (IPR/IPRH) are cleared by
writing a 1 to the corresponding bit in the interrupt pending clear register (ICR/ICRH). For example, a write
of 1 to ICR.E0 clears a pending interrupt in IPR.E0.
If an incoming transfer completion code (TCC) gets latched to a bit in IPR/IPRH, then additional bits that
get set due to a subsequent transfer completion will not result in asserting the EDMA3CC completion
interrupt. In order for the completion interrupt to be pulsed, the required transition is from a state where no
enabled interrupts are set to a state where at least one enabled interrupt is set.


#### 11.3.9.2 EDMA3 Interrupt Servicing

Upon completion of a transfer (early or normal completion), the EDMA3 channel controller sets the
appropriate bit in the interrupt pending registers (IPR/IPRH), as the transfer completion codes specify. If
the completion interrupts are appropriately enabled, then the CPU enters the interrupt service routine
(ISR) when the completion interrupt is asserted.
After servicing the interrupt, the ISR should clear the corresponding bit in IPR/IPRH, thereby enabling
recognition of future interrupts. The EDMA3CC will only assert additional completion interrupts when all
IPR/IPRH bits clear.
When one interrupt is serviced many other transfer completions may result in additional bits being set in
IPR/IPRH, thereby resulting in additional interrupts. Each of the bits in IPR/IPRH may need different types
of service; therefore, the ISR may check all pending interrupts and continue until all of the posted
interrupts are serviced appropriately.
Examples of pseudo code for a CPU interrupt service routine for an EDMA3CC completion interrupt are
shown in Example 11-2 and Example 11-3.
The ISR routine in Example 11-2 is more exhaustive and incurs a higher latency.

Example 11-2. Interrupt Servicing

The pseudo code:
1. Reads the interrupt pending register (IPR/IPRH).
2. Performs the operations needed.
3. Writes to the interrupt pending clear register (ICR/ICRH) to clear the corresponding IPR/IPRH bit(s).
4. Reads IPR/IPRH again:
a. If IPR/IPRH is not equal to 0, repeat from step 2 (implies occurrence of new event between step 2 to
step 4).
b. If IPR/IPRH is equal to 0, this should assure you that all of the enabled interrupts are inactive.

NOTE: An event may occur during step 4 while the IPR/IPRH bits are read as 0 and the application is
still in the interrupt service routine. If this happens, a new interrupt is recorded in the device
interrupt controller and a new interrupt generates as soon as the application exits in the interrupt
service routine.


Example 11-3 is less rigorous, with less burden on the software in polling for set interrupt bits, but can
occasionally cause a race condition as mentioned above.

Example 11-3. Interrupt Servicing

If you want to leave any enabled and pending (possibly lower priority) interrupts; you must force the interrupt
logic to reassert the interrupt pulse by setting the EVAL bit in the interrupt evaluation register (IEVAL).
The pseudo code is as follows:
1. Enters ISR.


Example 11-3. Interrupt Servicing (continued)
2. Reads IPR/IPRH.
3. For the condition that is set in IPR/IPRH that you want to service, do the following:
a. Service interrupt as the application requires.
b. Clear the bit for serviced conditions (others may still be set, and other transfers may have resulted in
returning the TCC to EDMA3CC after step 2).
4. Reads IPR/IPRH prior to exiting the ISR:
a. If IPR/IPRH is equal to 0, then exit the ISR.
b. If IPR/IPRH is not equal to 0, then set IEVAL so that upon exit of ISR, a new interrupt triggers if any
enabled interrupts are still pending.


#### 11.3.9.3 Interrupt Evaluation Operations

The EDMA3CC has interrupt evaluate registers (IEVAL) that exist in the global region and in each shadow
region. The registers in the shadow region are the only registers in the DMA channel shadow region
memory map that are not affected by the settings for the DMA region access enable registers
(DRAE/DRAEH). Writing a 1 to the EVAL bit in the registers that are associated with a particular shadow
region results in pulsing the associated region interrupt (global or shadow), if any enabled interrupt (via
IER/IERH) is still pending (IPR/IPRH). This register assures that the CPU does not miss the interrupts (or
the EDMA3 master associated with the shadow region) if the software architecture chooses not to use all
interrupts. See Example 11-3 for the use of IEVAL in the EDMA3 interrupt service routine (ISR).
Similarly, an error evaluation register (EEVAL) exists in the global region. Writing a 1 to the EVAL bit in
EEVAL causes the pulsing of the error interrupt if any pending errors are in EMR/EMRH, QEMR, or
CCERR. The EVAL bit must be written with 1 to clear interrupts to the INTC, even when all error interrupt
registers are cleared. See Section 11.3.9.4, Error Interrupts, for additional information regarding error
interrupts.

NOTE: While using IEVAL for shadow region completion interrupts, ensure that the IEVAL operated
upon is from that particular shadow region memory map.


#### 11.3.9.4 Error Interrupts

The EDMA3CC error registers provide the capability to differentiate error conditions (event missed,
threshold exceed, etc.). Additionally, setting the error bits in these registers results in asserting the
EDMA3CC error interrupt. If the EDMA3CC error interrupt is enabled in the device interrupt controller(s),
then it allows the CPU(s) to handle the error conditions.
The EDMA3CC has a single error interrupt (EDMA3CC_ERRINT) that is asserted for all EDMA3CC error
conditions. There are four conditions that cause the error interrupt to pulse:
• DMA missed events: for all 64 DMA channels. DMA missed events are latched in the event missed
registers (EMR/EMRH).
• QDMA missed events: for all 8 QDMA channels. QDMA missed events are latched in the QDMA event
missed register (QEMR).
• Threshold exceed: for all event queues. These are latched in EDMA3CC error register (CCERR).
• TCC error: for outstanding transfer requests that are expected to return completion code (TCCHEN or
TCINTEN bit in OPT is set to 1) exceeding the maximum limit of 63. This is also latched in the
EDMA3CC error register (CCERR).
Figure 11-15 illustrates the EDMA3CC error interrupt generation operation.
If any of the bits are set in the error registers due to any error condition, the EDMA3CC_ERRINT is always
asserted, as there are no enables for masking these error events. Similar to transfer completion interrupts
(EDMA3CC_INT), the error interrupt also only pulses when the error interrupt condition transitions from no
errors being set to at least one error being set. If additional error events are latched prior to the original
error bits clearing, the EDMA3CC does not generate additional interrupt pulses.
To reduce the burden on the software, there is an error evaluate register (EEVAL) that allows re-
evaluation of pending set error events/bits, similar to the interrupt evaluate register (IEVAL). You can use
this so that the CPU(s) does not miss any error events. You must write a 1 to the EEVAL.EVAL bit to clear
interrupts to the INTC after all error registers have been cleared.

NOTE: It is good practice to enable the error interrupt in the device interrupt controller and to
associate an interrupt service routine with it to address the various error conditions
appropriately. Doing so puts less burden on the software (polling for error status);
additionally, it provides a good debug mechanism for unexpected error conditions.


**Figure 11-15. Error Interrupt Operation**

EMR/EMRH                                  QEMR                                CCERR
63                                   1    0         7    ...   1    0        16    .......    3   2   1   0


EEVAL.EVAL

Eval/
pulse


EDMACC_ERRINT


### 11.3.10 Memory Protection

The EDMA3 channel controller supports two kinds of memory protection: active and proxy.


#### 11.3.10.1 Active Memory Protection

Active memory protection is a feature that allows or prevents read and write accesses (by any EDMA3
programmer) to the EDMA3CC registers (based on permission characteristics that you program). Active
memory protection is achieved by a set of memory protection permissions attribute (MPPA) registers.
The EDMA3CC register map is divided into three categories:
• a global region.
• a global channel region.
• eight shadow regions.
Each shadow region consists of the respective shadow region registers and the associated PaRAM. For
more detailed information regarding the contents of a shadow region, refer to section Table 11-11.
Each of the eight shadow regions has an associated MPPA register (MPPAn) that defines the specific
requestor(s) and types of requests that are allowed to the regions resources.
The global channel region is also protected with a memory-mapped register (MPPAG). The MPPAG
applies to the global region and to the global channel region, except the other MPPA registers themselves.
For more detailed information on the list of the registers in each region, refer to the register memory-map
in section Table 11-19.
See for the bit field descriptions of MPPAn. The MPPAn have a certain set of access rules.
Table 11-18 shows the accesses that are allowed or not allowed to the MPPAG and MPPAn. The active
memory protection uses the PRIV and PRIVID attributes of the EDMA programmer. The PRIV is the
privilege level (i.e., user vs. supervisor). The PRIVID refers to a privilege ID with a number that is
associated with an EDMA3 programmer. See the device-specific data manual for the PRIVIDs that are
associated with potential EDMA3 programmers.


**Table 11-18. Allowed Accesses**

Access                   Supervisor                User
Read                     Yes                       Yes
Write                    Yes                       No


Table 11-19 describes the MPPA register mapping for the shadow regions (which includes shadow region
registers and PaRAM addresses).
The region-based MPPA registers are used to protect accesses to the DMA shadow regions and the
associated region PaRAM. Because there are eight regions, there are eight MPPA region registers
(MPPA[0-7]).


**Table 11-19. MPPA Registers to Region Assignment**

Register                    Registers Protect         Address Range                 PaRAM Protect (1)     Address Range
MPPAG                       Global Range              0000h-1FFCh                   N/A                   N/A
MPPA0                       DMA Shadow 0              2000h-21FCh                   1st octant            4000h-47FCh
MPPA1                       DMA Shadow 1              2200h-23FCh                   2nd octant            4800h-4FFCh
MPPA2                       DMA Shadow 2              2400h-25FCh                   3rd octant            5000h-57FCh
MPPA3                       DMA Shadow 3              2600h-27FCh                   4th octant            5800h-5FFCh
MPPA4                       DMA Shadow 4              2800h-29FCh                   5th octant            6000h-67FCh
MPPA5                       DMA Shadow 5              2A00h-2BFCh                   6th octant            6800h-6FFCh
MPPA6                       DMA Shadow 6              2C00h-2DFCh                   7th octant            7000h-77FCh
MPPA7                       DMA Shadow 7              2E00h-2FFCh                   8th octant            7800h-7FFCh
(1)
The PARAM region is divided into 8 regions referred to as an octant.


Example Access denied.
Write access to shadow region 7's event enable set register (EESR):
1. The original value of the event enable register (EER) at address offset 0x1020 is 0x0.
2. The MPPA[7] is set to prevent user level accesses (UW = 0, UR = 0), but it allows supervisor level
accesses (SW = 1, SR = 1) with a privilege ID of 0. (AID0 = 1).
3. An EDMA3 programmer with a privilege ID of 0 attempts to perform a user-level write of a value of
0xFF00FF00 to shadow region 7's event enable set register (EESR) at address offset 0x2E30. Note
that the EER is a read-only register and the only way that you can write to it is by writing to the EESR.
Also remember that there is only one physical register for EER, EESR, etc. and that the shadow
regions only provide to the same physical set.
4. Since the MPPA[7] has UW = 0, though the privilege ID of the write access is set to 0, the access is
not allowed and the EER is not written to.


**Table 11-20. Example Access Denied**

Register                 Value           Description
EER                   0x0000 0000        Value in EER to begin with.
(offset 0x1020)
EESR                 0xFF00 FF00         Value attempted to be written to shadow region 7's EESR.
(offset 0x2E30)           ↓              This is done by an EDMA3 programmer with a privilege level of User and Privilege ID
of 0.
MPPA[7]               0x0000 04B0        Memory Protection Filter AID0 = 1, UW = 0, UR = 0, SW = 1, SR = 1.
(offset 0x082C)
X             Access Denied
EER                   0x0000 0000        Final value of EER
(offset 0x1020)


Example Access Allowed
Write access to shadow region 7's event enable set register (EESR):
1. The original value of the event enable register (EER) at address offset 0x1020 is 0x0.
2. The MPPA[7] is set to allow user-level accesses (UW = 1, UR = 1) and supervisor-level accesses (SW
= 1, SR = 1) with a privilege ID of 0. (AID0 = 1).
3. An EDMA3 programmer with a privilege ID of 0, attempts to perform a user-level write of a value of
0xABCD0123 to shadow region 7's event enable set register (EESR) at address offset 0x2E30. Note
that the EER is a read-only register and the only way that you can write to it is by writing to the EESR.
Also remember that there is only one physical register for EER, EESR, etc. and that the shadow
regions only provide to the same physical set.
4. Since the MPPA[7] has UW = 1 and AID0 = 1, the user-level write access is allowed.
5. Remember that accesses to shadow region registers are masked by their respective DRAE register. In
this example, the DRAE[7] is set of 0x9FF00FC2.
6. The value finally written to EER is 0x8BC00102.


1608Enhanced Direct Memory Access (EDMA)                                             SPRUH73Q – October 2011 – Revised December 2019


**Table 11-21. Example Access Allowed**

Register                    Value         Description
EER                      0x0000 0000      Value in EER to begin with.
(offset 0x1020)
EESR                     0xFF00 FF00      Value attempted to be written to shadow region 7's EESR. This is done by an EDMA3
(offset 0x2E30)                           programmer with a privilege level of User and Privilege ID of 0.
MPPA[7]                  0x0000 04B3      Memory Protection Filter AID = 1, UW = 1, UR = 1, SW = 1, SR = 1.
(offset 0x082C)
√           Access allowed.
↓
DRAE[7]                  0x9FF0 0FC2      DMA Region Access Enable Filter
(offset 0x0378)               ↓
EESR                     0x8BC0 0102      Value written to shadow region 7's EESR. This is done by an EDMA3 programmer with a
(offset 0x2E30)               ↓           privilege level of User and a Privilege ID of 0.
EER                          ↓            Final value of EER.
(offset 0x1020)          0xBC0 0102


#### 11.3.10.2 Proxy Memory Protection

Proxy memory protection allows an EDMA3 transfer programmed by a given EDMA3 programmer to have
its permissions travel with the transfer through the EDMA3TC. The permissions travel along with the read
transactions to the source and the write transactions to the destination endpoints. The PRIV bit and
PRIVID bit in the channel options parameter (OPT) is set with the EDMA3 programmer's PRIV value and
PRIVID values, respectively, when any part of the PaRAM set is written.
The PRIV is the privilege level (i.e., user vs. supervisor). The PRIVID refers to a privilege ID with a
number that is associated with an EDMA3 programmer.
See the data manual for the PRIVIDs that are associated with potential EDMA3 programmers.
These options are part of the TR that are submitted to the transfer controller. The transfer controller uses
the above values on their respective read and write command bus so that the target endpoints can
perform memory protection checks based on these values.
Consider a parameter set that is programmed by a CPU in user privilege level for a simple transfer with
the source buffer on an L2 page and the destination buffer on an L1D page. The PRIV is 0 for user-level
and the CPU has a PRIVID of 0.
The PaRAM set is shown in Figure 11-16.


**Figure 11-16. PaRAM Set Content for Proxy Memory Protection Example**

(a) EDMA3 Parameters


Parameter Contents                                                                      Parameter
0010 0007h                                                               Channel Options Parameter (OPT)
009F 0000h                                                                Channel Source Address (SRC)
0001h                         0004h                         Count for 2nd Dimension (BCNT)              Count for 1st Dimension (ACNT)
00F0 7800h                                                            Channel Destination Address (DST)
0001h                         0001h                        Destination BCNT Index (DSTBIDX)             Source BCNT Index (SRCBIDX)
0000h                        FFFFh                             BCNT Reload (BCNTRLD)                         Link Address (LINK)
0001h                         1000h                        Destination CCNT Index (DSTCIDX)             Source CCNT Index (SRCCIDX)
0000h                         0001h                                    Reserved                         Count for 3rd Dimension (CCNT)


**Figure 11-17. Channel Options Parameter (OPT) Example**

(b) Channel Options Parameter (OPT) Content


31   30      29        28    27                  24         23             22           21           20       19          18     17          16

0    0           00                   0000                 0              0            0             1              00               00

PRIV Rsvd       Rsvd                    PRIVID          ITCCHEN TCCHEN ITCINTEN TCINTEN                            Reserved            TCC


15                     12         11       10           8        7                                   4        3              2        1      0

0000                       0             000                              0000                         0              1        1      1

TCC                   TCCMOD             FWID                            Reserved                  STATIC SYNCDIM DAM SAM

The PRIV and PRIVID information travels along with the read and write requests that are issued to the
source and destination memories.
For example, if the access attributes that are associated with the L2 page with the source buffer only allow
supervisor read, write accesses (SR,SW), the user-level read request above is refused. Similarly, if the
access attributes that are associated with the L1D page with the destination buffer only allow supervisor
read and write accesses (SR, SW), the user-level write request above is refused. For the transfer to
succeed, the source and destination pages should have user-read and user-write permissions,
respectively, along with allowing accesses from a PRIVID 0.
Because the programmer's privilege level and privilege identification travel with the read and write
requests, EDMA3 acts as a proxy.
Figure 11-18 illustrates the propagation of PRIV and PRIVID at the boundaries of all the interacting
entities (CPU, EDMA3CC, EDMA3TC, and slave memories).


**Figure 11-18. Proxy Memory Protection Example**

Memory
Protection   L2 Page
Attribute    9F 0000h
Read req
EDMA3CC
PRIVID=0,
PaRAM                                         PRIV=0            AID0=1      Src Buffer
UR=1
EDMA3TC0
PRIVID=0

PaRAM                                                         Access allowed
CPU    User write                                                             Read
entry 5               TR
from user                  PRIVID=0,
PRIV=0            Submission
Privilege level                                                                               Access
Write                 L1D Page
allowed     F0 7800h


PRIVID = 0,        AID0=1      Dst Buffer
PRIV = 0           UW=1


Memory
Protection
Attribute


### 11.3.11 Event Queues

Event queues are a part of the EDMA3 channel controller. Event queues form the interface between the
event detection logic in the EDMA3CC and the transfer request (TR) submission logic of the EDMA3CC.
Each queue is 16 entries deep; thus, each event queue can queue a maximum of 16 events. If there are
more than 16 events, then the events that cannot find a place in the event queue remain set in the
associated event register and the CPU does not stall.
There are three event queues for the device: Queue0, Queue1, and Queue2. Events in Queue0 result in
submission of its associated transfer requests (TRs) to TC0. Similarly, transfer requests that are
associated with events in Queue2 are submitted to TC2.
An event that wins prioritization against other DMA and/or QDMA pending events is placed at the tail of
the appropriate event queue. Each event queue is serviced in FIFO order. Once the event reaches the
head of its queue and the corresponding transfer controller is ready to receive another TR, the event is
de-queued and the PaRAM set corresponding to the de-queued event is processed and submitted as a
transfer request packet (TRP) to the associated EDMA3 transfer controller.
Queue0 has highest priority and Queue2 has the lowest priority, if Queue0 and Queue1 both have at least
one event entry and if both TC0 and TC1 can accept transfer requests, then the event in Queue0 is de-
queued first and its associated PaRAM set is processed and submitted as a transfer request (TR) to TC0.
See Section 11.3.11.4 for system-level performance considerations. All of the event entries in all of the
event queues are software readable (not writeable) by accessing the event entry registers (Q0E0,
Q0E1,…Q1E15, etc.). Each event entry register characterizes the queued event in terms of the type of
event (manual, event, chained or auto-triggered) and the event number. See for a description of the bit
fields in the queue event entry registers.


#### 11.3.11.1 DMA/QDMA Channel to Event Queue Mapping

Each of the 64 DMA channels and eight QDMA channels are programmed independently to map to a
specific queue, using the DMA queue number register (DMAQNUM) and the QDMA queue number
register (QDMANUM). The mapping of DMA/QDMA channels is critical to achieving the desired
performance level for the EDMA and most importantly, in meeting real-time deadlines. See
Section 11.3.11.4.


NOTE: If an event is ready to be queued and both the event queue and the EDMA3 transfer
controller that is associated to the event queue are empty, then the event bypasses the
event queue, and moves the PaRAM processing logic, and eventually to the transfer request
submission logic for submission to the EDMA3TC. In this case, the event is not logged in the
event queue status registers.


#### 11.3.11.2 Queue RAM Debug Visibility

There are three event queues and each queue has 16 entries. These 16 entries are managed in a circular
FIFO manner. There is a queue status register (QSTAT) associated with each queue. These along with all
of the 16 entries per queue can be read via registers QSTATn and QxEy, respectively.
These registers provide user visibility and may be helpful while debugging real-time issues (typically post-
mortem), involving multiple events and event sources. The event queue entry register (QxEy) uniquely
identifies the specific event type (event-triggered, manually-triggered, chain-triggered, and QDMA events)
along with the event number (for all DMA/QDMA event channels) that are in the queue or have been de-
queued (passed through the queue).
Each of the 16 entries in the event queue are read using the EDMA3CC memory-mapped register. By
reading the event queue, you see the history of the last 16 TRs that have been processed by the EDMA3
on a given queue. This provides user/software visibility and is helpful for debugging real-time issues
(typically post-mortem), involving multiple events and event sources.
The queue status register (QSTATn) includes fields for the start pointer (STRTPTR) which provides the
offset to the head entry of an event. It also includes a field called NUMVAL that provides the total number
of valid entries residing in the event queue at a given instance of time. The STRTPTR may be used to
index appropriately into the 16 event entries. NUMVAL number of entries starting from STRTPTR are
indicative of events still queued in the respective queue. The remaining entry may be read to determine
what's already de-queued and submitted to the associated transfer controller.


#### 11.3.11.3 Queue Resource Tracking

The EDMA3CC event queue includes watermarking/threshold logic that allows you to keep track of
maximum usage of all event queues. This is useful for debugging real-time deadline violations that may
result from head-of-line blocking on a given EDMA3 event queue.
You can program the maximum number of events that can queue up in an event queue by programming
the threshold value (between 0 to 15) in the queue watermark threshold A register (QWMTHRA). The
maximum queue usage is recorded actively in the watermark (WM) field of the queue status register
(QSTATn) that keeps getting updated based on a comparison of number of valid entries, which is also
visible in the NUMVAL bit in QSTATn and the maximum number of entries (WM bit in QSTATn).
If the queue usage is exceeded, this status is visible in the EDMA3CC registers: the QTHRXCDn bit in the
channel controller error register (CCERR) and the THRXCD bit in QSTATn, where n stands for the event
queue number. Any bits that are set in CCERR also generate an EDMA3CC error interrupt.


#### 11.3.11.4 Performance Considerations

The main system bus infrastructure (L3) arbitrates bus requests from all of the masters (TCs, CPU(S), and
other bus masters) to the shared slave resources (peripherals and memories).
The priorities of transfer requests (read and write commands) from the EDMA3 transfer controllers with
respect to other masters within the system crossbar are programmed using the queue priority register
(QUEPRI). QUEPRI programs the priority of the event queues (or indirectly, TC0-TC2, because QueueN
transfer requests are submitted to TCN).
Therefore, the priority of unloading queues has a secondary affect compared to the priority of the transfers
as they are executed by the EDMA3TC (dictated by the priority set using QUEPRI).


### 11.3.12 EDMA3 Transfer Controller (EDMA3TC)

The EDMA3 channel controller is the user-interface of the EDMA3 and the EDMA3 transfer controller
(EDMA3TC) is the data movement engine of the EDMA3. The EDMA3CC submits transfer requests (TR)
to the EDMA3TC and the EDMA3TC performs the data transfers dictated by the TR; thus, the EDMA3TC
is a slave to the EDMA3CC.


#### 11.3.12.1 Architecture Details


##### 11.3.12.1.1 Command Fragmentation

The TC read and write controllers in conjunction with the source and destination register sets are
responsible for issuing optimally-sized reads and writes to the slave endpoints. An optimally-sized
command is defined by the transfer controller default burst size (DBS), which is defined in
Section 11.3.12.5.
The EDMA3TC attempts to issue the largest possible command size as limited by the DBS value or the
ACNT/BCNT value of the TR. EDMA3TC obeys the following rules:
• The read/write controllers always issue commands less than or equal to the DBS value.
• The first command of a 1D transfer command always aligns the address of subsequent commands to
the DBS value.
Table 11-22 lists the TR segmentation rules that are followed by the EDMA3TC. In summary, if the ACNT
value is larger than the DBS value, then the EDMA3TC breaks the ACNT array into DBS-sized commands
to the source/destination addresses. Each BCNT number of arrays are then serviced in succession.
For BCNT arrays of ACNT bytes (that is, a 2D transfer), if the ACNT value is less than or equal to the
DBS value, then the TR may be optimized into a 1D-transfer in order to maximize efficiency. The
optimization takes place if the EDMA3TC recognizes that the 2D-transfer is organized as a single
dimension (ACNT == BIDX) and the ACNT value is a power of 2.
Table 11-22 lists conditions in which the optimizations are performed.


**Table 11-22. Read/Write Command Optimization Rules**

SAM/DAM =
ACNT ≤ DBS       ACNT is power of 2       BIDX = ACNT           BCNT ≤ 1023             Increment       Description
Yes                 Yes                     Yes                  Yes                   Yes          Optimized
No                   x                       x                     x                    x           Not Optimized
x                  No                       x                     x                    x           Not Optimized
x                   x                      No                     x                    x           Not Optimized
x                   x                       x                    No                    x           Not Optimized
x                   x                       x                     x                   No           Not Optimized


##### 11.3.12.1.2 TR Pipelining

TR pipelining refers to the ability of the source active set to proceed ahead of the destination active set.
Essentially, the reads for a given TR may already be in progress while the writes of a previous TR may
not have completed.
The number of outstanding TRs is limited by the number of destination FIFO register entries.
TR pipelining is useful for maintaining throughput on back-to-back small TRs. It minimizes the startup
overhead because reads start in the background of a previous TR writes.


Example 11-4. Command Fragmentation (DBS = 64)

The pseudo code:
1. ACNT = 8, BCNT = 8, SRCBIDX = 8, DSTBIDX = 10, SRCADDR = 64, DSTADDR = 191
Read Controller: This is optimized from a 2D-transfer to a 1D-transfer such that the read side is equivalent
to ACNT = 64, BCNT = 1.
Cmd0 = 64 byte
Write Controller: Because DSTBIDX != ACNT, it is not optimized.
Cmd0 = 8 byte, Cmd1 = 8 byte, Cmd2 = 8 byte, Cmd3 = 8 byte, Cmd4 = 8 byte, Cmd5 = 8 byte, Cmd6 = 8
byte, Cmd7 = 8 byte.
2. ACNT=128, BCNT = 1,SRCADDR = 63, DSTADDR = 513
Read Controller: Read address is not aligned.
Cmd0 = 1 byte, (now the SRCADDR is aligned to 64 for the next command)
Cmd1 = 64 bytes
Cmd2 = 63 bytes
Write Controller: The write address is also not aligned.
Cmd0 = 63 bytes, (now the DSTADDR is aligned to 64 for the next command)
Cmd1 = 64 bytes
Cmd2 = 1 byte


##### 11.3.12.1.3 Performance Tuning

By default, reads are as issued as fast as possible. In some cases, the reads issued by the EDMA3TC
could fill the available command buffering for a slave, delaying other (potentially higher priority) masters
from successfully submitting commands to that slave. The rate at which read commands are issued by the
EDMA3TC is controlled by the RDRATE register. The RDRATE register defines the number of cycles that
the EDMA3TC read controller waits before issuing subsequent commands for a given TR, thus minimizing
the chance of the EDMA3TC consuming all available slave resources. The RDRATE value should be set
to a relatively small value if the transfer controller is targeted for high priority transfers and to a higher
value if the transfer controller is targeted for low priority transfers.
In contrast, the Write Interface does not have any performance turning knobs because writes always have
an interval between commands as write commands are submitted along with the associated write data.


#### 11.3.12.2 Memory Protection

The transfer controller plays an important role in handling proxy memory protection. There are two access
properties associated with a transfer: for instance, the privilege id (system-wide identification assigned to a
master) of the master initiating the transfer, and the privilege level (user versus supervisor) used to
program the transfer. This information is maintained in the PaRAM set when it is programmed in the
channel controller. When a TR is submitted to the transfer controller, this information is made available to
the EDMA3TC and used by the EDMA3TC while issuing read and write commands. The read or write
commands have the same privilege identification, and privilege level as that programmed in the EDMA3
transfer in the channel controller.


#### 11.3.12.3 Error Generation

Errors are generated if enabled under three conditions:
• EDMA3TC detection of an error signaled by the source or destination address.
• Attempt to read or write to an invalid address in the configuration memory map.
• Detection of a constant addressing mode TR violating the constant addressing mode transfer rules (the
source/destination addresses and source/destination indexes must be aligned to 32 bytes).


Either or all error types may be disabled. If an error bit is set and enabled, the error interrupt for the
concerned transfer controller is pulsed.


#### 11.3.12.4 Debug Features

The DMA program register set, DMA source active register set, and the destination FIFO register set are
used to derive a brief history of TRs serviced through the transfer controller.
Additionally, the EDMA3TC status register (TCSTAT) has dedicated bit fields to indicate the ongoing
activity within different parts of the transfer controller:
• The SRCACTV bit indicates whether the source active set is active.
• The DSTACTV bit indicates the number of TRs resident in the destination register active set at a given
instance.
• The PROGBUSY bit indicates whether a valid TR is present in the DMA program set.
If the TRs are in progression, caution must be used and you must realize that there is a chance that the
values read from the EDMA3TC status registers will be inconsistent since the EDMA3TC may change the
values of these registers due to ongoing activities.
It is recommended that you ensure no additional submission of TRs to the EDMA3TC in order to facilitate
ease of debug.


##### 11.3.12.4.1 Destination FIFO Register Pointer

The destination FIFO register pointer is implemented as a circular buffer with the start pointer being
DFSTRTPTR and a buffer depth of usually 2 or 4. The EDMA3TC maintains two important status details in
TCSTAT that may be used during advanced debugging, if necessary. The DFSTRTPTR is a start pointer,
that is, the index to the head of the destination FIFO register. The DSTACTV is a counter for the number
of valid (occupied) entries. These registers may be used to get a brief history of transfers.
Examples of some register field values and their interpretation:
• DFSTRTPTR = 0 and DSTACTV = 0 implies that no TRs are stored in the destination FIFO register.
• DFSTRTPTR = 1 and DSTACTV = 2h implies that two TRs are present. The first pending TR is read
from the destination FIFO register entry 1 and the second pending TR is read from the destination
FIFO register entry 2.
• DFSTRTPTR = 3h and DSTACTV = 2h implies that two TRs are present. The first pending TR is read
from the destination FIFO register entry 3 and the second pending TR is read from the destination
FIFO register entry 0.


#### 11.3.12.5 EDMA3TC Configuration

Table 11-23 provides the configuration of the individual EDMA3 transfer controllers present on the device.
The default burst size (DBS) for each transfer controller is configurable using the TPTC_CFG register in
the control module.


**Table 11-23. EDMA3 Transfer Controller Configurations**

Name             TC0                               TC1                             TC2
FIFOSIZE                    512 bytes                         512 bytes                       512 bytes
BUSWIDTH                    16 bytes                          16 bytes                        16 bytes
DSTREGDEPTH                 4 entries                         4 entries                       4 entries
DBS                         Configurable                      Configurable                    Configurable


### 11.3.13 Event Dataflow

This section summarizes the data flow of a single event, from the time the event is latched to the channel
controller to the time the transfer completion code is returned. The following steps list the sequence of
EDMA3CC activity:
1. Event is asserted from an external source (peripheral or external interrupt). This also is similar for a
manually-triggered, chained-triggered, or QDMA-triggered event. The event is latched into the
ER.En/ERH.En (or CER.En/CERH.En, ESR.En /ESRH.En, QER.En) bit.
2. Once an event is prioritized and queued into the appropriate event queue, the SER.En\SERH.En (or
QSER.En) bit is set to inform the event prioritization/processing logic to disregard this event since it is
already in the queue. Alternatively, if the transfer controller and the event queue are empty, then the
event bypasses the queue.
3. The EDMA3CC processing and the submission logic evaluates the appropriate PaRAM set and
determines whether it is a non-null and non-dummy transfer request (TR).
4. The EDMA3CC clears the ER.En/ERH.En (or CER.En/CERH.En, ESR.En/ESRH.En, QER.En) bit and
the SER.En/SERH.En bit as soon as it determines the TR is non-null. In the case of a null set, the
SER.En/SERH.En bit remains set. It submits the non-null/non-dummy TR to the associated transfer
controller. If the TR was programmed for early completion, the EDMA3CC immediately sets the
interrupt pending register (IPR.I[TCC]/IPRH.I[TCC]-32).
5. If the TR was programmed for normal completion, the EDMA3CC sets the interrupt pending register
(IPR.I[TCC]/IPRH.I[TCC]) when the EDMA3TC informs the EDMA3CC about completion of the transfer
(returns transfer completion codes).
6. The EDMA3CC programs the associated EDMA3TCn's Program Register Set with the TR.
7. The TR is then passed to the Source Active set and the DST FIFO Register Set, if both the register
sets are available.
8. The Read Controller processes the TR by issuing read commands to the source slave endpoint. The
Read Data lands in the Data FIFO of the EDMA3TCn.
9. As soon as sufficient data is available, the Write Controller begins processing the TR by issuing write
commands to the destination slave endpoint.
10. This continues until the TR completes and the EDMA3TCn then signals completion status to the
EDMA3CC.


### 11.3.14 EDMA3 Prioritization

The EDMA3 controller has many implementation rules to deal with concurrent events/channels, transfers,
etc. The following subsections detail various arbitration details whenever there might be occurrence of
concurrent activity. Figure 11-19 shows the different places EDMA3 priorities come into play.


#### 11.3.14.1 Channel Priority

The DMA event registers (ER and ERH) capture up to 64 events; likewise, the QDMA event register
(QER) captures QDMA events for all QDMA channels; therefore, it is possible for events to occur
simultaneously on the DMA/QDMA event inputs. For events arriving simultaneously, the event associated
with the lowest channel number is prioritized for submission to the event queues (for DMA events, channel
0 has the highest priority and channel 63 has the lowest priority; similarly, for QDMA events, channel 0
has the highest priority and channel 7 has the lowest priority). This mechanism only sorts simultaneous
events for submission to the event queues.
If a DMA and QDMA event occurs simultaneously, the DMA event always has prioritization against the
QDMA event for submission to the event queues.


#### 11.3.14.2 Trigger Source Priority

If a DMA channel is associated with more than one trigger source (event trigger, manual trigger, and chain
trigger), and if multiple events are set simultaneously for the same channel (ER.En = 1, ESR.En = 1 ,
CER.En = 1) , then the EDMA3CC always services these events in the following priority order: event
trigger (via ER) is higher priority than chain trigger (via CER) and chain trigger is higher priority than
manual trigger (via ESR).
This implies that if for channel 0, both ER.E0 = 1 and CER.E0 = 1 at the same time, then the ER.E0 event
is always queued before the CER.E0 event.


#### 11.3.14.3 Dequeue Priority

The priority of the associated transfer request (TR) is further mitigated by which event queue is being used
for event submission (dictated by DMAQNUM and QDMAQNUM). For submission of a TR to the transfer
request, events need to be de-queued from the event queues. Queue 0 has the highest dequeue priority
and Queue 2 the lowest.


#### 11.3.14.4 System (Transfer Controller) Priority

INIT_PRIORITY_0 and INIT_PRIORITY_1 registers in the chip configuration module are used to configure
the EDMA TC's priority through the system bus infrastructure.


NOTE: The default priority for all TCs is the same, 0 or highest priority relative to other masters. It is
recommended that this priority be changed based on system level considerations, such as
real-time deadlines for all masters including the priority of the transfer controllers with respect
to each other.


### 11.3.15 EDMA3 Operating Frequency (Clock Control)

The EDMA3 channel controller and transfer controller are clocked from PLL_L3 SYSCLK4. The EDMA3
system runs at the L3 clock frequency.


### 11.3.16 Reset Considerations

A hardware reset resets the EDMA3 (EDMA3CC and EDMA3TC) and the EDMA3 configuration registers.
The PaRAM memory contents are undefined after device reset and you should not rely on parameters to
be reset to a known state. The PaRAM entry must be initialized to a desired value before it is used.


### 11.3.17 Power Management

The EDMA3 (EDMA3CC and EDMA3TC) can be placed in reduced-power modes to conserve power
during periods of low activity. The power management of the peripheral is controlled by the power reset
clock management (PRCM). The PRCM acts as a master controller for power management for all
peripherals on the device.
The EDMA3 controller can be idled on receiving a clock stop request from the PRCM. The requests to
EDMA3CC and EDMA3TC are separate. In general, it should be verified that there are no pending
activities in the EDMA3 controller


### 11.3.18 Emulation Considerations

During debug when using the emulator, the CPU(s) may be halted on an execute packet boundary for
single-stepping, benchmarking, profiling, or other debug purposes. During an emulation halt, the EDMA3
channel controller and transfer controller operations continue. Events continue to be latched and
processed and transfer requests continue to be submitted and serviced.


Since EDMA3 is involved in servicing multiple master and slave peripherals, it is not feasible to have an
independent behavior of the EDMA3 for emulation halts. EDMA3 functionality would be coupled with the
peripherals it is servicing, which might have different behavior during emulation halts. For example, if a
McASP is halted during an emulation access (FREE = 0 and SOFT = 0 or 1 in McASP registers), the
McASP stops generating the McASP receive or transmit events (REVT or XEVT) to the EDMA. From the
point of view of the McASP, the EDMA3 is suspended, but other peripherals (for example, a timer) still
assert events and will be serviced by the EDMA.


**Figure 11-19. EDMA3 Prioritization**

From peripherals/external events         Trigger source priority
E63      E1 E0


Channel    Event queues                                              PaRAM
priority    0
Event                                                                             Dequeue                                Parameter
64:1 priority encoder


register                                                                            priority                               entry 0


Queue 0
TC0
Event     (ER/ERH)                                                                                                                   Parameter


Transfer request process submit
trigger      Event                                                                                                                     entry 1
enable           64


Channel mapping
register                                                         15
(EER/EERH)
0


L3
Queue 1


Event
Manual        set        64
trigger     register
Parameter
(ESR/ESRH)
64                                                                                                      entry 254
15
Chain                                                                                                                                Parameter                                                 TC2
Chained                                                           0
trigger                                                                                                                               entry 255
event
8:1 priority encoder


Queue 2


register
(CER/CERH)


QDMA                                                             15
event            8
EDMA3                                                                 System
Early completion
register                                                                                                                                                                                  priority
(QER)                                                                                                              channel
Queue bypass
controller
QDMA trigger


To chained event register (CER/CERH)                                         Completion                                                                                From
detection                                                                                EDMA3TC0


Completion
interface
Memory                         Error                                         Completion                                                                                             From
protection                    detection                                        interrupt                                                                                             EDMA3TC2


EDMA3CC_              Read/            EDMA3CC_                                  EDMA3CC_INT[0:7]
MPINT              write to/          ERRINT
from EDMA3
programmer


### 11.3.19 EDMA Transfer Examples

The EDMA3 channel controller performs a variety of transfers depending on the parameter configuration.
The following sections provide a description and PaRAM configuration for some typical use case
scenarios.


#### 11.3.19.1 Block Move Example

The most basic transfer performed by the EDMA3 is a block move. During device operation it is often
necessary to transfer a block of data from one location to another, usually between on-chip and off-chip
memory.
In this example, a section of data is to be copied from external memory to internal L2 SRAM as shown in

**Figure 11-20. Figure 11-21 shows the parameters for this transfer.**

The source address for the transfer is set to the start of the data block in external memory, and the
destination address is set to the start of the data block in L2. If the data block is less than 64K bytes, the
PaRAM configuration shown in Figure 11-21 holds true with the synchronization type set to A-
synchronized and indexes cleared to 0. If the amount of data is greater than 64K bytes, BCNT and the B-
indexes need to be set appropriately with the synchronization type set to AB-synchronized. The STATIC
bit in OPT is set to prevent linking.
This transfer example may also be set up using QDMA. For successive transfer submissions, of a similar
nature, the number of cycles used to submit the transfer are fewer depending on the number of changing
transfer parameters. You may program the QDMA trigger word to be the highest numbered offset in the
PaRAM set that undergoes change.


**Figure 11-20. Block Move Example**


Channel Source     1   2      3     4      5     6    7     8     Channel Destination     1     2     3       4    5     6     7     8
Address (SRC)                                                        Address (DST)
9   10    11    12      13   14    15    16                            9     10    11     12    13   14     15    16
17   18    19    20      21   ...   ...                                17     18    19     20    21    ...   ...


...   ...   244 245 246 247 248                                          ...   ...   244 245 246 247 248
249 250 251 252 253 254 255 256                                        249 250 251 252 253 254 255 256


**Figure 11-21. Block Move Example PaRAM Configuration**

(a) EDMA Parameters


Parameter Contents                                                               Parameter
0010 0008h                                                      Channel Options Parameter (OPT)
Channel Source Address (SRC)                                                Channel Source Address (SRC)
0001h                    0100h                         Count for 2nd Dimension (BCNT)             Count for 1st Dimension (ACNT)
Channel Destination Address (DST)                                          Channel Destination Address (DST)
0000h                    0000h                        Destination BCNT Index (DSTBIDX)            Source BCNT Index (SRCBIDX)
0000h                    FFFFh                            BCNT Reload (BCNTRLD)                         Link Address (LINK)
0000h                    0000h                        Destination CCNT Index (DSTCIDX)            Source CCNT Index (SRCCIDX)
0000h                    0001h                                   Reserved                         Count for 3rd Dimension (CCNT)


(b) Channel Options Parameter (OPT) Content


31    30          28       27                   24      23            22               21              20                 19                 18               17            16
0          000                 0000                     0            0                 0              1                            00                                00
PRIV    Reserved                 PRIVID               ITCCHEN      TCCHEN           ITCINTEN       TCINTEN                      Reserved                               TCC

15                12       11       10          8        7                                             4                   3                  2                1            0
0000                 0             000                                   0000                                        1                  0                0            0
TCC              TCCMOD            FWID                              Reserved                                  STATIC            SYNCDIM              DAM            SAM


#### 11.3.19.2 Subframe Extraction Example

The EDMA3 can efficiently extract a small frame of data from a larger frame of data. By performing a 2D-
to-1D transfer, the EDMA3 retrieves a portion of data for the CPU to process. In this example, a
640 × 480-pixel frame of video data is stored in external memory. Each pixel is represented by a 16-bit
halfword. The CPU extracts a 16 × 12-pixel subframe of the image for processing. To facilitate more
efficient processing time by the CPU, the EDMA3 places the subframe in internal L2 SRAM. Figure 11-22
shows the transfer of a subframe from external memory to L2. Figure 11-23 shows the parameters for this
transfer.
The same PaRAM entry options are used for QDMA channels, as well as DMA channels. The STATIC bit
in OPT is set to prevent linking. For successive transfers, only changed parameters need to be
programmed before triggering the channel.


**Figure 11-22. Subframe Extraction Example**

0
Channel Destination
Address (DST)
Channel Source                                                                             0_1 0_2 0_3 0_4 0_5 0_6 0_7 0_8 0_9 0_A 0_B 0_C 0_D 0_E 0_F 0_10

Address (SRC)                                                                             1_1 1_2 1_3 1_4 1_5 1_6 1_7 1_8 1_9 1_A 1_B 1_C 1_D 1_E 1_F 1_10

2_1 2_2 2_3 2_4 2_5 2_6 2_7 2_8 2_9 2_A 2_B 2_C 2_D 2_E 2_F 2_10

3_1 3_2 3_3 3_4 3_5 3_6 3_7 3_8 3_9 3_A 3_B 3_C 3_D 3_E 3_F 3_10

4_1 4_2 4_3 4_4 4_5 4_6 4_7 4_8 4_9 4_A 4_B 4_C 4_D 4_E 4_F 4_10

5_1 5_2 5_3 5_4 5_5 5_6 5_7 5_8 5_9 5_A 5_B 5_C 5_D 5_E 5_F 5_10

6_1 6_2 6_3 6_4 6_5 6_6 6_7 6_8 6_9 6_A 6_B 6_C 6_D 6_E 6_F 6_10

7_1 7_2 7_3 7_4 7_5 7_6 7_7 7_8 7_9 7_A 7_B 7_C 7_D 7_E 7_F 7_10

8_1 8_2 8_3 8_4 8_5 8_6 8_7 8_8 8_9 8_A 8_B 8_C 8_D 8_E 8_F 8_10

9_1 9_2 9_3 9_4 9_5 9_6 9_7 9_8 9_9 9_A 9_B 9_C 9_D 9_E 9_F 9_10

A_1 A_2 A_3 A_4 A_5 A_6 A_7 A_8 A_9 A_A A_B A_C A_D A_E A_F A_10

B_1 B_2 B_3 B_4 B_5 B_6 B_7 B_8 B_9 B_A B_B B_C B_D B_E B_F B_10


479
0                                          6
3
9


**Figure 11-23. Subframe Extraction Example PaRAM Configuration**

(a) EDMA Parameters


Parameter Contents                                                                   Parameter
0010 000Ch                                                         Channel Options Parameter (OPT)
Channel Source Address (SRC)                                                  Channel Source Address (SRC)
000Ch                    0020h                         Count for 2nd Dimension (BCNT)                    Count for 1st Dimension (ACNT)
Channel Destination Address (DST)                                           Channel Destination Address (DST)
0020h                    0500h                      Destination BCNT Index (DSTBIDX)                     Source BCNT Index (SRCBIDX)
0000h                   FFFFh                               BCNT Reload (BCNTRLD)                                Link Address (LINK)
0000h                    0000h                     Destination CCNT Index (DSTCIDX)                      Source CCNT Index (SRCCIDX)
0000h                    0001h                                     Reserved                              Count for 3rd Dimension (CCNT)


(b) Channel Options Parameter (OPT) Content


31      30         28         27                     24          23           22              21      20            19                 18         17              16
0           000                   0000                          0             0              0        1                    00                               00
PRIV     Reserved                   PRIVID                  ITCCHEN          TCCHEN      ITCINTEN   TCINTEN                Reserved                      TCC

15                 12         11         10          8           7                                     4            3                  2           1               0
0000                    0               000                                   0000                            1                  1           0               0
TCC              TCCMOD                FWID                                 Reserved                       STATIC      SYNCDIM             DAM             SAM


#### 11.3.19.3 Data Sorting Example

Many applications require the use of multiple data arrays; it is often desirable to have the arrays arranged
such that the first elements of each array are adjacent, the second elements are adjacent, and so on.
Often this is not how the data is presented to the device. Either data is transferred via a peripheral with
the data arrays arriving one after the other or the arrays are located in memory with each array occupying
a portion of contiguous memory spaces. For these instances, the EDMA3 can reorganize the data into the
desired format. Figure 11-24 shows the data sorting.
To determine the parameter set values, the following need to be considered:
• ACNT - Program this to be the size in bytes of an element.
• BCNT - Program this to be the number of elements in a frame.
• CCNT - Program this to be the number of frames.
• SRCBIDX - Program this to be the size of the element or ACNT.
• DSTBIDX - CCNT × ACNT
• SRCCDX - ACNT × BCNT
• DSTCIDX - ACNT
The synchronization type needs to be AB-synchronized and the STATIC bit is 0 to allow updates to the
parameter set. It is advised to use normal EDMA3 channels for sorting.
It is not possible to sort this with a single trigger event. Instead, the channel can be programmed to be
chained to itself. After BCNT elements get sorted, intermediate chaining could be used to trigger the
channel again causing the transfer of the next BCNT elements and so on. Figure 11-25 shows the
parameter set programming for this transfer, assuming channel 0 and an element size of 4 bytes.


**Figure 11-24. Data Sorting Example**


Channel Source     A_1        A_2         A_3         ...        ...   A_1022 A_1023 A_1024      Channel       A_1       B_1           C_1    D_1
Address (SRC)                                                                                 Destination
B_1        B_2         B_3         ...        ...   B_1022 B_1023 B_1024 Address (DST)      A_2       B_2           C_2    D_2

C_1        C_2         C_3         ...        ...   C_1022 C_1023 C_1024                    A_3       B_3           C_3    D_3

D_1        D_2         D_3         ...        ...   D_1022 D_1023 D_1024                    ...          ...        ...        ...

...          ...        ...        ...

A_1022 B_1022 C_1022 D_1022

A_1023 B_1023 C_1023 D_1023

A_1024 B_1024 C_1024 D_1024


**Figure 11-25. Data Sorting Example PaRAM Configuration**

(a) EDMA Parameters


Parameter Contents                                                               Parameter
0090 0004h                                                     Channel Options Parameter (OPT)
Channel Source Address (SRC)                                                Channel Source Address (SRC)
0400h                    0004h                         Count for 2nd Dimension (BCNT)         Count for 1st Dimension (ACNT)
Channel Destination Address (DST)                                         Channel Destination Address (DST)
0010h                    0004h                      Destination BCNT Index (DSTBIDX)          Source BCNT Index (SRCBIDX)
0000h                    FFFFh                           BCNT Reload (BCNTRLD)                       Link Address (LINK)
0004h                    1000h                     Destination CCNT Index (DSTCIDX)           Source CCNT Index (SRCCIDX)
0000h                    0004h                                   Reserved                     Count for 3rd Dimension (CCNT)


(b) Channel Options Parameter (OPT) Content


31    30          28       27                   24      23            22              21           20           19            18         17          16
0          000                 0000                     1            0               0             1                  00                       00
PRIV    Reserved                 PRIVID               ITCCHEN      TCCHEN         ITCINTEN       TCINTEN              Reserved                  TCC

15                12       11       10          8        7                                          4           3               2        1            0
0000                 0              000                                0000                               0               1        0            0
TCC              TCCMOD            FWID                              Reserved                          STATIC      SYNCDIM       DAM           SAM


#### 11.3.19.4 Peripheral Servicing Example

The EDMA3 channel controller also services peripherals in the background of CPU operation, without
requiring any CPU intervention. Through proper initialization of the EDMA3 channels, they can be
configured to continuously service on-chip and off-chip peripherals throughout the device operation. Each
event available to the EDMA3 has its own dedicated channel, and all channels operate simultaneously.
The only requirements are to use the proper channel for a particular transfer and to enable the channel
event in the event enable register (EER). When programming an EDMA3 channel to service a peripheral,
it is necessary to know how data is to be presented to the processor. Data is always provided with some
kind of synchronization event as either one element per event (non-bursting) or multiple elements per
event (bursting).


##### 11.3.19.4.1 Non-bursting Peripherals

Non-bursting peripherals include the on-chip multichannel audio serial port (McASP) and many external
devices, such as codecs. Regardless of the peripheral, the EDMA3 channel configuration is the same.
The McASP transmit and receive data streams are treated independently by the EDMA3. The transmit
and receive data streams can have completely different counts, data sizes, and formats. Figure 11-26
shows servicing incoming McASP data.
To transfer the incoming data stream to its proper location in DDR memory, the EDMA3 channel must be
set up for a 1D-to-1D transfer with A-synchronization. Because an event (AREVT) is generated for every
word as it arrives, it is necessary to have the EDMA3 issue the transfer request for each element
individually. Figure 11-27 shows the parameters for this transfer. The source address of the EDMA3
channel is set to the data port address (DAT) for McASP, and the destination address is set to the start of
the data block in DDR. Because the address of serializer buffer is fixed, the source B index is cleared to 0
(no modification) and the destination B index is set to 01b (increment).
Based on the premise that serial data is typically a high priority, the EDMA3 channel should be
programmed to be on queue 0.


**Figure 11-26. Servicing Incoming McASP Data Example**

:
3                                           Channel Destination         1        2         3      4         5        6         7     8
:                                               Address (DST)
2                                                                       9       10        11     12      13      14           15    16
:
1                                  AREVT                               17       18        19     20      21       ...         ...

McASP RX Address

RSR                                  DAT

Receive
serializer
...       ...    244    245      246          247   248

249      250      251     252    253      254          255   256


**Figure 11-27. Servicing Incoming McASP Data Example PaRAM Configuration**

(a) EDMA Parameters


Parameter Contents                                                                   Parameter
0010 0000h                                                       Channel Options Parameter (OPT)
McASP RX Address                                                          Channel Source Address (SRC)
0100h                   0001h                         Count for 2nd Dimension (BCNT)              Count for 1st Dimension (ACNT)
Channel Destination Address (DST)                                           Channel Destination Address (DST)
0001h                   0000h                      Destination BCNT Index (DSTBIDX)               Source BCNT Index (SRCBIDX)
0000h                   FFFFh                           BCNT Reload (BCNTRLD)                            Link Address (LINK)
0000h                   0000h                     Destination CCNT Index (DSTCIDX)                Source CCNT Index (SRCCIDX)
0000h                   0004h                                    Reserved                         Count for 3rd Dimension (CCNT)


(b) Channel Options Parameter (OPT) Content


31    30          28        27                  24      23            22              21               20            19               18         17          16
0          000                  0000                    0            0                  0              1                    00                        00
PRIV    Reserved                  PRIVID              ITCCHEN      TCCHEN         ITCINTEN           TCINTEN               Reserved                     TCC

15                12        11      10          8        7                                              4            3                2           1          0
0000                  0             000                                0000                                    0                0           0          0
TCC              TCCMOD            FWID                              Reserved                                STATIC     SYNCDIM            DAM        SAM


##### 11.3.19.4.2 Bursting Peripherals

Higher bandwidth applications require that multiple data elements be presented to the processor core for
every synchronization event. This frame of data can either be from multiple sources that are working
simultaneously or from a single high-throughput peripheral that streams data to/from the processor.
In this example, a port is receiving a video frame from a camera and presenting it to the processor one
array at a time. The video image is 640 × 480 pixels, with each pixel represented by a 16-bit element. The
image is to be stored in external memory. Figure 11-28 shows this example.
To transfer data from an external peripheral to an external buffer one array at a time based on EVTn ,
channel n must be configured. Due to the nature of the data (a video frame made up of arrays of pixels)
the destination is essentially a 2D entity. Figure 11-29 shows the parameters to service the incoming data
with a 1D-to-2D transfer using AB-synchronization. The source address is set to the location of the video
framer peripheral, and the destination address is set to the start of the data buffer. Because the input
address is static, the SRCBIDX is 0 (no modification to the source address). The destination is made up of
arrays of contiguous, linear elements; therefore, the DSTBIDX is set to pixel size, 2 bytes. ANCT is equal
to the pixel size, 2 bytes. BCNT is set to the number of pixels in an array, 640. CCNT is equal to the total
number of arrays in the block, 480. SRCCIDX is 0 because the source address undergoes no increment.
The DSTCIDX is equal to the difference between the starting addresses of each array. Because a pixel is
16 bits (2 bytes), DSTCIDX is equal to 640 × 2.


**Figure 11-28. Servicing Peripheral Burst Example**

EVTx
Destination Address   0_1     0_2     0_3        ...   ...   0_638    0_639   0_640
Destination Address + 500h   1_1     1_2      ...                     ...    1_639   1_640
Destination Address + A00h   2_1      ...                                     ...    2_640

External                          ...        ...                                                      ...
...1_2..1_1..0_2..0_1
peripheral                         ...        ...                                                      ...
Destination Address + 95100h   477_1    ...                                     ...    477_640
Destination Address + 95600h   478_1   478_2    ...                     ...   478_639 478_640
Destination Address + 95B00h   479_1   479_2   479_3      ...   ...   479_638 479_639 479_640


**Figure 11-29. Servicing Peripheral Burst Example PaRAM Configuration**

(a) EDMA Parameters


Parameter Contents                                                               Parameter
0010 0004h                                                      Channel Options Parameter (OPT)
Channel Source Address                                                   Channel Source Address (SRC)
0280h                      0002h                         Count for 2nd Dimension (BCNT)         Count for 1st Dimension (ACNT)
Channel Destination Address                                            Channel Destination Address (DST)
0002h                      0000h                      Destination BCNT Index (DSTBIDX)          Source BCNT Index (SRCBIDX)
0000h                     FFFFh                            BCNT Reload (BCNTRLD)                       Link Address (LINK)
0500h                      0000h                     Destination CCNT Index (DSTCIDX)           Source CCNT Index (SRCCIDX)
0000h                     01E0h                                    Reserved                     Count for 3rd Dimension (CCNT)


(b) Channel Options Parameter (OPT) Content


31    30          28         27                   24      23            22              21           20           19            18         17         16
0          000                   0000                     0            0               0             1                  00                     00
PRIV    Reserved                   PRIVID               ITCCHEN      TCCHEN         ITCINTEN       TCINTEN              Reserved                 TCC

15                12         11       10          8        7                                          4           3               2        1          0
0000                   0             000                                 0000                               0               1        0          0
TCC                TCCMOD            FWID                              Reserved                          STATIC      SYNCDIM       DAM         SAM


##### 11.3.19.4.3 Continuous Operation

Configuring an EDMA3 channel to receive a single frame of data is useful, and is applicable to some
systems. A majority of the time, however, data is going to be continuously transmitted and received
throughout the entire operation of the processor. In this case, it is necessary to implement some form of
linking such that the EDMA3 channels continuously reload the necessary parameter sets. In this example,
McASP is configured to transmit and receive data on a T1 array. To simplify the example, only two
channels are active for both transmit and receive data streams. Each channel receives packets of
128 elements. The packets are transferred from the serial port to internal memory and from internal
memory to the serial port, as shown Figure 11-30.
The McASP generates AREVT for every element received and generates AXEVT for every element
transmitted. To service the data streams, the DMA channels associated with the McASP must be setup for
1D-to-1D transfers with A-synchronization.
Figure 11-31 shows the parameter entries for the channel for these transfers. To service the McASP
continuously, the channels must be linked to a duplicate PaRAM set in the PaRAM. After all frames have
been transferred, the EDMA3 channels reload and continue. Figure 11-32 shows the reload parameters
for the channel.


##### 11.3.19.4.3.1 Receive Channel

EDMA3 channel 15 services the incoming data stream of McASP. The source address is set to that of the
receive serializer buffer, and the destination address is set to the first element of the data block. Because
there are two data channels being serviced, A and B, they are to be located separately within the L2
SRAM.
To facilitate continuous operation, a copy of the PaRAM set for the channel is placed in PaRAM set 64.
The LINK option is set and the link address is provided in the PaRAM set. Upon exhausting the channel
15 parameter set, the parameters located at the link address are loaded into the channel 15 parameter set
and operation continues. This function continues throughout device operation until halted by the CPU.


##### 11.3.19.4.3.2 Transmit Channel

EDMA3 channel 12 services the outgoing data stream of McASP. In this case the destination address
needs no update, hence, the parameter set changes accordingly. Linking is also used to allow continuous
operation by the EDMA3 channel, with duplicate PaRAM set entries at PaRAM set 65.


**Figure 11-30. Servicing Continuous McASP Data Example**


AREVT           Stream A Destination Address A1i         A2i     A3i   A4i   A5i   A6i   A7i   A8i

..B5..A5..B4..A4..B3..A3..B2..A2..B1..A1   McASP RX Register                 A9i    A10i A11i A12i A13i        ...   ...
RSR                                     DAT

Stream B Destination Address      B1i     B2i     B3i   B4i   B5i   B6i   B7i   B8i

B9i    B10i B11i B12i B13i        ...   ...


AXEVT              Stream A Source Address A1o A2o A3o A4o A5o A6o A7o A8o

A1..B1..A2..B2..A3..B3..A4..B4..A5..B5    McASP TX Register                 A9o A10o A11o A12o A13o           ...   ...
XSR                                     DAT

Stream B Source Address B1o B2o B3o B4o B5o B6o B7o B8o

B9o B10o B11o B12o B13o           ...   ...


**Figure 11-31. Servicing Continuous McASP Data Example PaRAM Configuration**

(a) EDMA Parameters for Receive Channel (PaRAM Set 15) being Linked to PaRAM Set 64


Parameter Contents                                                              Parameter
0010 0000h                                                     Channel Options Parameter (OPT)
McASP RX Register                                                     Channel Source Address (SRC)
0080h                    0001h                         Count for 2nd Dimension (BCNT)         Count for 1st Dimension (ACNT)
Channel Destination Address (DST)                                         Channel Destination Address (DST)
0001h                    0000h                      Destination BCNT Index (DSTBIDX)          Source BCNT Index (SRCBIDX)
0080h                    4800h                           BCNT Reload (BCNTRLD)                       Link Address (LINK)
0000h                    0000h                     Destination CCNT Index (DSTCIDX)           Source CCNT Index (SRCCIDX)
0000h                    FFFFh                                   Reserved                     Count for 3rd Dimension (CCNT)


(b) Channel Options Parameter (OPT) Content for Receive Channel (PaRAM Set 15)


31    30          28       27                   24      23            22              21           20           19            18         17         16
0          000                 0000                     0            0               0             1                  00                     00
PRIV    Reserved                 PRIVID               ITCCHEN      TCCHEN         ITCINTEN       TCINTEN              Reserved                 TCC

15                12       11       10          8        7                                          4           3               2        1          0
0000                 0              000                                0000                               0               0        0          0
TCC               TCCMOD           FWID                              Reserved                          STATIC      SYNCDIM       DAM         SAM


(c) EDMA Parameters for Transmit Channel (PaRAM Set 12) being Linked to PaRAM Set 65


Parameter Contents                                                              Parameter
0010 1000h                                                     Channel Options Parameter (OPT)
Channel Source Address (SRC)                                                Channel Source Address (SRC)
0080h                    0001h                         Count for 2nd Dimension (BCNT)         Count for 1st Dimension (ACNT)
McASP TX Register                                                  Channel Destination Address (DST)
0000h                    0001h                      Destination BCNT Index (DSTBIDX)          Source BCNT Index (SRCBIDX)
0080h                    4860h                           BCNT Reload (BCNTRLD)                       Link Address (LINK)
0000h                    0000h                     Destination CCNT Index (DSTCIDX)           Source CCNT Index (SRCCIDX)
0000h                    FFFFh                                   Reserved                     Count for 3rd Dimension (CCNT)


(d) Channel Options Parameter (OPT) Content for Transmit Channel (PaRAM Set 12)


31    30          28       27                   24      23            22              21           20           19            18         17         16
0          000                 0000                     0            0               0             1                  00                     00
PRIV    Reserved                 PRIVID               ITCCHEN      TCCHEN         ITCINTEN       TCINTEN              Reserved                 TCC

15                12       11       10          8        7                                          4           3               2        1          0
0001                 0              000                                0000                               0               0        0          0
TCC               TCCMOD           FWID                              Reserved                          STATIC      SYNCDIM       DAM         SAM


**Figure 11-32. Servicing Continuous McASP Data Example Reload PaRAM Configuration**

(a) EDMA Reload Parameters (PaRAM Set 64) for Receive Channel


Parameter Contents                                                               Parameter
0010 0000h                                                     Channel Options Parameter (OPT)
McASP RX Register                                                     Channel Source Address (SRC)
0080h                    0001h                         Count for 2nd Dimension (BCNT)         Count for 1st Dimension (ACNT)
Channel Destination Address (DST)                                         Channel Destination Address (DST)
0001h                    0000h                      Destination BCNT Index (DSTBIDX)          Source BCNT Index (SRCBIDX)
0080h                    4800h                           BCNT Reload (BCNTRLD)                       Link Address (LINK)
0000h                    0000h                     Destination CCNT Index (DSTCIDX)           Source CCNT Index (SRCCIDX)
0000h                    FFFFh                                   Reserved                     Count for 3rd Dimension (CCNT)


(b) Channel Options Parameter (OPT) Content for Receive Channel (PaRAM Set 64)


31    30          28       27                   24      23            22              21           20           19            18         17          16
0          000                 0000                     0            0               0             1                  00                       00
PRIV    Reserved                 PRIVID               ITCCHEN      TCCHEN         ITCINTEN       TCINTEN              Reserved                  TCC

15                12       11       10          8        7                                          4           3               2        1            0
0000                 0              000                                0000                               0               0        0            0
TCC              TCCMOD            FWID                              Reserved                          STATIC      SYNCDIM       DAM           SAM


(c) EDMA Reload Parameters (PaRAM Set 65) for Transmit Channel


Parameter Contents                                                               Parameter
0010 1000h                                                     Channel Options Parameter (OPT)
Channel Source Address (SRC)                                                Channel Source Address (SRC)
0080h                    0001h                         Count for 2nd Dimension (BCNT)         Count for 1st Dimension (ACNT)
McASP TX Register                                                  Channel Destination Address (DST)
0000h                    0001h                      Destination BCNT Index (DSTBIDX)          Source BCNT Index (SRCBIDX)
0080h                    4860h                           BCNT Reload (BCNTRLD)                       Link Address (LINK)
0000h                    0000h                     Destination CCNT Index (DSTCIDX)           Source CCNT Index (SRCCIDX)
0000h                    FFFFh                                   Reserved                     Count for 3rd Dimension (CCNT)


(d) Channel Options Parameter (OPT) Content for Transmit Channel (PaRAM Set 65)


31    30          28       27                   24      23            22              21           20           19            18         17          16
0          000                 0000                     0            0               0             1                  00                       00
PRIV    Reserved                 PRIVID               ITCCHEN      TCCHEN         ITCINTEN       TCINTEN              Reserved                  TCC

15                12       11       10          8        7                                          4           3               2        1            0
0001                 0              000                                0000                               0               0        0            0
TCC              TCCMOD            FWID                              Reserved                          STATIC      SYNCDIM       DAM           SAM


##### 11.3.19.4.4 Ping-Pong Buffering

Although the previous configuration allows the EDMA3 to service a peripheral continuously, it presents a
number of restrictions to the CPU. Because the input and output buffers are continuously being
filled/emptied, the CPU must match the pace of the EDMA3 very closely to process the data. The EDMA3
receive data must always be placed in memory before the CPU accesses it, and the CPU must provide
the output data before the EDMA3 transfers it. Though not impossible, this is an unnecessary challenge. It
is particularly difficult in a 2-level cache scheme.
Ping-pong buffering is a simple technique that allows the CPU activity to be distanced from the EDMA3
activity. This means that there are multiple (usually two) sets of data buffers for all incoming and outgoing
data streams. While the EDMA3 transfers the data into and out of the ping buffers, the CPU manipulates
the data in the pong buffers. When both CPU and EDMA3 activity completes, they switch. The EDMA3
then writes over the old input data and transfers the new output data. Figure 11-33 shows the ping-pong
scheme for this example.
To change the continuous operation example, such that a ping-pong buffering scheme is used, the
EDMA3 channels need only a moderate change. Instead of one parameter set, there are two; one for
transferring data to/from the ping buffers and one for transferring data to/from the pong buffers. As soon
as one transfer completes, the channel loads the PaRAM set for the other and the data transfers continue.
Figure 11-34 shows the EDMA3 channel configuration required.
Each channel has two parameter sets, ping and pong. The EDMA3 channel is initially loaded with the ping
parameters (Figure 11-34). The link address for the ping set is set to the PaRAM offset of the pong
parameter set (Figure 11-35). The link address for the pong set is set to the PaRAM offset of the ping
parameter set (Figure 11-36). The channel options, count values, and index values are all identical
between the ping and pong parameters for each channel. The only differences are the link address
provided and the address of the data buffer.


##### 11.3.19.4.4.1 Synchronization with the CPU

To utilize the ping-pong buffering technique, the system must signal the CPU when to begin to access the
new data set. After the CPU finishes processing an input buffer (ping), it waits for the EDMA3 to complete
before switching to the alternate (pong) buffer. In this example, both channels provide their channel
numbers as their report word and set the TCINTEN bit to generate an interrupt after completion. When
channel 15 fills an input buffer, the E15 bit in the interrupt pending register (IPR) is set; when channel 12
empties an output buffer, the E12 bit in IPR is set. The CPU must manually clear these bits. With the
channel parameters set, the CPU polls IPR to determine when to switch. The EDMA3 and CPU could
alternatively be configured such that the channel completion interrupts the CPU. By doing this, the CPU
could service a background task while waiting for the EDMA3 to complete.


**Figure 11-33. Ping-Pong Buffering for McASP Data Example**

..B5..A5..B4..A4..B3..A3..B2..A2..B1..A1             RSR

Ping                                                                                                 Pong

Stream A     A1i   A2i    A3i    A4i    A5i     A6i     A7i    A8i                                 Stream A     A1i   A2i   A3i       A4i   A5i   A6i   A7i   A8i
Destination                                                                                        Destination
Address     A9i   A10i A11i A12i A13i          ...      ...                                        Address     A9i   A10i A11i A12i A13i         ...   ...
DAT

Stream B B1i       B2i    B3i    B4i    B5i     B6i     B7i    B8i            McASP RX Register Stream B B1i          B2i   B3i       B4i   B5i   B6i   B7i   B8i
Destination                                                           AREVT                    Destination
Address B9i       B10i B11i B12i B13i          ...      ...                                     Address B9i          B10i B11i B12i B13i         ...   ...


Stream A A1o A2o A3o A4o A5o A6o                        A7o    A8o   AXEVT                         Stream A A1o A2o A3o A4o A5o A6o                     A7o   A8o
Source                                                                      McASP TX Register      Source
Address A9o A10o A11o A12o A13o ...                     ...                                        Address A9o A10o A11o A12o A13o ...                 ...
DAT

Stream B B1o B2o B3o B4o B5o B6o                        B7o    B8o                                 Stream B B1o B2o B3o B4o B5o B6o                     B7o   B8o
Source                                                                                             Source
Address B9o B10o B11o B12o B13o ...                     ...                                        Address B9o B10o B11o B12o B13o ...                 ...


A1..B1..A2..B2..A3..B3..A4..B4..A5..B5              XSR


**Figure 11-34. Ping-Pong Buffering for McASP Example PaRAM Configuration**

(a) EDMA Parameters for Channel 15 (Using PaRAM Set 15 Linked to Pong Set 64)


Parameter Contents                                                                                Parameter
0010 D000h                                                                   Channel Options Parameter (OPT)
McASP RX Register                                                                    Channel Source Address (SRC)
0080h                           0001h                              Count for 2nd Dimension (BCNT)                   Count for 1st Dimension (ACNT)
Channel Destination Address (DST)                                                        Channel Destination Address (DST)
0001h                           0000h                          Destination BCNT Index (DSTBIDX)                     Source BCNT Index (SRCBIDX)
0080h                           4800h                                BCNT Reload (BCNTRLD)                                    Link Address (LINK)
0000h                           0000h                          Destination CCNT Index (DSTCIDX)                     Source CCNT Index (SRCCIDX)
0000h                           0001h                                         Reserved                              Count for 3rd Dimension (CCNT)


(b) Channel Options Parameter (OPT) Content for Channel 15


31     30           28          27                     24             23            22              21                 20            19                  18          17          16
0          000                        0000                           0             0               0                  1                           00                      00
PRIV     Reserved                     PRIVID                     ITCCHEN       TCCHEN           ITCINTEN          TCINTEN                     Reserved                      TCC

15                  12          11        10            8             7                                                4                 3               2            1          0
1101                      0                000                                       0000                                          0               0            0          0
TCC                 TCCMOD               FWID                                      Reserved                                  STATIC             SYNCDIM        DAM        SAM


(c) EDMA Parameters for Channel 12 (Using PaRAM Set 12 Linked to Pong Set 66)


Parameter Contents                                                               Parameter
0010 C000h                                                      Channel Options Parameter (OPT)
Channel Source Address (SRC)                                                Channel Source Address (SRC)
0080h                    0001h                         Count for 2nd Dimension (BCNT)         Count for 1st Dimension (ACNT)
McASP TX Register                                                  Channel Destination Address (DST)
0000h                    0001h                      Destination BCNT Index (DSTBIDX)          Source BCNT Index (SRCBIDX)
0080h                    4840h                           BCNT Reload (BCNTRLD)                       Link Address (LINK)
0000h                    0000h                     Destination CCNT Index (DSTCIDX)           Source CCNT Index (SRCCIDX)
0000h                    0001h                                   Reserved                     Count for 3rd Dimension (CCNT)


(d) Channel Options Parameter (OPT) Content for Channel 12


31    30          28       27                   24      23            22              21           20           19            18         17         16
0          000                 0000                     0            0               0             1                  00                     00
PRIV    Reserved                 PRIVID               ITCCHEN      TCCHEN         ITCINTEN       TCINTEN              Reserved                 TCC

15                12       11       10          8        7                                          4           3               2        1          0
1100                 0             000                                 0000                               0               0        0          0
TCC              TCCMOD            FWID                              Reserved                          STATIC      SYNCDIM       DAM         SAM


**Figure 11-35. Ping-Pong Buffering for McASP Example Pong PaRAM Configuration**

(a) EDMA Pong Parameters for Channel 15 at Set 64 Linked to Set 65


Parameter Contents                                                               Parameter
0010 D000h                                                      Channel Options Parameter (OPT)
McASP RX Register                                                     Channel Source Address (SRC)
0080h                    0001h                         Count for 2nd Dimension (BCNT)         Count for 1st Dimension (ACNT)
Channel Destination Address (DST)                                         Channel Destination Address (DST)
0001h                    0000h                      Destination BCNT Index (DSTBIDX)          Source BCNT Index (SRCBIDX)
0080h                    4820h                           BCNT Reload (BCNTRLD)                       Link Address (LINK)
0000h                    0000h                     Destination CCNT Index (DSTCIDX)           Source CCNT Index (SRCCIDX)
0000h                    0001h                                   Reserved                     Count for 3rd Dimension (CCNT)


(b) EDMA Pong Parameters for Channel 12 at Set 66 Linked to Set 67


Parameter Contents                                                               Parameter
0010 C000h                                                      Channel Options Parameter (OPT)
Channel Source Address (SRC)                                                Channel Source Address (SRC)
0080h                    0001h                         Count for 2nd Dimension (BCNT)         Count for 1st Dimension (ACNT)
McASP TX Register                                                  Channel Destination Address (DST)
0000h                    0001h                      Destination BCNT Index (DSTBIDX)          Source BCNT Index (SRCBIDX)
0080h                    4860h                           BCNT Reload (BCNTRLD)                       Link Address (LINK)
0000h                    0000h                     Destination CCNT Index (DSTCIDX)           Source CCNT Index (SRCCIDX)
0000h                    0001h                                   Reserved                     Count for 3rd Dimension (CCNT)


**Figure 11-36. Ping-Pong Buffering for McASP Example Ping PaRAM Configuration**

(a) EDMA Ping Parameters for Channel 15 at Set 65 Linked to Set 64


Parameter Contents                                                             Parameter
0010 D000h                                                 Channel Options Parameter (OPT)
McASP RX Register                                                  Channel Source Address (SRC)
0080h                   0001h                      Count for 2nd Dimension (BCNT)           Count for 1st Dimension (ACNT)
Channel Destination Address (DST)                                      Channel Destination Address (DST)
0001h                   0000h                     Destination BCNT Index (DSTBIDX)          Source BCNT Index (SRCBIDX)
0080h                   4800h                         BCNT Reload (BCNTRLD)                      Link Address (LINK)
0000h                   0000h                    Destination CCNT Index (DSTCIDX)           Source CCNT Index (SRCCIDX)
0000h                   0001h                                 Reserved                      Count for 3rd Dimension (CCNT)


(b) EDMA Ping Parameters for Channel 12 at Set 67 Linked to Set 66


Parameter Contents                                                             Parameter
0010 C000h                                                 Channel Options Parameter (OPT)
Channel Source Address (SRC)                                            Channel Source Address (SRC)
0080h                   0001h                      Count for 2nd Dimension (BCNT)           Count for 1st Dimension (ACNT)
McASP TX Register                                             Channel Destination Address (DST)
0000h                   0001h                     Destination BCNT Index (DSTBIDX)          Source BCNT Index (SRCBIDX)
0080h                   4840h                         BCNT Reload (BCNTRLD)                      Link Address (LINK)
0000h                   0000h                    Destination CCNT Index (DSTCIDX)           Source CCNT Index (SRCCIDX)
0000h                   0001h                                 Reserved                      Count for 3rd Dimension (CCNT)


##### 11.3.19.4.5 Transfer Chaining Examples

The following examples explain the intermediate transfer complete chaining function.


##### 11.3.19.4.5.1 Servicing Input/Output FIFOs with a Single Event

Many systems require the use of a pair of external FIFOs that must be serviced at the same rate. One
FIFO buffers data input, and the other buffers data output. The EDMA3 channels that service these FIFOs
can be set up for AB-synchronized transfers. While each FIFO is serviced with a different set of
parameters, both can be signaled from a single event. For example, an external interrupt pin can be tied
to the status flags of one of the FIFOs. When this event arrives, the EDMA3 needs to perform servicing for
both the input and output streams. Without the intermediate transfer complete chaining feature this would
require two events, and thus two external interrupt pins. The intermediate transfer complete chaining
feature allows the use of a single external event (for example, a GPIO event). Figure 11-37 shows the
EDMA3 setup and illustration for this example.
A GPIO event (in this case, GPINT0) triggers an array transfer. Upon completion of each intermediate
array transfer of channel 48, intermediate transfer complete chaining sets the E8 bit (specified by TCC of
8) in the chained event register (CER) and provides a synchronization event to channel 8. Upon
completion of the last array transfer of channel 48, transfer complete chaining—not intermediate transfer
complete chaining—sets the E8 bit in CER (specified by TCCMODE:TCC) and provides a synchronization
event to channel 8. The completion of channel 8 sets the I8 bit (specified by TCCMODE:TCC) in the
interrupt pending register (IPR), which can generate an interrupt to the CPU, if the I8 bit in the interrupt
enable register (IER) is set.


**Figure 11-37. Intermediate Transfer Completion Chaining Example**

Hardwired event                                   Chained event
(tied to GPINT0, event 48)                                (event 8)
Event 48
Intermediate
transfer complete(A)
Channel 48, array 0                                Channel 8, array 0

Event 48
Intermediate
transfer complete(A)
Channel 48, array 1                                Channel 8, array 1

Event 48
Intermediate
transfer complete(A)
Channel 48, array 2                                Channel 8, array 2

Event 48
Transfer complete(B)
Channel 48, array 3                                Channel 8, array 3          Transfer complete sets
(last array)                                                                IPR.I8 = 1
If IPR.I8 = 1, interrupt
Notes:   (A) Intermediate transfer complete chaining synchronizes event 8                  EDMACC_INT* sent
to CPU
ITCCHEN = 1, TCC = 01000b, and sets CER.E8 = 1
(B) Transfer complete chaining synchronizes event 8
TCCHEN =1, TCC = 01000b and sets CER.E8 = 1

Setup
Channel 48 parameters                     Channel 8 parameters
for chaining                              for chaining                              Event enable register (EER)

Enable transfer                           Enable transfer                          Enable channel 48
complete chaining:                        complete chaining:                       EER.E48 = 1
OPT.TCCHEN = 1                            OPT.TCINTEN = 1
OPT.TCC = 01000b                          OPT.TCC = 01000b

Enable intermediate transfer              Disable intermediate transfer
complete chaining:                        complete chaining:
OPT.ITCCHEN = 1                           OPT.ITCCHEN = 0
OPT.TCC = 01000b


##### 11.3.19.4.5.2 Breaking Up Large Transfers with Intermediate Chaining

Another feature of intermediate transfer chaining (ITCCHEN) is for breaking up large transfers. A large
transfer may lock out other transfers of the same priority level for the duration of the transfer. For example,
a large transfer on queue 0 from the internal memory to the external memory using the EMIF may starve
other EDMA3 transfers on the same queue. In addition, this large high-priority transfer may prevent the
EMIF for a long duration to service other lower priority transfers. When a large transfer is considered to be
high priority, it should be split into multiple smaller transfers. Figure 11-38 shows the EDMA3 setup and
illustration of an example single large block transfer.


**Figure 11-38. Single Large Block Transfer Example**

Event 25 (CPU writes 1 to ESR.E25)
EDMA3 channel 25 setup

ACNT = 16384
16 KBytes data transfer                                    BCNT = 1
CCNT = 1
OPT.ITCINTEN = 0
OPT.TCC = Don’t care
1D transfer of 16 KByte elements


The intermediate transfer chaining enable (ITCCHEN) provides a method to break up a large transfer into
smaller transfers. For example, to move a single large block of memory (16K bytes), the EDMA3 performs
an A-synchronized transfer. The element count is set to a reasonable value, where reasonable derives
from the amount of time it would take to move this smaller amount of data. Assume 1 Kbyte is a
reasonable small transfer in this example. The EDMA3 is set up to transfer 16 arrays of 1 Kbyte elements,
for a total of 16K byte elements. The TCC field in the channel options parameter (OPT) is set to the same
value as the channel number and ITCCHEN are set. In this example, EDMA3 channel 25 is used and
TCC is also set to 25. The TCINTEN may also be set to trigger interrupt 25 when the last 1 Kbyte array is
transferred. The CPU starts the EDMA3 transfer by writing to the appropriate bit of the event set register
(ESR.E25). The EDMA3 transfers the first 1 Kbyte array. Upon completion of the first array, intermediate
transfer complete code chaining generates a synchronization event to channel 25, a value specified by the
TCC field. This intermediate transfer completion chaining event causes EDMA3 channel 25 to transfer the
next 1 Kbyte array. This process continues until the transfer parameters are exhausted, at which point the
EDMA3 has completed the 16K byte transfer. This method breaks up a large transfer into smaller packets,
thus providing natural time slices in the transfer such that other events may be processed. Figure 11-39
shows the EDMA3 setup and illustration of the broken up smaller packet transfers.


**Figure 11-39. Smaller Packet Data Transfers Example**

Event 25 (CPU writes 1 to ESR.E25)
ITCCHEN=1, TCC=25 causes
channel 25 to be synchronized again


1K     1K        1K      1K      1K      1K       1K      1K       1K      1K       1K        1K     1K    1K    1K    1K


Time gaps allow other transfers on the same priority level
to be performed


EDMA3 channel 25 setup
ACNT = 1024
BCNT = 16
CCNT = 1
OPT.SYNCDIM = A SYNC
OPT.ITCCHEN = 1
OPT.TCINTEN = 1
OPT.TCC = 25


### 11.3.20 EDMA Events


**Table 11-24. Direct Mapped**

Event Number                                  Event Name                                      Source Module
(1)
0                                             pr1_host[7]                                     PRU-ICSS
1                                             pr1_host[6] (1)                                 PRU-ICSS
2                                             SDTXEVT1                                        MMCHS1
3                                             SDRXEVT1                                        MMCHS1
4                                             Reserved                                        Reserved
5                                             Reserved                                        Reserved
6                                             Reserved                                        Reserved
7                                             Reserved                                        Reserved
8                                             AXEVT0                                          McASP0
9                                             AREVT0                                          McASP0
10                                            AXEVT1                                          McASP1
11                                            AREVT1                                          McASP1
12                                            Open                                            Open
13                                            Open                                            Open
14                                            ePWMEVT0                                        ePWM 0
15                                            ePWMEVT1                                        ePWM 1
16                                            SPIXEVT0                                        McSPI0
17                                            SPIREVT0                                        McSPI0
18                                            SPIXEVT1                                        McSPI0
19                                            SPIREVT1                                        McSPI0
20                                            Open                                            Open
21                                            Open                                            Open
22                                            GPIOEVT0                                        GPIO0
23                                            GPIOEVT1                                        GPIO1
24                                            SDTXEVT0                                        MMCHS0
25                                            SDRXEVT0                                        MMCHS0
26                                            UTXEVT0                                         UART0
27                                            URXEVT0                                         UART0
28                                            UTXEVT1                                         UART1
29                                            URXEVT1                                         UART1
30                                            UTXEVT2                                         UART2
31                                            URXEVT2                                         UART2
32                                            Open                                            Open
33                                            Open                                            Open
34                                            Open                                            Open
35                                            Open                                            Open
36                                            Open                                            Open
37                                            Open                                            Open
38                                            eCAPEVT0                                        eCAP 0
39                                            eCAPEVT1                                        eCAP 1
40                                            CAN_IF1DMA                                      DCAN 0
41                                            CAN_IF2DMA                                      DCAN 0
42                                            SPIXEVT0                                        McSPI1
43                                            SPIREVT0                                        McSPI1
44                                            SPIXEVT1                                        McSPI1
(1)
pr1_host_intr[0:7] corresponds to Host-2 to Host-9 of the PRU-ICSS interrupt controller.


**Table 11-24. Direct Mapped (continued)**

Event Number                          Event Name                                      Source Module
45                                    SPIREVT1                                        McSPI1
46                                    eQEPEVT0                                        eQEP 0
47                                    CAN_IF3DMA                                      DCAN 0
48                                    TINT4                                           Timer 4
49                                    TINT5                                           Timer 5
50                                    TINT6                                           Timer 6
51                                    TINT7                                           Timer 7
52                                    GPMCEVT                                         GPMC
53                                    tsc_adc_FIFO0                                   ADC/TSC
54                                    Open
55                                    Open
56                                    eQEPEVT1                                        eQEP 1
57                                    tsc_adc_FIFO1                                   ADC/TSC
58                                    I2CTXEVT0                                       I2C0
59                                    I2CRXEVT0                                       I2C0
60                                    I2CTXEVT1                                       I2C1
61                                    I2CRXEVT1                                       I2C1
62                                    eCAPEVT2                                        eCAP 2
63                                    eHRPWMEVT2                                      eHRPWM 2


**Table 11-25. Crossbar Mapped**

Event Number                          Event Name                                      Source Module
1                                     SDTXEVT2                                        MMCHS2
2                                     SDRXEVT2                                        MMCHS2
3                                     I2CTXEVT2                                       I2C2
4                                     I2CRXEVT2                                       I2C2
5                                     Open                                            Open
6                                     Open                                            Open
7                                     UTXEVT3                                         UART3
8                                     URXEVT3                                         UART3
9                                     UTXEVT4                                         UART4
10                                    URXEVT4                                         UART4
11                                    UTXEVT5                                         UART5
12                                    URXEVT5                                         UART5
13                                    CAN_IF1DMA                                      DCAN 1
14                                    CAN_IF2DMA                                      DCAN 1
15                                    CAN_IF3DMA                                      DCAN 1
16                                    Open                                            Open
17                                    Open                                            Open
18                                    Open                                            Open
19                                    Open                                            Open
20                                    Open                                            Open
21                                    Open                                            Open
22                                    TINT0                                           Timer 0
23
24                                    TINT2                                           Timer 2


1636Enhanced Direct Memory Access (EDMA)                                          SPRUH73Q – October 2011 – Revised December 2019


**Table 11-25. Crossbar Mapped (continued)**

Event Number                           Event Name                                      Source Module
25                                     TINT3                                           Timer 3
26                                     Open                                            Open
27                                     Open                                            Open
28                                     pi_x_dma_event_intr0                            External pin (XDMA_EVENT_INTR0)
29                                     pi_x_dma_event_intr1                            External pin (XDMA_EVENT_INTR1)
30                                     pi_x_dma_event_intr2                            External pin (XDMA_EVENT_INTR2)
31                                     eQEPEVT2                                        eQEP 2
32                                     GPIOEVT2                                        GPIO2
33                                     Open
34                                     Open
35                                     Open
36                                     Open
37                                     Open
38                                     Open
39                                     Open
40                                     Open
41                                     Open
42                                     Open
43                                     Open
44                                     Open
45                                     Open
46                                     Open
47                                     Open
48                                     Open
49                                     Open
50                                     Open
51                                     Open
52                                     Open
53                                     Open
54                                     Open
55                                     Open
56                                     Open
57                                     Open
58                                     Open
59                                     Open
60                                     Open
61                                     Open
62                                     Open
63                                     Open


## 11.4 EDMA3 Registers


### 11.4.1 EDMA3CC Registers

Table 11-26 lists the memory-mapped registers for the EDMA3CC. All register offset addresses not listed
in Table 11-26 should be considered as reserved locations and the register contents should not be
modified.


**Table 11-26. EDMA3CC Registers**

Offset    Acronym                                Register Name                                            Section
0h      PID                                    Peripheral Identification Register                   Section 11.4.1.1
4h      CCCFG                                  EDMA3CC Configuration Register                       Section 11.4.1.2
10h      SYSCONFIG                              EDMA3CC System Configuration Register                Section 15.1.3.2
100h to   DCHMAP_0 to DCHMAP_63                  DMA Channel Mapping Registers 0-63                   Section 11.4.1.4
1FCh
200h to   QCHMAP_0 to QCHMAP_7                   QDMA Channel Mapping Registers 0-7                   Section 11.4.1.5
21Ch
240h to   DMAQNUM_0 to DMAQNUM_7                 DMA Queue Number Registers 0-7                       Section 11.4.1.6
25Ch
260h     QDMAQNUM                               QDMA Queue Number Register                           Section 11.4.1.7
284h     QUEPRI                                 Queue Priority Register                              Section 11.4.1.8
300h     EMR                                    Event Missed Register                                Section 11.4.1.9
304h     EMRH                                   Event Missed Register High                           Section 11.4.1.10
308h     EMCR                                   Event Missed Clear Register                          Section 11.4.1.11
30Ch     EMCRH                                  Event Missed Clear Register High                     Section 11.4.1.12
310h     QEMR                                   QDMA Event Missed Register                           Section 11.4.1.13
314h     QEMCR                                  QDMA Event Missed Clear Register                     Section 11.4.1.14
318h     CCERR                                  EDMA3CC Error Register                               Section 11.4.1.15
31Ch     CCERRCLR                               EDMA3CC Error Clear Register                         Section 11.4.1.16
320h     EEVAL                                  Error Evaluate Register                              Section 11.4.1.17
340h     DRAE0                                  DMA Region Access Enable Register for Region 0       Section 11.4.1.18
344h     DRAEH0                                 DMA Region Access Enable Register High for           Section 11.4.1.19
Region 0
348h     DRAE1                                  DMA Region Access Enable Register for Region 1       Section 11.4.1.20
34Ch     DRAEH1                                 DMA Region Access Enable Register High for           Section 11.4.1.21
Region 1
350h     DRAE2                                  DMA Region Access Enable Register for Region 2       Section 11.4.1.22
354h     DRAEH2                                 DMA Region Access Enable Register High for           Section 11.4.1.23
Region 2
358h     DRAE3                                  DMA Region Access Enable Register for Region 3       Section 11.4.1.24
35Ch     DRAEH3                                 DMA Region Access Enable Register High for           Section 11.4.1.25
Region 3
360h     DRAE4                                  DMA Region Access Enable Register for Region 4       Section 11.4.1.26
364h     DRAEH4                                 DMA Region Access Enable Register High for           Section 11.4.1.27
Region 4
368h     DRAE5                                  DMA Region Access Enable Register for Region 5       Section 11.4.1.28
36Ch     DRAEH5                                 DMA Region Access Enable Register High for           Section 11.4.1.29
Region 5
370h     DRAE6                                  DMA Region Access Enable Register for Region 6       Section 11.4.1.30
374h     DRAEH6                                 DMA Region Access Enable Register High for           Section 11.4.1.31
Region 6
378h     DRAE7                                  DMA Region Access Enable Register for Region 7       Section 11.4.1.32
37Ch     DRAEH7                                 DMA Region Access Enable Register High for           Section 11.4.1.33
Region 7


1638Enhanced Direct Memory Access (EDMA)                                          SPRUH73Q – October 2011 – Revised December 2019


**Table 11-26. EDMA3CC Registers (continued)**

Offset    Acronym                                 Register Name                                             Section
380h to   QRAE_0 to QRAE_7                        QDMA Region Access Enable Registers for Region        Section 11.4.1.34
39Ch                                             0-7
400h   Q0E0                                    Event Queue 0 Entry 0 Register                        Section 11.4.1.35
404h   Q0E1                                    Event Queue 0 Entry 1 Register                        Section 11.4.1.36
408h   Q0E2                                    Event Queue 0 Entry 2 Register                        Section 11.4.1.37
40Ch   Q0E3                                    Event Queue 0 Entry 3 Register                        Section 11.4.1.38
410h   Q0E4                                    Event Queue 0 Entry 4 Register                        Section 11.4.1.39
414h   Q0E5                                    Event Queue 0 Entry 5 Register                        Section 11.4.1.40
418h   Q0E6                                    Event Queue 0 Entry 6 Register                        Section 11.4.1.41
41Ch   Q0E7                                    Event Queue 0 Entry 7 Register                        Section 11.4.1.42
420h   Q0E8                                    Event Queue 0 Entry 8 Register                        Section 11.4.1.43
424h   Q0E9                                    Event Queue 0 Entry 9 Register                        Section 11.4.1.44
428h   Q0E10                                   Event Queue 0 Entry 10 Register                       Section 11.4.1.45
42Ch   Q0E11                                   Event Queue 0 Entry 11 Register                       Section 11.4.1.46
430h   Q0E12                                   Event Queue 0 Entry 12 Register                       Section 11.4.1.47
434h   Q0E13                                   Event Queue 0 Entry 13 Register                       Section 11.4.1.48
438h   Q0E14                                   Event Queue 0 Entry 14 Register                       Section 11.4.1.49
43Ch   Q0E15                                   Event Queue 0 Entry 15 Register                       Section 11.4.1.50
440h   Q1E0                                    Event Queue 1 Entry 0 Register                        Section 11.4.1.51
444h   Q1E1                                    Event Queue 1 Entry 1 Register                        Section 11.4.1.52
448h   Q1E2                                    Event Queue 1 Entry 2 Register                        Section 11.4.1.53
44Ch   Q1E3                                    Event Queue 1 Entry 3 Register                        Section 11.4.1.54
450h   Q1E4                                    Event Queue 1 Entry 4 Register                        Section 11.4.1.55
454h   Q1E5                                    Event Queue 1 Entry 5 Register                        Section 11.4.1.56
458h   Q1E6                                    Event Queue 1 Entry 6 Register                        Section 11.4.1.57
45Ch   Q1E7                                    Event Queue 1 Entry 7 Register                        Section 11.4.1.58
460h   Q1E8                                    Event Queue 1 Entry 8 Register                        Section 11.4.1.59
464h   Q1E9                                    Event Queue 1 Entry 9 Register                        Section 11.4.1.60
468h   Q1E10                                   Event Queue 1 Entry 10 Register                       Section 11.4.1.61
46Ch   Q1E11                                   Event Queue 1 Entry 11 Register                       Section 11.4.1.62
470h   Q1E12                                   Event Queue 1 Entry 12 Register                       Section 11.4.1.63
474h   Q1E13                                   Event Queue 1 Entry 13 Register                       Section 11.4.1.64
478h   Q1E14                                   Event Queue 1 Entry 14 Register                       Section 11.4.1.65
47Ch   Q1E15                                   Event Queue 1 Entry 15 Register                       Section 11.4.1.66
480h   Q2E0                                    Event Queue 2 Entry 0 Register                        Section 11.4.1.67
484h   Q2E1                                    Event Queue 2 Entry 1 Register                        Section 11.4.1.68
488h   Q2E2                                    Event Queue 2 Entry 2 Register                        Section 11.4.1.69
48Ch   Q2E3                                    Event Queue 2 Entry 3 Register                        Section 11.4.1.70
490h   Q2E4                                    Event Queue 2 Entry 4 Register                        Section 11.4.1.71
494h   Q2E5                                    Event Queue 2 Entry 5 Register                        Section 11.4.1.72
498h   Q2E6                                    Event Queue 2 Entry 6 Register                        Section 11.4.1.73
49Ch   Q2E7                                    Event Queue 2 Entry 7 Register                        Section 11.4.1.74
4A0h   Q2E8                                    Event Queue 2 Entry 8 Register                        Section 11.4.1.75
4A4h   Q2E9                                    Event Queue 2 Entry 9 Register                        Section 11.4.1.76
4A8h   Q2E10                                   Event Queue 2 Entry 10 Register                       Section 11.4.1.77
4ACh   Q2E11                                   Event Queue 2 Entry 11 Register                       Section 11.4.1.78
4B0h   Q2E12                                   Event Queue 2 Entry 12 Register                       Section 11.4.1.79
4B4h   Q2E13                                   Event Queue 2 Entry 13 Register                       Section 11.4.1.80


**Table 11-26. EDMA3CC Registers (continued)**

Offset    Acronym                                 Register Name                                            Section
4B8h      Q2E14                                   Event Queue 2 Entry 14 Register                      Section 11.4.1.81
4BCh      Q2E15                                   Event Queue 2 Entry 15 Register                      Section 11.4.1.82
600h to    QSTAT_0 to QSTAT_2                      Queue Status Registers 0-2                           Section 11.4.1.83
608h
620h     QWMTHRA                                 Queue Watermark Threshold A Register                 Section 11.4.1.84
640h     CCSTAT                                  EDMA3CC Status Register                              Section 11.4.1.85
800h     MPFAR                                   Memory Protection Fault Address Register             Section 11.4.1.86
804h     MPFSR                                   Memory Protection Fault Status Register              Section 11.4.1.87
808h     MPFCR                                   Memory Protection Fault Command Register             Section 11.4.1.88
80Ch      MPPAG                                   Memory Protection Page Attribute Register Global     Section 11.4.1.89
810h to    MPPA_0 to MPPA_7                        Memory Protection Page Attribute Registers           Section 11.4.1.90
82Ch
1000h     ER                                      Event Register                                       Section 11.4.1.91
1004h     ERH                                     Event Register High                                  Section 11.4.1.92
1008h     ECR                                     Event Clear Register                                 Section 11.4.1.93
100Ch     ECRH                                    Event Clear Register High                            Section 11.4.1.94
1010h     ESR                                     Event Set Register                                   Section 11.4.1.95
1014h     ESRH                                    Event Set Register High                              Section 11.4.1.96
1018h     CER                                     Chained Event Register                               Section 11.4.1.97
101Ch     CERH                                    Chained Event Register High                          Section 11.4.1.98
1020h     EER                                     Event Enable Register                                Section 11.4.1.99
1024h     EERH                                    Event Enable Register High                          Section 11.4.1.100
1028h     EECR                                    Event Enable Clear Register                         Section 11.4.1.101
102Ch     EECRH                                   Event Enable Clear Register High                    Section 11.4.1.102
1030h     EESR                                    Event Enable Set Register                           Section 11.4.1.103
1034h     EESRH                                   Event Enable Set Register High                      Section 11.4.1.104
1038h     SER                                     Secondary Event Register                            Section 11.4.1.105
103Ch     SERH                                    Secondary Event Register High                       Section 11.4.1.106
1040h     SECR                                    Secondary Event Clear Register                      Section 11.4.1.107
1044h     SECRH                                   Secondary Event Clear Register High                 Section 11.4.1.108
1050h     IER                                     Interrupt Enable Register                           Section 11.4.1.109
1054h     IERH                                    Interrupt Enable Register High                      Section 11.4.1.110
1058h     IECR                                    Interrupt Enable Clear Register                     Section 11.4.1.111
105Ch     IECRH                                   Interrupt Enable Clear Register High                Section 11.4.1.112
1060h     IESR                                    Interrupt Enable Set Register                       Section 11.4.1.113
1064h     IESRH                                   Interrupt Enable Set Register High                  Section 11.4.1.114
1068h     IPR                                     Interrupt Pending Register                          Section 11.4.1.115
106Ch     IPRH                                    Interrupt Pending Register High                     Section 11.4.1.116
1070h     ICR                                     Interrupt Clear Register                            Section 11.4.1.117
1074h     ICRH                                    Interrupt Clear Register High                       Section 11.4.1.118
1078h     IEVAL                                   Interrupt Evaluate Register                         Section 11.4.1.119
1080h     QER                                     QDMA Event Register                                 Section 11.4.1.120
1084h     QEER                                    QDMA Event Enable Register                          Section 11.4.1.121
1088h     QEECR                                   QDMA Event Enable Clear Register                    Section 11.4.1.122
108Ch     QEESR                                   QDMA Event Enable Set Register                      Section 11.4.1.123
1090h     QSER                                    QDMA Secondary Event Register                       Section 11.4.1.124
1094h     QSECR                                   QDMA Secondary Event Clear Register                 Section 11.4.1.125


#### 11.4.1.1 PID Register (offset = 0h) [reset = 0h]

PID is shown in Figure 11-40 and described in Table 11-27.
The peripheral identification register (PID) uniquely identifies the EDMA3CC and the specific revision of
the EDMA3CC.


**Figure 11-40. PID Register**

15       14           13   12     11        10        9         8          7      6         5      4       3        2        1        0
PID
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-27. PID Register Field Descriptions**

Bit         Field                    Type            Reset          Description
15-0        PID                      R               0h             Peripheral identifier uniquely identifies the EDMA3CC and the
specific revision of the EDMA3CC.
Value 0 to FFFF FFFFh.
Reset value for PID[31] to PID[16] is 4001h.
Peripheral identifier uniquely identifies the EDMA3CC and the
specific revision of the EDMA3CC.
Value 0 to FFFF FFFFh.
Reset value for PID[15] to PID[0] is 4C00h.


#### 11.4.1.2 CCCFG Register (offset = 4h) [reset = 3224445h]

CCCFG is shown in Figure 11-41 and described in Table 11-28.
The EDMA3CC configuration register (CCCFG) provides the features/resources for the EDMA3CC in a
particular device.


**Figure 11-41. CCCFG Register**

31               30             29                    28               27                    26           25          24
RESERVED                                                         MP_EXIST   CHMAP_EXIST
R-0h                                                             R-1h        R-1h
23               22             21                    20             19                      18        17              16
RESERVED                        NUM_REGN                     RESERVED                         NUM_EVQUE
R-0h                            R-2h                         R-0h                              R-2h
15                   14            13                     12             11                      10         9              8
RESERVED                         NUM_PAENTRY                             RESERVED                          NUM_INTCH
R-0h                               R-4h                                  R-0h                               R-4h
7                   6             5                      4               3                      2         1               0
RESERVED                         NUM_QDMACH                              RESERVED                         NUM_DMACH
R-0h                               R-4h                                  R-0h                              R-5h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-28. CCCFG Register Field Descriptions**

Bit       Field                   Type             Reset          Description
31-26         RESERVED                R                0h
25        MP_EXIST                R                1h             Memory protection existence.
0h = Reserved.
1h = Memory protection logic included.
24        CHMAP_EXIST             R                1h             Channel mapping existence.
0h = Reserved.
1h = Channel mapping logic included.
23-22         RESERVED                R                0h
21-20         NUM_REGN                R                2h             Number of MP and shadow regions.
0h = Reserved.
1h = Reserved
2h = 4 regions.
3h = Reserved.
19        RESERVED                R                0h
18-16         NUM_EVQUE               R                2h             Number of queues/number of TCs.
0h = Reserved.
1h = Reserved.
2h = 3 EDMA3TCs/Event Queues
3h = Reserved from 3h to 7h.
7h = Reserved.
15        RESERVED                R                0h
14-12         NUM_PAENTRY             R                4h             Number of PaRAM sets.
0h = Reserved from 0h to 3h.
3h = Reserved
4h = 256 PaRAM sets.
5h = Reserved from 5h to 7h.
7h = Reserved.
11        RESERVED                R                0h


**Table 11-28. CCCFG Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
10-8     NUM_INTCH               R               4h             Number of interrupt channels.
0h = Reserved from 0h to 3h.
3h = Reserved.
4h = 64 interrupt channels.
5h = Reserved from 5h to 7.
7h = Reserved.
7       RESERVED                R               0h
6-4      NUM_QDMACH              R               4h             Number of QDMA channels.
0h = Reserved from 0h to 3h.
3h = Reserved.
4h = 8 QDMA channels.
5h = Reserved from 5h to 7.
7h = Reserved.
3       RESERVED                R               0h
2-0      NUM_DMACH               R               5h             Number of DMA channels.
0h = Reserved from 0h to 4h.
4h = Reserved.
5h = 64 DMA channels.
6h = Reserved.
7h = Reserved.


#### 11.4.1.3 SYSCONFIG Register (offset = 10h) [reset = 8h]

SYSCONFIG is shown in Figure 15-4 and described in Table 15-7.
The EDMA3CC system configuration register is used for clock management configuration.


**Figure 11-42. SYSCONFIG Register**

31               30             29                    28               27                    26            25                 24
RESERVED
R-0h
23               22             21                    20               19                    18            17                 16
RESERVED
R-0h
15               14             13                    12               11                    10             9                  8
RESERVED
R-0h
7              6              5                     4                 3                    2            1                  0
RESERVED                        RESERVED                              IDLEMODE                  RESERVED           RESERVED
R-0h                            R-0h                                  R/W-2h                    R-0h               R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-29. SYSCONFIG Register Field Descriptions**

Bit       Field                   Type             Reset          Description
31-6          RESERVED                R                0h
5-4       RESERVED                R                0h
3-2       IDLEMODE                R/W              2h             Configuration of the local target state management mode.
By definition, target can handle read/write transaction as long as it is
out of IDLE state.
0h = Force-idle mode: local target's idle state follows (acknowledges)
the system's idle requests unconditionally, i.e. regardless of the IP
module's internal requirements. Backup mode, for debug only.
1h = No-idle mode: local target never enters idle state. Backup
mode, for debug only.
2h = Smart-idle mode: local target's idle state eventually follows
(acknowledges) the system's idle requests, depending on the IP
module's internal requirements. IP module shall not generate (IRQ-
or DMA-request-related) wakeup events.
3h = Smart-idle wakeup-capable mode: local target's idle state
eventually follows (acknowledges) the system's idle requests,
depending on the IP module's internal requirements. IP module may
generate (IRQ- or DMA-request-related) wakeup events when in idle
state. Mode is only relevant if the appropriate IP module "swakeup"
output(s) is (are) implemented.
1         RESERVED                R                0h
0         RESERVED                R                0h


#### 11.4.1.4 DCHMAP_0 to DCHMAP_63 Register (offset = 100h to 1FCh) [reset = 0h]

DCHMAP_0 to DCHMAP_63 is shown in Figure 11-43 and described in Table 11-30.


**Figure 11-43. DCHMAP_0 to DCHMAP_63 Register**

31       30           29   28     27         26        25       24      23         22       21      20     19      18       17     16
RESERVED
R-0h
15      14            13   12     11         10      9           8        7        6         5      4       3       2    1         0
RESERVED                                          PAENTRY                                                       RESERVED
R-0h                                            R/W-0h                                                         R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-30. DCHMAP_0 to DCHMAP_63 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-14        RESERVED                  R               0h
13-5        PAENTRY                   R/W             0h             Points to the PaRAM set number for DMA channel n.
Value 0 to FFh.
4-0         RESERVED                  R               0h


#### 11.4.1.5 QCHMAP_0 to QCHMAP_7 Register (offset = 200h to 21Ch) [reset = 0h]

QCHMAP_0 to QCHMAP_7 is shown in Figure 11-44 and described in Table 11-31.
Each QDMA channel in EDMA3CC can be associated with any PaRAM set available on the device.
Furthermore, the specific trigger word (0-7) of the PaRAM set can be programmed. The PaRAM set
association and trigger word for every QDMA channel register is configurable using the QDMA channel
map register (QCHMAPn). At reset the QDMA channel map registers for all QDMA channels point to
PaRAM set 0. If an application makes use of both a DMA channel that points to PaRAM set 0 and any
QDMA channels, ensure that QCHMAP is programmed appropriately to point to a different PaRAM entry.


**Figure 11-44. QCHMAP_0 to QCHMAP_7 Register**

31               30             29                    28               27                    26            25                24
RESERVED
R-0h
23               22             21                    20               19                    18            17                16
RESERVED
R-0h
15               14             13                    12               11                    10             9                 8
RESERVED                                                               PAENTRY
R-0h                                                                  R/W-0h
7                 6             5                     4               3                      2              1                 0
PAENTRY                                             TRWORD                                       RESERVED
R/W-0h                                              R/W-0h                                        R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-31. QCHMAP_0 to QCHMAP_7 Register Field Descriptions**

Bit      Field                    Type             Reset          Description
31-14        RESERVED                 R                0h
13-5         PAENTRY                  R/W              0h             PAENTRY points to the PaRAM set number for QDMA channel .
0h = Parameter entry 0 through 255, from 0 to FFh.
1h = Reserved, from 100h to 1FFh. Always write 0 to this bit. Writes
of 1 to this bit are not supported and attempts to do so may result in
undefine behavior.
4-2      TRWORD                   R/W              0h             Points to the specific trigger word of the PaRAM set defined by
PAENTRY.
A write to the trigger word results in a QDMA event being
recognized.
1-0      RESERVED                 R                0h


#### 11.4.1.6 DMAQNUM_0 to DMAQNUM_7 Register (offset = 240h to 25Ch) [reset = 0h]

DMAQNUM_0 to DMAQNUM_7 is shown in Figure 11-45 and described in Table 11-32.
The DMA channel queue number register (DMAQNUMn) allows programmability of each of the 64 DMA
channels in the EDMA3CC to submit its associated synchronization event to any event queue in the
EDMA3CC. At reset, all channels point to event queue 0. Because the event queues in EDMA3CC have a
fixed association to the transfer controllers, that is, Q0 TRs are submitted to TC0, Q1 TRs are submitted
to TC1, etc., by programming DMAQNUM for a particular DMA channel also dictates which transfer
controller is utilized for the data movement (or which EDMA3TC receives the TR request).


**Figure 11-45. DMAQNUM_0 to DMAQNUM_7 Register**

31                30               29                    28             27                      26            25                 24
RESERVED                              E7                                RESERVED                                 E6
R-0h                              R/W-0h                                R-0h                                 R/W-0h
23                22               21                    20             19                      18            17                 16
RESERVED                              E5                                RESERVED                                 E4
R-0h                              R/W-0h                                R-0h                                 R/W-0h
15                14               13                    12             11                      10            9                  8
RESERVED                              E3                                RESERVED                                 E2
R-0h                              R/W-0h                                R-0h                                 R/W-0h
7                 6               5                     4               3                      2             1                  0
RESERVED                              E1                                RESERVED                                 E0
R-0h                              R/W-0h                                R-0h                                 R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-32. DMAQNUM_0 to DMAQNUM_7 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31        RESERVED                  R               0h
30-28       E7                        R/W             0h             DMA queue number.
Contains the event queue number to be used for the corresponding
DMA channel.
Programming DMAQNUM for an event queue number to a value
more then the number of queues available in the EDMA3CC results
in undefined behavior.
On DMAQNUM0, E[30] to E[28] is E7.
On DMAQNUM1, E[30] to E[28] is E15.
On DMAQNUM2, E[30] to E[28] is E23.
On DMAQNUM3, E[30] to E[28] is E31.
On DMAQNUM4, E[30] to E[28] is E39.
On DMAQNUM5, E[30] to E[28] is E47.
On DMAQNUM6, E[30] to E[28] is E55.
On DMAQNUM7, E[30] to E[28] is E63.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
27        RESERVED                  R               0h


**Table 11-32. DMAQNUM_0 to DMAQNUM_7 Register Field Descriptions (continued)**

Bit       Field                   Type            Reset          Description
26-24         E6                      R/W             0h             DMA queue number.
Contains the event queue number to be used for the corresponding
DMA channel.
Programming DMAQNUM for an event queue number to a value
more then the number of queues available in the EDMA3CC results
in undefined behavior.
On DMAQNUM0, E[26] to E[24] is E6.
On DMAQNUM1, E[26] to E[24] is E14.
On DMAQNUM2, E[26] to E[24] is E22.
On DMAQNUM3, E[26] to E[24] is E30.
On DMAQNUM4, E[26] to E[24] is E38.
On DMAQNUM5, E[26] to E[24] is E46.
On DMAQNUM6, E[26] to E[24] is E54.
On DMAQNUM7, E[26] to E[24] is E62.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
23        RESERVED                R               0h
22-20         E5                      R/W             0h             DMA queue number.
Contains the event queue number to be used for the corresponding
DMA channel.
Programming DMAQNUM for an event queue number to a value
more then the number of queues available in the EDMA3CC results
in undefined behavior.
On DMAQNUM0, E[22] to E[20] is E5.
On DMAQNUM1, E[22] to E[20] is E13.
On DMAQNUM2, E[22] to E[20] is E21.
On DMAQNUM3, E[22] to E[20] is E29.
On DMAQNUM4, E[22] to E[20] is E37.
On DMAQNUM5, E[22] to E[20] is E45.
On DMAQNUM6, E[22] to E[20] is E53.
On DMAQNUM7, E[22] to E[20] is E61.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
19        RESERVED                R               0h


**Table 11-32. DMAQNUM_0 to DMAQNUM_7 Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
18-16     E4                      R/W             0h             DMA queue number.
Contains the event queue number to be used for the corresponding
DMA channel.
Programming DMAQNUM for an event queue number to a value
more then the number of queues available in the EDMA3CC results
in undefined behavior.
On DMAQNUM0, E[18] to E[16] is E4.
On DMAQNUM1, E[18] to E[16] is E12.
On DMAQNUM2, E[18] to E[16] is E20.
On DMAQNUM3, E[18] to E[16] is E28.
On DMAQNUM4, E[18] to E[16] is E36.
On DMAQNUM5, E[18] to E[16] is E44.
On DMAQNUM6, E[18] to E[16] is E52.
On DMAQNUM7, E[18] to E[16] is E60.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
15      RESERVED                R               0h
14-12     E3                      R/W             0h             DMA queue number.
Contains the event queue number to be used for the corresponding
DMA channel.
Programming DMAQNUM for an event queue number to a value
more then the number of queues available in the EDMA3CC results
in undefined behavior.
On DMAQNUM0, E[14] to E[12] is E3.
On DMAQNUM1, E[14] to E[12] is E11.
On DMAQNUM2, E[14] to E[12] is E19.
On DMAQNUM3, E[14] to E[12] is E27.
On DMAQNUM4, E[14] to E[12] is E35.
On DMAQNUM5, E[14] to E[12] is E43.
On DMAQNUM6, E[14] to E[12] is E51.
On DMAQNUM7, E[14] to E[12] is E59.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
11      RESERVED                R               0h


**Table 11-32. DMAQNUM_0 to DMAQNUM_7 Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
10-8         E2                      R/W             0h             DMA queue number.
Contains the event queue number to be used for the corresponding
DMA channel.
Programming DMAQNUM for an event queue number to a value
more then the number of queues available in the EDMA3CC results
in undefined behavior.
On DMAQNUM0, E[10] to E[8] is E2.
On DMAQNUM1, E[10] to E[8] is E10.
On DMAQNUM2, E[10] to E[8] is E18.
On DMAQNUM3, E[10] to E[8] is E26.
On DMAQNUM4, E[10] to E[8] is E34.
On DMAQNUM5, E[10] to E[8] is E42.
On DMAQNUM6, E[10] to E[8] is E50.
On DMAQNUM7, E[10] to E[8] is E58.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
7        RESERVED                R               0h
6-4      E1                      R/W             0h             DMA queue number.
Contains the event queue number to be used for the corresponding
DMA channel.
Programming DMAQNUM for an event queue number to a value
more then the number of queues available in the EDMA3CC results
in undefined behavior.
On DMAQNUM0, E[6] to E[4] is E1.
On DMAQNUM1, E[6] to E[4] is E9.
On DMAQNUM2, E[6] to E[4] is E17.
On DMAQNUM3, E[6] to E[4] is E25.
On DMAQNUM4, E[6] to E[4] is E33.
On DMAQNUM5, E[6] to E[4] is E41.
On DMAQNUM6, E[6] to E[4] is E49.
On DMAQNUM7, E[6] to E[4] is E57.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
3        RESERVED                R               0h


**Table 11-32. DMAQNUM_0 to DMAQNUM_7 Register Field Descriptions (continued)**

Bit      Field                   Type          Reset          Description
2-0      E0                      R/W           0h             DMA queue number.
Contains the event queue number to be used for the corresponding
DMA channel.
Programming DMAQNUM for an event queue number to a value
more then the number of queues available in the EDMA3CC results
in undefined behavior.
On DMAQNUM0, E[2] to E[0] is E0.
On DMAQNUM1, E[2] to E[0] is E8.
On DMAQNUM2, E[2] to E[0] is E16.
On DMAQNUM3, E[2] to E[0] is E24.
On DMAQNUM4, E[2] to E[0] is E32.
On DMAQNUM5, E[2] to E[0] is E40.
On DMAQNUM6, E[2] to E[0] is E48.
On DMAQNUM7, E[2] to E[0] is E56.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.


#### 11.4.1.7 QDMAQNUM Register (offset = 260h) [reset = 0h]

QDMAQNUM is shown in Figure 11-46 and described in Table 11-33.
The QDMA channel queue number register (QDMAQNUMn) is used to program all the QDMA channels in
the EDMA3CC to submit the associated QDMA event to any of the event queues in the EDMA3CC.


**Figure 11-46. QDMAQNUM Register**

31                   30             29                   28             27                      26            25                 24
RESERVED                               E7                               RESERVED                                 E6
R-0h                               R/W-0h                               R-0h                                 R/W-0h
23                   22             21                   20             19                      18            17                 16
RESERVED                               E5                               RESERVED                                 E4
R-0h                               R/W-0h                               R-0h                                 R/W-0h
15                   14             13                   12             11                      10            9                  8
RESERVED                               E3                               RESERVED                                 E2
R-0h                               R/W-0h                               R-0h                                 R/W-0h
7                   6              5                    4               3                      2             1                  0
RESERVED                               E1                               RESERVED                                 E0
R-0h                               R/W-0h                               R-0h                                 R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-33. QDMAQNUM Register Field Descriptions**

Bit       Field                   Type            Reset          Description
31        RESERVED                R               0h
30-28         E7                      R/W             0h             QDMA queue number.
Contains the event queue number to be used for the corresponding
QDMA channel.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
27        RESERVED                R               0h
26-24         E6                      R/W             0h             QDMA queue number.
Contains the event queue number to be used for the corresponding
QDMA channel.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
23        RESERVED                R               0h


**Table 11-33. QDMAQNUM Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
22-20     E5                      R/W             0h             QDMA queue number.
Contains the event queue number to be used for the corresponding
QDMA channel.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
19      RESERVED                R               0h
18-16     E4                      R/W             0h             QDMA queue number.
Contains the event queue number to be used for the corresponding
QDMA channel.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
15      RESERVED                R               0h
14-12     E3                      R/W             0h             QDMA queue number.
Contains the event queue number to be used for the corresponding
QDMA channel.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
11      RESERVED                R               0h
10-8     E2                      R/W             0h             QDMA queue number.
Contains the event queue number to be used for the corresponding
QDMA channel.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
7       RESERVED                R               0h


**Table 11-33. QDMAQNUM Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
6-4      E1                      R/W             0h             QDMA queue number.
Contains the event queue number to be used for the corresponding
QDMA channel.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.
3        RESERVED                R               0h
2-0      E0                      R/W             0h             QDMA queue number.
Contains the event queue number to be used for the corresponding
QDMA channel.
0h = Event n is queued on Q0.
1h = Event n is queued on Q1.
2h = Event n is queued on Q2.
3h = Reserved, from 3h to 7h. Always write 0 to this bit; writes of 1
to this bit are not supported and attempts to do so may result in
undefined behavior.
7h = Reserved. Always write 0 to this bit; writes of 1 to this bit are
not supported and attempts to do so may result in undefined
behavior.


#### 11.4.1.8 QUEPRI Register (offset = 284h) [reset = 777h]

QUEPRI is shown in Figure 11-47 and described in Table 11-34.
The queue priority register (QUEPRI) allows you to change the priority of the individual queues and the
priority of the transfer request (TR) associated with the events queued in the queue. Because the queue to
EDMA3TC mapping is fixed, programming QUEPRI essentially governs the priority of the associated
transfer controller(s) read/write commands with respect to the other bus masters in the device. You can
modify the EDMA3TC priority to obtain the desired system performance.


**Figure 11-47. QUEPRI Register**

31               30                29                    28               27                    26            25                24
RESERVED
R-0h
23               22                21                    20               19                    18            17                16
RESERVED
R-0h
15               14              13                      12               11                    10           9                   8
RESERVED                                                                     PRIQ2
R-0h                                                                       R/W-7h
7                 6               5                      4               3                      2            1                   0
RESERVED                            PRIQ1                                RESERVED                              PRIQ0
R-0h                              R/W-7h                                 R-0h                                R/W-7h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-34. QUEPRI Register Field Descriptions**

Bit       Field                      Type             Reset          Description
31-11      RESERVED                   R                0h
10-8      PRIQ2                      R/W              7h             Priority level for queue 2.
Dictates the priority level used by TC2 relative to other masters in
the device.
A value of 0 means highest priority and a value of 7 means lowest
priority.
7        RESERVED                   R                0h
6-4       PRIQ1                      R/W              7h             Priority level for queue 1.
Dictates the priority level used by TC1 relative to other masters in
the device.
A value of 0 means highest priority and a value of 7 means lowest
priority.
3        RESERVED                   R                0h
2-0       PRIQ0                      R/W              7h             Priority level for queue 0.
Dictates the priority level used by TC0 relative to other masters in
the device.
A value of 0 means highest priority and a value of 7 means lowest
priority.


#### 11.4.1.9 EMR Register (offset = 300h) [reset = 0h]

EMR is shown in Figure 11-48 and described in Table 11-35.
For a particular DMA channel, if a second event is received prior to the first event getting cleared/serviced,
the bit corresponding to that channel is set/asserted in the event missed registers (EMR/EMRH). All
trigger types are treated individually, that is, manual triggered (ESR/ESRH), chain triggered (CER/CERH),
and event triggered (ER/ERH) are all treated separately. The EMR/EMRH bits for a channel are also set if
an event on that channel encounters a NULL entry (or a NULL TR is serviced). If any EMR/EMRH bit is
set (and all errors, including bits in other error registers (QEMR, CCERR) were previously cleared), the
EDMA3CC generates an error interrupt. For details on EDMA3CC error interrupt generation, see Error
Interrupts. This register is part of a set of registers that provide information on missed DMA and/or QDMA
events, and instances when event queue thresholds are exceeded. If any of the bits in these registers is
set, it results in the EDMA3CC generating an error interrupt.


**Figure 11-48. EMR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6    5    4   3    2   1    0
En
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-35. EMR Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       En                         R               0h             Channel 0 to 31 event missed.
En is cleared by writing a 1 to the corresponding bit in the event
missed clear register (EMCR).
0h = No missed event.
1h = Missed event occurred.


#### 11.4.1.10 EMRH Register (offset = 304h) [reset = 0h]

EMRH is shown in Figure 11-49 and described in Table 11-36.
For a particular DMA channel, if a second event is received prior to the first event getting cleared/serviced,
the bit corresponding to that channel is set/asserted in the event missed registers (EMR/EMRH). All
trigger types are treated individually, that is, manual triggered (ESR/ESRH), chain triggered (CER/CERH),
and event triggered (ER/ERH) are all treated separately. The EMR/EMRH bits for a channel are also set if
an event on that channel encounters a NULL entry (or a NULL TR is serviced). If any EMR/EMRH bit is
set (and all errors, including bits in other error registers (QEMR, CCERR) were previously cleared), the
EDMA3CC generates an error interrupt. For details on EDMA3CC error interrupt generation, see Error
Interrupts. This register is part of a set of registers that provide information on missed DMA and/or QDMA
events, and instances when event queue thresholds are exceeded. If any of the bits in these registers is
set, it results in the EDMA3CC generating an error interrupt.


**Figure 11-49. EMRH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6    5    4   3    2   1     0
En
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-36. EMRH Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     En                          R               0h             Channel 32 to 63 event missed.
En is cleared by writing a 1 to the corresponding bit in the event
missed clear register high (EMCRH).
0h = No missed event.
1h = Missed event occurred.


#### 11.4.1.11 EMCR Register (offset = 308h) [reset = 0h]

EMCR is shown in Figure 11-50 and described in Table 11-37.
Once a missed event is posted in the event missed registers (EMR/EMRH), the bit remains set and you
need to clear the set bit(s). This is done by way of CPU writes to the event missed clear registers
(EMCR/EMCRH). Writing a 1 to any of the bits clears the corresponding missed event (bit) in EMR/EMRH;
writing a 0 has no effect. This register is part of a set of registers that provide information on missed DMA
and/or QDMA events, and instances when event queue thresholds are exceeded. If any of the bits in
these registers is set, it results in the EDMA3CC generating an error interrupt.


**Figure 11-50. EMCR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8    7    6   5    4    3   2    1    0
En
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-37. EMCR Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       En                         W               0h             Event missed 0 to 31 clear.
All error bits must be cleared before additional error interrupts will be
asserted by the EDMA3CC.
0h = No effect.
1h = Corresponding missed event bit in the event missed register
(EMR) is cleared (En = 0).


#### 11.4.1.12 EMCRH Register (offset = 30Ch) [reset = 0h]

EMCRH is shown in Figure 11-51 and described in Table 11-38.
Once a missed event is posted in the event missed registers (EMR/EMRH), the bit remains set and you
need to clear the set bit(s). This is done by way of CPU writes to the event missed clear registers
(EMCR/EMCRH). Writing a 1 to any of the bits clears the corresponding missed event (bit) in EMR/EMRH;
writing a 0 has no effect. This register is part of a set of registers that provide information on missed DMA
and/or QDMA events, and instances when event queue thresholds are exceeded. If any of the bits in
these registers is set, it results in the EDMA3CC generating an error interrupt.


**Figure 11-51. EMCRH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8    7    6   5    4    3   2    1    0
En
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-38. EMCRH Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     En                          W               0h             Event missed 32 to 63 clear.
All error bits must be cleared before additional error interrupts will be
asserted by the EDMA3CC.
0h = No effect.
1h = Corresponding missed event bit in the event missed register
high (EMRH) is cleared (En = 0).


#### 11.4.1.13 QEMR Register (offset = 310h) [reset = 0h]

QEMR is shown in Figure 11-52 and described in Table 11-39.
For a particular QDMA channel, if two QDMA events are detected without the first event getting
cleared/serviced, the bit corresponding to that channel is set/asserted in the QDMA event missed register
(QEMR). The QEMR bits for a channel are also set if a QDMA event on the channel encounters a NULL
entry (or a NULL TR is serviced). If any QEMR bit is set (and all errors, including bits in other error
registers (EMR/EMRH, CCERR) were previously cleared), the EDMA3CC generates an error interrupt. For
details on EDMA3CC error interrupt generation, see Error Interrupts. This register is part of a set of
registers that provide information on missed DMA and/or QDMA events, and instances when event queue
thresholds are exceeded. If any of the bits in these registers is set, it results in the EDMA3CC generating
an error interrupt.


**Figure 11-52. QEMR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3    2   1     0
RESERVED                                                                                   En
R-0h                                                                                   R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-39. QEMR Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-8       RESERVED                   R               0h
7-0    En                         R               0h             Channel 0 to 7 QDMA event missed.
En is cleared by writing a 1 to the corresponding bit in the QDMA
event missed clear register (QEMCR).
0h = No missed event.
1h = Missed event occurred.


#### 11.4.1.14 QEMCR Register (offset = 314h) [reset = 0h]

QEMCR is shown in Figure 11-53 and described in Table 11-40.
Once a missed event is posted in the QDMA event missed registers (QEMR), the bit remains set and you
need to clear the set bit(s). This is done by way of CPU writes to the QDMA event missed clear registers
(QEMCR). Writing a 1 to any of the bits clears the corresponding missed event (bit) in QEMR; writing a 0
has no effect. This register is part of a set of registers that provide information on missed DMA and/or
QDMA events, and instances when event queue thresholds are exceeded. If any of the bits in these
registers is set, it results in the EDMA3CC generating an error interrupt.


**Figure 11-53. QEMCR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8    7    6   5    4   3    2    1    0
RESERVED                                                                                      En
R-0h                                                                                      W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-40. QEMCR Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-8     RESERVED                    R               0h
7-0      En                          W               0h             QDMA event missed clear.
All error bits must be cleared before additional error interrupts will be
asserted by the EDMA3CC.
0h = No effect.
1h = Corresponding missed event bit in the QDMA event missed
register (QEMR) is cleared (En= 0).


#### 11.4.1.15 CCERR Register (offset = 318h) [reset = 0h]

CCERR is shown in Figure 11-54 and described in Table 11-41.
The EDMA3CC error register (CCERR) indicates whether or not at any instant of time the number of
events queued up in any of the event queues exceeds or equals the threshold/watermark value that is set
in the queue watermark threshold register (QWMTHRA). Additionally, CCERR also indicates if when the
number of outstanding TRs that have been programmed to return transfer completion code (TRs which
have the TCINTEN or TCCHEN bit in OPT set) to the EDMA3CC has exceeded the maximum allowed
value of 63. If any bit in CCERR is set (and all errors, including bits in other error registers (EMR/EMRH,
QEMR) were previously cleared), the EDMA3CC generates an error interrupt. For details on EDMA3CC
error interrupt generation, see Error Interrupts. Once the error bits are set in CCERR, they can only be
cleared by writing to the corresponding bits in the EDMA3CC error clear register (CCERRCLR). This
register is part of a set of registers that provide information on missed DMA and/or QDMA events, and
instances when event queue thresholds are exceeded. If any of the bits in these registers is set, it results
in the EDMA3CC generating an error interrupt.


**Figure 11-54. CCERR Register**

31               30             29                    28               27                    26          25                24
RESERVED
R-0h
23               22             21                20                   19                    18          17                16
RESERVED                                                                 TCCERR
R-0h                                                                     R-0h
15               14             13                    12               11                    10           9                 8
RESERVED
R-0h
7              6              5                     4                 3               2               1                0
RESERVED                                                 QTHRXCD2        QTHRXCD1         QTHRXCD0
R-0h                                                     R-0h            R-0h             R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-41. CCERR Register Field Descriptions**

Bit       Field                   Type             Reset          Description
31-17         RESERVED                R                0h
16        TCCERR                  R                0h             Transfer completion code error.
TCCERR is cleared by writing a 1 to the corresponding bit in the
EDMA3CC error clear register (CCERRCLR).
0h = Total number of allowed TCCs outstanding has not been
reached.
1h = Total number of allowed TCCs has been reached.
15-3          RESERVED                R                0h
2         QTHRXCD2                R                0h             Queue threshold error for queue 2.
QTHRXCD2 is cleared by writing a 1 to the corresponding bit in the
EDMA3CC error clear register (CCERRCLR).
0h = Watermark/threshold has not been exceeded.
1h = Watermark/threshold has been exceeded.
1         QTHRXCD1                R                0h             Queue threshold error for queue 1 .
QTHRXCD1 is cleared by writing a 1 to the corresponding bit in the
EDMA3CC error clear register (CCERRCLR).
0h = Watermark/threshold has not been exceeded.
1h = Watermark/threshold has been exceeded.
0         QTHRXCD0                R                0h             Queue threshold error for queue 0.
QTHRXCD0 is cleared by writing a 1 to the corresponding bit in the
EDMA3CC error clear register (CCERRCLR).
0h = Watermark/threshold has not been exceeded.
1h = Watermark/threshold has been exceeded.


#### 11.4.1.16 CCERRCLR Register (offset = 31Ch) [reset = 0h]

CCERRCLR is shown in Figure 11-55 and described in Table 11-42.
The EDMA3CC error clear register (CCERRCLR) is used to clear any error bits that are set in the
EDMA3CC error register (CCERR). In addition, CCERRCLR also clears the values of some bit fields in
the queue status registers (QSTATn) associated with a particular event queue. Writing a 1 to any of the
bits clears the corresponding bit in CCERR; writing a 0 has no effect. This register is part of a set of
registers that provide information on missed DMA and/or QDMA events, and instances when event queue
thresholds are exceeded. If any of the bits in these registers is set, it results in the EDMA3CC generating
an error interrupt.


**Figure 11-55. CCERRCLR Register**

31                30               29                    28               27                    26         25             24
RESERVED
R-0h
23                22               21                20                   19                    18         17            16
RESERVED                                                             TCCERR
R-0h                                                                W-0h
15                14               13                    12               11                    10         9                  8
RESERVED
R-0h
7              6               5                      4                 3               2              1              0
RESERVED                                                  QTHRXCD2       QTHRXCD1       QTHRXCD0
R-0h                                                      W-0h           W-0h           W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-42. CCERRCLR Register Field Descriptions**

Bit        Field                     Type             Reset          Description
31-17       RESERVED                  R                0h
16        TCCERR                    W                0h             Transfer completion code error clear.
0h = No effect.
1h = Clears the TCCERR bit in the EDMA3CC error register
(CCERR).
15-3       RESERVED                  R                0h
2         QTHRXCD2                  W                0h             Queue threshold error clear for queue 2.
0h = No effect.
1h = Clears the QTHRXCD2 bit in the EDMA3CC error register
(CCERR) and the WM and THRXCD bits in the queue status register
2 (QSTAT2).
1         QTHRXCD1                  W                0h             Queue threshold error clear for queue 1.
0h = No effect.
1h = Clears the QTHRXCD1 bit in the EDMA3CC error register
(CCERR) and the WM and THRXCD bits in the queue status register
1 (QSTAT1).
0         QTHRXCD0                  W                0h             Queue threshold error clear for queue 0.
0h = No effect.
1h = Clears the QTHRXCD0 bit in the EDMA3CC error register
(CCERR) and the WM and THRXCD bits in the queue status register
0 (QSTAT0).


#### 11.4.1.17 EEVAL Register (offset = 320h) [reset = 0h]

EEVAL is shown in Figure 11-56 and described in Table 11-43.
The EDMA3CC error interrupt is asserted whenever an error bit is set in any of the error registers
(EMR/EMRH, QEMR, and CCERR). For subsequent error bits that get set, the EDMA3CC error interrupt
is reasserted only when transitioning from an all the error bits cleared to at least one error bit is set .
Alternatively, a CPU write of 1 to the EVAL bit in the error evaluation register (EEVAL) results in
reasserting the EDMA3CC error interrupt, if there are any outstanding error bits set due to subsequent
error conditions. Writes of 0 have no effect. This register is part of a set of registers that provide
information on missed DMA and/or QDMA events, and instances when event queue thresholds are
exceeded. If any of the bits in these registers is set, it results in the EDMA3CC generating an error
interrupt.


**Figure 11-56. EEVAL Register**

31              30              29                    28               27                    26          25                24
RESERVED
R-0h
23              22              21                    20               19                    18          17                16
RESERVED
R-0h
15              14              13                    12               11                    10           9                 8
RESERVED
R-0h
7             6               5                     4                 3                    2           1                 0
RESERVED                                                       RESERVED            EVAL
R-0h                                                           R-0h              W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-43. EEVAL Register Field Descriptions**

Bit      Field                    Type             Reset          Description
31-2         RESERVED                 R                0h
1        RESERVED                 R                0h
0        EVAL                     W                0h             Error interrupt evaluate.
0h = No effect.
1h = Write 1 to clear interrupts when all error registers have been
cleared. EDMA3CC error interrupt will remain if any errors have not
been cleared in any of the error registers (EMR/EMRH, CCERR,
QEMR)


#### 11.4.1.18 DRAE0 Register (offset = 340h) [reset = 0h]

DRAE0 is shown in Figure 11-57 and described in Table 11-44.
The DMA region access enable register for shadow region 0 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 0 view of the DMA channel
registers. Additionally, the DRAE0 configuration determines completion of which DMA channels will result
in assertion of the shadow region 0 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-57. DRAE0 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8    7   6    5    4   3    2    1    0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-44. DRAE0 Register Field Descriptions**

Bit      Field                       Type          Reset          Description
31-0     En                          R/W           0h             DMA region access enable for bit 31 to 0 in region 0.
0h = Accesses via region 0 address space to bit 31 to 0 in any DMA
channel register are not allowed. Reads return 0 on bit n and writes
do not modify the state of bit 31 to 0. Enabled interrupt bits for bit n
do not contribute to the generation of a transfer completion interrupt
for shadow region 0.
1h = Accesses via region 0 address space to bit 31 to 0 in any DMA
channel register are allowed. Reads return the value from bit n and
writes modify the state of bit 31 to 0. Enabled interrupt bits for bit n
contribute to the generation of a transfer completion interrupt for
shadow region 0.


#### 11.4.1.19 DRAEH0 Register (offset = 344h) [reset = 0h]

DRAEH0 is shown in Figure 11-58 and described in Table 11-45.
The DMA region access enable register for shadow region 0 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 0 view of the DMA channel
registers. Additionally, the DRAE0 configuration determines completion of which DMA channels will result
in assertion of the shadow region 0 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-58. DRAEH0 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8   7    6   5    4    3   2    1   0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-45. DRAEH0 Register Field Descriptions**

Bit    Field                      Type          Reset          Description
31-0       En                         R/W           0h             DMA region access enable for bit 63 to 32 in region 0.
0h = Accesses via region 0 address space to bit 63 to 32 in any
DMA channel register are not allowed. Reads return 0 on bit n and
writes do not modify the state of bit 63 to 32. Enabled interrupt bits
for bit 31 to 0 do not contribute to the generation of a transfer
completion interrupt for shadow region 0.
1h = Accesses via region 0 address space to bit 63 to 32 in any
DMA channel register are allowed. Reads return the value from bit n
and writes modify the state of bit 63 to 32. Enabled interrupt bits for
bit 31 to 0 contribute to the generation of a transfer completion
interrupt for shadow region 0.


#### 11.4.1.20 DRAE1 Register (offset = 348h) [reset = 0h]

DRAE1 is shown in Figure 11-59 and described in Table 11-46.
The DMA region access enable register for shadow region 1 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 1 view of the DMA channel
registers. Additionally, the DRAE1 configuration determines completion of which DMA channels will result
in assertion of the shadow region 1 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-59. DRAE1 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8    7   6    5    4   3    2    1    0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-46. DRAE1 Register Field Descriptions**

Bit      Field                       Type          Reset          Description
31-0     En                          R/W           0h             DMA region access enable for bit 31 to 0 in region 1.
0h = Accesses via region 1 address space to bit 31 to 0 in any DMA
channel register are not allowed. Reads return 0 on bit n and writes
do not modify the state of bit 31 to 0. Enabled interrupt bits for bit n
do not contribute to the generation of a transfer completion interrupt
for shadow region 1.
1h = Accesses via region 1 address space to bit 31 to 0 in any DMA
channel register are allowed. Reads return the value from bit n and
writes modify the state of bit 31 to 0. Enabled interrupt bits for bit n
contribute to the generation of a transfer completion interrupt for
shadow region 1.


#### 11.4.1.21 DRAEH1 Register (offset = 34Ch) [reset = 0h]

DRAEH1 is shown in Figure 11-60 and described in Table 11-47.
The DMA region access enable register for shadow region 1 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 1 view of the DMA channel
registers. Additionally, the DRAE1 configuration determines completion of which DMA channels will result
in assertion of the shadow region 1 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-60. DRAEH1 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8   7    6   5    4    3   2    1   0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-47. DRAEH1 Register Field Descriptions**

Bit    Field                      Type          Reset          Description
31-0       En                         R/W           0h             DMA region access enable for bit 63 to 32 in region 1.
0h = Accesses via region 1 address space to bit 63 to 32 in any
DMA channel register are not allowed. Reads return 0 on bit n and
writes do not modify the state of bit 63 to 32. Enabled interrupt bits
for bit 31 to 0 do not contribute to the generation of a transfer
completion interrupt for shadow region 1.
1h = Accesses via region 1 address space to bit 63 to 32 in any
DMA channel register are allowed. Reads return the value from bit n
and writes modify the state of bit 63 to 32. Enabled interrupt bits for
bit 31 to 0 contribute to the generation of a transfer completion
interrupt for shadow region 1.


#### 11.4.1.22 DRAE2 Register (offset = 350h) [reset = 0h]

DRAE2 is shown in Figure 11-61 and described in Table 11-48.
The DMA region access enable register for shadow region 2 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 2 view of the DMA channel
registers. Additionally, the DRAE2 configuration determines completion of which DMA channels will result
in assertion of the shadow region 2 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-61. DRAE2 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8    7   6    5    4   3    2    1    0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-48. DRAE2 Register Field Descriptions**

Bit      Field                       Type          Reset          Description
31-0     En                          R/W           0h             DMA region access enable for bit 31 to 0 in region 2.
0h = Accesses via region 2 address space to bit 31 to 0 in any DMA
channel register are not allowed. Reads return 0 on bit n and writes
do not modify the state of bit 31 to 0. Enabled interrupt bits for bit n
do not contribute to the generation of a transfer completion interrupt
for shadow region 2.
1h = Accesses via region 2 address space to bit 31 to 0 in any DMA
channel register are allowed. Reads return the value from bit n and
writes modify the state of bit 31 to 0. Enabled interrupt bits for bit n
contribute to the generation of a transfer completion interrupt for
shadow region 2.


#### 11.4.1.23 DRAEH2 Register (offset = 354h) [reset = 0h]

DRAEH2 is shown in Figure 11-62 and described in Table 11-49.
The DMA region access enable register for shadow region 2 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 2 view of the DMA channel
registers. Additionally, the DRAE2 configuration determines completion of which DMA channels will result
in assertion of the shadow region 2 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-62. DRAEH2 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8   7    6   5    4    3   2    1   0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-49. DRAEH2 Register Field Descriptions**

Bit    Field                      Type          Reset          Description
31-0       En                         R/W           0h             DMA region access enable for bit 63 to 32 in region 2.
0h = Accesses via region 2 address space to bit 63 to 32 in any
DMA channel register are not allowed. Reads return 0 on bit n and
writes do not modify the state of bit 63 to 32. Enabled interrupt bits
for bit 31 to 0 do not contribute to the generation of a transfer
completion interrupt for shadow region 2.
1h = Accesses via region 2 address space to bit 63 to 32 in any
DMA channel register are allowed. Reads return the value from bit n
and writes modify the state of bit 63 to 32. Enabled interrupt bits for
bit 31 to 0 contribute to the generation of a transfer completion
interrupt for shadow region 2.


#### 11.4.1.24 DRAE3 Register (offset = 358h) [reset = 0h]

DRAE3 is shown in Figure 11-63 and described in Table 11-50.
The DMA region access enable register for shadow region 3 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 3 view of the DMA channel
registers. Additionally, the DRAE3 configuration determines completion of which DMA channels will result
in assertion of the shadow region 3 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-63. DRAE3 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8    7   6    5    4   3    2    1    0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-50. DRAE3 Register Field Descriptions**

Bit      Field                       Type          Reset          Description
31-0     En                          R/W           0h             DMA region access enable for bit 31 to 0 in region 3.
0h = Accesses via region 3 address space to bit 31 to 0 in any DMA
channel register are not allowed. Reads return 0 on bit n and writes
do not modify the state of bit 31 to 0. Enabled interrupt bits for bit n
do not contribute to the generation of a transfer completion interrupt
for shadow region 3.
1h = Accesses via region 3 address space to bit 31 to 0 in any DMA
channel register are allowed. Reads return the value from bit n and
writes modify the state of bit 31 to 0. Enabled interrupt bits for bit n
contribute to the generation of a transfer completion interrupt for
shadow region 3.


#### 11.4.1.25 DRAEH3 Register (offset = 35Ch) [reset = 0h]

DRAEH3 is shown in Figure 11-64 and described in Table 11-51.
The DMA region access enable register for shadow region 3 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 3 view of the DMA channel
registers. Additionally, the DRAE3 configuration determines completion of which DMA channels will result
in assertion of the shadow region 3 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-64. DRAEH3 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8   7    6   5    4    3   2    1   0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-51. DRAEH3 Register Field Descriptions**

Bit    Field                      Type          Reset          Description
31-0       En                         R/W           0h             DMA region access enable for bit 63 to 32 in region 3.
0h = Accesses via region 3 address space to bit 63 to 32 in any
DMA channel register are not allowed. Reads return 0 on bit n and
writes do not modify the state of bit 63 to 32. Enabled interrupt bits
for bit 31 to 0 do not contribute to the generation of a transfer
completion interrupt for shadow region 3.
1h = Accesses via region 3 address space to bit 63 to 32 in any
DMA channel register are allowed. Reads return the value from bit n
and writes modify the state of bit 63 to 32. Enabled interrupt bits for
bit 31 to 0 contribute to the generation of a transfer completion
interrupt for shadow region 3.


#### 11.4.1.26 DRAE4 Register (offset = 360h) [reset = 0h]

DRAE4 is shown in Figure 11-65 and described in Table 11-52.
The DMA region access enable register for shadow region 4 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 4 view of the DMA channel
registers. Additionally, the DRAE4 configuration determines completion of which DMA channels will result
in assertion of the shadow region 4 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-65. DRAE4 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8    7   6    5    4   3    2    1    0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-52. DRAE4 Register Field Descriptions**

Bit      Field                       Type          Reset          Description
31-0     En                          R/W           0h             DMA region access enable for bit 31 to 0 in region 4.
0h = Accesses via region 4 address space to bit 31 to 0 in any DMA
channel register are not allowed. Reads return 0 on bit n and writes
do not modify the state of bit 31 to 0. Enabled interrupt bits for bit n
do not contribute to the generation of a transfer completion interrupt
for shadow region 4.
1h = Accesses via region 4 address space to bit 31 to 0 in any DMA
channel register are allowed. Reads return the value from bit n and
writes modify the state of bit 31 to 0. Enabled interrupt bits for bit n
contribute to the generation of a transfer completion interrupt for
shadow region 4.


#### 11.4.1.27 DRAEH4 Register (offset = 364h) [reset = 0h]

DRAEH4 is shown in Figure 11-66 and described in Table 11-53.
The DMA region access enable register for shadow region 4 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 4 view of the DMA channel
registers. Additionally, the DRAE4 configuration determines completion of which DMA channels will result
in assertion of the shadow region 4 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-66. DRAEH4 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8   7    6   5    4    3   2    1   0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-53. DRAEH4 Register Field Descriptions**

Bit    Field                      Type          Reset          Description
31-0       En                         R/W           0h             DMA region access enable for bit 63 to 32 in region 4.
0h = Accesses via region 4 address space to bit 63 to 32 in any
DMA channel register are not allowed. Reads return 0 on bit n and
writes do not modify the state of bit 63 to 32. Enabled interrupt bits
for bit 31 to 0 do not contribute to the generation of a transfer
completion interrupt for shadow region 4.
1h = Accesses via region 4 address space to bit 63 to 32 in any
DMA channel register are allowed. Reads return the value from bit n
and writes modify the state of bit 63 to 32. Enabled interrupt bits for
bit 31 to 0 contribute to the generation of a transfer completion
interrupt for shadow region 4.


#### 11.4.1.28 DRAE5 Register (offset = 368h) [reset = 0h]

DRAE5 is shown in Figure 11-67 and described in Table 11-54.
The DMA region access enable register for shadow region 5 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 5 view of the DMA channel
registers. Additionally, the DRAE5 configuration determines completion of which DMA channels will result
in assertion of the shadow region 5 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-67. DRAE5 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8    7   6    5    4   3    2    1    0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-54. DRAE5 Register Field Descriptions**

Bit      Field                       Type          Reset          Description
31-0     En                          R/W           0h             DMA region access enable for bit 31 to 0 in region 5.
0h = Accesses via region 5 address space to bit 31 to 0 in any DMA
channel register are not allowed. Reads return 0 on bit n and writes
do not modify the state of bit 31 to 0. Enabled interrupt bits for bit n
do not contribute to the generation of a transfer completion interrupt
for shadow region 5.
1h = Accesses via region 5 address space to bit 31 to 0 in any DMA
channel register are allowed. Reads return the value from bit n and
writes modify the state of bit 31 to 0. Enabled interrupt bits for bit n
contribute to the generation of a transfer completion interrupt for
shadow region 5.


#### 11.4.1.29 DRAEH5 Register (offset = 36Ch) [reset = 0h]

DRAEH5 is shown in Figure 11-68 and described in Table 11-55.
The DMA region access enable register for shadow region 5 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 5 view of the DMA channel
registers. Additionally, the DRAE5 configuration determines completion of which DMA channels will result
in assertion of the shadow region 5 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-68. DRAEH5 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8   7    6   5    4    3   2    1   0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-55. DRAEH5 Register Field Descriptions**

Bit    Field                      Type          Reset          Description
31-0       En                         R/W           0h             DMA region access enable for bit 63 to 32 in region 5.
0h = Accesses via region 5 address space to bit 63 to 32 in any
DMA channel register are not allowed. Reads return 0 on bit n and
writes do not modify the state of bit 63 to 32. Enabled interrupt bits
for bit 31 to 0 do not contribute to the generation of a transfer
completion interrupt for shadow region 5.
1h = Accesses via region 5 address space to bit 63 to 32 in any
DMA channel register are allowed. Reads return the value from bit n
and writes modify the state of bit 63 to 32. Enabled interrupt bits for
bit 31 to 0 contribute to the generation of a transfer completion
interrupt for shadow region 5.


#### 11.4.1.30 DRAE6 Register (offset = 370h) [reset = 0h]

DRAE6 is shown in Figure 11-69 and described in Table 11-56.
The DMA region access enable register for shadow region 6 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 6 view of the DMA channel
registers. Additionally, the DRAE6 configuration determines completion of which DMA channels will result
in assertion of the shadow region 6 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-69. DRAE6 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8    7   6    5    4   3    2    1    0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-56. DRAE6 Register Field Descriptions**

Bit      Field                       Type          Reset          Description
31-0     En                          R/W           0h             DMA region access enable for bit 31 to 0 in region 6.
0h = Accesses via region 6 address space to bit 31 to 0 in any DMA
channel register are not allowed. Reads return 0 on bit n and writes
do not modify the state of bit 31 to 0. Enabled interrupt bits for bit n
do not contribute to the generation of a transfer completion interrupt
for shadow region 6.
1h = Accesses via region 6 address space to bit 31 to 0 in any DMA
channel register are allowed. Reads return the value from bit n and
writes modify the state of bit 31 to 0. Enabled interrupt bits for bit n
contribute to the generation of a transfer completion interrupt for
shadow region 6.


#### 11.4.1.31 DRAEH6 Register (offset = 374h) [reset = 0h]

DRAEH6 is shown in Figure 11-70 and described in Table 11-57.
The DMA region access enable register for shadow region 6 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 6 view of the DMA channel
registers. Additionally, the DRAE6 configuration determines completion of which DMA channels will result
in assertion of the shadow region 6 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-70. DRAEH6 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8   7    6   5    4    3   2    1   0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-57. DRAEH6 Register Field Descriptions**

Bit    Field                      Type          Reset          Description
31-0       En                         R/W           0h             DMA region access enable for bit 63 to 32 in region 6.
0h = Accesses via region 6 address space to bit 63 to 32 in any
DMA channel register are not allowed. Reads return 0 on bit n and
writes do not modify the state of bit 63 to 32. Enabled interrupt bits
for bit 31 to 0 do not contribute to the generation of a transfer
completion interrupt for shadow region 6.
1h = Accesses via region 6 address space to bit 63 to 32 in any
DMA channel register are allowed. Reads return the value from bit n
and writes modify the state of bit 63 to 32. Enabled interrupt bits for
bit 31 to 0 contribute to the generation of a transfer completion
interrupt for shadow region 6.


#### 11.4.1.32 DRAE7 Register (offset = 378h) [reset = 0h]

DRAE7 is shown in Figure 11-71 and described in Table 11-58.
The DMA region access enable register for shadow region 7 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 7 view of the DMA channel
registers. Additionally, the DRAE7 configuration determines completion of which DMA channels will result
in assertion of the shadow region 7 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-71. DRAE7 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8    7   6    5    4   3    2    1    0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-58. DRAE7 Register Field Descriptions**

Bit      Field                       Type          Reset          Description
31-0     En                          R/W           0h             DMA region access enable for bit 31 to 0 in region 7.
0h = Accesses via region 7 address space to bit 31 to 0 in any DMA
channel register are not allowed. Reads return 0 on bit n and writes
do not modify the state of bit 31 to 0. Enabled interrupt bits for bit n
do not contribute to the generation of a transfer completion interrupt
for shadow region 7.
1h = Accesses via region 7 address space to bit 31 to 0 in any DMA
channel register are allowed. Reads return the value from bit n and
writes modify the state of bit 31 to 0. Enabled interrupt bits for bit n
contribute to the generation of a transfer completion interrupt for
shadow region 7.


#### 11.4.1.33 DRAEH7 Register (offset = 37Ch) [reset = 0h]

DRAEH7 is shown in Figure 11-72 and described in Table 11-59.
The DMA region access enable register for shadow region 7 is programmed to allow or disallow read/write
accesses on a bit-by-bit bases for all DMA registers in the shadow region 7 view of the DMA channel
registers. Additionally, the DRAE7 configuration determines completion of which DMA channels will result
in assertion of the shadow region 7 DMA completion interrupt. The DRAE registers are part of the group of
the region access enable registers, which includes DRAEm and QRAEm. Where m is the number of
shadow regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-72. DRAEH7 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8   7    6   5    4    3   2    1   0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-59. DRAEH7 Register Field Descriptions**

Bit    Field                      Type          Reset          Description
31-0       En                         R/W           0h             DMA region access enable for bit 63 to 32 in region 7.
0h = Accesses via region 7 address space to bit 63 to 32 in any
DMA channel register are not allowed. Reads return 0 on bit n and
writes do not modify the state of bit 63 to 32. Enabled interrupt bits
for bit 31 to 0 do not contribute to the generation of a transfer
completion interrupt for shadow region 7.
1h = Accesses via region 7 address space to bit 63 to 32 in any
DMA channel register are allowed. Reads return the value from bit n
and writes modify the state of bit 63 to 32. Enabled interrupt bits for
bit 31 to 0 contribute to the generation of a transfer completion
interrupt for shadow region 7.


#### 11.4.1.34 QRAE_0 to QRAE_7 Register (offset = 380h to 39Ch) [reset = 0h]

QRAE_0 to QRAE_7 is shown in Figure 11-73 and described in Table 11-60.
The QDMA region access enable register for shadow region m (QRAEm) is programmed to allow or
disallow read/write accesses on a bit-by-bit bases for all QDMA registers in the shadow region m view of
the QDMA registers. This includes all 4-bit QDMA registers. The QRAE register is part of the group of the
region access enable registers, which includes DRAEm and QRAEm. Where m is the number of shadow
regions in the EDMA3CC memory map for a device. You can configure these registers to assign
ownership of DMA/QDMA channels to a particular shadow region.


**Figure 11-73. QRAE_0 to QRAE_7 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4    3 2      1    0
RESERVED                                                                                    En
R-0h                                                                                   R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-60. QRAE_0 to QRAE_7 Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-8     RESERVED                    R               0h
7-0      En                          R/W             0h             QDMA region access enable for bit n/QDMA channel n in region m.
0h = Accesses via region m address space to bit n in any QDMA
channel register are not allowed. Reads return 0 on bit n and writes
do not modify the state of bit n.
1h = Accesses via region m address space to bit n in any QDMA
channel register are allowed. Reads return the value from bit n and
writes modify the state of bit n.


#### 11.4.1.35 Q0E0 Register (offset = 400h) [reset = 0h]

Q0E0 is shown in Figure 11-74 and described in Table 11-61.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E0 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-74. Q0E0 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-61. Q0E0 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 0 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 0 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.36 Q0E1 Register (offset = 404h) [reset = 0h]

Q0E1 is shown in Figure 11-75 and described in Table 11-62.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E1 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-75. Q0E1 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-62. Q0E1 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 1 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 1 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.37 Q0E2 Register (offset = 408h) [reset = 0h]

Q0E2 is shown in Figure 11-76 and described in Table 11-63.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E2 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-76. Q0E2 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-63. Q0E2 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 2 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 2 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.38 Q0E3 Register (offset = 40Ch) [reset = 0h]

Q0E3 is shown in Figure 11-77 and described in Table 11-64.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E3 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-77. Q0E3 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-64. Q0E3 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 3 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 3 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.39 Q0E4 Register (offset = 410h) [reset = 0h]

Q0E4 is shown in Figure 11-78 and described in Table 11-65.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E4 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-78. Q0E4 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-65. Q0E4 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 4 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 4 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.40 Q0E5 Register (offset = 414h) [reset = 0h]

Q0E5 is shown in Figure 11-79 and described in Table 11-66.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E5 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-79. Q0E5 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-66. Q0E5 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 5 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 5 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.41 Q0E6 Register (offset = 418h) [reset = 0h]

Q0E6 is shown in Figure 11-80 and described in Table 11-67.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E6 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-80. Q0E6 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-67. Q0E6 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 6 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 6 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.42 Q0E7 Register (offset = 41Ch) [reset = 0h]

Q0E7 is shown in Figure 11-81 and described in Table 11-68.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E7 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-81. Q0E7 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-68. Q0E7 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 7 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 7 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.43 Q0E8 Register (offset = 420h) [reset = 0h]

Q0E8 is shown in Figure 11-82 and described in Table 11-69.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E8 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-82. Q0E8 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-69. Q0E8 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 8 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 8 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.44 Q0E9 Register (offset = 424h) [reset = 0h]

Q0E9 is shown in Figure 11-83 and described in Table 11-70.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E9 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-83. Q0E9 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-70. Q0E9 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 9 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 9 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.45 Q0E10 Register (offset = 428h) [reset = 0h]

Q0E10 is shown in Figure 11-84 and described in Table 11-71.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E10 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-84. Q0E10 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-71. Q0E10 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 10 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 10 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.46 Q0E11 Register (offset = 42Ch) [reset = 0h]

Q0E11 is shown in Figure 11-85 and described in Table 11-72.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E11 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-85. Q0E11 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-72. Q0E11 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 11 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 11 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.47 Q0E12 Register (offset = 430h) [reset = 0h]

Q0E12 is shown in Figure 11-86 and described in Table 11-73.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E12 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-86. Q0E12 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-73. Q0E12 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 12 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 12 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.48 Q0E13 Register (offset = 434h) [reset = 0h]

Q0E13 is shown in Figure 11-87 and described in Table 11-74.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E13 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-87. Q0E13 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-74. Q0E13 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 13 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 13 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.49 Q0E14 Register (offset = 438h) [reset = 0h]

Q0E14 is shown in Figure 11-88 and described in Table 11-75.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E14 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-88. Q0E14 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-75. Q0E14 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 14 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 14 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.50 Q0E15 Register (offset = 43Ch) [reset = 0h]

Q0E15 is shown in Figure 11-89 and described in Table 11-76.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q0E15 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-89. Q0E15 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-76. Q0E15 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 15 in queue 0.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 15 in queue 0.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.51 Q1E0 Register (offset = 440h) [reset = 0h]

Q1E0 is shown in Figure 11-90 and described in Table 11-77.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E0 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-90. Q1E0 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-77. Q1E0 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 0 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 0 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.52 Q1E1 Register (offset = 444h) [reset = 0h]

Q1E1 is shown in Figure 11-91 and described in Table 11-78.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E1 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-91. Q1E1 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-78. Q1E1 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 1 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 1 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.53 Q1E2 Register (offset = 448h) [reset = 0h]

Q1E2 is shown in Figure 11-92 and described in Table 11-79.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E2 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-92. Q1E2 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-79. Q1E2 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 2 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 2 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.54 Q1E3 Register (offset = 44Ch) [reset = 0h]

Q1E3 is shown in Figure 11-93 and described in Table 11-80.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E3 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-93. Q1E3 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-80. Q1E3 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 3 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 3 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.55 Q1E4 Register (offset = 450h) [reset = 0h]

Q1E4 is shown in Figure 11-94 and described in Table 11-81.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E4 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-94. Q1E4 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-81. Q1E4 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 4 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 4 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.56 Q1E5 Register (offset = 454h) [reset = 0h]

Q1E5 is shown in Figure 11-95 and described in Table 11-82.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E5 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-95. Q1E5 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-82. Q1E5 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 5 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 5 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.57 Q1E6 Register (offset = 458h) [reset = 0h]

Q1E6 is shown in Figure 11-96 and described in Table 11-83.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E6 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-96. Q1E6 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-83. Q1E6 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 6 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 6 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.58 Q1E7 Register (offset = 45Ch) [reset = 0h]

Q1E7 is shown in Figure 11-97 and described in Table 11-84.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E7 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-97. Q1E7 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-84. Q1E7 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 7 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 7 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.59 Q1E8 Register (offset = 460h) [reset = 0h]

Q1E8 is shown in Figure 11-98 and described in Table 11-85.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E8 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-98. Q1E8 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-85. Q1E8 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 8 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 8 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.60 Q1E9 Register (offset = 464h) [reset = 0h]

Q1E9 is shown in Figure 11-99 and described in Table 11-86.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E9 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-99. Q1E9 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-86. Q1E9 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 9 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 9 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.61 Q1E10 Register (offset = 468h) [reset = 0h]

Q1E10 is shown in Figure 11-100 and described in Table 11-87.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E10 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-100. Q1E10 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-87. Q1E10 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 10 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 10 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.62 Q1E11 Register (offset = 46Ch) [reset = 0h]

Q1E11 is shown in Figure 11-101 and described in Table 11-88.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E11 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-101. Q1E11 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-88. Q1E11 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 11 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 11 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.63 Q1E12 Register (offset = 470h) [reset = 0h]

Q1E12 is shown in Figure 11-102 and described in Table 11-89.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E12 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-102. Q1E12 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-89. Q1E12 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 12 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 12 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.64 Q1E13 Register (offset = 474h) [reset = 0h]

Q1E13 is shown in Figure 11-103 and described in Table 11-90.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E13 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-103. Q1E13 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-90. Q1E13 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 13 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 13 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.65 Q1E14 Register (offset = 478h) [reset = 0h]

Q1E14 is shown in Figure 11-104 and described in Table 11-91.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E14 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-104. Q1E14 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-91. Q1E14 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 14 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 14 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.66 Q1E15 Register (offset = 47Ch) [reset = 0h]

Q1E15 is shown in Figure 11-105 and described in Table 11-92.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q1E15 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-105. Q1E15 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-92. Q1E15 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 15 in queue 1.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 15 in queue 1.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.67 Q2E0 Register (offset = 480h) [reset = 0h]

Q2E0 is shown in Figure 11-106 and described in Table 11-93.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E0 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-106. Q2E0 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-93. Q2E0 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 0 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 0 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.68 Q2E1 Register (offset = 484h) [reset = 0h]

Q2E1 is shown in Figure 11-107 and described in Table 11-94.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E1 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-107. Q2E1 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-94. Q2E1 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 1 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 1 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.69 Q2E2 Register (offset = 488h) [reset = 0h]

Q2E2 is shown in Figure 11-108 and described in Table 11-95.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E2 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-108. Q2E2 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-95. Q2E2 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 2 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 2 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.70 Q2E3 Register (offset = 48Ch) [reset = 0h]

Q2E3 is shown in Figure 11-109 and described in Table 11-96.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E3 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-109. Q2E3 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-96. Q2E3 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 3 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 3 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.71 Q2E4 Register (offset = 490h) [reset = 0h]

Q2E4 is shown in Figure 11-110 and described in Table 11-97.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E4 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-110. Q2E4 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-97. Q2E4 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 4 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 4 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.72 Q2E5 Register (offset = 494h) [reset = 0h]

Q2E5 is shown in Figure 11-111 and described in Table 11-98.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E5 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-111. Q2E5 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-98. Q2E5 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 5 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 5 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.73 Q2E6 Register (offset = 498h) [reset = 0h]

Q2E6 is shown in Figure 11-112 and described in Table 11-99.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E6 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-112. Q2E6 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-99. Q2E6 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 6 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 6 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.74 Q2E7 Register (offset = 49Ch) [reset = 0h]

Q2E7 is shown in Figure 11-113 and described in Table 11-100.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E7 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-113. Q2E7 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-100. Q2E7 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 7 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 7 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.75 Q2E8 Register (offset = 4A0h) [reset = 0h]

Q2E8 is shown in Figure 11-114 and described in Table 11-101.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E8 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-114. Q2E8 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-101. Q2E8 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 8 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 8 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.76 Q2E9 Register (offset = 4A4h) [reset = 0h]

Q2E9 is shown in Figure 11-115 and described in Table 11-102.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E9 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-115. Q2E9 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-102. Q2E9 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 9 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 9 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.77 Q2E10 Register (offset = 4A8h) [reset = 0h]

Q2E10 is shown in Figure 11-116 and described in Table 11-103.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E10 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-116. Q2E10 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-103. Q2E10 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 10 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 10 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.78 Q2E11 Register (offset = 4ACh) [reset = 0h]

Q2E11 is shown in Figure 11-117 and described in Table 11-104.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E11 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-117. Q2E11 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-104. Q2E11 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 11 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 11 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.79 Q2E12 Register (offset = 4B0h) [reset = 0h]

Q2E12 is shown in Figure 11-118 and described in Table 11-105.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E12 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-118. Q2E12 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-105. Q2E12 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 12 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 12 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.80 Q2E13 Register (offset = 4B4h) [reset = 0h]

Q2E13 is shown in Figure 11-119 and described in Table 11-106.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E13 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-119. Q2E13 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-106. Q2E13 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 13 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 13 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.81 Q2E14 Register (offset = 4B8h) [reset = 0h]

Q2E14 is shown in Figure 11-120 and described in Table 11-107.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E14 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-120. Q2E14 Register**

31         30           29   28     27         26        25       24      23         22        21      20       19       18       17          16
RESERVED
R-0h
15         14           13   12      11        10        9         8        7           6       5      4        3           2      1          0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-107. Q2E14 Register Field Descriptions**

Bit        Field                     Type            Reset          Description
31-8           RESERVED                  R               0h
7-6        ETYPE                     R               0h             Event entry 14 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0        ENUM                      R               0h             Event entry 14 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.82 Q2E15 Register (offset = 4BCh) [reset = 0h]

Q2E15 is shown in Figure 11-121 and described in Table 11-108.
The event queue entry registers (QxEy) exist for all 16 queue entries (the maximum allowed queue
entries) for all event queues in the EDMA3CC. The event queue entry registers range from Q0E0 to
Q0E15, Q1E0 to Q1E15, and Q2E0 to Q2E15. Each register details the event number (ENUM) and the
event type (ETYPE). For example, if the value in Q1E4 is read as 000 004Fh, this means the 4th entry in
queue 1 is a manually-triggered event on DMA channel 15. The Q2E15 register provides visibility into the
event queues and a TR life cycle. These are useful for system debug as they provide in-depth visibility for
the events queued up in the event queue and also provide information on what parts of the EDMA3CC
logic are active once the event has been received by the EDMA3CC.


**Figure 11-121. Q2E15 Register**

31       30           29   28     27         26        25       24      23         22       21       20       19       18         17        16
RESERVED
R-0h
15       14           13   12      11        10        9         8        7           6      5       4        3           2       1         0
RESERVED                                           ETYPE                               ENUM
R-0h                                             R-0h                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-108. Q2E15 Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-8        RESERVED                  R               0h
7-6         ETYPE                     R               0h             Event entry 15 in queue 2.
Specifies the specific event type for the given entry in the event
queue.
0h = Event triggered via ER.
1h = Manual triggered via ESR.
2h = Chain triggered via CER
3h = Autotriggered via QER
5-0         ENUM                      R               0h             Event entry 15 in queue 2.
Event number: QDMA channel number (0 to 3).
DMA channel/event number (0 to 63).


#### 11.4.1.83 QSTAT_0 to QSTAT_2 Register (offset = 600h to 608h) [reset = Fh]

QSTAT_0 to QSTAT_2 is shown in Figure 11-122 and described in Table 11-109.
The queue status register (QSTAT) provides visibility into the event queues and a TR life cycle. These are
useful for system debug as they provide in-depth visibility for the events queued up in the event queue
and also provide information on what parts of the EDMA3CC logic are active once the event has been
received by the EDMA3CC.


**Figure 11-122. QSTAT_0 to QSTAT_2 Register**

31                 30              29                28                   27                    26             25                24
RESERVED                                                                    THRXCD
R-0h                                                                        R-0h
23                 22              21                    20               19                 18                17                  16
RESERVED                                                                    WM
R-0h                                                                      R-0h
15                 14              13                    12               11               10                  9                   8
RESERVED                                                                 NUMVAL
R-0h                                                                    R-0h
7                  6               5                     4                 3                    2              1                   0
RESERVED                                                                     STRTPTR
R-0h                                                                         R-Fh
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-109. QSTAT_0 to QSTAT_2 Register Field Descriptions**

Bit       Field                      Type             Reset          Description
31-25         RESERVED                   R                0h
24        THRXCD                     R                0h             Threshold exceeded.
THRXCD is cleared by writing a 1 to the corresponding QTHRXCDn
bit in the EDMA3CC error clear register (CCERRCLR).
0h = Threshold specified by the Qn bit in the queue watermark
threshold A register (QWMTHRA) has not been exceeded.
1h = Threshold specified by the Qn bit in the queue watermark
threshold A register (QWMTHRA) has been exceeded.
23-21         RESERVED                   R                0h
20-16         WM                         R                0h             Watermark for maximum queue usage.
Watermark tracks the most entries that have been in queue n since
reset or since the last time that the watermark (WM) bit was cleared.
WM is cleared by writing a 1 to the corresponding QTHRXCDn bit in
the EDMA3CC error clear register (CCERRCLR).
0h = Legal values are 0 (empty) to 10h (full).
1h = Reserved, from 11h to 1Fh. Always write 0 to this bit; writes of
1 to this bit are not supported and attempts to do so may result in
undefined behavior.
15-13         RESERVED                   R                0h
12-8          NUMVAL                     R                0h             Number of valid entries in queue n.
The total number of entries residing in the queue manager FIFO at a
given instant.
Always enabled.
0h = Legal values are 0 (empty) to 10h (full).
1h = Reserved, from 11h to 1Fh. Always write 0 to this bit; writes of
1 to this bit are not supported and attempts to do so may result in
undefined behavior.
7-4       RESERVED                   R                0h
3-0       STRTPTR                    R                Fh             Start pointer.
The offset to the head entry of queue n, in units of entries.
Always enabled.
Legal values are 0 (0th entry) to Fh (15th entry).


#### 11.4.1.84 QWMTHRA Register (offset = 620h) [reset = A0A0Ah]

QWMTHRA is shown in Figure 11-123 and described in Table 11-110.
The queue watermark threshold A register (QWMTHRA) provides visibility into the event queues and a TR
life cycle. These are useful for system debug as they provide in-depth visibility for the events queued up in
the event queue and also provide information on what parts of the EDMA3CC logic are active once the
event has been received by the EDMA3CC.


**Figure 11-123. QWMTHRA Register**

31       30           29   28     27      26    25             24       23        22       21      20       19       18        17       16
RESERVED                                                                      Q2
R-0h                                                                      R/W-Ah
15      14    13           12     11      10          9         8        7       6    5             4        3        2        1        0
RESERVED                             Q1                                 RESERVED                                Q0
R-0h                             R/W-Ah                                 R-0h                                R/W-Ah
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-110. QWMTHRA Register Field Descriptions**

Bit         Field                    Type            Reset          Description
31-21        RESERVED                 R               0h
20-16        Q2                       R/W             Ah             Queue threshold for queue 2 value.
The QTHRXCD2 bit in the EDMA3CC error register (CCERR) and
the THRXCD bit in the queue status register 2 (QSTAT2) are set
when the number of events in queue 2 at an instant in time (visible
via the NUMVAL bit in QSTAT2) equals or exceeds the value
specified by Q2.
0h = From 0h to 10h, The default is 16 (maximum allowed).
11h = Disables the threshold errors.
12h = From 12h to 1Fh, Reserved. Always write 0 to this bit; writes
of 1 to this bit are not supported and attempts to do so may result in
undefined behavior.
15-13        RESERVED                 R               0h
12-8        Q1                       R/W             Ah             Queue threshold for queue 1 value.
The QTHRXCD1 bit in the EDMA3CC error register (CCERR) and
the THRXCD bit in the queue status register 1 (QSTAT1) are set
when the number of events in queue 1 at an instant in time (visible
via the NUMVAL bit in QSTAT1) equals or exceeds the value
specified by Q1.
0h = From 0h to 10h, The default is 16 (maximum allowed).
11h = Disables the threshold errors.
12h = From 12h to 1Fh, Reserved. Always write 0 to this bit; writes
of 1 to this bit are not supported and attempts to do so may result in
undefined behavior.
7-5         RESERVED                 R               0h
4-0         Q0                       R/W             Ah             Queue threshold for queue 0 value.
The QTHRXCD0 bit in the EDMA3CC error register (CCERR) and
the THRXCD bit in the queue status register 0 (QSTAT0) are set
when the number of events in queue 0 at an instant in time (visible
via the NUMVAL bit in QSTAT0) equals or exceeds the value
specified by Q0.
0h = From 0h to 10h, The default is 16 (maximum allowed).
11h = Disables the threshold errors.
12h = From 12h to 1Fh, Reserved. Always write 0 to this bit; writes
of 1 to this bit are not supported and attempts to do so may result in
undefined behavior.


#### 11.4.1.85 CCSTAT Register (offset = 640h) [reset = 0h]

CCSTAT is shown in Figure 11-124 and described in Table 11-111.
The EDMA3CC status register (CCSTAT) has a number of status bits that reflect which parts of the
EDMA3CC logic is active at any given instant of time. CCSTAT provides visibility into the event queues
and a TR life cycle. These are useful for system debug as they provide in-depth visibility for the events
queued up in the event queue and also provide information on what parts of the EDMA3CC logic are
active once the event has been received by the EDMA3CC.


**Figure 11-124. CCSTAT Register**

31                 30           29                    28               27                    26          25               24
RESERVED
R-0h
23                 22          21                     20               19               18             17                16
RESERVED                                                 QUEACTV2       QUEACTV1          QUEACTV0
R-0h                                                     R-0h           R-0h              R-0h
15                 14           13                    12               11                    10           9                8
RESERVED                                                               COMPACTV
R-0h                                                                   R-0h
7                   6            5                  4                 3                  2             1                  0
RESERVED                          ACTV            RESERVED            TRACTV        QEVTACTV           EVTACTV
R-0h                            R-0h              R-0h               R-0h           R-0h               R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-111. CCSTAT Register Field Descriptions**

Bit       Field                   Type             Reset          Description
31-19         RESERVED                R                0h
18        QUEACTV2                R                0h             Queue 2 active.
0h = No events are queued in queue 2.
1h = At least one TR is queued in queue 2.
17        QUEACTV1                R                0h             Queue 1 active.
0h = No events are queued in queue 1.
1h = At least one TR is queued in queue 1.
16        QUEACTV0                R                0h             Queue 0 active.
0h = No events are queued in queue 0.
1h = At least one TR is queued in queue 0.
15-14         RESERVED                R                0h
13-8          COMPACTV                R                0h             Completion request active.
The COMPACTV field reflects the count for the number of
completion requests submitted to the transfer controllers.
This count increments every time a TR is submitted and is
programmed to report completion (the TCINTEN or TCCCHEN bits
in OPT in the parameter entry associated with the TR are set).
The counter decrements for every valid TCC received back from the
transfer controllers.
If at any time the count reaches a value of 63, the EDMA3CC will not
service any new TRs until the count is less then 63 (or return a
transfer completion code from a transfer controller, which would
decrement the count).
0h = No completion requests outstanding.
1h = Total of 1 completion request to 63 completion requests are
outstanding, from 1h to 3Fh.
7-5       RESERVED                R                0h


**Table 11-111. CCSTAT Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
4       ACTV                    R               0h             Channel controller active.
Channel controller active is a logical-OR of each of the *ACTV bits.
The ACTV bit remains high through the life of a TR.
0h = Channel is idle..
1h = Channel is busy.
3       RESERVED                R               0h
2       TRACTV                  R               0h             Transfer request active.
0h = Transfer request processing/submission logic is inactive.
1h = Transfer request processing/submission logic is active.
1       QEVTACTV                R               0h             QDMA event active.
0h = No enabled QDMA events are active within the EDMA3CC.
1h = At least one enabled QDMA event (QER) is active within the
EDMA3CC.
0       EVTACTV                 R               0h             DMA event active.
0h = No enabled DMA events are active within the EDMA3CC.
1h = At least one enabled DMA event (ER and EER, ESR, CER) is
active within the EDMA3CC.


#### 11.4.1.86 MPFAR Register (offset = 800h) [reset = 0h]

MPFAR is shown in Figure 11-125 and described in Table 11-112.
A CPU write of 1 to the MPFCLR bit in the memory protection fault command register (MPFCR) causes
any error conditions stored in MPFAR to be cleared.


**Figure 11-125. MPFAR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5    4   3    2   1    0
FADDR
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-112. MPFAR Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       FADDR                      R               0h             Fault address.
This 32 bit read-only status register contains the fault address when
a memory protection violation is detected.
This register can only be cleared via the memory protection fault
command register (MPFCR).
Value 0 to FFFF FFFFh.


#### 11.4.1.87 MPFSR Register (offset = 804h) [reset = 0h]

MPFSR is shown in Figure 11-126 and described in Table 11-113.
A CPU write of 1 to the MPFCLR bit in the memory protection fault command register (MPFCR) causes
any error conditions stored in MPFSR to be cleared.


**Figure 11-126. MPFSR Register**

31         30           29   28   27          26       25       24      23         22       21       20     19       18       17       16
RESERVED
R-0h
15      14    13             12   11          10       9         8       7    6             5        4      3        2        1        0
RESERVED                          FID                           RESERVED              SRE      SWE    SXE      URE      UWE      UXE
R-0h                            R-0h                            R-0h                R-0h     R-0h   R-0h     R-0h     R-0h     R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-113. MPFSR Register Field Descriptions**

Bit           Field                   Type            Reset          Description
31-13          RESERVED                R               0h
12-9          FID                     R               0h             Faulted identification.
FID contains valid information if any of the MP error bits (UXE, UWE,
URE, SXE, SWE, SRE) are nonzero (that is, if an error has been
detected.) The FID field contains the privilege ID for the specific
request/requestor that resulted in an MP error.
Value 0 to Fh.
8-6           RESERVED                R               0h
5          SRE                     R               0h             Supervisor read error.
0h = No error detected.
1h = Supervisor level task attempted to read from a MP page without
SR permissions.
4          SWE                     R               0h             Supervisor write error.
0h = No error detected.
1h = Supervisor level task attempted to write to a MP page without
SW permissions.
3          SXE                     R               0h             Supervisor execute error.
0h = No error detected.
1h = Supervisor level task attempted to execute from a MP page
without SX permissions.
2          URE                     R               0h             User read error.
0h = No error detected.
1h = User level task attempted to read from a MP page without UR
permissions.
1          UWE                     R               0h             User write error.
0h = No error detected.
1h = User level task attempted to write to a MP page without UW
permissions.
0          UXE                     R               0h             User execute error.
0h = No error detected.
1h = User level task attempted to execute from a MP page without
UX permissions.


#### 11.4.1.88 MPFCR Register (offset = 808h) [reset = 0h]

MPFCR is shown in Figure 11-127 and described in Table 11-114.


**Figure 11-127. MPFCR Register**

31              30              29                    28               27                    26         25                24
RESERVED
R-0h
23              22              21                    20               19                    18         17                16
RESERVED
R-0h
15              14              13                    12               11                    10          9                8
RESERVED
R-0h
7             6               5                  4                    3                    2           1              0
RESERVED                                                               MPFCLR
R-0h                                                                  W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-114. MPFCR Register Field Descriptions**

Bit      Field                    Type             Reset          Description
31-1         RESERVED                 R                0h
0        MPFCLR                   W                0h             Fault clear register.
0h = CPU write of 0 has no effect.
1h = CPU write of 1 to the MPFCLR bit causes any error conditions
stored in the memory protection fault address register (MPFAR) and
the memory protection fault status register (MPFSR) to be cleared.


#### 11.4.1.89 MPPAG Register (offset = 80Ch) [reset = 676h]

MPPAG is shown in Figure 11-128 and described in Table 11-115.


**Figure 11-128. MPPAG Register**

31                30               29                    28               27                    26           25                24
RESERVED
R-0h
23                22               21                    20               19                    18           17                16
RESERVED
R-0h
15                14               13                    12               11                    10           9                8
AIDm                                                            EXT           RESERVED
R/W-1h                                                          R/W-1h           R-0h
7              6                5                    4                 3                  2                1                 0
RESERVED                   SR                   SW                SX                 UR               UW                UX
R-1h                   R/W-1h               R/W-1h            R/W-0h             R/W-1h           R/W-1h            R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-115. MPPAG Register Field Descriptions**

Bit        Field                     Type             Reset          Description
31-16       RESERVED                  R                0h
15-10       AIDm                      R/W              1h             Allowed ID 'N'
0h = Requests with Privilege ID == N are not allowed to region M,
regardless of permission settings (UW, UR, SW, SR).
1h = Requests with Privilege ID == N are permitted, if access type is
allowed as defined by permission settings (UW, UR, SW, SR).
9         EXT                       R/W              1h             External Allowed ID.
0h = Requests with Privilege ID >= 6 are not allowed to region M,
regardless of permission settings (UW, UR, SW, SR).
1h = Requests with Privilege ID >= 6 are permitted, if access type is
allowed as defined by permission settings (UW, UR, SW, SR).
8         RESERVED                  R                0h
7-6        RESERVED                  R                1h
5         SR                        R/W              1h             Supervisor read permission.
0h = Supervisor read accesses are not allowed from region M.
1h = Supervisor write accesses are allowed from region M
addresses.
4         SW                        R/W              1h             Supervisor write permission.
0h = Supervisor write accesses are not allowed to region M.
1h = Supervisor write accesses are allowed to region N addresses.
3         SX                        R/W              0h             Supervisor execute permission.
0h = Supervisor execute accesses are not allowed from region M.
1h = Supervisor execute accesses are allowed from region M
addresses.
2         UR                        R/W              1h             User read permission.
0h = User read accesses are not allowed from region M.
1h = User read accesses are allowed from region N addresses.
1         UW                        R/W              1h             User write permission.
0h = User write accesses are not allowed to region M.
1h = User write accesses are allowed to region M addresses.
0         UX                        R/W              0h             User execute permission.
0h = User execute accesses are not allowed from region M.
1h = User execute accesses are allowed from region M addresses.


#### 11.4.1.90 MPPA_0 to MPPA_7 Register (offset = 810h to 82Ch) [reset = 676h]

MPPA_0 to MPPA_7 is shown in Figure 11-129 and described in Table 11-116.


**Figure 11-129. MPPA_0 to MPPA_7 Register**

31               30             29                    28               27                    26           25                24
RESERVED
R-0h
23               22             21                    20               19                    18           17                16
RESERVED
R-0h
15               14             13                    12               11                    10           9                8
AIDm                                                            EXT           RESERVED
R/W-1h                                                          R/W-1h           R-0h
7              6              5                    4                 3                  2                1                 0
RESERVED                 SR                   SW                SX                 UR               UW                UX
R-1h                 R/W-1h               R/W-1h            R/W-0h             R/W-1h           R/W-1h            R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-116. MPPA_0 to MPPA_7 Register Field Descriptions**

Bit       Field                   Type             Reset          Description
31-16         RESERVED                R                0h
15-10         AIDm                    R/W              1h             Allowed ID 'N'
0h = Requests with Privilege ID == N are not allowed to region M,
regardless of permission settings (UW, UR, SW, SR).
1h = Requests with Privilege ID == N are permitted, if access type is
allowed as defined by permission settings (UW, UR, SW, SR).
9         EXT                     R/W              1h             External Allowed ID.
0h = Requests with Privilege ID >= 6 are not allowed to region M,
regardless of permission settings (UW, UR, SW, SR).
1h = Requests with Privilege ID >= 6 are permitted, if access type is
allowed as defined by permission settings (UW, UR, SW, SR).
8         RESERVED                R                0h
7-6       RESERVED                R                1h
5         SR                      R/W              1h             Supervisor read permission.
0h = Supervisor read accesses are not allowed from region M.
1h = Supervisor write accesses are allowed from region M
addresses.
4         SW                      R/W              1h             Supervisor write permission.
0h = Supervisor write accesses are not allowed to region M.
1h = Supervisor write accesses are allowed to region N addresses.
3         SX                      R/W              0h             Supervisor execute permission.
0h = Supervisor execute accesses are not allowed from region M.
1h = Supervisor execute accesses are allowed from region M
addresses.
2         UR                      R/W              1h             User read permission.
0h = User read accesses are not allowed from region M.
1h = User read accesses are allowed from region N addresses.
1         UW                      R/W              1h             User write permission.
0h = User write accesses are not allowed to region M.
1h = User write accesses are allowed to region M addresses.
0         UX                      R/W              0h             User execute permission.
0h = User execute accesses are not allowed from region M.
1h = User execute accesses are allowed from region M addresses.


#### 11.4.1.91 ER Register (offset = 1000h) [reset = 0h]

ER is shown in Figure 11-130 and described in Table 11-117.
All external events are captured in the event register (ER/ERH). The events are latched even when the
events are not enabled. If the event bit corresponding to the latched event is enabled (EER.En/EERH.En
= 1), then the event is evaluated by the EDMA3CC logic for an associated transfer request submission to
the transfer controllers. The event register bits are automatically cleared (ER.En/ERH.En= 0) once the
corresponding events are prioritized and serviced. If ER.En/ERH.En are already set and another event is
received on the same channel/event, then the corresponding event is latched in the event miss register
(EMR.En/EMRH.En), provided that the event was enabled (EER.En/EERH.En = 1). Event n can be
cleared by the CPU writing a 1 to corresponding event bit in the event clear register (ECR/ECRH). The
setting of an event is a higher priority relative to clear operations (via hardware or software). If set and
clear conditions occur concurrently, the set condition wins. If the event was previously set, then
EMR/EMRH would be set because an event is lost. If the event was previously clear, then the event
remains set and is prioritized for submission to the event queues. The Debug List table provides the type
of synchronization events and the EDMA3CC channels associated to each of these external events. This
register is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels consist of
a set of registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of each
register matches the DMA channel number. Each register is named with the format reg_name that
corresponds to DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32
through 64. For example, the event register (ER) corresponds to DMA channel 0 through 31 and the event
register high register (ERH) corresponds to DMA channel 32 through 63. The register is typically called
the event register. The DMA channel registers are accessible via read/writes to the global address range.
They are also accessible via read/writes to the shadow address range. The read/write ability to the
registers in the shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-130. ER Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1    0
En
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-117. ER Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     En                          R               0h             Event 0 to 31.
Events 0 to 31 are captured by the EDMA3CC and are latched into
ER.
The events are set (En = 1) even when events are disabled (En = 0
in the event enable register, EER).
0h = EDMA3CC event is not asserted.
1h = EDMA3CC event is asserted. Corresponding DMA event is
prioritized versus other pending DMA/QDMA events for submission
to the EDMA3TC.


#### 11.4.1.92 ERH Register (offset = 1004h) [reset = 0h]

ERH is shown in Figure 11-131 and described in Table 11-118.
All external events are captured in the event register (ER/ERH). The events are latched even when the
events are not enabled. If the event bit corresponding to the latched event is enabled (EER.En/EERH.En
= 1), then the event is evaluated by the EDMA3CC logic for an associated transfer request submission to
the transfer controllers. The event register bits are automatically cleared (ER.En/ERH.En= 0) once the
corresponding events are prioritized and serviced. If ER.En/ERH.En are already set and another event is
received on the same channel/event, then the corresponding event is latched in the event miss register
(EMR.En/EMRH.En), provided that the event was enabled (EER.En/EERH.En = 1). Event n can be
cleared by the CPU writing a 1 to corresponding event bit in the event clear register (ECR/ECRH). The
setting of an event is a higher priority relative to clear operations (via hardware or software). If set and
clear conditions occur concurrently, the set condition wins. If the event was previously set, then
EMR/EMRH would be set because an event is lost. If the event was previously clear, then the event
remains set and is prioritized for submission to the event queues. The Debug List table provides the type
of synchronization events and the EDMA3CC channels associated to each of these external events. This
register is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels consist of
a set of registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of each
register matches the DMA channel number. Each register is named with the format reg_name that
corresponds to DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32
through 64. For example, the event register (ER) corresponds to DMA channel 0 through 31 and the event
register high register (ERH) corresponds to DMA channel 32 through 63. The register is typically called
the event register. The DMA channel registers are accessible via read/writes to the global address range.
They are also accessible via read/writes to the shadow address range. The read/write ability to the
registers in the shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-131. ERH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1   0
En
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-118. ERH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       En                         R               0h             Event 32 to 63.
Events 32 to 63 are captured by the EDMA3CC and are latched into
ERH.
The events are set (En = 1) even when events are disabled (En = 0
in the event enable register high, EERH).
0h = EDMA3CC event is not asserted.
1h = EDMA3CC event is asserted. Corresponding DMA event is
prioritized versus other pending DMA/QDMA events for submission
to the EDMA3TC.


#### 11.4.1.93 ECR Register (offset = 1008h) [reset = 0h]

ECR is shown in Figure 11-132 and described in Table 11-119.
Once an event has been posted in the event registers (ER/ERH), the event is cleared in two ways. If the
event is enabled in the event enable register (EER/EERH) and the EDMA3CC submits a transfer request
for the event to the EDMA3TC, it clears the corresponding event bit in the event register. If the event is
disabled in the event enable register (EER/EERH), the CPU can clear the event by way of the event clear
registers (ECR/ECRH). Writing a 1 to any of the bits clears the corresponding event; writing a 0 has no
effect. Once an event bit is set in the event register, it remains set until EDMA3CC submits a transfer
request for that event or the CPU clears the event by setting the corresponding bit in ECR/ECRH. This
register is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels consist of
a set of registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of each
register matches the DMA channel number. Each register is named with the format reg_name that
corresponds to DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32
through 64. For example, the event register (ER) corresponds to DMA channel 0 through 31 and the event
register high register (ERH) corresponds to DMA channel 32 through 63. The register is typically called
the event register. The DMA channel registers are accessible via read/writes to the global address range.
They are also accessible via read/writes to the shadow address range. The read/write ability to the
registers in the shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-132. ECR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7    6   5    4   3    2    1    0
En
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-119. ECR Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     En                          W               0h             Event clear for event 0 to 31.
Any of the event bits in ECR is set to clear the event (En) in the
event register (ER).
A write of 0 has no effect.
0h = No effect.
1h = EDMA3CC event is cleared in the event register (ER).


#### 11.4.1.94 ECRH Register (offset = 100Ch) [reset = 0h]

ECRH is shown in Figure 11-133 and described in Table 11-120.
Once an event has been posted in the event registers (ER/ERH), the event is cleared in two ways. If the
event is enabled in the event enable register (EER/EERH) and the EDMA3CC submits a transfer request
for the event to the EDMA3TC, it clears the corresponding event bit in the event register. If the event is
disabled in the event enable register (EER/EERH), the CPU can clear the event by way of the event clear
registers (ECR/ECRH). Writing a 1 to any of the bits clears the corresponding event; writing a 0 has no
effect. Once an event bit is set in the event register, it remains set until EDMA3CC submits a transfer
request for that event or the CPU clears the event by setting the corresponding bit in ECR/ECRH. This
register is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels consist of
a set of registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of each
register matches the DMA channel number. Each register is named with the format reg_name that
corresponds to DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32
through 64. For example, the event register (ER) corresponds to DMA channel 0 through 31 and the event
register high register (ERH) corresponds to DMA channel 32 through 63. The register is typically called
the event register. The DMA channel registers are accessible via read/writes to the global address range.
They are also accessible via read/writes to the shadow address range. The read/write ability to the
registers in the shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-133. ECRH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5    4   3    2   1       0
En
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-120. ECRH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       En                         W               0h             Event clear for event 32 to 63.
Any of the event bits in ECRH are set to clear the event (En) in the
event register high (ERH).
A write of 0 has no effect.
0h = No effect.
1h = EDMA3CC event is cleared in the event register high (ERH).


#### 11.4.1.95 ESR Register (offset = 1010h) [reset = 0h]

ESR is shown in Figure 11-134 and described in Table 11-121.
The event set registers (ESR/ESRH) allow the CPU (EDMA3 programmers) to manually set events to
initiate DMA transfer requests. CPU writes of 1 to any event set register (En) bits set the corresponding
bits in the registers. The set event is evaluated by the EDMA3CC logic for an associated transfer request
submission to the transfer controllers. Writing a 0 has no effect. The event set registers operate
independent of the event registers (ER/ERH), and a write of 1 is always considered a valid event
regardless of whether the event is enabled (the corresponding event bits are set or cleared in
EER.En/EERH.En). Once the event is set in the event set registers, it cannot be cleared by CPU writes, in
other words, the event clear registers (ECR/ECRH) have no effect on the state of ESR/ESRH. The bits will
only be cleared once the transfer request corresponding to the event has been submitted to the transfer
controller. The setting of an event is a higher priority relative to clear operations (via hardware). If set and
clear conditions occur concurrently, the set condition wins. If the event was previously set, then
EMR/EMRH would be set because an event is lost. If the event was previously clear, then the event
remains set and is prioritized for submission to the event queues. Manually-triggered transfers via writes
to ESR/ESRH allow the CPU to submit DMA requests in the system, these are relevant for memory-to-
memory transfer scenarios. If the ESR.En/ESRH.En bit is already set and another CPU write of 1 is
attempted to the same bit, then the corresponding event is latched in the event missed registers
(EMR.En/EMRH.En = 1). This register is part of a set of registers that pertain to the 64 DMA channels.
The 64 DMA channels consist of a set of registers (with exception of DMAQNUMn) that each have 64 bits
and the bit position of each register matches the DMA channel number. Each register is named with the
format reg_name that corresponds to DMA channels 0 through 31 and reg_name_High that corresponds
to DMA channels 32 through 64. For example, the event register (ER) corresponds to DMA channel 0
through 31 and the event register high register (ERH) corresponds to DMA channel 32 through 63. The
register is typically called the event register. The DMA channel registers are accessible via read/writes to
the global address range. They are also accessible via read/writes to the shadow address range. The
read/write ability to the registers in the shadow region are controlled by the DMA region access registers
(DRAEm/DRAEHm).


**Figure 11-134. ESR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8   7   6   5   4   3   2   1     0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-121. ESR Register Field Descriptions**

Bit      Field                       Type          Reset          Description
31-0     En                          R/W           0h             Event set for event 0 to 31.
0h = No effect.
1h = Corresponding DMA event is prioritized versus other pending
DMA/QDMA events for submission to the EDMA3TC.


#### 11.4.1.96 ESRH Register (offset = 1014h) [reset = 0h]

ESRH is shown in Figure 11-135 and described in Table 11-122.
The event set registers (ESR/ESRH) allow the CPU (EDMA3 programmers) to manually set events to
initiate DMA transfer requests. CPU writes of 1 to any event set register (En) bits set the corresponding
bits in the registers. The set event is evaluated by the EDMA3CC logic for an associated transfer request
submission to the transfer controllers. Writing a 0 has no effect. The event set registers operate
independent of the event registers (ER/ERH), and a write of 1 is always considered a valid event
regardless of whether the event is enabled (the corresponding event bits are set or cleared in
EER.En/EERH.En). Once the event is set in the event set registers, it cannot be cleared by CPU writes, in
other words, the event clear registers (ECR/ECRH) have no effect on the state of ESR/ESRH. The bits will
only be cleared once the transfer request corresponding to the event has been submitted to the transfer
controller. The setting of an event is a higher priority relative to clear operations (via hardware). If set and
clear conditions occur concurrently, the set condition wins. If the event was previously set, then
EMR/EMRH would be set because an event is lost. If the event was previously clear, then the event
remains set and is prioritized for submission to the event queues. Manually-triggered transfers via writes
to ESR/ESRH allow the CPU to submit DMA requests in the system, these are relevant for memory-to-
memory transfer scenarios. If the ESR.En/ESRH.En bit is already set and another CPU write of 1 is
attempted to the same bit, then the corresponding event is latched in the event missed registers
(EMR.En/EMRH.En = 1). This register is part of a set of registers that pertain to the 64 DMA channels.
The 64 DMA channels consist of a set of registers (with exception of DMAQNUMn) that each have 64 bits
and the bit position of each register matches the DMA channel number. Each register is named with the
format reg_name that corresponds to DMA channels 0 through 31 and reg_name_High that corresponds
to DMA channels 32 through 64. For example, the event register (ER) corresponds to DMA channel 0
through 31 and the event register high register (ERH) corresponds to DMA channel 32 through 63. The
register is typically called the event register. The DMA channel registers are accessible via read/writes to
the global address range. They are also accessible via read/writes to the shadow address range. The
read/write ability to the registers in the shadow region are controlled by the DMA region access registers
(DRAEm/DRAEHm).


**Figure 11-135. ESRH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                  9   8   7   6   5   4   3   2   1     0
En
R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-122. ESRH Register Field Descriptions**

Bit    Field                      Type          Reset          Description
31-0       En                         R/W           0h             Event set for event 32 to 63.
0h = No effect.
1h = Corresponding DMA event is prioritized versus other pending
DMA/QDMA events for submission to the EDMA3TC.


#### 11.4.1.97 CER Register (offset = 1018h) [reset = 0h]

CER is shown in Figure 11-136 and described in Table 11-123.
When the OPTIONS parameter for a PaRAM entry is programmed to returned a chained completion code
(ITCCHEN = 1 and/or TCCHEN = 1), then the value dictated by the TCC[5:0] (also programmed in OPT)
forces the corresponding event bit to be set in the chained event registers (CER/CERH). The set chained
event is evaluated by the EDMA3CC logic for an associated transfer request submission to the transfer
controllers. This results in a chained-triggered transfer. The chained event registers do not have any
enables. The generation of a chained event is essentially enabled by the PaRAM entry that has been
configured for intermediate and/or final chaining on transfer completion. The En bit is set (regardless of
the state of EER.En/EERH.En) when a chained completion code is returned from one of the transfer
controllers or is generated by the EDMA3CC via the early completion path. The bits in the chained event
register are cleared when the corresponding events are prioritized and serviced. If the En bit is already set
and another chaining completion code is return for the same event, then the corresponding event is
latched in the event missed registers (EMR.En/EMRH.En= 1). The setting of an event is a higher priority
relative to clear operations (via hardware). If set and clear conditions occur concurrently, the set condition
wins. If the event was previously set, then EMR/EMRH would be set because an event is lost. If the event
was previously clear, then the event remains set and is prioritized for submission to the event queues.
This register is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels
consist of a set of registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of
each register matches the DMA channel number. Each register is named with the format reg_name that
corresponds to DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32
through 64. For example, the event register (ER) corresponds to DMA channel 0 through 31 and the event
register high register (ERH) corresponds to DMA channel 32 through 63. The register is typically called
the event register. The DMA channel registers are accessible via read/writes to the global address range.
They are also accessible via read/writes to the shadow address range. The read/write ability to the
registers in the shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-136. CER Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1     0
En
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-123. CER Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     En                          R               0h             Chained event for event 0 to 31.
0h = No effect.
1h = Corresponding DMA event is prioritized versus other pending
DMA/QDMA events for submission to the EDMA3TC.


#### 11.4.1.98 CERH Register (offset = 101Ch) [reset = 0h]

CERH is shown in Figure 11-137 and described in Table 11-124.
When the OPTIONS parameter for a PaRAM entry is programmed to returned a chained completion code
(ITCCHEN = 1 and/or TCCHEN = 1), then the value dictated by the TCC[5:0] (also programmed in OPT)
forces the corresponding event bit to be set in the chained event registers (CER/CERH). The set chained
event is evaluated by the EDMA3CC logic for an associated transfer request submission to the transfer
controllers. This results in a chained-triggered transfer. The chained event registers do not have any
enables. The generation of a chained event is essentially enabled by the PaRAM entry that has been
configured for intermediate and/or final chaining on transfer completion. The En bit is set (regardless of
the state of EER.En/EERH.En) when a chained completion code is returned from one of the transfer
controllers or is generated by the EDMA3CC via the early completion path. The bits in the chained event
register are cleared when the corresponding events are prioritized and serviced. If the En bit is already set
and another chaining completion code is return for the same event, then the corresponding event is
latched in the event missed registers (EMR.En/EMRH.En= 1). The setting of an event is a higher priority
relative to clear operations (via hardware). If set and clear conditions occur concurrently, the set condition
wins. If the event was previously set, then EMR/EMRH would be set because an event is lost. If the event
was previously clear, then the event remains set and is prioritized for submission to the event queues.
This register is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels
consist of a set of registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of
each register matches the DMA channel number. Each register is named with the format reg_name that
corresponds to DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32
through 64. For example, the event register (ER) corresponds to DMA channel 0 through 31 and the event
register high register (ERH) corresponds to DMA channel 32 through 63. The register is typically called
the event register. The DMA channel registers are accessible via read/writes to the global address range.
They are also accessible via read/writes to the shadow address range. The read/write ability to the
registers in the shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-137. CERH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1     0
En
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-124. CERH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       En                         R               0h             Chained event set for event 32 to 63.
0h = No effect.
1h = Corresponding DMA event is prioritized versus other pending
DMA/QDMA events for submission to the EDMA3TC.


#### 11.4.1.99 EER Register (offset = 1020h) [reset = 0h]

EER is shown in Figure 11-138 and described in Table 11-125.
The EDMA3CC provides the option of selectively enabling/disabling each event in the event registers
(ER/ERH) by using the event enable registers (EER/EERH). If an event bit in EER/EERH is set (using the
event enable set registers, EESR/EESRH), it will enable that corresponding event. Alternatively, if an
event bit in EER/EERH is cleared (using the event enable clear registers, EECR/EECRH), it will disable
the corresponding event. The event registers latch all events that are captured by EDMA3CC, even if the
events are disabled (although EDMA3CC does not process it). Enabling an event with a pending event
already set in the event registers enables the EDMA3CC to process the already set event like any other
new event. The EER/EERH settings do not have any effect on chained events (CER.En/CERH.En= 1) and
manually set events (ESR.En/ESRH.En= 1). This register is part of a set of registers that pertain to the 64
DMA channels. The 64 DMA channels consist of a set of registers (with exception of DMAQNUMn) that
each have 64 bits and the bit position of each register matches the DMA channel number. Each register is
named with the format reg_name that corresponds to DMA channels 0 through 31 and reg_name_High
that corresponds to DMA channels 32 through 64. For example, the event register (ER) corresponds to
DMA channel 0 through 31 and the event register high register (ERH) corresponds to DMA channel 32
through 63. The register is typically called the event register. The DMA channel registers are accessible
via read/writes to the global address range. They are also accessible via read/writes to the shadow
address range. The read/write ability to the registers in the shadow region are controlled by the DMA
region access registers (DRAEm/DRAEHm).


**Figure 11-138. EER Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3    2   1      0
En
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-125. EER Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     En                          R               0h             Event enable for events 0 to 31.
0h = Event is not enabled. An external event latched in the event
register (ER) is not evaluated by the EDMA3CC.
1h = Event is enabled. An external event latched in the event
register (ER) is evaluated by the EDMA3CC.


#### 11.4.1.100 EERH Register (offset = 1024h) [reset = 0h]

EERH is shown in Figure 11-139 and described in Table 11-126.
The EDMA3CC provides the option of selectively enabling/disabling each event in the event registers
(ER/ERH) by using the event enable registers (EER/EERH). If an event bit in EER/EERH is set (using the
event enable set registers, EESR/EESRH), it will enable that corresponding event. Alternatively, if an
event bit in EER/EERH is cleared (using the event enable clear registers, EECR/EECRH), it will disable
the corresponding event. The event registers latch all events that are captured by EDMA3CC, even if the
events are disabled (although EDMA3CC does not process it). Enabling an event with a pending event
already set in the event registers enables the EDMA3CC to process the already set event like any other
new event. The EER/EERH settings do not have any effect on chained events (CER.En/CERH.En= 1) and
manually set events (ESR.En/ESRH.En= 1). This register is part of a set of registers that pertain to the 64
DMA channels. The 64 DMA channels consist of a set of registers (with exception of DMAQNUMn) that
each have 64 bits and the bit position of each register matches the DMA channel number. Each register is
named with the format reg_name that corresponds to DMA channels 0 through 31 and reg_name_High
that corresponds to DMA channels 32 through 64. For example, the event register (ER) corresponds to
DMA channel 0 through 31 and the event register high register (ERH) corresponds to DMA channel 32
through 63. The register is typically called the event register. The DMA channel registers are accessible
via read/writes to the global address range. They are also accessible via read/writes to the shadow
address range. The read/write ability to the registers in the shadow region are controlled by the DMA
region access registers (DRAEm/DRAEHm).


**Figure 11-139. EERH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3    2   1     0
En
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-126. EERH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       En                         R               0h             Event enable for events 32 to 63.
0h = Event is not enabled. An external event latched in the event
register high (ERH) is not evaluated by the EDMA3CC.
1h = Event is enabled. An external event latched in the event
register high (ERH) is evaluated by the EDMA3CC.


#### 11.4.1.101 EECR Register (offset = 1028h) [reset = 0h]

EECR is shown in Figure 11-140 and described in Table 11-127.
The event enable registers (EER/EERH) cannot be modified by directly writing to them. The intent is to
ease the software burden for the case where multiple tasks are attempting to simultaneously modify these
registers. The event enable clear registers (EECR/EECRH) are used to disable events. Writes of 1 to the
bits in EECR/EECRH clear the corresponding event bits in EER/EERH; writes of 0 have no effect. This
register is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels consist of
a set of registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of each
register matches the DMA channel number. Each register is named with the format reg_name that
corresponds to DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32
through 64. For example, the event register (ER) corresponds to DMA channel 0 through 31 and the event
register high register (ERH) corresponds to DMA channel 32 through 63. The register is typically called
the event register. The DMA channel registers are accessible via read/writes to the global address range.
They are also accessible via read/writes to the shadow address range. The read/write ability to the
registers in the shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-140. EECR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3    2     1    0
En
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-127. EECR Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     En                          W               0h             Event enable clear for events 0 to 31.
0h = No effect.
1h = Event is disabled. Corresponding bit in the event enable
register (EER) is cleared (En = 0).


#### 11.4.1.102 EECRH Register (offset = 102Ch) [reset = 0h]

EECRH is shown in Figure 11-141 and described in Table 11-128.
The event enable registers (EER/EERH) cannot be modified by directly writing to them. The intent is to
ease the software burden for the case where multiple tasks are attempting to simultaneously modify these
registers. The event enable clear registers (EECR/EECRH) are used to disable events. Writes of 1 to the
bits in EECR/EECRH clear the corresponding event bits in EER/EERH; writes of 0 have no effect. This
register is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels consist of
a set of registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of each
register matches the DMA channel number. Each register is named with the format reg_name that
corresponds to DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32
through 64. For example, the event register (ER) corresponds to DMA channel 0 through 31 and the event
register high register (ERH) corresponds to DMA channel 32 through 63. The register is typically called
the event register. The DMA channel registers are accessible via read/writes to the global address range.
They are also accessible via read/writes to the shadow address range. The read/write ability to the
registers in the shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-141. EECRH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3    2     1   0
En
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-128. EECRH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       En                         W               0h             Event enable clear for events 32 to 63.
0h = No effect.
1h = Event is disabled. Corresponding bit in the event enable
register high (EERH) is cleared (En = 0).


#### 11.4.1.103 EESR Register (offset = 1030h) [reset = 0h]

EESR is shown in Figure 11-142 and described in Table 11-129.
The event enable registers (EER/EERH) cannot be modified by directly writing to them. The intent is to
ease the software burden for the case where multiple tasks are attempting to simultaneously modify these
registers. The event enable set registers (EESR/EESRH) are used to enable events. Writes of 1 to the bits
in EESR/EESRH set the corresponding event bits in EER/EERH; writes of 0 have no effect. This register
is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels consist of a set of
registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of each register
matches the DMA channel number. Each register is named with the format reg_name that corresponds to
DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32 through 64. For
example, the event register (ER) corresponds to DMA channel 0 through 31 and the event register high
register (ERH) corresponds to DMA channel 32 through 63. The register is typically called the event
register. The DMA channel registers are accessible via read/writes to the global address range. They are
also accessible via read/writes to the shadow address range. The read/write ability to the registers in the
shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-142. EESR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3    2   1    0
En
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-129. EESR Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     En                          W               0h             Event enable set for events 0 to 31.
0h = No effect.
1h = Event is enabled. Corresponding bit in the event enable register
(EER) is set (En = 1).


#### 11.4.1.104 EESRH Register (offset = 1034h) [reset = 0h]

EESRH is shown in Figure 11-143 and described in Table 11-130.
The event enable registers (EER/EERH) cannot be modified by directly writing to them. The intent is to
ease the software burden for the case where multiple tasks are attempting to simultaneously modify these
registers. The event enable set registers (EESR/EESRH) are used to enable events. Writes of 1 to the bits
in EESR/EESRH set the corresponding event bits in EER/EERH; writes of 0 have no effect. This register
is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels consist of a set of
registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of each register
matches the DMA channel number. Each register is named with the format reg_name that corresponds to
DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32 through 64. For
example, the event register (ER) corresponds to DMA channel 0 through 31 and the event register high
register (ERH) corresponds to DMA channel 32 through 63. The register is typically called the event
register. The DMA channel registers are accessible via read/writes to the global address range. They are
also accessible via read/writes to the shadow address range. The read/write ability to the registers in the
shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-143. EESRH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3    2   1    0
En
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-130. EESRH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       En                         W               0h             Event enable set for events 32 to 63.
0h = No effect.
1h = Event is enabled. Corresponding bit in the event enable register
high (EERH) is set (En= 1).


#### 11.4.1.105 SER Register (offset = 1038h) [reset = 0h]

SER is shown in Figure 11-144 and described in Table 11-131.
The secondary event registers (SER/SERH) provide information on the state of a DMA channel or event
(0 through 63). If the EDMA3CC receives a TR synchronization due to a manual-trigger, event-trigger, or
chained-trigger source (ESR.En/ESRH.En= 1, ER.En/ERH.En= 1, or CER.En/CERH.En= 1), which results
in the setting of a corresponding event bit in SER/SERH (SER.En/SERH.En = 1), it implies that the
corresponding DMA event is in the queue. Once a bit corresponding to an event is set in SER/SERH, the
EDMA3CC does not prioritize additional events on the same DMA channel. Depending on the condition
that lead to the setting of the SER bits, either the EDMA3CC hardware or the software (using
SECR/SECRH) needs to clear the SER/SERH bits for the EDMA3CC to evaluate subsequent events
(subsequent transfers) on the same channel. For additional conditions that can cause the secondary event
registers to be set, see EDMA Overview. This register is part of a set of registers that pertain to the 64
DMA channels. The 64 DMA channels consist of a set of registers (with exception of DMAQNUMn) that
each have 64 bits and the bit position of each register matches the DMA channel number. Each register is
named with the format reg_name that corresponds to DMA channels 0 through 31 and reg_name_High
that corresponds to DMA channels 32 through 64. For example, the event register (ER) corresponds to
DMA channel 0 through 31 and the event register high register (ERH) corresponds to DMA channel 32
through 63. The register is typically called the event register. The DMA channel registers are accessible
via read/writes to the global address range. They are also accessible via read/writes to the shadow
address range. The read/write ability to the registers in the shadow region are controlled by the DMA
region access registers (DRAEm/DRAEHm).


**Figure 11-144. SER Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7    6   5    4   3    2   1       0
En
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-131. SER Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     En                          R               0h             Secondary event register.
The secondary event register is used along with the event register
(ER) to provide information on the state of an event.
0h = Event is not currently stored in the event queue.
1h = Event is currently stored in the event queue. Event arbiter will
not prioritize additional events.


#### 11.4.1.106 SERH Register (offset = 103Ch) [reset = 0h]

SERH is shown in Figure 11-145 and described in Table 11-132.
The secondary event registers (SER/SERH) provide information on the state of a DMA channel or event
(0 through 63). If the EDMA3CC receives a TR synchronization due to a manual-trigger, event-trigger, or
chained-trigger source (ESR.En/ESRH.En= 1, ER.En/ERH.En= 1, or CER.En/CERH.En= 1), which results
in the setting of a corresponding event bit in SER/SERH (SER.En/SERH.En = 1), it implies that the
corresponding DMA event is in the queue. Once a bit corresponding to an event is set in SER/SERH, the
EDMA3CC does not prioritize additional events on the same DMA channel. Depending on the condition
that lead to the setting of the SER bits, either the EDMA3CC hardware or the software (using
SECR/SECRH) needs to clear the SER/SERH bits for the EDMA3CC to evaluate subsequent events
(subsequent transfers) on the same channel. For additional conditions that can cause the secondary event
registers to be set, see EDMA Overview. This register is part of a set of registers that pertain to the 64
DMA channels. The 64 DMA channels consist of a set of registers (with exception of DMAQNUMn) that
each have 64 bits and the bit position of each register matches the DMA channel number. Each register is
named with the format reg_name that corresponds to DMA channels 0 through 31 and reg_name_High
that corresponds to DMA channels 32 through 64. For example, the event register (ER) corresponds to
DMA channel 0 through 31 and the event register high register (ERH) corresponds to DMA channel 32
through 63. The register is typically called the event register. The DMA channel registers are accessible
via read/writes to the global address range. They are also accessible via read/writes to the shadow
address range. The read/write ability to the registers in the shadow region are controlled by the DMA
region access registers (DRAEm/DRAEHm).


**Figure 11-145. SERH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8    7   6    5    4    3   2    1      0
En
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-132. SERH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       En                         R               0h             Secondary event register.
The secondary event register is used along with the event register
high (ERH) to provide information on the state of an event.
0h = Event is not currently stored in the event queue.
1h = Event is currently stored in the event queue. Event
submission/prioritization logic will not prioritize additional events.


#### 11.4.1.107 SECR Register (offset = 1040h) [reset = 0h]

SECR is shown in Figure 11-146 and described in Table 11-133.
The secondary event clear registers (SECR/SECRH) clear the status of the secondary event registers
(SER/SERH). CPU writes of 1 clear the corresponding set bits in SER/SERH. Writes of 0 have no effect.
This register is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels
consist of a set of registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of
each register matches the DMA channel number. Each register is named with the format reg_name that
corresponds to DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32
through 64. For example, the event register (ER) corresponds to DMA channel 0 through 31 and the event
register high register (ERH) corresponds to DMA channel 32 through 63. The register is typically called
the event register. The DMA channel registers are accessible via read/writes to the global address range.
They are also accessible via read/writes to the shadow address range. The read/write ability to the
registers in the shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-146. SECR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2    1    0
En
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-133. SECR Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     En                          W               0h             Secondary event clear register.
0h = No effect.
1h = Corresponding bit in the secondary event register (SER) is
cleared (En = 0).


#### 11.4.1.108 SECRH Register (offset = 1044h) [reset = 0h]

SECRH is shown in Figure 11-147 and described in Table 11-134.
The secondary event clear registers (SECR/SECRH) clear the status of the secondary event registers
(SER/SERH). CPU writes of 1 clear the corresponding set bits in SER/SERH. Writes of 0 have no effect.
This register is part of a set of registers that pertain to the 64 DMA channels. The 64 DMA channels
consist of a set of registers (with exception of DMAQNUMn) that each have 64 bits and the bit position of
each register matches the DMA channel number. Each register is named with the format reg_name that
corresponds to DMA channels 0 through 31 and reg_name_High that corresponds to DMA channels 32
through 64. For example, the event register (ER) corresponds to DMA channel 0 through 31 and the event
register high register (ERH) corresponds to DMA channel 32 through 63. The register is typically called
the event register. The DMA channel registers are accessible via read/writes to the global address range.
They are also accessible via read/writes to the shadow address range. The read/write ability to the
registers in the shadow region are controlled by the DMA region access registers (DRAEm/DRAEHm).


**Figure 11-147. SECRH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1   0
En
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-134. SECRH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       En                         W               0h             Secondary event clear register.
0h = No effect.
1h = Corresponding bit in the secondary event registers high (SERH)
is cleared (En = 0).


#### 11.4.1.109 IER Register (offset = 1050h) [reset = 0h]

IER is shown in Figure 11-148 and described in Table 11-135.
Interrupt enable registers (IER/IERH) are used to enable/disable the transfer completion interrupt
generation by the EDMA3CC for all DMA/QDMA channels. The IER/IERH cannot be written to directly. To
set any interrupt bit in IER/IERH, a 1 must be written to the corresponding interrupt bit in the interrupt
enable set registers (IESR/IESRH). Similarly, to clear any interrupt bit in IER/IERH, a 1 must be written to
the corresponding interrupt bit in the interrupt enable clear registers (IECR/IECRH). All DMA/QDMA
channels can be set to assert an EDMA3CC completion interrupt to the CPU on transfer completion, by
appropriately configuring the PaRAM entry associated with the channels. This register is used for the
transfer completion interrupt reporting/generating by the EDMA3CC. For more details on EDMA3CC
completion interrupt generation, see EDMA3 Interrupts.


**Figure 11-148. IER Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1    0
In_
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-135. IER Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     In_                         R               0h             Interrupt enable for channels 0 to 31.
0h = Interrupt is not enabled.
1h = Interrupt is enabled.


#### 11.4.1.110 IERH Register (offset = 1054h) [reset = 0h]

IERH is shown in Figure 11-149 and described in Table 11-136.
Interrupt enable registers (IER/IERH) are used to enable/disable the transfer completion interrupt
generation by the EDMA3CC for all DMA/QDMA channels. The IER/IERH cannot be written to directly. To
set any interrupt bit in IER/IERH, a 1 must be written to the corresponding interrupt bit in the interrupt
enable set registers (IESR/IESRH). Similarly, to clear any interrupt bit in IER/IERH, a 1 must be written to
the corresponding interrupt bit in the interrupt enable clear registers (IECR/IECRH). All DMA/QDMA
channels can be set to assert an EDMA3CC completion interrupt to the CPU on transfer completion, by
appropriately configuring the PaRAM entry associated with the channels. This register is used for the
transfer completion interrupt reporting/generating by the EDMA3CC. For more details on EDMA3CC
completion interrupt generation, see EDMA3 Interrupts.


**Figure 11-149. IERH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7    6   5   4   3   2   1   0
In
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-136. IERH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       In                         R               0h             Interrupt enable for channels 32 to 63.
0h = Interrupt is not enabled.
1h = Interrupt is enabled.


#### 11.4.1.111 IECR Register (offset = 1058h) [reset = 0h]

IECR is shown in Figure 11-150 and described in Table 11-137.
The interrupt enable clear registers (IECR/IECRH) are used to clear interrupts. Writes of 1 to the bits in
IECR/IECRH clear the corresponding interrupt bits in the interrupt enable registers (IER/IERH); writes of 0
have no effect. All DMA/QDMA channels can be set to assert an EDMA3CC completion interrupt to the
CPU on transfer completion, by appropriately configuring the PaRAM entry associated with the channels.
This register is used for the transfer completion interrupt reporting/generating by the EDMA3CC. For more
details on EDMA3CC completion interrupt generation, see EDMA3 Interrupts.


**Figure 11-150. IECR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6    5    4   3    2      1    0
In
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-137. IECR Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     In                          W               0h             Interrupt enable clear for channels 0 to 31.
0h = No effect.
1h = Corresponding bit in the interrupt enable register (IER) is
cleared (In = 0).


#### 11.4.1.112 IECRH Register (offset = 105Ch) [reset = 0h]

IECRH is shown in Figure 11-151 and described in Table 11-138.
The interrupt enable clear registers (IECR/IECRH) are used to clear interrupts. Writes of 1 to the bits in
IECR/IECRH clear the corresponding interrupt bits in the interrupt enable registers (IER/IERH); writes of 0
have no effect. All DMA/QDMA channels can be set to assert an EDMA3CC completion interrupt to the
CPU on transfer completion, by appropriately configuring the PaRAM entry associated with the channels.
This register is used for the transfer completion interrupt reporting/generating by the EDMA3CC. For more
details on EDMA3CC completion interrupt generation, see EDMA3 Interrupts.


**Figure 11-151. IECRH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6    5   4    3   2    1   0
In
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-138. IECRH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       In                         W               0h             Interrupt enable clear for channels 32 to 63.
0h = No effect.
1h = Corresponding bit in the interrupt enable register high (IERH) is
cleared (In = 0).


#### 11.4.1.113 IESR Register (offset = 1060h) [reset = 0h]

IESR is shown in Figure 11-152 and described in Table 11-139.
The interrupt enable set registers (IESR/IESRH) are used to enable interrupts. Writes of 1 to the bits in
IESR/IESRH set the corresponding interrupt bits in the interrupt enable registers (IER/IERH); writes of 0
have no effect. All DMA/QDMA channels can be set to assert an EDMA3CC completion interrupt to the
CPU on transfer completion, by appropriately configuring the PaRAM entry associated with the channels.
This register is used for the transfer completion interrupt reporting/generating by the EDMA3CC. For more
details on EDMA3CC completion interrupt generation, see EDMA3 Interrupts.


**Figure 11-152. IESR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7    6    5   4    3   2    1    0
In
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-139. IESR Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     In                          W               0h             Interrupt enable set for channels 0 to 31.
0h = No effect.
1h = Corresponding bit in the interrupt enable register (IER) is set (In
= 1).


#### 11.4.1.114 IESRH Register (offset = 1064h) [reset = 0h]

IESRH is shown in Figure 11-153 and described in Table 11-140.
The interrupt enable set registers (IESR/IESRH) are used to enable interrupts. Writes of 1 to the bits in
IESR/IESRH set the corresponding interrupt bits in the interrupt enable registers (IER/IERH); writes of 0
have no effect. All DMA/QDMA channels can be set to assert an EDMA3CC completion interrupt to the
CPU on transfer completion, by appropriately configuring the PaRAM entry associated with the channels.
This register is used for the transfer completion interrupt reporting/generating by the EDMA3CC. For more
details on EDMA3CC completion interrupt generation, see EDMA3 Interrupts.


**Figure 11-153. IESRH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6    5   4    3   2    1   0
In
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-140. IESRH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       In                         W               0h             Interrupt enable clear for channels 32 to 63.
0h = No effect.
1h = Corresponding bit in the interrupt enable register high (IERH) is
set (In = 1).


#### 11.4.1.115 IPR Register (offset = 1068h) [reset = 0h]

IPR is shown in Figure 11-154 and described in Table 11-141.
If the TCINTEN and/or ITCINTEN bit in the channel option parameter (OPT) is set in the PaRAM entry
associated with the channel (DMA or QDMA), then the EDMA3TC (for normal completion) or the
EDMA3CC (for early completion) returns a completion code on transfer or intermediate transfer
completion. The value of the returned completion code is equal to the TCC bit in OPT for the PaRAM
entry associated with the channel. When an interrupt transfer completion code with TCC = n is detected
by the EDMA3CC, then the corresponding bit is set in the interrupt pending register (IPR.In, if n = 0 to 31;
IPRH.In, if n = 32 to 63). Note that once a bit is set in the interrupt pending registers, it remains set; it is
your responsibility to clear these bits. The bits set in IPR/IPRH are cleared by writing a 1 to the
corresponding bits in the interrupt clear registers (ICR/ICRH). All DMA/QDMA channels can be set to
assert an EDMA3CC completion interrupt to the CPU on transfer completion, by appropriately configuring
the PaRAM entry associated with the channels. This register is used for the transfer completion interrupt
reporting/generating by the EDMA3CC. For more details on EDMA3CC completion interrupt generation,
see EDMA3 Interrupts.


**Figure 11-154. IPR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6    5   4    3   2       1    0
In
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-141. IPR Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     In                          R               0h             Interrupt pending for TCC = 0 to 31.
0h = Interrupt transfer completion code is not detected or was
cleared.
1h = Interrupt transfer completion code is detected (In = 1, n =
EDMA3TC[2:0]).


#### 11.4.1.116 IPRH Register (offset = 106Ch) [reset = 0h]

IPRH is shown in Figure 11-155 and described in Table 11-142.
If the TCINTEN and/or ITCINTEN bit in the channel option parameter (OPT) is set in the PaRAM entry
associated with the channel (DMA or QDMA), then the EDMA3TC (for normal completion) or the
EDMA3CC (for early completion) returns a completion code on transfer or intermediate transfer
completion. The value of the returned completion code is equal to the TCC bit in OPT for the PaRAM
entry associated with the channel. When an interrupt transfer completion code with TCC = n is detected
by the EDMA3CC, then the corresponding bit is set in the interrupt pending register (IPR.In, if n = 0 to 31;
IPRH.In, if n = 32 to 63). Note that once a bit is set in the interrupt pending registers, it remains set; it is
your responsibility to clear these bits. The bits set in IPR/IPRH are cleared by writing a 1 to the
corresponding bits in the interrupt clear registers (ICR/ICRH). All DMA/QDMA channels can be set to
assert an EDMA3CC completion interrupt to the CPU on transfer completion, by appropriately configuring
the PaRAM entry associated with the channels. This register is used for the transfer completion interrupt
reporting/generating by the EDMA3CC. For more details on EDMA3CC completion interrupt generation,
see EDMA3 Interrupts.


**Figure 11-155. IPRH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6    5   4    3   2       1   0
In
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-142. IPRH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       In                         R               0h             Interrupt pending for TCC = 32 to 63.
0h = Interrupt transfer completion code is not detected or was
cleared.
1h = Interrupt transfer completion code is detected (In = 1, n =
EDMA3TC[2:0]).


#### 11.4.1.117 ICR Register (offset = 1070h) [reset = 0h]

ICR is shown in Figure 11-156 and described in Table 11-143.
The bits in the interrupt pending registers (IPR/IPRH) are cleared by writing a 1 to the corresponding bits
in the interrupt clear registers(ICR/ICRH). Writes of 0 have no effect. All set bits in IPR/IPRH must be
cleared to allow EDMA3CC to assert additional transfer completion interrupts. All DMA/QDMA channels
can be set to assert an EDMA3CC completion interrupt to the CPU on transfer completion, by
appropriately configuring the PaRAM entry associated with the channels. This register is used for the
transfer completion interrupt reporting/generating by the EDMA3CC. For more details on EDMA3CC
completion interrupt generation, see EDMA3 Interrupts.


**Figure 11-156. ICR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6    5   4    3   2    1    0
In
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-143. ICR Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     In                          W               0h             Interrupt clear register for TCC = 0 to 31.
0h = No effect.
1h = Corresponding bit in the interrupt pending register (IPR) is
cleared (In = 0).


#### 11.4.1.118 ICRH Register (offset = 1074h) [reset = 0h]

ICRH is shown in Figure 11-157 and described in Table 11-144.
The bits in the interrupt pending registers (IPR/IPRH) are cleared by writing a 1 to the corresponding bits
in the interrupt clear registers(ICR/ICRH). Writes of 0 have no effect. All set bits in IPR/IPRH must be
cleared to allow EDMA3CC to assert additional transfer completion interrupts. All DMA/QDMA channels
can be set to assert an EDMA3CC completion interrupt to the CPU on transfer completion, by
appropriately configuring the PaRAM entry associated with the channels. This register is used for the
transfer completion interrupt reporting/generating by the EDMA3CC. For more details on EDMA3CC
completion interrupt generation, see EDMA3 Interrupts.


**Figure 11-157. ICRH Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4    3   2    1   0
In
W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-144. ICRH Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       In                         W               0h             Interrupt clear register for TCC = 32 to 63.
0h = No effect.
1h = Corresponding bit in the interrupt pending register high (IPRH)
is cleared (In = 0).


#### 11.4.1.119 IEVAL Register (offset = 1078h) [reset = 0h]

IEVAL is shown in Figure 11-158 and described in Table 11-145.
The interrupt evaluate register (IEVAL) is the only register that physically exists in both the global region
and the shadow regions. In other words, the read/write accessibility for the shadow region IEVAL is not
affected by the DMA/QDMA region access registers (DRAEm/DRAEHm, QRAEn/QRAEHn). IEVAL is
needed for robust ISR operations to ensure that interrupts are not missed by the CPU. All DMA/QDMA
channels can be set to assert an EDMA3CC completion interrupt to the CPU on transfer completion, by
appropriately configuring the PaRAM entry associated with the channels. This register is used for the
transfer completion interrupt reporting/generating by the EDMA3CC. For more details on EDMA3CC
completion interrupt generation, see EDMA3 Interrupts.


**Figure 11-158. IEVAL Register**

31               30                29                    28               27                    26           25                24
RESERVED
R-0h
23               22                21                    20               19                    18           17                16
RESERVED
R-0h
15               14                13                    12               11                    10            9                 8
RESERVED
R-0h
7              6                5                     4                 3                    2           1                   0
RESERVED                                                       RESERVED              EVAL
R-0h                                                           R-0h                W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-145. IEVAL Register Field Descriptions**

Bit       Field                      Type             Reset          Description
31-2      RESERVED                   R                0h
1        RESERVED                   R                0h
0        EVAL                       W                0h             Interrupt evaluate.
The EDMA3CC completion interrupt that is pulsed depends on which
IEVAL is being exercised.
For example, writing to the EVAL bit in IEVAL pulses the global
completion interrupt, but writing to the EVAL bit in IEVAL0 pulses the
region 0 completion interrupt.
0h = No effect.
1h = Causes EDMA3CC completion interrupt to be pulsed, if any
enabled (IERn/IERHn = 1) interrupts are still pending (IPRn/IPRHn =
1).


#### 11.4.1.120 QER Register (offset = 1080h) [reset = 0h]

QER is shown in Figure 11-159 and described in Table 11-146.
The QDMA event register (QER) channel n bit is set (En = 1) when the CPU or any EDMA3 programmer
(including EDMA3) performs a write to the trigger word (using the QDMA channel mapping register
(QCHMAPn)) in the PaRAM entry associated with QDMA channel n (which is also programmed using
QCHMAPn). The En bit is also set when the EDMA3CC performs a link update on a PaRAM address that
matches the QCHMAPn settings. The QDMA event is latched only if the QDMA event enable register
(QEER) channel n bit is also enabled (QEER.En = 1). Once a bit is set in QER, then the corresponding
QDMA event (auto-trigger) is evaluated by the EDMA3CC logic for an associated transfer request
submission to the transfer controllers. For additional conditions that can lead to the setting of QER bits,
see EDMA Overview. The setting of an event is a higher priority relative to clear operations (via
hardware). If set and clear conditions occur concurrently, the set condition wins. If the event was
previously set, then the QDMA event missed register (QEMR) would be set because an event is lost. If the
event was previously clear, then the event remains set and is prioritized for submission to the event
queues. The set bits in QER are only cleared when the transfer request associated with the corresponding
channels has been processed by the EDMA3CC and submitted to the transfer controller. If the En bit is
already set and a QDMA event for the same QDMA channel occurs prior to the original being cleared,
then the second missed event is latched in QEMR (En = 1). QER is part of a set of register that control the
QDMA channels in EDMA3CC. QDMA channels (with the exception of the QDMA queue number register)
consist of a set of registers, each of which have a bit location. Each bit position corresponds to a QDMA
channel number. The QDMA channel registers are accessible via read/writes to the global address range.
They are also accessible via read/writes to the shadow address range. The read/write accessibility in the
shadow region address region is controlled by the QDMA region access registers (QRAEn/QRAEHn).


**Figure 11-159. QER Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1   0
RESERVED                                                                                   En
R-0h                                                                                   R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-146. QER Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-8       RESERVED                   R               0h
7-0    En                         R               0h             QDMA event for channels 0 to 7.
0h = No effect.
1h = Corresponding QDMA event is prioritized versus other pending
DMA/QDMA events for submission to the EDMA3TC.


#### 11.4.1.121 QEER Register (offset = 1084h) [reset = 0h]

QEER is shown in Figure 11-160 and described in Table 11-147.
The EDMA3CC provides the option of selectively enabling/disabling each channel in the QDMA event
register (QER) by using the QDMA event enable register (QEER). If any of the event bits in QEER is set
(using the QDMA event enable set register, QEESR), it will enable that corresponding event. Alternatively,
if any event bit in QEER is cleared (using the QDMA event enable clear register, QEECR), it will disable
the corresponding QDMA channel. The QDMA event register will not latch any event for a QDMA channel,
if it is not enabled via QEER. QEER is part of a set of register that control the QDMA channels in
EDMA3CC. QDMA channels (with the exception of the QDMA queue number register) consist of a set of
registers, each of which have a bit location. Each bit position corresponds to a QDMA channel number.
The QDMA channel registers are accessible via read/writes to the global address range. They are also
accessible via read/writes to the shadow address range. The read/write accessibility in the shadow region
address region is controlled by the QDMA region access registers (QRAEn/QRAEHn).


**Figure 11-160. QEER Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1    0
RESERVED                                                                                   En
R-0h                                                                                   R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-147. QEER Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-8     RESERVED                    R               0h
7-0      En                          R               0h             QDMA event enable for channels 0 to 7.
0h = QDMA channel n is not enabled. QDMA event will not be
recognized and will not latch in the QDMA event register (QER).
1h = QDMA channel n is enabled. QDMA events will be recognized
and will get latched in the QDMA event register (QER).


#### 11.4.1.122 QEECR Register (offset = 1088h) [reset = 0h]

QEECR is shown in Figure 11-161 and described in Table 11-148.
The QDMA event enable register (QEER) cannot be modified by directly writing to the register, to ease the
software burden when multiple tasks are attempting to simultaneously modify these registers. The QDMA
event enable clear register (QEECR) is used to disable events. Writes of 1 to the bits in QEECR clear the
corresponding QDMA channel bits in QEER; writes of 0 have no effect. QEECR is part of a set of register
that control the QDMA channels in EDMA3CC. QDMA channels (with the exception of the QDMA queue
number register) consist of a set of registers, each of which have a bit location. Each bit position
corresponds to a QDMA channel number. The QDMA channel registers are accessible via read/writes to
the global address range. They are also accessible via read/writes to the shadow address range. The
read/write accessibility in the shadow region address region is controlled by the QDMA region access
registers (QRAEn/QRAEHn).


**Figure 11-161. QEECR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1   0
RESERVED                                                                                   En
R-0h                                                                                   W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-148. QEECR Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-8       RESERVED                   R               0h
7-0    En                         W               0h             QDMA event enable clear for channels 0 to 7.
0h = No effect.
1h = QDMA event is disabled. Corresponding bit in the QDMA event
enable register (QEER) is cleared (En = 0).


#### 11.4.1.123 QEESR Register (offset = 108Ch) [reset = 0h]

QEESR is shown in Figure 11-162 and described in Table 11-149.
The QDMA event enable register (QEER) cannot be modified by directly writing to the register, to ease the
software burden when multiple tasks are attempting to simultaneously modify these registers. The QDMA
event enable set register (QEESR) is used to enable events. Writes of 1 to the bits in QEESR set the
corresponding QDMA channel bits in QEER; writes of 0 have no effect.


**Figure 11-162. QEESR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1    0
RESERVED                                                                                   En
R-0h                                                                                   W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-149. QEESR Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-8     RESERVED                    R               0h
7-0      En                          W               0h             QDMA event enable set for channels 0 to 7.
0h = No effect.
1h = QDMA event is enabled. Corresponding bit in the QDMA event
enable register (QEER) is set (En = 1).


#### 11.4.1.124 QSER Register (offset = 1090h) [reset = 0h]

QSER is shown in Figure 11-163 and described in Table 11-150.
The QDMA secondary event register (QSER) provides information on the state of a QDMA event. If at any
time a bit corresponding to a QDMA channel is set in QSER, that implies that the corresponding QDMA
event is in the queue. Once a bit corresponding to a QDMA channel is set in QSER, the EDMA3CC does
not prioritize additional events on the same QDMA channel. Depending on the condition that lead to the
setting of the QSER bits, either the EDMA3CC hardware or the software (using QSECR) needs to clear
the QSER bits for the EDMA3CC to evaluate subsequent QDMA events on the channel. Based on
whether the associated TR request is valid, or it is a null or dummy TR, the implications on the state of
QSER and the required user actions to submit another QDMA transfer might be different. For additional
conditions that can cause the secondary event registers (QSER\SER) to be set, see EDMA Overview.
QSER is part of a set of register that control the QDMA channels in EDMA3CC. QDMA channels (with the
exception of the QDMA queue number register) consist of a set of registers, each of which have a bit
location. Each bit position corresponds to a QDMA channel number. The QDMA channel registers are
accessible via read/writes to the global address range. They are also accessible via read/writes to the
shadow address range. The read/write accessibility in the shadow region address region is controlled by
the QDMA region access registers (QRAEn/QRAEHn).


**Figure 11-163. QSER Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1   0
RESERVED                                                                                   En
R-0h                                                                                   R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-150. QSER Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-8       RESERVED                   R               0h
7-0    En                         R               0h             QDMA secondary event register for channels 0 to 7.
0h = QDMA event is not currently stored in the event queue.
1h = QDMA event is currently stored in the event queue. EDMA3CC
will not prioritize additional events.


#### 11.4.1.125 QSECR Register (offset = 1094h) [reset = 0h]

QSECR is shown in Figure 11-164 and described in Table 11-151.
The QDMA secondary event clear register (QSECR) clears the status of the QDMA secondary event
register (QSER) and the QDMA event register (QER). CPU writes of 1 clear the corresponding set bits in
QSER and QER. Writes of 0 have no effect. Note that this differs from the secondary event clear register
(SECR) operation, which only clears the secondary event register (SER) bits and does not affect the event
registers. QESCR is part of a set of register that control the QDMA channels in EDMA3CC. QDMA
channels (with the exception of the QDMA queue number register) consist of a set of registers, each of
which have a bit location. Each bit position corresponds to a QDMA channel number. The QDMA channel
registers are accessible via read/writes to the global address range. They are also accessible via
read/writes to the shadow address range. The read/write accessibility in the shadow region address region
is controlled by the QDMA region access registers (QRAEn/QRAEHn).


**Figure 11-164. QSECR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                      9   8   7   6   5   4   3     2    1   0
RESERVED                                                                                     En
R-0h                                                                                     W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-151. QSECR Register Field Descriptions**

Bit       Field                      Type            Reset          Description
31-8      RESERVED                   R               0h
7-0       En                         W               0h             QDMA secondary event clear register for channels 0 to 7.
0h = No effect.
1h = Corresponding bit in the QDMA secondary event register
(QSER) and the QDMA event register (QER) is cleared (En = 0).


### 11.4.2 EDMA3TC Registers

Table 11-152 lists the memory-mapped registers for the EDMA3TC. All register offset addresses not listed
in Table 11-152 should be considered as reserved locations and the register contents should not be
modified.


**Table 11-152. EDMA3TC Registers**

Offset     Acronym                                     Register Name                                                    Section
0h      PID                                         Peripheral Identification Register                           Section 11.4.1.1
4h      TCCFG                                       EDMA3TC Configuration Register                               Section 11.4.2.2
10h      SYSCONFIG                                   EDMA3TC System Configuration Register                        Section 15.1.3.2
100h     TCSTAT                                      EDMA3TC Channel Status Register                              Section 11.4.2.4
120h     ERRSTAT                                     Error Register                                               Section 11.4.2.5
124h     ERREN                                       Error Enable Register                                        Section 11.4.2.6
128h     ERRCLR                                      Error Clear Register                                         Section 11.4.2.7
12Ch     ERRDET                                      Error Details Register                                       Section 11.4.2.8
130h     ERRCMD                                      Error Interrupt Command Register                             Section 11.4.2.9
140h     RDRATE                                      Read Rate Register                                       Section 11.4.2.10
240h     SAOPT                                       Source Active Options Register                           Section 11.4.2.11
244h     SASRC                                       Source Active Source Address Register                    Section 11.4.2.12
248h     SACNT                                       Source Active Count Register                             Section 11.4.2.13
24Ch     SADST                                       Source Active Destination Address Register               Section 11.4.2.14
250h     SABIDX                                      Source Active Source B-Index Register                    Section 11.4.2.15
254h     SAMPPRXY                                    Source Active Memory Protection Proxy Register           Section 11.4.2.16
258h     SACNTRLD                                    Source Active Count Reload Register                      Section 11.4.2.17


**Table 11-152. EDMA3TC Registers (continued)**

Offset    Acronym                                 Register Name                                            Section
25Ch      SASRCBREF                               Source Active Source Address B-Reference Register    Section 11.4.2.18
260h     SADSTBREF                               Source Active Destination Address B-Reference        Section 11.4.2.19
Register
280h     DFCNTRLD                                Destination FIFO Set Count Reload                    Section 11.4.2.20
284h     DFSRCBREF                               Destination FIFO Set Destination Address B           Section 11.4.2.21
Reference Register
288h     DFDSTBREF                               Destination FIFO Set Destination Address B           Section 11.4.2.22
Reference Register
300h     DFOPT0                                  Destination FIFO Options Register 0                  Section 11.4.2.23
304h     DFSRC0                                  Destination FIFO Source Address Register 0           Section 11.4.2.24
308h     DFCNT0                                  Destination FIFO Count Register 0                    Section 11.4.2.25
30Ch      DFDST0                                  Destination FIFO Destination Address Register 0      Section 11.4.2.26
310h     DFBIDX0                                 Destination FIFO BIDX Register 0                     Section 11.4.2.27
314h     DFMPPRXY0                               Destination FIFO Memory Protection Proxy Register    Section 11.4.2.28
0
340h     DFOPT1                                  Destination FIFO Options Register 1                  Section 11.4.2.29
344h     DFSRC1                                  Destination FIFO Source Address Register 1           Section 11.4.2.30
348h     DFCNT1                                  Destination FIFO Count Register 1                    Section 11.4.2.31
34Ch      DFDST1                                  Destination FIFO Destination Address Register 1      Section 11.4.2.32
350h     DFBIDX1                                 Destination FIFO BIDX Register 1                     Section 11.4.2.33
354h     DFMPPRXY1                               Destination FIFO Memory Protection Proxy Register    Section 11.4.2.34
1
380h     DFOPT2                                  Destination FIFO Options Register 2                  Section 11.4.2.35
384h     DFSRC2                                  Destination FIFO Source Address Register 2           Section 11.4.2.36
388h     DFCNT2                                  Destination FIFO Count Register 2                    Section 11.4.2.37
38Ch      DFDST2                                  Destination FIFO Destination Address Register 2      Section 11.4.2.38
390h     DFBIDX2                                 Destination FIFO BIDX Register 2                     Section 11.4.2.39
394h     DFMPPRXY2                               Destination FIFO Memory Protection Proxy Register    Section 11.4.2.40
2
3C0h      DFOPT3                                  Destination FIFO Options Register 3                  Section 11.4.2.41
3C4h      DFSRC3                                  Destination FIFO Source Address Register 3           Section 11.4.2.42
3C8h      DFCNT3                                  Destination FIFO Count Register 3                    Section 11.4.2.43
3CCh      DFDST3                                  Destination FIFO Destination Address Register 3      Section 11.4.2.44
3D0h      DFBIDX3                                 Destination FIFO BIDX Register 3                     Section 11.4.2.45
3D4h      DFMPPRXY3                               Destination FIFO Memory Protection Proxy Register    Section 11.4.2.46
3


#### 11.4.2.1 PID Register (offset = 0h) [reset = 0h]

PID is shown in Figure 11-165 and described in Table 11-153.
The peripheral identification register (PID) is a constant register that uniquely identifies the EDMA3TC and
specific revision of the EDMA3TC.


**Figure 11-165. PID Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8 7    6       5   4   3   2   1    0
RESERVED                                                                               PID
R-0h                                                                               R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-153. PID Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-16     RESERVED                    R               0h
15-0     PID                         R               0h             Peripheral identifier, value 0 to FFFF FFFFh.
Reset for PID[31] to PID[16] is 4000h.
Reset for PID[15] to PID[0] is 7C00h.


#### 11.4.2.2 TCCFG Register (offset = 4h) [reset = 224h]

TCCFG is shown in Figure 11-166 and described in Table 11-154.


**Figure 11-166. TCCFG Register**

31                 30           29                    28               27                    26          25               24
RESERVED
R-0h
23                 22           21                    20               19                    18          17               16
RESERVED
R-0h
15                 14           13                    12               11                    10           9               8
RESERVED                                                                DREGDEPTH
R-0h                                                                     R-2h
7                6            5                     4               3                      2           1                0
RESERVED                        BUSWIDTH                    RESERVED                           FIFOSIZE
R-0h                            R-2h                        R-0h                               R-4h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-154. TCCFG Register Field Descriptions**

Bit       Field                   Type             Reset          Description
31-10         RESERVED                R                0h
9-8       DREGDEPTH               R                2h             Destination register FIFO depth parameterization.
0h = Reserved.
1h = Reserved.
2h = 4 entry (for TC0, TC1, and TC2)
3h = Reserved.
7-6       RESERVED                R                0h
5-4       BUSWIDTH                R                2h             Bus width parameterization.
0h = Reserved.
1h = Reserved.
2h = 128-bit.
3h = Reserved.
3         RESERVED                R                0h
2-0       FIFOSIZE                R                4h             FIFO size
0h = Reserved.
1h = Reserved.
2h = Reserved.
3h = Reserved.
4h = 512 byte FIFO
5h = Reserved.
6h = Reserved.
7h = Reserved.


#### 11.4.2.3 SYSCONFIG Register (offset = 10h) [reset = 28h]

SYSCONFIG is shown in Figure 15-4 and described in Table 15-7.


**Figure 11-167. SYSCONFIG Register**

31               30                29                    28               27                    26            25                 24
RESERVED
R-0h
23               22                21                    20               19                    18            17                 16
RESERVED
R-0h
15               14                13                    12               11                    10             9                  8
RESERVED
R-0h
7                6                5           4                           3                    2              1                  0
RESERVED                       STANDBYMODE                                IDLEMODE                            RESERVED
R-0h                            R/W-2h                                    R/W-2h                              R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-155. SYSCONFIG Register Field Descriptions**

Bit      Field                       Type             Reset          Description
31-6     RESERVED                    R                0h
5-4      STANDBYMODE                 R/W              2h             Configuration of the local initiator state management mode.
By definition, initiator may generate read/write transaction as long as
it is out of STANDBY state.
0h = Force-standby mode: local initiator is unconditionally placed in
standby state. Backup mode, for debug only.
1h = No-standby mode: local initiator is unconditionally placed out of
standby state. Backup mode, for debug only.
2h = Smart-standby mode: local initiator standby status depends on
local conditions, i.e., the module's functional requirement from the
initiator. IP module should not generate (initiator-related) wakeup
events.
3h = Reserved.
3-2      IDLEMODE                    R/W              2h             Configuration of the local target state management mode.
By definition, target can handle read/write transaction as long as it is
out of IDLE state.
0h = Force-idle mode: local target's idle state follows (acknowledges)
the system's idle requests unconditionally, i.e. regardless of the IP
module's internal requirements. Backup mode, for debug only.
1h = No-idle mode: local target never enters idle state. Backup
mode, for debug only.
2h = Smart-idle mode: local target's idle state eventually follows
(acknowledges) the system's idle requests, depending on the IP
module's internal requirements. IP module shall not generate (IRQ or
DMA-request-related) wakeup events.
3h = Reserved.
1-0      RESERVED                    R                0h


#### 11.4.2.4 TCSTAT Register (offset = 100h) [reset = 100h]

TCSTAT is shown in Figure 11-168 and described in Table 11-156.


**Figure 11-168. TCSTAT Register**

31              30              29                    28               27                    26            25                 24
RESERVED
R-0h
23              22              21                    20               19                    18            17                 16
RESERVED
R-0h
15              14              13                    12               11               10                  9                8
RESERVED                        DFSTRTPTR                                       RESERVED                            RESERVED
R-0h                             R-0h                                           R-0h                                R-1h
7                   6             5                      4               3                 2                 1                 0
RESERVED                           DSTACTV                               RESERVED            WSACTV           SRCACTV           PROGBUSY
R-0h                               R-0h                                  R-0h               R-0h              R-0h              R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-156. TCSTAT Register Field Descriptions**

Bit      Field                    Type             Reset          Description
31-14        RESERVED                 R                0h
13-12        DFSTRTPTR                R                0h             Destination FIFO start pointer.
Represents the offset to the head entry of the destination register
FIFO, in units of entries.
Value 0 to 3h.
11-9         RESERVED                 R                0h
8        RESERVED                 R                1h
7        RESERVED                 R                0h
6-4      DSTACTV                  R                0h             Destination active state.
Specifies the number of transfer requests (TRs) that are resident in
the destination register FIFO at a given instant.
This bit field can be primarily used for advanced debugging.
Legal values are constrained by the destination register FIFO depth
parameterization (DSTREGDEPTH) parameter.
0h = Destination FIFO is empty.
1h = Destination FIFO contains 1 TR.
2h = Destination FIFO contains 2 TRs.
3h = Destination FIFO contains 3 TRs.
4h = Destination FIFO contains 4 TRs. (Full if DSTREGDEPTH==4).
If the destination register FIFO is empty, then any TR written to Prog
Set immediately transitions to the destination register FIFO. If the
destination register FIFO is not empty and not full, then any TR
written to Prog Set immediately transitions to the destination register
FIFO set if the source active state (SRCACTV) bit is set to idle. If the
destination register FIFO is full, then TRs cannot transition to the
destination register FIFO. The destination register FIFO becomes not
full when the TR at the head of the destination register FIFO is
completed.
5h = Reserved.
6h = Reserved.
7h = Reserved.
3        RESERVED                 R                0h
2        WSACTV                   R                0h             Write status active
0h = Write status is not pending. Write status has been received for
all previously issued write commands.
1h = Write status is pending. Write status has not been received for
all previously issued write commands.


**Table 11-156. TCSTAT Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
1       SRCACTV                 R               0h             Source active state
0h = Source controller is idle. Source active register set contains a
previously processed transfer request.
1h = Source controller is busy servicing a transfer request.
0       PROGBUSY                R               0h             Program register set busy
0h = Program set idle and is available for programming by the
EDMA3CC.
1h = Program set busy


#### 11.4.2.5 ERRSTAT Register (offset = 120h) [reset = 0h]

ERRSTAT is shown in Figure 11-169 and described in Table 11-157.


**Figure 11-169. ERRSTAT Register**

31              30              29                    28               27                    26            25                24
RESERVED
R-0h
23              22              21                    20               19                    18            17                16
RESERVED
R-0h
15              14              13                    12               11                    10            9                    8
RESERVED
R-0h
7             6               5                     4              3                   2                1                 0
RESERVED                                     MMRAERR              TRERR          RESERVED           BUSERR
R-0h                                         R-0h                R-0h            R-0h              R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-157. ERRSTAT Register Field Descriptions**

Bit      Field                    Type             Reset          Description
31-4         RESERVED                 R                0h
3        MMRAERR                  R                0h             MMR address error.
0h = Condition is not detected.
1h = User attempted to read or write to an invalid address in
configuration memory map.
2        TRERR                    R                0h             Transfer request (TR) error event.
0h = Condition is not detected.
1h = TR detected that violates constant addressing mode transfer
(SAM or DAM is set) alignment rules or has ACNT or BCNT == 0.
1        RESERVED                 R                0h
0        BUSERR                   R                0h             Bus error event.
0h = Condition is not detected.
1h = EDMA3TC has detected an error at source or destination
address. Error information can be read from the error details register
(ERRDET).


#### 11.4.2.6 ERREN Register (offset = 124h) [reset = 0h]

ERREN is shown in Figure 11-170 and described in Table 11-158.
When any of the enable bits are set, a bit set in the corresponding ERRSTAT causes an assertion of the
EDMA3TC interrupt.


**Figure 11-170. ERREN Register**

31               30                29                    28               27                    26         25                24
RESERVED
R-0h
23               22                21                    20               19                    18         17                16
RESERVED
R-0h
15               14                13                    12               11                    10         9                 8
RESERVED
R-0h
7              6                5                     4              3                   2              1               0
RESERVED                                       MMRAERR              TRERR        RESERVED         BUSERR
R-0h                                          R/W-0h              R/W-0h         R-0h            R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-158. ERREN Register Field Descriptions**

Bit       Field                      Type             Reset          Description
31-4      RESERVED                   R                0h
3        MMRAERR                    R/W              0h             Interrupt enable for MMR address error (MMRAERR).
0h = MMRAERR is disabled.
1h = MMRAERR is enabled and contributes to the state of
EDMA3TC error interrupt generation
2        TRERR                      R/W              0h             Interrupt enable for transfer request error (TRERR).
0h = TRERR is disabled.
1h = TRERR is enabled and contributes to the state of EDMA3TC
error interrupt generation.
1        RESERVED                   R                0h
0        BUSERR                     R/W              0h             Interrupt enable for bus error (BUSERR).
0h = BUSERR is disabled.
1h = BUSERR is enabled and contributes to the state of EDMA3TC
error interrupt generation.


#### 11.4.2.7 ERRCLR Register (offset = 128h) [reset = 0h]

ERRCLR is shown in Figure 11-171 and described in Table 11-159.


**Figure 11-171. ERRCLR Register**

31              30              29                    28               27                    26           25                24
RESERVED
R-0h
23              22              21                    20               19                    18           17                16
RESERVED
R-0h
15              14              13                    12               11                    10            9                 8
RESERVED
R-0h
7             6               5                     4              3                   2               1                 0
RESERVED                                     MMRAERR              TRERR         RESERVED           BUSERR
R-0h                                         W-0h                W-0h           R-0h              W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-159. ERRCLR Register Field Descriptions**

Bit      Field                    Type             Reset          Description
31-4         RESERVED                 R                0h
3        MMRAERR                  W                0h             Interrupt enable clear for the MMRAERR bit in the error status
register (ERRSTAT).
0h = No effect.
1h = Clears the MMRAERR bit in ERRSTAT but does not clear the
error details register (ERRDET).
2        TRERR                    W                0h             Interrupt enable clear for the TRERR bit in the error status register
(ERRSTAT).
0h = No effect.
1h = Clears the TRERR bit in ERRSTAT but does not clear the error
details register (ERRDET).
1        RESERVED                 R                0h
0        BUSERR                   W                0h             Interrupt clear for the BUSERR bit in the error status register
(ERRSTAT).
0h = No effect.
1h = Clears the BUSERR bit in ERRSTAT and clears the error
details register (ERRDET).


#### 11.4.2.8 ERRDET Register (offset = 12Ch) [reset = 0h]

ERRDET is shown in Figure 11-172 and described in Table 11-160.


**Figure 11-172. ERRDET Register**

31                30               29                    28               27                    26             25                 24
RESERVED
R-0h
23                22               21                    20               19                    18             17                16
RESERVED                                                           TCCHEN            TCINTEN
R-0h                                                               R-0h              R-0h
15                14               13                    12               11                    10              9                  8
RESERVED                                                                    TCC
R-0h                                                                      R-0h
7                6                5                     4                 3                    2               1                  0
RESERVED                                                                      STAT
R-0h                                                                        R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-160. ERRDET Register Field Descriptions**

Bit       Field                      Type             Reset          Description
31-18      RESERVED                   R                0h
17       TCCHEN                     R                0h             Transfer completion chaining enable.
Contains the TCCHEN value in the channel options parameter
(OPT) programmed by the channel controller for the read or write
transaction that resulted in an error.
16       TCINTEN                    R                0h             Transfer completion interrupt enable.
Contains the TCINTEN value in the channel options parameter
(OPT) programmed by the channel controller for the read or write
transaction that resulted in an error.
15-14      RESERVED                   R                0h
13-8      TCC                        R                0h             Transfer complete code.
Contains the TCC value in the channel options parameter (OPT)
programmed by the channel controller for the read or write
transaction that resulted in an error.
7-4       RESERVED                   R                0h
3-0       STAT                       R                0h             Transaction status.
Stores the nonzero status/error code that was detected on the read
status or write status bus.
If read status and write status are returned on the same cycle, then
the EDMA3TC chooses nonzero version.
If both are nonzero, then the write status is treated as higher priority.
0h = No error.
1h = From 1h to 7h, Read error.
8h = From 8h to Fh, Write error.


#### 11.4.2.9 ERRCMD Register (offset = 130h) [reset = 0h]

ERRCMD is shown in Figure 11-173 and described in Table 11-161.


**Figure 11-173. ERRCMD Register**

31              30              29                    28               27                    26           25                24
RESERVED
R-0h
23              22              21                    20               19                    18           17                16
RESERVED
R-0h
15              14              13                    12               11                    10            9                 8
RESERVED
R-0h
7             6               5                     4                 3                    2           1                   0
RESERVED                                                       RESERVED              EVAL
R-0h                                                           R-0h                W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-161. ERRCMD Register Field Descriptions**

Bit      Field                    Type             Reset          Description
31-2         RESERVED                 R                0h
1        RESERVED                 R                0h
0        EVAL                     W                0h             Error evaluate
0h = No effect
1h = EDMA3TC error line is pulsed if any of the error status register
(ERRSTAT) bits are set.


#### 11.4.2.10 RDRATE Register (offset = 140h) [reset = 0h]

RDRATE is shown in Figure 11-174 and described in Table 11-162.
The EDMA3 transfer controller issues read commands at a rate controlled by the read rate register
(RDRATE). The RDRATE defines the number of idle cycles that the read controller must wait before
issuing subsequent commands. This applies both to commands within a transfer request packet (TRP)
and for commands that are issued for different transfer requests (TRs). For instance, if RDRATE is set to
4 cycles between reads, there are 3 inactive cycles between reads. RDRATE allows flexibility in transfer
controller access requests to an endpoint. For an application, RDRATE can be manipulated to slow down
the access rate, so that the endpoint may service requests from other masters during the inactive
EDMA3TC cycles. Note: The RDRATE value for a transfer controller is expected to be static, as it is
decided based on the application requirement. It is not recommended to change the RDRATE value on
the fly.


**Figure 11-174. RDRATE Register**

31       30           29   28     27         26        25       24      23         22       21      20       19       18       17       16
RESERVED
R-0h
15       14           13   12     11         10       9    8              7        6         5       4        3        2      1             0
RESERVED                                                                  RDRATE
R-0h                                                                    R/W-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-162. RDRATE Register Field Descriptions**

Bit         Field                     Type            Reset          Description
31-3        RESERVED                  R               0h
2-0         RDRATE                    R/W             0h             Read rate.
Controls the number of cycles between read commands.
This is a global setting that applies to all TRs for this EDMA3TC.
0h = Reads issued as fast as possible.
1h = 4 cycles between reads.
2h = 8 cycles between reads.
3h = 16 cycles between reads.
4h = 32 cycles between reads.
5h = Reserved.
6h = Reserved.
7h = Reserved.


#### 11.4.2.11 SAOPT Register (offset = 240h) [reset = 0h]

SAOPT is shown in Figure 11-175 and described in Table 11-163.
The Source Active Options Register (SAOPT) is an EDMA3TC channel register. This EDMA3TC channel
register is part of the Source Active Register Set. It is read-only and provided to facilitate debugging by
providing a window into how the transfer controller (TC) was programmed by the channel controller (CC),
as well as showing the status of the transfer controller (TC) during a transfer.


**Figure 11-175. SAOPT Register**

31                30               29                    28               27                    26            25                 24
RESERVED
R-0h
23                     22              21                 20                  19                    18            17                 16
RESERVED               TCCHEN          RESERVED           TCINTEN                     RESERVED                               TCC
R-0h                   R-0h            R-0h               R-0h                        R-0h                                 R-0h
15                14               13                    12             11                      10           9                   8
TCC                                         RESERVED                               FWID
R-0h                                          R-0h                                 R-0h
7                    6                5                     4                 3                    2             1                 0
RESERVED                                 PRI                                          RESERVED                      DAM               SAM
R-0h                                   R-0h                                           R-0h                        R-0h              R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-163. SAOPT Register Field Descriptions**

Bit       Field                      Type             Reset          Description
31-23         RESERVED                   R                0h
22        TCCHEN                     R                0h             Transfer complete chaining enable
0h = Transfer complete chaining is disabled.
1h = Transfer complete chaining is enabled.
21        RESERVED                   R                0h
20        TCINTEN                    R                0h             Transfer complete interrupt enable.
0h = Transfer complete interrupt is disabled.
1h = Transfer complete interrupt is enabled.
19-18         RESERVED                   R                0h
17-12         TCC                        R                0h             Transfer complete code.
This 6 bit code is used to set the relevant bit in CER or IPR of the
EDMA3PCC module.
11        RESERVED                   R                0h
10-8          FWID                       R                0h             FIFO width.
Applies if either SAM or DAM is set to constant addressing mode.
0h = FIFO width is 8-bit.
1h = FIFO width is 16-bit.
2h = FIFO width is 32-bit.
3h = FIFO width is 64-bit.
4h = FIFO width is 128-bit.
5h = FIFO width is 256-bit.
6h = Reserved.
7h = Reserved.
7         RESERVED                   R                0h


**Table 11-163. SAOPT Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
6-4      PRI                     R               0h             Transfer priority.
Reflects the values programmed in the QUEPRI register in the
EDMACC.
0h = Priority 0 - Highest priority
1h = From 1h to 6h, Priority 1 to priority 6
7h = Priority 7 - Lowest priority
3-2      RESERVED                R               0h
1       DAM                     R               0h             Destination address mode within an array
0h = Increment (INCR) mode. Destination addressing within an array
increments.
1h = Constant addressing (CONST) mode. Destination addressing
within an array wraps around upon reaching FIFO width.
0       SAM                     R               0h             Source address mode within an array
0h = Increment (INCR) mode. Source addressing within an array
increments.
1h = Constant addressing (CONST) mode. Source addressing within
an array wraps around upon reaching FIFO width.


#### 11.4.2.12 SASRC Register (offset = 244h) [reset = 0h]

SASRC is shown in Figure 11-176 and described in Table 11-164.
The Source Active Source Address Register (SASRC) is an EDMA3TC channel register. This EDMA3TC
channel register is part of the Source Active Register Set. It is read-only and provided to facilitate
debugging by providing a window into how the transfer controller (TC) was programmed by the channel
controller (CC), as well as showing the status of the transfer controller (TC) during a transfer.


**Figure 11-176. SASRC Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1   0
SADDR
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-164. SASRC Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       SADDR                      R               0h             Source address for Source Active Register Set.
EDMA3TC updates value according to source addressing mode
(SAM bit in the source active options register, SAOPT).
This register does not update during a transfer.
Value 0 to FFFFh.


#### 11.4.2.13 SACNT Register (offset = 248h) [reset = 0h]

SACNT is shown in Figure 11-177 and described in Table 11-165.
The Source Active Count Register (SACNT) is an EDMA3TC channel register. This EDMA3TC channel
register is part of the Source Active Register Set. It is read-only and provided to facilitate debugging by
providing a window into how the transfer controller (TC) was programmed by the channel controller (CC),
as well as showing the status of the transfer controller (TC) during a transfer.


**Figure 11-177. SACNT Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8 7    6   5   4   3   2   1      0
BCNT                                                                                ACNT
R-0h                                                                               R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-165. SACNT Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-16     BCNT                        R               0h             B dimension count remaining for the Source Active Register Set.
Number of arrays to be transferred, where each array is ACNT in
length.
TC decrements BCNT as necessary after each read command is
issued.
The final value should be 0 when TR is complete.
Note that BCNT is always read as 0x0 in the Source Address
Register Set.
15-0     ACNT                        R               0h             A dimension count remaining for the Source Active Register Set.
Number of bytes to be transferred in first dimension.
Represents the amount of data remaining to be read.
TC decrements ACNT as necessary after each read command is
issued.
The final value should be 0 when TR is complete.


#### 11.4.2.14 SADST Register (offset = 24Ch) [reset = 0h]

SADST is shown in Figure 11-178 and described in Table 11-166.
The Source Active Destination Address Register (SADST) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Source Active Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing the status of the transfer controller (TC) during a transfer.
Note: Destination address is not applicable for Source Active Register Set. Read returns 0.


**Figure 11-178. SADST Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1   0
RESERVED
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-166. SADST Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       RESERVED                   R               0h


#### 11.4.2.15 SABIDX Register (offset = 250h) [reset = 0h]

SABIDX is shown in Figure 11-179 and described in Table 11-167.
The Source Active Source B-Dimension Index Register (SABIDX) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Source Active Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing the status of the transfer controller (TC) during a transfer.


**Figure 11-179. SABIDX Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8 7     6   5   4   3   2   1    0
DBIDX                                                                               SBIDX
R-0h                                                                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-167. SABIDX Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-16     DBIDX                       R               0h             B-Index offset between destination arrays.
Represents the offset in bytes between the starting address of each
destination array (there are BCND arrays of ACNT elements).
DBIDX is always used regardless of whether DAM is in Increment or
FIFO mode.
15-0     SBIDX                       R               0h             B-Index offset between source arrays.
Represents the offset in bytes between the starting address of each
source array (there are BCNT arrays of ACNT elements).
SBIDX is always used regardless of whether SAM is in Increment or
FIFO mode.
Value 0 to FFFFh.


#### 11.4.2.16 SAMPPRXY Register (offset = 254h) [reset = 0h]

SAMPPRXY is shown in Figure 11-180 and described in Table 11-168.
The Source Active Memory Protection Proxy Register (SAMPPRXY) is an EDMA3TC channel register.
This EDMA3TC channel register is part of the Source Active Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing the status of the transfer controller (TC) during a transfer.


**Figure 11-180. SAMPPRXY Register**

31              30              29                    28               27                    26            25              24
RESERVED
R-0h
23              22              21                    20               19                    18            17              16
RESERVED
R-0h
15              14              13                12                   11                    10            9               8
RESERVED                                                                   PRIV
R-0h                                                                     R-0h
7             6               5                     4                 3                    2             1                0
RESERVED                                                                     PRIVID
R-0h                                                                        R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-168. SAMPPRXY Register Field Descriptions**

Bit      Field                    Type             Reset          Description
31-9         RESERVED                 R                0h
8        PRIV                     R                0h             Privilege level.
The privilege level used by the host to set up the parameter entry in
the channel controller.
This field is set up when the associated TR is submitted to the
EDMA3TC.
The privilege ID is used while issuing read and write command to the
target endpoints so that the target endpoints can perform memory
protection checks based on the PRIV of the host that set up the
DMA transaction.
0h = User-level privilege.
1h = Supervisor-level privilege.
7-4      RESERVED                 R                0h
3-0      PRIVID                   R                0h             Privilege ID.
This contains the privilege ID of the host that set up the parameter
entry in the channel controller.
This field is set up when the associated TR is submitted to the
EDMA3TC.
This PRIVID value is used while issuing read and write commands to
the target endpoints so that the target endpoints can perform
memory protection checks based on the PRIVID of the host that set
up the DMA transaction.
Value 0 to Fh.


#### 11.4.2.17 SACNTRLD Register (offset = 258h) [reset = 0h]

SACNTRLD is shown in Figure 11-181 and described in Table 11-169.
The Source Active Count Reload Register (SACNTRLD) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Source Active Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing the status of the transfer controller (TC) during a transfer.


**Figure 11-181. SACNTRLD Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9     8 7 6    5   4    3   2   1       0
RESERVED                                                                              ACNTRLD
R-0h                                                                                 R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-169. SACNTRLD Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-16     RESERVED                    R               0h
15-0     ACNTRLD                     R               0h             A-count reload value.
Represents the originally programmed value of ACNT.
The reload value is used to reinitialize ACNT after each array is
serviced (that is, ACNT decrements to 0) by the source offset in
bytes between the starting address or each source array (there are
BCNT arrays of ACNT bytes).
Value 0 to FFFFh.


#### 11.4.2.18 SASRCBREF Register (offset = 25Ch) [reset = 0h]

SASRCBREF is shown in Figure 11-182 and described in Table 11-170.
The Source Active Source Address B-Reference Register (SASRCBREF) is an EDMA3TC channel
register. This EDMA3TC channel register is part of the Source Active Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing the status of the transfer controller (TC)
during a transfer.


**Figure 11-182. SASRCBREF Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5    4   3    2   1      0
SADDRBREF
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-170. SASRCBREF Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       SADDRBREF                  R               0h             Source address B-reference.
Represents the starting address for the array currently being read.
The next array's starting address is calculated as the reference
address plus the source b-idx value.
Value 0 to FFFF FFFFh.


#### 11.4.2.19 SADSTBREF Register (offset = 260h) [reset = 0h]

SADSTBREF is shown in Figure 11-183 and described in Table 11-171.
The Source Active Destination Address B-Reference Register (SADSTBREF) is an EDMA3TC channel
register. This EDMA3TC channel register is part of the Source Active Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing the status of the transfer controller (TC)
during a transfer.
Note: Destination address is not applicable for Source Active Register Set. Read returns 0.


**Figure 11-183. SADSTBREF Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1    0
RESERVED
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-171. SADSTBREF Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     RESERVED                    R               0h


#### 11.4.2.20 DFCNTRLD Register (offset = 280h) [reset = 0h]

DFCNTRLD is shown in Figure 11-184 and described in Table 11-172.
The Destination FIFO Count Reload Register (DFCNTRLD) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.


**Figure 11-184. DFCNTRLD Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9     8 7 6    5   4    3   2   1       0
RESERVED                                                                              ACNTRLD
R-0h                                                                                 R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-172. DFCNTRLD Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-16      RESERVED                   R               0h
15-0       ACNTRLD                    R               0h             A-count reload value for the Destination FIFO Register Set.
Represents the originally programmed value of ACNT.
The reload value is used to reinitialize ACNT after each array is
serviced (that is, ACNT decrements to 0) by the source offset in
bytes between the starting address of each source array (there are
BCNT arrays of ACNT bytes).
Value 0 to FFFFh.


#### 11.4.2.21 DFSRCBREF Register (offset = 284h) [reset = 0h]

DFSRCBREF is shown in Figure 11-185 and described in Table 11-173.
The Destination FIFO Source Address B-Reference Register (DFSRCBREF) is an EDMA3TC channel
register. This EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-
only and provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.
Note: Source address reference is not applicable for Destination FIFO Register Set. Read returns 0.


**Figure 11-185. DFSRCBREF Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1    0
RESERVED
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-173. DFSRCBREF Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     RESERVED                    R               0h


#### 11.4.2.22 DFDSTBREF Register (offset = 288h) [reset = 0h]

DFDSTBREF is shown in Figure 11-186 and described in Table 11-174.
The Destination FIFO Destination Address B-Reference Register (DFDSTBREF) is an EDMA3TC channel
register. This EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-
only and provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.


**Figure 11-186. DFDSTBREF Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6    5   4    3   2    1   0
DADDRBREF
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-174. DFDSTBREF Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       DADDRBREF                  R               0h             Destination address reference for the destination FIFO register set.
Represents the starting address for the array currently being written.
The next array's starting address is calculated as the reference
address plus the destination B-Index value.
Value 0 to FFFF FFFFh.


#### 11.4.2.23 DFOPT0 Register (offset = 300h) [reset = 0h]

DFOPT0 is shown in Figure 11-187 and described in Table 11-175.
The Destination FIFO Options Register (DFOPT) is an EDMA3TC channel register. This EDMA3TC
channel register is part of the Destination Register FIFO Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The
number of destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2
each have a destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of
these transfer controllers.


**Figure 11-187. DFOPT0 Register**

31                 30               29                    28               27                    26            25                 24
RESERVED
R-0h
23                   22              21                 20                  19                    18            17                 16
RESERVED             TCCHEN          RESERVED           TCINTEN                     RESERVED                               TCC
R-0h                 R-0h            R-0h               R-0h                        R-0h                                 R-0h
15                 14               13                    12             11                      10           9                   8
TCC                                         RESERVED                               FWID
R-0h                                          R-0h                                 R-0h
7                  6                5                     4                 3                    2             1                 0
RESERVED                               PRI                                          RESERVED                      DAM               SAM
R-0h                                 R-0h                                           R-0h                        R-0h              R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-175. DFOPT0 Register Field Descriptions**

Bit        Field                      Type             Reset          Description
31-23       RESERVED                   R                0h
22        TCCHEN                     R                0h             Transfer complete chaining enable
0h = Transfer complete chaining is disabled
1h = Transfer complete chaining is enabled
21        RESERVED                   R                0h
20        TCINTEN                    R                0h             Transfer complete interrupt enable.
0h = Transfer complete interrupt is disabled.
1h = Transfer complete interrupt is enabled.
19-18       RESERVED                   R                0h
17-12       TCC                        R                0h             Transfer complete code.
This 6 bit code is used to set the relevant bit in CER or IPR of the
EDMA3PCC module.
11        RESERVED                   R                0h
10-8       FWID                       R                0h             FIFO width.
Applies if either SAM or DAM is set to constant addressing mode.
0h = FIFO width is 8-bit.
1h = FIFO width is 16-bit.
2h = FIFO width is 32-bit.
3h = FIFO width is 64-bit.
4h = FIFO width is 128-bit.
5h = FIFO width is 256-bit.
6h = Reserved.
7h = Reserved.
7         RESERVED                   R                0h


**Table 11-175. DFOPT0 Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
6-4      PRI                     R               0h             Transfer priority
0h = Priority 0 - Highest priority
1h = From 1h to 6h, Priority 1 to priority 6
7h = Priority 7 - Lowest priority
3-2      RESERVED                R               0h
1        DAM                     R               0h             Destination address mode within an array
0h = Increment (INCR) mode. Destination addressing within an array
increments.
1h = Constant addressing (CONST) mode. Destination addressing
within an array wraps around upon reaching FIFO width.
0        SAM                     R               0h             Source address mode within an array
0h = Increment (INCR) mode. Source addressing within an array
increments.
1h = Constant addressing (CONST) mode. Source addressing within
an array wraps around upon reaching FIFO width.


#### 11.4.2.24 DFSRC0 Register (offset = 304h) [reset = 0h]

DFSRC0 is shown in Figure 11-188 and described in Table 11-176.
The Destination FIFO Source Address Register (DFSRC) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.
Note: Source address reference is not applicable for Destination FIFO Register Set. Read returns 0.


**Figure 11-188. DFSRC0 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1    0
RESERVED
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-176. DFSRC0 Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     RESERVED                    R               0h


#### 11.4.2.25 DFCNT0 Register (offset = 308h) [reset = 0h]

DFCNT0 is shown in Figure 11-189 and described in Table 11-177.
The Destination FIFO Count Register (DFCNT) is an EDMA3TC channel register. This EDMA3TC channel
register is part of the Destination Register FIFO Register Set. It is read-only and provided to facilitate
debugging by providing a window into how the transfer controller (TC) was programmed by the channel
controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The number of
destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2 each have a
destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of these transfer
controllers.


**Figure 11-189. DFCNT0 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8 7    6   5   4   3   2   1   0
BCNT                                                                                ACNT
R-0h                                                                               R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-177. DFCNT0 Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-16      BCNT                       R               0h             B-dimension count remaining for Destination Register Set.
Represents the amount of data remaining to be written.
For the final TR in the Destination Register FIFO: TC decrements
ACNT and BCNT as necessary after each write dataphase is issued.
The final value should be 0 when TR is complete.
For a non-final TR in the Destination Register FIFO: CNT will hold
the originally programmed value or the optimized BCNT value after
2D optimization calculation.
15-0       ACNT                       R               0h             A-dimension count remaining for Destination Register Set.
Represents the amount of data remaining to be written.
For the final TR in the Destination Register FIFO: TC decrements
ACNT and BCNT as necessary after each write dataphase is issued.
The final value should be 0 when TR is complete.
For a non-final TR in the Destination Register FIFO: CNT will hold
the originally programmed value or the optimized BCNT value after
2D optimization calculation.
Value 0 to FFFFh.


#### 11.4.2.26 DFDST0 Register (offset = 30Ch) [reset = 0h]

DFDST0 is shown in Figure 11-190 and described in Table 11-178.
The Destination FIFO Destination Address Register (DFDST) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.


**Figure 11-190. DFDST0 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1    0
DADDR
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-178. DFDST0 Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     DADDR                       R               0h             Destination address for the destination FIFO register set.
TC updates the value according to both the destination addressing
mode (OPT.SAM) or destination index value (BIDX.DBIDX) after
each write command is issued.
When a TR is complete, the final value should be the address of the
last write command issued.
Note: If DAM == CONST, the 'active' address will increment
internally as if the transfer were an 'Increment' transfer.
The address issued on the write command interface will correctly
issue the same address programmed by the user.


#### 11.4.2.27 DFBIDX0 Register (offset = 310h) [reset = 0h]

DFBIDX0 is shown in Figure 11-191 and described in Table 11-179.
The Destination FIFO B-Index Register (DFBIDX) is an EDMA3TC channel register. This EDMA3TC
channel register is part of the Destination Register FIFO Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The
number of destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2
each have a destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of
these transfer controllers.


**Figure 11-191. DFBIDX0 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8 7     6   5   4   3   2   1    0
DBIDX                                                                               SBIDX
R-0h                                                                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-179. DFBIDX0 Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-16      DBIDX                      R               0h             B-Index offset between destination arrays for the Destination FIFO
Register Set.
Represents the offset in bytes between the starting address of each
destination array (there are BCNT arrays of ACNT elements).
DBIDX is always used regardless of whether DAM is in Increment or
FIFO mode.
Value 0 to FFFFh.
15-0       SBIDX                      R               0h             B-Index offset between source arrays for the Destination FIFO
Register Set.
Represents the offset in bytes between the starting address of each
source array (there are BCNT arrays of ACNT elements).
SBIDX is always used regardless of whether SAM is in Increment or
FIFO mode.


#### 11.4.2.28 DFMPPRXY0 Register (offset = 314h) [reset = 0h]

DFMPPRXY0 is shown in Figure 11-192 and described in Table 11-180.
The Destination FIFO Memory Protection Proxy Register (DFMPPRXY) is an EDMA3TC channel register.
This EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.


**Figure 11-192. DFMPPRXY0 Register**

31               30                29                    28               27                    26            25             24
RESERVED
R-0h
23               22                21                    20               19                    18            17             16
RESERVED
R-0h
15               14                13                12                   11                    10            9              8
RESERVED                                                                  PRIV
R-0h                                                                    R-0h
7              6                5                     4                 3                    2             1               0
RESERVED                                                                       PRIVID
R-0h                                                                          R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-180. DFMPPRXY0 Register Field Descriptions**

Bit       Field                      Type             Reset          Description
31-9      RESERVED                   R                0h
8        PRIV                       R                0h             Privilege level.
This contains the Privilege level used by the EDMA3 programmer to
set up the parameter entry in the channel controller.
This field is set up when the associated TR is submitted to the
EDMA3TC.
The privilege ID is used while issuing read and write command to the
target endpoints so that the target endpoints can perform memory
protection checks based on the PRIV of the host that set up the
DMA transaction.
0h = User-level privilege
1h = Supervisor-level privilege
7-4       RESERVED                   R                0h
3-0       PRIVID                     R                0h             Privilege ID.
This contains the Privilege ID of the EDMA3 programmer that set up
the parameter entry in the channel controller.
This field is set up when the associated TR is submitted to the
EDMA3TC.
This PRIVID value is used while issuing read and write commands to
the target endpoints so that the target endpoints can perform
memory protection checks based on the PRIVID of the host that set
up the DMA transaction.
Value 0 to Fh.


#### 11.4.2.29 DFOPT1 Register (offset = 340h) [reset = 0h]

DFOPT1 is shown in Figure 11-193 and described in Table 11-181.
The Destination FIFO Options Register (DFOPT) is an EDMA3TC channel register. This EDMA3TC
channel register is part of the Destination Register FIFO Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The
number of destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2
each have a destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of
these transfer controllers.


**Figure 11-193. DFOPT1 Register**

31                30               29                    28               27                    26            25                 24
RESERVED
R-0h
23                     22              21                 20                  19                    18            17                 16
RESERVED               TCCHEN          RESERVED           TCINTEN                     RESERVED                               TCC
R-0h                   R-0h            R-0h               R-0h                        R-0h                                 R-0h
15                14               13                    12             11                      10           9                   8
TCC                                         RESERVED                               FWID
R-0h                                          R-0h                                 R-0h
7                    6                5                     4                 3                    2             1                 0
RESERVED                                 PRI                                          RESERVED                      DAM               SAM
R-0h                                   R-0h                                           R-0h                        R-0h              R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-181. DFOPT1 Register Field Descriptions**

Bit       Field                      Type             Reset          Description
31-23         RESERVED                   R                0h
22        TCCHEN                     R                0h             Transfer complete chaining enable
0h = Transfer complete chaining is disabled
1h = Transfer complete chaining is enabled
21        RESERVED                   R                0h
20        TCINTEN                    R                0h             Transfer complete interrupt enable.
0h = Transfer complete interrupt is disabled.
1h = Transfer complete interrupt is enabled.
19-18         RESERVED                   R                0h
17-12         TCC                        R                0h             Transfer complete code.
This 6 bit code is used to set the relevant bit in CER or IPR of the
EDMA3PCC module.
11        RESERVED                   R                0h
10-8          FWID                       R                0h             FIFO width.
Applies if either SAM or DAM is set to constant addressing mode.
0h = FIFO width is 8-bit.
1h = FIFO width is 16-bit.
2h = FIFO width is 32-bit.
3h = FIFO width is 64-bit.
4h = FIFO width is 128-bit.
5h = FIFO width is 256-bit.
6h = Reserved.
7h = Reserved.
7         RESERVED                   R                0h


**Table 11-181. DFOPT1 Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
6-4      PRI                     R               0h             Transfer priority
0h = Priority 0 - Highest priority
1h = From 1h to 6h, Priority 1 to priority 6
7h = Priority 7 - Lowest priority
3-2      RESERVED                R               0h
1       DAM                     R               0h             Destination address mode within an array
0h = Increment (INCR) mode. Destination addressing within an array
increments.
1h = Constant addressing (CONST) mode. Destination addressing
within an array wraps around upon reaching FIFO width.
0       SAM                     R               0h             Source address mode within an array
0h = Increment (INCR) mode. Source addressing within an array
increments.
1h = Constant addressing (CONST) mode. Source addressing within
an array wraps around upon reaching FIFO width.


#### 11.4.2.30 DFSRC1 Register (offset = 344h) [reset = 0h]

DFSRC1 is shown in Figure 11-194 and described in Table 11-182.
The Destination FIFO Source Address Register (DFSRC) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers. Note: Source address is not applicable for the Destination
FIFO Register Set. Read returns 0.
Note: Source address reference is not applicable for Destination FIFO Register Set. Read returns 0.


**Figure 11-194. DFSRC1 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1   0
RESERVED
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-182. DFSRC1 Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       RESERVED                   R               0h


#### 11.4.2.31 DFCNT1 Register (offset = 348h) [reset = 0h]

DFCNT1 is shown in Figure 11-195 and described in Table 11-183.
The Destination FIFO Count Register (DFCNT) is an EDMA3TC channel register. This EDMA3TC channel
register is part of the Destination Register FIFO Register Set. It is read-only and provided to facilitate
debugging by providing a window into how the transfer controller (TC) was programmed by the channel
controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The number of
destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2 each have a
destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of these transfer
controllers.


**Figure 11-195. DFCNT1 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8 7    6   5   4   3   2   1    0
BCNT                                                                                ACNT
R-0h                                                                               R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-183. DFCNT1 Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-16     BCNT                        R               0h             B-dimension count remaining for Destination Register Set.
Represents the amount of data remaining to be written.
For the final TR in the Destination Register FIFO: TC decrements
ACNT and BCNT as necessary after each write dataphase is issued.
The final value should be 0 when TR is complete.
For a non-final TR in the Destination Register FIFO: CNT will hold
the originally programmed value or the optimized BCNT value after
2D optimization calculation.
15-0     ACNT                        R               0h             A-dimension count remaining for Destination Register Set.
Represents the amount of data remaining to be written.
For the final TR in the Destination Register FIFO: TC decrements
ACNT and BCNT as necessary after each write dataphase is issued.
The final value should be 0 when TR is complete.
For a non-final TR in the Destination Register FIFO: CNT will hold
the originally programmed value or the optimized BCNT value after
2D optimization calculation.
Value 0 to FFFFh.


#### 11.4.2.32 DFDST1 Register (offset = 34Ch) [reset = 0h]

DFDST1 is shown in Figure 11-196 and described in Table 11-184.
The Destination FIFO Destination Address Register (DFDST) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.


**Figure 11-196. DFDST1 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1     0
DADDR
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-184. DFDST1 Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       DADDR                      R               0h             ARRAY(0x245d830) Note: If DAM == CONST, the 'active' address
will increment internally as if the transfer were an 'Increment'
transfer.
The address issued on the write command interface will correctly
issue the same address programmed by the user.


#### 11.4.2.33 DFBIDX1 Register (offset = 350h) [reset = 0h]

DFBIDX1 is shown in Figure 11-197 and described in Table 11-185.
The Destination FIFO B-Index Register (DFBIDX) is an EDMA3TC channel register. This EDMA3TC
channel register is part of the Destination Register FIFO Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The
number of destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2
each have a destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of
these transfer controllers.


**Figure 11-197. DFBIDX1 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8 7     6   5   4   3   2   1    0
DBIDX                                                                               SBIDX
R-0h                                                                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-185. DFBIDX1 Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-16     DBIDX                       R               0h             B-Index offset between destination arrays for the Destination FIFO
Register Set.
Represents the offset in bytes between the starting address of each
destination array (there are BCNT arrays of ACNT elements).
DBIDX is always used regardless of whether DAM is in Increment or
FIFO mode.
Value 0 to FFFFh.
15-0     SBIDX                       R               0h             B-Index offset between source arrays for the Destination FIFO
Register Set.
Represents the offset in bytes between the starting address of each
source array (there are BCNT arrays of ACNT elements).
SBIDX is always used regardless of whether SAM is in Increment or
FIFO mode.


#### 11.4.2.34 DFMPPRXY1 Register (offset = 354h) [reset = 0h]

DFMPPRXY1 is shown in Figure 11-198 and described in Table 11-186.
The Destination FIFO Memory Protection Proxy Register (DFMPPRXY) is an EDMA3TC channel register.
This EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.


**Figure 11-198. DFMPPRXY1 Register**

31              30              29                    28               27                    26            25             24
RESERVED
R-0h
23              22              21                    20               19                    18            17             16
RESERVED
R-0h
15              14              13                12                   11                    10            9              8
RESERVED                                                                  PRIV
R-0h                                                                    R-0h
7             6               5                     4                 3                    2             1               0
RESERVED                                                                     PRIVID
R-0h                                                                        R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-186. DFMPPRXY1 Register Field Descriptions**

Bit      Field                    Type             Reset          Description
31-9         RESERVED                 R                0h
8        PRIV                     R                0h             Privilege level.
This contains the Privilege level used by the EDMA3 programmer to
set up the parameter entry in the channel controller.
This field is set up when the associated TR is submitted to the
EDMA3TC.
The privilege ID is used while issuing read and write command to the
target endpoints so that the target endpoints can perform memory
protection checks based on the PRIV of the host that set up the
DMA transaction.
0h = User-level privilege
1h = Supervisor-level privilege
7-4      RESERVED                 R                0h
3-0      PRIVID                   R                0h             Privilege ID.
This contains the Privilege ID of the EDMA3 programmer that set up
the parameter entry in the channel controller.
This field is set up when the associated TR is submitted to the
EDMA3TC.
This PRIVID value is used while issuing read and write commands to
the target endpoints so that the target endpoints can perform
memory protection checks based on the PRIVID of the host that set
up the DMA transaction.
Value 0 to Fh.


#### 11.4.2.35 DFOPT2 Register (offset = 380h) [reset = 0h]

DFOPT2 is shown in Figure 11-199 and described in Table 11-187.
The Destination FIFO Options Register (DFOPT) is an EDMA3TC channel register. This EDMA3TC
channel register is part of the Destination Register FIFO Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The
number of destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2
each have a destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of
these transfer controllers.


**Figure 11-199. DFOPT2 Register**

31                 30               29                    28               27                    26            25                 24
RESERVED
R-0h
23                   22              21                 20                  19                    18            17                 16
RESERVED             TCCHEN          RESERVED           TCINTEN                     RESERVED                               TCC
R-0h                 R-0h            R-0h               R-0h                        R-0h                                 R-0h
15                 14               13                    12             11                      10           9                   8
TCC                                         RESERVED                               FWID
R-0h                                          R-0h                                 R-0h
7                  6                5                     4                 3                    2             1                 0
RESERVED                               PRI                                          RESERVED                      DAM               SAM
R-0h                                 R-0h                                           R-0h                        R-0h              R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-187. DFOPT2 Register Field Descriptions**

Bit        Field                      Type             Reset          Description
31-23       RESERVED                   R                0h
22        TCCHEN                     R                0h             Transfer complete chaining enable
0h = Transfer complete chaining is disabled
1h = Transfer complete chaining is enabled
21        RESERVED                   R                0h
20        TCINTEN                    R                0h             Transfer complete interrupt enable.
0h = Transfer complete interrupt is disabled.
1h = Transfer complete interrupt is enabled.
19-18       RESERVED                   R                0h
17-12       TCC                        R                0h             Transfer complete code.
This 6 bit code is used to set the relevant bit in CER or IPR of the
EDMA3PCC module.
11        RESERVED                   R                0h
10-8       FWID                       R                0h             FIFO width.
Applies if either SAM or DAM is set to constant addressing mode.
0h = FIFO width is 8-bit.
1h = FIFO width is 16-bit.
2h = FIFO width is 32-bit.
3h = FIFO width is 64-bit.
4h = FIFO width is 128-bit.
5h = FIFO width is 256-bit.
6h = Reserved.
7h = Reserved.
7         RESERVED                   R                0h


**Table 11-187. DFOPT2 Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
6-4      PRI                     R               0h             Transfer priority
0h = Priority 0 - Highest priority
1h = From 1h to 6h, Priority 1 to priority 6
7h = Priority 7 - Lowest priority
3-2      RESERVED                R               0h
1        DAM                     R               0h             Destination address mode within an array
0h = Increment (INCR) mode. Destination addressing within an array
increments.
1h = Constant addressing (CONST) mode. Destination addressing
within an array wraps around upon reaching FIFO width.
0        SAM                     R               0h             Source address mode within an array
0h = Increment (INCR) mode. Source addressing within an array
increments.
1h = Constant addressing (CONST) mode. Source addressing within
an array wraps around upon reaching FIFO width.


#### 11.4.2.36 DFSRC2 Register (offset = 384h) [reset = 0h]

DFSRC2 is shown in Figure 11-200 and described in Table 11-188.
The Destination FIFO Source Address Register (DFSRC) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers. Note: Source address is not applicable for Destination
FIFO Register Set. Read returns 0.
Note: Source address reference is not applicable for Destination FIFO Register Set. Read returns 0.


**Figure 11-200. DFSRC2 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1    0
RESERVED
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-188. DFSRC2 Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     RESERVED                    R               0h


#### 11.4.2.37 DFCNT2 Register (offset = 388h) [reset = 0h]

DFCNT2 is shown in Figure 11-201 and described in Table 11-189.
The Destination FIFO Count Register (DFCNT) is an EDMA3TC channel register. This EDMA3TC channel
register is part of the Destination Register FIFO Register Set. It is read-only and provided to facilitate
debugging by providing a window into how the transfer controller (TC) was programmed by the channel
controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The number of
destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2 each have a
destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of these transfer
controllers.


**Figure 11-201. DFCNT2 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8 7    6   5   4   3   2   1   0
BCNT                                                                                ACNT
R-0h                                                                               R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-189. DFCNT2 Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-16      BCNT                       R               0h             B-dimension count remaining for Destination Register Set.
Represents the amount of data remaining to be written.
For the final TR in the Destination Register FIFO: TC decrements
ACNT and BCNT as necessary after each write dataphase is issued.
The final value should be 0 when TR is complete.
For a non-final TR in the Destination Register FIFO: CNT will hold
the originally programmed value or the optimized BCNT value after
2D optimization calculation.
15-0       ACNT                       R               0h             A-dimension count remaining for Destination Register Set.
Represents the amount of data remaining to be written.
For the final TR in the Destination Register FIFO: TC decrements
ACNT and BCNT as necessary after each write dataphase is issued.
The final value should be 0 when TR is complete.
For a non-final TR in the Destination Register FIFO: CNT will hold
the originally programmed value or the optimized BCNT value after
2D optimization calculation.
Value 0 to FFFFh.


#### 11.4.2.38 DFDST2 Register (offset = 38Ch) [reset = 0h]

DFDST2 is shown in Figure 11-202 and described in Table 11-190.
The Destination FIFO Destination Address Register (DFDST) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.


**Figure 11-202. DFDST2 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1     0
DADDR
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-190. DFDST2 Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-0     DADDR                       R               0h             ARRAY(0x248ac60) Note: If DAM == CONST, the 'active' address
will increment internally as if the transfer were an 'Increment'
transfer.
The address issued on the write command interface will correctly
issue the same address programmed by the user.


#### 11.4.2.39 DFBIDX2 Register (offset = 390h) [reset = 0h]

DFBIDX2 is shown in Figure 11-203 and described in Table 11-191.
The Destination FIFO B-Index Register (DFBIDX) is an EDMA3TC channel register. This EDMA3TC
channel register is part of the Destination Register FIFO Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The
number of destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2
each have a destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of
these transfer controllers.


**Figure 11-203. DFBIDX2 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8 7     6   5   4   3   2   1    0
DBIDX                                                                               SBIDX
R-0h                                                                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-191. DFBIDX2 Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-16      DBIDX                      R               0h             B-Index offset between destination arrays for the Destination FIFO
Register Set.
Represents the offset in bytes between the starting address of each
destination array (there are BCNT arrays of ACNT elements).
DBIDX is always used regardless of whether DAM is in Increment or
FIFO mode.
Value 0 to FFFFh.
15-0       SBIDX                      R               0h             B-Index offset between source arrays for the Destination FIFO
Register Set.
Represents the offset in bytes between the starting address of each
source array (there are BCNT arrays of ACNT elements).
SBIDX is always used regardless of whether SAM is in Increment or
FIFO mode.


#### 11.4.2.40 DFMPPRXY2 Register (offset = 394h) [reset = 0h]

DFMPPRXY2 is shown in Figure 11-204 and described in Table 11-192.
The Destination FIFO Memory Protection Proxy Register (DFMPPRXY) is an EDMA3TC channel register.
This EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.


**Figure 11-204. DFMPPRXY2 Register**

31               30                29                    28               27                    26            25             24
RESERVED
R-0h
23               22                21                    20               19                    18            17             16
RESERVED
R-0h
15               14                13                12                   11                    10            9              8
RESERVED                                                                  PRIV
R-0h                                                                    R-0h
7              6                5                     4                 3                    2             1               0
RESERVED                                                                       PRIVID
R-0h                                                                          R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-192. DFMPPRXY2 Register Field Descriptions**

Bit       Field                      Type             Reset          Description
31-9      RESERVED                   R                0h
8        PRIV                       R                0h             Privilege level.
This contains the Privilege level used by the EDMA3 programmer to
set up the parameter entry in the channel controller.
This field is set up when the associated TR is submitted to the
EDMA3TC.
The privilege ID is used while issuing read and write command to the
target endpoints so that the target endpoints can perform memory
protection checks based on the PRIV of the host that set up the
DMA transaction.
0h = User-level privilege
1h = Supervisor-level privilege
7-4       RESERVED                   R                0h
3-0       PRIVID                     R                0h             Privilege ID.
This contains the Privilege ID of the EDMA3 programmer that set up
the parameter entry in the channel controller.
This field is set up when the associated TR is submitted to the
EDMA3TC.
This PRIVID value is used while issuing read and write commands to
the target endpoints so that the target endpoints can perform
memory protection checks based on the PRIVID of the host that set
up the DMA transaction.
Value 0 to Fh.


#### 11.4.2.41 DFOPT3 Register (offset = 3C0h) [reset = 0h]

DFOPT3 is shown in Figure 11-205 and described in Table 11-193.
The Destination FIFO Options Register (DFOPT) is an EDMA3TC channel register. This EDMA3TC
channel register is part of the Destination Register FIFO Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The
number of destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2
each have a destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of
these transfer controllers.


**Figure 11-205. DFOPT3 Register**

31                30               29                    28               27                    26            25                 24
RESERVED
R-0h
23                     22              21                 20                  19                    18            17                 16
RESERVED               TCCHEN          RESERVED           TCINTEN                     RESERVED                               TCC
R-0h                   R-0h            R-0h               R-0h                        R-0h                                 R-0h
15                14               13                    12             11                      10           9                   8
TCC                                         RESERVED                               FWID
R-0h                                          R-0h                                 R-0h
7                    6                5                     4                 3                    2             1                 0
RESERVED                                 PRI                                          RESERVED                      DAM               SAM
R-0h                                   R-0h                                           R-0h                        R-0h              R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-193. DFOPT3 Register Field Descriptions**

Bit       Field                      Type             Reset          Description
31-23         RESERVED                   R                0h
22        TCCHEN                     R                0h             Transfer complete chaining enable
0h = Transfer complete chaining is disabled
1h = Transfer complete chaining is enabled
21        RESERVED                   R                0h
20        TCINTEN                    R                0h             Transfer complete interrupt enable.
0h = Transfer complete interrupt is disabled.
1h = Transfer complete interrupt is enabled.
19-18         RESERVED                   R                0h
17-12         TCC                        R                0h             Transfer complete code.
This 6 bit code is used to set the relevant bit in CER or IPR of the
EDMA3PCC module.
11        RESERVED                   R                0h
10-8          FWID                       R                0h             FIFO width.
Applies if either SAM or DAM is set to constant addressing mode.
0h = FIFO width is 8-bit.
1h = FIFO width is 16-bit.
2h = FIFO width is 32-bit.
3h = FIFO width is 64-bit.
4h = FIFO width is 128-bit.
5h = FIFO width is 256-bit.
6h = Reserved.
7h = Reserved.
7         RESERVED                   R                0h


**Table 11-193. DFOPT3 Register Field Descriptions (continued)**

Bit      Field                   Type            Reset          Description
6-4      PRI                     R               0h             Transfer priority
0h = Priority 0 - Highest priority
1h = From 1h to 6h, Priority 1 to priority 6
7h = Priority 7 - Lowest priority
3-2      RESERVED                R               0h
1       DAM                     R               0h             Destination address mode within an array
0h = Increment (INCR) mode. Destination addressing within an array
increments.
1h = Constant addressing (CONST) mode. Destination addressing
within an array wraps around upon reaching FIFO width.
0       SAM                     R               0h             Source address mode within an array
0h = Increment (INCR) mode. Source addressing within an array
increments.
1h = Constant addressing (CONST) mode. Source addressing within
an array wraps around upon reaching FIFO width.


#### 11.4.2.42 DFSRC3 Register (offset = 3C4h) [reset = 0h]

DFSRC3 is shown in Figure 11-206 and described in Table 11-194.
The Destination FIFO Source Address Register (DFSRC) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers. Note: Source address is not applicable for Destination
FIFO Register Set. Read returns 0.
Note: Source address reference is not applicable for Destination FIFO Register Set. Read returns 0.


**Figure 11-206. DFSRC3 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1   0
RESERVED
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-194. DFSRC3 Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       RESERVED                   R               0h


#### 11.4.2.43 DFCNT3 Register (offset = 3C8h) [reset = 0h]

DFCNT3 is shown in Figure 11-207 and described in Table 11-195.
The Destination FIFO Count Register (DFCNT) is an EDMA3TC channel register. This EDMA3TC channel
register is part of the Destination Register FIFO Register Set. It is read-only and provided to facilitate
debugging by providing a window into how the transfer controller (TC) was programmed by the channel
controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The number of
destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2 each have a
destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of these transfer
controllers.


**Figure 11-207. DFCNT3 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8 7    6   5   4   3   2   1    0
BCNT                                                                                ACNT
R-0h                                                                               R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-195. DFCNT3 Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-16     BCNT                        R               0h             B-dimension count remaining for Destination Register Set.
Represents the amount of data remaining to be written.
For the final TR in the Destination Register FIFO: TC decrements
ACNT and BCNT as necessary after each write dataphase is issued.
The final value should be 0 when TR is complete.
For a non-final TR in the Destination Register FIFO: CNT will hold
the originally programmed value or the optimized BCNT value after
2D optimization calculation.
15-0     ACNT                        R               0h             A-dimension count remaining for Destination Register Set.
Represents the amount of data remaining to be written.
For the final TR in the Destination Register FIFO: TC decrements
ACNT and BCNT as necessary after each write dataphase is issued.
The final value should be 0 when TR is complete.
For a non-final TR in the Destination Register FIFO: CNT will hold
the originally programmed value or the optimized BCNT value after
2D optimization calculation.
Value 0 to FFFFh.


#### 11.4.2.44 DFDST3 Register (offset = 3CCh) [reset = 0h]

DFDST3 is shown in Figure 11-208 and described in Table 11-196.
The Destination FIFO Destination Address Register (DFDST) is an EDMA3TC channel register. This
EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.


**Figure 11-208. DFDST3 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8   7   6   5   4   3   2   1     0
DADDR
R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-196. DFDST3 Register Field Descriptions**

Bit    Field                      Type            Reset          Description
31-0       DADDR                      R               0h             ARRAY(0x24b1d10) Note: If DAM == CONST, the 'active' address
will increment internally as if the transfer were an 'Increment'
transfer.
The address issued on the write command interface will correctly
issue the same address programmed by the user.


#### 11.4.2.45 DFBIDX3 Register (offset = 3D0h) [reset = 0h]

DFBIDX3 is shown in Figure 11-209 and described in Table 11-197.
The Destination FIFO B-Index Register (DFBIDX) is an EDMA3TC channel register. This EDMA3TC
channel register is part of the Destination Register FIFO Register Set. It is read-only and provided to
facilitate debugging by providing a window into how the transfer controller (TC) was programmed by the
channel controller (CC), as well as showing status of the transfer controller (TC) during a transfer. The
number of destination FIFO register sets depends on the destination FIFO depth. TC0, TC1, and TC2
each have a destination FIFO depth of 4, so there are four sets of destination FIFO registers for each of
these transfer controllers.


**Figure 11-209. DFBIDX3 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10                                    9   8 7     6   5   4   3   2   1    0
DBIDX                                                                               SBIDX
R-0h                                                                                R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-197. DFBIDX3 Register Field Descriptions**

Bit      Field                       Type            Reset          Description
31-16     DBIDX                       R               0h             B-Index offset between destination arrays for the Destination FIFO
Register Set.
Represents the offset in bytes between the starting address of each
destination array (there are BCNT arrays of ACNT elements).
DBIDX is always used regardless of whether DAM is in Increment or
FIFO mode.
Value 0 to FFFFh.
15-0     SBIDX                       R               0h             B-Index offset between source arrays for the Destination FIFO
Register Set.
Represents the offset in bytes between the starting address of each
source array (there are BCNT arrays of ACNT elements).
SBIDX is always used regardless of whether SAM is in Increment or
FIFO mode.


#### 11.4.2.46 DFMPPRXY3 Register (offset = 3D4h) [reset = 0h]

DFMPPRXY3 is shown in Figure 11-210 and described in Table 11-198.
The Destination FIFO Memory Protection Proxy Register (DFMPPRXY) is an EDMA3TC channel register.
This EDMA3TC channel register is part of the Destination Register FIFO Register Set. It is read-only and
provided to facilitate debugging by providing a window into how the transfer controller (TC) was
programmed by the channel controller (CC), as well as showing status of the transfer controller (TC)
during a transfer. The number of destination FIFO register sets depends on the destination FIFO depth.
TC0, TC1, and TC2 each have a destination FIFO depth of 4, so there are four sets of destination FIFO
registers for each of these transfer controllers.


**Figure 11-210. DFMPPRXY3 Register**

31              30              29                    28               27                    26            25             24
RESERVED
R-0h
23              22              21                    20               19                    18            17             16
RESERVED
R-0h
15              14              13                12                   11                    10            9              8
RESERVED                                                                  PRIV
R-0h                                                                    R-0h
7             6               5                     4                 3                    2             1               0
RESERVED                                                                     PRIVID
R-0h                                                                        R-0h
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; -n = value after reset


**Table 11-198. DFMPPRXY3 Register Field Descriptions**

Bit      Field                    Type             Reset          Description
31-9         RESERVED                 R                0h
8        PRIV                     R                0h             Privilege level.
This contains the Privilege level used by the EDMA3 programmer to
set up the parameter entry in the channel controller.
This field is set up when the associated TR is submitted to the
EDMA3TC.
The privilege ID is used while issuing read and write command to the
target endpoints so that the target endpoints can perform memory
protection checks based on the PRIV of the host that set up the
DMA transaction.
0h = User-level privilege
1h = Supervisor-level privilege
7-4      RESERVED                 R                0h
3-0      PRIVID                   R                0h             Privilege ID.
This contains the Privilege ID of the EDMA3 programmer that set up
the parameter entry in the channel controller.
This field is set up when the associated TR is submitted to the
EDMA3TC.
This PRIVID value is used while issuing read and write commands to
the target endpoints so that the target endpoints can perform
memory protection checks based on the PRIVID of the host that set
up the DMA transaction.
Value 0 to Fh.


## 11.5 Appendix A


### 11.5.1 Debug Checklist

This section lists some tips to keep in mind while debugging applications using the EDMA3.
The following table provides some common issues and their probable causes and resolutions.


**Table 11-199. Debug List**

Issue                                       Description/Solution
The transfer associated with the channel    The EDMA3CC may not service a transfer request, even though the associated
does not happen. The channel does not       PaRAM set is programmed appropriately. Check for the following:
get serviced.                               1) Verify that events are enabled, i.e., if an external/peripheral event is latched in Event
Registers (ER/ERH), make sure that the event is enabled in the Event Enable
Registers (EER/EERH). Similarly, for QDMA channels, make sure that QDMA events
are appropriately enabled in the QDMA Event Enable Register (QEER).
2) Verify that the DMA or QDMA Secondary Event Register (SER/SERH/QSERH) bits
corresponding to the particular event or channel are not set.
The Secondary Event Registers bits are      It is possible that a trigger event was received when the parameter set associated with
set, not allowing additional transfers to   the channel/event was a NULL set for a previous transfer on the channel. This is
occur on a channel.                         typical in two cases:
1) QDMA channels: Typically if the parameter set is non-static and expected to be
terminated by a NULL set (i.e., OPT.STATIC = 0, LINK = 0xFFFF), the parameter set is
updated with a NULL set after submission of the last TR. Because QDMA channels are
auto-triggered, this update caused the generation of an event. An event generated for a
NULL set causes an error condition and results in setting the bits corresponding to the
QDMA channel in the QEMR and QSER. This will disable further prioritization of the
channel.
2) DMA channels used in a continuous mode: The peripheral may be set up to
continuously generate infinite events (for instance, in case of McASP, every time the
data shifts out from the DXR register, it generates an XEVT). The parameter set may
be programmed to expect only a finite number of events and to be terminated by a
NULL link. After the expected number of events, the parameter set is reloaded with a
NULL parameter set. Because the peripheral will generate additional events, an error
condition is set in the SER.Ex and EMR.Ex set, preventing further event prioritization.
You must ensure that the number of events received is limited to the expected number
of events for which the parameter set is programmed, or you must ensure that bits
corresponding to particular channel or event are not set in the Secondary event
registers (SER/SERH/QSER) and Event Missed Registers (EMR/EMRH/QEMR) before
trying to perform subsequent transfers for the event/channel.
Completion interrupts are not asserted, or You must ensure the following:
no further interrupts are received after the 1) The interrupt generation is enabled in the OPT of the associated PaRAM set
first completion interrupt.                  (TCINTEN = 1 and/or ITCINTEN = 1).
2) The interrupts are enabled in the EDMA3 Channel Controller, via the Interrupt
Enable Registers (IER/IERH).
3) The corresponding interrupts are enabled in the device interrupt controller.
4) The set interrupts are cleared in the interrupt pending registers (IPR/IPRH) before
exiting the transfer completion interrupt service routine (ISR). See Section 11.3.9.1.2
for details on writing EDMA3 ISRs.
5) If working with shadow region interrupts, make sure that the DMA Region Access
registers (DRAE/DRAEH) are set up properly, because the DRAE/DRAEH registers act
as secondary enables for shadow region completion interrupts, along with the
IER/IERH registers.
If working with shadow region interrupts, make sure that the bits corresponding to the
transfer completion code (TCC) value are also enabled in the DRAE/DRAEH registers.
For instance, if the PaRAM set associated with Channel 0 returns a completion code of
63 (OPT.TCC=63), ensure that DRAEH.E63 is also set for a shadow region completion
interrupt because the interrupt pending register bit set will be IPRH.I63 (not IPR.I0).


### 11.5.2 Miscellaneous Programming/Debug Tips

1. For several registers, the setting and clearing of bits needs to be done via separate dedicated
registers. For example, the Event Register (ER/ERH) can only be cleared by writing a 1 to the
corresponding bits in the Event Clear Registers (ECR/ECRH). Similarly, the Event Enable Register
(EER/EERH) bits can only be set with writes of 1 to the Event Enable Set Registers (EESR/EESRH)
and cleared with writes of 1 to the corresponding bits in the Event Enable Clear Register
(EECR/EECRH).
2. Writes to the shadow region memory maps are governed by region access registers
(DRAE/DRAEH/QRAE). If the appropriate channels are not enabled in these registers, read/write
access to the shadow region memory map is not enabled.
3. When working with shadow region completion interrupts, ensure that the DMA Region Access
Registers (DRAE/DRAEH) for every region are set in a mutually exclusive way (unless it is a
requirement for an application). If there is an overlap in the allocated channels and transfer completion
codes (setting of Interrupt Pending Register bits) in the region resource allocation, it results in multiple


shadow region completion interrupts. For example, if DRAE0.E0 and DRAE1.E0 are both set, then on
completion of a transfer that returns a TCC=0, they will generate both shadow region 0 and 1
completion interrupts.
4. While programming a non-dummy parameter set, ensure the CCNT is not left to zero.
5. Enable the EDMA3CC error interrupt in the device controller and attach an interrupt service routine
(ISR) to ensure that error conditions are not missed in an application and are appropriately addressed
with the ISR.
6. Depending on the application, you may want to break large transfers into smaller transfers and use
self-chaining to prevent starvation of other events in an event queue.
7. In applications where a large transfer is broken into sets of small transfers using chaining or other
methods, you might choose to use the early chaining option to reduce the time between the sets of
transfers and increase the throughput. However, keep in mind that with early completion, all data might
have not been received at the end point when completion is reported because the EDMA3CC internally
signals completion when the TR is submitted to the EDMA3TC, potentially before any data has been
transferred.
8. The event queue entries can be observed to determine the last few events if there is a system failure
(provided the entries were not bypassed).


### 11.5.3 Setting Up a Transfer

The following list provides a quick guide for the typical steps involved in setting up a transfer.
Step 1. Initiating a DMA/QDMA channel
1. Determine the type of channel (QDMA or DMA) to be used.
2. Channel mapping
i. If using a QDMA channel, program the QCHMAP with the parameter set number to
which the channel maps and the trigger word.
ii. If using a DMA channel, program the DCHMAP with the parameter set number to
which the channel maps.
3. If the channel is being used in the context of a shadow region, ensure the DRAE/DRAEH
for the region is properly set up to allow read write accesses to bits in the event registers
and interrupt registers in the Shadow region memory map. The subsequent steps in this
process should be done using the respective shadow region registers. (Shadow region
descriptions and usage are provided in Section 11.3.7.1.)
4. Determine the type of triggering used.
i. If external events are used for triggering (DMA channels), enable the respective
event in EER/EERH by writing into EESR/EESRH.
ii. If QDMA Channel is used, enable the channel in QEER by writing into QEESR.
5. Queue setup
i. If a QDMA channel is used, set up the QDMAQNUM to map the channel to the
respective event queue.
ii. If a DMA channel is used, set up the DMAQNUM to map the event to the respective
event queue.
Step 2. Parameter set setup
1. Program the PaRAM set number associated with the channel. Note that if it is a QDMA
channel, the PaRAM entry that is configured as trigger word is written to last.
Alternatively, enable the QDMA channel (step 1-b-ii above) just before the write to the
trigger word.
See Section 11.3.19 for parameter set field setups for different types of transfers. See the
sections on chaining (Section 11.3.8) and interrupt completion (Section 11.3.9) on how to
set up final/intermediate completion chaining and/or interrupts.
Step 3. Interrupt setup
1. Enable the interrupt in the IER/IERH by writing into IESR/IESRH.
2. Ensure that the EDMA3CC completion interrupt (either the global or the shadow region
interrupt) is enabled properly in the device interrupt controller.
3. Ensure the EDMA3CC completion interrupt (this refers to either the Global interrupt or the
shadow region interrupt) is enabled properly in the Device Interrupt controller.
4. Set up the interrupt controller properly to receive the expected EDMA3 interrupt.


Step 4.   Initiate transfer
1. This step is highly dependent on the event trigger source:
i. If the source is an external event coming from a peripheral, the peripheral will be
enabled to start generating relevant EDMA3 events that can be latched to the ER
transfer.
ii. For QDMA events, writes to the trigger word (step 2-a above) will initiate the transfer.
iii. Manually triggered transfers will be initiated by writes to the Event Set Registers
(ESR/ESRH).
iv. Chained-trigger events initiate when a previous transfer returns a transfer completion
code equal to the chained channel number.
Step 5.   Wait for completion
1. If the interrupts are enabled as mentioned in step 3 above, then the EDMA3CC will
generate a completion interrupt to the CPU whenever transfer completion results in
setting the corresponding bits in the interrupt pending register (IPR/IPRH). The set bits
must be cleared in the IPR\IPRH by writing to corresponding bit in ICR\ICRH.
2. If polling for completion (interrupts not enabled in the device controller), then the
application code can wait on the expected bits to be set in the IPR\IPRH. Again, the set
bits in the IPR\IPRH must be manually cleared via ICR\ICRH before the next set of
transfers is performed for the same transfer completion code values.


