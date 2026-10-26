#include "Renderer.h"
#include "maze.h"
#include "wallset.h"
#include "GameState.h"
#include "script.h"
#include "monster.h"
#include "ground_item.h"
#include "item.h"
#include "wallbutton.h"
#include "doorbutton.h"
#include "wall_interactable_placeholder.h"

#include <ace/managers/blit.h>
#include <string.h>
#define SOFFX 5
#define VIEW_W 240
#define VIEW_H 180

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

static tWallGfx *findTypedGfxForSlot(tWallset *pWallset, BYTE tx, BYTE ty, UBYTE type)
{
    if (!pWallset)
        return NULL;
    for (UWORD w = 0; w < pWallset->_tilesetCount; ++w) {
        if (tx == pWallset->_tileset[w]->_location[0] && ty == pWallset->_tileset[w]->_location[1]
            && pWallset->_tileset[w]->_type == type)
            return pWallset->_tileset[w];
    }
    return NULL;
}

#define PROP_NONE 0
#define PROP_CHARGER 1
#define PROP_LIFT 2
#define PROP_PLATE 3
#define PROP_EXIT 4

/** Wallset colours live at screen indices 32+. */
#define PROP_COL_BLACK 32
#define PROP_COL_CYAN 40
#define PROP_COL_ORANGE 38
#define PROP_COL_GOLD 45
#define PROP_COL_PALE 47
#define PROP_COL_GRAY 36

static UBYTE floorPropAt(tGameState *pGameState, UBYTE x, UBYTE y)
{
    UBYTE kind = PROP_NONE;
    tMaze *pMaze;
    UBYTE i;
    if (!pGameState || !pGameState->m_pCurrentMaze)
        return PROP_NONE;
    pMaze = pGameState->m_pCurrentMaze;
    for (tMazeEvent *e = pMaze->_events; e; e = e->_next) {
        if (e->_x != x || e->_y != y)
            continue;
        if (e->_eventType == EVENT_CHANGELEVEL)
            return PROP_LIFT;
        if (e->_eventType == EVENT_WIN)
            kind = PROP_EXIT;
        else if (e->_eventType == EVENT_BATTERY_CHARGER && kind == PROP_NONE)
            kind = PROP_CHARGER;
    }
    if (kind != PROP_NONE)
        return kind;
    for (i = 0; i < pGameState->m_pressurePlates.count; i++) {
        if (pGameState->m_pressurePlates.plates[i].x == x
            && pGameState->m_pressurePlates.plates[i].y == y)
            return PROP_PLATE;
    }
    return PROP_NONE;
}

static void fillClipped(tBitMap *pBuf, WORD x, WORD y, WORD w, WORD h, UBYTE color)
{
    if (w <= 0 || h <= 0)
        return;
    if (x < SOFFX) {
        w = (WORD)(w - (SOFFX - x));
        x = SOFFX;
    }
    if (y < SOFFX) {
        h = (WORD)(h - (SOFFX - y));
        y = SOFFX;
    }
    if (x + w > SOFFX + VIEW_W)
        w = (WORD)(SOFFX + VIEW_W - x);
    if (y + h > SOFFX + VIEW_H)
        h = (WORD)(SOFFX + VIEW_H - y);
    if (w <= 0 || h <= 0)
        return;
    blitRect(pBuf, (UWORD)x, (UWORD)y, (UWORD)w, (UWORD)h, color);
}

static void drawShaft(tBitMap *pBuf, WORD x, WORD y, WORD w, WORD h, UBYTE outer, UBYTE inner)
{
    WORD m;
    if (w < 4 || h < 4)
        return;
    m = (w < 16 || h < 16) ? 1 : 2;
    fillClipped(pBuf, x, y, w, h, outer);
    fillClipped(pBuf, (WORD)(x + m), (WORD)(y + m), (WORD)(w - 2 * m), (WORD)(h - 2 * m), inner);
}

/** A grate lying in the floor strip, not the tall event-sprite box. */
static void floorHatchRect(tWallGfx *floor, WORD *x, WORD *y, WORD *w, WORD *h)
{
    WORD fw = (WORD)floor->_width;
    WORD fh = (WORD)floor->_height;
    WORD hw = fw / 5;
    WORD hh = fh - 4;
    if (hw < 12)
        hw = 12;
    if (hw > 36)
        hw = 36;
    if (hh < 6)
        hh = 6;
    if (hh > 14)
        hh = 14;
    if (hw > fw)
        hw = fw;
    if (hh > fh)
        hh = fh;
    *w = hw;
    *h = hh;
    *x = floor->_screen[0] + SOFFX + (fw - hw) / 2;
    *y = floor->_screen[1] + SOFFX + (fh - hh) / 2;
}

static void drawHatch(tBitMap *pBuf, WORD x, WORD y, WORD w, WORD h, UBYTE outer, UBYTE inner)
{
    WORD m, gap, bar, innerW;
    drawShaft(pBuf, x, y, w, h, outer, inner);
    m = (w < 16 || h < 10) ? 1 : 2;
    innerW = (WORD)(w - 2 * m);
    if (innerW < 6 || h - 2 * m < 3)
        return;
    bar = (innerW > 18) ? 2 : 1;
    gap = innerW / 3;
    fillClipped(pBuf, (WORD)(x + m + gap), (WORD)(y + m), bar, (WORD)(h - 2 * m), outer);
    fillClipped(pBuf, (WORD)(x + m + gap * 2), (WORD)(y + m), bar, (WORD)(h - 2 * m), outer);
}

/**
 * The wallset ships one event picture, and it reads as the charger.
 * Lifts, plates and the way out get their own mark. Story tiles stay bare floor.
 * A charger seen from the side has no event picture, so it gets a small cyan pad.
 */
static void drawFloorProp(tBitMap *pBuf, tWallset *pWallset, BYTE tx, BYTE ty, UBYTE kind)
{
    tWallGfx *floor = findFloorGfxForSlot(pWallset, tx, ty);
    tWallGfx *eventGfx = findTypedGfxForSlot(pWallset, tx, ty, MAZE_EVENT_TRIGGER);
    if (kind == PROP_CHARGER) {
        WORD s, x, y;
        if (eventGfx || !floor)
            return;
        s = (WORD)floor->_width / 8;
        if (s < 5)
            s = 5;
        if (s > 14)
            s = 14;
        x = floor->_screen[0] + SOFFX + (WORD)floor->_width / 2 - s / 2;
        y = floor->_screen[1] + SOFFX + (WORD)floor->_height - s - 2;
        drawShaft(pBuf, x, y, s, s, PROP_COL_CYAN, PROP_COL_GOLD);
        return;
    }
    if ((kind == PROP_LIFT || kind == PROP_EXIT) && floor) {
        WORD x, y, w, h;
        floorHatchRect(floor, &x, &y, &w, &h);
        drawHatch(pBuf, x, y, w, h,
            (kind == PROP_LIFT) ? PROP_COL_GOLD : PROP_COL_ORANGE, PROP_COL_BLACK);
        return;
    }
    if (kind == PROP_PLATE && floor) {
        WORD bw = (WORD)floor->_width / 2;
        WORD bh = (WORD)floor->_height / 5;
        WORD x, y;
        if (bw < 8)
            bw = 8;
        if (bw > 48)
            bw = 48;
        if (bh < 3)
            bh = 3;
        if (bh > 8)
            bh = 8;
        x = floor->_screen[0] + SOFFX + (WORD)floor->_width / 2 - bw / 2;
        y = floor->_screen[1] + SOFFX + (WORD)floor->_height - bh - 2;
        fillClipped(pBuf, x, y, bw, bh, PROP_COL_PALE);
        if (bh > 4)
            fillClipped(pBuf, (WORD)(x + 2), (WORD)(y + 1), (WORD)(bw - 4), (WORD)(bh - 2), PROP_COL_GRAY);
    }
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

static UBYTE monsterDepthForSlot(BYTE ty)
{
    if (ty == 0)
        return 0;
    if (ty == -1)
        return 1;
    if (ty == -2)
        return 2;
    return 3;
}

/** Feet on the floor tile. Clip to the viewport; the blitter does not. */
static void drawMonsterAt(tBitMap *pBuf, tWallGfx *floor, tMonster *mon, BYTE slotTy, tGameState *pGameState)
{
    tBitMap *gfx = NULL;
    tBitMap *msk = NULL;
    UBYTE attacking = 0;
    UBYTE partyFacing = 0;
    UBYTE partyX = 0;
    UBYTE partyY = 0;
    if (pGameState && pGameState->m_pCurrentParty) {
        partyFacing = pGameState->m_pCurrentParty->_PartyFacing;
        partyX = pGameState->m_pCurrentParty->_PartyX;
        partyY = pGameState->m_pCurrentParty->_PartyY;
        if (mon->_state == MONSTER_STATE_AGGRESSIVE
            && mon->_partyPosX == partyX && mon->_partyPosY == partyY)
            attacking = 1;
    }
    monsterGfxGet(mon->_monsterType,
        monsterGfxPick(mon->_monsterType, attacking, mon->_facing, partyFacing,
            mon->_partyPosX, mon->_partyPosY, partyX, partyY),
        monsterDepthForSlot(slotTy), &gfx, &msk);
    if (!gfx || !msk || !msk->Planes[0] || gfx->BytesPerRow == 0 || gfx->Rows == 0) {
        drawMonsterPlaceholder(pBuf, floor, mon);
        return;
    }

    WORD fw = (WORD)(gfx->BytesPerRow * 8);
    WORD fh = (WORD)gfx->Rows;
    WORD dx = floor->_screen[0] + SOFFX + (WORD)floor->_width / 2 - fw / 2;
    WORD dy = floor->_screen[1] + SOFFX + (WORD)floor->_height - fh;
    WORD sx = 0;
    WORD sy = 0;
    WORD dw = fw;
    WORD dh = fh;
    const WORD viewR = SOFFX + VIEW_W;
    const WORD viewB = SOFFX + VIEW_H;

    if (dx < SOFFX) {
        sx = (WORD)(sx + (SOFFX - dx));
        dw = (WORD)(dw - (SOFFX - dx));
        dx = SOFFX;
    }
    if (dy < SOFFX) {
        sy = (WORD)(sy + (SOFFX - dy));
        dh = (WORD)(dh - (SOFFX - dy));
        dy = SOFFX;
    }
    if (dx + dw > viewR)
        dw = (WORD)(viewR - dx);
    if (dy + dh > viewB)
        dh = (WORD)(viewB - dy);
    if (sx < 0)
        sx = 0;
    if (sy < 0)
        sy = 0;
    if (sx + dw > fw)
        dw = (WORD)(fw - sx);
    if (sy + dh > fh)
        dh = (WORD)(fh - sy);
    if (dw <= 0 || dh <= 0)
        return;

    blitUnsafeCopyMask(gfx, sx, sy, pBuf, dx, dy, dw, dh, (UBYTE *)msk->Planes[0]);
}

static void blitSpriteOnFloor(tBitMap *pBuf, tWallGfx *floor, tBitMap *gfx, tBitMap *msk)
{
    WORD fw = (WORD)floor->_width;
    WORD fh = (WORD)floor->_height;
    WORD iw = (WORD)(gfx->BytesPerRow * 8);
    WORD ih = (WORD)gfx->Rows;
    WORD dx = floor->_screen[0] + SOFFX + fw / 2 - iw / 2;
    WORD dy = floor->_screen[1] + SOFFX + fh - ih;
    WORD sx = 0;
    WORD sy = 0;
    WORD dw = iw;
    WORD dh = ih;
    const WORD viewR = SOFFX + VIEW_W;
    const WORD viewB = SOFFX + VIEW_H;

    if (dx < SOFFX) {
        sx = (WORD)(sx + (SOFFX - dx));
        dw = (WORD)(dw - (SOFFX - dx));
        dx = SOFFX;
    }
    if (dy < SOFFX) {
        sy = (WORD)(sy + (SOFFX - dy));
        dh = (WORD)(dh - (SOFFX - dy));
        dy = SOFFX;
    }
    if (dx + dw > viewR)
        dw = (WORD)(viewR - dx);
    if (dy + dh > viewB)
        dh = (WORD)(viewB - dy);
    if (sx < 0)
        sx = 0;
    if (sy < 0)
        sy = 0;
    if (sx + dw > iw)
        dw = (WORD)(iw - sx);
    if (sy + dh > ih)
        dh = (WORD)(ih - sy);
    if (dw <= 0 || dh <= 0)
        return;
    blitUnsafeCopyMask(gfx, sx, sy, pBuf, dx, dy, dw, dh, (UBYTE *)msk->Planes[0]);
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

static void drawGroundItemAt(tBitMap *pBuf, tWallGfx *floor, UBYTE itemIdx, BYTE slotTy)
{
    tBitMap *gfx = NULL;
    tBitMap *msk = NULL;
    if (!itemGfxFloor(itemIdx, monsterDepthForSlot(slotTy), &gfx, &msk)
        || !gfx || !msk || !msk->Planes[0]) {
        drawGroundItemPlaceholder(pBuf, floor, itemIdx);
        return;
    }
    blitSpriteOnFloor(pBuf, floor, gfx, msk);
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
        /*
         * Animate the panel with a real vertical slide (LoL-style feel), rather
         * than only shrinking in place, which looks like a floating crop.
         */
        UWORD fullHeight = pWallset->_tileset[doorW]->_height;
        UWORD stepDen = (DOOR_ANIM_FRAMES > 1) ? (DOOR_ANIM_FRAMES - 1) : 1;
        UWORD frame = anim->frame;
        if (frame > stepDen)
            frame = stepDen;

        /* Amount the door has opened in pixels. */
        UWORD openPx = (fullHeight * frame) / stepDen;
        if (anim->state == DOOR_ANIM_CLOSING)
            openPx = fullHeight - openPx;

        if (openPx < fullHeight) {  // Skip if fully open
            UWORD visibleHeight = fullHeight - openPx;
            UWORD srcY = pWallset->_tileset[doorW]->_y;
            UWORD dstY = pWallset->_tileset[doorW]->_screen[1] + SOFFX + openPx;
            UWORD si = pWallset->_tileset[doorW]->_setIndex;
            tBitMap *g = (si < pWallset->_gfxCount) ? pWallset->_gfx[si] : NULL;
            tBitMap *m = (si < pWallset->_gfxCount) ? pWallset->_mask[si] : NULL;
            if (g && m) {
                blitUnsafeCopyMask(g,
                    pWallset->_tileset[doorW]->_x, srcY,
                    pCurrentBuffer,
                    pWallset->_tileset[doorW]->_screen[0] + SOFFX, dstY,
                    pWallset->_tileset[doorW]->_width, visibleHeight,
                    (UBYTE *)m->Planes[0]);
            }
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

    /* A pad in front of you is walked onto. Clicking it is the same step. */
    if (doorCell == MAZE_EVENT_TRIGGER)
    {
        for (UBYTE i = 0; i < 18; i++)
        {
            tWallGfx *g;
            WORD x0, y0;
            if (!slotValid[i] || slotCx[i] != doorX || slotCy[i] != doorY)
                continue;
            g = findFloorGfxForSlot(pWallset, slotTx[i], slotTy[i]);
            if (floorPropAt(pGameState, doorX, doorY) == PROP_LIFT
                || floorPropAt(pGameState, doorX, doorY) == PROP_EXIT) {
                /* The whole floor strip in front of you, and the old tall prop box. */
                tWallGfx *hit = g;
                tWallGfx *tall = findTypedGfxForSlot(pWallset, slotTx[i], slotTy[i], MAZE_EVENT_TRIGGER);
                if (hit) {
                    x0 = hit->_screen[0] + SOFFX;
                    y0 = hit->_screen[1] + SOFFX;
                    if (bx >= x0 && by >= y0 && bx < x0 + (WORD)hit->_width && by < y0 + (WORD)hit->_height) {
                        pick->kind = VIEWPORT_PICK_FLOOR_AHEAD;
                        pick->cellX = doorX;
                        pick->cellY = doorY;
                        return;
                    }
                }
                if (tall) {
                    x0 = tall->_screen[0] + SOFFX;
                    y0 = tall->_screen[1] + SOFFX;
                    if (bx >= x0 && by >= y0 && bx < x0 + (WORD)tall->_width && by < y0 + (WORD)tall->_height) {
                        pick->kind = VIEWPORT_PICK_FLOOR_AHEAD;
                        pick->cellX = doorX;
                        pick->cellY = doorY;
                        return;
                    }
                }
                continue;
            }
            if (!g)
                g = findTypedGfxForSlot(pWallset, slotTx[i], slotTy[i], MAZE_EVENT_TRIGGER);
            if (!g)
                continue;
            x0 = g->_screen[0] + SOFFX;
            y0 = g->_screen[1] + SOFFX;
            if (bx >= x0 && by >= y0 && bx < x0 + (WORD)g->_width && by < y0 + (WORD)g->_height)
            {
                pick->kind = VIEWPORT_PICK_FLOOR_AHEAD;
                pick->cellX = doorX;
                pick->cellY = doorY;
                return;
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
        
        UBYTE prop = PROP_NONE;
        if (wmi == MAZE_EVENT_TRIGGER)
            prop = floorPropAt(pGameState, slotCx[i], slotCy[i]);

        for (UBYTE w = 0; w < pWallset->_tilesetCount; ++w)
        {
            if (tx == pWallset->_tileset[w]->_location[0] && ty == pWallset->_tileset[w]->_location[1])
            {
                UBYTE tileType = pWallset->_tileset[w]->_type;
                /* Locked cells are type 4. The wallset stores the door panel as
                 * type 2, so a locked door used to draw as bare floor until it
                 * opened and the cell type changed.
                 * Type 5 is one picture. Only the charger wears it. */
                UBYTE drawTile = (UBYTE)(wmi == tileType);
                if (drawTile && wmi == MAZE_EVENT_TRIGGER && tileType == MAZE_EVENT_TRIGGER
                    && prop != PROP_CHARGER)
                    drawTile = 0;
                if ((wmi == MAZE_DOOR_LOCKED && tileType == MAZE_DOOR)
                    || (wmi == MAZE_DOOR_OPEN && tileType == 0)
                    || (wmi == MAZE_DOOR && tileType == 0)
                    || (wmi == MAZE_DOOR_LOCKED && tileType == 0)
                    || (wmi == MAZE_EVENT_TRIGGER && tileType == 0))
                    drawTile = 1;
                if (drawTile)
                {
                    UBYTE doorWidth = pWallset->_tileset[w]->_width;
                    UBYTE doorHeight = pWallset->_tileset[w]->_height;
                    UWORD si2 = pWallset->_tileset[w]->_setIndex;
                    tBitMap *g2 = (si2 < pWallset->_gfxCount) ? pWallset->_gfx[si2] : NULL;
                    tBitMap *m2 = (si2 < pWallset->_gfxCount) ? pWallset->_mask[si2] : NULL;
                    if (g2 && m2) {
                    blitUnsafeCopyMask(g2,
                                       pWallset->_tileset[w]->_x, pWallset->_tileset[w]->_y,
                                       pCurrentBuffer,
                                       pWallset->_tileset[w]->_screen[0] + SOFFX, pWallset->_tileset[w]->_screen[1] + SOFFX,
                                       doorWidth, doorHeight,
                                       (UBYTE *)m2->Planes[0]);
                    }
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

        if (prop != PROP_NONE)
            drawFloorProp(pCurrentBuffer, pWallset, tx, ty, prop);

        if (wmi == MAZE_FLOOR || wmi == MAZE_DOOR_OPEN || wmi == MAZE_EVENT_TRIGGER) {
            tWallGfx *floorGfx = findFloorGfxForSlot(pWallset, tx, ty);
            tMonsterList *ml = pGameState->m_pMonsterList;
            if (floorGfx && ml) {
                for (UBYTE mi = 0; mi < ml->_numMonsters; mi++) {
                    tMonster *mon = ml->_monsters[mi];
                    if (!mon || mon->_state == MONSTER_STATE_DEAD)
                        continue;
                    if (mon->_partyPosX == slotCx[i] && mon->_partyPosY == slotCy[i])
                        drawMonsterAt(pCurrentBuffer, floorGfx, mon, ty, pGameState);
                }
            }
        }
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

        tGroundItemList *gl = &pGameState->m_groundItems;
        for (UBYTE gi = 0; gi < gl->count; gi++)
        {
            if (gl->items[gi].x == cx && gl->items[gi].y == cy)
                drawGroundItemAt(pCurrentBuffer, floorGfx, gl->items[gi].itemIdx, slotTy[i]);
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
