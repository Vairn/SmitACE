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

typedef struct _wallGfx
{
    WORD _location[2];
    WORD _screen[2];
    UWORD _x;
    UWORD _y;
    UWORD _width;
    UWORD _height;
    UBYTE _type;
    

} tWallGfx;

typedef struct _wallset 
{
    
    UWORD _tilesetCount;
    UBYTE _paletteSize;
    UBYTE* _palette;
    tWallGfx** _tileset;
    tBitMap* _gfx;
    tBitMap* _mask;

} tWallset;


tWallset* wallsetLoad(const char* filename);
void wallsetSave(tWallset* pWallset, const char* filename);
void wallsetDestroy(tWallset* pWallset);

void wallsetBlitTile(tWallset* pWallset, tBitMap* pDest, UWORD tile, WORD x, WORD y);
