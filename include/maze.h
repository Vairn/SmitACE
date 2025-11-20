#pragma once

#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/joy.h>
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/managers/ptplayer.h>
#include <ace/utils/palette.h>
#include <fade.h>

#ifndef AMIGA
#include "amiTypes.h"

#endif

#define MAZE_FLOOR 0
#define MAZE_WALL 1
#define MAZE_DOOR 2
#define MAZE_DOOR_OPEN 3
#define MAZE_DOOR_LOCKED 4
#define MAZE_EVENT_TRIGGER 5

// Door animation states
#define DOOR_ANIM_NONE 0
#define DOOR_ANIM_OPENING 1
#define DOOR_ANIM_CLOSING 2

// Door animation constants
#define DOOR_ANIM_FRAMES 6  // Number of frames in the animation
#define DOOR_ANIM_FRAME_TIME 1  // How many game ticks each frame lasts

typedef struct _doorAnim {
    BYTE x;
    BYTE y;
    UBYTE state;      // DOOR_ANIM_OPENING or DOOR_ANIM_CLOSING
    UBYTE frame;      // Current frame (0 to DOOR_ANIM_FRAMES-1)
    UBYTE timer;      // Frame timer
    UBYTE _pad;       // Padding to ensure pointer is 4-byte aligned for 68020+
    struct _doorAnim* next;
} tDoorAnim;

typedef struct _mazeevent {
    UBYTE _x;
    UBYTE _y;
    UBYTE _eventType;
    UBYTE _pad;       // Padding to ensure pointer is 4-byte aligned for 68020+
    UBYTE* _eventData;
    UBYTE _eventDataSize;
    UBYTE _pad2[3];   // Padding to ensure pointers are 4-byte aligned for 68020+
    struct _mazeevent* _next;
    struct _mazeevent* _prev;
} tMazeEvent;

typedef struct _mazeString
{
    UWORD _length;
    UBYTE* _string;
    struct _mazeString* _next;
}
tMazeString;

typedef struct _maze
{
    UBYTE _width;
    UBYTE _height;
    UWORD _eventCount;
    UWORD _stringCount;
    UBYTE *_mazeData;
    UBYTE *_mazeCol;
    UBYTE *_mazeFloor;
    tMazeEvent *_events;
    tMazeString* _strings;
    tDoorAnim* _doorAnims;  // List of active door animations
} tMaze;

tMaze* mazeCreateDemoData(void);

tMaze* mazeCreate(UBYTE width, UBYTE height);

tMaze* mazeLoad(const char* filename);

void mazeSave(tMaze* pMaze, const char* filename);

UBYTE mazeGetCell(tMaze* pMaze, UBYTE x, UBYTE y);

void mazeSetCell(tMaze* pMaze, UBYTE x, UBYTE y, UBYTE value);

void mazeDraw(tMaze* pMaze, UBYTE x, UBYTE y);
void mazeDraw2D(tMaze* pMaze, UBYTE x, UBYTE y);
void mazeDraw2DRange(tMaze* pMaze, UBYTE x, UBYTE y, UBYTE startX, UBYTE startY, UBYTE endX, UBYTE endY);


tMazeEvent* mazeEventCreate(UBYTE x, UBYTE y, UBYTE eventType, UBYTE eventDataSize, UBYTE* eventData);
void mazeAppendEvent(tMaze* maze, tMazeEvent* newEvent);
void mazeRemoveEvent(tMaze* maze, tMazeEvent* event);

void mazeIterateEvents(tMaze* maze, void (*callback)(tMazeEvent*));

void mazeDelete(tMaze* maze);

void mazeRemoveAllEvents(tMaze* maze);

void mazeAddString(tMaze* maze, char* string, UWORD length);
void mazeRemoveStrings(tMaze* maze);

tDoorAnim* doorAnimCreate(BYTE x, BYTE y, UBYTE state);
void doorAnimAdd(tMaze* maze, tDoorAnim* anim);
void doorAnimRemove(tMaze* maze, tDoorAnim* anim);
void doorAnimUpdate(tMaze* maze);
tDoorAnim* doorAnimFind(tMaze* maze, BYTE x, BYTE y);
