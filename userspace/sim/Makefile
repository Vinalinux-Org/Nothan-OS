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
CFLAGS := -g -O0 -Wall -DLV_SIM_HOST \
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

.PHONY: all clean asan builtin guard stacktest m32 plain arm

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

# -----------------------------------------------------------------------
# ASan repro build — catches the home-press heap over-run on the host.
#
#   make asan && ASAN_OPTIONS=detect_leaks=0 ./nothan-sim-asan
#
# Same sources, but every LVGL allocation is a real malloc with redzones
# (lv_conf.h → LV_STDLIB_CLIB under LV_SIM_HOST), and SIM_AUTOTAP swaps the
# SDL event loop for a deterministic press sweep over the home tiles. The
# tree lives under build-asan/ so it never mixes with the normal objects.
# -----------------------------------------------------------------------

ABUILD      := build-asan
ATARGET     := nothan-sim-asan
ASAN_FLAGS  := -fsanitize=address -fno-omit-frame-pointer -DSIM_AUTOTAP -DNOTHAN_MEM_TRACE=16384

ASAN_SIM_OBJS  := $(patsubst %.c,              $(ABUILD)/sim/%.o,  $(SIM_SRC))
ASAN_GUI_OBJS  := $(patsubst $(ROOT)/gui/%.c,  $(ABUILD)/gui/%.o,  $(GUI_SRC))
ASAN_LVGL_OBJS := $(patsubst $(ROOT)/lib/lvgl/src/%.c, $(ABUILD)/lvgl/%.o, $(LVGL_SRC))
ASAN_OBJS := $(ASAN_SIM_OBJS) $(ASAN_GUI_OBJS) $(ASAN_LVGL_OBJS)

asan: $(ATARGET)

$(ATARGET): $(ASAN_OBJS)
	$(CC) $^ $(LDFLAGS) $(ASAN_FLAGS) -o $@

$(ABUILD)/sim/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(ASAN_FLAGS) -c $< -o $@

$(ABUILD)/gui/%.o: $(ROOT)/gui/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(ASAN_FLAGS) -c $< -o $@

$(ABUILD)/lvgl/%.o: $(ROOT)/lib/lvgl/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(ASAN_FLAGS) -c $< -o $@

# -----------------------------------------------------------------------
# HW-faithful build — the built-in TLSF pool (one static work_mem array,
# exactly like hardware) under ASan + per-action heap integrity checks.
#
#   make builtin && ASAN_OPTIONS=detect_leaks=0 ./nothan-sim-builtin
#
# ASan red-zones the work_mem global, so a write past the pool (the DFAR =
# __bss_end Data Abort signature) surfaces as a global-buffer-overflow with
# file:line; LV_USE_ASSERT_MEM_INTEGRITY + the autotap's lv_mem_test() after
# every action name the first tap/scroll that corrupts the pool.
# -----------------------------------------------------------------------

BBUILD      := build-builtin
BTARGET     := nothan-sim-builtin
BSAN_FLAGS  := -fsanitize=address -fno-omit-frame-pointer -DSIM_AUTOTAP -DSIM_BUILTIN

BSAN_SIM_OBJS  := $(patsubst %.c,              $(BBUILD)/sim/%.o,  $(SIM_SRC))
BSAN_GUI_OBJS  := $(patsubst $(ROOT)/gui/%.c,  $(BBUILD)/gui/%.o,  $(GUI_SRC))
BSAN_LVGL_OBJS := $(patsubst $(ROOT)/lib/lvgl/src/%.c, $(BBUILD)/lvgl/%.o, $(LVGL_SRC))
BSAN_OBJS := $(BSAN_SIM_OBJS) $(BSAN_GUI_OBJS) $(BSAN_LVGL_OBJS)

builtin: $(BTARGET)

$(BTARGET): $(BSAN_OBJS)
	$(CC) $^ $(LDFLAGS) $(BSAN_FLAGS) -o $@

$(BBUILD)/sim/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(BSAN_FLAGS) -c $< -o $@

$(BBUILD)/gui/%.o: $(ROOT)/gui/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(BSAN_FLAGS) -c $< -o $@

$(BBUILD)/lvgl/%.o: $(ROOT)/lib/lvgl/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(BSAN_FLAGS) -c $< -o $@

# -----------------------------------------------------------------------
# Guard-page build — the most faithful host reproduction of the hardware
# fault. Built-in TLSF pool AND the render/rotate buffers each sit against a
# PROT_NONE guard page, so a write ANY distance past a buffer end (the ~3.7 KB
# FAR over-run ASan red-zones miss) faults instantly at the boundary, exactly
# like the unmapped page above __bss_end.
#
#   make guard && ASAN_OPTIONS=detect_leaks=0 ./nothan-sim-guard
# -----------------------------------------------------------------------

GBUILD      := build-guard
GTARGET     := nothan-sim-guard
GSAN_FLAGS  := -fsanitize=address -fno-omit-frame-pointer -DSIM_AUTOTAP -DSIM_BUILTIN -DSIM_GUARD

GSAN_SIM_OBJS  := $(patsubst %.c,              $(GBUILD)/sim/%.o,  $(SIM_SRC))
GSAN_GUI_OBJS  := $(patsubst $(ROOT)/gui/%.c,  $(GBUILD)/gui/%.o,  $(GUI_SRC))
GSAN_LVGL_OBJS := $(patsubst $(ROOT)/lib/lvgl/src/%.c, $(GBUILD)/lvgl/%.o, $(LVGL_SRC))
GSAN_OBJS := $(GSAN_SIM_OBJS) $(GSAN_GUI_OBJS) $(GSAN_LVGL_OBJS)

guard: $(GTARGET)

$(GTARGET): $(GSAN_OBJS)
	$(CC) $^ $(LDFLAGS) $(GSAN_FLAGS) -o $@

$(GBUILD)/sim/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(GSAN_FLAGS) -c $< -o $@

$(GBUILD)/gui/%.o: $(ROOT)/gui/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(GSAN_FLAGS) -c $< -o $@

$(GBUILD)/lvgl/%.o: $(ROOT)/lib/lvgl/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(GSAN_FLAGS) -c $< -o $@

# -----------------------------------------------------------------------
# Stack high-water build — measures how deep LVGL's render call chain goes.
# NothanOS gives the GUI no heap, so every render temporary lives on the
# 128 KB user stack; this reports the real peak to compare against it.
# No ASan (it inflates frames); runs the autotap on an owned pthread stack.
#
#   make stacktest && ./nothan-sim-stacktest
# -----------------------------------------------------------------------

SBUILD      := build-stack
STARGET     := nothan-sim-stacktest
STK_FLAGS   := -DSIM_AUTOTAP -DSIM_STACKTEST

STK_SIM_OBJS  := $(patsubst %.c,              $(SBUILD)/sim/%.o,  $(SIM_SRC))
STK_GUI_OBJS  := $(patsubst $(ROOT)/gui/%.c,  $(SBUILD)/gui/%.o,  $(GUI_SRC))
STK_LVGL_OBJS := $(patsubst $(ROOT)/lib/lvgl/src/%.c, $(SBUILD)/lvgl/%.o, $(LVGL_SRC))
STK_OBJS := $(STK_SIM_OBJS) $(STK_GUI_OBJS) $(STK_LVGL_OBJS)

stacktest: $(STARGET)

$(STARGET): $(STK_OBJS)
	$(CC) $^ $(LDFLAGS) -lpthread -o $@

$(SBUILD)/sim/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(STK_FLAGS) -c $< -o $@

$(SBUILD)/gui/%.o: $(ROOT)/gui/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(STK_FLAGS) -c $< -o $@

$(SBUILD)/lvgl/%.o: $(ROOT)/lib/lvgl/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(STK_FLAGS) -c $< -o $@

# -----------------------------------------------------------------------
# 32-bit ILP32 build (`make m32`) — reproduces target-class (long/ptr = 4 B)
# bugs the 64-bit host cannot. Headless (no SDL, driven by the autotap), with
# the built-in TLSF pool + guard page like hardware. A corrupt fill area prints
# "CORRUPT fill area …" via the lv_draw_sw_blend.c guard; a far over-run that
# escapes it faults on the guard page. Needs gcc-multilib (32-bit libc only).
#
#   make m32 && ./nothan-sim-m32
# -----------------------------------------------------------------------

MBUILD      := build-m32
MTARGET     := nothan-sim-m32
M32_CFLAGS  := -m32 -g -O0 -Wall -DLV_SIM_HOST \
               -DSIM_AUTOTAP -DSIM_HEADLESS -DSIM_BUILTIN -DSIM_GUARD \
               -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter \
               -iquote $(ROOT)/lib -iquote $(ROOT)/gui
M32_LDFLAGS := -m32 -lm -lrt

M32_SIM_OBJS  := $(patsubst %.c,              $(MBUILD)/sim/%.o,  $(SIM_SRC))
M32_GUI_OBJS  := $(patsubst $(ROOT)/gui/%.c,  $(MBUILD)/gui/%.o,  $(GUI_SRC))
M32_LVGL_OBJS := $(patsubst $(ROOT)/lib/lvgl/src/%.c, $(MBUILD)/lvgl/%.o, $(LVGL_SRC))
M32_OBJS := $(M32_SIM_OBJS) $(M32_GUI_OBJS) $(M32_LVGL_OBJS)

m32: $(MTARGET)

$(MTARGET): $(M32_OBJS)
	$(CC) $^ $(M32_LDFLAGS) -o $@

$(MBUILD)/sim/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(M32_CFLAGS) -c $< -o $@

$(MBUILD)/gui/%.o: $(ROOT)/gui/%.c
	@mkdir -p $(dir $@)
	$(CC) $(M32_CFLAGS) -c $< -o $@

$(MBUILD)/lvgl/%.o: $(ROOT)/lib/lvgl/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(M32_CFLAGS) -c $< -o $@

# -----------------------------------------------------------------------
# Plain headless autotap (`make plain`) — 64-bit, CLIB malloc, NO ASan, no
# SDL. Built for Valgrind/memcheck: catches reads of UNINITIALISED memory
# (the corrupt blend-area coordinate suspect) that ASan/UBSan/-m32 all miss.
#
#   make plain && valgrind --track-origins=yes ./nothan-sim-plain
# -----------------------------------------------------------------------

PBUILD      := build-plain
PTARGET     := nothan-sim-plain
# POPT/PALIAS overridable: `make plain POPT=-O2 PALIAS=-fno-strict-aliasing`
# matches the firmware (userspace/Makefile: -O2 -fno-strict-aliasing). Testing
# at -O0 (the old default) hid the -O2 SW-draw miscompile entirely.
POPT        ?= -O2
PALIAS      ?= -fno-strict-aliasing
PSAN        ?=
P_CFLAGS    := -g $(POPT) $(PALIAS) $(PSAN) -Wall -DLV_SIM_HOST -DSIM_AUTOTAP -DSIM_HEADLESS \
               -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter \
               -iquote $(ROOT)/lib -iquote $(ROOT)/gui

P_SIM_OBJS  := $(patsubst %.c,              $(PBUILD)/sim/%.o,  $(SIM_SRC))
P_GUI_OBJS  := $(patsubst $(ROOT)/gui/%.c,  $(PBUILD)/gui/%.o,  $(GUI_SRC))
P_LVGL_OBJS := $(patsubst $(ROOT)/lib/lvgl/src/%.c, $(PBUILD)/lvgl/%.o, $(LVGL_SRC))
P_OBJS := $(P_SIM_OBJS) $(P_GUI_OBJS) $(P_LVGL_OBJS)

plain: $(PTARGET)

$(PTARGET): $(P_OBJS)
	$(CC) $^ $(PSAN) -lm -lrt -o $@

$(PBUILD)/sim/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(P_CFLAGS) -c $< -o $@

$(PBUILD)/gui/%.o: $(ROOT)/gui/%.c
	@mkdir -p $(dir $@)
	$(CC) $(P_CFLAGS) -c $< -o $@

$(PBUILD)/lvgl/%.o: $(ROOT)/lib/lvgl/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(P_CFLAGS) -c $< -o $@

# -----------------------------------------------------------------------
# REAL ARM build (`make arm`) — the definitive host reproduction. Compiles
# the headless autotap with the SAME flags as the firmware
# (-marm -mcpu=cortex-a8 -O2 -fno-strict-aliasing) using the ARM cross-gcc,
# statically linked, and runs it under qemu-arm. This is real ARMv7 -O2
# codegen — the only way to hit the ARM-GCC SW-draw miscompile that every
# x86 build (incl. -O2, -m32, ASan, UBSan, Valgrind) cannot.
#
#   make arm && qemu-arm ./nothan-sim-arm
# -----------------------------------------------------------------------

RBUILD      := build-arm
RTARGET     := nothan-sim-arm
ARMCC       := arm-linux-gnueabihf-gcc
ARM_CFLAGS  := -marm -mcpu=cortex-a8 -O2 -fno-strict-aliasing -g -DLV_SIM_HOST \
               -DSIM_AUTOTAP -DSIM_HEADLESS -DSIM_BUILTIN -DSIM_GUARD \
               -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter \
               -iquote $(ROOT)/lib -iquote $(ROOT)/gui

R_SIM_OBJS  := $(patsubst %.c,              $(RBUILD)/sim/%.o,  $(SIM_SRC))
R_GUI_OBJS  := $(patsubst $(ROOT)/gui/%.c,  $(RBUILD)/gui/%.o,  $(GUI_SRC))
R_LVGL_OBJS := $(patsubst $(ROOT)/lib/lvgl/src/%.c, $(RBUILD)/lvgl/%.o, $(LVGL_SRC))
R_OBJS := $(R_SIM_OBJS) $(R_GUI_OBJS) $(R_LVGL_OBJS)

arm: $(RTARGET)

$(RTARGET): $(R_OBJS)
	$(ARMCC) $^ -static -lm -lrt -o $@

$(RBUILD)/sim/%.o: %.c
	@mkdir -p $(dir $@)
	$(ARMCC) $(ARM_CFLAGS) -c $< -o $@

$(RBUILD)/gui/%.o: $(ROOT)/gui/%.c
	@mkdir -p $(dir $@)
	$(ARMCC) $(ARM_CFLAGS) -c $< -o $@

$(RBUILD)/lvgl/%.o: $(ROOT)/lib/lvgl/src/%.c
	@mkdir -p $(dir $@)
	$(ARMCC) $(ARM_CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) $(ABUILD) $(BBUILD) $(GBUILD) $(SBUILD) $(MBUILD) $(PBUILD) $(RBUILD) \
	       $(TARGET) $(ATARGET) $(BTARGET) $(GTARGET) $(STARGET) $(MTARGET) $(PTARGET) $(RTARGET)
