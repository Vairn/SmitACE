#include "Renderer.h"
#include "maze.h"

#include <ace/managers/blit.h>
#define SOFFX 5

CMazePos g_mazePos[18];
CMazeDr g_mazeDr[4];
tBitMap *g_pMazeBitmap = NULL;

void clearViews(UBYTE *currentView, int startIndex, int endIndex)
{
    for (int i = startIndex; i <= endIndex; i++)
    {
        currentView[i] = 0;
    }
}

void preFillFacing(CMazeDr *mazeDr)
{
    static const int mazeFacingData[4][2] = {
        {1, 1},   // north
        {-1, 1},  // east
        {-1, -1}, // south
        {1, -1}   // west
    };

    for (int i = 0; i < 4; i++)
    {
        mazeDr[i].xs = mazeFacingData[i][0];
        mazeDr[i].ys = mazeFacingData[i][1];
    }
}

void preFillPosDataNoFacing(CMazePos *mazePos)
{
    int data[18][2] = {
        {-3, -3},
        {-2, -3},
        {-1, -3},
        {3, -3},
        {2, -3},
        {1, -3},
        {0, -3},
        {-2, -2},
        {-1, -2},
        {2, -2},
        {1, -2},
        {0, -2},
        {-1, -1},
        {1, -1},
        {0, -1},
        {-1, 0},
        {1, 0},
        {0, 0}};

    for (int i = 0; i < 18; i++)
    {
        mazePos[i].xDelta = data[i][0];
        mazePos[i].yDelta = data[i][1];
    }
}

int drel(int d1, int d2)
{
    int lookupTable[4][4] = {
        {1, 3, 0, 2},
        {2, 1, 3, 0},
        {0, 2, 1, 3},
        {3, 0, 2, 1}};

    return lookupTable[d2][d1];
}

void cleanUpView(UBYTE *pCurrentView)
{

    if (pCurrentView[14])
    {
        pCurrentView[11] = 0;
        pCurrentView[6] = 0;
    }
    if (pCurrentView[13])
    {
        pCurrentView[9] = 0;
        pCurrentView[4] = 0;
        pCurrentView[3] = 0;
    }
    if (pCurrentView[12])
    {
        pCurrentView[7] = 0;
        pCurrentView[1] = 0;
        pCurrentView[0] = 0;
    }

    if (pCurrentView[11])
    {
        pCurrentView[6] = 0;
    }
    if (pCurrentView[10])
    {
        pCurrentView[9] = 0;
        pCurrentView[3] = 0;
    }
    if (pCurrentView[9])
    {

        pCurrentView[4] = 0;
        pCurrentView[3] = 0;
    }

    if (pCurrentView[8])
    {
        pCurrentView[7] = 0;
        // pCurrentView[2] =0;
        // pCurrentView[1] =0;
        pCurrentView[0] = 0;
    }

    if (pCurrentView[7])
    {
        pCurrentView[1] = 0;
        pCurrentView[0] = 0;
    }
}
void drawView(tGameState *pGameState, tBitMap *pCurrentBuffer)
{

    static UBYTE currentView[18];
    memset(currentView, 0, sizeof(currentView));
    UBYTE px, py;
   // static tWallGfx *render[100];
   // memset(render, 0, sizeof(render));
    blitRect(pCurrentBuffer, SOFFX, SOFFX, 240, 180, 0);
    px = pGameState->m_pCurrentParty->_PartyX;
    py = pGameState->m_pCurrentParty->_PartyY;
    UBYTE facing = pGameState->m_pCurrentParty->_PartyFacing;
    tMaze *pMaze = pGameState->m_pCurrentMaze;
    tWallset *pWallset = pGameState->m_pCurrentWallset;
    if (g_pMazeBitmap == NULL)
    {
        g_pMazeBitmap = bitmapCreate(pMaze->_width * 5, pMaze->_height * 5, 3, BMF_CLEAR);
        for (int y = 0; y < pMaze->_height; y++)
        {
            for (int x = 0; x < pMaze->_width; x++)
            {
                UWORD mazeOffset = y * pMaze->_width + x;
                blitRect(g_pMazeBitmap, x * 5, y * 5, 5, 5, pMaze->_mazeData[mazeOffset]);
            }
        }
    }
    for (int i = 0; i < 17; ++i)
    {
        BYTE x = 0;
        BYTE y = 0;
        if (facing % 2 == 0)
        {
            x = g_mazeDr[facing].xs * g_mazePos[i].xDelta;
            y = g_mazeDr[facing].ys * g_mazePos[i].yDelta;
        }
        else
        {
            x = g_mazeDr[facing].xs * g_mazePos[i].yDelta;
            y = g_mazeDr[facing].ys * g_mazePos[i].xDelta;
        }
        if ((px + x) < 0 || (py + y) < 0)
        {
            continue;
        }
        if ((px + x) > pMaze->_width || (py + y) > pMaze->_height)
        {
            continue;
        }

        UWORD mazeOffset = (py + y) * pMaze->_width + (px + x);
        UBYTE wmi = pMaze->_mazeData[mazeOffset];
        currentView[i] = wmi;
    }
    WORD srcMapX = (px - 6) * 5;
    WORD srcMapY = 1+(py - 3) * 5;
    WORD dstMapX = 182;
    WORD dstMapY = 194;
    WORD width = 60;
    WORD height = 32;
    if(srcMapX < 0)
    {
        width = 60 + srcMapX;
        srcMapX = 0;
        dstMapX = 182 - (px - 6) * 5;
        
    }
    if (srcMapY < 0)
    {
        height = 32 + srcMapY;
        srcMapY = 0;
        dstMapY = 194 - (py - 3) * 5;
        
    }
    if (srcMapX > 5 * pMaze->_width)
    {
        width = width - (srcMapX - 5 * pMaze->_width);
        srcMapX = 5 * pMaze->_width;
        
        
    }
    if (srcMapY > 5 * pMaze->_height)
    {
        height = height - (srcMapY - 5 * pMaze->_height);
        srcMapY = 5 * pMaze->_height;
    }
    //blitCopy(g_pMazeBitmap, (MAX(0,), MAX(0,1+((py - 3) * 5)), pCurrentBuffer, 182, 194, 60, 32, MINTERM_COOKIE);
    blitCopy(g_pMazeBitmap, srcMapX, srcMapY, pCurrentBuffer, dstMapX, dstMapY, width, height, MINTERM_COOKIE);
    
    // blitRect(pCurrentBuffer, 250, 20, 7*8, 8*8,0);
    // for (int i=0; i<17; i++)
    // {
    //     //if (currentView[i] == 1)
    //     //{
    //         blitRect(pCurrentBuffer,
    //         278+(g_mazePos[i].xDelta*8),
    //         46+(g_mazePos[i].yDelta*8), 8, 8, currentView[i]);
    //     //}
    // }
    cleanUpView(currentView);
    //  for (int i=0; i<17; i++)
    //     {
    //         //if (currentView[i] == 1)
    //         //{
    //             blitRect(pCurrentBuffer,
    //             278+(g_mazePos[i].xDelta*8),
    //             86+(g_mazePos[i].yDelta*8), 8, 8, currentView[i]);
    //         //}
    //     }
    // draw the view
    // 5int j = 0;
    UBYTE j = 0;
    for (UBYTE i = 0; i < 18; i++)
    {
        BYTE tx = g_mazePos[i].xDelta;
        BYTE ty = g_mazePos[i].yDelta;
        // for (UBYTE w = 52; w < 90; ++w)
        for (UBYTE w = 0; w < pWallset->_tilesetCount; ++w)
        {
            if (tx == pWallset->_tileset[w]->_location[0] && ty == pWallset->_tileset[w]->_location[1])
            {
                if (currentView[i] == pWallset->_tileset[w]->_type )
                 {
                    blitCopyMask(pWallset->_gfx[pWallset->_tileset[w]->_setIndex],
                     pWallset->_tileset[w]->_x, pWallset->_tileset[w]->_y,
                     pCurrentBuffer,
                     pWallset->_tileset[w]->_screen[0] + SOFFX, pWallset->_tileset[w]->_screen[1] + SOFFX,
                     pWallset->_tileset[w]->_width, pWallset->_tileset[w]->_height,
                     (UBYTE *)pWallset->_mask[pWallset->_tileset[w]->_setIndex]->Planes[0]);
                   }
                   // render[j++] = pWallset->_tileset[w];
                 //break;
            }
        }
    }
}
