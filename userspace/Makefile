CROSS_COMPILE ?= arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

BUILD   := build

CFLAGS  := -mcpu=cortex-a8 -marm -O2 -std=gnu11 -ffreestanding -nostdlib
# -fno-strict-aliasing: LVGL (and our type-punning) violate strict aliasing;
# without this, ARM GCC -O2 miscompiles the SW draw path (the masked RGB565
# blend's loop bound and lv_area pointers came out garbage → Data Aborts).
# This is the standard, LVGL-recommended fix and keeps -O2 performance.
CFLAGS  += -fno-strict-aliasing
CFLAGS  += -Wall -Wextra -Wno-unused-parameter -I lib
AFLAGS  := -mcpu=cortex-a8 -marm -D__ASSEMBLY__

LDS     := lib/user.lds
CRT0    := $(BUILD)/lib/crt0.o

# LVGL static library
LVGL_DIR  := lib/lvgl
LVGL_SRCS := $(shell find $(LVGL_DIR)/src -name '*.c')
LVGL_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(LVGL_SRCS))
LVGL_LIB  := $(BUILD)/liblvgl.a
LVGL_CFLAGS := $(CFLAGS) -I lib/lvgl -DLV_CONF_INCLUDE_SIMPLE \
               -Wno-unused-variable -Wno-type-limits -Wno-sign-compare -O1

.PHONY: all clean size

all: $(BUILD)/shell.bin $(BUILD)/gui.bin $(BUILD)/phone_daemon.bin

$(BUILD)/lib/crt0.o: lib/crt0.S | $(BUILD)/lib
	$(CC) $(AFLAGS) -c -o $@ $<

# Shell
$(BUILD)/sh/main.o: sh/main.c | $(BUILD)/sh
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/shell.elf: $(CRT0) $(BUILD)/sh/main.o
	$(CC) -nostdlib -T $(LDS) -o $@ $^ -lgcc
	$(OBJDUMP) -d $@ > $(BUILD)/shell.dis

$(BUILD)/shell.bin: $(BUILD)/shell.elf
	$(OBJCOPY) -O binary $< $@

# Phone daemon (modem backend) — links like the shell: crt0 + -lgcc, no newlib.
# Bare-metal flags: no poll()/clock_gettime()/usleep() (use getticks/yield).
PD_SRCS   := phone_daemon/phone_daemon.c phone_daemon/phone_frame.c phone_daemon/json.c \
             lib/mem.c
PD_OBJS   := $(patsubst %.c,$(BUILD)/%.o,$(PD_SRCS))
PD_CFLAGS := $(CFLAGS) -DPD_NO_POLL -DPD_NO_CLOCK -DPD_NO_USLEEP \
             -Wno-unused-function -Wno-unused-variable

$(BUILD)/phone_daemon/%.o: phone_daemon/%.c | $(BUILD)/phone_daemon
	$(CC) $(PD_CFLAGS) -c -o $@ $<

$(BUILD)/lib/mem.o: lib/mem.c | $(BUILD)/lib
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/phone_daemon.elf: $(CRT0) $(PD_OBJS)
	$(CC) -nostdlib -T $(LDS) -o $@ $^ -lgcc
	$(OBJDUMP) -d $@ > $(BUILD)/phone_daemon.dis

$(BUILD)/phone_daemon.bin: $(BUILD)/phone_daemon.elf
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
            gui/core/nav.c gui/core/log.c gui/core/keyboard.c gui/core/call_ui.c \
            gui/services/contacts.c gui/services/messages.c gui/services/telephony.c \
            gui/services/modem_client.c \
            gui/services/storage.c \
            gui/widgets/status_bar.c gui/widgets/app_tile.c gui/widgets/nav_bar.c \
            gui/widgets/app_header.c gui/widgets/avatar.c \
            gui/screens/boot.c gui/screens/home.c \
            gui/screens/contacts_list.c gui/screens/contact_detail.c \
            gui/screens/contacts_add.c \
            gui/screens/sms_list.c gui/screens/sms_chat.c \
            gui/screens/dialer.c gui/screens/call_log.c
GUI_OBJS := $(patsubst gui/%.c,$(BUILD)/gui/%.o,$(GUI_SRCS)) \
            $(BUILD)/gui/phone_daemon/phone_frame.o \
            $(BUILD)/gui/phone_daemon/json.o
GUI_CFLAGS := $(CFLAGS) -I lib/lvgl -I gui -DLV_CONF_INCLUDE_SIMPLE


$(BUILD)/gui/%.o: gui/%.c | $(BUILD)/gui/port
	mkdir -p $(dir $@)
	$(CC) $(GUI_CFLAGS) -c -o $@ $<

$(BUILD)/gui/phone_daemon/%.o: phone_daemon/%.c | $(BUILD)/gui/phone_daemon
	mkdir -p $(dir $@)
	$(CC) $(GUI_CFLAGS) -c -o $@ $<

$(BUILD)/gui.elf: $(CRT0) $(GUI_OBJS) $(LVGL_LIB)
	$(CC) -nostdlib -T $(LDS) -o $@ $(CRT0) $(GUI_OBJS) $(LVGL_LIB) -lgcc -lc
	$(OBJDUMP) -d $@ > $(BUILD)/gui.dis

$(BUILD)/gui.bin: $(BUILD)/gui.elf
	$(OBJCOPY) -O binary $< $@

LVGL_LIB_DIRS := $(sort $(dir $(LVGL_OBJS)))

DIRS := $(BUILD)/lib $(BUILD)/sh $(BUILD)/phone_daemon \
        $(BUILD)/gui/port $(BUILD)/gui/core $(BUILD)/gui/services \
        $(BUILD)/gui/widgets $(BUILD)/gui/screens \
        $(BUILD)/gui/phone_daemon

$(DIRS):
	mkdir -p $@

size: all
	ls -la $(BUILD)/shell.bin $(BUILD)/gui.bin

clean:
	rm -rf $(BUILD)
