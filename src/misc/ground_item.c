#include "ground_item.h"
#include <string.h>

void groundItemListClear(tGroundItemList *list)
{
	if (!list)
		return;
	list->count = 0;
	memset(list->items, 0, sizeof(list->items));
}

UBYTE groundItemAdd(tGroundItemList *list, UBYTE x, UBYTE y, UBYTE itemIdx, UBYTE qty)
{
	if (!list || qty == 0 || list->count >= GROUND_ITEMS_MAX)
		return 0;
	for (UBYTE i = 0; i < list->count; i++) {
		if (list->items[i].x == x && list->items[i].y == y && list->items[i].itemIdx == itemIdx) {
			list->items[i].qty += qty;
			return 1;
		}
	}
	tGroundItem *g = &list->items[list->count];
	g->x = x;
	g->y = y;
	g->itemIdx = itemIdx;
	g->qty = qty;
	list->count++;
	return 1;
}

UBYTE groundItemRemoveAt(tGroundItemList *list, UBYTE x, UBYTE y, UBYTE itemIdx, UBYTE qty)
{
	if (!list || qty == 0)
		return 0;
	for (UBYTE i = 0; i < list->count; i++) {
		if (list->items[i].x != x || list->items[i].y != y || list->items[i].itemIdx != itemIdx)
			continue;
		if (list->items[i].qty > qty) {
			list->items[i].qty -= qty;
			return 1;
		}
		for (UBYTE j = i; j < list->count - 1; j++)
			list->items[j] = list->items[j + 1];
		list->count--;
		return 1;
	}
	return 0;
}
