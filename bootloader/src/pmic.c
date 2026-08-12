/*
 * bootloader/src/pmic.c - TPS65217 rail setup for the MPU operating point
 *
 * The MPU DPLL and VDD_MPU have to move together.  Setting the PLL to 1 GHz
 * while the rail sits at its 1.100 V power-on default does not produce an
 * error — it produces a CPU that computes the wrong answer now and then:
 * single-bit corruption, data aborts at unrelated PCs, one boot working and
 * the next dying on the same image.  That failure mode is invisible to a log,
 * because the place that crashes is the victim, not the cause.
 *
 * Required rail per operating point (matches what u-boot programs on this
 * board, board/ti/am335x/board.c scale_vcores_bone()):
 *
 *     600 MHz  (OPP100)  ->  1.100 V   <- power-on default
 *    1000 MHz  (OPP NT)  ->  1.325 V
 *
 * So: raise the rail, read it back, and only report success if the PMIC
 * actually took the value.  The caller raises the DPLL only on success, which
 * makes "1 GHz on the wrong voltage" unreachable by construction rather than
 * by remembering to check.
 *
 * Register numbering and the write-protection dance come from the TPS65217
 * programming model; the chip sits at I2C address 0x24 on I2C0.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "am335x.h"
#include "am335x_i2c.h"
#include "boot.h"

#define TPS65217_ADDR		0x24

#define TPS_CHIPID		0x00
#define TPS_STATUS		0x0A
#define TPS_PASSWORD		0x0B
#define TPS_DEFDCDC2		0x0F	/* VDD_MPU setpoint */
#define TPS_DEFSLEW		0x11	/* carries the GO bit */

#define TPS_PASSWORD_UNLOCK	0x7D
#define TPS_DCDC_GO		0x80

#define TPS_VOLT_SEL_1100MV	0x08
#define TPS_VOLT_SEL_1325MV	0x11

/* STATUS bit: power came in through the DC barrel jack rather than USB. */
#define TPS_PWR_SRC_AC		(1 << 3)
#define TPS_PWR_SRC_USB		(1 << 2)

/*
 * Protected registers ignore a plain write.  Each one has to be preceded by
 * writing (register ^ 0x7D) to the PASSWORD register, and level-2 protected
 * registers — DEFDCDC2 among them — need the whole pair done twice.
 */
static int tps_write_protected(uint8_t reg, uint8_t val)
{
	uint8_t pw[2] = { TPS_PASSWORD, (uint8_t)(reg ^ TPS_PASSWORD_UNLOCK) };
	uint8_t wr[2] = { reg, val };
	int i;

	for (i = 0; i < 2; i++) {
		if (i2c0_write(TPS65217_ADDR, pw, 2))
			return -1;
		if (i2c0_write(TPS65217_ADDR, wr, 2))
			return -1;
	}

	return 0;
}

/*
 * Read-modify-write a protected register, so bits nobody asked about survive.
 * DEFSLEW in particular carries the DC-DC slew rate configuration alongside
 * the GO bit; writing the whole byte would silently reprogram the ramp.
 */
static int tps_set_bits(uint8_t reg, uint8_t mask, uint8_t val)
{
	uint8_t cur;

	if (i2c0_read_reg(TPS65217_ADDR, reg, &cur))
		return -1;

	cur = (uint8_t)((cur & ~mask) | (val & mask));
	return tps_write_protected(reg, cur);
}

/**
 * pmic_set_mpu_1v325() - raise VDD_MPU to the 1 GHz operating point
 *
 * Return: 0 if the PMIC read back the requested voltage, negative otherwise.
 * On any failure the rail is left wherever it was, which is the safe default.
 */
int pmic_set_mpu_1v325(void)
{
	uint8_t val;
	int i;

	i2c0_init();

	if (i2c0_read_reg(TPS65217_ADDR, TPS_CHIPID, &val)) {
		uart_puts("[PMIC] not responding on I2C0 — staying at 600 MHz\r\n");
		return -1;
	}
	uart_puts("[PMIC] TPS65217 chipid=");
	uart_print_hex(val);

	if (!i2c0_read_reg(TPS65217_ADDR, TPS_STATUS, &val)) {
		uart_puts(" power=");
		if (val & TPS_PWR_SRC_AC)
			uart_puts("AC");
		else if (val & TPS_PWR_SRC_USB)
			uart_puts("USB");
		else
			uart_puts("battery/unknown");
	}
	uart_puts("\r\n");

	if (tps_write_protected(TPS_DEFDCDC2, TPS_VOLT_SEL_1325MV)) {
		uart_puts("[PMIC] DEFDCDC2 write failed — staying at 600 MHz\r\n");
		return -1;
	}

	if (i2c0_read_reg(TPS65217_ADDR, TPS_DEFDCDC2, &val) ||
	    (val & 0x3F) != TPS_VOLT_SEL_1325MV) {
		uart_puts("[PMIC] DEFDCDC2 did not take the setpoint — staying at 600 MHz\r\n");
		return -1;
	}

	/*
	 * Writing DEFDCDC2 only records a setpoint; the rail does not move
	 * until the GO bit is set.  Reading DEFDCDC2 back therefore proves
	 * nothing about the actual voltage — it proves the request was stored,
	 * which is exactly the kind of check that passes while the hardware
	 * stays where it was.
	 */
	if (tps_set_bits(TPS_DEFSLEW, TPS_DCDC_GO, TPS_DCDC_GO)) {
		uart_puts("[PMIC] GO write failed — staying at 600 MHz\r\n");
		return -1;
	}

	/*
	 * The PMIC clears GO once the ramp has finished.  Waiting for that is
	 * the only evidence available from here that the rail actually moved.
	 */
	for (i = 0; i < 1000; i++) {
		if (i2c0_read_reg(TPS65217_ADDR, TPS_DEFSLEW, &val)) {
			uart_puts("[PMIC] DEFSLEW readback failed — staying at 600 MHz\r\n");
			return -1;
		}
		if (!(val & TPS_DCDC_GO))
			break;
		delay(2000);
	}

	if (val & TPS_DCDC_GO) {
		uart_puts("[PMIC] voltage transition did not complete — staying at 600 MHz\r\n");
		return -1;
	}

	uart_puts("[PMIC] VDD_MPU ramped to 1.325 V (GO cleared)\r\n");
	return 0;
}
