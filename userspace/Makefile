CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

BUILD_DIR := build

CFLAGS := -mcpu=cortex-a8 -marm -O2 -std=gnu11 -ffreestanding -nostdlib
CFLAGS += -Wall -Wextra -Wno-unused-parameter -I lib
AFLAGS := -mcpu=cortex-a8 -marm -D__ASSEMBLY__

.PHONY: all shell example clean size

all: shell example

# === shell ===
SHELL_DIR := shell

$(BUILD_DIR)/shell/%.o: $(SHELL_DIR)/%.c | $(BUILD_DIR)/shell
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/shell/%.o: $(SHELL_DIR)/%.S | $(BUILD_DIR)/shell
	$(CC) $(AFLAGS) -c -o $@ $<

$(BUILD_DIR)/shell.elf: $(BUILD_DIR)/shell/start.o $(BUILD_DIR)/shell/main.o
	$(CC) -nostdlib -T $(SHELL_DIR)/user.lds -o $@ $^ -lgcc
	$(OBJDUMP) -d $@ > $(BUILD_DIR)/shell.dis

$(BUILD_DIR)/shell.bin: $(BUILD_DIR)/shell.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/shell:
	mkdir -p $@

shell: $(BUILD_DIR)/shell.bin

# === example ===
EXAMPLE_DIR := example

$(BUILD_DIR)/example/%.o: $(EXAMPLE_DIR)/%.c | $(BUILD_DIR)/example
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/example/%.o: $(EXAMPLE_DIR)/%.S | $(BUILD_DIR)/example
	$(CC) $(AFLAGS) -c -o $@ $<

$(BUILD_DIR)/example.elf: $(BUILD_DIR)/example/start.o $(BUILD_DIR)/example/main.o
	$(CC) -nostdlib -T $(EXAMPLE_DIR)/user.lds -o $@ $^
	$(OBJDUMP) -d $@ > $(BUILD_DIR)/example.dis

$(BUILD_DIR)/example.bin: $(BUILD_DIR)/example.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/example:
	mkdir -p $@

example: $(BUILD_DIR)/example.bin

size: $(BUILD_DIR)/shell.bin $(BUILD_DIR)/example.bin
	ls -la $(BUILD_DIR)/shell.bin $(BUILD_DIR)/example.bin

clean:
	rm -rf $(BUILD_DIR)
