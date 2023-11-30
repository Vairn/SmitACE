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
UBYTE g_ubRedrawRequire =0;
tBob g_pBob;
tBitMap *temp = NULL;

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
    mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    g_ubRedrawRequire = 2;
}

void MoveLeft()
{
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing + 3;
    modFace %= 4;
    mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    g_ubRedrawRequire = 2;
}

void MoveBackwards()
{
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing + 2;
    modFace %= 4;
    mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    g_ubRedrawRequire = 2;
}

void MoveForwards()
{
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing;
    modFace %= 4;
    mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
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
        //7gameExit();
    }
}


void cbGameOnPressed(Region *pRegion)
{
    UBYTE id = ((UBYTE)(ULONG)pRegion->context);
    switch (id)
    {
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
    case GAME_UI_GADGET_TURN_LEFT:
        TurnLeft();
        break;
    case GAME_UI_GADGET_TURN_RIGHT:
        TurnRight();
        break;
        default:
        break;
    }

}

void cbGameOnReleased(Region *pRegion)
{

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

    pBackground = bitmapCreate(240, 180, 6, BMF_INTERLEAVED | BMF_CLEAR);
    blitRect(pBackground, 0, 0, 240, 180, 0);

    paletteLoad("data/playfield.plt", pScreen->_pFade->pPaletteRef, 64);
    for(int p=0; p<256; p++)
    {
        ULONG* pPalRef = (ULONG *)pScreen->_pFade->pPaletteRef;
        pPalRef[p] = pWallset->_palette[3*p] << 16 | pWallset->_palette[(3*p)+1] << 8 | pWallset->_palette[(3*p+2)];
    }
    pMod = ptplayerModCreate("data/suspense.mod");
    ptplayerLoadMod(pMod, NULL, 0);
    ptplayerSetMasterVolume(64);
    ptplayerEnableMusic(1);
    tBitMap *pPlayfield = bitmapCreateFromFile("data/playfield.bm", 0);
    blitCopy(pPlayfield, 0, 0, pScreen->_pBfr->pBack, 0, 0, 320, 256, MINTERM_COPY);
    // ScreenUpdate();
    blitCopy(pPlayfield, 0, 0, pScreen->_pBfr->pFront, 0, 0, 320, 256, MINTERM_COPY);
    bitmapDestroy(pPlayfield);
    // do an initial render to both front and back.

    blitCopy(pBackground, 0, 0, pScreen->_pBfr->pBack, SOFFX, SOFFX, 240, 180, MINTERM_COOKIE);
    drawView(g_pGameState, pScreen->_pBfr->pBack);
    blitCopy(pBackground, 0, 0, pScreen->_pBfr->pFront, SOFFX, SOFFX, 240, 180, MINTERM_COOKIE);
    drawView(g_pGameState, pScreen->_pBfr->pFront);
    mouse_pointer_create("data/pointers.bm");
    systemUnuse();

    gameUIInit(cbGameOnHovered, cbGameOnUnhovered, cbGameOnPressed, cbGameOnReleased);

    // viewUpdateCLUT(pScreen->_pView);
    ScreenUpdate();
    ScreenFadeFromBlack(NULL, 7, fadeInComplete); // 7 is the speed of the fade
}

static void gameGsLoop(void)
{
    static mouse_pointer_t current_pointer_gfx = MOUSE_POINTER;
    mouse_pointer_update();

    // Added to Test the mouse pointer code.
    // And sprite bitmap switching for AGA - VAIRN.
    gameUIUpdate();
    if(mouseUse(MOUSE_PORT_1, MOUSE_LMB))
    {
        current_pointer_gfx +=1;
        if(current_pointer_gfx == MOUSE_MAX_COUNT)
        {
            current_pointer_gfx = MOUSE_TEST;
        }
        //spriteSetOddColourPaletteBank(current_pointer_gfx);
        mouse_pointer_switch(current_pointer_gfx);
    }
    if (g_ubGameActive)
    {
        
        if (g_ubRedrawRequire)
        {
           drawView(g_pGameState, pScreen->_pBfr->pBack);
           g_ubRedrawRequire --;
        }
        
        if (keyCheck(KEY_ESCAPE))
            gameExit();

        if (keyCheck(KEY_W))
        {
            MoveForwards();
        }
        if (keyCheck(KEY_S))
        {
            MoveBackwards();
        }
        if (keyCheck(KEY_A))
        {
            MoveLeft();
        }
        if (keyCheck(KEY_D))
        {
            MoveRight();
        }

        if (keyCheck(KEY_Q))
        {
            TurnLeft();
        }
        if (keyCheck(KEY_E))
        {
            TurnRight();
        }
        
    }

    ScreenUpdate();
}


static void gameGsDestroy(void)
{
    gameUIDestroy();
    ScreenFadeToBlack(NULL, 7, 0);
    bobManagerDestroy();
}
tState g_sStateGame = {
    .cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy};

tState g_statePaused = {
    .cbCreate = NULL, .cbLoop = NULL, .cbDestroy = NULL};
