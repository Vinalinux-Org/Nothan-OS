#!/bin/bash
# deploy_and_flash.sh — build, update rootfs ELFs, flash MLO + kernel.
# Requires SD card already initialized by setup_sdcard.sh.
# Usage: sudo ./scripts/deploy_and_flash.sh /dev/sdX

set -e

DEVICE=${1:-/dev/sda}
PART="${DEVICE}1"

REAL_USER="${SUDO_USER:-$USER}"
MOUNT=/media/$REAL_USER/NOTHAN

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOPDIR="$(dirname "$SCRIPT_DIR")"
MLO="$TOPDIR/bootloader/MLO"
KERNEL="$TOPDIR/nothan-kernel/build/kernel.bin"

echo "==> build"
make -C "$TOPDIR/bootloader"
make -C "$TOPDIR/userspace"
make -C "$TOPDIR/nothan-kernel"

echo "==> deploy to $MOUNT"
MOUNTED_BY_US=0
if ! mountpoint -q "$MOUNT" 2>/dev/null; then
    if ! lsblk -no LABEL "$PART" 2>/dev/null | grep -q "^NOTHAN$"; then
        echo "error: $PART is not a NOTHAN partition — run setup-sdcard.sh first"
        exit 1
    fi
    mkdir -p "$MOUNT"
    mount "$PART" "$MOUNT"
    MOUNTED_BY_US=1
fi

mkdir -p "$MOUNT/bin" "$MOUNT/sbin" "$MOUNT/etc"
# Wipe stale binaries from previous flash before copying.
rm -f "$MOUNT/bin/"* "$MOUNT/sbin/"*

BIN_APPS="shell ls cat echo ps kill pwd free uname hello rm mv"
for app in $BIN_APPS; do
    dest=$app
    [ "$app" = "shell" ] && dest=sh
    src="$TOPDIR/userspace/build/apps/$app/$app.elf"
    [ -f "$src" ] || { echo "error: missing $src"; exit 1; }
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

echo "==> flash $DEVICE"
# Unmount FAT32 before raw-writing raw sectors.
if mountpoint -q "$MOUNT"; then
    udisksctl unmount -b "$PART" 2>/dev/null || umount "$PART"
    [ "$MOUNTED_BY_US" -eq 1 ] && rmdir "$MOUNT" 2>/dev/null || true
fi

if [ ! -f "$MLO" ]; then
    echo "error: MLO not found — run: make -C $TOPDIR bootloader"
    exit 1
fi
if [ ! -f "$KERNEL" ]; then
    echo "error: kernel.bin not found — run: make -C $TOPDIR/nothan-kernel"
    exit 1
fi

# AM335x ROM expects MLO at sectors 256, 512, 768 (TRM 26.1.8.5.5).
dd if="$MLO"    of="$DEVICE" bs=512 seek=256  conv=notrunc status=none
dd if="$MLO"    of="$DEVICE" bs=512 seek=512  conv=notrunc status=none
dd if="$MLO"    of="$DEVICE" bs=512 seek=768  conv=notrunc status=none
dd if="$KERNEL" of="$DEVICE" bs=512 seek=2048 conv=notrunc status=none

sync
echo "done — eject and boot BBB."
