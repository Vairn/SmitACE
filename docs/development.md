# Development Guide

Guidelines and tips for developing SmitACE.

## Getting Started

1. Complete the [Installation Guide](INSTALL.md)
2. Build with debug options: `cmake .. -DACE_DEBUG=ON -DGAME_DEBUG=ON`
3. Use the "debug game" or "debug game+ai" VS Code task for development builds

## Code Organization

| Directory | Purpose |
|-----------|---------|
| `src/game/` | Game loop, states, rendering — start here for gameplay changes |
| `src/maze/` | Maze logic — for level format, movement, doors |
| `src/items/` | Items and inventory — for item/equipment systems |
| `src/Gfx/` | Graphics assets — wallsets, UI |
| `src/misc/` | Shared utilities — scripts, monsters, buttons, effects |
| `include/` | Public headers — keep interfaces here |

## Debugging

### Debug Flags

- **GAME_DEBUG** — Enables game-specific debug output and features
- **GAME_DEBUG_AI** — Additional AI/monster debug logging
- **ACE_DEBUG** — ACE framework debug mode
- **ACE_DEBUG_PTPLAYER** — Audio/module debug output

### VS Code

Use the Amiga C extension for:

- Building (CMake + Ninja)
- Running in UAE/FS-UAE
- Debugging with breakpoints

Ensure the correct CMake kit is selected (status bar).

### Emulator Testing

Test in UAE or FS-UAE. The project supports both native Amiga builds and cross-compilation with elf2hunk for faster iteration.

## Script System

The game includes a simple script language in `script.c`:

- **Gosub** — Call script subroutine (return stack)
- **If / Else** — Conditional execution
- **Flags** — Global (256) and local (256) flags per level

Scripts are typically embedded in maze event data.

## Adding New Content

### New Levels

1. Create maze data (format defined in `maze.c` / `maze.h`)
2. Add wallset reference
3. Register in level loading (`LoadLevel` in `gameState.c`)

### New Items

1. Define in item data (see `item.c`, `item.h`)
2. Add to inventory logic if equippable
3. Update UI if shown in inventory/equipment screens

### New Monsters

1. Extend `monster.h` / `monster.c`
2. Add to `tMonsterList` in level data
3. Hook into encounter/combat flow

## Performance Notes

- **68020 target** — Code is tuned for 68020+; tail-call optimization is disabled due to GCC bugs
- **AGA** — Uses 256-color AGA features; ensure assets match
- **Blitter** — ACE handles blitter operations; avoid excessive redraws

## Code Style

- Use `UBYTE`, `UWORD`, `ULONG`, `BOOL` (ACE/Amiga types) for clarity
- Keep functions in the appropriate module
- Document public APIs in headers
- Prefer `#pragma once` for include guards

## Related Projects

- [ACE Framework](https://github.com/Vairn/ACE/tree/AGA) — Custom AGA branch
- [Original Smite (Blitz)](https://github.com/Vairn/Smite/tree/PostJam) — BlitzBasic source
- [My Life Sucks](https://vairn.itch.io/my-life-sucks) — Original game jam entry
