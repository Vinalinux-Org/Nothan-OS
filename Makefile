MAKEFLAGS += --no-print-directory

.PHONY: all nothanos bootloader kernel clean help

all: nothanos

nothanos: bootloader kernel
	@echo ""
	@echo "Build complete"

bootloader:
	@echo "=== Bootloader ==="
	$(MAKE) -C bootloader

kernel:
	@echo "=== Kernel ==="
	$(MAKE) -C nothan-kernel
	@echo ""

clean:
	$(MAKE) -C bootloader clean
	$(MAKE) -C nothan-kernel clean

help:
	@echo "Targets: all, nothanos, bootloader, kernel, clean, help"
	@echo "  make           = bootloader + kernel"
	@echo "  make kernel    = kernel only"
	@echo "  make bootloader = bootloader only"
