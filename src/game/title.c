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
#include <ace/managers/mouse.h>
#include "screen.h"
#include "mouse_pointer.h"
#include "layer.h"

#define BUTTON_WIDTH 68
#define BUTTON_HEIGHT 12

#define NEW_GAME_IDX 0
#define LOAD_GAME_IDX 1
#define QUIT_IDX 2

static Layer *s_menuLayer = NULL;
static tBitMap *g_pMenuItems;
static tScreen *g_pMainScreen;

// Regions for Title Menu
RegionId newGameRegionId;
RegionId loadGameRegionId;
RegionId exitGameRegionId;

static void fadeCompleteTitle()
{
	stateChange(g_pStateMachineGame, &g_sStateGame);
}

static void fadeCompleteExit()
{
	gameExit();
}
void cbOnHovered(Region *pRegion)
{
	UBYTE id = ((UBYTE)(ULONG)pRegion->context);

	blitCopy(
		g_pMenuItems, 0, (24 * id) + 12,
		g_pMainScreen->_pBfr->pBack, pRegion->bounds.uwX, pRegion->bounds.uwY,
		pRegion->bounds.uwWidth, pRegion->bounds.uwHeight,
		MINTERM_COOKIE);

	blitCopy(
		g_pMenuItems, 0, (24 * id) + 12,
		g_pMainScreen->_pBfr->pFront, pRegion->bounds.uwX, pRegion->bounds.uwY,
		pRegion->bounds.uwWidth, pRegion->bounds.uwHeight,
		MINTERM_COOKIE);
}

void cbOnUnhovered(Region *pRegion)
{
	UBYTE id = ((UBYTE)(ULONG)pRegion->context);

	blitCopy(
		g_pMenuItems, 0, (24 * (id)),
		g_pMainScreen->_pBfr->pBack, pRegion->bounds.uwX, pRegion->bounds.uwY,
		pRegion->bounds.uwWidth, pRegion->bounds.uwHeight,
		MINTERM_COOKIE);

	blitCopy(
		g_pMenuItems, 0, (24 * (id)),
		g_pMainScreen->_pBfr->pFront, pRegion->bounds.uwX, pRegion->bounds.uwY,
		pRegion->bounds.uwWidth, pRegion->bounds.uwHeight,
		MINTERM_COOKIE);
}

void cbOnPressed(Region *pRegion, UBYTE left, UBYTE right)
{
	logWrite("Pressing %d", (UWORD)(ULONG)pRegion->context);
	UBYTE id = ((UBYTE)(ULONG)pRegion->context);
	switch (id)
	{
	case 0:
		ScreenFadeToBlack(NULL, 7, fadeCompleteTitle);
		break;
	case 1:
		break;
	case 2:
		ScreenFadeToBlack(NULL, 7, fadeCompleteExit);
		break;
	default:
		break;
	}
}

void cbOnReleased(Region *pRegion, UBYTE ubLeft, UBYTE ubRight)
{
	logWrite("Releasing %d", (UWORD)(ULONG)pRegion->context);
}

static void titleGsCreate(void)
{

	logBlockBegin("titleGsCreate()");
	systemUse();

	g_pMainScreen = ScreenGetActive();
	// UWORD pPaletteRef[256];
	paletteLoad("data/title.plt", g_pMainScreen->_pFade->pPaletteRef, 255);
	tBitMap *pLogo = bitmapCreateFromFile("data/title.bm", 0);
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
	

	g_pMenuItems = bitmapCreateFromFile("data/MenuItems.bm", 0);

	s_menuLayer = layerCreate();
	Region newGameButton = {
		.pointer = MOUSE_USE,
		.bounds = (tUwRect){.uwX = 223, .uwY = 60, .uwWidth = BUTTON_WIDTH, .uwHeight = BUTTON_HEIGHT},
		.cbOnHovered = cbOnHovered,
		.cbOnUnhovered = cbOnUnhovered,
		.cbOnPressed = cbOnPressed,
		.cbOnReleased = cbOnReleased,
		.context = (void *)NEW_GAME_IDX,
	};

	Region loadGameButton = {
		.pointer = MOUSE_USE,
		.bounds = (tUwRect){.uwX = 223, .uwY = 93, .uwWidth = BUTTON_WIDTH, .uwHeight = BUTTON_HEIGHT},
		.cbOnHovered = cbOnHovered,
		.cbOnUnhovered = cbOnUnhovered,
		.cbOnPressed = cbOnPressed,
		.cbOnReleased = cbOnReleased,
		.context = (void *)LOAD_GAME_IDX};

	Region exitGameButton = {
		.pointer = MOUSE_USE,
		.bounds = (tUwRect){.uwX = 223, .uwY = 125, .uwWidth = BUTTON_WIDTH, .uwHeight = BUTTON_HEIGHT},
		.cbOnHovered = cbOnHovered,
		.cbOnUnhovered = cbOnUnhovered,
		.cbOnPressed = cbOnPressed,
		.cbOnReleased = cbOnReleased,
		.context = (void *)QUIT_IDX};

	newGameRegionId = layerAddRegion(s_menuLayer, &newGameButton);
	loadGameRegionId = layerAddRegion(s_menuLayer, &loadGameButton);
	exitGameRegionId = layerAddRegion(s_menuLayer, &exitGameButton);

	blitCopy(g_pMenuItems, 0, 0, g_pMainScreen->_pBfr->pBack, 223, 60, BUTTON_WIDTH, BUTTON_HEIGHT, MINTERM_COOKIE);
	blitCopy(g_pMenuItems, 0, 24, g_pMainScreen->_pBfr->pBack, 223, 93, BUTTON_WIDTH, BUTTON_HEIGHT, MINTERM_COOKIE);
	blitCopy(g_pMenuItems, 0, 48, g_pMainScreen->_pBfr->pBack, 223, 125, BUTTON_WIDTH, BUTTON_HEIGHT, MINTERM_COOKIE);

	blitCopy(g_pMenuItems, 0, 0, g_pMainScreen->_pBfr->pFront, 223, 60, BUTTON_WIDTH, BUTTON_HEIGHT, MINTERM_COOKIE);
	blitCopy(g_pMenuItems, 0, 24, g_pMainScreen->_pBfr->pFront, 223, 93, BUTTON_WIDTH, BUTTON_HEIGHT, MINTERM_COOKIE);
	blitCopy(g_pMenuItems, 0, 48, g_pMainScreen->_pBfr->pFront, 223, 125, BUTTON_WIDTH, BUTTON_HEIGHT, MINTERM_COOKIE);

	layerSetEnable(s_menuLayer, 1);
	layerSetUpdateOutsideBounds(s_menuLayer, 1);
	systemUnuse();
}


static void titleGsLoop(void)
{
	layerUpdate(s_menuLayer);
	ScreenUpdate();
}

static void titleGsDestroy(void)
{
	bitmapDestroy(g_pMenuItems);
	layerRemoveRegion(s_menuLayer, newGameRegionId);
	layerRemoveRegion(s_menuLayer, loadGameRegionId);
	layerRemoveRegion(s_menuLayer, exitGameRegionId);
	
	layerDestroy(s_menuLayer);
	
	// unload stuff here
}

tState g_sStateTitle = {
	.cbCreate = titleGsCreate, .cbLoop = titleGsLoop, .cbDestroy = titleGsDestroy};