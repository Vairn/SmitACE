#include "Renderer.h"
#include "maze.h"
#include "wallset.h"
#include "GameState.h"

#include <ace/managers/blit.h>
#define SOFFX 5

// Define cookie mode for better performance
#define BLIT_COOKIE_MODE (0xCA)

CMazePos g_mazePos[18];
CMazeDr g_mazeDr[4];
tBitMap *g_pMazeBitmap = NULL;

// Pre-calculated view data
typedef struct {
    BYTE x, y;
    UBYTE wmi;
    tWallGfx* wall;
} tViewData;

static tViewData viewData[18];  // Pre-allocated for max 18 walls

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
    blitUnsafeCopy(g_pMazeBitmap, srcMapX, srcMapY, pCurrentBuffer, dstMapX, dstMapY, width, height, BLIT_COOKIE_MODE);
    
    cleanUpView(currentView);
    
    for (UBYTE i = 0; i < 18; i++)
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
      
        BYTE tx = g_mazePos[i].xDelta;
        BYTE ty = g_mazePos[i].yDelta;
    
        for (UBYTE w = 0; w < pWallset->_tilesetCount; ++w)
        {
            if (tx == pWallset->_tileset[w]->_location[0] && ty == pWallset->_tileset[w]->_location[1])
            {
                if ((wmi == pWallset->_tileset[w]->_type) 
                || (wmi == MAZE_DOOR_OPEN && pWallset->_tileset[w]->_type == 0)
                || (wmi == MAZE_DOOR && pWallset->_tileset[w]->_type == 0)
                || (wmi == MAZE_DOOR_LOCKED && pWallset->_tileset[w]->_type == 0))
                {
                    // Calculate door width based on animation state
                    UBYTE doorWidth = pWallset->_tileset[w]->_width;
                    UBYTE doorHeight = pWallset->_tileset[w]->_height;
                    UBYTE xOffset = 0;
                    
                    // Check for active door animation
                    tDoorAnim* anim = doorAnimFind(pMaze, tx, ty);
                    if (anim) {
                        switch (anim->state) {
                            case DOOR_ANIM_OPENING_1:
                                doorWidth = (doorWidth * 3) / 4;  // 75% of original width
                                xOffset = doorWidth / 4;  // Center the remaining door
                                break;
                            case DOOR_ANIM_OPENING_2:
                                doorWidth = doorWidth / 2;  // 50% of original width
                                xOffset = doorWidth / 2;  // Center the remaining door
                                break;
                            case DOOR_ANIM_OPENING_3:
                                doorWidth = doorWidth / 4;  // 25% of original width
                                xOffset = (doorWidth * 3) / 4;  // Center the remaining door
                                break;
                            case DOOR_ANIM_CLOSING_1:
                                doorWidth = doorWidth / 4;  // 25% of original width
                                xOffset = (doorWidth * 3) / 4;  // Center the remaining door
                                break;
                            case DOOR_ANIM_CLOSING_2:
                                doorWidth = doorWidth / 2;  // 50% of original width
                                xOffset = doorWidth / 2;  // Center the remaining door
                                break;
                            case DOOR_ANIM_CLOSING_3:
                                doorWidth = (doorWidth * 3) / 4;  // 75% of original width
                                xOffset = doorWidth / 4;  // Center the remaining door
                                break;
                        }
                    }
                    else if (wmi == MAZE_DOOR) {
                        doorWidth = pWallset->_tileset[w]->_width;  // Full width for closed door
                    }
                    
                    // Only draw if door has width
                    if (doorWidth > 0) {
                        blitUnsafeCopyMask(pWallset->_gfx[pWallset->_tileset[w]->_setIndex],
                         pWallset->_tileset[w]->_x + xOffset, pWallset->_tileset[w]->_y,
                         pCurrentBuffer,
                         pWallset->_tileset[w]->_screen[0] + SOFFX + xOffset, pWallset->_tileset[w]->_screen[1] + SOFFX,
                         doorWidth, doorHeight,
                         (UBYTE *)pWallset->_mask[pWallset->_tileset[w]->_setIndex]->Planes[0]);
                    }
                }
            }
        }
    }
}
