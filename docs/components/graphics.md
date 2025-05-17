# Graphics System Documentation

## Overview
The graphics system in SmitACE is responsible for handling all rendering operations using the Amiga's AGA capabilities. It consists of two main components: the wallset renderer and the UI graphics system.

## Components

### Wallset Renderer (`wallset.c`)
The wallset renderer handles the rendering of walls and environment elements in the game.

#### Key Functions
```c
// Initialize the wallset system
void InitWallset(void);

// Render a wall at specified coordinates
void RenderWall(int x, int y, int type);

// Update wall textures
void UpdateWallTextures(void);
```

#### Wall Types
- Standard walls
- Special walls (doors, windows)
- Decorative elements

### UI Graphics System (`uigfx.c`)
Handles the rendering of user interface elements.

#### Key Functions
```c
// Initialize UI graphics
void InitUIGraphics(void);

// Draw UI element
void DrawUIElement(int x, int y, int type);
```

## Screen Modes
The graphics system supports various AGA screen modes:
- 320x256 (8-bit)
- 640x256 (8-bit)
- Custom modes for specific effects

## Memory Management
- Uses double-buffering for smooth rendering
- Manages texture memory efficiently
- Implements sprite caching

## Performance Considerations
1. Use hardware sprites when possible
2. Implement dirty rectangle tracking
3. Optimize texture loading
4. Minimize screen mode switches

## Best Practices
1. Always check return values from graphics functions
2. Clean up resources when switching screens
3. Use appropriate color depths
4. Implement proper error handling

## Example Usage
```c
// Initialize graphics
InitWallset();
InitUIGraphics();

// Render a wall
RenderWall(100, 100, WALL_TYPE_STANDARD);

// Draw UI element
DrawUIElement(10, 10, UI_TYPE_HEALTH);
```

## Troubleshooting
Common issues and solutions:
1. Screen mode not supported
   - Check AGA compatibility
   - Verify screen mode settings
2. Memory allocation failures
   - Reduce texture quality
   - Implement memory cleanup
3. Rendering artifacts
   - Check sprite alignment
   - Verify texture loading 