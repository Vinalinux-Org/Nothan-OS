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

# LVGL static library
LVGL_DIR  := lib/lvgl
LVGL_SRCS := $(shell find $(LVGL_DIR)/src -name '*.c')
LVGL_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(LVGL_SRCS))
LVGL_LIB  := $(BUILD)/liblvgl.a
LVGL_CFLAGS := $(CFLAGS) -I lib/lvgl -DLV_CONF_INCLUDE_SIMPLE \
               -Wno-unused-variable -Wno-type-limits -Wno-sign-compare

.PHONY: all clean size

all: $(BUILD)/shell.bin $(BUILD)/example.bin $(ALL_BINS) $(BUILD)/gui.bin

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

# LVGL build rules
$(BUILD)/$(LVGL_DIR)/%.o: $(LVGL_DIR)/%.c | $(LVGL_LIB_DIRS)
	mkdir -p $(dir $@)
	$(CC) $(LVGL_CFLAGS) -c -o $@ $<

$(LVGL_LIB): $(LVGL_OBJS)
	$(AR) rcs $@ $^

# GUI app
GUI_SRCS := gui/main.c \
            gui/port/lv_port_disp.c gui/port/lv_port_indev.c \
            gui/core/nav.c gui/core/log.c \
            gui/services/contacts.c gui/services/messages.c gui/services/telephony.c \
            gui/widgets/status_bar.c gui/widgets/app_tile.c gui/widgets/nav_bar.c \
            gui/widgets/app_header.c gui/widgets/avatar.c \
            gui/screens/boot.c gui/screens/home.c \
            gui/screens/contacts_list.c gui/screens/contact_detail.c \
            gui/screens/contacts_add.c \
            gui/screens/sms_list.c gui/screens/sms_chat.c \
            gui/screens/dialer.c gui/screens/active_call.c gui/screens/incoming_call.c
GUI_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(GUI_SRCS))
GUI_CFLAGS := $(CFLAGS) -I lib/lvgl -I gui -DLV_CONF_INCLUDE_SIMPLE

$(BUILD)/gui/%.o: gui/%.c | $(BUILD)/gui/port
	mkdir -p $(dir $@)
	$(CC) $(GUI_CFLAGS) -c -o $@ $<

$(BUILD)/gui.elf: $(CRT0) $(GUI_OBJS) $(LVGL_LIB)
	$(CC) -nostdlib -T $(LDS) -o $@ $(CRT0) $(GUI_OBJS) $(LVGL_LIB) -lgcc -lc
	$(OBJDUMP) -d $@ > $(BUILD)/gui.dis

$(BUILD)/gui.bin: $(BUILD)/gui.elf
	$(OBJCOPY) -O binary $< $@

LVGL_LIB_DIRS := $(sort $(dir $(LVGL_OBJS)))

DIRS := $(BUILD)/lib $(BUILD)/example \
        $(BUILD)/gui/port $(BUILD)/gui/core $(BUILD)/gui/services \
        $(BUILD)/gui/widgets $(BUILD)/gui/screens \
        $(foreach p,$(BIN_PROGRAMS),$(BUILD)/bin/$(p)) \
        $(foreach p,$(SBIN_PROGRAMS),$(BUILD)/sbin/$(p))

$(DIRS):
	mkdir -p $@

size: all
	ls -la $(ALL_BINS) $(BUILD)/gui.bin

clean:
	rm -rf $(BUILD)
