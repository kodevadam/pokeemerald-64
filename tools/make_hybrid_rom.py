#!/usr/bin/env python3
"""
tools/make_hybrid_rom.py — Inject miniboot boot code into elite's ROM structure.

Takes elite_newkind.z64 (known working) and replaces ROM[0x1000..0x1400+bootsize]
with our miniboot boot code. Everything else (header, IPL3, ROM size) stays
exactly as elite's ROM.

If this hybrid boots → our miniboot's header/structure is wrong.
If this hybrid stays black → our boot code itself has a bug.

Usage:
    python3 tools/make_hybrid_rom.py <elite.z64> <miniboot.z64> <output.z64>

Example:
    python3 tools/make_hybrid_rom.py ~/Downloads/elite_newkind.z64 test/miniboot.z64 test/hybrid.z64
"""

import sys
import os

def main():
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <elite.z64> <miniboot.z64> <output.z64>")
        sys.exit(1)

    elite_path   = sys.argv[1]
    miniboot_path = sys.argv[2]
    output_path  = sys.argv[3]

    if not os.path.exists(elite_path):
        print(f"Error: elite ROM not found: {elite_path}")
        sys.exit(1)
    if not os.path.exists(miniboot_path):
        print(f"Error: miniboot ROM not found: {miniboot_path}")
        sys.exit(1)

    with open(elite_path, 'rb') as f:
        elite = bytearray(f.read())
    with open(miniboot_path, 'rb') as f:
        miniboot = bytearray(f.read())

    # Verify both are z64 format (magic 0x80371240)
    for name, data in [('elite', elite), ('miniboot', miniboot)]:
        if data[:4] != bytes([0x80, 0x37, 0x12, 0x40]):
            print(f"Warning: {name} ROM doesn't start with z64 magic (got {data[:4].hex()})")

    print(f"Elite ROM:    {len(elite)} bytes")
    print(f"Miniboot ROM: {len(miniboot)} bytes")

    # Extract elite's header (0x00-0x3F) and IPL3 (0x40-0xFFF)
    print()
    print(f"Elite header[0x00-0x3F]:  kept as-is")
    print(f"  clock rate: 0x{int.from_bytes(elite[4:8], 'big'):08X}")
    print(f"  entry point: 0x{int.from_bytes(elite[8:12], 'big'):08X}")
    print(f"  byte 0x3F: 0x{elite[0x3F]:02X}")
    print(f"Elite IPL3 first word:    0x{int.from_bytes(elite[0x40:0x44], 'big'):08X}")

    # Build hybrid ROM:
    # - bytes 0x0000..0x0FFF: from elite (header + IPL3)
    # - bytes 0x1000..0x13FF: zeros (exception vector pad, from miniboot)
    # - bytes 0x1400..: boot code from miniboot
    # - rest of ROM beyond boot code: zeros (elite's data kept after that is
    #   irrelevant since the boot code halts, but we need the file to be big
    #   enough for the IPL3 DMA = 1MB from offset 0x1000)

    output = bytearray(len(elite))  # same size as elite

    # Copy elite's header + IPL3 (0x0000..0x0FFF)
    output[0x0000:0x1000] = elite[0x0000:0x1000]

    # Inject miniboot's content from 0x1000 onwards
    # (exception vectors + boot code)
    boot_region = miniboot[0x1000:]
    copy_len = min(len(boot_region), len(output) - 0x1000)
    output[0x1000:0x1000 + copy_len] = boot_region[:copy_len]

    # If miniboot's boot region was shorter than elite's, zero-fill the rest
    if copy_len < len(output) - 0x1000:
        output[0x1000 + copy_len:] = bytes(len(output) - 0x1000 - copy_len)

    # Verify the hybrid
    boot_code_start_offset = 0x1400
    first_instr = int.from_bytes(output[boot_code_start_offset:boot_code_start_offset+4], 'big')
    print()
    print(f"Hybrid ROM:   {len(output)} bytes")
    print(f"  header/IPL3: from elite")
    print(f"  boot code first instruction at 0x{boot_code_start_offset:04X}: 0x{first_instr:08X}")
    # lui $t0, 0xA440 = 0x3C08A440
    if first_instr == 0x3C08A440:
        print(f"  (looks correct: lui $t0, 0xA440 = VI base address)")
    else:
        print(f"  WARNING: expected 0x3C08A440 (lui $t0, 0xA440)")

    with open(output_path, 'wb') as f:
        f.write(output)

    print()
    print(f"Written: {output_path}")
    print()
    print("To test:")
    print(f"  sc64deployer upload {output_path}")
    print()
    print("If RED: our miniboot header/structure was wrong (elite's header fixes it)")
    print("If BLACK: our boot code itself has a bug")

if __name__ == "__main__":
    main()
