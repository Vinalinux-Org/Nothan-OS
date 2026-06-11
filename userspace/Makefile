CROSS_COMPILE ?= arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

BUILD   := build

CFLAGS  := -mcpu=cortex-a8 -marm -O2 -std=gnu11 -ffreestanding -nostdlib
CFLAGS  += -Wall -Wextra -Wno-unused-parameter -I lib
AFLAGS  := -mcpu=cortex-a8 -marm -D__ASSEMBLY__

LDS     := lib/user.lds
CRT0    := $(BUILD)/lib/crt0.o

BIN_PROGRAMS  := sh ps ls info uname reboot shutdown
SBIN_PROGRAMS := init
ALL_BINS      := $(foreach p,$(BIN_PROGRAMS),$(BUILD)/bin/$(p).bin) \
                 $(foreach p,$(SBIN_PROGRAMS),$(BUILD)/sbin/$(p).bin)

.PHONY: all clean size

all: $(BUILD)/shell.bin $(BUILD)/example.bin $(ALL_BINS)

$(BUILD)/lib/crt0.o: lib/crt0.S | $(BUILD)/lib
	$(CC) $(AFLAGS) -c -o $@ $<

define BIN_template
$(BUILD)/bin/$(1)/main.o: bin/$(1)/main.c | $(BUILD)/bin/$(1)
	$(CC) $(CFLAGS) -c -o $$@ $$<

$(BUILD)/bin/$(1).elf: $(CRT0) $(BUILD)/bin/$(1)/main.o
	$(CC) -nostdlib -T $(LDS) -o $$@ $$^ -lgcc
	$(OBJDUMP) -d $$@ > $(BUILD)/bin/$(1).dis

$(BUILD)/bin/$(1).bin: $(BUILD)/bin/$(1).elf
	$(OBJCOPY) -O binary $$< $$@
endef

define SBIN_template
$(BUILD)/sbin/$(1)/main.o: sbin/$(1)/main.c | $(BUILD)/sbin/$(1)
	$(CC) $(CFLAGS) -c -o $$@ $$<

$(BUILD)/sbin/$(1).elf: $(CRT0) $(BUILD)/sbin/$(1)/main.o
	$(CC) -nostdlib -T $(LDS) -o $$@ $$^ -lgcc
	$(OBJDUMP) -d $$@ > $(BUILD)/sbin/$(1).dis

$(BUILD)/sbin/$(1).bin: $(BUILD)/sbin/$(1).elf
	$(OBJCOPY) -O binary $$< $$@
endef

$(foreach p,$(BIN_PROGRAMS),$(eval $(call BIN_template,$(p))))
$(foreach p,$(SBIN_PROGRAMS),$(eval $(call SBIN_template,$(p))))

$(BUILD)/shell.bin: $(BUILD)/bin/sh.bin
	cp $< $@

$(BUILD)/example/main.o: example/main.c | $(BUILD)/example
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/example.elf: $(CRT0) $(BUILD)/example/main.o
	$(CC) -nostdlib -T $(LDS) -o $@ $^
	$(OBJDUMP) -d $@ > $(BUILD)/example.dis

$(BUILD)/example.bin: $(BUILD)/example.elf
	$(OBJCOPY) -O binary $< $@

DIRS := $(BUILD)/lib $(BUILD)/example \
        $(foreach p,$(BIN_PROGRAMS),$(BUILD)/bin/$(p)) \
        $(foreach p,$(SBIN_PROGRAMS),$(BUILD)/sbin/$(p))

$(DIRS):
	mkdir -p $@

size: all
	ls -la $(ALL_BINS)

clean:
	rm -rf $(BUILD)
