#pragma once

#include <ace/types.h>
#include "item.h"

// Forward declaration
struct _character;
typedef struct _character tCharacter;

#define INVENTORY_MAX_ITEMS 64

typedef struct  _tInventory
{
    UBYTE pItems[INVENTORY_MAX_ITEMS];  // Item indices
    UBYTE pItemQuantities[INVENTORY_MAX_ITEMS];  // Quantities for stackable items
    UBYTE ubItemCount;
    UBYTE pEquippedItem[4];  // Equipped items (weapon, armor, shield, accessory)
    UBYTE ubShowInventoryIndex;
    UBYTE _pad[2];  // Padding for 68020+ alignment

} tInventory;

// Inventory management
tInventory* inventoryCreate(void);
void inventoryDestroy(tInventory* pInventory);

// Item management
UBYTE inventoryAddItem(tInventory* pInventory, UBYTE ubItemIndex, UBYTE ubQuantity);
UBYTE inventoryRemoveItem(tInventory* pInventory, UBYTE ubItemIndex, UBYTE ubQuantity);
UBYTE inventoryHasItem(tInventory* pInventory, UBYTE ubItemIndex);
UBYTE inventoryHasItemByKeyId(tInventory* pInventory, UBYTE ubKeyId);
UBYTE inventoryGetItemCount(tInventory* pInventory, UBYTE ubItemIndex);

// Item usage
UBYTE inventoryUseItem(tInventory* pInventory, UBYTE ubItemIndex, tCharacter* pCharacter);
UBYTE inventoryEquipItem(tInventory* pInventory, UBYTE ubItemIndex, UBYTE ubSlot);
UBYTE inventoryUnequipItem(tInventory* pInventory, UBYTE ubSlot);
