#!/usr/bin/env python3
"""
gp_header.py
Generate MLO boot image for AM335x ROM code (GP device, MMC RAW mode)

Output: MLO = TOC (512B) + GP header (8B) + executable code

Written to raw SD card sectors via dd. ROM finds TOC with CHSETTINGS
at fixed offsets (0x20000, 0x40000, 0x60000), reads GP header from
next sector, loads code to SRAM and executes.

TRM references:
  - Section 26.1.8.5.5: MMC/SD Raw Mode
  - Section 26.1.11: Table of Contents (TOC)
  - Table 26-37: GP Device Image Format
  - Table 26-38: TOC Item Fields
  - Table 26-39: Magic Values for MMC RAW Mode
"""

import struct
import sys
import os

# Load address in SRAM (TRM sec 26.1.4.2)
LOAD_ADDR = 0x402F0400

# Max image size (TRM sec 26.1.9.2)
MAX_SIZE = 109 * 1024


def make_toc():
    """
    Create 512-byte TOC with CHSETTINGS (TRM Table 26-38, 26-39).

    @return  512-byte TOC block
    """
    toc = bytearray(512)

    # TOC Item 1 — CHSETTINGS
    toc[0x00:0x04] = struct.pack('<I', 0x00000040)   # Start
    toc[0x04:0x08] = struct.pack('<I', 0x0000000C)   # Size
    toc[0x14:0x20] = b'CHSETTINGS\x00\x00'           # Filename

    # TOC Item 2 — terminator
    toc[0x20:0x40] = b'\xFF' * 32

    # CHSETTINGS magic values
    toc[0x40:0x48] = struct.pack('<II', 0xC0C0C0C1, 0x00000100)

    return bytes(toc)


def main():
    if len(sys.argv) != 3:
        print("Usage: gp_header.py <input.bin> <output.MLO>")
        return 1

    input_bin = sys.argv[1]
    output_mlo = sys.argv[2]

    if not os.path.exists(input_bin):
        print(f"Error: Input file '{input_bin}' not found")
        return 1

    with open(input_bin, 'rb') as f:
        code = f.read()

    code_size = len(code)
    if code_size > MAX_SIZE:
        print(f"Error: Image ({code_size} bytes) exceeds SRAM limit ({MAX_SIZE} bytes)")
        return 1

    toc = make_toc()
    gp_header = struct.pack('<II', code_size, LOAD_ADDR)

    with open(output_mlo, 'wb') as f:
        f.write(toc)
        f.write(gp_header)
        f.write(code)

    total = len(toc) + len(gp_header) + code_size
    print(f"Generated {output_mlo}: {total} bytes (512 TOC + 8 GP + {code_size} code)")
    print(f"  Load address: 0x{LOAD_ADDR:08X}")
    print(f"  SRAM usage:   {code_size}/{MAX_SIZE} bytes ({code_size * 100 // MAX_SIZE}%)")

    return 0


if __name__ == '__main__':
    sys.exit(main())
