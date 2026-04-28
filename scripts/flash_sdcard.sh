#!/bin/bash
# ============================================================
# flash_sdcard.sh
# ------------------------------------------------------------
# Update MLO + Kernel on existing SD card (RAW mode).
# Does NOT reformat — only overwrites raw sectors.
#
# Usage: ./flash_sdcard.sh /dev/sdX
# ============================================================

DEVICE=$1

if [ -z "$DEVICE" ]; then
    echo "Usage: $0 <device>"
    echo "Example: $0 /dev/sdb"
    exit 1
fi

if ! sudo blockdev --getsz "$DEVICE" >/dev/null 2>&1; then
    echo "Error: Device $DEVICE is not a valid block device or requires root permissions."
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOPDIR="$(dirname "$SCRIPT_DIR")"
MLO="$TOPDIR/vinix-kernel/bootloader/MLO"
KERNEL="$TOPDIR/vinix-kernel/kernel/build/kernel.bin"

echo "========================================"
echo " Update MLO + Kernel (RAW mode)"
echo "========================================"
echo "Device: $DEVICE"

if [ ! -f "$MLO" ]; then
    echo "Error: MLO not found. Run: make -C $TOPDIR/VinixOS bootloader"
    exit 1
fi
if [ ! -f "$KERNEL" ]; then
    echo "Error: kernel.bin not found. Run: make -C $TOPDIR/VinixOS kernel"
    exit 1
fi

echo "MLO:    $(stat -c%s "$MLO") bytes"
echo "Kernel: $(stat -c%s "$KERNEL") bytes"
echo ""

# MLO at RAW offsets (TRM 26.1.8.5.5)
echo "Writing MLO at sectors 256, 512, 768..."
sudo dd if="$MLO" of="$DEVICE" bs=512 seek=256 conv=notrunc status=none
sudo dd if="$MLO" of="$DEVICE" bs=512 seek=512 conv=notrunc status=none
sudo dd if="$MLO" of="$DEVICE" bs=512 seek=768 conv=notrunc status=none

# Kernel at sector 2048
echo "Writing Kernel at sector 2048..."
sudo dd if="$KERNEL" of="$DEVICE" bs=512 seek=2048 conv=notrunc status=none

sudo sync
echo ""
echo "Done. MLO (x3) + Kernel updated."
