# RefARM-OS Top-Level Makefile
# Build entire project: bootloader + userspace + kernel + CrossCompiler

MAKEFLAGS += --no-print-directory

.PHONY: all clean vinixos bootloader userspace kernel compiler help test

# Default target
all: vinixos compiler
	@echo ""
	@echo "========================================="
	@echo "RefARM-OS Build Complete!"
	@echo "========================================="
	@echo "VinixOS kernel built successfully"
	@echo "VinCC compiler ready to use"
	@echo ""
	@echo "Next steps:"
	@echo "  1. Install compiler: bash scripts/install_compiler.sh"
	@echo "  2. Deploy to SD card: sudo bash scripts/deploy_and_flash.sh /dev/sdX"
	@echo "  3. Connect serial: screen /dev/ttyUSB0 115200"
	@echo ""

# VinixOS = bootloader + userspace + kernel (kernel embeds shell payload, so userspace must precede)
vinixos: bootloader userspace kernel
	@echo "========================================="
	@echo " VinixOS Build Complete                  "
	@echo "========================================="

bootloader:
	@echo "========================================="
	@echo " Building Bootloader                     "
	@echo "========================================="
	$(MAKE) -C bootloader

userspace:
	@echo "========================================="
	@echo " Building Userspace                      "
	@echo "========================================="
	$(MAKE) -C userspace

kernel: userspace
	@echo "========================================="
	@echo " Building Kernel                         "
	@echo "========================================="
	$(MAKE) -C vinix-kernel

# Build/verify compiler
compiler:
	@echo ""
	@echo "========================================="
	@echo "Verifying VinCC Compiler..."
	@echo "========================================="
	@if [ ! -f CrossCompiler/toolchain/main.py ]; then \
		echo "Error: Compiler source not found"; \
		exit 1; \
	fi
	@echo "Compiler source verified"
	@echo "Run 'bash scripts/install_compiler.sh' to install"

# Clean all build artifacts
clean:
	@echo "Cleaning Bootloader..."
	@$(MAKE) -C bootloader clean
	@echo "Cleaning Userspace..."
	@$(MAKE) -C userspace clean
	@echo "Cleaning Kernel..."
	@$(MAKE) -C vinix-kernel clean
	@echo "Cleaning CrossCompiler..."
	@$(MAKE) -C CrossCompiler clean
	@echo "Clean complete"

# Run tests
test: vinixos
	@echo "Running VinixOS tests..."
	@$(MAKE) -C vinix-kernel test
	@echo ""
	@echo "Running Compiler tests..."
	@$(MAKE) -C CrossCompiler test

# Help target
help:
	@echo "RefARM-OS Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build VinixOS and verify compiler (default)"
	@echo "  vinixos    - Build VinixOS (bootloader + userspace + kernel)"
	@echo "  bootloader - Build bootloader only (MLO)"
	@echo "  userspace  - Build userspace only (init, shell, coreutils)"
	@echo "  kernel     - Build kernel only (depends on userspace for embedded shell)"
	@echo "  compiler   - Verify compiler source"
	@echo "  clean      - Clean all build artifacts"
	@echo "  test       - Run test suites"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Quick Start:"
	@echo "  1. make all                              # Build everything"
	@echo "  2. bash scripts/install_compiler.sh      # Install compiler"
	@echo "  3. sudo bash scripts/deploy_and_flash.sh /dev/sdX  # Deploy to SD"
	@echo ""
	@echo "For detailed instructions, see README.md"
