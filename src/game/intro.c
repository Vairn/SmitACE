#include "intro.h"
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
//tState g_sStateLogo;
//tState g_sStateIntro;

static void introGsCreate(void) 
{

	logBlockBegin("introGsCreate()");
	tScreen* pScreen = ScreenGetActive();
  //UWORD pPaletteRef[256];
	paletteLoad("data/jamelogo.plt", pScreen->_pFade->pPaletteRef, 255);
	tBitMap *pLogo = bitmapCreateFromFile("data/jamelogo.bm", 0);
  	blitCopy(
		pLogo, 0, 0, pScreen->_pBfr->pBack,
		0,0,
    320, 256, MINTERM_COOKIE
	);
	systemUnuse();
  ScreenFadeFromBlack(NULL, 7, 0);
}

static void logoGsCreate(void) 
{

  logBlockBegin("logoGsCreate()");
  tScreen* pScreen = ScreenGetActive();
  paletteLoad("data/playfield.plt", pScreen->_pFade->pPaletteRef, 255);
	tBitMap *pLogo = bitmapCreateFromFile("data/dt.bm", 0);
  	blitCopy(
		pLogo, 0, 0, pScreen->_pBfr->pBack,
		0,0,
    320, 256, MINTERM_COOKIE
	);
	
      
  systemUnuse();
  ScreenFadeFromBlack(NULL, 7, 0);
}
static void introGsLoop(void)
{
    if (keyUse(KEY_RETURN) || keyUse(KEY_ESCAPE) || keyUse(KEY_SPACE) ||
		keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		joyUse(JOY1 + JOY_FIRE) || joyUse(JOY2 + JOY_FIRE))
    {
      if(g_pStateMachineGame->pCurrent == &g_sStateLogo)
      {
        stateChange(g_pStateMachineGame, &g_sStateIntro);
      }
      else{
        stateChange(g_pStateMachineGame, &g_sStateTitle);
      }
       
      //  statePush(g_pStateMachineGame, &g_sStateTitle);
    }

    ScreenUpdate();
}

static void introGsDestroy(void)
{
  ScreenFadeToBlack(NULL, 7, 0);
    // unload stuff here
}


tState g_sStateIntro = {
	.cbCreate = introGsCreate, .cbLoop = introGsLoop, .cbDestroy = introGsDestroy
};

tState g_sStateLogo = {
  .cbCreate = logoGsCreate, .cbLoop = introGsLoop, .cbDestroy = introGsDestroy
};