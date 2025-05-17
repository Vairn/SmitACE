# Game System Documentation

## Overview
The game system is the core component of SmitACE, managing the game loop, state transitions, and overall game flow. It coordinates between different subsystems and handles user input.

## Core Components

### Main Game Loop (`game.c`)
The main game loop handles the primary game flow and state management.

#### Key Functions
```c
// Initialize the game
void InitGame(void);

// Main game loop
void GameLoop(void);

// Update game state
void UpdateGameState(void);

// Handle user input
void ProcessInput(void);
```

### Game State Management (`gameState.c`)
Manages different game states and transitions between them.

#### Key Functions
```c
// Change game state
void ChangeGameState(GameState newState);

// Get current game state
GameState GetCurrentState(void);

// Update current state
void UpdateCurrentState(void);
```

### Game UI (`game_ui.c`)
Handles the game's user interface elements and HUD.

#### Key Functions
```c
// Initialize game UI
void InitGameUI(void);

// Update UI elements
void UpdateGameUI(void);

// Draw UI
void DrawGameUI(void);
```

## Game States
The game has several distinct states:
1. Title Screen (`title.c`)
2. Intro Sequence (`intro.c`)
3. Loading Screen (`loading.c`)
4. Main Game
5. Game Over (`gameOver.c`)
6. Victory Screen (`gameWin.c`)

## Rendering System (`Renderer.c`)
Handles the main rendering pipeline and coordinates with the graphics system.

### Key Functions
```c
// Initialize renderer
void InitRenderer(void);

// Render current frame
void RenderFrame(void);

// Update view
void UpdateView(void);
```

## Graphics Utilities (`gfx_util.c`)
Common graphics utility functions used across the game.

### Key Functions
```c
// Load graphics resources
void LoadGraphics(void);

// Clear screen
void ClearScreen(void);

// Draw sprite
void DrawSprite(int x, int y, SpriteType type);
```

## State Transitions
The game implements smooth transitions between states:
1. Fade effects
2. Loading screens
3. State-specific animations
4. Resource management

## Memory Management
- State-specific resource loading
- Dynamic memory allocation
- Resource cleanup on state change
- Memory pool management

## Performance Considerations
1. Optimize state transitions
2. Efficient resource loading
3. Minimize memory fragmentation
4. Implement proper cleanup

## Best Practices
1. Always check state validity
2. Implement proper error handling
3. Use state machines for complex flows
4. Document state dependencies

## Example Usage
```c
// Initialize game
InitGame();

// Main game loop
while (IsGameRunning()) {
    ProcessInput();
    UpdateGameState();
    RenderFrame();
}

// Handle game over
if (IsGameOver()) {
    ChangeGameState(GAME_STATE_OVER);
}
```

## Troubleshooting
Common issues and solutions:
1. State transition errors
   - Verify state dependencies
   - Check resource loading
   - Validate state machine
2. Memory leaks
   - Monitor allocations
   - Implement proper cleanup
   - Use memory tracking
3. Performance issues
   - Profile state transitions
   - Optimize resource loading
   - Implement caching 