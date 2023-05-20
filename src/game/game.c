#include "game.h"

#include <ace/managers/mouse.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/blit.h>
#include "gameState.h"
#include "screen.h"

ULONG seed = 1;

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
    systemUse();

    pScreen = ScreenGetActive();

    g_pCurrentMaze = mazeCreate(32, 32);
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            if (i == 0 || j == 0 || i == 31 || j == 31)
                mazeSetCell(g_pCurrentMaze, i, j, 1);
        }
    }

    // mazeEventCreate(2,2,EVENT_SETWALLCOL,1,(UBYTE[]){1});
    mazeAppendEvent(g_pCurrentMaze, mazeEventCreate(2, 2, EVENT_SETWALLCOL, 1, (UBYTE[]){1}));
    mazeAddString(g_pCurrentMaze, "Welcome to My Life Sucks!", 25);
    mazeAddString(g_pCurrentMaze, "Press any key to continue...", 28);
    mazeAddString(g_pCurrentMaze, "Pretty Dusty Here...", 20);
    systemUse();
    mazeSave(g_pCurrentMaze, "data/MAZE.DAT");
    mazeDelete(g_pCurrentMaze);
    g_pCurrentMaze = mazeLoad("data/MAZE.DAT");

    pWallset = wallsetLoad("data/factory.wll");
    systemUnuse();
    ULONG *pPaletteAGA = (ULONG *)pScreen->_pView->pFirstVPort->pPalette;
    for (int p = 0; p < 64; p++)
    {
        pPaletteAGA[p] = pWallset->_palette[p * 3 + 0] << 16 | pWallset->_palette[p * 3 + 1] << 8 | pWallset->_palette[p * 3 + 2] << 0;
        // pScreen->_pView->pFirstVPort->pPalette[(p*4)+0] = pWallset->_palette[p*3+0];
        // pScreen->_pView->pFirstVPort->pPalette[(p*4)+1] = pWallset->_palette[p*3+1];
        // pScreen->_pView->pFirstVPort->pPalette[(p*4)+2] = pWallset->_palette[p*3+2];
        // logWrite("Palette %d: %d %d %d\n", p, pWallset->_palette[p*3+0], pWallset->_palette[p*3+1], pWallset->_palette[p*3+2]);
        // logWrite("Palette %d: %d %d %d\n", p,
        //            pScreen->_pView->pFirstVPort->pPalette[p*4+0],
        //            pScreen->_pView->pFirstVPort->pPalette[p*4+1],
        //            pScreen->_pView->pFirstVPort->pPalette[p*4+2]);
    }
    viewUpdateCLUT(pScreen->_pView);

    pBackground = bitmapCreate(240, 196, 6, BMF_INTERLEAVED | BMF_CLEAR);
    blitRect(pBackground, 0, 0, 240, 196, 0);
    for (int i = 0; i < 52; i++)
    {
        blitCopyMask(pWallset->_gfx, pWallset->_tileset[i]->_x, pWallset->_tileset[i]->_y, pBackground, pWallset->_tileset[i]->_screen[0], pWallset->_tileset[i]->_screen[1], pWallset->_tileset[i]->_width, pWallset->_tileset[i]->_height, (UWORD *)pWallset->_mask->Planes[0]);
        bitmapDestroy(pWallset->_tileset[i]->_mask);
        bitmapDestroy(pWallset->_tileset[i]->_gfx);
    }

    ScreenUpdate();
}

static void gameGsLoop(void)
{
    static int i = 0;
    // blitRect(pScreen->_pBfr->pBack, 0, 48, 240, 50, 0);
    blitCopy(pBackground, 0, 0, pScreen->_pBfr->pBack, 0, 0, 240, 196, MINTERM_COPY);
    static int j = 52;
    for (i = j; i < j+5; i++)
    {
        // if (i == j)
        //     blitCopy(pWallset->_gfx, pWallset->_tileset[i]->_x, pWallset->_tileset[i]->_y, pScreen->_pBfr->pBack, pWallset->_tileset[i]->_screen[0], pWallset->_tileset[i]->_screen[1], pWallset->_tileset[i]->_width, pWallset->_tileset[i]->_height, MINTERM_COPY);
        if(i<90)
            blitCopyMask(pWallset->_gfx, pWallset->_tileset[i]->_x, pWallset->_tileset[i]->_y, pScreen->_pBfr->pBack, pWallset->_tileset[i]->_screen[0], pWallset->_tileset[i]->_screen[1], pWallset->_tileset[i]->_width, pWallset->_tileset[i]->_height, (UWORD *)pWallset->_mask->Planes[0]);
      
    }
    j+=5;
    if (j > 90)
        j = 52;
    // blitWait();
    ScreenUpdate();
}

static void gameGsDestroy(void)
{
}
tState g_sStateGame = {
    .cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy};

tState g_statePaused = {
    .cbCreate = NULL, .cbLoop = NULL, .cbDestroy = NULL};
