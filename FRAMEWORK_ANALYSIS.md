# SmitACE - Deep Dive Framework Analysis

## Executive Summary

**SmitACE** is a C port of the game "Smite" designed to run on classic Amiga hardware with AGA (Advanced Graphics Architecture) chipset support. It uses a custom AGA-enhanced branch of the **ACE (Amiga C Engine)** framework, which provides a lightweight, high-performance game engine specifically designed for classic Amiga hardware.

---

## Core Framework: ACE (Amiga C Engine)

### Overview
ACE is a game engine/framework written entirely in C for classic Amiga hardware. It's designed to:
- Provide direct hardware access for maximum performance
- Be lightweight and hackable
- Support Kickstart 1.3+ while providing OS 2.0+ features
- Enable high-performance games without requiring assembly language expertise

### Key Characteristics
- **Hardware-First Approach**: ACE uses Amiga hardware features directly (blitter, copper, sprites, audio chips)
- **OS-Friendly**: Can run from and exit to Workbench gracefully
- **Optional OS**: Disables OS for performance but allows re-enabling for file access, memory allocation, etc.
- **Debug Builds**: Extensive runtime error checks in debug mode (completely removed in release builds)
- **Thin Abstraction**: Provides hardware abstraction layer that's as thin and documented as possible

---

## ACE Framework Components Used in SmitACE

### 1. **Managers** (Global Resources)
Managers handle single global resources and are typically created once per program:

#### **State Manager** (`ace/managers/state.h`)
- **Purpose**: Manages game state machine with push/pop stack support
- **Usage**: Handles state transitions (Title → Intro → Game → GameOver → Win)
- **Features**: 
  - State lifecycle: create → loop → destroy
  - State suspend/resume for nested states
  - Stack-based state management for pause menus, etc.
- **SmitACE States**: Logo, Intro, Title, Game, GameOver, Win, Paused, Loading

#### **System Manager** (`ace/managers/system.h`)
- **Purpose**: Core system initialization and OS management
- **Features**:
  - Freezes/unfreezes OS
  - Manages screen/viewport
  - Handles vblank timing
  - Provides system-level utilities

#### **Blitter Manager** (`ace/managers/blit.h`)
- **Purpose**: Hardware-accelerated bitmap operations
- **Capabilities**:
  - Rectangle filling (`blitRect()`)
  - Bitmap copying (`blitCopy()`)
  - Line drawing (`blitLine()`)
  - Masked blitting (`blitMaskedCopy()`)
  - Hardware-accelerated operations using Amiga's blitter chip

#### **Sprite Manager** (`ace/managers/sprite.h`)
- **Purpose**: Hardware sprite management
- **Features**:
  - Up to 8 sprite channels (16 pixels wide each)
  - Sprite linking/lists
  - 2BPP sprites (can pair for 4BPP)
  - Hardware-accelerated sprite display

#### **Bob Manager** (`ace/managers/bob.h`)
- **Purpose**: Blitter Object management (hardware-accelerated moving objects)
- **Usage**: Used for characters and moving game elements
- **Features**: Hardware-accelerated object manipulation

#### **Copper Manager** (`ace/managers/copper.h`)
- **Purpose**: Copper list management (Amiga's co-processor)
- **Usage**: Real-time graphics effects, color changes, split-screen effects
- **Features**: Hardware-based visual effects

#### **Viewport/Buffer Managers** (`ace/managers/viewport/`)
- **simplebuffer.h**: Simple bitmap buffer for rendering
- **scrollbuffer.h**: Scrollable buffer with Y-axis wrapping
- **tilebuffer.h**: Tile-based rendering with scrolling
- **camera.h**: Camera/view tracking for playfield

#### **Input Managers**
- **Key Manager** (`ace/managers/key.h`): Keyboard input
- **Mouse Manager** (`ace/managers/mouse.h`): Mouse input
- **Joy Manager** (`ace/managers/joy.h`): Joystick/gamepad input

#### **Audio Manager**
- **PTPlayer** (`ace/managers/ptplayer.h`): ProTracker module player
- **Features**: Plays .mod files using Paula audio chip
- **Usage**: Background music and sound effects

#### **Game Manager** (`ace/managers/game.h`)
- **Purpose**: High-level game loop management
- **Features**: Coordinates vblank, double buffering, frame timing

#### **Memory Manager** (`ace/managers/memory.h`)
- **Purpose**: Memory allocation with CHIP/FAST distinction
- **Features**:
  - CHIP memory (accessible by graphics hardware)
  - FAST memory (CPU-only, faster)
  - Debug memory tracking in debug builds

#### **Log Manager** (`ace/managers/log.h`)
- **Purpose**: Debug logging (only in debug builds)
- **Features**: 
  - File-based logging (`game.log`)
  - Memory usage logging (`memory.log`)
  - Indented log blocks for readability

### 2. **Utils** (Resource Utilities)
Utils are for working with multiple resources of the same kind:

#### **Bitmap Utils** (`ace/utils/bitmap.h`)
- **Purpose**: Bitmap creation and manipulation
- **Features**: Create, load, save, manipulate bitmaps

#### **File Utils** (`ace/utils/file.h`, `ace/utils/disk_file.h`)
- **Purpose**: File I/O operations
- **Features**: Load resources, read game data, file operations

#### **Palette Utils** (`ace/utils/palette.h`)
- **Purpose**: Color palette management
- **Features**: Load palettes, fade effects, palette manipulation

#### **Font Utils** (`ace/utils/font.h`)
- **Purpose**: Font rendering and text display

#### **Sprite Utils** (`ace/utils/sprite.h`)
- **Purpose**: Sprite data structure utilities

#### **Tag Utils** (`ace/utils/tag.h`)
- **Purpose**: Tag-based parameter passing (Amiga OS style)

### 3. **Generic Main** (`ace/generic/main.h`)
- **Purpose**: Boilerplate reduction for main loop
- **Features**: Provides standard game main loop
- **Requires**: Define `genericCreate()`, `genericProcess()`, `genericDestroy()`

---

## SmitACE-Specific Architecture

### Game Components

#### **1. Game State System** (`GameState.h`, `gameState.c`)
- **Structure**: `tGameState` contains:
  - Current maze and wallset references
  - Global/local flags (256 bytes each)
  - Character party
  - Monster list
  - Script state (custom script interpreter)
- **Persistence**: Save/load game state to/from files

#### **2. Maze System** (`maze.h`, `maze.c`)
- **Structure**: `tMaze` contains:
  - 2D grid (`_mazeData`, `_mazeCol`, `_mazeFloor`)
  - Event system (linked list of events)
  - String table for messages
  - Door animation system
- **Features**:
  - Load/save from disk
  - Event triggers (script execution)
  - Door animations
  - Cell-based navigation

#### **3. Script System** (`script.h`, `script.c`)
- **Purpose**: Custom bytecode interpreter for game events
- **Features**:
  - Stack-based execution
  - Conditional logic (IF/ELSE/ENDIF)
  - Subroutines (GOSUB/RETURN)
  - Flow control (GOTO)
  - Rich event types:
    - Map manipulation (SET_WALL, OPEN_DOOR, etc.)
    - Game state (SET_FLAG, TELEPORT, etc.)
    - Combat (DAMAGE, ADD_MONSTER, etc.)
    - Items (GIVE_ITEM, TAKE_ITEM, etc.)
    - Conditionals (PARTY_ON_POS, HAS_CLASS, etc.)

#### **4. Graphics System**
- **Wallset Renderer** (`wallset.h`, `wallset.c`): Renders 3D-style maze walls
- **UI Graphics** (`uigfx.c`): User interface rendering
- **Renderer** (`Renderer.h`, `Renderer.c`): Main rendering pipeline
- **Graphics Utils** (`gfx_util.c`): Common graphics utilities

#### **5. Game Entities**
- **Character System** (`character.h`, `character.c`): Party management
- **Monster System** (`monster.h`, `monster.c`): Enemy management
- **Item System** (`item.h`, `item.c`): Item definitions and properties
- **Inventory System** (`inventory.h`, `inventory.c`): Inventory management

#### **6. UI System**
- **Game UI** (`game_ui.h`, `game_ui.c`): Main game interface
- **UI Regions** (`game_ui_regions.h`): Clickable UI regions
- **Mouse Pointer** (`mouse_pointer.h`, `mouse_pointer.c`): Custom cursor

#### **7. State Screens**
- **Title Screen** (`title.h`, `title.c`)
- **Intro Sequence** (`intro.h`, `intro.c`)
- **Loading Screen** (`loading.h`, `loading.c`)
- **Game Over** (`gameOver.h`, `gameOver.c`)
- **Win Screen** (`gameWin.h`, `gameWin.c`)
- **Fade Effects** (`fade.h`, `fade.c`)

---

## Build System & Toolchain

### CMake Configuration
- **Min Version**: CMake 3.14.0
- **Languages**: ASM, C
- **ACE Integration**: Subdirectory build of `deps/ACE`
- **Features Enabled**:
  - `ACE_USE_AGA_FEATURES`: AGA chipset features
  - `ACE_USE_ECS_FEATURES`: Enhanced Chip Set features
  - `ACE_DEBUG_UAE`: UAE emulator debugging support

### Compiler: Bartman's GCC
- **Type**: GCC 10.1+ for m68k (Motorola 68000) architecture
- **Target**: Amiga (m68k-amiga-elf)
- **IDE Integration**: VSCode extension (`BartmanAbyss.amiga-debug`)
- **Special Requirements**:
  - Includes `bartman/gcc8_c_support.h` for compiler-specific support
  - Requires `proto/exec.h` and `proto/dos.h` for Amiga OS prototypes
  - Produces ELF executables (converted to Hunk format via `elf2hunk`)

### Cross-Compilation Setup
- **Toolchain**: m68k-amiga-elf-gcc (cross-compiler)
- **Toolchain File**: `m68k-bartman.cmake` (for Bartman's GCC)
- **Output Format**: Hunk format executable (`.exe`) for Amiga

### Build Outputs
- **Executable**: `smite.exe` (or `smite.elf` before conversion)
- **Assembler Output**: `smite.s` (disassembly for debugging)
- **Map File**: `smite.map` (linker map)
- **Distribution**: ZIP and ADF (Amiga Disk Format) targets

---

## Key Technical Constraints (Amiga-Specific)

### Memory Architecture
- **CHIP Memory**: Required for graphics data (sprites, bitmaps, copper lists)
  - Limited (512KB-2MB depending on Amiga model)
  - Accessible by graphics hardware
- **FAST Memory**: CPU-only memory (faster access)
  - Can be expanded with accelerator cards
  - Used for game logic, data structures

### Hardware Limitations
- **CPU**: Motorola 68000/68020/68040 (7-50 MHz typically)
- **Sprites**: 8 channels, 16 pixels wide each (unless paired for 4BPP)
- **Blitter**: Hardware-accelerated bitmap operations
- **Copper**: Co-processor for real-time graphics effects
- **Paula**: Audio chip for sound/music (4 channels, 8-bit samples)

### AGA Features (Advanced Graphics Architecture)
- **Enhanced Color Modes**: Up to 256 colors in 8-bit modes
- **Wider Sprites**: AGA supports wider sprites (partially implemented)
- **Enhanced Palettes**: More color registers (256 vs 32)
- **Higher Resolutions**: Up to 640x512 in certain modes

### Performance Considerations
- **No Floating Point**: Uses fixed-point math (`fixmath` library)
- **Optimization**: `-fomit-frame-pointer` compiler flag for performance
- **Direct Hardware Access**: Minimal abstraction for speed
- **Memory Efficiency**: Critical due to limited CHIP memory

---

## Dependencies

### Primary Framework
- **ACE Framework** (AGA branch): `deps/ACE/`
  - Custom branch with AGA enhancements
  - Full source included in project

### Compiler Support
- **bartman_gcc_support**: Compiler-specific support library
  - Fetched via CPM (CMake Package Manager)
  - Provides missing standard library functions

### Mini Standard Library
- **mini_std**: Lightweight C standard library replacement
  - Used with Bartman's GCC (incomplete stdlib)
  - Includes: printf, string functions, ctype, etc.
  - Disabled features: floats, long long (for performance)

### Fixed-Point Math
- **fixmath**: Fixed-point mathematics library
  - Alternative to floating-point (Amiga lacks FPU in most models)
  - Provides: fix16_t, trig functions, basic math operations

---

## File Structure

```
SmitACE/
├── src/                    # Source code
│   ├── smite.c            # Main entry point (uses generic/main.h)
│   ├── game/              # Game logic
│   │   ├── game.c         # Main game loop
│   │   ├── gameState.c    # Game state management
│   │   ├── Renderer.c     # Rendering pipeline
│   │   └── ...            # Other game states
│   ├── Gfx/               # Graphics systems
│   │   ├── wallset.c      # Wall rendering
│   │   └── uigfx.c        # UI graphics
│   ├── maze/              # Maze system
│   │   └── maze.c         # Maze loading/rendering
│   ├── items/             # Item system
│   │   ├── item.c
│   │   └── inventory.c
│   └── misc/              # Utilities
│       ├── script.c       # Script interpreter
│       ├── monster.c      # Monster management
│       └── ...
├── include/               # Header files
├── deps/ACE/              # ACE framework
│   ├── include/ace/       # ACE headers
│   ├── src/ace/           # ACE source
│   └── docs/              # ACE documentation
├── res/                   # Resources (images, sounds, data)
├── CMakeLists.txt         # Build configuration
└── docs/                  # Project documentation
```

---

## Development Workflow

### Typical Development Cycle
1. **Edit Code**: Make changes in `src/` or `include/`
2. **Build**: CMake generates build files, compiler produces ELF
3. **Convert**: ELF → Hunk format via `elf2hunk`
4. **Test**: Run in UAE (Amiga emulator) or transfer to real hardware
5. **Debug**: Use debug logging (if enabled) or debugger integration

### Debugging
- **Debug Logs**: `game.log` and `memory.log` (only in debug builds)
- **UAE Integration**: Bartman's extension provides WinUAE integration
- **Memory Tracking**: Tracks all allocations/deallocations in debug mode
- **Sanity Checks**: Runtime validation in debug builds (removed in release)

---

## What Makes This Different from Modern Development

### No Standard Library
- Bartman's GCC has incomplete stdlib
- Uses `mini_std` replacement
- Many standard functions missing or limited

### Fixed-Point Math
- No floating-point by default (no FPU on most Amigas)
- Uses fixed-point arithmetic (`fix16_t`, `fract32_t`)

### Direct Hardware Access
- No high-level graphics APIs (OpenGL, DirectX)
- Direct register manipulation for graphics hardware
- Manual memory management with CHIP/FAST distinction

### Limited Resources
- Memory constraints (especially CHIP memory)
- CPU performance limits (7-50 MHz)
- Careful optimization required

### Custom Scripting Language
- Bytecode interpreter instead of Lua/Python/etc.
- Lightweight, designed for Amiga constraints

---

## Summary

SmitACE is a sophisticated Amiga game built on the ACE framework, which provides a thin abstraction over Amiga hardware. The project demonstrates:

1. **Deep Hardware Integration**: Direct use of blitter, copper, sprites, and audio hardware
2. **Resource Management**: Careful CHIP/FAST memory distinction
3. **Performance Focus**: Optimization for 68000-era hardware
4. **State Management**: Clean state machine architecture
5. **Custom Systems**: Script interpreter, maze system, event system
6. **AGA Enhancement**: Uses Advanced Graphics Architecture features

The codebase is well-structured with clear separation between game logic, graphics, and system management, all built on top of ACE's lightweight but powerful foundation.

