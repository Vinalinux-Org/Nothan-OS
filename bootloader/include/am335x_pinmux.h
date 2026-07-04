#ifndef AM335X_PINMUX_H
#define AM335X_PINMUX_H

#ifndef CONTROL_MODULE_BASE
#define CONTROL_MODULE_BASE     0x44E10000
#endif

/* GPIO1 — USR LEDs on BBB (pins 21-24) */
#define GPIO1_BASE              0x4804C000
#define GPIO1_OE                (GPIO1_BASE + 0x134)
#define GPIO1_SETDATAOUT        (GPIO1_BASE + 0x194)
#define GPIO1_CLEARDATAOUT      (GPIO1_BASE + 0x190)
#define GPIO1_USR_LEDS          (0xF << 21)	/* USR0-USR3 = GPIO1[21:24] */

/* UART0 */
#define CONF_UART0_RXD      (CONTROL_MODULE_BASE + 0x970)
#define CONF_UART0_TXD      (CONTROL_MODULE_BASE + 0x974)

/* MMC0 */
#define CONF_MMC0_DAT3      (CONTROL_MODULE_BASE + 0x8F0)
#define CONF_MMC0_DAT2      (CONTROL_MODULE_BASE + 0x8F4)
#define CONF_MMC0_DAT1      (CONTROL_MODULE_BASE + 0x8F8)
#define CONF_MMC0_DAT0      (CONTROL_MODULE_BASE + 0x8FC)
#define CONF_MMC0_CLK       (CONTROL_MODULE_BASE + 0x900)
#define CONF_MMC0_CMD       (CONTROL_MODULE_BASE + 0x904)

/* Pin configuration bits */
#define PIN_MODE_0          0
#define PIN_MODE_1          1
#define PIN_MODE_2          2
#define PIN_MODE_3          3

#define PIN_INPUT_EN        (1 << 5)
#define PIN_PULLUP_EN       (1 << 4)
#define PIN_RXACTIVE        (1 << 5)

#endif /* AM335X_PINMUX_H */
