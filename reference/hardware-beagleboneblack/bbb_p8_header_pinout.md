# BeagleBone Black P8 Header Pinout

## P8 Header Pin Configuration

| Pin | GPIO No. | Name | Mode 0 | Mode 1 | Mode 2 | Mode 3 | Mode 4 | Mode 5 | Mode 6 | Mode 7 | Physical Pin | Notes |
|-----|----------|------|--------|--------|--------|--------|--------|--------|--------|--------|--------------|-------|
| P8_01 | - | DGND | Ground | - | - | - | - | - | - | - | - | Ground |
| P8_02 | - | DGND | Ground | - | - | - | - | - | - | - | - | Ground |
| P8_03 | 38 | GPIO1_6 | gpmc_ad6 | - | - | - | - | - | mmc1_dat6 | gpio1[6] | R9 | Used on Board (emmc2) |
| P8_04 | 39 | GPIO1_7 | gpmc_ad7 | - | - | - | - | - | mmc1_dat7 | gpio1[7] | T9 | Used on Board (emmc2) |
| P8_05 | 34 | GPIO1_2 | gpmc_ad2 | - | - | - | - | - | mmc1_dat2 | gpio1[2] | R8 | Used on Board (emmc2) |
| P8_06 | 35 | GPIO1_3 | gpmc_ad3 | - | - | - | - | - | mmc1_dat3 | gpio1[3] | T8 | Used on Board (emmc2) |
| P8_07 | 66 | TIMER4 | gpmc_advn_ale | - | - | - | timer4 | - | - | gpio2[2] | R7 | - |
| P8_08 | 67 | TIMER7 | gpmc_oen_ren | - | - | - | timer7 | - | - | gpio2[3] | T7 | - |
| P8_09 | 69 | TIMER5 | gpmc_be0n_cle | - | - | - | timer5 | - | - | gpio2[5] | T6 | - |
| P8_10 | 68 | TIMER6 | gpmc_wen | - | - | - | timer6 | - | - | gpio2[4] | U6 | - |
| P8_11 | 45 | GPIO1_13 | gpmc_ad13 | lcd_data18 | mmc1_dat5 | mmc2_dat1 | eQEP2B_in | pr1_pru0_pru_r30_15 | - | gpio1[13] | R12 | - |
| P8_12 | 44 | GPIO1_12 | GPMC_AD12 | LCD_DATA19 | MMC1_DAT4 | MMC2_DAT0 | EQEP2A_IN | pr1_pru0_pru_r30_14 | - | gpio1[12] | T12 | - |
| P8_13 | 23 | EHRPWM2B | gpmc_ad9 | lcd_data22 | mmc1_dat1 | mmc2_dat5 | ehrpwm2B | - | - | gpio0[23] | T10 | - |
| P8_14 | 26 | GPIO0_26 | gpmc_ad10 | lcd_data21 | mmc1_dat2 | mmc2_dat6 | ehrpwm2_tripzone_in | - | - | gpio0[26] | T11 | - |
| P8_15 | 47 | GPIO1_15 | gpmc_ad15 | lcd_data16 | mmc1_dat7 | mmc2_dat3 | eQEP2_strobe | pr1_pru0_pru_r31_15 | - | gpio1[15] | U13 | - |
| P8_16 | 46 | GPIO1_14 | gpmc_ad14 | lcd_data17 | mmc1_dat6 | mmc2_dat2 | eQEP2_index | pr1_pru0_pru_r31_14 | - | gpio1[14] | V13 | - |
| P8_17 | 27 | GPIO0_27 | gpmc_ad11 | lcd_data20 | mmc1_dat3 | mmc2_dat7 | ehrpwm0_synco | - | - | gpio0[27] | U12 | - |
| P8_18 | 65 | GPIO2_1 | gpmc_clk_mux0 | lcd_memory_clk | gpmc_wait1 | mmc2_clk | mcasp0_fsr | - | - | gpio2[1] | V12 | - |
| P8_19 | 22 | EHRPWM2A | gpmc_ad8 | lcd_data23 | mmc1_dat0 | mmc2_dat4 | ehrpwm2A | - | - | gpio0[22] | U10 | - |
| P8_20 | 63 | GPIO1_31 | gpmc_csn2 | gpmc_be1n | mmc1_cmd | pr1_pru1_pru_r30_13 | pr1_pru1_pru_r31_13 | - | - | gpio1[31] | V9 | Used on Board (emmc2) |
| P8_21 | 62 | GPIO1_30 | gpmc_csn1 | gpmc_clk | mmc1_clk | pr1_pru1_pru_r30_12 | pr1_pru1_pru_r31_12 | - | - | gpio1[30] | U9 | Used on Board (emmc2) |
| P8_22 | 37 | GPIO1_5 | gpmc_ad5 | - | - | - | - | mmc1_dat5 | - | gpio1[5] | V8 | Used on Board (emmc2) |
| P8_23 | 36 | GPIO1_4 | gpmc_ad4 | - | - | - | - | mmc1_dat4 | - | gpio1[4] | U8 | Used on Board (emmc2) |
| P8_24 | 33 | GPIO1_1 | gpmc_ad1 | - | - | - | - | mmc1_dat1 | - | gpio1[1] | V7 | Used on Board (emmc2) |
| P8_25 | 32 | GPIO1_0 | gpmc_ad0 | - | - | - | - | mmc1_dat0 | - | gpio1[0] | U7 | Used on Board (emmc2) |
| P8_26 | 61 | GPIO1_29 | gpmc_csn0 | - | - | - | - | - | - | gpio1[29] | V6 | - |
| P8_27 | 86 | GPIO2_22 | lcd_vsync | gpmc_a8 | pr1_pru1_pru_r30_8 | pr1_pru1_pru_r31_8 | - | - | - | gpio2[22] | U5 | Allocated (HDMI) |
| P8_28 | 88 | GPIO2_24 | lcd_pclk | gpmc_a10 | pr1_pru1_pru_r30_10 | pr1_pru1_pru_r31_10 | - | - | - | gpio2[24] | V5 | Allocated (HDMI) |
| P8_29 | 87 | GPIO2_23 | lcd_hsync | gpmc_a9 | pr1_pru1_pru_r30_9 | pr1_pru1_pru_r31_9 | - | - | - | gpio2[23] | R5 | Allocated (HDMI) |
| P8_30 | 89 | GPIO2_25 | lcd_ac_bias_en | gpmc_a11 | - | - | - | - | - | gpio2[25] | R6 | Allocated (HDMI) |
| P8_31 | 10 | UART5_CTSN | lcd_data14 | gpmc_a18 | eQEP1_index | mcasp0_axr1 | uart5_rxd | uart5_ctsn | - | gpio0[10] | V4 | Allocated (HDMI) |
| P8_32 | 11 | UART5_RTSN | lcd_data15 | gpmc_a19 | eQEP1_strobe | mcasp0_ahclkx | mcasp0_axr3 | uart5_rtsn | - | gpio0[11] | T5 | Allocated (HDMI) |
| P8_33 | 9 | UART4_RTSN | lcd_data13 | gpmc_a17 | eQEP1B_in | mcasp0_fsr | mcasp0_axr3 | uart4_rtsn | - | gpio0[9] | V3 | Allocated (HDMI) |
| P8_34 | 81 | UART3_RTSN | lcd_data11 | gpmc_a15 | ehrpwm1B | mcasp0_ahclkr | mcasp0_axr2 | uart3_rtsn | - | gpio2[17] | U4 | Allocated (HDMI) |
| P8_35 | 8 | UART4_CTSN | lcd_data12 | gpmc_a16 | eQEP1A_in | mcasp0_aclkr | mcasp0_axr2 | uart4_ctsn | - | gpio0[8] | V2 | Allocated (HDMI) |
| P8_36 | 80 | UART3_CTSN | lcd_data10 | gpmc_a14 | ehrpwm1A | mcasp0_axr0 | uart3_ctsn | - | - | gpio2[16] | U3 | Allocated (HDMI) |
| P8_37 | 78 | UART5_TXD | lcd_data8 | gpmc_a12 | ehrpwm1_tripzone_in | mcasp0_aclkx | uart5_txd | uart2_ctsn | - | gpio2[14] | U1 | Allocated (HDMI) |
| P8_38 | 79 | UART5_RXD | lcd_data9 | gpmc_a13 | ehrpwm0_synco | mcasp0_fsx | uart5_rxd | uart2_rtsn | - | gpio2[15] | U2 | Allocated (HDMI) |
| P8_39 | 76 | GPIO2_12 | lcd_data6 | gpmc_a6 | eQEP2_index | pr1_pru1_pru_r30_6 | pr1_pru1_pru_r31_6 | - | - | gpio2[12] | T3 | Allocated (HDMI) |
| P8_40 | 77 | GPIO2_13 | lcd_data7 | gpmc_a7 | eQEP2_strobe | pr1_edio_data_out7 | pr1_pru1_pru_r30_7 | pr1_pru1_pru_r31_7 | - | gpio2[13] | T4 | Allocated (HDMI) |
| P8_41 | 74 | GPIO2_10 | lcd_data4 | gpmc_a4 | eQEP2A_in | pr1_pru1_pru_r30_4 | pr1_pru1_pru_r31_4 | - | - | gpio2[10] | T1 | Allocated (HDMI) |
| P8_42 | 75 | GPIO2_11 | lcd_data5 | gpmc_a5 | eQEP2B_in | pr1_pru1_pru_r30_5 | pr1_pru1_pru_r31_5 | - | - | gpio2[11] | T2 | Allocated (HDMI) |
| P8_43 | 72 | GPIO2_8 | lcd_data2 | gpmc_a2 | ehrpwm2_tripzone_in | pr1_pru1_pru_r30_2 | pr1_pru1_pru_r31_2 | - | - | gpio2[8] | R3 | Allocated (HDMI) |
| P8_44 | 73 | GPIO2_9 | lcd_data3 | gpmc_a3 | ehrpwm0_synco | pr1_pru1_pru_r30_3 | pr1_pru1_pru_r31_3 | - | - | gpio2[9] | R4 | Allocated (HDMI) |
| P8_45 | 70 | GPIO2_6 | lcd_data0 | gpmc_a0 | ehrpwm2A | pr1_pru1_pru_r30_0 | pr1_pru1_pru_r31_0 | - | - | gpio2[6] | R1 | Allocated (HDMI) |
| P8_46 | 71 | GPIO2_7 | lcd_data1 | gpmc_a1 | ehrpwm2B | pr1_pru1_pru_r30_1 | pr1_pru1_pru_r31_1 | - | - | gpio2[7] | R2 | Allocated (HDMI) |

## User LEDs

| LED | Pin | GPIO No. | Name | Physical Pin |
|-----|-----|----------|------|--------------|
| USR0 | 21 | 53 | GPIO1_21 | - |
| USR1 | 22 | 86 | GPIO1_22 | - |
| USR2 | 23 | 87 | GPIO1_23 | - |
| USR3 | 24 | 88 | GPIO1_24 | - |

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

## Register Information

- **Base Address**: 0x44e10800
- **Offset**: Pin-specific offset from base address
- **Mode 7**: GPIO mode for all pins

## Notes

- Many pins on P8 header are allocated to HDMI (nxp_hdmi_bonelt_pins group)
- Several pins are used on board for eMMC functionality (pinmux_emmc2_pins group)
- Ground pins: P8_01, P8_02
- Physical pin locations follow standard BeagleBone Black layout

---

*Reference: www.derekmolloy.ie*
