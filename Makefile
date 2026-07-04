MAKEFLAGS += --no-print-directory

.PHONY: all nothanos bootloader kernel userspace clean help

all: nothanos

nothanos: bootloader userspace kernel
	@echo ""
	@echo "Build complete"

bootloader:
	@echo "=== Bootloader ==="
	$(MAKE) -C bootloader

userspace:
	@echo "=== Userspace ==="
	$(MAKE) -C userspace

kernel:
	@echo "=== Kernel ==="
	$(MAKE) -C nothan-kernel
	@echo ""

clean:
	$(MAKE) -C bootloader clean
	$(MAKE) -C nothan-kernel clean
	$(MAKE) -C userspace clean

help:
	@echo "Targets: all, nothanos, bootloader, kernel, userspace, clean, help"
	@echo "  make           = bootloader + userspace + kernel"
	@echo "  make kernel    = kernel only"
	@echo "  make userspace = userspace only"
	@echo "  make bootloader = bootloader only"
