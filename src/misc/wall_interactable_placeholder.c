#include "wall_interactable_placeholder.h"
#include "wallset.h"
#include "maze.h"
#include <ace/managers/blit.h>

#ifndef SOFFX
#define SOFFX 5
#endif

tWallGfx* wallInteractableAnchorGfxAtSlot(tWallset* pWallset, BYTE slotTx, BYTE slotTy)
{
    if (!pWallset)
        return NULL;
    tWallGfx* floor = NULL;
    for (UWORD i = 0; i < pWallset->_tilesetCount; i++)
    {
        tWallGfx* g = pWallset->_tileset[i];
        if (!g || g->_location[0] != slotTx || g->_location[1] != slotTy)
            continue;
        if (g->_type == MAZE_WALL)
            return g;
        if (g->_type == MAZE_FLOOR && !floor)
            floor = g;
    }
    return floor;
}

UBYTE wallInteractablePlaceholderGetRect(tWallset* pWallset, BYTE slotTx, BYTE slotTy,
    WORD* outSx, WORD* outSy, UWORD* outW, UWORD* outH)
{
    tWallGfx* a = wallInteractableAnchorGfxAtSlot(pWallset, slotTx, slotTy);
    if (!a || !outSx || !outSy || !outW || !outH)
        return 0;
    WORD fw = (WORD)a->_width;
    WORD fh = (WORD)a->_height;
    UWORD bw = (UWORD)((fw * 14) / 100);
    UWORD bh = (UWORD)((fh * 12) / 100);
    if (bw < 8)
        bw = 8;
    if (bh < 7)
        bh = 7;
    *outSx = a->_screen[0] + SOFFX + (fw - (WORD)bw) / 2;
    *outSy = a->_screen[1] + SOFFX + fh / 5;
    *outW = bw;
    *outH = bh;
    if (*outSx < 0 || *outSy < 0)
        return 0;
    return 1;
}

UBYTE wallInteractablePlaceholderHit(tWallset* pWallset, BYTE slotTx, BYTE slotTy, WORD bufX, WORD bufY)
{
    WORD sx, sy;
    UWORD bw, bh;
    if (!wallInteractablePlaceholderGetRect(pWallset, slotTx, slotTy, &sx, &sy, &bw, &bh))
        return 0;
    return (bufX >= sx && bufY >= sy && bufX < sx + (WORD)bw && bufY < sy + (WORD)bh) ? 1 : 0;
}

void wallInteractablePlaceholderDraw(tBitMap* pBuffer, tWallset* pWallset, BYTE slotTx, BYTE slotTy,
    UBYTE colorIdle, UBYTE colorActive, UBYTE isActive)
{
    if (!pBuffer || !pWallset)
        return;
    WORD sx, sy;
    UWORD bw, bh;
    if (!wallInteractablePlaceholderGetRect(pWallset, slotTx, slotTy, &sx, &sy, &bw, &bh))
        return;
    UBYTE c = isActive ? colorActive : colorIdle;
    blitRect(pBuffer, (UWORD)sx, (UWORD)sy, bw, bh, c);
}
