# 17 Interprocessor Communication

This chapter describes the interprocessor communication of the device.

| Topic |          | Page |
|-------|----------|------|
| 17.1  | Mailbox  | 4105 |
| 17.2  | Spinlock | 4175 |

## **17.1 Mailbox**

### *17.1.1 Introduction*

#### **17.1.1.1 Features**

Global features of the Mailbox module are:

- OCP slave interface (L4) supports:
  - 32-bit data bus width
  - 8/16/32 bit access supported
  - 9-bit address bus width
  - Burst not supported
- 8 mailbox sub-modules
- Each mailbox sub module allows 1-way communication between 2 initiators
- Flexible mailbox/initiators assignment scheme
- 4 messages per mailbox sub-module
- 32-bit message width
- Support of 16/32-bit addressing scheme
- Non-intrusive emulation
- 4 interrupts (one per user: 1 to MPU Subsystem, 2 to PRU-ICSS, and 1 to WakeM3)

#### **17.1.1.2 Unsupported Features**

There are no unsupported features for Mailbox on this device.

#### **Integration**

This device contains a single instantiation of the Mailbox module at the system level. The mailbox function is made of eight sub-module mailboxes each supporting a 1-way communication between two initiators. The communication protocol from the sender to the receiver is implemented with mailbox registers using interrupts. The sender sends information to the receiver by writing to the mailbox. Interrupt signaling is used to notify the receiver a message has been queued or the sender for overflow situation.

The eight mailboxes are enough to handle communications between the MPU Subsystem, PRU-ICSS PRUs, and WakeM3. Note that because the WakeM3 has access only to L4\_Wakeup peripherals it does not have access to the Mailbox registers. A mailbox interrupt can still be sent to the M3 to trigger message notification. The actual message payload must be placed in either M3 internal memory or in the Control Module Interprocessor Message registers (IPC\_MSG\_REG{0-7}).

## **Mailbox Integration**

```
+------------------------------------------+
|          L4 Peripheral Interconnect      |
+------------------------------------------+
                    ||
                    \/
+-----------+       MAIL_U0_IRQ        +-----------------------------+
|           |------------------------->|                             |
| MPU       |                          |                             |
| Subsystem |                          |                             |
+-----------+       MAIL_U1_IRQ        |           Mailbox           |
+-----------+------------------------->|                             |
|           |       MAIL_U2_IRQ        |   (x8)                      |
| PRU-ICSS  |------------------------->|   +-----------------------+ |
|  PRU0/1   |                          |   |       MAILBOXn        | |
+-----------+                          |   | (4 x 32-bit messages) | |
+-----------+       MAIL_U3_IRQ        |   +-----------------------+ |
|           |------------------------->|                             |
| WakeM3    |                          |                             |
+-----------+                          +-----------------------------+
```
#### **17.1.1.1 Mailbox Connectivity Attributes**

The general connectivity for the Mailbox is shown in Mailbox [Connectivity](#page-2-0) Attributes.

#### **Mailbox Connectivity Attributes**

| Attributes          | Type                                                                                                                                       |
|---------------------|--------------------------------------------------------------------------------------------------------------------------------------------|
| Power Domain        | Peripheral Domain                                                                                                                          |
| Clock Domain        | L4PER_L4LS_GCLK                                                                                                                            |
| Reset Signals       | PER_DOM_RST_N                                                                                                                              |
| Idle/Wakeup Signals | Smart Idle                                                                                                                                 |
| Interrupt Requests  | 4 Interrupts<br>mail_u0 (MBINT0) – to MPU Subsystem<br>mail_u1 – to PRU-ICSS (PRU0)<br>mail_u2 – to PRU-ICSS (PRU1)<br>mail_u3 – to WakeM3 |
| DMA Requests        | None                                                                                                                                       |
| Physical Address    | L4 Peripheral slave port                                                                                                                   |

#### **17.1.1.2 Mailbox Clock and Reset Management**

The mailbox function operates from the L4 interface clock.

## **Table 17-1. Mailbox Clock Signals**

| Clock Signal                 | Max Freq | Reference / Source | Comments                      |
|------------------------------|----------|--------------------|-------------------------------|
| Functional / Interface clock | 100 MHz  | CORE_CLKOUTM4 / 2  | pd_per_l4ls_gclk<br>From PRCM |

#### **17.1.1.3 Mailbox Pin List**

The Mailbox module does not include any external interface pins.

## Functional Description

This device has the following mailbox instances:

• System mailbox

[Table](#page-3-0) 17-2 shows Mailbox Implementation in this device, where u is the user number and m is the mailbox number.

**Table 17-2. Mailbox Implementation**

| Mailbox Type   | User Number(u) | Mailbox Number(m) | Messages per Mailbox |
|----------------|----------------|-------------------|----------------------|
| System mailbox | 0 to 3         | 0 to 7            | 4                    |

The mailbox module provides a means of communication through message queues among the users (depending on the mailbox module instance). The individual mailbox modules (8 for the system mailbox instance), or FIFOs, can associate (or de-associate) with any of the processors using the MAILBOX\_IRQENABLE\_SET\_u (or MAILBOX\_IRQENABLE\_CLR\_u) register.

The system mailbox module includes the following user subsystems:

- User 0: MPU Subsystem (u = 0)
- User 1: PRU\_ICSS PRU0 (u = 1)
- User 2: PRU\_ICSS PRU1 (u = 2)
- User 3: WakeM3 (u = 3)

Each user has a dedicated interrupt signal from the corresponding mailbox module instance and dedicated interrupt enabling and status registers. Each

MAILBOX\_IRQSTATUS\_RAW\_u/MAILBOX\_IRQSTATUS\_CLR\_u interrupt status register corresponds to a particular user.

For the system mailbox instance, a user can query its interrupt status register through the L4\_STANDARD interconnect.

#### **17.1.1.1 Mailbox Block Diagram**

[Figure](#page-4-0) 17-1 shows the mailbox block diagram.

32 Message out Message in Mailbox m 4 message FIFO Local interface Mailbox Interconnect 32 32 32 MAILBOX\_SYSCONFIG System register MAILBOX\_IRQENABLE\_CLR\_u MAILBOX\_IRQENABLE\_SET\_u MAILBOX\_MSGSTATUS\_m MAILBOX\_FIFOSTATUS\_m MAILBOX\_MESSAGE\_m MAILBOX\_IRQSTATUS\_CLR\_u MAILBOX\_IRQSTATUS\_RAW\_u Interrupt registers Interrupt requests

**Figure 17-1. Mailbox Block Diagram**

#### **17.1.1.2 Software Reset**

The mailbox module supports a software reset through the MAILBOX\_SYSCONFIG[0].SOFTRESET bit. Setting this bit to 1 enables an active software reset that is functionally equivalent to a hardware reset. Reading the MAILBOX\_SYSCONFIG[0] SOFTRESET bit gives the status of the software reset:

- Read 1: the software reset is on-going
- Read 0: the software reset is complete

The software must ensure that the software reset completes before doing mailbox operations.

#### **17.1.1.3 Power Management**

[Table](#page-4-1) 17-3 describes power-management features available for the mailbox module.

**Table 17-3. Local Power Management Features**

| Feature                | Registers                            | Description                                               |
|------------------------|--------------------------------------|-----------------------------------------------------------|
| Clock autogating       | NA                                   | Feature not available                                     |
| Slave idle modes       | MAILBOX_SYSCONFIG[3:2].SIDLEMODE | Force-idle, no-idle and smart-idle modes<br>are available |
| Clock activity         | NA                                   | Feature not available                                     |
| Master standby modes   | NA                                   | Feature not available                                     |
| Global wake-up enable  | NA                                   | Feature not available                                     |
| Wake-up sources enable | NA                                   | Feature not available                                     |

The mailbox module can be configured using the MAILBOX\_SYSCONFIG[3:2] SIDLEMODE bit field to one of the following acknowledgment modes:

- Force-idle mode (SIDLEMODE = 0x0): The mailbox module immediately enters the idle state on receiving a low-power-mode request from the PRCM module. In this mode, the software must ensure that there are no asserted output interrupts before requesting this mode to go into the idle state.
- No-idle mode (SIDLEMODE = 0x1): The mailbox module never enters the idle state.
- Smart-idle mode (SIDLEMODE = 0x2): After receiving a low-power-mode request from the PRCM module, the mailbox module enters the idle state only after all asserted output interrupts are acknowledged.

#### **17.1.1.4 Interrupt Requests**

An interrupt request allows the user of the mailbox to be notified when a message is received or when the message queue is not full. There is one interrupt per user. [Table](#page-5-0) 17-4 lists the event flags, and their mask, that can cause module interrupts.

|  | Table 17-4. Interrupt Events
|--|------------------------------

| Non-Maskable Event<br>Flag(1)                            | Maskable Event Flag                                      | Event Mask Bit                                               | Event Unmask Bit                                             | Description                            |
|----------------------------------------------------------|----------------------------------------------------------|--------------------------------------------------------------|--------------------------------------------------------------|----------------------------------------|
| MAILBOX_IRQSTATUS_RAW_u[0+m*2].NEWMSGSTATUSUUMBm | MAILBOX_IRQSTATUS_CLR_u[0+m*2].NEWMSGSTATUSUUMBm | MAILBOX_IRQENABLE_CLR_u[0+m*2].                          |                                                              |                                        |
| MAILBOX_IRQSTATUS_RAW_u[0+m*2].NEWMSGSTATUSUUMBm | MAILBOX_IRQSTATUS_CLR_u[0+m*2].NEWMSGSTATUSUUMBm | MAILBOX_IRQENABLE_CLR_u[0+m*2].<br>NEWMSGSTATUSUUMBm | MAILBOX_IRQENABLE_SET_u[0+m*2].<br>NEWMSGSTATUSUUMBm | Mailbox m receives a<br>new message    |
| MAILBOX_IRQSTATUS_RAW_u[1+m*2].NOTFULLSTATUSUMBm | MAILBOX_IRQSTATUS_CLR_u[1+m*2].NOTFULLSTATUSUMBm | MAILBOX_IRQENABLE_CLR_u[1+m*2].<br>NOTFULLSTATUSUMB<br>m | MAILBOX_IRQENABLE_SET_u[1+m*2].<br>NOTFULLSTATUSUMB<br>m | Mailbox m message<br>queue is not full |

<sup>(1)</sup> MAILBOX.MAILBOX\_IRQSTATUS\_RAW\_u register is mostly used for debug purposes.

**CAUTION**

Once an event generating the interrupt request has been processed by the software, it must be cleared by writing a logical 1 in the corresponding bit of the MAILBOX\_IRQSTATUS\_CLR\_u register. Writing a logical 1 in a bit of the MAILBOX\_IRQSTATUS\_CLR\_u register will also clear to 0 the corresponding bit in the appropriate MAILBOX\_IRQSTATUS\_RAW\_u register.

An event can generate an interrupt request when a logical 1 is written to the corresponding unmask bit in the MAILBOX\_IRQENABLE\_SET\_u register. Events are reported in the appropriate MAILBOX\_IRQSTATUS\_CLR\_u and MAILBOX\_IRQSTATUS\_RAW\_u registers.

An event stops generating interrupt requests when a logical 1 is written to the corresponding mask bit in the MAILBOX\_IRQENABLE\_CLR\_u register. Events are only reported in the appropriate MAILBOX\_IRQSTATUS\_RAW\_u register.

In case of the MAILBOX\_IRQSTATUS\_RAW\_u register, the event is reported in the corresponding bit even if the interrupt request generation is disabled for this event.

#### **17.1.1.5 Assignment**

#### *17.1.1.5.1 Description*

To assign a receiver to a mailbox, set the new message interrupt enable bit corresponding to the desired mailbox in the MAILBOX\_IRQENABLE\_SET\_u register. The receiver reads the MAILBOX\_MESSAGE\_m register to retrieve a message from the mailbox.

An alternate method for the receiver that does not use the interrupts is to poll the MAILBOX\_FIFOSTATUS\_m and/or MAILBOX\_MSGSTATUS\_m registers to know when to send or retrieve a message to or from the mailbox. This method does not require assigning a receiver to a mailbox. Because this method does not include the explicit assignment of the mailbox, the software must avoid having multiple receivers use the same mailbox, which can result in incoherency.

To assign a sender to a mailbox, set the queue-not-full interrupt enable bit of the desired mailbox in the MAILBOX\_IRQENABLE\_SET\_u register, where u is the number of the sending user. However, direct allocation of a mailbox to a sender is not recommended because it can cause the sending processor to be constantly interrupted.

It is recommended that register polling be used to:

- Check the status of either the MAILBOX\_FIFOSTATUS\_m or MAILBOX\_MSGSTATUS\_m registers
- Write the message to the corresponding MAILBOX\_MESSAGE\_m register, if space is available

The sender might use the queue-not-full interrupt when the initial mailbox status check indicates the mailbox is full. In this case, the sender can enable the queue-not-full interrupt for its mailbox in the appropriate MAILBOX\_IRQENABLE\_SET\_u register. This allows the sender to be notified by interrupt only when a FIFO queue has at least one available entry.

Reading the MAILBOX\_IRQSTATUS\_CLR\_u register determines the status of the new message and the queue-not-full interrupts for a particular user. Writing 1 to the corresponding bit in the MAILBOX\_IRQSTATUS\_CLR\_u register acknowledges, and subsequently clears, an interrupt.

**CAUTION**

Assigning multiple senders or multiple receivers to the same mailbox is not recommended.

#### **17.1.1.6 Sending and Receiving Messages**

#### *17.1.1.6.1 Description*

When a 32-bit message is written to the MAILBOX\_MESSAGE\_m register, the message is appended into the FIFO queue. This queue holds four messages. If the queue is full, the message is discarded. Queue overflow can be avoided by first reading the MAILBOX\_FIFOSTATUS\_m register to check that the mailbox message queue is not full before writing a new message to it. Reading the MAILBOX\_MESSAGE\_m register returns the message at the beginning of the FIFO queue and removes it from the queue. If the FIFO queue is empty when the MAILBOX\_MESSAGE\_m register is read, the value 0 is returned. The new message interrupt is asserted when at least one message is in the mailbox message FIFO queue. To determine the number of messages in the mailbox message FIFO queue, read the MAILBOX\_MSGSTATUS\_m register.

#### **17.1.1.7 16-Bit Register Access**

#### *17.1.1.7.1 Description*

So that 16-bit processors can access the mailbox module, the module allows 16-bit register read and write access, with restrictions for the MAILBOX\_MESSAGE\_m registers. The 16-bit half-words are organized in little endian fashion; that is, the least-significant 16 bits are at the low address and the most-significant 16 bits are at the high address (low address + 0x02). All mailbox module registers can be read or written to directly using individual 16-bit accesses with no restriction on interleaving, except the MAILBOX\_MESSAGE\_m registers, which must always be accessed by either single 32-bit accesses or two consecutive 16-bit accesses.

**CAUTION**

When using 16-bit accesses to the MAILBOX\_MESSAGE\_m registers, the order of access must be the least-significant half-word first (low address) and the most-significant half-word last (high address). This requirement is because of the update operation by the message FIFO of the MAILBOX\_MSGSTATUS\_m registers. The update of the FIFO queue contents and the associated status registers and possible interrupt generation occurs only when the most-significant 16 bits of a MAILBOX\_MESSAGE\_m are accessed.

### *17.1.2 Programming Guide*

#### **17.1.2.1 Low-level Programming Models**

This section covers the low-level hardware programming sequences for configuration and usage of the mailbox module.

#### *17.1.2.1.1 Global Initialization*

#### *17.1.2.1.1.1 Surrounding Modules Global Initialization*

This section identifies the requirements of initializing the surrounding modules when the mailbox module is to be used for the first time after a device reset. This initialization of surrounding modules is based on the integration of the mailbox.

See [Integration](#page-2-1) for further information.

**Table 17-5. Global Initialization of Surrounding Modules for System Mailbox**

| Surrounding Modules   | Comments                                                                                              |
|-----------------------|-------------------------------------------------------------------------------------------------------|
| PRCM                  | Mailbox functional/interface clock must be enabled.                                                   |
| Interrupt Controllers | MPU interrupt controller must be configured to enable the<br>interrupt request generation to the MPU. |

#### *17.1.2.1.1.2 Mailbox Global Initialization*

#### *17.1.2.1.1.2.1 Main Sequence - Mailbox Global Initialization*

This procedure initializes the mailbox module after a power-on or software reset.

**Table 17-6. Mailbox Global Initialization**

|                              | Register/Bitfield/Programming Model  | Value |
|------------------------------|--------------------------------------|-------|
| Perform a software reset     | MAILBOX_SYSCONFIG[0].SOFTRESET       | 1     |
| Wait until reset is complete | MAILBOX_SYSCONFIG[0].SOFTRESET       | 0     |
| Set idle mode configuration  | MAILBOX_SYSCONFIG[3:2].SIDLEMODE | 0x    |

#### *17.1.2.1.2 Operational Modes Configuration*

#### *17.1.2.1.2.1 Main Sequence - Sending a Message (Polling Method)*

**Table 17-7. Sending a Message (Polling Method)**

| Step                                                 | Register/Bitfield/Programming Model         | Value |
|------------------------------------------------------|---------------------------------------------|-------|
| IF : Is FIFO full ?                                  | MAILBOX_FIFOSTATUS_m[0].FIFOFULLMB      | =1h   |
| Wait until at least one message slot is<br>available | MAILBOX_FIFOSTATUS_m[0].FIFOFULLMB      | =0h   |
| ELSE                                                 |                                             |       |
| Write message                                        | MAILBOX_MESSAGE_m[31:0].MESSAGEVALUEMBM | h     |
| ENDIF                                                |                                             |       |

#### *17.1.2.1.2.2 Main Sequence - Sending a Message (Interrupt Method)*

**Table 17-8. Sending a Message (Interrupt Method)**

| Step                                                              | Register/Bitfield/Programming Model         | Value |
|-------------------------------------------------------------------|---------------------------------------------|-------|
| IF : Is FIFO full ?                                               | MAILBOX_FIFOSTATUS_m[0].FIFOFULLMB      | =1h   |
| Enable interrupt event                                            | MAILBOX_IRQENABLE_SET_u[1+ m*2]             | 1h    |
| User(processor) can perform anothr task<br>until interrupt occurs |                                             |       |
| ELSE                                                              |                                             |       |
| Write message                                                     | MAILBOX_MESSAGE_m[31:0].MESSAGEVALUEMBM | h     |
| ENDIF                                                             |                                             |       |

#### *17.1.2.1.2.3 Main Sequence - Receiving a Message (Polling Method)*

**Table 17-9. Receiving a Message (Polling Method)**

| Step                                      | Register/Bitfield/Programming Model         | Value |
|-------------------------------------------|---------------------------------------------|-------|
| IF : Number of messages is not equal to 0 | MAILBOX_MSGSTATUS_m[2:0].NBOFMSGMB      | !=0h  |
| Read message                              | MAILBOX_MESSAGE_m[31:0].MESSAGEVALUEMBM | h     |
| ENDIF                                     |                                             |       |

#### *17.1.2.1.2.4 Main Sequence - Receiving a Message (Interrupt Method)*

**Table 17-10. Receiving a Message (Interrupt Method)**

| Step                                                           | Register/Bitfield/Programming Model | Value |
|----------------------------------------------------------------|-------------------------------------|-------|
| Enable interrupt event                                         | MAILBOX_IRQENABLE_SET_u[0 + m*2]    | 1h    |
| User(processor) can perform anothr task until interrupt occurs |                                     |       |

#### *17.1.2.1.3 Events Servicing*

#### *17.1.2.1.3.1 Sending Mode*

Table [17-11](#page-9-0) describes the events servicing in sending mode.

#### **Table 17-11. Events Servicing in Sending Mode**

| Step                             | Register/Bitfield/Programming Model         | Value |
|----------------------------------|---------------------------------------------|-------|
| Read interrupt status bit        | MAILBOX_IRQSTATUS_CLR_u[1 + m*2]            | 1     |
| Write message                    | MAILBOX_MESSAGE_m[31:0].MESSAGEVALUEMBM | h     |
| Write 1 to acknowledge interrupt | MAILBOX_IRQSTATUS_CLR_u[1 + m*2]            | 1     |

#### *17.1.2.1.3.2 Receiving Mode*

Table [17-12](#page-9-1) describes the events servicing in receiving mode.

#### **Table 17-12. Events Servicing in Receiving Mode**

| Step                                           | Register/Bitfield/Programming Model         | Value |
|------------------------------------------------|---------------------------------------------|-------|
| Read interrupt status bit                      | MAILBOX_IRQSTATUS_CLR_u[0 + m*2]            | 1     |
| IF : Number of messages is not equal to 0<br>? | MAILBOX_MSGSTATUS_m[2:0].NBOFMSGMB      | !=0h  |
| Read message                                   | MAILBOX_MESSAGE_m[31:0].MESSAGEVALUEMBM | h     |
| ELSE                                           |                                             |       |
| Write 1 to acknowledge interrupt               | MAILBOX_IRQSTATUS_CLR_u[0 + m*2]            | 1     |
| ENDIF                                          |                                             |       |

### *17.1.3 MAILBOX Registers*

Table [17-13](#page-10-0) lists the memory-mapped registers for the MAILBOX. All register offset addresses not listed in Table [17-13](#page-10-0) should be considered as reserved locations and the register contents should not be modified.

**Table 17-13. MAILBOX REGISTERS**

| Offset | Acronym      | Register Name                                                                                                              | Section           |
|--------|--------------|----------------------------------------------------------------------------------------------------------------------------|-------------------|
| 0h     | REVISION     | This register contains the IP revision code                                                                                | Section 17.1.3.1  |
| 10h    | SYSCONFIG    | This register controls the various parameters of the OCP<br>interface                                                      | Section 17.1.3.2  |
| 40h    | MESSAGE_0    | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.3  |
| 44h    | MESSAGE_1    | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.4  |
| 48h    | MESSAGE_2    | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.5  |
| 4Ch    | MESSAGE_3    | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.6  |
| 50h    | MESSAGE_4    | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.7  |
| 54h    | MESSAGE_5    | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.8  |
| 58h    | MESSAGE_6    | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.9  |
| 5Ch    | MESSAGE_7    | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.10 |
| 80h    | FIFOSTATUS_0 | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.11 |
| 84h    | FIFOSTATUS_1 | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.12 |
| 88h    | FIFOSTATUS_2 | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.13 |
| 8Ch    | FIFOSTATUS_3 | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.14 |
| 90h    | FIFOSTATUS_4 | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.15 |
| 94h    | FIFOSTATUS_5 | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.16 |
| 98h    | FIFOSTATUS_6 | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.17 |
| 9Ch    | FIFOSTATUS_7 | The message register stores the next to be read<br>message of the mailbox.<br>Reads remove the message from the FIFO queue | Section 17.1.3.18 |
| C0h    | MSGSTATUS_0  | The message status register has the status of the<br>messages in the mailbox                                               | Section 17.1.3.19 |
| C4h    | MSGSTATUS_1  | The message status register has the status of the<br>messages in the mailbox                                               | Section 17.1.3.20 |

**Table 17-13. MAILBOX REGISTERS (continued)**

| Offset | Acronym         | Register Name                                                                                                                                                                                                                                          | Section           |
|--------|-----------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------|
| C8h    | MSGSTATUS_2     | The message status register has the status of the<br>messages in the mailbox                                                                                                                                                                           | Section 17.1.3.21 |
| CCh    | MSGSTATUS_3     | The message status register has the status of the<br>messages in the mailbox                                                                                                                                                                           | Section 17.1.3.22 |
| D0h    | MSGSTATUS_4     | The message status register has the status of the<br>messages in the mailbox                                                                                                                                                                           | Section 17.1.3.23 |
| D4h    | MSGSTATUS_5     | The message status register has the status of the<br>messages in the mailbox                                                                                                                                                                           | Section 17.1.3.24 |
| D8h    | MSGSTATUS_6     | The message status register has the status of the<br>messages in the mailbox                                                                                                                                                                           | Section 17.1.3.25 |
| DCh    | MSGSTATUS_7     | The message status register has the status of the<br>messages in the mailbox                                                                                                                                                                           | Section 17.1.3.26 |
| 100h   | IRQSTATUS_RAW_0 | The interrupt status register has the status for each<br>event that may be responsible for the generation of an<br>interrupt to the corresponding user - write 1 to a given bit<br>resets this bit.<br>This register is mainly used for debug purpose. | Section 17.1.3.27 |
| 104h   | IRQSTATUS_CLR_0 | The interrupt status register has the status combined<br>with irq-enable for each event that may be responsible<br>for the generation of an interrupt to the corresponding<br>user - write 1 to a given bit resets this bit.                           | Section 17.1.3.28 |
| 108h   | IRQENABLE_SET_0 | The interrupt enable register enables to unmask the<br>module internal source of interrupt to the corresponding<br>user.<br>This register is write 1 to set.                                                                                           | Section 17.1.3.29 |
| 10Ch   | IRQENABLE_CLR_0 | The interrupt enable register enables to mask the<br>module internal source of interrupt to the corresponding<br>user.<br>This register is write 1 to clear.                                                                                           | Section 17.1.3.30 |
| 110h   | IRQSTATUS_RAW_1 | The interrupt status register has the status for each<br>event that may be responsible for the generation of an<br>interrupt to the corresponding user - write 1 to a given bit<br>resets this bit.<br>This register is mainly used for debug purpose. | Section 17.1.3.31 |
| 114h   | IRQSTATUS_CLR_1 | The interrupt status register has the status combined<br>with irq-enable for each event that may be responsible<br>for the generation of an interrupt to the corresponding<br>user - write 1 to a given bit resets this bit.                           | Section 17.1.3.32 |
| 118h   | IRQENABLE_SET_1 | The interrupt enable register enables to unmask the<br>module internal source of interrupt to the corresponding<br>user.<br>This register is write 1 to set.                                                                                           | Section 17.1.3.33 |
| 11Ch   | IRQENABLE_CLR_1 | The interrupt enable register enables to mask the<br>module internal source of interrupt to the corresponding<br>user.<br>This register is write 1 to clear.                                                                                           | Section 17.1.3.34 |
| 120h   | IRQSTATUS_RAW_2 | The interrupt status register has the status for each<br>event that may be responsible for the generation of an<br>interrupt to the corresponding user - write 1 to a given bit<br>resets this bit.<br>This register is mainly used for debug purpose. | Section 17.1.3.35 |
| 124h   | IRQSTATUS_CLR_2 | The interrupt status register has the status combined<br>with irq-enable for each event that may be responsible<br>for the generation of an interrupt to the corresponding<br>user - write 1 to a given bit resets this bit.                           | Section 17.1.3.36 |
| 128h   | IRQENABLE_SET_2 | The interrupt enable register enables to unmask the<br>module internal source of interrupt to the corresponding<br>user.<br>This register is write 1 to set.                                                                                           | Section 17.1.3.37 |

**Table 17-13. MAILBOX REGISTERS (continued)**

| Offset | Acronym         | Register Name                                                                                                                                                                                                                                          | Section           |
|--------|-----------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------|
| 12Ch   | IRQENABLE_CLR_2 | The interrupt enable register enables to mask the<br>module internal source of interrupt to the corresponding<br>user.<br>This register is write 1 to clear.                                                                                           | Section 17.1.3.38 |
| 130h   | IRQSTATUS_RAW_3 | The interrupt status register has the status for each<br>event that may be responsible for the generation of an<br>interrupt to the corresponding user - write 1 to a given bit<br>resets this bit.<br>This register is mainly used for debug purpose. | Section 17.1.3.39 |
| 134h   | IRQSTATUS_CLR_3 | The interrupt status register has the status combined<br>with irq-enable for each event that may be responsible<br>for the generation of an interrupt to the corresponding<br>user - write 1 to a given bit resets this bit.                           | Section 17.1.3.40 |
| 138h   | IRQENABLE_SET_3 | The interrupt enable register enables to unmask the<br>module internal source of interrupt to the corresponding<br>user.<br>This register is write 1 to set.                                                                                           | Section 17.1.3.41 |
| 13Ch   | IRQENABLE_CLR_3 | The interrupt enable register enables to mask the<br>module internal source of interrupt to the corresponding<br>user.<br>This register is write 1 to clear.                                                                                           | Section 17.1.3.42 |

#### **17.1.3.1 REVISION Register (offset = 0h) [reset = 400h]**

REVISION is shown in [Figure](#page-13-1) 17-2 and described in Table [17-14](#page-13-2).

This register contains the IP revision code

## **Figure 17-2. REVISION Register**

```
Bit:  31                               24
      +-----------+-----------+-----------+
      |  SCHEME   |    RES    |   FUNC    |
      |   R-0h    |   R-0h    |   R-0h    |
      +-----------+-----------+-----------+

Bit:  23                               16
      +---------------------------------+
      |              FUNC               |
      |              R-0h               |
      +---------------------------------+

Bit:  15            12             8
      +---------------------------+------+
      |            RTL            | MAJOR|
      |           R-0h            | R-4h |
      +---------------------------+------+

Bit:   7                     4     0
      +---------------------+-----------+
      |       Custom        |  MINOR    |
      |        R-10h        |  R-400h   |
      +---------------------+-----------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-14. REVISION Register Field Descriptions**

| Bit   | Field  | Type | Reset | Description       |
|-------|--------|------|-------|-------------------|
| 31-30 | SCHEME | R    | 0h    | Not defined yet   |
| 29-28 | RES    | R    | 0h    | Reserved          |
| 27-16 | FUNC   | R    | 0h    | Not defined yet   |
| 15-11 | RTL    | R    | 0h    | Not defined yet   |
| 10-8  | MAJOR  | R    | 4h    | IP-Major Revision |
| 7-6   | Custom | R    | 10h   | Not Defined Yet   |
| 5-0   | MINOR  | R    | 400h  | IP-Minor Revision |

#### **17.1.3.2 SYSCONFIG Register (offset = 10h) [reset = 8h]**

SYSCONFIG is shown in [Figure](#page-14-1) 17-3 and described in Table [17-15](#page-14-2).

This register controls the various parameters of the OCP interface

**Figure 17-3. SYSCONFIG Register**

```
Bit: 31                                      24
     +----------------------------------------+
     |               Reserved                 |
     |                R/W-0h                  |
     +----------------------------------------+

Bit: 23                                      16
     +----------------------------------------+
     |               Reserved                 |
     |                R/W-0h                  |
     +----------------------------------------+

Bit: 15                                      8
     +----------------------------------------+
     |               Reserved                 |
     |                R/W-0h                  |
     +----------------------------------------+

Bit: 7                 4   3        2     1      0
     +-------------------+----------+-----+-------+
     |     Reserved      | SIdleMode|Res|SoftReset|
     |      R/W-0h       |  R/W-2h  |R/W-4h|R/W-8h|
     +-------------------+----------+-----+-------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-15. SYSCONFIG Register Field Descriptions**

| Bit  | Field     | Type | Reset | Description                                                                                                                                                            |
|------|-----------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-4 | Reserved  | R/W  | 0h    | Write 0's for future compatibility Reads returns 0                                                                                                                     |
| 3-2  | SIdleMode | R/W  | 2h    |                                                                                                                                                                        |
| 1    | Reserved  | R/W  | 4h    | Write 0's for future compatibility Read returns 0                                                                                                                      |
| 0    | SoftReset | R/W  | 8h    | Software reset.<br>This bit is automatically reset by the hardware.<br>During reads, it always return 00 = Normal : Normal mode<br>1 = Reset : The module is reset |

#### **17.1.3.3 MESSAGE\_0 Register (offset = 40h) [reset = 0h]**

MESSAGE\_0 is shown in [Figure](#page-15-1) 17-4 and described in Table [17-16](#page-15-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-4. MESSAGE\_0 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

MESSAGEVALUEMBM

R/W-0

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-16. MESSAGE\_0 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |

#### **17.1.3.4 MESSAGE\_1 Register (offset = 44h) [reset = 0h]**

MESSAGE\_1 is shown in [Figure](#page-16-1) 17-5 and described in Table [17-17](#page-16-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-5. MESSAGE\_1 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

MESSAGEVALUEMBM

R/W-0

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-17. MESSAGE\_1 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |

#### **17.1.3.5 MESSAGE\_2 Register (offset = 48h) [reset = 0h]**

MESSAGE\_2 is shown in [Figure](#page-17-1) 17-6 and described in Table [17-18](#page-17-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-6. MESSAGE\_2 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

MESSAGEVALUEMBM

R/W-0

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-18. MESSAGE\_2 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |

#### **17.1.3.6 MESSAGE\_3 Register (offset = 4Ch) [reset = 0h]**

MESSAGE\_3 is shown in [Figure](#page-18-1) 17-7 and described in Table [17-19](#page-18-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-7. MESSAGE\_3 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

MESSAGEVALUEMBM

R/W-0

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-19. MESSAGE\_3 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |

#### **17.1.3.7 MESSAGE\_4 Register (offset = 50h) [reset = 0h]**

MESSAGE\_4 is shown in [Figure](#page-19-1) 17-8 and described in Table [17-20](#page-19-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-8. MESSAGE\_4 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

MESSAGEVALUEMBM

R/W-0

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-20. MESSAGE\_4 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |

#### **17.1.3.8 MESSAGE\_5 Register (offset = 54h) [reset = 0h]**

MESSAGE\_5 is shown in [Figure](#page-20-1) 17-9 and described in Table [17-21](#page-20-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-9. MESSAGE\_5 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

MESSAGEVALUEMBM

R/W-0

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-21. MESSAGE\_5 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |

#### **17.1.3.9 MESSAGE\_6 Register (offset = 58h) [reset = 0h]**

MESSAGE\_6 is shown in [Figure](#page-21-1) 17-10 and described in Table [17-22.](#page-21-2)

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-10. MESSAGE\_6 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

MESSAGEVALUEMBM

R/W-0

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-22. MESSAGE\_6 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |

#### **17.1.3.10 MESSAGE\_7 Register (offset = 5Ch) [reset = 0h]**

MESSAGE\_7 is shown in [Figure](#page-22-1) 17-11 and described in Table [17-23.](#page-22-2)

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-11. MESSAGE\_7 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

MESSAGEVALUEMBM

R/W-0

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-23. MESSAGE\_7 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |

#### **17.1.3.11 FIFOSTATUS\_0 Register (offset = 80h) [reset = 0h]**

FIFOSTATUS\_0 is shown in [Figure](#page-23-1) 17-12 and described in Table [17-24](#page-23-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-12. FIFOSTATUS\_0 Register**

| 31 | 30              | 29 | 28 | 27              | 26 | 25 | 24
|----|-----------------|----|----|-----------------|----|----|----
|    |                 |    |    | MESSAGEVALUEMBM |    |    |
|    | R/W-0           |    |    |                 |    |    |
| 23 | 22              | 21 | 20 | 19              | 18 | 17 | 16
|    | MESSAGEVALUEMBM |    |    |                 |    |    |
|    | R/W-0           |    |    |                 |    |    |
| 15 | 14              | 13 | 12 | 11              | 10 | 9  | 8
|    |                 |    |    | MESSAGEVALUEMBM |    |    |
|    |                 |    |    | R/W-0           |    |    |
| 7  | 6               | 5  | 4  | 3               | 2  | 1  | 0
|    | MESSAGEVALUEMBM |    |    |                 |    |    |
|    | R/W-0           |    |    |                 |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-24. FIFOSTATUS\_0 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |
| 0    | FIFOFULLMBM     | R-0   | 0     | Full flag for Mailbox<br>0 = NotFull : Mailbox FIFO is not full<br>1 = Full : Mailbox FIFO is full                                                 |

#### **17.1.3.12 FIFOSTATUS\_1 Register (offset = 84h) [reset = 0h]**

FIFOSTATUS\_1 is shown in [Figure](#page-24-1) 17-13 and described in Table [17-25](#page-24-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-13. FIFOSTATUS\_1 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |               R/W-0h                 |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |               R/W-0h                 |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |               R/W-0h                 |
     +--------------------------------------+

Bit: 7           4   3          2     1      0
     +-----------+---+----------+-----+-------+
     | Reserved  |   | SIdleMode|Res. |SoftRst|
     |  R/W-0h   |   |  R/W-2h  |R/W-4h|R/W-8h|
     +-----------+---+----------+-----+-------+
```
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-25. FIFOSTATUS\_1 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |
| 0    | FIFOFULLMBM     | R-0   | 0     | Full flag for Mailbox<br>0 = NotFull : Mailbox FIFO is not full<br>1 = Full : Mailbox FIFO is full                                                 |

#### **17.1.3.13 FIFOSTATUS\_2 Register (offset = 88h) [reset = 0h]**

FIFOSTATUS\_2 is shown in [Figure](#page-25-1) 17-14 and described in Table [17-26](#page-25-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-14. FIFOSTATUS\_2 Register**

| 31 | 30 | 29 | 28              | 27              | 26 | 25 | 24          |
|----|----|----|-----------------|-----------------|----|----|-------------|
|    |    |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |    |    |                 | R/W-0           |    |    |             |
| 23 | 22 | 21 | 20              | 19              | 18 | 17 | 16          |
|    |    |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |    |    |                 | R/W-0           |    |    |             |
| 15 | 14 | 13 | 12              | 11              | 10 | 9  | 8           |
|    |    |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |    |    |                 | R/W-0           |    |    |             |
| 7  | 6  | 5  | 4               | 3               | 2  | 1  | 0           |
|    |    |    | MESSAGEVALUEMBM |                 |    |    | FIFOFULLMBM |
|    |    |    | R/W-0           |                 |    |    | R-0         |
|    |    |    |                 |                 |    |    |             |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-26. FIFOSTATUS\_2 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |
| 0    | FIFOFULLMBM     | R-0   | 0     | Full flag for Mailbox<br>0 = NotFull : Mailbox FIFO is not full<br>1 = Full : Mailbox FIFO is full                                                 |

#### **17.1.3.14 FIFOSTATUS\_3 Register (offset = 8Ch) [reset = 0h]**

FIFOSTATUS\_3 is shown in [Figure](#page-26-1) 17-15 and described in Table [17-27](#page-26-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-15. FIFOSTATUS\_3 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |            MESSAGEVALUEMBM           |
     |                 R/W-0                |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |            MESSAGEVALUEMBM           |
     |                 R/W-0                |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |            MESSAGEVALUEMBM           |
     |                 R/W-0                |
     +--------------------------------------+

Bit: 7             4   1                 0
     +-------------+----------------------+
     | MESSAGEVALUEMBM |   FIFOFULLMBM    |
     |      R/W-0      |       R-0        |
     +-----------------+------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-27. FIFOSTATUS\_3 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |
| 0    | FIFOFULLMBM     | R-0   | 0     | Full flag for Mailbox<br>0 = NotFull : Mailbox FIFO is not full<br>1 = Full : Mailbox FIFO is full                                                 |

#### **17.1.3.15 FIFOSTATUS\_4 Register (offset = 90h) [reset = 0h]**

FIFOSTATUS\_4 is shown in [Figure](#page-27-1) 17-16 and described in Table [17-28](#page-27-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-16. FIFOSTATUS\_4 Register**

| 31 | 30              | 29 | 28              | 27              | 26 | 25 | 24          |
|----|-----------------|----|-----------------|-----------------|----|----|-------------|--
|    | MESSAGEVALUEMBM |    |                 |                 |    |    |             |
|    |                 |    |                 | R/W-0           |    |    |             |
| 23 | 22              | 21 | 20              | 19              | 18 | 17 | 16          |
|    |                 |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |                 |    |                 | R/W-0           |    |    |             |
| 15 | 14              | 13 | 12              | 11              | 10 | 9  | 8           |
|    |                 |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |                 |    |                 | R/W-0           |    |    |             |
| 7  | 6               | 5  | 4               | 3               | 2  | 1  | 0           |
|    |                 |    | MESSAGEVALUEMBM |                 |    |    | FIFOFULLMBM |
|    |                 |    | R/W-0           |                 |    |    | R-0         |
|    |                 |    |                 |                 |    |    |             |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-28. FIFOSTATUS\_4 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |
| 0    | FIFOFULLMBM     | R-0   | 0     | Full flag for Mailbox<br>0 = NotFull : Mailbox FIFO is not full<br>1 = Full : Mailbox FIFO is full                                                 |

#### **17.1.3.16 FIFOSTATUS\_5 Register (offset = 94h) [reset = 0h]**

FIFOSTATUS\_5 is shown in [Figure](#page-28-1) 17-17 and described in Table [17-29](#page-28-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-17. FIFOSTATUS\_5 Register**

| 31 | 30 | 29 | 28              | 27              | 26 | 25 | 24          |
|----|----|----|-----------------|-----------------|----|----|-------------|
|    |    |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |    |    |                 | R/W-0           |    |    |             |
| 23 | 22 | 21 | 20              | 19              | 18 | 17 | 16          |
|    |    |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |    |    |                 | R/W-0           |    |    |             |
| 15 | 14 | 13 | 12              | 11              | 10 | 9  | 8           |
|    |    |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |    |    |                 | R/W-0           |    |    |             |
| 7  | 6  | 5  | 4               | 3               | 2  | 1  | 0           |
|    |    |    | MESSAGEVALUEMBM |                 |    |    | FIFOFULLMBM |
|    |    |    | R/W-0           |                 |    |    | R-0         |
|    |    |    |                 |                 |    |    |             |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-29. FIFOSTATUS\_5 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |
| 0    | FIFOFULLMBM     | R-0   | 0     | Full flag for Mailbox<br>0 = NotFull : Mailbox FIFO is not full<br>1 = Full : Mailbox FIFO is full                                                 |

#### **17.1.3.17 FIFOSTATUS\_6 Register (offset = 98h) [reset = 0h]**

FIFOSTATUS\_6 is shown in [Figure](#page-29-1) 17-18 and described in Table [17-30](#page-29-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-18. FIFOSTATUS\_6 Register**

| 31 | 30              | 29 | 28              | 27              | 26 | 25 | 24          |
|----|-----------------|----|-----------------|-----------------|----|----|-------------|--
|    | MESSAGEVALUEMBM |    |                 |                 |    |    |             |
|    |                 |    |                 | R/W-0           |    |    |             |
| 23 | 22              | 21 | 20              | 19              | 18 | 17 | 16          |
|    |                 |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |                 |    |                 | R/W-0           |    |    |             |
| 15 | 14              | 13 | 12              | 11              | 10 | 9  | 8           |
|    |                 |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |                 |    |                 | R/W-0           |    |    |             |
| 7  | 6               | 5  | 4               | 3               | 2  | 1  | 0           |
|    |                 |    | MESSAGEVALUEMBM |                 |    |    | FIFOFULLMBM |
|    |                 |    | R/W-0           |                 |    |    | R-0         |
|    |                 |    |                 |                 |    |    |             |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-30. FIFOSTATUS\_6 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |
| 0    | FIFOFULLMBM     | R-0   | 0     | Full flag for Mailbox<br>0 = NotFull : Mailbox FIFO is not full<br>1 = Full : Mailbox FIFO is full                                                 |

#### **17.1.3.18 FIFOSTATUS\_7 Register (offset = 9Ch) [reset = 0h]**

FIFOSTATUS\_7 is shown in [Figure](#page-30-1) 17-19 and described in Table [17-31](#page-30-2).

The message register stores the next to be read message of the mailbox. Reads remove the message from the FIFO queue

**Figure 17-19. FIFOSTATUS\_7 Register**

| 31 | 30 | 29 | 28              | 27              | 26 | 25 | 24          |
|----|----|----|-----------------|-----------------|----|----|-------------|
|    |    |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |    |    |                 | R/W-0           |    |    |             |
| 23 | 22 | 21 | 20              | 19              | 18 | 17 | 16          |
|    |    |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |    |    |                 | R/W-0           |    |    |             |
| 15 | 14 | 13 | 12              | 11              | 10 | 9  | 8           |
|    |    |    |                 | MESSAGEVALUEMBM |    |    |             |
|    |    |    |                 | R/W-0           |    |    |             |
| 7  | 6  | 5  | 4               | 3               | 2  | 1  | 0           |
|    |    |    | MESSAGEVALUEMBM |                 |    |    | FIFOFULLMBM |
|    |    |    | R/W-0           |                 |    |    | R-0         |
|    |    |    |                 |                 |    |    |             |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-31. FIFOSTATUS\_7 Register Field Descriptions**

| Bit  | Field           | Type  | Reset | Description                                                                                                                                        |
|------|-----------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | MESSAGEVALUEMBM | R/W-0 | 0     | Message in Mailbox.<br>The message register stores the next to be read message of the<br>mailbox.<br>Reads remove the message from the FIFO queue. |
| 0    | FIFOFULLMBM     | R-0   | 0     | Full flag for Mailbox<br>0 = NotFull : Mailbox FIFO is not full<br>1 = Full : Mailbox FIFO is full                                                 |

#### **17.1.3.19 MSGSTATUS\_0 Register (offset = C0h) [reset = 0h]**

MSGSTATUS\_0 is shown in [Figure](#page-31-1) 17-20 and described in Table [17-32](#page-31-2).

The message status register has the status of the messages in the mailbox

#### **Figure 17-20. MSGSTATUS\_0 Register**

```
Bits: 31                                                         1  0
      +------------------------------------------------------------+----+
      |                        Reserved                            |NBOF|
      |                                                            |MSG |
      |                                                            |MBM |
      |                                                            |R-0h|
      +------------------------------------------------------------+----+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-32. MSGSTATUS\_0 Register Field Descriptions**

| Bit | Field      | Type | Reset | Description                                                                    |
|-----|------------|------|-------|--------------------------------------------------------------------------------|
| 2-0 | NBOFMSGMBM | R    | 0h    | Number of unread messages in Mailbox.<br>Limited to four messages per mailbox. |

#### **17.1.3.20 MSGSTATUS\_1 Register (offset = C4h) [reset = 0h]**

MSGSTATUS\_1 is shown in [Figure](#page-32-1) 17-21 and described in Table [17-33](#page-32-2).

The message status register has the status of the messages in the mailbox

#### **Figure 17-21. MSGSTATUS\_1 Register**

```
Bits: 31                                                         1  0
      +------------------------------------------------------------+----+
      |                        Reserved                            |NBOF|
      |                                                            |MSG |
      |                                                            |MBM |
      |                                                            |R-0h|
      +------------------------------------------------------------+----+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-33. MSGSTATUS\_1 Register Field Descriptions**

| Bit | Field      | Type | Reset | Description                                                                    |
|-----|------------|------|-------|--------------------------------------------------------------------------------|
| 2-0 | NBOFMSGMBM | R    | 0h    | Number of unread messages in Mailbox.<br>Limited to four messages per mailbox. |

#### **17.1.3.21 MSGSTATUS\_2 Register (offset = C8h) [reset = 0h]**

MSGSTATUS\_2 is shown in [Figure](#page-33-1) 17-22 and described in Table [17-34](#page-33-2).

The message status register has the status of the messages in the mailbox

#### **Figure 17-22. MSGSTATUS\_2 Register**

```
Bits: 31                                                         1  0
      +------------------------------------------------------------+----+
      |                        Reserved                            |NBOF|
      |                                                            |MSG |
      |                                                            |MBM |
      |                                                            |R-0h|
      +------------------------------------------------------------+----+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-34. MSGSTATUS\_2 Register Field Descriptions**

| Bit | Field      | Type | Reset | Description                                                                    |
|-----|------------|------|-------|--------------------------------------------------------------------------------|
| 2-0 | NBOFMSGMBM | R    | 0h    | Number of unread messages in Mailbox.<br>Limited to four messages per mailbox. |

#### **17.1.3.22 MSGSTATUS\_3 Register (offset = CCh) [reset = 0h]**

MSGSTATUS\_3 is shown in [Figure](#page-34-1) 17-23 and described in Table [17-35](#page-34-2).

The message status register has the status of the messages in the mailbox

#### **Figure 17-23. MSGSTATUS\_3 Register**

```
Bits: 31                                                           1    0
      +------------------------------------------------------------+----+
      |                        Reserved                            |NBOF|
      |                                                            |MSG |
      |                                                            |MBM |
      |                                                            |R-0h|
      +------------------------------------------------------------+----+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-35. MSGSTATUS\_3 Register Field Descriptions**

| Bit | Field      | Type | Reset<br>Description |                                                                                |
|-----|------------|------|----------------------|--------------------------------------------------------------------------------|
| 2-0 | NBOFMSGMBM | R    | 0h                   | Number of unread messages in Mailbox.<br>Limited to four messages per mailbox. |

#### **17.1.3.23 MSGSTATUS\_4 Register (offset = D0h) [reset = 0h]**

MSGSTATUS\_4 is shown in [Figure](#page-35-1) 17-24 and described in Table [17-36](#page-35-2).

The message status register has the status of the messages in the mailbox

#### **Figure 17-24. MSGSTATUS\_4 Register**

```
Bits: 31                                                         1  0
      +------------------------------------------------------------+----+
      |                        Reserved                            |NBOF|
      |                                                            |MSG |
      |                                                            |MBM |
      |                                                            |R-0h|
      +------------------------------------------------------------+----+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-36. MSGSTATUS\_4 Register Field Descriptions**

| Bit | Field      | Type | Reset | Description                                                                    |
|-----|------------|------|-------|--------------------------------------------------------------------------------|
| 2-0 | NBOFMSGMBM | R    | 0h    | Number of unread messages in Mailbox.<br>Limited to four messages per mailbox. |

#### **17.1.3.24 MSGSTATUS\_5 Register (offset = D4h) [reset = 0h]**

MSGSTATUS\_5 is shown in [Figure](#page-36-1) 17-25 and described in Table [17-37](#page-36-2).

The message status register has the status of the messages in the mailbox

#### **Figure 17-25. MSGSTATUS\_5 Register**

```
Bits: 31                                                         1  0
      +------------------------------------------------------------+----+
      |                        Reserved                            |NBOF|
      |                                                            |MSG |
      |                                                            |MBM |
      |                                                            |R-0h|
      +------------------------------------------------------------+----+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-37. MSGSTATUS\_5 Register Field Descriptions**

| Bit | Field      | Type | Reset | Description                                                                    |
|-----|------------|------|-------|--------------------------------------------------------------------------------|
| 2-0 | NBOFMSGMBM | R    | 0h    | Number of unread messages in Mailbox.<br>Limited to four messages per mailbox. |

#### **17.1.3.25 MSGSTATUS\_6 Register (offset = D8h) [reset = 0h]**

MSGSTATUS\_6 is shown in [Figure](#page-37-1) 17-26 and described in Table [17-38](#page-37-2).

The message status register has the status of the messages in the mailbox

#### **Figure 17-26. MSGSTATUS\_6 Register**

```
Bits: 31                                                         1  0
      +------------------------------------------------------------+----+
      |                        Reserved                            |NBOF|
      |                                                            |MSG |
      |                                                            |MBM |
      |                                                            |R-0h|
      +------------------------------------------------------------+----+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-38. MSGSTATUS\_6 Register Field Descriptions**

| Bit | Field      | Type | Reset | Description                                                                    |
|-----|------------|------|-------|--------------------------------------------------------------------------------|
| 2-0 | NBOFMSGMBM | R    | 0h    | Number of unread messages in Mailbox.<br>Limited to four messages per mailbox. |

#### **17.1.3.26 MSGSTATUS\_7 Register (offset = DCh) [reset = 0h]**

MSGSTATUS\_7 is shown in [Figure](#page-38-1) 17-27 and described in Table [17-39](#page-38-2).

The message status register has the status of the messages in the mailbox

#### **Figure 17-27. MSGSTATUS\_7 Register**

```
Bits: 31                                                         1  0
      +------------------------------------------------------------+----+
      |                        Reserved                            |NBOF|
      |                                                            |MSG |
      |                                                            |MBM |
      |                                                            |R-0h|
      +------------------------------------------------------------+----+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-39. MSGSTATUS\_7 Register Field Descriptions**

| Bit | Field      | Type | Reset | Description                                                                    |
|-----|------------|------|-------|--------------------------------------------------------------------------------|
| 2-0 | NBOFMSGMBM | R    | 0h    | Number of unread messages in Mailbox.<br>Limited to four messages per mailbox. |

#### **17.1.3.27 IRQSTATUS\_RAW\_0 Register (offset = 100h) [reset = 0h]**

IRQSTATUS\_RAW\_0 is shown in [Figure](#page-39-1) 17-28 and described in Table [17-40](#page-39-2).

The interrupt status register has the status for each event that may be responsible for the generation of an interrupt to the corresponding user - write 1 to a given bit resets this bit. This register is mainly used for debug purpose.

**Figure 17-28. IRQSTATUS\_RAW\_0 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|
|                        |                       |                        |                       | Reserved               |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        |                       |                        |                       | Reserved               |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-40. IRQSTATUS\_RAW\_0 Register Field Descriptions**

| Bit | Field              | Type | Reset                                                                                                                      | Description                                                                                                          |
|-----|--------------------|------|----------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h<br>New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |                                                                                                                      |
| 13  | NotFullStatusUuMB6 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-40. IRQSTATUS\_RAW\_0 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.28 IRQSTATUS\_CLR\_0 Register (offset = 104h) [reset = 0h]**

IRQSTATUS\_CLR\_0 is shown in [Figure](#page-41-1) 17-29 and described in Table [17-41.](#page-41-2)

The interrupt status register has the status combined with irq-enable for each event that may be responsible for the generation of an interrupt to the corresponding user - write 1 to a given bit resets this bit.

## **Figure 17-29. IRQSTATUS\_CLR\_0 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|
|                        |                       |                        | Reserved              |                        |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        |                       |                        | Reserved              |                        |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-41. IRQSTATUS\_CLR\_0 Register Field Descriptions**

| Bit | Field              | Type | Reset                                                                                                                      | Description                                                                                                          |
|-----|--------------------|------|----------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h<br>New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |                                                                                                                      |
| 13  | NotFullStatusUuMB6 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-41. IRQSTATUS\_CLR\_0 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.29 IRQENABLE\_SET\_0 Register (offset = 108h) [reset = 0h]**

IRQENABLE\_SET\_0 is shown in [Figure](#page-43-1) 17-30 and described in Table [17-42](#page-43-2).

The interrupt enable register enables to unmask the module internal source of interrupt to the corresponding user. This register is write 1 to set.

**Figure 17-30. IRQENABLE\_SET\_0 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|--
|                        |                       |                        |                       | Reserved               |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-42. IRQENABLE\_SET\_0 Register Field Descriptions**

| Bit | Field              | Type | Reset                                                                                                                      | Description                                                                                                          |
|-----|--------------------|------|----------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h<br>New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |                                                                                                                      |
| 13  | NotFullStatusUuMB6 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-42. IRQENABLE\_SET\_0 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.30 IRQENABLE\_CLR\_0 Register (offset = 10Ch) [reset = 0h]**

IRQENABLE\_CLR\_0 is shown in [Figure](#page-45-1) 17-31 and described in Table [17-43](#page-45-2).

The interrupt enable register enables to mask the module internal source of interrupt to the corresponding user. This register is write 1 to clear.

**Figure 17-31. IRQENABLE\_CLR\_0 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|--
|                        |                       |                        | Reserved              |                        |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-43. IRQENABLE\_CLR\_0 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-43. IRQENABLE\_CLR\_0 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.31 IRQSTATUS\_RAW\_1 Register (offset = 110h) [reset = 0h]**

IRQSTATUS\_RAW\_1 is shown in [Figure](#page-47-1) 17-32 and described in Table [17-44](#page-47-2).

The interrupt status register has the status for each event that may be responsible for the generation of an interrupt to the corresponding user - write 1 to a given bit resets this bit. This register is mainly used for debug purpose.

**Figure 17-32. IRQSTATUS\_RAW\_1 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|--
|                        |                       |                        |                       | Reserved               |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-44. IRQSTATUS\_RAW\_1 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-44. IRQSTATUS\_RAW\_1 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.32 IRQSTATUS\_CLR\_1 Register (offset = 114h) [reset = 0h]**

IRQSTATUS\_CLR\_1 is shown in [Figure](#page-49-1) 17-33 and described in Table [17-45.](#page-49-2)

The interrupt status register has the status combined with irq-enable for each event that may be responsible for the generation of an interrupt to the corresponding user - write 1 to a given bit resets this bit.

## **Figure 17-33. IRQSTATUS\_CLR\_1 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|--|--
|                        | Reserved              |                        |                       |                        |                       |                        |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16
|                        | Reserved              |                        |                       |                        |                       |                        |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-45. IRQSTATUS\_CLR\_1 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-45. IRQSTATUS\_CLR\_1 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.33 IRQENABLE\_SET\_1 Register (offset = 118h) [reset = 0h]**

IRQENABLE\_SET\_1 is shown in [Figure](#page-51-1) 17-34 and described in Table [17-46](#page-51-2).

The interrupt enable register enables to unmask the module internal source of interrupt to the corresponding user. This register is write 1 to set.

**Figure 17-34. IRQENABLE\_SET\_1 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|--
|                        |                       |                        | Reserved              |                        |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-46. IRQENABLE\_SET\_1 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                                           |
|-----|--------------------|------|-------|-----------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 70 = NoAction : No action |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 7                          |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 6                             |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 6                          |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 5                             |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 5                          |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 4                             |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 4                          |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 3                             |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |

**Table 17-46. IRQENABLE\_SET\_1 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.34 IRQENABLE\_CLR\_1 Register (offset = 11Ch) [reset = 0h]**

IRQENABLE\_CLR\_1 is shown in [Figure](#page-53-1) 17-35 and described in Table [17-47](#page-53-2).

The interrupt enable register enables to mask the module internal source of interrupt to the corresponding user. This register is write 1 to clear.

**Figure 17-35. IRQENABLE\_CLR\_1 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|--|--
| Reserved               |                       |                        |                       |                        |                       |                        |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16
| Reserved               |                       |                        |                       |                        |                       |                        |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-47. IRQENABLE\_CLR\_1 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-47. IRQENABLE\_CLR\_1 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.35 IRQSTATUS\_RAW\_2 Register (offset = 120h) [reset = 0h]**

IRQSTATUS\_RAW\_2 is shown in [Figure](#page-55-1) 17-36 and described in Table [17-48](#page-55-2).

The interrupt status register has the status for each event that may be responsible for the generation of an interrupt to the corresponding user - write 1 to a given bit resets this bit. This register is mainly used for debug purpose.

**Figure 17-36. IRQSTATUS\_RAW\_2 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|--|--|--
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-48. IRQSTATUS\_RAW\_2 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-48. IRQSTATUS\_RAW\_2 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.36 IRQSTATUS\_CLR\_2 Register (offset = 124h) [reset = 0h]**

IRQSTATUS\_CLR\_2 is shown in [Figure](#page-57-1) 17-37 and described in Table [17-49.](#page-57-2)

The interrupt status register has the status combined with irq-enable for each event that may be responsible for the generation of an interrupt to the corresponding user - write 1 to a given bit resets this bit.

## **Figure 17-37. IRQSTATUS\_CLR\_2 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|--|--|--
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-49. IRQSTATUS\_CLR\_2 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-49. IRQSTATUS\_CLR\_2 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.37 IRQENABLE\_SET\_2 Register (offset = 128h) [reset = 0h]**

IRQENABLE\_SET\_2 is shown in [Figure](#page-59-1) 17-38 and described in Table [17-50](#page-59-2).

The interrupt enable register enables to unmask the module internal source of interrupt to the corresponding user. This register is write 1 to set.

**Figure 17-38. IRQENABLE\_SET\_2 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|--|--|--
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-50. IRQENABLE\_SET\_2 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-50. IRQENABLE\_SET\_2 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.38 IRQENABLE\_CLR\_2 Register (offset = 12Ch) [reset = 0h]**

IRQENABLE\_CLR\_2 is shown in [Figure](#page-61-1) 17-39 and described in Table [17-51](#page-61-2).

The interrupt enable register enables to mask the module internal source of interrupt to the corresponding user. This register is write 1 to clear.

**Figure 17-39. IRQENABLE\_CLR\_2 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|--|--|--
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        | Reserved              |                        |                       |                        |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-51. IRQENABLE\_CLR\_2 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                                           |
|-----|--------------------|------|-------|-----------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 70 = NoAction : No action |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 7                          |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 6                             |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 6                          |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 5                             |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 5                          |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 4                             |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 4                          |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 3                             |
|     |                    |      |       | 0 = NoAction : No action                                              |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)                              |

**Table 17-51. IRQENABLE\_CLR\_2 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.39 IRQSTATUS\_RAW\_3 Register (offset = 130h) [reset = 0h]**

IRQSTATUS\_RAW\_3 is shown in [Figure](#page-63-1) 17-40 and described in Table [17-52](#page-63-2).

The interrupt status register has the status for each event that may be responsible for the generation of an interrupt to the corresponding user - write 1 to a given bit resets this bit. This register is mainly used for debug purpose.

**Figure 17-40. IRQSTATUS\_RAW\_3 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|
|                        |                       |                        |                       | Reserved               |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        |                       |                        |                       | Reserved               |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-52. IRQSTATUS\_RAW\_3 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-52. IRQSTATUS\_RAW\_3 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.40 IRQSTATUS\_CLR\_3 Register (offset = 134h) [reset = 0h]**

IRQSTATUS\_CLR\_3 is shown in [Figure](#page-65-1) 17-41 and described in Table [17-53.](#page-65-2)

The interrupt status register has the status combined with irq-enable for each event that may be responsible for the generation of an interrupt to the corresponding user - write 1 to a given bit resets this bit.

## **Figure 17-41. IRQSTATUS\_CLR\_3 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|
|                        |                       |                        | Reserved              |                        |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        |                       |                        | Reserved              |                        |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-53. IRQSTATUS\_CLR\_3 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-53. IRQSTATUS\_CLR\_3 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.41 IRQENABLE\_SET\_3 Register (offset = 138h) [reset = 0h]**

IRQENABLE\_SET\_3 is shown in [Figure](#page-67-1) 17-42 and described in Table [17-54](#page-67-2).

The interrupt enable register enables to unmask the module internal source of interrupt to the corresponding user. This register is write 1 to set.

**Figure 17-42. IRQENABLE\_SET\_3 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|
|                        |                       |                        | Reserved              |                        |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        |                       |                        | Reserved              |                        |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-54. IRQENABLE\_SET\_3 Register Field Descriptions**

| Bit | Field              | Type | Reset                                                                                                                      | Description                                                                                                          |
|-----|--------------------|------|----------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h<br>New Message Status bit for User u, Mailbox 70 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |                                                                                                                      |
| 13  | NotFullStatusUuMB6 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 60 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 11  | NotFullStatusUuMB5 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 50 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 9   | NotFullStatusUuMB4 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h                                                                                                                         | New Message Status bit for User u, Mailbox 40 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 7   | NotFullStatusUuMB3 | R/W  | 0h                                                                                                                         | Not Full Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |

**Table 17-54. IRQENABLE\_SET\_3 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

#### **17.1.3.42 IRQENABLE\_CLR\_3 Register (offset = 13Ch) [reset = 0h]**

IRQENABLE\_CLR\_3 is shown in [Figure](#page-69-1) 17-43 and described in Table [17-55](#page-69-2).

The interrupt enable register enables to mask the module internal source of interrupt to the corresponding user. This register is write 1 to clear.

**Figure 17-43. IRQENABLE\_CLR\_3 Register**

| 31                     | 30                    | 29                     | 28                    | 27                     | 26                    | 25                     | 24                    |
|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|------------------------|-----------------------|
|                        |                       |                        |                       | Reserved               |                       |                        |                       |
| 23                     | 22                    | 21                     | 20                    | 19                     | 18                    | 17                     | 16                    |
|                        |                       |                        |                       | Reserved               |                       |                        |                       |
| 15                     | 14                    | 13                     | 12                    | 11                     | 10                    | 9                      | 8                     |
| NotFullStatusU<br>uMB7 | NewMSGStatus<br>UuMB7 | NotFullStatusU<br>uMB6 | NewMSGStatus<br>UuMB6 | NotFullStatusU<br>uMB5 | NewMSGStatus<br>UuMB5 | NotFullStatusU<br>uMB4 | NewMSGStatus<br>UuMB4 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |
| 7                      | 6                     | 5                      | 4                     | 3                      | 2                     | 1                      | 0                     |
| NotFullStatusU<br>uMB3 | NewMSGStatus<br>UuMB3 | NotFullStatusU<br>uMB2 | NewMSGStatus<br>UuMB2 | NotFullStatusU<br>uMB1 | NewMSGStatus<br>UuMB1 | NotFullStatusU<br>uMB0 | NewMSGStatus<br>UuMB0 |
| R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                | R/W-0h                 | R/W-0h                |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 17-55. IRQENABLE\_CLR\_3 Register Field Descriptions**

| Bit | Field              | Type | Reset | Description                                  |
|-----|--------------------|------|-------|----------------------------------------------|
| 15  | NotFullStatusUuMB7 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 7    |
|     |                    |      |       | 0 = NoAction : No action                     |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)     |
| 14  | NewMSGStatusUuMB7  | R/W  | 0h    | New Message Status bit for User u, Mailbox 7 |
|     |                    |      |       | 0 = NoAction : No action                     |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)     |
| 13  | NotFullStatusUuMB6 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 6    |
|     |                    |      |       | 0 = NoAction : No action                     |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)     |
| 12  | NewMSGStatusUuMB6  | R/W  | 0h    | New Message Status bit for User u, Mailbox 6 |
|     |                    |      |       | 0 = NoAction : No action                     |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)     |
| 11  | NotFullStatusUuMB5 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 5    |
|     |                    |      |       | 0 = NoAction : No action                     |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)     |
| 10  | NewMSGStatusUuMB5  | R/W  | 0h    | New Message Status bit for User u, Mailbox 5 |
|     |                    |      |       | 0 = NoAction : No action                     |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)     |
| 9   | NotFullStatusUuMB4 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 4    |
|     |                    |      |       | 0 = NoAction : No action                     |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)     |
| 8   | NewMSGStatusUuMB4  | R/W  | 0h    | New Message Status bit for User u, Mailbox 4 |
|     |                    |      |       | 0 = NoAction : No action                     |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)     |
| 7   | NotFullStatusUuMB3 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 3    |
|     |                    |      |       | 0 = NoAction : No action                     |
|     |                    |      |       | 1 = SetEvent : Set the event (for debug)     |
|     |                    |      |       |                                              |

**Table 17-55. IRQENABLE\_CLR\_3 Register Field Descriptions (continued)**

| Bit | Field              | Type | Reset | Description                                                                                                          |
|-----|--------------------|------|-------|----------------------------------------------------------------------------------------------------------------------|
| 6   | NewMSGStatusUuMB3  | R/W  | 0h    | New Message Status bit for User u, Mailbox 30 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 5   | NotFullStatusUuMB2 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 4   | NewMSGStatusUuMB2  | R/W  | 0h    | New Message Status bit for User u, Mailbox 20 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 3   | NotFullStatusUuMB1 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 2   | NewMSGStatusUuMB1  | R/W  | 0h    | New Message Status bit for User u, Mailbox 10 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |
| 1   | NotFullStatusUuMB0 | R/W  | 0h    | Not Full Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug)    |
| 0   | NewMSGStatusUuMB0  | R/W  | 0h    | New Message Status bit for User u, Mailbox 00 = NoAction : No action<br>1 = SetEvent : Set the event (for debug) |

## **17.2 Spinlock**

### *17.2.1 SPINLOCK Registers*

Table [17-56](#page-71-1) lists the memory-mapped registers for the SPINLOCK. All register offset addresses not listed in Table [17-56](#page-71-1) should be considered as reserved locations and the register contents should not be modified.

**Table 17-56. SPINLOCK REGISTERS**

| Offset | Acronym    | Register Name                                                                                                                                                                                                                                                                  | Section           |
|--------|------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------|
| 0h     | REV        | Read-only IP revision identifier (X.Y.R) used by software<br>to determine features, bugs and compatibility of an<br>instance of this the Spin Lock module.                                                                                                                     | Section 17.2.1.1  |
| 10h    | SYSCONFIG  | This register controls the various parameters of the OCP<br>interface.<br>Note that several fields are present by read-only.                                                                                                                                                   | Section 17.2.1.2  |
| 14h    | SYSTATUS   | This register provides status information about this<br>instance of the Spin Lock module.                                                                                                                                                                                      | Section 17.2.1.3  |
| 800h   | LOCK_REG_0 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.4  |
| 804h   | LOCK_REG_1 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.5  |
| 808h   | LOCK_REG_2 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.6  |
| 80Ch   | LOCK_REG_3 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.7  |
| 810h   | LOCK_REG_4 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.8  |
| 814h   | LOCK_REG_5 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.9  |
| 818h   | LOCK_REG_6 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.10 |
| 81Ch   | LOCK_REG_7 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.11 |
| 820h   | LOCK_REG_8 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.12 |

**Table 17-56. SPINLOCK REGISTERS (continued)**

| Offset | Acronym     | Register Name                                                                                                                                                                                                                                                                  | Section           |
|--------|-------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------|--|--|--
| 824h   | LOCK_REG_9  | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.13 |
| 828h   | LOCK_REG_10 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.14 |
| 82Ch   | LOCK_REG_11 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.15 |
| 830h   | LOCK_REG_12 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.16 |
| 834h   | LOCK_REG_13 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.17 |
| 838h   | LOCK_REG_14 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.18 |
| 83Ch   | LOCK_REG_15 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.19 |
| 840h   | LOCK_REG_16 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.20 |
| 844h   | LOCK_REG_17 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.21 |
| 848h   | LOCK_REG_18 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.22 |
| 84Ch   | LOCK_REG_19 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.23 |
| 850h   | LOCK_REG_20 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.24 |

**Table 17-56. SPINLOCK REGISTERS (continued)**

| Offset | Acronym     | Register Name                                                                                                                                                                                                                                                                  | Section           |
|--------|-------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------|
| 854h   | LOCK_REG_21 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.25 |
| 858h   | LOCK_REG_22 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.26 |
| 85Ch   | LOCK_REG_23 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.27 |
| 860h   | LOCK_REG_24 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.28 |
| 864h   | LOCK_REG_25 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.29 |
| 868h   | LOCK_REG_26 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.30 |
| 86Ch   | LOCK_REG_27 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.31 |
| 870h   | LOCK_REG_28 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.32 |
| 874h   | LOCK_REG_29 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.33 |
| 878h   | LOCK_REG_30 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.34 |
| 87Ch   | LOCK_REG_31 | This register is read when attempting to acquire a lock.<br>The lock is automatically taken if it was not taken and<br>the value returned by the read is zero.<br>If the lock was already taken, then the read returns one.<br>Writing a zero to this register frees the lock. | Section 17.2.1.35 |

#### **17.2.1.1 REV Register (offset = 0h) [reset = 50020000h]**

REV is shown in [Figure](#page-74-1) 17-44 and described in Table [17-57.](#page-74-2)

Read-only IP revision identifier (X.Y.R) used by software to determine features, bugs and compatibility of an instance of this the Spin Lock module.

**Figure 17-44. REV Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

REV

R-50020000h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-57. REV Register Field Descriptions**

| Bit  | Field | Type<br>Reset |           | Description       |
|------|-------|---------------|-----------|-------------------|--
| 31-0 | REV   | R             | 50020000h | IP Revision Code. |

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +----------------------------------+---+
     |             Reserved             |CLK |
     |              R-0h                |ACTI|
     |                                  |VITY|
     |                                  |R-0h|
     +----------------------------------+---+

Bit: 7         5   4            2   1        0
     +---------+---+------------+---+--------+-----------+
     | Reserved|   |  SIDLEMODE |ENW|SOFTRESET|AUTOGATING|
     |  R-0h   |   |   R-2h     |R-4h| W-8h  |  R-11h    |
     +---------+---+------------+---+--------+-----------+
```

#### **17.2.1.2 SYSCONFIG Register (offset = 10h) [reset = 11h]**

SYSCONFIG is shown in [Figure](#page-75-1) 17-45 and described in Table [17-58](#page-75-2).

This register controls the various parameters of the OCP interface. Note that several fields are present by read-only.

**Figure 17-45. SYSCONFIG Register**
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-58. SYSCONFIG Register Field Descriptions**

| Bit  | Field         | Type    | Reset | Description                                                                                                                                                                                                                                                                                                                                                  |
|------|---------------|---------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-9 | Reserved      | R       | 0h    |                                                                                                                                                                                                                                                                                                                                                              |
| 8    | CLOCKACTIVITY | R0h |       | Indicates whether the module requires the OCP when in IDLE mode.<br>0 = NOTREQUIRED : OCP clock is not required by the module<br>during IDLE mode and may be switched off.<br>1 = REQUIRED : OCP clock is required by the module, even during<br>idle mode.                                                                                                  |
| 7-5  | Reserved      | R       | 0h    |                                                                                                                                                                                                                                                                                                                                                              |
| 4-3  | SIDLEMODE     | R       | 2h    | Control of the slave interface power management IDLE request<br>acknowledgement.<br>0 = FORCEIDLE : IDLE request is acknowledged unconditionally<br>and immediately.<br>1 = NOIDLE : IDLE request is never acknowledged.<br>2 = SMARTIDLE : IDLE request acknowledgement is based on the<br>internal module activity.<br>3 = unused3 : Reserved. Do not use. |
| 2    | ENWAKEUP      | R       | 4h    | Asynchronous wakeup gereration.<br>0 = NOWAKEUPGEN : Wakeup generation is disabled.<br>1 = WAKEUPGEN1 : Enable wakeup generation.                                                                                                                                                                                                                            |
| 1    | SOFTRESET     | W       | 8h    | Module software reset.<br>0 = NOOP : No Description<br>1 = DORESET : Start a soft reset sequence of the Spin Lock<br>module.                                                                                                                                                                                                                                 |
| 0    | AUTOGATING    | R       | 11h   | Internal OCP clock gating strategy.<br>0 = FREERUNNING : OCP clock is not gated when OCP interface is<br>idle.<br>1 = GATED : Automatic internal OCP clock gating strategy is<br>applied, based on the OCP interface activity.                                                                                                                               |

#### **17.2.1.3 SYSTATUS Register (offset = 14h) [reset = 1000001h]**

SYSTATUS is shown in [Figure](#page-76-1) 17-46 and described in Table [17-59.](#page-76-2)

This register provides status information about this instance of the Spin Lock module.

**Figure 17-46. SYSTATUS Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |              NUMLOCKS                |
     |               R-1h                   |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |              Reserved                |
     |              R-100h                  |
     +--------------------------------------+

Bit: 15  14  13  12  11  10   9   8
     +---+---+---+---+---+----+----+----+
     |IU7|IU6|IU5|IU4|IU3| IU2| IU1| IU0|
     |R- |R- |R- |R- |R- |R-  |R-  |R-  |
     |200h|400h|800h|1000h|2000h|4000h|8000h|10000h|
     +---+---+---+---+---+----+----+----+

Bit: 7                                   1  0
     +--------------------------------------+----------+
     |               Reserved               |RESETDONE |
     |             R-800000h                |R-1000001h|
     +--------------------------------------+----------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-59. SYSTATUS Register Field Descriptions**

| Bit   | Field     | Type | Reset    | Description                                                                                                                                                                                                                   |
|-------|-----------|------|----------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | NUMLOCKS  | R    | 1h       |                                                                                                                                                                                                                               |
| 23-16 | Reserved  | R    | 100h     |                                                                                                                                                                                                                               |
| 15    | IU7       | R    | 200h     |                                                                                                                                                                                                                               |
| 14    | IU6       | R    | 400h     |                                                                                                                                                                                                                               |
| 13    | IU5       | R    | 800h     |                                                                                                                                                                                                                               |
| 12    | IU4       | R    | 1000h    |                                                                                                                                                                                                                               |
| 11    | IU3       | R    | 2000h    |                                                                                                                                                                                                                               |
| 10    | IU2       | R    | 4000h    |                                                                                                                                                                                                                               |
| 9     | IU1       | R    | 8000h    | In-Use flag 1, covering lock registers<br>32 - 63.<br>Reads as one only if one or more lock registers in this range are<br>TAKEN.<br>If no lock registers are implemented in this range, then this flag<br>always reads as 0. |
| 8     | IU0       | R    | 10000h   | In-Use flag 0, covering lock registers<br>0 - 31.<br>Reads as one only if one or more lock registers in this range are<br>TAKEN.                                                                                              |
| 7-1   | Reserved  | R    | 800000h  | reserved                                                                                                                                                                                                                      |
| 0     | RESETDONE | R    | 1000001h | 0: Reset in progress.<br>1: Reset is completed.                                                                                                                                                                               |

#### **17.2.1.4 LOCK\_REG\_0 Register (offset = 800h) [reset = 0h]**

LOCK\_REG\_0 is shown in [Figure](#page-77-1) 17-47 and described in Table [17-60.](#page-77-2)

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-47. LOCK\_REG\_0 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-60. LOCK\_REG\_0 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.5 LOCK\_REG\_1 Register (offset = 804h) [reset = 0h]**

LOCK\_REG\_1 is shown in [Figure](#page-78-1) 17-48 and described in Table [17-61.](#page-78-2)

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-48. LOCK\_REG\_1 Register**

| 31   | 30       | 29 | 28 | 27       | 26 | 25 | 24
|------|----------|----|----|----------|----|----|----
|      |          |    |    | Reserved |    |    |
| R-0h |          |    |    |          |    |    |
| 23   | 22       | 21 | 20 | 19       | 18 | 17 | 16
|      |          |    |    | Reserved |    |    |
| R-0h |          |    |    |          |    |    |
| 15   | 14       | 13 | 12 | 11       | 10 | 9  | 8
|      |          |    |    | Reserved |    |    |
|      |          |    |    | R-0h     |    |    |
| 7    | 6        | 5  | 4  | 3        | 2  | 1  | 0
|      | Reserved |    |    |          |    |    |
|      | R-0h     |    |    |          |    |    |
|      |          |    |    |          |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-61. LOCK\_REG\_1 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.6 LOCK\_REG\_2 Register (offset = 808h) [reset = 0h]**

LOCK\_REG\_2 is shown in [Figure](#page-79-1) 17-49 and described in Table [17-62.](#page-79-2)

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-49. LOCK\_REG\_2 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-62. LOCK\_REG\_2 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.7 LOCK\_REG\_3 Register (offset = 80Ch) [reset = 0h]**

LOCK\_REG\_3 is shown in [Figure](#page-80-1) 17-50 and described in Table [17-63.](#page-80-2)

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-50. LOCK\_REG\_3 Register**

| 31   | 30       | 29 | 28 | 27       | 26 | 25 | 24
|------|----------|----|----|----------|----|----|----
|      |          |    |    | Reserved |    |    |
| R-0h |          |    |    |          |    |    |
| 23   | 22       | 21 | 20 | 19       | 18 | 17 | 16
|      |          |    |    | Reserved |    |    |
|      | R-0h     |    |    |          |    |    |
| 15   | 14       | 13 | 12 | 11       | 10 | 9  | 8
|      |          |    |    | Reserved |    |    |
|      |          |    |    | R-0h     |    |    |
| 7    | 6        | 5  | 4  | 3        | 2  | 1  | 0
|      | Reserved |    |    |          |    |    |
|      | R-0h     |    |    |          |    |    |
|      |          |    |    |          |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-63. LOCK\_REG\_3 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.8 LOCK\_REG\_4 Register (offset = 810h) [reset = 0h]**

LOCK\_REG\_4 is shown in [Figure](#page-81-1) 17-51 and described in Table [17-64.](#page-81-2)

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-51. LOCK\_REG\_4 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-64. LOCK\_REG\_4 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.9 LOCK\_REG\_5 Register (offset = 814h) [reset = 0h]**

LOCK\_REG\_5 is shown in [Figure](#page-82-1) 17-52 and described in Table [17-65.](#page-82-2)

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-52. LOCK\_REG\_5 Register**

| 31   | 30       | 29 | 28 | 27       | 26 | 25 | 24
|------|----------|----|----|----------|----|----|----
|      |          |    |    | Reserved |    |    |
| R-0h |          |    |    |          |    |    |
| 23   | 22       | 21 | 20 | 19       | 18 | 17 | 16
|      |          |    |    | Reserved |    |    |
|      | R-0h     |    |    |          |    |    |
| 15   | 14       | 13 | 12 | 11       | 10 | 9  | 8
|      |          |    |    | Reserved |    |    |
|      |          |    |    | R-0h     |    |    |
| 7    | 6        | 5  | 4  | 3        | 2  | 1  | 0
|      | Reserved |    |    |          |    |    |
|      | R-0h     |    |    |          |    |    |
|      |          |    |    |          |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-65. LOCK\_REG\_5 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.10 LOCK\_REG\_6 Register (offset = 818h) [reset = 0h]**

LOCK\_REG\_6 is shown in [Figure](#page-83-1) 17-53 and described in Table [17-66.](#page-83-2)

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-53. LOCK\_REG\_6 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-66. LOCK\_REG\_6 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.11 LOCK\_REG\_7 Register (offset = 81Ch) [reset = 0h]**

LOCK\_REG\_7 is shown in [Figure](#page-84-1) 17-54 and described in Table [17-67.](#page-84-2)

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-54. LOCK\_REG\_7 Register**

| 31       | 30 | 29 | 28       | 27       | 26 | 25 | 24     |
|----------|----|----|----------|----------|----|----|--------|
| Reserved |    |    |          |          |    |    |        |
| R-0h     |    |    |          |          |    |    |        |
| 23       | 22 | 21 | 20       | 19       | 18 | 17 | 16     |
|          |    |    |          | Reserved |    |    |        |
| R-0h     |    |    |          |          |    |    |        |
| 15       | 14 | 13 | 12       | 11       | 10 | 9  | 8      |
|          |    |    |          | Reserved |    |    |        |
|          |    |    |          | R-0h     |    |    |        |
| 7        | 6  | 5  | 4        | 3        | 2  | 1  | 0      |
|          |    |    | Reserved |          |    |    | TAKEN  |
|          |    |    | R-0h     |          |    |    | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-67. LOCK\_REG\_7 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.12 LOCK\_REG\_8 Register (offset = 820h) [reset = 0h]**

LOCK\_REG\_8 is shown in [Figure](#page-85-1) 17-55 and described in Table [17-68.](#page-85-2)

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-55. LOCK\_REG\_8 Register**

| 31   | 30       | 29 | 28 | 27       | 26 | 25 | 24
|------|----------|----|----|----------|----|----|--------
|      | Reserved |    |    |          |    |    |
| R-0h |          |    |    |          |    |    |
| 23   | 22       | 21 | 20 | 19       | 18 | 17 | 16
|      |          |    |    | Reserved |    |    |
| R-0h |          |    |    |          |    |    |
| 15   | 14       | 13 | 12 | 11       | 10 | 9  | 8
|      |          |    |    | Reserved |    |    |
|      |          |    |    | R-0h     |    |    |
| 7    | 6        | 5  | 4  | 3        | 2  | 1  | 0
|      | Reserved |    |    |          |    |    |
| R-0h |          |    |    |          |    |    |
|      |          |    |    |          |    |    | R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-68. LOCK\_REG\_8 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.13 LOCK\_REG\_9 Register (offset = 824h) [reset = 0h]**

LOCK\_REG\_9 is shown in [Figure](#page-86-1) 17-56 and described in Table [17-69.](#page-86-2)

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-56. LOCK\_REG\_9 Register**

| 31       | 30       | 29 | 28 | 27       | 26 | 25 | 24 |
|----------|----------|----|----|----------|----|----|----|--
| Reserved |          |    |    |          |    |    |    |
| R-0h     |          |    |    |          |    |    |    |
| 23       | 22       | 21 | 20 | 19       | 18 | 17 | 16 |
|          |          |    |    | Reserved |    |    |    |
| R-0h     |          |    |    |          |    |    |    |
| 15       | 14       | 13 | 12 | 11       | 10 | 9  | 8  |
|          |          |    |    | Reserved |    |    |    |
|          |          |    |    | R-0h     |    |    |    |
| 7        | 6        | 5  | 4  | 3        | 2  | 1  | 0  |
|          | Reserved |    |    |          |    |    |    |
|          | R-0h     |    |    |          |    |    |    |
|          |          |    |    |          |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-69. LOCK\_REG\_9 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.14 LOCK\_REG\_10 Register (offset = 828h) [reset = 0h]**

LOCK\_REG\_10 is shown in [Figure](#page-87-1) 17-57 and described in Table [17-70](#page-87-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-57. LOCK\_REG\_10 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                      1      0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-70. LOCK\_REG\_10 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.15 LOCK\_REG\_11 Register (offset = 82Ch) [reset = 0h]**

LOCK\_REG\_11 is shown in [Figure](#page-88-1) 17-58 and described in Table [17-71](#page-88-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-58. LOCK\_REG\_11 Register**

| 31       | 30       | 29 | 28 | 27       | 26 | 25 | 24 |
|----------|----------|----|----|----------|----|----|----|--
| Reserved |          |    |    |          |    |    |    |
| R-0h     |          |    |    |          |    |    |    |
| 23       | 22       | 21 | 20 | 19       | 18 | 17 | 16 |
|          |          |    |    | Reserved |    |    |    |
|          |          |    |    | R-0h     |    |    |    |
| 15       | 14       | 13 | 12 | 11       | 10 | 9  | 8  |
|          |          |    |    | Reserved |    |    |    |
|          |          |    |    | R-0h     |    |    |    |
| 7        | 6        | 5  | 4  | 3        | 2  | 1  | 0  |
|          | Reserved |    |    |          |    |    |    |
|          | R-0h     |    |    |          |    |    |    |
|          |          |    |    |          |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-71. LOCK\_REG\_11 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.16 LOCK\_REG\_12 Register (offset = 830h) [reset = 0h]**

LOCK\_REG\_12 is shown in [Figure](#page-89-1) 17-59 and described in Table [17-72](#page-89-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-59. LOCK\_REG\_12 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-72. LOCK\_REG\_12 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.17 LOCK\_REG\_13 Register (offset = 834h) [reset = 0h]**

LOCK\_REG\_13 is shown in [Figure](#page-90-1) 17-60 and described in Table [17-73](#page-90-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-60. LOCK\_REG\_13 Register**

| 31       | 30       | 29 | 28 | 27       | 26 | 25 | 24 |
|----------|----------|----|----|----------|----|----|----|--
| Reserved |          |    |    |          |    |    |    |
| R-0h     |          |    |    |          |    |    |    |
| 23       | 22       | 21 | 20 | 19       | 18 | 17 | 16 |
|          |          |    |    | Reserved |    |    |    |
|          |          |    |    | R-0h     |    |    |    |
| 15       | 14       | 13 | 12 | 11       | 10 | 9  | 8  |
|          |          |    |    | Reserved |    |    |    |
|          |          |    |    | R-0h     |    |    |    |
| 7        | 6        | 5  | 4  | 3        | 2  | 1  | 0  |
|          | Reserved |    |    |          |    |    |    |
|          | R-0h     |    |    |          |    |    |    |
|          |          |    |    |          |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-73. LOCK\_REG\_13 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.18 LOCK\_REG\_14 Register (offset = 838h) [reset = 0h]**

LOCK\_REG\_14 is shown in [Figure](#page-91-1) 17-61 and described in Table [17-74](#page-91-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-61. LOCK\_REG\_14 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-74. LOCK\_REG\_14 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.19 LOCK\_REG\_15 Register (offset = 83Ch) [reset = 0h]**

LOCK\_REG\_15 is shown in [Figure](#page-92-1) 17-62 and described in Table [17-75](#page-92-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-62. LOCK\_REG\_15 Register**

| 31                                     | 30       | 29 | 28 | 27       | 26 | 25 | 24     |
|----------------------------------------|----------|----|----|----------|----|----|--------|--
|                                        | Reserved |    |    |          |    |    |        |
| R-0h                                   |          |    |    |          |    |    |        |
| 2322-17 |          |    |    |          |    |    |        |
|                                        |          |    |    | Reserved |    |    |        |
|                                        | R-0h     |    |    |          |    |    |        |
| 15                                     | 14       | 13 | 12 | 11       | 10 | 9  | 8      |
|                                        |          |    |    | Reserved |    |    |        |
|                                        |          |    |    | R-0h     |    |    |        |
| 7                                      | 6        | 5  | 4  | 3        | 2  | 1  | 0      |
|                                        | Reserved |    |    |          |    |    |        |
|                                        | R-0h     |    |    |          |    |    |        |
|                                        |          |    |    |          |    |    | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-75. LOCK\_REG\_15 Register Field Descriptions**

| Bit  | Field<br>Type |     | Reset | Description |
|------|---------------|-----|-------|-------------|
| 31-1 | Reserved      | R   | 0h    |             |
| 0    | TAKEN         | R/W | 0h    |             |

#### **17.2.1.20 LOCK\_REG\_16 Register (offset = 840h) [reset = 0h]**

LOCK\_REG\_16 is shown in [Figure](#page-93-1) 17-63 and described in Table [17-76](#page-93-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-63. LOCK\_REG\_16 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-76. LOCK\_REG\_16 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.21 LOCK\_REG\_17 Register (offset = 844h) [reset = 0h]**

LOCK\_REG\_17 is shown in [Figure](#page-94-1) 17-64 and described in Table [17-77](#page-94-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-64. LOCK\_REG\_17 Register**

| 31                                     | 30       | 29 | 28 | 27       | 26 | 25 | 24
|----------------------------------------|----------|----|----|----------|----|----|----|--|--
|                                        | Reserved |    |    |          |    |    |
| R-0h                                   |          |    |    |          |    |    |
| 2322-17 |          |    |    |          |    |    |
|                                        |          |    |    | Reserved |    |    |
|                                        | R-0h     |    |    |          |    |    |
| 15                                     | 14       | 13 | 12 | 11       | 10 | 9  | 8
|                                        |          |    |    | Reserved |    |    |
|                                        |          |    |    | R-0h     |    |    |
| 7                                      | 6        | 5  | 4  | 3        | 2  | 1  | 0
| Reserved                               |          |    |    |          |    |    |
|                                        | R-0h     |    |    |          |    |    |
|                                        |          |    |    |          |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-77. LOCK\_REG\_17 Register Field Descriptions**

| Bit  | Field<br>Type |     | Reset | Description |
|------|---------------|-----|-------|-------------|
| 31-1 | Reserved      | R   | 0h    |             |
| 0    | TAKEN         | R/W | 0h    |             |

#### **17.2.1.22 LOCK\_REG\_18 Register (offset = 848h) [reset = 0h]**

LOCK\_REG\_18 is shown in [Figure](#page-95-1) 17-65 and described in Table [17-78](#page-95-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-65. LOCK\_REG\_18 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-78. LOCK\_REG\_18 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.23 LOCK\_REG\_19 Register (offset = 84Ch) [reset = 0h]**

LOCK\_REG\_19 is shown in [Figure](#page-96-1) 17-66 and described in Table [17-79](#page-96-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-66. LOCK\_REG\_19 Register**

| 31                                     | 30       | 29 | 28 | 27       | 26 | 25 | 24
|----------------------------------------|----------|----|----|----------|----|----|----|--|--
|                                        | Reserved |    |    |          |    |    |
| R-0h                                   |          |    |    |          |    |    |
| 2322-17 |          |    |    |          |    |    |
|                                        |          |    |    | Reserved |    |    |
|                                        | R-0h     |    |    |          |    |    |
| 15                                     | 14       | 13 | 12 | 11       | 10 | 9  | 8
|                                        |          |    |    | Reserved |    |    |
|                                        |          |    |    | R-0h     |    |    |
| 7                                      | 6        | 5  | 4  | 3        | 2  | 1  | 0
| Reserved                               |          |    |    |          |    |    |
|                                        | R-0h     |    |    |          |    |    |
|                                        |          |    |    |          |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-79. LOCK\_REG\_19 Register Field Descriptions**

| Bit  | Field<br>Type |     | Reset | Description |
|------|---------------|-----|-------|-------------|
| 31-1 | Reserved      | R   | 0h    |             |
| 0    | TAKEN         | R/W | 0h    |             |

#### **17.2.1.24 LOCK\_REG\_20 Register (offset = 850h) [reset = 0h]**

LOCK\_REG\_20 is shown in [Figure](#page-97-1) 17-67 and described in Table [17-80](#page-97-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-67. LOCK\_REG\_20 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-80. LOCK\_REG\_20 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.25 LOCK\_REG\_21 Register (offset = 854h) [reset = 0h]**

LOCK\_REG\_21 is shown in [Figure](#page-98-1) 17-68 and described in Table [17-81](#page-98-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-68. LOCK\_REG\_21 Register**

| 31                                     | 30       | 29 | 28 | 27       | 26 | 25 | 24     |
|----------------------------------------|----------|----|----|----------|----|----|--------|--
|                                        | Reserved |    |    |          |    |    |        |
| R-0h                                   |          |    |    |          |    |    |        |
| 2322-17 |          |    |    |          |    |    |        |
|                                        |          |    |    | Reserved |    |    |        |
|                                        | R-0h     |    |    |          |    |    |        |
| 15                                     | 14       | 13 | 12 | 11       | 10 | 9  | 8      |
|                                        |          |    |    | Reserved |    |    |        |
|                                        |          |    |    | R-0h     |    |    |        |
| 7                                      | 6        | 5  | 4  | 3        | 2  | 1  | 0      |
|                                        | Reserved |    |    |          |    |    |        |
|                                        | R-0h     |    |    |          |    |    |        |
|                                        |          |    |    |          |    |    | R/W-0h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-81. LOCK\_REG\_21 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.26 LOCK\_REG\_22 Register (offset = 858h) [reset = 0h]**

LOCK\_REG\_22 is shown in [Figure](#page-99-1) 17-69 and described in Table [17-82](#page-99-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-69. LOCK\_REG\_22 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-82. LOCK\_REG\_22 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.27 LOCK\_REG\_23 Register (offset = 85Ch) [reset = 0h]**

LOCK\_REG\_23 is shown in [Figure](#page-100-1) 17-70 and described in Table [17-83](#page-100-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-70. LOCK\_REG\_23 Register**

| 31                                     | 30       | 29 | 28 | 27       | 26 | 25 | 24
|----------------------------------------|----------|----|----|----------|----|----|----|--|--
|                                        | Reserved |    |    |          |    |    |
| R-0h                                   |          |    |    |          |    |    |
| 2322-17 |          |    |    |          |    |    |
|                                        |          |    |    | Reserved |    |    |
|                                        | R-0h     |    |    |          |    |    |
| 15                                     | 14       | 13 | 12 | 11       | 10 | 9  | 8
|                                        |          |    |    | Reserved |    |    |
|                                        |          |    |    | R-0h     |    |    |
| 7                                      | 6        | 5  | 4  | 3        | 2  | 1  | 0
| Reserved                               |          |    |    |          |    |    |
|                                        | R-0h     |    |    |          |    |    |
|                                        |          |    |    |          |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-83. LOCK\_REG\_23 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.28 LOCK\_REG\_24 Register (offset = 860h) [reset = 0h]**

LOCK\_REG\_24 is shown in [Figure](#page-101-1) 17-71 and described in Table [17-84](#page-101-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-71. LOCK\_REG\_24 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-84. LOCK\_REG\_24 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.29 LOCK\_REG\_25 Register (offset = 864h) [reset = 0h]**

LOCK\_REG\_25 is shown in [Figure](#page-102-1) 17-72 and described in Table [17-85](#page-102-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-72. LOCK\_REG\_25 Register**

| 31   | 30   | 29 | 28       | 27       | 26 | 25 | 24
|------|------|----|----------|----------|----|----|-------
|      |      |    |          | Reserved |    |    |
| R-0h |      |    |          |          |    |    |
| 23   | 22   | 21 | 20       | 19       | 18 | 17 | 16
|      |      |    |          | Reserved |    |    |
| R-0h |      |    |          |          |    |    |
| 15   | 14   | 13 | 12       | 11       | 10 | 9  | 8
|      |      |    |          | Reserved |    |    |
|      |      |    |          | R-0h     |    |    |
| 7    | 6    | 5  | 4        | 3        | 2  | 1  | 0
|      |      |    | Reserved |          |    |    | TAKEN
|      | R-0h |    |          |          |    |    |
|      |      |    |          |          |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-85. LOCK\_REG\_25 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.30 LOCK\_REG\_26 Register (offset = 868h) [reset = 0h]**

LOCK\_REG\_26 is shown in [Figure](#page-103-1) 17-73 and described in Table [17-86](#page-103-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-73. LOCK\_REG\_26 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-86. LOCK\_REG\_26 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.31 LOCK\_REG\_27 Register (offset = 86Ch) [reset = 0h]**

LOCK\_REG\_27 is shown in [Figure](#page-104-1) 17-74 and described in Table [17-87](#page-104-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-74. LOCK\_REG\_27 Register**

| 31   | 30   | 29 | 28       | 27       | 26 | 25 | 24
|------|------|----|----------|----------|----|----|-------
|      |      |    |          | Reserved |    |    |
| R-0h |      |    |          |          |    |    |
| 23   | 22   | 21 | 20       | 19       | 18 | 17 | 16
|      |      |    |          | Reserved |    |    |
| R-0h |      |    |          |          |    |    |
| 15   | 14   | 13 | 12       | 11       | 10 | 9  | 8
|      |      |    |          | Reserved |    |    |
|      |      |    |          | R-0h     |    |    |
| 7    | 6    | 5  | 4        | 3        | 2  | 1  | 0
|      |      |    | Reserved |          |    |    | TAKEN
|      | R-0h |    |          |          |    |    |
|      |      |    |          |          |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-87. LOCK\_REG\_27 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.32 LOCK\_REG\_28 Register (offset = 870h) [reset = 0h]**

LOCK\_REG\_28 is shown in [Figure](#page-105-1) 17-75 and described in Table [17-88](#page-105-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-75. LOCK\_REG\_28 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-88. LOCK\_REG\_28 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.33 LOCK\_REG\_29 Register (offset = 874h) [reset = 0h]**

LOCK\_REG\_29 is shown in [Figure](#page-106-1) 17-76 and described in Table [17-89](#page-106-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-76. LOCK\_REG\_29 Register**

| 31   | 30   | 29 | 28       | 27       | 26 | 25 | 24
|------|------|----|----------|----------|----|----|-------
|      |      |    |          | Reserved |    |    |
| R-0h |      |    |          |          |    |    |
| 23   | 22   | 21 | 20       | 19       | 18 | 17 | 16
|      |      |    |          | Reserved |    |    |
| R-0h |      |    |          |          |    |    |
| 15   | 14   | 13 | 12       | 11       | 10 | 9  | 8
|      |      |    |          | Reserved |    |    |
|      |      |    |          | R-0h     |    |    |
| 7    | 6    | 5  | 4        | 3        | 2  | 1  | 0
|      |      |    | Reserved |          |    |    | TAKEN
|      | R-0h |    |          |          |    |    |
|      |      |    |          |          |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-89. LOCK\_REG\_29 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |

#### **17.2.1.34 LOCK\_REG\_30 Register (offset = 878h) [reset = 0h]**

LOCK\_REG\_30 is shown in [Figure](#page-107-1) 17-77 and described in Table [17-90](#page-107-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-77. LOCK\_REG\_30 Register**

```
Bit: 31                                  24
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 23                                  16
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 15                                   8
     +--------------------------------------+
     |               Reserved               |
     |                R-0h                  |
     +--------------------------------------+

Bit: 7                                   1  0
     +--------------------------------------+------+
     |               Reserved               |TAKEN |
     |                R-0h                  |R/W-0h|
     +--------------------------------------+------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 17-90. LOCK\_REG\_30 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |
#### **17.2.1.35 LOCK\_REG\_31 Register (offset = 87Ch) [reset = 0h]**

LOCK\_REG\_31 is shown in [Figure](#page-108-1) 17-78 and described in Table [17-91](#page-108-2).

This register is read when attempting to acquire a lock. The lock is automatically taken if it was not taken and the value returned by the read is zero. If the lock was already taken, then the read returns one. Writing a zero to this register frees the lock.

**Figure 17-78. LOCK\_REG\_31 Register**

| 31   | 30   | 29 | 28       | 27       | 26 | 25 | 24
|------|------|----|----------|----------|----|----|-------
|      |      |    |          | Reserved |    |    |
| R-0h |      |    |          |          |    |    |
| 23   | 22   | 21 | 20       | 19       | 18 | 17 | 16
|      |      |    |          | Reserved |    |    |
| R-0h |      |    |          |          |    |    |
| 15   | 14   | 13 | 12       | 11       | 10 | 9  | 8
|      |      |    |          | Reserved |    |    |
|      |      |    |          | R-0h     |    |    |
| 7    | 6    | 5  | 4        | 3        | 2  | 1  | 0
|      |      |    | Reserved |          |    |    | TAKEN
|      | R-0h |    |          |          |    |    |
|      |      |    |          |          |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 17-91. LOCK\_REG\_31 Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description |
|------|----------|------|-------|-------------|
| 31-1 | Reserved | R    | 0h    |             |
| 0    | TAKEN    | R/W  | 0h    |             |
