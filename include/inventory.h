#include <ace/types.h>
#include "item.h"
#define INVENTORY_MAX_ITEMS 64

typedef struct  _tInventory
{
    UBYTE pItems[INVENTORY_MAX_ITEMS];
    UBYTE ubItemCount;
    UBYTE pEquippedItem[4];
    UBYTE ubShowInventoryIndex;

} tInventory;
