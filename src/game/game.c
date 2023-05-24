#include "game.h"

#include <ace/managers/mouse.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/blit.h>
#include "gameState.h"
#include "screen.h"
#include "Renderer.h"

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
static void gameGsCreate(void)
{
    InitNewGame();
    pScreen = ScreenGetActive();

    g_pGameState->m_pCurrentMaze = mazeCreate(32, 32);
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            if (i == 0 || j == 0 || i == 31 || j == 31)
                mazeSetCell(g_pGameState->m_pCurrentMaze, i, j, 1);
            else
            {
                UBYTE isWall = rand() % 5 > 3 ? 1 : 0;
                mazeSetCell(g_pGameState->m_pCurrentMaze, i, j, isWall);
            }
        }
    }

    // mazeEventCreate(2,2,EVENT_SETWALLCOL,1,(UBYTE[]){1});
    mazeAppendEvent(g_pGameState->m_pCurrentMaze, mazeEventCreate(2, 2, EVENT_SETWALLCOL, 1, (UBYTE[]){1}));
    mazeAddString(g_pGameState->m_pCurrentMaze, "Welcome to My Life Sucks!", 25);
    mazeAddString(g_pGameState->m_pCurrentMaze, "Press any key to continue...", 28);
    mazeAddString(g_pGameState->m_pCurrentMaze, "Pretty Dusty Here...", 20);
    g_pGameState->m_pCurrentParty->_PartyX=1;
    g_pGameState->m_pCurrentParty->_PartyY=1;
    g_pGameState->m_pCurrentParty->_PartyFacing=0;

    systemUse();
    mazeSave(g_pGameState->m_pCurrentMaze, "data/MAZE.DAT");
    mazeDelete(g_pGameState->m_pCurrentMaze);
    g_pGameState->m_pCurrentMaze = mazeLoad("data/MAZE.DAT");

    pWallset = wallsetLoad("data/factory.wll");
    g_pGameState->m_pCurrentWallset = pWallset;

    
    pBackground = bitmapCreate(240, 180, 6, BMF_INTERLEAVED | BMF_CLEAR);
    blitRect(pBackground, 0, 0, 240, 180, 0);
    for (int i = 0; i < 52; i++)
    {
        blitCopyMask(pWallset->_gfx, pWallset->_tileset[i]->_x, pWallset->_tileset[i]->_y, pBackground, pWallset->_tileset[i]->_screen[0], pWallset->_tileset[i]->_screen[1], pWallset->_tileset[i]->_width, pWallset->_tileset[i]->_height, (UWORD *)pWallset->_mask->Planes[0]);
    }
    paletteLoad("data/playfield.plt", pScreen->_pFade->pPaletteRef, 64);
   

    tBitMap* pPlayfield = bitmapCreateFromFile("data/playfield.bm",0);
    blitCopy(pPlayfield,0,0,pScreen->_pBfr->pBack,0,0,320,256,MINTERM_COPY);
    //ScreenUpdate();
    blitCopy(pPlayfield,0,0,pScreen->_pBfr->pFront,0,0,320,256,MINTERM_COPY);
    bitmapDestroy(pPlayfield);
    // do an initial render to both front and back.
    
     blitCopy(pBackground, 0, 0, pScreen->_pBfr->pBack, SOFFX, SOFFX, 240, 180, MINTERM_COOKIE);
     drawView(g_pGameState, pScreen->_pBfr->pBack);
    blitCopy(pBackground, 0, 0, pScreen->_pBfr->pFront, SOFFX, SOFFX, 240, 180, MINTERM_COOKIE);
     drawView(g_pGameState, pScreen->_pBfr->pFront);

    systemUnuse();
    //viewUpdateCLUT(pScreen->_pView);
    ScreenUpdate();
    ScreenFadeFromBlack(NULL, 7, 0); // 7 is the speed of the fade
        
}

static void gameGsLoop(void)
{
    static int i = 0;
    // blitRect(pScreen->_pBfr->pBack, 0, 48, 240, 50, 0);
    blitCopy(pBackground, 0, 0, pScreen->_pBfr->pBack, SOFFX, SOFFX, 240, 180, MINTERM_COOKIE);
    // static int j = 52;
    // for (i = j; i < j+5; i++)
    // {
    //     // if (i == j)
    //     //     blitCopy(pWallset->_gfx, pWallset->_tileset[i]->_x, pWallset->_tileset[i]->_y, pScreen->_pBfr->pBack, pWallset->_tileset[i]->_screen[0], pWallset->_tileset[i]->_screen[1], pWallset->_tileset[i]->_width, pWallset->_tileset[i]->_height, MINTERM_COPY);
    //     if(i<90)
    //         blitCopyMask(pWallset->_gfx, pWallset->_tileset[i]->_x, pWallset->_tileset[i]->_y, pScreen->_pBfr->pBack, pWallset->_tileset[i]->_screen[0]+SOFFX, pWallset->_tileset[i]->_screen[1]+SOFFX, pWallset->_tileset[i]->_width, pWallset->_tileset[i]->_height, (UWORD *)pWallset->_mask->Planes[0]);
      
    // }
    // j+=5;
    // if (j > 90)
    //     j=53;
        
    //     //gameExit();
    //  blitWait();
    drawView(g_pGameState, pScreen->_pBfr->pBack);
   // g_pGameState->m_pCurrentParty->_PartyFacing++;
   // g_pGameState->m_pCurrentParty->_PartyFacing%=4;
    
    if (keyCheck(KEY_ESCAPE))
        gameExit();
    
    if (keyCheck(KEY_W))
    {
        UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing;
        modFace %=4;
        mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    }
    if (keyCheck(KEY_S))
    {
        UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing+2;
        modFace %=4;
        mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    }
    if (keyCheck(KEY_A))
    {
        UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing+3;
        modFace %=4;
        mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    }
    if (keyCheck(KEY_D))
    {
        UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing+1;
        modFace %=4;
        mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    }

    if (keyCheck(KEY_Q))
    {
        g_pGameState->m_pCurrentParty->_PartyFacing--;
        g_pGameState->m_pCurrentParty->_PartyFacing %= 4;
    }
    if (keyCheck(KEY_E))
    {
        g_pGameState->m_pCurrentParty->_PartyFacing++;
        g_pGameState->m_pCurrentParty->_PartyFacing %= 4;
    }
    blitWait();
    ScreenUpdate();

}

static void gameGsDestroy(void)
{
    ScreenFadeToBlack(NULL, 7, 0);
}
tState g_sStateGame = {
    .cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy};

tState g_statePaused = {
    .cbCreate = NULL, .cbLoop = NULL, .cbDestroy = NULL};
