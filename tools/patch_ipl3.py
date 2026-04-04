#!/usr/bin/env python3
"""
tools/patch_ipl3.py — Assemble ipl3.s and patch it into an N64 ROM file.

Usage:
  python3 tools/patch_ipl3.py <rom.z64>
      Assembles tools/ipl3.s and patches it into the ROM.

  python3 tools/patch_ipl3.py --bin <ipl3.bin> <rom.z64>
      Uses a pre-built IPL3 binary (e.g. extracted from elite_newkind.z64
      via tools/extract_ipl3.py) instead of assembling ipl3.s.
      This is the recommended path — libdragon's IPL3 is recognized by SC64.

Steps:
  1. Get IPL3 bytes: either assemble ipl3.s or read --bin file
  2. Verify the binary fits in 4032 bytes (0x040–0x0FFF)
  3. Patch the binary into the ROM at offset 0x040
"""

import os
import subprocess
import sys
import tempfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT   = os.path.dirname(SCRIPT_DIR)
IPL3_SRC    = os.path.join(SCRIPT_DIR, "ipl3.s")
IPL3_SIZE   = 0xFC0          # 4032 bytes — bytes 0x040–0x0FFF of ROM header
IPL3_OFFSET = 0x040          # byte offset in ROM where IPL3 starts

def assemble_ipl3():
    """Assemble ipl3.s to a raw binary, return bytes."""
    with tempfile.TemporaryDirectory() as tmpdir:
        obj  = os.path.join(tmpdir, "ipl3.o")
        raw  = os.path.join(tmpdir, "ipl3.bin")

        # Assemble: big-endian MIPS III, no ABI calls
        as_cmd = [
            "mipsel-linux-gnu-as",
            "-EB", "-mips3", "-mabi=32", "-G0",
            IPL3_SRC, "-o", obj
        ]
        r = subprocess.run(as_cmd, capture_output=True, text=True)
        if r.returncode:
            print("Assembler error:", r.stderr)
            sys.exit(1)

        # Extract raw binary
        oc_cmd = [
            "mipsel-linux-gnu-objcopy",
            "-O", "binary",
            "--only-section=.text",
            obj, raw
        ]
        r = subprocess.run(oc_cmd, capture_output=True, text=True)
        if r.returncode:
            print("objcopy error:", r.stderr)
            sys.exit(1)

        with open(raw, "rb") as f:
            data = f.read()
    return data

def patch_rom(rom_path, ipl3_bytes):
    if len(ipl3_bytes) > IPL3_SIZE:
        print(f"IPL3 too large: {len(ipl3_bytes)} > {IPL3_SIZE}")
        sys.exit(1)

    # Pad to exactly IPL3_SIZE with NOPs (0x00000000 = NOP in BE MIPS)
    padded = ipl3_bytes + b'\x00' * (IPL3_SIZE - len(ipl3_bytes))

    with open(rom_path, "r+b") as f:
        f.seek(IPL3_OFFSET)
        f.write(padded)

    print(f"IPL3 patched into {rom_path}")
    print(f"  Code size: {len(ipl3_bytes)} bytes  Padded to: {IPL3_SIZE} bytes")

def main():
    args = sys.argv[1:]

    # Parse --bin <file> flag
    bin_path = None
    if "--bin" in args:
        idx = args.index("--bin")
        if idx + 1 >= len(args):
            print("Error: --bin requires a file argument")
            sys.exit(1)
        bin_path = args[idx + 1]
        args = args[:idx] + args[idx + 2:]

    if not args:
        print(f"Usage: {sys.argv[0]} [--bin <ipl3.bin>] <rom.z64>")
        sys.exit(1)

    rom_path = args[0]
    if not os.path.exists(rom_path):
        print(f"ROM not found: {rom_path}")
        sys.exit(1)

    if bin_path:
        if not os.path.exists(bin_path):
            print(f"IPL3 binary not found: {bin_path}")
            sys.exit(1)
        with open(bin_path, "rb") as f:
            ipl3_bytes = f.read()
        print(f"Using pre-built IPL3: {bin_path} ({len(ipl3_bytes)} bytes)")
    else:
        print("Assembling IPL3...")
        ipl3_bytes = assemble_ipl3()
        print(f"IPL3 assembled: {len(ipl3_bytes)} bytes")

    # Verify first instruction looks sane (should be a LUI or similar)
    if len(ipl3_bytes) >= 4:
        first_word = int.from_bytes(ipl3_bytes[:4], 'big')
        print(f"  First instruction: 0x{first_word:08X}")
        if first_word == 0x3044d236:
            print("  (libdragon IPL3 — recognized by SC64)")

    patch_rom(rom_path, ipl3_bytes)

if __name__ == "__main__":
    main()
