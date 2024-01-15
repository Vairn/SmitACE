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

typedef struct {
  UWORD uwX;
  UWORD uwY;
  UWORD uwWidth;
  UWORD uwHeight;

} tUigfxSprite;


void LoadUIGraphics(const char* sFilename);

void DrawUIGfx(tBitMap* pDest, tUigfxSprite* pSprite, UWORD uwX, UWORD uwY);

void DestroyUIGraphics(void);
