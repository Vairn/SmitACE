# SmitACE Architecture

This document provides an overview of the SmitACE codebase structure and how the major components interact.

## High-Level Overview

SmitACE is a C port of the game *Smite*, built on the ACE framework with AGA (Advanced Graphics Architecture) support. It uses a state-machine-based game loop with modular subsystems for maze navigation, combat, items, and scripting.

## Directory Structure

```
SmitACE/
├── src/                 # Source code
│   ├── game/            # Core game logic, states, rendering
│   ├── Gfx/             # Graphics (wallsets, UI assets)
│   ├── items/           # Inventory and item system
│   ├── maze/            # Maze generation and navigation
│   └── misc/            # Utilities, scripts, effects
├── include/             # Public headers
├── res/                 # Resource files (data, assets)
├── deps/
│   └── ACE/             # ACE framework (submodule)
├── docs/                # Documentation
└── build/               # Build output
```

## Game States

The game uses a state machine (`tStateManager`) with these states:

| State | Module | Description |
|-------|--------|-------------|
| `g_sStateLogo` | — | Logo / splash screen |
| `g_sStateIntro` | `intro.c` | Intro sequence |
| `g_sStateTitle` | `title.c` | Title / main menu |
| `g_sStateGame` | `game.c` | Main gameplay loop |
| `g_sStateGameOver` | `gameOver.c` | Game over screen |
| `g_sStateWin` | `gameWin.c` | Victory screen |
| `g_sStatePaused` | — | Pause menu |
| `g_sStateLoading` | `loading.c` | Level loading |

State transitions are handled by the ACE `state` manager.

## Core Modules

### Game (`src/game/`)

- **game.c** — Main game loop, input handling, viewport rendering
- **gameState.c** — Save/load, level loading, global state
- **Renderer.c** — 3D viewport and wall rendering
- **game_ui.c** / **game_ui_regions.c** — UI layout and click handling
- **title.c**, **intro.c**, **loading.c** — State-specific screens
- **gameOver.c**, **gameWin.c** — End-game screens
- **gfx_util.c** — Graphics utilities

### Maze (`src/maze/`)

- **maze.c** — Maze data structures, movement, door handling
- Cell types: floor, wall, door, event triggers
- Event system for scripts and interactions
- Door animation state machine

### Items (`src/items/`)

- **item.c** — Item definitions and behavior
- **inventory.c** — Party inventory management
- Equipment slots: hands, armor, shield, helmet, accessory, feet

### Graphics (`src/Gfx/`)

- **wallset.c** — Wall texture sets for 3D view
- **uigfx.c** — UI graphics and layouts

### Miscellaneous (`src/misc/`)

- **script.c** — In-game scripting (Gosub, If/Else, flags)
- **monster.c** — Monster/encounter handling
- **character.c** — Party and character stats
- **doorlock.c**, **doorbutton.c**, **wallbutton.c** — Interactive elements
- **fade.c** — Screen fade effects
- **text_render.c** — Text rendering
- **screen.c**, **layer.c** — Display management
- **mouse_pointer.c** — Cursor handling

## Key Data Structures

### GameState (`GameState.h`)

Central game state including:

- Current maze and wallset
- Character party
- Monster list
- Inventory
- Wall buttons, door buttons, door locks
- Script state (return stack, condition flags, program counter)
- Global and local flags (256 each)

### Maze (`maze.h`)

- Grid-based layout
- Door animation tracking
- Event system for script triggers
- String tables for messages

### Character (`character.h`)

- Stats: HP, MP, Level, Experience
- Attributes: Strength, Agility, Vitality, Intelligence, Luck
- Equipment slots
- Spell and status effect tracking
- Elemental resistances

## ACE Framework Dependencies

SmitACE relies on these ACE managers and utilities:

- **state** — Game state machine
- **key**, **mouse**, **joy** — Input
- **blit**, **sprite**, **bob** — Graphics
- **viewport** (simplebuffer) — 3D viewport
- **ptplayer** — ProTracker module playback
- **palette** — Color management
- **memory**, **system** — Low-level support

## Entry Point

`src/smite.c` implements the ACE `genericCreate`, `genericProcess`, and `genericDestroy` callbacks. The initial state is set to `g_sStateGame` (can be changed for intro/title flow).
