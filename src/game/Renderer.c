#include "Renderer.h"
#include "maze.h"

#include <ace/managers/blit.h>
#define SOFFX 5

 CMazePos g_mazePos[18];
 CMazeDr g_mazeDr[4];

void clearViews(UBYTE* currentView, int startIndex, int endIndex) {
    for (int i = startIndex; i <= endIndex; i++) {
        currentView[i] = 0;
    }
}

void preFillFacing(CMazeDr* mazeDr) {
    static const int mazeFacingData[4][2] = {
        {1, 1},   // north
        {-1, 1},  // east
        {-1, -1}, // south
        {1, -1}   // west
    };

    for (int i = 0; i < 4; i++) {
        mazeDr[i].xs = mazeFacingData[i][0];
        mazeDr[i].ys = mazeFacingData[i][1];
    }
}

void preFillPosDataNoFacing(CMazePos* mazePos) {
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
        {0, 0}
    };

    for (int i = 0; i < 18; i++) {
        mazePos[i].xDelta = data[i][0];
        mazePos[i].yDelta = data[i][1];
    }
}

int drel(int d1, int d2) {
    int lookupTable[4][4] = {
        {1, 3, 0, 2},
        {2, 1, 3, 0},
        {0, 2, 1, 3},
        {3, 0, 2, 1}
    };

    return lookupTable[d2][d1];
}


void drawView(tGameState* pGameState, tBitMap* pCurrentBuffer)
{
    UBYTE currentView[18];
    memset(currentView, 0, sizeof(currentView));
    UBYTE px,py;
    px = pGameState->m_pCurrentParty->_PartyX;
    py = pGameState->m_pCurrentParty->_PartyY;
    UBYTE facing = pGameState->m_pCurrentParty->_PartyFacing;
    tMaze *pMaze = pGameState->m_pCurrentMaze;
    tWallset *pWallset = pGameState->m_pCurrentWallset;

    for (int i=0;i<17;++i)
    {
        BYTE x=0;
        BYTE y=0;
        if (facing % 2 == 0)
        {
            x=g_mazeDr[facing].xs * g_mazePos[i].xDelta;
            y=g_mazeDr[facing].ys * g_mazePos[i].yDelta;
        }
        else
        {
            x=g_mazeDr[facing].xs * g_mazePos[i].yDelta;
            y=g_mazeDr[facing].ys * g_mazePos[i].xDelta;
        }

        UWORD nazeOffset = (py+y)*32 + (px+x);
        UBYTE wmi = pMaze->_mazeData[nazeOffset];
        currentView[i] = wmi;
    } 
    
     if (currentView[14] > 0) {
        if (currentView[15] > 0) {
            if (currentView[16] > 0) {
                clearViews(currentView, 0, 13);
            }
        }
    }

    if (currentView[12] > 0) {
        if (currentView[13] > 0) {
            if (currentView[14] > 0) {
                clearViews(currentView, 0, 11);
            }
        }
    }

    if (currentView[8] > 0) {
        if (currentView[10] > 0) {
            if (currentView[11] > 0) {
                clearViews(currentView, 0, 7);
                currentView[9] = 0;
            }
        }
    }

    if (currentView[14] > 0) {
        currentView[11] = 0;
        currentView[6] = 0;
    }
    if (currentView[15] > 0) {
        currentView[0] = 0;
        currentView[1] = 0;
        currentView[7] = 0;
    }
    if (currentView[16] > 0) {
        currentView[3] = 0;
        currentView[4] = 0;
        currentView[9] = 0;
    }
    if (currentView[11] > 0) {
        currentView[6] = 0;
    }

    // draw the view
    for (UBYTE i = 0; i < 17; i++)
    {
        if (currentView[i])
        {
            for (UBYTE w=52; w<90;++w)
            {
                BYTE tx = -pWallset->_tileset[w]->_location[0];
                BYTE ty = pWallset->_tileset[w]->_location[1];
                if (tx == g_mazePos[i].xDelta) 
                {
                    if (ty == g_mazePos[i].yDelta)
                    {
                        blitCopyMask(pWallset->_gfx, 
                            pWallset->_tileset[w]->_x, pWallset->_tileset[w]->_y, 
                            pCurrentBuffer, 
                            pWallset->_tileset[w]->_screen[0]+SOFFX, pWallset->_tileset[w]->_screen[1]+SOFFX, 
                            pWallset->_tileset[w]->_width, pWallset->_tileset[w]->_height, 
                            (UWORD *)pWallset->_mask->Planes[0]);
                            continue;
                            
                    }
                }
            }    
        }    
    }
    
}
