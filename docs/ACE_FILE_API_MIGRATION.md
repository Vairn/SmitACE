# ACE file I/O / endian API migration

This document records the breaking change landed in the vendored ACE
(`deps/ACE`) and the work required — and now done — across the SmitACE
monorepo to consume it. Keep it around as the reference for the new
endian-aware file API contract so future loaders/savers stay correct on
both Amiga (big-endian native) and host builds (little-endian).

---

## TL;DR for the repo

ACE's old `fileRead()` / `fileWrite()` functions are **gone**. They have been
replaced by typed variants that auto-convert between the on-disk
**big-endian wire format** and the host's native endianness:

| Old call        | New call            | Use for                                         |
| --------------- | ------------------- | ----------------------------------------------- |
| `fileRead(...)`  | `fileReadBytes()`   | Raw byte payloads (bitplanes, PCM, names, arrays) |
|                 | `fileReadWords()`   | `UWORD` / `WORD` scalars (BE on disk)           |
|                 | `fileReadLongs()`   | `ULONG` / `LONG` scalars (BE on disk)           |
| `fileWrite(...)` | `fileWriteBytes()`  | Raw byte payloads                               |
|                 | `fileWriteWords()`  | `UWORD` / `WORD` scalars (writes BE on disk)    |
|                 | `fileWriteLongs()`  | `ULONG` / `LONG` scalars (writes BE on disk)    |

`fileWriteStr()` still exists for NUL-terminated strings.

The byte-count argument is now a **count of elements**, not a byte size:
`fileReadWords(pFile, &uwCount, 1)` reads one `UWORD` (and swaps it on a
little-endian host). Pass `1`, not `sizeof(UWORD)`.

**Bitplane / PCM / arbitrary byte blobs must use the `Bytes` variants** —
those are copied verbatim and never swapped, which is what the Amiga
hardware (blitter, copper, Paula) expects.

---

## What changed in ACE and why

The patch is KaiN's ACE PR #285 (AmigaPorts/ACE#285) merged onto our
`SDL-ACE` host branch, with the host-specific divergences preserved:

- `include/ace/utils/endian.h` — rewritten around `ENDIAN_NATIVE_BIG` /
  `ENDIAN_NATIVE_LITTLE` with `endianSwap16/32` and the
  `endianBigToNative16/32`, `endianNativeToBig16/32`, … macros. On this
  branch the native-endian detection guard is
  `#if defined(AMIGA) && !defined(ACE_HOST)` so that **host builds (which
  define both `AMIGA` and `ACE_HOST`) are treated as little-endian** — the
  unconditional `#if defined(AMIGA)` from the upstream PR would have broken
  the host. The old `endianBig16/32` and `endianLittle16/32` helpers are
  **removed**; use the macros above or the typed file functions instead.
- `include/ace/utils/file.h` + `src/ace/utils/file.c` — the new typed
  `fileRead/WriteBytes/Words/Longs`. On the host (`ENDIAN_NATIVE_LITTLE`)
  the `Words`/`Longs` variants swap each element to/from native; on Amiga
  they are bulk byte transfers. The typed functions are defined **once**,
  shared by both the `ACE_FILE_USE_ONLY_DISK` and the callback file backends
  (the upstream PR only emitted them on the callback path; we restructured
  `file.c` so the disk-only Amiga build links too).
- `src/ace/utils/disk_file.c` — buffer-bound check now uses
  `DISK_FILE_BUFFER_SIZE` instead of a hardcoded 512.
- ACE loaders migrated: `bitmap.c`, `font.c`, `palette.c`, `pak_file.c`,
  `ptplayer.c`, `log.c`. Notable fixes folded in during the merge:
  - `bitmapSave()` previously wrote its `UWORD` width/height in **native**
    endianness — on the host that produced little-endian headers that the
    Amiga couldn't read. It now writes them big-endian via
    `fileWriteWords()`. Bitplane payload stays raw via
    `fileWriteBytes()` (NOT `fileWriteWords` — the upstream PR routed
    planes through `Words`, which would byte-swap chip-RAM data on LE;
    we kept `Bytes`).
  - `bitmapSaveBmp()` keeps its little-endian BMP wire format, now expressed
    as `fileWriteLongs/Words/Bytes` with `endianSwap32/16` on the values.
  - `ptplayer.c` reads MOD sample headers field-by-field, pattern data via
    `fileReadWords()`, and **dropped the redundant `endianBig16` swap loop**
    that would otherwise double-convert on the host.

---

## What had to happen in this repo

Because `fileRead()`/`fileWrite()` were removed, every consumer in the
monorepo had to move to the typed API in the same change or fail to link.
All of the following have been done; the notes are here so anyone
re-touching these files knows the contract.

### SmitACE game (`src/`, `include/`)

The game had hand-rolled big-endian word helpers that wrapped the old
`fileRead`/`fileWrite`. They are **deleted**; their call sites now use the
typed ACE functions, which do the conversion for you.

- `src/maze/maze.c` — removed `mazeReadU16Be()` / `mazeWriteU16Be()`.
  Counts and string lengths use `fileReadWords()` / `fileWriteWords()`;
  maze cells, event bytes and string payloads use `fileReadBytes()` /
  `fileWriteBytes()`. Maze data is byte-oriented, so it is unaffected by
  endianness.
- `src/Gfx/wallset.c` — removed `readU16Be()` / `readI16Be()` /
  `writeU16Be()`. The `_screen` fields are `WORD` (signed 16-bit); read
  them with `fileReadWords(pFile, (UWORD*)&screen[i], 1)` (bit pattern is
  preserved) and write via a `UWORD` temporary + `fileWriteWords()`. All
  single-byte header/palette/tile fields use `fileReadBytes` /
  `fileWriteBytes`.
- `src/misc/monster.c` — removed `readU16Be()`. `uwMaxHP` and
  `uwExperience` use `fileReadWords()`; everything else is byte-oriented
  (`fileReadBytes()`).
- `src/items/item.c`, `src/game/gameState.c`,
  `src/game/level_entities.c`, `src/game/game_manifest.c` — these only ever
  did single-byte / raw-byte reads and writes, so they were mechanically
  renamed `fileRead` → `fileReadBytes` and `fileWrite` → `fileWriteBytes`.
  Their on-disk layouts are unchanged (byte streams).
- `src/maze/maze.c`'s saved-map format — `UWORD` event/string counts and
  string lengths are big-endian on disk (via `fileReadWords` /
  `fileWriteWords`); everything else is raw bytes. Documented in
  [`docs/formats/maze.md`](formats/maze.md).

### Vendored deps (`deps/aminer`, `deps/germz`)

Decision: these projects' save files were historically written as **native
host-struct dumps** (`fileWrite(pFile, &x, sizeof(x))`). To preserve their
existing save-file layout byte-for-byte (no format change, no behaviour
change on either platform), **all** of their `fileRead`/`fileWrite` calls
were migrated to `fileReadBytes`/`fileWriteBytes` — raw byte copies with
no endian conversion. This keeps current saves loadable.

Files migrated (26 total):

- `deps/aminer/src/`: `comm/inbox.c`, `comm/page_accounting.c`,
  `comm/page_bribe.c`, `comm/page_market.c`, `comm/page_office.c`,
  `comm/page_questioning.c`, `dino.c`, `game.c`, `heat.c`, `hi_score.c`,
  `hud.c`, `inventory.c`, `plan.c`, `protests.c`, `quest_crate.c`,
  `quest_gate.c`, `save.c`, `settings.c`, `string_array.c`, `tile.c`,
  `tutorial.c`, `utf8_remap.c`, `vehicle.c`, `warehouse.c`.
- `deps/germz/src/`: `json/json.c`, `map_data.c`.

One exception that needed a real semantic fix:

- `deps/aminer/src/string_array.c` — `stringArrayCreateFromFd()` read a
  `UWORD` count as raw bytes then called the now-removed `endianBig16()`.
  Since the count is a genuine BE `UWORD` on disk, it now uses
  `fileReadWords(pFile, &uwCount, 1)` (which swaps on host) and the manual
  swap line is gone.

> **If you add a new save field to aminer/germz**, decide deliberately:
> a raw `Bytes` dump keeps the existing native-struct-on-disk scheme (host
> and Amiga saves are **not** portable to each other); a typed
> `Words`/`Longs` call makes that field big-endian on disk and portable.
> Don't mix the two for the same struct without thinking about it.

---

## Checklist (for any future ACE bump in this repo)

When upgrading `deps/ACE`, re-run this checklist:

- [ ] `grep -rn '\bfileRead\b\|\bfileWrite\b\|\bendianBig16\|\bendianBig32\|\bendianLittle16\|\bendianLittle32' src include deps/aminer deps/germz`
      returns nothing (the old API and old endian helpers are gone).
- [ ] No hand-rolled `*U16Be` / `*U16Le` helpers remain in `src/` — use
      `fileReadWords`/`fileWriteWords` (or the `endian*ToNative*` macros for
      non-file work).
- [ ] Bitplane / PCM / raw payload reads and writes use the `Bytes`
      variants, never `Words`/`Longs`.
- [ ] Typed `Words`/`Longs` calls pass an **element count**, not a byte
      size (`... , 1)`, not `... , sizeof(UWORD))`).
- [ ] Host build (`build/Native`, `ACE_HOST=ON`) compiles and links
      `smite.exe` with no new warnings in migrated files.
- [ ] `deps/ACE/build-host-aga` showcase compiles and the font/`.bm`/`.plt`
      assets load (headers are big-endian on disk; the typed reader swaps
      on the little-endian host).
- [ ] Any new on-disk `UWORD`/`ULONG` field added to a SmitACE or
      aminer/germz format is documented in `docs/formats/` with its
      endianness noted.

---

## Why big-endian on disk

ACE's asset formats (`.bm`, `.fnt`, `.plt`, `.pak`, MOD) predate the host
port and are big-endian, matching the Amiga. Keeping the wire format BE and
doing the conversion in the I/O layer means **the same files load on real
Amiga hardware and on the little-endian host** with no per-call site
thought. The typed file API is the one place that knows the platform
endianness; everything else just says "read me a `UWORD`" and gets the
right value.

The host's chip RAM and copper/Paula DMA are also modelled as big-endian
16-bit words (see `docs/programming/ace_host.md` in ACE), so bitplane data
must stay raw bytes end-to-end — swapping it would corrupt what the
hardware sees.
