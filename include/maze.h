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

typedef struct _mazeevent {
    UBYTE _x;
    UBYTE _y;
    UBYTE _eventType;
    UBYTE* _eventData;
    UBYTE _eventDataSize;
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
