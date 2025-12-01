#pragma once

#include "maze.h"
#include <ace/types.h>
#include <ace/utils/bitmap.h>

// Forward declarations
struct _wallset;
typedef struct _wallset tWallset;

// Wall button states
#define WALLBUTTON_STATE_OFF 0
#define WALLBUTTON_STATE_ON 1
#define WALLBUTTON_STATE_PRESSED 2

// Wall button types
#define WALLBUTTON_TYPE_TOGGLE 0      // Toggles on/off
#define WALLBUTTON_TYPE_MOMENTARY 1    // Press and release
#define WALLBUTTON_TYPE_TRIGGER 2     // One-time trigger

typedef struct _wallButton
{
    UBYTE _x;              // X position in maze
    UBYTE _y;              // Y position in maze
    UBYTE _wallSide;       // Which side of wall (0-3: N, E, S, W)
    UBYTE _state;          // Current state
    UBYTE _type;           // Button type
    UBYTE _gfxIndex;       // Graphics index in wallset
    UBYTE _eventType;      // Event to trigger when pressed
    UBYTE _pad;            // Padding for 68020+ alignment
    UBYTE* _pEventData;    // Event data (if any)
    UBYTE _eventDataSize;  // Size of event data
    UBYTE _pad2[3];        // Padding for pointer alignment
    struct _wallButton* _next;
} tWallButton;

typedef struct _wallButtonList
{
    UBYTE _numButtons;
    tWallButton* _buttons;
} tWallButtonList;

// Wall button management
tWallButton* wallButtonCreate(UBYTE x, UBYTE y, UBYTE wallSide, UBYTE type, UBYTE gfxIndex);
void wallButtonDestroy(tWallButton* pButton);
void wallButtonListCreate(tWallButtonList* pList);
void wallButtonListDestroy(tWallButtonList* pList);
void wallButtonAdd(tWallButtonList* pList, tWallButton* pButton);
void wallButtonRemove(tWallButtonList* pList, tWallButton* pButton);

// Wall button interaction
UBYTE wallButtonClick(tWallButton* pButton, tMaze* pMaze);
tWallButton* wallButtonFindAt(tWallButtonList* pList, UBYTE x, UBYTE y, UBYTE wallSide);

// Wall button rendering
void wallButtonRender(tWallButton* pButton, tWallset* pWallset, tBitMap* pBuffer);

