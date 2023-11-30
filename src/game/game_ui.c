#include "smite.h"
#include "game_ui.h"
#include "game_ui_regions.h"
#include "mouse_pointer.h"

static Layer *s_gameUILayer = NULL;
RegionId uiRegions[VIEWPORT_UI_GADGET_MAX];
static cbRegion s_cbOnHovered;
static cbRegion s_cbOnUnHovered;
static cbRegion s_cbOnPressed;
static cbRegion s_cbOnReleased;

RegionId gameAddRegion(UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight, UBYTE ubContext, UBYTE ubPointer)
{
    Region new_region = {
		.pointer = ubPointer,
		.bounds = (tUwRect){.uwX = uwX, .uwY = uwY, .uwWidth = uwWidth, .uwHeight = uwHeight},
		.cbOnHovered = s_cbOnHovered,
		.cbOnUnhovered = s_cbOnUnHovered,
		.cbOnPressed = s_cbOnPressed,
		.cbOnReleased = s_cbOnReleased,
		.context = (void *)ubContext};

    RegionId regionId = layerAddRegion(s_gameUILayer, &new_region);
    uiRegions[ubContext] = regionId;
    return regionId;
}

void gameUIInit(cbRegion cbOnHovered, cbRegion cbOnUnHovered, cbRegion cbOnPressed, cbRegion cbOnReleased)
{
    s_gameUILayer = layerCreate();
    s_cbOnHovered = cbOnHovered;
    s_cbOnUnHovered = cbOnUnHovered;
    s_cbOnPressed = cbOnPressed;
    s_cbOnReleased = cbOnReleased;

    // Create regions
    gameAddRegion(0,0,8,8,GAME_UI_GADGET_MENU, MOUSE_POINTER);
    //gameAddRegion()

    // Movement
    gameAddRegion(5,192,18,18,GAME_UI_GADGET_TURN_LEFT, MOUSE_USE);
    gameAddRegion(45,192,18,18,GAME_UI_GADGET_TURN_RIGHT, MOUSE_USE);
    gameAddRegion(25,192,18,18,GAME_UI_GADGET_FORWARD, MOUSE_USE);
    gameAddRegion(25,212,18,18,GAME_UI_GADGET_BACKWARD, MOUSE_USE);
    gameAddRegion(5,212,18,18,GAME_UI_GADGET_LEFT, MOUSE_USE);
    gameAddRegion(45,212,18,18,GAME_UI_GADGET_RIGHT, MOUSE_USE);

    // Inventory

    // Screen

    // Misc



	layerSetEnable(s_gameUILayer, 1);
	layerSetUpdateOutsideBounds(s_gameUILayer, 1);
    
}

void gameUIUpdate()
{
    layerUpdate(s_gameUILayer);
}

void gameUIDestroy()
{
    layerDestroy(s_gameUILayer);
    s_gameUILayer = NULL;
}

Layer *gameUIGetLayer()
{
    return s_gameUILayer;
}