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
# Header dependency tracking. Without this, editing a header does NOT rebuild
# the .c files that include it, so an incremental build can link objects
# compiled against two different versions of the same inline function — a
# corruption class that is invisible until it misbehaves on the board.
KBUILD_CFLAGS += -MMD -MP

KBUILD_AFLAGS := -mcpu=cortex-a8 -marm -D__ASSEMBLY__
KBUILD_AFLAGS += -I include -I arch/arm/include
KBUILD_AFLAGS += -MMD -MP

LDSCRIPT := arch/arm/kernel/kernel.ld
LDFLAGS  := -nostdlib -T $(LDSCRIPT) -Map $(BUILD_DIR)/kernel.map

# libgcc = compiler runtime (NOT libc): 64-bit divide (__aeabi_uldivmod) etc.
# needed by CFS-lite vruntime math. Self-contained arithmetic, safe to link.
LIBGCC := $(shell $(CC) $(KBUILD_CFLAGS) -print-libgcc-file-name)

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
OBJS += $(BUILD_DIR)/lib/rbtree.o
OBJS += $(BUILD_DIR)/kernel/spawn.o
OBJS += $(BUILD_DIR)/kernel/sched/core.o
OBJS += $(BUILD_DIR)/kernel/sched/fair.o
OBJS += $(BUILD_DIR)/kernel/sched/wait.o
OBJS += $(BUILD_DIR)/kernel/sched/completion.o
OBJS += $(BUILD_DIR)/kernel/ipc/msgq.o
OBJS += $(BUILD_DIR)/kernel/syscall.o
OBJS += $(BUILD_DIR)/kernel/exit.o
OBJS += $(BUILD_DIR)/kernel/panic.o
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

DEPS := $(OBJS:.o=.d)

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

# Default target: build kernel binary
.PHONY: all
all: $(KERNEL_BIN)

# init is the only program embedded in the kernel image (the bootstrap).
# Everything else ships on the SD card - see the sdcard target below.
USER_INIT_BIN  := $(BUILD_DIR)/init.usr

.PHONY: userspace
userspace:
	$(MAKE) -C ../userspace

$(USER_INIT_BIN): userspace | $(BUILD_DIR)
	cp ../userspace/build/init.bin $@

$(BUILD_DIR)/kernel/userspace_blobs.o: kernel/userspace_blobs.S \
	$(USER_INIT_BIN) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(KBUILD_AFLAGS) -c -o $@ $<

#
# SD-card staging tree: what the running system loads by path.
#
# 'make sdcard' builds build/sdcard/ with the layout the board expects:
#
#   /bin/{gui,shell,phoned,storaged}
#   /etc/inittab      - the services init starts, one path per line
#
# Copy the CONTENTS of build/sdcard/ onto the FAT partition, e.g.
#   cp -r build/sdcard/* /media/$$USER/BOOT/
#
# Kernel and services are now separate artifacts: changing a service means
# copying one file, not reflashing the kernel.
#
# NAMES ARE 8.3 ON PURPOSE. The FAT driver reads short directory entries and
# skips long-name (LFN) ones, so a file must be findable by its 8.3 name:
# "phone_daemon" would be stored as an LFN plus a mangled "PHONE_~1" short
# name and would never be found. Keep every name here <= 8 characters.
#
SDCARD_DIR := $(BUILD_DIR)/sdcard

.PHONY: sdcard
sdcard: userspace
	@rm -rf $(SDCARD_DIR)	# a renamed binary must not leave the old one behind
	@mkdir -p $(SDCARD_DIR)/bin $(SDCARD_DIR)/etc
	cp ../userspace/build/gui.bin            $(SDCARD_DIR)/bin/gui
	cp ../userspace/build/shell.bin          $(SDCARD_DIR)/bin/shell
	cp ../userspace/build/phone_daemon.bin   $(SDCARD_DIR)/bin/phoned
	cp ../userspace/build/storage_daemon.bin $(SDCARD_DIR)/bin/storaged
	cp inittab                               $(SDCARD_DIR)/etc/inittab
	@echo "[SDCARD] staged in $(SDCARD_DIR) - copy its contents to the FAT partition"
	@ls -la $(SDCARD_DIR)/bin $(SDCARD_DIR)/etc

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(KERNEL_ELF): $(OBJS) $(LDSCRIPT) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBGCC)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(KBUILD_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(KBUILD_AFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ---------------------------------------------------------------------------
# Host-side logic tests (native gcc, no board).
#
# Race and lost-wakeup bugs cannot be investigated over UART, so anything that
# touches shared state gets shaken out here first, on a machine with a real
# debugger and ThreadSanitizer.
#
# tools/test/include is searched FIRST so that <asm/...> resolves to the host
# stubs instead of the ARM headers. The kernel headers themselves are NOT
# forked — include/nothan/wait.h pulls in <asm/irqflags.h> unconditionally, and
# the stub is what lets it compile off-target. One source of truth.
# ---------------------------------------------------------------------------
HOSTCC ?= cc
HOST_CFLAGS := -Wall -Wextra -I tools/test/include -I include

.PHONY: test-rbtree
test-rbtree: | $(BUILD_DIR)
	$(HOSTCC) $(HOST_CFLAGS) -o $(BUILD_DIR)/rbtree_test \
		tools/test/rbtree_test.c lib/rbtree.c && $(BUILD_DIR)/rbtree_test

.PHONY: test-fair
test-fair: | $(BUILD_DIR)
	$(HOSTCC) $(HOST_CFLAGS) -o $(BUILD_DIR)/fair_sim \
		tools/test/fair_sim.c lib/rbtree.c && $(BUILD_DIR)/fair_sim

# msgq/wait compiled from the SAME sources the board builds, run on a pthread
# scheduler stub (tools/test/host_sched.c). Catches lost wakeups and dropped or
# duplicated messages — the failures UART cannot show, because their symptom on
# the board is silence or a fault far from the cause.
.PHONY: test-msgq
test-msgq: | $(BUILD_DIR)
	$(HOSTCC) $(HOST_CFLAGS) -I tools/test -o $(BUILD_DIR)/msgq_test \
		tools/test/msgq_test.c tools/test/host_sched.c \
		kernel/ipc/msgq.c kernel/sched/wait.c -lpthread \
		&& $(BUILD_DIR)/msgq_test

.PHONY: test
test: test-rbtree test-fair test-msgq

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
