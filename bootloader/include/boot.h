#ifndef BOOT_H
#define BOOT_H

#include "types.h"

void bootloader_main(void);
void panic(const char *msg);

void clock_domains_early_init(void);
void clock_init(void);
void clock_mpu_set_1ghz(void);

/* Returns 0 only if the PMIC read back 1.325 V on VDD_MPU. */
int pmic_set_mpu_1v325(void);

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_print_dec(uint32_t val);
void uart_print_hex(uint32_t val);
void uart_flush(void);

void ddr_init(void);
int  ddr_test(int silent);

int mmc_init(void);
int mmc_read_sectors(uint32_t start_sector, uint32_t count, void *dest);

void  delay(uint32_t count);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

#endif /* BOOT_H */
