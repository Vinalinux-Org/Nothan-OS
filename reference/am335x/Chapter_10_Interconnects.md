# 10 Interconnects


This chapter describes the interconnects of the device.



## 10.1 Introduction

The system interconnect is based on a 2-level hierarchical architecture (L3, L4) driven by system
performance. The L4 interconnect is based on a fully native OCP infrastructure, directly complying with the
OCPIP2.2 reference standard.


### 10.1.1 Terminology

The following is a brief explanation of some terms used in this document:
Initiator: Module able to initiate read and write requests to the chip interconnect (typically: processors,
DMA, etc.).
Target: Unlike an initiator, a target module cannot generate read/write requests to the chip interconnect,
but it can respond to these requests. However, it may generate interrupts or a DMA request to the system
(typically: peripherals, memory controllers). Note: A module can have several separate ports; therefore, a
module can be an initiator and a target.
Agent: Each connection of one module to one interconnect is done using an agent, which is an adaptation
(sometimes configurable) between the module and the interconnect. A target module is connected by a
target agent (TA), and an initiator module is connected by an initiator agent (IA).
Interconnect: The decoding, routing, and arbitration logic that enable the connection between multiple
initiator modules and multiple target modules connected on it.
Register Target (RT): Special TA used to access the interconnect internal configuration registers.
Data-flow Signal: Any signal that is part of a clearly identified transfer or data flow (typically: command,
address, byte enables, etc.). Signal behavior is defined by the protocol semantics.
Sideband Signal: Any signal whose behavior is not associated to a precise transaction or data flow.
Command Slot: A command slot is a subset of the command list. It is the memory buffer for a single
command. A total of 32 command slots exist.
Out-of-band Error: Any signal whose behavior is associated to a device error-reporting scheme, as
opposed to in-band errors. Note: Interrupt requests and DMA requests are not routed by the interconnect
in the device.
ConnID: Any transaction in the system interconnect is tagged by an in-band qualifier ConnID, which
uniquely identifies the initiator at a given interconnect point. A ConnID is transmitted in band with the
request and is used for error-logging mechanism.


### 10.1.2 L3 Interconnect

The L3 high-performance interconnect is based on a Network-On-Chip (NoC) interconnect infrastructure.
The NoC uses an internal packet-based protocol for forward (read command, write command with data
payload) and backward (read response with data payload, write response) transactions. All exposed
interfaces of this NoC interconnect, both for Targets and Initiators; comply with the OCPIP2.2 reference
standard.


#### 10.1.2.1 L3 Topology

The L3 topology is driven by performance requirements, bus types, and clocking structure. The main L3
paths are shown in Figure 10-1. Arrows indicate the master/slave relationship not data flow. L3 is
partitioned into two separate clock domains: L3F corresponds to L3 Fast clock domain and L3S
corresponds to L3 Slow clock domain.


**Figure 10-1. L3 Topology**

MPUSS                                                 2 Port           PRU-ICSS                                                   USB
SGX530                         (Cortex A8)                                            GEMAC
TPTC                                                   Debug
LCD                                                              Switch             0     1                                               0         1     IEEE
3 Channels                                                 Acc
Ctrl                                                                                                                                                      1500
R0 W0 R1 W1 R2 W2                                               Port
128                 32          128   64         128 128 128 128 128 128            32               32   32     32                                       32        32      32


L3F                                                                                                         L3S


128         32   32    32        32              64         64      FR           32      32       32                         32   32   32   32    32      32              32     32        32      32        32
L4_WKUP
0        1    2
OCMC                             OCP-WP            L4_FAST                                           ADC
TPTC CFG                                         SGX530                                                                                                      McASP1               USB             L4_WKUP
RAM                                                                                                 TSC
32
EMIF                             TPCC                                                                                           L4_PER                   McASP0                  GPMC             MMCHS2
DebugSS


#### 10.1.2.2 L3 Port Mapping

Each initiator and target core is connected to the L3 interconnect through a Network Interface Unit (NIU).
The NIUs act as entry and exit points to the L3 Network on Chip – converting between the IP’s OCP
protocol and the NoC’s internal protocol, and also include various programming registers. All ports are
single threaded with tags used to enable pipelined transactions. The interconnect includes:
Initiator Ports:
• L3F
– Cortex-A8 MPUSS 128-bit initiator port0 and 64-bit initiator port1
– SGX530 128-bit initiator port
– 3 TPTC 128-bit read initiator ports
– 3 TPTC 128-bit write initiator ports
– LCDC 32-bit initiator port
– 2 PRU-ICSS1 32-bit initiator ports
– 2 port Gigabit Ethernet Switch (2PGSW) 32-bit initiator port
– Debug Subsystem 32-bit initiator port
• L3S
– USB 32-bit CPPI DMA initiator port
– USB 32-bit Queue Manager initiator port
– P1500 32-bit initiator port
Target Ports:
• L3F
– EMIF 128-bit target port
– 3 TPTC CFG 32-bit target ports
– TPCC CFG 32-bit target port
– OCM RAM0 64-bit target port
– DebugSS 32-bit target port
– SGX530 64-bit target port
– L4_FAST 32-bit target port
• L3S
– 4 L4_PER peripheral 32-bit target ports
– GPMC 32-bit target port
– McASP0 32-bit target port

–   McASP1 32-bit target port
–   ADC_TSC 32-bit target port
–   USB 32-bit target port
–   MMHCS2 32-bit target port
–   L4_WKUP wakeup 32-bit target port


#### 10.1.2.3 Interconnect Requirements

The required L3 connections between bus masters and slave ports are shown in Table 10-1. The L3
interconnect will return an address-hole error if any initiator attempts to access a target to which it has no
connection.


**Table 10-1. L3 Master — Slave Connectivity**

Slaves
OCMC-RAM


TPCC


L4_PER ort0


MMCHS2


NOC Registers
SGX530


L4_WKUP

DebugSS
TPTC0–2 FG


Expansion Slot


USBCFG
EMIF


L4_Fast


L4_PER Port1

L4_PER Port 2

L4_PER Port 3

GPMC

ADC / TSC
Master ID          Masters


0x00           MPUSS M1 (128-bit)      R
0x00           MPUSS M2 (64-bit)       R      R          R            R      R        R                R         R                                                            R      R           R        R        R         R         R
0x18           TPTC0 RD                R      R                       R               R                R                                      R                               R      R           R        R        R
0x19           TPTC0 WR                R      R                       R               R                R                                      R                               R      R           R        R        R         R
0x1A           TPTC1 RD                R      R                       R               R                R                                      R                               R      R           R        R        R
0x1B           TPTC1 WR                R      R                       R               R                R                                      R                               R      R           R        R        R
0x1C           TPTC2 RD                R      R                       R               R                R                                                      R               R      R           R        R        R
0x1D           TPTC2 WR                R      R                       R               R                R                                                      R               R      R           R        R        R
0x24           LCD Controller          R      R                                                                                                                               R
0x0E           PRU-ICSS (PRU0)         R      R          R            R               R                R                       R                                              R      R                             R         R         R
0x0F           PRU-ICSS (PRU1)         R      R          R            R               R                R                       R                                              R      R                             R         R         R
0x30           GEMAC                   R      R                                                                                                                               R
0x20           SGX530                  R      R                                                                                                                               R
0x34           USB0 DMA                R      R
0x35           USB1 Queue Mgr          R      R                                                                                                                               R
0x04           EMU (DAP)               R      R          R            R      R        R                R                                                      R               R      R           R        R        R         R         R
0x05           IEEE1500                R      R          R            R      R        R                R                                                      R               R      R           R        R        R         R         R


### 10.1.3 L4 Interconnect

The L4 interconnect is a non-blocking peripheral interconnect that provides low latency access to a large
number of low bandwidth, physically dispersed target cores. The L4 can handle incoming traffic from up to
four initiators and can distribute those communication requests to and collect related responses from up to
63 targets.
This device provides three interfaces with L3 interconnect for High Speed Peripheral, Standard Peripheral,
and Wakeup Peripherals. . Figure 10-2 shows the L4 bus architecture and memory-mapped peripherals.


**Figure 10-2. L4 Topology**


L3S
M3

32       32        32           32              32                   32              32


L4_PER                                     L4_FAST               L4_WKUP


DCAN0             LCD Ctlr                         GEMAC        ADC_TSC
DCAN1             Mailbox0                                      Control Module
DMTIMER2          McASP0 CFG                                    DMTIMER0
DMTIMER3          McASP1 CFG                                    DMTIMER1_1MS
DMTIMER4          MMCHS0                PRU-ICSS                GPIO0
DMTIMER5          MMCHS1                                        I2C0
DMTIMER6          OCP Watchpoint                                M3 UMEM
RTC
DMTIMER7          SPI0                                          M3 DMEM
eCAP/eQEP/ePWM0   SPI1                                          SmartReflex 0
eCAP/eQEP/ePWM1   Spinlock                                      SmartReflex 1
eCAP/eQEP/ePWM2   UART1                                         UART0
eFuse Ctl         UART2                                         WDT1
ELM               UART3
GPIO1             UART4
GPIO2             UART5
GPIO3
I2C1                                                                                          DebugSS
PRCM
I2C2                                                                                         HWMaster1
IEEE1500


