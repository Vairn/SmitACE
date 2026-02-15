[![MIT License][license-shield]][license-text]

# SmitACE

**Smite Ported To C** — A dungeon-crawling RPG for the Amiga, built with a custom AGA branch of [ACE](https://github.com/Vairn/ACE/tree/AGA).

Based on the BlitzBasic game jam entry [My Life Sucks](https://vairn.itch.io/my-life-sucks) and the [Smite PostJam](https://github.com/Vairn/Smite/tree/PostJam) source.

---

## Overview

SmitACE brings the dungeon-crawler *Smite* to the Amiga platform with full AGA (Advanced Graphics Architecture) support. The game features maze navigation, turn-based combat, an inventory system, and an embedded script language for level events and interactions.

| Platform | Requirements |
|----------|--------------|
| **Amiga** | AGA chipset, 68020+ CPU |
| **Development** | Windows, VS Code, Amiga C Extension, CMake |

---

## Quick Start

```bash
git clone --recursive https://github.com/Vairn/SmitACE.git
cd SmitACE
mkdir build && cd build
cmake ..
cmake --build .
```

Copy the `data` folder to the build output directory before running. See **[Installation Guide](docs/INSTALL.md)** for detailed setup.

---

## Documentation

| Document | Description |
|----------|-------------|
| [**Installation Guide**](docs/INSTALL.md) | Prerequisites, toolchain setup, and verification |
| [**Building**](docs/BUILDING.md) | CMake options, build targets, and distribution packages |
| [**Architecture**](docs/ARCHITECTURE.md) | Code structure, game states, and module overview |
| [**Development**](docs/DEVELOPMENT.md) | Debugging, scripting, and adding new content |

---

## Project Structure

```
SmitACE/
├── src/
│   ├── game/      # Core game logic, states, rendering
│   ├── Gfx/       # Graphics (wallsets, UI)
│   ├── items/     # Inventory and items
│   ├── maze/      # Maze generation and navigation
│   └── misc/      # Scripts, monsters, utilities
├── include/       # Header files
├── res/           # Resource files
├── docs/          # Documentation
└── deps/ACE/      # ACE framework (submodule)
```

---

## Build Options

| Option | Purpose |
|--------|---------|
| `-DACE_DEBUG=ON` | Enable ACE debug mode |
| `-DGAME_DEBUG=ON` | Enable game debug features |
| `-DGAME_DEBUG_AI=ON` | Enable AI debugging |
| `-DACE_DEBUG=OFF -DGAME_DEBUG=OFF` | Release build |

Example debug build:

```bash
cmake .. -DACE_DEBUG=ON -DGAME_DEBUG=ON
```

See [docs/BUILDING.md](docs/BUILDING.md) for full details.

---

## Distribution

- **ZIP**: `cmake --build . --target generateZip` — executable + data for emulator use  
- **ADF**: `cmake --build . --target generateAdf` — Amiga disk image (requires `exe2adf`)

---

## License

This project is licensed under the MIT License — see [LICENSE.md](LICENSE.md) for details.

---

[license-text]: https://opensource.org/license/mit/
[license-shield]: https://img.shields.io/badge/license-MIT-brightgreen
