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

/* Wallset tile _type values not tied to maze cell types (decorative / UI overlays). */
#define WALL_GFX_WALL_BUTTON 250
#define WALL_GFX_DOOR_BUTTON 251

typedef struct _wallGfx
{
    WORD _location[2];
    WORD _screen[2];
    UWORD _x;
    UWORD _y;
    UWORD _width;
    UWORD _height;
    UBYTE _type;
    UBYTE _setIndex;
    

} tWallGfx;

typedef struct _wallset 
{
    
    UWORD _tilesetCount;
    UWORD _gfxCount;
    UBYTE _paletteSize;
    UBYTE* _palette;
    tWallGfx** _tileset;
    tBitMap** _gfx;
    tBitMap** _mask;
    /** Per-plane tile counts (length _gfxCount); sum equals _tilesetCount. Filled at load; required for save. */
    UBYTE* _tilesPerGroup;
    UBYTE _header[3];

} tWallset;


tWallset* wallsetLoad(const char* filename);
void wallsetSave(tWallset* pWallset, const char* filename);
void wallsetDestroy(tWallset* pWallset);

void wallsetBlitTile(tWallset* pWallset, tBitMap* pDest, UWORD tile, WORD x, WORD y);
