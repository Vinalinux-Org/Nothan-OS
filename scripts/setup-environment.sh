#!/bin/bash
# setup-environment.sh — install VinixOS build dependencies on Ubuntu 22.04.
# Run from project root with sudo: sudo bash scripts/setup-environment.sh

set -e

echo "Installing VinixOS dependencies..."

apt-get update
apt-get upgrade -y

# Build tools
apt-get install -y build-essential make gcc g++ flex bison git

# Libraries needed if building arm-none-eabi toolchain from source
apt-get install -y libgmp3-dev libmpc-dev libmpfr-dev libisl-dev texinfo

# ARM bare-metal toolchain (kernel + userspace)
apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi

# ARM Linux toolchain (VinCC code generation backend)
apt-get install -y binutils-arm-linux-gnueabihf

# Python (VinCC compiler)
apt-get install -y python3 python3-pip python3-venv

# Serial console
apt-get install -y screen minicom

# Multilib
apt-get install -y gcc-multilib libc6-i386

# SD card tools
apt-get install -y parted dosfstools

echo ""
echo "Installing VinCC Python dependencies..."
if [ -f "compiler/requirements.txt" ]; then
    pip3 install -r compiler/requirements.txt
else
    echo "warning: compiler/requirements.txt not found — run from project root"
fi

echo ""
echo "Done. Next:"
echo "  make                          # build userspace then kernel"
echo "  bash scripts/install_compiler.sh"
