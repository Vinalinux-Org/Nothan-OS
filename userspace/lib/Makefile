# ==============================================================================
# NothanOS Userspace Library (librefarm.a)
# ==============================================================================

include ../rules.mk

# Output targets
LIB_BUILD_DIR = $(BUILD_DIR)/lib
TARGET_LIB    = $(LIB_BUILD_DIR)/librefarm.a
TARGET_CRT0   = $(LIB_BUILD_DIR)/crt0.o

# Source files
SRC_C = syscall.c
SRC_S = crt0.S

# nothanlibc modules — hand-written POSIX subset
NOTHANLIBC_SRC_DIR = ../nothanlibc/src
NOTHANLIBC_SRCS    = string.c ctype.c errno.c printf.c stdlib.c unistd.c \
                    signal.c stat.c file.c

# Object files
OBJ_C     = $(addprefix $(LIB_BUILD_DIR)/, $(SRC_C:.c=.o))
OBJ_S     = $(addprefix $(LIB_BUILD_DIR)/, $(SRC_S:.S=.o))
OBJ_LIBC  = $(addprefix $(LIB_BUILD_DIR)/nothanlibc_, $(NOTHANLIBC_SRCS:.c=.o))
OBJS      = $(OBJ_C) $(OBJ_S) $(OBJ_LIBC)

all: $(TARGET_CRT0) $(TARGET_LIB)

$(LIB_BUILD_DIR):
	@mkdir -p $(LIB_BUILD_DIR)

# crt0 must be kept separate because the linker needs it as the ENTRY object
$(TARGET_CRT0): $(SRC_S) | $(LIB_BUILD_DIR)
	@echo "  CC      lib/$(SRC_S)"
	@$(CC) $(CFLAGS) -c $< -o $@

$(LIB_BUILD_DIR)/%.o: %.c | $(LIB_BUILD_DIR)
	@echo "  CC      lib/$<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(LIB_BUILD_DIR)/nothanlibc_%.o: $(NOTHANLIBC_SRC_DIR)/%.c | $(LIB_BUILD_DIR)
	@echo "  CC      nothanlibc/src/$*.c"
	@$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LIB): $(OBJ_C) $(OBJ_LIBC)
	@echo "  AR      $(TARGET_LIB)"
	@$(AR) rcs $@ $^

clean:
	@rm -rf $(LIB_BUILD_DIR)

.PHONY: all clean
