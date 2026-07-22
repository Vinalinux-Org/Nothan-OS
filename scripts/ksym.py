#!/usr/bin/env python3
"""
ksym.py - resolve NothanOS crash addresses to function names, offline.

The kernel's exception handler prints a "Call trace" of raw addresses (see
arch/arm/kernel/traps.c). Feed those addresses here to turn them into
function names, using the symbol tables of the built ELFs.

Usage:
    python3 scripts/ksym.py 0xc0012f4c 0x0001a20c
    # or paste the whole "Call trace" block on stdin:
    python3 scripts/ksym.py < crash.log

Kernel addresses (>= 0xC0000000) resolve against kernel.elf; user addresses
resolve against gui.elf. Point at other ELFs with --kernel / --user.
"""

import argparse
import bisect
import os
import re
import subprocess
import sys

NM = os.environ.get("CROSS_COMPILE", "arm-none-eabi-") + "nm"
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KERNEL_ELF = os.path.join(ROOT, "nothan-kernel", "build", "kernel.elf")
GUI_ELF = os.path.join(ROOT, "userspace", "build", "gui.elf")

# nm -n line: "c0012f30 T lv_draw_sw_blend"  (addr may be absent for undefined)
NM_LINE = re.compile(r"^([0-9a-fA-F]+)\s+[a-zA-Z]\s+(\S+)")


def load_syms(elf):
    """Return (addrs, names) sorted by address, or None if the ELF is missing."""
    if not os.path.exists(elf):
        return None
    out = subprocess.run([NM, "-n", elf], capture_output=True, text=True).stdout
    pairs = []
    for line in out.splitlines():
        m = NM_LINE.match(line)
        if m:
            pairs.append((int(m.group(1), 16), m.group(2)))
    pairs.sort()
    return [a for a, _ in pairs], [n for _, n in pairs]


def resolve(addr, table):
    """Greatest symbol whose address <= addr -> 'name+0xoff', or None."""
    if not table:
        return None
    addrs, names = table
    i = bisect.bisect_right(addrs, addr) - 1
    if i < 0:
        return None
    return "%s+0x%x" % (names[i], addr - addrs[i])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("addrs", nargs="*", help="addresses (hex); else read stdin")
    ap.add_argument("--kernel", default=KERNEL_ELF)
    ap.add_argument("--user", default=GUI_ELF)
    args = ap.parse_args()

    kern = load_syms(args.kernel)
    user = load_syms(args.user)
    if kern is None:
        print("warn: no kernel ELF at %s (build first)" % args.kernel, file=sys.stderr)
    if user is None:
        print("warn: no user ELF at %s" % args.user, file=sys.stderr)

    text = " ".join(args.addrs) if args.addrs else sys.stdin.read()
    addrs = [int(t, 16) for t in re.findall(r"0x[0-9a-fA-F]+", text)]
    if not addrs:
        print("no 0x... addresses found", file=sys.stderr)
        return 1

    for a in addrs:
        if a >= 0xC0000000:
            who, name = "kernel", resolve(a, kern)
        else:
            who, name = "gui", resolve(a, user)
        print("0x%08x  %-8s  %s" % (a, who, name or "??"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
