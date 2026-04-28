#!/bin/bash
# ============================================================
# deploy_and_flash.sh
# ------------------------------------------------------------
# One-shot build → deploy userspace ELFs onto FAT32 → flash
# MLO + kernel onto the raw sectors.
#
# Usage: ./deploy_and_flash.sh /dev/sda
#   (argument is the SD card block device, not the partition)
# ============================================================

set -e

DEVICE=${1:-/dev/sda}
PART="${DEVICE}1"

# When invoked via sudo, $USER is 'root' but the card is mounted
# under the logged-in user's /media path. Prefer SUDO_USER so the
# copy step finds the right directory either way.
REAL_USER="${SUDO_USER:-$USER}"
MOUNT=/media/$REAL_USER/VINIX

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOPDIR="$(dirname "$SCRIPT_DIR")"

echo "========================================"
echo " 1/3  Build kernel + userspace"
echo "========================================"
make -C "$TOPDIR/userspace"
make -C "$TOPDIR/vinix-kernel/vinix-kernel"

echo ""
echo "========================================"
echo " 2/3  Deploy ELFs onto $MOUNT  (FHS layout)"
echo "========================================"
if [ ! -d "$MOUNT" ]; then
    echo "SD not mounted at $MOUNT — mount it then retry."
    exit 1
fi

# Ensure directory layout: /bin for utilities, /sbin for init copy,
# /etc for config files. Host-side mkdir is fine — the kernel reads
# subdirectories natively after P5.
sudo -u "$REAL_USER" mkdir -p "$MOUNT/bin" "$MOUNT/sbin" "$MOUNT/etc"

BIN_APPS="shell ls cat echo ps kill pwd free uname hello rm mv"
for app in $BIN_APPS; do
    dest=$app
    [ "$app" = "shell" ] && dest=sh
    src="$TOPDIR/userspace/build/apps/$app/$app.elf"
    [ -f "$src" ] || { echo "missing $src"; exit 1; }
    cp "$src" "$MOUNT/bin/$dest"
    echo "  $(printf '%-6s' $app) -> $MOUNT/bin/$dest  ($(stat -c%s "$src") bytes)"
done

INIT_SRC="$TOPDIR/userspace/build/apps/init/init.elf"
if [ -f "$INIT_SRC" ]; then
    cp "$INIT_SRC" "$MOUNT/sbin/init"
    echo "  init   -> $MOUNT/sbin/init   ($(stat -c%s "$INIT_SRC") bytes)"
fi

MOTD="$MOUNT/etc/motd"
if [ ! -f "$MOTD" ]; then
    cat > "$MOTD" <<'MOTDEOF'
VinixOS 0.1 — BeagleBone Black
100% hand-written: kernel, libc, userspace, compiler.
type `help` for built-ins, or run any /bin/<name>.
MOTDEOF
    echo "  motd   -> $MOTD"
fi
sync

echo ""
echo "========================================"
echo " 3/3  Flash MLO + kernel.bin to $DEVICE"
echo "========================================"
# Need to unmount the FAT32 partition before raw-writing the card.
if mountpoint -q "$MOUNT"; then
    udisksctl unmount -b "$PART" || sudo umount "$PART"
fi
"$SCRIPT_DIR/flash_sdcard.sh" "$DEVICE"

echo ""
echo "Eject the card and boot the BBB."
