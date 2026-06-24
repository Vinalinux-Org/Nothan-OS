#!/bin/bash
# deploy-and-flash.sh — build and flash MLO + kernel + userspace to SD card.
# Usage: sudo ./scripts/deploy-and-flash.sh /dev/sdX

set -e

DEVICE=${1:-/dev/sda}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOPDIR="$(dirname "$SCRIPT_DIR")"
MLO="$TOPDIR/bootloader/MLO"
KERNEL="$TOPDIR/nothan-kernel/build/kernel.bin"
USPACE="$TOPDIR/userspace/build"
PART="${DEVICE}1"

echo "==> build bootloader"
make -C "$TOPDIR/bootloader"

echo "==> build kernel"
make -C "$TOPDIR/nothan-kernel"

echo "==> build userspace"
make -C "$TOPDIR/userspace"

if [ ! -f "$MLO" ];    then echo "error: MLO not found";    exit 1; fi
if [ ! -f "$KERNEL" ]; then echo "error: kernel.bin not found"; exit 1; fi

echo "==> flash $DEVICE"
dd if="$MLO"    of="$DEVICE" bs=512 seek=256  conv=notrunc status=none
dd if="$MLO"    of="$DEVICE" bs=512 seek=512  conv=notrunc status=none
dd if="$MLO"    of="$DEVICE" bs=512 seek=768  conv=notrunc status=none
dd if="$KERNEL" of="$DEVICE" bs=512 seek=2048 conv=notrunc status=none

if [ ! -b "$PART" ]; then
    echo "warning: $PART not found, skipping userspace copy"
    sync
    echo "done"
    exit 0
fi

echo "==> update userspace on $PART"
MOUNT=$(mktemp -d)
mount "$PART" "$MOUNT"

# All process binaries are embedded in the kernel image.
# FAT partition holds only persistent data (contacts, messages, etc.)
echo "  (no userspace binaries — all embedded in kernel)"

sync
umount "$MOUNT"
rmdir "$MOUNT"

echo "done"
