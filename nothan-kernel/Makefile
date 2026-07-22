CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

BUILD_DIR := build

KBUILD_CFLAGS := -mcpu=cortex-a8 -marm -O2 -std=gnu11 -Wall -Wextra
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
OBJS += $(BUILD_DIR)/arch/arm/kernel/switch_to.o
OBJS += $(BUILD_DIR)/arch/arm/mm/mmu.o
OBJS += $(BUILD_DIR)/arch/arm/mm/mmu_enable.o
OBJS += $(BUILD_DIR)/arch/arm/mm/pgd.o
OBJS += $(BUILD_DIR)/arch/arm/mm/uaccess.o
OBJS += $(BUILD_DIR)/drivers/irqchip/irq-omap-intc.o
OBJS += $(BUILD_DIR)/kernel/irq/irq_core.o
OBJS += $(BUILD_DIR)/drivers/tty/serial/omap-serial.o
OBJS += $(BUILD_DIR)/drivers/clocksource/timer-ti-dm.o
OBJS += $(BUILD_DIR)/kernel/printk.o
OBJS += $(BUILD_DIR)/kernel/time/delay.o
OBJS += $(BUILD_DIR)/kernel/time/timer.o
OBJS += $(BUILD_DIR)/mm/page_alloc.o
OBJS += $(BUILD_DIR)/mm/slab.o
OBJS += $(BUILD_DIR)/kernel/spawn.o
OBJS += $(BUILD_DIR)/kernel/sched/core.o
OBJS += $(BUILD_DIR)/kernel/sched/rt.o
OBJS += $(BUILD_DIR)/kernel/sched/wait.o
OBJS += $(BUILD_DIR)/kernel/sched/completion.o
OBJS += $(BUILD_DIR)/kernel/ipc/msgq.o
OBJS += $(BUILD_DIR)/kernel/syscall.o
OBJS += $(BUILD_DIR)/kernel/exit.o
OBJS += $(BUILD_DIR)/init/main.o
OBJS += $(BUILD_DIR)/kernel/userspace_blobs.o
OBJS += $(BUILD_DIR)/kernel/vfs/vfs.o
OBJS += $(BUILD_DIR)/drivers/block/genhd.o
OBJS += $(BUILD_DIR)/kernel/fs/fat/fat.o
OBJS += $(BUILD_DIR)/kernel/fs/fat/dir.o
OBJS += $(BUILD_DIR)/kernel/fs/fat/file.o
OBJS += $(BUILD_DIR)/drivers/base/bus.o
OBJS += $(BUILD_DIR)/drivers/base/platform.o
OBJS += $(BUILD_DIR)/drivers/base/init.o
OBJS += $(BUILD_DIR)/drivers/base/cdev.o
OBJS += $(BUILD_DIR)/drivers/gpio/gpiolib.o
OBJS += $(BUILD_DIR)/drivers/gpio/gpio-omap.o
OBJS += $(BUILD_DIR)/drivers/pinctrl/pinctrl-am335x.o
OBJS += $(BUILD_DIR)/kernel/fs/devfs/devfs.o
OBJS += $(BUILD_DIR)/drivers/mmc/omap-hsmmc.o
OBJS += $(BUILD_DIR)/arch/arm/mach-omap2/board-bbb.o
OBJS += $(BUILD_DIR)/drivers/video/fb.o
OBJS += $(BUILD_DIR)/drivers/video/lcdc.o
OBJS += $(BUILD_DIR)/drivers/video/tda19988.o
OBJS += $(BUILD_DIR)/drivers/input/input.o
OBJS += $(BUILD_DIR)/drivers/char/phonebus.o
OBJS += $(BUILD_DIR)/drivers/char/storagebus.o
OBJS += $(BUILD_DIR)/drivers/i2c/i2c-core.o
OBJS += $(BUILD_DIR)/drivers/i2c/i2c-omap.o
OBJS += $(BUILD_DIR)/drivers/usb/musb-hcd.o

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

# Default target: build kernel binary
.PHONY: all
all: $(KERNEL_BIN)

# User-space binary blobs (embedded into kernel image)
USER_SHELL_BIN := $(BUILD_DIR)/shell.usr
USER_GUI_BIN   := $(BUILD_DIR)/gui.usr
USER_PHONE_DAEMON_BIN := $(BUILD_DIR)/phone_daemon.usr
USER_STORAGE_DAEMON_BIN := $(BUILD_DIR)/storage_daemon.usr

.PHONY: userspace
userspace:
	$(MAKE) -C ../userspace

$(USER_SHELL_BIN): userspace | $(BUILD_DIR)
	cp ../userspace/build/shell.bin $@

$(USER_GUI_BIN): userspace | $(BUILD_DIR)
	cp ../userspace/build/gui.bin $@

$(USER_PHONE_DAEMON_BIN): userspace | $(BUILD_DIR)
	cp ../userspace/build/phone_daemon.bin $@

$(USER_STORAGE_DAEMON_BIN): userspace | $(BUILD_DIR)
	cp ../userspace/build/storage_daemon.bin $@

$(BUILD_DIR)/kernel/userspace_blobs.o: kernel/userspace_blobs.S \
	$(USER_SHELL_BIN) $(USER_GUI_BIN) $(USER_PHONE_DAEMON_BIN) $(USER_STORAGE_DAEMON_BIN) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(KBUILD_AFLAGS) -c -o $@ $<

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
