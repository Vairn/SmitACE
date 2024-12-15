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

void MoveRight()
{
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing + 1;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    g_ubRedrawRequire = 2;
}

void MoveLeft()
{
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing + 3;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    g_ubRedrawRequire = 2;
}

void MoveBackwards()
{
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing + 2;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    g_ubRedrawRequire = 2;
}

void MoveForwards()
{
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
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

void cbGameOnPressed(Region *pRegion, UBYTE ubLeft, UBYTE ubRight)
{
    UBYTE id = ((UBYTE)(ULONG)pRegion->context);
    switch (id)
    {
    case GAME_UI_GADGET_FORWARD:
        if (ubLeft)
            MoveForwards();
        break;
    case GAME_UI_GADGET_BACKWARD:
        if (ubLeft)
            MoveBackwards();
        break;
    case GAME_UI_GADGET_LEFT:
        if (ubLeft)
            MoveLeft();
        break;
    case GAME_UI_GADGET_RIGHT:
        if (ubLeft)
            MoveRight();
        break;
    case GAME_UI_GADGET_TURN_LEFT:
        if (ubLeft)
            TurnLeft();
        break;
    case GAME_UI_GADGET_TURN_RIGHT:
        if (ubLeft)
            TurnRight();
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
    default:
        break;
    }
}

void cbGameOnReleased(Region *pRegion, UBYTE ubLeft, UBYTE ubRight)
{
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

    
        paletteLoadFromPath("data/playfield.plt", pScreen->_pFade->pPaletteRef, 64);
("data/playfield.plt", pScreen->_pFade->pPaletteRef, 64);
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
            g_ubRedrawRequire =0;
        }

       if (g_ubRedrawRequire)
        {
            drawView(g_pGameState, pScreen->_pBfr->pBack);
            g_ubRedrawRequire--;
        }

        gameUpdateBattery(g_pGameState->m_pCurrentParty->_BatteryLevel);
        
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
