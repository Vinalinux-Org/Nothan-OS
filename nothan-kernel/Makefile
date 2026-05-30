CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

BUILD_DIR := build

KBUILD_CFLAGS := -mcpu=cortex-a8 -marm -O2 -std=c11 -Wall -Wextra
KBUILD_CFLAGS += -mno-unaligned-access -fno-common
KBUILD_CFLAGS += -ffreestanding -nostdlib -fno-stack-protector
KBUILD_CFLAGS += -Wno-array-bounds -Wno-unused-parameter
KBUILD_CFLAGS += -I include -I arch/arm/include

KBUILD_AFLAGS := -mcpu=cortex-a8 -marm -D__ASSEMBLY__
KBUILD_AFLAGS += -I include -I arch/arm/include

LDSCRIPT := arch/arm/kernel/kernel.ld
LDFLAGS  := -nostdlib -T $(LDSCRIPT) -Map $(BUILD_DIR)/kernel.map

OBJS := $(BUILD_DIR)/arch/arm/kernel/vectors.o
OBJS += $(BUILD_DIR)/arch/arm/kernel/traps.o
OBJS += $(BUILD_DIR)/arch/arm/kernel/head.o
OBJS += $(BUILD_DIR)/arch/arm/mm/mmu.o
OBJS += $(BUILD_DIR)/arch/arm/mm/mmu_enable.o
OBJS += $(BUILD_DIR)/arch/arm/mm/pgd.o
OBJS += $(BUILD_DIR)/drivers/irqchip/irq-omap-intc.o
OBJS += $(BUILD_DIR)/kernel/irq/irq_core.o
OBJS += $(BUILD_DIR)/drivers/tty/serial/omap-serial.o
OBJS += $(BUILD_DIR)/drivers/clocksource/timer-ti-dm.o
OBJS += $(BUILD_DIR)/kernel/printk.o
OBJS += $(BUILD_DIR)/init/main.o

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(KERNEL_ELF): $(OBJS) $(LDSCRIPT) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(KBUILD_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(KBUILD_AFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
