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
typedef struct _pal
    {
        UBYTE r,g,b;
    } pal;

typedef struct _tPalette
{
    pal  _data[256];
} tPalette;

#ifdef AMIGA

typedef struct _tScreen
{

    tView *_pView;
    tVPort *_pVp;
    tSimpleBufferManager *_pBfr;
    tFade *_pFade;
//    tPalette _palette[256];
    
    UBYTE _screenPalette[768];
	UBYTE _palettes[2][768];
	//tPalette _internFadePalette;

} tScreen;

tScreen* CreateNewScreen(BYTE palCount);

void ScreenMakeActive(tScreen* pScreen);

tScreen* ScreenGetActive(void);

void ScreenUpdate(void);

void ScreenFadePalette(LONG delay, void* pFunc);

void ScreenFadeFromBlack(UWORD* pal,LONG delay, void *upFunc);
void ScreenFadeToBlack(UWORD* pal,LONG delay, void *upFunc);
UWORD* ScreenGetPalette();
void ScreenClose();
#endif // AMIGA