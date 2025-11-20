#include "game.h"

#include <ace/managers/mouse.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/blit.h>
#include <ace/utils/sprite.h>
#include <ace/managers/bob.h>

#include "gameState.h"
#include "screen.h"
#include "Renderer.h"
#include "mouse_pointer.h"
#include "maze.h"

#include "game_ui.h"
#include "game_ui_regions.h"
#include "script.h"
ULONG seed = 1;
#define SOFFX 5
UBYTE s_lastMoveResult = 0;
ULONG rand()
{
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return seed;
}

void srand(ULONG new_seed)
{
    seed = new_seed;
}

tScreen *pScreen = NULL;
tWallset *pWallset = NULL;
tBitMap *pBackground = NULL;

static ULONG s_ulSampleSize;
UWORD *g_pModSamples;
tPtplayerMod *pMod;

UBYTE g_ubGameActive = 0;
UBYTE g_ubRedrawRequire = 0;
tBob g_pBob;
tBitMap *temp = NULL;

// Track which movement button is currently pressed (for repeat-while-held)
static UBYTE s_ubPressedMovementButton = 0;
static UBYTE s_ubPressedTurnButton = 0;

void handleEquipmentClicked(WORD slotID)
{
}

void handleEquipmentUsed(WORD slotID)
{
}

void handleInventoryClicked(WORD slotID)
{
}

void handleInventoryScrolled(WORD direction, UBYTE page)
{
}

void handleMapClicked(void)
{
}

void TurnRight()
{
    g_pGameState->m_pCurrentParty->_PartyFacing++;
    g_pGameState->m_pCurrentParty->_PartyFacing %= 4;
    g_ubRedrawRequire = 2;
}

void TurnLeft()
{
    g_pGameState->m_pCurrentParty->_PartyFacing--;
    g_pGameState->m_pCurrentParty->_PartyFacing %= 4;
    g_ubRedrawRequire = 2;
}

static void handleEventTrigger(void)
{
    if (s_lastMoveResult == 3) {
        tMazeEvent* pEvent = mazeFindEventAtPosition(g_pGameState->m_pCurrentMaze, 
            g_pGameState->m_pCurrentParty->_PartyX, g_pGameState->m_pCurrentParty->_PartyY);
        if (pEvent) {
            handleEvent(g_pGameState->m_pCurrentMaze, pEvent);
        }
    }
}

static void updateStandingOnEventTrigger(void)
{
    // Check if player is standing on an event trigger cell
    UBYTE cellType = mazeGetCell(g_pGameState->m_pCurrentMaze, 
        g_pGameState->m_pCurrentParty->_PartyX, 
        g_pGameState->m_pCurrentParty->_PartyY);
    
    if (cellType == MAZE_EVENT_TRIGGER) {
        tMazeEvent* pEvent = mazeFindEventAtPosition(g_pGameState->m_pCurrentMaze, 
            g_pGameState->m_pCurrentParty->_PartyX, 
            g_pGameState->m_pCurrentParty->_PartyY);
        if (pEvent && pEvent->_eventType == EVENT_BATTERY_CHARGER) {
            // Only charge if charger has charge left
            handleEvent(g_pGameState->m_pCurrentMaze, pEvent);
        }
    }
}

void MoveRight()
{
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing + 1;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    handleEventTrigger();
    g_ubRedrawRequire = 2;
}

void MoveLeft()
{
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing + 3;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    handleEventTrigger();
    g_ubRedrawRequire = 2;
}

void MoveBackwards()
{
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing + 2;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    handleEventTrigger();
    g_ubRedrawRequire = 2;
}

void MoveForwards()
{
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    handleEventTrigger();
    g_ubRedrawRequire = 2;
}

void cbGameOnUnhovered(Region *pRegion)
{
}

void cbGameOnHovered(Region *pRegion)
{
    UBYTE id = ((UBYTE)(ULONG)pRegion->context);
    if (id == GAME_UI_GADGET_BATTERY)
    {
        // 7gameExit();
    }
}

void handleDoorClick(UBYTE x, UBYTE y)
{
    tMaze* pMaze = g_pGameState->m_pCurrentMaze;
    UBYTE cellType = mazeGetCell(pMaze, x, y);
    
    switch (cellType)
    {
    case MAZE_DOOR:
        // Create door event to open door (uses event position, no data needed)
        {
            UBYTE eventData[1] = {0};
            tMazeEvent* pEvent = mazeEventCreate(x, y, EVENT_OPENDOOR, 0, eventData);
            handleEvent(pMaze, pEvent);
            mazeRemoveEvent(pMaze, pEvent);
            g_ubRedrawRequire = 2;
        }
        break;
        
    case MAZE_DOOR_OPEN:
        // Create door event to close door (uses event position, no data needed)
        {
            UBYTE eventData[1] = {0};
            tMazeEvent* pEvent = mazeEventCreate(x, y, EVENT_CLOSEDOOR, 0, eventData);
            handleEvent(pMaze, pEvent);
            mazeRemoveEvent(pMaze, pEvent);
            g_ubRedrawRequire = 2;
        }
        break;
        
    case MAZE_DOOR_LOCKED:
        // Check if player has key (you can implement key checking logic here)
        // For now, just show a message that the door is locked
        // You can add a message event here
        break;
    }
}

void cbGameOnPressed(Region *pRegion, UBYTE ubLeft, UBYTE ubRight)
{
    UBYTE id = ((UBYTE)(ULONG)pRegion->context);
    switch (id)
    {
    case GAME_UI_GADGET_FORWARD:
        if (ubLeft) {
            s_ubPressedMovementButton = GAME_UI_GADGET_FORWARD;
            MoveForwards();
        }
        break;
    case GAME_UI_GADGET_BACKWARD:
        if (ubLeft) {
            s_ubPressedMovementButton = GAME_UI_GADGET_BACKWARD;
            MoveBackwards();
        }
        break;
    case GAME_UI_GADGET_LEFT:
        if (ubLeft) {
            s_ubPressedMovementButton = GAME_UI_GADGET_LEFT;
            MoveLeft();
        }
        break;
    case GAME_UI_GADGET_RIGHT:
        if (ubLeft) {
            s_ubPressedMovementButton = GAME_UI_GADGET_RIGHT;
            MoveRight();
        }
        break;
    case GAME_UI_GADGET_TURN_LEFT:
        if (ubLeft) {
            s_ubPressedTurnButton = GAME_UI_GADGET_TURN_LEFT;
            TurnLeft();
        }
        break;
    case GAME_UI_GADGET_TURN_RIGHT:
        if (ubLeft) {
            s_ubPressedTurnButton = GAME_UI_GADGET_TURN_RIGHT;
            TurnRight();
        }
        break;
    case GAME_UI_GADGET_EQUIPMENT_1:
    case GAME_UI_GADGET_EQUIPMENT_2:
    case GAME_UI_GADGET_EQUIPMENT_3:
    case GAME_UI_GADGET_EQUIPMENT_4:
        if (ubLeft)
            handleEquipmentClicked(id - GAME_UI_GADGET_EQUIPMENT_1);
        if (ubRight)
            handleEquipmentUsed(id - GAME_UI_GADGET_EQUIPMENT_1);
        break;
    case GAME_UI_GADGET_INV_SLOT_1:
    case GAME_UI_GADGET_INV_SLOT_2:
    case GAME_UI_GADGET_INV_SLOT_3:
    case GAME_UI_GADGET_INV_SLOT_4:
    case GAME_UI_GADGET_INV_SLOT_5:
    case GAME_UI_GADGET_INV_SLOT_6:
        if (ubLeft)
            handleInventoryClicked(id - GAME_UI_GADGET_INV_SLOT_1);
        break;
    case GAME_UI_GADGET_INV_DOWN:
    case GAME_UI_GADGET_INV_UP:
        handleInventoryScrolled(id - GAME_UI_GADGET_INV_UP, ubRight);
        break;
    case GAME_UI_GADGET_MAP:
        handleMapClicked();
        break;
    case VIEWPORT_UI_GADGET_DOOR:
        if (ubLeft)
        {
            // Get the door position based on player's position and facing
            UBYTE doorX = g_pGameState->m_pCurrentParty->_PartyX;
            UBYTE doorY = g_pGameState->m_pCurrentParty->_PartyY;
            switch (g_pGameState->m_pCurrentParty->_PartyFacing)
            {
            case 0: // North
                doorY--;
                break;
            case 1: // East
                doorX++;
                break;
            case 2: // South
                doorY++;
                break;
            case 3: // West
                doorX--;
                break;
            }
            handleDoorClick(doorX, doorY);
        }
        break;
    default:
        break;
    }
}

void cbGameOnReleased(Region *pRegion, UBYTE ubLeft, UBYTE ubRight)
{
    UBYTE id = ((UBYTE)(ULONG)pRegion->context);
    // Clear pressed movement button if this region was released
    if (id == s_ubPressedMovementButton) {
        s_ubPressedMovementButton = 0;
    }
    // Clear pressed turn button if this region was released
    if (id == s_ubPressedTurnButton) {
        s_ubPressedTurnButton = 0;
    }
}

static void fadeCompleteNoBat(void)
{
    ptplayerEnableMusic(0);
    ptplayerModDestroy(pMod);

    stateChange(g_pStateMachineGame, &g_sStateGameOver);
}

static void fadeInComplete(void)
{
    g_ubGameActive = 1;
}
static void gameGsCreate(void)
{
    systemUse();
    InitNewGame();
    pScreen = ScreenGetActive();

    g_pGameState->m_pCurrentMaze = mazeCreateDemoData();
    g_pGameState->m_pCurrentParty->_PartyX = 2;
    g_pGameState->m_pCurrentParty->_PartyY = 2;
    g_pGameState->m_pCurrentParty->_PartyFacing = 0;

    pWallset = wallsetLoad("data/factory2/factory2.wll");
    g_pGameState->m_pCurrentWallset = pWallset;

    for (int p = 0; p < 256; p++)
    {
        ULONG *pPalRef = (ULONG *)pScreen->_pFade->pPaletteRef;
        pPalRef[p] = pWallset->_palette[3 * p] << 16 | pWallset->_palette[(3 * p) + 1] << 8 | pWallset->_palette[(3 * p + 2)];
    }
    
    pMod = ptplayerModCreateFromPath("data/suspense.mod");
    ptplayerLoadMod(pMod, NULL, 0);
    ptplayerSetMasterVolume(64);
    ptplayerEnableMusic(1);
    tBitMap *pPlayfield = bitmapCreateFromPath("data/playfield.bm", 0);
    blitCopyAligned(pPlayfield, 0, 0, pScreen->_pBfr->pBack, 0, 0, 320, 256);
    // ScreenUpdate();
    blitCopyAligned(pPlayfield, 0, 0, pScreen->_pBfr->pFront, 0, 0, 320, 256);
    bitmapDestroy(pPlayfield);
    // do an initial render to both front and back.
    drawView(g_pGameState, pScreen->_pBfr->pBack);
    drawView(g_pGameState, pScreen->_pBfr->pFront);
    
    
    systemUnuse();

    gameUIInit(cbGameOnHovered, cbGameOnUnhovered, cbGameOnPressed, cbGameOnReleased);
    Layer *pLayer = gameUIGetLayer();
    layerEnablePointerUpdate(pLayer, 1);
    // viewUpdateCLUT(pScreen->_pView);
    ScreenUpdate();
    ScreenFadeFromBlack(NULL, 7, fadeInComplete); // 7 is the speed of the fade
}

static void gameGsLoop(void)
{
    if (g_ubGameActive)
    {
        gameUIUpdate();

        if (g_pGameState->m_pCurrentParty->_BatteryLevel <= 0)
        {
            ScreenFadeToBlack(NULL, 7, fadeCompleteNoBat);
            g_ubGameActive = 0;
            g_ubRedrawRequire = 0;
        }

        // Update door animations
        UBYTE animationsCompleted = doorAnimUpdate(g_pGameState->m_pCurrentMaze);
        
        // If there are active door animations or one just completed, we need to redraw
        if (g_pGameState->m_pCurrentMaze->_doorAnims != NULL || animationsCompleted) {
            g_ubRedrawRequire = 2;
        }
        
        // Update battery chargers (slowly recharge over time)
        updateBatteryChargers(g_pGameState->m_pCurrentMaze);
        
        // Check if standing on event triggers (e.g., battery chargers)
        updateStandingOnEventTrigger();

        // Update monsters
        for (UBYTE i = 0; i < g_pGameState->m_pMonsterList->_numMonsters; i++) {
            tMonster* monster = g_pGameState->m_pMonsterList->_monsters[i];
            if (monster && monster->_state != MONSTER_STATE_DEAD) {
                // Update monster state
                monsterUpdate(monster, g_pGameState->m_pCurrentParty);

                // Check for combat
                if (monster->_state == MONSTER_STATE_AGGRESSIVE) {
                    // If monster is in same cell as party, initiate combat
                    if (monster->_partyPosX == g_pGameState->m_pCurrentParty->_PartyX &&
                        monster->_partyPosY == g_pGameState->m_pCurrentParty->_PartyY) {
                        // Monster attacks first character in party
                        if (g_pGameState->m_pCurrentParty->_numCharacters > 0) {
                            monsterAttack(monster, g_pGameState->m_pCurrentParty->_characters[0]);
                            
                            // Check if monster was defeated
                            if (monster->_base._HP == 0) {
                                monster->_state = MONSTER_STATE_DEAD;
                                monsterDropLoot(monster);
                                // Give experience to party
                                for (UBYTE j = 0; j < g_pGameState->m_pCurrentParty->_numCharacters; j++) {
                                    g_pGameState->m_pCurrentParty->_characters[j]->_Experience += monster->_experienceValue;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (g_ubRedrawRequire)
        {
            drawView(g_pGameState, pScreen->_pBfr->pBack);
            g_ubRedrawRequire--;
        }

        gameUpdateBattery(g_pGameState->m_pCurrentParty->_BatteryLevel);
        
        // Process mouse multiple times per frame for better responsiveness
        mouseProcess();
        mouseProcess();
        
        // Check if a movement button is still pressed (repeat-while-held, like EOB/DM)
        if (s_ubPressedMovementButton != 0 && mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
            UWORD uwMouseX = mouseGetX(MOUSE_PORT_1);
            UWORD uwMouseY = mouseGetY(MOUSE_PORT_1);
            UBYTE ubOverButton = 0;
            
            // Check if mouse is still over the pressed button (using known button coordinates)
            switch (s_ubPressedMovementButton) {
            case GAME_UI_GADGET_FORWARD:
                ubOverButton = (uwMouseX >= 25 && uwMouseX < 43 && uwMouseY >= 192 && uwMouseY < 210);
                break;
            case GAME_UI_GADGET_BACKWARD:
                ubOverButton = (uwMouseX >= 25 && uwMouseX < 43 && uwMouseY >= 212 && uwMouseY < 230);
                break;
            case GAME_UI_GADGET_LEFT:
                ubOverButton = (uwMouseX >= 5 && uwMouseX < 23 && uwMouseY >= 212 && uwMouseY < 230);
                break;
            case GAME_UI_GADGET_RIGHT:
                ubOverButton = (uwMouseX >= 45 && uwMouseX < 63 && uwMouseY >= 212 && uwMouseY < 230);
                break;
            }
            
            if (ubOverButton) {
                // Button still pressed, trigger movement each frame
                switch (s_ubPressedMovementButton) {
                case GAME_UI_GADGET_FORWARD:
                    MoveForwards();
                    break;
                case GAME_UI_GADGET_BACKWARD:
                    MoveBackwards();
                    break;
                case GAME_UI_GADGET_LEFT:
                    MoveLeft();
                    break;
                case GAME_UI_GADGET_RIGHT:
                    MoveRight();
                    break;
                }
            } else {
                // Mouse moved away from button, clear pressed state
                s_ubPressedMovementButton = 0;
            }
        }
        
        // Check if a turn button is still pressed (repeat-while-held)
        if (s_ubPressedTurnButton != 0 && mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
            UWORD uwMouseX = mouseGetX(MOUSE_PORT_1);
            UWORD uwMouseY = mouseGetY(MOUSE_PORT_1);
            UBYTE ubOverButton = 0;
            
            // Check if mouse is still over the pressed button
            switch (s_ubPressedTurnButton) {
            case GAME_UI_GADGET_TURN_LEFT:
                ubOverButton = (uwMouseX >= 5 && uwMouseX < 23 && uwMouseY >= 192 && uwMouseY < 210);
                break;
            case GAME_UI_GADGET_TURN_RIGHT:
                ubOverButton = (uwMouseX >= 45 && uwMouseX < 63 && uwMouseY >= 192 && uwMouseY < 210);
                break;
            }
            
            if (ubOverButton) {
                // Button still pressed, trigger turn each frame
                switch (s_ubPressedTurnButton) {
                case GAME_UI_GADGET_TURN_LEFT:
                    TurnLeft();
                    break;
                case GAME_UI_GADGET_TURN_RIGHT:
                    TurnRight();
                    break;
                }
            } else {
                // Mouse moved away from button, clear pressed state
                s_ubPressedTurnButton = 0;
            }
        }
        
        if (keyCheck(KEY_ESCAPE))
            gameExit();

        if (keyCheck(KEY_W) || keyCheck(KEY_UP))
        {
            MoveForwards();
        }
        if (keyCheck(KEY_S) || keyCheck(KEY_DOWN))
        {
            MoveBackwards();
        }
        if (keyCheck(KEY_A) || keyCheck(KEY_LEFT))
        {
            MoveLeft();
        }
        if (keyCheck(KEY_D) || keyCheck(KEY_RIGHT))
        {
            MoveRight();
        }

        if (keyCheck(KEY_Q) || keyCheck(KEY_HELP))
        {
            TurnLeft();
        }
        if (keyCheck(KEY_E) || keyCheck(KEY_DEL))
        {
            TurnRight();
        }
    }

    ScreenUpdate();
}

static void gameGsDestroy(void)
{
    systemUse();
    gameUIDestroy();
    FreeGameState();
    systemUnuse();
}
tState g_sStateGame = {
    .cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy};

tState g_statePaused = {
    .cbCreate = NULL, .cbLoop = NULL, .cbDestroy = NULL};
