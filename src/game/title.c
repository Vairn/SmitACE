#include "title.h"
#include "smite.h"

#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/joy.h>
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/utils/palette.h>
#include <ace/managers/ptplayer.h>

#include "screen.h"

static void titleGsCreate(void) 
{

	logBlockBegin("titleGsCreate()");
	systemUse();
	
	tScreen* pScreen = ScreenGetActive();
  //UWORD pPaletteRef[256];
	paletteLoad("data/title.plt", pScreen->_pFade->pPaletteRef, 255);
	tBitMap *pLogo = bitmapCreateFromFile("data/title.bm", 0);
  	blitCopy(
		pLogo, 0, 0, pScreen->_pBfr->pBack,
		0,0,
    320, 256, MINTERM_COOKIE
	);
	systemUnuse();
  ScreenFadeFromBlack(NULL, 7, 0);
	
}

static void titleGsLoop(void)
{
    if (keyUse(KEY_RETURN) || keyUse(KEY_ESCAPE) || keyUse(KEY_SPACE) ||
		keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		joyUse(JOY1 + JOY_FIRE) || joyUse(JOY2 + JOY_FIRE))
    {
       gameExit();
       //stateChange(g_pStateMachineGame, &g_sStateTitle);
      //  statePush(g_pStateMachineGame, &g_sStateTitle);
    }

  ScreenUpdate();
	
}

static void titleGsDestroy(void)
{
    // unload stuff here
    ScreenFadeToBlack(NULL, 7, 0);
}


tState g_sStateTitle = {
	.cbCreate = titleGsCreate, .cbLoop = titleGsLoop, .cbDestroy = titleGsDestroy
};