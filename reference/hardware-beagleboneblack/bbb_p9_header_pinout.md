# BeagleBone Black P9 Header Pinout

## P9 Header Pin Configuration

| Pin | GPIO No. | Name | Mode 0 | Mode 1 | Mode 2 | Mode 3 | Mode 4 | Mode 5 | Mode 6 | Mode 7 | Physical Pin | Notes |
|-----|----------|------|--------|--------|--------|--------|--------|--------|--------|--------|--------------|-------|
| P9_01 | - | GND | Ground | - | - | - | - | - | - | - | - | Ground |
| P9_02 | - | GND | Ground | - | - | - | - | - | - | - | - | Ground |
| P9_03 | - | DC_3.3V | - | - | - | - | - | - | - | - | - | 250mA Max Current |
| P9_04 | - | DC_3.3V | - | - | - | - | - | - | - | - | - | 250mA Max Current |
| P9_05 | - | VDD_5V | - | - | - | - | - | - | - | - | - | 1A Max (DC jack only) |
| P9_06 | - | VDD_5V | - | - | - | - | - | - | - | - | - | 1A Max (DC jack only) |
| P9_07 | - | SYS_5V | - | - | - | - | - | - | - | - | - | 250mA Max Current |
| P9_08 | - | SYS_5V | - | - | - | - | - | - | - | - | - | 250mA Max Current |
| P9_09 | - | PWR_BUT | - | - | - | - | - | - | - | - | A10 | 5V Level (TPS65217C) |
| P9_10 | - | SYS_RESETn | RESET_OUT | - | - | - | - | - | - | - | A10 | - |
| P9_11 | 30 | UART4_RXD | gpmc_wait0 | mii2_crs | gpmc_csn4 | rmii2_crs_dv | mmc1_sdcd | uart4_rxd_mux2 | - | gpio0[30] | T17 | GPIO 4-6mA output, 8mA input |
| P9_12 | 60 | GPIO1_28 | gpmc_be1n | mii2_col | gpmc_csn6 | mmc2_dat3 | gpmc_dir | mcasp0_aclkr_mux3 | - | gpio1[28] | U18 | - |
| P9_13 | 31 | UART4_TXD | gpmc_wpn | mii2_rxerr | gpmc_csn5 | rmii2_rxerr | mmc2_sdcd | uart4_txd_mux2 | - | gpio0[31] | U17 | - |
| P9_14 | 50 | EHRPWM1A | gpmc_a2 | mii2_txd3 | rgmii2_td3 | mmc2_dat1 | gpmc_a18 | ehrpwm1A_mux1 | - | gpio1[18] | U14 | - |
| P9_15 | 48 | GPIO1_16 | gpmc_a0 | gmii2_txen | rmii2_tctl | mii2_txen | gpmc_a16 | ehrpwm1_tripzone_input | - | gpio1[16] | R13 | - |
| P9_16 | 51 | EHRPWM1B | gpmc_a3 | mii2_txd2 | rgmii2_td2 | mmc2_dat2 | gpmc_a19 | ehrpwm1B_mux1 | - | gpio1[19] | T14 | - |
| P9_17 | 5 | I2C1_SCL | spi0_cs0 | mmc2_sdwp | I2C1_SCL | ehrpwm0_synci | pr1_uart0_txd | - | - | gpio0[5] | A16 | - |
| P9_18 | 4 | I2C1_SDA | spi0_d1 | mmc1_sdwp | I2C1_SDA | ehrpwm0_tripzone | pr1_uart0_rxd | - | - | gpio0[4] | B16 | - |
| P9_19 | 13 | I2C2_SCL | uart1_rtsn | timer5 | dcan0_rx | I2C2_SCL | spi1_cs1 | pr1_uart0_rts_n | - | gpio0[13] | D17 | Allocated (i2c2_pins) |
| P9_20 | 12 | I2C2_SDA | uart1_ctsn | timer6 | dcan0_tx | I2C2_SDA | spi1_cs0 | pr1_uart0_cts_n | - | gpio0[12] | D18 | Allocated (i2c2_pins) |
| P9_21 | 3 | UART2_TXD | spi0_d0 | uart2_txd | I2C2_SCL | ehrpwm0B | pr1_uart0_rts_n | EMU3_mux1 | - | gpio0[3] | B17 | - |
| P9_22 | 2 | UART2_RXD | spi0_sclk | uart2_rxd | I2C2_SDA | ehrpwm0A | pr1_uart0_cts_n | EMU2_mux1 | - | gpio0[2] | A17 | - |
| P9_23 | 49 | GPIO1_17 | gpmc_a1 | gmii2_rxdv | rgmii2_rxdv | mmc2_dat0 | gpmc_a17 | ehrpwm0_synco | - | gpio1[17] | V14 | - |
| P9_24 | 15 | UART1_TXD | uart1_txd | mmc2_sdwp | dcan1_rx | I2C1_SCL | pr1_uart0_txd | pr1_pru0_pru_r31_16 | - | gpio0[15] | D15 | - |
| P9_25 | 117 | GPIO3_21 | mcasp0_ahclkx | eQEP0_strobe | mcasp0_axr3 | mcasp1_axr1 | EMU4_mux2 | pr1_pru0_pru_r30_7 | pr1_pru0_pru_r31_7 | gpio3[21] | A14 | Allocated (mcasp0_pins) |
| P9_26 | 14 | UART1_RXD | uart1_rxd | mmc1_sdwp | dcan1_tx | I2C1_SDA | pr1_uart0_rxd | pr1_pru1_pru_r31_16 | - | gpio0[14] | D16 | - |
| P9_27 | 115 | GPIO3_19 | mcasp0_fsr | eQEP0B_in | mcasp0_axr3 | mcasp1_fsx | EMU2_mux2 | pr1_pru0_pru_r30_5 | pr1_pru0_pru_r31_5 | gpio3[19] | C13 | - |
| P9_28 | 113 | SPI1_CS0 | mcasp0_ahclkr | ehrpwm0_synci | mcasp0_axr2 | spi1_cs0 | eCAP2_in_PWM2_out | pr1_pru0_pru_r30_3 | pr1_pru0_pru_r31_3 | gpio3[17] | C12 | Allocated (mcasp0_pins) |
| P9_29 | 111 | SPI1_D0 | mcasp0_fsx | ehrpwm0B | spi1_d0 | mmc1_sdcd_mux1 | pr1_pru0_pru_r30_1 | pr1_pru0_pru_r31_1 | - | gpio3[15] | B13 | Allocated (mcasp0_pins) |
| P9_30 | 112 | SPI1_D1 | mcasp0_axr0 | ehrpwm0_tripzone | spi1_d1 | mmc2_sdcd_mux1 | pr1_pru0_pru_r30_2 | pr1_pru0_pru_r31_2 | - | gpio3[16] | D12 | Allocated (mcasp0_pins) |
| P9_31 | 110 | SPI1_SCLK | mcasp0_aclkx | ehrpwm0A | spi1_sclk | mmc0_sdcd_mux1 | pr1_pru0_pru_r30_0 | pr1_pru0_pru_r31_0 | - | gpio3[14] | A13 | Allocated (mcasp0_pins) |
| P9_32 | - | VADC | - | - | - | - | - | - | - | - | - | ADC Voltage Ref (1.8V) |
| P9_33 | - | AIN4 | - | - | - | - | - | - | - | - | C8 | 1.8V tolerant |
| P9_34 | - | AGND | - | - | - | - | - | - | - | - | - | ADC Ground |
| P9_35 | - | AIN6 | - | - | - | - | - | - | - | - | A8 | 1.8V tolerant |
| P9_36 | - | AIN5 | - | - | - | - | - | - | - | - | B8 | 1.8V tolerant |
| P9_37 | - | AIN2 | - | - | - | - | - | - | - | - | B7 | 1.8V tolerant |
| P9_38 | - | AIN3 | - | - | - | - | - | - | - | - | A7 | 1.8V tolerant |
| P9_39 | - | AIN0 | - | - | - | - | - | - | - | - | B6 | 1.8V tolerant |
| P9_40 | - | AIN1 | - | - | - | - | - | - | - | - | C7 | 1.8V tolerant |
| P9_41A | 20 | CLKOUT2 | eCAP0_in_PWM0_out | xdma_event_intr1 | tclkin | clkout2 | timer7_mux1 | pr1_pru0_pru_r31_16 | EMU3_mux0 | gpio0[20] | D14 | Connected to P21 of P11 |
| P9_41B | 116 | GPIO3_20 | mcasp0_axr1 | eQEP0_index | Mcasp1_axr0 | emu3 | pr1_pru0_pru_r30_6 | pr1_pru0_pru_r31_6 | - | gpio3[20] | D13 | Connected to P21 of P11 |
| P9_42A | 7 | GPIO0_7 | eCAP0_in_PWM0_out | uart3_txd | spi1_cs1 | pr1_ecap0_ecap_capin_apwm_o | spi1_sclk | mmc0_sdwp | xdma_event_intr2 | gpio0[7] | C18 | Connected to P22 of P11 |
| P9_42B | 114 | GPIO3_18 | Mcasp0_aclkr | eQEP0A_in | Mcaspo_axr2 | Mcasp1_aclkx | pr1_pru0_pru_r30_4 | pr1_pru0_pru_r31_4 | - | gpio3[18] | B12 | Allocated (mcasp0_pins) |
| P9_43 | - | GND | Ground | - | - | - | - | - | - | - | - | See SRM Page 50 |
| P9_44 | - | GND | Ground | - | - | - | - | - | - | - | - | Ground |
| P9_45 | - | GND | Ground | - | - | - | - | - | - | - | - | Ground |
| P9_46 | - | GND | Ground | - | - | - | - | - | - | - | - | Ground |

## Power Supply Pins

| Pin | Name | Specification | Notes |
|-----|------|---------------|-------|
| P9_01, P9_02 | GND | Ground | Digital ground |
| P9_03, P9_04 | DC_3.3V | 3.3V, 250mA max | Digital 3.3V supply |
| P9_05, P9_06 | VDD_5V | 5V, 1A max | Only available with DC jack power |
| P9_07, P9_08 | SYS_5V | 5V, 250mA max | System 5V supply |
| P9_09 | PWR_BUT | Power button | 5V level, pulled up by TPS65217C |
| P9_10 | SYS_RESETn | System reset | Active low reset output |
| P9_32 | VADC | 1.8V | ADC voltage reference |
| P9_34 | AGND | Ground | Analog ground for ADC |
| P9_43-P9_46 | GND | Ground | Additional ground pins |

## Analog Input Pins (ADC)

| Pin | Name | Physical Pin | Notes |
|-----|------|--------------|-------|
| P9_33 | AIN4 | C8 | 1.8V tolerant |
| P9_35 | AIN6 | A8 | 1.8V tolerant |
| P9_36 | AIN5 | B8 | 1.8V tolerant |
| P9_37 | AIN2 | B7 | 1.8V tolerant |
| P9_38 | AIN3 | A7 | 1.8V tolerant |
| P9_39 | AIN0 | B6 | 1.8V tolerant |
| P9_40 | AIN1 | C7 | 1.8V tolerant |

## Communication Interfaces

### UART Pins
| Pin | Name | Function |
|-----|------|----------|
| P9_11 | UART4_RXD | UART4 Receive |
| P9_13 | UART4_TXD | UART4 Transmit |
| P9_21 | UART2_TXD | UART2 Transmit |
| P9_22 | UART2_RXD | UART2 Receive |
| P9_24 | UART1_TXD | UART1 Transmit |
| P9_26 | UART1_RXD | UART1 Receive |

### I2C Pins
| Pin | Name | Function | Notes |
|-----|------|----------|-------|
| P9_17 | I2C1_SCL | I2C1 Clock | - |
| P9_18 | I2C1_SDA | I2C1 Data | - |
| P9_19 | I2C2_SCL | I2C2 Clock | Allocated (i2c2_pins) |
| P9_20 | I2C2_SDA | I2C2 Data | Allocated (i2c2_pins) |

### SPI Pins
| Pin | Name | Function | Notes |
|-----|------|----------|-------|
| P9_28 | SPI1_CS0 | SPI1 Chip Select | Allocated (mcasp0_pins) |
| P9_29 | SPI1_D0 | SPI1 Data Out | Allocated (mcasp0_pins) |
| P9_30 | SPI1_D1 | SPI1 Data In | Allocated (mcasp0_pins) |
| P9_31 | SPI1_SCLK | SPI1 Clock | Allocated (mcasp0_pins) |

### PWM Pins
| Pin | Name | Function |
|-----|------|----------|
| P9_14 | EHRPWM1A | Enhanced PWM 1A |
| P9_16 | EHRPWM1B | Enhanced PWM 1B |

## GPIO Settings Configuration

### Pin Configuration Register Format (Mode 7)

| Bit | Function | Values |
|-----|----------|--------|
| 6 | Slew Control | 0 = Fast, 1 = Slow |
| 5 | Receiver Active | 0 = Disable, 1 = Enable |
| 4 | Pullup/Pulldown Enable | 0 = Pulldown select, 1 = Pullup select |
| 3 | Pullup/down | 0 = Enabled, 1 = Disabled |
| 2,1,0 | Mux Mode | 000 = Mode 0 to 111 = Mode 7 |

### Common GPIO Configuration Values

| Configuration | Value | Description |
|---------------|-------|-------------|
| OUTPUT GPIO | 0x07 | Output, pulldown |
| OUTPUT GPIO | 0x17 | Output, pullup |
| OUTPUT GPIO | 0x?f | Output, no pullup/down |
| INPUT GPIO | 0x27 | Input, pulldown |
| INPUT GPIO | 0x37 | Input, pullup |
| INPUT GPIO | 0x?f | Input, no pullup/down |

## Special Pin Notes

- **GPIO Current Limits**: GPIO pins are limited to 4-6mA output current and approximately 8mA input current
- **Dual Function Pins**: P9_41 and P9_42 have A/B variants that are both connected to the same physical pins (P21 and P22 of P11 connector)
- **McASP0 Allocation**: Many pins (P9_25, P9_28-P9_31, P9_42B) are allocated to the McASP0 audio interface
- **ADC Pins**: All analog input pins are 1.8V tolerant - do not exceed this voltage
- **Power Limitations**: 
  - VDD_5V pins only available when powered by DC jack
  - Various current limitations on power supply pins

## Register Information

- **Base Address**: 0x44e10800
- **Offset**: Pin-specific offset from base address  
- **Mode 7**: GPIO mode for all pins

---

*Reference: www.derekmolloy.ie*  
*Special thanks to Frank for PRU work*  
*Contact: derek@derekmolloy.ie for corrections*
