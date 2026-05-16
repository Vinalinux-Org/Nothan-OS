#!/bin/bash
# setup_sdcard.sh — first-time SD card initialization for NothanOS.
#
# SD layout:
#   sector 256, 512, 768 — MLO (raw, AM335x ROM requirement)
#   sector 2048           — kernel.bin (raw, ~3MB budget before FAT32)
#   sector 8192+          — FAT32 partition, label NOTHAN (rootfs)
#
# WARNING: this ERASES the entire device. Run once per card.
# Usage: sudo ./scripts/setup_sdcard.sh /dev/sdX

set -e

DEVICE=$1

if [ -z "$DEVICE" ]; then
    echo "Usage: $0 <device>  e.g. $0 /dev/sdb"
    exit 1
fi

if [ ! -b "$DEVICE" ]; then
    echo "error: $DEVICE is not a block device"
    exit 1
fi

if [ "$EUID" -ne 0 ]; then
    echo "error: run as root (sudo)"
    exit 1
fi

echo "WARNING: this will erase $DEVICE completely."
read -r -p "Type YES to continue: " confirm
[ "$confirm" = "YES" ] || { echo "aborted."; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOPDIR="$(dirname "$SCRIPT_DIR")"
MLO="$TOPDIR/bootloader/MLO"
KERNEL="$TOPDIR/nothan-kernel/build/kernel.bin"
PART="${DEVICE}1"

if [ ! -f "$MLO" ]; then
    echo "error: MLO not found — run: make -C $TOPDIR bootloader"
    exit 1
fi
if [ ! -f "$KERNEL" ]; then
    echo "error: kernel.bin not found — run: make -C $TOPDIR/nothan-kernel"
    exit 1
fi

# Unmount any mounted partitions on this device.
for mp in $(lsblk -lno MOUNTPOINT "${DEVICE}"* 2>/dev/null | grep -v '^$'); do
    umount "$mp" && echo "unmounted $mp"
done

echo "==> partition"
parted -s "$DEVICE" mklabel msdos
# FAT32 partition from 4MB (sector 8192) to end of card.
parted -s "$DEVICE" mkpart primary fat32 8192s 100%
partprobe "$DEVICE"
sleep 1

echo "==> format FAT32"
mkfs.vfat -F 32 -n NOTHAN "$PART"

echo "==> flash MLO + kernel"
# MLO at three redundant locations (AM335x TRM 26.1.8.5.5).
dd if="$MLO"    of="$DEVICE" bs=512 seek=256  conv=notrunc status=none
dd if="$MLO"    of="$DEVICE" bs=512 seek=512  conv=notrunc status=none
dd if="$MLO"    of="$DEVICE" bs=512 seek=768  conv=notrunc status=none
dd if="$KERNEL" of="$DEVICE" bs=512 seek=2048 conv=notrunc status=none

echo "==> populate rootfs"
REAL_USER="${SUDO_USER:-$USER}"
MOUNT=$(mktemp -d)
mount "$PART" "$MOUNT"

mkdir -p "$MOUNT/bin" "$MOUNT/sbin" "$MOUNT/etc"

BIN_APPS="shell ls cat echo ps kill pwd free uname hello rm mv"
for app in $BIN_APPS; do
    dest=$app
    [ "$app" = "shell" ] && dest=sh
    src="$TOPDIR/userspace/build/apps/$app/$app.elf"
    [ -f "$src" ] || { echo "warning: missing $src, skipping"; continue; }
    cp "$src" "$MOUNT/bin/$dest"
    echo "  $app -> /bin/$dest  ($(stat -c%s "$src") bytes)"
done

INIT_SRC="$TOPDIR/userspace/build/apps/init/init.elf"
if [ -f "$INIT_SRC" ]; then
    cp "$INIT_SRC" "$MOUNT/sbin/init"
    echo "  init -> /sbin/init  ($(stat -c%s "$INIT_SRC") bytes)"
fi

cat > "$MOUNT/etc/motd" <<'EOF'
NothanOS 0.1 — BeagleBone Black
100% hand-written: kernel, libc, userspace, compiler.
type `help` for built-ins, or run any /bin/<name>.
EOF

sync
umount "$MOUNT"
rmdir "$MOUNT"

echo "done — eject and boot BBB."
