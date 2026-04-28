# VinixOS Kernel Makefile (Linux-style layout)

CROSS_COMPILE = arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

USERSPACE_DIR = ../userspace
BUILD_DIR     = build

# Source layout — Linux-style flat structure
ASM_SRC = arch/arm/entry/reset.S \
          arch/arm/entry/entry.S \
          arch/arm/exceptions/vectors.S \
          arch/arm/exceptions/exception_entry.S \
          arch/arm/scheduler/context_switch.S \
          arch/arm/mm/mmu_enable.S

C_SRC = arch/arm/kernel/exception_handlers.c \
        arch/arm/kernel/svc_handler.c \
        arch/arm/mm/mmu.c \
        arch/arm/mach-omap2/memmap.c \
        arch/arm/mach-omap2/board-bbb.c \
        init/main.c \
        init/initcall.c \
        kernel/fork.c \
        kernel/wait.c \
        kernel/exec.c \
        kernel/irq/irq_core.c \
        kernel/i2c/i2c-core.c \
        kernel/irqchip/irqchip.c \
        kernel/time/clockevents.c \
        kernel/mmc/core.c \
        kernel/mmc/mmc_block.c \
        kernel/printk/printk.c \
        kernel/tty/serial/serial_core.c \
        kernel/sched/scheduler.c \
        kernel/sched/task.c \
        kernel/sched/idle.c \
        kernel/locking/atomic.c \
        kernel/locking/spinlock.c \
        kernel/locking/wait_queue.c \
        kernel/locking/sleep.c \
        kernel/locking/sync_selftest.c \
        drivers/tty/serial/omap_serial.c \
        drivers/watchdog/omap_wdt.c \
        drivers/irqchip/irq-omap-intc.c \
        drivers/clocksource/timer-omap-dm.c \
        drivers/i2c/busses/i2c-omap.c \
        drivers/gpu/drm/tilcdc/tilcdc_drv.c \
        drivers/gpu/drm/i2c/tda998x.c \
        drivers/mmc/host/omap_hsmmc.c \
        block/partitions/msdos.c \
        drivers/video/fbdev/fbmem.c \
        drivers/video/fbdev/fbcon.c \
        lib/fonts/font_8x16.c \
        drivers/char/char_dev.c \
        drivers/base/device.c \
        drivers/base/platform.c \
        drivers/video/boot_screen.c \
        fs/vfs.c \
        fs/fat32.c \
        fs/devfs.c \
        fs/procfs.c \
        mm/page_alloc.c \
        mm/slab.c \
        mm/vmm.c \
        block/block.c \
        block/buffer_cache.c \
        lib/string.c \
        lib/format.c \
        lib/test/selftest.c

# Object names — flatten under build/, basename only (relies on unique names)
ASM_OBJ     = $(addprefix $(BUILD_DIR)/, $(notdir $(ASM_SRC:.S=.o)))
C_OBJ       = $(addprefix $(BUILD_DIR)/, $(notdir $(C_SRC:.c=.o)))
PAYLOAD_OBJ = $(BUILD_DIR)/payload.o
OBJS        = $(ASM_OBJ) $(C_OBJ) $(PAYLOAD_OBJ)

# VPATH lets Make find sources by basename across directory tree
VPATH = arch/arm/entry:arch/arm/exceptions:arch/arm/scheduler:arch/arm/mm:arch/arm/kernel:arch/arm/mach-omap2:init:kernel:kernel/irq:kernel/irqchip:kernel/i2c:kernel/mmc:kernel/printk:kernel/time:kernel/tty/serial:kernel/sched:kernel/locking:drivers/char:drivers/base:drivers/video:drivers/video/fbdev:drivers/tty/serial:drivers/irqchip:drivers/clocksource:drivers/i2c/busses:drivers/mmc/host:drivers/watchdog:drivers/gpu/drm/tilcdc:drivers/gpu/drm/i2c:fs:mm:block:block/partitions:lib:lib/fonts:lib/test

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding \
         -march=armv7-a -mtune=cortex-a8 \
         -mfloat-abi=soft -mno-unaligned-access \
         -fno-builtin -fno-stack-protector \
         -Iinclude \
         -Ilib/include \
         -Iarch/arm/include \
         -Iarch/arm/mach-omap2/include \
         -I$(USERSPACE_DIR)/include \
         -O2 -g
ASFLAGS = -march=armv7-a -mfloat-abi=soft
LDFLAGS = -T arch/arm/kernel.ld -nostdlib -Wl,-Map=$(BUILD_DIR)/kernel.map

all: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Userspace shell payload (embedded into kernel image)
PAYLOAD_BIN = $(USERSPACE_DIR)/build/apps/shell/shell.bin
$(PAYLOAD_BIN):
	$(MAKE) -C $(USERSPACE_DIR)

$(PAYLOAD_OBJ): init/payload.S $(PAYLOAD_BIN) | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

# Generic pattern rules — VPATH locates the source
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lgcc
	@echo "=== Kernel sections ==="
	@$(CROSS_COMPILE)size $@

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@echo ""
	@echo "Build complete: $@"
	@ls -lh $@

clean:
	@echo "  CLEAN   kernel objects and elf"
	@rm -rf $(BUILD_DIR) *.o *.elf *.bin *.map uImage
	@echo "  CLEAN   userspace payload"
	@$(MAKE) -C $(USERSPACE_DIR) clean

.PHONY: all clean
