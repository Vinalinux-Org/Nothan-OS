#!/bin/bash
# deploy_and_flash.sh — build and flash MLO + kernel to SD card.
# SD must already have MLO written at sectors 256/512/768 and kernel at 2048.
# Usage: sudo ./scripts/deploy-and-flash.sh /dev/sdX

set -e

DEVICE=${1:-/dev/sda}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOPDIR="$(dirname "$SCRIPT_DIR")"
MLO="$TOPDIR/bootloader/MLO"
KERNEL="$TOPDIR/nothan-kernel/build/kernel.bin"

echo "==> build bootloader"
make -C "$TOPDIR/bootloader"

echo "==> build kernel"
make -C "$TOPDIR/nothan-kernel"

if [ ! -f "$MLO" ]; then
    echo "error: MLO not found"
    exit 1
fi
if [ ! -f "$KERNEL" ]; then
    echo "error: kernel.bin not found"
    exit 1
fi

echo "==> flash $DEVICE"
dd if="$MLO"    of="$DEVICE" bs=512 seek=256  conv=notrunc status=none
dd if="$MLO"    of="$DEVICE" bs=512 seek=512  conv=notrunc status=none
dd if="$MLO"    of="$DEVICE" bs=512 seek=768  conv=notrunc status=none
dd if="$KERNEL" of="$DEVICE" bs=512 seek=2048 conv=notrunc status=none

sync
echo "done"
