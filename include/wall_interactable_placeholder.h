#pragma once

#include <ace/types.h>
#include <ace/utils/bitmap.h>

/* Viewport uses bitplane-5 band (colors 32–63); pick distinct fills for wall vs door placeholders */
#define WALL_INTERACT_PLACEHOLDER_WALL_IDLE 50
#define WALL_INTERACT_PLACEHOLDER_WALL_ACTIVE 52
#define WALL_INTERACT_PLACEHOLDER_DOOR_IDLE 46
#define WALL_INTERACT_PLACEHOLDER_DOOR_ACTIVE 48

struct _wallset;
typedef struct _wallset tWallset;
struct _wallGfx;
typedef struct _wallGfx tWallGfx;

/** Wall/floor tile at this projection slot to anchor a fake button rect (prefer wall, else floor). */
tWallGfx* wallInteractableAnchorGfxAtSlot(tWallset* pWallset, BYTE slotTx, BYTE slotTy);

/** Buffer-space rect (includes +SOFFX like wall blits). Returns 0 if no anchor. */
UBYTE wallInteractablePlaceholderGetRect(tWallset* pWallset, BYTE slotTx, BYTE slotTy,
    WORD* outSx, WORD* outSy, UWORD* outW, UWORD* outH);

UBYTE wallInteractablePlaceholderHit(tWallset* pWallset, BYTE slotTx, BYTE slotTy, WORD bufX, WORD bufY);

void wallInteractablePlaceholderDraw(tBitMap* pBuffer, tWallset* pWallset, BYTE slotTx, BYTE slotTy,
    UBYTE colorIdle, UBYTE colorActive, UBYTE isActive);
