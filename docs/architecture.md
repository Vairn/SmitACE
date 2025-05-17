# SmitACE Architecture

## Overview
SmitACE is a C port of the game Smite, designed to run on Amiga systems with AGA support. The architecture is built around the ACE framework, providing a robust foundation for game development on the Amiga platform.

## Core Components

### Game Engine
- Located in `src/game/`
- Handles core game loop
- Manages game state
- Coordinates between different subsystems

### Graphics System
- Located in `src/Gfx/`
- Handles rendering using AGA capabilities
- Manages sprites and animations
- Handles screen modes and display

### Item System
- Located in `src/items/`
- Manages game items and inventory
- Handles item properties and effects
- Controls item interactions

### Maze System
- Located in `src/maze/`
- Handles maze generation
- Manages level layout
- Controls navigation and collision

### Utilities
- Located in `src/misc/`
- Common utility functions
- Helper routines
- System-specific implementations

## Dependencies
- ACE Framework (AGA branch)
- Amiga system libraries
- Custom AGA-specific extensions

## Build System
- Uses CMake for build configuration
- Supports cross-compilation for Amiga
- Integrates with VSCode for development

## Development Guidelines
1. Follow C coding standards
2. Document all public APIs
3. Maintain compatibility with AGA hardware
4. Optimize for Amiga performance
5. Use appropriate memory management techniques

## Future Considerations
- Potential for additional platform support
- Performance optimizations
- Feature expansions
- Community contributions 