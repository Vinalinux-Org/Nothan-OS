# 14 Ethernet Subsystem

This chapter describes the ethernet subsystem of the device.

| 14.1 | Introduction                 | 1997 |
|------|------------------------------|------|
| 14.2 | Integration                  | 1999 |
| 14.3 | Functional Description       | 2009 |
| 14.4 | Software Operation           | 2069 |
| 14.5 | Ethernet Subsystem Registers | 2074 |
|      |                              |      |

## **14.1 Introduction**

Described in the following sections is the Layer 2 3-port switch (3PSW) Ethernet subsystem. The 3-port switch gigabit ethernet subsystem provides ethernet packet communication and can be configured as an ethernet switch. It provides the gigabit media independent interface (GMII),reduced gigabit media independent interface (RGMII), reduced media independent interface (RMII), the management data input output (MDIO) for physical layer device (PHY) management.

As a Layer 2 switch, this device is capable of supporting all higher layers and various protocols (e.g., IPV4, IPV6, 802.3x) in software.

### *14.1.1 Features*

The general features of the ethernet switch subsystem are:

- Two 10/100/1000 Ethernet ports with GMII, RMII and RGMII interfaces
- Wire rate switching (802.1d)
- Non Blocking switch fabric
- Flexible logical FIFO based packet buffer structure
- Four priority level QOS support (802.1p)
- CPPI 3.1 compliant DMA controllers
- Support for IEEE 1588v2 Clock Synchronization (2008 Annex D and Annex F)
  - Timing FIFO and time stamping logic inside the SS
- Device Level Ring (DLR) Support
- Address Lookup Engine
  - 1024 addresses plus VLANs
  - Wire rate lookup
  - VLAN support
  - Host controlled time-based aging
  - Spanning tree support
  - L2 address lock and L2 filtering support
  - MAC authentication (802.1x)
  - Receive or destination based Multicast and Broadcast limits
  - MAC address blocking
  - Source port locking
  - OUI host accept/deny feature
- Flow Control Support (802.3x)
- EtherStats and 802.3Stats RMON statistics gathering (shared)
- Support for external packet dropping engine
- CPGMAC\_SL transmit to CPGMAC\_SL receive Loopback mode (digital loopback) supported
- CPGMAC\_SL receive to CPGMAC\_SL transmit Loobback mode (FIFO loopback) supported
- Maximum frame size 2016 bytes (2020 with VLAN)
- 8k (2048 x 32) internal CPPI buffer descriptor memory
- MDIO module for PHY Management
- Programmable interrupt control with selected interrupt pacing
- Emulation Support.
- Programmable transmit Inter-Packet Gap (IPG)
- Reset isolation

### *14.1.2 Unsupported Features*

There are 18 level interrupts from the CPGMAC module and 2 (used) level interrupts from the MDIO module. The CPSW\_3GSS includes an interrupt combiner/pacer to combine these interrupts together to produce 4 interrupt outputs (per processing core). This device does not split processing among multiple cores but allows servicing of the Core0 interrupts by the Cortex-A8 or the PRU-ICSS.

The unsupported CPGMAC features in the device are shown in the following table.

**Table 14-1. Unsupported CPGMAC Features**

| Feature                          | Reason                                                                                                                                  |
|----------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------|
| Multi-core split processing      | Core 1 and Core 2 interrupts not connected.                                                                                             |
| GMII                             | Only 4 Rx/Tx data pins are pinned out for each port. The device<br>supports MII (on GMII interface), RGMII, and RMII interfaces<br>only |
| Phy link status                  | The MLINK inputs are not pinned out. Phy link status outputs<br>can be connected to device GPIOs.                                       |
| Internal Delay mode for RGMII    | RGMII Internal Delay mode is not supported.                                                                                             |
| RMII reference clock output mode | RMII reference clock does not satisfy input requirements of RMII<br>Ethernet PHYs.                                                      |
| Reset isolation                  | Silicon bug. For more information, see AM335x ARM Cortex-A8<br>Microprocessors (MPUs) Silicon Errata (literature number<br>SPRZ360).    |

## **14.2 Integration**

This device includes a single instantiation of the three-port Gigabit Ethernet Switch Subsystem (CPSW\_3GSS\_RG). The switch provides 2 external ethernet ports (ports 1 and 2) and an internal CPPI interface port (port 0) with IEEE 1588v2 and 802.1ae support. The subsystem consists of:

- One instance of the 3-port Gigabit switch CPSW-3G, which contains:
  - 2 CPGMAC\_SL 10/100/1000 ethernet port modules with GMII interface
- Two RGMII interface modules
- Two RMII interface modules
- One MDIO interface module
- One Interrupt Controller module
- Local CPPI memory of size 8K Bytes

The integration of the Ethernet Switch is shown in [Figure](#page-3-1) 14-1

```
                                 (Device)
                     L3 Fast Interconnect / L4 Fast Interconnect
        ┌─────────────────────────────────────────────────────────────────────────────┐
        │                                                                             │
        │  L3 Fast Interconnect                                                       │
        │      │                                                                      │
        │   ┌───────┐        ┌───────────┐        tx vbusp                            │
        │   │ Mstr  │───────▶│  CPPI DMA  │──────────────────────┐                    │
        │   │ OHCP  │        └───────────┘        rx vbusp       │                    │
        │   │Bridge │────────────────────────────────────────────┘                    │
        │   └───────┘                                        ┌────────────────┐       │
        │                                                    │    CPSW_3G     │       │
        │   ┌───────────────────────────┐                    │ (3GMAC Switch  │       │
        │   │ MPU Subsystem, PRU-ICSS   │◀── C0_INTS         │   Subsystem)   │       │
        │   │ Interrupts                │◀── C1_INTS         │                │       │
        │   └───────────────────────────┘◀── C2_INTS         │  GMII1_* pins  │       │
        │                         ▲                          │  GMII2_* pins  │       │
        │                         │                          └───────────┬────┘       │
        │                      ┌─────┐                                   │            │
        │                      │ Int │───────────────────────────────────┘            │
        │                      └─────┘                                                │
        │                                                                     GMAC    │
        │                                                                     Switch  │
        │                                                                     Pads    │
        │                                                                     (GMII1  │
        │                                                                      /GMII2)│
        │                                                                      ───────│
        │                                                                     GMII1_TXCLK,
        │                                                                     GMII1_TXEN,
        │                                                                     GMII1_TXD[3:0],
        │                                                                     GMII1_RXDV,
        │                                                                     GMII1_RXER,
        │                                                                     GMII1_COL,
        │                                                                     GMII1_CRS,
        │                                                                     GMII1_RXCLK,
        │                                                                     GMII1_RXD[3:0]
        │                                                                     (tương tự GMII2_*)
        │
        │  ┌───────────┐     (clocks/resets)            ┌───────────────────────┐
        │  │   PRCM    │───────────────────────────────▶│ CPSW_3G clock/reset   │
        │  │           │  main_arst_n / iso_main_arst_n └───────────────────────┘
        │  │ CORE_CLKOUTM4 (200MHz)                     mhz_250_CLK / mhz_50_CLK
        │  │ CORE_CLKOUTM5 (250MHz)                     mhz_5_CLK / rft_clk ...
        │  └───────────┘
        │
        │  ┌───────────────────────────┐
        │  │ DMTIMER4/5/6/7            │── POTIMERPWM ──▶ (to CPSW_3G time sync)
        │  └───────────────────────────┘
        │
        │  ┌───────────────────────────┐
        │  │ Control Module (GMII_SEL) │── gmii1_sel[1:0], gmii2_sel[1:0],      │
        │  │                           │   rgmii2_id_mode1_n, rgmii2_id_mode2_n │
        │  │                           │   rmii1_io_clk_en, rmii1_io_clk_en     │
        │  └───────────────────────────┘
        │                 │
        │                 └───────────────▶ To RMII REFCLK I/O Buffers
        │
        │  L4 Fast Interconnect
        │      │
        │   ┌───────┐      ┌─────────┐      ┌──────┐
        │   │  Slv  │─────▶│   SCR   │─────▶│ Regs │─────▶ (config to CPSW_3G)
        │   │ OHCP  │      └─────────┘      └──────┘
        │   │Bridge │───────────────▶┌──────┐
        │   └───────┘                │ MDIO │──────────────▶ MDIO_CLK, MDIO_DATA
        │                            └──────┘
        │
        │                                  ┌──────────┐
        │                                  │ CPRGMII1 │──▶ RGMII1_TCLK/TCTL/TD[3:0]
        │                                  ├──────────┤    RGMII1_RCLK/RCTL/RD[3:0]
        │                                  │ CPRGMII2 │──▶ RGMII2_TCLK/TCTL/TD[3:0]
        │                                  ├──────────┤    RGMII2_RCLK/RCTL/RD[3:0]
        │                                  │ CPRMII1  │──▶ RMII1_REFCLK/TXEN/TXD[1:0]
        │                                  ├──────────┤    RMII1_RXER/RXD[1:0]/CRS_DV
        │                                  │ CPRMII2  │──▶ RMII2_REFCLK/TXEN/TXD[1:0]
        │                                  └──────────┘    RMII2_RXER/RXD[1:0]/CRS_DV
        │                                                                     │
        └─────────────────────────────────────────────────────────────────────┘
```

**Figure 14-1. Ethernet Switch Integration**

### *14.2.1 Ethernet Switch Connectivity Attributes*

The general connectivity attributes for the Ethernet Switch module are shown in [Table](#page-4-0) 14-2.

**Table 14-2. Ethernet Switch Connectivity Attributes**

| Attributes          | Type                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|---------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Power Domain        | Peripheral Domain                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| Clock Domain        | PD_PER_CPSW_125MHZ_GCLK (Main)<br>PD_PER_CPSW_250MHZ_GCLK (MHZ_250_CLK)<br>PD_PER_CPSW_50MHZ_GCLK (MHZ_50_CLK)<br>PD_PER_CPSW_5MHZ_GCLK (MHZ_5_CLK)<br>PD_PER_CPSW_CPTS_RFT_CLK (CPTS_RFT_CLK)                                                                                                                                                                                                                                                                                        |
| Reset Signals       | CPSW_MAIN_ARST_N<br>CPSW_ISO_MAIN_ARST_N                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| Idle/Wakeup Signals | Idle<br>Standby                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| Interrupt Requests  | 4 Interrupts<br>RX_THRESH (3PGSWRXTHR0) – Receive Threshold interrupt<br>(nonpaced)<br>RX (3PGSWRXINT0) – Receive interrupt (paced)<br>TX (3PGSWTXINT0) – Transmit interrupt (paced)<br>Misc (3PGSWMISC0) – Other interrupts<br>All Ethernet Switch interrupts go to MPU Subsystem and PRU<br>ICSS.<br>The Subsystem contains 3 sets of interrupts C0, C1, and C2 to<br>allow for split core processing of packets. On this device, only<br>the C0 version of the interrupts is used. |
| DMA Requests        | None                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| Physical Address    | L4 Fast slave port<br>L3 Fast initiator port                                                                                                                                                                                                                                                                                                                                                                                                                                          |

### *14.2.2 Ethernet Switch Clock and Reset Management*

The ethernet switch controller operates in its own clock domain and its initiator and target interfaces are connected to the L3/L4 through asynchronous bridges. The OCP interfaces are driven by the MAIN clock input. Additional reference clock inputs are provided for operating the various ethernet ports at different rates.

**Table 14-3. Ethernet Switch Clock Signals**

| Clock Signal                                           | Max Freq | Reference / Source             | Comments                                 |
|--------------------------------------------------------|----------|--------------------------------|------------------------------------------|
| rft_clk<br>Gigabit GMII Tx Reference<br>clock          | 125 MHz  | not supported                  |                                          |
| main_clk<br>Logic/Interface clock                      | 125 MHz  | CORE_CLKOUTM5 / 2              | pd_per_cpsw_125mhz_gclk<br>from PRCM     |
| mhz250_clk<br>Gigabit RGMII Reference clock            | 250 MHz  | CORE_CLKOUTM5                  | pd_per_cpsw_250mhz_gclk<br>from PRCM     |
| mhz50_clk<br>RMII and 100mbps RGMII<br>Reference clock | 50 MHz   | CORE_CLKOUTM5 / 5              | pd_per_cpsw_50mhz_gclk<br>from PRCM      |
| mhz5_clk<br>10 mbpsRGMII Reference<br>clock            | 5 MHz    | CORE_CLKOUTM5 / 50             | pd_per_cpsw_5mhz_gclk<br>from PRCM       |
| cpts_rft_clk<br>IEEE 1588v2 clock                      | 250 MHz  | CORE_CLKOUTM4<br>CORE_CLKOUTM5 | pd_per_cpsw_cpts_rft_clk<br>from PRCM    |
| gmii1_mr_clk<br>GMII Port 1 Receive clock              | 25 MHz   | External Pin                   | gmii1_rxclk_in<br>from GMII1_RCLK pad    |
| gmii2_mr_clk<br>GMII Port 2 Receive clock              | 25 MHz   | External Pin                   | gmii2_rxclk_in<br>from GMII2_RCLK pad    |
| gmii1_mt_clk<br>GMII Port 1 Transmit clock             | 25 MHz   | External Pin                   | gmii1_txclk_in<br>from GMII1_TCLK pad    |
| gmii2_mt_clk<br>GMII Port 2 Transmit clock             | 25 MHz   | External Pin                   | gmii2_txclk_in<br>from GMII2_TCLK pad    |
| rgmii1_rxc_clk<br>RGMII Port 1 Receive clock           | 250 MHz  | External Pin                   | rgmii1_rclk_in<br>from RGMII1_RCLK pad   |
| rgmii2_rxc_clk<br>RGMII Port 2 Receive clock           | 250 MHz  | External Pin                   | rgmii2_rclk_in<br>from RGMII2_RCLK pad   |
| rmii1_mhz_50_clk<br>RMII Port 1 Reference clock        | 50 MHz   | External Pin                   | rmii1_refclk_in<br>from RMII1_REFCLK pad |
| rmii2_mhz_50_clk<br>RMII Port 2 Reference clock        | 50 MHz   | External Pin                   | rmii2_refclk_in<br>from RMII2_REFCLK pad |

### *14.2.3 Ethernet Switch Pin List*

The external signals for the Ethernet Switch module are shown in the following table.

**Table 14-4. Ethernet Switch Pin List**

| Pin            | Type* | Description                     |
|----------------|-------|---------------------------------|
| GMIIx_RXCLK    | I     | GMII/MII Receive clock          |
| GMIIx_RXD[3:0] | I     | GMII/MII Receive data           |
| GMIIx_RXDV     | I     | GMII/MII Receive data valid     |
| GMIIx_RXER     | I     | GMII/MII Receive error          |
| GMIIx_COL      | I     | GMII/MII Collision detect       |
| GMIIx_CRS      | I     | GMII/MII Carrier sense          |
| GMIIx_TXCLK    | I     | GMII/MII Transmit clock         |
| GMIIx_TXD[3:0] | O     | GMII/MII Transmit data          |
| GMIIx_TXEN     | O     | GMII/MII Transmit enable        |
| RGMIIx_RCLK    | I     | RGMII Receive clock             |
| RGMIIx_RCTL    | I     | RGMII Receive control           |
| RGMIIx_RD[3:0] | I     | RGMII Receive data              |
| RGMIIx_TCLK    | O     | RGMII Transmit clock            |
| RGMIIx_TCTL    | O     | RGMII Transmit control          |
| RGMIIx_TD[3:0] | O     | RGMII Transmit data             |
| RMIIx_RXD[1:0] | I     | RMII Receiver data              |
| RMIIx_RXER     | I     | RMII Receiver error             |
| RMIIx_CRS_DV   | I     | RMII Carrier sense / Data valid |
| RMIIx_TXEN     | O     | RMII Transmit enable            |
| RMIIx_REFCLK   | I/O   | RMII Reference clock            |
| RMIIx_TXD[1:0] | O     | RMII Transmit data              |
| MDIO_CLK       | O     | MDIO Serial clock               |
| MDIO_DATA      | I/O   | MDIO Serial data                |

### *14.2.4 Ethernet Switch RMII Clocking Details*

The RMII interface reference clock pin operates as an input. When used as an input, the clock is driven by the I/O pad. The operation is controlled by the GMII\_SEL[RMIIx\_IO\_CLK\_EN] fields in the Control Module, as shown in [Figure](#page-7-0) 14-2.

```
                        +------------------+
                        |   Control Module |
                        |   GMII_SEL       |
                        +---------+--------+
                                  |
                           rmii1_io_clk_en
                                  |
                                  v
+---------+              +---------------------+
|  PRCM   |--------------> pd_per_cpsw_50mhz   |
| (Clock  |              |       gclk          |
|  ctrl)  |              +----------+----------+
+----+----+                         |
     |                              |
     |                              v
     |                   +-----------------------+
     |                   |   RMII1 REFCLK I/O    |
     |                   |                       |
     | rmii1_50mhz_clk ->|  clock buffer/driver  |-----> RMII1_REFCLK
     |                   |                       |
     |                   +-----------------------+
     |
     |
     |                   +-----------------------+
     |                   |   RMII2 REFCLK I/O    |
     | rmii2_50mhz_clk ->|  clock buffer/driver  |-----> RMII2_REFCLK
     |                   |                       |
     |                   +-----------------------+
     |
     v
+-------------------+
|   CPSW_3GSS       |
| (Ethernet switch) |
|                   |
| rmii1_mhz50_clk <-|
| rmii2_mhz50_clk <-|
+-------------------+
```
**Figure 14-2. Ethernet Switch RMII Clock Detail**

### *14.2.5 GMII Interface Signal Connections and Descriptions*

GMII Interface can operate in MII mode.

In MII Mode(100/10 Mbps) 3PSW operates in Full duplex and Half Duplex.

The pin connections of the GMII Interface are shown in [Figure](#page-8-0) 14-3.

```
 ┌───────────────────────────────────┐                      ┌──────────────────────────┐
 │           System Core             │                      │  Physical Layer Device   │
 │                                   │                      │          (PHY)           │
 │  ┌───────────────┐                │                      └──────────────────────────┘
 │  │     EMAC      │                │
 │  │               │<───────────────┤ GMII_TXCLK
 │  │               │───────────────▶│ GMII_TXD[3:0]
 │  │               │───────────────▶│ GMII_TXEN
 │  │               │<───────────────┤ GMII_COL
 │  │               │<───────────────┤ GMII_CRS
 │  │               │<───────────────┤ GMII_RXCLK
 │  │               │<───────────────┤ GMII_RXD[3:0]
 │  │               │<───────────────┤ GMII_RXDV
 │  │               │<───────────────┤ GMII_RXER
 │  └───────────────┘                │
 │                                   │
 │  ┌───────────────┐                │
 │  │     MDIO      │                │
 │  │               │───────────────▶│ MDIO_CLK
 │  │               │───────────────▶│ MDIO_DATA
 │  └───────────────┘                │
 └───────────────────────────────────┘
```
**Figure 14-3. MII Interface Connections**

See the detailed description of the signals in MII mode in the following sections.

### **Table 14-5. GMII Interface Signal Descriptions in MII (100/10Mbps) Mode**

| Signal        | Type | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|---------------|------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| GMII_TXCLK    | I    | The transmit clock is a continuous clock that provides the timing reference for transmit<br>operations. The GMII_TXD and GMII_TXEN signals are tied to this clock. The clock is<br>generated by the PHY and is 2.5 MHz at 10 Mbps operation and 25 MHz at 100 Mbps<br>operation.                                                                                                                                                                                                                                                                                           |
| GMII_TXD[3:0] | O    | The transmit data pins are a collection of 4 data signals GMII_TXD[3:0] comprising 4<br>bits of data. GMII_TXD[0] is the least-significant bit (LSB). The signals are<br>synchronized by GMII_TXCLK and valid only when GMII_TXEN is asserted.                                                                                                                                                                                                                                                                                                                             |
| GMII_TXEN     | O    | The transmit enable signal indicates that the GMII_TXD[3:0] pins are generating 4-bit<br>data for use by the PHY. It is driven synchronously by GMII_TXCLK                                                                                                                                                                                                                                                                                                                                                                                                                 |
| GMII_COL      | I    | In half-duplex operation, the GMII_COL pin is asserted by the PHY when it detects a<br>collision on the network. It remains asserted while the collision condition persists. This<br>signal is not necessarily synchronous to GMII_TXCLK nor GMII_RXCLK<br>In full-duplex operation, the GMII_COL pin is used for hardware transmit flow control.<br>Asserting the GMII_COL pin stops packet transmissions; packets transmitting when<br>MCOL is asserted will complete transmission. The GMII_COL pin should be held low if<br>hardware transmit flow control is not used |
| GMII_CRS      | I    | In half-duplex operation, the GMII_CRS pin is asserted by the PHY when the network<br>is not idle in either transmit or receive. The pin is deasserted when both transmit and<br>receive are idle. This signal is not necessarily synchronous to GMII_TXCLK nor<br>GMII_RXCLK.<br>In full-duplex operation, the GMII_CRS pin should be held low.                                                                                                                                                                                                                           |
| GMII_RXCLK    | I    | The receive clock is a continuous clock that provides the timing reference for receive<br>operations. The GMII_RXD, GMII_RXDV, and MRXER signals are tied to this clock.<br>The clock is generated by the PHY and is 2.5 MHz at 10 Mbps operation and 25 MHz<br>at 100 Mbps operation.                                                                                                                                                                                                                                                                                     |
| GMII_RXD[3:0] | I    | The receive data pins are a collection of 4 data signals comprising 4 bits of data.<br>GMII_RXD[0] is the least-significant bit (LSB).The signals are synchronized by<br>GMII_RXCLK and valid only when GMII_RXDV is asserted.                                                                                                                                                                                                                                                                                                                                             |
| GMII_RXDV     | I    | The receive data valid signal indicates that the GMII_RXD pins are generating nibble<br>data for use by the 3PSW. It is driven synchronously to GMII_RXCLK                                                                                                                                                                                                                                                                                                                                                                                                                 |
| MDIO_CLK      | O    | Management data clock (MDIO_CLK). The MDIO data clock is sourced by the MDIO<br>module on the system. It is used to synchronize MDIO data access operations done<br>on the MDIO_DATA pin.                                                                                                                                                                                                                                                                                                                                                                                  |
| MDIO_DATA     | I/O  | The MDIO_DATA pin drives PHY management data into and out of the PHY by way of<br>an access frame consisting of start of frame, read/write indication, PHY address,<br>register address, and data bit cycles. The MDIO_DATA pin acts as an output for all<br>but the data bit cycles at which time it is an input for read operations.                                                                                                                                                                                                                                     |

### *14.2.6 RMII Signal Connections and Descriptions*

[Figure](#page-10-0) 14-4 shows a device with integrated 3PSW and MDIO interfaced via a RMII connection in a typical system.

The individual CPSW and MDIO signals for the RMII interface are summarized in [Table](#page-10-1) 14-6.

For more information, see either the IEEE 802.3 standard or ISO/IEC 8802-3:2000(E).

```
                   +---------------------+
                   |        EMAC         |
                   |  (Ethernet MAC)     |
                   +----------+----------+
                              |
                              |  RMII TX (MAC → PHY)
                              |
                              |---- RMII_TXD[1:0] ----------->
                              |---- RMII_TXEN --------------->

                              |
                              |  RMII RX (PHY → MAC)
                              |
                              <---- RMII_RXD[1:0] -------------
                              <---- RMII_CRS_DV ---------------
                              <---- RMII_RXER -----------------

                              |
                              |  Reference clock
                              |
                              <---- RMII_REFCLK (50 MHz) -----

                   +---------------------+
                   |       MDIO I/O      |
                   +----------+----------+
                              |
                              |---- MDIO_CLK ------------------>
                              |<--> MDIO_DATA -----------------
                              |
                              v
                   +-----------------------------+
                   |     Physical Layer Device   |
                   |            (PHY)            |
                   +--------------+--------------+
                                  |
                                  |
                             50 MHz clock
                                  |
                                  v
                              +--------+
                              |Crystal |
                              | 50MHz  |
                              +--------+

                                  |
                                  v
                              +-----------+
                              |Transformer|
                              +-----------+
                                  |
                                  v
                               +------+
                               | RJ45 |
                               +------+
```

**Figure 14-4. RMII Interface Connections**

**Table 14-6. RMII Interface Signal Descriptions**

| Signal        | Type | Description                                                                                                                                                                                                                                                                                                                                        |
|---------------|------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| RMII_TXD[1-0] | O    | Transmit data. The transmit data pins are a collection of 2 bits of data.<br>RMII_TXD0 is the least-significant bit (LSB). The signals are synchronized by<br>RMII_REFCLK and valid only when RMII_TXEN is asserted.                                                                                                                               |
| RMII_TXEN     | O    | Transmit enable. The transmit enable signal indicates that the RMII_TXD pins<br>are generating data for use by the PHY. RMII_TXEN is synchronous to<br>RMII_MHZ_50_CLK.                                                                                                                                                                            |
|               |      | RMII reference clock.                                                                                                                                                                                                                                                                                                                              |
| RMII_REFCLK   | I    | The reference clock is used to synchronize all RMII signals. RMII_REFCLK must<br>be continuous and fixed at 50 MHz.                                                                                                                                                                                                                                |
| RMII_RXD[1-0] | I    | Receive data. The receive data pins are a collection of 2 bits of data.<br>RMII_RXD0 is the least-significant bit (LSB). The signals are synchronized by<br>RMII_REFCLK and valid only when RM_CRS_DV is asserted and RMII_RXER is<br>deasserted.                                                                                                  |
| RMII_CRS_DV   | I    | Carrier sense/receive data valid. Multiplexed signal between carrier sense and<br>receive data valid.                                                                                                                                                                                                                                              |
| RMII_RXER     | I    | Receive error. The receive error signal is asserted to indicate that an error was<br>detected in the received frame.                                                                                                                                                                                                                               |
| MDIO_CLK      | O    | Management data clock. The MDIO data clock is sourced by the MDIO module<br>on the system. It is used to synchronize MDIO data access operations done on<br>the MDIO pin.                                                                                                                                                                          |
| MDIO_DATA     | I/O  | MDIO DATA. MDIO data pin drives PHY management data into and out of the<br>PHY by way of an access frame consisting of start of frame, read/write<br>indication,PHY address, register address, and data bit cycles. The MDIO_DATA<br>signal acts as an output for all but the data bit cycles at which time it is an input<br>for read operations. |

### *14.2.7 RGMII Signal Connections and Descriptions*

[Figure](#page-11-0) 14-5 shows a device with integrated CPSW and MDIO interfaced via a RGMII connection in a typical system.

```
                     SYSTEM CORE
                 +------------------+
                 |                  |
                 |       EMAC       |
                 |                  |
                 +--------+---------+
                          |
                          |  RGMII_TX Interface
                          |
                          |---- RGMII_TCLK  -------------->
                          |---- RGMII_TD[3:0] ------------>
                          |---- RGMII_TCTL --------------->

                          |
                          |  RGMII_RX Interface
                          |
                          <---- RGMII_RCLK  ---------------
                          <---- RGMII_RD[3:0] -------------
                          <---- RGMII_RCTL ----------------

                 +------------------+
                 |     MDIO I/O     |
                 +--------+---------+
                          |
                          |---- MDIO_CLK ------------------>
                          |<--> MDIO_DATA -----------------
                          |
                          v
               +--------------------------+
               |   Physical Layer Device  |
               |          (PHY)           |
               +--------------------------+
```
**Figure 14-5. RGMII Interface Connections**

**Table 14-7. RGMII Interface Signal Descriptions**

| Signal        | Type | Description                                                                                                                                                          |
|---------------|------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| RGMII_TD[3-0] | O    | The transmit data pins are a collection of 4 bits of data. RGMII_RD0 is the least<br>significant bit (LSB).                                                          |
|               |      | The signals are valid only when RGMII_TCTL is asserted.                                                                                                              |
| RGMII_TCTL    | O    | Transmit Control/enable .The transmit enable signal indicates that the RGMII_TD<br>pins are generating data for use by the PHY.                                      |
| RGMII_TCLK    | O    | The transmit reference clock will be 125Mhz, 25Mhz, or 2.5Mhz depending on<br>speed of operation.                                                                    |
| RGMII_RD[3-0] | I    | The receive data pins are a collection of 4 bits of data. RGMII_RD is the least<br>significant bit (LSB).<br>The signals are valid only when RGMII_RCTL is asserted. |

### **Table 14-7. RGMII Interface Signal Descriptions (continued)**

| Signal     | Type | Description                                                                                                                                                                                                                                                                                                                                     |
|------------|------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| RGMII_RCTL | I    | The receive data valid/control signal indicates that the RGMII_RD pins are nibble<br>data for use by the 3PSW.                                                                                                                                                                                                                                  |
| RGMII_RCLK | I    | The receive clock is a continuous clock that provides the timing reference for<br>receive operations.The clock is generated by the PHY and is 2.5 MHz at 10<br>Mbps operation and 25 MHz at 100 Mbps operation,125 MHz at 1000Mbps of<br>operation.                                                                                             |
| MDIO_CLK   | O    | Management data clock. The MDIO data clock is sourced by the MDIO module<br>on the system. It is used to synchronize MDIO data access operations done on<br>the MDIO pin.                                                                                                                                                                       |
| MDIO_DATA  | I/O  | MDIO DATA. MDIO data pin drives PHY management data into and out of the<br>PHY by way of an access frame consisting of start of frame, read/write<br>indication,PHY address, register address, and data bit cycles. The MDIO_DATA<br>pin acts as an output for all but the data bit cycles at which time it is an input for<br>read operations. |

## **14.3 Functional Description**

The 3 port switch (3PSW) Ethernet Subsystem peripheral are compliant to the IEEE Std 802.3 Specification. The 3PSW Ethernet Subsystem contains two RGMII/RMII interfaces, one CPPI 3.0 interface, Interrupt Controller, MDIO and CPSW\_3G which contains two GMII interfaces as shown in [Figure](#page-19-0) 14-6.

The subsystem modules are discussed in detail in the following sections.

### *14.3.1 CPSW\_3G Subsystem*

#### **14.3.1.1 Interrupt Pacing**

The receive and transmit pulse interrupts can be paced. The receive threshold and miscellaneous interrupts are not paced. The Interrupt pacing feature limits the number of interrupts that occur during a given period of time. For heavily loaded systems in which interrupts can occur at a very high rate (e.g. 148,800 packets per second for Ethernet), the performance benefit is significant due to minimizing the overhead associated with servicing each interrupt. Interrupt pacing increases the CPU cache hit ratio by minimizing the number of times that large interrupt service routines are moved to and from the CPU instruction cache.

Each CPU receive and transmit pulse interrupt contains an interrupt pacing sub-block (six total). Each sub-block is disabled by default allowing the selected interrupt inputs to pass through unaffected. The interrupt pacing module counts the number of interrupts that occur over a 1ms interval of time. At the end of each 1ms interval, the current number of interrupts is compared with a target number of interrupts (specified by the associated maximum number of interrupts register).

Based on the results of the comparison, the length of time during which interrupts are blocked is dynamically adjusted. The 1ms interval is derived from a 4us pulse that is created from a prescale counter whose value is set in the int\_prescale value in the Int\_Control register. The int\_prescale value should be written with the number of VBUSP\_CLK periods in 4us. The pacing timer determines the interval during which interrupts are blocked and decrements every 4us. It is reloaded each time a zero count is reached. The value loaded into the pacing timer is calculated by hardware every 1ms according to the following algorithm:

```
if (intr_count > 2*intr_max)
pace_timer = 255;
else if (intr_count > 1.5*intr_max)
pace_timer = last_pace_timer*2 + 1;
else if (intr_count > 1.0*intr_max)
pace_timer = last_pace_timer + 1;
else if (intr_count > 0.5*intr_max)
pace_timer = last_pace_timer - 1;
else if (intr_count != 0)
pace_timer = last_pace_timer/2;
else
pace_timer = 0;
```

If the rate of interrupt inputs is much less than the target interrupt rate specified in the associated maximum interrupts register, then the interrupt is not blocked. If the interrupt rate is greater than the target rate, the interrupt will be "paced" at the rate specified in the interrupt maximum register. The interrupt maximum register should be written with a value between 2 and 63 inclusive indicating the target number of interrupts per milli-second.

#### **14.3.1.2 Reset Isolation**

Reset isolation for the Ethernet switch on Device is that the switch function of the ethernet IP remains active even in case of all device resets except for POR pin reset and ICEPICK COLD reset. Packet traffic to/from the 3PSW host will be flushed/dropped, but the ethernet switch will remain operational for all traffic between external devices on the switch even though the device is under-going a device reset.Pin mux configuration for ethernet related IO and reference clocks needed by the Ethernet switch IP to be active is controlled by a protected control module bit. If reset isolation is enabled, then only a POR pin or ICEPICK COLD reset event should fully reset the Ethernet switch IP including the actual switch and also the reference clocks and pin mux control specifically associated with the Ethernet IP.

#### *14.3.1.2.1 Modes of Operation*

The device has two modes of operation concerning the reset of the 3PSW Ethernet switch.

The mode is controlled by the ISO\_CONTROL bit in **RESET\_ISO** register of the **device control module**.This bit should default to '0'. Writes to the ISO\_CONTROL bit must be supervisor mode writes.

Mode 1: ISO\_CONTROL=0 (reset isolation disabled)

- This mode is selected when ISO\_CONTROL bit of control module is = 0. This should be the default state of the bit after control module reset.
- Upon any device level resets, the entire CPSW\_3GSS\_R IP, L3/L4, control module (including all pin mux control and the ISO\_CONTROL bit) is immediately reset.

Mode 2: ISO\_CONTROL=1 (reset isolation enabled)

- This mode is selected when ISO\_CONTROL bit of control module is = 1.
- Upon any device reset source other than POR pin or ICEPICK cold (so this includes SW global cold, any watchdog reset, warm RESETn pin, ICEPICK warm, SW global warm), the following should be true:
  - The CPSW\_3GSS\_R is put into 'isolate' mode and non-switch related portions of the IP are reset.
  - The 50-MHz and 125-MHz reference clocks to the 3PSW Ethernet Subsystem remains active throughout the entire reset condition.
  - The control for pin multiplexing for all of the signals should maintain their current configuration throughout the entire reset condition.
  - The reset isolated logic inside 3PSW Ethernet Subsystem IP which maintains the switch functionality

–

• Upon any cold reset sources, the entire 3PSW Ethernet Subsystem, control module (including all pin mux control and the ISO\_CONTROL bit itself) is reset.

#### **14.3.1.3 Interrupts**

The 3 Port Switch Ethernet Subsystem generates four interrupt events.

#### *14.3.1.3.1 Receive Packet Completion Pulse Interrupt (RX\_PULSE)*

The RX\_PULSE interrupt is a paced pulse interrupt selected from the 3PSW RX\_PEND [7:0] interrupts. The receive DMA controller has eight channels with each channel having a corresponding (RX\_PEND[7:0]).

The following steps will enable the receive packet completion interrupt.

- Enable the required channel interrupts of the DMA engine by setting 1 to the appropriate bit in the RX\_INTMASK\_SET register.
- The receive completion interrupt(s) to be routed to RX\_PULSE is selected by setting one or more bits in the receive interrupt enable register C*n*\_RX\_EN. The masked interrupt status can be read in the Receive Interrupt Masked Interrupt Status (Cn\_RX\_STAT) register.

When the 3PSW completes a packet reception, the subsystem issues an interrupt to the CPU by writing the packet's last buffer descriptor address to the appropriate channel queue's receive completion pointer located in the state RAM block. The interrupt is generated by the write when enabled by the interrupt mask, regardless of the value written.

Upon interrupt reception, the CPU processes one or more packets from the buffer chain and then acknowledges one or more interrupt(s) by writing the address of the last buffer descriptor processed to the queue's associated receive completion pointer (RX*n*\_CP) in the receive DMA state RAM.

Upon reception of an interrupt, software should perform the following:

- Read the RX\_STAT register to determine which channel(s) caused the interrupt.
- Process received packets for the interrupting channel(s).
- Write the 3PSW completion pointer(s) (RX*n*\_CP). The data written by the host (buffer descriptor address of the last processed buffer) is compared to the data in the register written by the subsystem (address of last buffer descriptor used by the subsystem). If the two values are not equal (which means that the 3PSW has received more packets than the CPU has processed), the receive packet completion interrupt signal remains asserted. If the two values are equal (which means that the host has processed all packets that the system has received), the pending interrupt is de-asserted. The value that the 3PSW is expecting is found by reading the receive channel*n* completion pointer register (RX*n*\_CP).
- Write the value 1h to the CPDMA\_EOI\_VECTOR register.

To disable the interrupt:

- The eight channel interrupts may be individually disabled by writing to 1 the appropriate bit in the RX\_INTMASK\_CLEAR
- The receive completion pulse interrupt could be disabled by clearing to 0 all the bits of the RX\_EN.

The software could still poll for the RX\_INTSTAT\_RAW and RX\_INTSTAT\_MASKED registers if the corresponding interrupts are enabled.

#### *14.3.1.3.2 Transmit Packet Completion Pulse Interrupt (TX\_PULSE)*

The TX\_PULSE interrupt is a paced pulse interrupt selected from the 3PSW TX\_PEND [7:0] interrupts. The transmit DMA controller has eight channels with each channel having a corresponding (TX\_PEND[7:0]).

To enable the transmit packet completion interrupt:

- Enable the required channel interrupts of the DMA engine by setting 1 to the appropriate bit in the TX\_INTMASK\_SET register.
- The transmit completion interrupt(s) to be routed to TX\_PULSE is selected by setting one or more bits in the transmit interrupt enable register C*n*\_TX\_EN .The masked interrupt status can be read in the Transmit Interrupt Masked Interrupt Status (C*n*\_TX\_STAT) register.

When the 3PSW completes the transmission of a packet, the 3PSW subsystem issues an interrupt to the CPU by writing the packet's last buffer descriptor address to the appropriate channel queue's transmit completion pointer located in the state RAM block. The interrupt is generated by the write when enabled by the interrupt mask, regardless of the value written.

Upon receiving an interrupt, software should perform the following:

- Read the TX\_STAT register to determine which channel(s) caused the interrupt
- Process received packets for the interrupting channel(s).
- Write the 3PSW completion pointer(s) (TX*n*\_CP). The data written by the host (buffer descriptor address of the last processed buffer) is compared to the data in the register written by the 3PSW (address of last buffer descriptor used by the 3PSW). If the two values are not equal (which means that the 3PSW has transmitted more packets than the CPU has processed), the transmit packet completion interrupt signal remains asserted. If the two values are equal (which means that the host has processed all packets that the subsystem has transferred), the pending interrupt is cleared. The value that the 3PSW is expecting is found by reading the transmit channel*n* completion pointer register (TX*n*\_CP).
- Write the 2h to the CPDMA\_EOI\_VECTOR register.

To disable the interrupt:

- The eight channel interrupts may be individually disabled by writing to 1 the appropriate bit in the TX\_INTMASK\_CLEAR.
- The receive completion pulse interrupt could be disabled by clearing to 0 all the bits of the TX\_EN. The software could still poll for the TX\_INTSTAT\_RAW and TX\_INTSTAT\_MASKED registers if the corresponding interrupts are enabled.

#### *14.3.1.3.3 Receive Threshold Pulse Interrupt (RX\_THRESH\_PULSE)*

The RX\_THRESH\_PULSE interrupt is an immediate (non-paced) pulse interrupt selected from the CPSW\_3G RX\_THRESH\_PEND[7:0] interrupts. The receive DMA controller has eight channels with each channel having a corresponding threshold pulse interrupt (RX\_THRESH\_PEND [7:0]).

To enable the receive threshold pulse Interrupt:

- Enable the required channel interrupts of the DMA engine by setting 1 to the appropriate bit in the RX\_INTMASK\_SET register.
- The receive threshold interrupt(s) to be routed to RX\_THRESH\_PULSE is selected by setting one or more bits in the receive threshold interrupt enable register RX\_THRESH\_EN. The masked interrupt status can be read in the Receive Threshold Masked Interrupt Status (C*n*\_RX\_THRESH\_STAT) register.

The RX\_THRESH\_PULSE is asserted when enabled when the channel's associated free buffer count RX*n*\_FREEBUFFER is less than or equal to the corresponding RX*n*\_PENDTHRESH register.

Upon receiving an interrupt, software should perform the following:

- Read the C*n*\_RX\_THRESH\_STAT bit address location to determine which channel(s) caused the interrupt.
- Process the received packets in order to add more buffers to any channel that is below the threshold value.
- Write the CPSW\_3G completion pointer(s).
- Write the value 0h to the CPDMA\_EOI\_VECTOR register.

The threshold pulse interrupt is an immediate interrupt intended to indicate that software should immediately process packets to preclude an overrun condition from occurring for the particular channels.

To disable the interrupt:

- The eight channel receive threshold interrupts may be individually disabled by writing to 1 the appropriate bit in the RX\_INTMASK\_CLEAR register.
- The receive threshold pulse interrupt could be disabled by clearing to Zero the corresponding bits of the RX\_THRESH\_EN. The software could still poll for the RX\_INTSTAT\_RAW and INTSTAT\_MASKED registers if the corresponding interrupts are enabled.

#### *14.3.1.3.4 Miscellaneous Pulse Interrupt (MISC\_PULSE)*

The MISC\_PULSE interrupt is an immediate, non-paced, pulse interrupt selected from the miscellaneous interrupts (EVNT\_PEND, STAT\_PEND, HOST\_PEND, MDIO\_LINKINT, MDIO\_USERINT). The miscellaneous interrupt(s) is selected by setting one or more bits in the C*n*\_MISC\_EN[4:0] register. The masked interrupt status can be read in the C*n*\_MISC\_STAT[4:0] register. Upon reception of an interrupt, software should perform the following:

- Read the Misc\_Stat[4:0] register to determine which caused the interrupt.
- Process the interrupt.
- Write the appropriate value (0x3) to the CPDMA\_EOI\_VECTOR register.
- Write a 1 to the appropriate bit in the MDIOLINKINTRAW register.

This device does not support multiple link interrupts. Only MDIO\_LINKINT[0] and MDIO\_USERINT[0] are used. MDIO\_LINKINT[1] and MDIO\_USERINT[1] are not used.

#### *14.3.1.3.4.1 EVNT\_PEND (CPTS\_PEND) Interrupt*

See [Section](#page-65-0) 14.3.7, *Common Platform Time Sync (CPTS)*, for information on the time sync event interrupt.

#### *14.3.1.3.4.2 Statistics Interrupt*

The statistics level interrupt (STAT\_PEND) will be asserted if enabled when any statistics value is greater than or equal to 0x80000000. The statistics interrupt is cleared by writing to decrement all statistics values greater than 0x80000000 (such that their new values are less than 0x80000000). The statistics interrupt is enabled by setting to one the appropriate bit in the INTMASK\_SET register in the CPDMA submodule.

The statistics interrupt is disabled by writing one to the appropriate bit in the INTMASK\_CLEAR register. The raw and masked statistics interrupt status may be read by reading the TX\_IntStat\_Raw and TX\_IntStat\_Masked registers, respectively

#### *14.3.1.3.4.3 Host Error Interrupt*

The host error interrupt (HOST\_PEND) will be asserted if enabled when a host error is detected during transmit or receive CPDMA transactions. The host error interrupt is intended for software debug, and is cleared by a warm reset or a system reset. The raw and masked statistics interrupt status can be read by reading the TX\_INTSTAT\_RAW and TXINTSTAT\_MASKED registers, respectively.

The following list shows the transmit host error conditions:

- SOP error
- OWNERSHIP bit not set in SOP buffer
- next buffer descriptor pointer without EOP set to 0
- buffer pointer set to 0
- buffer length set to 0
- packet length error

The receive host error conditions are show in the following list:

- Ownership bit not set in input buffer.
- Zero buffer pointer.
- Zero buffer Length on non-SOP descriptor.
- SOP buffer length not greater than offset.

The host error interrupt is disabled by clearing to 0 the appropriate bit in the CPDMA\_INTMASK\_CLEAR register.

#### *14.3.1.3.4.4 MDIO Interrupts*

MDIO\_LINKINT is set if there is a change in the link state of the PHY corresponding to the address in the PHYADDRMON field of the MDIOUSERPHYSEL*n* register and the corresponding LINKINTENB bit is set.The MDIO\_LINKINT event is also captured in the MDIOLINKINTMASKED register.When the GO bit in the MDIOUSERACCESS*n* registers transitions from 1 to 0, indicating the completion of a user access, and the corresponding USERINTMASKSET bit in the MDIOUSERINTMASKSET register is set, the MDIO\_USERINT signal is asserted 1. The MDIO\_USERINT event is also captured in the MDIOUSERINTMASKED register.

To enable the miscellaneous pulse interrupt:

The miscellaneous interrupt(s) is selected by setting one or more bits in the miscellaneous interrupt enable register (MISC\_EN).

- The Statistics interrupt is enabled by setting to 1 the STAT\_INT\_MASK bit in the DMA\_INTMASK\_SET register.
- The HOST\_PEND is enabled by setting to 1 the HOST\_ERR\_INT\_MASK in the DMA\_INTMASK\_SET register.

Upon receiving of an interrupt, software should perform the following:

- Read the C*n*\_MISC\_STAT register to determine the source of the interrupt.
- Process the interrupt.
- Write the value 3h to the CPDMA\_EOI\_VECTOR register.

#### **14.3.1.4 Embedded Memories**

| Memory Type Description             | Number of Instances |
|-------------------------------------|---------------------|
| Single port 2560 by 64 RAM          | 3 (Packet FIFOs)    |
| Single port 64-word by 1152-bit RAM | 1 (ALE)             |
| Single port 2048-word by 32-bit RAM | 1 (CPPI)            |

### *14.3.2 CPSW\_3G*

The CPSW\_3G GMII interfaces are compliant to the IEEE Std 802.3 Specification.

The CPSW\_3G contains two CPGMAC\_SL interfaces (ports 1 and 2), one CPPI 3.0 interface Host Port (port 0), Common Platform Time Sync (CPTS), ALE Engine and CPDMA.

The top level block diagram of CPSW\_3G is shown below:

```
      ┌────────┐
      │  ALE   │
      └───┬────┘
          │
 ┌────────▼────────┐
 │       CR        │
 └───┬─────────────┘
     │
     │
     │          ┌───────────┐
     │─────────▶│ CPSW_FIFO │
     │          └───────────┘
     │          ┌───────────┐
     │─────────▶│ CPSW_FIFO │
     │          └───────────┘
     │          ┌───────────┐
     │─────────▶│ CPSW_FIFO │
     │          └───────────┘
     │
 ┌───▼────────────┐
 │      SCR       │
 └────────────────┘
     │   (bus to RX/TX FIFO DB and CPDMA)
     │
     ├───────────────┐
     │               │
     │        ┌────────────┐
     │        │ RX_FIFO_DB │
     │        └────────────┘
     │        ┌────────────┐
     │        │ TX_FIFO_DB │
     │        └────────────┘
     │               │
     │               ▼
     │        ┌────────────┐
     └───────▶│   CPDMA    │
              └────────────┘
                 ▲   ▲   ▲   ▲   ▲   ▲
                 │   │   │   │   │   │
                 │   │   │   │   │   └── STAT_PEND
                 │   │   │   │   └────── HOST_PEND
                 │   │   │   └────────── TX_PEND
                 │   │   └────────────── RX_PEND
                 │   └────────────────── TX
                 └────────────────────── RX

    (upper right block)
           ┌────────┐
           │  CPTS  │──────────────▶ EVNT_PEND
           └───┬────┘
               │
           ┌───▼────┐
           │ STATS  │
           └───┬────┘
               │
      ┌────────▼──────────┐
      │    CPGMAC_SL      │──────────────▶ GMII_1
      └────────┬──────────┘
               │
      ┌────────▼──────────┐
      │    CPGMAC_SL      │──────────────▶ GMII_0
      └───────────────────┘
```
**Figure 14-6. CPSW\_3G Block Diagram**

#### **14.3.2.1 Media Independent Interface (GMII)**

The CPSW\_3G contains two CPGMAC\_SL submodules. Each CPGMAC\_SL has a single GMII interface. The CPGMAC\_SL submodules are ports 1 and 2. For more details, see [Section](#page-61-0) 14.3.3, *Ethernet Mac Sliver (CPGMAC\_SL)*.

#### **14.3.2.2 IEEE 1588v2 Clock Synchronization Support**

The CPSW\_3G supports IEEE 1588v2 clock synchronization. Ethernet GMII Transmit (egress) and receive (ingress) time sync operation are also supported.

#### *14.3.2.2.1 IEEE 1588v2 Receive Packet Operation*

There are two CPSW\_3G receive time sync interfaces for each ethernet port. The first is the TS\_RX\_MII interface and the second is the TS\_RX\_DEC interface. Both interfaces are generated in the switch and are input to the CPTS module. There are register bits in the CPSW\_3G that control time sync operations in addition to the registers in the CPTS module. The pX\_ts\_rx\_en bit in the switch Px\_Control register must be set for receive time sync operation to be enabled (TS\_RX\_MII).

The TS\_RX\_MII interface issues a record signal (pX\_ts\_rx\_mii\_rec) along with a handle (pX\_ts\_rx\_mii\_hndl[3:0]) to the CPTS controller for each packet that is received. The record signal is a single clock pulse indicating that a receive packet has been detected at the associated port MII interface.

The handle value is incremented with each packet and rolls over to zero after 15. There are 16 possible handle values so there can be a maximum of 16 packets "in flight" from the TS\_RX\_MII to the TS\_RX\_DEC block (through the CPGMAC\_SL) at any given time. A handle value is reused (not incremented) for any received packet that is shorter than about 31 octets (including preamble). Handle reuse on short packets prevents any possible overrun condition if multiple fragments are consecutively received.

The TS\_RX\_MII logic is in the receive wireside clock domain. There is no decode logic in the TS\_RX\_MII to determine if the packet is a time sync event packet or not. Each received packet generates a record signal and new handle. The handle is sent to the CPTS controller with the record pulse and the handle is also sent to the TS\_RX\_DEC block along with the packet. The packet decode is performed in the TS\_RX\_DEC block. The decode function is separated from the record function because in some systems the incoming packet can be encrypted. The decode function would be after packet decryption in those systems.

The TS\_RX\_DEC function decodes each received packet and determines if the packet meets the time sync event packet criteria. If the packet is determined to be a time sync event packet, then the time sync event packet is signaled to the CPTS controller via the TS\_RX\_DEC interface (pX\_ts\_rx\_dec\_evnt, pX\_ts\_rx\_dec\_hndl[3:0], pX\_ts\_rx\_dec\_msg\_type, pX\_ts\_rx\_dec\_seq\_id). The event signal is a single clock pulse indicating that the packet matched the time sync event packet criteria and that the associated packet handle, message type, and sequence ID are valid. No indication is given for received packets that do not meet the time sync event criteria. The 16-bit sequence ID is found in the time sync event packet at the sequence ID offset into the PTP message header (pX\_ts\_seq\_id\_offset). A packet is determined to be a receive event packet under the following conditions:

#### *14.3.2.2.1.1 Annex F*

- 1. Receive time sync is enabled (pX\_ts\_rx\_en is set in the switch Px\_Control register).
- 2. One of the following sequences is true:

Where the first packet ltype matches:

- ts\_ltype1 and pX\_ts\_ltype1\_en is set
- ts\_ltype2 and pX\_ts\_ltype2\_en is set
- vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches ts\_ltype1 and pX\_ts\_ltype1\_en is set
- vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches ts\_ltype2 and pX\_ts\_ltype2\_en is set
- vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the second packet ltype matches ts\_ltype1 and pX\_ts\_ltype1\_en is set
- vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the second packet ltype matches ts\_ltype2 and pX\_ts\_ltype2\_en is set
- vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the third packet ltype matches ts\_ltype1 and pX\_ts\_ltype1\_en is set
- vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the third packet ltype matches ts\_ltype2 and pX\_ts\_ltype2\_en is set
- 3. The PTP message begins in the byte after the LTYPE.
- 4. The packet message type is enabled in the pX\_ts\_msg\_type\_en field in the Px\_TS\_SEQ\_MTYPE register.
- 5. The packet was received without error (not long/short/mac\_ctl/crc/code/align).
- 6. The ALE determined that the packet is to be sent only to the host (port 0).

#### *14.3.2.2.1.2 Annex D*

- 1. Receive time sync is enabled (pX\_ts\_rx\_en is set in the switch Px\_Control register).
- 2. One of the following sequences is true:

Where the first packet ltype matches:

- 0x0800 and pX\_ts\_annex\_d\_en is set
- vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches 0x0800 and pX\_ts\_annex\_d\_en is set
- vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the second packet ltype matches 0x0800 and pX\_ts\_annex\_d\_en is set
- vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the third packet ltype matches 0x0800 and pX\_ts\_annex\_d\_en is set
- 3. Byte 14 (the byte after the LTYPE) contains 0x45 (IP\_VERSION).

**Note:** The byte numbering assumes that there are no vlans. The byte number is intended to show the relative order of the bytes.

4. Byte 22 contains 0x00 if the pX\_ts\_ttl\_nonzero bit in the switch Px\_Control register is zero or byte 22 contains any value if pX\_ts\_ttl\_nonzero is set. Byte 22 is the time to live field.

- 5. Byte 23 contains 0x11 (UDP Fixed).
- 6. Byte 30 contains decimal 224 (0xe0).
- 7. Byte 31 contains 0x00.
- 8. Byte 32 contains 0x01.
- 9. Byte 33 contains one of the following:
  - Decimal 129 and the pX\_ts\_129 bit in the switch Px\_Control register is set
  - Decimal 130 and the pX\_ts\_130 bit in the switch Px\_Control register is set
  - Decimal 131 and the pX\_ts\_131 bit in the switch Px\_Control register is set
  - Decimal 129 and the pX\_ts\_132 bit in the switch Px\_Control register is set
- 10. Bytes 36 and 37 contain one of the following:
  - Decimal 0x01 and 0x3f respectively and and the pX\_ts\_319 bit in the switch Px\_Control register is set.
  - Decimal 0x01 and 0x40 respectively and and the pX\_ts\_320 bit in the switch Px\_Control register is set.
- 11. The PTP message begins in byte 42.
- 12. The packet message type is enabled in the pX\_ts\_msg\_type\_en field in Px\_Control.
- 13. The packet was received without error (not long/short/mac\_ctl/crc/code/align).
- 14. The ALE determined that the packet is to be sent only to the host (port 0).

#### *14.3.2.2.2 IEEE 1588v2 Transmit Packet Operation*

There are two CPSW\_3G transmit time sync interfaces for each ethernet port. The first is the TS\_TX\_DEC interface and the second is the TS\_TX\_MII interface. Both interfaces are generated in the switch and are input to the CPTS module. The pX\_ts\_tx\_en bit in the Px\_Control register must be set for transmit time sync operation to be enabled.

The TS\_TX\_DEC function decodes each packet to be transmitted and determines if the packet meets the time sync event packet criteria. If the packet is determined to be a time sync event packet, then the time sync event is signaled to the CPTS controller via the TS\_TX\_DEC interface (pX\_ts\_tx\_dec\_evnt, pX\_ts\_tx\_dec\_hndl[3:0], pX\_ts\_tx\_dec\_msg\_type, pX\_ts\_tx\_dec\_seq\_id). The event signal is a single clock pulse indicating that the packet matched the time sync event packet criteria and that the associated packet handle, message type, and sequence ID are valid.

The 16-bit sequence ID is found in the time sync event packet at the sequence ID offset into the message header (pX\_ts\_seq\_id\_offset). No indication is given for transmit packets that do not meet the time sync event criteria. The time sync event packet handle is also passed along with the packet to the TS\_TX\_MII with an indication that the packet is a time sync event packet. Unlike receive, only transmit event packets increment the handle value. The decode function is separated from the record function because some systems may encript the packet. The encription is after the decode function on transmit (egress). A packet is determined to be a transmit event packet if the following is met:

#### *14.3.2.2.2.1 Annex F*

- 1. Transmit time sync is enabled (pX\_ts\_tx\_en is set in the switch Px\_Control register).
- 2. One of the following sequences is true:
  - The first packet ltype matches ts\_ltype1 and pX\_ts\_ltype1\_en is set
  - The first packet ltype matches ts\_ltype2 and pX\_ts\_ltype2\_en is set
  - The first packet ltype matches vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet matches ts\_ltype1 and pX\_ts\_ltype1\_en is set
  - The first packet ltype matches vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches ts\_ltype2 and pX\_ts\_ltype2\_en is set
  - The first packet ltype matches vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the second packet ltype matches ts\_ltype1 and pX\_ts\_ltype1\_en is set
  - The first packet ltype matches vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the second packet

ltype matches ts\_ltype2 and pX\_ts\_ltype2\_en is set

- The first packet ltype matches vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the third packet ltype matches ts\_ltype1 and pX\_ts\_ltype1\_en is set
- The first packet ltype matches vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the third packet ltype matches ts\_ltype2 and pX\_ts\_ltype2\_en is set
- 3. The packet message type is enabled in pX\_ts\_msg\_type\_en.
- 4. The packet was received by the host (port 0).

#### *14.3.2.2.2.2 Annex D*

- 1. Transmit time sync is enabled (pX\_ts\_tx\_en is set in the switch Px\_Control register).
- 2. One of the following sequences is true:
  - The first packet ltype matches 0x0800 and pX\_ts\_annex\_d\_en is set
  - The first packet ltype matches vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches 0x0800 and pX\_ts\_annex\_d\_en is set
  - The first packet ltype matches vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the second packet ltype matches 0x0800 and pX\_ts\_annex\_d\_en is set
  - The first packet ltype matches vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the third packet ltype matches 0x0800 and pX\_ts\_annex\_d\_en is set
- 3. Byte 14 (the byte after the LTYPE) contains 0x45 (IP\_VERSION).

**Note:** The byte numbering assumes that there are no vlans. The byte number is intended to show the relative order of the bytes. If VLAN(s) are present, then the byte numbers push down.

- 4. Byte 22 contains 0x00 if the pX\_ts\_ttl\_nonzero bit in the switch Px\_Control register is zero or byte 22 contains any value if pX\_ts\_ttl\_nonzero is set. Byte 22 is the time to live field.
- 5. Byte 23 contains 0x11 (UDP Fixed).
- 6. Byte 30 contains decimal 224 (0xe0)
- 7. Byte 31 contains 0x00
- 8. Byte 32 contains 0x01
- 9. Byte 33 contains one of the following:
  - Decimal 129 and the pX\_ts\_129 bit in the switch Px\_Control register is set
  - Decimal 130 and the pX\_ts\_130 bit in the switch Px\_Control register is set
  - Decimal 131 and the pX\_ts\_131 bit in the switch Px\_Control register is set
  - Decimal 132 and the pX\_ts\_132 bit in the switch Px\_Control register is set
- 10. Bytes 36 and 37 contain either of the following:
  - Decimal 1 (hex 0x01) and decimal 63 (hex 0x3f) respectively and and the pX\_ts\_319 bit in the switch Px\_Control register is set
  - Decimal 1 (hex 0x01) and decimal 64 (hex 0x40) respectively and and the pX\_ts\_320 bit in the switch Px\_Control register is set
- 11. The PTP message begins in byte 42 (this is offset 0).
- 12. The packet message type is enabled in pX\_ts\_msg\_type\_en.
- 13. The packet was received by the host (port 0).

The TS\_TX\_MII interface issues a single clock record signal (pX\_ts\_tx\_mii\_rec) at the beginning of each transmitted packet. If the packet is a time sync event packet then a single clock event signal (pX\_ts\_tx\_mii\_evnt) along with a handle (pX\_ts\_rx\_mii\_hndl[2:0]) will be issued before the next record signal for the next packet. The event signal will not be issued for packets that did not meet the time sync event criteria in the TS\_TX\_DEC function. If consecutive record indications occur without an interleaving event indication, then the packet associated with the first record was not a time sync event packet.

The record signal is a single clock pulse indicating that a transmit packet egress has been detected at the associated port MII interface. The handle value is incremented with each time sync event packet and rolls over to zero after 7. There are 8 possible handle values so there can be a maximum of eight time sync event packets "in flight" from the TS\_TX\_DEC to the TS\_TX\_MII block at any given time. The handle value increments only on time sync event packets. The TS\_TX\_MII logic is in the transmit wireside clock domain.

#### **14.3.2.3 Device Level Ring (DLR) Support**

Device Level Ring (DLR) support is enabled by setting the dlr\_en bit in the CPSW\_Control register. When enabled, incoming received DLR packets are detected and sent to queue 3 (highest priority) of the egress port(s). If the host port is the egress port for a DLR packet then the packet is sent on the CPDMA Rx channel selected by the p0\_dlr\_cpdma\_ch field in the P0\_Control register. The supervisor node MAC address feature is supported with the dlr\_unicast bit in the unicast address table entry.

When set, the dlr\_unicast bit causes a packet with the matching destination address to be flooded to the vlan\_member\_list minus the receive port and minus the host port (the port\_number field in the unicast address table entry is a don't care). Matching dlr\_unicast packets are flooded regardless of whether the packet is a DLR packet or not. The en\_p0\_uni\_flood bit in the ALE\_Control register has no effect on DLR unicast packets. Packets are determined to be DLR packets, as shown:

- 1. DLR is enabled (dlr\_en is set in the switch CPSW\_Control register).
- 2. One of the following sequences is true:
  - The first packet ltype matches vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches dlr\_ltype.
  - The first packet ltype matches vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the second packet ltype matches dlr\_ltype.
  - The first packet ltype matches vlan\_ltype1 and pX\_vlan\_ltype1\_en is set and the second packet ltype matches vlan\_ltype2 and pX\_vlan\_ltype2\_en is set and the third packet ltype matches dlr\_ltype.

#### **14.3.2.4 CPDMA RX and TX Interfaces**

The CPDMA submodule is a CPPI 3.0 compliant packet DMA transfer controller. The CPPI 3.0 interface is port 0.

After reset, initialization, and configuration the host may initiate transmit operations. Transmit operations are initiated by host writes to the appropriate transmit channel head descriptor pointer contained in the STATERAM block. The transmit DMA controller then fetches the first packet in the packet chain from memory in accordance with CPPI 3.0 protocol. The DMA controller writes the packet into the external transmit FIFO in 64-byte bursts (maximum).

Receive operations are initiated by host writes to the appropriate receive channel head descriptor pointer after host initialization and configuration. The receive DMA controller writes the receive packet data to external memory in accordance with CPPI 3.0 protocol. See the CPPI Buffer Descriptors section for detailed description of Buffer Descriptors

#### *14.3.2.4.1 CPPI Buffer Descriptors*

The buffer descriptor is a central part of the 3PSW Ethernet Subsytem and is how the application software describes Ethernet packets to be sent and empty buffers to be filled with incoming packet data.

Host Software sends and receives network frames via the CPPI 3.0 compliant host interface. The host interface includes module registers and host memory data structures. The host memory data structures are buffer descriptors and data buffers. Buffer descriptors are data structures that contain information about a single data buffer. Buffer descriptors may be linked together to describe frames or queues of frames for transmission of data and free buffer queues available for received data.

Note: The 8k bytes of Ethernet Subsystem CPPI RAM begin at address 0x4a102000 and end at 0x4a103FFF from the 3PSW perspective. The buffer descriptors programmed to access the CPPI RAM memory should use the address range from 0x4a102000.

#### *14.3.2.4.1.1 TX Buffer Descriptors*

A TX buffer descriptor is a contiguous block of four 32-bit data words aligned on a 32-bit word boundary.

Word 0 31 0 Next Descriptor Pointer Word 1 31 0 Buffer Pointer Word 2 31 16 15 0 Buffer Offset Buffer Length Word 3 31 30 29 28 27 26 25 21 20 19 18 17 16 SOP EOP Owner ship EOQ Teardown\_Com plete Pass CRC Reserved To\_Po rt\_En Reserved To\_Port 15 11 10 0 Reserved packet\_length

**Figure 14-7. Tx Buffer Descriptor Format**

#### *14.3.2.4.1.1.1 CPPI Tx Data Word – 0*

### **Next Descriptor Pointer**

The next descriptor pointer points to the 32-bit word aligned memory address of the next buffer descriptor in the transmit queue. This pointer is used to create a linked list of buffer descriptors. If the value of this pointer is zero, then the current buffer is the last buffer in the queue. The software application must set this value prior to adding the descriptor to the active transmit list. This pointer is not altered by the EMAC.The value of pNext should never be altered once the descriptor is in an active transmit queue, unless its current value is NULL.

If the pNext pointer is initially NULL, and more packets need to be queued for transmit, the software application may alter this pointer to point to a newly appended descriptor. The EMAC will use the new pointer value and proceed to the next descriptor unless the pNext value has already been read. In this latter case, the transmitter will halt on the transmit channel in question, and the software application may restart it at that time. The software can detect this case by checking for an end of queue (EOQ) condition flag on the updated packet descriptor when it is returned by the EMAC.

#### *14.3.2.4.1.1.2 CPPI Tx Data Word – 1*

**Buffer Pointer**

The byte aligned memory address of the buffer associated with the buffer descriptor. The host sets the **buffer\_pointer**. The software application must set this value prior to adding the descriptor to the active transmit list. This pointer is not altered by the EMAC.

#### *14.3.2.4.1.1.3 CPPI Tx Data Word – 2*

#### **Buffer \_Offset**

Buffer Offset – Indicates how many unused bytes are at the start of the buffer. A value of 0x0000 indicates that no unused bytes are at the start of the buffer and that valid data begins on the first byte of the buffer. A value of 0x000F (decimal 15) indicates that the first 15 bytes of the buffer are to be ignored by the port and that valid buffer data starts on byte 16 of the buffer. The host sets the buffer\_offset value (which may be zero to the buffer length minus 1). Valid only on sop.

#### **Buffer \_Length**

Buffer Length – Indicates how many valid data bytes are in the buffer. Unused or protocol specific bytes at the beginning of the buffer are not counted in the Buffer Length field. The host sets the buffer\_length. The buffer\_length must be greater than zero.

#### *14.3.2.4.1.1.4 CPPI Tx Data Word – 3*

## **Start of Packet (SOP) Flag**

When set, this flag indicates that the descriptor points to a packet buffer that is the start of a new packet.In the case of a single fragment packet, both the SOP and end of packet (EOP) flags are set. Otherwise,the descriptor pointing to the last packet buffer for the packet sets the EOP flag. This bit is set by the software application and is not altered by the EMAC.

- 0 Not start of packet buffer
- 1 Start of packet buffer

#### **End of Packet (EOP) Flag**

When set, this flag indicates that the descriptor points to a packet buffer that is last for a given packet. In the case of a single fragment packet, both the start of packet (SOP) and EOP flags are set. Otherwise, the descriptor pointing to the last packet buffer for the packet sets the EOP flag. This bit is set by the software application and is not altered by the EMAC.

- 0 Not end of packet buffer.
- 1 End of packet buffer.

## **Ownership**

When set this flag indicates that all the descriptors for the given packet (from SOP to EOP) are currently owned by the EMAC. This flag is set by the software application on the SOP packet descriptor before adding the descriptor to the transmit descriptor queue. For a single fragment packet, the SOP, EOP, and OWNER flags are all set. The OWNER flag is cleared by the EMAC once it is finished with all the descriptors for the given packet. Note that this flag is valid on SOP descriptors only.

- 0 The packet is owned by the host
- 1 The packet is owned by the port

#### **EOQ**

When set, this flag indicates that the descriptor in question was the last descriptor in the transmit queue for a given transmit channel, and that the transmitter has halted. This flag is initially cleared by the software application prior to adding the descriptor to the transmit queue. This bit is set by the EMAC when the EMAC identifies that a descriptor is the last for a given packet (the EOP flag is set), and there are no more descriptors in the transmit list (next descriptor pointer is NULL).

The software application can use this bit to detect when the EMAC transmitter for the corresponding channel has halted. This is useful when the application appends additional packet descriptors to a transmit queue list that is already owned by the EMAC. Note that this flag is valid on EOP descriptors only.

0 - The Tx queue has more packets to transfer.

1 - The Descriptor buffer is the last buffer in the last packet in the queue.

### **teardown\_Complete**

This flag is used when a transmit queue is being torn down, or aborted, instead of allowing it to be

transmitted. This would happen under device driver reset or shutdown conditions. The EMAC sets this bit in the SOP descriptor of each packet as it is aborted from transmission. Note that this flag is valid on SOP descriptors only. Also note that only the first packet in an unsent list has the TDOWNCMPLT flag set. Subsequent descriptors are not processed by the EMAC.

- 0 The port has not completed the teardown process.
- 1 The port has completed the commanded teardown process.

#### **pass\_crc**

This flag is set by the software application in the SOP packet descriptor before it adds the descriptor to the transmit queue. Setting this bit indicates to the EMAC that the 4 byte Ethernet CRC is already present in the packet data, and that the EMAC should not generate its own version of the CRC.When the CRC flag is cleared, the EMAC generates and appends the 4-byte CRC. The buffer length and packet length fields do not include the CRC bytes. When the CRC flag is set, the 4-byte CRC is supplied by the software application and is already appended to the end of the packet data. The buffer length and packet length fields include the CRC bytes, as they are part of the valid packet data. Note that this flag is valid on SOP descriptors only.

- 0 The CRC is not included with the packet data and packet length.
- 1 The CRC is included with the packet data and packet length.

## **to\_port**

To Port – Port number to send the directed packet to. This field is set by the host. This field is valid on SOP. Directed packets go to the directed port, but an ALE lookup is performed to determine untagged egress in VLAN\_AWARE mode.

- 1 Send the packet to port 1 if to\_port\_en is asserted.
- 2 Send the packet to port 2 if to\_port\_en is asserted.

#### **To\_port\_enable**

To Port Enable – Indicates when set that the packet is a directed packet to be sent to the to\_port field port number. This field is set by the host. The packet is sent to one port only (index not mask). This bit is valid on SOP.

- 0 not a directed packet
- 1 directed packet

#### **Packet Length**

Specifies the number of bytes in the entire packet. Offset bytes are not included. The sum of the buffer\_length fields should equal the packet\_length. Valid only on SOP. The packet length must be greater than zero. The packet data will be truncated to the packet length if the packet length is shorter than the sum of the packet buffer descriptor buffer lengths. A host error occurs if the packet length is greater than the sum of the packet buffer descriptor buffer lengths

#### *14.3.2.4.1.2 RX Buffer Descriptors*

An RX buffer descriptor is a contiguous block of four 32-bit data words aligned on a 32-bit word boundary.

|          |          |               |     |                           |                    |      |       |                           | Figure 14-8. Rx Buffer Descriptor Format |               |         |                   |           |    |
|----------|----------|---------------|-----|---------------------------|--------------------|------|-------|---------------------------|------------------------------------------|---------------|---------|-------------------|-----------|----|
| Word 0   |          |               |     |                           |                    |      |       |                           |                                          |               |         |                   |           |    |
| 31       |          |               |     |                           |                    |      |       |                           |                                          |               |         |                   |           | 0  |
|          |          |               |     |                           |                    |      |       | Next Descriptor Pointer   |                                          |               |         |                   |           |    |
| Word 1   |          |               |     |                           |                    |      |       |                           |                                          |               |         |                   |           |    |
| 31       |          |               |     |                           |                    |      |       |                           |                                          |               |         |                   |           | 0  |
|          |          |               |     |                           |                    |      |       | Buffer Pointer            |                                          |               |         |                   |           |    |
| Word 2   |          |               |     |                           |                    |      |       |                           |                                          |               |         |                   |           |    |
| 31       |          | 27<br>26      |     |                           |                    |      | 16    | 15                        |                                          | 11<br>10      |         |                   |           | 0  |
|          | Reserved |               |     |                           | Buffer Offset      |      |       | Reserved<br>Buffer Length |                                          |               |         |                   |           |    |
| Word 3   |          |               |     |                           |                    |      |       |                           |                                          |               |         |                   |           |    |
| 31       | 30       | 29            | 28  | 27                        | 26                 | 25   | 24    | 23                        | 22                                       | 21            | 20      | 19                | 18        | 16 |
| SOP      | EOP      | Owner<br>ship | EOQ | Teardo<br>wn_Co<br>mplete | Passe<br>d_CR<br>C | Long | Short | MAC_<br>Ctl               | Overru<br>n                              |               | PKT_Err | Rx_Vlan_Enca<br>p | From_Port |    |
| 15       |          |               |     | 11                        | 10                 |      |       |                           |                                          |               |         |                   |           | 0  |
| Reserved |          |               |     |                           |                    |      |       |                           |                                          | packet_length |         |                   |           |    |

#### *14.3.2.4.1.2.1 CPPI Rx Data Word – 0*

### **next\_descriptor\_pointer**

The 32-bit word aligned memory address of the next buffer descriptor in the RX queue. This is the mechanism used to reference the next buffer descriptor from the current buffer descriptor. If the value of this pointer is zero then the current buffer is the last buffer in the queue. The host sets the **next\_descriptor\_pointer**.

#### *14.3.2.4.1.2.2 CPPI Rx Data Word – 1*

### **buffer\_pointer**

The byte aligned memory address of the buffer associated with the buffer descriptor. The host sets the **buffer\_pointer**.

#### *14.3.2.4.1.2.3 CPPI Rx Data Word – 2*

#### **Buffer \_Offset**

Buffer Offset – Indicates how many unused bytes are at the start of the buffer. A value of 0x0000 indicates that there are no unused bytes at the start of the buffer and that valid data begins on the first byte of the buffer. A value of 0x000F (decimal 15) indicates that the first 15 bytes of the buffer are to be ignored by the port and that valid buffer data starts on byte 16 of the buffer. The port writes the **buffer\_offset** with the value from the **rx\_buffer\_offset** register value. The host initializes the **buffer\_offset** to zero for free buffers. The **buffer\_length** must be greater than the **rx\_buffer\_offset** value. The buffer offset is valid only on **sop**.

## **Buffer \_Length**

Buffer Length – Indicates how many valid data bytes are in the buffer. Unused or protocol specific bytes at the beginning of the buffer are not counted in the Buffer Length field. The host initializes the **buffer\_length**, but the port may overwrite the host initiated value with the actual buffer length value on SOP and/or EOP buffer descriptors. SOP buffer length values will be overwritten if the packet size is less than the size of the buffer or if the offset is nonzero. EOP buffer length values will be overwritten if the entire buffer is not filled up with data. The **buffer\_length** must be greater than zero.

#### *14.3.2.4.1.2.4 CPPI Rx Data Word – 3*

### **Start of Packet (SOP) Flag**

When set, this flag indicates that the descriptor points to a packet buffer that is the start of a new packet.In the case of a single fragment packet, both the SOP and end of packet (EOP) flags are set. Otherwise, the descriptor pointing to the last packet buffer for the packet has the EOP flag set. This flag is initially cleared by the software application before adding the descriptor to the receive queue. This bit is set by the EMAC on SOP descriptors.

## **End of Packet (EOP) Flag**

When set, this flag indicates that the descriptor points to a packet buffer that is last for a given packet. In the case of a single fragment packet, both the start of packet (SOP) and EOP flags are set. Otherwise, the descriptor pointing to the last packet buffer for the packet has the EOP flag set. This flag is initially cleared by the software application before adding the descriptor to the receive queue. This bit is set by the EMAC on EOP descriptors.

## **Ownership (OWNER) Flag**

When set, this flag indicates that the descriptor is currently owned by the EMAC. This flag is set by the software application before adding the descriptor to the receive descriptor queue. This flag is cleared by the EMAC once it is finished with a given set of descriptors, associated with a received packet. The flag is updated by the EMAC on SOP descriptor only. So when the application identifies that the OWNER flag is cleared on an SOP descriptor, it may assume that all descriptors up to and including the first with the EOP flag set have been released by the EMAC. (Note that in the case of single buffer packets, the same descriptor will have both the SOP and EOP flags set.)

## **End of Queue (EOQ) Flag**

When set, this flag indicates that the descriptor in question was the last descriptor in the receive queue for a given receive channel, and that the corresponding receiver channel has halted. This flag is initially cleared by the software application prior to adding the descriptor to the receive queue. This bit is set by the EMAC when the EMAC identifies that a descriptor is the last for a given packet received (also sets the EOP flag), and there are no more descriptors in the receive list (next descriptor pointer is NULL).The software application can use this bit to detect when the EMAC receiver for the corresponding channel has halted. This is useful when the application appends additional free buffer descriptors to an active receive queue. Note that this flag is valid on EOP descriptors only.

### **Teardown Complete (TDOWNCMPLT) Flag**

This flag is used when a receive queue is being torn down, or aborted, instead of being filled with received data. This would happen under device driver reset or shutdown conditions. The EMAC sets this bit in the descriptor of the first free buffer when the tear down occurs. No additional queue processing is performed.

## **Pass CRC (PASSCRC) Flag**

This flag is set by the EMAC in the SOP buffer descriptor if the received packet includes the 4-byte CRC.This flag should be cleared by the software application before submitting the descriptor to the receive queue.

#### **Long (Jabber) Flag**

This flag is set by the EMAC in the SOP buffer descriptor, if the received packet is a jabber frame and was not discarded because the RX\_CEF\_EN bit was set in the MacControl. Jabber frames are frames that exceed the RXMAXLEN in length, and have CRC, code, or alignment errors.

### **Short (Fragment) Flag**

This flag is set by the EMAC in the SOP buffer descriptor, if the received packet is only a packet fragment and was not discarded because the RX\_CSF\_EN bit was set in the MacControl.

#### **Control Flag**

This flag is set by the EMAC in the SOP buffer descriptor, if the received packet is an EMAC control frame and was not discarded because the RX\_CMF\_EN bit was set in the MacControl.

#### **Overrun Flag**

This flag is set by the EMAC in the SOP buffer descriptor, if the received packet was aborted due to a receive overrun.

### **Pkt\_error Flag**

Packet Contained Error on Ingress –

00 – no error

01 – CRC error on ingress

10 – Code error on ingress

11 – Align error on ingress

#### **rx\_vlan\_encap**

VLAN Encapsulated Packet – Indicates when set that the packet data contains a 32-bit VLAN header word that is included in the packet byte count. This field is set by the port to be the value of the CPSW control register rx\_vlan\_encap bit

### **from\_port**

From Port – Indicates the port number that the packet was received on (ingress to the switch).

## **Packet Length**

Specifies the number of bytes in the entire packet. The packet length is reduced to 12-bits. Offset bytes are not included. The sum of the buffer\_length fields should equal the packet\_length. Valid only on SOP.

#### *14.3.2.4.2 Receive DMA Interface*

The Receive DMA is an eight channel CPPI 3.0 compliant interface. Each channel has a single queue for frame reception.

#### *14.3.2.4.2.1 Receive DMA Host Configuration*

To configure the Rx DMA for operation the host must perform the following:

- Initialize the receive addresses.
- Initialize the Rx\_HDP Registers to zero.
- Enable the desired receive interrupts in the IntMask register.
- Write the rx\_buffer\_offset register value.
- Setup the receive channel(s) buffer descriptors in host memory as required by CPPI 3.0.
- Enable the RX DMA controller by setting the rx\_en bit in the Rx\_Control register.

#### *14.3.2.4.2.2 Receive Channel Teardown*

The host commands a receive channel teardown by writing the channel number to the Rx\_Teardown register. When a teardown command is issued to an enabled receive channel the following will occur:

- Any current frame in reception will complete normally.
- The teardown\_complete bit will be set in the next buffer descriptor in the chain

if there is one.

- The channel head descriptor pointer will be cleared to zero
- A receive interrupt for the channel will be issued to the host.
- The host should acknowledge a teardown interrupt with a 0xfffffffc acknowledge value.

Channel teardown may be commanded on any channel at any time. The host is informed of the teardown completion by the set teardown complete buffer descriptor bit. The port does not clear any channel enables due to a teardown command. A teardown command to an inactive channel issues an interrupt that software should acknowledge with a 0xfffffffc acknowledge value (note that there is no buffer descriptor in this case). Software may read the interrupt acknowledge location to determine if the interrupt was due to a commanded teardown. The read value will be 0xfffffffc if the interrupt was due to a teardown command.

#### *14.3.2.4.3 Transmit DMA Interface*

The Transmit DMA is an eight channel CPPI 3.0 compliant interface. Priority between the eight queues may be either fixed or round robin as selected by tx\_ptype in the DMAControl register. If the priority type is fixed, then channel 7 has the highest priority and channel 0 has the lowest priority. Round robin priority proceeds from channel 0 to channel 7. Packet Data transfers occur on the TX\_VBUSP interface in 64 byte maximum burst transfers

#### *14.3.2.4.3.1 Transmit DMA Host Configuration*

To configure the TX DMA for operation the host must do the following:

- Initialize the Tx\_HDP registers to a zero value.
- Enable the desired transmit interrupts in the IntMask register.
- Setup the transmit channel(s) buffer descriptors in host memory as defined in CPPI 3.0.
- Configure and enable the transmit operation as desired in the TxControl register.
- Write the appropriate Tx\_HDP registers with the appropriate values to start transmit operations.

#### *14.3.2.4.3.2 Transmit Channel Teardown*

The host commands a transmit channel teardown by writing the channel number to the Tx\_Teardown register. When a teardown command is issued to an enabled transmit channel the following will occur:

- Any frame currently in transmission will complete normally
- The teardown complete bit will be set in the next sop buffer descriptor (if there is one).
- The channel head descriptor pointer will be set to zero.
- An interrupt will be issued to inform the host of the channel teardown.
- The host should acknowledge a teardown interrupt with a 0xfffffffc acknowledge value

Channel teardown may be commanded on any channel at any time. The host is informed of the teardown completion by the set teardown complete buffer descriptor bit. The port does not clear any channel enables due to a teardown command. A teardown command to an inactive channel issues an interrupt that software should acknowledge with a 0xfffffffc acknowledge value (note that there is no buffer descriptor in this case). Software may read the interrupt acknowledge location to determine if the interrupt was due to a commanded teardown. The read value will be 0xfffffffc if the interrupt was due to a teardown command.

#### *14.3.2.4.4 Transmit Rate Limiting*

Transmit operations can be configured to rate limit the transmit data for each transmit priority. Rate limiting is enabled for a channel when the tx\_rlim[7:0] bit associated with that channel is set in the DMAControl register. Rate limited channels must be the highest priority channels. For example, if two rate limited channels are required then tx\_rlim[7:0] should be set to 11000000 with the msb corresponding to channel 7.

When any channels are configured to be rate-limiting, the priority type must be fixed for transmit. Roundrobin priority type is not allowed when rate-limiting. Each of the eight transmit priorities has an associated register to control the rate at which the priority is allowed to send data (Tx\_Pri(7..0)\_Rate) when the channel is rate-limiting. Each priority has a send count (pri(7..0)\_send\_cnt[13:0]) and an idle count (pri(7..0)\_idle\_cnt[13:0]). The transfer rate includes the inter-packet gap (12 bytes) and the preamble (8 bytes). The rate in Mbits/second that each priority is allowed to send is controlled by the below equation.

Priority Transfer rate in Mbit/s = ((priN\_idle\_cnt/(priN\_idle\_cnt + priN\_send\_cnt)) \* frequency \* 32

Where the frequency is the CLK frequency.

#### *14.3.2.4.5 Command IDLE*

The cmd\_idle bit in the DMA\_Control register allows CPDMA operation to be suspended. When the idle state is commanded, the CPDMA will stop processing receive and transmit frames at the next frame boundary. Any frame currently in reception or transmission will be completed normally without suspension. For transmission, any complete or partial frame in the tx cell fifo will be transmitted. For receive, frames that are detected by the CPDMA after the suspend state is entered are ignored. No statistics will be kept for ignored frames. Commanded idle is similar in operation to emulation control and clock stop.

#### **14.3.2.5 VLAN Aware Mode**

The CPSW\_3G is in VLAN aware mode when the CPSW Control register vlan\_aware bit is set. In VLAN aware mode ports 0 receive packets (out of the CPSW\_3G) may or may not be VLAN encapsulated depending on the CPSW Control register rx\_vlan\_encap bit. The header packet VLAN is generated as described in [Section](#page-61-0) 14.3.3, *Ethernet Mac Sliver (CPGMAC\_SL)*. Port 0 receive packet data is never modified. VLAN is not removed regardless of the force untagged egress bit for Port 0. VLAN encapsulated receive packets have a 32-bit VLAN header encapsulation word added to the packet data.VLAN encapsulated packets are specified by a set rx\_vlan\_encap bit in the packet buffer descriptor.

Port 0 transmit packets are never VLAN encapsulated (encapsulation is not allowed).

In VLAN aware mode, transmitted packet data is changed depending on the packet type (pkt\_type), packet priority (pkt\_pri), and VLAN information as shown in the below tables:

## 31 29 28 27 16 HDR\_PKT\_Priority HDR\_PKT\_CFI HDR\_PKT\_Vid 15 10 9 8 7 6 5 4 3 2 1 0

Reserved PKT\_Type Reserved

**Figure 14-9. VLAN Header Encapsulation Word**

### **Table 14-8. VLAN Header Encapsulation Word Field Descriptions**

| Field            | Description                                                                               |  |  |  |  |  |
|------------------|-------------------------------------------------------------------------------------------|--|--|--|--|--|
| HDR_PKT_Priority | Header Packet VLAN priority (Highest priority: 7)                                         |  |  |  |  |  |
| HDR_PKT_CFI      | Header Packet VLAN CFI bit.                                                               |  |  |  |  |  |
| HDR_PKT_Vid      | Header Packet VLAN ID                                                                     |  |  |  |  |  |
|                  | Packet Type. Indicates whether the packet is VLAN-tagged, priority-tagged, or non-tagged. |  |  |  |  |  |
|                  | 00: VLAN-tagged packet                                                                    |  |  |  |  |  |
| PKT_Type         | 01: Reserved                                                                              |  |  |  |  |  |
|                  | 10: Priority-tagged packet                                                                |  |  |  |  |  |
|                  | 11: Non-tagged packet                                                                     |  |  |  |  |  |

#### **14.3.2.6 VLAN Unaware Mode**

The CPSW\_3G is in VLAN unaware mode when the CPSW Control register vlan\_aware bit is cleared. Port 0 receive packets (out of the CPSW\_3G) may or may not be VLAN encapsulated depending on the CPSW Control register rx\_vlan\_encap bit. Port 0 transmit packets are never VLAN encapsulated.

#### **14.3.2.7 Address Lookup Engine (ALE)**

The address lookup engine (ALE) processes all received packets to determine which port(s) if any that the packet should the forwarded to. The ALE uses the incoming packet received port number, destination address, source address, length/type, and VLAN information to determine how the packet should be forwarded. The ALE outputs the port mask to the switch fabric that indicates the port(s) the packet should be forwarded to. The ALE is enabled when the ale\_enable bit in the ALE\_Control register is set. All packets are dropped when the ale\_enable bit is cleared to zero.

In normal operation, the CPGMAC\_SL modules are configured to issue an abort, instead of an end of packet, at the end of a packet that contains an error (runt, frag, oversize, jabber, crc, alignment, code etc.) or at the end of a mac control packet. However, when the CPGMAC\_SL configuration bit(s) cef, csf, or cmf are set, error frames, short frames or mac control frames have a normal end of packet instead of an abort at the end of the packet. When the ALE receives a packet that contains errors (due to a set header error bit), or a mac control frame and does not receive an abort, the packet will be forwarded only to the host port (port 0). No ALE learning occurs on packets with errors or mac control frames. Learning is based on source address and lookup is based on destination address.

The ALE may be configured to operate in bypass mode by setting the ale\_bypass bit in the ALE\_Control register. When in bypass mode, all CPGMAC\_SL received packets are forwarded only to the host port (port 0). Packets from the two ports can be on separate Rx DMA channels by configuring the CPDMA\_Rx\_Ch\_Map register. In bypass mode, the ALE processes host port transmit packets the same as in normal mode. In general, packets would be directed by the host in bypass mode.

The ALE may be configured to operate in OUI deny mode by setting the enable\_oui\_deny bit in the ALE\_Control register. When in OUI deny mode, a packet with a non-matching OUI source address will be dropped unless the destination address matches a multicast table entry with the super bit set. Broadcast packets will be dropped unless the broadcast address is entered into the table with the super bit set. Unicast packets will be dropped unless the unicast address is in the table with block and secure both set (supervisory unicast packet).

Multicast supervisory packets are designated by the super bit in the table entry. Unicast supervisory packets are indicated when block and secure are both set. Supervisory packets are not dropped due to rate limiting, OUI, or VLAN processing.

#### *14.3.2.7.1 Address Table Entry*

The ALE table contains 1024 entries. Each table entry represents a free entry, an address, a VLAN, an address/VLAN pair, or an OUI address. Software should ensure that there are not double address entries in the table. The double entry used would be indeterminate. Reserved table bits must be written with zeroes.

Source Address learning occurs for packets with a unicast, multicast or broadcast destination address and a unicast or multicast (including broadcast) source address. Multicast source addresses have the group bit (bit 40) cleared before ALE processing begins, changing the multicast source address to a unicast source address. A multicast address of all ones is the broadcast address which may be added to the table. A learned unicast source address is added to the table with the following control bits:

**Table 14-9. Learned Address Control Bits**

| unicast_type | 11 |
|--------------|----|
| Block        | 0  |
| Secure       | 0  |

If a received packet has a source address that is equal to the destination address then the following occurs:

- The address is learned if the address is not found in the table.
- The address is updated if the address is found.
- The packet is dropped.

#### *14.3.2.7.1.1 Free Table Entry*

**Table 14-10. Free (Unused) Address Table Entry Bit Values**

| 71:62    | 61:60           | 59:0     |
|----------|-----------------|----------|
| Reserved | Entry Type (00) | Reserved |

#### *14.3.2.7.1.2 Multicast Address Table Entry*

**Table 14-11. Multicast Address Table Entry Bit Values**

| 71:70    | 68:66     | 65    | 64       | 63:62              | 61:60              | 59:48    | 47:0                 |
|----------|-----------|-------|----------|--------------------|--------------------|----------|----------------------|
| Reserved | Port Mask | Super | Reserved | Mcast Fwd<br>State | Entry Type<br>(01) | Reserved | Multicast<br>Address |

## **Table Entry Type**

00: Free Entry

01: Address Entry : unicast or multicast determined by dest **address bit 40 .**

10: VLAN entry

11: VLAN Address Entry : unicast or multicast determined by **address bit 40.**

## **Supervisory Packet (SUPER)**

When set, this field indicates that the packet with a matching multicast destination address is a supervisory packet.

0: Non-supervisory packet

1: Supervisory packet

### **Port Mask(2:0) (PORT\_MASK)**

This three bit field is the port bit mask that is returned with a found multicast destination address. There may be multiple bits set indicating that the multicast packet may be forwarded to multiple ports (but not the receiving port).

#### **Multicast Forward State (MCAST\_FWD\_STATE)**

Multicast Forward State – Indicates the port state(s) required for the received port on a destination address lookup in order for the multicast packet to be forwarded to the transmit port(s). A transmit port must be in the Forwarding state in order to forward the packet. If the transmit port\_mask has multiple set bits then each forward decision is independent of the other transmit port(s) forward decision.

00: Forwarding

01: Blocking/Forwarding/Learning

10: Forwarding/Learning

11: Forwarding

The forward state test returns a true value if both the Rx and Tx ports are in the required state.

## **Table Entry Type (ENTRY\_TYPE)**

Address entry type. Unicast or multicast determined by address bit 40.

01: Address entry. Unicast or multicast determined by address bit 40.

#### **Packet Address (MULTICAST\_ADDRESS)**

This is the 48-bit packet MAC address. For an OUI address, only the upper 24-bits of the address are used in the source or destination address lookup. Otherwise, all 48-bits are used in the lookup.

#### *14.3.2.7.1.3 VLAN/Multicast Address Table Entry*

**Table 14-12. VLAN/Multicast Address Table Entry Bit Values**

| 71:69    | 68:66     | 65    | 64       | 63:62              | 61:60              | 59:48   | 47:0                 |
|----------|-----------|-------|----------|--------------------|--------------------|---------|----------------------|
| Reserved | Port Mask | Super | Reserved | Mcast Fwd<br>State | Entry Type<br>(11) | vlan_id | Multicast<br>Address |

## **Supervisory Packet (SUPER)**

When set, this field indicates that the packet with a matching multicast destination address is a supervisory packet.

- 0: Non-supervisory packet
- 1: Supervisory packet

### **Port Mask(2:0) (PORT\_MASK)**

This three bit field is the port bit mask that is returned with a found multicast destination address. There may be multiple bits set indicating that the multicast packet may be forwarded to multiple ports (but not the receiving port).

## **Multicast Forward State (MCAST\_FWD\_STATE)**

Multicast Forward State – Indicates the port state(s) required for the received port on a destination address lookup in order for the multicast packet to be forwarded to the transmit port(s). A transmit port must be in the Forwarding state in order to forward the packet. If the transmit port\_mask has multiple set bits then each forward decision is independent of the other transmit port(s) forward decision.

- 00 Forwarding
- 01 Blocking/Forwarding/Learning
- 10 Forwarding/Learning
- 11 Forwarding

The forward state test returns a true value if both the Rx and Tx ports are in the required state.

## **Table Entry Type (ENTRY\_TYPE)**

Address entry type. Unicast or multicast determined by address bit 40.

11: VLAN address entry. Unicast or multicast determined by address bit 40.

### **VLAN ID (VLAN\_ID)**

The unique identifier for VLAN identification. This is the 12-bit VLAN ID.

#### **Packet Address (MULTICAST\_ADDRESS)**

This is the 48-bit packet MAC address. For an OUI address, only the upper 24-bits of the address are used in the source or destination address lookup. Otherwise, all 48-bits are used in the lookup.

#### *14.3.2.7.1.4 Unicast Address Table Entry*

#### **Table 14-13. Unicast Address Table Entry Bit Values**

| 71:70    | 69             | 68       | 67:66          | 65    | 64     | 63:62                           | 61:60              | 59:48    | 47:0               |
|----------|----------------|----------|----------------|-------|--------|---------------------------------|--------------------|----------|--------------------|
| Reserved | DLR<br>Unicast | Reserved | Port<br>Number | Block | Secure | Unicast<br>Type (00)<br>or (X1) | Entry Type<br>(01) | Reserved | Unicast<br>Address |

## **DLR Unicast**

DLR Unicast – When set, this bit indicates that the address is a Device Level Ring (DLR) unicast address. Received packets with a matching destination address will be flooded to the vlan\_member\_list (minus the receive port and the host port). The port\_number field is a don't care when this bit is set. Matching packets received on port 1 egress on port 2. Matching packets received on port 2 egress on port 1. Matching packets received on port 0 egress on ports 1 and 2.

#### **Port Number (PORT\_NUMBER)**

Port Number – This field indicates the port number (not port mask) that the packet with a unicast destination address may be forwarded to. Packets with unicast destination addresses are forwarded only to a single port (but not the receiving port).

## **Block (BLOCK)**

Block – The block bit indicates that a packet with a matching source or destination address should be dropped (block the address).

- 0 Address is not blocked.
- 1 Drop a packet with a matching source or destination address (secure

must be zero)

If block and secure are both set, then they no longer mean block and secure. When both are set, the block and secure bits indicate that the packet is a unicast supervisory (super) packet and they determine the unicast forward state test criteria. If both bits are set then the packet is forwarded if the receive port is in the Forwarding/Blocking/Learning state. If both bits are not set then the packet is forwarded if the receive port is in the Forwarding state.

### **Secure (SECURE)**

Secure – This bit indicates that a packet with a matching source address should be dropped if the received port number is not equal to the table entry port\_number.

- 0 Received port number is a don't care.
- 1 Drop the packet if the received port is not the secure port for the source address and do not update the address (block must be zero)

## **Unicast Type (UNICAST\_TYPE)**

Unicast Type – This field indicates the type of unicast address the table entry contains.

- 00 Unicast address that is not ageable.
- 01 Ageable unicast address that has not been touched.
- 10 OUI address lower 24-bits are don't cares (not ageable).
- 11 Ageable unicast address that has been touched.

### **Table Entry Type (ENTRY\_TYPE)**

Address entry. Unicast or multicast determined by address bit 40.

01: Address entry. Unicast or multicast determined by address bit 40.

#### **Packet Address (UNICAST\_ADDRESS)**

This is the 48-bit packet MAC address. All 48-bits are used in the lookup.

#### *14.3.2.7.1.5 OUI Unicast Address Table Entry*

## **Table 14-14. OUI Unicast Address Table Entry Bit Values**

| 71:64    | 63:62             | 61:60           | 59:48    | 47:24       | 23:0     |
|----------|-------------------|-----------------|----------|-------------|----------|
| Reserved | Unicast Type (10) | Entry Type (01) | Reserved | Unicast OUI | Reserved |

## **Unicast Type (UNICAST\_TYPE)**

Unicast Type – This field indicates the type of unicast address the table entry contains.

- 00 Unicast address that is not ageable.
- 01 Ageable unicast address that has not been touched.
- 10 OUI address lower 24-bits are don't cares (not ageable).
- 11 Ageable unicast address that has been touched.

### **Table Entry Type (ENTRY\_TYPE)**

Address entry. Unicast or multicast determined by address bit 40.

01: Address entry. Unicast or multicast determined by address bit 40.

### **Packet Address (UNICAST\_OUI)**

For an OUI address, only the upper 24-bits of the address are used in the source or destination address lookup.

#### *14.3.2.7.1.6 VLAN/Unicast Address Table Entry*

**Table 14-15. Unicast Address Table Entry Bit Values**

| 71:68    | 67:66       | 65    | 64     | 63:62                        | 61:60              | 59:48   | 47:0               |
|----------|-------------|-------|--------|------------------------------|--------------------|---------|--------------------|
| Reserved | Port Number | Block | Secure | Unicast Type<br>(00) or (X1) | Entry Type<br>(11) | vlan_id | Unicast<br>Address |

## **Port Number (PORT\_NUMBER)**

Port Number – This field indicates the port number (not port mask) that the packet with a unicast destination address may be forwarded to. Packets with unicast destination addresses are forwarded only to a single port (but not the receiving port).]

## **Block (BLOCK)**

Block – The block bit indicates that a packet with a matching source or destination address should be dropped (block the address).

- 0 Address is not blocked.
- 1 Drop a packet with a matching source or destination address (secure

must be zero)

If block and secure are both set, then they no longer mean block and secure. When both are set, the block and secure bits indicate that the packet is a unicast supervisory (super) packet and they determine the unicast forward state test criteria. If both bits are set then the packet is forwarded if the receive port is in the Forwarding/Blocking/Learning state. If both bits are not set then the packet is forwarded if the receive port is in the Forwarding state.

## **Secure (SECURE)**

Secure – This bit indicates that a packet with a matching source address should be dropped if the received port number is not equal to the table entry port\_number.

- 0 Received port number is a don't care.
- 1 Drop the packet if the received port is not the secure port for the source address and do not update the address (block must be zero)

### **Unicast Type (UNICAST\_TYPE)**

Unicast Type – This field indicates the type of unicast address the table entry contains.

- 00 Unicast address that is not ageable.
- 01 Ageable unicast address that has not been touched.
- 10 OUI address lower 24-bits are don't cares (not ageable).
- 11 Ageable unicast address that has been touched.

### **Table Entry Type (ENTRY\_TYPE)**

Address entry. Unicast or multicast determined by address bit 40.

11 – VLAN address entry. Unicast or multicast determined by address bit 40.

#### **VLAN ID (VLAN\_ID)**

The unique identifier for VLAN identification. This is the 12-bit VLAN ID.

### **Packet Address (UNICAST\_ADDRESS)**

This is the 48-bit packet MAC address. All 48-bits are used in the lookup.

#### *14.3.2.7.1.7 VLAN Table Entry*

#### **Table 14-16. VLAN Table Entry**

| 71:62    | 61:60              | 59:48   | 47:27    | 26:24                       | 23:19    | 18:16                         | 15:11    | 10:8                            | 7:3      | 2:0                    |
|----------|--------------------|---------|----------|-----------------------------|----------|-------------------------------|----------|---------------------------------|----------|------------------------|
| Reserved | Entry<br>Type (10) | vlan_id | Reserved | Force<br>Untagged<br>Egress | Reserved | Reg<br>Mcast<br>Flood<br>Mask | Reserved | Unreg<br>Mcast<br>Flood<br>Mask | Reserved | Vlan<br>Member<br>List |

## **Table Entry Type (ENTRY\_TYPE)**

10: VLAN entry

## **VLAN ID (VLAN\_ID)**

The unique identifier for VLAN identification. This is the 12-bit VLAN ID.

## **Force Untagged Packet Egress (FORCE\_UNTAGGED\_EGRESS)**

This field causes the packet VLAN tag to be removed on egress (except on port 0).

## **Registered Multicast Flood Mask (REG\_MCAST\_FLOOD\_MASK)**

Mask used for multicast when the multicast address is found

## **Unregistered Multicast Flood Mask (UNREG\_MCAST\_FLOOD\_MASK)**

Mask used for multicast when the multicast address is not found

## **VLAN Member List (VLAN\_MEMBER\_LIST)**

This three bit field indicates which port(s) are members of the associated VLAN.

#### *14.3.2.7.2 Packet Forwarding Processes*

There are four processes that an incoming received packet may go through to determine packet forwarding. The processes are Ingress Filtering, VLAN\_Aware Lookup, VLAN\_Unaware Lookup, and Egress.

Packet processing begins in the Ingress Filtering process. Each port has an associated packet forwarding state that can be one of four values (Disabled, Blocked, Learning, or Forwarding). The default state for all ports is disabled. The host sets the packet forwarding state for each port. The receive packet processes are described in the following sections.

In the packet ingress process (receive packet process), there is a forward state test for unicast destination addresses and a forward state test for multicast addresses. The multicast forward state test indicates the port states required for the receiving port in order for the multicast packet to be forwarded to the transmit port(s). A transmit port must be in the Forwarding state for the packet to be forwarded for transmission. The mcast\_fwd\_state indicates the required port state for the receiving port as indicated in Table [14-12.](#page-33-0)

The unicast forward state test indicates the port state required for the receiving port in order to forward the unicast packet. The transmit port must be in the Forwarding state in order to forward the packet. The block and secure bits determine the unicast forward state test criteria. If both bits are set then the packet is forwarded if the receive port is in the Forwarding/Blocking/Learning state. If both bits are not set then the packet is forwarded if the receive port is in the Forwarding state. The transmit port must be in the Forwarding state regardless. The forward state test used in the ingress process is determined by the destination address packet type (multicast/unicast).

In general, packets received with errors are dropped by the address lookup engine without learning, updating, or touching the address. The error condition and the abort are indicated by the CPGMAC\_SL to the ALE. Packets with errors may be passed to the host (not aborted) by a CPGMAC\_SL port if the port has a set rx\_cmf\_en, rx\_cef\_en, or rx\_csf\_en bit(s).

Error packets that are passed to the host by the CPGMAC\_SL are considered to be bypass packets by the ALE and are sent only to the host. Error packets do not learn, update, or touch addresses regardless of whether they are aborted or sent to the host. Packets with errors received by the host are forwarded as normal.

The following control bits are in the CPGMAC\_SL1/2\_MacControl register.

rx\_cef\_en: This CPGMAC\_SL control bit enables frames that are fragments, long, jabber, CRC, code, and alignment errors to be forwarded.

rx\_csf\_en: This CPGMAC\_SL bit enables short frames to be forwarded.

rx\_cmf\_en: This CPGMAC\_SL control bit enables mac control frames to be forwarded.

#### *14.3.2.7.2.1 Ingress Filtering Process*

If (Rx port\_state is Disabled)

then discard the packet

if (directed packet)

then use directed port number and go to Egress process

if ((ale\_bypass or error packet) and (host port is not the receive port))

then use host portmask and go to Egress process

if (((block) and (unicast source address found)) or ((block) and (unicast destination address found))) then discard the packet

if ((enable\_rate\_limit) and (rate limit exceeded) and (not rate\_limit\_tx)

then if (((mcast/bcast destination address found) and (not super)) or (mcast/bcast destination address not found)) then discard the packet

if ((not forward state test valid) and (destination address found))

then discard the packet to any port not meeting the requirements

• Unicast destination addresses use the unicast forward state test and multicast destination addresses use the multicast forward state test.

if ((destination address not found) and ((not transmit port forwarding) or (not receive port forwarding))) then discard the packet to any ports not meeting the above requirements

if (source address found) and (secure) and (receive port number != port\_number)) then discard the packet

if ((not super) and (drop\_untagged) and ((non-tagged packet) or ((priority tagged) and not(en\_vid0\_mode))

If (VLAN\_Unaware)

then discard the packet

force\_untagged\_egress = "000000"

reg\_mcast\_flood\_mask = "111111"

unreg\_mcast\_flood\_mask = "111111"

vlan\_member\_list = "111111"

else if (VLAN not found)

force\_untagged\_egress = unknown\_force\_untagged\_egress

reg\_mcast\_flood\_mask = unknown\_reg\_mcast\_flood\_mask

unreg\_mcast\_flood\_mask = unknown unreg\_mcast\_flood\_mask

vlan\_member\_list = unknown\_vlan\_member\_list

else

force\_untagged\_egress = found force\_untagged\_egress

reg\_mcast\_flood\_mask = found reg\_mcast\_flood\_mask

unreg\_mcast\_flood\_mask = found unreg\_mcast\_flood\_mask

vlan\_member\_list = found vlan\_member\_list

if ((not super) and (vid\_ingress\_check) and (Rx port is not VLAN member))

then discard the packet

if ((enable\_auth\_mode) and (source address not found) and not(destination address found and (super)))

then discard the packet

if (destination address equals source address) then discard the packet

if (vlan\_aware) goto VLAN\_Aware\_Lookup process else goto VLAN\_Unaware\_Lookup process

#### *14.3.2.7.2.2 VLAN\_Aware Lookup Process*

if ((unicast packet) and (destination address found with or without VLAN) and dlr\_unicast) then portmask is the vlan\_member\_list less the host port and goto Egress process

if ((unicast packet) and (destination address found with or without VLAN) and (not super)) then portmask is the logical "AND" of the port\_number and the vlan\_member\_list and goto Egress process

if ((unicast packet) and (destination address found with or without VLAN) and (super)) then portmask is the port\_number and goto Egress process

if (Unicast packet) # destination address not found then portmask is vlan\_member\_list less host port and goto Egress process

if ((Multicast packet) and (destination address found with or without VLAN) and (not super)) then portmask is the logical "AND" of reg\_mcast\_flood\_mask and found destination address/VLAN portmask (port\_mask) and vlan\_member\_list and goto Egress process

if ((Multicast packet) and (destination address found with or without VLAN) and (super)) then portmask is the port\_mask and goto Egress process

if (Multicast packet) # destination address not found then portmask is the logical "AND" of unreg\_mcast\_flood\_mask and vlan\_member\_list then goto Egress process

if (Broadcast packet) then use found vlan\_member\_list and goto Egress process

#### *14.3.2.7.2.3 VLAN\_Unaware Lookup Process*

if ((unicast packet) and (destination address found with or without VLAN) and dlr\_unicast) then portmask is the vlan\_member\_list less the host port and goto Egress process

if ((unicast packet) and (destination address found with or without VLAN) and (not super)) then portmask is the logical "AND" of the port\_number and the vlan\_member\_list and goto Egress process

if ((unicast packet) and (destination address found with or without VLAN) and (super)) then portmask is the port\_number and goto Egress process

if (Unicast packet) # destination address not found then portmask is vlan\_member\_list less host port and goto Egress process

if ((Multicast packet) and (destination address found with or without VLAN) and (not super)) then portmask is the logical "AND" of reg\_mcast\_flood\_mask and found destination address/VLAN portmask (port\_mask) and vlan\_member\_list and goto Egress process

if ((Multicast packet) and (destination address found with or without VLAN) and (super)) then portmask is the port\_mask and goto Egress process

if (Multicast packet) # destination address not found then portmask is the logical "AND" of unreg\_mcast\_flood\_mask and vlan\_member\_list then goto Egress process

if (Broadcast packet) then use found vlan\_member\_list and goto Egress process

#### *14.3.2.7.2.4 Egress Process*

Clear Rx port from portmask (don't send packet to Rx port).

Clear disabled ports from portmask.

if ((enable\_oui\_deny) and (OUI source address not found) and (not ale\_bypass) and (not error packet) and not ((mcast destination address) and (super)))

then Clear host port from portmask

if ((enable\_rate\_limit) and (rate\_limit\_tx))

then if (not super) and (rate limit exceeded on any tx port)

then clear rate limited tx port from portmask

If address not found then super cannot be set.

If portmask is zero then discard packet

Send packet to portmask ports.

#### *14.3.2.7.3 Learning/Updating/Touching Processes*

The learning, updating, and touching processes are applied to each receive packet that is not aborted. The processes are concurrent with the packet forwarding process. In addition to the following, a packet must be received without error in order to learn/update/touch an address.

#### *14.3.2.7.3.1 Learning Process*

If (not(Learning or Forwarding) or (enable\_auth\_mode) or (packet error) or (no\_learn))

then do not learn address

if ((Non-tagged packet) and (drop\_untagged))

then do not learn address

if ((vlan\_aware) and (VLAN not found) and (unknown\_vlan\_member\_list = "000"))

then do not learn address

if ((vid\_ingress\_check) and (Rx port is not VLAN member) and (VLAN found))

then do not learn address

if ((source address not found) and (vlan\_aware) and not(learn\_no\_vid))

then learn address with VLAN

if ((source address not found) and ((not vlan\_aware) or (vlan\_aware and learn\_no\_vid)))

then learn address without VLAN

#### *14.3.2.7.3.2 Updating Process*

if (dlr\_unicast)

then do not update address

If (not(Learning or Forwarding) or (enable\_auth\_mode) or (packet error) or (no\_sa\_update))

then do not update address

if ((Non-tagged packet) and (drop\_untagged))

then do not update address

if ((vlan\_aware) and (VLAN not found) and (unknown\_vlan\_member\_list = "000"))

then do not update address

if ((vid\_ingress\_check) and (Rx port is not VLAN member) and (VLAN found))

then do not update address

if ((source address found) and (receive port number != port\_number) and (secure or block))

then do not update address

if ((source address found) and (receive port number != port\_number)) then update address

#### *14.3.2.7.3.3 Touching Process*

if ((source address found) and (ageable) and (not touched)) then set touched

#### **14.3.2.8 Packet Priority Handling**

Packets are received on three ports, two of which are CPGMAC\_SL Ethernet ports and the third port is the CPPI host port. Received packets have a received packet priority (0 to 7 with 7 being the highest priority). The received packet priority is determined as shown:

- 1. If the first packet LTYPE = 0x8100 then the received packet priority is the packet priority (VLAN tagged and priority tagged packets).
- 2. If the first packet LTYPE = 0x0800 and byte 14 (following the LTYPE) is equal to 0x45 then the received packet priority is the 6-bit TOS field in byte 15 (upper 6-bits) mapped through the port's DSCP priority mapping register (IPV4 packet).
- 3. The received packet priority is the source (ingress) port priority (untagged non-IPV4 packet).

The received packet priority is mapped through the receive ports associated "packet priority to header packet priority mapping register" to obtain the header packet priority (the CPDMA Rx and Tx nomenclature is reversed from the CPGMAC\_SL nomenclature for legacy reasons). The header packet priority is mapped through the "header priority to switch priority mapping register" to obtain the hardware switch priority (0 to 3 with 3 being the highest priority). The header packet priority is then used as the actual transmit packet priority if the VLAN information is to be sent on egress.

#### **14.3.2.9 FIFO Memory Control**

Each of the three CPSW\_3G ports has an identical associated FIFO. Each FIFO contains a single logical receive (ingress) queue and four logical transmit queues (priority 0 through 3). Each FIFO memory contains 20,480 bytes (20k) total organized as 2560 by 64-bit words contained in a single memory instance. The FIFO memory is used for the associated port transmit and receive queues. The tx\_max\_blks field in the FIFO's associated Max\_Blks register determines the maximum number of 1k FIFO memory blocks to be allocated to the four logical transmit queues (transmit total).

The rx\_max\_blks field in the FIFO's associated Max\_Blks register determines the maximum number of 1k memory blocks to be allocated to the logical receive queue. The tx\_max\_blks value plus the rx\_max\_blks value must sum to 20 (the total number of blocks in the FIFO). If the sum were less than 20 then some memory blocks would be unused.The default is 17 (decimal) transmit blocks and three receive blocks. The FIFO's follow the naming convention of the Ethernet ports.Host Port is Port0 and External Ports are Port1,2

#### **14.3.2.10 FIFO Transmit Queue Control**

There are four transmit queues in each transmit FIFO. Software has some flexibility in determining how packets are loaded into the queues and on how packet priorities are selected for transmission (how packets are removed and transmitted from queues). All ports on the switch have identical FIFO's. For the purposes of the below the transmit FIFO is switch egress even though the port 0 transmit FIFO is connected to the CPDMA receive (also switch egress). The CPDMA nomenclature is reversed from the CPGMAC\_SL nomenclature due to legacy reasons.

#### *14.3.2.10.1 Normal Priority Mode*

When operating in normal mode, lower priority frames are dropped before higher priority frames. The intention is to give preference to higher priority frames. Priority 3 is the highest priority and is allowed to fill the FIFO. Priority 2 will drop packets if the packet is going to take space in the last 2k available. Priority 1 will drop packets if the packet is going to take space in the last 4k available. Priority 0 will drop packets if the packet is going to take space in the last 6k available. If fewer than 4 priorities are to be implemented then the priorities should be mapped such that the highest priorities are used.

For example, if two priorities are going to be used then all packets should be mapped to priorities 3 and 2 and priorities 1 and 0 should be unused. Priority escalation may be used in normal priority mode if desired. Normal priority mode is configured as described below:

- Select normal priority mode by setting tx\_in\_sel[1:0] = 00 for all ports (default value in P0/1/2\_Tx\_In\_Ctl)
- Configure priority mapping to use only the highest priorities if less than 4 priorities are used. Refer to the Packet Priority Handling section of this chapter.

#### *14.3.2.10.2 Dual Mac Mode*

When operating in dual mac mode the intention is to transfer packets between ports 0 and 1 and ports 0 and 2, but not between ports 1 and 2. Each CPGMAC\_SL appears as a single MAC with no bridging between MAC's. Each CPGMAC\_SL has at least one unique (not the same) mac address.

Dual mac mode is configured as described below:

• Set the ale\_vlan\_aware bit in the ALE\_Control register. This bit configures the ALE to process in vlan aware mode.The CPSW\_3G vlan aware bit (vlan\_aware in CPSW\_Control) determines how packets VLAN's are processed on CPGMAC\_SL egress and does not affect how the ALE processes packets or the packet destination. The CPSW\_3G vlan aware bit may be set or not as required (must be set if VLAN's are to exit the switch).

### • **Configure the Port 1 to Port 0 VLAN**

Add a VLAN Table Entry with ports 0 and 1 as members (clear the flood masks).

Add a VLAN/Unicast Address Table Entry with the Port1/0 VLAN and a port number of 0. Packets received on port 1 with this unicast address will be sent only to port 0 (egress). If multiple mac addresses are desired for this port then multiple entries of this type may be configured.

### • **Configure the Port 2 to Port 0 VLAN**

Add a VLAN Table Entry with ports 0 and 2 as members (clear the flood masks).

Add a VLAN/Unicast Address Table Entry with the Port2/0 VLAN and a port number of 0. Packets received on port 2 with this unicast address will be sent only to port 0 (egress). If multiple mac addresses are desired for this port then multiple entries of this type may be configured.

- Packets from the host (port 0) to ports 1 and 2 should be directed. If directed packets are not desired then VLAN with addresses can be added for both destination ports.
- Select the dual mac mode on the port 0 FIFO by setting tx\_in\_sel[1:0] = 01 in P0\_Tx\_In\_Ctl. The intention of this mode is to allow packets from both ethernet ports to be written into the FIFO without one port starving the other port.
- The priority levels may be configured such that packets received on port 1 egress on one CPDMA RX channel while packets received on port 2 egress on a different CPDMA RX channel.

#### *14.3.2.10.3 Rate Limit Mode*

Rate-limit mode is intended to allow some CPDMA transmit (switch ingress) channels and some CPGMAC\_SL FIFO priorities (switch egress) to be rate-limited. Non rate-limited traffic (bulk traffic) is allowed on non rate-limited channels and FIFO priorities. The bulk traffic does not impact the rate-limited traffic. Rate-limited traffic must be configured to be sent to rate-limited queues (via packet priority handling).

The allocated rates for rate-limited traffic must not be oversubscribed. For example, if port 1 is sending 15% rate limited traffic to port 2 priority 3, and port 0 is also sending 10% rate-limited traffic to port 2 priority 3, then the port 2 priority 3 egress rate must be configured to be 25% plus a percent or two for margin. The switch must be configured to allow some percentage of non rate-limited traffic. Non-ratelimited traffic must be configured to be sent to non rate-limited queues. No packets from the host should be dropped, but non rate-limited traffic received on an ethernet port can be dropped. Rate-limited mode is configured as shown:

- Set tx\_in\_sel[1:0] = 10 in P1/2\_Tx\_In\_Ctl to enable ports 1 and 2 transmit FIFO inputs to be configured for rate-limiting queues. Enabling a queue to be rate-limiting with this field affects only the packet being loaded into the FIFO, it does not configure the transmit for queue shaping.
- Configure the number of rate-limited queues for port 1 and 2 transmit FIFO's by setting the tx\_rate\_en[3:0] field in P1/2\_Tx\_In\_Ctl. Rate limited queues must be the highest number. For example, if there are two rate limited queues then 1100 would be written to this field for priorities 3 and 2. This field enables the FIFO to allow rate-limited traffic into rate-limited queues while discriminating against non rate-limited queues.
- Set p1\_priN\_shape\_en and p2\_priN\_shape\_en in the CPSW\_3G PTYPE register. These bits determine which queues actually shape the output data stream. In general, the same priorities that are set in tx\_rate\_en are set in these bits as well, but the FIFO input and output enable bits are separate to allow rate-limiting from the host to non shaped channels if desired. When queue shaping is not enabled for a queue then packets are selected for egress based on priority. When queue shaping is enabled then packets are selected for egress based on queue percentages. If shaping is required on a single queue then it must be priority 3 (priorities 2, 1 and 0 are strict priority). If shaping is required on two queues then it must be on priorities 2 and 3 (priorities 1 and 0 are strict priority). If shaping is required on three queues then it must be priorities 3, 2, and 1 (priority 0 would then get the leftovers). Priority shaping follows the requirements in the IEEE P802.1Qav/D6.0 specification. Priority shaping is not compatible with priority escalation (escalation must be disabled).
- P0\_Tx\_In\_Ctl[1:0] should remain at the default 00 value. Port 0 egress (CPDMA RX) should not be rate-limited.
- The CPDMA is configured for rate-limited transmit (switch ingress) channels by setting the highest bits of the tx\_rlim[7:0] field in the CPDMA DMA\_Control register. If there are two rate limited channels then tx\_rlim[7:0] = 11000000 (the rate limited channels must be the highest priorities). Also, tx\_ptype in the DMA\_Control register must be set (fixed priority mode). Rate limited channels must go to rate-limited FIFO queues, and the FIFO queue rate must not be oversubscribed.

#### **14.3.2.11 Packet Padding**

VLAN tagged ingress packets of 64 to 67-bytes will be padded to 64-bytes on egress (all ports) if the VLAN is removed on egress.

#### **14.3.2.12 Flow Control**

There are two types of switch flow control – CPPI port flow control and Ethernet port flow control. The CPPI and Ethernet port naming conventions for data flow into and out of the switch are reversed. For the CPPI port (port 0), transmit operations move packets from external memory into the switch and then out to either or both Ethernet transmit ports (ports 1 and 2). CPPI receive operations move packets that were received on either or both Ethernet receive ports to external memory.

#### *14.3.2.12.1 CPPI Port Flow Control*

The CPPI port has flow control available for transmit (switch ingress). CPPI receive operations (switch egress) do not require flow control. CPPI Transmit flow control is initiated when enabled and triggered. CPPI transmit flow control is enabled by setting the p0\_flow\_en bit in the **CPSW\_Flow\_Control** register. CPPI transmit flow control is enabled by default on reset because host packets should not be dropped in any mode of operation.

#### *14.3.2.12.2 Ethernet Port Flow Control*

The Ethernet ports have flow control available for transmit and receive. Transmit flow control stops the Ethernet port from transmitting packets to the wire (switch egress) in response to a received pause frame. Transmit flow control does not depend on FIFO usage.

The ethernet ports have flow control available for receive operations (packet ingress). Ethernet port receive flow control is initiated when enabled and triggered. Packets received on an ethernet port can be sent to the other ethernet port or the CPPI port (or both). Each destination port can trigger the receive ethernet port flow control. An ethernet destination port triggers another ethernet receive flow control when the destination port is full.

When a packet is received on an ethernet port interface with enabled flow control the below occurs:

- The packet will be sent to all ports that currently have room to take the entire packet.
- The packet will be retried until successful to all ports that indicate they don't have room for the packet.

The flow control trigger to the CPGMAC\_SL will be asserted until the packet has been sent, and there is room in the logical receive FIFO for packet runout from another flow control trigger (**rx\_pkt\_cnt** = 0). Ethernet port receive flow control is disabled by default on reset. Ethernet port receive flow control requires that the **rx\_flow\_en** bit in the associated CPGMAC\_SL be set to one.

When receive flow control is enabled on a port, the port's associated FIFO block allocation must be adjusted. The port RX allocation must increase from the default three blocks to accommodate the flow control runout. A corresponding decrease in the TX block allocation is required. If a sending port ignores a pause frame then packets may overrun on receive (and be dropped) but will not be dropped on transmit. If flow control is disabled for gmii ports, then any packets that are dropped are dropped on transmit and not on receive.

#### *14.3.2.12.2.1 Receive Flow Control*

When enabled and triggered, receive flow control is initiated to limit the CPGMAC\_SL from further frame reception. Half-duplex mode receive flow control is collision based while full duplex mode issues 802.3X pause frames. In either case, receive flow control prevents frame reception by issuing the flow control appropriate for the current mode of operation. Receive flow control is enabled by the **rx\_flow\_en** bit in the **MacControl** register. Receive flow control is triggered (when enabled) when the **RX\_FLOW\_TRIGGER** input is asserted. The CPGMAC\_SL is configured for collision or IEEE 802.3X flow control via the **fullduplex** bit in the **MacControl** register.

#### *14.3.2.12.2.1.1 Collision Based Receive Buffer Flow Control*

Collision-based receive buffer flow control provides a means of preventing frame reception when the port is operating in half-duplex mode (**fullduplex** is cleared in **MacControl)**. When receive flow control is enabled and triggered, the port will generate collisions for received frames. The jam sequence transmitted will be the twelve byte sequence C3.C3.C3.C3.C3.C3.C3.C3.C3.C3.C3.C3 (hex). The jam sequence will begin no later than approximately as the source address starts to be received. Note that these forced collisions will not be limited to a maximum of 16 consecutive collisions, and are independent of the normal back-off algorithm. Receive flow control does not depend on the value of the incoming frame destination address. A collision will be generated for any incoming packet, regardless of the destination address.

#### *14.3.2.12.2.1.2 IEEE 802.3X Based Receive Flow Control*

IEEE 802.3x based receive flow control provides a means of preventing frame reception when the port is operating in full-duplex mode (**fullduplex** is set in **MacControl)**. When receive flow control is enabled and triggered, the port will transmit a pause frame to request that the sending station stop transmitting for the period indicated within the transmitted pause frame.

The CPGMAC\_SL will transmit a pause frame to the reserved multicast address at the first available opportunity (immediately if currently idle, or following the completion of the frame currently being transmitted). The pause frame will contain the maximum possible value for the pause time (0xFFFF). The MAC will count the receive pause frame time (decrements 0xFF00 downto zero) and retransmit an outgoing pause frame if the count reaches zero. When the flow control request is removed, the MAC will transmit a pause frame with a zero pause time to cancel the pause request.

Note that transmitted pause frames are only a request to the other end station to stop transmitting. Frames that are received during the pause interval will be received normally (provided the Rx FIFO is not full).

Pause frames will be transmitted if enabled and triggered regardless of whether or not the port is observing the pause time period from an incoming pause frame.

The CPGMAC\_SL will transmit pause frames as described below:

- The 48-bit reserved multicast destination address 01.80.C2.00.00.01.
- The 48-bit source address SL\_SA(47:0).
- The 16-bit length/type field containing the value 88.08
- The 16-bit pause opcode equal to 00.01
- The 16-bit pause time value FF.FF. A pause-quantum is 512 bit-times. Pause frames sent to cancel a pause request will have a pause time value of 00.00.
- Zero padding to 64-byte data length (The CPGMAC\_SL will transmit only 64 byte pause frames).
- The 32-bit frame-check sequence (CRC word).

All quantities above are hexadecimal and are transmitted most-significant byte first. The least-significant bit is transferred first in each byte.

If **rx\_flow\_en** is cleared to zero while the pause time is nonzero, then the pause time will be cleared to zero and a zero count pause frame will be sent.

#### *14.3.2.12.2.2 Transmit Flow Control*

Incoming pause frames are acted upon, when enabled, to prevent the CPGMAC\_SL from transmitting any further frames. Incoming pause frames are only acted upon when the **fullduplex** and **tx\_flow\_en** bits in the **MacControl** register are set. Pause frames are not acted upon in half-duplex mode. Pause frame action will be taken if enabled, but normally the frame will be filtered and not transferred to memory.

MAC control frames will be transferred to memory if the **rx\_cmf\_en** (Copy MAC Frames) bit in the **MacControl** register is set. The **tx\_flow\_en** and **fullduplex** bits effect whether or not MAC control frames are acted upon, but they have no effect upon whether or not MAC control frames are transferred to memory or filtered.

Pause frames are a subset of MAC Control Frames with an opcode field=0x0001. Incoming pause frames will only be acted upon by the port if:

- **tx\_flow\_en** is set in **MacControl**, and
- the frame's length is 64 to **rx\_maxlen** bytes inclusive, and
- the frame contains no crc error or align/code errors.

The pause time value from valid frames will be extracted from the two bytes following the opcode. The pause time will be loaded into the port's transmit pause timer and the transmit pause time period will begin.

If a valid pause frame is received during the transmit pause time period of a previous transmit pause frame then:

- if the destination address is not equal to the reserved multicast address or any enabled or disabled unicast address, then the transmit pause timer will immediately expire, or
- if the new pause time value is zero then the transmit pause timer will immediately expire, else
- the port transmit pause timer will immediately be set to the new pause frame pause time value. (Any remaining pause time from the previous pause frame will be discarded).

If **tx\_flow\_en** in **MacControl** is cleared, then the pause-timer will immediately expire.

The port will not start the transmission of a new data frame any sooner than 512-bit times after a pause frame with a non-zero pause time has finished being received (**GMII\_RXDV** going inactive). No transmission will begin until the pause timer has expired (the port may transmit pause frames in order to initiate outgoing flow control). Any frame already in transmission when a pause frame is received will be completed and unaffected.

Incoming pause frames consist of the below:

- A 48-bit destination address equal to:
- The reserved multicast destination address 01.80.C2.00.00.01, or
- The SL\_SA(47:0) input mac source address.
- The 48-bit source address of the transmitting device.
- The 16-bit length/type field containing the value 88.08
- The 16-bit pause opcode equal to 00.01
- The 16-bit pause\_time. A pause-quantum is 512 bit-times.
- Padding to 64-byte data length.
- The 32-bit frame-check sequence (CRC word).

All quantities above are hexadecimal and are transmitted most-significant byte first. The least-significant bit is transferred first in each byte.

The padding is required to make up the frame to a minimum of 64 bytes. The standard allows pause frames longer than 64 bytes to be discarded or interpreted as valid pause frames. The CPGMAC\_SL will recognize any pause frame between 64 bytes and **rx\_maxlen** bytes in length.

#### **14.3.2.13 Packet Drop Interface**

The packet drop interface supports an external packet drop engine. The port 1 (and port 2) CPGMAC\_SL receive FIFO VBUSP interface signals are CPSW\_3G outputs. The receive packet interface has an associated packet drop input P1\_RFIFO\_DROP (P2\_RFIFO\_DROP). An external packet drop engine may "snoop" the received packet header and data to determine whether or not the packet should be dropped.

If the packet is to be dropped the external logic must assert the drop signal by no later than the second clock after the end of packet (or abort) indication from the CPGMAC\_SL. The drop signal should remain asserted until the second clock after the end of packet (or abort) indication. If the packet is not to be dropped then the drop signal should remain deasserted. The CPGMAC\_SL section contains more information on the receive FIFO VBUSP interface signals and end of packet indication.

#### **14.3.2.14 Short Gap**

The port 1 (and port 2) transmit inter-packet gap (IPG) may be shortened by eight bit times when enabled and triggered. The **tx\_short\_gap\_en** bit in the **SL1\_MacControl (SL2\_MacControl)** register enables the gap to be shortened when triggered. The condition is triggered when the port 1 (port 2) transmit FIFO has a user defined number of FIFO blocks used. The port 1 transmit FIFO blocks used determines if the port 1 gap is shortened, and the port 2 transmit FIFO blocks used determines if the port 2 gap is shortened. The **CPSW\_Gap\_Thresh** register value determines the port 1 short gap threshold, and the **CPSW\_Gap\_Thresh** register value determines the port 2 short gap threshold.

#### **14.3.2.15 Switch Latency**

The CPSW\_3G is a store and forward switch. The switch latency is defined as the amount of time between the end of packet reception of the received packet to the start of the output packet transmit.

| Mode       | Latency |
|------------|---------|
| Gig (1000) | 880ns   |
| 100        | 1.3us   |
| 10         | 6.5us   |

#### **14.3.2.16 Emulation Control**

The emulation control input (EMUSUSP) and submodule emulation control registers allow CPSW\_3G operation to be completely or partially suspended. There are three CPSW\_3G submodules that contain emulation control registers (CPGMAC\_SL1, CPGMAC\_SL2, and CPDMA). The submodule emulation control registers must be accessed to facilitate CPSW\_3G emulation control. The CPSW\_3G module enters the emulation suspend state if all three submodules are configured for emulation suspend and the emulation suspend input is asserted.

A partial emulation suspend state is entered if one or two submodules is configured for emulation suspend and the emulation suspend input is asserted. Emulation suspend occurs at packet boundaries. The emulation control feature is implemented for compatibility with other peripherals.

## **CPGMAC\_SL Emulation Control**

The emulation control input (**TBEMUSUP**) and register bits (**soft** and **free** in the **EMControl** register) allow CPGMAC\_SL operation to be suspended. When the emulation suspend state is entered, the CPGMAC\_SL will stop processing receive and transmit frames at the next frame boundary. Any frame currently in reception or transmission will be completed normally without suspension. For receive, frames that are detected by the CPGMAC\_SL after the suspend state is entered are ignored. Emulation control is implemented for compatibility with other peripherals.

### **CPDMA Emulation Control**

The emulation control input (**TBEMUSUP**) and register bits (**soft** and **free** in the **EMControl** register) allow CPDMA operation to be suspended. When the emulation suspend state is entered, the CPDMA will stop processing receive and transmit frames at the next frame boundary. Any frame currently in reception or transmission will be completed normally without suspension. For transmission, any complete or partial frame in the tx cell fifo will be transmitted. For receive, frames that are detected by the CPDMA after the suspend state is entered are ignored. No statistics will be kept for ignored frames. Emulation control is implemented for compatibility with other peripherals.

The following table shows the operations of the emulation control input and register bits:

**EMUSUSP soft free Description** 0 X X Normal Operation 1 0 0 Normal Operation 1 1 0 Emulation Suspend 1 X 1 Normal Operation

**Table 14-17. Operations of Emulation Control Input and Register Bits**

#### **Emulation Suspend Input**

The emulation suspend input described above comes from the Debug Subsystem. See Chapter 27, *Debug Subsystem*, to enable an emulation suspend event input for the Ethernet Subsystem (EMAC).

#### **14.3.2.17 Software IDLE**

The submodule software idle register bits enable CPSW\_3G operation to be completely or partially suspended by software control. There are three CPSW\_3G submodules that contain software idle register bits (CPGMAC\_SL1, CPGMAC\_SL2, and CPDMA). Each of the three submodules may be individually commanded to enter the idle state. The idle state is entered at packet boundaries, and no further packet operations will occur on an idled submodule until the idle command is removed. The CPSW\_3G module enters the idle state when all three submodules are commanded to enter and have entered the idle state. Idle status is determined by reading or polling the three submodule idle bits. The CPSW\_3G is in the idle state when all three submodules are in the idle state. The **CPSW\_Soft\_Idle** bit may be set if desired after the submodules are in the idle state. The **CPSW\_Soft\_Idle** bit causes packets to not be transferred from one FIFO to another FIFO internal to the switch.

#### **14.3.2.18 Software Reset**

The CPSW\_3G software reset register, CPSW\_3GSS software reset register and the three submodule software reset registers enable the CPSW\_3GSS to be reset by software. There are three CPSW\_3G submodules that contain software reset registers (CPGMAC\_SL1, CPGMAC\_SL2, and CPDMA). Each of the three submodules may be individually commanded to be reset by software.

For the CPDMA, the reset state is entered at packet boundaries, at which time the CPDMA reset occurs. The CPGMAC\_SL soft reset is immediate. Submodule reset status is determined by reading or polling the submodule reset bit. If the submodule reset bit is read as a one, then the reset process has not yet completed. The submodule soft reset process could take up to 2ms each. The reset has completed if the submodule reset bit is read as a zero.

After all three submodules (in any order) have been reset and a read of each submodule reset bit indicates that the reset process is complete, the CPSW\_3G software reset register bit may be written to complete the CPSW\_3G module software reset operation. The CPSW\_3G software reset bit controls the reset of the FIFO's, the statistics submodule, and the address lookup engine (ALE). The CPSW\_3G software reset is immediate and will be indicated by reading a zero from the soft reset bit.

The CPSW\_3GSS software reset bit controls the reset of the INT, REGS and CPPI. The CPSW\_3GSS software reset is immediate and will be indicated by reading a zero from the soft reset bit.

#### **14.3.2.19 FIFO Loopback**

FIFO loopback mode is entered when the fifo\_loopback bit in the CPSW\_Control register is set. FIFO loopback mode causes packets received on a port to be turned around and transmitted back on the same port. Port 0 receive is fixed on channel zero in FIFO loopback mode. The RXSOFOVERRUN statistic is incremented for each packet sent in FIFO loopback mode. Packets sent in with errors are returned with errors (they are not dropped). FIFO loopback is intended as a simple mechanism for test purposes. FIFO loopback should be performed in fullduplex mode only.

#### **14.3.2.20 CPSW\_3G Network Statistics**

The CPSW\_3G has a set of statistics that record events associated with frame traffic on selected switch ports. The statistics values are cleared to zero 38 clocks after the rising edge of VBUSP\_RST\_N. When one or more port enable bits (stat\_port\_en[2:0]) are set, all statistics registers are write to decrement. The value written will be subtracted from the register value with the result being stored in the register. If a value greater than the statistics value is written, then zero will be written to the register (writing 0xffffffff will clear a statistics location).

When all port enable bits are cleared to zero, all statistics registers are read/write (normal write direct, so writing 0x00000000 will clear a statistics location). All write accesses must be 32-bit accesses. In the below statistics descriptions, "the port" refers to any enabled port (with a corresponding set stat\_port\_en[2:0] bit).

The statistics interrupt (STAT\_PEND) will be issued if enabled when any statistics value is greater than or equal to 0x80000000. The statistics interrupt is removed by writing to decrement any statistics value greater than 0x80000000. The statistics are mapped into internal memory space and are 32-bits wide. All statistics rollover from 0xFFFFFFFF to 0x00000000.

#### *14.3.2.20.1 Rx-only Statistics Descriptions*

#### *14.3.2.20.1.1 Good Rx Frames (Offset = 0h)*

The total number of good frames received on the port. A good frame is defined to be:

- Any data or MAC control frame which matched a unicast, broadcast or multicast address, or matched due to promiscuous mode
- Had a length of 64 to rx\_maxlen bytes inclusive
- Had no CRC error, alignment error or code error.

See the Rx Align/Code Errors and Rx CRC errors statistic descriptions for definitions of alignment, code and CRC errors. Overruns have no effect upon this statistic.

#### *14.3.2.20.1.2 Broadcast Rx Frames (Offset = 4h)*

The total number of good broadcast frames received on the port. A good broadcast frame is defined to be:

- Any data or MAC control frame which was destined for only address 0xFFFFFFFFFFFF
- Had a length of 64 to rx\_maxlen bytes inclusive
- Had no CRC error, alignment error or code error.

See the Rx Align/Code Errors and Rx CRC errors statistic descriptions for definitions of alignment, code and CRC errors. Overruns have no effect upon this statistic.

#### *14.3.2.20.1.3 Multicast Rx Frames (Offset = 8h)*

The total number of good multicast frames received on the port. A good multicast frame is defined to be:

- Any data or MAC control frame which was destined for any multicast address other than 0xFFFFFFFFFFFF
- Had a length of 64 to rx\_maxlen bytes inclusive
- Had no CRC error, alignment error or code error

See the Rx Align/Code Errors and Rx CRC errors statistic descriptions for definitions of alignment, code and CRC errors. Overruns have no effect upon this statistic.

#### *14.3.2.20.1.4 Pause Rx Frames (Offset = Ch)*

The total number of IEEE 802.3X pause frames received by the port (whether acted upon or not). Such a frame:

- Contained any unicast, broadcast, or multicast address
- Contained the length/type field value 88.08 (hex) and the opcode 0x0001
- Was of length 64 to rx\_maxlen bytes inclusive
- Had no CRC error, alignment error or code error
- Pause-frames had been enabled on the port (tx\_flow\_en = 1).

The port could have been in either half or full-duplex mode.

See the Rx Align/Code Errors and Rx CRC errors statistic descriptions for definitions of alignment, code and CRC errors. Overruns have no effect upon this statistic.

#### *14.3.2.20.1.5 Rx CRC Errors (Offset = 10h)*

The total number of frames received on the port that experienced a CRC error. Such a frame:

- Was any data or MAC control frame which matched a unicast, broadcast or multicast address, or matched due to promiscuous mode
- Was of length 64 to rx\_maxlen bytes inclusive
- Had no code/align error,
- Had a CRC error

Overruns have no effect upon this statistic.

A CRC error is defined to be:

- A frame containing an even number of nibbles
- Failing the Frame Check Sequence test

#### *14.3.2.20.1.6 Rx Align/Code Errors (Offset = 14h)*

The total number of frames received on the port that experienced an alignment error or code error. Such a frame:

• Was any data or MAC control frame which matched a unicast, broadcast or multicast address, or matched due to promiscuous mode

- Was of length 64 to rx\_maxlen bytes inclusive
- Had either an alignment error or a code error

Overruns have no effect upon this statistic.

An alignment error is defined to be:

- A frame containing an odd number of nibbles
- Failing the Frame Check Sequence test if the final nibble is ignored

A code error is defined to be a frame which has been discarded because the port's MRXER pin driven with a one for at least one bit-time's duration at any point during the frame's reception.

Note: RFC 1757 etherStatsCRCAlignErrors Ref. 1.5 can be calculated by summing Rx Align/Code Errors and Rx CRC errors.

#### *14.3.2.20.1.7 Oversize Rx Frames (Offset = 18h)*

The total number of oversized frames received on the port. An oversized frame is defined to be:

- Was any data or MAC control frame which matched a unicast, broadcast or multicast address, or matched due to promiscuous mode
- Was greater than rx\_maxlen in bytes
- Had no CRC error, alignment error or code error

See the Rx Align/Code Errors and Rx CRC errors statistic descriptions for definitions of alignment, code and CRC errors. Overruns have no effect upon this statistic.

#### *14.3.2.20.1.8 Rx Jabbers (Offset = 1Ch)*

The total number of jabber frames received on the port. A jabber frame:

- Was any data or MAC control frame which matched a unicast, broadcast or multicast address, or matched due to promiscuous mode
- Was greater than rx\_maxlen in bytes
- Had no CRC error, alignment error or code error

See the Rx Align/Code Errors and Rx CRC errors statistic descriptions for definitions of alignment, code and CRC errors. Overruns have no effect upon this statistic.

#### *14.3.2.20.1.9 Undersize (Short) Rx Frames (Offset = 20h)*

The total number of undersized frames received on the port. An undersized frame is defined to be:

- Was any data frame which matched a unicast, broadcast or multicast address, or matched due to promiscuous mode
- Was greater than rx\_maxlen in bytes
- Had no CRC error, alignment error or code error

See the Rx Align/Code Errors and Rx CRC errors statistic descriptions for definitions of alignment, code and CRC errors. Overruns have no effect upon this statistic.

#### *14.3.2.20.1.10 Rx Fragments (Offset = 24h)*

The total number of frame fragments received on the port. A frame fragment is defined to be:

- Any data frame (address matching does not matter)
- Less than 64 bytes long
- Having a CRC error, an alignment error, or a code error
- Not the result of a collision caused by half duplex, collision based flow control

See the Rx Align/Code Errors and Rx CRC errors statistic descriptions for definitions of alignment, code and CRC errors. Overruns have no effect upon this statistic.

#### *14.3.2.20.1.11 Rx Start of Frame Overruns (Offset = 84h)*

The total number of frames received on the port that had a CPDMA start of frame (SOF) overrun or were dropped by due to FIFO resource limitations. SOF overrun frame is defined to be:

- Any data or MAC control frame which matched a unicast, broadcast or multicast address, or matched due to promiscuous mode
- Any length (including less than 64 bytes and greater than rx\_maxlen bytes)
- The CPDMA had a start of frame overrun or the packet was dropped due to FIFO resource limitations

#### *14.3.2.20.1.12 Rx Middle of Frame Overruns (Offset = 88h)*

The total number of frames received on the port that had a CPDMA middle of frame (MOF) overrun. MOF overrun frame is defined to be:

- Any data or MAC control frame which matched a unicast, broadcast or multicast address, or matched due to promiscuous mode
- Any length (including less than 64 bytes and greater than rx\_maxlen bytes)
- The CPDMA had a middle of frame overrun

#### *14.3.2.20.1.13 Rx DMA Overruns (Offset = 8Ch)*

The total number of frames received on the port that had either a DMA start of frame (SOF) overrun or a DMA MOF overrun. An Rx DMA overrun frame is defined to be:

- Any data or MAC control frame which matched a unicast, broadcast or multicast address, or matched due to promiscuous mode
- Any length (including less than 64 bytes and greater than rx\_maxlen bytes)
- The CPGMAC\_SL was unable to receive it because it did not have the DMA buffer resources to receive it (zero head descriptor pointer at the start or during the middle of the frame reception)

CRC errors, alignment errors and code errors have no effect upon this statistic.

#### *14.3.2.20.1.14 Rx Octets (Offset = 30h)*

The total number of bytes in all good frames received on the port. A good frame is defined to be:

- Any data or MAC control frame which matched a unicast, broadcast or multicast address, or matched due to promiscuous mode
- Of length 64 to rx\_maxlen bytes inclusive
- Had no CRC error, alignment error or code error

See the Rx Align/Code Errors and Rx CRC errors statistic descriptions for definitions of alignment, code and CRC errors. Overruns have no effect upon this statistic.

#### *14.3.2.20.1.15 Net Octets (Offset = 80h)*

The total number of bytes of frame data received and transmitted on the port. Each frame counted:

- was any data or MAC control frame destined for any unicast, broadcast or multicast address (address match does not matter)
- Any length (including less than 64 bytes and greater than rx\_maxlen bytes)

Also counted in this statistic is:

- Every byte transmitted before a carrier-loss was experienced
- Every byte transmitted before each collision was experienced, (i.e. multiple retries are counted each time)
- Every byte received if the port is in half-duplex mode until a jam sequence was transmitted to initiate flow control. (The jam sequence was not counted to prevent double-counting)

Error conditions such as alignment errors, CRC errors, code errors, overruns and underruns do not affect the recording of bytes by this statistic.

The objective of this statistic is to give a reasonable indication of ethernet utilization

#### *14.3.2.20.2 Tx-only Statistics Descriptions*

The maximum and minimum transmit frame size is software controllable.

#### *14.3.2.20.2.1 Good Tx Frames (Offset = 34h)*

The total number of good frames received on the port. A good frame is defined to be:

- Any data or MAC control frame which matched a unicast, broadcast or multicast address, or matched due to promiscuous mode
- Any length
- Had no late or excessive collisions, no carrier loss and no underrun

#### *14.3.2.20.2.2 Broadcast Tx Frames (Offset = 38h)*

The total number of good broadcast frames received on the port. A good broadcast frame is defined to be:

- Any data or MAC control frame which was destined for only address 0xFFFFFFFFFFFF
- Any length
- Had no late or excessive collisions, no carrier loss and no underrun

#### *14.3.2.20.2.3 Multicast Tx Frames (Offset = 3Ch)*

The total number of good multicast frames received on the port. A good multicast frame is defined to be:

- Any data or MAC control frame which was destined for any multicast address other than 0xFFFFFFFFFFFF
- Any length
- Had no late or excessive collisions, no carrier loss and no underrun

#### *14.3.2.20.2.4 Pause Tx Frames (Offset = 40h)*

This statistic indicates the number of IEEE 802.3X pause frames transmitted by the port.

Pause frames cannot underrun or contain a CRC error because they are created in the transmitting MAC, so these error conditions have no effect upon the statistic. Pause frames sent by software will not be included in this count.

Since pause frames are only transmitted in full duplex carrier loss and collisions have no effect upon this statistic.

Transmitted pause frames are always 64 byte multicast frames so will appear in the Tx Multicast Frames and 64octet Frames statistics.

#### *14.3.2.20.2.5 Collisions (Offset = 48h)*

This statistic records the total number of times that the port experienced a collision. Collisions occur under two circumstances.

- 1. When a transmit data or MAC control frame:
  - Was destined for any unicast, broadcast or multicast address
  - Was any size
  - Had no carrier loss and no underrun
  - Experienced a collision. A jam sequence is sent for every non-late collision, so this statistic will increment on each occasion if a frame experiences multiple collisions (and increments on late collisions)

CRC errors have no effect upon this statistic.

2. When the port is in half-duplex mode, flow control is active, and a frame reception begins.

#### *14.3.2.20.2.6 Single Collision Tx Frames (Offset = 4Ch)*

The total number of frames transmitted on the port that experienced exactly one collision. Such a frame:

- Was any data or MAC control frame destined for any unicast, broadcast or multicast address
- Was any size
- Had no carrier loss and no underrun
- Experienced one collision before successful transmission. The collision was not late.

CRC errors have no effect upon this statistic.

#### *14.3.2.20.2.7 Multiple Collision Tx Frames (Offset = 50h)*

The total number of frames transmitted on the port that experienced multiple collisions. Such a frame:

- Was any data or MAC control frame destined for any unicast, broadcast or multicast address
- Was any size
- Had no carrier loss and no underrun
- Experienced 2 to 15 collisions before being successfully transmitted. None of the collisions were late.

CRC errors have no effect upon this statistic.

#### *14.3.2.20.2.8 Excessive Collisions (Offset = 54h)*

The total number of frames for which transmission was abandoned due to excessive collisions. Such a frame:

- Was any data or MAC control frame destined for any unicast, broadcast or multicast address
- Was any size
- Had no carrier loss and no underrun
- Experienced 16 collisions before abandoning all attempts at transmitting the frame. None of the collisions were late.

CRC errors have no effect upon this statistic.

#### *14.3.2.20.2.9 Late Collisions (Offset = 58h)*

The total number of frames on the port for which transmission was abandoned because they experienced a late collision. Such a frame:

- Was any data or MAC control frame destined for any unicast, broadcast or multicast address
- Was any size
- Experienced a collision later than 512 bit-times into the transmission. There may have been up to 15 previous (non-late) collisions which had previously required the transmission to be re-attempted. The Late Collisions statistic dominates over the single, multiple and excessive Collisions statistics - if a late collision occurs the frame will not be counted in any of these other three statistics.

CRC errors have no effect upon this statistic.

#### *14.3.2.20.2.10 Tx Underrun (Offset = 5Ch)*

There should be no transmitted frames that experience underrun.

#### *14.3.2.20.2.11 Deferred Tx Frames (Offset = 44h)*

The total number of frames transmitted on the port that first experienced deferment. Such a frame:

- Was any data or MAC control frame destined for any unicast, broadcast or multicast address
- Was any size
- Had no carrier loss and no underrun
- Experienced no collisions before being successfully transmitted

• Found the medium busy when transmission was first attempted, so had to wait.

CRC errors have no effect upon this statistic.

#### *14.3.2.20.2.12 Carrier Sense Errors (Offset = 60h)*

The total number of frames received on the port that had a CPDMA middle of frame (MOF) overrun. MOF overrun frame is defined to be:

- Was any data or MAC control frame destined for any unicast, broadcast or multicast address
- Was any size
- The carrier sense condition was lost or never asserted when transmitting the frame (the frame is not retransmitted). This is a transmit only statistic. Carrier Sense is a don't care for received frames. Transmit frames with carrier sense errors are sent until completion and are not aborted.

CRC errors have no effect upon this statistic.

#### *14.3.2.20.2.13 Tx Octets (Offset = 64h)*

The total number of bytes in all good frames transmitted on the port. A good frame is defined to be:

- Any data or MAC control frame which was destined for any unicast, broadcast or multicast address
- Was any size
- Had no late or excessive collisions, no carrier loss and no underrun.

#### *14.3.2.20.3 Rx- and Tx-Shared Statistics Descriptions*

#### *14.3.2.20.3.1 Rx + Tx 64 Octet Frames (Offset = 68h)*

The total number of 64-byte frames received and transmitted on the port. Such a frame is defined to be:

- Any data or MAC control frame which was destined for any unicast, broadcast or multicast address
- Did not experience late collisions, excessive collisions, or carrier sense error
- Was exactly 64 bytes long. (If the frame was being transmitted and experienced carrier loss that resulted in a frame of this size being transmitted, then the frame will be recorded in this statistic).

CRC errors, code/align errors and overruns do not affect the recording of frames in this statistic.

#### *14.3.2.20.3.2 Rx + Tx 65–127 Octet Frames (Offset = 6Ch)*

The total number of frames of size 65 to 127 bytes received and transmitted on the port. Such a frame is defined to be:

- Any data or MAC control frame which was destined for any unicast, broadcast or multicast address
- Did not experience late collisions, excessive collisions, or carrier sense error
- Was 65 to 127 bytes long

CRC errors, code/align errors, underruns and overruns do not affect the recording of frames in this statistic.

#### *14.3.2.20.3.3 Rx + Tx 128–255 Octet Frames (Offset = 70h)*

The total number of frames of size 128 to 255 bytes received and transmitted on the port. Such a frame is defined to be:

- Any data or MAC control frame which was destined for any unicast, broadcast or multicast address
- Did not experience late collisions, excessive collisions, or carrier sense error
- Was 128 to 255 bytes long

CRC errors, code/align errors, underruns and overruns do not affect the recording of frames in this statistic.

#### *14.3.2.20.3.4 Rx + Tx 256–511 Octet Frames (Offset = 74h)*

The total number of frames of size 256 to 511 bytes received and transmitted on the port. Such a frame is defined to be:

- Any data or MAC control frame which was destined for any unicast, broadcast or multicast address
- Did not experience late collisions, excessive collisions, or carrier sense error
- Was 256 to 511 bytes long

CRC errors, code/align errors, underruns and overruns do not affect the recording of frames in this statistic.

#### *14.3.2.20.3.5 Rx + Tx 512–1023 Octet Frames (Offset = 78h)*

The total number of frames of size 512 to 1023 bytes received and transmitted on the port. Such a frame is defined to be:

- Any data or MAC control frame which was destined for any unicast, broadcast or multicast address
- Did not experience late collisions, excessive collisions, or carrier sense error
- Was 512 to 1023 bytes long

CRC errors, code/align errors, underruns and overruns do not affect the recording of frames in this statistic.

#### *14.3.2.20.3.6 Rx + Tx 1024\_Up Octet Frames (Offset = 7Ch)*

The total number of frames of size 1024 to rx\_maxlen bytes for receive or 1024 up for transmit on the port. Such a frame is defined to be:

- Any data or MAC control frame which was destined for any unicast, broadcast or multicast address
- Did not experience late collisions, excessive collisions, or carrier sense error
- Was 1024 to rx\_maxlen bytes long on receive, or any size on transmit

CRC errors, code/align errors, underruns and overruns do not affect the recording of frames in this statistic.

## Table 14-18. Rx Statistics Summary

|                      |        |           |             | Fr                | ame Ty         | pe             |              |     |     | F          | rame S      | Size (byt   | es)          |               |            |               |              | Event          |              |                |
|----------------------|--------|-----------|-------------|-------------------|----------------|----------------|--------------|-----|-----|------------|-------------|-------------|--------------|---------------|------------|---------------|--------------|----------------|--------------|----------------|
|                      | Frame/ | Rx/       | MAC c       | ontrol            |                | Data           |              |     |     |            |             |             |              | 1024-         | >rx        |               |              |                |              |                |
| Rx Statistic         | Oct    | Rx+T<br>x | Pause frame | Non-<br>paus<br>e | Multi-<br>cast | Broad-<br>cast | Uni-<br>cast | <64 | 64  | 65-<br>127 | 128-<br>255 | 256-<br>511 | 512-<br>1023 | rx_<br>maxlen | maxle<br>n | Flow<br>Coll. | CRC<br>Error | Align/<br>Code | Over-<br>run | Addr.<br>Disc. |
| Good Rx Frames       | F      | Rx        | (yl         | yl                | yl             | yl             | y)           | n   | (yl | yl         | yl          | yl          | yl           | y)            | n          | -             | n            | n              | -            | n              |
| Broadcast Rx Frames  | F      | Rx        | (%          | %                 | n              | y)             | n            | n   | (y  | yl         | yl          | yl          | yl           | y)            | n          | -             | n            | n              | -            | n              |
| Multicast Rx Frames  | F      | Rx        | (%          | %                 | y)             | n              | n            | n   | (y  | yl         | yl          | yl          | yl           | y)            | n          | -             | n            | n              | -            | n              |
| Pause Rx Frames      | F      | Rx        | y | n                 | n              | n              | n            | n   | (y  | yl         | yl          | yl          | yl           | y)            | n          | -             | n            | n              | -            | -              |
| Rx CRC Errors        | F      | Rx        | (yl         | yl                | yl             | yl             | y)           | n   | (y  | yl         | yl          | yl          | yl           | y)            | n          | -             | Y | n              | -            | n              |
| Rx Align/Code Errors | F      | Rx        | (yl         | yl                | yl             | yl             | y)           | n   | (y  | yl         | yl          | yl          | yl           | y)            | n          | -             | -            | Y | -            | n              |
| Oversized Rx Frames  | F      | Rx        | (yl         | yl                | yl             | yl             | y)           | n   | n   | n          | n           | n           | n            | n             | y | -             | n            | n              | -            | n              |
| Rx Jabbers           | F      | Rx        | (yl         | yl                | yl             | yl             | y)           | n   | n   | n          | n           | n           | n            | n             | y | -             | (yl          | y)             | -            | n              |
| Undersized Rx Frames | F      | Rx        | n           | n                 | (yl            | yl             | y)           | y | n   | n          | n           | n           | n            | n             | n          | -             | n            | n              | -            | n              |
| Rx Fragments         | F      | Rx        | n           | n                 | (yl            | yl             | y)           | y^^ | n   | n          | n           | n           | n            | n             | n          | -             | (yl          | y)             | -            | -              |
| Rx Overruns          | F      | Rx        | (yl         | yl                | yl             | yl             | y)           | (y  | yl  | yl         | yl          | yl          | yl           | yl            | y)         | -             | -            | -              | Y | n              |
| 64octet Frames       | F      | Rx+T<br>x | (yl         | yl                | yl             | yl             | y)           | n   | y | n          | n           | n           | n            | n             | n          | -             | -            | -              | -            | n              |
| 65-127octet Frames   | F      | Rx+T<br>x | (yl         | yl                | yl             | yl             | y)           | n   | n   | y | n           | n           | n            | n             | n          | -             | -            | -              | -            | n              |
| 128-255octet Frames  | F      | Rx+T<br>x | (yl         | yl                | yl             | yl             | y)           | n   | n   |            | y | n           | n            | n             | n          | =             | -            | -              | -            | n              |
| 256-511octet Frames  | F      | Rx+T<br>x | (yl         | yl                | yl             | yl             | y)           | n   | n   | n          | n           | y | n            | n             | n          | -             | -            | -              | -            | n              |
| 512-1023octet Frames | F      | Rx+T<br>x | (yl         | yl                | yl             | yl             | y)           | n   | n   | n          | n           | n           | y | n             | n          | -             | -            | -              | -            | n              |
| 1024-UPoctet Frames  | F      | Rx+T<br>x | (yl         | yl                | yl             | yl             | y)           | n   | n   | n          | n           | n           | n            | y | n          | -             | -            | -              | -            | n              |
| Rx Octets            | 0      | Rx        | (yl         | yl                | yl             | yl             | y)           | n   | (yl | yl         | yl          | yl          | yl           | y)            | n          | -             | n            | n              | -            | n              |
| Net Octets           | 0      | Rx+T<br>x | (yl         | yl                | yl             | yl             | y)           | (yl | yl  | yl         | yl          | yl          | yl           | yl            | yl         | y)            | -            | -              | -            | -              |

## **Notes for the Rx Statistics Summary:**

- 1. "AND" is assumed horizontally across the table between all conditions which form the statistic (marked y or n) except where (y|y), meaning "OR" is indicated. Parentheses are significant.
- 2. "-" indicates conditions which are ignored in the formations of the statistic.
- 3. Statistics marked "Rx+Tx" are formed by summing the Rx and Tx statistics, each of which is formed independently.
- 4. The non-pause column refers to all MAC control frames (i.e. frames with length/type=88.08) with opcodes other than 0x0001. The pauseframe column refers to MAC frames with the opcode=0x0001.
- 5. The multicast, broadcast and unicast columns in the table refer to non-MAC Control/non-pause frames (i.e. data frames).
- 6. "%" If either a MAC control frame or pause frame has a multicast or broadcast destination address then the appropriate statistics will be updated.
- 7. "y^" Frame fragments are not counted if less than 8 bytes.
- 8. flow coll. are half-duplex collisions forced by the MAC to achieve flow-control. A collision will be forced during the first 8 bytes so should not show in frame fragments. Some of the '-'s in this column might in reality be 'n's.
- 9. The rx\_overruns stat show above is for rx\_mof\_overruns and rx\_sof\_overruns added together.

## Table 14-19. Tx Statistics Summary

|                                 |               |           |                        | Fr               | ame Ty             | <sub>′</sub> pe    |              |     |            | Frame       | Size        | (bytes)      |                   |           |              |      |     |          | Ev  | ent  |                   |            |              |                   |
|---------------------------------|---------------|-----------|------------------------|------------------|--------------------|--------------------|--------------|-----|------------|-------------|-------------|--------------|-------------------|-----------|--------------|------|-----|----------|-----|------|-------------------|------------|--------------|-------------------|
|                                 |               | Tx/       | MAC o                  | control          |                    | Data               |              |     |            |             |             |              |                   |           |              |      | Col | lision T | ype |      |                   |            |              |                   |
| Tx Statistic                    | Frame/<br>Oct | Rx+<br>Tx | Paus<br>e<br>(MA<br>C) | Any<br>(CP<br>U) | Multi<br>-<br>cast | Broa<br>d-<br>cast | Uni-<br>cast | 64  | 65-<br>127 | 128-<br>255 | 256-<br>511 | 512-<br>1023 | 1024<br>-<br>1535 | ><br>1535 | CRC<br>Error | Flow | 1   | 2-<br>15 | 16  | Late | No<br>Carri<br>er | Que<br>ued | Defe<br>rred | Und<br>er-<br>run |
| Good Tx Frames                  | F             | Tx        | (yl                    | yl               | yl                 | y | y)           | (yl | yl         | yl          | yl          | yl           | y | y)        | -            | -    | -   | -        | n   | n    | n                 | -          | -            | n                 |
| Broadcast Tx<br>Frames          | F             | Tx        | n                      | (%               | n                  | y)                 | n            | (yl | yl         | yl          | yl          | yl           | yl                | y)        | -            | -    | -   | -        | n   | n    | n                 | -          | -            | n                 |
| Multicast Tx<br>Frames          | F             | Tx        | (yl                    | %                | y)                 | n                  | n            | yl  | yl         | yl          | yl          | yl           | yl                | y)        | -            | -    | -   | -        | n   | n    | n                 | 1          | -            | n                 |
| Pause Tx Frames                 | F             | Tx        | Y | n                | n                  | n                  | n            | Y | n          | n           | n           | n            | n                 | n         | -            | -    | -   | -        | -   | -    | -                 | -          | -            | -                 |
| Collisions                      | F             | Tx        | n                      | (yl              | yl                 | y | y)           | (y  | yl         | y | yl          | yl           | y | y)        | -            | (+   | +   | +        | +   | +)   | n                 | -          | -            | -                 |
| Single Collision Tx<br>Frames   | F             | Tx        | n                      | (yl              | yl                 | yl                 | y)           | (yl | yl         | yl          | yl          | yl           | yl                | y)        | -            | -    | y | n        | n   | n    | n                 | -          | -            | -                 |
| Multiple Collision<br>Tx Frames | F             | Tx        | n                      | (yl              | yl                 | yl                 | y)           | (yl | yl         | yl          | yl          | yl           | yl                | y)        | -            | -    | n   | y | n   | n    | n                 | -          | -            | -                 |
| Excessive<br>Collisions         | F             | Tx        | n                      | (yl              | yl                 | yl                 | y)           | (yl | yl         | yl          | yl          | yl           | yl                | y)        | -            | -    | n   | n        | y | n    | n                 | -          | -            | -                 |
| Late Collisions                 | F             | Tx        | n                      | (yl              | yl                 | yl                 | y)           | n   | (yl        | yl          | yl          | yl           | yl                | y)        | -            | -    | -   | -        | -   | y | -                 | -          | -            | -                 |
| Deferred Tx<br>Frames           | F             | Tx        | n                      | (yl              | yl                 | yl                 | y)           | (yl | yl         | yl          | yl          | yl           | yl                | y)        | -            | -    | n   | n        | n   | n    | n                 | -          | y | n                 |
| Carrier Sense<br>Errors         | F             | Tx        | (yl                    | yl               | yl                 | yl                 | y)           | (yl | yl         | yl          | yl          | yl           | yl                | y)        | -            | -    | -   | -        | -   | -    | y | -          | -            | -                 |
| 64octet Frames                  | F             | Rx+<br>Tx | (yl                    | yl               | yl                 | yl                 | y)           | y | n          | n           | n           | n            | n                 | n         | -            | -    | -   | -        | n   | n    | n                 | -          | -            | -                 |
| 65-127octet<br>Frames           | F             | Rx+<br>Tx | (yl                    | yl               | yl                 | yl                 | y)           | n   | y | n           | n           | n            | n                 | n         | -            | -    | -   | -        | n   | n    | n                 | -          | -            | -                 |
| 128-255octet<br>Frames          | F             | Rx+<br>Tx | (yl                    | yl               | yl                 | yl                 | y)           | n   | n          | y | n           | n            | n                 | n         | -            | -    | -   | -        | n   | n    | n                 | -          | -            | -                 |
| 256-511octet<br>Frames          | F             | Rx+<br>Tx | (yl                    | yl               | yl                 | yl                 | y)           | n   | n          | n           | y | n            | n                 | n         | -            | -    | -   | -        | n   | n    | n                 | -          | -            | -                 |
| 512-1023octet<br>Frames         | F             | Rx+<br>Tx | (yl                    | yl               | yl                 | yl                 | y)           | n   | n          | n           | n           | y | n                 | n         | -            | -    | -   | -        | n   | n    | n                 | -          | -            | -                 |
| 1024-UPoctet<br>Frames          | F             | Rx+<br>Tx | (yl                    | yl               | yl                 | yl                 | y)           | n   | n          | n           | n           | n            | y | y | -            | -    | -   | -        | n   | n    | n                 | -          | -            | -                 |
| Tx Octets                       | 0             | Tx        | (yl                    | yl               | yl                 | yl                 | y)           | (yl | yl         | yl          | yl          | yl           | yl                | y)        | -            | -    | -   | -        | n   | n    | n                 | -          | -            | n                 |
| Net Octets                      | 0             | Rx+<br>Tx | (yl                    | yl               | yl                 | yl                 | y)           | (yl | yl         | yl          | yl          | yl           | yl                | y)        | -            | -    | \$  | \$       | \$  | \$   | \$                | 1          | -            | -                 |

## **Notes for the Tx Statistics Summary:**

- 1. "AND" is assumed horizontally across the table between all conditions which form the statistic (marked y or n) except where (y|y), meaning "OR" is indicated. Parentheses are significant.
- 2. "-" indicates conditions which are ignored in the formations of the statistic.
- 3. Statistics marked "Rx+Tx" are formed by summing the Rx and Tx statistics, each of which is formed independently.
- 4. Pause (MAC) frames are issued in the MAC as perfect (no CRC error) 64 byte frames in full duplex only, so they cannot collide.
- 5. "%" If a CPU sourced MAC control frame has a multicast or broadcast destination address then the appropriate statistics will be updated.
- 6. "+" indicates collisions which are "summed" (i.e. every collision is counted in the Collisions statistic). Jam sequences used for halfduplex flow control are also counted.
- 7. "\$" Every byte written on the wire during each retry attempt is also counted in addition to frames which experience no collisions or carrier loss.
- 8. The flow collision type is for half-duplex collisions forced by the MAC to achieve flow control. Some of the '-'s in this column might in reality be 'n's. To prevent double-counting, Net Octets are unaffected by the jam sequence – the 'received' bytes, however, are counted. (See Table [14-18.](#page-56-0))
- 9. When the transmit Tx FIFO is drained due to the MAC being disabled or link being lost, then the frames being purged will not appear in the Tx statistics.

### *14.3.3 Ethernet Mac Sliver (CPGMAC\_SL)*

The CPGMAC\_SL peripheral shall be compliant to the IEEE Std 802.3 Specification. Half duplex mode is supported in 10/100 Mbps mode, but not in 1000 Mbps (gigabit) mode.

### Features:

- Synchronous 10/100/1000 Mbit operation.
- G/MII Interface.
- Hardware Error handling including CRC.
- Full Duplex Gigabit operation (half duplex gigabit is not supported).
- EtherStats and 802.3Stats RMON statistics gathering support for external statistics collection module.
- Transmit CRC generation selectable on a per channel basis.
- Emulation Support.
- VLAN Aware Mode Support.
- Hardware flow control.
- Programmable Inter Packet Gap (IPG)

#### **14.3.3.1 GMII/MII Media Independent Interface**

The following sections cover operation of the Media Independent Interface in 10/100/1000 Mbps modes. An IEEE 802.3 compliant Ethernet MAC controls the interface.

#### *14.3.3.1.1 Data Reception*

#### *14.3.3.1.1.1 Receive Control*

Data received from the PHY is interpreted and output. Interpretation involves detection and removal of the preamble and start of frame delimiter, extraction of the address and frame length, data handling, error checking and reporting, cyclic redundancy checking (CRC), and statistics control signal generation.

#### *14.3.3.1.1.2 Receive Inter-Frame Interval*

The 802.3 required inter-packet gap (IPG) is 24 GMII clocks (96 bit times) for 10/100 Mbit modes, and 12 GMII clocks (96 bit times) for 1000 Mbit mode. However, the MAC can tolerate a reduced IPG (2 GMII clocks in 10/100 mode and 5 GMII clocks in 1000 mode) with a correct preamble and start frame delimiter.

This interval between frames must comprise (in the following order):

- An Inter-Packet Gap (IPG).
- A seven octet preamble (all octets 0x55).
- A one octet start frame delimiter (0x5D).

#### *14.3.3.1.2 Data Transmission*

The Gigabit Ethernet Mac Sliver (GMII) passes data to the PHY when enabled. Data is synchronized to the transmit clock rate. The smallest frame that can be sent is two bytes of data with four bytes of CRC (6 byte frame).

#### *14.3.3.1.2.1 Transmit Control*

A jam sequence is output if a collision is detected on a transmit packet. If the collision was late (after the first 64 bytes have been transmitted) the collision is ignored. If the collision is not late, the controller will back off before retrying the frame transmission. When operating in full duplex mode the carrier sense (CRS) and collision sensing modes are disabled.

#### *14.3.3.1.2.2 CRC Insertion*

The MAC generates and appends a 32-bit Ethernet CRC onto the transmitted data if the transmit packet header **pass\_crc** bit is zero. For the CPMAC\_SL generated CRC case, a CRC at the end of the input packet data is not allowed. If a CRC is not needed, set the pass\_crc bit to zero and adjust the packet length accordingly.

If the header word **pass\_crc** bit is set, then the last four bytes of the TX data are transmitted as the frame CRC. The four CRC data bytes should be the last four bytes of the frame and should be included in the packet byte count value. The MAC performs no error checking on the outgoing CRC when the **pass\_crc** bit is set.

#### *14.3.3.1.2.3 TXER*

The GMII\_TXER signal is not used. If an underflow condition occurs on a transmitted frame, the frame CRC will be inverted to indicate the error to the network. Underflow is a hardware error.

#### *14.3.3.1.2.4 Adaptive Performance Optimization (APO)*

The Ethernet MAC port incorporates Adaptive Performance Optimization (APO) logic that may be enabled by setting the **tx\_pace** bit in the **MacControl** register. Transmission pacing to enhance performance is enabled when set. Adaptive performance pacing introduces delays into the normal transmission of frames, delaying transmission attempts between stations, reducing the probability of collisions occurring during heavy traffic (as indicated by frame deferrals and collisions) thereby increasing the chance of successful transmission.

When a frame is deferred, suffers a single collision, multiple collisions or excessive collisions, the pacing counter is loaded with an initial value of 31. When a frame is transmitted successfully (without experiencing a deferral, single collision, multiple collision or excessive collision) the pacing counter is decremented by one, down to zero.

With pacing enabled, a new frame is permitted to immediately (after one IPG) attempt transmission only if the pacing counter is zero. If the pacing counter is non zero, the frame is delayed by the pacing delay, a delay of approximately four inter-packet gap delays. APO only affects the IPG preceding the first attempt at transmitting a frame. It does not affect the back-off algorithm for retransmitted frames.

#### *14.3.3.1.2.5 Inter-Packet-Gap Enforcement*

The measurement reference for the IPG of 96 bit times is changed depending on frame traffic conditions. If a frame is successfully transmitted without collision, and **GMII\_CRS** is de-asserted within approximately 48 bit times of **GMII\_TXEN** being de-asserted, then 96 bit times is measured from **GMII\_TXEN**. If the frame suffered a collision, or if **GMII\_CRS** is not de-asserted until more than approximately 48 bit times after **GMII\_TXEN** s de-asserted, then 96 bit times (approximately, but not less) is measured from **GMII\_CRS**.

The transmit IPG can be shortened by eight bit times when enabled and triggered. The **tx\_short\_gap\_en** bit in the **MacControl** register enables the **TX\_SHORT\_GAP** input to determine whether the transmit IPG is shorted by eight bit times.

#### *14.3.3.1.2.6 Back Off*

The Gigabit Ethernet Mac Sliver (GMII) implements the 802.3 binary exponential back-off algorithm.

#### *14.3.3.1.2.7 Programmable Transmit Inter-Packet Gap*

The transmit inter-packet gap (IPG) is programmable through the **Tx\_Gap** register. The default value is decimal 12. The transmit IPG may be increased to the maximum value of 0x1ff. Increasing the IPG is not compatible with transmit pacing. The short gap feature will override the increased gap value, so the short gap feature may not be compatible with an increased IPG.

#### *14.3.3.1.2.8 Speed, Duplex, and Pause Frame Support Negotiation*

The CPMAC\_SL can operate in half duplex or full duplex in 10/100 Mbit modes, and can operate in full duplex only in 1000 Mbit mode. Pause frame support is included in 10/100/1000 Mbit modes as configured by the host.

#### **14.3.3.2 Frame Classification**

Received frames are proper (good) frames if they are between 64 and **rx\_maxlen** in length (inclusive) and contain no errors (code/align/CRC).

Received frames are long frames if their frame count exceeds the value in the **rx\_maxlen** register. The **rx\_maxlen** register reset (default) value is 1518 (dec). Long received frames are either oversized or jabber frames. Long frames with no errors are oversized frames. Long frames with CRC, code, or alignment errors are jabber frames.

Received frames are short frames if their frame count is less than 64 bytes. Short frames that contain no errors are undersized frames. Short frames with CRC, code, or alignment errors are fragment frames.

A received long packet will always contain **rx\_maxlen** number of bytes transferred to memory (if **rx\_cef\_en** = 1). An example with **rx\_maxlen** = 1518 is below:

- If the frame length is 1518, then the packet is not a long packet and there will be 1518 bytes transferred to memory.
- If the frame length is 1519, there will be 1518 bytes transferred to memory. The last three bytes will be the first three CRC bytes.
- If the frame length is 1520, there will be 1518 bytes transferred to memory. The last two bytes will be the first two CRC bytes.
- If the frame length is 1521, there will be 1518 bytes transferred to memory. The last byte will be the first CRC byte.

If the frame length is 1522, there will be 1518 bytes transferred to memory. The last byte will be the last data byte.

### *14.3.4 Command IDLE*

The cmd\_idle bit in the MACCONTROL register allows CPGMAC\_SL operation to be suspended. When the idle state is commanded, the CPGMAC\_SL will stop processing receive and transmit frames at the next frame boundary. Any frame currently in reception or transmission will be completed normally without suspension. Received frames that are detected after the suspend state is entered are ignored. Commanded idle is similar in operation to emulation control and clock stop.

### *14.3.5 RMII Interface*

The CPRMII peripheral shall be compliant to the RMII specification document.

## Features:

- Source Synchronous 10/100 Mbit operation.
- Full and Half Duplex support.

#### **14.3.5.1 RMII Receive (RX)**

The CPRMII receive (RX) interface converts the input data from the external RMII PHY (or switch) into the required MII (CPGMAC) signals. The carrier sense and collision signals are determined from the RMII input data stream and transmit inputs as defined in the RMII specification.

An asserted RMII\_RXER on any di-bit in the received packet will cause an MII\_RXER assertion to the CPGMAC during the packet. In 10Mbps mode, the error is not required to be duplicated on 10 successive clocks. Any di-bit which has an asserted RMII\_RXER during any of the 10 replications of the data will cause the error to be propagated.

Any received packet that ends with an improper nibble boundary aligned RMII\_CRS\_DV toggle will issue an MII\_RXER during the packet to the CPGMAC. Also, a change in speed or duplex mode during packet operations will cause packet corruption.

The CPRMII can accept receive packets with shortened preambles, but 0x55 followed by a 0x5d is the shortest preamble that will be recognized (1 preamble byte with the start of frame byte). At least one byte of preamble with the start of frame indicator is required to begin a packet. An asserted RMII\_CRS\_DV without at least a single correct preamble byte followed by the start of frame indicator will be ignored.

#### **14.3.5.2 RMII Transmit (TX)**

The CPRMII transmit (TX) interface converts the 3PSW MII input data into the RMII transmit format. The data is then output to the external RMII PHY.

The 3PSW does not source the transmit error (MII TXERR) signal. Any transmit frame from the CPGMAC with an error (ie. underrun) will be indicated as an error by an error CRC. Transmit error is assumed to be deasserted at all times and is not an input into the CPRMII module.Zeroes are output on RMII\_TXD[1:0] for each clock that RMII\_TXEN is deasserted.

### *14.3.6 RGMII Interface*

The CPRGMII peripheral shall be compliant to the RGMII specification document.

## Features:

- Supports 1000/100/10 Mbps Speed.
- MII mode is not supported.

If RGMII is used, and a 10Mbit operation is desired, in-band mode must be used and an ethernet PHY that supports in-band status signaling must be selected.

#### **14.3.6.1 RGMII Receive (RX)**

The CPRGMII receive (RX) interface converts the source synchronous DDR input data from the external RGMII PHY into the required G/MII (CPGMAC) signals.

#### **14.3.6.2 In-Band Mode of Operation**

The CPRGMII is operating in the in-band mode of operation when the **RGMII\_RX\_INBAND** input is asserted.RGMII\_RX\_INPUT is asserted by configuring the ext\_en bit to 1 of the MACCONTROL register. The link status, duplexity, and speed are determined from the RGMII input data stream as defined in the RGMII specification. The link speed is indicated as shown in the following table:

| RGMII_SPEED(1:0) | Link Speed    |
|------------------|---------------|
| 00               | 10 Mbs mode   |
| 01               | 100 Mbs mode  |
| 10               | 1000 Mbs mode |
| 11               | reserved      |

#### **14.3.6.3 Forced Mode of Operation**

The CPRGMII is operating in the forced mode of operation when the **RGMII\_RX\_INBAND** input is deasserted by setting MACCONTROL.EXT\_EN to 0. In the forced mode of operation, the in-band data is ignored if present. In this mode, the contents of RGMII\_CTL are meaningless. Link status, duplexity, and speed status should be determined from the external ethernet PHY via MDIO transactions.

#### **14.3.6.4 RGMII Transmit (TX)**

The CPRGMII transmit (TX) interface converts the CPGMAC G/MII input data into the DDR RGMII format. The DDR data is then output to the external PHY.

The CPGMAC does not source the transmit error (MTXERR) signal. Any transmit frame from the CPGMAC with an error (that is, underrun) will be indicated as an error by an error CRC. Transmit error is assumed to be deasserted at all times and is not an input into the CPRGMII module.

The RGMII0/1\_ID\_MODE bit value in the GMII\_SEL register should only be set to 1 for 'no internal delay'. The device does not support internal delay mode for RGMII.

### *14.3.7 Common Platform Time Sync (CPTS)*

The CPTS module is used to facilitate host control of time sync operations. It enables compliance with the IEEE 1588-2008(v2) standard for a precision clock synchronization protocol.

#### **14.3.7.1 Architecture**

**Figure 14-10. CPTS Block Diagram**

```
          CPTS_RFT_CLK
                ▶
           ┌────────┐
           │ REGS   │
           └───┬────┘
               │
               │
        ┌──────▼────────┐
        │  EVENT_FIFO   │
        └─────┬─────────┘
              │
              │ EVNT_PEND
              ◀──────────────────────────────

              │
              │ RCLK
              ▶
              │
              ▼
        ┌───────────┐
        │    SCR    │
        └─┬─────┬───┘
          │     │
          │     ├────────────▶ TSPUSH
          │     ├────────────▶ TSCNTROLL
          │     │
          │     ├────────────▶ GMII_RX_0 ───▶ P1_TS_RX_DEC
          │     │                              P1_TS_RX_MII
          │     ├────────────▶ GMII_TX_0 ───▶ P1_TS_TX_DEC
          │     │                              P1_TS_TX_MII
          │     │
          │     ├────────────▶ ...
          │     │
          │     ├────────────▶ GMII_RX_n ───▶ Pn_TS_RX_DEC
          │     │                              Pn_TS_RX_MII
          │     └────────────▶ GMII_TX_n ───▶ Pn_TS_TX_DEC
          │                                     Pn_TS_TX_MII
          │
          │
   HW1_TS_PUSH ▶ ┌───────────┐
                 │HW1_TS_PUSH│─────────▶ (into SCR)
   HW2_TS_PUSH ▶ ├───────────┤
                 │HW2_TS_PUSH│─────────▶
   HW3_TS_PUSH ▶ ├───────────┤
                 │HW3_TS_PUSH│─────────▶
   HW4_TS_PUSH ▶ ├───────────┤
                 │HW4_TS_PUSH│─────────▶
                 └───────────┘
```

[Figure](#page-65-1) 14-10 shows the architecture of the CPTS module inside the 3PSW Ethernet Subsystem. Time stamp values for every packet transmitted or received on either port of the 3PSW are recorded. At the same time, each packet is decoded to determine if it is a valid time sync event. If so, an event is loaded into the Event FIFO for processing containing the recorded time stamp value when the packet was transmitted or received.

In addition, both hardware (HWx\_TS\_PUSH) and software (TS\_PUSH) can be used to read the current time stamp value though the Event FIFO

The reference clock used for the time stamp (RCLK) is sourced from one of the two sources, as shown in [Figure](#page-65-1) 14-10. The source can be selected by configuring the CM\_CPTS\_RFT\_CLKSEL register in the Control Module. For more details, see Chapter 9, *Control Module*.

#### **14.3.7.2 Time Sync Overview**

The CPTS module is used to facilitate host control of time sync operations. The CPTS collects time sync events and then presents them to the host for processing. There are five types of time sync events (ethernet receive event, ethernet transmit event, time stamp push event, time stamp rollover event, and time stamp half-rollover event). Each ethernet port can cause transmit and receive events. The time stamp push is initiated by software.

#### *14.3.7.2.1 Time Sync Initialization*

The CPTS module should be configured as shown:

- Complete the reset sequence (VBUSP\_RST\_N) to reset the module.
- Write the rftclk\_sel[4:0] value in the RFTCLK\_Sel register with the desired reference clock multiplexor value. This value is allowed to be written only when the cpts\_en bit is cleared to zero.
- Write a one to the cpts\_en bit in the TS\_Control register. The RCLK domain is in reset while this bit is low.
- Enable the interrupt by writing a one to the ts\_pend\_en bit in the TS\_Int\_Enable register (if using interrupts and not polling).

#### *14.3.7.2.2 Time Stamp Value*

The time stamp value is a 32-bit value that increments on each RCLK rising edge when CPTS\_EN is set to one. When CPTS\_EN is cleared to zero the time stamp value is reset to zero. If more than 32-bits of time stamp are required by the application, the host software must maintain the necessary number of upper bits. The upper time stamp value should be incremented by the host when the rollover event is detected.

For test purposes, the time stamp can be written via the time stamp load function (CPTS\_LOAD\_VAL and CPTS\_LOAD\_EN registers).

#### *14.3.7.2.3 Event FIFO*

All time sync events are pushed onto the Event FIFO. There are 16 locations in the event FIFO with no overrun indication supported. Software must service the event FIFO in a timely manner to prevent FIFO overrun.

#### *14.3.7.2.4 Time Sync Events*

Time Sync events are 64-bit values that are pushed onto the event FIFO and read in two 32-bit reads. CPTS\_EVENT\_LOW and CPTS\_EVENT\_HIGH are defined in Section 14.5.3.10 and Section 14.5.3.11, respectively.

There are six types of sync events

- Time stamp push event
- Hardware time stamp push event
- Time stamp counter rollover event
- Time stamp counter half-rollover event
- Ethernet receive event
- Ethernet transmit event

#### *14.3.7.2.4.1 Time Stamp Push Event*

Software can obtain the current time stamp value (at the time of the write) by initiating a time stamp push event. The push event is initiated by setting the TS\_PUSH bit of the CPTS\_TS\_PUSH register. The time stamp value is returned in the event, along with a time stamp push event code. Software should not push a second time stamp event on to the FIFO until the first time stamp value has been read from the event FIFO.

#### *14.3.7.2.4.2 Time Stamp Counter Rollover Event*

The CPTS module contains a 32-bit time stamp value. The counter upper bits are maintained by host software. The rollover event indicates to software that the time stamp counter has rolled over from 0xFFFF\_FFFF to 0x0000\_0000, and the software maintained upper count value should be incremented.

#### *14.3.7.2.4.3 Time Stamp Counter Half-Rollover Event*

The CPTS includes a time stamp counter half-rollover event. The half-rollover event indicates to software that the time stamp value has incremented from 0x7FFF\_FFFF to 0x8000\_0000. The half-rollover event is included to enable software to correct a misaligned event condition.The half-rollover event is included to enable software to determine the correct time for each event that contains a valid time stamp value – such as an Ethernet event. If an Ethernet event occurs around a counter rollover (full rollover), the rollover event could possibly be loaded into the event FIFO before the Ethernet event, even though the Ethernet event time was actually taken before the rollover. Figure 3 below shows a misalignment condition.

This misaligned event condition arises because an ethernet event time stamp occurs at the beginning of a packet and time passes before the packet is determined to be a valid synchronization packet. The misaligned event condition occurs if the rollover occurs in the middle, after the packet time stamp has been taken, but before the packet has been determined to be a valid time sync packet.

```
        ┌────────────────────┐
        │  Ethernet Event 1  │
        └─────────┬──────────┘
                  │
                  │
        ┌─────────▼──────────┐           EVENT FIFO
        │  Ethernet Event 2  │
        │    (highlighted)   │
        └─────────┬──────────┘             ┌────────────────────┐   Entry 1
                  │                        │  Ethernet Event 1  │
                  │                        ├────────────────────┤   Entry 2
        ┌─────────▼──────────┐             │   Rollover Event   │
        │   Rollover Event   │────────────▶├────────────────────┤   Entry 3
        └────────────────────┘             │  Ethernet Event 2  │ (highlighted)
                                           ├────────────────────┤   Entry 4
                                           │        ...         │
                                           ├────────────────────┤   Entry 15
                                           │                    │
                                           ├────────────────────┤   Entry 16
                                           │                    │
                                           └────────────────────┘
```

**Figure 14-11. Event FIFO Misalignment Condition**

Host software must detect and correct for misaligned event conditions. For every event after a rollover and before a half-rollover, software must examine the time stamp most significant bit. If bit 31 of the time stamp value is low (0x0000\_0000 through 0x7FFF\_FFFF), then the event time stamp was taken after the rollover and no correction is required.

If the value is high (0x8000\_0000 through 0xFFFF\_FFFF), the time stamp value was taken before the rollover and a misalignment is detected. The misaligned case indicates to software that it must subtract one from the upper count value stored in software to calculate the correct time for the misaligned event.The misaligned event occurs only on the rollover boundary and not on the half-rollover boundary. Software only needs to check for misalignment from a rollover event to a half-rollover event.

#### *14.3.7.2.4.4 Hardware Time Stamp Push Event*

There are four hardware time stamp inputs (HW1/4\_TS\_PUSH) that can cause hardware time stamp push events to be loaded into the Event FIFO. Each hardware time stamp input is internally connected to the PORTIMERPWM output of each timer as shown in Figure 4.

TIMER4 CPTS TIMER5 TIMER6 TIMER7 portimerpwm portimerpwm portimerpwm portimerpwm hw1\_ts\_push hw2\_ts\_push hw3\_ts\_push hw4\_ts\_push piclktimer piclktimer piclktimer piclktimer TIMER\_CLKSRC

**Figure 14-12. HW1/4\_TSP\_PUSH Connection**

The event is loaded into the event FIFO on the rising edge of the timer, and the PORT\_NUMBER field in the EVENT\_HIGH register indicates the hardware time stamp input that caused the event.

Each hardware time stamp input must be asserted for at least 10 periods of the selected RCLK clock. Each input can be enabled or disabled by setting the respective bits in the *CONTROL* register.

Hardware time stamps are intended to be an extremely low frequency signals, such that the event FIFO does not overrun. Software must keep up with the event FIFO and ensure that there is no overrun, or events will be lost.

#### *14.3.7.2.4.5 Ethernet Port Events*

#### *14.3.7.2.4.5.1 Ethernet Port Receive Event*

Each ethernet port can generate a receive ethernet event. Receive ethernet events are generated for valid received time sync packets. There are two CPTS interfaces for each ethernet receive port. The first is the Px\_TS\_RX\_MII interface and the second is the Px\_TS\_RX\_DEC interface. Information from these interfaces is used to generates an ethernet receive event for each ethernet time sync packet received on the associated port.

The Px\_TS\_RX\_MII interface issues a record signal (pX\_ts\_rx\_mii\_rec) along with a handle (pX\_ts\_rx\_mii\_hndl) for each packet (every packet) that is received on the associated ethernet port. The record signal is a single clock pulse indicating that a receive packet has been detected at the associated port MII interface. The handle value is incremented with each packet and rolls over to zero after 15.

There are 16 possible handle values so there can be a maximum of 16 packets "in flight" from the TS\_RX\_MII to the TS\_RX\_DEC block at any given time. A handle value is reused (not incremented) for any received packet that is shorter than about 31 octets (including preamble). Handle reuse on short packets prevents any possible overrun condition (more than 16 "in flight" packets) if multiple fragments are consecutively received.

Valid receive ethernet time sync events are signaled to the CPTS via the Px\_TS\_RX\_DEC interface. When the pX\_ts\_rx\_dec\_evnt is asserted, a valid event is detected and will be loaded into the event FIFO. Only valid receive time sync packets are indicated on the Px\_TS\_RX\_DEC interface. The pX\_ts\_rx\_dec\_hndl, pX\_ts\_rx\_dec\_msg\_type, and pX\_ts\_rx\_dec\_seq\_id signals are registered on an asserted pX\_ts\_rx\_dec\_evnt. When a Tx\_TS\_RX\_DEC event is asserted, the handle value is used to retrieve the time stamp that was loaded with the same handle value from the Px\_TS\_RX\_MII interface.

#### *14.3.7.2.4.5.2 Ethernet Port Transmit Event*

Each ethernet port can generate a transmit ethernet event. Transmit ethernet events are generated for valid transmitted time sync packets. There are two CPTS interfaces for each ethernet transmit port. The first is the Px\_TS\_TX\_DEC interface and the second is the Px\_TS\_TX\_MII interface. Information from these interfaces is used to generates an ethernet transmit event for each ethernet time sync packet transmitted on the associated port.

Valid ethernet transmit time sync events are signaled to the CPTS via the Px\_TS\_TX\_DEC interface. When the pX\_ts\_tx\_dec\_evnt signal is asserted, a valid time sync event has been detected and will be loaded into the event FIFO. Only valid transmit time sync packets are indicated on the Px\_TS\_RX\_DEC interface. The pX\_ts\_tx\_dec\_hndl, pX\_ts\_tx\_dec\_msg\_type, pX\_ts\_tx\_dec\_seq\_id signals are registered on an asserted pX\_ts\_tx\_dec\_evnt.

The time stamp for the event will be generated and signaled from the Px\_TS\_TX\_MII interface when the packet is actually transmitted. The event will be loaded into the event FIFO when the time stamp is recorded as controlled by the Px\_TS\_TX\_MII interface. The handle value is incremented with each time sync event packet and rolls over to zero after 7. There are 8 possible handle values so there can be a maximum of 8 time sync event packets "in flight" from the TS\_TX\_DEC to the TS\_TX\_MII block at any given time. The handle value increments only on time sync event packets.

The Px\_TS\_TX\_MII interface issues a single clock record signal (pX\_ts\_tx\_mii\_rec) at the beginning of each transmitted packet. If the packet is a time sync event packet then a single clock event signal (pX\_ts\_tx\_mii\_evnt) along with a handle (pX\_ts\_rx\_mii\_hndl) will be issued before the next record signal for the next transmitted packet. The event signal will not be issued for packets that were not indicated as valid time sync event packets on the Px\_TS\_TX\_DEC interface. If consecutive record indications occur without an interleaving event indication, then the packet associated with the first record was not a time sync event packet. The record signal is a single clock pulse indicating that a transmit packet egress has been detected at the associated port MII interface.

**Table 14-20. Values of messageType field**

| Message Type          | Value (hex) |
|-----------------------|-------------|
| Sync                  | 0           |
| Delay_Req             | 1           |
| Pdelay_Req            | 2           |
| Pdelay_Resp           | 3           |
| Reserved              | 4-7         |
| Follow_Up             | 8           |
| Delay_Resp            | 9           |
| Pdelay_Resp_Follow_Up | A           |
| Announce              | B           |
| Signaling             | C           |
| Management            | D           |
| Reserved              | E-F         |

#### 14.3.7.3 Interrupt Handling

When an event is push onto the Event FIFO, an interrupt can be generated to indicate to software that a time sync event occurred. The following steps should be taken to process time sync events using interrupts:

- Enable the TS PEND interrupt by setting the TS PEND EN bit of the CPTS INT ENABLE register.
- Upon interrupt, read the CPTS\_EVENT\_LOW and CPTS\_EVENT\_HIGH register values.
- Set the EVENT\_POP field (bit zero) of the CPTS\_EVENT\_POP register to pop the previously read
  value off of the event FIFO.
- Process the interrupt as required by the application software

Software has the option of processing more than a single event from the event FIFO in the interrupt service routine in the following way:

- 1. Enable the TS\_PEND interrupt by setting the TS\_PEND\_EN bit of the CPTS\_INT\_ENABLE register.
- 2. Upon interrupt enter the CPTS service routine.
- 3. Read the CPTS EVENT LOW and CPTS EVENT HIGH register values.
- 4. Set the EVENT\_POP bit of the CPTS\_EVENT\_POP register to pop the previously read value off of the event FIFO.
- 5. Wait for an amount of time greater than eight CPTS\_RFT\_CLK periods
- 6. Read the ts\_pend\_raw bit in the CPTS\_INTSTAT\_RAW register to determine if another valid event is in the event FIFO. If it is asserted then goto step 3. Otherwise goto step 7.
- 7. Process the interrupt(s) as required by the application software

Software also has the option of disabling the interrupt and polling the ts\_pend\_raw bit of the CPTS\_INTSTAT\_RAW register to determine if a valid event is on the event FIFO.

### 14.3.8 MDIO

The MII Management I/F module implements the 802.3 serial management interface to interrogate and control two Ethernet PHYs simultaneously using a shared two-wire bus. Two user access registers to control and monitor up to two PHYs simultaneously.

#### 14.3.8.1 MII Management Interface Frame Formats

The following tables show the read and write format of the 32-bit MII Management interface frames, respectively.

#### Table 14-21, MDIO Read Frame Format

| Preamble       | Start<br>Delimiter | Operation Code | PHY Address | Register Address | Turnaround | Data                    |
|----------------|--------------------|----------------|-------------|------------------|------------|-------------------------|
| 0xFFFF<br>FFFF | 01                 | 10             | AAAAA       | RRRRR            | Z0         | DDDD.DDDD.<br>DDDD.DDDD |

#### **Table 14-22. MDIO Write Frame Format**

| Preamble       | Start<br>Delimiter | Operation Code | PHY Address | Register Address | Turnaround | Data                    |
|----------------|--------------------|----------------|-------------|------------------|------------|-------------------------|
| 0xFFFF<br>FFFF | 01                 | 01             | AAAAA       | RRRRR            | 10         | DDDD.DDDD.<br>DDDD.DDDD |

The default or idle state of the two wire serial interface is a logic one. All tri-state drivers should be disabled and the PHY's pull-up resistor will pull the **MDIO\_DATA** line to a logic one. Prior to initiating any other transaction, the station management entity shall send a preamble sequence of 32 contiguous logic one bits on the **MDIO\_DATA** line with 32 corresponding cycles on **MDIO\_CLK** to provide the PHY with a pattern that it can use to establish synchronization. A PHY shall observe a sequence of 32 contiguous logic one bits on **MDIO\_DATA** with 32 corresponding **MDIO\_CLK** cycles before it responds to any other transaction.

### **Preamble**

The start of a frame is indicated by a preamble, which consists of a sequence of 32 contiguous bits all of which are a "1". This sequence provides the PHY a pattern to use to establish synchronization.

### **Start Delimiter**

The preamble is followed by the start delimiter which is indicated by a "01" pattern. The pattern assures transitions from the default logic one state to zero and back to one.

### **Operation Code**

The operation code for a read is "10", while the operation code for a write is a "01".

#### **PHY Address**

The PHY address is 5 bits allowing 32 unique values. The first bit transmitted is the MSB of the PHY address.

## **Register Address**

The Register address is 5 bits allowing 32 registers to be addressed within each PHY. Refer to the 10/100 PHY address map for addresses of individual registers.

## **Turnaround**

An idle bit time during which no device actively drives the MDIO\_DATA signal shall be inserted between the register address field and the data field of a read frame in order to avoid contention. During a read frame, the PHY shall drive a zero bit onto MDIO\_DATA for the first bit time following the idle bit and preceding the Data field. During a write frame, this field shall consist of a one bit followed by a zero bit.

### **Data**

The Data field is 16 bits. The first bit transmitted and received is the MSB of the data word.

#### **14.3.8.2 Functional Description**

The MII Management I/F will remain idle until enabled by setting the enable bit in the MDIOControl register. The MII Management I/F will then continuously poll the link status from within the Generic Status Register of all possible 32 PHY addresses in turn recording the results in the MDIO link register.

The linksel bit in the MDIOUserPhySel register determines the status input that is used. A change in the link status of the two PHYs being monitored will set the appropriate bit in the MDIOLinkIntRaw register and the MDIOLinkIntMasked register, if enabled by the linkint\_enable bit in the MDIOUserPhySel register.

The MDIO Alive register is updated by the MII Management I/F module if the PHY acknowledged the read of the generic status register. In addition, any PHY register read transactions initiated by the host also cause the MDIOAlive register to be updated.

At any time, the host can define a transaction for the MII Management interface module to undertake using the data, phyadr, regadr, and write fields in a MDIOUserAccess register. When the host sets the go bit in this register, the MII Management interface module will begin the transaction without any further intervention from the host. Upon completion, the MII Management interface will clear the go bit and set the userintraw bit in the MDIOUserIntRaw register corresponding to the MDIOUserAccess register being used.

The corresponding bit in the MDIOUserIntMasked register may also be set depending on the mask setting in the MDIOUserIntMaskSet and MDIOUserIntMaskClr registers. A round-robin arbitration scheme is used to schedule transactions which may queued by the host in different MDIOUserAccess registers. The host should check the status of the go bit in the MDIOUserAccess register before initiating a new transaction to ensure that the previous transaction has completed. The host can use the ack bit in the MDIOUserAccess register to determine the status of a read transaction.

It is necessary for software to use the MII Management interface module to setup the auto-negotiation parameters of each PHY attached to a MAC port, retrieve the negotiation results, and setup the MACControl register in the corresponding MAC.

## **14.4 Software Operation**

### *14.4.1 Transmit Operation*

After reset the host must write zeroes to all Tx DMA State head descriptor pointers. The Tx port may then be enabled. To initiate packet transmission the host constructs transmit queues in memory (one or more packets for transmission) and then writes the appropriate Tx DMA state head descriptor pointers. For each buffer added to a transmit queue, the host must initialize the Tx buffer descriptor values as follows:

- 1. Write the Next Descriptor Pointer with the 32-bit aligned address of the next descriptor in the queue (zero if last descriptor).
- 2. Write the Buffer Pointer with the byte aligned address of the buffer data.
- 3. Write the Buffer Length with the number of bytes in the buffer.
- 4. Write the Buffer Offset with the number of bytes in the offset to the data (nonzero with SOP only).
- 5. Set the SOP, EOP, and Ownership bits as appropriate.
- 6. Clear the End Of Queue bit.

The port begins Tx packet transmission on a given channel when the host writes the channel's Tx queue head descriptor pointer with the address of the first buffer descriptor in the queue (nonzero value). Each channel may have one or more queues, so each channel may have one or more head descriptor pointers. The first buffer descriptor for each Tx packet must have the Start of Packet (SOP) bit and the Ownership bit set to one by the host. The last buffer descriptor for each Tx packet must have the End of Packet (EOP) bit set to one by the host.

The port will transmit packets until all queued packets have been transmitted and the queue(s) are empty. When each packet transmission is complete, the port will clear the Ownership bit in the packet's SOP buffer descriptor and issue an interrupt to the host by writing the packet's last buffer descriptor address to the queue's Tx DMA State Completion Pointer. The interrupt is generated by the write, regardless of the value written.

When the last packet in a queue has been transmitted, the port sets the End Of Queue bit in the EOP buffer descriptor, clears the Ownership bit in the SOP Descriptor, zeroes the appropriate DMA state head descriptor pointer, and then issues a Tx interrupt to the host by writing to the queue's associated Tx completion pointer (address of the last buffer descriptor processed by the port). The port issues a maskable level interrupt (which may then be routed through external interrupt control logic to the host).

On interrupt from the port, the host processes the buffer queue, detecting transmitted packets by the status of the Ownership bit in the SOP buffer descriptor. If the Ownership bit is cleared to zero, then the packet has been transmitted and the host may reclaim the buffers associated with the packet. The host continues queue processing until the end of the queue or until a SOP buffer descriptor is read that contains a set Ownership bit indicating that the packet transmission is not complete.

The host determines that all packets in the queue have been transmitted when the last packet in the queue has a cleared Ownership bit in the SOP buffer descriptor, the End of Queue bit is set in the last packet EOP buffer descriptor, and the Next Descriptor Pointer of the last packet EOP buffer descriptor is zero. The host acknowledges an interrupt by writing the address of the last buffer descriptor to the queue's associated Tx Completion Pointer in the Tx DMA State.

If the host written buffer address value is different from the buffer address written by the port, then the level interrupt remains asserted. If the host written buffer address value is equal to the port written value, then the level interrupt is deasserted. The port write to the completion pointer actually stores the value in the state register (ram). The host written value is actually not written to the register location. The host written value is compared to the register contents (which was written by the port) and if the two values are equal, the interrupt is removed, otherwise the interrupt remains asserted. The host may process multiple packets previous to acknowledging an interrupt, or the host may acknowledge interrupts for every packet.

A misqueued packet condition may occur when the host adds a packet to a queue for transmission as the port finishes transmitting the previous last packet in the queue. The misqueued packet is detected by the host when queue processing detects a cleared Ownership bit in the SOP buffer descriptor, a set End of Queue bit in the EOP buffer descriptor, and a nonzero Next Descriptor Pointer in the EOP buffer descriptor.

A misqueued packet means that the port read the last EOP buffer descriptor before the host added the new last packet to the queue, so the port determined queue empty just before the last packet was added. The host corrects the misqueued packet condition by initiating a new packet transfer for the misqueued packet by writing the misqueued packet's SOP buffer descriptor address to the appropriate DMA State Tx Queue head Descriptor Pointer.

The host may add packets to the tail end of an active Tx queue at any time by writing the Next Descriptor Pointer to the current last descriptor in the queue. If a Tx queue is empty (inactive), the host may initiate packet transmission at any time by writing the appropriate Tx DMA State head descriptor pointer.

The host software should always check for and reinitiate transmission for misqueued packets during queue processing on interrupt from the port. In order to preclude software underrun, the host should avoid adding buffers to an active queue for any Tx packet that is not complete and ready for transmission.

The port determines that a packet is the last packet in the queue by detecting the End of Packet bit set with a zero Next Descriptor Pointer in the packet buffer descriptor. If the End of Packet bit is set and the Next Descriptor Pointer is nonzero, then the queue still contains one or more packets to be transmitted. If the EOP bit is set with a zero Next Descriptor Pointer, then the port will set the EOQ bit in the packet's EOP buffer descriptor and then zero the appropriate head descriptor pointer previous to interrupting the port (by writing the completion pointer) when the packet transmission is complete.

```
                              Host Memory
                     (Descriptors and Buffers)

               ┌───────────────────┐       ┌──────────┐
   ───────────▶│   SOP Descriptor  │──────▶│  Buffer  │
               └─────────┬─────────┘       └──────────┘
                         │
                         ▼
               ┌───────────────────┐       ┌──────────┐
               │    Descriptor     │──────▶│  Buffer  │
               └─────────┬─────────┘       └──────────┘
                         │
                         ▼
               ┌───────────────────┐       ┌──────────┐
               │   EOP Descriptor  │──────▶│  Buffer  │
               └─────────┬─────────┘       └──────────┘
                         │
                         ▼
               ┌───────────────────┐       ┌──────────┐
               │   SOP Descriptor  │──────▶│  Buffer  │
               └─────────┬─────────┘       └──────────┘
                         │
                         ▼
               ┌───────────────────┐       ┌──────────┐
               │   EOP Descriptor  │──────▶│  Buffer  │
               └─────────┬─────────┘       └──────────┘
                         │
                         ▼
   ---------------------------------------------------------------
                       Port Tx State RAM Entry

               ┌────────────────────────────────────────────┐
               │      Tx Queue Head Descriptor Pointer      │
               └────────────────────────────────────────────┘
```

**Figure 14-13. Port TX State RAM Entry**

### *14.4.2 Receive Operation*

Descriptor Descriptor Descriptor Descriptor Buffer Buffer Buffer Buffer **Figure 14-14. Port RX DMA State**

Buffer

Host Memory

Port Rx DMA State

After reset the host must write zeroes to all Rx DMA State head descriptor pointers. The Rx port may then be enabled. To initiate packet reception, the host constructs receive queues in memory and then writes the appropriate Rx DMA state head descriptor pointer. For each Rx buffer descriptor added to the queue, the host must initialize the Rx buffer descriptor values as follows:

• Write the Next Descriptor Pointer with the 32-bit aligned address of the next descriptor in the queue (zero if last descriptor)

Rx Queue Head Descriptor Pointer

• Write the Buffer Pointer with the byte aligned address of the buffer data

Descriptor

- Clear the Offset field
- Write the Buffer Length with the number of bytes in the buffer
- Clear the SOP, EOP, and EOQ bits
- Set the Ownership bit

The host enables packet reception on a given channel by writing the address of the first buffer descriptor in the queue (nonzero value) to the channel's head descriptor pointer in the channel's Rx DMA state. When packet reception begins on a given channel, the port fills each Rx buffer with data in order starting with the first buffer and proceeding through the Rx queue. If the Buffer Offset in the Rx DMA State is nonzero, then the port will begin writing data after the offset number of bytes in the SOP buffer. The port performs the following operations at the end of each packet reception:

- Overwrite the buffer length in the packet's EOP buffer descriptor with the number of bytes actually received in the packet's last buffer. The host initialized value is the buffer size. The overwritten value will be less than or equal to the host initialized value.
- Set the EOP bit in the packet's EOP buffer descriptor.
- Set the EOQ bit in the packet's EOP buffer descriptor if the current packet is the last packet in the queue.
- Overwrite the packet's SOP buffer descriptor Buffer Offset with the Rx DMA state value (the host initialized the buffer descriptor Buffer Offset value to zero). All non SOP buffer descriptors must have a zero Buffer Offset initialized by the host.

• Overwrite the packet's SOP buffer descriptor buffer length with the number of valid data bytes in the buffer. If the buffer is filled up, the buffer length will be the buffer size minus buffer offset.

- Set the SOP bit in the packet's SOP buffer descriptor.
- Write the SOP buffer descriptor Packet Length field.
- Clear the Ownership bit in the packet's SOP buffer descriptor.
- Issue an Rx host interrupt by writing the address of the packet's last buffer descriptor to the queue's Rx DMA State Completion Pointer. The interrupt is generated by the write to the Rx DMA State Completion Pointer address location, regardless of the value written.

On interrupt the host processes the Rx buffer queue detecting received packets by the status of the Ownership bit in each packet's SOP buffer descriptor. If the Ownership bit is cleared then the packet has been completely received and is available to be processed by the host.

The host may continue Rx queue processing until the end of the queue or until a buffer descriptor is read that contains a set Ownership bit indicating that the next packet's reception is not complete. The host determines that the Rx queue is empty when the last packet in the queue has a cleared Ownership bit in the SOP buffer descriptor, a set End of Queue bit in the EOP buffer descriptor, and the Next Descriptor Pointer in the EOP buffer descriptor is zero.

A misqueued buffer may occur when the host adds buffers to a queue as the port finishes the reception of the previous last packet in the queue. The misqueued buffer is detected by the host when queue processing detects a cleared Ownership bit in the SOP buffer descriptor, a set End of Queue bit in the EOP buffer descriptor, and a nonzero Next Descriptor Pointer in the EOP buffer descriptor.

A misqueued buffer means that the port read the last EOP buffer descriptor before the host added buffer descriptor(s) to the queue, so the port determined queue empty just before the host added more buffer descriptor(s). In the transmit case, the packet transmission is delayed by the time required for the host to determine the condition and reinitiate the transaction, but the packet is not actually lost. In the receive case, receive overrun condition may occur in the misqueued buffer case.

If a new packet reception is begun during the time that the port has determined the end of queue condition, then the received packet will overrun (start of packet overrun). If the misqueued buffer occurs during the middle of a packet reception then middle of packet overrun may occur. If the misqueued buffer occurs after the last packet has completed, and is corrected before the next packet reception begins, then overrun will not occur. The host acts on the misqueued buffer condition by writing the added buffer descriptor address to the appropriate Rx DMA State Head Descriptor Pointer.

### *14.4.3 Initializing the MDIO Module*

The following steps are performed by the application software or device driver to initialize the MDIO device:

- 1. 1. Configure the PREAMBLE and CLKDIV bits in the MDIO control register (MDIOCONTROL).
- 2. Enable the MDIO module by setting the ENABLE bit in MDIOCONTROL.
- 3. The MDIO PHY alive status register (MDIOALIVE) can be read in polling fashion until a PHY connected to the system responded, and the MDIO PHY link status register (MDIOLINK) can determine whether this PHY already has a link.
- 4. Setup the appropriate PHY addresses in the MDIO user PHY select register (MDIOUSERPHYSEL*n*), and set the LINKINTENB bit to enable a link change event interrupt if desirable.
- If an interrupt on general MDIO register access is desired, set the corresponding bit in the MDIO user command complete interrupt mask set register (MDIOUSERINTMASKSET) to use the MDIO user access register (MDIOUSERACCESS*n*). Since only one PHY is used in this device, the application software can use one MDIOUSERACCESS*n* to trigger a completion interrupt; the other MDIOUSERACCESS*n* is not setup.

### *14.4.4 Writing Data to a PHY Register*

The MDIO module includes a user access register (MDIOUSERACCESS*n*) to directly access a specified PHY device.To write a PHY register, perform the following:

1. Ensure the GO bit in the MDIO user access register (MDIOUSERACCESS*n*) is cleared.

2. Write to the GO, WRITE, REGADR, PHYADR, and DATA bits in MDIOUSERACCESS*n* corresponding to the PHY and PHY register you want to write.

- 3. The write operation to the PHY is scheduled and completed by the MDIO module. Completion of the write operation can be determined by polling the GO bit in MDIOUSERACCESS*n* for a 0.
- 4. Completion of the operation sets the corresponding USERINTRAW bit (0 or 1) in the MDIO user command complete interrupt register (MDIOUSERINTRAW) corresponding to USERACCESS*n* used. If interrupts have been enabled on this bit using the MDIO user command complete interrupt mask set register (MDIOUSERINTMASKSET), then the bit is also set in the MDIO user command complete interrupt register (MDIOUSERINTMASKED) and an interrupt is triggered on the CPU.

### *14.4.5 Reading Data from a PHY Register*

The MDIO module includes a user access register (MDIOUSERACCESS*n*) to directly access a specified PHY device. To read a PHY register, perform the following:

- 1. Ensure the GO bit in the MDIO user access register (MDIOUSERACCESS*n*) is cleared.
- 2. Write to the GO, REGADR, and PHYADR bits in MDIOUSERACCESS*n* corresponding to the PHY and PHY register you want to read.
- 3. The read data value is available in the DATA bits in MDIOUSERACCESS*n* after the module completes the read operation on the serial bus. Completion of the read operation can be determined by polling the GO and ACK bits in MDIOUSERACCESS*n*. After the GO bit has cleared, the ACK bit is set on a successful read.
- 4. Completion of the operation sets the corresponding USERINTRAW bit (0 or 1) in the MDIO user command complete interrupt register (MDIOUSERINTRAW) corresponding to USERACCESS*n* used. If interrupts have been enabled on this bit using the MDIO user command complete interrupt mask set register (MDIOUSERINTMASKSET), then the bit is also set in the MDIO user command complete interrupt register (MDIOUSERINTMASKED) and an interrupt is triggered on the CPU.

### *14.4.6 Initialization and Configuration of CPSW*

To configure the 3PSW Ethernet Subsystem for operation the host must perform the following:

- Select the Interface (GMII/RGMII/MII) Mode in the Control Module.
- Configure pads (PIN muxing) as per the Interface Selected using the appropriate pin muxing conf\_xxx registers in the Control Module.
- Enable the 3PSW Ethernet Subsystem Clocks. See [Section](#page-5-0) 14.2.2 and Section 8.1 to enable the appropriate clocks.
- Configure the PRCM registers CM\_PER\_CPSW\_CLKSTCTRL and CM\_PER\_CPGMAC0\_CLKCTRL to enable power and clocks to the 3PSW Ethernet Subsystem. See Section 8.1 for register details.
- Apply soft reset to 3PSW Subsytem, CPSW\_3G, CPGMAC\_SL1/2, and CPDMA (see the soft reset registers in the following sections).
- Initialize the HDPs (Header Description Pointer) and CPs (Completion Pointer) to NULL
- Configure the Interrupts (see Chapter 6).
- Configure the CPSW\_3G Control register.
- Configure the Statistics Port Enable register
- Configure the ALE. (See Section [14.3.2.7](#page-31-0).)
- Configure the MDIO.
- Configure the CPDMA receive DMA controller.
- Configure the CPDMA transmit DMA controller.
- Configure the CPPI Tx and Rx Descriptors.
- Configure CPGMAC\_SL1 and CPGMAC\_SL2 as per the desired mode of operations.
- Start up RX and TX DMA (write to HDP of Rx and Tx).
- Wait for the completion of the transfer (HDP cleared to zero).

## **14.5 Ethernet Subsystem Registers**

### *14.5.1 CPSW\_ALE Registers*

Table [14-23](#page-78-1) lists the memory-mapped registers for the CPSW\_ALE. All register offset addresses not listed in Table [14-23](#page-78-1) should be considered as reserved locations and the register contents should not be modified.

**Table 14-23. CPSW\_ALE REGISTERS**

| Offset | Acronym      | Register Name                                    | Section           |
|--------|--------------|--------------------------------------------------|-------------------|
| 0h     | IDVER        | ADDRESS LOOKUP ENGINE ID/VERSION REGISTER        | Section 14.5.1.1  |
| 8h     | CONTROL      | ADDRESS LOOKUP ENGINE CONTROL REGISTER           | Section 14.5.1.2  |
| 10h    | PRESCALE     | ADDRESS LOOKUP ENGINE PRESCALE REGISTER          | Section 14.5.1.3  |
| 18h    | UNKNOWN_VLAN | ADDRESS LOOKUP ENGINE UNKNOWN VLAN<br>REGISTER   | Section 14.5.1.4  |
| 20h    | TBLCTL       | ADDRESS LOOKUP ENGINE TABLE CONTROL              | Section 14.5.1.5  |
| 34h    | TBLW2        | ADDRESS LOOKUP ENGINE TABLE WORD 2<br>REGISTER   | Section 14.5.1.6  |
| 38h    | TBLW1        | ADDRESS LOOKUP ENGINE TABLE WORD 1<br>REGISTER   | Section 14.5.1.7  |
| 3Ch    | TBLW0        | ADDRESS LOOKUP ENGINE TABLE WORD 0<br>REGISTER   | Section 14.5.1.8  |
| 40h    | PORTCTL0     | ADDRESS LOOKUP ENGINE PORT 0 CONTROL<br>REGISTER | Section 14.5.1.9  |
| 44h    | PORTCTL1     | ADDRESS LOOKUP ENGINE PORT 1 CONTROL<br>REGISTER | Section 14.5.1.10 |
| 48h    | PORTCTL2     | ADDRESS LOOKUP ENGINE PORT 2 CONTROL<br>REGISTER | Section 14.5.1.11 |
| 4Ch    | PORTCTL3     | ADDRESS LOOKUP ENGINE PORT 3 CONTROL<br>REGISTER | Section 14.5.1.12 |
| 50h    | PORTCTL4     | ADDRESS LOOKUP ENGINE PORT 4 CONTROL<br>REGISTER | Section 14.5.1.13 |
| 54h    | PORTCTL5     | ADDRESS LOOKUP ENGINE PORT 5 CONTROL<br>REGISTER | Section 14.5.1.14 |

#### **14.5.1.1 IDVER Register (offset = 0h) [reset = 290104h]**

IDVER is shown in [Figure](#page-79-1) 14-15 and described in Table [14-24.](#page-79-2)

ADDRESS LOOKUP ENGINE ID/VERSION REGISTER

## **Figure 14-15. IDVER Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           IDENT                                                  │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                            R-0

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           IDENT                                                  │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                            R-0

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                          MAJ_VER                                                 │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                            R-0

 7             6             5             4             3             2             1             0
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         MINOR_VER                                                │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                            R-0
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-24. IDVER Register Field Descriptions**

| Bit   | Field     | Type | Reset | Description              |
|-------|-----------|------|-------|--------------------------|
| 31-16 | IDENT     | R-0  | 0     | ALE Identification Value |
| 15-8  | MAJ_VER   | R-0  | 0     | ALE Major Version Value  |
| 7-0   | MINOR_VER | R-0  | 0     | ALE Minor Version Value  |

#### **14.5.1.2 CONTROL Register (offset = 8h) [reset = 0h]**

CONTROL is shown in [Figure](#page-80-1) 14-16 and described in Table [14-25.](#page-80-2)

ADDRESS LOOKUP ENGINE CONTROL REGISTER

#### **Figure 14-16. CONTROL Register**

| 31               | 30               | 29                  | 28       | 27                | 26         | 25                   | 24                    |
|------------------|------------------|---------------------|----------|-------------------|------------|----------------------|-----------------------|
| ENABLE_ALE       | CLEAR_TABLE      | AGE_OUT_NO<br>W     |          |                   | Reserved   |                      |                       |
| R/W-0            | R/W-0            | R/W-0               |          |                   |            |                      |                       |
| 23               | 22               | 21                  | 20       | 19                | 18         | 17                   | 16                    |
|                  |                  |                     |          | Reserved          |            |                      |                       |
| 15               | 14               | 13                  | 12       | 11                | 10         | 9                    | 8                     |
|                  |                  |                     | Reserved |                   |            |                      | EN_P0_UNI_F<br>LOOD   |
|                  |                  |                     |          |                   |            |                      | R/W-0                 |
| 7                | 6                | 5                   | 4        | 3                 | 2          | 1                    | 0                     |
| LEARN_NO_VI<br>D | EN_VID0_MOD<br>E | ENABLE_OUI_<br>DENY | BYPASS   | RATE_LIMIT_T<br>X | VLAN_AWARE | ENABLE_AUT<br>H_MODE | ENABLE_RAT<br>E_LIMIT |
| R/W-0            | R/W-0            | R/W-0               | R/W-0    | R/W-0             | R/W-0      | R/W-0                | R/W-0                 |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-25. CONTROL Register Field Descriptions**

| Bit | Field           | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|-----|-----------------|-------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31  | ENABLE_ALE      | R/W-0 | 0     | Enable ALE -<br>0 - Drop all packets<br>1 - Enable ALE packet processing                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| 30  | CLEAR_TABLE     | R/W-0 | 0     | Clear ALE address table - Setting this bit causes the ALE hardware<br>to write all table bit values to zero.<br>Software must perform a clear table operation as part of the ALE<br>setup/configuration process.<br>Setting this bit causes all ALE accesses to be held up for 64 clocks<br>while the clear is performed.<br>Access to all ALE registers will be blocked (wait states) until the 64<br>clocks have completed.<br>This bit cannot be read as one because the read is blocked until the<br>clear table is completed at which time this bit is cleared to zero. |
| 29  | AGE_OUT_NOW     | R/W-0 | 0     | Age Out Address Table Now - Setting this bit causes the ALE<br>hardware to remove (free up) any ageable table entry that does not<br>have a set touch bit.<br>This bit is cleared when the age out process has completed.<br>This bit may be read.<br>The age out process takes 4096 clocks best case (no ale packet<br>processing during ageout) and 66550 clocks absolute worst case.                                                                                                                                                                                      |
| 8   | EN_P0_UNI_FLOOD | R/W-0 | 0     | Enable Port 0 (Host Port) unicast flood<br>0 - do not flood unknown unicast packets to host port (p0)<br>1 - flood unknown unicast packets to host port (p0)                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 7   | LEARN_NO_VID    | R/W-0 | 0     | Learn No VID -<br>0 - VID is learned with the source address<br>1 - VID is not learned with the source address (source address is not<br>tied to VID).                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 6   | EN_VID0_MODE    | R/W-0 | 0     | Enable VLAN ID = 0 Mode<br>0 - Process the packet with VID = PORT_VLAN[<br>11:0].<br>1 - Process the packet with VID = 0.                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 5   | ENABLE_OUI_DENY | R/W-0 | 0     | Enable OUI Deny Mode - When set this bit indicates that a packet<br>with a non OUI table entry matching source address will be dropped<br>to the host unless the destination address matches a multicast table<br>entry with the super bit set.                                                                                                                                                                                                                                                                                                                              |

## **Table 14-25. CONTROL Register Field Descriptions (continued)**

| Bit | Field             | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-----|-------------------|-------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 4   | BYPASS            | R/W-0 | 0     | ALE Bypass - When set, all packets received on ports 0 and 1 are<br>sent to the host (only to the host).                                                                                                                                                                                                                                                                                                                                          |
| 3   | RATE_LIMIT_TX     | R/W-0 | 0     | Rate Limit Transmit mode -<br>0 - Broadcast and multicast rate limit counters are received port<br>based<br>1 - Broadcast and multicast rate limit counters are transmit port<br>based.                                                                                                                                                                                                                                                           |
| 2   | VLAN_AWARE        | R/W-0 | 0     | ALE VLAN Aware - Determines what is done if VLAN not found.<br>0 - Flood if VLAN not found<br>1 - Drop packet if VLAN not found                                                                                                                                                                                                                                                                                                                   |
| 1   | ENABLE_AUTH_MODE  | R/W-0 | 0     | Enable MAC Authorization Mode - Mac authorization mode requires<br>that all table entries be made by the host software.<br>There are no learned address in authorization mode and the packet<br>will be dropped if the source address is not found (and the<br>destination address is not a multicast address with the super table<br>entry bit set).<br>0 - The ALE is not in MAC authorization mode<br>1 - The ALE is in MAC authorization mode |
| 0   | ENABLE_RATE_LIMIT | R/W-0 | 0     | Enable Broadcast and Multicast Rate Limit<br>0 - Broadcast/Multicast rates not limited<br>1 - Broadcast/Multicast packet reception limited to the port control<br>register rate limit fields.                                                                                                                                                                                                                                                     |

#### **14.5.1.3 PRESCALE Register (offset = 10h) [reset = 0h]**

PRESCALE is shown in [Figure](#page-82-1) 14-17 and described in Table [14-26](#page-82-2).

ADDRESS LOOKUP ENGINE PRESCALE REGISTER

#### **Figure 14-17. PRESCALE Register**

```
31 30 29 28 27 26 25 24 23 22 21 20   19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
┌────────────────────────────────────┬─────────────────────────────────────────────────────────────┐
│               Reserved             │                        PRESCALE                             │
└────────────────────────────────────┴─────────────────────────────────────────────────────────────┘
```

R/W-0

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-26. PRESCALE Register Field Descriptions**

| Bit  | Field    | Type  | Reset | Description                                                                                                                                                                                                   |
|------|----------|-------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 19-0 | PRESCALE | R/W-0 | 0     | ALE Prescale Register - The input clock is divided by this value for<br>use in the multicast/broadcast rate limiters.<br>The minimum operating value is 0x10.<br>The prescaler is off when the value is zero. |

#### **14.5.1.4 UNKNOWN\_VLAN Register (offset = 18h) [reset = 0h]**

UNKNOWN\_VLAN is shown in [Figure](#page-83-1) 14-18 and described in Table [14-27.](#page-83-2)

ADDRESS LOOKUP ENGINE UNKNOWN VLAN REGISTER

## **Figure 14-18. UNKNOWN\_VLAN Register**

| 31 | 30       | 29 | 28                       | 27 | 26                            | 25 | 24 |  |  |
|----|----------|----|--------------------------|----|-------------------------------|----|----|--|--|
|    | Reserved |    |                          |    | UNKNOWN_FORCE_UNTAGGED_EGRESS |    |    |  |  |
|    |          |    |                          |    | R/W-0                         |    |    |  |  |
| 23 | 22       | 21 | 20                       | 19 | 18                            | 17 | 16 |  |  |
|    | Reserved |    |                          |    | UNKNOWN_REG_MCAST_FLOOD_MASK  |    |    |  |  |
|    |          |    |                          |    | R/W-0                         |    |    |  |  |
| 15 | 14       | 13 | 12                       | 11 | 10                            | 9  | 8  |  |  |
|    | Reserved |    |                          |    | UNKNOWN_MCAST_FLOOD_MASK      |    |    |  |  |
|    |          |    | R/W-0                    |    |                               |    |    |  |  |
| 7  | 6        | 5  | 4                        | 3  | 2                             | 1  | 0  |  |  |
|    | Reserved |    | UNKNOWN_VLAN_MEMBER_LIST |    |                               |    |    |  |  |
|    |          |    |                          |    |                               |    |    |  |  |

R/W-0

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-27. UNKNOWN\_VLAN Register Field Descriptions**

| Bit   | Field                             | Type  | Reset | Description                                  |
|-------|-----------------------------------|-------|-------|----------------------------------------------|
| 29-24 | UNKNOWN_FORCE_UN<br>TAGGED_EGRESS | R/W-0 | 0     | Unknown VLAN Force Untagged Egress.          |
| 21-16 | UNKNOWN_REG_MCAS<br>T_FLOOD_MASK  | R/W-0 | 0     | Unknown VLAN Registered Multicast Flood Mask |
| 13-8  | UNKNOWN_MCAST_FLO<br>OD_MASK      | R/W-0 | 0     | Unknown VLAN Multicast Flood Mask            |
| 5-0   | UNKNOWN_VLAN_MEM<br>BER_LIST      | R/W-0 | 0     | Unknown VLAN Member List                     |

#### **14.5.1.5 TBLCTL Register (offset = 20h) [reset = 0h]**

TBLCTL is shown in [Figure](#page-84-1) 14-19 and described in Table [14-28](#page-84-2).

ADDRESS LOOKUP ENGINE TABLE CONTROL

## **Figure 14-19. TBLCTL Register**

| 31        | 30       | 29       | 28       | 27            | 26 | 25 | 24            |  |  |
|-----------|----------|----------|----------|---------------|----|----|---------------|--|--|
| WRITE_RDZ |          | Reserved |          |               |    |    |               |  |  |
| R/W-0     |          |          |          |               |    |    |               |  |  |
| 23        | 22       | 21       | 20       | 19            | 18 | 17 | 16            |  |  |
|           | Reserved |          |          |               |    |    |               |  |  |
| 15        | 14       | 13       | 12       | 11            | 10 | 9  | 8             |  |  |
|           |          |          | Reserved |               |    |    | ENTRY_POINTER |  |  |
|           |          |          |          |               |    |    | R/W-0         |  |  |
| 7         | 6        | 5        | 4        | 3             | 2  | 1  | 0             |  |  |
|           |          |          |          | ENTRY_POINTER |    |    |               |  |  |
|           | R/W-0    |          |          |               |    |    |               |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 14-28. TBLCTL Register Field Descriptions**

| Bit | Field         | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|-----|---------------|-------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31  | WRITE_RDZ     | R/W-0 | 0     | Write Bit - This bit is always read as zero.<br>Writing a 1 to this bit causes the three table word register values to<br>be written to the entry_pointer location in the address table.<br>Writing a 0 to this bit causes the three table word register values to<br>be loaded from the entry_pointer location in the address table so that<br>they may be subsequently read.<br>A read of any ALE address location will be stalled until the read or<br>write has completed. |
| 9-0 | ENTRY_POINTER | R/W-0 | 0     | Table Entry Pointer - The entry_pointer contains the table entry<br>value that will be read/written with accesses to the table word<br>registers.                                                                                                                                                                                                                                                                                                                              |

#### **14.5.1.6 TBLW2 Register (offset = 34h) [reset = 0h]**

TBLW2 is shown in [Figure](#page-85-1) 14-20 and described in Table [14-29](#page-85-2).

ADDRESS LOOKUP ENGINE TABLE WORD 2 REGISTER

## **Figure 14-20. TBLW2 Register**

```
31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8   7 6 5 4 3 2 1 0
┌───────────────────────────────────────────────────────────────────────┬───────────────┐
│                               Reserved                                │  ENTRY71_64   │
└───────────────────────────────────────────────────────────────────────┴───────────────┘
                                                                        R/W-0
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-29. TBLW2 Register Field Descriptions**

| Bit | Field      | Type  | Reset | Description               |
|-----|------------|-------|-------|---------------------------|
| 7-0 | ENTRY71-64 | R/W-0 | 0     | Table entry bits<br>71:64 |

#### **14.5.1.7 TBLW1 Register (offset = 38h) [reset = 0h]**

TBLW1 is shown in [Figure](#page-86-1) 14-21 and described in Table [14-30](#page-86-2).

ADDRESS LOOKUP ENGINE TABLE WORD 1 REGISTER

#### **Figure 14-21. TBLW1 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

ENTRY63\_32

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-30. TBLW1 Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description               |
|------|------------|------|-------|---------------------------|
| 31-0 | ENTRY63_32 | R/W  | 0h    | Table entry bits<br>63:32 |

#### **14.5.1.8 TBLW0 Register (offset = 3Ch) [reset = 0h]**

TBLW0 is shown in [Figure](#page-87-1) 14-22 and described in Table [14-31](#page-87-2).

ADDRESS LOOKUP ENGINE TABLE WORD 0 REGISTER

## **Figure 14-22. TBLW0 Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

ENTRY31\_0

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-31. TBLW0 Register Field Descriptions**

| Bit  | Field     | Type | Reset | Description              |
|------|-----------|------|-------|--------------------------|
| 31-0 | ENTRY31_0 | R/W  | 0h    | Table entry bits<br>31:0 |

#### **14.5.1.9 PORTCTL0 Register (offset = 40h) [reset = 0h]**

PORTCTL0 is shown in [Figure](#page-88-1) 14-23 and described in Table [14-32](#page-88-2).

ADDRESS LOOKUP ENGINE PORT 0 CONTROL REGISTER

## **Figure 14-23. PORTCTL0 Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         BCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         MCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘

 7             6             5             4             3             2             1             0
┌──────────────┬──────────────┬────────────────┬──────────────────┬────────────────┬──────────────┐
│  Reserved    │ NO_SA_UPDATE │   NO_LEARN     │ VID_INGRESS_CHECK│ DROP_UNTAGGED  │  PORT_STATE  │
└──────────────┴──────────────┴────────────────┴──────────────────┴────────────────┴──────────────┘
                R/W-0           R/W-0              R/W-0              R/W-0            R/W-0
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-32. PORTCTL0 Register Field Descriptions**

| Bit   | Field             | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                     |
|-------|-------------------|-------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | BCAST_LIMIT       | R/W-0 | 0     | Broadcast Packet Rate Limit - Each prescale pulse loads this field<br>into the port broadcast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Broadcast rate limiting is enabled by a non-zero value in this field  |
| 23-16 | MCAST_LIMIT       | R/W-0 | 0     | Multicast Packet Rate Limit - Each prescale pulse loads this field into<br>the port multicast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Multicast rate limiting is enabled by a non-zero value in this field. |
| 5     | NO_SA_UPDATE      | R/W-0 | 0     | No Souce Address Update - When set the port is disabled from<br>updating the source port number in an ALE table entry.                                                                                                                                                                                                                                                                                                                          |
| 4     | NO_LEARN          | R/W-0 | 0     | No Learn Mode - When set the port is disabled from learning an<br>address.                                                                                                                                                                                                                                                                                                                                                                      |
| 3     | VID_INGRESS_CHECK | R/W-0 | 0     | VLAN ID Ingress Check - If VLAN not found then drop the packet.<br>Packets with an unknown (default) VLAN will be dropped.                                                                                                                                                                                                                                                                                                                      |
| 2     | DROP_UNTAGGED     | R/W-0 | 0     | Drop Untagged Packets - Drop non-VLAN tagged ingress packets.                                                                                                                                                                                                                                                                                                                                                                                   |
| 1-0   | PORT_STATE        | R/W-0 | 0     | Port State<br>0 - Disabled<br>1 - Blocked<br>2 - Learn<br>3 - Forward                                                                                                                                                                                                                                                                                                                                                                           |

#### **14.5.1.10 PORTCTL1 Register (offset = 44h) [reset = 0h]**

PORTCTL1 is shown in [Figure](#page-89-1) 14-24 and described in Table [14-33](#page-89-2).

ADDRESS LOOKUP ENGINE PORT 1 CONTROL REGISTER

## **Figure 14-24. PORTCTL1 Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         BCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         MCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘

 7             6             5             4             3             2             1             0
┌──────────────┬──────────────┬────────────────┬──────────────────┬────────────────┬──────────────┐
│  Reserved    │ NO_SA_UPDATE │   NO_LEARN     │ VID_INGRESS_CHECK│ DROP_UNTAGGED  │  PORT_STATE  │
└──────────────┴──────────────┴────────────────┴──────────────────┴────────────────┴──────────────┘
                R/W-0           R/W-0              R/W-0              R/W-0            R/W-0
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-33. PORTCTL1 Register Field Descriptions**

| Bit   | Field             | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                     |
|-------|-------------------|-------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | BCAST_LIMIT       | R/W-0 | 0     | Broadcast Packet Rate Limit - Each prescale pulse loads this field<br>into the port broadcast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Broadcast rate limiting is enabled by a non-zero value in this field  |
| 23-16 | MCAST_LIMIT       | R/W-0 | 0     | Multicast Packet Rate Limit - Each prescale pulse loads this field into<br>the port multicast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Multicast rate limiting is enabled by a non-zero value in this field. |
| 5     | NO_SA_UPDATE      | R/W-0 | 0     | No Souce Address Update - When set the port is disabled from<br>updating the source port number in an ALE table entry.                                                                                                                                                                                                                                                                                                                          |
| 4     | NO_LEARN          | R/W-0 | 0     | No Learn Mode - When set the port is disabled from learning an<br>address.                                                                                                                                                                                                                                                                                                                                                                      |
| 3     | VID_INGRESS_CHECK | R/W-0 | 0     | VLAN ID Ingress Check - If VLAN not found then drop the packet.<br>Packets with an unknown (default) VLAN will be dropped.                                                                                                                                                                                                                                                                                                                      |
| 2     | DROP_UNTAGGED     | R/W-0 | 0     | Drop Untagged Packets - Drop non-VLAN tagged ingress packets.                                                                                                                                                                                                                                                                                                                                                                                   |
| 1-0   | PORT_STATE        | R/W-0 | 0     | Port State<br>0 - Disabled<br>1 - Blocked<br>2 - Learn<br>3 - Forward                                                                                                                                                                                                                                                                                                                                                                           |

#### **14.5.1.11 PORTCTL2 Register (offset = 48h) [reset = 0h]**

PORTCTL2 is shown in [Figure](#page-90-1) 14-25 and described in Table [14-34](#page-90-2).

ADDRESS LOOKUP ENGINE PORT 2 CONTROL REGISTER

## **Figure 14-25. PORTCTL2 Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         BCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         MCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘

 7             6             5             4             3             2             1             0
┌──────────────┬──────────────┬────────────────┬──────────────────┬────────────────┬──────────────┐
│  Reserved    │ NO_SA_UPDATE │   NO_LEARN     │ VID_INGRESS_CHECK│ DROP_UNTAGGED  │  PORT_STATE  │
└──────────────┴──────────────┴────────────────┴──────────────────┴────────────────┴──────────────┘
                R/W-0           R/W-0              R/W-0              R/W-0            R/W-0
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-34. PORTCTL2 Register Field Descriptions**

| Bit   | Field             | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                     |  |  |  |
|-------|-------------------|-------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|--|
| 31-24 | BCAST_LIMIT       | R/W-0 | 0     | Broadcast Packet Rate Limit - Each prescale pulse loads this field<br>into the port broadcast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Broadcast rate limiting is enabled by a non-zero value in this field  |  |  |  |
| 23-16 | MCAST_LIMIT       | R/W-0 | 0     | Multicast Packet Rate Limit - Each prescale pulse loads this field into<br>the port multicast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Multicast rate limiting is enabled by a non-zero value in this field. |  |  |  |
| 5     | NO_SA_UPDATE      | R/W-0 | 0     | No Souce Address Update - When set the port is disabled from<br>updating the source port number in an ALE table entry.                                                                                                                                                                                                                                                                                                                          |  |  |  |
| 4     | NO_LEARN          | R/W-0 | 0     | No Learn Mode - When set the port is disabled from learning an<br>address.                                                                                                                                                                                                                                                                                                                                                                      |  |  |  |
| 3     | VID_INGRESS_CHECK | R/W-0 | 0     | VLAN ID Ingress Check - If VLAN not found then drop the packet.<br>Packets with an unknown (default) VLAN will be dropped.                                                                                                                                                                                                                                                                                                                      |  |  |  |
| 2     | DROP_UNTAGGED     | R/W-0 | 0     | Drop Untagged Packets - Drop non-VLAN tagged ingress packets.                                                                                                                                                                                                                                                                                                                                                                                   |  |  |  |
| 1-0   | PORT_STATE        | R/W-0 | 0     | Port State<br>0 - Disabled<br>1 - Blocked<br>2 - Learn<br>3 - Forward                                                                                                                                                                                                                                                                                                                                                                           |  |  |  |

#### **14.5.1.12 PORTCTL3 Register (offset = 4Ch) [reset = 0h]**

PORTCTL3 is shown in [Figure](#page-91-1) 14-26 and described in Table [14-35](#page-91-2).

ADDRESS LOOKUP ENGINE PORT 3 CONTROL REGISTER

### **Figure 14-26. PORTCTL3 Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         BCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         MCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘

 7             6             5             4             3             2             1             0
┌──────────────┬──────────────┬────────────────┬──────────────────┬────────────────┬──────────────┐
│  Reserved    │ NO_SA_UPDATE │   NO_LEARN     │ VID_INGRESS_CHECK│ DROP_UNTAGGED  │  PORT_STATE  │
└──────────────┴──────────────┴────────────────┴──────────────────┴────────────────┴──────────────┘
                R/W-0           R/W-0              R/W-0              R/W-0            R/W-0
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-35. PORTCTL3 Register Field Descriptions**

| Bit   | Field             | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                     |  |  |  |
|-------|-------------------|-------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|--|
| 31-24 | BCAST_LIMIT       | R/W-0 | 0     | Broadcast Packet Rate Limit - Each prescale pulse loads this field<br>into the port broadcast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Broadcast rate limiting is enabled by a non-zero value in this field  |  |  |  |
| 23-16 | MCAST_LIMIT       | R/W-0 | 0     | Multicast Packet Rate Limit - Each prescale pulse loads this field into<br>the port multicast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Multicast rate limiting is enabled by a non-zero value in this field. |  |  |  |
| 5     | NO_SA_UPDATE      | R/W-0 | 0     | No Souce Address Update - When set the port is disabled from<br>updating the source port number in an ALE table entry.                                                                                                                                                                                                                                                                                                                          |  |  |  |
| 4     | NO_LEARN          | R/W-0 | 0     | No Learn Mode - When set the port is disabled from learning an<br>address.                                                                                                                                                                                                                                                                                                                                                                      |  |  |  |
| 3     | VID_INGRESS_CHECK | R/W-0 | 0     | VLAN ID Ingress Check - If VLAN not found then drop the packet.<br>Packets with an unknown (default) VLAN will be dropped.                                                                                                                                                                                                                                                                                                                      |  |  |  |
| 2     | DROP_UNTAGGED     | R/W-0 | 0     | Drop Untagged Packets - Drop non-VLAN tagged ingress packets.                                                                                                                                                                                                                                                                                                                                                                                   |  |  |  |
| 1-0   | PORT_STATE        | R/W-0 | 0     | Port State<br>0 - Disabled<br>1 - Blocked<br>2 - Learn<br>3 - Forward                                                                                                                                                                                                                                                                                                                                                                           |  |  |  |

#### **14.5.1.13 PORTCTL4 Register (offset = 50h) [reset = 0h]**

PORTCTL4 is shown in [Figure](#page-92-1) 14-27 and described in Table [14-36](#page-92-2).

ADDRESS LOOKUP ENGINE PORT 4 CONTROL REGISTER

## **Figure 14-27. PORTCTL4 Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         BCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         MCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘

 7             6             5             4             3             2             1             0
┌──────────────┬──────────────┬────────────────┬──────────────────┬────────────────┬──────────────┐
│  Reserved    │ NO_SA_UPDATE │   NO_LEARN     │ VID_INGRESS_CHECK│ DROP_UNTAGGED  │  PORT_STATE  │
└──────────────┴──────────────┴────────────────┴──────────────────┴────────────────┴──────────────┘
                R/W-0           R/W-0              R/W-0              R/W-0            R/W-0
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-36. PORTCTL4 Register Field Descriptions**

| Bit   | Field             | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                     |  |  |  |
|-------|-------------------|-------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|--|
| 31-24 | BCAST_LIMIT       | R/W-0 | 0     | Broadcast Packet Rate Limit - Each prescale pulse loads this field<br>into the port broadcast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Broadcast rate limiting is enabled by a non-zero value in this field  |  |  |  |
| 23-16 | MCAST_LIMIT       | R/W-0 | 0     | Multicast Packet Rate Limit - Each prescale pulse loads this field into<br>the port multicast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Multicast rate limiting is enabled by a non-zero value in this field. |  |  |  |
| 5     | NO_SA_UPDATE      | R/W-0 | 0     | No Souce Address Update - When set the port is disabled from<br>updating the source port number in an ALE table entry.                                                                                                                                                                                                                                                                                                                          |  |  |  |
| 4     | NO_LEARN          | R/W-0 | 0     | No Learn Mode - When set the port is disabled from learning an<br>address.                                                                                                                                                                                                                                                                                                                                                                      |  |  |  |
| 3     | VID_INGRESS_CHECK | R/W-0 | 0     | VLAN ID Ingress Check - If VLAN not found then drop the packet.<br>Packets with an unknown (default) VLAN will be dropped.                                                                                                                                                                                                                                                                                                                      |  |  |  |
| 2     | DROP_UNTAGGED     | R/W-0 | 0     | Drop Untagged Packets - Drop non-VLAN tagged ingress packets.                                                                                                                                                                                                                                                                                                                                                                                   |  |  |  |
| 1-0   | PORT_STATE        | R/W-0 | 0     | Port State<br>0 - Disabled<br>1 - Blocked<br>2 - Learn<br>3 - Forward                                                                                                                                                                                                                                                                                                                                                                           |  |  |  |

#### **14.5.1.14 PORTCTL5 Register (offset = 54h) [reset = 0h]**

PORTCTL5 is shown in [Figure](#page-93-1) 14-28 and described in Table [14-37](#page-93-2).

ADDRESS LOOKUP ENGINE PORT 5 CONTROL REGISTER

## **Figure 14-28. PORTCTL5 Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         BCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         MCAST_LIMIT                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R/W-0

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘

 7             6             5             4             3             2             1             0
┌──────────────┬──────────────┬────────────────┬──────────────────┬────────────────┬──────────────┐
│  Reserved    │ NO_SA_UPDATE │   NO_LEARN     │ VID_INGRESS_CHECK│ DROP_UNTAGGED  │  PORT_STATE  │
└──────────────┴──────────────┴────────────────┴──────────────────┴────────────────┴──────────────┘
                R/W-0           R/W-0              R/W-0              R/W-0            R/W-0
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

## **Table 14-37. PORTCTL5 Register Field Descriptions**

| Bit   | Field             | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                     |  |  |  |
|-------|-------------------|-------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|--|
| 31-24 | BCAST_LIMIT       | R/W-0 | 0     | Broadcast Packet Rate Limit - Each prescale pulse loads this field<br>into the port broadcast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Broadcast rate limiting is enabled by a non-zero value in this field  |  |  |  |
| 23-16 | MCAST_LIMIT       | R/W-0 | 0     | Multicast Packet Rate Limit - Each prescale pulse loads this field into<br>the port multicast rate limit counter.<br>The port counters are decremented with each packet received or<br>transmitted depending on whether the mode is transmit or receive.<br>If the counters decrement to zero, then further packets are rate<br>limited until the next prescale pulse.<br>Multicast rate limiting is enabled by a non-zero value in this field. |  |  |  |
| 5     | NO_SA_UPDATE      | R/W-0 | 0     | No Souce Address Update - When set the port is disabled from<br>updating the source port number in an ALE table entry.                                                                                                                                                                                                                                                                                                                          |  |  |  |
| 4     | NO_LEARN          | R/W-0 | 0     | No Learn Mode - When set the port is disabled from learning an<br>address.                                                                                                                                                                                                                                                                                                                                                                      |  |  |  |
| 3     | VID_INGRESS_CHECK | R/W-0 | 0     | VLAN ID Ingress Check - If VLAN not found then drop the packet.<br>Packets with an unknown (default) VLAN will be dropped.                                                                                                                                                                                                                                                                                                                      |  |  |  |
| 2     | DROP_UNTAGGED     | R/W-0 | 0     | Drop Untagged Packets - Drop non-VLAN tagged ingress packets.                                                                                                                                                                                                                                                                                                                                                                                   |  |  |  |
| 1-0   | PORT_STATE        | R/W-0 | 0     | Port State<br>0 - Disabled<br>1 - Blocked<br>2 - Learn                                                                                                                                                                                                                                                                                                                                                                                          |  |  |  |

### *14.5.2 CPSW\_CPDMA Registers*

Table [14-38](#page-94-0) lists the memory-mapped registers for the CPSW\_CPDMA. All register offset addresses not listed in Table [14-38](#page-94-0) should be considered as reserved locations and the register contents should not be modified.

### **Table 14-38. CPSW\_CPDMA REGISTERS**

| Offset | Acronym            | Register Name                                             | Section           |
|--------|--------------------|-----------------------------------------------------------|-------------------|
| 0h     | TX_IDVER           | CPDMA_REGS TX IDENTIFICATION AND VERSION<br>REGISTER      | Section 14.5.2.1  |
| 4h     | TX_CONTROL         | CPDMA_REGS TX CONTROL REGISTER                            | Section 14.5.2.2  |
| 8h     | TX_TEARDOWN        | CPDMA_REGS TX TEARDOWN REGISTER                           | Section 14.5.2.3  |
| 10h    | RX_IDVER           | CPDMA_REGS RX IDENTIFICATION AND VERSION<br>REGISTER      | Section 14.5.2.4  |
| 14h    | RX_CONTROL         | CPDMA_REGS RX CONTROL REGISTER                            | Section 14.5.2.5  |
| 18h    | RX_TEARDOWN        | CPDMA_REGS RX TEARDOWN REGISTER                           | Section 14.5.2.6  |
| 1Ch    | CPDMA_SOFT_RESET   | CPDMA_REGS SOFT RESET REGISTER                            | Section 14.5.2.7  |
| 20h    | DMACONTROL         | CPDMA_REGS CPDMA CONTROL REGISTER                         | Section 14.5.2.8  |
| 24h    | DMASTATUS          | CPDMA_REGS CPDMA STATUS REGISTER                          | Section 14.5.2.9  |
| 28h    | RX_BUFFER_OFFSET   | CPDMA_REGS RECEIVE BUFFER OFFSET                          | Section 14.5.2.10 |
| 2Ch    | EMCONTROL          | CPDMA_REGS EMULATION CONTROL                              | Section 14.5.2.11 |
| 30h    | TX_PRI0_RATE       | CPDMA_REGS TRANSMIT (INGRESS) PRIORITY 0<br>RATE          | Section 14.5.2.12 |
| 34h    | TX_PRI1_RATE       | CPDMA_REGS TRANSMIT (INGRESS) PRIORITY 1<br>RATE          | Section 14.5.2.13 |
| 38h    | TX_PRI2_RATE       | CPDMA_REGS TRANSMIT (INGRESS) PRIORITY 2<br>RATE          | Section 14.5.2.14 |
| 3Ch    | TX_PRI3_RATE       | CPDMA_REGS TRANSMIT (INGRESS) PRIORITY 3<br>RATE          | Section 14.5.2.15 |
| 40h    | TX_PRI4_RATE       | CPDMA_REGS TRANSMIT (INGRESS) PRIORITY 4<br>RATE          | Section 14.5.2.16 |
| 44h    | TX_PRI5_RATE       | CPDMA_REGS TRANSMIT (INGRESS) PRIORITY 5<br>RATE          | Section 14.5.2.17 |
| 48h    | TX_PRI6_RATE       | CPDMA_REGS TRANSMIT (INGRESS) PRIORITY 6<br>RATE          | Section 14.5.2.18 |
| 4Ch    | TX_PRI7_RATE       | CPDMA_REGS TRANSMIT (INGRESS) PRIORITY 7<br>RATE          | Section 14.5.2.19 |
| 80h    | TX_INTSTAT_RAW     | CPDMA_INT TX INTERRUPT STATUS REGISTER<br>(RAW VALUE)     | Section 14.5.2.20 |
| 84h    | TX_INTSTAT_MASKED  | CPDMA_INT TX INTERRUPT STATUS REGISTER<br>(MASKED VALUE)  | Section 14.5.2.21 |
| 88h    | TX_INTMASK_SET     | CPDMA_INT TX INTERRUPT MASK SET REGISTER                  | Section 14.5.2.22 |
| 8Ch    | TX_INTMASK_CLEAR   | CPDMA_INT TX INTERRUPT MASK CLEAR<br>REGISTER             | Section 14.5.2.23 |
| 90h    | CPDMA_IN_VECTOR    | CPDMA_INT INPUT VECTOR (READ ONLY)                        | Section 14.5.2.24 |
| 94h    | CPDMA_EOI_VECTOR   | CPDMA_INT END OF INTERRUPT VECTOR                         | Section 14.5.2.25 |
| A0h    | RX_INTSTAT_RAW     | CPDMA_INT RX INTERRUPT STATUS REGISTER<br>(RAW VALUE)     | Section 14.5.2.26 |
| A4h    | RX_INTSTAT_MASKED  | CPDMA_INT RX INTERRUPT STATUS REGISTER<br>(MASKED VALUE)  | Section 14.5.2.27 |
| A8h    | RX_INTMASK_SET     | CPDMA_INT RX INTERRUPT MASK SET REGISTER                  | Section 14.5.2.28 |
| ACh    | RX_INTMASK_CLEAR   | CPDMA_INT RX INTERRUPT MASK CLEAR<br>REGISTER             | Section 14.5.2.29 |
| B0h    | DMA_INTSTAT_RAW    | CPDMA_INT DMA INTERRUPT STATUS REGISTER<br>(RAW VALUE)    | Section 14.5.2.30 |
| B4h    | DMA_INTSTAT_MASKED | CPDMA_INT DMA INTERRUPT STATUS REGISTER<br>(MASKED VALUE) | Section 14.5.2.31 |
| B8h    | DMA_INTMASK_SET    | CPDMA_INT DMA INTERRUPT MASK SET REGISTER                 | Section 14.5.2.32 |
| BCh    | DMA_INTMASK_CLEAR  | CPDMA_INT DMA INTERRUPT MASK CLEAR<br>REGISTER            | Section 14.5.2.33 |
| C0h    | RX0_PENDTHRESH     | CPDMA_INT RECEIVE THRESHOLD PENDING<br>REGISTER CHANNEL 0 | Section 14.5.2.34 |

## **Table 14-38. CPSW\_CPDMA REGISTERS (continued)**

| Offset | Acronym        | Register Name                                             | Section           |
|--------|----------------|-----------------------------------------------------------|-------------------|
| C4h    | RX1_PENDTHRESH | CPDMA_INT RECEIVE THRESHOLD PENDING<br>REGISTER CHANNEL 1 | Section 14.5.2.35 |
| C8h    | RX2_PENDTHRESH | CPDMA_INT RECEIVE THRESHOLD PENDING<br>REGISTER CHANNEL 2 | Section 14.5.2.36 |
| CCh    | RX3_PENDTHRESH | CPDMA_INT RECEIVE THRESHOLD PENDING<br>REGISTER CHANNEL 3 | Section 14.5.2.37 |
| D0h    | RX4_PENDTHRESH | CPDMA_INT RECEIVE THRESHOLD PENDING<br>REGISTER CHANNEL 4 | Section 14.5.2.38 |
| D4h    | RX5_PENDTHRESH | CPDMA_INT RECEIVE THRESHOLD PENDING<br>REGISTER CHANNEL 5 | Section 14.5.2.39 |
| D8h    | RX6_PENDTHRESH | CPDMA_INT RECEIVE THRESHOLD PENDING<br>REGISTER CHANNEL 6 | Section 14.5.2.40 |
| DCh    | RX7_PENDTHRESH | CPDMA_INT RECEIVE THRESHOLD PENDING<br>REGISTER CHANNEL 7 | Section 14.5.2.41 |
| E0h    | RX0_FREEBUFFER | CPDMA_INT RECEIVE FREE BUFFER REGISTER<br>CHANNEL 0       | Section 14.5.2.42 |
| E4h    | RX1_FREEBUFFER | CPDMA_INT RECEIVE FREE BUFFER REGISTER<br>CHANNEL 1       | Section 14.5.2.43 |
| E8h    | RX2_FREEBUFFER | CPDMA_INT RECEIVE FREE BUFFER REGISTER<br>CHANNEL 2       | Section 14.5.2.44 |
| ECh    | RX3_FREEBUFFER | CPDMA_INT RECEIVE FREE BUFFER REGISTER<br>CHANNEL 3       | Section 14.5.2.45 |
| F0h    | RX4_FREEBUFFER | CPDMA_INT RECEIVE FREE BUFFER REGISTER<br>CHANNEL 4       | Section 14.5.2.46 |
| F4h    | RX5_FREEBUFFER | CPDMA_INT RECEIVE FREE BUFFER REGISTER<br>CHANNEL 5       | Section 14.5.2.47 |
| F8h    | RX6_FREEBUFFER | CPDMA_INT RECEIVE FREE BUFFER REGISTER<br>CHANNEL 6       | Section 14.5.2.48 |
| FCh    | RX7_FREEBUFFER | CPDMA_INT RECEIVE FREE BUFFER REGISTER<br>CHANNEL 7       | Section 14.5.2.49 |

#### **14.5.2.1 TX\_IDVER Register (offset = 0h) [reset = 180108h]**

TX\_IDVER is shown in [Figure](#page-96-1) 14-29 and described in Table [14-39](#page-96-2). CPDMA\_REGS TX IDENTIFICATION AND VERSION REGISTER

## **Figure 14-29. TX\_IDVER Register**

| 31 | 30 | 29 | 28 | 27           | 26 | 25 | 24 |
|----|----|----|----|--------------|----|----|----|
|    |    |    |    | TX_IDENT     |    |    |    |
|    |    |    |    | R-18h        |    |    |    |
| 23 | 22 | 21 | 20 | 19           | 18 | 17 | 16 |
|    |    |    |    | TX_IDENT     |    |    |    |
|    |    |    |    | R-18h        |    |    |    |
| 15 | 14 | 13 | 12 | 11           | 10 | 9  | 8  |
|    |    |    |    | TX_MAJOR_VER |    |    |    |
|    |    |    |    | R-1h         |    |    |    |
| 7  | 6  | 5  | 4  | 3            | 2  | 1  | 0  |
|    |    |    |    | TX_MINOR_VER |    |    |    |
|    |    |    |    |              |    |    |    |

R-8h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-39. TX\_IDVER Register Field Descriptions**

| Bit   | Field        | Type | Reset | Description                                                            |
|-------|--------------|------|-------|------------------------------------------------------------------------|
| 31-16 | TX_IDENT     | R    | 18h   | TX Identification Value                                                |
| 15-8  | TX_MAJOR_VER | R    | 1h    | TX Major Version Value - The value read is the major version<br>number |
| 7-0   | TX_MINOR_VER | R    | 8h    | TX Minor Version Value - The value read is the minor version<br>number |

#### **14.5.2.2 TX\_CONTROL Register (offset = 4h) [reset = 0h]**

TX\_CONTROL is shown in [Figure](#page-97-1) 14-30 and described in Table [14-40](#page-97-2). CPDMA\_REGS TX CONTROL REGISTER

## **Figure 14-30. TX\_CONTROL Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

 7             6             5             4             3             2             1             0
┌───────────────────────────────────────────────────────────────────────────────┬─────────────────┐
│                                   Reserved                                    │      TX_EN      │
└───────────────────────────────────────────────────────────────────────────────┴─────────────────┘
                                                                                R/W-0h
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-40. TX\_CONTROL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                              |
|------|----------|------|-------|------------------------------------------|
| 31-1 | Reserved | R    | 0h    |                                          |
| 0    | TX_EN    | R/W  | 0h    | TX Enable<br>0 - Disabled<br>1 - Enabled |

#### **14.5.2.3 TX\_TEARDOWN Register (offset = 8h) [reset = 0h]**

TX\_TEARDOWN is shown in [Figure](#page-98-1) 14-31 and described in Table [14-41.](#page-98-2) CPDMA\_REGS TX TEARDOWN REGISTER

## **Figure 14-31. TX\_TEARDOWN Register**

| 31         | 30                    | 29       | 28 | 27       | 26 | 25     | 24 |  |  |
|------------|-----------------------|----------|----|----------|----|--------|----|--|--|
| TX_TDN_RDY |                       | Reserved |    |          |    |        |    |  |  |
| R-0h       |                       |          |    | R-0h     |    |        |    |  |  |
| 23         | 22                    | 21       | 20 | 19       | 18 | 17     | 16 |  |  |
|            |                       |          |    | Reserved |    |        |    |  |  |
|            | R-0h                  |          |    |          |    |        |    |  |  |
| 15         | 14                    | 13       | 12 | 11       | 10 | 9      | 8  |  |  |
|            |                       |          |    | Reserved |    |        |    |  |  |
|            |                       |          |    | R-0h     |    |        |    |  |  |
| 7          | 6                     | 5        | 4  | 3        | 2  | 1      | 0  |  |  |
|            | Reserved<br>TX_TDN_CH |          |    |          |    |        |    |  |  |
|            |                       | R-0h     |    |          |    | R/W-0h |    |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-41. TX\_TEARDOWN Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                                                                                  |
|------|------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31   | TX_TDN_RDY | R    | 0h    | Tx Teardown Ready - read as zero, but is always assumed to be<br>one (unused).                                                                                                                                                                               |
| 30-3 | Reserved   | R    | 0h    |                                                                                                                                                                                                                                                              |
| 2-0  | TX_TDN_CH  | R/W  | 0h    | Tx Teardown Channel - Transmit channel teardown is commanded<br>by writing the encoded value of the transmit channel to be torn<br>down.<br>The teardown register is read as zero.<br>000 - teardown transmit channel 0<br>111 - teardown transmit channel 7 |

#### **14.5.2.4 RX\_IDVER Register (offset = 10h) [reset = C0107h]**

RX\_IDVER is shown in [Figure](#page-99-1) 14-32 and described in Table [14-42](#page-99-2). CPDMA\_REGS RX IDENTIFICATION AND VERSION REGISTER

### **Figure 14-32. RX\_IDVER Register**

| 31 | 30 | 29 | 28 | 27           | 26 | 25 | 24 |
|----|----|----|----|--------------|----|----|----|
|    |    |    |    | RX_IDENT     |    |    |    |
|    |    |    |    | R-Ch         |    |    |    |
| 23 | 22 | 21 | 20 | 19           | 18 | 17 | 16 |
|    |    |    |    | RX_IDENT     |    |    |    |
|    |    |    |    | R-Ch         |    |    |    |
| 15 | 14 | 13 | 12 | 11           | 10 | 9  | 8  |
|    |    |    |    | RX_MAJOR_VER |    |    |    |
|    |    |    |    | R-1h         |    |    |    |
| 7  | 6  | 5  | 4  | 3            | 2  | 1  | 0  |
|    |    |    |    | RX_MINOR_VER |    |    |    |
|    |    |    |    | R-7h         |    |    |    |
|    |    |    |    |              |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-42. RX\_IDVER Register Field Descriptions**

| Bit   | Field        | Type | Reset | Description             |
|-------|--------------|------|-------|-------------------------|
| 31-16 | RX_IDENT     | R    | Ch    | RX Identification Value |
| 15-8  | RX_MAJOR_VER | R    | 1h    | RX Major Version Value  |
| 7-0   | RX_MINOR_VER | R    | 7h    | RX Minor Version Value  |

#### **14.5.2.5 RX\_CONTROL Register (offset = 14h) [reset = 0h]**

RX\_CONTROL is shown in [Figure](#page-100-1) 14-33 and described in Table [14-43](#page-100-2). CPDMA\_REGS RX CONTROL REGISTER

## **Figure 14-33. RX\_CONTROL Register**

| 30       | 29 | 28       | 27 | 26                           | 25 | 24     |  |  |  |
|----------|----|----------|----|------------------------------|----|--------|--|--|--|
| Reserved |    |          |    |                              |    |        |  |  |  |
| R-0h     |    |          |    |                              |    |        |  |  |  |
| 22       | 21 | 20       | 19 | 18                           | 17 | 16     |  |  |  |
|          |    |          |    |                              |    |        |  |  |  |
| R-0h     |    |          |    |                              |    |        |  |  |  |
| 14       | 13 | 12       | 11 | 10                           | 9  | 8      |  |  |  |
|          |    |          |    |                              |    |        |  |  |  |
|          |    |          |    |                              |    |        |  |  |  |
| 6        | 5  | 4        | 3  | 2                            | 1  | 0      |  |  |  |
|          |    | Reserved |    |                              |    | RX_EN  |  |  |  |
|          |    | R-0h     |    |                              |    | R/W-0h |  |  |  |
|          |    |          |    | Reserved<br>Reserved<br>R-0h |    |        |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-43. RX\_CONTROL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                  |
|------|----------|------|-------|----------------------------------------------|
| 31-1 | Reserved | R    | 0h    |                                              |
| 0    | RX_EN    | R/W  | 0h    | RX DMA Enable<br>0 - Disabled<br>1 - Enabled |

#### **14.5.2.6 RX\_TEARDOWN Register (offset = 18h) [reset = 0h]**

RX\_TEARDOWN is shown in [Figure](#page-101-1) 14-34 and described in Table [14-44.](#page-101-2) CPDMA\_REGS RX TEARDOWN REGISTER

## **Figure 14-34. RX\_TEARDOWN Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│ RX_TDN_RDY                                  Reserved                                             │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
 R-0h                                          R-0h

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

 7             6             5             4             3             2             1             0
┌───────────────────────────────────────────────────────────────┬─────────────────────────────────┐
│                           Reserved                            │           RX_TDN_CH             │
└───────────────────────────────────────────────────────────────┴─────────────────────────────────┘
                                                                R/W-0h
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-44. RX\_TEARDOWN Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                                                                          |
|------|------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31   | RX_TDN_RDY | R    | 0h    | Teardown Ready - read as zero, but is always assumed to be one<br>(unused).                                                                                                                                                                          |
| 30-3 | Reserved   | R    | 0h    |                                                                                                                                                                                                                                                      |
| 2-0  | RX_TDN_CH  | R/W  | 0h    | Rx Teardown Channel -Receive channel teardown is commanded by<br>writing the encoded value of the receive channel to be torn down.<br>The teardown register is read as zero.<br>000 - teardown receive channel 0<br>111 - teardown receive channel 7 |

#### **14.5.2.7 CPDMA\_SOFT\_RESET Register (offset = 1Ch) [reset = 0h]**

CPDMA\_SOFT\_RESET is shown in [Figure](#page-102-1) 14-35 and described in Table [14-45](#page-102-2). CPDMA\_REGS SOFT RESET REGISTER

## **Figure 14-35. CPDMA\_SOFT\_RESET Register**

| 30       | 29 | 28       | 27 | 26               | 25 | 24         |  |  |
|----------|----|----------|----|------------------|----|------------|--|--|
| Reserved |    |          |    |                  |    |            |  |  |
| R-0h     |    |          |    |                  |    |            |  |  |
| 22       | 21 | 20       | 19 | 18               | 17 | 16         |  |  |
| Reserved |    |          |    |                  |    |            |  |  |
| R-0h     |    |          |    |                  |    |            |  |  |
| 14       | 13 | 12       | 11 | 10               | 9  | 8          |  |  |
|          |    |          |    |                  |    |            |  |  |
|          |    |          |    |                  |    |            |  |  |
| 6        | 5  | 4        | 3  | 2                | 1  | 0          |  |  |
|          |    | Reserved |    |                  |    | SOFT_RESET |  |  |
|          |    | R-0h     |    |                  |    | R/W-0h     |  |  |
|          |    |          |    | Reserved<br>R-0h |    |            |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-45. CPDMA\_SOFT\_RESET Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                 |
|------|------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | Reserved   | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                             |
| 0    | SOFT_RESET | R/W  | 0h    | Software reset - Writing a one to this bit causes the CPDMA logic to<br>be reset.<br>Software reset occurs when the RX and TX DMA Controllers are in<br>an idle state to avoid locking up the VBUSP bus.<br>After writing a one to this bit, it may be polled to determine if the<br>reset has occurred.<br>If a one is read, the reset has not yet occurred.<br>If a zero is read then reset has occurred. |

#### **14.5.2.8 DMACONTROL Register (offset = 20h) [reset = 0h]**

DMACONTROL is shown in [Figure](#page-103-1) 14-36 and described in Table [14-46](#page-103-2). CPDMA\_REGS CPDMA CONTROL REGISTER

## **Figure 14-36. DMACONTROL Register**

| 31 | 30       | 29 | 28 | 27       | 26                  | 25               | 24       |  |  |  |
|----|----------|----|----|----------|---------------------|------------------|----------|--|--|--|
|    | Reserved |    |    |          |                     |                  |          |  |  |  |
|    |          |    |    | R-0h     |                     |                  |          |  |  |  |
| 23 | 22       | 21 | 20 | 19       | 18                  | 17               | 16       |  |  |  |
|    | Reserved |    |    |          |                     |                  |          |  |  |  |
|    | R-0h     |    |    |          |                     |                  |          |  |  |  |
| 15 | 14       | 13 | 12 | 11       | 10                  | 9                | 8        |  |  |  |
|    |          |    |    | TX_RLIM  |                     |                  |          |  |  |  |
|    |          |    |    | R/W-0h   |                     |                  |          |  |  |  |
| 7  | 6        | 5  | 4  | 3        | 2                   | 1                | 0        |  |  |  |
|    | Reserved |    |    | CMD_IDLE | RX_OFFLEN_B<br>LOCK | RX_OWNERS<br>HIP | TX_PTYPE |  |  |  |
|    | R-0h     |    |    | R/W-0h   | R/W-0h              | R/W-0h           | R/W-0h   |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-46. DMACONTROL Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
|-------|-----------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | Reserved        | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 15-8  | TX_RLIM         | R/W  | 0h    | Transmit Rate Limit Channel Bus<br>00000000 - no rate-limited channels<br>10000000 - channel 7 is rate-limited<br>11000000 - channels 7 downto 6 are rate-limited<br>11100000 - channels 7 downto 5 are rate-limited<br>11110000 - channels 7 downto 4 are rate-limited<br>11111000 - channels 7 downto 3 are rate-limited<br>11111100 - channels 7 downto 2 are rate-limited<br>11111110 - channels 7 downto 1 are rate-limited<br>11111111 - channels 7 downto 0 are rate-limited all others invalid -<br>this bus must be set msb towards lsb.<br>tx_ptype must be set if any tx_rlim bit is set for fixed priority. |
| 7-5   | Reserved        | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 4     | RX_CEF          | R/W  | 0h    | RX Copy Error Frames Enable - Enables DMA overrun frames to be<br>transferred to memory (up to the point of overrun).<br>The overrun error bit will be set in the frame EOP buffer descriptor.<br>Overrun frame data will be filtered when rx_cef is not set.<br>Frames coming from the receive FIFO with other error bits set are<br>not effected by this bit.<br>0 - Frames containing overrun errors are filtered.<br>1 - Frames containing overrun errors are transferred to memory.                                                                                                                                |
| 3     | CMD_IDLE        | R/W  | 0h    | Command Idle<br>0 - Idle not commanded<br>1 - Idle Commanded (read idle in DMAStatus)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 2     | RX_OFFLEN_BLOCK | R/W  | 0h    | Receive Offset/Length word write block.<br>0 - Do not block the DMA writes to the receive buffer descriptor<br>offset/buffer length word.<br>The offset/buffer length word is written as specified in CPPI 3.0.<br>1 - Block all CPDMA DMA controller writes to the receive buffer<br>descriptor offset/buffer length words during CPPI packet processing.<br>when this bit is set, the CPDMA will never write the third word to any<br>receive buffer descriptor.                                                                                                                                                      |

## **Table 14-46. DMACONTROL Register Field Descriptions (continued)**

| Bit | Field        | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                          |
|-----|--------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1   | RX_OWNERSHIP | R/W  | 0h    | Receive Ownership Write Bit Value.<br>0 - The CPDMA writes the receive ownership bit to zero at the end<br>of packet processing as specified in CPPI 3.0.<br>1 - The CPDMA writes the receive ownership bit to one at the end of<br>packet processing.<br>Users who do not use the ownership mechanism can use this mode<br>to preclude the necessity of software having to set this bit each time<br>the buffer descriptor is used. |
| 0   | TX_PTYPE     | R/W  | 0h    | Transmit Queue Priority Type<br>0 - The queue uses a round robin scheme to select the next channel<br>for transmission.<br>1 - The queue uses a fixed (channel 7 highest priority) priority<br>scheme to select the next channel for transmission                                                                                                                                                                                    |

#### **14.5.2.9 DMASTATUS Register (offset = 24h) [reset = 0h]**

DMASTATUS is shown in [Figure](#page-105-1) 14-37 and described in Table [14-47.](#page-105-2)

CPDMA\_REGS CPDMA STATUS REGISTER

## **Figure 14-37. DMASTATUS Register**

| 31   | 30   | 29               | 28 | 27       | 26        | 25        | 24 |  |
|------|------|------------------|----|----------|-----------|-----------|----|--|
| IDLE |      | Reserved         |    |          |           |           |    |  |
| R-0h |      |                  |    | R-0h     |           |           |    |  |
| 23   | 22   | 21               | 20 | 19       | 18        | 17        | 16 |  |
|      |      | TX_HOST_ERR_CODE |    | Reserved | TX_ERR_CH |           |    |  |
|      |      | R-0h             |    | R-0h     | R-0h      |           |    |  |
| 15   | 14   | 13               | 12 | 11       | 10        | 9         | 8  |  |
|      |      | RX_HOST_ERR_CODE |    | Reserved |           | RX_ERR_CH |    |  |
|      | R-0h |                  |    | R-0h     | R-0h      |           |    |  |
| 7    | 6    | 5                | 4  | 3        | 2         | 1         | 0  |  |
|      |      |                  |    | Reserved |           |           |    |  |

R-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-47. DMASTATUS Register Field Descriptions**

| Bit   | Field            | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
|-------|------------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31    | IDLE             | R    | 0h    | Idle Status Bit - Indicates when set that the CPDMA is not<br>transferring a packet on transmit or receive.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 30-24 | Reserved         | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 23-20 | TX_HOST_ERR_CODE | R    | 0h    | TX Host Error Code - This field is set to indicate CPDMA detected<br>TX DMA related host errors.<br>The host should read this field after a HOST_ERR_INT to determine<br>the error.<br>Host error Interrupts require hardware reset in order to recover.<br>A zero packet length is an error, but it is not detected.<br>0000 - No error<br>0001 - SOP error.<br>0010 - Ownership bit not set in SOP buffer.<br>0011 - Zero Next Buffer Descriptor Pointer Without EOP<br>0100 - Zero Buffer Pointer.<br>0101 - Zero Buffer Length<br>0110 - Packet Length Error (sum of buffers is less than packet<br>length)<br>0111 - reserved<br>1111 - reserved |
| 19    | Reserved         | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 18-16 | TX_ERR_CH        | R    | 0h    | TX Host Error Channel - This field indicates which TX channel (if<br>applicable) the host error occurred on.<br>This field is cleared to zero on a host read.<br>000 - The host error occurred on TX channel 0<br>111 - The host error occurred on TX channel 7                                                                                                                                                                                                                                                                                                                                                                                       |
| 15-12 | RX_HOST_ERR_CODE | R    | 0h    | RX Host Error Code - This field is set to indicate CPDMA detected<br>RX DMA related host errors.<br>The host should read this field after a HOST_ERR_INT to determine<br>the error.<br>Host error Interrupts require hardware reset in order to recover.<br>0000 - No error<br>0001 - reserved<br>0010 - Ownership bit not set in input buffer.<br>0011 - reserved<br>0100 - Zero Buffer Pointer.<br>0101 - Zero buffer length on non-SOP descriptor<br>0110 - SOP buffer length not greater than offset<br>1111 - reserved                                                                                                                           |

## **Table 14-47. DMASTATUS Register Field Descriptions (continued)**

| Bit  | Field     | Type | Reset | Description                                                                                                                                                                                                                                     |
|------|-----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 11   | Reserved  | R    | 0h    |                                                                                                                                                                                                                                                 |
| 10-8 | RX_ERR_CH | R    | 0h    | RX Host Error Channel - This field indicates which RX channel the<br>host error occurred on.<br>This field is cleared to zero on a host read.<br>000 - The host error occurred on RX channel 0<br>111 - The host error occurred on RX channel 7 |
| 7-0  | Reserved  | R    | 0h    |                                                                                                                                                                                                                                                 |

#### **14.5.2.10 RX\_BUFFER\_OFFSET Register (offset = 28h) [reset = 0h]**

RX\_BUFFER\_OFFSET is shown in [Figure](#page-107-1) 14-38 and described in Table [14-48.](#page-107-2) CPDMA\_REGS RECEIVE BUFFER OFFSET

### **Figure 14-38. RX\_BUFFER\_OFFSET Register**

| 31 | 30       | 29 | 28 | 27               | 26 | 25 | 24 |  |  |
|----|----------|----|----|------------------|----|----|----|--|--|
|    | Reserved |    |    |                  |    |    |    |  |  |
|    | R-0h     |    |    |                  |    |    |    |  |  |
| 23 | 22       | 21 | 20 | 19               | 18 | 17 | 16 |  |  |
|    | Reserved |    |    |                  |    |    |    |  |  |
|    | R-0h     |    |    |                  |    |    |    |  |  |
| 15 | 14       | 13 | 12 | 11               | 10 | 9  | 8  |  |  |
|    |          |    |    | RX_BUFFER_OFFSET |    |    |    |  |  |
|    |          |    |    | R/W-0h           |    |    |    |  |  |
| 7  | 6        | 5  | 4  | 3                | 2  | 1  | 0  |  |  |
|    |          |    |    | RX_BUFFER_OFFSET |    |    |    |  |  |
|    |          |    |    |                  |    |    |    |  |  |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-48. RX\_BUFFER\_OFFSET Register Field Descriptions**

| Bit   | Field            | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|-------|------------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | Reserved         | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| 15-0  | RX_BUFFER_OFFSET | R/W  | 0h    | Receive Buffer Offset Value - The rx_buffer_offset will be written by<br>the port into each frame SOP buffer descriptor buffer_offset field.<br>The frame data will begin after the rx_buffer_offset value of bytes.<br>A value of 0x0000 indicates that there are no unused bytes at the<br>beginning of the data and that valid data begins on the first byte of<br>the buffer.<br>A value of 0x000F (decimal 15) indicates that the first 15 bytes of the<br>buffer are to be ignored by the port and that valid buffer data starts<br>on byte 16 of the buffer.<br>This value is used for all channels. |

#### **14.5.2.11 EMCONTROL Register (offset = 2Ch) [reset = 0h]**

EMCONTROL is shown in [Figure](#page-108-1) 14-39 and described in Table [14-49.](#page-108-2) CPDMA\_REGS EMULATION CONTROL

**Figure 14-39. EMCONTROL Register**

| 31   | 30       | 29 | 28       | 27       | 26 | 25   | 24     |  |  |
|------|----------|----|----------|----------|----|------|--------|--|--|
|      | Reserved |    |          |          |    |      |        |  |  |
|      | R-0h     |    |          |          |    |      |        |  |  |
| 23   | 22       | 21 | 20       | 19       | 18 | 17   | 16     |  |  |
|      | Reserved |    |          |          |    |      |        |  |  |
| R-0h |          |    |          |          |    |      |        |  |  |
| 15   | 14       | 13 | 12       | 11       | 10 | 9    | 8      |  |  |
|      |          |    |          | Reserved |    |      |        |  |  |
|      |          |    |          | R-0h     |    |      |        |  |  |
| 7    | 6        | 5  | 4        | 3        | 2  | 1    | 0      |  |  |
|      |          |    | Reserved |          |    | SOFT | FREE   |  |  |
|      | R-0h     |    |          |          |    |      | R/W-0h |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-49. EMCONTROL Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description        |
|------|----------|------|-------|--------------------|
| 31-2 | Reserved | R    | 0h    |                    |
| 1    | SOFT     | R/W  | 0h    | Emulation Soft Bit |
| 0    | FREE     | R/W  | 0h    | Emulation Free Bit |

#### **14.5.2.12 TX\_PRI0\_RATE Register (offset = 30h) [reset = 0h]**

TX\_PRI0\_RATE is shown in [Figure](#page-109-1) 14-40 and described in Table [14-50.](#page-109-2) CPDMA\_REGS TRANSMIT (INGRESS) PRIORITY 0 RATE

## **Figure 14-40. TX\_PRI0\_RATE Register**

| 31 | 30       | 29 | 28 | 27            | 26            | 25 | 24 |
|----|----------|----|----|---------------|---------------|----|----|
|    | Reserved |    |    |               | PRIN_IDLE_CNT |    |    |
|    | R-0h     |    |    |               | R/W-0h        |    |    |
| 23 | 22       | 21 | 20 | 19            | 18            | 17 | 16 |
|    |          |    |    | PRIN_IDLE_CNT |               |    |    |
|    |          |    |    | R/W-0h        |               |    |    |
| 15 | 14       | 13 | 12 | 11            | 10            | 9  | 8  |
|    | Reserved |    |    |               | PRIN_SEND_CNT |    |    |
|    | R-0h     |    |    |               | R/W-0h        |    |    |
| 7  | 6        | 5  | 4  | 3             | 2             | 1  | 0  |
|    |          |    |    | PRIN_SEND_CNT |               |    |    |
|    |          |    |    |               |               |    |    |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-50. TX\_PRI0\_RATE Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                   |
|-------|---------------|------|-------|-------------------------------|
| 31-30 | Reserved      | R    | 0h    |                               |
| 29-16 | PRIN_IDLE_CNT | R/W  | 0h    | Priority (<br>7:0) idle count |
| 15-14 | Reserved      | R    | 0h    |                               |
| 13-0  | PRIN_SEND_CNT | R/W  | 0h    | Priority (<br>7:0) send count |

#### **14.5.2.13 TX\_PRI1\_RATE Register (offset = 34h) [reset = 0h]**

TX\_PRI1\_RATE is shown in [Figure](#page-110-1) 14-41 and described in Table [14-51.](#page-110-2) CPDMA\_REGS TRANSMIT (INGRESS) PRIORITY 1 RATE

## **Figure 14-41. TX\_PRI1\_RATE Register**

| 31 | 30       | 29 | 28            | 27            | 26            | 25 | 24 |
|----|----------|----|---------------|---------------|---------------|----|----|
|    | Reserved |    |               |               | PRIN_IDLE_CNT |    |    |
|    | R-0h     |    |               |               | R/W-0h        |    |    |
| 23 | 22       | 21 | 20            | 19            | 18            | 17 | 16 |
|    |          |    |               | PRIN_IDLE_CNT |               |    |    |
|    |          |    |               | R/W-0h        |               |    |    |
| 15 | 14       | 13 | 12            | 11            | 10            | 9  | 8  |
|    | Reserved |    | PRIN_SEND_CNT |               |               |    |    |
|    | R-0h     |    |               |               | R/W-0h        |    |    |
| 7  | 6        | 5  | 4             | 3             | 2             | 1  | 0  |
|    |          |    |               | PRIN_SEND_CNT |               |    |    |
|    |          |    |               |               |               |    |    |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-51. TX\_PRI1\_RATE Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                   |
|-------|---------------|------|-------|-------------------------------|
| 31-30 | Reserved      | R    | 0h    |                               |
| 29-16 | PRIN_IDLE_CNT | R/W  | 0h    | Priority (<br>7:0) idle count |
| 15-14 | Reserved      | R    | 0h    |                               |
| 13-0  | PRIN_SEND_CNT | R/W  | 0h    | Priority (<br>7:0) send count |

#### **14.5.2.14 TX\_PRI2\_RATE Register (offset = 38h) [reset = 0h]**

TX\_PRI2\_RATE is shown in [Figure](#page-111-1) 14-42 and described in Table [14-52.](#page-111-2) CPDMA\_REGS TRANSMIT (INGRESS) PRIORITY 2 RATE

## **Figure 14-42. TX\_PRI2\_RATE Register**

| 31 | 30       | 29 | 28            | 27            | 26            | 25 | 24 |
|----|----------|----|---------------|---------------|---------------|----|----|
|    | Reserved |    |               |               | PRIN_IDLE_CNT |    |    |
|    | R-0h     |    |               |               | R/W-0h        |    |    |
| 23 | 22       | 21 | 20            | 19            | 18            | 17 | 16 |
|    |          |    |               | PRIN_IDLE_CNT |               |    |    |
|    |          |    |               | R/W-0h        |               |    |    |
| 15 | 14       | 13 | 12            | 11            | 10            | 9  | 8  |
|    | Reserved |    | PRIN_SEND_CNT |               |               |    |    |
|    | R-0h     |    |               |               | R/W-0h        |    |    |
| 7  | 6        | 5  | 4             | 3             | 2             | 1  | 0  |
|    |          |    |               | PRIN_SEND_CNT |               |    |    |
|    |          |    |               |               |               |    |    |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-52. TX\_PRI2\_RATE Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                   |
|-------|---------------|------|-------|-------------------------------|
| 31-30 | Reserved      | R    | 0h    |                               |
| 29-16 | PRIN_IDLE_CNT | R/W  | 0h    | Priority (<br>7:0) idle count |
| 15-14 | Reserved      | R    | 0h    |                               |
| 13-0  | PRIN_SEND_CNT | R/W  | 0h    | Priority (<br>7:0) send count |

#### **14.5.2.15 TX\_PRI3\_RATE Register (offset = 3Ch) [reset = 0h]**

TX\_PRI3\_RATE is shown in [Figure](#page-112-1) 14-43 and described in Table [14-53.](#page-112-2) CPDMA\_REGS TRANSMIT (INGRESS) PRIORITY 3 RATE

## **Figure 14-43. TX\_PRI3\_RATE Register**

| 31 | 30       | 29 | 28            | 27            | 26            | 25 | 24 |
|----|----------|----|---------------|---------------|---------------|----|----|
|    | Reserved |    |               |               | PRIN_IDLE_CNT |    |    |
|    | R-0h     |    |               |               | R/W-0h        |    |    |
| 23 | 22       | 21 | 20            | 19            | 18            | 17 | 16 |
|    |          |    |               | PRIN_IDLE_CNT |               |    |    |
|    |          |    |               | R/W-0h        |               |    |    |
| 15 | 14       | 13 | 12            | 11            | 10            | 9  | 8  |
|    | Reserved |    | PRIN_SEND_CNT |               |               |    |    |
|    | R-0h     |    |               |               | R/W-0h        |    |    |
| 7  | 6        | 5  | 4             | 3             | 2             | 1  | 0  |
|    |          |    |               | PRIN_SEND_CNT |               |    |    |
|    |          |    |               |               |               |    |    |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-53. TX\_PRI3\_RATE Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                   |
|-------|---------------|------|-------|-------------------------------|
| 31-30 | Reserved      | R    | 0h    |                               |
| 29-16 | PRIN_IDLE_CNT | R/W  | 0h    | Priority (<br>7:0) idle count |
| 15-14 | Reserved      | R    | 0h    |                               |
| 13-0  | PRIN_SEND_CNT | R/W  | 0h    | Priority (<br>7:0) send count |

#### **14.5.2.16 TX\_PRI4\_RATE Register (offset = 40h) [reset = 0h]**

TX\_PRI4\_RATE is shown in [Figure](#page-113-1) 14-44 and described in Table [14-54.](#page-113-2) CPDMA\_REGS TRANSMIT (INGRESS) PRIORITY 4 RATE

## **Figure 14-44. TX\_PRI4\_RATE Register**

| 31 | 30       | 29 | 28 | 27            | 26            | 25 | 24 |
|----|----------|----|----|---------------|---------------|----|----|
|    | Reserved |    |    |               | PRIN_IDLE_CNT |    |    |
|    | R-0h     |    |    |               | R/W-0h        |    |    |
| 23 | 22       | 21 | 20 | 19            | 18            | 17 | 16 |
|    |          |    |    | PRIN_IDLE_CNT |               |    |    |
|    |          |    |    | R/W-0h        |               |    |    |
| 15 | 14       | 13 | 12 | 11            | 10            | 9  | 8  |
|    | Reserved |    |    |               | PRIN_SEND_CNT |    |    |
|    | R-0h     |    |    |               | R/W-0h        |    |    |
| 7  | 6        | 5  | 4  | 3             | 2             | 1  | 0  |
|    |          |    |    | PRIN_SEND_CNT |               |    |    |
|    |          |    |    |               |               |    |    |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-54. TX\_PRI4\_RATE Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                   |
|-------|---------------|------|-------|-------------------------------|
| 31-30 | Reserved      | R    | 0h    |                               |
| 29-16 | PRIN_IDLE_CNT | R/W  | 0h    | Priority (<br>7:0) idle count |
| 15-14 | Reserved      | R    | 0h    |                               |
| 13-0  | PRIN_SEND_CNT | R/W  | 0h    | Priority (<br>7:0) send count |

#### **14.5.2.17 TX\_PRI5\_RATE Register (offset = 44h) [reset = 0h]**

TX\_PRI5\_RATE is shown in [Figure](#page-114-1) 14-45 and described in Table [14-55.](#page-114-2) CPDMA\_REGS TRANSMIT (INGRESS) PRIORITY 5 RATE

## **Figure 14-45. TX\_PRI5\_RATE Register**

| 31 | 30       | 29 | 28            | 27            | 26            | 25 | 24 |
|----|----------|----|---------------|---------------|---------------|----|----|
|    | Reserved |    |               |               | PRIN_IDLE_CNT |    |    |
|    | R-0h     |    |               |               | R/W-0h        |    |    |
| 23 | 22       | 21 | 20            | 19            | 18            | 17 | 16 |
|    |          |    |               | PRIN_IDLE_CNT |               |    |    |
|    |          |    |               | R/W-0h        |               |    |    |
| 15 | 14       | 13 | 12            | 11            | 10            | 9  | 8  |
|    | Reserved |    | PRIN_SEND_CNT |               |               |    |    |
|    | R-0h     |    |               |               | R/W-0h        |    |    |
| 7  | 6        | 5  | 4             | 3             | 2             | 1  | 0  |
|    |          |    |               | PRIN_SEND_CNT |               |    |    |
|    |          |    |               |               |               |    |    |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-55. TX\_PRI5\_RATE Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                   |
|-------|---------------|------|-------|-------------------------------|
| 31-30 | Reserved      | R    | 0h    |                               |
| 29-16 | PRIN_IDLE_CNT | R/W  | 0h    | Priority (<br>7:0) idle count |
| 15-14 | Reserved      | R    | 0h    |                               |
| 13-0  | PRIN_SEND_CNT | R/W  | 0h    | Priority (<br>7:0) send count |

#### **14.5.2.18 TX\_PRI6\_RATE Register (offset = 48h) [reset = 0h]**

TX\_PRI6\_RATE is shown in [Figure](#page-115-1) 14-46 and described in Table [14-56.](#page-115-2) CPDMA\_REGS TRANSMIT (INGRESS) PRIORITY 6 RATE

## **Figure 14-46. TX\_PRI6\_RATE Register**

| 31 | 30       | 29 | 28 | 27            | 26            | 25 | 24 |
|----|----------|----|----|---------------|---------------|----|----|
|    | Reserved |    |    |               | PRIN_IDLE_CNT |    |    |
|    | R-0h     |    |    |               | R/W-0h        |    |    |
| 23 | 22       | 21 | 20 | 19            | 18            | 17 | 16 |
|    |          |    |    | PRIN_IDLE_CNT |               |    |    |
|    |          |    |    | R/W-0h        |               |    |    |
| 15 | 14       | 13 | 12 | 11            | 10            | 9  | 8  |
|    | Reserved |    |    |               | PRIN_SEND_CNT |    |    |
|    | R-0h     |    |    |               | R/W-0h        |    |    |
| 7  | 6        | 5  | 4  | 3             | 2             | 1  | 0  |
|    |          |    |    | PRIN_SEND_CNT |               |    |    |
|    |          |    |    |               |               |    |    |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-56. TX\_PRI6\_RATE Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                   |
|-------|---------------|------|-------|-------------------------------|
| 31-30 | Reserved      | R    | 0h    |                               |
| 29-16 | PRIN_IDLE_CNT | R/W  | 0h    | Priority (<br>7:0) idle count |
| 15-14 | Reserved      | R    | 0h    |                               |
| 13-0  | PRIN_SEND_CNT | R/W  | 0h    | Priority (<br>7:0) send count |

#### **14.5.2.19 TX\_PRI7\_RATE Register (offset = 4Ch) [reset = 0h]**

TX\_PRI7\_RATE is shown in [Figure](#page-116-1) 14-47 and described in Table [14-57.](#page-116-2) CPDMA\_REGS TRANSMIT (INGRESS) PRIORITY 7 RATE

## **Figure 14-47. TX\_PRI7\_RATE Register**

| 31 | 30            | 29 | 28            | 27            | 26     | 25 | 24 |  |  |
|----|---------------|----|---------------|---------------|--------|----|----|--|--|
|    | Reserved      |    | PRIN_IDLE_CNT |               |        |    |    |  |  |
|    | R-0h          |    |               |               | R/W-0h |    |    |  |  |
| 23 | 22            | 21 | 20            | 19            | 18     | 17 | 16 |  |  |
|    | PRIN_IDLE_CNT |    |               |               |        |    |    |  |  |
|    | R/W-0h        |    |               |               |        |    |    |  |  |
| 15 | 14            | 13 | 12            | 11            | 10     | 9  | 8  |  |  |
|    | Reserved      |    | PRIN_SEND_CNT |               |        |    |    |  |  |
|    | R-0h          |    |               | R/W-0h        |        |    |    |  |  |
| 7  | 6             | 5  | 4             | 3             | 2      | 1  | 0  |  |  |
|    |               |    |               | PRIN_SEND_CNT |        |    |    |  |  |
|    |               |    |               |               |        |    |    |  |  |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-57. TX\_PRI7\_RATE Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                   |
|-------|---------------|------|-------|-------------------------------|
| 31-30 | Reserved      | R    | 0h    |                               |
| 29-16 | PRIN_IDLE_CNT | R/W  | 0h    | Priority (<br>7:0) idle count |
| 15-14 | Reserved      | R    | 0h    |                               |
| 13-0  | PRIN_SEND_CNT | R/W  | 0h    | Priority (<br>7:0) send count |

#### **14.5.2.20 TX\_INTSTAT\_RAW Register (offset = 80h) [reset = 0h]**

TX\_INTSTAT\_RAW is shown in [Figure](#page-117-1) 14-48 and described in Table [14-58](#page-117-2). CPDMA\_INT TX INTERRUPT STATUS REGISTER (RAW VALUE)

### **Figure 14-48. TX\_INTSTAT\_RAW Register**

| 31       | 30       | 29       | 28       | 27       | 26       | 25       | 24       |  |
|----------|----------|----------|----------|----------|----------|----------|----------|--|
|          |          |          | Reserved |          |          |          |          |  |
|          |          |          | R-0h     |          |          |          |          |  |
| 23       | 22       | 21       | 20       | 19       | 18       | 17       | 16       |  |
|          | Reserved |          |          |          |          |          |          |  |
| R-0h     |          |          |          |          |          |          |          |  |
| 15       | 14       | 13       | 12       | 11       | 10       | 9        | 8        |  |
|          |          |          | Reserved |          |          |          |          |  |
|          |          |          | R-0h     |          |          |          |          |  |
| 7        | 6        | 5        | 4        | 3        | 2        | 1        | 0        |  |
| TX7_PEND | TX6_PEND | TX5_PEND | TX4_PEND | TX3_PEND | TX2_PEND | TX1_PEND | TX0_PEND |  |
| R-0h     | R-0h     | R-0h     | R-0h     | R-0h     | R-0h     | R-0h     | R-0h     |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-58. TX\_INTSTAT\_RAW Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                          |
|------|----------|------|-------|--------------------------------------|
| 31-8 | Reserved | R    | 0h    |                                      |
| 7    | TX7_PEND | R    | 0h    | TX7_PEND raw int read (before mask). |
| 6    | TX6_PEND | R    | 0h    | TX6_PEND raw int read (before mask). |
| 5    | TX5_PEND | R    | 0h    | TX5_PEND raw int read (before mask). |
| 4    | TX4_PEND | R    | 0h    | TX4_PEND raw int read (before mask). |
| 3    | TX3_PEND | R    | 0h    | TX3_PEND raw int read (before mask). |
| 2    | TX2_PEND | R    | 0h    | TX2_PEND raw int read (before mask). |
| 1    | TX1_PEND | R    | 0h    | TX1_PEND raw int read (before mask). |
| 0    | TX0_PEND | R    | 0h    | TX0_PEND raw int read (before mask). |

#### **14.5.2.21 TX\_INTSTAT\_MASKED Register (offset = 84h) [reset = 0h]**

TX\_INTSTAT\_MASKED is shown in [Figure](#page-118-1) 14-49 and described in Table [14-59.](#page-118-2) CPDMA\_INT TX INTERRUPT STATUS REGISTER (MASKED VALUE)

## **Figure 14-49. TX\_INTSTAT\_MASKED Register**

| 30       | 29       | 28       | 27       | 26                                   | 25       | 24       |  |
|----------|----------|----------|----------|--------------------------------------|----------|----------|--|
|          |          |          |          |                                      |          |          |  |
|          |          |          |          |                                      |          |          |  |
| 22       | 21       | 20       | 19       | 18                                   | 17       | 16       |  |
| Reserved |          |          |          |                                      |          |          |  |
| R-0h     |          |          |          |                                      |          |          |  |
| 14       | 13       | 12       | 11       | 10                                   | 9        | 8        |  |
|          |          |          |          |                                      |          |          |  |
|          |          |          |          |                                      |          |          |  |
| 6        | 5        | 4        | 3        | 2                                    | 1        | 0        |  |
| TX6_PEND | TX5_PEND | TX4_PEND | TX3_PEND | TX2_PEND                             | TX1_PEND | TX0_PEND |  |
|          | R-0h     | R-0h     | R-0h     | R-0h                                 | R-0h     | R-0h     |  |
|          | R-0h     |          |          | Reserved<br>R-0h<br>Reserved<br>R-0h |          |          |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-59. TX\_INTSTAT\_MASKED Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                     |
|------|----------|------|-------|---------------------------------|
| 31-8 | Reserved | R    | 0h    |                                 |
| 7    | TX7_PEND | R    | 0h    | TX7_PEND masked interrupt read. |
| 6    | TX6_PEND | R    | 0h    | TX6_PEND masked interrupt read. |
| 5    | TX5_PEND | R    | 0h    | TX5_PEND masked interrupt read. |
| 4    | TX4_PEND | R    | 0h    | TX4_PEND masked interrupt read. |
| 3    | TX3_PEND | R    | 0h    | TX3_PEND masked interrupt read. |
| 2    | TX2_PEND | R    | 0h    | TX2_PEND masked interrupt read. |
| 1    | TX1_PEND | R    | 0h    | TX1_PEND masked interrupt read. |
| 0    | TX0_PEND | R    | 0h    | TX0_PEND masked interrupt read. |

#### **14.5.2.22 TX\_INTMASK\_SET Register (offset = 88h) [reset = 0h]**

TX\_INTMASK\_SET is shown in [Figure](#page-119-1) 14-50 and described in Table [14-60.](#page-119-2) CPDMA\_INT TX INTERRUPT MASK SET REGISTER

## **Figure 14-50. TX\_INTMASK\_SET Register**

| 30       | 29       | 28       | 27       | 26                                   | 25       | 24       |  |  |
|----------|----------|----------|----------|--------------------------------------|----------|----------|--|--|
|          |          |          |          |                                      |          |          |  |  |
|          |          |          |          |                                      |          |          |  |  |
| 22       | 21       | 20       | 19       | 18                                   | 17       | 16       |  |  |
| Reserved |          |          |          |                                      |          |          |  |  |
| R-0h     |          |          |          |                                      |          |          |  |  |
| 14       | 13       | 12       | 11       | 10                                   | 9        | 8        |  |  |
|          |          |          |          |                                      |          |          |  |  |
|          |          |          |          |                                      |          |          |  |  |
| 6        | 5        | 4        | 3        | 2                                    | 1        | 0        |  |  |
| TX6_MASK | TX5_MASK | TX4_MASK | TX3_MASK | TX2_MASK                             | TX1_MASK | TX0_MASK |  |  |
| R/W-0h   | R/W-0h   | R/W-0h   | R/W-0h   | R/W-0h                               | R/W-0h   | R/W-0h   |  |  |
|          |          |          |          | Reserved<br>R-0h<br>Reserved<br>R-0h |          |          |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-60. TX\_INTMASK\_SET Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                        |
|------|----------|------|-------|----------------------------------------------------|
| 31-8 | Reserved | R    | 0h    |                                                    |
| 7    | TX7_MASK | R/W  | 0h    | TX Channel 7 Mask - Write one to enable interrupt. |
| 6    | TX6_MASK | R/W  | 0h    | TX Channel 6 Mask - Write one to enable interrupt. |
| 5    | TX5_MASK | R/W  | 0h    | TX Channel 5 Mask - Write one to enable interrupt. |
| 4    | TX4_MASK | R/W  | 0h    | TX Channel 4 Mask - Write one to enable interrupt. |
| 3    | TX3_MASK | R/W  | 0h    | TX Channel 3 Mask - Write one to enable interrupt. |
| 2    | TX2_MASK | R/W  | 0h    | TX Channel 2 Mask - Write one to enable interrupt. |
| 1    | TX1_MASK | R/W  | 0h    | TX Channel 1 Mask - Write one to enable interrupt. |
| 0    | TX0_MASK | R/W  | 0h    | TX Channel 0 Mask - Write one to enable interrupt. |

#### **14.5.2.23 TX\_INTMASK\_CLEAR Register (offset = 8Ch) [reset = 0h]**

TX\_INTMASK\_CLEAR is shown in [Figure](#page-120-1) 14-51 and described in Table [14-61.](#page-120-2) CPDMA\_INT TX INTERRUPT MASK CLEAR REGISTER

## **Figure 14-51. TX\_INTMASK\_CLEAR Register**

| 31       | 30       | 29       | 28       | 27       | 26       | 25       | 24       |  |
|----------|----------|----------|----------|----------|----------|----------|----------|--|
|          |          |          | Reserved |          |          |          |          |  |
|          |          |          | R-0h     |          |          |          |          |  |
| 23       | 22       | 21       | 20       | 19       | 18       | 17       | 16       |  |
|          | Reserved |          |          |          |          |          |          |  |
|          | R-0h     |          |          |          |          |          |          |  |
| 15       | 14       | 13       | 12       | 11       | 10       | 9        | 8        |  |
|          |          |          | Reserved |          |          |          |          |  |
|          |          |          | R-0h     |          |          |          |          |  |
| 7        | 6        | 5        | 4        | 3        | 2        | 1        | 0        |  |
| TX7_MASK | TX6_MASK | TX5_MASK | TX4_MASK | TX3_MASK | TX2_MASK | TX1_MASK | TX0_MASK |  |
| R/W-0h   | R/W-0h   | R/W-0h   | R/W-0h   | R/W-0h   | R/W-0h   | R/W-0h   | R/W-0h   |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-61. TX\_INTMASK\_CLEAR Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                         |
|------|----------|------|-------|-----------------------------------------------------|
| 31-8 | Reserved | R    | 0h    |                                                     |
| 7    | TX7_MASK | R/W  | 0h    | TX Channel 7 Mask - Write one to disable interrupt. |
| 6    | TX6_MASK | R/W  | 0h    | TX Channel 6 Mask - Write one to disable interrupt. |
| 5    | TX5_MASK | R/W  | 0h    | TX Channel 5 Mask - Write one to disable interrupt. |
| 4    | TX4_MASK | R/W  | 0h    | TX Channel 4 Mask - Write one to disable interrupt. |
| 3    | TX3_MASK | R/W  | 0h    | TX Channel 3 Mask - Write one to disable interrupt. |
| 2    | TX2_MASK | R/W  | 0h    | TX Channel 2 Mask - Write one to disable interrupt. |
| 1    | TX1_MASK | R/W  | 0h    | TX Channel 1 Mask - Write one to disable interrupt. |
| 0    | TX0_MASK | R/W  | 0h    | TX Channel 0 Mask - Write one to disable interrupt. |

#### **14.5.2.24 CPDMA\_IN\_VECTOR Register (offset = 90h) [reset = 0h]**

CPDMA\_IN\_VECTOR is shown in [Figure](#page-121-1) 14-52 and described in Table [14-62](#page-121-2). CPDMA\_INT INPUT VECTOR (READ ONLY)

#### **Figure 14-52. CPDMA\_IN\_VECTOR Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

DMA\_IN\_VECTOR

R-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-62. CPDMA\_IN\_VECTOR Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                                                                                                                                                          |
|------|---------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | DMA_IN_VECTOR | R    | 0h    | DMA Input Vector - The value of DMA_In_Vector is reset to zero, but<br>will change to the IN_VECTOR bus value one clock after reset is<br>deasserted.<br>Thereafter, this value will change to a new IN_VECTOR value one<br>clock after the IN_VECTOR value changes. |

#### **14.5.2.25 CPDMA\_EOI\_VECTOR Register (offset = 94h) [reset = 0h]**

CPDMA\_EOI\_VECTOR is shown in [Figure](#page-122-1) 14-53 and described in Table [14-63](#page-122-2). CPDMA\_INT END OF INTERRUPT VECTOR

## **Figure 14-53. CPDMA\_EOI\_VECTOR Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

 7             6             5             4             3             2             1             0
┌───────────────────────────────────────────┬─────────────────────────────────────────────────────┐
│                 Reserved                  │                 DMA_EOI_VECTOR                      │
└───────────────────────────────────────────┴─────────────────────────────────────────────────────┘
                                            R-0h                             R/W-0h
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-63. CPDMA\_EOI\_VECTOR Register Field Descriptions**

| Bit  | Field          | Type | Reset | Description                                                                                                                                                                                                                                                                                       |
|------|----------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-5 | Reserved       | R    | 0h    |                                                                                                                                                                                                                                                                                                   |
| 4-0  | DMA_EOI_VECTOR | R/W  | 0h    | DMA End of Interrupt Vector - The EOI_VECTOR(<br>4:0) pins reflect the value written to this location one CLK cycle after<br>a write to this location.<br>The EOI_WR signal is asserted for a single clock cycle after a<br>latency of two CLK cycles when a write is performed to this location. |

#### **14.5.2.26 RX\_INTSTAT\_RAW Register (offset = A0h) [reset = 0h]**

RX\_INTSTAT\_RAW is shown in [Figure](#page-123-1) 14-54 and described in Table [14-64](#page-123-2). CPDMA\_INT RX INTERRUPT STATUS REGISTER (RAW VALUE)

## **Figure 14-54. RX\_INTSTAT\_RAW Register**

| 31                  | 30                  | 29                  | 28                  | 27                  | 26                  | 25                  | 24                  |  |  |
|---------------------|---------------------|---------------------|---------------------|---------------------|---------------------|---------------------|---------------------|--|--|
|                     | Reserved            |                     |                     |                     |                     |                     |                     |  |  |
|                     |                     |                     |                     | R-0h                |                     |                     |                     |  |  |
| 23                  | 22                  | 21                  | 20                  | 19                  | 18                  | 17                  | 16                  |  |  |
|                     |                     |                     |                     | Reserved            |                     |                     |                     |  |  |
|                     | R-0h                |                     |                     |                     |                     |                     |                     |  |  |
| 15                  | 14                  | 13                  | 12                  | 11                  | 10                  | 9                   | 8                   |  |  |
| RX7_THRESH<br>_PEND | RX6_THRESH<br>_PEND | RX5_THRESH<br>_PEND | RX4_THRESH<br>_PEND | RX3_THRESH<br>_PEND | RX2_THRESH<br>_PEND | RX1_THRESH<br>_PEND | RX0_THRESH<br>_PEND |  |  |
| R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                |  |  |
| 7                   | 6                   | 5                   | 4                   | 3                   | 2                   | 1                   | 0                   |  |  |
| RX7_PEND            | RX6_PEND            | RX5_PEND            | RX4_PEND            | RX3_PEND            | RX2_PEND            | RX1_PEND            | RX0_PEND            |  |  |
| R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-64. RX\_INTSTAT\_RAW Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                                 |
|-------|-----------------|------|-------|---------------------------------------------|
| 31-16 | Reserved        | R    | 0h    |                                             |
| 15    | RX7_THRESH_PEND | R    | 0h    | RX7_THRESH_PEND raw int read (before mask). |
| 14    | RX6_THRESH_PEND | R    | 0h    | RX6_THRESH_PEND raw int read (before mask). |
| 13    | RX5_THRESH_PEND | R    | 0h    | RX5_THRESH_PEND raw int read (before mask). |
| 12    | RX4_THRESH_PEND | R    | 0h    | RX4_THRESH_PEND raw int read (before mask). |
| 11    | RX3_THRESH_PEND | R    | 0h    | RX3_THRESH_PEND raw int read (before mask). |
| 10    | RX2_THRESH_PEND | R    | 0h    | RX2_THRESH_PEND raw int read (before mask). |
| 9     | RX1_THRESH_PEND | R    | 0h    | RX1_THRESH_PEND raw int read (before mask). |
| 8     | RX0_THRESH_PEND | R    | 0h    | RX0_THRESH_PEND raw int read (before mask). |
| 7     | RX7_PEND        | R    | 0h    | RX7_PEND raw int read (before mask).        |
| 6     | RX6_PEND        | R    | 0h    | RX6_PEND raw int read (before mask).        |
| 5     | RX5_PEND        | R    | 0h    | RX5_PEND raw int read (before mask).        |
| 4     | RX4_PEND        | R    | 0h    | RX4_PEND raw int read (before mask).        |
| 3     | RX3_PEND        | R    | 0h    | RX3_PEND raw int read (before mask).        |
| 2     | RX2_PEND        | R    | 0h    | RX2_PEND raw int read (before mask).        |
| 1     | RX1_PEND        | R    | 0h    | RX1_PEND raw int read (before mask).        |
| 0     | RX0_PEND        | R    | 0h    | RX0_PEND raw int read (before mask).        |

#### **14.5.2.27 RX\_INTSTAT\_MASKED Register (offset = A4h) [reset = 0h]**

RX\_INTSTAT\_MASKED is shown in [Figure](#page-124-1) 14-55 and described in Table [14-65](#page-124-2). CPDMA\_INT RX INTERRUPT STATUS REGISTER (MASKED VALUE)

#### **Figure 14-55. RX\_INTSTAT\_MASKED Register**

| 31                  | 30                  | 29                  | 28                  | 27                  | 26                  | 25                  | 24                  |  |  |
|---------------------|---------------------|---------------------|---------------------|---------------------|---------------------|---------------------|---------------------|--|--|
|                     | Reserved            |                     |                     |                     |                     |                     |                     |  |  |
|                     | R-0h                |                     |                     |                     |                     |                     |                     |  |  |
| 23                  | 22                  | 21                  | 20                  | 19                  | 18                  | 17                  | 16                  |  |  |
|                     |                     |                     |                     | Reserved            |                     |                     |                     |  |  |
|                     | R-0h                |                     |                     |                     |                     |                     |                     |  |  |
| 15                  | 14                  | 13                  | 12                  | 11                  | 10                  | 9                   | 8                   |  |  |
| RX7_THRESH<br>_PEND | RX6_THRESH<br>_PEND | RX5_THRESH<br>_PEND | RX4_THRESH<br>_PEND | RX3_THRESH<br>_PEND | RX2_THRESH<br>_PEND | RX1_THRESH<br>_PEND | RX0_THRESH<br>_PEND |  |  |
| R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                |  |  |
| 7                   | 6                   | 5                   | 4                   | 3                   | 2                   | 1                   | 0                   |  |  |
| RX7_PEND            | RX6_PEND            | RX5_PEND            | RX4_PEND            | RX3_PEND            | RX2_PEND            | RX1_PEND            | RX0_PEND            |  |  |
| R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                | R-0h                |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-65. RX\_INTSTAT\_MASKED Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                      |
|-------|-----------------|------|-------|----------------------------------|
| 31-16 | Reserved        | R    | 0h    |                                  |
| 15    | RX7_THRESH_PEND | R    | 0h    | RX7_THRESH_PEND masked int read. |
| 14    | RX6_THRESH_PEND | R    | 0h    | RX6_THRESH_PEND masked int read. |
| 13    | RX5_THRESH_PEND | R    | 0h    | RX5_THRESH_PEND masked int read. |
| 12    | RX4_THRESH_PEND | R    | 0h    | RX4_THRESH_PEND masked int read. |
| 11    | RX3_THRESH_PEND | R    | 0h    | RX3_THRESH_PEND masked int read. |
| 10    | RX2_THRESH_PEND | R    | 0h    | RX2_THRESH_PEND masked int read. |
| 9     | RX1_THRESH_PEND | R    | 0h    | RX1_THRESH_PEND masked int read. |
| 8     | RX0_THRESH_PEND | R    | 0h    | RX0_THRESH_PEND masked int read. |
| 7     | RX7_PEND        | R    | 0h    | RX7_PEND masked int read.        |
| 6     | RX6_PEND        | R    | 0h    | RX6_PEND masked int read.        |
| 5     | RX5_PEND        | R    | 0h    | RX5_PEND masked int read.        |
| 4     | RX4_PEND        | R    | 0h    | RX4_PEND masked int read.        |
| 3     | RX3_PEND        | R    | 0h    | RX3_PEND masked int read.        |
| 2     | RX2_PEND        | R    | 0h    | RX2_PEND masked int read.        |
| 1     | RX1_PEND        | R    | 0h    | RX1_PEND masked int read.        |
| 0     | RX0_PEND        | R    | 0h    | RX0_PEND masked int read.        |

#### **14.5.2.28 RX\_INTMASK\_SET Register (offset = A8h) [reset = 0h]**

RX\_INTMASK\_SET is shown in [Figure](#page-125-1) 14-56 and described in Table [14-66.](#page-125-2) CPDMA\_INT RX INTERRUPT MASK SET REGISTER

## **Figure 14-56. RX\_INTMASK\_SET Register**

| 31                       | 30                       | 29                       | 28                       | 27                       | 26                       | 25                       | 24                       |  |  |  |
|--------------------------|--------------------------|--------------------------|--------------------------|--------------------------|--------------------------|--------------------------|--------------------------|--|--|--|
|                          | Reserved                 |                          |                          |                          |                          |                          |                          |  |  |  |
|                          | R-0h                     |                          |                          |                          |                          |                          |                          |  |  |  |
| 23                       | 22                       | 21                       | 20                       | 19                       | 18                       | 17                       | 16                       |  |  |  |
|                          |                          |                          |                          | Reserved                 |                          |                          |                          |  |  |  |
|                          | R-0h                     |                          |                          |                          |                          |                          |                          |  |  |  |
| 15                       | 14                       | 13                       | 12                       | 11                       | 10                       | 9                        | 8                        |  |  |  |
| RX7_THRESH<br>_PEND_MASK | RX6_THRESH<br>_PEND_MASK | RX5_THRESH<br>_PEND_MASK | RX4_THRESH<br>_PEND_MASK | RX3_THRESH<br>_PEND_MASK | RX2_THRESH<br>_PEND_MASK | RX1_THRESH<br>_PEND_MASK | RX0_THRESH<br>_PEND_MASK |  |  |  |
| R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   |  |  |  |
| 7                        | 6                        | 5                        | 4                        | 3                        | 2                        | 1                        | 0                        |  |  |  |
| RX7_PEND_M<br>ASK        | RX6_PEND_M<br>ASK        | RX5_PEND_M<br>ASK        | RX4_PEND_M<br>ASK        | RX3_PEND_M<br>ASK        | RX2_PEND_M<br>ASK        | RX1_PEND_M<br>ASK        | RX0_PEND_M<br>ASK        |  |  |  |
| R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-66. RX\_INTMASK\_SET Register Field Descriptions**

| Bit   | Field                    | Type | Reset | Description                                                            |
|-------|--------------------------|------|-------|------------------------------------------------------------------------|
| 31-16 | Reserved                 | R    | 0h    |                                                                        |
| 15    | RX7_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 7 Threshold Pending Int.<br>Mask - Write one to enable Int. |
| 14    | RX6_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 6 Threshold Pending Int.<br>Mask - Write one to enable Int. |
| 13    | RX5_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 5 Threshold Pending Int.<br>Mask - Write one to enable Int. |
| 12    | RX4_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 4 Threshold Pending Int.<br>Mask - Write one to enable Int. |
| 11    | RX3_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 3 Threshold Pending Int.<br>Mask - Write one to enable Int. |
| 10    | RX2_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 2 Threshold Pending Int.<br>Mask - Write one to enable Int. |
| 9     | RX1_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 1 Threshold Pending Int.<br>Mask - Write one to enable Int. |
| 8     | RX0_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 0 Threshold Pending Int.<br>Mask - Write one to enable Int. |
| 7     | RX7_PEND_MASK            | R/W  | 0h    | RX Channel 7 Pending Int.<br>Mask - Write one to enable Int.           |
| 6     | RX6_PEND_MASK            | R/W  | 0h    | RX Channel 6 Pending Int.<br>Mask - Write one to enable Int.           |
| 5     | RX5_PEND_MASK            | R/W  | 0h    | RX Channel 5 Pending Int.<br>Mask - Write one to enable Int.           |
| 4     | RX4_PEND_MASK            | R/W  | 0h    | RX Channel 4 Pending Int.<br>Mask - Write one to enable Int.           |
| 3     | RX3_PEND_MASK            | R/W  | 0h    | RX Channel 3 Pending Int.<br>Mask - Write one to enable Int.           |
| 2     | RX2_PEND_MASK            | R/W  | 0h    | RX Channel 2 Pending Int.<br>Mask - Write one to enable Int.           |
| 1     | RX1_PEND_MASK            | R/W  | 0h    | RX Channel 1 Pending Int.<br>Mask - Write one to enable Int.           |

## **Table 14-66. RX\_INTMASK\_SET Register Field Descriptions (continued)**

| Bit | Field         | Type | Reset | Description                                                  |
|-----|---------------|------|-------|--------------------------------------------------------------|
| 0   | RX0_PEND_MASK | R/W  | 0h    | RX Channel 0 Pending Int.<br>Mask - Write one to enable Int. |

#### **14.5.2.29 RX\_INTMASK\_CLEAR Register (offset = ACh) [reset = 0h]**

RX\_INTMASK\_CLEAR is shown in [Figure](#page-127-1) 14-57 and described in Table [14-67.](#page-127-2) CPDMA\_INT RX INTERRUPT MASK CLEAR REGISTER

## **Figure 14-57. RX\_INTMASK\_CLEAR Register**

| 31                       | 30                       | 29                       | 28                       | 27                       | 26                       | 25                       | 24                       |  |  |  |
|--------------------------|--------------------------|--------------------------|--------------------------|--------------------------|--------------------------|--------------------------|--------------------------|--|--|--|
|                          | Reserved                 |                          |                          |                          |                          |                          |                          |  |  |  |
|                          | R-0h                     |                          |                          |                          |                          |                          |                          |  |  |  |
| 23                       | 22                       | 21                       | 20                       | 19                       | 18                       | 17                       | 16                       |  |  |  |
|                          | Reserved                 |                          |                          |                          |                          |                          |                          |  |  |  |
|                          | R-0h                     |                          |                          |                          |                          |                          |                          |  |  |  |
| 15                       | 14                       | 13                       | 12                       | 11                       | 10                       | 9                        | 8                        |  |  |  |
| RX7_THRESH<br>_PEND_MASK | RX6_THRESH<br>_PEND_MASK | RX5_THRESH<br>_PEND_MASK | RX4_THRESH<br>_PEND_MASK | RX3_THRESH<br>_PEND_MASK | RX2_THRESH<br>_PEND_MASK | RX1_THRESH<br>_PEND_MASK | RX0_THRESH<br>_PEND_MASK |  |  |  |
| R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   |  |  |  |
| 7                        | 6                        | 5                        | 4                        | 3                        | 2                        | 1                        | 0                        |  |  |  |
| RX7_PEND_M<br>ASK        | RX6_PEND_M<br>ASK        | RX5_PEND_M<br>ASK        | RX4_PEND_M<br>ASK        | RX3_PEND_M<br>ASK        | RX2_PEND_M<br>ASK        | RX1_PEND_M<br>ASK        | RX0_PEND_M<br>ASK        |  |  |  |
| R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   | R/W-0h                   |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-67. RX\_INTMASK\_CLEAR Register Field Descriptions**

| Bit   | Field                    | Type | Reset | Description                                                             |
|-------|--------------------------|------|-------|-------------------------------------------------------------------------|
| 31-16 | Reserved                 | R    | 0h    |                                                                         |
| 15    | RX7_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 7 Threshold Pending Int.<br>Mask - Write one to disable Int. |
| 14    | RX6_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 6 Threshold Pending Int.<br>Mask - Write one to disable Int. |
| 13    | RX5_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 5 Threshold Pending Int.<br>Mask - Write one to disable Int. |
| 12    | RX4_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 4 Threshold Pending Int.<br>Mask - Write one to disable Int. |
| 11    | RX3_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 3 Threshold Pending Int.<br>Mask - Write one to disable Int. |
| 10    | RX2_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 2 Threshold Pending Int.<br>Mask - Write one to disable Int. |
| 9     | RX1_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 1 Threshold Pending Int.<br>Mask - Write one to disable Int. |
| 8     | RX0_THRESH_PEND_M<br>ASK | R/W  | 0h    | RX Channel 0 Threshold Pending Int.<br>Mask - Write one to disable Int. |
| 7     | RX7_PEND_MASK            | R/W  | 0h    | RX Channel 7 Pending Int.<br>Mask - Write one to disable Int.           |
| 6     | RX6_PEND_MASK            | R/W  | 0h    | RX Channel 6 Pending Int.<br>Mask - Write one to disable Int.           |
| 5     | RX5_PEND_MASK            | R/W  | 0h    | RX Channel 5 Pending Int.<br>Mask - Write one to disable Int.           |
| 4     | RX4_PEND_MASK            | R/W  | 0h    | RX Channel 4 Pending Int.<br>Mask - Write one to disable Int.           |
| 3     | RX3_PEND_MASK            | R/W  | 0h    | RX Channel 3 Pending Int.<br>Mask - Write one to disable Int.           |
| 2     | RX2_PEND_MASK            | R/W  | 0h    | RX Channel 2 Pending Int.<br>Mask - Write one to disable Int.           |
| 1     | RX1_PEND_MASK            | R/W  | 0h    | RX Channel 1 Pending Int.<br>Mask - Write one to disable Int.           |

## **Table 14-67. RX\_INTMASK\_CLEAR Register Field Descriptions (continued)**

| Bit | Field         | Type | Reset | Description                                                   |
|-----|---------------|------|-------|---------------------------------------------------------------|
| 0   | RX0_PEND_MASK | R/W  | 0h    | RX Channel 0 Pending Int.<br>Mask - Write one to disable Int. |

#### **14.5.2.30 DMA\_INTSTAT\_RAW Register (offset = B0h) [reset = 0h]**

DMA\_INTSTAT\_RAW is shown in [Figure](#page-129-1) 14-58 and described in Table [14-68](#page-129-2). CPDMA\_INT DMA INTERRUPT STATUS REGISTER (RAW VALUE)

## **Figure 14-58. DMA\_INTSTAT\_RAW Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

 7             6             5             4             3             2             1             0
┌───────────────────────────────────────────────────────────┬─────────────────┬──────────────────┐
│                        Reserved                           │   HOST_PEND     │    STAT_PEND     │
└───────────────────────────────────────────────────────────┴─────────────────┴──────────────────┘
                                                            R-0h              R-0h
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-68. DMA\_INTSTAT\_RAW Register Field Descriptions**

| Bit  | Field     | Type | Reset | Description                                                |
|------|-----------|------|-------|------------------------------------------------------------|
| 31-2 | Reserved  | R    | 0h    |                                                            |
| 1    | HOST_PEND | R    | 0h    | Host Pending Interrupt - raw int read (before mask).       |
| 0    | STAT_PEND | R    | 0h    | Statistics Pending Interrupt - raw int read (before mask). |

#### **14.5.2.31 DMA\_INTSTAT\_MASKED Register (offset = B4h) [reset = 0h]**

DMA\_INTSTAT\_MASKED is shown in [Figure](#page-130-1) 14-59 and described in Table [14-69](#page-130-2). CPDMA\_INT DMA INTERRUPT STATUS REGISTER (MASKED VALUE)

## **Figure 14-59. DMA\_INTSTAT\_MASKED Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

 7             6             5             4             3             2             1             0
┌───────────────────────────────────────────────────────────┬─────────────────┬──────────────────┐
│                        Reserved                           │   HOST_PEND     │    STAT_PEND     │
└───────────────────────────────────────────────────────────┴─────────────────┴──────────────────┘
                                                            R-0h              R-0h
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-69. DMA\_INTSTAT\_MASKED Register Field Descriptions**

| Bit  | Field     | Type | Reset | Description                                           |
|------|-----------|------|-------|-------------------------------------------------------|
| 31-2 | Reserved  | R    | 0h    |                                                       |
| 1    | HOST_PEND | R    | 0h    | Host Pending Interrupt - masked interrupt read.       |
| 0    | STAT_PEND | R    | 0h    | Statistics Pending Interrupt - masked interrupt read. |

#### **14.5.2.32 DMA\_INTMASK\_SET Register (offset = B8h) [reset = 0h]**

DMA\_INTMASK\_SET is shown in [Figure](#page-131-1) 14-60 and described in Table [14-70](#page-131-2). CPDMA\_INT DMA INTERRUPT MASK SET REGISTER

## **Figure 14-60. DMA\_INTMASK\_SET Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

 7             6             5             4             3             2             1             0
┌───────────────────────────────────────────────────────────┬──────────────────────┬──────────────┐
│                        Reserved                           │ HOST_ERR_INT_MASK    │ STAT_INT_MASK│
└───────────────────────────────────────────────────────────┴──────────────────────┴──────────────┘
                                                            R/W-0h                 R/W-0h
```
LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-70. DMA\_INTMASK\_SET Register Field Descriptions**

| Bit  | Field             | Type | Reset | Description                                                |
|------|-------------------|------|-------|------------------------------------------------------------|
| 31-2 | Reserved          | R    | 0h    |                                                            |
| 1    | HOST_ERR_INT_MASK | R/W  | 0h    | Host Error Interrupt Mask - Write one to enable interrupt. |
| 0    | STAT_INT_MASK     | R/W  | 0h    | Statistics Interrupt Mask - Write one to enable interrupt. |

#### **14.5.2.33 DMA\_INTMASK\_CLEAR Register (offset = BCh) [reset = 0h]**

DMA\_INTMASK\_CLEAR is shown in [Figure](#page-132-1) 14-61 and described in Table [14-71](#page-132-2). CPDMA\_INT DMA INTERRUPT MASK CLEAR REGISTER

## **Figure 14-61. DMA\_INTMASK\_CLEAR Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

 7             6             5             4             3             2             1             0
┌───────────────────────────────────────────────────────────┬──────────────────────┬──────────────┐
│                        Reserved                           │ HOST_ERR_INT_MASK    │ STAT_INT_MASK│
└───────────────────────────────────────────────────────────┴──────────────────────┴──────────────┘
                                                            R/W-0h                 R/W-0h
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-71. DMA\_INTMASK\_CLEAR Register Field Descriptions**

| Bit  | Field             | Type | Reset | Description                                                 |
|------|-------------------|------|-------|-------------------------------------------------------------|
| 31-2 | Reserved          | R    | 0h    |                                                             |
| 1    | HOST_ERR_INT_MASK | R/W  | 0h    | Host Error Interrupt Mask - Write one to disable interrupt. |
| 0    | STAT_INT_MASK     | R/W  | 0h    | Statistics Interrupt Mask - Write one to disable interrupt. |

#### **14.5.2.34 RX0\_PENDTHRESH Register (offset = C0h) [reset = 0h]**

RX0\_PENDTHRESH is shown in [Figure](#page-133-1) 14-62 and described in Table [14-72.](#page-133-2) CPDMA\_INT RECEIVE THRESHOLD PENDING REGISTER CHANNEL 0

## **Figure 14-62. RX0\_PENDTHRESH Register**

| 31   | 30            | 29 | 28 | 27       | 26 | 25 | 24 |  |  |
|------|---------------|----|----|----------|----|----|----|--|--|
|      |               |    |    | Reserved |    |    |    |  |  |
| R-0h |               |    |    |          |    |    |    |  |  |
| 23   | 22            | 21 | 20 | 19       | 18 | 17 | 16 |  |  |
|      |               |    |    | Reserved |    |    |    |  |  |
|      | R-0h          |    |    |          |    |    |    |  |  |
| 15   | 14            | 13 | 12 | 11       | 10 | 9  | 8  |  |  |
|      |               |    |    | Reserved |    |    |    |  |  |
|      | R-0h          |    |    |          |    |    |    |  |  |
| 7    | 6             | 5  | 4  | 3        | 2  | 1  | 0  |  |  |
|      | RX_PENDTHRESH |    |    |          |    |    |    |  |  |
|      |               |    |    |          |    |    |    |  |  |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-72. RX0\_PENDTHRESH Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                     |
|------|---------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved      | R    | 0h    |                                                                                                                                 |
| 7-0  | RX_PENDTHRESH | R/W  | 0h    | Rx Flow Threshold - This field contains the threshold value for<br>issuing receive threshold pending interrupts (when enabled). |

#### **14.5.2.35 RX1\_PENDTHRESH Register (offset = C4h) [reset = 0h]**

RX1\_PENDTHRESH is shown in [Figure](#page-134-1) 14-63 and described in Table [14-73.](#page-134-2) CPDMA\_INT RECEIVE THRESHOLD PENDING REGISTER CHANNEL 1

## **Figure 14-63. RX1\_PENDTHRESH Register**

| 31 | 30            | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |  |
|----|---------------|----|----|----------|----|----|----|--|--|--|--|
|    | Reserved      |    |    |          |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 23 | 22            | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 15 | 14            | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    |               |    |    | R-0h     |    |    |    |  |  |  |  |
| 7  | 6             | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |  |
|    | RX_PENDTHRESH |    |    |          |    |    |    |  |  |  |  |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-73. RX1\_PENDTHRESH Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                     |
|------|---------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved      | R    | 0h    |                                                                                                                                 |
| 7-0  | RX_PENDTHRESH | R/W  | 0h    | Rx Flow Threshold - This field contains the threshold value for<br>issuing receive threshold pending interrupts (when enabled). |

#### **14.5.2.36 RX2\_PENDTHRESH Register (offset = C8h) [reset = 0h]**

RX2\_PENDTHRESH is shown in [Figure](#page-135-1) 14-64 and described in Table [14-74.](#page-135-2) CPDMA\_INT RECEIVE THRESHOLD PENDING REGISTER CHANNEL 2

## **Figure 14-64. RX2\_PENDTHRESH Register**

| 31 | 30            | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |  |
|----|---------------|----|----|----------|----|----|----|--|--|--|--|
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 23 | 22            | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 15 | 14            | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    |               |    |    | R-0h     |    |    |    |  |  |  |  |
| 7  | 6             | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |  |
|    | RX_PENDTHRESH |    |    |          |    |    |    |  |  |  |  |
|    |               |    |    |          |    |    |    |  |  |  |  |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-74. RX2\_PENDTHRESH Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                     |
|------|---------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved      | R    | 0h    |                                                                                                                                 |
| 7-0  | RX_PENDTHRESH | R/W  | 0h    | Rx Flow Threshold - This field contains the threshold value for<br>issuing receive threshold pending interrupts (when enabled). |

#### **14.5.2.37 RX3\_PENDTHRESH Register (offset = CCh) [reset = 0h]**

RX3\_PENDTHRESH is shown in [Figure](#page-136-1) 14-65 and described in Table [14-75.](#page-136-2) CPDMA\_INT RECEIVE THRESHOLD PENDING REGISTER CHANNEL 3

## **Figure 14-65. RX3\_PENDTHRESH Register**

| 31 | 30            | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |  |
|----|---------------|----|----|----------|----|----|----|--|--|--|--|
|    | Reserved      |    |    |          |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 23 | 22            | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 15 | 14            | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    |               |    |    | R-0h     |    |    |    |  |  |  |  |
| 7  | 6             | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |  |
|    | RX_PENDTHRESH |    |    |          |    |    |    |  |  |  |  |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-75. RX3\_PENDTHRESH Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                     |
|------|---------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved      | R    | 0h    |                                                                                                                                 |
| 7-0  | RX_PENDTHRESH | R/W  | 0h    | Rx Flow Threshold - This field contains the threshold value for<br>issuing receive threshold pending interrupts (when enabled). |

#### **14.5.2.38 RX4\_PENDTHRESH Register (offset = D0h) [reset = 0h]**

RX4\_PENDTHRESH is shown in [Figure](#page-137-1) 14-66 and described in Table [14-76.](#page-137-2) CPDMA\_INT RECEIVE THRESHOLD PENDING REGISTER CHANNEL 4

## **Figure 14-66. RX4\_PENDTHRESH Register**

| 31 | 30            | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |  |
|----|---------------|----|----|----------|----|----|----|--|--|--|--|
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 23 | 22            | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 15 | 14            | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    |               |    |    | R-0h     |    |    |    |  |  |  |  |
| 7  | 6             | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |  |
|    | RX_PENDTHRESH |    |    |          |    |    |    |  |  |  |  |
|    |               |    |    |          |    |    |    |  |  |  |  |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-76. RX4\_PENDTHRESH Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                     |
|------|---------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved      | R    | 0h    |                                                                                                                                 |
| 7-0  | RX_PENDTHRESH | R/W  | 0h    | Rx Flow Threshold - This field contains the threshold value for<br>issuing receive threshold pending interrupts (when enabled). |

#### **14.5.2.39 RX5\_PENDTHRESH Register (offset = D4h) [reset = 0h]**

RX5\_PENDTHRESH is shown in [Figure](#page-138-1) 14-67 and described in Table [14-77.](#page-138-2) CPDMA\_INT RECEIVE THRESHOLD PENDING REGISTER CHANNEL 5

## **Figure 14-67. RX5\_PENDTHRESH Register**

| 31 | 30            | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |  |
|----|---------------|----|----|----------|----|----|----|--|--|--|--|
|    | Reserved      |    |    |          |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 23 | 22            | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 15 | 14            | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    |               |    |    | R-0h     |    |    |    |  |  |  |  |
| 7  | 6             | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |  |
|    | RX_PENDTHRESH |    |    |          |    |    |    |  |  |  |  |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-77. RX5\_PENDTHRESH Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                     |
|------|---------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved      | R    | 0h    |                                                                                                                                 |
| 7-0  | RX_PENDTHRESH | R/W  | 0h    | Rx Flow Threshold - This field contains the threshold value for<br>issuing receive threshold pending interrupts (when enabled). |

#### **14.5.2.40 RX6\_PENDTHRESH Register (offset = D8h) [reset = 0h]**

RX6\_PENDTHRESH is shown in [Figure](#page-139-1) 14-68 and described in Table [14-78.](#page-139-2) CPDMA\_INT RECEIVE THRESHOLD PENDING REGISTER CHANNEL 6

## **Figure 14-68. RX6\_PENDTHRESH Register**

| 31 | 30            | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |  |
|----|---------------|----|----|----------|----|----|----|--|--|--|--|
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 23 | 22            | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 15 | 14            | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    |               |    |    | R-0h     |    |    |    |  |  |  |  |
| 7  | 6             | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |  |
|    | RX_PENDTHRESH |    |    |          |    |    |    |  |  |  |  |
|    |               |    |    |          |    |    |    |  |  |  |  |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-78. RX6\_PENDTHRESH Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                     |
|------|---------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved      | R    | 0h    |                                                                                                                                 |
| 7-0  | RX_PENDTHRESH | R/W  | 0h    | Rx Flow Threshold - This field contains the threshold value for<br>issuing receive threshold pending interrupts (when enabled). |

#### **14.5.2.41 RX7\_PENDTHRESH Register (offset = DCh) [reset = 0h]**

RX7\_PENDTHRESH is shown in [Figure](#page-140-1) 14-69 and described in Table [14-79.](#page-140-2) CPDMA\_INT RECEIVE THRESHOLD PENDING REGISTER CHANNEL 7

## **Figure 14-69. RX7\_PENDTHRESH Register**

| 31 | 30            | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |  |
|----|---------------|----|----|----------|----|----|----|--|--|--|--|
|    | Reserved      |    |    |          |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 23 | 22            | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    | R-0h          |    |    |          |    |    |    |  |  |  |  |
| 15 | 14            | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |  |
|    |               |    |    | Reserved |    |    |    |  |  |  |  |
|    |               |    |    | R-0h     |    |    |    |  |  |  |  |
| 7  | 6             | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |  |
|    | RX_PENDTHRESH |    |    |          |    |    |    |  |  |  |  |

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-79. RX7\_PENDTHRESH Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                     |
|------|---------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved      | R    | 0h    |                                                                                                                                 |
| 7-0  | RX_PENDTHRESH | R/W  | 0h    | Rx Flow Threshold - This field contains the threshold value for<br>issuing receive threshold pending interrupts (when enabled). |

#### **14.5.2.42 RX0\_FREEBUFFER Register (offset = E0h) [reset = 0h]**

RX0\_FREEBUFFER is shown in [Figure](#page-141-1) 14-70 and described in Table [14-80.](#page-141-2) CPDMA\_INT RECEIVE FREE BUFFER REGISTER CHANNEL 0

## **Figure 14-70. RX0\_FREEBUFFER Register**

| 31 | 30 | 29 | 28 | 27            | 26 | 25 | 24 |
|----|----|----|----|---------------|----|----|----|
|    |    |    |    | Reserved      |    |    |    |
|    |    |    |    | R-0h          |    |    |    |
| 23 | 22 | 21 | 20 | 19            | 18 | 17 | 16 |
|    |    |    |    | Reserved      |    |    |    |
|    |    |    |    | R-0h          |    |    |    |
| 15 | 14 | 13 | 12 | 11            | 10 | 9  | 8  |
|    |    |    |    | RX_FREEBUFFER |    |    |    |
|    |    |    |    | W-0h          |    |    |    |
| 7  | 6  | 5  | 4  | 3             | 2  | 1  | 0  |
|    |    |    |    | RX_FREEBUFFER |    |    |    |
|    |    |    |    |               |    |    |    |

W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-80. RX0\_FREEBUFFER Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------|---------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | Reserved      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 15-0  | RX_FREEBUFFER | W    | 0h    | Rx Free Buffer Count - This field contains the count of free buffers<br>available.<br>The rx_pendthresh value is compared with this field to determine if<br>the receive threshold pending interrupt should be asseted (if<br>enabled).<br>This is a write to increment field.<br>This field rolls over to zero on overflow.<br>If receive threshold pending interrupts are used, the host must<br>initialize this field to the number of available buffers (one register per<br>channel).<br>The port decrements (by the number of buffers in the received<br>frame) the associated channel register for each received frame.<br>This is a write to increment field.<br>The host must write this field with the number of buffers that have<br>been freed due to host processing. |

#### **14.5.2.43 RX1\_FREEBUFFER Register (offset = E4h) [reset = 0h]**

RX1\_FREEBUFFER is shown in [Figure](#page-142-1) 14-71 and described in Table [14-81.](#page-142-2) CPDMA\_INT RECEIVE FREE BUFFER REGISTER CHANNEL 1

## **Figure 14-71. RX1\_FREEBUFFER Register**

| 31 | 30       | 29 | 28 | 27            | 26 | 25 | 24 |  |  |  |
|----|----------|----|----|---------------|----|----|----|--|--|--|
|    | Reserved |    |    |               |    |    |    |  |  |  |
|    | R-0h     |    |    |               |    |    |    |  |  |  |
| 23 | 22       | 21 | 20 | 19            | 18 | 17 | 16 |  |  |  |
|    |          |    |    | Reserved      |    |    |    |  |  |  |
|    | R-0h     |    |    |               |    |    |    |  |  |  |
| 15 | 14       | 13 | 12 | 11            | 10 | 9  | 8  |  |  |  |
|    |          |    |    | RX_FREEBUFFER |    |    |    |  |  |  |
|    |          |    |    | W-0h          |    |    |    |  |  |  |
| 7  | 6        | 5  | 4  | 3             | 2  | 1  | 0  |  |  |  |
|    |          |    |    | RX_FREEBUFFER |    |    |    |  |  |  |
|    |          |    |    |               |    |    |    |  |  |  |

W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-81. RX1\_FREEBUFFER Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------|---------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | Reserved      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 15-0  | RX_FREEBUFFER | W    | 0h    | Rx Free Buffer Count - This field contains the count of free buffers<br>available.<br>The rx_pendthresh value is compared with this field to determine if<br>the receive threshold pending interrupt should be asseted (if<br>enabled).<br>This is a write to increment field.<br>This field rolls over to zero on overflow.<br>If receive threshold pending interrupts are used, the host must<br>initialize this field to the number of available buffers (one register per<br>channel).<br>The port decrements (by the number of buffers in the received<br>frame) the associated channel register for each received frame.<br>This is a write to increment field.<br>The host must write this field with the number of buffers that have<br>been freed due to host processing. |

#### **14.5.2.44 RX2\_FREEBUFFER Register (offset = E8h) [reset = 0h]**

RX2\_FREEBUFFER is shown in [Figure](#page-143-1) 14-72 and described in Table [14-82.](#page-143-2) CPDMA\_INT RECEIVE FREE BUFFER REGISTER CHANNEL 2

## **Figure 14-72. RX2\_FREEBUFFER Register**

| 31 | 30 | 29 | 28 | 27            | 26 | 25 | 24 |
|----|----|----|----|---------------|----|----|----|
|    |    |    |    | Reserved      |    |    |    |
|    |    |    |    | R-0h          |    |    |    |
| 23 | 22 | 21 | 20 | 19            | 18 | 17 | 16 |
|    |    |    |    | Reserved      |    |    |    |
|    |    |    |    | R-0h          |    |    |    |
| 15 | 14 | 13 | 12 | 11            | 10 | 9  | 8  |
|    |    |    |    | RX_FREEBUFFER |    |    |    |
|    |    |    |    | W-0h          |    |    |    |
| 7  | 6  | 5  | 4  | 3             | 2  | 1  | 0  |
|    |    |    |    | RX_FREEBUFFER |    |    |    |
|    |    |    |    |               |    |    |    |

W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-82. RX2\_FREEBUFFER Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------|---------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | Reserved      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 15-0  | RX_FREEBUFFER | W    | 0h    | Rx Free Buffer Count - This field contains the count of free buffers<br>available.<br>The rx_pendthresh value is compared with this field to determine if<br>the receive threshold pending interrupt should be asseted (if<br>enabled).<br>This is a write to increment field.<br>This field rolls over to zero on overflow.<br>If receive threshold pending interrupts are used, the host must<br>initialize this field to the number of available buffers (one register per<br>channel).<br>The port decrements (by the number of buffers in the received<br>frame) the associated channel register for each received frame.<br>This is a write to increment field.<br>The host must write this field with the number of buffers that have<br>been freed due to host processing. |

#### **14.5.2.45 RX3\_FREEBUFFER Register (offset = ECh) [reset = 0h]**

RX3\_FREEBUFFER is shown in [Figure](#page-144-1) 14-73 and described in Table [14-83.](#page-144-2) CPDMA\_INT RECEIVE FREE BUFFER REGISTER CHANNEL 3

## **Figure 14-73. RX3\_FREEBUFFER Register**

| 31 | 30 | 29 | 28 | 27            | 26 | 25 | 24 |
|----|----|----|----|---------------|----|----|----|
|    |    |    |    | Reserved      |    |    |    |
|    |    |    |    | R-0h          |    |    |    |
| 23 | 22 | 21 | 20 | 19            | 18 | 17 | 16 |
|    |    |    |    | Reserved      |    |    |    |
|    |    |    |    | R-0h          |    |    |    |
| 15 | 14 | 13 | 12 | 11            | 10 | 9  | 8  |
|    |    |    |    | RX_FREEBUFFER |    |    |    |
|    |    |    |    | W-0h          |    |    |    |
| 7  | 6  | 5  | 4  | 3             | 2  | 1  | 0  |
|    |    |    |    | RX_FREEBUFFER |    |    |    |
|    |    |    |    | W-0h          |    |    |    |
|    |    |    |    |               |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-83. RX3\_FREEBUFFER Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------|---------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | Reserved      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 15-0  | RX_FREEBUFFER | W    | 0h    | Rx Free Buffer Count - This field contains the count of free buffers<br>available.<br>The rx_pendthresh value is compared with this field to determine if<br>the receive threshold pending interrupt should be asseted (if<br>enabled).<br>This is a write to increment field.<br>This field rolls over to zero on overflow.<br>If receive threshold pending interrupts are used, the host must<br>initialize this field to the number of available buffers (one register per<br>channel).<br>The port decrements (by the number of buffers in the received<br>frame) the associated channel register for each received frame.<br>This is a write to increment field.<br>The host must write this field with the number of buffers that have<br>been freed due to host processing. |

#### **14.5.2.46 RX4\_FREEBUFFER Register (offset = F0h) [reset = 0h]**

RX4\_FREEBUFFER is shown in [Figure](#page-145-1) 14-74 and described in Table [14-84.](#page-145-2) CPDMA\_INT RECEIVE FREE BUFFER REGISTER CHANNEL 4

## **Figure 14-74. RX4\_FREEBUFFER Register**

| 31 | 30 | 29 | 28 | 27            | 26 | 25 | 24 |
|----|----|----|----|---------------|----|----|----|
|    |    |    |    | Reserved      |    |    |    |
|    |    |    |    | R-0h          |    |    |    |
| 23 | 22 | 21 | 20 | 19            | 18 | 17 | 16 |
|    |    |    |    | Reserved      |    |    |    |
|    |    |    |    | R-0h          |    |    |    |
| 15 | 14 | 13 | 12 | 11            | 10 | 9  | 8  |
|    |    |    |    | RX_FREEBUFFER |    |    |    |
|    |    |    |    | W-0h          |    |    |    |
| 7  | 6  | 5  | 4  | 3             | 2  | 1  | 0  |
|    |    |    |    | RX_FREEBUFFER |    |    |    |
|    |    |    |    |               |    |    |    |

W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-84. RX4\_FREEBUFFER Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------|---------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | Reserved      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 15-0  | RX_FREEBUFFER | W    | 0h    | Rx Free Buffer Count - This field contains the count of free buffers<br>available.<br>The rx_pendthresh value is compared with this field to determine if<br>the receive threshold pending interrupt should be asseted (if<br>enabled).<br>This is a write to increment field.<br>This field rolls over to zero on overflow.<br>If receive threshold pending interrupts are used, the host must<br>initialize this field to the number of available buffers (one register per<br>channel).<br>The port decrements (by the number of buffers in the received<br>frame) the associated channel register for each received frame.<br>This is a write to increment field.<br>The host must write this field with the number of buffers that have<br>been freed due to host processing. |

#### **14.5.2.47 RX5\_FREEBUFFER Register (offset = F4h) [reset = 0h]**

RX5\_FREEBUFFER is shown in [Figure](#page-146-1) 14-75 and described in Table [14-85.](#page-146-2) CPDMA\_INT RECEIVE FREE BUFFER REGISTER CHANNEL 5

## **Figure 14-75. RX5\_FREEBUFFER Register**

| 31 | 30   | 29 | 28 | 27            | 26 | 25 | 24 |  |  |  |
|----|------|----|----|---------------|----|----|----|--|--|--|
|    |      |    |    | Reserved      |    |    |    |  |  |  |
|    | R-0h |    |    |               |    |    |    |  |  |  |
| 23 | 22   | 21 | 20 | 19            | 18 | 17 | 16 |  |  |  |
|    |      |    |    | Reserved      |    |    |    |  |  |  |
|    | R-0h |    |    |               |    |    |    |  |  |  |
| 15 | 14   | 13 | 12 | 11            | 10 | 9  | 8  |  |  |  |
|    |      |    |    | RX_FREEBUFFER |    |    |    |  |  |  |
|    |      |    |    | W-0h          |    |    |    |  |  |  |
| 7  | 6    | 5  | 4  | 3             | 2  | 1  | 0  |  |  |  |
|    |      |    |    | RX_FREEBUFFER |    |    |    |  |  |  |
|    |      |    |    |               |    |    |    |  |  |  |

W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-85. RX5\_FREEBUFFER Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------|---------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | Reserved      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 15-0  | RX_FREEBUFFER | W    | 0h    | Rx Free Buffer Count - This field contains the count of free buffers<br>available.<br>The rx_pendthresh value is compared with this field to determine if<br>the receive threshold pending interrupt should be asseted (if<br>enabled).<br>This is a write to increment field.<br>This field rolls over to zero on overflow.<br>If receive threshold pending interrupts are used, the host must<br>initialize this field to the number of available buffers (one register per<br>channel).<br>The port decrements (by the number of buffers in the received<br>frame) the associated channel register for each received frame.<br>This is a write to increment field.<br>The host must write this field with the number of buffers that have<br>been freed due to host processing. |

#### **14.5.2.48 RX6\_FREEBUFFER Register (offset = F8h) [reset = 0h]**

RX6\_FREEBUFFER is shown in [Figure](#page-147-1) 14-76 and described in Table [14-86.](#page-147-2) CPDMA\_INT RECEIVE FREE BUFFER REGISTER CHANNEL 6

## **Figure 14-76. RX6\_FREEBUFFER Register**

| 31 | 30 | 29 | 28 | 27            | 26 | 25 | 24 |
|----|----|----|----|---------------|----|----|----|
|    |    |    |    | Reserved      |    |    |    |
|    |    |    |    | R-0h          |    |    |    |
| 23 | 22 | 21 | 20 | 19            | 18 | 17 | 16 |
|    |    |    |    | Reserved      |    |    |    |
|    |    |    |    | R-0h          |    |    |    |
| 15 | 14 | 13 | 12 | 11            | 10 | 9  | 8  |
|    |    |    |    | RX_FREEBUFFER |    |    |    |
|    |    |    |    | W-0h          |    |    |    |
| 7  | 6  | 5  | 4  | 3             | 2  | 1  | 0  |
|    |    |    |    | RX_FREEBUFFER |    |    |    |
|    |    |    |    |               |    |    |    |

W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-86. RX6\_FREEBUFFER Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------|---------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | Reserved      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 15-0  | RX_FREEBUFFER | W    | 0h    | Rx Free Buffer Count - This field contains the count of free buffers<br>available.<br>The rx_pendthresh value is compared with this field to determine if<br>the receive threshold pending interrupt should be asseted (if<br>enabled).<br>This is a write to increment field.<br>This field rolls over to zero on overflow.<br>If receive threshold pending interrupts are used, the host must<br>initialize this field to the number of available buffers (one register per<br>channel).<br>The port decrements (by the number of buffers in the received<br>frame) the associated channel register for each received frame.<br>This is a write to increment field.<br>The host must write this field with the number of buffers that have<br>been freed due to host processing. |

#### **14.5.2.49 RX7\_FREEBUFFER Register (offset = FCh) [reset = 0h]**

RX7\_FREEBUFFER is shown in [Figure](#page-148-1) 14-77 and described in Table [14-87.](#page-148-2) CPDMA\_INT RECEIVE FREE BUFFER REGISTER CHANNEL 7

**Figure 14-77. RX7\_FREEBUFFER Register**

| 31 | 30       | 29 | 28 | 27            | 26 | 25 | 24 |  |  |  |
|----|----------|----|----|---------------|----|----|----|--|--|--|
|    | Reserved |    |    |               |    |    |    |  |  |  |
|    | R-0h     |    |    |               |    |    |    |  |  |  |
| 23 | 22       | 21 | 20 | 19            | 18 | 17 | 16 |  |  |  |
|    |          |    |    | Reserved      |    |    |    |  |  |  |
|    | R-0h     |    |    |               |    |    |    |  |  |  |
| 15 | 14       | 13 | 12 | 11            | 10 | 9  | 8  |  |  |  |
|    |          |    |    | RX_FREEBUFFER |    |    |    |  |  |  |
|    | W-0h     |    |    |               |    |    |    |  |  |  |
| 7  | 6        | 5  | 4  | 3             | 2  | 1  | 0  |  |  |  |
|    |          |    |    | RX_FREEBUFFER |    |    |    |  |  |  |
|    |          |    |    |               |    |    |    |  |  |  |

W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-87. RX7\_FREEBUFFER Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------|---------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | Reserved      | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 15-0  | RX_FREEBUFFER | W    | 0h    | Rx Free Buffer Count - This field contains the count of free buffers<br>available.<br>The rx_pendthresh value is compared with this field to determine if<br>the receive threshold pending interrupt should be asseted (if<br>enabled).<br>This is a write to increment field.<br>This field rolls over to zero on overflow.<br>If receive threshold pending interrupts are used, the host must<br>initialize this field to the number of available buffers (one register per<br>channel).<br>The port decrements (by the number of buffers in the received<br>frame) the associated channel register for each received frame.<br>This is a write to increment field.<br>The host must write this field with the number of buffers that have<br>been freed due to host processing. |

### *14.5.3 CPSW\_CPTS Registers*

Table [14-88](#page-148-3) lists the memory-mapped registers for the CPSW\_CPTS. All register offset addresses not listed in Table [14-88](#page-148-3) should be considered as reserved locations and the register contents should not be modified.

**Table 14-88. CPSW\_CPTS REGISTERS**

| Offset | Acronym             | Register Name                                 | Section          |
|--------|---------------------|-----------------------------------------------|------------------|
| 0h     | CPTS_IDVER          | IDENTIFICATION AND VERSION REGISTER           | Section 14.5.3.1 |
| 4h     | CPTS_CONTROL        | TIME SYNC CONTROL REGISTER                    | Section 14.5.3.2 |
| Ch     | CPTS_TS_PUSH        | TIME STAMP EVENT PUSH REGISTER                | Section 14.5.3.3 |
| 10h    | CPTS_TS_LOAD_VAL    | TIME STAMP LOAD VALUE REGISTER                | Section 14.5.3.4 |
| 14h    | CPTS_TS_LOAD_EN     | TIME STAMP LOAD ENABLE REGISTER               | Section 14.5.3.5 |
| 20h    | CPTS_INTSTAT_RAW    | TIME SYNC INTERRUPT STATUS RAW REGISTER       | Section 14.5.3.6 |
| 24h    | CPTS_INTSTAT_MASKED | TIME SYNC INTERRUPT STATUS MASKED<br>REGISTER | Section 14.5.3.7 |

## **Table 14-88. CPSW\_CPTS REGISTERS (continued)**

| Offset | Acronym         | Register Name                       | Section           |
|--------|-----------------|-------------------------------------|-------------------|
| 28h    | CPTS_INT_ENABLE | TIME SYNC INTERRUPT ENABLE REGISTER | Section 14.5.3.8  |
| 30h    | CPTS_EVENT_POP  | EVENT INTERRUPT POP REGISTER        | Section 14.5.3.9  |
| 34h    | CPTS_EVENT_LOW  | LOWER 32-BITS OF THE EVENT VALUE    | Section 14.5.3.10 |
| 38h    | CPTS_EVENT_HIGH | UPPER 32-BITS OF THE EVENT VALUE    | Section 14.5.3.11 |

#### **14.5.3.1 CPTS\_IDVER Register (offset = 0h) [reset = 4E8A0101h]**

CPTS\_IDVER is shown in [Figure](#page-150-1) 14-78 and described in Table [14-89.](#page-150-2)

IDENTIFICATION AND VERSION REGISTER

#### **Figure 14-78. CPTS\_IDVER Register**

| 31       | 30 | 29      | 28 | 27        | 26 | 25        | 24 |  |  |
|----------|----|---------|----|-----------|----|-----------|----|--|--|
| TX_IDENT |    |         |    |           |    |           |    |  |  |
| R-4E8Ah  |    |         |    |           |    |           |    |  |  |
| 23       | 22 | 21      | 20 | 19        | 18 | 17        | 16 |  |  |
|          |    |         |    | TX_IDENT  |    |           |    |  |  |
| R-4E8Ah  |    |         |    |           |    |           |    |  |  |
| 15       | 14 | 13      | 12 | 11        | 10 | 9         | 8  |  |  |
|          |    | RTL_VER |    | MAJOR_VER |    |           |    |  |  |
| R-9D140h |    |         |    |           |    | R-4E8A01h |    |  |  |
| 7        | 6  | 5       | 4  | 3         | 2  | 1         | 0  |  |  |
|          |    |         |    | MINOR_VER |    |           |    |  |  |
|          |    |         |    |           |    |           |    |  |  |

R-4E8A0101h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-89. CPTS\_IDVER Register Field Descriptions**

| Bit   | Field     | Type | Reset     | Description             |
|-------|-----------|------|-----------|-------------------------|
| 31-16 | TX_IDENT  | R    | 4E8Ah     | TX Identification Value |
| 15-11 | RTL_VER   | R    | 9D140h    | RTL Version Value       |
| 10-8  | MAJOR_VER | R    | 4E8A01h   | Major Version Value     |
| 7-0   | MINOR_VER | R    | 4E8A0101h | Minor Version Value     |

#### **14.5.3.2 CPTS\_CONTROL Register (offset = 4h) [reset = 0h]**

CPTS\_CONTROL is shown in [Figure](#page-151-1) 14-79 and described in Table [14-90](#page-151-2). TIME SYNC CONTROL REGISTER

## **Figure 14-79. CPTS\_CONTROL Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

 7             6             5             4             3             2             1             0
┌───────────────────────────────────────────────────────────┬─────────────────────────────────────┐
│                        Reserved                           │                TS_PUSH              │
└───────────────────────────────────────────────────────────┴─────────────────────────────────────┘
                                                            W-0h
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-90. CPTS\_CONTROL Register Field Descriptions**

| Bit   | Field          | Type | Reset | Description                                                                                                                                 |
|-------|----------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------|
| 31-12 | Reserved       | R    | 0h    |                                                                                                                                             |
| 11    | HW4_TS_PUSH_EN | R/W  | 0h    | Hardware push 4 enable                                                                                                                      |
| 10    | HW3_TS_PUSH_EN | R/W  | 0h    | Hardware push 3 enable                                                                                                                      |
| 9     | HW2_TS_PUSH_EN | R/W  | 0h    | Hardware push 2 enable                                                                                                                      |
| 8     | HW1_TS_PUSH_EN | R/W  | 0h    | Hardware push 1 enable                                                                                                                      |
| 7-2   | Reserved       | R    | 0h    |                                                                                                                                             |
| 1     | INT_TEST       | R/W  | 0h    | Interrupt Test - When set, this bit allows the raw interrupt to be<br>written to facilitate interrupt test.                                 |
| 0     | CPTS_EN        | R/W  | 0h    | Time Sync Enable - When disabled (cleared to zero), the RCLK<br>domain is held in reset.<br>0 - Time Sync Disabled<br>1 - Time Sync Enabled |

#### **14.5.3.3 CPTS\_TS\_PUSH Register (offset = Ch) [reset = 0h]**

CPTS\_TS\_PUSH is shown in [Figure](#page-152-1) 14-80 and described in Table [14-91](#page-152-2).

TIME STAMP EVENT PUSH REGISTER

### **Figure 14-80. CPTS\_TS\_PUSH Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-91. CPTS\_TS\_PUSH Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | Reserved | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 0    | TS_PUSH  | W    | 0h    | Time stamp event push - When a logic high is written to this bit a<br>time stamp event is pushed onto the event FIFO.<br>The time stamp value is the time of the write of this register, not the<br>time of the event read.<br>The time stamp value can then be read on interrupt via the event<br>registers.<br>Software should not push a second time stamp event onto the event<br>FIFO until the first time stamp value has been read from the event<br>FIFO (there should be only one time stamp event in the event FIFO<br>at any given time).<br>This bit is write only and always reads zero. |

#### **14.5.3.4 CPTS\_TS\_LOAD\_VAL Register (offset = 10h) [reset = 0h]**

CPTS\_TS\_LOAD\_VAL is shown in [Figure](#page-153-1) 14-81 and described in Table [14-92](#page-153-2).

TIME STAMP LOAD VALUE REGISTER

## **Figure 14-81. CPTS\_TS\_LOAD\_VAL Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

TS\_LOAD\_VAL

R/W-0h

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-92. CPTS\_TS\_LOAD\_VAL Register Field Descriptions**

| Bit  | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                   |
|------|-------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TS_LOAD_VAL | R/W  | 0h    | Time Stamp Load Value - Writing the ts_load_en bit causes the<br>value contained in this register to be written into the time stamp.<br>The time stamp value is read by initiating a time stamp push event,<br>not by reading this register.<br>When reading this register, the value read is not the time stamp, but<br>is the value that was last written to this register. |

#### **14.5.3.5 CPTS\_TS\_LOAD\_EN Register (offset = 14h) [reset = 0h]**

CPTS\_TS\_LOAD\_EN is shown in [Figure](#page-154-1) 14-82 and described in Table [14-93](#page-154-2).

TIME STAMP LOAD ENABLE REGISTER

## **Figure 14-82. CPTS\_TS\_LOAD\_EN Register**

```
31            30            29            28            27            26            25            24
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

23            22            21            20            19            18            17            16
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

15            14            13            12            11            10             9             8
┌──────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           Reserved                                               │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
                                                R-0h

 7             6             5             4             3             2             1             0
┌───────────────────────────────────────────────────────────┬─────────────────────────────────────┐
│                        Reserved                           │              TS_LOAD_EN             │
└───────────────────────────────────────────────────────────┴─────────────────────────────────────┘
                                                            W-0h
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-93. CPTS\_TS\_LOAD\_EN Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                                 |
|------|------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | Reserved   | R    | 0h    |                                                                                                                                                                                                             |
| 0    | TS_LOAD_EN | W    | 0h    | Time Stamp Load - Writing a one to this bit enables the time stamp<br>value to be written via the ts_load_val[<br>31:0] register.<br>This feature is included for test purposes.<br>This bit is write only. |

#### **14.5.3.6 CPTS\_INTSTAT\_RAW Register (offset = 20h) [reset = 0h]**

CPTS\_INTSTAT\_RAW is shown in [Figure](#page-0-0) 14-83 and described in Table [14-94.](#page-0-1)

TIME SYNC INTERRUPT STATUS RAW REGISTER

### **Figure 14-83. CPTS\_INTSTAT\_RAW Register**

```
|                |                  |                  |                  |TS_|
|    Reserved    |     Reserved     |     Reserved     |     Reserved     |PEN|
|                |                  |                  |                  |D_R|
+----------------+------------------+------------------+------------------+AW |
   R-0h              R-0h               R-0h               R-0h             R/W-0h
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-94. CPTS\_INTSTAT\_RAW Register Field Descriptions**

| Bit  | Field       | Type | Reset | Description                                                                                                                                            |
|------|-------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | Reserved    | R    | 0h    |                                                                                                                                                        |
| 0    | TS_PEND_RAW | R/W  | 0h    | TS_PEND_RAW int read (before enable).<br>Writable when int_test = 1 A one in this bit indicates that there is one<br>or more events in the event FIFO. |

#### **14.5.3.7 CPTS\_INTSTAT\_MASKED Register (offset = 24h) [reset = 0h]**

CPTS\_INTSTAT\_MASKED is shown in [Figure](#page-1-0) 14-84 and described in Table [14-95.](#page-1-1)

TIME SYNC INTERRUPT STATUS MASKED REGISTER

**Figure 14-84. CPTS\_INTSTAT\_MASKED Register**

```
|                |                  |                  |      |   |   |  |TS_|
|    Reserved    |     Reserved     |     Reserved     |Reser-|   |   |  |PEN|
|                |                  |                  |ved   |   |   |  |D  |
+----------------+------------------+------------------+------+---+---+--+---+-+
   R-0h              R-0h               R-0h                R-0h               R-0h
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-95. CPTS\_INTSTAT\_MASKED Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                   |
|------|----------|------|-------|-----------------------------------------------|
| 31-1 | Reserved | R    | 0h    |                                               |
| 0    | TS_PEND  | R    | 0h    | TS_PEND masked interrupt read (after enable). |

#### **14.5.3.8 CPTS\_INT\_ENABLE Register (offset = 28h) [reset = 0h]**

CPTS\_INT\_ENABLE is shown in [Figure](#page-2-0) 14-85 and described in Table [14-96](#page-2-1).

TIME SYNC INTERRUPT ENABLE REGISTER

**Figure 14-85. CPTS\_INT\_ENABLE Register**

```
|                |                  |                  |Rese|   |   |  |TS_|
|    Reserved    |     Reserved     |     Reserved     |rved|   |   |  |PEN|
|                |                  |                  |    |   |   |  |D_E|
+----------------+------------------+------------------+----+---+---+--+---+-+
   R-0h              R-0h               R-0h               R-0h              R/W-0h
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-96. CPTS\_INT\_ENABLE Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                      |
|------|------------|------|-------|----------------------------------|
| 31-1 | Reserved   | R    | 0h    |                                  |
| 0    | TS_PEND_EN | R/W  | 0h    | TS_PEND masked interrupt enable. |

#### **14.5.3.9 CPTS\_EVENT\_POP Register (offset = 30h) [reset = 0h]**

CPTS\_EVENT\_POP is shown in [Figure](#page-3-0) 14-86 and described in Table [14-97](#page-3-1). EVENT INTERRUPT POP REGISTER

**Figure 14-86. CPTS\_EVENT\_POP Register**

| 31 | 30 | 29 | 28       | 27       | 26 | 25 | 24        |
|----|----|----|----------|----------|----|----|-----------|
|    |    |    |          | Reserved |    |    |           |
|    |    |    |          | R-0h     |    |    |           |
| 23 | 22 | 21 | 20       | 19       | 18 | 17 | 16        |
|    |    |    |          | Reserved |    |    |           |
|    |    |    |          | R-0h     |    |    |           |
| 15 | 14 | 13 | 12       | 11       | 10 | 9  | 8         |
|    |    |    |          | Reserved |    |    |           |
|    |    |    |          | R-0h     |    |    |           |
| 7  | 6  | 5  | 4        | 3        | 2  | 1  | 0         |
|    |    |    | Reserved |          |    |    | EVENT_POP |
|    |    |    | R-0h     |          |    |    | W-0h      |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-97. CPTS\_EVENT\_POP Register Field Descriptions**

| Bit  | Field     | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                            |
|------|-----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | Reserved  | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                        |
| 0    | EVENT_POP | W    | 0h    | Event Pop - When a logic high is written to this bit an event is<br>popped off the event FIFO.<br>The event FIFO pop occurs as part of the interrupt process after the<br>event has been read in the Event_Low and Event_High registers.<br>Popping an event discards the event and causes the next event, if<br>any, to be moved to the top of the FIFO ready to be read by software<br>on interrupt. |

#### **14.5.3.10 CPTS\_EVENT\_LOW Register (offset = 34h) [reset = 0h]**

CPTS\_EVENT\_LOW is shown in [Figure](#page-4-0) 14-87 and described in Table [14-98.](#page-4-1) LOWER 32-BITS OF THE EVENT VALUE

### **Figure 14-87. CPTS\_EVENT\_LOW Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-98. CPTS\_EVENT\_LOW Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                      |
|------|------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TIME_STAMP | R    | 0h    | Time Stamp - The timestamp is valid for transmit, receive, and time<br>stamp push event types.<br>The timestamp value is not valid for counter roll event types. |

#### **14.5.3.11 CPTS\_EVENT\_HIGH Register (offset = 38h) [reset = 0h]**

CPTS\_EVENT\_HIGH is shown in [Figure](#page-5-0) 14-88 and described in Table [14-99](#page-5-1). UPPER 32-BITS OF THE EVENT VALUE

**Figure 14-88. CPTS\_EVENT\_HIGH Register**

| 31 | 30          | 29 | 28 | 27          | 26           | 25 | 24 |
|----|-------------|----|----|-------------|--------------|----|----|
|    | Reserved    |    |    |             | PORT_NUMBER  |    |    |
|    | R-0h        |    |    |             | R-0h         |    |    |
| 23 | 22          | 21 | 20 | 19          | 18           | 17 | 16 |
|    | EVENT_TYPE  |    |    |             | MESSAGE_TYPE |    |    |
|    | R-0h        |    |    |             | R-0h         |    |    |
| 15 | 14          | 13 | 12 | 11          | 10           | 9  | 8  |
|    |             |    |    | SEQUENCE_ID |              |    |    |
|    |             |    |    | R-0h        |              |    |    |
| 7  | 6<br>5<br>4 |    |    |             | 2            | 1  | 0  |
|    |             |    |    | SEQUENCE_ID |              |    |    |
|    |             |    |    |             |              |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-99. CPTS\_EVENT\_HIGH Register Field Descriptions**

| Bit   | Field        | Type | Reset | Description                                                                                                                                                                                                                                   |
|-------|--------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-29 | Reserved     | R    | 0h    |                                                                                                                                                                                                                                               |
| 28-24 | PORT_NUMBER  | R    | 0h    | Port Number - indicates the port number of an ethernet event or the<br>hardware push pin number (1 to 4).                                                                                                                                     |
| 23-20 | EVENT_TYPE   | R    | 0h    | Time Sync Event Type<br>0000 - Time Stamp Push Event<br>0001 - Time Stamp Rollover Event<br>0010 - Time Stamp Half Rollover Event<br>0011 - Hardware Time Stamp Push Event<br>0100 - Ethernet Receive Event<br>0101 - Ethernet Transmit Event |
| 19-16 | MESSAGE_TYPE | R    | 0h    | Message type - The message type value that was contained in an<br>ethernet transmit or receive time sync packet.<br>This field is valid only for ethernet transmit or receive events.                                                         |
| 15-0  | SEQUENCE_ID  | R    | 0h    | Sequence ID - The<br>16-bit sequence id is the value that was contained in an ethernet<br>transmit or receivetime sync packet.<br>This field is valid only for ethernet transmit or receive events.                                           |

### *14.5.4 CPSW\_STATS Registers*

For a full description of the CPSW\_STATS registers, see Section 14.3.2.20, *CPSW\_3G Network Statistics*. The registers are summarized in Table [14-100](#page-6-0).

**Table 14-100. CPSW\_STATS REGISTERS**

| Offset | Acronym | Register Name                 | Section                |
|--------|---------|-------------------------------|------------------------|
| 00h    |         | Good Rx Frames                | Section 14.3.2.20.1.1  |
| 04h    |         | Broadcast Rx Frames           | Section 14.3.2.20.1.2  |
| 08h    |         | Multicast Rx Frames           | Section 14.3.2.20.1.3  |
| 0Ch    |         | Pause Rx Frames               | Section 14.3.2.20.1.4  |
| 10h    |         | Rx CRC Errors                 | Section 14.3.2.20.1.5  |
| 14h    |         | Rx Align/Code Errors          | Section 14.3.2.20.1.6  |
| 18h    |         | Oversize Rx Frames            | Section 14.3.2.20.1.7  |
| 1Ch    |         | Rx Jabbers                    | Section 14.3.2.20.1.8  |
| 20h    |         | Undersize (Short) Rx Frames   | Section 14.3.2.20.1.9  |
| 24h    |         | Rx Fragments                  | Section 14.3.2.20.1.10 |
| 30h    |         | Rx Octets                     | Section 14.3.2.20.1.14 |
| 34h    |         | Good Tx Frames                | Section 14.3.2.20.2.1  |
| 38h    |         | Broadcast Tx Frames           | Section 14.3.2.20.2.2  |
| 3Ch    |         | Multicast Tx Frames           | Section 14.3.2.20.2.3  |
| 40h    |         | Pause Tx Frames               | Section 14.3.2.20.2.4  |
| 44h    |         | Deferred Tx Frames            | Section 14.3.2.20.2.11 |
| 48h    |         | Collisions                    | Section 14.3.2.20.2.5  |
| 4Ch    |         | Single Collision Tx Frames    | Section 14.3.2.20.2.6  |
| 50h    |         | Multiple Collision Tx Frames  | Section 14.3.2.20.2.7  |
| 54h    |         | Excessive Collisions          | Section 14.3.2.20.2.8  |
| 58h    |         | Late Collisions               | Section 14.3.2.20.2.9  |
| 5Ch    |         | Tx Underrun                   | Section 14.3.2.20.2.10 |
| 60h    |         | Carrier Sense Errors          | Section 14.3.2.20.2.12 |
| 64h    |         | Tx Octets                     | Section 14.3.2.20.2.13 |
| 68h    |         | Rx + Tx 64 Octet Frames       | Section 14.3.2.20.3.1  |
| 6Ch    |         | Rx + Tx 65–127 Octet Frames   | Section 14.3.2.20.3.2  |
| 70h    |         | Rx + Tx 128–255 Octet Frames  | Section 14.3.2.20.3.3  |
| 74h    |         | Rx + Tx 256–511 Octet Frames  | Section 14.3.2.20.3.4  |
| 78h    |         | Rx + Tx 512–1023 Octet Frames | Section 14.3.2.20.3.5  |
| 7Ch    |         | Rx + Tx 1024_Up Octet Frames  | Section 14.3.2.20.3.6  |
| 80h    |         | Net Octets                    | Section 14.3.2.20.1.15 |
| 84h    |         | Rx Start of Frame Overruns    | Section 14.3.2.20.1.11 |
| 88h    |         | Rx Middle of Frame Overruns   | Section 14.3.2.20.1.12 |
| 8Ch    |         | Rx DMA Overruns               | Section 14.3.2.20.1.13 |

### *14.5.5 CPDMA\_STATERAM Registers*

Table [14-101](#page-7-0) lists the memory-mapped registers for the CPSW\_CPDMA. All register offset addresses not listed in Table [14-101](#page-7-0) should be considered as reserved locations and the register contents should not be modified.

**Table 14-101. CPDMA\_STATERAM REGISTERS**

| Offset | Acronym | Register Name                                                | Section           |
|--------|---------|--------------------------------------------------------------|-------------------|
| 00h    | TX0_HDP | CPDMA_STATERAM TX CHANNEL 0 HEAD DESC                        | Section 14.5.5.1  |
| 04h    | TX1_HDP | POINTER *<br>CPDMA_STATERAM TX CHANNEL 1 HEAD DESC           | Section 14.5.5.2  |
|        |         | POINTER *                                                    |                   |
| 08h    | TX2_HDP | CPDMA_STATERAM TX CHANNEL 2 HEAD DESC<br>POINTER *           | Section 14.5.5.3  |
| 0Ch    | TX3_HDP | CPDMA_STATERAM TX CHANNEL 3 HEAD DESC<br>POINTER *           | Section 14.5.5.4  |
| 10h    | TX4_HDP | CPDMA_STATERAM TX CHANNEL 4 HEAD DESC<br>POINTER *           | Section 14.5.5.5  |
| 14h    | TX5_HDP | CPDMA_STATERAM TX CHANNEL 5 HEAD DESC<br>POINTER *           | Section 14.5.5.6  |
| 18h    | TX6_HDP | CPDMA_STATERAM TX CHANNEL 6 HEAD DESC<br>POINTER *           | Section 14.5.5.7  |
| 1Ch    | TX7_HDP | CPDMA_STATERAM TX CHANNEL 7 HEAD DESC<br>POINTER *           | Section 14.5.5.8  |
| 20h    | RX0_HDP | CPDMA_STATERAM RX 0 CHANNEL 0 HEAD DESC<br>POINTER *         | Section 14.5.5.9  |
| 24h    | RX1_HDP | CPDMA_STATERAM RX 1 CHANNEL 1 HEAD DESC<br>POINTER *         | Section 14.5.5.10 |
| 28h    | RX2_HDP | CPDMA_STATERAM RX 2 CHANNEL 2 HEAD DESC<br>POINTER *         | Section 14.5.5.11 |
| 2Ch    | RX3_HDP | CPDMA_STATERAM RX 3 CHANNEL 3 HEAD DESC<br>POINTER *         | Section 14.5.5.12 |
| 30h    | RX4_HDP | CPDMA_STATERAM RX 4 CHANNEL 4 HEAD DESC<br>POINTER *         | Section 14.5.5.13 |
| 34h    | RX5_HDP | CPDMA_STATERAM RX 5 CHANNEL 5 HEAD DESC<br>POINTER *         | Section 14.5.5.14 |
| 38h    | RX6_HDP | CPDMA_STATERAM RX 6 CHANNEL 6 HEAD DESC<br>POINTER *         | Section 14.5.5.15 |
| 3Ch    | RX7_HDP | CPDMA_STATERAM RX 7 CHANNEL 7 HEAD DESC<br>POINTER *         | Section 14.5.5.16 |
| 40h    | TX0_CP  | CPDMA_STATERAM TX CHANNEL 0 COMPLETION<br>POINTER REGISTER   | Section 14.5.5.17 |
| 44h    | TX1_CP  | CPDMA_STATERAM TX CHANNEL 1 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.18 |
| 48h    | TX2_CP  | CPDMA_STATERAM TX CHANNEL 2 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.19 |
| 4Ch    | TX3_CP  | CPDMA_STATERAM TX CHANNEL 3 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.20 |
| 50h    | TX4_CP  | CPDMA_STATERAM TX CHANNEL 4 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.21 |
| 54h    | TX5_CP  | CPDMA_STATERAM TX CHANNEL 5 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.22 |
| 58h    | TX6_CP  | CPDMA_STATERAM TX CHANNEL 6 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.23 |
| 5Ch    | TX7_CP  | CPDMA_STATERAM TX CHANNEL 7 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.24 |
| 60h    | RX0_CP  | CPDMA_STATERAM RX CHANNEL 0 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.25 |
| 64h    | RX1_CP  | CPDMA_STATERAM RX CHANNEL 1 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.26 |
| 68h    | RX2_CP  | CPDMA_STATERAM RX CHANNEL 2 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.27 |
| 6Ch    | RX3_CP  | CPDMA_STATERAM RX CHANNEL 3 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.28 |

**Table 14-101. CPDMA\_STATERAM REGISTERS (continued)**

| Offset | Acronym | Register Name                                                | Section           |
|--------|---------|--------------------------------------------------------------|-------------------|
| 70h    | RX4_CP  | CPDMA_STATERAM RX CHANNEL 4 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.29 |
| 74h    | RX5_CP  | CPDMA_STATERAM RX CHANNEL 5 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.30 |
| 78h    | RX6_CP  | CPDMA_STATERAM RX CHANNEL 6 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.31 |
| 7Ch    | RX7_CP  | CPDMA_STATERAM RX CHANNEL 7 COMPLETION<br>POINTER REGISTER * | Section 14.5.5.32 |

#### **14.5.5.1 TX0\_HDP Register (offset = A00h) [reset = 0h]**

TX0\_HDP is shown in [Figure](#page-9-1) 14-89 and described in Table [14-102.](#page-9-2) CPDMA\_STATERAM TX CHANNEL 0 HEAD DESC POINTER \*

#### **Figure 14-89. TX0\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-102. TX0\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                            |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_HDP | R/W  | 0h    | TX Channel (07) DMA Head Descriptor Pointer - Writing a TX DMA<br>Buffer Descriptor address to a head pointer location initiates TX<br>DMA operations in the queue for the selected channel.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.2 TX1\_HDP Register (offset = A04h) [reset = 0h]**

TX1\_HDP is shown in [Figure](#page-10-1) 14-90 and described in Table [14-103.](#page-10-2)

CPDMA\_STATERAM TX CHANNEL 1 HEAD DESC POINTER \*

**Figure 14-90. TX1\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-103. TX1\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                            |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_HDP | R/W  | 0h    | TX Channel (07) DMA Head Descriptor Pointer - Writing a TX DMA<br>Buffer Descriptor address to a head pointer location initiates TX<br>DMA operations in the queue for the selected channel.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.3 TX2\_HDP Register (offset = A08h) [reset = 0h]**

TX2\_HDP is shown in [Figure](#page-11-1) 14-91 and described in Table [14-104.](#page-11-2) CPDMA\_STATERAM TX CHANNEL 2 HEAD DESC POINTER \*

#### **Figure 14-91. TX2\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-104. TX2\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                            |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_HDP | R/W  | 0h    | TX Channel (07) DMA Head Descriptor Pointer - Writing a TX DMA<br>Buffer Descriptor address to a head pointer location initiates TX<br>DMA operations in the queue for the selected channel.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.4 TX3\_HDP Register (offset = A0Ch) [reset = 0h]**

TX3\_HDP is shown in [Figure](#page-12-1) 14-92 and described in Table [14-105.](#page-12-2)

CPDMA\_STATERAM TX CHANNEL 3 HEAD DESC POINTER \*

#### **Figure 14-92. TX3\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-105. TX3\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                            |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_HDP | R/W  | 0h    | TX Channel (07) DMA Head Descriptor Pointer - Writing a TX DMA<br>Buffer Descriptor address to a head pointer location initiates TX<br>DMA operations in the queue for the selected channel.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.5 TX4\_HDP Register (offset = A10h) [reset = 0h]**

TX4\_HDP is shown in [Figure](#page-13-1) 14-93 and described in Table [14-106.](#page-13-2)

CPDMA\_STATERAM TX CHANNEL 4 HEAD DESC POINTER \*

#### **Figure 14-93. TX4\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-106. TX4\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                            |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_HDP | R/W  | 0h    | TX Channel (07) DMA Head Descriptor Pointer - Writing a TX DMA<br>Buffer Descriptor address to a head pointer location initiates TX<br>DMA operations in the queue for the selected channel.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.6 TX5\_HDP Register (offset = A14h) [reset = 0h]**

TX5\_HDP is shown in [Figure](#page-14-1) 14-94 and described in Table [14-107.](#page-14-2)

CPDMA\_STATERAM TX CHANNEL 5 HEAD DESC POINTER \*

**Figure 14-94. TX5\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-107. TX5\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                            |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_HDP | R/W  | 0h    | TX Channel (07) DMA Head Descriptor Pointer - Writing a TX DMA<br>Buffer Descriptor address to a head pointer location initiates TX<br>DMA operations in the queue for the selected channel.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.7 TX6\_HDP Register (offset = A18h) [reset = 0h]**

TX6\_HDP is shown in [Figure](#page-15-1) 14-95 and described in Table [14-108.](#page-15-2) CPDMA\_STATERAM TX CHANNEL 6 HEAD DESC POINTER \*

#### **Figure 14-95. TX6\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-108. TX6\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                            |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_HDP | R/W  | 0h    | TX Channel (07) DMA Head Descriptor Pointer - Writing a TX DMA<br>Buffer Descriptor address to a head pointer location initiates TX<br>DMA operations in the queue for the selected channel.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.8 TX7\_HDP Register (offset = A1Ch) [reset = 0h]**

TX7\_HDP is shown in [Figure](#page-16-1) 14-96 and described in Table [14-109.](#page-16-2)

CPDMA\_STATERAM TX CHANNEL 7 HEAD DESC POINTER \*

#### **Figure 14-96. TX7\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-109. TX7\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                            |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_HDP | R/W  | 0h    | TX Channel (07) DMA Head Descriptor Pointer - Writing a TX DMA<br>Buffer Descriptor address to a head pointer location initiates TX<br>DMA operations in the queue for the selected channel.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.9 RX0\_HDP Register (offset = A20h) [reset = 0h]**

RX0\_HDP is shown in [Figure](#page-17-1) 14-97 and described in Table [14-110.](#page-17-2) CPDMA\_STATERAM RX 0 CHANNEL 0 HEAD DESC POINTER \*

#### **Figure 14-97. RX0\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-110. RX0\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                      |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_HDP | R/W  | 0h    | RX DMA Head Descriptor Pointer - Writing an RX DMA Buffer<br>Descriptor address to this location allows RX DMA operations in the<br>selected channel when a channel frame is received.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.10 RX1\_HDP Register (offset = A24h) [reset = 0h]**

RX1\_HDP is shown in [Figure](#page-18-1) 14-98 and described in Table [14-111.](#page-18-2)

CPDMA\_STATERAM RX 1 CHANNEL 1 HEAD DESC POINTER \*

**Figure 14-98. RX1\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-111. RX1\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                      |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_HDP | R/W  | 0h    | RX DMA Head Descriptor Pointer - Writing an RX DMA Buffer<br>Descriptor address to this location allows RX DMA operations in the<br>selected channel when a channel frame is received.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.11 RX2\_HDP Register (offset = A28h) [reset = 0h]**

RX2\_HDP is shown in [Figure](#page-19-1) 14-99 and described in Table [14-112.](#page-19-2)

CPDMA\_STATERAM RX 2 CHANNEL 2 HEAD DESC POINTER \*

#### **Figure 14-99. RX2\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-112. RX2\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                      |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_HDP | R/W  | 0h    | RX DMA Head Descriptor Pointer - Writing an RX DMA Buffer<br>Descriptor address to this location allows RX DMA operations in the<br>selected channel when a channel frame is received.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.12 RX3\_HDP Register (offset = A2Ch) [reset = 0h]**

RX3\_HDP is shown in Figure [14-100](#page-20-1) and described in Table [14-113](#page-20-2). CPDMA\_STATERAM RX 3 CHANNEL 3 HEAD DESC POINTER \*

**Figure 14-100. RX3\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-113. RX3\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                      |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_HDP | R/W  | 0h    | RX DMA Head Descriptor Pointer - Writing an RX DMA Buffer<br>Descriptor address to this location allows RX DMA operations in the<br>selected channel when a channel frame is received.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.13 RX4\_HDP Register (offset = A30h) [reset = 0h]**

RX4\_HDP is shown in Figure [14-101](#page-21-1) and described in Table [14-114](#page-21-2). CPDMA\_STATERAM RX 4 CHANNEL 4 HEAD DESC POINTER \*

#### **Figure 14-101. RX4\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-114. RX4\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                      |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_HDP | R/W  | 0h    | RX DMA Head Descriptor Pointer - Writing an RX DMA Buffer<br>Descriptor address to this location allows RX DMA operations in the<br>selected channel when a channel frame is received.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.14 RX5\_HDP Register (offset = A34h) [reset = 0h]**

RX5\_HDP is shown in Figure [14-102](#page-22-1) and described in Table [14-115](#page-22-2). CPDMA\_STATERAM RX 5 CHANNEL 5 HEAD DESC POINTER \*

#### **Figure 14-102. RX5\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-115. RX5\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                      |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_HDP | R/W  | 0h    | RX DMA Head Descriptor Pointer - Writing an RX DMA Buffer<br>Descriptor address to this location allows RX DMA operations in the<br>selected channel when a channel frame is received.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.15 RX6\_HDP Register (offset = A38h) [reset = 0h]**

RX6\_HDP is shown in Figure [14-103](#page-23-1) and described in Table [14-116](#page-23-2). CPDMA\_STATERAM RX 6 CHANNEL 6 HEAD DESC POINTER \*

#### **Figure 14-103. RX6\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-116. RX6\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                      |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_HDP | R/W  | 0h    | RX DMA Head Descriptor Pointer - Writing an RX DMA Buffer<br>Descriptor address to this location allows RX DMA operations in the<br>selected channel when a channel frame is received.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.16 RX7\_HDP Register (offset = A3Ch) [reset = 0h]**

RX7\_HDP is shown in Figure [14-104](#page-24-1) and described in Table [14-117](#page-24-2).

CPDMA\_STATERAM RX 7 CHANNEL 7 HEAD DESC POINTER \*

**Figure 14-104. RX7\_HDP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-117. RX7\_HDP Register Field Descriptions**

| Bit  | Field  | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                      |
|------|--------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_HDP | R/W  | 0h    | RX DMA Head Descriptor Pointer - Writing an RX DMA Buffer<br>Descriptor address to this location allows RX DMA operations in the<br>selected channel when a channel frame is received.<br>Writing to these locations when they are non-zero is an error (except<br>at reset).<br>Host software must initialize these locations to zero on reset. |

#### **14.5.5.17 TX0\_CP Register (offset = A40h) [reset = 0h]**

TX0\_CP is shown in Figure [14-105](#page-25-1) and described in Table [14-118](#page-25-2). CPDMA\_STATERAM TX CHANNEL 0 COMPLETION POINTER REGISTER

#### **Figure 14-105. TX0\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-118. TX0\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                     |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_CP | R/W  | 0h    | Tx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted. |

#### **14.5.5.18 TX1\_CP Register (offset = A44h) [reset = 0h]**

TX1\_CP is shown in Figure [14-106](#page-26-1) and described in Table [14-119](#page-26-2).

CPDMA\_STATERAM TX CHANNEL 1 COMPLETION POINTER REGISTER \*

**Figure 14-106. TX1\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-119. TX1\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                     |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_CP | R/W  | 0h    | Tx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted. |

#### **14.5.5.19 TX2\_CP Register (offset = A48h) [reset = 0h]**

TX2\_CP is shown in Figure [14-107](#page-27-1) and described in Table [14-120](#page-27-2).

CPDMA\_STATERAM TX CHANNEL 2 COMPLETION POINTER REGISTER \*

#### **Figure 14-107. TX2\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-120. TX2\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                     |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_CP | R/W  | 0h    | Tx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted. |

#### **14.5.5.20 TX3\_CP Register (offset = A4Ch) [reset = 0h]**

TX3\_CP is shown in Figure [14-108](#page-28-1) and described in Table [14-121](#page-28-2).

CPDMA\_STATERAM TX CHANNEL 3 COMPLETION POINTER REGISTER \*

**Figure 14-108. TX3\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-121. TX3\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                     |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_CP | R/W  | 0h    | Tx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted. |

#### **14.5.5.21 TX4\_CP Register (offset = A50h) [reset = 0h]**

TX4\_CP is shown in Figure [14-109](#page-29-1) and described in Table [14-122](#page-29-2).

CPDMA\_STATERAM TX CHANNEL 4 COMPLETION POINTER REGISTER \*

#### **Figure 14-109. TX4\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-122. TX4\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                     |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_CP | R/W  | 0h    | Tx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted. |

#### **14.5.5.22 TX5\_CP Register (offset = A54h) [reset = 0h]**

TX5\_CP is shown in Figure [14-110](#page-30-1) and described in Table [14-123](#page-30-2).

CPDMA\_STATERAM TX CHANNEL 5 COMPLETION POINTER REGISTER \*

**Figure 14-110. TX5\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-123. TX5\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                     |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_CP | R/W  | 0h    | Tx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted. |

#### **14.5.5.23 TX6\_CP Register (offset = A58h) [reset = 0h]**

TX6\_CP is shown in Figure [14-111](#page-31-1) and described in Table [14-124](#page-31-2).

CPDMA\_STATERAM TX CHANNEL 6 COMPLETION POINTER REGISTER \*

#### **Figure 14-111. TX6\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-124. TX6\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                     |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_CP | R/W  | 0h    | Tx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted. |

#### **14.5.5.24 TX7\_CP Register (offset = A5Ch) [reset = 0h]**

TX7\_CP is shown in Figure [14-112](#page-32-1) and described in Table [14-125](#page-32-2).

CPDMA\_STATERAM TX CHANNEL 7 COMPLETION POINTER REGISTER \*

#### **Figure 14-112. TX7\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-125. TX7\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                     |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | TX_CP | R/W  | 0h    | Tx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted. |

#### **14.5.5.25 RX0\_CP Register (offset = A60h) [reset = 0h]**

RX0\_CP is shown in Figure [14-113](#page-33-1) and described in Table [14-126](#page-33-2).

CPDMA\_STATERAM RX CHANNEL 0 COMPLETION POINTER REGISTER \*

#### **Figure 14-113. RX0\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-126. RX0\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_CP | R/W  | 0h    | Rx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted.<br>Note: The value read is the completion pointer (interrupt<br>acknowledge) value that was written by the CPDMA DMA controller<br>(port).<br>The value written to this register by the host is compared with the<br>value that the port wrote to determine if the interrupt should remain<br>asserted.<br>The value written is not actually stored in the location.<br>The interrupt is deasserted if the two values are equal. |

#### **14.5.5.26 RX1\_CP Register (offset = A64h) [reset = 0h]**

RX1\_CP is shown in Figure [14-114](#page-34-1) and described in Table [14-127](#page-34-2).

CPDMA\_STATERAM RX CHANNEL 1 COMPLETION POINTER REGISTER \*

**Figure 14-114. RX1\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-127. RX1\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_CP | R/W  | 0h    | Rx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted.<br>Note: The value read is the completion pointer (interrupt<br>acknowledge) value that was written by the CPDMA DMA controller<br>(port).<br>The value written to this register by the host is compared with the<br>value that the port wrote to determine if the interrupt should remain<br>asserted.<br>The value written is not actually stored in the location.<br>The interrupt is deasserted if the two values are equal. |

#### **14.5.5.27 RX2\_CP Register (offset = A68h) [reset = 0h]**

RX2\_CP is shown in Figure [14-115](#page-35-1) and described in Table [14-128](#page-35-2).

CPDMA\_STATERAM RX CHANNEL 2 COMPLETION POINTER REGISTER \*

#### **Figure 14-115. RX2\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-128. RX2\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_CP | R/W  | 0h    | Rx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted.<br>Note: The value read is the completion pointer (interrupt<br>acknowledge) value that was written by the CPDMA DMA controller<br>(port).<br>The value written to this register by the host is compared with the<br>value that the port wrote to determine if the interrupt should remain<br>asserted.<br>The value written is not actually stored in the location.<br>The interrupt is deasserted if the two values are equal. |

#### **14.5.5.28 RX3\_CP Register (offset = A6Ch) [reset = 0h]**

RX3\_CP is shown in Figure [14-116](#page-36-1) and described in Table [14-129](#page-36-2).

CPDMA\_STATERAM RX CHANNEL 3 COMPLETION POINTER REGISTER \*

**Figure 14-116. RX3\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-129. RX3\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_CP | R/W  | 0h    | Rx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted.<br>Note: The value read is the completion pointer (interrupt<br>acknowledge) value that was written by the CPDMA DMA controller<br>(port).<br>The value written to this register by the host is compared with the<br>value that the port wrote to determine if the interrupt should remain<br>asserted.<br>The value written is not actually stored in the location.<br>The interrupt is deasserted if the two values are equal. |

#### **14.5.5.29 RX4\_CP Register (offset = A70h) [reset = 0h]**

RX4\_CP is shown in Figure [14-117](#page-37-1) and described in Table [14-130](#page-37-2).

CPDMA\_STATERAM RX CHANNEL 4 COMPLETION POINTER REGISTER \*

#### **Figure 14-117. RX4\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-130. RX4\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_CP | R/W  | 0h    | Rx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted.<br>Note: The value read is the completion pointer (interrupt<br>acknowledge) value that was written by the CPDMA DMA controller<br>(port).<br>The value written to this register by the host is compared with the<br>value that the port wrote to determine if the interrupt should remain<br>asserted.<br>The value written is not actually stored in the location.<br>The interrupt is deasserted if the two values are equal. |

#### **14.5.5.30 RX5\_CP Register (offset = A74h) [reset = 0h]**

RX5\_CP is shown in Figure [14-118](#page-38-1) and described in Table [14-131](#page-38-2).

CPDMA\_STATERAM RX CHANNEL 5 COMPLETION POINTER REGISTER \*

**Figure 14-118. RX5\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-131. RX5\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_CP | R/W  | 0h    | Rx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted.<br>Note: The value read is the completion pointer (interrupt<br>acknowledge) value that was written by the CPDMA DMA controller<br>(port).<br>The value written to this register by the host is compared with the<br>value that the port wrote to determine if the interrupt should remain<br>asserted.<br>The value written is not actually stored in the location.<br>The interrupt is deasserted if the two values are equal. |

#### **14.5.5.31 RX6\_CP Register (offset = A78h) [reset = 0h]**

RX6\_CP is shown in Figure [14-119](#page-39-1) and described in Table [14-132](#page-39-2).

CPDMA\_STATERAM RX CHANNEL 6 COMPLETION POINTER REGISTER \*

#### **Figure 14-119. RX6\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-132. RX6\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_CP | R/W  | 0h    | Rx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted.<br>Note: The value read is the completion pointer (interrupt<br>acknowledge) value that was written by the CPDMA DMA controller<br>(port).<br>The value written to this register by the host is compared with the<br>value that the port wrote to determine if the interrupt should remain<br>asserted.<br>The value written is not actually stored in the location.<br>The interrupt is deasserted if the two values are equal. |

#### **14.5.5.32 RX7\_CP Register (offset = A7Ch) [reset = 0h]**

RX7\_CP is shown in Figure [14-120](#page-40-1) and described in Table [14-133](#page-40-2).

CPDMA\_STATERAM RX CHANNEL 7 COMPLETION POINTER REGISTER \*

**Figure 14-120. RX7\_CP Register**

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-133. RX7\_CP Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | RX_CP | R/W  | 0h    | Rx Completion Pointer Register - This register is written by the host<br>with the buffer descriptor address for the last buffer processed by the<br>host during interrupt processing.<br>The port uses the value written to determine if the interrupt should<br>be deasserted.<br>Note: The value read is the completion pointer (interrupt<br>acknowledge) value that was written by the CPDMA DMA controller<br>(port).<br>The value written to this register by the host is compared with the<br>value that the port wrote to determine if the interrupt should remain<br>asserted.<br>The value written is not actually stored in the location.<br>The interrupt is deasserted if the two values are equal. |

### *14.5.6 CPSW\_PORT Registers*

Table [14-134](#page-40-3) lists the memory-mapped registers for the CPSW\_PORT. All register offset addresses not listed in Table [14-134](#page-40-3) should be considered as reserved locations and the register contents should not be modified.

**Table 14-134. CPSW\_PORT Registers**

| Offset | Acronym             | Register Name | Section           |
|--------|---------------------|---------------|-------------------|
| 0h     | P0_CONTROL          |               | Section 14.5.6.1  |
| 8h     | P0_MAX_BLKS         |               | Section 14.5.6.2  |
| Ch     | P0_BLK_CNT          |               | Section 14.5.6.3  |
| 10h    | P0_TX_IN_CTL        |               | Section 14.5.6.4  |
| 14h    | P0_PORT_VLAN        |               | Section 14.5.6.5  |
| 18h    | P0_TX_PRI_MAP       |               | Section 14.5.6.6  |
| 1Ch    | P0_CPDMA_TX_PRI_MAP |               | Section 14.5.6.7  |
| 20h    | P0_CPDMA_RX_CH_MAP  |               | Section 14.5.6.8  |
| 30h    | P0_RX_DSCP_PRI_MAP0 |               | Section 14.5.6.9  |
| 34h    | P0_RX_DSCP_PRI_MAP1 |               | Section 14.5.6.10 |
| 38h    | P0_RX_DSCP_PRI_MAP2 |               | Section 14.5.6.11 |
| 3Ch    | P0_RX_DSCP_PRI_MAP3 |               | Section 14.5.6.12 |
| 40h    | P0_RX_DSCP_PRI_MAP4 |               | Section 14.5.6.13 |
| 44h    | P0_RX_DSCP_PRI_MAP5 |               | Section 14.5.6.14 |
| 48h    | P0_RX_DSCP_PRI_MAP6 |               | Section 14.5.6.15 |
| 4Ch    | P0_RX_DSCP_PRI_MAP7 |               | Section 14.5.6.16 |
| 100h   | P1_CONTROL          |               | Section 14.5.6.17 |
| 108h   | P1_MAX_BLKS         |               | Section 14.5.6.18 |
| 10Ch   | P1_BLK_CNT          |               | Section 14.5.6.19 |
| 110h   | P1_TX_IN_CTL        |               | Section 14.5.6.20 |

**Table 14-134. CPSW\_PORT Registers (continued)**

| Offset | Acronym             | Register Name | Section           |
|--------|---------------------|---------------|-------------------|
| 114h   | P1_PORT_VLAN        |               | Section 14.5.6.21 |
| 118h   | P1_TX_PRI_MAP       |               | Section 14.5.6.22 |
| 11Ch   | P1_TS_SEQ_MTYPE     |               | Section 14.5.6.23 |
| 120h   | P1_SA_LO            |               | Section 14.5.6.24 |
| 124h   | P1_SA_HI            |               | Section 14.5.6.25 |
| 128h   | P1_SEND_PERCENT     |               | Section 14.5.6.26 |
| 130h   | P1_RX_DSCP_PRI_MAP0 |               | Section 14.5.6.27 |
| 134h   | P1_RX_DSCP_PRI_MAP1 |               | Section 14.5.6.28 |
| 138h   | P1_RX_DSCP_PRI_MAP2 |               | Section 14.5.6.29 |
| 13Ch   | P1_RX_DSCP_PRI_MAP3 |               | Section 14.5.6.30 |
| 140h   | P1_RX_DSCP_PRI_MAP4 |               | Section 14.5.6.31 |
| 144h   | P1_RX_DSCP_PRI_MAP5 |               | Section 14.5.6.32 |
| 148h   | P1_RX_DSCP_PRI_MAP6 |               | Section 14.5.6.33 |
| 14Ch   | P1_RX_DSCP_PRI_MAP7 |               | Section 14.5.6.34 |
| 200h   | P2_CONTROL          |               | Section 14.5.6.35 |
| 208h   | P2_MAX_BLKS         |               | Section 14.5.6.36 |
| 20Ch   | P2_BLK_CNT          |               | Section 14.5.6.37 |
| 210h   | P2_TX_IN_CTL        |               | Section 14.5.6.38 |
| 214h   | P2_PORT_VLAN        |               | Section 14.5.6.39 |
| 218h   | P2_TX_PRI_MAP       |               | Section 14.5.6.40 |
| 21Ch   | P2_TS_SEQ_MTYPE     |               | Section 14.5.6.41 |
| 220h   | P2_SA_LO            |               | Section 14.5.6.42 |
| 224h   | P2_SA_HI            |               | Section 14.5.6.43 |
| 228h   | P2_SEND_PERCENT     |               | Section 14.5.6.44 |
| 230h   | P2_RX_DSCP_PRI_MAP0 |               | Section 14.5.6.45 |
| 234h   | P2_RX_DSCP_PRI_MAP1 |               | Section 14.5.6.46 |
| 238h   | P2_RX_DSCP_PRI_MAP2 |               | Section 14.5.6.47 |
| 23Ch   | P2_RX_DSCP_PRI_MAP3 |               | Section 14.5.6.48 |
| 240h   | P2_RX_DSCP_PRI_MAP4 |               | Section 14.5.6.49 |
| 244h   | P2_RX_DSCP_PRI_MAP5 |               | Section 14.5.6.50 |
| 248h   | P2_RX_DSCP_PRI_MAP6 |               | Section 14.5.6.51 |
| 24Ch   | P2_RX_DSCP_PRI_MAP7 |               | Section 14.5.6.52 |

#### **14.5.6.1 P0\_CONTROL Register (offset = 0h) [reset = 0h]**

P0\_CONTROL is shown in Figure [14-121](#page-42-1) and described in Table [14-135.](#page-42-2) CPSW PORT 0 CONTROL REGISTER

**Figure 14-121. P0\_CONTROL Register**

| 31       | 30                                                         | 29              | 28       | 27     | 26                     | 25 | 24                 |  |
|----------|------------------------------------------------------------|-----------------|----------|--------|------------------------|----|--------------------|--|
| RESERVED |                                                            | P0_DLR_CPDMA_CH |          |        | P0_PASS_PRI<br>_TAGGED |    |                    |  |
| R/W-0h   |                                                            | R/W-0h          |          |        | R/W-0h                 |    |                    |  |
| 23       | 22<br>21<br>20                                             |                 |          | 19     | 18                     | 17 | 16                 |  |
|          | RESERVED<br>P0_VLAN_LTY<br>P0_VLAN_LTY<br>PE2_EN<br>PE1_EN |                 |          |        | RESERVED               |    | P0_DSCP_PRI<br>_EN |  |
|          | R/W-0h                                                     | R/W-0h          | R/W-0h   | R/W-0h |                        |    | R/W-0h             |  |
| 15       | 14                                                         | 13              | 12       | 11     | 10                     | 9  | 8                  |  |
|          |                                                            |                 | RESERVED |        |                        |    |                    |  |
|          |                                                            |                 | R/W-0h   |        |                        |    |                    |  |
| 7        | 6<br>5<br>4                                                |                 | 3        | 2      | 1                      | 0  |                    |  |
|          |                                                            |                 | RESERVED |        |                        |    |                    |  |
|          |                                                            |                 | R/W-0h   |        |                        |    |                    |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-135. P0\_CONTROL Register Field Descriptions**

| Bit   | Field              | Type | Reset | Description                                                                                                                                                                                                                                             |
|-------|--------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31    | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 30-28 | P0_DLR_CPDMA_CH    | R/W  | 0h    | Port 0 DLR CPDMA Channel This field indicates the CPDMA<br>channel that DLR packets will be received on.                                                                                                                                                |
| 27-25 | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 24    | P0_PASS_PRI_TAGGED | R/W  | 0h    | Port 0 Pass Priority Tagged<br>0 - Priority tagged packets have the zero VID replaced with the input<br>port P0_PORT_VLAN<br>[11:0]<br>1 - Priority tagged packets are processed unchanged.                                                             |
| 23-22 | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 21    | P0_VLAN_LTYPE2_EN  | R/W  | 0h    | Port 0 VLAN LTYPE 2 enable<br>0 - disabled<br>1 - enabled                                                                                                                                                                                               |
| 20    | P0_VLAN_LTYPE1_EN  | R/W  | 0h    | Port 0 VLAN LTYPE 1 enable<br>0 - disabled<br>1 - enabled                                                                                                                                                                                               |
| 19-17 | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 16    | P0_DSCP_PRI_EN     | R/W  | 0h    | Port 0 DSCP Priority Enable<br>0 - DSCP priority disabled<br>1 - DSCP priority enabled.<br>All non-tagged IPV4 packets have their received packet priority<br>determined by mapping the 6 TOS bits through the port DSCP<br>priority mapping registers. |
| 15-0  | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |

#### **14.5.6.2 P0\_MAX\_BLKS Register (offset = 8h) [reset = 104h]**

P0\_MAX\_BLKS is shown in Figure [14-122](#page-43-1) and described in Table [14-136.](#page-43-2) CPSW PORT 0 MAXIMUM FIFO BLOCKS REGISTER

**Figure 14-122. P0\_MAX\_BLKS Register**

| 31 | 30             | 29 | 28       | 27       | 26             | 25 | 24                 |  |  |  |
|----|----------------|----|----------|----------|----------------|----|--------------------|--|--|--|
|    | RESERVED       |    |          |          |                |    |                    |  |  |  |
|    | R/W-0h         |    |          |          |                |    |                    |  |  |  |
| 23 | 22             | 21 | 20       | 19       | 18             | 17 | 16                 |  |  |  |
|    |                |    |          | RESERVED |                |    |                    |  |  |  |
|    | R/W-0h         |    |          |          |                |    |                    |  |  |  |
| 15 | 14             | 13 | 12       | 11       | 10             | 9  | 8                  |  |  |  |
|    |                |    | RESERVED |          |                |    | P0_TX_MAX_B<br>LKS |  |  |  |
|    |                |    | R/W-0h   |          |                |    | R/W-10h            |  |  |  |
| 7  | 6              | 5  | 4        | 3        | 2              | 1  | 0                  |  |  |  |
|    | P0_TX_MAX_BLKS |    |          |          | P0_RX_MAX_BLKS |    |                    |  |  |  |
|    | R/W-10h        |    |          |          | R/W-4h         |    |                    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-136. P0\_MAX\_BLKS Register Field Descriptions**

| Bit  | Field          | Type | Reset | Description                                                                                                                                                                                                                                                                                                |
|------|----------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-9 | RESERVED       | R/W  | 0h    |                                                                                                                                                                                                                                                                                                            |
| 8-4  | P0_TX_MAX_BLKS | R/W  | 10h   | Transmit FIFO Maximum Blocks - This value is the maximum<br>number of 1k memory blocks that may be allocated to the FIFO's<br>logical transmit priority queues.<br>0x10 is the recommended value of p0_tx_max_blks.<br>Port 0 should remain in flow control mode.<br>0xe is the minimum value tx max blks. |
| 3-0  | P0_RX_MAX_BLKS | R/W  | 4h    | Receive FIFO Maximum Blocks - This value is the maximum number<br>of 1k memory blocks that may be allocated to the FIFO's logical<br>receive queue.<br>0x4 is the recommended value.<br>0x3 is the minimum value rx max blks and 0x6 is the maximum<br>value.                                              |

#### **14.5.6.3 P0\_BLK\_CNT Register (offset = Ch) [reset = 41h]**

P0\_BLK\_CNT is shown in Figure [14-123](#page-44-1) and described in Table [14-137](#page-44-2). CPSW PORT 0 FIFO BLOCK USAGE COUNT (READ ONLY)

**Figure 14-123. P0\_BLK\_CNT Register**

```
(BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        RESERVED                           |
|                         R-0h                              |
+-----------------------------------------------------------+

(BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        RESERVED                           |
|                         R-0h                              |
+-----------------------------------------------------------+

(BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        RESERVED                           |
|                         R-0h                              |
+-----------------------------------------------------------+

(BITS 7..0)
 7       6       5       4       3       2       1       0
+-------------------------------+---------------------------+
|          P0_TX_BLK_CNT        |        P0_RX_BLK_CNT      |
|             R-4h              |           R-1h            |
+-------------------------------+---------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-137. P0\_BLK\_CNT Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                              |
|------|---------------|------|-------|--------------------------------------------------------------------------------------------------------------------------|
| 31-9 | RESERVED      | R    | 0h    |                                                                                                                          |
| 8-4  | P0_TX_BLK_CNT | R    | 4h    | Port 0 Transmit Block Count Usage - This value is the number of<br>blocks allocated to the FIFO logical transmit queues. |
| 3-0  | P0_RX_BLK_CNT | R    | 1h    | Port 0 Receive Block Count Usage - This value is the number of<br>blocks allocated to the FIFO logical receive queues.   |

#### **14.5.6.4 P0\_TX\_IN\_CTL Register (offset = 10h) [reset = 40C0h]**

P0\_TX\_IN\_CTL is shown in Figure [14-124](#page-45-1) and described in Table [14-138.](#page-45-2) CPSW PORT 0 TRANSMIT FIFO CONTROL

**Figure 14-124. P0\_TX\_IN\_CTL Register**

| 27<br>26<br>25         |  |    |        |             |    |  |  |  |  |
|------------------------|--|----|--------|-------------|----|--|--|--|--|
|                        |  | 28 | 29     | 30          | 31 |  |  |  |  |
| RESERVED               |  |    |        |             |    |  |  |  |  |
| R/W                    |  |    |        |             |    |  |  |  |  |
| 19<br>18<br>17         |  | 20 | 21     | 22          | 23 |  |  |  |  |
| RESERVED<br>TX_IN_SEL  |  |    |        | TX_RATE_EN  |    |  |  |  |  |
| R/W-<br>R/W-0h         |  |    | R/W-0h |             |    |  |  |  |  |
| 11<br>10<br>9          |  | 12 | 13     | 14          | 15 |  |  |  |  |
| RESERVED<br>TX_PRI_WDS |  |    |        | TX_BLKS_REM |    |  |  |  |  |
| R/W-<br>R/W-C0h        |  |    |        | R/W-4h      |    |  |  |  |  |
| 3<br>2<br>1            |  | 4  | 5      | 6           | 7  |  |  |  |  |
| TX_PRI_WDS             |  |    |        |             |    |  |  |  |  |
| R/W-C0h                |  |    |        |             |    |  |  |  |  |
|                        |  |    |        |             |    |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-138. P0\_TX\_IN\_CTL Register Field Descriptions**

| Bit   | Field       | Type | Reset | Description                                                                                                                                                                                                                                                                    |
|-------|-------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | RESERVED    | R/W  |       |                                                                                                                                                                                                                                                                                |
| 23-20 | TX_RATE_EN  | R/W  | 0h    | Transmit FIFO Input Rate Enable                                                                                                                                                                                                                                                |
| 19-18 | RESERVED    | R/W  |       |                                                                                                                                                                                                                                                                                |
| 17-16 | TX_IN_SEL   | R/W  | 0h    | Transmit FIFO Input Queue Type Select<br>00 - Normal priority mode<br>01 - Dual MAC mode<br>10 - Rate Limit mode<br>11 - reserved Note that Dual MAC mode is not compatible with<br>escalation or shaping because dual mac mode forces round robin<br>priority on FIFO egress. |
| 15-12 | TX_BLKS_REM | R/W  | 4h    | Transmit FIFO Input Blocks to subtract in dual mac mode                                                                                                                                                                                                                        |
| 11-10 | RESERVED    | R/W  |       |                                                                                                                                                                                                                                                                                |
| 9-0   | TX_PRI_WDS  | R/W  | C0h   | Transmit FIFO Words in queue                                                                                                                                                                                                                                                   |

#### **14.5.6.5 P0\_PORT\_VLAN Register (offset = 14h) [reset = 0h]**

P0\_PORT\_VLAN is shown in Figure [14-125](#page-46-1) and described in Table [14-139](#page-46-2). CPSW PORT 0 VLAN REGISTER

**Figure 14-125. P0\_PORT\_VLAN Register**

| 31 | 30       | 29 | 28       | 27 | 26       | 25 | 24 |  |  |  |  |
|----|----------|----|----------|----|----------|----|----|--|--|--|--|
|    | RESERVED |    |          |    |          |    |    |  |  |  |  |
|    | R/W-0h   |    |          |    |          |    |    |  |  |  |  |
| 23 | 22       | 21 | 20       | 19 | 18       | 17 | 16 |  |  |  |  |
|    |          |    | RESERVED |    |          |    |    |  |  |  |  |
|    | R/W-0h   |    |          |    |          |    |    |  |  |  |  |
| 15 | 14       | 13 | 12       | 11 | 10       | 9  | 8  |  |  |  |  |
|    | PORT_PRI |    | PORT_CFI |    | PORT_VID |    |    |  |  |  |  |
|    | R/W-0h   |    | R/W-0h   |    | R/W-0h   |    |    |  |  |  |  |
| 7  | 6        | 5  | 4        | 3  | 2        | 1  | 0  |  |  |  |  |
|    | PORT_VID |    |          |    |          |    |    |  |  |  |  |
|    | R/W-0h   |    |          |    |          |    |    |  |  |  |  |
|    |          |    |          |    |          |    |    |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-139. P0\_PORT\_VLAN Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                |
|-------|----------|------|-------|--------------------------------------------|
| 31-16 | RESERVED | R/W  | 0h    |                                            |
| 15-13 | PORT_PRI | R/W  | 0h    | Port VLAN Priority (7 is highest priority) |
| 12    | PORT_CFI | R/W  | 0h    | Port CFI bit                               |
| 11-0  | PORT_VID | R/W  | 0h    | Port VLAN ID                               |

#### **14.5.6.6 P0\_TX\_PRI\_MAP Register (offset = 18h) [reset = 33221001h]**

P0\_TX\_PRI\_MAP is shown in Figure [14-126](#page-47-1) and described in Table [14-140](#page-47-2). CPSW PORT 0 TX HEADER PRI TO SWITCH PRI MAPPING REGISTER

### **Figure 14-126. P0\_TX\_PRI\_MAP Register**

| 31       | 30 | 29     | 28 | 27       | 26 | 25     | 24 | 23 | 22       | 21     | 20   | 19 | 18       | 17     | 16   |  |
|----------|----|--------|----|----------|----|--------|----|----|----------|--------|------|----|----------|--------|------|--|
| RESERVED |    | PRI7   |    | RESERVED |    | PRI6   |    |    | RESERVED |        | PRI5 |    | RESERVED |        | PRI4 |  |
| R/W-0h   |    | R/W-3h |    | R/W-0h   |    | R/W-3h |    |    | R/W-0h   | R/W-2h |      |    | R/W-0h   | R/W-2h |      |  |
| 15       | 14 | 13     | 12 | 11       | 10 | 9      | 8  | 7  | 6        | 5      | 4    | 3  | 2        | 1      | 0    |  |
| RESERVED |    | PRI3   |    | RESERVED |    | PRI2   |    |    | RESERVED | PRI1   |      |    | RESERVED | PRI0   |      |  |
| R/W-0h   |    | R/W-1h |    | R/W-0h   |    | R/W-0h |    |    | R/W-0h   | R/W-0h |      |    | R/W-0h   | R/W-1h |      |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-140. P0\_TX\_PRI\_MAP Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                     |
|-------|----------|------|-------|---------------------------------------------------------------------------------|
| 31-30 | RESERVED | R/W  | 0h    |                                                                                 |
| 29-28 | PRI7     | R/W  | 3h    | Priority<br>7 - A packet header priority of 0x7 is given this switch queue pri. |
| 27-26 | RESERVED | R/W  | 0h    |                                                                                 |
| 25-24 | PRI6     | R/W  | 3h    | Priority<br>6 - A packet header priority of 0x6 is given this switch queue pri. |
| 23-22 | RESERVED | R/W  | 0h    |                                                                                 |
| 21-20 | PRI5     | R/W  | 2h    | Priority<br>5 - A packet header priority of 0x5 is given this switch queue pri. |
| 19-18 | RESERVED | R/W  | 0h    |                                                                                 |
| 17-16 | PRI4     | R/W  | 2h    | Priority<br>4 - A packet header priority of 0x4 is given this switch queue pri. |
| 15-14 | RESERVED | R/W  | 0h    |                                                                                 |
| 13-12 | PRI3     | R/W  | 1h    | Priority<br>3 - A packet header priority of 0x3 is given this switch queue pri. |
| 11-10 | RESERVED | R/W  | 0h    |                                                                                 |
| 9-8   | PRI2     | R/W  | 0h    | Priority<br>2 - A packet header priority of 0x2 is given this switch queue pri. |
| 7-6   | RESERVED | R/W  | 0h    |                                                                                 |
| 5-4   | PRI1     | R/W  | 0h    | Priority<br>1 - A packet header priority of 0x1 is given this switch queue pri. |
| 3-2   | RESERVED | R/W  | 0h    |                                                                                 |
| 1-0   | PRI0     | R/W  | 1h    | Priority<br>0 - A packet header priority of 0x0 is given this switch queue pri. |

#### **14.5.6.7 P0\_CPDMA\_TX\_PRI\_MAP Register (offset = 1Ch) [reset = 76543210h]**

P0\_CPDMA\_TX\_PRI\_MAP is shown in Figure [14-127](#page-48-1) and described in Table [14-141](#page-48-2). CPSW CPDMA TX (PORT 0 RX) PKT PRIORITY TO HEADER PRIORITY

**Figure 14-127. P0\_CPDMA\_TX\_PRI\_MAP Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI7   |    | RESE<br>RVED |    | PRI6   |    | RESE<br>RVED |    | PRI5   |    | RESE<br>RVED |    | PRI4   |    |
| R/W<br>0h    |    | R/W-7h |    | R/W<br>0h    |    | R/W-6h |    | R/W<br>0h    |    | R/W-5h |    | R/W<br>0h    |    | R/W-4h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI3   |    | RESE<br>RVED |    | PRI2   |    | RESE<br>RVED |    | PRI1   |    | RESE<br>RVED |    | PRI0   |    |
| R/W<br>0h    |    | R/W-3h |    | R/W<br>0h    |    | R/W-2h |    | R/W<br>0h    |    | R/W-1h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-141. P0\_CPDMA\_TX\_PRI\_MAP Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                           |
|-------|----------|------|-------|---------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                       |
| 30-28 | PRI7     | R/W  | 7h    | Priority<br>7 - A packet pri of 0x7 is mapped (changed) to this header packet<br>pri. |
| 27    | RESERVED | R/W  | 0h    |                                                                                       |
| 26-24 | PRI6     | R/W  | 6h    | Priority<br>6 - A packet pri of 0x6 is mapped (changed) to this header packet<br>pri. |
| 23    | RESERVED | R/W  | 0h    |                                                                                       |
| 22-20 | PRI5     | R/W  | 5h    | Priority<br>5 - A packet pri of 0x5 is mapped (changed) to this header packet<br>pri. |
| 19    | RESERVED | R/W  | 0h    |                                                                                       |
| 18-16 | PRI4     | R/W  | 4h    | Priority<br>4 - A packet pri of 0x4 is mapped (changed) to this header packet<br>pri. |
| 15    | RESERVED | R/W  | 0h    |                                                                                       |
| 14-12 | PRI3     | R/W  | 3h    | Priority<br>3 - A packet pri of 0x3 is mapped (changed) to this header packet<br>pri. |
| 11    | RESERVED | R/W  | 0h    |                                                                                       |
| 10-8  | PRI2     | R/W  | 2h    | Priority<br>2 - A packet pri of 0x2 is mapped (changed) to this header packet<br>pri. |
| 7     | RESERVED | R/W  | 0h    |                                                                                       |
| 6-4   | PRI1     | R/W  | 1h    | Priority<br>1 - A packet pri of 0x1 is mapped (changed) to this header packet<br>pri. |
| 3     | RESERVED | R/W  | 0h    |                                                                                       |
| 2-0   | PRI0     | R/W  | 0h    | Priority<br>0 - A packet pri of 0x0 is mapped (changed) to this header packet<br>pri. |

#### **14.5.6.8 P0\_CPDMA\_RX\_CH\_MAP Register (offset = 20h) [reset = 0h]**

P0\_CPDMA\_RX\_CH\_MAP is shown in Figure [14-128](#page-49-1) and described in Table [14-142.](#page-49-2) CPSW CPDMA RX (PORT 0 TX) SWITCH PRIORITY TO DMA CHANNEL

**Figure 14-128. P0\_CPDMA\_RX\_CH\_MAP Register**

| 31           | 30 | 29      | 28 | 27           | 26 | 25      | 24 | 23           | 22 | 21      | 20 | 19           | 18 | 17      | 16 |
|--------------|----|---------|----|--------------|----|---------|----|--------------|----|---------|----|--------------|----|---------|----|
| RESE<br>RVED |    | P2_PRI3 |    | RESE<br>RVED |    | P2_PRI2 |    | RESE<br>RVED |    | P2_PRI1 |    | RESE<br>RVED |    | P2_PRI0 |    |
| R/W<br>0h    |    | R/W-0h  |    | R/W<br>0h    |    | R/W-0h  |    | R/W<br>0h    |    | R/W-0h  |    | R/W<br>0h    |    | R/W-0h  |    |
| 15           | 14 | 13      | 12 | 11           | 10 | 9       | 8  | 7            | 6  | 5       | 4  | 3            | 2  | 1       | 0  |
| RESE<br>RVED |    | P1_PRI3 |    | RESE<br>RVED |    | P1_PRI2 |    | RESE<br>RVED |    | P1_PRI1 |    | RESE<br>RVED |    | P1_PRI0 |    |
| R/W<br>0h    |    | R/W-0h  |    | R/W<br>0h    |    | R/W-0h  |    | R/W<br>0h    |    | R/W-0h  |    | R/W<br>0h    |    | R/W-0h  |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-142. P0\_CPDMA\_RX\_CH\_MAP Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                           |
|-------|----------|------|-------|-------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                       |
| 30-28 | P2_PRI3  | R/W  | 0h    | Port 2 Priority 3 packets go to this CPDMA Rx Channel |
| 27    | RESERVED | R/W  | 0h    |                                                       |
| 26-24 | P2_PRI2  | R/W  | 0h    | Port 2 Priority 2 packets go to this CPDMA Rx Channel |
| 23    | RESERVED | R/W  | 0h    |                                                       |
| 22-20 | P2_PRI1  | R/W  | 0h    | Port 2 Priority 1 packets go to this CPDMA Rx Channel |
| 19    | RESERVED | R/W  | 0h    |                                                       |
| 18-16 | P2_PRI0  | R/W  | 0h    | Port 2 Priority 0 packets go to this CPDMA Rx Channel |
| 15    | RESERVED | R/W  | 0h    |                                                       |
| 14-12 | P1_PRI3  | R/W  | 0h    | Port 1 Priority 3 packets go to this CPDMA Rx Channel |
| 11    | RESERVED | R/W  | 0h    |                                                       |
| 10-8  | P1_PRI2  | R/W  | 0h    | Port 1 Priority 2 packets go to this CPDMA Rx Channel |
| 7     | RESERVED | R/W  | 0h    |                                                       |
| 6-4   | P1_PRI1  | R/W  | 0h    | Port 1 Priority 1 packets go to this CPDMA Rx Channel |
| 3     | RESERVED | R/W  | 0h    |                                                       |
| 2-0   | P1_PRI0  | R/W  | 0h    | Port 1 Priority 0 packets go to this CPDMA Rx Channel |

#### **14.5.6.9 P0\_RX\_DSCP\_PRI\_MAP0 Register (offset = 30h) [reset = 0h]**

P0\_RX\_DSCP\_PRI\_MAP0 is shown in Figure [14-129](#page-50-1) and described in Table [14-143](#page-50-2). CPSW PORT 0 RX DSCP PRIORITY TO RX PACKET MAPPING REG 0

**Figure 14-129. P0\_RX\_DSCP\_PRI\_MAP0 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI7   |    | RESE<br>RVED |    | PRI6   |    | RESE<br>RVED |    | PRI5   |    | RESE<br>RVED |    | PRI4   |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI3   |    | RESE<br>RVED |    | PRI2   |    | RESE<br>RVED |    | PRI1   |    | RESE<br>RVED |    | PRI0   |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-143. P0\_RX\_DSCP\_PRI\_MAP0 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                     |
|-------|----------|------|-------|---------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                 |
| 30-28 | PRI7     | R/W  | 0h    | Priority<br>7 - A packet TOS of 0d7 is mapped to this received packet priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                 |
| 26-24 | PRI6     | R/W  | 0h    | Priority<br>6 - A packet TOS of 0d6 is mapped to this received packet priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                 |
| 22-20 | PRI5     | R/W  | 0h    | Priority<br>5 - A packet TOS of 0d5 is mapped to this received packet priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                 |
| 18-16 | PRI4     | R/W  | 0h    | Priority<br>4 - A packet TOS of 0d4 is mapped to this received packet priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                 |
| 14-12 | PRI3     | R/W  | 0h    | Priority<br>3 - A packet TOS of 0d3 is mapped to this received packet priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                 |
| 10-8  | PRI2     | R/W  | 0h    | Priority<br>2 - A packet TOS of 0d2 is mapped to this received packet priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                 |
| 6-4   | PRI1     | R/W  | 0h    | Priority<br>1 - A packet TOS of 0d1 is mapped to this received packet priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                 |
| 2-0   | PRI0     | R/W  | 0h    | Priority<br>0 - A packet TOS of 0d0 is mapped to this received packet priority. |

#### **14.5.6.10 P0\_RX\_DSCP\_PRI\_MAP1 Register (offset = 34h) [reset = 0h]**

P0\_RX\_DSCP\_PRI\_MAP1 is shown in Figure [14-130](#page-51-1) and described in Table [14-144](#page-51-2). CPSW PORT 0 RX DSCP PRIORITY TO RX PACKET MAPPING REG 1

**Figure 14-130. P0\_RX\_DSCP\_PRI\_MAP1 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI15  |    | RESE<br>RVED |    | PRI14  |    | RESE<br>RVED |    | PRI13  |    | RESE<br>RVED |    | PRI12  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI11  |    | RESE<br>RVED |    | PRI10  |    | RESE<br>RVED |    | PRI9   |    | RESE<br>RVED |    | PRI8   |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-144. P0\_RX\_DSCP\_PRI\_MAP1 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI15    | R/W  | 0h    | Priority<br>15 - A packet TOS of 0d15 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI14    | R/W  | 0h    | Priority<br>14 - A packet TOS of 0d14 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI13    | R/W  | 0h    | Priority<br>13 - A packet TOS of 0d13 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI12    | R/W  | 0h    | Priority<br>12 - A packet TOS of 0d12 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI11    | R/W  | 0h    | Priority<br>11 - A packet TOS of 0d11 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI10    | R/W  | 0h    | Priority<br>10 - A packet TOS of 0d10 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI9     | R/W  | 0h    | Priority<br>9 - A packet TOS of 0d9 is mapped to this received packet priority.      |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI8     | R/W  | 0h    | Priority<br>8 - A packet TOS of 0d8 is mapped to this received packet priority.      |

#### **14.5.6.11 P0\_RX\_DSCP\_PRI\_MAP2 Register (offset = 38h) [reset = 0h]**

P0\_RX\_DSCP\_PRI\_MAP2 is shown in Figure [14-131](#page-52-1) and described in Table [14-145](#page-52-2). CPSW PORT 0 RX DSCP PRIORITY TO RX PACKET MAPPING REG 2

**Figure 14-131. P0\_RX\_DSCP\_PRI\_MAP2 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI23  |    | RESE<br>RVED |    | PRI22  |    | RESE<br>RVED |    | PRI21  |    | RESE<br>RVED |    | PRI20  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI19  |    | RESE<br>RVED |    | PRI18  |    | RESE<br>RVED |    | PRI17  |    | RESE<br>RVED |    | PRI16  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-145. P0\_RX\_DSCP\_PRI\_MAP2 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI23    | R/W  | 0h    | Priority<br>23 - A packet TOS of 0d23 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI22    | R/W  | 0h    | Priority<br>22 - A packet TOS of 0d22 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI21    | R/W  | 0h    | Priority<br>21 - A packet TOS of 0d21 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI20    | R/W  | 0h    | Priority<br>20 - A packet TOS of 0d20 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI19    | R/W  | 0h    | Priority<br>19 - A packet TOS of 0d19 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI18    | R/W  | 0h    | Priority<br>18 - A packet TOS of 0d18 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI17    | R/W  | 0h    | Priority<br>17 - A packet TOS of 0d17 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI16    | R/W  | 0h    | Priority<br>16 - A packet TOS of 0d16 is mapped to this received packet<br>priority. |

#### **14.5.6.12 P0\_RX\_DSCP\_PRI\_MAP3 Register (offset = 3Ch) [reset = 0h]**

P0\_RX\_DSCP\_PRI\_MAP3 is shown in Figure [14-132](#page-53-1) and described in Table [14-146](#page-53-2). CPSW PORT 0 RX DSCP PRIORITY TO RX PACKET MAPPING REG 3

**Figure 14-132. P0\_RX\_DSCP\_PRI\_MAP3 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI31  |    | RESE<br>RVED |    | PRI30  |    | RESE<br>RVED |    | PRI29  |    | RESE<br>RVED |    | PRI28  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI27  |    | RESE<br>RVED |    | PRI26  |    | RESE<br>RVED |    | PRI25  |    | RESE<br>RVED |    | PRI24  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-146. P0\_RX\_DSCP\_PRI\_MAP3 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI31    | R/W  | 0h    | Priority<br>31 - A packet TOS of 0d31 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI30    | R/W  | 0h    | Priority<br>30 - A packet TOS of 0d30 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI29    | R/W  | 0h    | Priority<br>29 - A packet TOS of 0d39 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI28    | R/W  | 0h    | Priority<br>28 - A packet TOS of 0d28 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI27    | R/W  | 0h    | Priority<br>27 - A packet TOS of 0d27 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI26    | R/W  | 0h    | Priority<br>26 - A packet TOS of 0d26 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI25    | R/W  | 0h    | Priority<br>25 - A packet TOS of 0d25 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI24    | R/W  | 0h    | Priority<br>24 - A packet TOS of 0d24 is mapped to this received packet<br>priority. |

#### **14.5.6.13 P0\_RX\_DSCP\_PRI\_MAP4 Register (offset = 40h) [reset = 0h]**

P0\_RX\_DSCP\_PRI\_MAP4 is shown in Figure [14-133](#page-54-1) and described in Table [14-147](#page-54-2). CPSW PORT 0 RX DSCP PRIORITY TO RX PACKET MAPPING REG 4

**Figure 14-133. P0\_RX\_DSCP\_PRI\_MAP4 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI39  |    | RESE<br>RVED |    | PRI38  |    | RESE<br>RVED |    | PRI37  |    | RESE<br>RVED |    | PRI36  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI35  |    | RESE<br>RVED |    | PRI34  |    | RESE<br>RVED |    | PRI33  |    | RESE<br>RVED |    | PRI32  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-147. P0\_RX\_DSCP\_PRI\_MAP4 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI39    | R/W  | 0h    | Priority<br>39 - A packet TOS of 0d39 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI38    | R/W  | 0h    | Priority<br>38 - A packet TOS of 0d38 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI37    | R/W  | 0h    | Priority<br>37 - A packet TOS of 0d37 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI36    | R/W  | 0h    | Priority<br>36 - A packet TOS of 0d36 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI35    | R/W  | 0h    | Priority<br>35 - A packet TOS of 0d35 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI34    | R/W  | 0h    | Priority<br>34 - A packet TOS of 0d34 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI33    | R/W  | 0h    | Priority<br>33 - A packet TOS of 0d33 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI32    | R/W  | 0h    | Priority<br>32 - A packet TOS of 0d32 is mapped to this received packet<br>priority. |

#### **14.5.6.14 P0\_RX\_DSCP\_PRI\_MAP5 Register (offset = 44h) [reset = 0h]**

P0\_RX\_DSCP\_PRI\_MAP5 is shown in Figure [14-134](#page-55-1) and described in Table [14-148](#page-55-2). CPSW PORT 0 RX DSCP PRIORITY TO RX PACKET MAPPING REG 5

**Figure 14-134. P0\_RX\_DSCP\_PRI\_MAP5 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI47  |    | RESE<br>RVED |    | PRI46  |    | RESE<br>RVED |    | PRI45  |    | RESE<br>RVED |    | PRI44  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI43  |    | RESE<br>RVED |    | PRI42  |    | RESE<br>RVED |    | PRI41  |    | RESE<br>RVED |    | PRI40  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-148. P0\_RX\_DSCP\_PRI\_MAP5 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI47    | R/W  | 0h    | Priority<br>47 - A packet TOS of 0d47 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI46    | R/W  | 0h    | Priority<br>46 - A packet TOS of 0d46 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI45    | R/W  | 0h    | Priority<br>45 - A packet TOS of 0d45 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI44    | R/W  | 0h    | Priority<br>44 - A packet TOS of 0d44 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI43    | R/W  | 0h    | Priority<br>43 - A packet TOS of 0d43 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI42    | R/W  | 0h    | Priority<br>42 - A packet TOS of 0d42 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI41    | R/W  | 0h    | Priority<br>41 - A packet TOS of 0d41 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI40    | R/W  | 0h    | Priority<br>40 - A packet TOS of 0d40 is mapped to this received packet<br>priority. |

#### **14.5.6.15 P0\_RX\_DSCP\_PRI\_MAP6 Register (offset = 48h) [reset = 0h]**

P0\_RX\_DSCP\_PRI\_MAP6 is shown in Figure [14-135](#page-56-1) and described in Table [14-149](#page-56-2). CPSW PORT 0 RX DSCP PRIORITY TO RX PACKET MAPPING REG 6

**Figure 14-135. P0\_RX\_DSCP\_PRI\_MAP6 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI55  |    | RESE<br>RVED |    | PRI54  |    | RESE<br>RVED |    | PRI53  |    | RESE<br>RVED |    | PRI52  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI51  |    | RESE<br>RVED |    | PRI50  |    | RESE<br>RVED |    | PRI49  |    | RESE<br>RVED |    | PRI48  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-149. P0\_RX\_DSCP\_PRI\_MAP6 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI55    | R/W  | 0h    | Priority<br>55 - A packet TOS of 0d55 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI54    | R/W  | 0h    | Priority<br>54 - A packet TOS of 0d54 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI53    | R/W  | 0h    | Priority<br>53 - A packet TOS of 0d53 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI52    | R/W  | 0h    | Priority<br>52 - A packet TOS of 0d52 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI51    | R/W  | 0h    | Priority<br>51 - A packet TOS of 0d51 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI50    | R/W  | 0h    | Priority<br>50 - A packet TOS of 0d50 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI49    | R/W  | 0h    | Priority<br>49 - A packet TOS of 0d49 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI48    | R/W  | 0h    | Priority<br>48 - A packet TOS of 0d48 is mapped to this received packet<br>priority. |

#### **14.5.6.16 P0\_RX\_DSCP\_PRI\_MAP7 Register (offset = 4Ch) [reset = 0h]**

P0\_RX\_DSCP\_PRI\_MAP7 is shown in Figure [14-136](#page-57-1) and described in Table [14-150](#page-57-2). CPSW PORT 0 RX DSCP PRIORITY TO RX PACKET MAPPING REG 7

**Figure 14-136. P0\_RX\_DSCP\_PRI\_MAP7 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI63  |    | RESE<br>RVED |    | PRI62  |    | RESE<br>RVED |    | PRI61  |    | RESE<br>RVED |    | PRI60  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI59  |    | RESE<br>RVED |    | PRI58  |    | RESE<br>RVED |    | PRI57  |    | RESE<br>RVED |    | PRI56  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-150. P0\_RX\_DSCP\_PRI\_MAP7 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI63    | R/W  | 0h    | Priority<br>63 - A packet TOS of 0d63 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI62    | R/W  | 0h    | Priority<br>62 - A packet TOS of 0d62 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI61    | R/W  | 0h    | Priority<br>61 - A packet TOS of 0d61 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI60    | R/W  | 0h    | Priority<br>60 - A packet TOS of 0d60 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI59    | R/W  | 0h    | Priority<br>59 - A packet TOS of 0d59 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI58    | R/W  | 0h    | Priority<br>58 - A packet TOS of 0d58 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI57    | R/W  | 0h    | Priority<br>57 - A packet TOS of 0d57 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI56    | R/W  | 0h    | Priority<br>56 - A packet TOS of 0d56 is mapped to this received packet<br>priority. |

#### **14.5.6.17 P1\_CONTROL Register (offset = 100h) [reset = 0h]**

P1\_CONTROL is shown in Figure [14-137](#page-58-1) and described in Table [14-151.](#page-58-2) CPSW PORT 1 CONTROL REGISTER

**Figure 14-137. P1\_CONTROL Register**

| 31       | 30        | 29                    | 28                    | 27                  | 26                  | 25          | 24                     |
|----------|-----------|-----------------------|-----------------------|---------------------|---------------------|-------------|------------------------|
|          |           |                       | RESERVED              |                     |                     |             | P1_PASS_PRI<br>_TAGGED |
|          |           |                       | R/W-0h                |                     |                     |             | R/W-0h                 |
| 23       | 22        | 21                    | 20                    | 19                  | 18                  | 17          | 16                     |
|          | RESERVED  | P1_VLAN_LTY<br>PE2_EN | P1_VLAN_LTY<br>PE1_EN |                     | RESERVED            |             | P1_DSCP_PRI<br>_EN     |
|          | R/W-0h    | R/W-0h                | R/W-0h                |                     | R/W-0h              |             | R/W-0h                 |
| 15       | 14        | 13                    | 12                    | 11                  | 10                  | 9           | 8                      |
| RESERVED | P1_TS_320 | P1_TS_319             | P1_TS_132             | P1_TS_131           | P1_TS_130           | P1_TS_129   | P1_TS_TTL_N<br>ONZERO  |
| R/W-0h   | R/W-0h    | R/W-0h                | R/W-0h                | R/W-0h              | R/W-0h              | R/W-0h      | R/W-0h                 |
| 7        | 6         | 5                     | 4                     | 3                   | 2                   | 1           | 0                      |
|          | RESERVED  |                       | P1_TS_ANNEX<br>_D_EN  | P1_TS_LTYPE<br>2_EN | P1_TS_LTYPE<br>1_EN | P1_TS_TX_EN | P1_TS_RX_EN            |
|          | R/W-0h    |                       | R/W-0h                | R-0h                | R/W-0h              | R/W-0h      | R/W-0h                 |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-151. P1\_CONTROL Register Field Descriptions**

| Bit   | Field              | Type | Reset | Description                                                                                                                                                                                                                                             |
|-------|--------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-25 | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 24    | P1_PASS_PRI_TAGGED | R/W  | 0h    | Port 1 Pass Priority Tagged<br>0 - Priority tagged packets have the zero VID replaced with the input<br>port P1_PORT_VLAN<br>[11:0]<br>1 - Priority tagged packets are processed unchanged.                                                             |
| 23-22 | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 21    | P1_VLAN_LTYPE2_EN  | R/W  | 0h    | Port 1 VLAN LTYPE 2 enable<br>0 - disabled<br>1 - VLAN LTYPE2 enabled on transmit and receive                                                                                                                                                           |
| 20    | P1_VLAN_LTYPE1_EN  | R/W  | 0h    | Port 1 VLAN LTYPE 1 enable<br>0 - disabled<br>1 - VLAN LTYPE1 enabled on transmit and receive                                                                                                                                                           |
| 19-17 | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 16    | P1_DSCP_PRI_EN     | R/W  | 0h    | Port 1 DSCP Priority Enable<br>0 - DSCP priority disabled<br>1 - DSCP priority enabled.<br>All non-tagged IPV4 packets have their received packet priority<br>determined by mapping the 6 TOS bits through the port DSCP<br>priority mapping registers. |
| 15    | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 14    | P1_TS_320          | R/W  | 0h    | Port 1 Time Sync Destination Port Number 320 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination port number<br>320 (decimal) is enabled.                                                                                     |
| 13    | P1_TS_319          | R/W  | 0h    | Port 1 Time Sync Destination Port Number 319 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination port number<br>319 (decimal) is enabled.                                                                                     |
| 12    | P1_TS_132          | R/W  | 0h    | Port 1 Time Sync Destination IP Address 132 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination IP address<br>number 132 (decimal) is enabled.                                                                                |

**Table 14-151. P1\_CONTROL Register Field Descriptions (continued)**

| Bit | Field             | Type | Reset | Description                                                                                                                                                              |
|-----|-------------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 11  | P1_TS_131         | R/W  | 0h    | Port 1 Time Sync Destination IP Address 131 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination IP address<br>number 131 (decimal) is enabled. |
| 10  | P1_TS_130         | R/W  | 0h    | Port 1 Time Sync Destination IP Address 130 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination IP address<br>number 130 (decimal) is enabled. |
| 9   | P1_TS_129         | R/W  | 0h    | Port 1 Time Sync Destination IP Address 129 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination IP address<br>number 129 (decimal) is enabled. |
| 8   | P1_TS_TTL_NONZERO | R/W  | 0h    | Port 1 Time Sync Time To Live Non-zero enable.<br>0 = TTL must be zero.<br>1 = TTL may be any value.                                                                     |
| 7-5 | RESERVED          | R/W  | 0h    |                                                                                                                                                                          |
| 4   | P1_TS_ANNEX_D_EN  | R/W  | 0h    | Port 1 Time Sync Annex D enable<br>0 - Annex D disabled<br>1 - Annex D enabled                                                                                           |
| 3   | P1_TS_LTYPE2_EN   | R    | 0h    | Port 1 Time Sync LTYPE 2 enable<br>0 - disabled<br>1 - enabled                                                                                                           |
| 2   | P1_TS_LTYPE1_EN   | R/W  | 0h    | Port 1 Time Sync LTYPE 1 enable<br>0 - disabled<br>1 - enabled                                                                                                           |
| 1   | P1_TS_TX_EN       | R/W  | 0h    | Port 1 Time Sync Transmit Enable<br>0 - disabled<br>1 - enabled                                                                                                          |
| 0   | P1_TS_RX_EN       | R/W  | 0h    | Port 1 Time Sync Receive Enable<br>0 - Port 1 Receive Time Sync disabled<br>1 - Port 1 Receive Time Sync enabled                                                         |

#### **14.5.6.18 P1\_MAX\_BLKS Register (offset = 108h) [reset = 113h]**

P1\_MAX\_BLKS is shown in Figure [14-138](#page-60-1) and described in Table [14-152.](#page-60-2)

CPSW PORT 1 MAXIMUM FIFO BLOCKS REGISTER

### **Figure 14-138. P1\_MAX\_BLKS Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        RESERVED                           |
|                        R/W-0h                             |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        RESERVED                           |
|                        R/W-0h                             |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------+-----------+
|                    RESERVED                    |P1_TX_MAX_|
|                    R/W-0h                      |  BLKS    |
|                                               (R/W-11h)   |
+-----------------------------------------------+-----------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-------------------------------+---------------------------+
|          P1_TX_MAX_BLKS       |       P1_RX_MAX_BLKS      |
|             R/W-11h           |           R/W-3h          |
+-------------------------------+---------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-152. P1\_MAX\_BLKS Register Field Descriptions**

| Bit  | Field          | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
|------|----------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-9 | RESERVED       | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 8-4  | P1_TX_MAX_BLKS | R/W  | 11h   | Transmit FIFO Maximum Blocks - This value is the maximum<br>number of 1k memory blocks that may be allocated to the FIFO's<br>logical transmit priority queues.<br>0x11 is the recommended value of p1_tx_max_blks unless the port<br>is in fullduplex flow control mode.<br>In flow control mode, the p1_rx_max_blks will need to increase in<br>order to accept the required run out in fullduplex mode.<br>This value will need to decrease by the amount of increase in<br>p1_rx_max_blks.<br>0xe is the minimum value tx max blks. |
| 3-0  | P1_RX_MAX_BLKS | R/W  | 3h    | Receive FIFO Maximum Blocks - This value is the maximum number<br>of 1k memory blocks that may be allocated to the FIFO's logical<br>receive queue.<br>This value must be greater than or equal to 0x3.<br>It should be increased In fullduplex flow control mode to 0x5 or 0x6<br>depending on the required runout space.<br>The p1_tx_max_blks value must be decreased by the amount of<br>increase in p1_rx_max_blks.<br>0x3 is the minimum value rx max blks and 0x6 is the maximum<br>value.                                       |

#### **14.5.6.19 P1\_BLK\_CNT Register (offset = 10Ch) [reset = 41h]**

P1\_BLK\_CNT is shown in Figure [14-139](#page-61-1) and described in Table [14-153](#page-61-2). CPSW PORT 1 FIFO BLOCK USAGE COUNT (READ ONLY)

**Figure 14-139. P1\_BLK\_CNT Register**

| 31 | 30            | 29 | 28       | 27       | 26            | 25 | 24                |
|----|---------------|----|----------|----------|---------------|----|-------------------|
|    |               |    |          | RESERVED |               |    |                   |
|    |               |    |          | R-0h     |               |    |                   |
| 23 | 22            | 21 | 20       | 19       | 18            | 17 | 16                |
|    |               |    |          | RESERVED |               |    |                   |
|    |               |    |          | R-0h     |               |    |                   |
| 15 | 14            | 13 | 12       | 11       | 10            | 9  | 8                 |
|    |               |    | RESERVED |          |               |    | P1_TX_BLK_C<br>NT |
|    |               |    | R-0h     |          |               |    | R-4h              |
| 7  | 6             | 5  | 4        | 3        | 2             | 1  | 0                 |
|    | P1_TX_BLK_CNT |    |          |          | P1_RX_BLK_CNT |    |                   |
|    | R-4h          |    |          |          | R-1h          |    |                   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-153. P1\_BLK\_CNT Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                              |
|------|---------------|------|-------|--------------------------------------------------------------------------------------------------------------------------|
| 31-9 | RESERVED      | R    | 0h    |                                                                                                                          |
| 8-4  | P1_TX_BLK_CNT | R    | 4h    | Port 1 Transmit Block Count Usage - This value is the number of<br>blocks allocated to the FIFO logical transmit queues. |
| 3-0  | P1_RX_BLK_CNT | R    | 1h    | Port 1 Receive Block Count Usage - This value is the number of<br>blocks allocated to the FIFO logical receive queues.   |

#### **14.5.6.20 P1\_TX\_IN\_CTL Register (offset = 110h) [reset = 80040C0h]**

P1\_TX\_IN\_CTL is shown in Figure [14-140](#page-62-1) and described in Table [14-154.](#page-62-2) CPSW PORT 1 TRANSMIT FIFO CONTROL

**Figure 14-140. P1\_TX\_IN\_CTL Register**

| 31 | 30 | 29          | 28         | 27            | 26       | 25     | 24         |  |  |
|----|----|-------------|------------|---------------|----------|--------|------------|--|--|
|    |    | RESERVED    |            | HOST_BLKS_REM |          |        |            |  |  |
|    |    | R/W-        |            | R/W-8h        |          |        |            |  |  |
| 23 | 22 | 21          | 20         | 19            | 18       | 17     | 16         |  |  |
|    |    | TX_RATE_EN  |            |               | RESERVED |        | TX_IN_SEL  |  |  |
|    |    | R/W-0h      |            |               | R/W-     | R/W-0h |            |  |  |
| 15 | 14 | 13          | 12         | 11            | 10       | 9      | 8          |  |  |
|    |    | TX_BLKS_REM |            |               | RESERVED |        | TX_PRI_WDS |  |  |
|    |    | R/W-4h      |            |               | R/W-     |        | R/W-C0h    |  |  |
| 7  | 6  | 5           | 4          | 3             | 2        | 1      | 0          |  |  |
|    |    |             | TX_PRI_WDS |               |          |        |            |  |  |
|    |    |             | R/W-C0h    |               |          |        |            |  |  |
|    |    |             |            |               |          |        |            |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-154. P1\_TX\_IN\_CTL Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                       |
|-------|---------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED      | R/W  |       |                                                                                                                                   |
| 27-24 | HOST_BLKS_REM | R/W  | 8h    | Transmit FIFO Blocks that must be free before a non rate-limited<br>CPDMA channel can begin sending a packet to the FIFO.         |
| 23-20 | TX_RATE_EN    | R/W  | 0h    | Transmit FIFO Input Rate Enable                                                                                                   |
| 19-18 | RESERVED      | R/W  |       |                                                                                                                                   |
| 17-16 | TX_IN_SEL     | R/W  | 0h    | Transmit FIFO Input Queue Type Select<br>00 - Normal priority mode<br>01 - reserved<br>10 - Rate Limit mode<br>11 - reserved      |
| 15-12 | TX_BLKS_REM   | R/W  | 4h    | Transmit FIFO Input Blocks to subtract in dual mac mode and blocks<br>to subtract on non rate-limited traffic in rate-limit mode. |
| 11-10 | RESERVED      | R/W  |       |                                                                                                                                   |
| 9-0   | TX_PRI_WDS    | R/W  | C0h   | Transmit FIFO Words in queue                                                                                                      |

#### **14.5.6.21 P1\_PORT\_VLAN Register (offset = 114h) [reset = 0h]**

P1\_PORT\_VLAN is shown in Figure [14-141](#page-63-1) and described in Table [14-155](#page-63-2). CPSW PORT 1 VLAN REGISTER

**Figure 14-141. P1\_PORT\_VLAN Register**

|    | 31<br>30<br>29<br>28<br>27<br>26<br>25<br>24 |    |          |    |          |    |    |  |  |  |  |  |  |  |
|----|----------------------------------------------|----|----------|----|----------|----|----|--|--|--|--|--|--|--|
|    |                                              |    |          |    |          |    |    |  |  |  |  |  |  |  |
|    |                                              |    | RESERVED |    |          |    |    |  |  |  |  |  |  |  |
|    |                                              |    | R/W-0h   |    |          |    |    |  |  |  |  |  |  |  |
| 23 | 22                                           | 21 | 20       | 19 | 18       | 17 | 16 |  |  |  |  |  |  |  |
|    |                                              |    | RESERVED |    |          |    |    |  |  |  |  |  |  |  |
|    | R/W-0h                                       |    |          |    |          |    |    |  |  |  |  |  |  |  |
| 15 | 14<br>13<br>12<br>11<br>10<br>9<br>8         |    |          |    |          |    |    |  |  |  |  |  |  |  |
|    | PORT_PRI                                     |    | PORT_CFI |    | PORT_VID |    |    |  |  |  |  |  |  |  |
|    | R/W-0h                                       |    | R/W-0h   |    | R/W-0h   |    |    |  |  |  |  |  |  |  |
| 7  | 6                                            | 5  | 4        | 3  | 2        | 1  | 0  |  |  |  |  |  |  |  |
|    |                                              |    | PORT_VID |    |          |    |    |  |  |  |  |  |  |  |
|    |                                              |    | R/W-0h   |    |          |    |    |  |  |  |  |  |  |  |
|    |                                              |    |          |    |          |    |    |  |  |  |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-155. P1\_PORT\_VLAN Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                |
|-------|----------|------|-------|--------------------------------------------|
| 31-16 | RESERVED | R/W  | 0h    |                                            |
| 15-13 | PORT_PRI | R/W  | 0h    | Port VLAN Priority (7 is highest priority) |
| 12    | PORT_CFI | R/W  | 0h    | Port CFI bit                               |
| 11-0  | PORT_VID | R/W  | 0h    | Port VLAN ID                               |

#### **14.5.6.22 P1\_TX\_PRI\_MAP Register (offset = 118h) [reset = 33221001h]**

P1\_TX\_PRI\_MAP is shown in Figure [14-142](#page-64-1) and described in Table [14-156](#page-64-2).

CPSW PORT 1 TX HEADER PRIORITY TO SWITCH PRI MAPPING REGISTER

### **Figure 14-142. P1\_TX\_PRI\_MAP Register**

| 30       | 29 | 28 | 27                               | 26 | 25                                       | 24     | 23   | 22               | 21       | 20     | 19             | 18     | 17                   | 16 |
|----------|----|----|----------------------------------|----|------------------------------------------|--------|------|------------------|----------|--------|----------------|--------|----------------------|----|
| RESERVED |    |    |                                  |    |                                          |        |      |                  |          |        |                |        | PRI4                 |    |
| R/W-0h   |    |    |                                  |    |                                          | R/W-3h |      | R/W-0h           |          |        | R/W-0h         |        | R/W-2h               |    |
| 14       | 13 | 12 | 11                               | 10 | 9                                        | 8      | 7    | 6                | 5        | 4      | 3              | 2      | 1                    | 0  |
| RESERVED |    |    |                                  |    |                                          | PRI2   |      | RESERVED         |          | PRI1   |                |        | PRI0                 |    |
| R/W-0h   |    |    |                                  |    | R/W-0h                                   |        |      | R/W-0h<br>R/W-0h |          | R/W-0h |                | R/W-1h |                      |    |
|          |    |    | PRI7<br>R/W-3h<br>PRI3<br>R/W-1h |    | RESERVED<br>R/W-0h<br>RESERVED<br>R/W-0h |        | PRI6 |                  | RESERVED |        | PRI5<br>R/W-2h |        | RESERVED<br>RESERVED |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-156. P1\_TX\_PRI\_MAP Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                     |
|-------|----------|------|-------|---------------------------------------------------------------------------------|
| 31-30 | RESERVED | R/W  | 0h    |                                                                                 |
| 29-28 | PRI7     | R/W  | 3h    | Priority<br>7 - A packet header priority of 0x7 is given this switch queue pri. |
| 27-26 | RESERVED | R/W  | 0h    |                                                                                 |
| 25-24 | PRI6     | R/W  | 3h    | Priority<br>6 - A packet header priority of 0x6 is given this switch queue pri. |
| 23-22 | RESERVED | R/W  | 0h    |                                                                                 |
| 21-20 | PRI5     | R/W  | 2h    | Priority<br>5 - A packet header priority of 0x5 is given this switch queue pri. |
| 19-18 | RESERVED | R/W  | 0h    |                                                                                 |
| 17-16 | PRI4     | R/W  | 2h    | Priority<br>4 - A packet header priority of 0x4 is given this switch queue pri. |
| 15-14 | RESERVED | R/W  | 0h    |                                                                                 |
| 13-12 | PRI3     | R/W  | 1h    | Priority<br>3 - A packet header priority of 0x3 is given this switch queue pri. |
| 11-10 | RESERVED | R/W  | 0h    |                                                                                 |
| 9-8   | PRI2     | R/W  | 0h    | Priority<br>2 - A packet header priority of 0x2 is given this switch queue pri. |
| 7-6   | RESERVED | R/W  | 0h    |                                                                                 |
| 5-4   | PRI1     | R/W  | 0h    | Priority<br>1 - A packet header priority of 0x1 is given this switch queue pri. |
| 3-2   | RESERVED | R/W  | 0h    |                                                                                 |
| 1-0   | PRI0     | R/W  | 1h    | Priority<br>0 - A packet header priority of 0x0 is given this switch queue pri. |

#### **14.5.6.23 P1\_TS\_SEQ\_MTYPE Register (offset = 11Ch) [reset = 1E0000h]**

P1\_TS\_SEQ\_MTYPE is shown in Figure [14-143](#page-65-1) and described in Table [14-157](#page-65-2). CPSW PORT 1 TIME SYNC SEQUENCE ID OFFSET AND MSG TYPE.

**Figure 14-143. P1\_TS\_SEQ\_MTYPE Register**

| 31 | 30                | 29 | 28 | 27                  | 26 | 25 | 24 |  |  |  |  |  |  |  |
|----|-------------------|----|----|---------------------|----|----|----|--|--|--|--|--|--|--|
|    |                   |    |    | RESERVED            |    |    |    |  |  |  |  |  |  |  |
|    |                   |    |    | R/W-0h              |    |    |    |  |  |  |  |  |  |  |
| 23 | 22                | 21 | 20 | 19                  | 18 | 17 | 16 |  |  |  |  |  |  |  |
|    | RESERVED          |    |    | P1_TS_SEQ_ID_OFFSET |    |    |    |  |  |  |  |  |  |  |
|    | R/W-0h<br>R/W-1Eh |    |    |                     |    |    |    |  |  |  |  |  |  |  |
| 15 | 14                | 13 | 12 | 11                  | 10 | 9  | 8  |  |  |  |  |  |  |  |
|    |                   |    |    | P1_TS_MSG_TYPE_EN   |    |    |    |  |  |  |  |  |  |  |
|    |                   |    |    | R/W-0h              |    |    |    |  |  |  |  |  |  |  |
| 7  | 6                 | 5  | 4  | 3                   | 2  | 1  | 0  |  |  |  |  |  |  |  |
|    |                   |    |    | P1_TS_MSG_TYPE_EN   |    |    |    |  |  |  |  |  |  |  |
|    |                   |    |    | R/W-0h              |    |    |    |  |  |  |  |  |  |  |
|    |                   |    |    |                     |    |    |    |  |  |  |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-157. P1\_TS\_SEQ\_MTYPE Register Field Descriptions**

| Bit   | Field               | Type | Reset | Description                                                                                                                                                                                |
|-------|---------------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-22 | RESERVED            | R/W  | 0h    |                                                                                                                                                                                            |
| 21-16 | P1_TS_SEQ_ID_OFFSET | R/W  | 1Eh   | Port 1 Time Sync Sequence ID Offset This is the number of octets<br>that the sequence ID is offset in the tx and rx time sync message<br>header.<br>The minimum value is 6.                |
| 15-0  | P1_TS_MSG_TYPE_EN   | R/W  | 0h    | Port 1 Time Sync Message Type Enable - Each bit in this field<br>enables the corresponding message type in receive and transmit<br>time sync messages (Bit 0 enables message type 0 etc.). |

#### **14.5.6.24 P1\_SA\_LO Register (offset = 120h) [reset = 0h]**

P1\_SA\_LO is shown in Figure [14-144](#page-66-1) and described in Table [14-158](#page-66-2). CPSW CPGMAC\_SL1 SOURCE ADDRESS LOW REGISTER

**Figure 14-144. P1\_SA\_LO Register**

```
ROW 1  (BITS 31..16)
|                                                       RESERVED                                                            |
|                                                       R/W-0h                                                              |
+---------------------------------------------------------------------------------------------------------------------------+

ROW 2  (BITS 15..0)
|                        MACSRCADDR_7_0                         |                     MACSRCADDR_15_8                       |
|                             R/W-0h                            |                           R/W-0h                          |
+---------------------------------------------------------------+-----------------------------------------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-158. P1\_SA\_LO Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                          |
|-------|-----------------|------|-------|--------------------------------------|
| 31-16 | RESERVED        | R/W  | 0h    |                                      |
| 15-8  | MACSRCADDR_7_0  | R/W  | 0h    | Source Address Lower 8 bits (byte 0) |
| 7-0   | MACSRCADDR_15_8 | R/W  | 0h    | Source Address bits<br>15:8 (byte 1) |

#### **14.5.6.25 P1\_SA\_HI Register (offset = 124h) [reset = 0h]**

P1\_SA\_HI is shown in Figure [14-145](#page-67-1) and described in Table [14-159.](#page-67-2) CPSW CPGMAC\_SL1 SOURCE ADDRESS HIGH REGISTER

**Figure 14-145. P1\_SA\_HI Register**

| 31 | 30 | 29 | 28 | 27               | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19               | 18 | 17 | 16 |
|----|----|----|----|------------------|----|----|----|----|----|----|----|------------------|----|----|----|
|    |    |    |    | MACSRCADDR_23_16 |    |    |    |    |    |    |    | MACSRCADDR_31_24 |    |    |    |
|    |    |    |    | R/W-0h           |    |    |    |    |    |    |    | R/W-0h           |    |    |    |
| 15 | 14 | 13 | 12 | 11               | 10 | 9  | 8  | 7  | 6  | 5  | 4  | 3                | 2  | 1  | 0  |
|    |    |    |    | MACSRCADDR_39_32 |    |    |    |    |    |    |    | MACSRCADDR_47_40 |    |    |    |
|    |    |    |    | R/W-0h           |    |    |    |    |    |    |    | R/W-0h           |    |    |    |
|    |    |    |    |                  |    |    |    |    |    |    |    |                  |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-159. P1\_SA\_HI Register Field Descriptions**

| Bit   | Field            | Type | Reset | Description                           |
|-------|------------------|------|-------|---------------------------------------|
| 31-24 | MACSRCADDR_23_16 | R/W  | 0h    | Source Address bits<br>23:16 (byte 2) |
| 23-16 | MACSRCADDR_31_24 | R/W  | 0h    | Source Address bits<br>31:24 (byte 3) |
| 15-8  | MACSRCADDR_39_32 | R/W  | 0h    | Source Address bits<br>39:32 (byte 4) |
| 7-0   | MACSRCADDR_47_40 | R/W  | 0h    | Source Address bits<br>47:40 (byte 5) |

#### **14.5.6.26 P1\_SEND\_PERCENT Register (offset = 128h) [reset = 0h]**

P1\_SEND\_PERCENT is shown in Figure [14-146](#page-68-1) and described in Table [14-160](#page-68-2). CPSW PORT 1 TRANSMIT QUEUE SEND PERCENTAGES

**Figure 14-146. P1\_SEND\_PERCENT Register**

| 31           | 30                  | 29 | 28 | 27                | 26 | 25 | 24           | 23 | 22 | 21 | 20                | 19                | 18 | 17 | 16 |
|--------------|---------------------|----|----|-------------------|----|----|--------------|----|----|----|-------------------|-------------------|----|----|----|
|              |                     |    |    | RESERVED          |    |    |              |    |    |    |                   | PRI3_SEND_PERCENT |    |    |    |
|              |                     |    |    | R/W-0h            |    |    |              |    |    |    |                   | R/W-0h            |    |    |    |
| 15           | 14                  | 13 | 12 | 11                | 10 | 9  | 8            | 7  | 6  | 5  | 4                 | 3                 | 2  | 1  | 0  |
| RESE<br>RVED |                     |    |    | PRI2_SEND_PERCENT |    |    | RESE<br>RVED |    |    |    | PRI1_SEND_PERCENT |                   |    |    |    |
| R/W<br>0h    | R/W-0h<br>R/W<br>0h |    |    |                   |    |    |              |    |    |    |                   | R/W-0h            |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-160. P1\_SEND\_PERCENT Register Field Descriptions**

| Bit   | Field             | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                |
|-------|-------------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-23 | RESERVED          | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                            |
| 22-16 | PRI3_SEND_PERCENT | R/W  | 0h    | Priority 3 Transmit Percentage - This percentage value is sent from<br>FIFO priority 3 (maximum) when the p1_pri3_shape_en is set<br>(queue shaping enabled).<br>This is the percentage of the wire that packets from priority 3 receive<br>(which includes interpacket gap and preamble bytes).<br>If shaping is enabled on this queue then this value must be between<br>zero and 0d100 (not inclusive). |
| 15    | RESERVED          | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                            |
| 14-8  | PRI2_SEND_PERCENT | R/W  | 0h    | Priority 2 Transmit Percentage - This percentage value is sent from<br>FIFO priority 2 (maximum) when the p1_pri2_shape_en is set<br>(queue shaping enabled).<br>This is the percentage of the wire that packets from priority 2 receive<br>(which includes interpacket gap and preamble bytes).<br>If shaping is enabled on this queue then this value must be between<br>zero and 0d100 (not inclusive). |
| 7     | RESERVED          | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                            |
| 6-0   | PRI1_SEND_PERCENT | R/W  | 0h    | Priority 1 Transmit Percentage - This percentage value is sent from<br>FIFO priority 1 (maximum) when the p1_pri1_shape_en is set<br>(queue shaping enabled).<br>This is the percentage of the wire that packets from priority 1 receive<br>(which includes interpacket gap and preamble bytes).<br>If shaping is enabled on this queue then this value must be between<br>zero and 0d100 (not inclusive). |

#### **14.5.6.27 P1\_RX\_DSCP\_PRI\_MAP0 Register (offset = 130h) [reset = 0h]**

P1\_RX\_DSCP\_PRI\_MAP0 is shown in Figure [14-147](#page-69-1) and described in Table [14-161](#page-69-2). CPSW PORT 1 RX DSCP PRIORITY TO RX PACKET MAPPING REG 0

**Figure 14-147. P1\_RX\_DSCP\_PRI\_MAP0 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI7   |    | RESE<br>RVED |    | PRI6   |    | RESE<br>RVED |    | PRI5   |    | RESE<br>RVED |    | PRI4   |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI3   |    | RESE<br>RVED |    | PRI2   |    | RESE<br>RVED |    | PRI1   |    | RESE<br>RVED |    | PRI0   |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-161. P1\_RX\_DSCP\_PRI\_MAP0 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                     |
|-------|----------|------|-------|---------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                 |
| 30-28 | PRI7     | R/W  | 0h    | Priority<br>7 - A packet TOS of 0d7 is mapped to this received packet priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                 |
| 26-24 | PRI6     | R/W  | 0h    | Priority<br>6 - A packet TOS of 0d6 is mapped to this received packet priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                 |
| 22-20 | PRI5     | R/W  | 0h    | Priority<br>5 - A packet TOS of 0d5 is mapped to this received packet priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                 |
| 18-16 | PRI4     | R/W  | 0h    | Priority<br>4 - A packet TOS of 0d4 is mapped to this received packet priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                 |
| 14-12 | PRI3     | R/W  | 0h    | Priority<br>3 - A packet TOS of 0d3 is mapped to this received packet priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                 |
| 10-8  | PRI2     | R/W  | 0h    | Priority<br>2 - A packet TOS of 0d2 is mapped to this received packet priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                 |
| 6-4   | PRI1     | R/W  | 0h    | Priority<br>1 - A packet TOS of 0d1 is mapped to this received packet priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                 |
| 2-0   | PRI0     | R/W  | 0h    | Priority<br>0 - A packet TOS of 0d0 is mapped to this received packet priority. |

#### **14.5.6.28 P1\_RX\_DSCP\_PRI\_MAP1 Register (offset = 134h) [reset = 0h]**

P1\_RX\_DSCP\_PRI\_MAP1 is shown in Figure [14-148](#page-70-1) and described in Table [14-162](#page-70-2). CPSW PORT 1 RX DSCP PRIORITY TO RX PACKET MAPPING REG 1

**Figure 14-148. P1\_RX\_DSCP\_PRI\_MAP1 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI15  |    | RESE<br>RVED |    | PRI14  |    | RESE<br>RVED |    | PRI13  |    | RESE<br>RVED |    | PRI12  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI11  |    | RESE<br>RVED |    | PRI10  |    | RESE<br>RVED |    | PRI9   |    | RESE<br>RVED |    | PRI8   |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-162. P1\_RX\_DSCP\_PRI\_MAP1 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI15    | R/W  | 0h    | Priority<br>15 - A packet TOS of 0d15 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI14    | R/W  | 0h    | Priority<br>14 - A packet TOS of 0d14 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI13    | R/W  | 0h    | Priority<br>13 - A packet TOS of 0d13 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI12    | R/W  | 0h    | Priority<br>12 - A packet TOS of 0d12 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI11    | R/W  | 0h    | Priority<br>11 - A packet TOS of 0d11 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI10    | R/W  | 0h    | Priority<br>10 - A packet TOS of 0d10 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI9     | R/W  | 0h    | Priority<br>9 - A packet TOS of 0d9 is mapped to this received packet priority.      |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI8     | R/W  | 0h    | Priority<br>8 - A packet TOS of 0d8 is mapped to this received packet priority.      |

#### **14.5.6.29 P1\_RX\_DSCP\_PRI\_MAP2 Register (offset = 138h) [reset = 0h]**

P1\_RX\_DSCP\_PRI\_MAP2 is shown in Figure [14-149](#page-71-1) and described in Table [14-163](#page-71-2). CPSW PORT 1 RX DSCP PRIORITY TO RX PACKET MAPPING REG 2

**Figure 14-149. P1\_RX\_DSCP\_PRI\_MAP2 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI23  |    | RESE<br>RVED |    | PRI22  |    | RESE<br>RVED |    | PRI21  |    | RESE<br>RVED |    | PRI20  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI19  |    | RESE<br>RVED |    | PRI18  |    | RESE<br>RVED |    | PRI17  |    | RESE<br>RVED |    | PRI16  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-163. P1\_RX\_DSCP\_PRI\_MAP2 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI23    | R/W  | 0h    | Priority<br>23 - A packet TOS of 0d23 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI22    | R/W  | 0h    | Priority<br>22 - A packet TOS of 0d22 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI21    | R/W  | 0h    | Priority<br>21 - A packet TOS of 0d21 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI20    | R/W  | 0h    | Priority<br>20 - A packet TOS of 0d20 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI19    | R/W  | 0h    | Priority<br>19 - A packet TOS of 0d19 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI18    | R/W  | 0h    | Priority<br>18 - A packet TOS of 0d18 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI17    | R/W  | 0h    | Priority<br>17 - A packet TOS of 0d17 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI16    | R/W  | 0h    | Priority<br>16 - A packet TOS of 0d16 is mapped to this received packet<br>priority. |

#### **14.5.6.30 P1\_RX\_DSCP\_PRI\_MAP3 Register (offset = 13Ch) [reset = 0h]**

P1\_RX\_DSCP\_PRI\_MAP3 is shown in Figure [14-150](#page-72-1) and described in Table [14-164](#page-72-2). CPSW PORT 1 RX DSCP PRIORITY TO RX PACKET MAPPING REG 3

**Figure 14-150. P1\_RX\_DSCP\_PRI\_MAP3 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI31  |    | RESE<br>RVED |    | PRI30  |    | RESE<br>RVED |    | PRI29  |    | RESE<br>RVED |    | PRI28  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI27  |    | RESE<br>RVED |    | PRI26  |    | RESE<br>RVED |    | PRI25  |    | RESE<br>RVED |    | PRI24  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-164. P1\_RX\_DSCP\_PRI\_MAP3 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI31    | R/W  | 0h    | Priority<br>31 - A packet TOS of 0d31 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI30    | R/W  | 0h    | Priority<br>30 - A packet TOS of 0d30 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI29    | R/W  | 0h    | Priority<br>29 - A packet TOS of 0d39 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI28    | R/W  | 0h    | Priority<br>28 - A packet TOS of 0d28 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI27    | R/W  | 0h    | Priority<br>27 - A packet TOS of 0d27 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI26    | R/W  | 0h    | Priority<br>26 - A packet TOS of 0d26 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI25    | R/W  | 0h    | Priority<br>25 - A packet TOS of 0d25 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI24    | R/W  | 0h    | Priority<br>24 - A packet TOS of 0d24 is mapped to this received packet<br>priority. |

#### **14.5.6.31 P1\_RX\_DSCP\_PRI\_MAP4 Register (offset = 140h) [reset = 0h]**

P1\_RX\_DSCP\_PRI\_MAP4 is shown in Figure [14-151](#page-73-1) and described in Table [14-165](#page-73-2). CPSW PORT 1 RX DSCP PRIORITY TO RX PACKET MAPPING REG 4

**Figure 14-151. P1\_RX\_DSCP\_PRI\_MAP4 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI39  |    | RESE<br>RVED |    | PRI38  |    | RESE<br>RVED |    | PRI37  |    | RESE<br>RVED |    | PRI36  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI35  |    | RESE<br>RVED |    | PRI34  |    | RESE<br>RVED |    | PRI33  |    | RESE<br>RVED |    | PRI32  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-165. P1\_RX\_DSCP\_PRI\_MAP4 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI39    | R/W  | 0h    | Priority<br>39 - A packet TOS of 0d39 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI38    | R/W  | 0h    | Priority<br>38 - A packet TOS of 0d38 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI37    | R/W  | 0h    | Priority<br>37 - A packet TOS of 0d37 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI36    | R/W  | 0h    | Priority<br>36 - A packet TOS of 0d36 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI35    | R/W  | 0h    | Priority<br>35 - A packet TOS of 0d35 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI34    | R/W  | 0h    | Priority<br>34 - A packet TOS of 0d34 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI33    | R/W  | 0h    | Priority<br>33 - A packet TOS of 0d33 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI32    | R/W  | 0h    | Priority<br>32 - A packet TOS of 0d32 is mapped to this received packet<br>priority. |

#### **14.5.6.32 P1\_RX\_DSCP\_PRI\_MAP5 Register (offset = 144h) [reset = 0h]**

P1\_RX\_DSCP\_PRI\_MAP5 is shown in Figure [14-152](#page-74-1) and described in Table [14-166](#page-74-2). CPSW PORT 1 RX DSCP PRIORITY TO RX PACKET MAPPING REG 5

**Figure 14-152. P1\_RX\_DSCP\_PRI\_MAP5 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI47  |    | RESE<br>RVED |    | PRI46  |    | RESE<br>RVED |    | PRI45  |    | RESE<br>RVED |    | PRI44  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI43  |    | RESE<br>RVED |    | PRI42  |    | RESE<br>RVED |    | PRI41  |    | RESE<br>RVED |    | PRI40  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-166. P1\_RX\_DSCP\_PRI\_MAP5 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI47    | R/W  | 0h    | Priority<br>47 - A packet TOS of 0d47 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI46    | R/W  | 0h    | Priority<br>46 - A packet TOS of 0d46 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI45    | R/W  | 0h    | Priority<br>45 - A packet TOS of 0d45 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI44    | R/W  | 0h    | Priority<br>44 - A packet TOS of 0d44 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI43    | R/W  | 0h    | Priority<br>43 - A packet TOS of 0d43 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI42    | R/W  | 0h    | Priority<br>42 - A packet TOS of 0d42 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI41    | R/W  | 0h    | Priority<br>41 - A packet TOS of 0d41 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI40    | R/W  | 0h    | Priority<br>40 - A packet TOS of 0d40 is mapped to this received packet<br>priority. |

#### **14.5.6.33 P1\_RX\_DSCP\_PRI\_MAP6 Register (offset = 148h) [reset = 0h]**

P1\_RX\_DSCP\_PRI\_MAP6 is shown in Figure [14-153](#page-75-1) and described in Table [14-167](#page-75-2). CPSW PORT 1 RX DSCP PRIORITY TO RX PACKET MAPPING REG 6

**Figure 14-153. P1\_RX\_DSCP\_PRI\_MAP6 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI55  |    | RESE<br>RVED |    | PRI54  |    | RESE<br>RVED |    | PRI53  |    | RESE<br>RVED |    | PRI52  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI51  |    | RESE<br>RVED |    | PRI50  |    | RESE<br>RVED |    | PRI49  |    | RESE<br>RVED |    | PRI48  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-167. P1\_RX\_DSCP\_PRI\_MAP6 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI55    | R/W  | 0h    | Priority<br>55 - A packet TOS of 0d55 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI54    | R/W  | 0h    | Priority<br>54 - A packet TOS of 0d54 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI53    | R/W  | 0h    | Priority<br>53 - A packet TOS of 0d53 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI52    | R/W  | 0h    | Priority<br>52 - A packet TOS of 0d52 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI51    | R/W  | 0h    | Priority<br>51 - A packet TOS of 0d51 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI50    | R/W  | 0h    | Priority<br>50 - A packet TOS of 0d50 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI49    | R/W  | 0h    | Priority<br>49 - A packet TOS of 0d49 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI48    | R/W  | 0h    | Priority<br>48 - A packet TOS of 0d48 is mapped to this received packet<br>priority. |

#### **14.5.6.34 P1\_RX\_DSCP\_PRI\_MAP7 Register (offset = 14Ch) [reset = 0h]**

P1\_RX\_DSCP\_PRI\_MAP7 is shown in Figure [14-154](#page-76-1) and described in Table [14-168](#page-76-2). CPSW PORT 1 RX DSCP PRIORITY TO RX PACKET MAPPING REG 7

**Figure 14-154. P1\_RX\_DSCP\_PRI\_MAP7 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI63  |    | RESE<br>RVED |    | PRI62  |    | RESE<br>RVED |    | PRI61  |    | RESE<br>RVED |    | PRI60  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI59  |    | RESE<br>RVED |    | PRI58  |    | RESE<br>RVED |    | PRI57  |    | RESE<br>RVED |    | PRI56  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-168. P1\_RX\_DSCP\_PRI\_MAP7 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI63    | R/W  | 0h    | Priority<br>63 - A packet TOS of 0d63 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI62    | R/W  | 0h    | Priority<br>62 - A packet TOS of 0d62 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI61    | R/W  | 0h    | Priority<br>61 - A packet TOS of 0d61 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI60    | R/W  | 0h    | Priority<br>60 - A packet TOS of 0d60 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI59    | R/W  | 0h    | Priority<br>59 - A packet TOS of 0d59 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI58    | R/W  | 0h    | Priority<br>58 - A packet TOS of 0d58 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI57    | R/W  | 0h    | Priority<br>57 - A packet TOS of 0d57 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI56    | R/W  | 0h    | Priority<br>56 - A packet TOS of 0d56 is mapped to this received packet<br>priority. |

#### **14.5.6.35 P2\_CONTROL Register (offset = 200h) [reset = 0h]**

P2\_CONTROL is shown in Figure [14-155](#page-77-1) and described in Table [14-169.](#page-77-2) CPSW\_3GF PORT 2 CONTROL REGISTER

**Figure 14-155. P2\_CONTROL Register**

| 31       | 30        | 29                    | 28                    | 27                  | 26                  | 25          | 24                     |
|----------|-----------|-----------------------|-----------------------|---------------------|---------------------|-------------|------------------------|
|          |           |                       | RESERVED              |                     |                     |             | P2_PASS_PRI<br>_TAGGED |
|          |           |                       | R/W-0h                |                     |                     |             | R/W-0h                 |
| 23       | 22        | 21                    | 20                    | 19                  | 18                  | 17          | 16                     |
|          | RESERVED  | P2_VLAN_LTY<br>PE2_EN | P2_VLAN_LTY<br>PE1_EN |                     | RESERVED            |             | P2_DSCP_PRI<br>_EN     |
|          | R/W-0h    | R/W-0h                | R/W-0h                |                     | R/W-0h              |             | R/W-0h                 |
| 15       | 14        | 13                    | 12                    | 11                  | 10                  | 9           | 8                      |
| RESERVED | P2_TS_320 | P2_TS_319             | P2_TS_132             | P2_TS_131           | P2_TS_130           | P2_TS_129   | P2_TS_TTL_N<br>ONZERO  |
| R/W-0h   | R/W-0h    | R/W-0h                | R/W-0h                | R/W-0h              | R/W-0h              | R/W-0h      | R/W-0h                 |
| 7        | 6         | 5                     | 4                     | 3                   | 2                   | 1           | 0                      |
|          | RESERVED  |                       | P2_TS_ANNEX<br>_D_EN  | P2_TS_LTYPE<br>2_EN | P2_TS_LTYPE<br>1_EN | P2_TS_TX_EN | P2_TS_RX_EN            |
|          | R/W-0h    |                       | R/W-0h                | R-0h                | R/W-0h              | R/W-0h      | R/W-0h                 |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-169. P2\_CONTROL Register Field Descriptions**

| Bit   | Field              | Type | Reset | Description                                                                                                                                                                                                                                             |
|-------|--------------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-25 | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 24    | P2_PASS_PRI_TAGGED | R/W  | 0h    | Port 2 Pass Priority Tagged<br>0 - Priority tagged packets have the zero VID replaced with the input<br>port P2_PORT_VLAN<br>[11:0]<br>1 - Priority tagged packets are processed unchanged.                                                             |
| 23-22 | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 21    | P2_VLAN_LTYPE2_EN  | R/W  | 0h    | Port 2 VLAN LTYPE 2 enable<br>0 - disabled<br>1 - VLAN LTYPE2 enabled on transmit and receive                                                                                                                                                           |
| 20    | P2_VLAN_LTYPE1_EN  | R/W  | 0h    | Port 2 VLAN LTYPE 1 enable<br>0 - disabled<br>1 - VLAN LTYPE1 enabled on transmit and receive                                                                                                                                                           |
| 19-17 | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 16    | P2_DSCP_PRI_EN     | R/W  | 0h    | Port 0 DSCP Priority Enable<br>0 - DSCP priority disabled<br>1 - DSCP priority enabled.<br>All non-tagged IPV4 packets have their received packet priority<br>determined by mapping the 6 TOS bits through the port DSCP<br>priority mapping registers. |
| 15    | RESERVED           | R/W  | 0h    |                                                                                                                                                                                                                                                         |
| 14    | P2_TS_320          | R/W  | 0h    | Port 2 Time Sync Destination Port Number 320 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination port number<br>320 (decimal) is enabled.                                                                                     |
| 13    | P2_TS_319          | R/W  | 0h    | Port 2 Time Sync Destination Port Number 319 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination port number<br>319 (decimal) is enabled.                                                                                     |
| 12    | P2_TS_132          | R/W  | 0h    | Port 2 Time Sync Destination IP Address 132 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination IP address<br>number 132 (decimal) is enabled.                                                                                |

**Table 14-169. P2\_CONTROL Register Field Descriptions (continued)**

| Bit | Field             | Type | Reset | Description                                                                                                                                                              |
|-----|-------------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 11  | P2_TS_131         | R/W  | 0h    | Port 2 Time Sync Destination IP Address 131 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination IP address<br>number 131 (decimal) is enabled. |
| 10  | P2_TS_130         | R/W  | 0h    | Port 2 Time Sync Destination IP Address 130 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination IP address<br>number 130 (decimal) is enabled. |
| 9   | P2_TS_129         | R/W  | 0h    | Port 2 Time Sync Destination IP Address 129 enable<br>0 - disabled<br>1 - Annex D (UDP/IPv4) time sync packet destination IP address<br>number 129 (decimal) is enabled. |
| 8   | P2_TS_TTL_NONZERO | R/W  | 0h    | Port 2 Time Sync Time To Live Non-zero enable.<br>0 = TTL must be zero.<br>1 = TTL may be any value.                                                                     |
| 7-5 | RESERVED          | R/W  | 0h    |                                                                                                                                                                          |
| 4   | P2_TS_ANNEX_D_EN  | R/W  | 0h    | Port 2 Time Sync Annex D enable<br>0 - Annex D disabled<br>1 - Annex D enabled                                                                                           |
| 3   | P2_TS_LTYPE2_EN   | R    | 0h    | Port 2 Time Sync LTYPE 2 enable<br>0 - disabled<br>1 - enabled                                                                                                           |
| 2   | P2_TS_LTYPE1_EN   | R/W  | 0h    | Port 2 Time Sync LTYPE 1 enable<br>0 - disabled<br>1 - enabled                                                                                                           |
| 1   | P2_TS_TX_EN       | R/W  | 0h    | Port 2 Time Sync Transmit Enable<br>0 - disabled<br>1 - enabled                                                                                                          |
| 0   | P2_TS_RX_EN       | R/W  | 0h    | Port 2 Time Sync Receive Enable<br>0 - Port 1 Receive Time Sync disabled<br>1 - Port 1 Receive Time Sync enabled                                                         |

#### **14.5.6.36 P2\_MAX\_BLKS Register (offset = 208h) [reset = 113h]**

P2\_MAX\_BLKS is shown in Figure [14-156](#page-79-1) and described in Table [14-170.](#page-79-2) CPSW PORT 2 MAXIMUM FIFO BLOCKS REGISTER

**Figure 14-156. P2\_MAX\_BLKS Register**

| 31 | 30             | 29 | 28       | 27       | 26             | 25 | 24                 |
|----|----------------|----|----------|----------|----------------|----|--------------------|
|    |                |    |          | RESERVED |                |    |                    |
|    |                |    |          | R/W-0h   |                |    |                    |
| 23 | 22             | 21 | 20       | 19       | 18             | 17 | 16                 |
|    |                |    |          | RESERVED |                |    |                    |
|    |                |    |          | R/W-0h   |                |    |                    |
| 15 | 14             | 13 | 12       | 11       | 10             | 9  | 8                  |
|    |                |    | RESERVED |          |                |    | P2_TX_MAX_B<br>LKS |
|    |                |    | R/W-0h   |          |                |    | R/W-11h            |
| 7  | 6              | 5  | 4        | 3        | 2              | 1  | 0                  |
|    | P2_TX_MAX_BLKS |    |          |          | P2_RX_MAX_BLKS |    |                    |
|    | R/W-11h        |    |          |          | R/W-3h         |    |                    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-170. P2\_MAX\_BLKS Register Field Descriptions**

| Bit  | Field          | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
|------|----------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-9 | RESERVED       | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 8-4  | P2_TX_MAX_BLKS | R/W  | 11h   | Transmit FIFO Maximum Blocks - This value is the maximum<br>number of 1k memory blocks that may be allocated to the FIFO's<br>logical transmit priority queues.<br>0x11 is the recommended value of p2_tx_max_blks unless the port<br>is in fullduplex flow control mode.<br>In flow control mode, the p2_rx_max_blks will need to increase in<br>order to accept the required run out in fullduplex mode.<br>This value will need to decrease by the amount of increase in<br>p2_rx_max_blks.<br>0xe is the minimum value tx max blks. |
| 3-0  | P2_RX_MAX_BLKS | R/W  | 3h    | Receive FIFO Maximum Blocks - This value is the maximum number<br>of 1k memory blocks that may be allocated to the FIFO's logical<br>receive queue.<br>This value must be greater than or equal to 0x3.<br>It should be increased In fullduplex flow control mode to 0x5 or 0x6<br>depending on the required runout space.<br>The p2_tx_max_blks value must be decreased by the amount of<br>increase in p2_rx_max_blks.<br>0x3 is the minimum value rx max blks and 0x6 is the maximum<br>value.                                       |

#### **14.5.6.37 P2\_BLK\_CNT Register (offset = 20Ch) [reset = 41h]**

P2\_BLK\_CNT is shown in Figure [14-157](#page-80-1) and described in Table [14-171](#page-80-2). CPSW PORT 2 FIFO BLOCK USAGE COUNT (READ ONLY)

**Figure 14-157. P2\_BLK\_CNT Register**

| 31 | 30            | 29 | 28       | 27       | 26            | 25 | 24                |
|----|---------------|----|----------|----------|---------------|----|-------------------|
|    |               |    |          | RESERVED |               |    |                   |
|    |               |    | R-0h     |          |               |    |                   |
| 23 | 22            | 21 | 20       | 19       | 18            | 17 | 16                |
|    |               |    |          | RESERVED |               |    |                   |
|    |               |    |          | R-0h     |               |    |                   |
| 15 | 14            | 13 | 12       | 11       | 10            | 9  | 8                 |
|    |               |    | RESERVED |          |               |    | P2_TX_BLK_C<br>NT |
|    |               |    | R-0h     |          |               |    | R-4h              |
| 7  | 6             | 5  | 4        | 3        | 2             | 1  | 0                 |
|    | P2_TX_BLK_CNT |    |          |          | P2_RX_BLK_CNT |    |                   |
|    | R-4h          |    |          |          | R-1h          |    |                   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-171. P2\_BLK\_CNT Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                              |
|------|---------------|------|-------|--------------------------------------------------------------------------------------------------------------------------|
| 31-9 | RESERVED      | R    | 0h    |                                                                                                                          |
| 8-4  | P2_TX_BLK_CNT | R    | 4h    | Port 2 Transmit Block Count Usage - This value is the number of<br>blocks allocated to the FIFO logical transmit queues. |
| 3-0  | P2_RX_BLK_CNT | R    | 1h    | Port 2 Receive Block Count Usage - This value is the number of<br>blocks allocated to the FIFO logical receive queues.   |

#### **14.5.6.38 P2\_TX\_IN\_CTL Register (offset = 210h) [reset = 80040C0h]**

P2\_TX\_IN\_CTL is shown in Figure [14-158](#page-81-1) and described in Table [14-172.](#page-81-2) CPSW PORT 2 TRANSMIT FIFO CONTROL

**Figure 14-158. P2\_TX\_IN\_CTL Register**

| 30 | 29 | 28                                                                | 27                     | 26                    | 25               | 24                      |
|----|----|-------------------------------------------------------------------|------------------------|-----------------------|------------------|-------------------------|
|    |    |                                                                   |                        |                       |                  |                         |
|    |    |                                                                   |                        |                       |                  |                         |
| 22 | 21 | 20                                                                | 19                     | 18                    | 17               | 16                      |
|    |    |                                                                   |                        |                       | TX_IN_SEL        |                         |
|    |    |                                                                   | R/W-<br>R/W-0h         |                       |                  |                         |
| 14 | 13 | 12                                                                | 11                     | 10                    | 9                | 8                       |
|    |    |                                                                   | RESERVED<br>TX_PRI_WDS |                       |                  |                         |
|    |    |                                                                   |                        |                       | R/W-C0h          |                         |
| 6  | 5  | 4                                                                 | 3                      | 2                     | 1                | 0                       |
|    |    |                                                                   |                        |                       |                  |                         |
|    |    |                                                                   |                        |                       |                  |                         |
|    |    | RESERVED<br>R/W-<br>TX_RATE_EN<br>R/W-0h<br>TX_BLKS_REM<br>R/W-4h |                        | TX_PRI_WDS<br>R/W-C0h | RESERVED<br>R/W- | HOST_BLKS_REM<br>R/W-8h |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-172. P2\_TX\_IN\_CTL Register Field Descriptions**

| Bit   | Field         | Type | Reset | Description                                                                                                                       |
|-------|---------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------|
| 31-28 | RESERVED      | R/W  |       |                                                                                                                                   |
| 27-24 | HOST_BLKS_REM | R/W  | 8h    | Transmit FIFO Blocks that must be free before a non rate-limited<br>CPDMA channel can begin sending a packet to the FIFO.         |
| 23-20 | TX_RATE_EN    | R/W  | 0h    | Transmit FIFO Input Rate Enable                                                                                                   |
| 19-18 | RESERVED      | R/W  |       |                                                                                                                                   |
| 17-16 | TX_IN_SEL     | R/W  | 0h    | Transmit FIFO Input Queue Type Select<br>00 - Normal priority mode<br>01 - reserved<br>10 - Rate Limit mode<br>11 - reserved      |
| 15-12 | TX_BLKS_REM   | R/W  | 4h    | Transmit FIFO Input Blocks to subtract in dual mac mode and blocks<br>to subtract on non rate-limited traffic in rate-limit mode. |
| 11-10 | RESERVED      | R/W  |       |                                                                                                                                   |
| 9-0   | TX_PRI_WDS    | R/W  | C0h   | Transmit FIFO Words in queue                                                                                                      |

#### **14.5.6.39 P2\_PORT\_VLAN Register (offset = 214h) [reset = 0h]**

P2\_PORT\_VLAN is shown in Figure [14-159](#page-82-1) and described in Table [14-173](#page-82-2). CPSW PORT 2 VLAN REGISTER

**Figure 14-159. P2\_PORT\_VLAN Register**

| 31 | 30       | 29 | 28       | 27 | 26       | 25 | 24 |  |  |
|----|----------|----|----------|----|----------|----|----|--|--|
|    | RESERVED |    |          |    |          |    |    |  |  |
|    | R/W-0h   |    |          |    |          |    |    |  |  |
| 23 | 22       | 21 | 20       | 19 | 18       | 17 | 16 |  |  |
|    |          |    | RESERVED |    |          |    |    |  |  |
|    | R/W-0h   |    |          |    |          |    |    |  |  |
| 15 | 14       | 13 | 12       | 11 | 10       | 9  | 8  |  |  |
|    | PORT_PRI |    | PORT_CFI |    | PORT_VID |    |    |  |  |
|    | R/W-0h   |    | R/W-0h   |    | R/W-0h   |    |    |  |  |
| 7  | 6        | 5  | 4        | 3  | 2        | 1  | 0  |  |  |
|    | PORT_VID |    |          |    |          |    |    |  |  |
|    | R/W-0h   |    |          |    |          |    |    |  |  |
|    |          |    |          |    |          |    |    |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-173. P2\_PORT\_VLAN Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                |
|-------|----------|------|-------|--------------------------------------------|
| 31-16 | RESERVED | R/W  | 0h    |                                            |
| 15-13 | PORT_PRI | R/W  | 0h    | Port VLAN Priority (7 is highest priority) |
| 12    | PORT_CFI | R/W  | 0h    | Port CFI bit                               |
| 11-0  | PORT_VID | R/W  | 0h    | Port VLAN ID                               |

#### **14.5.6.40 P2\_TX\_PRI\_MAP Register (offset = 218h) [reset = 33221001h]**

P2\_TX\_PRI\_MAP is shown in Figure [14-160](#page-83-1) and described in Table [14-174](#page-83-2). CPSW PORT 2 TX HEADER PRIORITY TO SWITCH PRI MAPPING REGISTER

### **Figure 14-160. P2\_TX\_PRI\_MAP Register**

| 31       | 30 | 29     | 28 | 27       | 26 | 25     | 24 | 23 | 22       | 21 | 20     | 19 | 18       | 17     | 16 |
|----------|----|--------|----|----------|----|--------|----|----|----------|----|--------|----|----------|--------|----|
| RESERVED |    | PRI7   |    | RESERVED |    | PRI6   |    |    | RESERVED |    | PRI5   |    | RESERVED | PRI4   |    |
| R/W-0h   |    | R/W-3h |    | R/W-0h   |    | R/W-3h |    |    | R/W-0h   |    | R/W-2h |    | R/W-0h   | R/W-2h |    |
| 15       | 14 | 13     | 12 | 11       | 10 | 9      | 8  | 7  | 6        | 5  | 4      | 3  | 2        | 1      | 0  |
| RESERVED |    | PRI3   |    | RESERVED |    | PRI2   |    |    | RESERVED |    | PRI1   |    | RESERVED | PRI0   |    |
| R/W-0h   |    | R/W-1h |    | R/W-0h   |    | R/W-0h |    |    | R/W-0h   |    | R/W-0h |    | R/W-0h   | R/W-1h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-174. P2\_TX\_PRI\_MAP Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                     |
|-------|----------|------|-------|---------------------------------------------------------------------------------|
| 31-30 | RESERVED | R/W  | 0h    |                                                                                 |
| 29-28 | PRI7     | R/W  | 3h    | Priority<br>7 - A packet header priority of 0x7 is given this switch queue pri. |
| 27-26 | RESERVED | R/W  | 0h    |                                                                                 |
| 25-24 | PRI6     | R/W  | 3h    | Priority<br>6 - A packet header priority of 0x6 is given this switch queue pri. |
| 23-22 | RESERVED | R/W  | 0h    |                                                                                 |
| 21-20 | PRI5     | R/W  | 2h    | Priority<br>5 - A packet header priority of 0x5 is given this switch queue pri. |
| 19-18 | RESERVED | R/W  | 0h    |                                                                                 |
| 17-16 | PRI4     | R/W  | 2h    | Priority<br>4 - A packet header priority of 0x4 is given this switch queue pri. |
| 15-14 | RESERVED | R/W  | 0h    |                                                                                 |
| 13-12 | PRI3     | R/W  | 1h    | Priority<br>3 - A packet header priority of 0x3 is given this switch queue pri. |
| 11-10 | RESERVED | R/W  | 0h    |                                                                                 |
| 9-8   | PRI2     | R/W  | 0h    | Priority<br>2 - A packet header priority of 0x2 is given this switch queue pri. |
| 7-6   | RESERVED | R/W  | 0h    |                                                                                 |
| 5-4   | PRI1     | R/W  | 0h    | Priority<br>1 - A packet header priority of 0x1 is given this switch queue pri. |
| 3-2   | RESERVED | R/W  | 0h    |                                                                                 |
| 1-0   | PRI0     | R/W  | 1h    | Priority<br>0 - A packet header priority of 0x0 is given this switch queue pri. |

#### **14.5.6.41 P2\_TS\_SEQ\_MTYPE Register (offset = 21Ch) [reset = 1E0000h]**

P2\_TS\_SEQ\_MTYPE is shown in Figure [14-161](#page-84-1) and described in Table [14-175](#page-84-2). CPSW\_3GF PORT 2 TIME SYNC SEQUENCE ID OFFSET AND MSG TYPE.

**Figure 14-161. P2\_TS\_SEQ\_MTYPE Register**

| 31 | 30                | 29 | 28     | 27                  | 26 | 25 | 24 |  |  |  |
|----|-------------------|----|--------|---------------------|----|----|----|--|--|--|
|    |                   |    |        | RESERVED            |    |    |    |  |  |  |
|    |                   |    | R/W-0h |                     |    |    |    |  |  |  |
| 23 | 22                | 21 | 20     | 19                  | 18 | 17 | 16 |  |  |  |
|    | RESERVED          |    |        | P2_TS_SEQ_ID_OFFSET |    |    |    |  |  |  |
|    | R/W-0h<br>R/W-1Eh |    |        |                     |    |    |    |  |  |  |
| 15 | 14                | 13 | 12     | 11                  | 10 | 9  | 8  |  |  |  |
|    |                   |    |        | P2_TS_MSG_TYPE_EN   |    |    |    |  |  |  |
|    |                   |    |        | R/W-0h              |    |    |    |  |  |  |
| 7  | 6                 | 5  | 4      | 3                   | 2  | 1  | 0  |  |  |  |
|    |                   |    |        | P2_TS_MSG_TYPE_EN   |    |    |    |  |  |  |
|    |                   |    |        | R/W-0h              |    |    |    |  |  |  |
|    |                   |    |        |                     |    |    |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-175. P2\_TS\_SEQ\_MTYPE Register Field Descriptions**

| Bit   | Field               | Type | Reset | Description                                                                                                                                                                                |
|-------|---------------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-22 | RESERVED            | R/W  | 0h    |                                                                                                                                                                                            |
| 21-16 | P2_TS_SEQ_ID_OFFSET | R/W  | 1Eh   | Port 2 Time Sync Sequence ID Offset This is the number of octets<br>that the sequence ID is offset in the tx and rx time sync message<br>header.<br>The minimum value is 6.                |
| 15-0  | P2_TS_MSG_TYPE_EN   | R/W  | 0h    | Port 2 Time Sync Message Type Enable - Each bit in this field<br>enables the corresponding message type in receive and transmit<br>time sync messages (Bit 0 enables message type 0 etc.). |

#### **14.5.6.42 P2\_SA\_LO Register (offset = 220h) [reset = 0h]**

P2\_SA\_LO is shown in Figure [14-162](#page-85-1) and described in Table [14-176](#page-85-2).

CPSW CPGMAC\_SL2 SOURCE ADDRESS LOW REGISTER

**Figure 14-162. P2\_SA\_LO Register**

| 31 | 30 | 29 | 28             | 27     | 26 | 25 | 24 | 23       | 22 | 21 | 20              | 19 | 18 | 17 | 16 |
|----|----|----|----------------|--------|----|----|----|----------|----|----|-----------------|----|----|----|----|
|    |    |    |                |        |    |    |    | RESERVED |    |    |                 |    |    |    |    |
|    |    |    |                |        |    |    |    | R/W-0h   |    |    |                 |    |    |    |    |
| 15 | 14 | 13 | 12             | 11     | 10 | 9  | 8  | 7        | 6  | 5  | 4               | 3  | 2  | 1  | 0  |
|    |    |    | MACSRCADDR_7_0 |        |    |    |    |          |    |    | MACSRCADDR_15_8 |    |    |    |    |
|    |    |    |                | R/W-0h |    |    |    |          |    |    | R/W-0h          |    |    |    |    |
|    |    |    |                |        |    |    |    |          |    |    |                 |    |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-176. P2\_SA\_LO Register Field Descriptions**

| Bit   | Field           | Type | Reset | Description                          |
|-------|-----------------|------|-------|--------------------------------------|
| 31-16 | RESERVED        | R/W  | 0h    |                                      |
| 15-8  | MACSRCADDR_7_0  | R/W  | 0h    | Source Address Lower 8 bits (byte 0) |
| 7-0   | MACSRCADDR_15_8 | R/W  | 0h    | Source Address bits<br>15:8 (byte 1) |

#### **14.5.6.43 P2\_SA\_HI Register (offset = 224h) [reset = 0h]**

P2\_SA\_HI is shown in Figure [14-163](#page-86-1) and described in Table [14-177.](#page-86-2)

CPSW CPGMAC\_SL2 SOURCE ADDRESS HIGH REGISTER

**Figure 14-163. P2\_SA\_HI Register**

| 31 | 30 | 29 | 28     | 27               | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19               | 18 | 17 | 16 |
|----|----|----|--------|------------------|----|----|----|----|----|----|----|------------------|----|----|----|
|    |    |    |        | MACSRCADDR_23_16 |    |    |    |    |    |    |    | MACSRCADDR_31_23 |    |    |    |
|    |    |    | R/W-0h |                  |    |    |    |    |    |    |    | R/W-0h           |    |    |    |
| 15 | 14 | 8  | 7      | 6                | 5  | 4  | 3  | 2  | 1  | 0  |    |                  |    |    |    |
|    |    |    |        | MACSRCADDR_39_32 |    |    |    |    |    |    |    | MACSRCADDR_47_40 |    |    |    |
|    |    |    | R/W-0h |                  |    |    |    |    |    |    |    | R/W-0h           |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-177. P2\_SA\_HI Register Field Descriptions**

| Bit   | Field            | Type | Reset | Description                           |
|-------|------------------|------|-------|---------------------------------------|
| 31-24 | MACSRCADDR_23_16 | R/W  | 0h    | Source Address bits<br>23:16 (byte 2) |
| 23-16 | MACSRCADDR_31_23 | R/W  | 0h    | Source Address bits<br>31:23 (byte 3) |
| 15-8  | MACSRCADDR_39_32 | R/W  | 0h    | Source Address bits<br>39:32 (byte 4) |
| 7-0   | MACSRCADDR_47_40 | R/W  | 0h    | Source Address bits<br>47:40 (byte 5) |

#### **14.5.6.44 P2\_SEND\_PERCENT Register (offset = 228h) [reset = 0h]**

P2\_SEND\_PERCENT is shown in Figure [14-164](#page-87-1) and described in Table [14-178](#page-87-2). CPSW PORT 2 TRANSMIT QUEUE SEND PERCENTAGES

**Figure 14-164. P2\_SEND\_PERCENT Register**

| 31           | 30     | 29 | 28 | 27                | 26 | 25 | 24 | 23           | 22 | 21 | 20 | 19                | 18 | 17 | 16 |
|--------------|--------|----|----|-------------------|----|----|----|--------------|----|----|----|-------------------|----|----|----|
|              |        |    |    | RESERVED          |    |    |    |              |    |    |    | PRI3_SEND_PERCENT |    |    |    |
|              |        |    |    | R/W-0h            |    |    |    |              |    |    |    | R/W-0h            |    |    |    |
| 15           | 14     | 13 | 12 | 11                | 10 | 9  | 8  | 7            | 6  | 5  | 4  | 3                 | 2  | 1  | 0  |
| RESE<br>RVED |        |    |    | PRI2_SEND_PERCENT |    |    |    | RESE<br>RVED |    |    |    | PRI1_SEND_PERCENT |    |    |    |
| R/W<br>0h    | R/W-0h |    |    |                   |    |    |    | R/W<br>0h    |    |    |    | R/W-0h            |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-178. P2\_SEND\_PERCENT Register Field Descriptions**

| Bit   | Field             | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                |
|-------|-------------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-23 | RESERVED          | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                            |
| 22-16 | PRI3_SEND_PERCENT | R/W  | 0h    | Priority 3 Transmit Percentage - This percentage value is sent from<br>FIFO priority 3 (maximum) when the p1_pri3_shape_en is set<br>(queue shaping enabled).<br>This is the percentage of the wire that packets from priority 3 receive<br>(which includes interpacket gap and preamble bytes).<br>If shaping is enabled on this queue then this value must be between<br>zero and 0d100 (not inclusive). |
| 15    | RESERVED          | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                            |
| 14-8  | PRI2_SEND_PERCENT | R/W  | 0h    | Priority 2 Transmit Percentage - This percentage value is sent from<br>FIFO priority 2 (maximum) when the p1_pri2_shape_en is set<br>(queue shaping enabled).<br>This is the percentage of the wire that packets from priority 2 receive<br>(which includes interpacket gap and preamble bytes).<br>If shaping is enabled on this queue then this value must be between<br>zero and 0d100 (not inclusive). |
| 7     | RESERVED          | R/W  | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                            |
| 6-0   | PRI1_SEND_PERCENT | R/W  | 0h    | Priority 1 Transmit Percentage - This percentage value is sent from<br>FIFO priority 1 (maximum) when the p1_pri1_shape_en is set<br>(queue shaping enabled).<br>This is the percentage of the wire that packets from priority 1 receive<br>(which includes interpacket gap and preamble bytes).<br>If shaping is enabled on this queue then this value must be between<br>zero and 0d100 (not inclusive). |

#### **14.5.6.45 P2\_RX\_DSCP\_PRI\_MAP0 Register (offset = 230h) [reset = 0h]**

P2\_RX\_DSCP\_PRI\_MAP0 is shown in Figure [14-165](#page-88-1) and described in Table [14-179](#page-88-2). CPSW PORT 2 RX DSCP PRIORITY TO RX PACKET MAPPING REG 0

**Figure 14-165. P2\_RX\_DSCP\_PRI\_MAP0 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI7   |    | RESE<br>RVED |    | PRI6   |    | RESE<br>RVED |    | PRI5   |    | RESE<br>RVED |    | PRI4   |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI3   |    | RESE<br>RVED |    | PRI2   |    | RESE<br>RVED |    | PRI1   |    | RESE<br>RVED |    | PRI0   |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-179. P2\_RX\_DSCP\_PRI\_MAP0 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                     |
|-------|----------|------|-------|---------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                 |
| 30-28 | PRI7     | R/W  | 0h    | Priority<br>7 - A packet TOS of 0d7 is mapped to this received packet priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                 |
| 26-24 | PRI6     | R/W  | 0h    | Priority<br>6 - A packet TOS of 0d6 is mapped to this received packet priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                 |
| 22-20 | PRI5     | R/W  | 0h    | Priority<br>5 - A packet TOS of 0d5 is mapped to this received packet priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                 |
| 18-16 | PRI4     | R/W  | 0h    | Priority<br>4 - A packet TOS of 0d4 is mapped to this received packet priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                 |
| 14-12 | PRI3     | R/W  | 0h    | Priority<br>3 - A packet TOS of 0d3 is mapped to this received packet priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                 |
| 10-8  | PRI2     | R/W  | 0h    | Priority<br>2 - A packet TOS of 0d2 is mapped to this received packet priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                 |
| 6-4   | PRI1     | R/W  | 0h    | Priority<br>1 - A packet TOS of 0d1 is mapped to this received packet priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                 |
| 2-0   | PRI0     | R/W  | 0h    | Priority<br>0 - A packet TOS of 0d0 is mapped to this received packet priority. |

#### **14.5.6.46 P2\_RX\_DSCP\_PRI\_MAP1 Register (offset = 234h) [reset = 0h]**

P2\_RX\_DSCP\_PRI\_MAP1 is shown in Figure [14-166](#page-89-1) and described in Table [14-180](#page-89-2). CPSW PORT 2 RX DSCP PRIORITY TO RX PACKET MAPPING REG 1

**Figure 14-166. P2\_RX\_DSCP\_PRI\_MAP1 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI15  |    | RESE<br>RVED |    | PRI14  |    | RESE<br>RVED |    | PRI13  |    | RESE<br>RVED |    | PRI12  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI11  |    | RESE<br>RVED |    | PRI10  |    | RESE<br>RVED |    | PRI9   |    | RESE<br>RVED |    | PRI8   |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-180. P2\_RX\_DSCP\_PRI\_MAP1 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI15    | R/W  | 0h    | Priority<br>15 - A packet TOS of 0d15 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI14    | R/W  | 0h    | Priority<br>14 - A packet TOS of 0d14 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI13    | R/W  | 0h    | Priority<br>13 - A packet TOS of 0d13 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI12    | R/W  | 0h    | Priority<br>12 - A packet TOS of 0d12 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI11    | R/W  | 0h    | Priority<br>11 - A packet TOS of 0d11 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI10    | R/W  | 0h    | Priority<br>10 - A packet TOS of 0d10 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI9     | R/W  | 0h    | Priority<br>9 - A packet TOS of 0d9 is mapped to this received packet priority.      |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI8     | R/W  | 0h    | Priority<br>8 - A packet TOS of 0d8 is mapped to this received packet priority.      |

#### **14.5.6.47 P2\_RX\_DSCP\_PRI\_MAP2 Register (offset = 238h) [reset = 0h]**

P2\_RX\_DSCP\_PRI\_MAP2 is shown in Figure [14-167](#page-90-1) and described in Table [14-181](#page-90-2). CPSW PORT 2 RX DSCP PRIORITY TO RX PACKET MAPPING REG 2

**Figure 14-167. P2\_RX\_DSCP\_PRI\_MAP2 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI23  |    | RESE<br>RVED |    | PRI22  |    | RESE<br>RVED |    | PRI21  |    | RESE<br>RVED |    | PRI20  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI19  |    | RESE<br>RVED |    | PRI18  |    | RESE<br>RVED |    | PRI17  |    | RESE<br>RVED |    | PRI16  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-181. P2\_RX\_DSCP\_PRI\_MAP2 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI23    | R/W  | 0h    | Priority<br>23 - A packet TOS of 0d23 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI22    | R/W  | 0h    | Priority<br>22 - A packet TOS of 0d22 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI21    | R/W  | 0h    | Priority<br>21 - A packet TOS of 0d21 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI20    | R/W  | 0h    | Priority<br>20 - A packet TOS of 0d20 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI19    | R/W  | 0h    | Priority<br>19 - A packet TOS of 0d19 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI18    | R/W  | 0h    | Priority<br>18 - A packet TOS of 0d18 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI17    | R/W  | 0h    | Priority<br>17 - A packet TOS of 0d17 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI16    | R/W  | 0h    | Priority<br>16 - A packet TOS of 0d16 is mapped to this received packet<br>priority. |

#### **14.5.6.48 P2\_RX\_DSCP\_PRI\_MAP3 Register (offset = 23Ch) [reset = 0h]**

P2\_RX\_DSCP\_PRI\_MAP3 is shown in Figure [14-168](#page-91-1) and described in Table [14-182](#page-91-2). CPSW PORT 2 RX DSCP PRIORITY TO RX PACKET MAPPING REG 3

**Figure 14-168. P2\_RX\_DSCP\_PRI\_MAP3 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI31  |    | RESE<br>RVED |    | PRI30  |    | RESE<br>RVED |    | PRI29  |    | RESE<br>RVED |    | PRI28  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI27  |    | RESE<br>RVED |    | PRI26  |    | RESE<br>RVED |    | PRI25  |    | RESE<br>RVED |    | PRI24  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-182. P2\_RX\_DSCP\_PRI\_MAP3 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI31    | R/W  | 0h    | Priority<br>31 - A packet TOS of 0d31 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI30    | R/W  | 0h    | Priority<br>30 - A packet TOS of 0d30 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI29    | R/W  | 0h    | Priority<br>29 - A packet TOS of 0d39 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI28    | R/W  | 0h    | Priority<br>28 - A packet TOS of 0d28 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI27    | R/W  | 0h    | Priority<br>27 - A packet TOS of 0d27 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI26    | R/W  | 0h    | Priority<br>26 - A packet TOS of 0d26 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI25    | R/W  | 0h    | Priority<br>25 - A packet TOS of 0d25 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI24    | R/W  | 0h    | Priority<br>24 - A packet TOS of 0d24 is mapped to this received packet<br>priority. |

#### **14.5.6.49 P2\_RX\_DSCP\_PRI\_MAP4 Register (offset = 240h) [reset = 0h]**

P2\_RX\_DSCP\_PRI\_MAP4 is shown in Figure [14-169](#page-92-1) and described in Table [14-183](#page-92-2). CPSW PORT 2 RX DSCP PRIORITY TO RX PACKET MAPPING REG 4

**Figure 14-169. P2\_RX\_DSCP\_PRI\_MAP4 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI39  |    | RESE<br>RVED |    | PRI38  |    | RESE<br>RVED |    | PRI37  |    | RESE<br>RVED |    | PRI36  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI35  |    | RESE<br>RVED |    | PRI34  |    | RESE<br>RVED |    | PRI33  |    | RESE<br>RVED |    | PRI32  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-183. P2\_RX\_DSCP\_PRI\_MAP4 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI39    | R/W  | 0h    | Priority<br>39 - A packet TOS of 0d39 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI38    | R/W  | 0h    | Priority<br>38 - A packet TOS of 0d38 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI37    | R/W  | 0h    | Priority<br>37 - A packet TOS of 0d37 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI36    | R/W  | 0h    | Priority<br>36 - A packet TOS of 0d36 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI35    | R/W  | 0h    | Priority<br>35 - A packet TOS of 0d35 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI34    | R/W  | 0h    | Priority<br>34 - A packet TOS of 0d34 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI33    | R/W  | 0h    | Priority<br>33 - A packet TOS of 0d33 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI32    | R/W  | 0h    | Priority<br>32 - A packet TOS of 0d32 is mapped to this received packet<br>priority. |

#### **14.5.6.50 P2\_RX\_DSCP\_PRI\_MAP5 Register (offset = 244h) [reset = 0h]**

P2\_RX\_DSCP\_PRI\_MAP5 is shown in Figure [14-170](#page-93-1) and described in Table [14-184](#page-93-2). CPSW PORT 2 RX DSCP PRIORITY TO RX PACKET MAPPING REG 5

**Figure 14-170. P2\_RX\_DSCP\_PRI\_MAP5 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI47  |    | RESE<br>RVED |    | PRI46  |    | RESE<br>RVED |    | PRI45  |    | RESE<br>RVED |    | PRI44  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI43  |    | RESE<br>RVED |    | PRI42  |    | RESE<br>RVED |    | PRI41  |    | RESE<br>RVED |    | PRI40  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-184. P2\_RX\_DSCP\_PRI\_MAP5 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI47    | R/W  | 0h    | Priority<br>47 - A packet TOS of 0d47 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI46    | R/W  | 0h    | Priority<br>46 - A packet TOS of 0d46 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI45    | R/W  | 0h    | Priority<br>45 - A packet TOS of 0d45 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI44    | R/W  | 0h    | Priority<br>44 - A packet TOS of 0d44 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI43    | R/W  | 0h    | Priority<br>43 - A packet TOS of 0d43 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI42    | R/W  | 0h    | Priority<br>42 - A packet TOS of 0d42 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI41    | R/W  | 0h    | Priority<br>41 - A packet TOS of 0d41 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI40    | R/W  | 0h    | Priority<br>40 - A packet TOS of 0d40 is mapped to this received packet<br>priority. |

#### **14.5.6.51 P2\_RX\_DSCP\_PRI\_MAP6 Register (offset = 248h) [reset = 0h]**

P2\_RX\_DSCP\_PRI\_MAP6 is shown in Figure [14-171](#page-94-1) and described in Table [14-185](#page-94-2). CPSW PORT 2 RX DSCP PRIORITY TO RX PACKET MAPPING REG 6

**Figure 14-171. P2\_RX\_DSCP\_PRI\_MAP6 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI55  |    | RESE<br>RVED |    | PRI54  |    | RESE<br>RVED |    | PRI53  |    | RESE<br>RVED |    | PRI52  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI51  |    | RESE<br>RVED |    | PRI50  |    | RESE<br>RVED |    | PRI49  |    | RESE<br>RVED |    | PRI48  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-185. P2\_RX\_DSCP\_PRI\_MAP6 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI55    | R/W  | 0h    | Priority<br>55 - A packet TOS of 0d55 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI54    | R/W  | 0h    | Priority<br>54 - A packet TOS of 0d54 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI53    | R/W  | 0h    | Priority<br>53 - A packet TOS of 0d53 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI52    | R/W  | 0h    | Priority<br>52 - A packet TOS of 0d52 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI51    | R/W  | 0h    | Priority<br>51 - A packet TOS of 0d51 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI50    | R/W  | 0h    | Priority<br>50 - A packet TOS of 0d50 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI49    | R/W  | 0h    | Priority<br>49 - A packet TOS of 0d49 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI48    | R/W  | 0h    | Priority<br>48 - A packet TOS of 0d48 is mapped to this received packet<br>priority. |

#### **14.5.6.52 P2\_RX\_DSCP\_PRI\_MAP7 Register (offset = 24Ch) [reset = 0h]**

P2\_RX\_DSCP\_PRI\_MAP7 is shown in Figure [14-172](#page-95-1) and described in Table [14-186](#page-95-2). CPSW PORT 2 RX DSCP PRIORITY TO RX PACKET MAPPING REG 7

**Figure 14-172. P2\_RX\_DSCP\_PRI\_MAP7 Register**

| 31           | 30 | 29     | 28 | 27           | 26 | 25     | 24 | 23           | 22 | 21     | 20 | 19           | 18 | 17     | 16 |
|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|--------------|----|--------|----|
| RESE<br>RVED |    | PRI63  |    | RESE<br>RVED |    | PRI62  |    | RESE<br>RVED |    | PRI61  |    | RESE<br>RVED |    | PRI60  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |
| 15           | 14 | 13     | 12 | 11           | 10 | 9      | 8  | 7            | 6  | 5      | 4  | 3            | 2  | 1      | 0  |
| RESE<br>RVED |    | PRI59  |    | RESE<br>RVED |    | PRI58  |    | RESE<br>RVED |    | PRI57  |    | RESE<br>RVED |    | PRI56  |    |
| R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    | R/W<br>0h    |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-186. P2\_RX\_DSCP\_PRI\_MAP7 Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description                                                                          |
|-------|----------|------|-------|--------------------------------------------------------------------------------------|
| 31    | RESERVED | R/W  | 0h    |                                                                                      |
| 30-28 | PRI63    | R/W  | 0h    | Priority<br>63 - A packet TOS of 0d63 is mapped to this received packet<br>priority. |
| 27    | RESERVED | R/W  | 0h    |                                                                                      |
| 26-24 | PRI62    | R/W  | 0h    | Priority<br>62 - A packet TOS of 0d62 is mapped to this received packet<br>priority. |
| 23    | RESERVED | R/W  | 0h    |                                                                                      |
| 22-20 | PRI61    | R/W  | 0h    | Priority<br>61 - A packet TOS of 0d61 is mapped to this received packet<br>priority. |
| 19    | RESERVED | R/W  | 0h    |                                                                                      |
| 18-16 | PRI60    | R/W  | 0h    | Priority<br>60 - A packet TOS of 0d60 is mapped to this received packet<br>priority. |
| 15    | RESERVED | R/W  | 0h    |                                                                                      |
| 14-12 | PRI59    | R/W  | 0h    | Priority<br>59 - A packet TOS of 0d59 is mapped to this received packet<br>priority. |
| 11    | RESERVED | R/W  | 0h    |                                                                                      |
| 10-8  | PRI58    | R/W  | 0h    | Priority<br>58 - A packet TOS of 0d58 is mapped to this received packet<br>priority. |
| 7     | RESERVED | R/W  | 0h    |                                                                                      |
| 6-4   | PRI57    | R/W  | 0h    | Priority<br>57 - A packet TOS of 0d57 is mapped to this received packet<br>priority. |
| 3     | RESERVED | R/W  | 0h    |                                                                                      |
| 2-0   | PRI56    | R/W  | 0h    | Priority<br>56 - A packet TOS of 0d56 is mapped to this received packet<br>priority. |

### *14.5.7 CPSW\_SL Registers*

Table [14-187](#page-96-0) lists the memory-mapped registers for the CPSW\_SL. All register offset addresses not listed in Table [14-187](#page-96-0) should be considered as reserved locations and the register contents should not be modified.

### **Table 14-187. CPSW\_SL Registers**

| Offset | Acronym    | Register Name | Section           |
|--------|------------|---------------|-------------------|
| 0h     | IDVER      |               | Section 14.5.7.1  |
| 4h     | MACCONTROL |               | Section 14.5.7.2  |
| 8h     | MACSTATUS  |               | Section 14.5.7.3  |
| Ch     | SOFT_RESET |               | Section 14.5.7.4  |
| 10h    | RX_MAXLEN  |               | Section 14.5.7.5  |
| 14h    | BOFFTEST   |               | Section 14.5.7.6  |
| 18h    | RX_PAUSE   |               | Section 14.5.7.7  |
| 1Ch    | TX_PAUSE   |               | Section 14.5.7.8  |
| 20h    | EMCONTROL  |               | Section 14.5.7.9  |
| 24h    | RX_PRI_MAP |               | Section 14.5.7.10 |
| 28h    | TX_GAP     |               | Section 14.5.7.11 |

#### **14.5.7.1 IDVER Register (offset = 0h) [reset = 170112h]**

Register mask: FFFFFFFFh

IDVER is shown in Figure [14-173](#page-97-1) and described in Table [14-188.](#page-97-2)

CPGMAC\_SL ID/VERSION REGISTER

**Figure 14-173. IDVER Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23    | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13   | 12 | 11 | 10 | 9    | 8 | 7 | 6 | 5 | 4 | 3     | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|-------|----|----|----|----|----|----|----|----|----|------|----|----|----|------|---|---|---|---|---|-------|---|---|---|
|    |    |    |    |    |    |    |    | IDENT |    |    |    |    |    |    |    |    |    | Z    |    |    |    | X    |   |   |   |   |   | Y     |   |   |   |
|    |    |    |    |    |    |    |    | R-17h |    |    |    |    |    |    |    |    |    | R-0h |    |    |    | R-1h |   |   |   |   |   | R-12h |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-188. IDVER Register Field Descriptions**

| Bit   | Field | Type | Reset | Description             |
|-------|-------|------|-------|-------------------------|
| 31-16 | IDENT | R    | 17h   | Rx Identification Value |
| 15-11 | Z     | R    | 0h    | Rx Z value (X.Y.Z)      |
| 10-8  | X     | R    | 1h    | Rx X value (major)      |
| 7-0   | Y     | R    | 12h   | Rx Y value (minor)      |

#### **14.5.7.2 MACCONTROL Register (offset = 4h) [reset = 0h]**

Register mask: FFFFFFFFh

MACCONTROL is shown in Figure [14-174](#page-98-1) and described in Table [14-189](#page-98-2).

CPGMAC\_SL MAC CONTROL REGISTER

**Figure 14-174. MACCONTROL Register**

| 31        | 30        | 29                      | 28            | 27            | 26                  | 25        | 24            |
|-----------|-----------|-------------------------|---------------|---------------|---------------------|-----------|---------------|
|           |           |                         | RESERVED      |               |                     |           | RX_CMF_EN     |
|           |           |                         | Rreturns0s-0h |               |                     |           | R/W-0h        |
| 23        | 22        | 21                      | 20            | 19            | 18                  | 17        | 16            |
| RX_CSF_EN | RX_CEF_EN | TX_SHORT_G<br>AP_LIM_EN |               | RESERVED      | EXT_EN              | GIG_FORCE | IFCTL_B       |
| R/W-0h    | R/W-0h    | R/W-0h                  |               | Rreturns0s-0h | R/W-0h              | R/W-0h    | R/W-0h        |
| 15        | 14        | 13                      | 12            | 11            | 10                  | 9         | 8             |
| IFCTL_A   |           | RESERVED                |               | CMD_IDLE      | TX_SHORT_G<br>AP_EN |           | RESERVED      |
| R/W-0h    |           | Rreturns0s-0h           |               | R/W-0h        | R/W-0h              |           | Rreturns0s-0h |
| 7         | 6         | 5                       | 4             | 3             | 2                   | 1         | 0             |
| GIG       | TX_PACE   | GMII_EN                 | TX_FLOW_EN    | RX_FLOW_EN    | MTEST               | LOOPBACK  | FULLDUPLEX    |
| R/W-0h    | R/W-0h    | R/W-0h                  | R/W-0h        | R/W-0h        | R/W-0h              | R/W-0h    | R/W-0h        |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-189. MACCONTROL Register Field Descriptions**

| Bit   | Field                   | Type       | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|-------|-------------------------|------------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-25 | RESERVED                | Rreturns0s | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 24    | RX_CMF_EN               | R/W        | 0h    | RX Copy MAC Control Frames Enable - Enables MAC control<br>frames to be transferred to memory.<br>MAC control frames are normally acted upon (if enabled), but not<br>copied to memory.<br>MAC control frames that are pause frames will be acted upon if<br>enabled in the MacControl register, regardless of the value of<br>rx_cmf_en.<br>Frames transferred to memory due to rx_cmf_en will have the<br>control bit set in their EOP buffer descriptor.<br>0 - MAC control frames are filtered (but acted upon if enabled).<br>1 - MAC control frames are transferred to memory. |
| 23    | RX_CSF_EN               | R/W        | 0h    | RX Copy Short Frames Enable - Enables frames or fragments<br>shorter than 64 bytes to be copied to memory.<br>Frames transferred to memory due to rx_csf_en will have the<br>fragment or undersized bit set in their receive footer.<br>Fragments are short frames that contain CRC/align/code errors and<br>undersized are short frames without errors.<br>0 - Short frames are filtered<br>1 - Short frames are transferred to memory.                                                                                                                                             |
| 22    | RX_CEF_EN               | R/W        | 0h    | RX Copy Error Frames Enable - Enables frames containing errors to<br>be transferred to memory.<br>The appropriate error bit will be set in the frame receive footer.<br>Frames containing errors will be filtered when rx_cef _en is not set.<br>0 - Frames containing errors are filtered.<br>1 - Frames containing errors are transferred to memory.                                                                                                                                                                                                                               |
| 21    | TX_SHORT_GAP_LIM_E<br>N | R/W        | 0h    | Transmit Short Gap Limit Enable When set this bit limits the number<br>of short gap packets transmitted to 100ppm.<br>Each time a short gap packet is sent, a counter is loaded with<br>10,000 and decremented on each wireside clock.<br>Another short gap packet will not be sent out until the counter<br>decrements to zero.<br>This mode is included to preclude the host from filling up the FIFO<br>and sending every packet out with short gap which would violate the<br>maximum number of packets per second allowed.                                                      |

**Table 14-189. MACCONTROL Register Field Descriptions (continued)**

| Bit         | Field              | Type              | Reset    | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
|-------------|--------------------|-------------------|----------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 20-19<br>18 | RESERVED<br>EXT_EN | Rreturns0s<br>R/W | 0h<br>0h | Mode of operation.<br>0 - Forced mode of operation.<br>1 - In-band mode of operation.                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 17          | GIG_FORCE          | R/W               | 0h       | Gigabit Mode Force - This bit is used to force the CPGMAC_SL into<br>gigabit mode if the input GMII_MTCLK has been stopped by the<br>PHY.                                                                                                                                                                                                                                                                                                                                                                                     |
| 16          | IFCTL_B            | R/W               | 0h       | Connects to the speed_in input of the respective RMII gasket.<br>When using RMII mode:<br>0 - 10Mbps operation<br>1 - 100Mbps operation                                                                                                                                                                                                                                                                                                                                                                                       |
| 15          | IFCTL_A            | R/W               | 0h       | Connects to the speed_in input of the respective RMII gasket.<br>When using RMII mode:<br>0 - 10Mbps operation<br>1 - 100Mbps operation                                                                                                                                                                                                                                                                                                                                                                                       |
| 14-12       | RESERVED           | Rreturns0s        | 0h       |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| 11          | CMD_IDLE           | R/W               | 0h       | Command Idle<br>0 - Idle not commanded<br>1 - Idle Commanded (read idle in MacStatus)                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 10          | TX_SHORT_GAP_EN    | R/W               | 0h       | Transmit Short Gap Enable<br>0 - Transmit with a short IPG is disabled<br>1 - Transmit with a short IPG (when TX_SHORT_GAP input is<br>asserted) is enabled.                                                                                                                                                                                                                                                                                                                                                                  |
| 9-8         | RESERVED           | Rreturns0s        | 0h       |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| 7           | GIG                | R/W               | 0h       | Gigabit Mode -<br>0 - 10/100 mode<br>1 - Gigabit mode (full duplex only) The GIG_OUT output is the value<br>of this bit.                                                                                                                                                                                                                                                                                                                                                                                                      |
| 6           | TX_PACE            | R/W               | 0h       | Transmit Pacing Enable<br>0 - Transmit Pacing Disabled<br>1 - Transmit Pacing Enabled                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 5           | GMII_EN            | R/W               | 0h       | GMII Enable Bit. This bit must be set before the MAC will transmit or<br>receive data in any of the supported interface modes (such as MII<br>and RMII). This bit does not select the interface mode, but rather<br>holds or releases the MAC TX and RX state machines from reset.<br>0 - The MAC RX and TX state machines are held in reset<br>1 - The MAC RX and TX state machines are released from reset and<br>transmit/receive are enabled.                                                                             |
| 4           | TX_FLOW_EN         | R/W               | 0h       | Transmit Flow Control Enable - Determines if incoming pause<br>frames are acted upon in full-duplex mode.<br>Incoming pause frames are not acted upon in half-duplex mode<br>regardless of this bit setting.<br>The RX_MBP_Enable bits determine whether or not received pause<br>frames are transferred to memory.<br>0 - Transmit Flow Control Disabled.<br>Full-duplex mode - Incoming pause frames are not acted upon.<br>1 - Transmit Flow Control Enabled .<br>Full-duplex mode - Incoming pause frames are acted upon. |
| 3           | RX_FLOW_EN         | R/W               | 0h       | Receive Flow Control Enable -<br>0 - Receive Flow Control Disabled Half-duplex mode - No flow<br>control generated collisions are sent.<br>Full-duplex mode - No outgoing pause frames are sent.<br>1 - Receive Flow Control Enabled Half-duplex mode - Collisions are<br>initiated when receive flow control is triggered.<br>Full-duplex mode - Outgoing pause frames are sent when receive<br>flow control is triggered.                                                                                                   |
| 2           | MTEST              | R/W               | 0h       | Manufacturing Test mode - This bit must be set to allow writes to the<br>Backoff_Test and PauseTimer registers.                                                                                                                                                                                                                                                                                                                                                                                                               |

**Table 14-189. MACCONTROL Register Field Descriptions (continued)**

| Bit | Field      | Type | Reset | Description                                                                                                                                                                                                                                              |
|-----|------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1   | LOOPBACK   | R/W  | 0h    | Loop Back Mode - Loopback mode forces internal fullduplex mode<br>regardless of whether the fullduplex bit is set or not.<br>The loopback bit should be changed only when GMII_en is<br>deasserted.<br>0 - Not looped back<br>1 - Loop Back Mode enabled |
| 0   | FULLDUPLEX | R/W  | 0h    | Full Duplex mode - Gigabit mode forces fullduplex mode regardless<br>of whether the fullduplex bit is set or not.<br>The FULLDUPLEX_OUT output is the value of this register bit<br>0 - half duplex mode<br>1 - full duplex mode                         |

#### **14.5.7.3 MACSTATUS Register (offset = 8h) [reset = 0h]**

Register mask: FFFFFFFFh

MACSTATUS is shown in Figure [14-175](#page-101-1) and described in Table [14-190.](#page-101-2)

CPGMAC\_SL MAC STATUS REGISTER

**Figure 14-175. MACSTATUS Register**

| 31   | 30            | 29 | 28      | 27                 | 26            | 25              | 24              |
|------|---------------|----|---------|--------------------|---------------|-----------------|-----------------|
| IDLE |               |    |         | RESERVED           |               |                 |                 |
| R-0h |               |    |         | Rreturns0s-0h      |               |                 |                 |
| 23   | 22            | 21 | 20      | 19                 | 18            | 17              | 16              |
|      |               |    |         | RESERVED           |               |                 |                 |
|      |               |    |         | Rreturns0s-0h      |               |                 |                 |
| 15   | 14            | 13 | 12      | 11                 | 10            | 9               | 8               |
|      |               |    |         | RESERVED           |               |                 |                 |
|      |               |    |         | Rreturns0s-0h      |               |                 |                 |
| 7    | 6             | 5  | 4       | 3                  | 2             | 1               | 0               |
|      | RESERVED      |    | EXT_GIG | EXT_FULLDUP<br>LEX | RESERVED      | RX_FLOW_AC<br>T | TX_FLOW_AC<br>T |
|      | Rreturns0s-0h |    | R-0h    | R-0h               | Rreturns0s-0h | R-0h            | R-0h            |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-190. MACSTATUS Register Field Descriptions**

| Bit  | Field          | Type       | Reset | Description                                                                                                                                                                                                                                                                                                                         |
|------|----------------|------------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31   | IDLE           | R          | 0h    | CPGMAC_SL IDLE - The CPGMAC_SL is in the idle state (valid<br>after an idle command)<br>0 - The CPGMAC_SL is not in the idle state.<br>1 - The CPGMAC_SL is in the idle state.                                                                                                                                                      |
| 30-5 | RESERVED       | Rreturns0s | 0h    |                                                                                                                                                                                                                                                                                                                                     |
| 4    | EXT_GIG        | R          | 0h    | External GIG - This is the value of the EXT_GIG input bit.                                                                                                                                                                                                                                                                          |
| 3    | EXT_FULLDUPLEX | R          | 0h    | External Fullduplex - This is the value of the EXT_FULLDUPLEX<br>input bit.                                                                                                                                                                                                                                                         |
| 2    | RESERVED       | Rreturns0s | 0h    |                                                                                                                                                                                                                                                                                                                                     |
| 1    | RX_FLOW_ACT    | R          | 0h    | Receive Flow Control Active - When asserted, indicates that receive<br>flow control is enabled and triggered.                                                                                                                                                                                                                       |
| 0    | TX_FLOW_ACT    | R          | 0h    | Transmit Flow Control Active - When asserted, this bit indicates that<br>the pause time period is being observed for a received pause frame.<br>No new transmissions will begin while this bit is asserted except for<br>the transmission of pause frames.<br>Any transmission in progress when this bit is asserted will complete. |

#### **14.5.7.4 SOFT\_RESET Register (offset = Ch) [reset = 0h]**

Register mask: FFFFFFFFh

SOFT\_RESET is shown in Figure [14-176](#page-102-1) and described in Table [14-191.](#page-102-2)

CPGMAC\_SL SOFT RESET REGISTER

**Figure 14-176. SOFT\_RESET Register**

| 31 | 30            | 29 | 28            | 27 | 26 | 25 | 24         |  |  |  |  |  |  |
|----|---------------|----|---------------|----|----|----|------------|--|--|--|--|--|--|
|    |               |    | RESERVED      |    |    |    |            |  |  |  |  |  |  |
|    |               |    | Rreturns0s-0h |    |    |    |            |  |  |  |  |  |  |
| 23 | 22            | 21 | 20            | 19 | 18 | 17 | 16         |  |  |  |  |  |  |
|    |               |    | RESERVED      |    |    |    |            |  |  |  |  |  |  |
|    | Rreturns0s-0h |    |               |    |    |    |            |  |  |  |  |  |  |
| 15 | 14            | 13 | 12            | 11 | 10 | 9  | 8          |  |  |  |  |  |  |
|    |               |    | RESERVED      |    |    |    |            |  |  |  |  |  |  |
|    |               |    | Rreturns0s-0h |    |    |    |            |  |  |  |  |  |  |
| 7  | 6             | 5  | 4             | 3  | 2  | 1  | 0          |  |  |  |  |  |  |
|    |               |    | RESERVED      |    |    |    | SOFT_RESET |  |  |  |  |  |  |
|    |               |    | Rreturns0s-0h |    |    |    | R/W-0h     |  |  |  |  |  |  |
|    |               |    |               |    |    |    |            |  |  |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-191. SOFT\_RESET Register Field Descriptions**

| Bit  | Field      | Type       | Reset | Description                                                                                                                                                                                                                                                                              |
|------|------------|------------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | RESERVED   | Rreturns0s | 0h    |                                                                                                                                                                                                                                                                                          |
| 0    | SOFT_RESET | R/W        | 0h    | Software reset - Writing a one to this bit causes the CPGMAC_SL<br>logic to be reset.<br>After writing a one to this bit, it may be polled to determine if the<br>reset has occurred.<br>If a one is read, the reset has not yet occurred.<br>If a zero is read then reset has occurred. |

#### **14.5.7.5 RX\_MAXLEN Register (offset = 10h) [reset = 5EEh]**

Register mask: FFFFFFFFh

RX\_MAXLEN is shown in Figure [14-177](#page-103-1) and described in Table [14-192.](#page-103-2)

CPGMAC\_SL RX MAXIMUM LENGTH REGISTER

### **Figure 14-177. RX\_MAXLEN Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23            | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6         | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|---------------|----|----|----|----|----|----|----|----|----|----|----|----|----|---|---|---|-----------|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    | RESERVED      |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   |   | RX_MAXLEN |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    | Rreturns0s-0h |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   |   | R/W-5EEh  |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-192. RX\_MAXLEN Register Field Descriptions**

| Bit   | Field     | Type       | Reset | Description                                                                                                                                                                                                                                                                                                                                              |
|-------|-----------|------------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-14 | RESERVED  | Rreturns0s | 0h    |                                                                                                                                                                                                                                                                                                                                                          |
| 13-0  | RX_MAXLEN | R/W        | 5EEh  | RX Maximum Frame Length - This field determines the maximum<br>length of a received frame.<br>The reset value is 1518 (dec).<br>Frames with byte counts greater than rx_maxlen are long frames.<br>Long frames with no errors are oversized frames.<br>Long frames with CRC, code, or alignment error are jabber frames.<br>The maximum value is 16,383. |

#### **14.5.7.6 BOFFTEST Register (offset = 14h) [reset = 0h]**

Register mask: FFFFFFFFh

BOFFTEST is shown in Figure [14-178](#page-104-1) and described in Table [14-193](#page-104-2).

CPGMAC\_SL BACKOFF TEST REGISTER

**Figure 14-178. BOFFTEST Register**

| 31            | 30 | 29         | 28         | 27 | 26            | 25 | 24         |
|---------------|----|------------|------------|----|---------------|----|------------|
| RESERVED      |    |            | PACEVAL    |    |               |    | RNDNUM     |
| Rreturns0s-0h |    |            | R/W-0h     |    |               |    | R/W-0h     |
| 23            | 22 | 21         | 20         | 19 | 18            | 17 | 16         |
|               |    |            | RNDNUM     |    |               |    |            |
|               |    |            | R/W-0h     |    |               |    |            |
| 15            | 14 | 13         | 12         | 11 | 10            | 9  | 8          |
|               |    | COLL_COUNT |            |    | RESERVED      |    | TX_BACKOFF |
|               |    | R-0h       |            |    | Rreturns0s-0h |    | R-0h       |
| 7             | 6  | 5          | 4          | 3  | 2             | 1  | 0          |
|               |    |            | TX_BACKOFF |    |               |    |            |
|               |    |            | R-0h       |    |               |    |            |
|               |    |            |            |    |               |    |            |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-193. BOFFTEST Register Field Descriptions**

| Bit   | Field      | Type       | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|-------|------------|------------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31    | RESERVED   | Rreturns0s | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 30-26 | PACEVAL    | R/W        | 0h    | Pacing Register Current Value.<br>A non-zero value in this field indicates that transmit pacing is active.<br>A transmit frame collision or deferral causes paceval to loaded with<br>decimal 31, good frame transmissions (with no collisions or<br>deferrals) cause paceval to be decremented down to zero.<br>When paceval is nonzero, the transmitter delays 4 IPGs between<br>new frame transmissions after each successfully transmitted frame<br>that had no deferrals or collisions.<br>Transmit pacing helps reduce "capture" effects improving overall<br>network bandwidth. |
| 25-16 | RNDNUM     | R/W        | 0h    | Backoff Random Number Generator - This field allows the Backoff<br>Random Number Generator to be read (or written in test mode only).<br>This field can be written only when mtest has previously been set.<br>Reading this field returns the generator's current value.<br>The value is reset to zero and begins counting on the clock after the<br>deassertion of reset.                                                                                                                                                                                                             |
| 15-12 | COLL_COUNT | R          | 0h    | Collision Count - The number of collisions the current frame has<br>experienced.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 11-10 | RESERVED   | Rreturns0s | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 9-0   | TX_BACKOFF | R          | 0h    | Backoff Count - This field allows the current value of the backoff<br>counter to be observed for test purposes.<br>This field is loaded automatically according to the backoff algorithm,<br>and is decremented by one for each slot time after the collision.                                                                                                                                                                                                                                                                                                                         |

#### **14.5.7.7 RX\_PAUSE Register (offset = 18h) [reset = 0h]**

Register mask: FFFFFFFFh

RX\_PAUSE is shown in Figure [14-179](#page-105-1) and described in Table [14-194.](#page-105-2)

CPGMAC\_SL RECEIVE PAUSE TIMER REGISTER

### **Figure 14-179. RX\_PAUSE Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23            | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7             | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|---------------|----|----|----|----|----|----|----|----|----|----|----|----|----|---|---|---------------|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    | rx_pausetimer |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   | RESERVED      |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    | R-0h          |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   | Rreturns0s-0h |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-194. RX\_PAUSE Register Field Descriptions**

| Bit   | Field         | Type       | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
|-------|---------------|------------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | rx_pausetimer | R          | 0h    | RX Pause Timer Value - This field allows the contents of the receive<br>pause timer to be observed (and written in test mode).<br>The receive pause timer is loaded with 0xFF00 when the<br>CPGMAC_SL sends an outgoing pause frame (with pause time of<br>0xFFFF).<br>The receive pause timer is decremented at slot time intervals.<br>If the receive pause timer decrements to zero, then another outgoing<br>pause frame will be sent and the load/decrement process will be<br>repeated. |
| 15-0  | RESERVED      | Rreturns0s | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |

#### **14.5.7.8 TX\_PAUSE Register (offset = 1Ch) [reset = 0h]**

Register mask: FFFFFFFFh

TX\_PAUSE is shown in Figure [14-180](#page-106-1) and described in Table [14-195](#page-106-2).

CPGMAC\_SL TRANSMIT PAUSE TIMER REGISTER

**Figure 14-180. TX\_PAUSE Register**

| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23            | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7             | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|----|----|----|----|----|----|----|---------------|----|----|----|----|----|----|----|----|----|----|----|----|----|---|---|---------------|---|---|---|---|---|---|---|
|    |    |    |    |    |    |    |    | tx_pausetimer |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   | RESERVED      |   |   |   |   |   |   |   |
|    |    |    |    |    |    |    |    | R-0h          |    |    |    |    |    |    |    |    |    |    |    |    |    |   |   | Rreturns0s-0h |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-195. TX\_PAUSE Register Field Descriptions**

| Bit   | Field         | Type       | Reset | Description                                                                                                                                                                                                                                                                                                                            |
|-------|---------------|------------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-16 | tx_pausetimer | R          | 0h    | TX Pause Timer Value - This field allows the contents of the transmit<br>pause timer to be observed (and written in test mode).<br>The transmit pause timer is loaded by a received (incoming) pause<br>frame, and then decremented, at slottime intervals, down to zero at<br>which time CPGMAC_SL transmit frames are again enabled. |
| 15-0  | RESERVED      | Rreturns0s | 0h    |                                                                                                                                                                                                                                                                                                                                        |

#### **14.5.7.9 EMCONTROL Register (offset = 20h) [reset = 0h]**

Register mask: FFFFFFFFh

EMCONTROL is shown in Figure [14-181](#page-107-1) and described in Table [14-196](#page-107-2).

CPGMAC\_SL EMULATION CONTROL REGISTER

**Figure 14-181. EMCONTROL Register**

| 31 | 30            | 29            | 28            | 27 | 26 | 25     | 24     |  |  |  |  |  |  |
|----|---------------|---------------|---------------|----|----|--------|--------|--|--|--|--|--|--|
|    |               |               | RESERVED      |    |    |        |        |  |  |  |  |  |  |
|    |               |               | Rreturns0s-0h |    |    |        |        |  |  |  |  |  |  |
| 23 | 22            | 21            | 20            | 19 | 18 | 17     | 16     |  |  |  |  |  |  |
|    |               |               | RESERVED      |    |    |        |        |  |  |  |  |  |  |
|    | Rreturns0s-0h |               |               |    |    |        |        |  |  |  |  |  |  |
| 15 | 14            | 13            | 12            | 11 | 10 | 9      | 8      |  |  |  |  |  |  |
|    |               |               | RESERVED      |    |    |        |        |  |  |  |  |  |  |
|    |               |               | Rreturns0s-0h |    |    |        |        |  |  |  |  |  |  |
| 7  | 6             | 5             | 4             | 3  | 2  | 1      | 0      |  |  |  |  |  |  |
|    |               | RESERVED      |               |    |    | SOFT   | FREE   |  |  |  |  |  |  |
|    |               | Rreturns0s-0h |               |    |    | R/W-0h | R/W-0h |  |  |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-196. EMCONTROL Register Field Descriptions**

| Bit  | Field    | Type       | Reset | Description        |
|------|----------|------------|-------|--------------------|
| 31-2 | RESERVED | Rreturns0s | 0h    |                    |
| 1    | SOFT     | R/W        | 0h    | Emulation Soft Bit |
| 0    | FREE     | R/W        | 0h    | Emulation Free Bit |

#### **14.5.7.10 RX\_PRI\_MAP Register (offset = 24h) [reset = 76543210h]**

Register mask: FFFFFFFFh

RX\_PRI\_MAP is shown in Figure [14-182](#page-108-1) and described in Table [14-197.](#page-108-2)

CPGMAC\_SL RX PKT PRIORITY TO HEADER PRIORITY MAPPING REGISTER

**Figure 14-182. RX\_PRI\_MAP Register**

| 31            | 30 | 29     | 28 | 27            | 26 | 25     | 24 |
|---------------|----|--------|----|---------------|----|--------|----|
| RESERVED      |    | PRI7   |    | RESERVED      |    | PRI6   |    |
| Rreturns0s-0h |    | R/W-7h |    | Rreturns0s-0h |    | R/W-6h |    |
| 23            | 22 | 21     | 20 | 19            | 18 | 17     | 16 |
| RESERVED      |    | PRI5   |    | RESERVED      |    | PRI4   |    |
| Rreturns0s-0h |    | R/W-5h |    | Rreturns0s-0h |    | R/W-4h |    |
| 15            | 14 | 13     | 12 | 11            | 10 | 9      | 8  |
| RESERVED      |    | PRI3   |    | RESERVED      |    | PRI2   |    |
| Rreturns0s-0h |    | R/W-3h |    | Rreturns0s-0h |    | R/W-2h |    |
| 7             | 6  | 5      | 4  | 3             | 2  | 1      | 0  |
| RESERVED      |    | PRI1   |    | RESERVED      |    | PRI0   |    |
| Rreturns0s-0h |    | R/W-1h |    | Rreturns0s-0h |    | R/W-0h |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-197. RX\_PRI\_MAP Register Field Descriptions**

| Bit   | Field    | Type       | Reset | Description                                                                 |
|-------|----------|------------|-------|-----------------------------------------------------------------------------|
| 31    | RESERVED | Rreturns0s | 0h    |                                                                             |
| 30-28 | PRI7     | R/W        | 7h    | Priority<br>7 - A packet priority of 0x7 is mapped (changed) to this value. |
| 27    | RESERVED | Rreturns0s | 0h    |                                                                             |
| 26-24 | PRI6     | R/W        | 6h    | Priority<br>6 - A packet priority of 0x6 is mapped (changed) to this value. |
| 23    | RESERVED | Rreturns0s | 0h    |                                                                             |
| 22-20 | PRI5     | R/W        | 5h    | Priority<br>5 - A packet priority of 0x5 is mapped (changed) to this value. |
| 19    | RESERVED | Rreturns0s | 0h    |                                                                             |
| 18-16 | PRI4     | R/W        | 4h    | Priority<br>4 - A packet priority of 0x4 is mapped (changed) to this value. |
| 15    | RESERVED | Rreturns0s | 0h    |                                                                             |
| 14-12 | PRI3     | R/W        | 3h    | Priority<br>3 - A packet priority of 0x3 is mapped (changed) to this value. |
| 11    | RESERVED | Rreturns0s | 0h    |                                                                             |
| 10-8  | PRI2     | R/W        | 2h    | Priority<br>2 - A packet priority of 0x2 is mapped (changed) to this value. |
| 7     | RESERVED | Rreturns0s | 0h    |                                                                             |
| 6-4   | PRI1     | R/W        | 1h    | Priority<br>1 - A packet priority of 0x1 is mapped (changed) to this value. |
| 3     | RESERVED | Rreturns0s | 0h    |                                                                             |
| 2-0   | PRI0     | R/W        | 0h    | Priority<br>0 - A packet priority of 0x0 is mapped (changed) to this value. |

#### **14.5.7.11 TX\_GAP Register (offset = 28h) [reset = Ch]**

Register mask: FFFFFFFFh

TX\_GAP is shown in Figure [14-183](#page-109-1) and described in Table [14-198.](#page-109-2)

TRANSMIT INTER-PACKET GAP REGISTER

### **Figure 14-183. TX\_GAP Register**

| 31 |               | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17     | 16 | 15     | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|----|---------------|----|----|----|----|----|----|----|----|----|----|----|----|----|--------|----|--------|----|----|----|----|----|---|---|---|---|---|---|---|---|---|---|
|    | RESERVED      |    |    |    |    |    |    |    |    |    |    |    |    |    |        |    | TX_GAP |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |
|    | Rreturns0s-0h |    |    |    |    |    |    |    |    |    |    |    |    |    | R/W-Ch |    |        |    |    |    |    |    |   |   |   |   |   |   |   |   |   |   |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-198. TX\_GAP Register Field Descriptions**

| Bit  | Field    | Type       | Reset | Description               |
|------|----------|------------|-------|---------------------------|
| 31-9 | RESERVED | Rreturns0s | 0h    |                           |
| 8-0  | TX_GAP   | R/W        | Ch    | Transmit Inter-Packet Gap |

### *14.5.8 CPSW\_SS Registers*

Table [14-199](#page-109-3) lists the memory-mapped registers for the CPSW\_SS. All register offset addresses not listed in Table [14-199](#page-109-3) should be considered as reserved locations and the register contents should not be modified.

#### **Table 14-199. CPSW\_SS REGISTERS**

| Offset | Acronym      | Register Name                        | Section           |
|--------|--------------|--------------------------------------|-------------------|
| 0h     | ID_VER       | ID VERSION REGISTER                  | Section 14.5.8.1  |
| 4h     | CONTROL      | SWITCH CONTROL REGISTER              | Section 14.5.8.2  |
| 8h     | SOFT_RESET   | SOFT RESET REGISTER                  | Section 14.5.8.3  |
| Ch     | STAT_PORT_EN | STATISTICS PORT ENABLE REGISTER      | Section 14.5.8.4  |
| 10h    | PTYPE        | TRANSMIT PRIORITY TYPE REGISTER      | Section 14.5.8.5  |
| 14h    | SOFT_IDLE    | SOFTWARE IDLE                        | Section 14.5.8.6  |
| 18h    | THRU_RATE    | THROUGHPUT RATE                      | Section 14.5.8.7  |
| 1Ch    | GAP_THRESH   | CPGMAC_SL SHORT GAP THRESHOLD        | Section 14.5.8.8  |
| 20h    | TX_START_WDS | TRANSMIT START WORDS                 | Section 14.5.8.9  |
| 24h    | FLOW_CONTROL | FLOW CONTROL                         | Section 14.5.8.10 |
| 28h    | VLAN_LTYPE   | LTYPE1 AND LTYPE 2 REGISTER          | Section 14.5.8.11 |
| 2Ch    | TS_LTYPE     | VLAN_LTYPE1 AND VLAN_LTYPE2 REGISTER | Section 14.5.8.12 |
| 30h    | DLR_LTYPE    | DLR LTYPE REGISTER                   | Section 14.5.8.13 |

#### **14.5.8.1 ID\_VER Register (offset = 0h) [reset = 190112h]**

ID\_VER is shown in Figure [14-184](#page-110-1) and described in Table [14-200](#page-110-2).

ID VERSION REGISTER

**Figure 14-184. ID\_VER Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                     CPSW_3G_IDENT                         |
|                           R-0                             |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                     CPSW_3G_IDENT                         |
|                           R-0                             |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-------------------------------+---------------------------+
|        CPSW_3G_RTL_VER        |       CPSW_3G_MAJ_VER     |
|              R-0              |            R-0            |
+-------------------------------+---------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-----------------------------------------------------------+
|                    CPSW_3G_MINOR_VER                      |
|                            R-0                            |
+-----------------------------------------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-200. ID\_VER Register Field Descriptions**

| Bit   | Field             | Type | Reset | Description             |  |  |  |
|-------|-------------------|------|-------|-------------------------|--|--|--|
| 31-16 | CPSW_3G_IDENT     | R-0  | 0     | 3G Identification Value |  |  |  |
| 15-11 | CPSW_3G_RTL_VER   | R-0  | 0     | 3G RTL Version Value    |  |  |  |
| 10-8  | CPSW_3G_MAJ_VER   | R-0  | 0     | 3G Major Version Value  |  |  |  |
| 7-0   | CPSW_3G_MINOR_VER | R-0  | 0     | 3G Minor Version Value  |  |  |  |

#### **14.5.8.2 CONTROL Register (offset = 4h) [reset = 0h]**

CONTROL is shown in Figure [14-185](#page-111-1) and described in Table [14-201](#page-111-2). SWITCH CONTROL REGISTER

**Figure 14-185. CONTROL Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                         R/W-0                             |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                         R/W-0                             |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                         R/W-0                             |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3           2              1             0
+-----------------------+-------+-----------+--------------+-------------+-------------+
|       Reserved        |DLR_EN |RX_VLAN_EN |  VLAN_AWARE  | FIFO_LOOPBA |     CK      |
|        R/W-0          | R/W-0 |  CAP      |    R/W-0     |    R/W-0    |    R/W-0    |
|                       |       |  R/W-0    |              |             |             |
+-----------------------+-------+-----------+--------------+-------------+-------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-201. CONTROL Register Field Descriptions**

| Bit | Field         | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                     |
|-----|---------------|-------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 3   | DLR_EN        | R/W-0 | 0     | DLR enable<br>0 - DLR is disabled.<br>DLR packets will not be moved to queue priority 3 and will not be<br>separated out onto dlr_cpdma_ch.<br>1 - DLR is disabled.<br>DLR packets be moved to destination port transmit queue priority 3<br>and will be separated out onto dlr_cpdma_ch when packet is to<br>egress on port 0. |
| 2   | RX_VLAN_ENCAP | R/W-0 | 0     | Port 0 VLAN Encapsulation (egress):<br>0 - Port 0 receive packets (from 3G) are not VLAN encapsulated.<br>1 - Port 0 receive packets (from 3G) are VLAN encapsulated.                                                                                                                                                           |
| 1   | VLAN_AWARE    | R/W-0 | 0     | VLAN Aware Mode:<br>0 - 3G is in the VLAN unaware mode.<br>1 - 3G is in the VLAN aware mode.                                                                                                                                                                                                                                    |
| 0   | FIFO_LOOPBACK | R/W-0 | 0     | FIFO Loopback Mode<br>0 - Loopback is disabled<br>1 - FIFO Loopback mode enabled.<br>Each packet received is turned around and sent out on the same<br>port's transmit path.<br>Port 2 receive is fixed on channel zero.<br>The RXSOFOVERRUN statistic will increment for every packet sent<br>in FIFO loopback mode.           |

#### **14.5.8.3 SOFT\_RESET Register (offset = 8h) [reset = 0h]**

SOFT\_RESET is shown in Figure [14-186](#page-112-1) and described in Table [14-202.](#page-112-2) SOFT RESET REGISTER

**Figure 14-186. SOFT\_RESET Register**

```
BITS 31..0

|                                              Reserved                                                     |
|                                              R/W-0                                                        |
+-----------------------------------------------------------------------------------------------------------+
                                                                                                   ^
                                                                                                   |
                                                                                               SOFT_RESET
                                                                                               (bit 0, R/W-0)
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-202. SOFT\_RESET Register Field Descriptions**

| Bit | Field      | Type  | Reset | Description                                                                                                                                                                                                                                                                       |
|-----|------------|-------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0   | SOFT_RESET | R/W-0 | 0     | Software reset - Writing a one to this bit causes the 3G logic to be<br>reset.<br>After writing a one to this bit, it may be polled to determine if the<br>reset has occurred.<br>If a one is read, the reset has not yet occurred.<br>If a zero is read then reset has occurred. |

#### **14.5.8.4 STAT\_PORT\_EN Register (offset = Ch) [reset = 0h]**

STAT\_PORT\_EN is shown in Figure [14-187](#page-113-1) and described in Table [14-203.](#page-113-2) STATISTICS PORT ENABLE REGISTER

### **Figure 14-187. STAT\_PORT\_EN Register**

| 31 | 30 | 29       | 28         | 27         | 26         | 25    | 24    |
|----|----|----------|------------|------------|------------|-------|-------|
|    |    |          |            | Reserved   |            |       |       |
| 23 | 22 | 21       | 20         | 19         | 18         | 17    | 16    |
|    |    |          |            | Reserved   |            |       |       |
| 15 | 14 | 13       | 12         | 11         | 10         | 9     | 8     |
|    |    |          |            | Reserved   |            |       |       |
| 7  | 6  | 5        | 4          | 3          | 2          | 1     | 0     |
|    |    | Reserved | P2_STAT_EN | P1_STAT_EN | P0_STAT_EN |       |       |
|    |    |          |            |            | R/W-0      | R/W-0 | R/W-0 |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-203. STAT\_PORT\_EN Register Field Descriptions**

| Bit | Field      | Type  | Reset | Description                                                                                                                                                                                           |
|-----|------------|-------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 2   | P2_STAT_EN | R/W-0 | 0     | Port 2 (GMII2 and Port 2 FIFO) Statistics Enable<br>0 - Port 2 statistics are not enabled.<br>1 - Port 2 statistics are enabled.                                                                      |
| 1   | P1_STAT_EN | R/W-0 | 0     | Port 1 (GMII1 and Port 1 FIFO) Statistics Enable<br>0 - Port 1 statistics are not enabled.<br>1 - Port 1 statistics are enabled.                                                                      |
| 0   | P0_STAT_EN | R/W-0 | 0     | Port 0 Statistics Enable<br>0 - Port 0 statistics are not enabled<br>1 - Port 0 statistics are enabled.<br>FIFO overruns (SOFOVERRUNS) are the only port 0 statistics that<br>are enabled to be kept. |

#### **14.5.8.5 PTYPE Register (offset = 10h) [reset = 0h]**

PTYPE is shown in Figure [14-188](#page-114-1) and described in Table [14-204](#page-114-2).

TRANSMIT PRIORITY TYPE REGISTER

**Figure 14-188. PTYPE Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                          R/W-0                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21          20          19          18          17          16
+-----------+------------+------------+------------+------------+------------+------------+
| Reserved  | P2_PRI3_   | P2_PRI2_   | P2_PRI1_   | P1_PRI3_   | P1_PRI2_   | P1_PRI1_   |
|   R/W-0   | SHAPE_EN   | SHAPE_EN   | SHAPE_EN   | SHAPE_EN   | SHAPE_EN   | SHAPE_EN   |
|           |   R/W-0    |   R/W-0    |   R/W-0    |   R/W-0    |   R/W-0    |   R/W-0    |
+-----------+------------+------------+------------+------------+------------+------------+

ROW 3  (BITS 15..8)
15      14      13      12      11          10          9           8
+-------------------------------+------------+------------+-----------+
|           Reserved            | P2_PTYPE_  | P1_PTYPE_  | P0_PTYPE_ |
|            R/W-0              |   ESC      |   ESC      |   ESC     |
|                               |  R/W-0     |  R/W-0     |  R/W-0    |
+-------------------------------+------------+------------+-----------+

ROW 4  (BITS 7..0)
 7       6       5       4       3                 2                 1                 0
+-------------------------------+-----------------------------------------------------+
|           Reserved            |                    ESC_PRI_LD_VAL                   |
|            R/W-0              |                         R/W-0                       |
+-------------------------------+-----------------------------------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-204. PTYPE Register Field Descriptions**

| Bit | Field            | Type  | Reset | Description                                                                                                                                                                                                                                                                                                   |
|-----|------------------|-------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 21  | P2_PRI3_SHAPE_EN | R/W-0 | 0     | Port 2 Queue Priority 3 Transmit Shape Enable - If there is only one<br>shaping queue then it must be priority 3.                                                                                                                                                                                             |
| 20  | P2_PRI2_SHAPE_EN | R/W-0 | 0     | Port 2 Queue Priority 2 Transmit Shape Enable - If there are two<br>shaping queues then they must be priorities 3 and 2.                                                                                                                                                                                      |
| 19  | P2_PRI1_SHAPE_EN | R/W-0 | 0     | Port 2 Queue Priority 1 Transmit Shape Enable - If there are three<br>shaping queues all three bits should be set.                                                                                                                                                                                            |
| 18  | P1_PRI3_SHAPE_EN | R/W-0 | 0     | Port 1 Queue Priority 3 Transmit Shape Enable - If there is only one<br>shaping queue then it must be priority 3.                                                                                                                                                                                             |
| 17  | P1_PRI2_SHAPE_EN | R/W-0 | 0     | Port 1 Queue Priority 2 Transmit Shape Enable- If there are two<br>shaping queues then they must be priorities 3 and 2.                                                                                                                                                                                       |
| 16  | P1_PRI1_SHAPE_EN | R/W-0 | 0     | Port 1 Queue Priority 1 Transmit Shape Enable- If there are three<br>shaping queues all three bits should be set.                                                                                                                                                                                             |
| 10  | P2_PTYPE_ESC     | R/W-0 | 0     | Port 2 Priority Type Escalate -<br>0 - Port 2 priority type fixed<br>1 - Port 2 priority type escalate Escalate should not be used with<br>queue shaping.                                                                                                                                                     |
| 9   | P1_PTYPE_ESC     | R/W-0 | 0     | Port 1 Priority Type Escalate -<br>0 - Port 1 priority type fixed<br>1 - Port 1 priority type escalate Escalate should not be used with<br>queue shaping.                                                                                                                                                     |
| 8   | P0_PTYPE_ESC     | R/W-0 | 0     | Port 0 Priority Type Escalate -<br>0 - Port 0 priority type fixed<br>1 - Port 0 priority type escalate Escalate should not be used with<br>queue shaping.                                                                                                                                                     |
| 4-0 | ESC_PRI_LD_VAL   | R/W-0 | 0     | Escalate Priority Load Value When a port is in escalate priority, this<br>is the number of higher priority packets sent before the next lower<br>priority is allowed to send a packet.<br>Escalate priority allows lower priority packets to be sent at a fixed<br>rate relative to the next higher priority. |

#### **14.5.8.6 SOFT\_IDLE Register (offset = 14h) [reset = 0h]**

SOFT\_IDLE is shown in Figure [14-189](#page-115-1) and described in Table [14-205.](#page-115-2) SOFTWARE IDLE

#### **Figure 14-189. SOFT\_IDLE Register**

```
BITS 31..0

|                                              Reserved                                                     |
|                                              R/W-0                                                        |
+-----------------------------------------------------------------------------------------------------------+
                                                                                                   ^
                                                                                                   |
                                                                                               ST_IDLE_SOF
                                                                                               (bit 0, R/W-0)
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-205. SOFT\_IDLE Register Field Descriptions**

| Bit | Field     | Type  | Reset | Description                                                                                                          |
|-----|-----------|-------|-------|----------------------------------------------------------------------------------------------------------------------|
| 0   | SOFT_IDLE | R/W-0 | 0     | Software Idle - Setting this bit causes the switch fabric to stop<br>forwarding packets at the next start of packet. |

R/W-0

#### **14.5.8.7 THRU\_RATE Register (offset = 18h) [reset = 3003h]**

THRU\_RATE is shown in Figure [14-190](#page-116-1) and described in Table [14-206](#page-116-2). THROUGHPUT RATE

**Figure 14-190. THRU\_RATE Register**

| 31 | 30       | 29              | 28 | 27       | 26 | 25              | 24 |  |  |
|----|----------|-----------------|----|----------|----|-----------------|----|--|--|
|    |          |                 |    | Reserved |    |                 |    |  |  |
| 23 | 22       | 21              | 20 | 19       | 18 | 17              | 16 |  |  |
|    |          |                 |    | Reserved |    |                 |    |  |  |
| 15 | 14       | 13              | 12 | 11       | 10 | 9               | 8  |  |  |
|    |          | SL_RX_THRU_RATE |    | Reserved |    |                 |    |  |  |
|    |          | R/W-0           |    |          |    |                 |    |  |  |
| 7  | 6        | 5               | 4  | 3        | 2  | 1               | 0  |  |  |
|    | Reserved |                 |    |          |    | CPDMA_THRU_RATE |    |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-206. THRU\_RATE Register Field Descriptions**

| Bit   | Field           | Type  | Reset | Description                                                                                                                                                                                                      |
|-------|-----------------|-------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 15-12 | SL_RX_THRU_RATE | R/W-0 | 0     | CPGMAC_SL Switch FIFO receive through rate.<br>This register value is the maximum throughput of the ethernet ports<br>to the crossbar SCR.<br>The default is one<br>8-byte word for every 3 CLK periods maximum. |
| 3-0   | CPDMA_THRU_RATE | R/W-0 | 0     | CPDMA Switch FIFO receive through rate.<br>This register value is the maximum throughput of the CPDMA host<br>port to the crossbar SCR.<br>The default is one<br>8-byte word for every 3 CLK periods maximum.    |

#### **14.5.8.8 GAP\_THRESH Register (offset = 1Ch) [reset = Bh]**

GAP\_THRESH is shown in Figure [14-191](#page-117-1) and described in Table [14-207](#page-117-2). CPGMAC\_SL SHORT GAP THRESHOLD

#### **Figure 14-191. GAP\_THRESH Register**

```
BITS 31..0

|                                                     Reserved                                                              |
|                                                      R/W-0                                                                |
+-------------------------------------------------------------------------------------------------------------------+-------+
                                                                                                                   |GAP_   |
                                                                                                                   |THRESH |
                                                                                                                   | R/W-0 |
                                                                                                                   +-------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-207. GAP\_THRESH Register Field Descriptions**

| Bit | Field      | Type  | Reset | Description                                                                                                                         |
|-----|------------|-------|-------|-------------------------------------------------------------------------------------------------------------------------------------|
| 4-0 | GAP_THRESH | R/W-0 | 0     | CPGMAC_SL Short Gap Threshold - This is the CPGMAC_SL<br>associated FIFO transmit block usage value for triggering<br>TX_SHORT_GAP. |

#### **14.5.8.9 TX\_START\_WDS Register (offset = 20h) [reset = 20h]**

TX\_START\_WDS is shown in Figure [14-192](#page-118-1) and described in Table [14-208](#page-118-2). TRANSMIT START WORDS

**Figure 14-192. TX\_START\_WDS Register**

```
BITS 31..0

|                                                     Reserved                                                              |
|                                                      R/W-0                                                                |
+---------------------------------------------------------------+-----------------------------------------------------------+
                                                                |                     TX_START_WDS                          |
                                                                |                           R/W-0                           |
                                                                +-----------------------------------------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-208. TX\_START\_WDS Register Field Descriptions**

| Bit  | Field        | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                  |
|------|--------------|-------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 10-0 | TX_START_WDS | R/W-0 | 0     | FIFO Packet Transmit (egress) Start Words.<br>This value is the number of required packet words in the transmit<br>FIFO before the packet egress will begin.<br>This value is non-zero to preclude underrun.<br>Decimal 32 is the recommended value.<br>It should not be increased unnecessairly to prevent adding to the<br>switch latency. |

#### **14.5.8.10 FLOW\_CONTROL Register (offset = 24h) [reset = 1h]**

FLOW\_CONTROL is shown in Figure [14-193](#page-119-1) and described in Table [14-209.](#page-119-2) FLOW CONTROL

**Figure 14-193. FLOW\_CONTROL Register**

| 31 | 30       | 29       | 28 | 27       | 26         | 25         | 24         |  |  |  |  |
|----|----------|----------|----|----------|------------|------------|------------|--|--|--|--|
|    | Reserved |          |    |          |            |            |            |  |  |  |  |
| 23 | 22       | 21       | 20 | 19       | 18         | 17         | 16         |  |  |  |  |
|    | Reserved |          |    |          |            |            |            |  |  |  |  |
| 15 | 14       | 13       | 12 | 11       | 10         | 9          | 8          |  |  |  |  |
|    |          |          |    | Reserved |            |            |            |  |  |  |  |
| 7  | 6        | 5        | 4  | 3        | 2          | 1          | 0          |  |  |  |  |
|    |          | Reserved |    |          | P2_FLOW_EN | P1_FLOW_EN | P0_FLOW_EN |  |  |  |  |
|    |          |          |    |          | R/W-0      | R/W-0      | R/W-0      |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-209. FLOW\_CONTROL Register Field Descriptions**

| Bit | Field      | Type  | Reset | Description                        |
|-----|------------|-------|-------|------------------------------------|
| 2   | P2_FLOW_EN | R/W-0 | 0     | Port 2 Receive flow control enable |
| 1   | P1_FLOW_EN | R/W-0 | 0     | Port 1 Receive flow control enable |
| 0   | P0_FLOW_EN | R/W-0 | 0     | Port 0 Receive flow control enable |

#### **14.5.8.11 VLAN\_LTYPE Register (offset = 28h) [reset = 81008100h]**

VLAN\_LTYPE is shown in Figure [14-194](#page-120-1) and described in Table [14-210.](#page-120-2) LTYPE1 AND LTYPE 2 REGISTER

### **Figure 14-194. VLAN\_LTYPE Register**

| 31 | 30 | 29 | 28 | 27          | 26 | 25 | 24 |
|----|----|----|----|-------------|----|----|----|
|    |    |    |    | VLAN_LTYPE2 |    |    |    |
|    |    |    |    | R/W-0       |    |    |    |
| 23 | 22 | 21 | 20 | 19          | 18 | 17 | 16 |
|    |    |    |    | VLAN_LTYPE2 |    |    |    |
|    |    |    |    | R/W-0       |    |    |    |
| 15 | 14 | 13 | 12 | 11          | 10 | 9  | 8  |
|    |    |    |    | VLAN_LTYPE1 |    |    |    |
|    |    |    |    | R/W-0       |    |    |    |
| 7  | 6  | 5  | 4  | 3           | 2  | 1  | 0  |
|    |    |    |    | VLAN_LTYPE1 |    |    |    |
|    |    |    |    | R/W-0       |    |    |    |
|    |    |    |    |             |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-210. VLAN\_LTYPE Register Field Descriptions**

| Bit   | Field       | Type  | Reset | Description                                                                                                          |
|-------|-------------|-------|-------|----------------------------------------------------------------------------------------------------------------------|
| 31-16 | VLAN_LTYPE2 | R/W-0 | 0     | Time Sync VLAN LTYPE2 This VLAN LTYPE value is used for tx<br>and rx.<br>This is the inner VLAN if both are present. |
| 15-0  | VLAN_LTYPE1 | R/W-0 | 0     | Time Sync VLAN LTYPE1 This VLAN LTYPE value is used for tx<br>and rx.<br>This is the outer VLAN if both are present. |

#### **14.5.8.12 TS\_LTYPE Register (offset = 2Ch) [reset = 0h]**

TS\_LTYPE is shown in Figure [14-195](#page-121-1) and described in Table [14-211](#page-121-2).

VLAN\_LTYPE1 AND VLAN\_LTYPE2 REGISTER

**Figure 14-195. TS\_LTYPE Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                          R/W-0                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------+-----------------------------------+
|       Reserved        |             TS_LTYPE2             |
|        R/W-0          |              R/W-0                |
+-----------------------+-----------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                       TS_LTYPE1                           |
|                         R/W-0                             |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-----------------------------------------------------------+
|                       TS_LTYPE1                           |
|                         R/W-0                             |
+-----------------------------------------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-211. TS\_LTYPE Register Field Descriptions**

| Bit   | Field     | Type  | Reset | Description                                                                              |
|-------|-----------|-------|-------|------------------------------------------------------------------------------------------|
| 21-16 | TS_LTYPE2 | R/W-0 | 0     | Time Sync LTYPE2 This is an Ethertype value to match for tx and rx<br>time sync packets. |
| 15-0  | TS_LTYPE1 | R/W-0 | 0     | Time Sync LTYPE1 This is an ethertype value to match for tx and rx<br>time sync packets. |

#### **14.5.8.13 DLR\_LTYPE Register (offset = 30h) [reset = 80E1h]**

DLR\_LTYPE is shown in Figure [14-196](#page-122-1) and described in Table [14-212](#page-122-2). DLR LTYPE REGISTER

#### **Figure 14-196. DLR\_LTYPE Register**

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0 Reserved DLR\_LTYPE

R/W-0

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-212. DLR\_LTYPE Register Field Descriptions**

| Bit  | Field     | Type  | Reset | Description |
|------|-----------|-------|-------|-------------|
| 15-0 | DLR_LTYPE | R/W-0 | 0     | DLR LTYPE   |

### *14.5.9 CPSW\_WR Registers*

Table [14-213](#page-122-3) lists the memory-mapped registers for the CPSW\_WR. All register offset addresses not listed in Table [14-213](#page-122-3) should be considered as reserved locations and the register contents should not be modified.

### **Table 14-213. CPSW\_WR REGISTERS**

| Offset | Acronym           | Register Name | Section           |
|--------|-------------------|---------------|-------------------|
| 0h     | IDVER             |               | Section 14.5.9.1  |
| 4h     | SOFT_RESET        |               | Section 14.5.9.2  |
| 8h     | CONTROL           |               | Section 14.5.9.3  |
| Ch     | INT_CONTROL       |               | Section 14.5.9.4  |
| 10h    | C0_RX_THRESH_EN   |               | Section 14.5.9.5  |
| 14h    | C0_RX_EN          |               | Section 14.5.9.6  |
| 18h    | C0_TX_EN          |               | Section 14.5.9.7  |
| 1Ch    | C0_MISC_EN        |               | Section 14.5.9.8  |
| 20h    | C1_RX_THRESH_EN   |               | Section 14.5.9.9  |
| 24h    | C1_RX_EN          |               | Section 14.5.9.10 |
| 28h    | C1_TX_EN          |               | Section 14.5.9.11 |
| 2Ch    | C1_MISC_EN        |               | Section 14.5.9.12 |
| 30h    | C2_RX_THRESH_EN   |               | Section 14.5.9.13 |
| 34h    | C2_RX_EN          |               | Section 14.5.9.14 |
| 38h    | C2_TX_EN          |               | Section 14.5.9.15 |
| 3Ch    | C2_MISC_EN        |               | Section 14.5.9.16 |
| 40h    | C0_RX_THRESH_STAT |               | Section 14.5.9.17 |
| 44h    | C0_RX_STAT        |               | Section 14.5.9.18 |
| 48h    | C0_TX_STAT        |               | Section 14.5.9.19 |
| 4Ch    | C0_MISC_STAT      |               | Section 14.5.9.20 |
| 50h    | C1_RX_THRESH_STAT |               | Section 14.5.9.21 |
| 54h    | C1_RX_STAT        |               | Section 14.5.9.22 |
| 58h    | C1_TX_STAT        |               | Section 14.5.9.23 |
| 5Ch    | C1_MISC_STAT      |               | Section 14.5.9.24 |
| 60h    | C2_RX_THRESH_STAT |               | Section 14.5.9.25 |
| 64h    | C2_RX_STAT        |               | Section 14.5.9.26 |
| 68h    | C2_TX_STAT        |               | Section 14.5.9.27 |
| 6Ch    | C2_MISC_STAT      |               | Section 14.5.9.28 |

**Table 14-213. CPSW\_WR REGISTERS (continued)**

| Offset | Acronym    | Register Name | Section           |
|--------|------------|---------------|-------------------|
| 70h    | C0_RX_IMAX |               | Section 14.5.9.29 |
| 74h    | C0_TX_IMAX |               | Section 14.5.9.30 |
| 78h    | C1_RX_IMAX |               | Section 14.5.9.31 |
| 7Ch    | C1_TX_IMAX |               | Section 14.5.9.32 |
| 80h    | C2_RX_IMAX |               | Section 14.5.9.33 |
| 84h    | C2_TX_IMAX |               | Section 14.5.9.34 |
| 88h    | RGMII_CTL  |               | Section 14.5.9.35 |

#### **14.5.9.1 IDVER Register (offset = 0h) [reset = 4EDB0100h]**

IDVER is shown in Figure [14-197](#page-124-1) and described in Table [14-214.](#page-124-2)

SUBSYSTEM ID VERSION REGISTER

**Figure 14-197. IDVER Register**

| 31 | 30       | 29    | 28       | 27       | 26 | 25   | 24 |  |  |
|----|----------|-------|----------|----------|----|------|----|--|--|
|    | SCHEME   |       | Reserved | FUNCTION |    |      |    |  |  |
|    | R-1h     |       | R-0h     | R-EDBh   |    |      |    |  |  |
| 23 | 22       | 21    | 20       | 19       | 18 | 17   | 16 |  |  |
|    | FUNCTION |       |          |          |    |      |    |  |  |
|    | R-EDBh   |       |          |          |    |      |    |  |  |
| 15 | 14       | 13    | 12       | 11       | 10 | 9    | 8  |  |  |
|    |          | RTL   |          | MAJOR    |    |      |    |  |  |
|    |          | R-0h  |          |          |    | R-1h |    |  |  |
| 7  | 6        | 5     | 4        | 3        | 2  | 1    | 0  |  |  |
|    | CUSTOM   | MINOR |          |          |    |      |    |  |  |
|    | R-0h     |       | R-0h     |          |    |      |    |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-214. IDVER Register Field Descriptions**

| Bit   | Field    | Type | Reset | Description    |
|-------|----------|------|-------|----------------|
| 31-30 | SCHEME   | R    | 1h    | Scheme value   |
| 29-28 | Reserved | R    | 0h    |                |
| 27-16 | FUNCTION | R    | EDBh  | function value |
| 15-11 | RTL      | R    | 0h    | rtl version    |
| 10-8  | MAJOR    | R    | 1h    | major version  |
| 7-6   | CUSTOM   | R    | 0h    | custom version |
| 5-0   | MINOR    | R    | 0h    | minor version  |

#### **14.5.9.2 SOFT\_RESET Register (offset = 4h) [reset = 0h]**

SOFT\_RESET is shown in Figure [14-198](#page-125-1) and described in Table [14-215.](#page-125-2) SUBSYSTEM SOFT RESET REGISTER

### **Figure 14-198. SOFT\_RESET Register**

| 31   | 30       | 29 | 28       | 27       | 26 | 25 | 24         |  |  |  |
|------|----------|----|----------|----------|----|----|------------|--|--|--|
|      | Reserved |    |          |          |    |    |            |  |  |  |
| R-0h |          |    |          |          |    |    |            |  |  |  |
| 23   | 22       | 21 | 20       | 19       | 18 | 17 | 16         |  |  |  |
|      | Reserved |    |          |          |    |    |            |  |  |  |
| R-0h |          |    |          |          |    |    |            |  |  |  |
| 15   | 14       | 13 | 12       | 11       | 10 | 9  | 8          |  |  |  |
|      |          |    |          | Reserved |    |    |            |  |  |  |
|      |          |    |          | R-0h     |    |    |            |  |  |  |
| 7    | 6        | 5  | 4        | 3        | 2  | 1  | 0          |  |  |  |
|      |          |    | Reserved |          |    |    | SOFT_RESET |  |  |  |
|      | R-0h     |    |          |          |    |    |            |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-215. SOFT\_RESET Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                      |
|------|------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-1 | Reserved   | R    | 0h    |                                                                                                                                                                                  |
| 0    | SOFT_RESET | R/W  | 0h    | Software reset - Writing a one to this bit causes the CPGMACSS_R<br>logic to be reset (INT, REGS, CPPI).<br>Software reset occurs on the clock following the register bit write. |

#### **14.5.9.3 CONTROL Register (offset = 8h) [reset = 0h]**

CONTROL is shown in Figure [14-199](#page-126-1) and described in Table [14-216](#page-126-2).

SUBSYSTEM CONTROL REGISTER

**Figure 14-199. CONTROL Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3           2           1           0
+-------------------------------+-----------+-----------+-----------+
|           Reserved            | MMR_STDBY | MMR_STDBY | MMR_IDLE  |
|              R-0h             |  MODE     |  MODE     |  MODE     |
|                               |  R/W-0h   |  R/W-0h   |  R/W-0h   |
+-------------------------------+-----------+-----------+-----------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-216. CONTROL Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                                                           |
|------|---------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-4 | Reserved      | R    | 0h    |                                                                                                                                                                       |
| 3-2  | MMR_STDBYMODE | R/W  | 0h    | Configuration of the local initiator state management mode.<br>By definition, initiator may generate read/write transaction as long as<br>it is out of STANDBY state. |
|      |               |      |       | 0x0 = Force-standby mode : Local initiator is unconditionally placed<br>in standbystate.                                                                              |
|      |               |      |       | 0x1 = No-standby mode : Local initiator is unconditionally placed out<br>of standby state.                                                                            |
|      |               |      |       | 0x2 = Reserved : Reserved.                                                                                                                                            |
|      |               |      |       | 0x3 = Reserved : Reserved.                                                                                                                                            |
| 1-0  | MMR_IDLEMODE  | R/W  | 0h    | Configuration of the local initiator state management mode.<br>By definition, initiator may generate read/write transaction as long as<br>it is out of IDLE state.    |
|      |               |      |       | 0x0 = Force-idle mode : Local initiator is unconditionally placed in<br>idle state.                                                                                   |
|      |               |      |       | 0x1 = No-idle mode : Local initiator is unconditionally placed out of<br>idle state.                                                                                  |
|      |               |      |       | 0x2 = Reserved : Reserved.                                                                                                                                            |
|      |               |      |       | 0x3 = Reserved : Reserved.                                                                                                                                            |

#### **14.5.9.4 INT\_CONTROL Register (offset = Ch) [reset = 0h]**

INT\_CONTROL is shown in Figure [14-200](#page-127-1) and described in Table [14-217.](#page-127-2) SUBSYSTEM INTERRUPT CONTROL

**Figure 14-200. INT\_CONTROL Register**

| 31       | 30       | 29       | 28 | 27           | 26          | 25 | 24 |  |  |  |
|----------|----------|----------|----|--------------|-------------|----|----|--|--|--|
| INT_TEST |          | Reserved |    |              |             |    |    |  |  |  |
| R/W-0h   |          | R-0h     |    |              |             |    |    |  |  |  |
| 23       | 22       | 21       | 20 | 19           | 18          | 17 | 16 |  |  |  |
|          | Reserved |          |    |              | INT_PACE_EN |    |    |  |  |  |
|          | R-0h     |          |    |              | R/W-0h      |    |    |  |  |  |
| 15       | 14       | 13       | 12 | 11           | 10          | 9  | 8  |  |  |  |
|          |          | Reserved |    | INT_PRESCALE |             |    |    |  |  |  |
|          |          | R-0h     |    | R-0h         |             |    |    |  |  |  |
| 7        | 6        | 5        | 4  | 3            | 2           | 1  | 0  |  |  |  |
|          |          |          |    | INT_PRESCALE |             |    |    |  |  |  |
|          |          |          |    |              |             |    |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-217. INT\_CONTROL Register Field Descriptions**

| Bit   | Field        | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                         |  |  |  |
|-------|--------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|--|--|
| 31    | INT_TEST     | R/W  | 0h    | Interrupt Test - Test bit to the interrupt pacing blocks                                                                                                                                                                                                                                                                                                                                                                                            |  |  |  |
| 30-22 | Reserved     | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                     |  |  |  |
| 21-16 | INT_PACE_EN  | R/W  | 0h    | Interrupt Pacing Enable Bus int_pace_en[0] - Enables C0_Rx_Pulse<br>Pacing (0 is pacing bypass) int_pace_en[1] - Enables C0_Tx_Pulse<br>Pacing (0 is pacing bypass) int_pace_en[2] - Enables C1_Rx_Pulse<br>Pacing (0 is pacing bypass) int_pace_en[3] - Enables C1_Tx_Pulse<br>Pacing (0 is pacing bypass) int_pace_en[4] - Enables C2_Rx_Pulse<br>Pacing (0 is pacing bypass) int_pace_en[5] - Enables C2_Tx_Pulse<br>Pacing (0 is pacing bypass) |  |  |  |
| 15-12 | Reserved     | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                                                                                                     |  |  |  |
| 11-0  | INT_PRESCALE | R    | 0h    | Interrupt Counter Prescaler - The number of MAIN_CLK periods in<br>4us.                                                                                                                                                                                                                                                                                                                                                                             |  |  |  |

#### **14.5.9.5 C0\_RX\_THRESH\_EN Register (offset = 10h) [reset = 0h]**

C0\_RX\_THRESH\_EN is shown in Figure [14-201](#page-128-1) and described in Table [14-218.](#page-128-2) SUBSYSTEM CORE 0 RECEIVE THRESHOLD INT ENABLE REGISTER

### **Figure 14-201. C0\_RX\_THRESH\_EN Register**

| 31 | 30              | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |
|----|-----------------|----|----|----------|----|----|----|--|--|--|
|    | Reserved        |    |    |          |    |    |    |  |  |  |
|    | R-0h            |    |    |          |    |    |    |  |  |  |
| 23 | 22              | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |
|    | Reserved        |    |    |          |    |    |    |  |  |  |
|    |                 |    |    | R-0h     |    |    |    |  |  |  |
| 15 | 14              | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |
|    |                 |    |    | Reserved |    |    |    |  |  |  |
|    |                 |    |    | R-0h     |    |    |    |  |  |  |
| 7  | 6               | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |
|    | C0_RX_THRESH_EN |    |    |          |    |    |    |  |  |  |
|    |                 |    |    |          |    |    |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-218. C0\_RX\_THRESH\_EN Register Field Descriptions**

| Bit  | Field           | Type | Reset | Description                                                                                                                                                                                 |
|------|-----------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved        | R    | 0h    |                                                                                                                                                                                             |
| 7-0  | C0_RX_THRESH_EN | R/W  | 0h    | Core 0 Receive Threshold Enable - Each bit in this register<br>corresponds to the bit in the receive threshold interrupt that is<br>enabled to generate an interrupt on C0_RX_THRESH_PULSE. |

#### **14.5.9.6 C0\_RX\_EN Register (offset = 14h) [reset = 0h]**

C0\_RX\_EN is shown in Figure [14-202](#page-129-1) and described in Table [14-219.](#page-129-2)

SUBSYSTEM CORE 0 RECEIVE INTERRUPT ENABLE REGISTER

**Figure 14-202. C0\_RX\_EN Register**

| 31 | 30 | 29 | 28 | 27       | 26 | 25 | 24 |
|----|----|----|----|----------|----|----|----|
|    |    |    |    | Reserved |    |    |    |
|    |    |    |    | R-0h     |    |    |    |
| 23 | 22 | 21 | 20 | 19       | 18 | 17 | 16 |
|    |    |    |    | Reserved |    |    |    |
|    |    |    |    | R-0h     |    |    |    |
| 15 | 14 | 13 | 12 | 11       | 10 | 9  | 8  |
|    |    |    |    | Reserved |    |    |    |
|    |    |    |    | R-0h     |    |    |    |
| 7  | 6  | 5  | 4  | 3        | 2  | 1  | 0  |
|    |    |    |    | C0_RX_EN |    |    |    |
|    |    |    |    |          |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-219. C0\_RX\_EN Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                 |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved | R    | 0h    |                                                                                                                                                             |
| 7-0  | C0_RX_EN | R/W  | 0h    | Core 0 Receive Enable - Each bit in this register corresponds to the<br>bit in the rx interrupt that is enabled to generate an interrupt on<br>C0_RX_PULSE. |

#### **14.5.9.7 C0\_TX\_EN Register (offset = 18h) [reset = 0h]**

C0\_TX\_EN is shown in Figure [14-203](#page-130-1) and described in Table [14-220.](#page-130-2)

#### SUBSYSTEM CORE 0 TRANSMIT INTERRUPT ENABLE REGISTER

### **Figure 14-203. C0\_TX\_EN Register**

| 31 | 30 | 29 | 28 | 27       | 26 | 25 | 24 |
|----|----|----|----|----------|----|----|----|
|    |    |    |    | Reserved |    |    |    |
|    |    |    |    | R-0h     |    |    |    |
| 23 | 22 | 21 | 20 | 19       | 18 | 17 | 16 |
|    |    |    |    | Reserved |    |    |    |
|    |    |    |    | R-0h     |    |    |    |
| 15 | 14 | 13 | 12 | 11       | 10 | 9  | 8  |
|    |    |    |    | Reserved |    |    |    |
|    |    |    |    | R-0h     |    |    |    |
| 7  | 6  | 5  | 4  | 3        | 2  | 1  | 0  |
|    |    |    |    | C0_TX_EN |    |    |    |
|    |    |    |    |          |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-220. C0\_TX\_EN Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                  |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved | R    | 0h    |                                                                                                                                                              |
| 7-0  | C0_TX_EN | R/W  | 0h    | Core 0 Transmit Enable - Each bit in this register corresponds to the<br>bit in the tx interrupt that is enabled to generate an interrupt on<br>C0_TX_PULSE. |

#### **14.5.9.8 C0\_MISC\_EN Register (offset = 1Ch) [reset = 0h]**

C0\_MISC\_EN is shown in Figure [14-204](#page-131-1) and described in Table [14-221](#page-131-2).

SUBSYSTEM CORE 0 MISC INTERRUPT ENABLE REGISTER

**Figure 14-204. C0\_MISC\_EN Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-------------------------------+---------------------------+
|           Reserved            |         C0_MISC_EN        |
|             R-0h              |          R/W-0h           |
+-------------------------------+---------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-221. C0\_MISC\_EN Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                |
|------|------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-5 | Reserved   | R    | 0h    |                                                                                                                                                                                                                                                                                                                                            |
| 4-0  | C0_MISC_EN | R/W  | 0h    | Core 0 Misc Enable - Each bit in this register corresponds to the<br>miscellaneous interrupt (evnt_pend, stat_pend, host_pend,<br>mdio_linkint, mdio_userint) that is enabled to generate an interrupt<br>on C0_Misc_PULSE.<br>Bit 4 = evnt_pend<br>Bit 3 = stat_pend<br>Bit 2 = host_pend<br>Bit 1 = mdio_linkint<br>Bit 0 = mdio_userint |

#### **14.5.9.9 C1\_RX\_THRESH\_EN Register (offset = 20h) [reset = 0h]**

C1\_RX\_THRESH\_EN is shown in Figure [14-205](#page-132-1) and described in Table [14-222.](#page-132-2)

SUBSYSTEM CORE 1 RECEIVE THRESHOLD INT ENABLE REGISTER

### **Figure 14-205. C1\_RX\_THRESH\_EN Register**

| 31 | 30              | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |
|----|-----------------|----|----|----------|----|----|----|--|--|--|
|    | Reserved        |    |    |          |    |    |    |  |  |  |
|    | R-0h            |    |    |          |    |    |    |  |  |  |
| 23 | 22              | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |
|    | Reserved        |    |    |          |    |    |    |  |  |  |
|    | R-0h            |    |    |          |    |    |    |  |  |  |
| 15 | 14              | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |
|    |                 |    |    | Reserved |    |    |    |  |  |  |
|    | R-0h            |    |    |          |    |    |    |  |  |  |
| 7  | 6               | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |
|    | C1_RX_THRESH_EN |    |    |          |    |    |    |  |  |  |
|    |                 |    |    |          |    |    |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-222. C1\_RX\_THRESH\_EN Register Field Descriptions**

| Bit  | Field           | Type | Reset | Description                                                                                                                                                                                 |
|------|-----------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved        | R    | 0h    |                                                                                                                                                                                             |
| 7-0  | C1_RX_THRESH_EN | R/W  | 0h    | Core 1 Receive Threshold Enable - Each bit in this register<br>corresponds to the bit in the receive threshold interrupt that is<br>enabled to generate an interrupt on C1_RX_THRESH_PULSE. |

#### **14.5.9.10 C1\_RX\_EN Register (offset = 24h) [reset = 0h]**

C1\_RX\_EN is shown in Figure [14-206](#page-133-1) and described in Table [14-223.](#page-133-2)

SUBSYSTEM CORE 1 RECEIVE INTERRUPT ENABLE REGISTER

**Figure 14-206. C1\_RX\_EN Register**

| 31 | 30       | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |
|----|----------|----|----|----------|----|----|----|--|--|--|
|    |          |    |    | Reserved |    |    |    |  |  |  |
|    |          |    |    | R-0h     |    |    |    |  |  |  |
| 23 | 22       | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |
|    | Reserved |    |    |          |    |    |    |  |  |  |
|    | R-0h     |    |    |          |    |    |    |  |  |  |
| 15 | 14       | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |
|    |          |    |    | Reserved |    |    |    |  |  |  |
|    |          |    |    | R-0h     |    |    |    |  |  |  |
| 7  | 6        | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |
|    |          |    |    | C1_RX_EN |    |    |    |  |  |  |
|    |          |    |    | R/W-0h   |    |    |    |  |  |  |
|    |          |    |    |          |    |    |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

#### **Table 14-223. C1\_RX\_EN Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                 |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved | R    | 0h    |                                                                                                                                                             |
| 7-0  | C1_RX_EN | R/W  | 0h    | Core 1 Receive Enable - Each bit in this register corresponds to the<br>bit in the rx interrupt that is enabled to generate an interrupt on<br>C1_RX_PULSE. |

#### **14.5.9.11 C1\_TX\_EN Register (offset = 28h) [reset = 0h]**

C1\_TX\_EN is shown in Figure [14-207](#page-134-1) and described in Table [14-224.](#page-134-2)

### SUBSYSTEM CORE 1 TRANSMIT INTERRUPT ENABLE REGISTER

**Figure 14-207. C1\_TX\_EN Register**

| 30       | 29 | 28 | 27 | 26                           | 25 | 24 |  |  |  |  |
|----------|----|----|----|------------------------------|----|----|--|--|--|--|
| Reserved |    |    |    |                              |    |    |  |  |  |  |
| R-0h     |    |    |    |                              |    |    |  |  |  |  |
| 22       | 21 | 20 | 19 | 18                           | 17 | 16 |  |  |  |  |
| Reserved |    |    |    |                              |    |    |  |  |  |  |
| R-0h     |    |    |    |                              |    |    |  |  |  |  |
| 14       | 13 | 12 | 11 | 10                           | 9  | 8  |  |  |  |  |
|          |    |    |    |                              |    |    |  |  |  |  |
|          |    |    |    |                              |    |    |  |  |  |  |
| 6        | 5  | 4  | 3  | 2                            | 1  | 0  |  |  |  |  |
|          |    |    |    |                              |    |    |  |  |  |  |
| R/W-0h   |    |    |    |                              |    |    |  |  |  |  |
|          |    |    |    | Reserved<br>R-0h<br>C1_TX_EN |    |    |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-224. C1\_TX\_EN Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                  |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved | R    | 0h    |                                                                                                                                                              |
| 7-0  | C1_TX_EN | R/W  | 0h    | Core 1 Transmit Enable - Each bit in this register corresponds to the<br>bit in the tx interrupt that is enabled to generate an interrupt on<br>C1_TX_PULSE. |

#### **14.5.9.12 C1\_MISC\_EN Register (offset = 2Ch) [reset = 0h]**

C1\_MISC\_EN is shown in Figure [14-208](#page-135-1) and described in Table [14-225](#page-135-2).

SUBSYSTEM CORE 1 MISC INTERRUPT ENABLE REGISTER

**Figure 14-208. C1\_MISC\_EN Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-------------------------------+---------------------------+
|           Reserved            |         C1_MISC_EN        |
|             R-0h              |          R/W-0h           |
+-------------------------------+---------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-225. C1\_MISC\_EN Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                |
|------|------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-5 | Reserved   | R    | 0h    |                                                                                                                                                                                                                                                                                                                                            |
| 4-0  | C1_MISC_EN | R/W  | 0h    | Core 1 Misc Enable - Each bit in this register corresponds to the<br>miscellaneous interrupt (evnt_pend, stat_pend, host_pend,<br>mdio_linkint, mdio_userint) that is enabled to generate an interrupt<br>on C1_Misc_PULSE.<br>Bit 4 = evnt_pend<br>Bit 3 = stat_pend<br>Bit 2 = host_pend<br>Bit 1 = mdio_linkint<br>Bit 0 = mdio_userint |

#### **14.5.9.13 C2\_RX\_THRESH\_EN Register (offset = 30h) [reset = 0h]**

C2\_RX\_THRESH\_EN is shown in Figure [14-209](#page-136-1) and described in Table [14-226.](#page-136-2)

SUBSYSTEM CORE 2 RECEIVE THRESHOLD INT ENABLE REGISTER

### **Figure 14-209. C2\_RX\_THRESH\_EN Register**

| 31 | 30              | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |  |
|----|-----------------|----|----|----------|----|----|----|--|--|--|--|
|    | Reserved        |    |    |          |    |    |    |  |  |  |  |
|    | R-0h            |    |    |          |    |    |    |  |  |  |  |
| 23 | 22              | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |  |
|    | Reserved        |    |    |          |    |    |    |  |  |  |  |
|    | R-0h            |    |    |          |    |    |    |  |  |  |  |
| 15 | 14              | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |  |
|    |                 |    |    | Reserved |    |    |    |  |  |  |  |
|    |                 |    |    | R-0h     |    |    |    |  |  |  |  |
| 7  | 6               | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |  |
|    | C2_RX_THRESH_EN |    |    |          |    |    |    |  |  |  |  |
|    |                 |    |    |          |    |    |    |  |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-226. C2\_RX\_THRESH\_EN Register Field Descriptions**

| Bit  | Field           | Type | Reset | Description                                                                                                                                                                                 |
|------|-----------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved        | R    | 0h    |                                                                                                                                                                                             |
| 7-0  | C2_RX_THRESH_EN | R/W  | 0h    | Core 2 Receive Threshold Enable - Each bit in this register<br>corresponds to the bit in the receive threshold interrupt that is<br>enabled to generate an interrupt on C2_RX_THRESH_PULSE. |

#### **14.5.9.14 C2\_RX\_EN Register (offset = 34h) [reset = 0h]**

C2\_RX\_EN is shown in Figure [14-210](#page-137-1) and described in Table [14-227.](#page-137-2)

SUBSYSTEM CORE 2 RECEIVE INTERRUPT ENABLE REGISTER

**Figure 14-210. C2\_RX\_EN Register**

| 31 | 30   | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |
|----|------|----|----|----------|----|----|----|--|--|--|
|    |      |    |    | Reserved |    |    |    |  |  |  |
|    |      |    |    | R-0h     |    |    |    |  |  |  |
| 23 | 22   | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |
|    |      |    |    | Reserved |    |    |    |  |  |  |
|    |      |    |    | R-0h     |    |    |    |  |  |  |
| 15 | 14   | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |
|    |      |    |    | Reserved |    |    |    |  |  |  |
|    | R-0h |    |    |          |    |    |    |  |  |  |
| 7  | 6    | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |
|    |      |    |    | C2_RX_EN |    |    |    |  |  |  |
|    |      |    |    |          |    |    |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-227. C2\_RX\_EN Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                 |
|------|----------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved | R    | 0h    |                                                                                                                                                             |
| 7-0  | C2_RX_EN | R/W  | 0h    | Core 2 Receive Enable - Each bit in this register corresponds to the<br>bit in the rx interrupt that is enabled to generate an interrupt on<br>C2_RX_PULSE. |

#### **14.5.9.15 C2\_TX\_EN Register (offset = 38h) [reset = 0h]**

C2\_TX\_EN is shown in Figure [14-211](#page-138-1) and described in Table [14-228.](#page-138-2)

#### SUBSYSTEM CORE 2 TRANSMIT INTERRUPT ENABLE REGISTER

### **Figure 14-211. C2\_TX\_EN Register**

| 31   | 30       | 29 | 28 | 27       | 26 | 25 | 24 |  |  |  |
|------|----------|----|----|----------|----|----|----|--|--|--|
|      | Reserved |    |    |          |    |    |    |  |  |  |
| R-0h |          |    |    |          |    |    |    |  |  |  |
| 23   | 22       | 21 | 20 | 19       | 18 | 17 | 16 |  |  |  |
|      | Reserved |    |    |          |    |    |    |  |  |  |
|      | R-0h     |    |    |          |    |    |    |  |  |  |
| 15   | 14       | 13 | 12 | 11       | 10 | 9  | 8  |  |  |  |
|      |          |    |    | Reserved |    |    |    |  |  |  |
|      |          |    |    | R-0h     |    |    |    |  |  |  |
| 7    | 6        | 5  | 4  | 3        | 2  | 1  | 0  |  |  |  |
|      |          |    |    | C2_TX_EN |    |    |    |  |  |  |
|      | R/W-0h   |    |    |          |    |    |    |  |  |  |
|      |          |    |    |          |    |    |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-228. C2\_TX\_EN Register Field Descriptions**

| Bit  | Field    | Type | Reset | Description                                                                                                                                                  |
|------|----------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved | R    | 0h    |                                                                                                                                                              |
| 7-0  | C2_TX_EN | R/W  | 0h    | Core 2 Transmit Enable - Each bit in this register corresponds to the<br>bit in the tx interrupt that is enabled to generate an interrupt on<br>C2_TX_PULSE. |

#### **14.5.9.16 C2\_MISC\_EN Register (offset = 3Ch) [reset = 0h]**

C2\_MISC\_EN is shown in Figure [14-212](#page-139-1) and described in Table [14-229](#page-139-2).

SUBSYSTEM CORE 2 MISC INTERRUPT ENABLE REGISTER

**Figure 14-212. C2\_MISC\_EN Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-------------------------------+---------------------------+
|           Reserved            |         C2_MISC_EN        |
|             R-0h              |          R/W-0h           |
+-------------------------------+---------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-229. C2\_MISC\_EN Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                |
|------|------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-5 | Reserved   | R    | 0h    |                                                                                                                                                                                                                                                                                                                                            |
| 4-0  | C2_MISC_EN | R/W  | 0h    | Core 2 Misc Enable - Each bit in this register corresponds to the<br>miscellaneous interrupt (evnt_pend, stat_pend, host_pend,<br>mdio_linkint, mdio_userint) that is enabled to generate an interrupt<br>on C2_Misc_PULSE.<br>Bit 4 = evnt_pend<br>Bit 3 = stat_pend<br>Bit 2 = host_pend<br>Bit 1 = mdio_linkint<br>Bit 0 = mdio_userint |

#### **14.5.9.17 C0\_RX\_THRESH\_STAT Register (offset = 40h) [reset = 0h]**

C0\_RX\_THRESH\_STAT is shown in Figure [14-213](#page-140-1) and described in Table [14-230](#page-140-2). SUBSYSTEM CORE 0 RX THRESHOLD MASKED INT STATUS REGISTER

### **Figure 14-213. C0\_RX\_THRESH\_STAT Register**

| 31 | 30       | 29 | 28 | 27                | 26 | 25 | 24 |  |  |
|----|----------|----|----|-------------------|----|----|----|--|--|
|    |          |    |    | Reserved          |    |    |    |  |  |
|    |          |    |    | R-0h              |    |    |    |  |  |
| 23 | 22       | 21 | 20 | 19                | 18 | 17 | 16 |  |  |
|    | Reserved |    |    |                   |    |    |    |  |  |
|    |          |    |    | R-0h              |    |    |    |  |  |
| 15 | 14       | 13 | 12 | 11                | 10 | 9  | 8  |  |  |
|    |          |    |    | Reserved          |    |    |    |  |  |
|    |          |    |    | R-0h              |    |    |    |  |  |
| 7  | 6        | 5  | 4  | 3                 | 2  | 1  | 0  |  |  |
|    |          |    |    | C0_RX_THRESH_STAT |    |    |    |  |  |
|    |          |    |    |                   |    |    |    |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-230. C0\_RX\_THRESH\_STAT Register Field Descriptions**

| Bit  | Field             | Type | Reset | Description                                                                                                                                                                                                                  |
|------|-------------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved          | R    | 0h    |                                                                                                                                                                                                                              |
| 7-0  | C0_RX_THRESH_STAT | R    | 0h    | Core 0 Receive Threshold Masked Interrupt Status - Each bit in this<br>read only register corresponds to the bit in the receive threshold<br>interrupt that is enabled and generating an interrupt on<br>C0_RX_THRESH_PULSE. |

#### **14.5.9.18 C0\_RX\_STAT Register (offset = 44h) [reset = 0h]**

C0\_RX\_STAT is shown in Figure [14-214](#page-141-1) and described in Table [14-231.](#page-141-2)

SUBSYSTEM CORE 0 RX INTERRUPT MASKED INT STATUS REGISTER

**Figure 14-214. C0\_RX\_STAT Register**

| 31 | 30 | 29 | 28 | 27         | 26 | 25 | 24 |
|----|----|----|----|------------|----|----|----|
|    |    |    |    | Reserved   |    |    |    |
|    |    |    |    | R-0h       |    |    |    |
| 23 | 22 | 21 | 20 | 19         | 18 | 17 | 16 |
|    |    |    |    | Reserved   |    |    |    |
|    |    |    |    | R-0h       |    |    |    |
| 15 | 14 | 13 | 12 | 11         | 10 | 9  | 8  |
|    |    |    |    | Reserved   |    |    |    |
|    |    |    |    | R-0h       |    |    |    |
| 7  | 6  | 5  | 4  | 3          | 2  | 1  | 0  |
|    |    |    |    | C0_RX_STAT |    |    |    |
|    |    |    |    |            |    |    |    |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-231. C0\_RX\_STAT Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                               |
|------|------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved   | R    | 0h    |                                                                                                                                                                                           |
| 7-0  | C0_RX_STAT | R    | 0h    | Core 0 Receive Masked Interrupt Status - Each bit in this read only<br>register corresponds to the bit in the Rx interrupt that is enabled and<br>generating an interrupt on C0_RX_PULSE. |

#### **14.5.9.19 C0\_TX\_STAT Register (offset = 48h) [reset = 0h]**

C0\_TX\_STAT is shown in Figure [14-215](#page-142-1) and described in Table [14-232](#page-142-2).

#### SUBSYSTEM CORE 0 TX INTERRUPT MASKED INT STATUS REGISTER

**Figure 14-215. C0\_TX\_STAT Register**

| 31       | 30 | 29 | 28 | 27         | 26 | 25 | 24 |  |  |
|----------|----|----|----|------------|----|----|----|--|--|
| Reserved |    |    |    |            |    |    |    |  |  |
| R-0h     |    |    |    |            |    |    |    |  |  |
| 23       | 22 | 21 | 20 | 19         | 18 | 17 | 16 |  |  |
| Reserved |    |    |    |            |    |    |    |  |  |
| R-0h     |    |    |    |            |    |    |    |  |  |
| 15       | 14 | 13 | 12 | 11         | 10 | 9  | 8  |  |  |
|          |    |    |    | Reserved   |    |    |    |  |  |
|          |    |    |    | R-0h       |    |    |    |  |  |
| 7        | 6  | 5  | 4  | 3          | 2  | 1  | 0  |  |  |
|          |    |    |    | C0_TX_STAT |    |    |    |  |  |
| R-0h     |    |    |    |            |    |    |    |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-232. C0\_TX\_STAT Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                 |
|------|------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved   | R    | 0h    |                                                                                                                                                                                             |
| 7-0  | C0_TX_STAT | R    | 0h    | Core 0 Transmit Masked Interrupt Status - Each bit in this read only<br>register corresponds to the bit in the Tx interrupt that is enabled and<br>generating an interrupt on C0_TX_PULSE . |

#### **14.5.9.20 C0\_MISC\_STAT Register (offset = 4Ch) [reset = 0h]**

C0\_MISC\_STAT is shown in Figure [14-216](#page-143-1) and described in Table [14-233.](#page-143-2)

SUBSYSTEM CORE 0 MISC INTERRUPT MASKED INT STATUS REGISTER

**Figure 14-216. C0\_MISC\_STAT Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-------------------------------+---------------------------+
|           Reserved            |       C0_MISC_STAT        |
|             R-0h              |           R-0h            |
+-------------------------------+---------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-233. C0\_MISC\_STAT Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                    |
|------|--------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-5 | Reserved     | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                |
| 4-0  | C0_MISC_STAT | R    | 0h    | Core 0 Misc Masked Interrupt Status - Each bit in this register<br>corresponds to the miscellaneous interrupt (evnt_pend, stat_pend,<br>host_pend, mdio_linkint, mdio_userint) that is enabled and<br>generating an interrupt on C0_MISC_PULSE.<br>Bit 4 = evnt_pend<br>Bit 3 = stat_pend<br>Bit 2 = host_pend<br>Bit 1 = mdio_linkint<br>Bit 0 = mdio_userint |

#### **14.5.9.21 C1\_RX\_THRESH\_STAT Register (offset = 50h) [reset = 0h]**

C1\_RX\_THRESH\_STAT is shown in Figure [14-217](#page-144-1) and described in Table [14-234](#page-144-2). SUBSYSTEM CORE 1 RX THRESHOLD MASKED INT STATUS REGISTER

**Figure 14-217. C1\_RX\_THRESH\_STAT Register**

| 31 | 30       | 29 | 28 | 27                | 26 | 25 | 24 |  |  |  |
|----|----------|----|----|-------------------|----|----|----|--|--|--|
|    | Reserved |    |    |                   |    |    |    |  |  |  |
|    | R-0h     |    |    |                   |    |    |    |  |  |  |
| 23 | 22       | 21 | 20 | 19                | 18 | 17 | 16 |  |  |  |
|    | Reserved |    |    |                   |    |    |    |  |  |  |
|    | R-0h     |    |    |                   |    |    |    |  |  |  |
| 15 | 14       | 13 | 12 | 11                | 10 | 9  | 8  |  |  |  |
|    |          |    |    | Reserved          |    |    |    |  |  |  |
|    | R-0h     |    |    |                   |    |    |    |  |  |  |
| 7  | 6        | 5  | 4  | 3                 | 2  | 1  | 0  |  |  |  |
|    |          |    |    | C1_RX_THRESH_STAT |    |    |    |  |  |  |
|    |          |    |    |                   |    |    |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-234. C1\_RX\_THRESH\_STAT Register Field Descriptions**

| Bit  | Field             | Type | Reset | Description                                                                                                                                                                                                     |
|------|-------------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved          | R    | 0h    |                                                                                                                                                                                                                 |
| 7-0  | C1_RX_THRESH_STAT | R    | 0h    | Core 1 Receive Threshold Masked Interrupt Status - Each bit in this<br>register corresponds to the bit in the receive threshold interrupt that<br>is enabled and generating an interrupt on C1_RX_THRESH_PULSE. |

#### **14.5.9.22 C1\_RX\_STAT Register (offset = 54h) [reset = 0h]**

C1\_RX\_STAT is shown in Figure [14-218](#page-145-1) and described in Table [14-235.](#page-145-2)

SUBSYSTEM CORE 1 RECEIVE MASKED INTERRUPT STATUS REGISTER

**Figure 14-218. C1\_RX\_STAT Register**

| 30       | 29 | 28 | 27 | 26                             | 25 | 24 |  |  |  |
|----------|----|----|----|--------------------------------|----|----|--|--|--|
| Reserved |    |    |    |                                |    |    |  |  |  |
| R-0h     |    |    |    |                                |    |    |  |  |  |
| 22       | 21 | 20 | 19 | 18                             | 17 | 16 |  |  |  |
| Reserved |    |    |    |                                |    |    |  |  |  |
| R-0h     |    |    |    |                                |    |    |  |  |  |
| 14       | 13 | 12 | 11 | 10                             | 9  | 8  |  |  |  |
|          |    |    |    |                                |    |    |  |  |  |
|          |    |    |    |                                |    |    |  |  |  |
| 6        | 5  | 4  | 3  | 2                              | 1  | 0  |  |  |  |
|          |    |    |    |                                |    |    |  |  |  |
| R-0h     |    |    |    |                                |    |    |  |  |  |
|          |    |    |    | Reserved<br>R-0h<br>C1_RX_STAT |    |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-235. C1\_RX\_STAT Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                     |
|------|------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved   | R    | 0h    |                                                                                                                                                                                 |
| 7-0  | C1_RX_STAT | R    | 0h    | Core 1 Receive Masked Interrupt Status - Each bit in this register<br>corresponds to the bit in the Rx interrupt that is enabled and<br>generating an interrupt on C1_RX_PULSE. |

#### **14.5.9.23 C1\_TX\_STAT Register (offset = 58h) [reset = 0h]**

C1\_TX\_STAT is shown in Figure [14-219](#page-146-1) and described in Table [14-236](#page-146-2).

SUBSYSTEM CORE 1 TRANSMIT MASKED INTERRUPT STATUS REGISTER

### **Figure 14-219. C1\_TX\_STAT Register**

| 31       | 30       | 29 | 28 | 27         | 26 | 25 | 24 |  |  |  |
|----------|----------|----|----|------------|----|----|----|--|--|--|
|          | Reserved |    |    |            |    |    |    |  |  |  |
|          | R-0h     |    |    |            |    |    |    |  |  |  |
| 23       | 22       | 21 | 20 | 19         | 18 | 17 | 16 |  |  |  |
| Reserved |          |    |    |            |    |    |    |  |  |  |
|          | R-0h     |    |    |            |    |    |    |  |  |  |
| 15       | 14       | 13 | 12 | 11         | 10 | 9  | 8  |  |  |  |
|          |          |    |    | Reserved   |    |    |    |  |  |  |
|          |          |    |    | R-0h       |    |    |    |  |  |  |
| 7        | 6        | 5  | 4  | 3          | 2  | 1  | 0  |  |  |  |
|          |          |    |    | C1_TX_STAT |    |    |    |  |  |  |
|          |          |    |    | R-0h       |    |    |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-236. C1\_TX\_STAT Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                      |
|------|------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved   | R    | 0h    |                                                                                                                                                                                  |
| 7-0  | C1_TX_STAT | R    | 0h    | Core 1 Transmit Masked Interrupt Status - Each bit in this register<br>corresponds to the bit in the Tx interrupt that is enabled and<br>generating an interrupt on C1_TX_PULSE. |

#### **14.5.9.24 C1\_MISC\_STAT Register (offset = 5Ch) [reset = 0h]**

C1\_MISC\_STAT is shown in Figure [14-220](#page-147-1) and described in Table [14-237.](#page-147-2)

SUBSYSTEM CORE 1 MISC MASKED INTERRUPT STATUS REGISTER

**Figure 14-220. C1\_MISC\_STAT Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-------------------------------+---------------------------+
|           Reserved            |       C1_MISC_STAT        |
|             R-0h              |           R-0h            |
+-------------------------------+---------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-237. C1\_MISC\_STAT Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                    |
|------|--------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-5 | Reserved     | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                |
| 4-0  | C1_MISC_STAT | R    | 0h    | Core 1 Misc Masked Interrupt Status - Each bit in this register<br>corresponds to the miscellaneous interrupt (evnt_pend, stat_pend,<br>host_pend, mdio_linkint, mdio_userint) that is enabled and<br>generating an interrupt on C1_MISC_PULSE.<br>Bit 4 = evnt_pend<br>Bit 3 = stat_pend<br>Bit 2 = host_pend<br>Bit 1 = mdio_linkint<br>Bit 0 = mdio_userint |

#### **14.5.9.25 C2\_RX\_THRESH\_STAT Register (offset = 60h) [reset = 0h]**

C2\_RX\_THRESH\_STAT is shown in Figure [14-221](#page-148-1) and described in Table [14-238](#page-148-2). SUBSYSTEM CORE 2 RX THRESHOLD MASKED INT STATUS REGISTER

**Figure 14-221. C2\_RX\_THRESH\_STAT Register**

| 31 | 30       | 29 | 28 | 27                | 26 | 25 | 24 |  |  |  |
|----|----------|----|----|-------------------|----|----|----|--|--|--|
|    | Reserved |    |    |                   |    |    |    |  |  |  |
|    | R-0h     |    |    |                   |    |    |    |  |  |  |
| 23 | 22       | 21 | 20 | 19                | 18 | 17 | 16 |  |  |  |
|    | Reserved |    |    |                   |    |    |    |  |  |  |
|    | R-0h     |    |    |                   |    |    |    |  |  |  |
| 15 | 14       | 13 | 12 | 11                | 10 | 9  | 8  |  |  |  |
|    |          |    |    | Reserved          |    |    |    |  |  |  |
|    | R-0h     |    |    |                   |    |    |    |  |  |  |
| 7  | 6        | 5  | 4  | 3                 | 2  | 1  | 0  |  |  |  |
|    |          |    |    | C2_RX_THRESH_STAT |    |    |    |  |  |  |
|    |          |    |    |                   |    |    |    |  |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-238. C2\_RX\_THRESH\_STAT Register Field Descriptions**

| Bit  | Field             | Type | Reset | Description                                                                                                                                                                                                     |
|------|-------------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved          | R    | 0h    |                                                                                                                                                                                                                 |
| 7-0  | C2_RX_THRESH_STAT | R    | 0h    | Core 2 Receive Threshold Masked Interrupt Status - Each bit in this<br>register corresponds to the bit in the receive threshold interrupt that<br>is enabled and generating an interrupt on C2_RX_THRESH_PULSE. |

#### **14.5.9.26 C2\_RX\_STAT Register (offset = 64h) [reset = 0h]**

C2\_RX\_STAT is shown in Figure [14-222](#page-149-1) and described in Table [14-239.](#page-149-2)

### SUBSYSTEM CORE 2 RECEIVE MASKED INTERRUPT STATUS REGISTER

**Figure 14-222. C2\_RX\_STAT Register**

| 31       | 30 | 29 | 28 | 27         | 26 | 25 | 24 |  |  |
|----------|----|----|----|------------|----|----|----|--|--|
| Reserved |    |    |    |            |    |    |    |  |  |
| R-0h     |    |    |    |            |    |    |    |  |  |
| 23       | 22 | 21 | 20 | 19         | 18 | 17 | 16 |  |  |
| Reserved |    |    |    |            |    |    |    |  |  |
| R-0h     |    |    |    |            |    |    |    |  |  |
| 15       | 14 | 13 | 12 | 11         | 10 | 9  | 8  |  |  |
|          |    |    |    | Reserved   |    |    |    |  |  |
|          |    |    |    | R-0h       |    |    |    |  |  |
| 7        | 6  | 5  | 4  | 3          | 2  | 1  | 0  |  |  |
|          |    |    |    | C2_RX_STAT |    |    |    |  |  |
| R-0h     |    |    |    |            |    |    |    |  |  |
|          |    |    |    |            |    |    |    |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-239. C2\_RX\_STAT Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                     |
|------|------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved   | R    | 0h    |                                                                                                                                                                                 |
| 7-0  | C2_RX_STAT | R    | 0h    | Core 2 Receive Masked Interrupt Status - Each bit in this register<br>corresponds to the bit in the Rx interrupt that is enabled and<br>generating an interrupt on C2_RX_PULSE. |

#### **14.5.9.27 C2\_TX\_STAT Register (offset = 68h) [reset = 0h]**

C2\_TX\_STAT is shown in Figure [14-223](#page-150-1) and described in Table [14-240](#page-150-2).

SUBSYSTEM CORE 2 TRANSMIT MASKED INTERRUPT STATUS REGISTER

### **Figure 14-223. C2\_TX\_STAT Register**

| 31 | 30         | 29 | 28 | 27       | 26 | 25 | 24 |  |  |
|----|------------|----|----|----------|----|----|----|--|--|
|    |            |    |    | Reserved |    |    |    |  |  |
|    |            |    |    | R-0h     |    |    |    |  |  |
| 23 | 22         | 21 | 20 | 19       | 18 | 17 | 16 |  |  |
|    | Reserved   |    |    |          |    |    |    |  |  |
|    |            |    |    | R-0h     |    |    |    |  |  |
| 15 | 14         | 13 | 12 | 11       | 10 | 9  | 8  |  |  |
|    |            |    |    | Reserved |    |    |    |  |  |
|    |            |    |    | R-0h     |    |    |    |  |  |
| 7  | 6          | 5  | 4  | 3        | 2  | 1  | 0  |  |  |
|    | C2_TX_STAT |    |    |          |    |    |    |  |  |
|    |            |    |    | R-0h     |    |    |    |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-240. C2\_TX\_STAT Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                      |
|------|------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved   | R    | 0h    |                                                                                                                                                                                  |
| 7-0  | C2_TX_STAT | R    | 0h    | Core 2 Transmit Masked Interrupt Status - Each bit in this register<br>corresponds to the bit in the Tx interrupt that is enabled and<br>generating an interrupt on C2_TX_PULSE. |

#### **14.5.9.28 C2\_MISC\_STAT Register (offset = 6Ch) [reset = 0h]**

C2\_MISC\_STAT is shown in Figure [14-224](#page-151-1) and described in Table [14-241.](#page-151-2)

SUBSYSTEM CORE 2 MISC MASKED INTERRUPT STATUS REGISTER

**Figure 14-224. C2\_MISC\_STAT Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-------------------------------+---------------------------+
|           Reserved            |       C2_MISC_STAT        |
|             R-0h              |           R-0h            |
+-------------------------------+---------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-241. C2\_MISC\_STAT Register Field Descriptions**

| Bit  | Field        | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                    |
|------|--------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-5 | Reserved     | R    | 0h    |                                                                                                                                                                                                                                                                                                                                                                |
| 4-0  | C2_MISC_STAT | R    | 0h    | Core 2 Misc Masked Interrupt Status - Each bit in this register<br>corresponds to the miscellaneous interrupt (evnt_pend, stat_pend,<br>host_pend, mdio_linkint, mdio_userint) that is enabled and<br>generating an interrupt on C2_MISC_PULSE.<br>Bit 4 = evnt_pend<br>Bit 3 = stat_pend<br>Bit 2 = host_pend<br>Bit 1 = mdio_linkint<br>Bit 0 = mdio_userint |

#### **14.5.9.29 C0\_RX\_IMAX Register (offset = 70h) [reset = 0h]**

C0\_RX\_IMAX is shown in Figure [14-225](#page-152-1) and described in Table [14-242](#page-152-2). SUBSYSTEM CORE 0 RECEIVE INTERRUPTS PER MILLISECOND

**Figure 14-225. C0\_RX\_IMAX Register**

| 31   | 30                     | 29     | 28 | 27       | 26 | 25 | 24 |  |
|------|------------------------|--------|----|----------|----|----|----|--|
|      |                        |        |    | Reserved |    |    |    |  |
|      |                        |        |    | R-0h     |    |    |    |  |
| 23   | 22                     | 21     | 20 | 19       | 18 | 17 | 16 |  |
|      | Reserved               |        |    |          |    |    |    |  |
|      | R-0h                   |        |    |          |    |    |    |  |
| 15   | 14                     | 13     | 12 | 11       | 10 | 9  | 8  |  |
|      |                        |        |    | Reserved |    |    |    |  |
|      |                        |        |    | R-0h     |    |    |    |  |
| 7    | 6                      | 5      | 4  | 3        | 2  | 1  | 0  |  |
|      | Reserved<br>C0_RX_IMAX |        |    |          |    |    |    |  |
| R-0h |                        | R/W-0h |    |          |    |    |    |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-242. C0\_RX\_IMAX Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                          |
|------|------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-6 | Reserved   | R    | 0h    |                                                                                                                                                                      |
| 5-0  | C0_RX_IMAX | R/W  | 0h    | Core 0 Receive Interrupts per Millisecond - The maximum number of<br>interrupts per millisecond generated on C0_RX_PULSE if pacing is<br>enabled for this interrupt. |

#### **14.5.9.30 C0\_TX\_IMAX Register (offset = 74h) [reset = 0h]**

C0\_TX\_IMAX is shown in Figure [14-226](#page-153-1) and described in Table [14-243](#page-153-2).

SUBSYSTEM CORE 0 TRANSMIT INTERRUPTS PER MILLISECOND

### **Figure 14-226. C0\_TX\_IMAX Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-------------------------------+---------------------------+
|           Reserved            |       C2_MISC_STAT        |
|             R-0h              |           R-0h            |
+-------------------------------+---------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-243. C0\_TX\_IMAX Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                           |
|------|------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-6 | Reserved   | R    | 0h    |                                                                                                                                                                       |
| 5-0  | C0_TX_IMAX | R/W  | 0h    | Core 0 Transmit Interrupts per Millisecond - The maximum number<br>of interrupts per millisecond generated on C0_TX_PULSE if pacing<br>is enabled for this interrupt. |

#### **14.5.9.31 C1\_RX\_IMAX Register (offset = 78h) [reset = 0h]**

C1\_RX\_IMAX is shown in Figure [14-227](#page-154-1) and described in Table [14-244](#page-154-2).

SUBSYSTEM CORE 1 RECEIVE INTERRUPTS PER MILLISECOND

**Figure 14-227. C1\_RX\_IMAX Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-----------+-----------------------------------------------+
| Reserved  |                 C1_RX_IMAX                    |
|   R-0h    |                   R/W-0h                      |
+-----------+-----------------------------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-244. C1\_RX\_IMAX Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                          |
|------|------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-6 | Reserved   | R    | 0h    |                                                                                                                                                                      |
| 5-0  | C1_RX_IMAX | R/W  | 0h    | Core 1 Receive Interrupts per Millisecond - The maximum number of<br>interrupts per millisecond generated on C1_RX_PULSE if pacing is<br>enabled for this interrupt. |

#### **14.5.9.32 C1\_TX\_IMAX Register (offset = 7Ch) [reset = 0h]**

C1\_TX\_IMAX is shown in Figure [14-228](#page-155-1) and described in Table [14-245](#page-155-2).

SUBSYSTEM CORE 1 TRANSMIT INTERRUPTS PER MILLISECOND

### **Figure 14-228. C1\_TX\_IMAX Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-----------+-----------------------------------------------+
| Reserved  |                  C1_TX_IMAX                   |
|   R-0h    |                   R/W-0h                      |
+-----------+-----------------------------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-245. C1\_TX\_IMAX Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                           |
|------|------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-6 | Reserved   | R    | 0h    |                                                                                                                                                                       |
| 5-0  | C1_TX_IMAX | R/W  | 0h    | Core 1 Transmit Interrupts per Millisecond - The maximum number<br>of interrupts per millisecond generated on C1_TX_PULSE if pacing<br>is enabled for this interrupt. |

#### **14.5.9.33 C2\_RX\_IMAX Register (offset = 80h) [reset = 0h]**

C2\_RX\_IMAX is shown in Figure [14-229](#page-156-1) and described in Table [14-246](#page-156-2).

SUBSYSTEM CORE 2 RECEIVE INTERRUPTS PER MILLISECOND

**Figure 14-229. C2\_RX\_IMAX Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-----------+-----------------------------------------------+
| Reserved  |                  C2_RX_IMAX                   |
|   R-0h    |                   R/W-0h                      |
+-----------+-----------------------------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-246. C2\_RX\_IMAX Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                          |
|------|------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-6 | Reserved   | R    | 0h    |                                                                                                                                                                      |
| 5-0  | C2_RX_IMAX | R/W  | 0h    | Core 2 Receive Interrupts per Millisecond - The maximum number of<br>interrupts per millisecond generated on C2_RX_PULSE if pacing is<br>enabled for this interrupt. |

#### **14.5.9.34 C2\_TX\_IMAX Register (offset = 84h) [reset = 0h]**

C2\_TX\_IMAX is shown in Figure [14-230](#page-157-1) and described in Table [14-247](#page-157-2).

SUBSYSTEM CORE 2 TRANSMIT INTERRUPTS PER MILLISECOND

**Figure 14-230. C2\_TX\_IMAX Register**

```
ROW 1  (BITS 31..24)
31      30      29      28      27      26      25      24
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 2  (BITS 23..16)
23      22      21      20      19      18      17      16
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 3  (BITS 15..8)
15      14      13      12      11      10       9       8
+-----------------------------------------------------------+
|                        Reserved                           |
|                           R-0h                            |
+-----------------------------------------------------------+

ROW 4  (BITS 7..0)
 7       6       5       4       3       2       1       0
+-----------+-----------------------------------------------+
| Reserved  |                  C2_TX_IMAX                   |
|   R-0h    |                   R/W-0h                      |
+-----------+-----------------------------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

### **Table 14-247. C2\_TX\_IMAX Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                           |
|------|------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-6 | Reserved   | R    | 0h    |                                                                                                                                                                       |
| 5-0  | C2_TX_IMAX | R/W  | 0h    | Core 2 Transmit Interrupts per Millisecond - The maximum number<br>of interrupts per millisecond generated on C2_TX_PULSE if pacing<br>is enabled for this interrupt. |

```
ROW 1  (BITS 31..16)
31                                  16
+-------------------------------------+
|               MODID                 |
|               R-0x7                 |
+-------------------------------------+

ROW 2  (BITS 15..0)
15                8 7                0
+------------------+------------------+
|      REVMAJ      |      REVMIN      |
|      R-0x1       |      R-0x4       |
+------------------+------------------+
```

#### **14.5.9.35 RGMII\_CTL Register (offset = 88h) [reset = 0h]**

RGMII\_CTL is shown in Figure [14-231](#page-158-1) and described in Table [14-248.](#page-158-2)

RGMII CONTROL SIGNAL REGISTER

NOTE: This register only has context in RGMII in-band mode. This register is not updated during forced mode of operation. Note that in-band mode is selected via MACCONTROL.EXT\_EN.

**Figure 14-231. RGMII\_CTL Register**

| 31                    | 30           | 29 | 28          | 27                    | 26           | 25 | 24          |  |  |
|-----------------------|--------------|----|-------------|-----------------------|--------------|----|-------------|--|--|
|                       | Reserved     |    |             |                       |              |    |             |  |  |
|                       |              |    |             | R-0h                  |              |    |             |  |  |
| 23                    | 22           | 21 | 20          | 19                    | 18           | 17 | 16          |  |  |
|                       |              |    |             | Reserved              |              |    |             |  |  |
|                       | R-0h         |    |             |                       |              |    |             |  |  |
| 15                    | 14           | 13 | 12          | 11                    | 10           | 9  | 8           |  |  |
|                       |              |    |             | Reserved              |              |    |             |  |  |
|                       |              |    |             | R-0h                  |              |    |             |  |  |
| 7                     | 6            | 5  | 4           | 3                     | 2            | 1  | 0           |  |  |
| RGMII2_FULL<br>DUPLEX | RGMII2_SPEED |    | RGMII2_LINK | RGMII1_FULL<br>DUPLEX | RGMII1_SPEED |    | RGMII1_LINK |  |  |
| R-0h                  | R-0h         |    | R-0h        | R-0h                  | R-0h         |    | R-0h        |  |  |

LEGEND: R/W = Read/Write; R = Read only; W1toCl = Write 1 to clear bit; *-n* = value after reset

**Table 14-248. RGMII\_CTL Register Field Descriptions**

| Bit  | Field             | Type | Reset | Description                                                                                                                                  |
|------|-------------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved          | R    | 0h    |                                                                                                                                              |
| 7    | RGMII2_FULLDUPLEX | R    | 0h    | RGMII 2 Fullduplex - This is the CPRGMII fullduplex output signal.<br>0 - Half-duplex mode<br>1 - Fullduplex mode                            |
| 6-5  | RGMII2_SPEED      | R    | 0h    | RGMII2 Speed - This is the CPRGMI speed output signal<br>00 - 10Mbps mode<br>01 - 100Mbps mode<br>10 - 1000Mbps (gig) mode<br>11 - reserved  |
| 4    | RGMII2_LINK       | R    | 0h    | RGMII2 Link Indicator - This is the CPRGMII link output signal<br>0 - RGMII2 link is down<br>1 - RGMII2 link is up                           |
| 3    | RGMII1_FULLDUPLEX | R    | 0h    | RGMII1 Fullduplex - This is the CPRGMII fullduplex output signal.<br>0 - Half-duplex mode<br>1 - Fullduplex mode                             |
| 2-1  | RGMII1_SPEED      | R    | 0h    | RGMII1 Speed - This is the CPRGMII speed output signal<br>00 - 10Mbps mode<br>01 - 100Mbps mode<br>10 - 1000Mbps (gig) mode<br>11 - reserved |
| 0    | RGMII1_LINK       | R    | 0h    | RGMII1 Link Indicator - This is the CPRGMII link output signal<br>0 - RGMII1 link is down                                                    |

### *14.5.10 MDIO Registers*

This section describes the memory-mapped registers for the Management Data Input/Output (MDIO).

Table [14-249](#page-159-0) lists the memory-mapped registers for the Management Data Input/Output (MDIO). See the device-specific data manual for the memory address of these registers.

**Table 14-249. MDIO Registers**

| Offset | Acronym            | Register Name                                                | Section               |
|--------|--------------------|--------------------------------------------------------------|-----------------------|
| 0      | MDIOVER            | MDIO Version Register                                        | Section<br>14.5.10.1  |
| 4h     | MDIOCONTROL        | MDIO Control Register                                        | Section<br>14.5.10.2  |
| 8h     | MDIOALIVE          | PHY Alive Status Register                                    | Section<br>14.5.10.3  |
| Ch     | MDIOLINK           | PHY Link Status Register                                     | Section<br>14.5.10.4  |
| 10h    | MDIOLINKINTRAW     | MDIO Link Status Change Interrupt Register                   | Section<br>14.5.10.5  |
| 14h    | MDIOLINKINTMASKED  | MDIO Link Status Change Interrupt Register (Masked Value)    | Section<br>14.5.10.6  |
| 20h    | MDIOUSERINTRAW     | MDIO User Command Complete Interrupt Register (Raw Value)    | Section<br>14.5.10.7  |
| 24h    | MDIOUSERINTMASKED  | MDIO User Command Complete Interrupt Register (Masked Value) | Section<br>14.5.10.8  |
| 28h    | MDIOUSERINTMASKSET | MDIO User Command Complete Interrupt Mask Set Register       | Section<br>14.5.10.9  |
| 2Ch    | MDIOUSERINTMASKCLR | MDIO User Interrupt Mask Clear Register                      | Section<br>14.5.10.10 |
| 80h    | MDIOUSERACCESS0    | MDIO User Access Register 0                                  | Section<br>14.5.10.11 |
| 84h    | MDIOUSERPHYSEL0    | MDIO User PHY Select Register 0                              | Section<br>14.5.10.12 |
| 88h    | MDIOUSERACCESS1    | MDIO User Access Register 1                                  | Section<br>14.5.10.13 |
| 8Ch    | MDIOUSERPHYSEL1    | MDIO User PHY Select Register 1                              | Section<br>14.5.10.14 |

#### **14.5.10.1 MDIOVER Register**

The MDIO version register (MDIOVER) is shown in Figure [14-232](#page-159-2) and described in Table [14-250](#page-159-3).

### **Figure 14-232. MDIO Version Register (MDIOVER)**

LEGEND: R = Read only; -*n* = value after reset

#### **Table 14-250. MDIOVER Register Field Descriptions**

| Bit   | Field  | Type | Reset | Description                                       |
|-------|--------|------|-------|---------------------------------------------------|
| 31-16 | MODID  | R    | 7     | Identifies type of peripheral.                    |
| 15-8  | REVMAJ | R    | 1     | Management interface module major revision value. |
| 7-0   | REVMIN | R    | 4     | Management interface module minor revision value. |

#### **14.5.10.2 MDIOCONTROL Register**

The MDIO control register (MDIOCONTROL) is shown in Figure [14-233](#page-160-1) and described in Table [14-251.](#page-160-2)

### **Figure 14-233. MDIO Control Register (MDIOCONTROL)**

| 31    | 30      | 29       | 28                   | 24 | 23       | 21 | 20       | 19      | 18       | 17         | 16       |
|-------|---------|----------|----------------------|----|----------|----|----------|---------|----------|------------|----------|
| IDLE  | ENABLE  | Reserved | HIGHEST_USER_CHANNEL |    | Reserved |    | PREAMBLE | FAULT   | FAULTENB | INTTESTENB | Reserved |
| R-0x1 | R/W-0x0 | R-0x0    | R-0x1                |    | R-0x0    |    | R/W-0x0  | RWC-0x0 | R/W-0x0  | R/W-0x0    | R-0x0    |
| 15    |         |          |                      |    |          |    |          |         |          |            | 0        |
|       | CLKDIV  |          |                      |    |          |    |          |         |          |            |          |

R/W-0x255

LEGEND: R/W = Read/Write; RWC = Read/Write/Clear; R = Read only; -*n* = value after reset

**Table 14-251. MDIOCONTROL Register Field Descriptions**

| Bit   | Field                    | Type | Reset | Description                                                                                                                                                                                                                                                    |
|-------|--------------------------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31    | IDLE                     | R    | 1     | MDIO state machine IDLE. Set to 1 when the state machine is in<br>the idle state.                                                                                                                                                                              |
|       |                          |      |       | 0 = State machine is not in idle state.<br>1 = State machine is in idle state.                                                                                                                                                                                 |
| 30    | ENABLE                   | R/W  | 0     | Enable control. If the MDIO state machine is active at the time it is<br>disabled, it will complete the current operation before halting and<br>setting the idle bit. If using byte access, the enable bit has to be<br>the last bit written in this register. |
|       |                          |      |       | 0 = Disables the MDIO state machine.                                                                                                                                                                                                                           |
|       |                          |      |       | 1 = Enable the MDIO state machine.                                                                                                                                                                                                                             |
| 29    | Reserved                 | R    | 0     | Reserved.                                                                                                                                                                                                                                                      |
| 28-24 | HIGHEST_USER_CHANNE<br>L | R    | 1     | Highest user channel. This field specifies the highest user access<br>channel that is available in the module and is currently set to 1.<br>This implies that the MDIOUSERACCESS1 register is the highest<br>available user access channel.                    |
| 23-21 | Reserved                 | R    | 0     | Reserved.                                                                                                                                                                                                                                                      |
| 20    | PREAMBLE                 | R/W  | 0     | Preamble disable.<br>0 = Standard MDIO preamble is used.<br>1 = Disables this device from sending MDIO frame preambles.                                                                                                                                        |
| 19    | FAULT                    | R/WC | 0     | Fault indicator. This bit is set to 1 if the MDIO pins fail to read<br>back what the device is driving onto them. This indicates a<br>physical layer fault and the module state machine is reset. Writing<br>a 1 to it clears this bit.                        |
|       |                          |      |       | 0 = No failure.                                                                                                                                                                                                                                                |
|       |                          |      |       | 1 = Physical layer fault; the MDIO state machine is reset.                                                                                                                                                                                                     |
| 18    | FAULTENB                 | R/W  | 0     | Fault detect enable. This bit has to be set to 1 to enable the<br>physical layer fault detection.                                                                                                                                                              |
|       |                          |      |       | 0 = Disables the physical layer fault detection.                                                                                                                                                                                                               |
|       |                          |      |       | 1 = Enables the physical layer fault detection.                                                                                                                                                                                                                |
| 17    | INTTESTENB               | R/W  | 0     | Interrupt test enable. This bit can be set to 1 to enable the host to<br>set the USERINT and LINKINT bits for test purposes.                                                                                                                                   |
|       |                          |      |       | 0 = Interrupt bits are not set.                                                                                                                                                                                                                                |
|       |                          |      |       | 1 = Enables the host to set the USERINT and LINKINT bits for<br>test purposes.                                                                                                                                                                                 |
| 16    | Reserved                 | R    | 0     | Reserved.                                                                                                                                                                                                                                                      |
| 15-0  | CLKDIV                   | R/W  | 255   | Clock divider. This field specifies the division ratio between CLK<br>and the frequency of MDIO_CLK. MDIO_CLK is disabled when<br>clkdiv is set to 0. MDIO_CLK frequency = clk frequency/(clkdiv+1).                                                           |

#### **14.5.10.3 MDIOALIVE Register**

The PHY acknowledge status register (MDIOALIVE) is shown in Figure [14-234](#page-161-2) and described in [Table](#page-161-3) 14- [252.](#page-161-3)

#### **Figure 14-234. PHY Acknowledge Status Register (MDIOALIVE)**

```
+-----------------------------------------------------------------------+
|                                 ALIVE                                 |
|                               R/WC-0x0                                |
+-----------------------------------------------------------------------+
```

LEGEND: RWC = Read/Write/Clear

**Table 14-252. MDIOALIVE Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
|------|-------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | ALIVE | R/WC | 0     | MDIO alive. Each of the 32 bits of this register is set if the most recent<br>access to the PHY with address corresponding to the register bit number<br>was acknowledged by the PHY, the bit is reset if the PHY fails to<br>acknowledge the access. Both the user and polling accesses to a PHY will<br>cause the corresponding alive bit to be updated. The alive bits are only<br>meant to be used to give an indication of the presence or not of a PHY with<br>the corresponding address. Writing a 1 to any bit will clear it, writing a 0 has<br>no effect. |

#### **14.5.10.4 MDIOLINK Register**

The PHY link status register (MDIOLINK) is shown in Figure [14-235](#page-161-4) and described in Table [14-253](#page-161-5).

**Figure 14-235. PHY Link Status Register (MDIOLINK)**

LEGEND: R = Read only; -*n* = value after reset

#### **Table 14-253. MDIOLINK Register Field Descriptions**

| Bit  | Field | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|------|-------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-0 | LINK  | R    | 0     | MDIO link state. This register is updated after a read of the Generic Status<br>Register of a PHY. The bit is set if the PHY with the corresponding address<br>has link and the PHY acknowledges the read transaction. The bit is reset if<br>the PHY indicates it does not have link or fails to acknowledge the read<br>transaction. Writes to the register have no effect. In addition, the status of<br>the two PHYs specified in the MDIOUSERPHYSELn registers can be<br>determined using the MLINK input pins. This is determined by the LINKSEL<br>bit in the MDIOUSERPHYSELn register. |

#### **14.5.10.5 MDIOLINKINTRAW Register**

The MDIO link status change interrupt register (MDIOLINKINTRAW) is shown in Figure [14-236](#page-162-2) and described in Table [14-254](#page-162-3).

**Figure 14-236. MDIO Link Status Change Interrupt Register (MDIOLINKINTRAW)**

```
ROW 1  (BITS 31..16)
31                                  16
+-------------------------------------+
|              Reserved               |
|               R-0x0                 |
+-------------------------------------+

ROW 2  (BITS 15..0)
15                        3  2   1   0
+--------------------------+-----------+
|         Reserved         | LINKINTRAW|
|          R-0x0           | RWC-0x0   |
+--------------------------+-----------+
```

LEGEND: RWC = Read/Write/Clear; R = Read only; -*n* = value after reset

**Table 14-254. MDIOLINKINTRAW Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|------|------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-2 | Reserved   | R    | 0     | Reserved.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 1-0  | LINKINTRAW | R/WC | 0     | MDIO link change event, raw value. When asserted 1, a bit indicates that there was<br>an MDIO link change event (i.e. change in the MDIOLINK register) corresponding to<br>the PHY address in the MDIOUSERPHYSELn register. LINKINTRAW[0] and<br>LINKINTRAW[1] correspond to MDIOUSERPHYSEL0 and MDIOUSERPHYSEL1,<br>respectively. Writing a 1 will clear the event and writing 0 has no effect. If the<br>INTTESTENB bit in the MDIOCONTROL register is set, the host may set the<br>LINKINTRAW bits to a 1. This mode may be used for test purposes. |

#### **14.5.10.6 MDIOLINKINTMASKED Register**

The MDIO link status change interrupt register (Masked Value) (MDIOLINKINTMASKED) is shown in Figure [14-237](#page-162-4) and described in Table [14-255](#page-162-5).

#### **Figure 14-237. MDIO Link Status Change Interrupt Register (Masked Value) (MDIOLINKINTMASKED)**

```
ROW 1  (BITS 31..16)
31                                  16
+-------------------------------------+
|              Reserved               |
|               R-0x0                 |
+-------------------------------------+

ROW 2  (BITS 15..0)
15                           3  2   1   0
+--------------------------+---------------+
|         Reserved         | LINKINTMASKED |
|          R-0x0           |   RWC-0x0     |
+--------------------------+---------------+
```

LEGEND: RWC = Read/Write/Clear; R = Read only; -*n* = value after reset

#### **Table 14-255. MDIOLINKINTMASKED Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|------|---------------|------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-2 | Reserved      | R    | 0     | Reserved.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 1-0  | LINKINTMASKED | R/WC | 0     | MDIO link change interrupt, masked value. When asserted 1, a bit indicates<br>that there was an MDIO link change event (i.e. change in the MDIO Link<br>register) corresponding to the PHY address in the MDIOUSERPHYSELn<br>register and the corresponding LINKINTENB bit was set.<br>LINKINTMASKED[0] and LINKINTMASKED[1] correspond to<br>MDIOUSERPHYSEL0 and MDIOUSERPHYSEL1, respectively. Writing a 1<br>will clear the interrupt and writing 0 has no effect. If the INTTESTENB bit in<br>the MDIOCONTROL register is set, the host may set the LINKINT bits to a<br>1. This mode may be used for test purposes. |

#### **14.5.10.7 MDIOUSERINTRAW Register**

The MDIO user command complete interrupt register (Raw Value) (MDIOUSERINTRAW) is shown in Figure [14-238](#page-163-2) and described in Table [14-256](#page-163-3).

**Figure 14-238. MDIO User Command Complete Interrupt Register (Raw Value) (MDIOUSERINTRAW)**

```
ROW 1  (BITS 31..16)
31                                  16
+-------------------------------------+
|              Reserved               |
|               R-0x0                 |
+-------------------------------------+

ROW 2  (BITS 15..0)
15                        3  2   1   0
+--------------------------+-----------+
|         Reserved         | USERINTRAW|
|          R-0x0           | RWC-0x0   |
+--------------------------+-----------+
```

LEGEND: RWC = Read/Write/Clear; R = Read only; -*n* = value after reset

**Table 14-256. MDIOUSERINTRAW Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|------|------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-2 | Reserved   | R    | 0     | Reserved.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 1-0  | USERINTRAW | R/WC | 0     | Raw value of MDIO user command complete event for the<br>MDIOUSERACCESS1 register through the MDIOUSERACCESS0 register,<br>respectively. When asserted 1, a bit indicates that the previously scheduled<br>PHY read or write command using that particular MDIOUSERACCESSn<br>register has completed. Writing a 1 will clear the event and writing 0 has no<br>effect. If the INTTESTENB bit in the MDIOCONTROL register is set, the host<br>may set the USERINTRAW bits to a 1. This mode may be used for test<br>purposes. |

#### **14.5.10.8 MDIOUSERINTMASKED Register**

The MDIO user command complete interrupt register (Masked Value) (MDIOUSERINTMASKED) is shown in Figure [14-239](#page-163-4) and described in Table [14-257](#page-163-5).

#### **Figure 14-239. MDIO User Command Complete Interrupt Register (Masked Value) (MDIOUSERINTMASKED)**

```
ROW 1  (BITS 31..16)
31                                  16
+-------------------------------------+
|              Reserved               |
|               R-0x0                 |
+-------------------------------------+

ROW 2  (BITS 15..0)
15                        3  2   1   0
+--------------------------+--------------+
|         Reserved         | USERINTMASKED|
|          R-0x0           |   RWC-0x0    |
+--------------------------+--------------+
```

LEGEND: RWC = Read/Write/Clear; R = Read only; -*n* = value after reset

#### **Table 14-257. MDIOUSERINTMASKED Register Field Descriptions**

| Bit  | Field         | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
|------|---------------|------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-2 | Reserved      | R    | 0     | Reserved.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 1-0  | USERINTMASKED | R/WC | 0     | Masked value of MDIO user command complete interrupt for the<br>MDIOUSERACCESS1 register through the MDIOUSERACCESS0 register,<br>respectively. When asserted 1, a bit indicates that the previously scheduled<br>PHY read or write command using that particular MDIOUSERACCESSn<br>register has completed and the corresponding USERINTMASKSET bit is set<br>to 1. Writing a 1 will clear the interrupt and writing 0 has no effect. If the<br>INTTESTENB bit in the MDIOCONTROL register is set, the host may set the<br>USERINTMASKED bits to a 1. This mode may be used for test purposes. |

#### **14.5.10.9 MDIOUSERINTMASKSET Register**

The MDIO user command complete interrupt mask set register (MDIOUSERINTMASKSET) is shown in Figure [14-240](#page-164-2) and described in Table [14-258](#page-164-3).

**Figure 14-240. MDIO User Command Complete Interrupt Mask Set Register (MDIOUSERINTMASKSET)**

```
ROW 1  (BITS 31..16)
31                                  16
+-------------------------------------+
|              Reserved               |
|               R-0x0                 |
+-------------------------------------+

ROW 2  (BITS 15..0)
15                        3  2   1   0
+--------------------------+------------------+
|         Reserved         | USERINTMASKSET   |
|          R-0x0           |     RWC-0x0      |
+--------------------------+------------------+
```

LEGEND: RWC = Read/Write/Clear; R = Read only; -*n* = value after reset

**Table 14-258. MDIOUSERINTMASKSET Register Field Descriptions**

| Bit  | Field          | Type | Reset | Description                                                                                                                                                                                                                                                                                                                                       |
|------|----------------|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-2 | Reserved       | R    | 0     | Reserved.                                                                                                                                                                                                                                                                                                                                         |
| 1-0  | USERINTMASKSET | R/WC | 0     | MDIO user interrupt mask set for USERINTMASKED, respectively. Writing a<br>bit to 1 will enable MDIO user command complete interrupts for that<br>particular MDIOUSERACCESSn register. MDIO user interrupt for a particular<br>MDIOUSERACCESSn register is disabled if the corresponding bit is 0.<br>Writing a 0 to this register has no effect. |

#### **14.5.10.10 MDIOUSERINTMASKCLR Register**

The MDIO user interrupt mask clear register (MDIOUSERINTMASKCLR) is shown in Figure [14-241](#page-164-4) and described in Table [14-259](#page-164-5).

#### **Figure 14-241. MDIO User Command Complete Interrupt Mask Clear Register (MDIOUSERINTMASKCLR)**

```
ROW 1  (BITS 31..16)
31                                  16
+-------------------------------------+
|              Reserved               |
|               R-0x0                 |
+-------------------------------------+

ROW 2  (BITS 15..0)
15                        3  2   1   0
+--------------------------+-------------------+
|         Reserved         | USERINTMASKCLEAR   |
|          R-0x0           |      RWC-0x0       |
+--------------------------+-------------------+
```

LEGEND: RWC = Read/Write/Clear; R = Read only; -*n* = value after reset

#### **Table 14-259. MDIOUSERINTMASKCLR Register Field Descriptions**

| Bit  | Field            | Type | Reset<br>Description |                                                                                                                                                                                                                                                              |
|------|------------------|------|----------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-2 | Reserved         | R    | 0                    | Reserved.                                                                                                                                                                                                                                                    |
| 1-0  | USERINTMASKCLEAR | R/WC | 0                    | MDIO user command complete interrupt mask clear for USERINTMASKED,<br>respectively. Writing a bit to 1 will disable further user command complete<br>interrupts for that particular MDIOUSERACCESSn register. Writing a 0 to<br>this register has no effect. |

#### **14.5.10.11 MDIOUSERACCESS0 Register**

The MDIO user access register 0 (MDIOUSERACCESS0) is shown in Figure [14-242](#page-165-1) and described in Table [14-260](#page-165-2).

**Figure 14-242. MDIO User Access Register 0 (MDIOUSERACCESS0)**

| 31      | 30      | 29      | 28       | 26 | 25 |         | 21 | 20      | 16 |
|---------|---------|---------|----------|----|----|---------|----|---------|----|
| GO      | WRITE   | ACK     | Reserved |    |    | REGADR  |    | PHYADR  |    |
| R/W/S-0 | R/W-0x0 | R/W-0x0 | R-0x0    |    |    | R/W-0x0 |    | R/W-0x0 |    |
| 15      |         |         |          |    |    |         |    |         | 0  |
|         |         |         |          |    |    | DATA    |    |         |    |

R/W-0x0 LEGEND: R/W = Read/Write; R = Read only; S = Status; -*n* = value after reset

**Table 14-260. MDIOUSERACCESS0 Register Field Descriptions**

| Bit   | Field    | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |  |
|-------|----------|-------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--|
| 31    | GO       | R/W/S | 0     | Go. Writing a 1 to this bit causes the MDIO state machine to perform an<br>MDIO access when it is convenient for it to do so, this is not an<br>instantaneous process. Writing a 0 to this bit has no effect. This bit is write<br>able only if the MDIO state machine is enabled. This bit will self clear when<br>the requested access has been completed. Any writes to the<br>MDIOUSERACCESS0 register are blocked when the GO bit is 1. If byte<br>access is being used, the GO bit should be written last. |  |
| 30    | WRITE    | R/W   | 0     | Write enable. Setting this bit to a 1 causes the MDIO transaction to be a<br>register write, otherwise it is a register read.                                                                                                                                                                                                                                                                                                                                                                                    |  |
| 29    | ACK      | R/W   | 0     | Acknowledge. This bit is set if the PHY acknowledged the read transaction.                                                                                                                                                                                                                                                                                                                                                                                                                                       |  |
| 28-26 | Reserved | R     | 0     | Reserved.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |  |
| 25-21 | REGADR   | R/W   | 0     | Register address. Specifies the PHY register to be accessed for this<br>transaction.                                                                                                                                                                                                                                                                                                                                                                                                                             |  |
| 20-16 | PHYADR   | R/W   | 0     | PHY address. Specifies the PHY to be accesses for this transaction.                                                                                                                                                                                                                                                                                                                                                                                                                                              |  |
| 15-0  | DATA     | R/W   | 0     | User data. The data value read from or to be written to the specified PHY<br>register.                                                                                                                                                                                                                                                                                                                                                                                                                           |  |

#### **14.5.10.12 MDIOUSERPHYSEL0 Register**

The MDIO user PHY select register 0 (MDIOUSERPHYSEL0) is shown in Figure [14-243](#page-166-1) and described in Table [14-261](#page-166-2).

**Figure 14-243. MDIO User PHY Select Register 0 (MDIOUSERPHYSEL0)**

```
ROW 1  (BITS 31..16)
31                                  16
+-------------------------------------+
|              Reserved               |
|               R-0x0                 |
+-------------------------------------+

ROW 2  (BITS 15..0)
|  Reserved   |LIN|LINK|Res|  PHYADDRMON  |
|   R-0x0     |KSE|INTE|erv|   R/W-0x0    |
|             |L  |NB  |ed |              |
|             |R/W|R/W |R-0|              |
|             |-0x|-0x0|x0 |              |
+-------------+---+---+---+---------------+
```

LEGEND: R/W = Read/Write; R = Read only; -*n* = value after reset

**Table 14-261. MDIOUSERPHYSEL0 Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                   |
|------|------------|------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved   | R    | 0     | Reserved.                                                                                                                                                                                     |
| 7    | LINKSEL    | R/W  | 0     | Link status determination select. Set to 1 to determine link status<br>using the MLINK pin. Default value is 0 which implies that the link<br>status is determined by the MDIO state machine. |
| 6    | LINKINTENB | R/W  | 0     | Link change interrupt enable. Set to 1 to enable link change<br>status interrupts for PHY address specified in PHYADDRMON.<br>Link change interrupts are disabled if this bit is set to 0.    |
|      |            |      |       | 0 = Link change interrupts are disabled.                                                                                                                                                      |
|      |            |      |       | 1 = Link change status interrupts for PHY address specified in<br>PHYADDRMON bits are enabled.                                                                                                |
| 5    | Reserved   | R    | 0     | Reserved.                                                                                                                                                                                     |
| 4-0  | PHYADDRMON | R/W  | 0     | PHY address whose link status is to be monitored.                                                                                                                                             |

#### **14.5.10.13 MDIOUSERACCESS1 Register**

The MDIO user access register 1 (MDIOUSERACCESS1) is shown in Figure [14-244](#page-167-1) and described in Table [14-262](#page-167-2).

**Figure 14-244. MDIO User Access Register 1 (MDIOUSERACCESS1)**

| 31      | 30      | 29      | 28       | 26 | 25      |  | 21 | 20      | 16 |
|---------|---------|---------|----------|----|---------|--|----|---------|----|
| GO      | WRITE   | ACK     | Reserved |    | REGADR  |  |    | PHYADR  |    |
| R/W/S-0 | R/W-0x0 | R/W-0x0 | R-0x0    |    | R/W-0x0 |  |    | R/W-0x0 |    |
| 15      |         |         |          |    |         |  |    |         | 0  |
|         | DATA    |         |          |    |         |  |    |         |    |

R/W-0x0

LEGEND: R/W = Read/Write; R = Read only; S = Status; -*n* = value after reset

**Table 14-262. MDIOUSERACCESS1 Register Field Descriptions**

| Bit   | Field    | Type  | Reset | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|-------|----------|-------|-------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31    | GO       | R/W/S | 0     | Writing a 1 to this bit causes the MDIO state machine to perform an MDIO<br>access when it is convenient for it to do so, this is not an instantaneous<br>process. Writing a 0 to this bit has no effect. This bit is write able only if the<br>MDIO state machine is enabled. This bit will self clear when the requested<br>access has been completed. Any writes to the MDIOUSERACCESS0<br>register are blocked when the GO bit is 1. If byte access is being used, the<br>GO bit should be written last. |
| 30    | WRITE    | R/W   | 0     | Write enable. Setting this bit to a 1 causes the MDIO transaction to be a<br>register write, otherwise it is a register read.                                                                                                                                                                                                                                                                                                                                                                                |
| 29    | ACK      | R/W   | 0     | Acknowledge. This bit is set if the PHY acknowledged the read transaction.                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 28-26 | Reserved | R     | 0     | Reserved.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 25-21 | REGADR   | R/W   | 0     | Register address; specifies the PHY register to be accessed for this<br>transaction.                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 20-16 | PHYADR   | R/W   | 0     | PHY address; specifies the PHY to be accesses for this transaction.                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| 15-0  | DATA     | R/W   | 0     | User data. The data value read from or to be written to the specified PHY<br>register.                                                                                                                                                                                                                                                                                                                                                                                                                       |

#### **14.5.10.14 MDIOUSERPHYSEL1 Register**

The MDIO user PHY select register 1 (MDIOUSERPHYSEL1) is shown in Figure [14-245](#page-168-1) and described in Table [14-263](#page-168-2).

**Figure 14-245. MDIO User PHY Select Register 1 (MDIOUSERPHYSEL1)**

```
ROW 1  (BITS 31..16)
31                                  16
+-------------------------------------+
|              Reserved               |
|               R-0x0                 |
+-------------------------------------+

ROW 2  (BITS 15..0)
15        8   7        6         5        4                       0
+-------------+--------+----------+--------+------------------------+
|  Reserved   | LINKSEL|LINKINTENB|Reserved|       PHYADDRMON       |
|   R-0x0     | R/W-0x0| R/W-0x0  | R-0x0  |        R/W-0x0         |
+-------------+--------+----------+--------+------------------------+
```

LEGEND: R/W = Read/Write; R = Read only; -*n* = value after reset

**Table 14-263. MDIOUSERPHYSEL1 Register Field Descriptions**

| Bit  | Field      | Type | Reset | Description                                                                                                                                                                                    |
|------|------------|------|-------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-8 | Reserved   | R    | 0     | Reserved                                                                                                                                                                                       |
| 7    | LINKSEL    | R/W  | 0     | Link status determination select. Set to 1 to determine link status<br>using the MLINK pin. Default value is 0 which implies that the link<br>status is determined by the MDIO state machine.  |
| 6    | LINKINTENB | R/W  | 0     | Link change interrupt enable. Set to 1 to enable link change<br>status interrupts for PHY address specified in PHYADDRMON.<br>Link change interrupts are disabled if this bit is cleared to 0. |
|      |            |      |       | 0 = Link change interrupts are disabled                                                                                                                                                        |
|      |            |      |       | 1 = Link change status interrupts for PHY address specified in<br>PHYADDRMON bits are enabled.                                                                                                 |
| 5    | Reserved   | R    | 0     | Reserved.                                                                                                                                                                                      |
| 4-0  | PHYADDRMON | R/W  | 0     | PHY address whose link status is to be monitored.                                                                                                                                              |
