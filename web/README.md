# Emerald64 Cartridge Builder

A single-page, entirely client-side patcher: someone drops in their own
Pokémon Emerald (USA) `.gba` and gets back `pokeemerald64.z64` for an
EverDrive 64, SC64, or an emulator. The ROM never leaves the browser.

`index.html` is the whole site — no build step, no dependencies, no server.
Host it anywhere static.

## Why a patch and not just the ROM

The N64 build is a recompilation, so its MIPS code, pointer tables and
byte-swapped script operands have no counterpart in the GBA ROM and have to
travel in the patch. What it *does* share with a retail cartridge is the bulk
asset data — compressed graphics, tilesets, palettes, map blocks, text — which
both builds embed byte-for-byte identically. The patch reuses those from the
user's ROM and carries the rest, which is both smaller to distribute and the
convention the romhacking scene already expects.

Run `make_bps.py --stats` to see the actual split for a given build.

## Building the patch

The site needs `pokeemerald64.bps` sitting next to `index.html`. Generating it
requires a retail ROM, so it is not checked in — build it once on a machine
that has one:

```sh
make -f Makefile.n64                       # produces build/n64/pokeemerald64.z64
python3 tools/make_bps.py \
        /path/to/pokeemerald.gba \
        build/n64/pokeemerald64.z64 \
        web/pokeemerald64.bps --stats
```

The source ROM must be Pokémon Emerald (USA),
sha1 `f3ae088181bf583e55daf962a92bb46f4f1d07b7`. `make_bps.py` refuses anything
else unless you pass `--allow-any-source`, and it applies the patch back and
compares against the target before writing, so a patch that exists is a patch
that round-trips.

Regenerate the patch on every `.z64` rebuild — it is pinned to that exact
output by CRC-32.

## Without a bundled patch

If `pokeemerald64.bps` is missing, the page says so and offers a second drop
target for the `.bps`, so it stays usable for anyone who built their own.

## Saving the result

Self-hosted, the page hands back `pokeemerald64.z64` as an ordinary download.
Published as a claude.ai artifact, page-initiated downloads are blocked and the
host's allowlist takes `.zip` but not `.z64`, so there the ROM is wrapped in
`pokeemerald64.zip` (deflate, roughly 44% of the raw size) and saved through
the `downloads` capability. Both paths produce the same bytes.

## Checks the page makes

- SHA-1 of the input against the known-good Emerald (USA) hash
- The GBA header's internal title and game code, so a wrong game is named
  (`AXVE` → "That is Pokémon Ruby") rather than rejected as a hash mismatch
- CRC-32 of the source, from the BPS header, before patching
- CRC-32 of the output after patching — a build that completes is verified
