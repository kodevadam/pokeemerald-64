#!/usr/bin/env python3
"""
tools/extract_ipl3.py — Extract the IPL3 boot code from a known-good N64 ROM.

Usage: python3 tools/extract_ipl3.py <source_rom.z64> [output.bin]

Extracts bytes 0x040–0x0FFF (4032 bytes) from the source ROM and saves them
as a raw binary.  Feed the output to patch_ipl3.py via the --bin flag to use
a proven IPL3 (e.g. libdragon's open-source one from Elite: The New Kind)
instead of assembling tools/ipl3.s.

Libdragon's IPL3 is BSD-licensed and freely distributable:
  https://github.com/DragonMinded/libdragon
"""

import sys, os

IPL3_OFFSET = 0x040
IPL3_SIZE   = 0xFC0   # 4032 bytes

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <source_rom.z64> [output.bin]")
        sys.exit(1)

    src  = sys.argv[1]
    dest = sys.argv[2] if len(sys.argv) > 2 else "tools/ipl3_libdragon.bin"

    with open(src, "rb") as f:
        magic = f.read(4)
        if magic[0] != 0x80:
            print(f"WARNING: ROM magic {magic.hex()} is not z64 (expected 0x80...).")
            print("         Make sure this is a big-endian z64 ROM.")

        f.seek(IPL3_OFFSET)
        ipl3 = f.read(IPL3_SIZE)

    if len(ipl3) != IPL3_SIZE:
        print(f"Error: ROM too small to contain IPL3 (got {len(ipl3)} bytes)")
        sys.exit(1)

    # Sanity: first word should be a non-zero MIPS instruction
    first_word = int.from_bytes(ipl3[:4], 'big')
    if first_word == 0:
        print("WARNING: first IPL3 word is 0x00000000 — IPL3 may be zeroed.")
    else:
        print(f"IPL3 first instruction: 0x{first_word:08X}")

    os.makedirs(os.path.dirname(dest) if os.path.dirname(dest) else '.', exist_ok=True)
    with open(dest, "wb") as f:
        f.write(ipl3)

    print(f"Extracted {IPL3_SIZE} bytes → {dest}")
    print(f"Now run:  python3 tools/patch_ipl3.py --bin {dest} <rom.z64>")

if __name__ == "__main__":
    main()
