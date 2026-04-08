#include "Renderer.h"
#include "maze.h"
#include "wallset.h"
#include "GameState.h"
#include "script.h"
#include "monster.h"
#include "ground_item.h"
#include "wallbutton.h"
#include "doorbutton.h"
#include "wall_interactable_placeholder.h"

#include <ace/managers/blit.h>
#include <string.h>
#define SOFFX 5

static tWallGfx *findFloorGfxForSlot(tWallset *pWallset, BYTE tx, BYTE ty)
{
    if (!pWallset)
        return NULL;
    for (UWORD w = 0; w < pWallset->_tilesetCount; ++w) {
        if (tx == pWallset->_tileset[w]->_location[0] && ty == pWallset->_tileset[w]->_location[1]
            && pWallset->_tileset[w]->_type == 0)
            return pWallset->_tileset[w];
    }
    return NULL;
}

static void drawMonsterPlaceholder(tBitMap *pBuf, tWallGfx *floor, tMonster *mon)
{
    WORD fw = (WORD)floor->_width;
    WORD fh = (WORD)floor->_height;
    WORD mw = (fw * 40) / 100;
    WORD mh = (fh * 55) / 100;
    if (mw < 4)
        mw = 4;
    if (mh < 4)
        mh = 4;
    WORD sx = floor->_screen[0] + SOFFX + (fw - mw) / 2;
    WORD sy = floor->_screen[1] + SOFFX + fh - mh - 2;
    if (sx < 0 || sy < 0)
        return;
    UBYTE color = (UBYTE)(33 + (mon->_monsterType % 7));
    blitRect(pBuf, sx, sy, (UWORD)mw, (UWORD)mh, color);
}

static void drawGroundItemPlaceholder(tBitMap *pBuf, tWallGfx *floor, UBYTE itemIdx)
{
    WORD fw = (WORD)floor->_width;
    WORD fh = (WORD)floor->_height;
    WORD iw = (fw * 18) / 100;
    WORD ih = (fh * 22) / 100;
    if (iw < 3)
        iw = 3;
    if (ih < 3)
        ih = 3;
    WORD sx = floor->_screen[0] + SOFFX + (fw / 4) - (iw / 2);
    WORD sy = floor->_screen[1] + SOFFX + fh - ih - 4;
    if (sx < 0 || sy < 0)
        return;
    UBYTE color = (UBYTE)(40 + (itemIdx % 8));
    blitRect(pBuf, sx, sy, (UWORD)iw, (UWORD)ih, color);
}

// Define cookie mode for better performance
#define BLIT_COOKIE_MODE (0xCA)

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

UBYTE renderDoor(tWallset *pWallset, tMaze *pMaze, tBitMap *pCurrentBuffer,
                 BYTE tx, BYTE ty, UBYTE px, UBYTE py, UBYTE x, UBYTE y)
{
    // Find the door graphics (MAZE_DOOR) first
    UBYTE doorW;
    for (doorW = 0; doorW < pWallset->_tilesetCount; ++doorW) {
        if (tx == pWallset->_tileset[doorW]->_location[0] &&
            ty == pWallset->_tileset[doorW]->_location[1] &&
            pWallset->_tileset[doorW]->_type == MAZE_DOOR) {
            break;
        }
    }
    
    // If no door found at this location, exit
    if (doorW >= pWallset->_tilesetCount) {
        return 0;
    }

    // Check for active door animation
    tDoorAnim *anim = doorAnimFind(pMaze, px + x, py + y);
    if (anim) {
        UBYTE doorHeight = pWallset->_tileset[doorW]->_height;
        UBYTE yOffset = 0;

        // Get height ratio based on frame number
        UBYTE heightRatio;
        if (anim->state == DOOR_ANIM_OPENING) {
            // Opening: go from 16 to 0
            heightRatio = 16 - ((anim->frame * 16) / DOOR_ANIM_FRAMES);
        } else {
            // Closing: go from 0 to 16
            heightRatio = (anim->frame * 16) / DOOR_ANIM_FRAMES;
        }
        
        if (heightRatio > 0) {  // Skip if door is fully open
            doorHeight = (doorHeight * heightRatio) / 16;
            
            // Draw the door
            blitUnsafeCopyMask(pWallset->_gfx[pWallset->_tileset[doorW]->_setIndex],
                             pWallset->_tileset[doorW]->_x, pWallset->_tileset[doorW]->_y + yOffset,
                             pCurrentBuffer,
                             pWallset->_tileset[doorW]->_screen[0] + SOFFX, pWallset->_tileset[doorW]->_screen[1] + SOFFX + yOffset,
                             pWallset->_tileset[doorW]->_width, doorHeight,
                             (UBYTE *)pWallset->_mask[pWallset->_tileset[doorW]->_setIndex]->Planes[0]);
            return 1; // Animation frame was drawn
        }
    }
    
    return 0; // No animation frame was drawn
}

UBYTE mazeWallSideFacingParty(UBYTE px, UBYTE py, UBYTE cx, UBYTE cy)
{
    int dx = (int)cx - (int)px;
    int dy = (int)cy - (int)py;
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    if (dx == 0 && dy == 0)
        return 0;
    if (adx > ady)
        return (UBYTE)(dx > 0 ? 3 : 1);
    if (ady > adx)
        return (UBYTE)(dy > 0 ? 0 : 2);
    if (dx > 0 && dy >= 0)
        return 3;
    if (dx < 0 && dy <= 0)
        return 1;
    if (dy > 0)
        return 0;
    return 2;
}

static void rendererFillSlotLayout(tGameState *pGameState,
    UBYTE *slotValid, BYTE *slotTx, BYTE *slotTy, UBYTE *slotCx, UBYTE *slotCy)
{
    UBYTE px = pGameState->m_pCurrentParty->_PartyX;
    UBYTE py = pGameState->m_pCurrentParty->_PartyY;
    UBYTE facing = pGameState->m_pCurrentParty->_PartyFacing;
    tMaze *pMaze = pGameState->m_pCurrentMaze;
    memset(slotValid, 0, 18);
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
            continue;
        if ((px + x) > pMaze->_width || (py + y) > pMaze->_height)
            continue;
        slotValid[i] = 1;
        slotCx[i] = (UBYTE)(px + x);
        slotCy[i] = (UBYTE)(py + y);
        slotTx[i] = g_mazePos[i].xDelta;
        slotTy[i] = g_mazePos[i].yDelta;
    }
}

void viewportPickAtScreen(tGameState *pGameState, UWORD screenX, UWORD screenY, tViewportPick *pick)
{
    pick->kind = VIEWPORT_PICK_NONE;
    pick->cellX = 0;
    pick->cellY = 0;
    pick->wallSide = 0;
    if (!pGameState || !pGameState->m_pCurrentMaze || !pGameState->m_pCurrentWallset)
        return;
    if (screenX < VIEWPORT_UI_REGION_X || screenY < VIEWPORT_UI_REGION_Y)
        return;
    if (screenX >= VIEWPORT_UI_REGION_X + VIEWPORT_UI_REGION_W
        || screenY >= VIEWPORT_UI_REGION_Y + VIEWPORT_UI_REGION_H)
        return;
    WORD bx = (WORD)(screenX - VIEWPORT_UI_REGION_X + SOFFX);
    WORD by = (WORD)(screenY - VIEWPORT_UI_REGION_Y + SOFFX);

    UBYTE slotValid[18];
    BYTE slotTx[18], slotTy[18];
    UBYTE slotCx[18], slotCy[18];
    rendererFillSlotLayout(pGameState, slotValid, slotTx, slotTy, slotCx, slotCy);

    UBYTE px = pGameState->m_pCurrentParty->_PartyX;
    UBYTE py = pGameState->m_pCurrentParty->_PartyY;
    UBYTE facing = pGameState->m_pCurrentParty->_PartyFacing;
    tMaze *pMaze = pGameState->m_pCurrentMaze;
    tWallset *pWallset = pGameState->m_pCurrentWallset;

    UBYTE doorX = px;
    UBYTE doorY = py;
    switch (facing)
    {
    case 0:
        doorY--;
        break;
    case 1:
        doorX++;
        break;
    case 2:
        doorY++;
        break;
    case 3:
        doorX--;
        break;
    default:
        break;
    }
    UBYTE doorCell = mazeGetCell(pMaze, doorX, doorY);
    if (doorCell == MAZE_DOOR || doorCell == MAZE_DOOR_OPEN || doorCell == MAZE_DOOR_LOCKED)
    {
        for (UBYTE i = 0; i < 18; i++)
        {
            if (!slotValid[i] || slotCx[i] != doorX || slotCy[i] != doorY)
                continue;
            tWallGfx *dg = NULL;
            for (UWORD w = 0; w < pWallset->_tilesetCount; w++)
            {
                if (slotTx[i] == pWallset->_tileset[w]->_location[0]
                    && slotTy[i] == pWallset->_tileset[w]->_location[1]
                    && pWallset->_tileset[w]->_type == MAZE_DOOR)
                {
                    dg = pWallset->_tileset[w];
                    break;
                }
            }
            if (dg)
            {
                WORD x0 = dg->_screen[0] + SOFFX;
                WORD y0 = dg->_screen[1] + SOFFX;
                if (bx >= x0 && by >= y0 && bx < x0 + (WORD)dg->_width && by < y0 + (WORD)dg->_height)
                {
                    pick->kind = VIEWPORT_PICK_DOOR_AHEAD;
                    pick->cellX = doorX;
                    pick->cellY = doorY;
                    return;
                }
            }
        }
    }

    for (int i = 17; i >= 0; i--)
    {
        if (!slotValid[i])
            continue;
        UBYTE ws = mazeWallSideFacingParty(px, py, slotCx[i], slotCy[i]);
        for (tWallButton *wb = pGameState->m_wallButtons._buttons; wb; wb = wb->_next)
        {
            if (wb->_x != slotCx[i] || wb->_y != slotCy[i] || wb->_wallSide != ws)
                continue;
            tWallGfx *g = wallButtonGfxForSlot(wb, pWallset, slotTx[i], slotTy[i]);
            UBYTE hit = 0;
            if (g)
            {
                WORD x0 = g->_screen[0] + SOFFX;
                WORD y0 = g->_screen[1] + SOFFX;
                hit = (UBYTE)(bx >= x0 && by >= y0 && bx < x0 + (WORD)g->_width && by < y0 + (WORD)g->_height);
            }
            else
                hit = wallInteractablePlaceholderHit(pWallset, slotTx[i], slotTy[i], bx, by);
            if (hit)
            {
                pick->kind = VIEWPORT_PICK_WALL_BUTTON;
                pick->cellX = wb->_x;
                pick->cellY = wb->_y;
                pick->wallSide = wb->_wallSide;
                return;
            }
        }
        for (tDoorButton *db = pGameState->m_doorButtons._buttons; db; db = db->_next)
        {
            if (db->_x != slotCx[i] || db->_y != slotCy[i] || db->_wallSide != ws)
                continue;
            tWallGfx *g = doorButtonGfxForSlot(db, pWallset, slotTx[i], slotTy[i]);
            UBYTE hit = 0;
            if (g)
            {
                WORD x0 = g->_screen[0] + SOFFX;
                WORD y0 = g->_screen[1] + SOFFX;
                hit = (UBYTE)(bx >= x0 && by >= y0 && bx < x0 + (WORD)g->_width && by < y0 + (WORD)g->_height);
            }
            else
                hit = wallInteractablePlaceholderHit(pWallset, slotTx[i], slotTy[i], bx, by);
            if (hit)
            {
                pick->kind = VIEWPORT_PICK_DOOR_BUTTON;
                pick->cellX = db->_x;
                pick->cellY = db->_y;
                pick->wallSide = db->_wallSide;
                return;
            }
        }
    }
}

void drawView(tGameState *pGameState, tBitMap *pCurrentBuffer)
{
    static UBYTE currentView[18];
    memset(currentView, 0, sizeof(currentView));
    UBYTE px, py;
    blitRect(pCurrentBuffer, SOFFX, SOFFX, 240, 180, 0);
    
    // Fill bitplane 5 in the viewport area so walls use colors 32-63 instead of 0-31
    // This avoids conflict with UI colors (0-31)
    // Using color 32 (binary 100000) sets only bitplane 5
    blitRect(pCurrentBuffer, SOFFX, SOFFX, 240, 180, 32);
    
    px = pGameState->m_pCurrentParty->_PartyX;
    py = pGameState->m_pCurrentParty->_PartyY;
    UBYTE facing = pGameState->m_pCurrentParty->_PartyFacing;
    tMaze *pMaze = pGameState->m_pCurrentMaze;
    tWallset *pWallset = pGameState->m_pCurrentWallset;
    /* Lazy-build: minimap only shown after map opened (bitmap created in drawFullScreenMap) */
    if (g_pMazeBitmap != NULL)
    {
    WORD srcMapX = (px - 6) * 5;
    WORD srcMapY = 1 + (py - 3) * 5;
    WORD dstMapX = 182;
    WORD dstMapY = 194;
    WORD width = 60;
    WORD height = 32;
    if (srcMapX < 0)
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
    }

    cleanUpView(currentView);

    UBYTE slotValid[18];
    BYTE slotTx[18], slotTy[18];
    UBYTE slotCx[18], slotCy[18];
    UBYTE slotWmi[18];
    memset(slotValid, 0, sizeof(slotValid));

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

        slotValid[i] = 1;
        slotCx[i] = (UBYTE)(px + x);
        slotCy[i] = (UBYTE)(py + y);

        UWORD mazeOffset = (py + y) * pMaze->_width + (px + x);
        UBYTE wmi = pMaze->_mazeData[mazeOffset];

        BYTE tx = g_mazePos[i].xDelta;
        BYTE ty = g_mazePos[i].yDelta;
        slotTx[i] = tx;
        slotTy[i] = ty;

        if (wmi == MAZE_DOOR || wmi == MAZE_DOOR_OPEN || wmi == MAZE_DOOR_LOCKED)
        {
            if (renderDoor(pWallset, pMaze, pCurrentBuffer, tx, ty, px, py, x, y))
            {
                if (wmi == MAZE_DOOR || wmi == MAZE_DOOR_OPEN || wmi == MAZE_DOOR_LOCKED ){
                    wmi = MAZE_DOOR_OPEN;
                }
            }
        }
        
        // Check if this cell is a charger (maze data = 5 = MAZE_EVENT_TRIGGER)
        // and render indicator on floor if it's a battery charger event
        if (wmi == MAZE_EVENT_TRIGGER)  // MAZE_EVENT_TRIGGER is 5
        {
            tMazeEvent* pEvent = mazeFindEventAtPosition(pMaze, px + x, py + y);
            if (pEvent && pEvent->_eventType == EVENT_BATTERY_CHARGER)
            {
                // Draw charger indicator on the floor - find floor rendering position
                for (UBYTE w = 0; w < pWallset->_tilesetCount; ++w)
                {
                    if (tx == pWallset->_tileset[w]->_location[0] && ty == pWallset->_tileset[w]->_location[1] && pWallset->_tileset[w]->_type == 0)
                    {
                        // This is a floor tile - draw charger indicator in center
                        WORD centerX = pWallset->_tileset[w]->_screen[0] + SOFFX + (pWallset->_tileset[w]->_width / 2) - 4;
                        WORD centerY = pWallset->_tileset[w]->_screen[1] + SOFFX + pWallset->_tileset[w]->_height - 12;
                        blitRect(pCurrentBuffer, centerX, centerY, 8, 8, 34); // Green square indicator (32+2, using wallset color range)
                        break;
                    }
                }
            }
        }

        for (UBYTE w = 0; w < pWallset->_tilesetCount; ++w)
        {
            if (tx == pWallset->_tileset[w]->_location[0] && ty == pWallset->_tileset[w]->_location[1])
            {
                if ((wmi == pWallset->_tileset[w]->_type) || (wmi == MAZE_DOOR_OPEN && pWallset->_tileset[w]->_type == 0) || (wmi == MAZE_DOOR && pWallset->_tileset[w]->_type == 0) || (wmi == MAZE_DOOR_LOCKED && pWallset->_tileset[w]->_type == 0) || (wmi == MAZE_EVENT_TRIGGER && pWallset->_tileset[w]->_type == 0))
                {
                    UBYTE doorWidth = pWallset->_tileset[w]->_width;
                    UBYTE doorHeight = pWallset->_tileset[w]->_height;

                    blitUnsafeCopyMask(pWallset->_gfx[pWallset->_tileset[w]->_setIndex],
                                       pWallset->_tileset[w]->_x, pWallset->_tileset[w]->_y,
                                       pCurrentBuffer,
                                       pWallset->_tileset[w]->_screen[0] + SOFFX, pWallset->_tileset[w]->_screen[1] + SOFFX,
                                       doorWidth, doorHeight,
                                       (UBYTE *)pWallset->_mask[pWallset->_tileset[w]->_setIndex]->Planes[0]);
                }
            }
        }

        {
            UBYTE wsBtn = mazeWallSideFacingParty(px, py, slotCx[i], slotCy[i]);
            for (tWallButton *wb = pGameState->m_wallButtons._buttons; wb; wb = wb->_next)
            {
                if (wb->_x == slotCx[i] && wb->_y == slotCy[i] && wb->_wallSide == wsBtn)
                    wallButtonRender(wb, pWallset, pCurrentBuffer, slotTx[i], slotTy[i]);
            }
            for (tDoorButton *db = pGameState->m_doorButtons._buttons; db; db = db->_next)
            {
                if (db->_x == slotCx[i] && db->_y == slotCy[i] && db->_wallSide == wsBtn)
                    doorButtonRender(db, pWallset, pCurrentBuffer, slotTx[i], slotTy[i]);
            }
        }

        slotWmi[i] = wmi;
    }

    for (UBYTE i = 0; i < 18; i++)
    {
        if (!slotValid[i])
            continue;
        UBYTE wmi = slotWmi[i];
        if (wmi != MAZE_FLOOR && wmi != MAZE_DOOR_OPEN && wmi != MAZE_EVENT_TRIGGER)
            continue;

        tWallGfx *floorGfx = findFloorGfxForSlot(pWallset, slotTx[i], slotTy[i]);
        if (!floorGfx)
            continue;

        UBYTE cx = slotCx[i];
        UBYTE cy = slotCy[i];

        tMonsterList *ml = pGameState->m_pMonsterList;
        if (ml)
        {
            for (UBYTE mi = 0; mi < ml->_numMonsters; mi++)
            {
                tMonster *mon = ml->_monsters[mi];
                if (!mon || mon->_state == MONSTER_STATE_DEAD)
                    continue;
                if (mon->_partyPosX == cx && mon->_partyPosY == cy)
                    drawMonsterPlaceholder(pCurrentBuffer, floorGfx, mon);
            }
        }

        tGroundItemList *gl = &pGameState->m_groundItems;
        for (UBYTE gi = 0; gi < gl->count; gi++)
        {
            if (gl->items[gi].x == cx && gl->items[gi].y == cy)
                drawGroundItemPlaceholder(pCurrentBuffer, floorGfx, gl->items[gi].itemIdx);
        }
    }
}

void drawFullScreenMap(tGameState *pGameState, tBitMap *pCurrentBuffer)
{
    tMaze *pMaze = pGameState->m_pCurrentMaze;
    if (!pMaze) return;

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

    blitRect(pCurrentBuffer, SOFFX, SOFFX, 240, 180, 0);
    blitUnsafeCopy(g_pMazeBitmap, 0, 0, pCurrentBuffer, SOFFX + 40, SOFFX + 10, 160, 160, BLIT_COOKIE_MODE);

    UBYTE px = pGameState->m_pCurrentParty->_PartyX;
    UBYTE py = pGameState->m_pCurrentParty->_PartyY;
    UWORD markerX = SOFFX + 40 + (UWORD)px * 5;
    UWORD markerY = SOFFX + 10 + (UWORD)py * 5;
    blitRect(pCurrentBuffer, markerX, markerY, 5, 5, 9);
}
