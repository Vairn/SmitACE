
#include "smite.h"

#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/joy.h>
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/utils/palette.h>
#include <ace/managers/ptplayer.h>
#include <ace/managers/mouse.h>
#include "screen.h"
#include "mouse_pointer.h"

static tScreen *g_pMainScreen;

static void winGsCreate(void)
{

	logBlockBegin("winGsCreate()");
	systemUse();

	g_pMainScreen = ScreenGetActive();
	// UWORD pPaletteRef[256];
	paletteLoadFromPath("data/Win.plt", g_pMainScreen->_pFade->pPaletteRef, 255);
	
	tBitMap *pLogo = bitmapCreateFromPath("data/Win_i.bm", 0);
	
	blitCopy(
		pLogo, 0, 0, g_pMainScreen->_pBfr->pBack,
		0, 0,
		320, 256, MINTERM_COOKIE);

	blitCopy(
		pLogo, 0, 0, g_pMainScreen->_pBfr->pFront,
		0, 0,
		320, 256, MINTERM_COOKIE);

	bitmapDestroy(pLogo);

	ScreenFadeFromBlack(NULL, 7, 0);

	systemUnuse();
	timerCreate();
}

static void fadeCompleteWin(void)
{
  stateChange(g_pStateMachineGame, &g_sStateTitle);
}


static void winGsLoop(void)
{
		  if (keyUse(KEY_RETURN) || keyUse(KEY_ESCAPE) || keyUse(KEY_SPACE) ||
		keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		joyUse(JOY1 + JOY_FIRE) || joyUse(JOY2 + JOY_FIRE) || mouseUse(MOUSE_PORT_1, MOUSE_LMB) || mouseUse(MOUSE_PORT_1, MOUSE_RMB) || timerGet() > 200)
    {
		ScreenFadeToBlack(NULL, 7, fadeCompleteWin);
	}
	
	ScreenUpdate();
}

static void winGsDestroy(void)
{
	
	// unload stuff here
}



tState g_sStateWin = {
	.cbCreate = winGsCreate, .cbLoop = winGsLoop, .cbDestroy = winGsDestroy};