#pragma once

#include "maze.h"
#include <ace/types.h>
#include <ace/utils/bitmap.h>

// Forward declarations
struct _wallset;
typedef struct _wallset tWallset;

// Door button states
#define DOORBUTTON_STATE_OFF 0
#define DOORBUTTON_STATE_ON 1
#define DOORBUTTON_STATE_PRESSED 2

// Door button types
#define DOORBUTTON_TYPE_TOGGLE 0      // Toggles door open/closed
#define DOORBUTTON_TYPE_OPEN_ONLY 1   // Only opens door
#define DOORBUTTON_TYPE_CLOSE_ONLY 2  // Only closes door
#define DOORBUTTON_TYPE_TRIGGER 3     // One-time trigger

typedef struct _doorButton
{
    UBYTE _x;              // X position in maze
    UBYTE _y;              // Y position in maze
    UBYTE _wallSide;       // Which side of wall (0-3: N, E, S, W)
    UBYTE _state;          // Current state
    UBYTE _type;           // Button type
    UBYTE _gfxIndex;       // Graphics index in wallset
    UBYTE _targetDoorX;   // X position of door to control
    UBYTE _targetDoorY;   // Y position of door to control
    UBYTE _pad[2];        // Padding for 68020+ alignment
    struct _doorButton* _next;
} tDoorButton;

typedef struct _doorButtonList
{
    UBYTE _numButtons;
    tDoorButton* _buttons;
} tDoorButtonList;

// Door button management
tDoorButton* doorButtonCreate(UBYTE x, UBYTE y, UBYTE wallSide, UBYTE type, UBYTE gfxIndex, UBYTE targetDoorX, UBYTE targetDoorY);
void doorButtonDestroy(tDoorButton* pButton);
void doorButtonListCreate(tDoorButtonList* pList);
void doorButtonListDestroy(tDoorButtonList* pList);
void doorButtonAdd(tDoorButtonList* pList, tDoorButton* pButton);
void doorButtonRemove(tDoorButtonList* pList, tDoorButton* pButton);

// Door button interaction
UBYTE doorButtonClick(tDoorButton* pButton, tMaze* pMaze);
tDoorButton* doorButtonFindAt(tDoorButtonList* pList, UBYTE x, UBYTE y, UBYTE wallSide);

// Door button rendering
void doorButtonRender(tDoorButton* pButton, tWallset* pWallset, tBitMap* pBuffer);

