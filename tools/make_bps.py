#!/usr/bin/env python3
"""
make_bps.py -- build a BPS patch from a retail Pokemon Emerald (GBA) ROM to
the N64 port's .z64.

    python3 tools/make_bps.py pokeemerald.gba build/n64/pokeemerald64.z64 \
            web/pokeemerald64.bps

The patch is what the web patcher (web/index.html) ships, so that people
reconstruct the .z64 from a ROM they already own rather than downloading a
built one.

A word on what this can and cannot do: the N64 build is a recompilation, not
a modified GBA binary. Its MIPS code, its pointer tables and its byte-swapped
script data have no counterpart in the GBA ROM, so the patch has to carry all
of that itself. What it *can* pull from the source ROM is the bulk asset data
-- compressed graphics, tilesets, palettes, map blocks, text -- which the two
builds embed byte-for-byte identically. Expect the patch to come out somewhere
around half the size of the .z64; run with --stats to see the actual split.

BPS format reference: https://www.romhacking.net/documents/746/
"""

import argparse
import hashlib
import os
import struct
import sys
import zlib

SOURCE_READ = 0
TARGET_READ = 1
SOURCE_COPY = 2
TARGET_COPY = 3

# Retail Pokemon Emerald (USA), the ROM this repo's GBA build reproduces.
EMERALD_USA_SHA1 = "f3ae088181bf583e55daf962a92bb46f4f1d07b7"

# Hash window for the match index. Long enough that a hit is almost certainly a
# real match rather than a coincidence, short enough to catch modest blobs.
BLOCK = 16
# Index every Nth position instead of every position: a 16 MB source indexed at
# every byte needs gigabytes of dict. Any common run of >= 2*STEP bytes still
# gets found, because some indexed source position must fall inside it.
STEP = 16
# Below this a copy action costs more to encode than the bytes it saves.
MIN_MATCH = 16


def encode_varint(n):
    """BPS variable-width number: 7 bits per byte, high bit marks the last."""
    out = bytearray()
    while True:
        x = n & 0x7F
        n >>= 7
        if n == 0:
            out.append(0x80 | x)
            break
        out.append(x)
        n -= 1
    return bytes(out)


def build_index(buf):
    """Map the first BLOCK bytes at every STEPth position to that position."""
    index = {}
    limit = len(buf) - BLOCK
    pos = 0
    while pos <= limit:
        index.setdefault(buf[pos:pos + BLOCK], pos)
        pos += STEP
    return index


def match_forward(a, a_pos, b, b_pos, limit):
    """Length of the common prefix of a[a_pos:] and b[b_pos:], capped at limit."""
    limit = min(limit, len(a) - a_pos, len(b) - b_pos)
    matched = 0
    # Compare in chunks; byte-at-a-time over megabytes is far too slow in Python.
    while matched < limit:
        span = min(4096, limit - matched)
        chunk_a = a[a_pos + matched:a_pos + matched + span]
        chunk_b = b[b_pos + matched:b_pos + matched + span]
        if chunk_a == chunk_b:
            matched += span
            continue
        for i in range(span):
            if chunk_a[i] != chunk_b[i]:
                return matched + i
        return matched + span
    return matched


def match_backward(a, a_pos, b, b_pos, limit):
    """How far back a[:a_pos] and b[:b_pos] agree, capped at limit."""
    limit = min(limit, a_pos, b_pos)
    matched = 0
    while matched < limit and a[a_pos - matched - 1] == b[b_pos - matched - 1]:
        matched += 1
    return matched


def create_patch(source, target, metadata=b"", progress=None):
    source_index = build_index(source)
    target_index = {}

    actions = bytearray()
    literals = bytearray()

    source_relative_offset = 0
    target_relative_offset = 0
    output_offset = 0
    target_len = len(target)

    stats = {"source_copy": 0, "target_copy": 0, "source_read": 0, "literal": 0}

    def flush_literals():
        if not literals:
            return
        actions.extend(encode_varint(((len(literals) - 1) << 2) | TARGET_READ))
        actions.extend(literals)
        stats["literal"] += len(literals)
        literals.clear()

    next_report = 0
    while output_offset < target_len:
        if progress and output_offset >= next_report:
            progress(output_offset, target_len)
            next_report = output_offset + (target_len // 100 or 1)

        best_len = 0
        best_mode = None
        best_pos = 0

        # SourceRead: the same offset in both files. Costs nothing to encode
        # beyond the length, so it wins ties.
        if output_offset < len(source):
            n = match_forward(target, output_offset, source, output_offset,
                              target_len - output_offset)
            if n >= MIN_MATCH:
                best_len, best_mode = n, SOURCE_READ

        key = bytes(target[output_offset:output_offset + BLOCK])
        if len(key) == BLOCK:
            # SourceCopy: the same bytes somewhere else in the source ROM.
            sp = source_index.get(key)
            if sp is not None:
                n = match_forward(target, output_offset, source, sp,
                                  target_len - output_offset)
                if n > best_len:
                    best_len, best_mode, best_pos = n, SOURCE_COPY, sp

            # TargetCopy: bytes we have already written. This is what keeps
            # repetitive regions (zero fill, similar tables) from being spelled
            # out twice.
            tp = target_index.get(key)
            if tp is not None and tp < output_offset:
                n = match_forward(target, output_offset, target, tp,
                                  min(target_len - output_offset,
                                      output_offset - tp))
                if n > best_len:
                    best_len, best_mode, best_pos = n, TARGET_COPY, tp

        if best_mode is None or best_len < MIN_MATCH:
            literals.append(target[output_offset])
            if len(target) - output_offset >= BLOCK:
                target_index.setdefault(
                    bytes(target[output_offset:output_offset + BLOCK]),
                    output_offset)
            output_offset += 1
            continue

        # Reclaim any pending literal bytes the match also covers. Without this
        # we would lose up to STEP bytes off the front of every match, since the
        # index only knows about every STEPth source position.
        if best_mode in (SOURCE_COPY, TARGET_COPY) and literals:
            other = source if best_mode == SOURCE_COPY else target
            back = match_backward(target, output_offset, other, best_pos,
                                  len(literals))
            if back:
                del literals[len(literals) - back:]
                output_offset -= back
                best_pos -= back
                best_len += back

        flush_literals()

        actions.extend(encode_varint(((best_len - 1) << 2) | best_mode))
        if best_mode == SOURCE_COPY:
            delta = best_pos - source_relative_offset
            actions.extend(encode_varint((abs(delta) << 1) | (1 if delta < 0 else 0)))
            source_relative_offset = best_pos + best_len
            stats["source_copy"] += best_len
        elif best_mode == TARGET_COPY:
            delta = best_pos - target_relative_offset
            actions.extend(encode_varint((abs(delta) << 1) | (1 if delta < 0 else 0)))
            target_relative_offset = best_pos + best_len
            stats["target_copy"] += best_len
        else:
            stats["source_read"] += best_len

        # Index the region we just emitted so later TargetCopy actions can see it.
        end = output_offset + best_len
        pos = output_offset - (output_offset % STEP) + STEP
        while pos + BLOCK <= end:
            target_index.setdefault(bytes(target[pos:pos + BLOCK]), pos)
            pos += STEP

        output_offset = end

    flush_literals()

    patch = bytearray(b"BPS1")
    patch.extend(encode_varint(len(source)))
    patch.extend(encode_varint(len(target)))
    patch.extend(encode_varint(len(metadata)))
    patch.extend(metadata)
    patch.extend(actions)
    patch.extend(struct.pack("<I", zlib.crc32(source) & 0xFFFFFFFF))
    patch.extend(struct.pack("<I", zlib.crc32(target) & 0xFFFFFFFF))
    patch.extend(struct.pack("<I", zlib.crc32(bytes(patch)) & 0xFFFFFFFF))

    return bytes(patch), stats


def decode_varint(data, pos):
    value = 0
    shift = 1
    while True:
        x = data[pos]
        pos += 1
        value += (x & 0x7F) * shift
        if x & 0x80:
            return value, pos
        shift <<= 7
        value += shift


def apply_patch(patch, source):
    """Reference decoder -- mirrors what the browser does, used by --verify."""
    if patch[:4] != b"BPS1":
        raise ValueError("not a BPS patch")
    if zlib.crc32(patch[:-4]) & 0xFFFFFFFF != struct.unpack("<I", patch[-4:])[0]:
        raise ValueError("patch is corrupt (checksum mismatch)")

    pos = 4
    source_size, pos = decode_varint(patch, pos)
    target_size, pos = decode_varint(patch, pos)
    metadata_size, pos = decode_varint(patch, pos)
    pos += metadata_size

    if len(source) != source_size:
        raise ValueError(f"source is {len(source)} bytes, patch expects {source_size}")
    expected_source_crc = struct.unpack("<I", patch[-12:-8])[0]
    if zlib.crc32(source) & 0xFFFFFFFF != expected_source_crc:
        raise ValueError("source ROM does not match the one this patch was built from")

    target = bytearray(target_size)
    output_offset = 0
    source_relative_offset = 0
    target_relative_offset = 0
    end_of_actions = len(patch) - 12

    while pos < end_of_actions:
        data, pos = decode_varint(patch, pos)
        action = data & 3
        length = (data >> 2) + 1

        if action == SOURCE_READ:
            target[output_offset:output_offset + length] = \
                source[output_offset:output_offset + length]
            output_offset += length
        elif action == TARGET_READ:
            target[output_offset:output_offset + length] = patch[pos:pos + length]
            pos += length
            output_offset += length
        elif action == SOURCE_COPY:
            raw, pos = decode_varint(patch, pos)
            offset = (raw >> 1) * (-1 if raw & 1 else 1)
            source_relative_offset += offset
            target[output_offset:output_offset + length] = \
                source[source_relative_offset:source_relative_offset + length]
            source_relative_offset += length
            output_offset += length
        else:
            raw, pos = decode_varint(patch, pos)
            offset = (raw >> 1) * (-1 if raw & 1 else 1)
            target_relative_offset += offset
            # Byte-at-a-time: a TargetCopy may legitimately overlap itself.
            for _ in range(length):
                target[output_offset] = target[target_relative_offset]
                target_relative_offset += 1
                output_offset += 1

    expected_target_crc = struct.unpack("<I", patch[-8:-4])[0]
    if zlib.crc32(target) & 0xFFFFFFFF != expected_target_crc:
        raise ValueError("patched output failed its checksum")
    return bytes(target)


def human(n):
    return f"{n / (1024 * 1024):.2f} MB" if n >= 1024 * 1024 else f"{n / 1024:.1f} KB"


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source", help="retail Pokemon Emerald (USA) .gba")
    ap.add_argument("target", help="the built .z64")
    ap.add_argument("output", help="patch to write")
    ap.add_argument("--stats", action="store_true",
                    help="report how much of the .z64 came from the source ROM")
    ap.add_argument("--gzip", action="store_true",
                    help="gzip the patch (the web patcher inflates it transparently). "
                         "Worth it: what the patch carries is mostly MIPS code, which "
                         "compresses to around 40%%.")
    ap.add_argument("--verify", action="store_true", default=True,
                    help="apply the patch back and check it reproduces the target")
    ap.add_argument("--no-verify", dest="verify", action="store_false")
    ap.add_argument("--allow-any-source", action="store_true",
                    help="skip the Emerald (USA) SHA-1 check on the source ROM")
    args = ap.parse_args()

    source = open(args.source, "rb").read()
    target = open(args.target, "rb").read()

    digest = hashlib.sha1(source).hexdigest()
    if digest != EMERALD_USA_SHA1 and not args.allow_any_source:
        print(f"error: {args.source} has sha1 {digest}", file=sys.stderr)
        print(f"       expected {EMERALD_USA_SHA1} (Pokemon Emerald, USA)",
              file=sys.stderr)
        print("       pass --allow-any-source to build a patch anyway",
              file=sys.stderr)
        return 1

    def progress(done, total):
        pct = 100 * done // total
        print(f"\r  matching... {pct:3d}%", end="", file=sys.stderr, flush=True)

    print(f"source {args.source}  {human(len(source))}  sha1 {digest}")
    print(f"target {args.target}  {human(len(target))}")

    patch, stats = create_patch(source, target, progress=progress)
    print("\r  matching... done ", file=sys.stderr)

    if args.verify:
        rebuilt = apply_patch(patch, source)
        if rebuilt != target:
            print("error: patch did not reproduce the target", file=sys.stderr)
            return 1
        print("verified: patch reproduces the target byte for byte")

    out_path = args.output
    blob = patch
    if args.gzip:
        import gzip as _gzip
        if not out_path.endswith(".gz"):
            out_path += ".gz"
        blob = _gzip.compress(patch, 9)

    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, "wb") as f:
        f.write(blob)

    if args.gzip:
        print(f"wrote  {out_path}  {human(len(blob))}  "
              f"({human(len(patch))} uncompressed)")
    else:
        print(f"wrote  {out_path}  {human(len(patch))}")

    if args.stats:
        total = len(target)
        from_source = stats["source_copy"] + stats["source_read"]
        print()
        print("where the .z64's bytes come from:")
        print(f"  reused from your GBA ROM   {human(from_source):>10}"
              f"  {100 * from_source / total:5.1f}%")
        print(f"  repeated within the .z64   {human(stats['target_copy']):>10}"
              f"  {100 * stats['target_copy'] / total:5.1f}%")
        print(f"  carried by the patch       {human(stats['literal']):>10}"
              f"  {100 * stats['literal'] / total:5.1f}%")

    return 0


if __name__ == "__main__":
    sys.exit(main())
