# ==============================================================================
# NothanOS Userspace Application: hello (built by NothCC)
# ==============================================================================

include ../../rules.mk

APP_NAME      = hello
APP_BUILD_DIR = $(BUILD_DIR)/apps/$(APP_NAME)
COMPILER_DIR  = $(abspath $(USERSPACE_ROOT)/../compiler)
NOTHCC         = PYTHONPATH=$(COMPILER_DIR) python3 -m toolchain.main

TARGET_ELF = $(APP_BUILD_DIR)/$(APP_NAME).elf

all: $(APP_BUILD_DIR) $(TARGET_ELF)

$(APP_BUILD_DIR):
	@mkdir -p $(APP_BUILD_DIR)

$(TARGET_ELF): hello.c | $(APP_BUILD_DIR)
	@echo "  NOTHCC   apps/$(APP_NAME)/hello.c"
	@$(NOTHCC) $(abspath hello.c) -o $@

clean:
	@rm -rf $(APP_BUILD_DIR)

.PHONY: all clean
