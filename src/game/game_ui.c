#include "smite.h"
#include "game_ui.h"
#include "game_ui_regions.h"
#include "mouse_pointer.h"
#include "screen.h"
#include <ace/managers/blit.h>

static Layer *s_gameUILayer = NULL;
RegionId uiRegions[VIEWPORT_UI_GADGET_MAX];
static cbRegion s_cbOnHovered;
static cbRegion s_cbOnUnHovered;
static cbRegionClick s_cbOnPressed;
static cbRegionClick s_cbOnReleased;

RegionId gameAddRegion(UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight, UBYTE ubContext, UBYTE ubPointer)
{
    Region new_region = {
		.pointer = ubPointer,
		.bounds = (tUwRect){.uwX = uwX, .uwY = uwY, .uwWidth = uwWidth, .uwHeight = uwHeight},
		.cbOnHovered = s_cbOnHovered,
		.cbOnUnhovered = s_cbOnUnHovered,
		.cbOnPressed = s_cbOnPressed,
		.cbOnReleased = s_cbOnReleased,
		.context = (void *)(int)ubContext};

    RegionId regionId = layerAddRegion(s_gameUILayer, &new_region);
    uiRegions[ubContext] = regionId;
    return regionId;
}

void gameUIInit(cbRegion cbOnHovered, cbRegion cbOnUnHovered, cbRegionClick cbOnPressed, cbRegionClick cbOnReleased)
{
    s_gameUILayer = layerCreate();
    s_cbOnHovered = cbOnHovered;
    s_cbOnUnHovered = cbOnUnHovered;
    s_cbOnPressed = cbOnPressed;
    s_cbOnReleased = cbOnReleased;

    // Create regions
    gameAddRegion(261,42,45,45,GAME_UI_GADGET_MENU, MOUSE_POINTER);
    //gameAddRegion()

    // Movement
    gameAddRegion(5,192,18,18,GAME_UI_GADGET_TURN_LEFT, MOUSE_USE);
    gameAddRegion(45,192,18,18,GAME_UI_GADGET_TURN_RIGHT, MOUSE_USE);
    gameAddRegion(25,192,18,18,GAME_UI_GADGET_FORWARD, MOUSE_USE);
    gameAddRegion(25,212,18,18,GAME_UI_GADGET_BACKWARD, MOUSE_USE);
    gameAddRegion(5,212,18,18,GAME_UI_GADGET_LEFT, MOUSE_USE);
    gameAddRegion(45,212,18,18,GAME_UI_GADGET_RIGHT, MOUSE_USE);

    // Inventory
    gameAddRegion(252,145,30,28, GAME_UI_GADGET_INV_SLOT_1, MOUSE_USE);
    gameAddRegion(285,145,30,28, GAME_UI_GADGET_INV_SLOT_2, MOUSE_USE);
    gameAddRegion(252,176,30,28, GAME_UI_GADGET_INV_SLOT_3, MOUSE_USE);
    gameAddRegion(285,176,30,28, GAME_UI_GADGET_INV_SLOT_4, MOUSE_USE);
    gameAddRegion(252,208,30,28, GAME_UI_GADGET_INV_SLOT_5, MOUSE_USE);
    gameAddRegion(285,208,30,28, GAME_UI_GADGET_INV_SLOT_6, MOUSE_USE);

    gameAddRegion(252,8,30,28, GAME_UI_GADGET_EQUIPMENT_1, MOUSE_USE);
    gameAddRegion(285,8,30,28, GAME_UI_GADGET_EQUIPMENT_2, MOUSE_USE);
    gameAddRegion(252,92,30,28, GAME_UI_GADGET_EQUIPMENT_3, MOUSE_USE);
    gameAddRegion(285,92,30,28, GAME_UI_GADGET_EQUIPMENT_4, MOUSE_USE);

    gameAddRegion(252,131,63,12, GAME_UI_GADGET_INV_UP, MOUSE_USE);
    gameAddRegion(252,240,63,12, GAME_UI_GADGET_INV_DOWN, MOUSE_USE);
    
    
    // Screen
    gameAddRegion(1,28,248,160, GAME_UI_GADGET_VIEWPORT, MOUSE_POINTER);
    gameAddRegion(2,234,246,21, GAME_UI_GADGET_TEXTFIELD, MOUSE_POINTER);
    // Misc
    gameAddRegion(65,192,22,38, GAME_UI_GADGET_BATTERY, MOUSE_EXAMINE);
    gameAddRegion(178,192,66,38, GAME_UI_GADGET_MAP, MOUSE_EXAMINE);

    // Viewport - door interaction only
    gameAddRegion(1,1,248,160, VIEWPORT_UI_GADGET_DOOR, MOUSE_EXAMINE);


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


void gameUpdateBattery(UBYTE ubBattery)
{
    // Battery region is at (65,192) with size 22x38
    // Draw a vertical bar from bottom up showing charge level (0-100)
    tScreen* pScreen = ScreenGetActive();
    if (!pScreen) return;
    
    // Clamp battery to max 100
    if (ubBattery > 100) {
        ubBattery = 100;
    }
    
    // Battery bar dimensions
    const UWORD BATTERY_X = 65;
    const UWORD BATTERY_Y = 192;
    const UWORD BATTERY_WIDTH = 18;  // Leave 2px border on each side
    const UWORD BATTERY_HEIGHT = 34; // Leave 2px border on top and bottom
    
    // Calculate filled height based on battery level (0-100)
    UWORD filledHeight = (BATTERY_HEIGHT * ubBattery) / 100;
    
    // Clear the battery area (draw empty background)
    blitRect(pScreen->_pBfr->pBack, BATTERY_X + 2, BATTERY_Y + 2, BATTERY_WIDTH, BATTERY_HEIGHT, 0);
    
    // Draw filled portion from bottom up
    if (filledHeight > 0) {
        UBYTE color = 2; // Green color for battery
        if (ubBattery < 25) {
            color = 4; // Red when low
        } else if (ubBattery < 50) {
            color = 3; // Yellow when medium
        }
        
        UWORD barY = BATTERY_Y + 2 + BATTERY_HEIGHT - filledHeight;
        blitRect(pScreen->_pBfr->pBack, BATTERY_X + 2, barY, BATTERY_WIDTH, filledHeight, color);
    }
}