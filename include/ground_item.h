#pragma once

#include <ace/types.h>

#define GROUND_ITEMS_MAX 32

typedef struct {
	UBYTE x;
	UBYTE y;
	UBYTE itemIdx;
	UBYTE qty;
} tGroundItem;

typedef struct {
	UBYTE count;
	tGroundItem items[GROUND_ITEMS_MAX];
} tGroundItemList;

void groundItemListClear(tGroundItemList *list);

/** Add stack at (x,y). Fails if list full or qty 0. */
UBYTE groundItemAdd(tGroundItemList *list, UBYTE x, UBYTE y, UBYTE itemIdx, UBYTE qty);

/** Remove qty from stack at cell matching itemIdx; returns 1 if something removed. */
UBYTE groundItemRemoveAt(tGroundItemList *list, UBYTE x, UBYTE y, UBYTE itemIdx, UBYTE qty);
