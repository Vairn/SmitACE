#include "gameOver.h"
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

static void gameOverGsCreate(void)
{

	logBlockBegin("gameOverGsCreate()");
	systemUse();

	g_pMainScreen = ScreenGetActive();
	// UWORD pPaletteRef[256];
	paletteLoad("data/NoBattery.plt", g_pMainScreen->_pFade->pPaletteRef, 255);
	tBitMap *pLogo = bitmapCreateFromFile("data/NoBattery.bm", 0);
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
	mouse_pointer_create("data/pointers.bm");

	systemUnuse();
}


static void gameOverGsLoop(void)
{
	mouse_pointer_update();
	ScreenUpdate();
}

static void gameOverGsDestroy(void)
{
	mouse_pointer_destroy();
	// unload stuff here
}



tState g_sStateGameOver = {
	.cbCreate = gameOverGsCreate, .cbLoop = gameOverGsLoop, .cbDestroy = gameOverGsDestroy};