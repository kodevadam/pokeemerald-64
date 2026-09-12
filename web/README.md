# Emerald64 Cartridge Builder

A single-page, entirely client-side patcher: someone drops in their own
Pokémon Emerald (USA) `.gba` and gets back `pokeemerald64.z64` for an
EverDrive 64, SC64, or an emulator. The ROM never leaves the browser.

`index.html` is the whole site — no build step, no dependencies, no server.
Host it anywhere static.

## Why a patch and not just the ROM

The N64 build is a recompilation, so its MIPS code and pointer tables have no
counterpart in the GBA ROM and have to travel in the patch. What it *does*
share with a retail cartridge is the bulk asset data — compressed graphics,
tilesets, palettes, map blocks, text — which both builds embed byte-for-byte
identically. The patch reuses those from the user's ROM and carries the rest.

Measured against the current build (`--stats`):

```
where the .z64's bytes come from:
  reused from your GBA ROM      5.60 MB   55.8%
  repeated within the .z64      1.90 MB   18.9%
  carried by the patch          2.54 MB   25.3%
```

So a 10.05 MB ROM ships as a 1.70 MB gzipped patch, and the majority of what
comes out really does come off the user's cartridge.

## Building the patch

The site needs `pokeemerald64.bps` sitting next to `index.html`. Generating it
requires a retail ROM, so it is not checked in — build it once on a machine
that has one:

```sh
make -f Makefile.n64                       # produces build/n64/pokeemerald64.z64
python3 tools/make_bps.py \
        /path/to/pokeemerald.gba \
        build/n64/pokeemerald64.z64 \
        web/pokeemerald64.bps --stats --gzip --json
```

`--gzip` is worth using: what the patch carries is dominated by MIPS code,
which compresses to roughly 40%. The page sniffs the gzip magic and inflates
transparently.

`--json` writes a base64-in-JSON copy alongside. Some static hosts — the
claude.ai artifact host among them — only serve standard web media types and
will not serve a raw `.bps`. The page tries `pokeemerald64.bps.gz`, then
`pokeemerald64.bps`, then `pokeemerald64.bps.json`, so on an ordinary host the
raw file is used and the JSON copy is just dead weight you can delete.

### No retail ROM on hand?

You don't need one. pokeemerald is a *matching* decompilation: built with
agbcc it reproduces the retail cartridge byte for byte. Build the last
pristine upstream commit (`61674ecd`, the parent of the first N64 commit) in a
worktree and check it against `rom.sha1`:

```sh
git worktree add /tmp/pristine 61674ecd
cp -r tools/agbcc /tmp/pristine/tools/     # from github.com/pret/agbcc
cd /tmp/pristine && make -j"$(nproc)"
sha1sum -c rom.sha1                        # must print OK
```

Use `/tmp/pristine/pokeemerald.gba` as the patch source. Do not use this
repo's own GBA build — see below.

### This repo's GBA target no longer matches

Building `make` at HEAD produces a ROM that is *not* byte-identical to retail
(`591eb873…` rather than `f3ae0881…`). Some of the N64 port's edits to shared
source change GBA codegen even though they are semantically inert there — the
`CopyMapBlocks` helper in `fieldmap.c` and the unconditional `MAP_ASSET_16`
wrapping in `battle_pyramid.c`, `decoration.c`, `secret_base.c` and
`trainer_hill.c` are the known ones. A patch built against that ROM would
reject every real cartridge dump, so always build the source ROM from the
pristine commit until matching is restored.

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
