# Building SmitACE

This document describes the build system, configuration options, and build targets for SmitACE.

## Build System

SmitACE uses **CMake**. There are two targets:

- **Amiga** — cross-compile with Bartman's GCC (default)
- **Native** — host SDL chipset backend (`ACE_HOST=ON`), so the game runs on Windows/Linux without UAE

## Quick Start

**Amiga:**

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

**Native (ACE host):**

```bash
cmake -S . -B build/Native -G Ninja -DACE_HOST=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build/Native --parallel
```

Run from the Native build directory so `data/` is found:

```bash
cd build/Native
./smite.exe
```

On Windows with MSYS2 UCRT, install `mingw-w64-ucrt-x86_64-gcc`, `mingw-w64-ucrt-x86_64-SDL2`, and Ninja. Point CMake at SDL2 with `-DCMAKE_PREFIX_PATH=C:/msys64/ucrt64`.

In VS Code, select the **Native** CMake kit, configure, then use the **Native** launch configuration (cwd is `build/Native`).

## CMake Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `ACE_HOST` | `OFF` | Build with ACE host SDL chipset backend (Windows/Linux) |
| `ACE_HOST_DEBUG` | `ON` | Host HUD (F11 memory, F12 DMA/copper overlay) |
| `ACE_HOST_MACHINE` | `A1200` | Host machine preset (AGA default) |
| `ACE_DEBUG` | `ON` | Enable ACE framework debug mode |
| `ACE_DEBUG_UAE` | `ON` | UAE/emulator console logging. On Native this is a no-op; ACE_HOST still logs to stdout |
| `GAME_DEBUG` | `OFF` | Enable game-specific debug features |
| `GAME_DEBUG_AI` | `OFF` | Enable AI debugging (when GAME_DEBUG is ON) |
| `ACE_DEBUG_PTPLAYER` | `OFF` | Enable ProTracker/audio debug output |
| `ELF2HUNK` | — | Path to elf2hunk converter (when cross-compiling) |

### Debug Builds

For development with full debugging:

```bash
cmake .. -DACE_DEBUG=ON -DGAME_DEBUG=ON
```

With AI debugging:

```bash
cmake .. -DACE_DEBUG=ON -DGAME_DEBUG=ON -DGAME_DEBUG_AI=ON -DACE_DEBUG_PTPLAYER=ON
```

### Release Builds

For optimized, distributable builds:

```bash
cmake .. -DACE_DEBUG=OFF -DGAME_DEBUG=OFF
```

Native release:

```bash
cmake -S . -B build/Native -G Ninja -DACE_HOST=ON -DACE_DEBUG=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
```

## Build Targets

| Target | Description |
|--------|-------------|
| `smite` / `smite.exe` | Main game executable |
| `generateZip` | Creates a ZIP archive with executable and data folder |
| `generateAdf` | Creates an Amiga disk image (ADF) for distribution (Amiga builds only) |

### Generating Distribution Packages

**ZIP package** (for emulator use):

```bash
cmake --build . --target generateZip
```

Output: `Smite XX_YY_ZZ.zip` in the build directory.

**ADF disk image** (for real Amiga or floppy-based emulation):

```bash
cmake --build . --target generateAdf
```

Requires `exe2adf` to be available in PATH. Output: `Smite.adf`. Native builds do not produce an ADF.

## Project Structure (Build Output)

```
build/
├── unknown/           # Amiga kit (or your CMake kit name)
│   ├── smite.exe      # Game executable
│   ├── smite.elf      # ELF (when ELF2HUNK used)
│   ├── smite.map      # Linker map file
│   ├── smite.s        # Disassembly (when ELF2HUNK)
│   └── data/
└── Native/            # ACE host SDL build
    ├── smite.exe
    ├── SDL2.dll
    └── data/
```

## ACE Framework Features

The project enables these ACE features (see `CMakeLists.txt`):

- `ACE_USE_AGA_FEATURES` — Advanced Graphics Architecture support
- `ACE_USE_ECS_FEATURES` — Enhanced Chip Set

Native builds emulate an A1200 (AGA) by default. Host keys: F10 timing log, F11 memory HUD, F12 DMA/copper overlay.

## Compiler Notes

- **Target CPU:** 68020+ (configurable via `M68K_CPU`; Amiga builds only)
- **Optimization:** `-fno-optimize-sibling-calls` is applied to avoid known GCC tail-call bugs on 68020+
- **Build number:** Auto-generated from timestamp and written to `build_number.h`

## VS Code Integration

The project includes VS Code tasks for common configurations:

- **debug game** — Debug build (ACE + game debug)
- **debug game+ai** — Debug build with AI debugging
- **debug off** — Release build (default)
- **configure native** / **build native** — ACE host SDL build

Run tasks via `Terminal > Run Task...` or `Ctrl+Shift+B` for the default build.
