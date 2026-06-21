# sim/Makefile — NothanOS GUI simulator (host GCC + SDL2)
#
# Build:  make
# Run:    ./nothan-sim
# Clean:  make clean
#
# Prerequisites: sudo apt install libsdl2-dev

ROOT   := $(abspath ..)
BUILD  := build
TARGET := nothan-sim

CC     := gcc
CFLAGS := -g -O0 -Wall \
           -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter \
           -iquote $(ROOT)/lib \
           -iquote $(ROOT)/gui \
           $(shell pkg-config --cflags sdl2)
LDFLAGS := $(shell pkg-config --libs sdl2) -lm -lrt

# -----------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------

# These 4 files replace their kernel-specific counterparts:
#   gui/main.c              → sim_main.c        (POSIX timing, SDL event pump)
#   gui/core/log.c          → log_sim.c         (printf instead of svc write)
#   gui/port/lv_port_disp.c → lv_port_disp_sdl.c (SDL2 instead of FB ioctl)
#   gui/port/lv_port_indev.c → lv_port_indev_sdl.c (SDL2 mouse)
#   gui/services/storage.c  → storage_sim.c       (no-op; no kernel/FS in sim)
SIM_SRC := sim_main.c log_sim.c lv_port_disp_sdl.c lv_port_indev_sdl.c storage_sim.c

GUI_ALL  := $(shell find $(ROOT)/gui -name "*.c")
GUI_EXCL := $(ROOT)/gui/main.c \
            $(ROOT)/gui/core/log.c \
            $(ROOT)/gui/port/lv_port_disp.c \
            $(ROOT)/gui/port/lv_port_indev.c \
            $(ROOT)/gui/services/storage.c
GUI_SRC  := $(filter-out $(GUI_EXCL), $(GUI_ALL))

LVGL_SRC := $(shell find $(ROOT)/lib/lvgl/src -name "*.c")

# -----------------------------------------------------------------------
# Objects (preserve tree under build/ → no basename collisions)
# -----------------------------------------------------------------------

SIM_OBJS  := $(patsubst %.c,              $(BUILD)/sim/%.o,  $(SIM_SRC))
GUI_OBJS  := $(patsubst $(ROOT)/gui/%.c,  $(BUILD)/gui/%.o,  $(GUI_SRC))
LVGL_OBJS := $(patsubst $(ROOT)/lib/lvgl/src/%.c, $(BUILD)/lvgl/%.o, $(LVGL_SRC))

ALL_OBJS := $(SIM_OBJS) $(GUI_OBJS) $(LVGL_OBJS)

# -----------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(ALL_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(BUILD)/sim/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/gui/%.o: $(ROOT)/gui/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/lvgl/%.o: $(ROOT)/lib/lvgl/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) $(TARGET)
