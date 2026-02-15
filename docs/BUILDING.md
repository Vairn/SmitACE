# Building SmitACE

This document describes the build system, configuration options, and build targets for SmitACE.

## Build System

SmitACE uses **CMake** as its build system, targeting the Amiga platform with the ACE framework and GCC (via Bartman's Amiga C extension).

## Quick Start

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## CMake Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `ACE_DEBUG` | `ON` | Enable ACE framework debug mode |
| `ACE_DEBUG_UAE` | `ON` | UAE/emulator-specific debugging |
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

## Build Targets

| Target | Description |
|--------|-------------|
| `smite` / `smite.exe` | Main game executable |
| `generateZip` | Creates a ZIP archive with executable and data folder |
| `generateAdf` | Creates an Amiga disk image (ADF) for distribution |

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

Requires `exe2adf` to be available in PATH. Output: `Smite.adf`.

## Project Structure (Build Output)

```
build/
├── unknown/           # or your CMake kit name
│   ├── smite.exe      # Game executable
│   ├── smite.elf      # ELF (when ELF2HUNK used)
│   ├── smite.map      # Linker map file
│   ├── smite.s        # Disassembly (when ELF2HUNK)
│   └── data/          # Runtime data (copy from res)
└── ...
```

## ACE Framework Features

The project enables these ACE features (see `CMakeLists.txt`):

- `ACE_USE_AGA_FEATURES` — Advanced Graphics Architecture support
- `ACE_USE_ECS_FEATURES` — Enhanced Chip Set

## Compiler Notes

- **Target CPU:** 68020+ (configurable via `M68K_CPU`)
- **Optimization:** `-fno-optimize-sibling-calls` is applied to avoid known GCC tail-call bugs on 68020+
- **Build number:** Auto-generated from timestamp and written to `build_number.h`

## VS Code Integration

The project includes VS Code tasks for common configurations:

- **debug game** — Debug build (ACE + game debug)
- **debug game+ai** — Debug build with AI debugging
- **debug off** — Release build (default)

Run tasks via `Terminal > Run Task...` or `Ctrl+Shift+B` for the default build.
