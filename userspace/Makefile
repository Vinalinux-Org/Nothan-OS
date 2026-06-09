CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

BUILD_DIR := build
HELLO_DIR := hello

CFLAGS := -mcpu=cortex-a8 -marm -O2 -std=gnu11 -ffreestanding -nostdlib
CFLAGS += -Wall -Wextra -Wno-unused-parameter -I lib
AFLAGS := -mcpu=cortex-a8 -marm -D__ASSEMBLY__
LDFLAGS := -nostdlib -T $(HELLO_DIR)/user.lds

.PHONY: all hello clean size

all: hello

hello: $(BUILD_DIR)/hello.bin

$(BUILD_DIR)/hello.elf: $(BUILD_DIR)/hello/start.o $(BUILD_DIR)/hello/main.o | $(BUILD_DIR)/hello
	$(LD) $(LDFLAGS) -o $@ $^
	$(OBJDUMP) -d $@ > $(BUILD_DIR)/hello.dis

$(BUILD_DIR)/hello.bin: $(BUILD_DIR)/hello.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/hello/%.o: $(HELLO_DIR)/%.c | $(BUILD_DIR)/hello
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/hello/%.o: $(HELLO_DIR)/%.S | $(BUILD_DIR)/hello
	$(CC) $(AFLAGS) -c -o $@ $<

$(BUILD_DIR)/hello:
	mkdir -p $@

size: $(BUILD_DIR)/hello.bin
	ls -la $(BUILD_DIR)/hello.bin
	$(OBJDUMP) -h $(BUILD_DIR)/hello.elf

clean:
	rm -rf $(BUILD_DIR)
