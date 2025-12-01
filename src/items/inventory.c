#include "inventory.h"
#include "item.h"
#include "character.h"
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>
#include <string.h>

tInventory* inventoryCreate(void)
{
    tInventory* pInventory = (tInventory*)memAllocFastClear(sizeof(tInventory));
    if (pInventory)
    {
        pInventory->ubItemCount = 0;
        pInventory->ubShowInventoryIndex = 0;
        // Initialize arrays
        for (UBYTE i = 0; i < INVENTORY_MAX_ITEMS; i++)
        {
            pInventory->pItems[i] = 255; // 255 = empty slot
            pInventory->pItemQuantities[i] = 0;
        }
        for (UBYTE i = 0; i < 4; i++)
        {
            pInventory->pEquippedItem[i] = 255; // 255 = no item equipped
        }
    }
    return pInventory;
}

void inventoryDestroy(tInventory* pInventory)
{
    if (pInventory)
    {
        memFree(pInventory, sizeof(tInventory));
    }
}

UBYTE inventoryAddItem(tInventory* pInventory, UBYTE ubItemIndex, UBYTE ubQuantity)
{
    if (!pInventory || ubItemIndex >= s_ubItemCount)
        return 0;
    
    tItem* pItem = getItem(ubItemIndex);
    if (!pItem)
        return 0;
    
    // Check if item is stackable and already in inventory
    if (pItem->ubFlags & ITEM_FLAG_STACKABLE)
    {
        for (UBYTE i = 0; i < pInventory->ubItemCount; i++)
        {
            if (pInventory->pItems[i] == ubItemIndex)
            {
                pInventory->pItemQuantities[i] += ubQuantity;
                return 1;
            }
        }
    }
    
    // Check if item is unique and already in inventory
    if (pItem->ubFlags & ITEM_FLAG_UNIQUE)
    {
        if (inventoryHasItem(pInventory, ubItemIndex))
        {
            return 0; // Already have unique item
        }
    }
    
    // Find empty slot
    if (pInventory->ubItemCount >= INVENTORY_MAX_ITEMS)
        return 0; // Inventory full
    
    pInventory->pItems[pInventory->ubItemCount] = ubItemIndex;
    pInventory->pItemQuantities[pInventory->ubItemCount] = ubQuantity;
    pInventory->ubItemCount++;
    return 1;
}

UBYTE inventoryRemoveItem(tInventory* pInventory, UBYTE ubItemIndex, UBYTE ubQuantity)
{
    if (!pInventory)
        return 0;
    
    for (UBYTE i = 0; i < pInventory->ubItemCount; i++)
    {
        if (pInventory->pItems[i] == ubItemIndex)
        {
            tItem* pItem = getItem(ubItemIndex);
            if (pItem && (pItem->ubFlags & ITEM_FLAG_STACKABLE))
            {
                if (pInventory->pItemQuantities[i] > ubQuantity)
                {
                    pInventory->pItemQuantities[i] -= ubQuantity;
                    return 1;
                }
                else
                {
                    // Remove item completely
                    // Shift remaining items
                    for (UBYTE j = i; j < pInventory->ubItemCount - 1; j++)
                    {
                        pInventory->pItems[j] = pInventory->pItems[j + 1];
                        pInventory->pItemQuantities[j] = pInventory->pItemQuantities[j + 1];
                    }
                    pInventory->ubItemCount--;
                    pInventory->pItems[pInventory->ubItemCount] = 255;
                    pInventory->pItemQuantities[pInventory->ubItemCount] = 0;
                    return 1;
                }
            }
            else
            {
                // Non-stackable item, remove it
                for (UBYTE j = i; j < pInventory->ubItemCount - 1; j++)
                {
                    pInventory->pItems[j] = pInventory->pItems[j + 1];
                    pInventory->pItemQuantities[j] = pInventory->pItemQuantities[j + 1];
                }
                pInventory->ubItemCount--;
                pInventory->pItems[pInventory->ubItemCount] = 255;
                pInventory->pItemQuantities[pInventory->ubItemCount] = 0;
                return 1;
            }
        }
    }
    return 0; // Item not found
}

UBYTE inventoryHasItem(tInventory* pInventory, UBYTE ubItemIndex)
{
    if (!pInventory)
        return 0;
    
    for (UBYTE i = 0; i < pInventory->ubItemCount; i++)
    {
        if (pInventory->pItems[i] == ubItemIndex)
        {
            return 1;
        }
    }
    return 0;
}

UBYTE inventoryHasItemByKeyId(tInventory* pInventory, UBYTE ubKeyId)
{
    if (!pInventory)
        return 0;
    
    for (UBYTE i = 0; i < pInventory->ubItemCount; i++)
    {
        tItem* pItem = getItem(pInventory->pItems[i]);
        if (pItem && (pItem->ubFlags & ITEM_FLAG_KEY))
        {
            // Check if this key matches (0 = master key, or specific key ID)
            if (pItem->ubKeyId == 0 || pItem->ubKeyId == ubKeyId)
            {
                return 1;
            }
        }
    }
    return 0;
}

UBYTE inventoryGetItemCount(tInventory* pInventory, UBYTE ubItemIndex)
{
    if (!pInventory)
        return 0;
    
    for (UBYTE i = 0; i < pInventory->ubItemCount; i++)
    {
        if (pInventory->pItems[i] == ubItemIndex)
        {
            return pInventory->pItemQuantities[i];
        }
    }
    return 0;
}

UBYTE inventoryUseItem(tInventory* pInventory, UBYTE ubItemIndex, tCharacter* pCharacter)
{
    if (!pInventory || !pCharacter)
        return 0;
    
    tItem* pItem = getItem(ubItemIndex);
    if (!pItem || !(pItem->ubFlags & ITEM_FLAG_ACTIVATABLE))
        return 0;
    
    // Check if item is in inventory
    if (!inventoryHasItem(pInventory, ubItemIndex))
        return 0;
    
    // Use item based on usage type
    switch (pItem->ubUsageType)
    {
        case ITEM_USAGE_HEAL:
            if (pCharacter->_HP < pCharacter->_MaxHP)
            {
                pCharacter->_HP += pItem->ubValue;
                if (pCharacter->_HP > pCharacter->_MaxHP)
                    pCharacter->_HP = pCharacter->_MaxHP;
                // Remove consumable item
                if (pItem->ubFlags & ITEM_FLAG_CONSUMABLE)
                {
                    inventoryRemoveItem(pInventory, ubItemIndex, 1);
                }
                return 1;
            }
            break;
            
        case ITEM_USAGE_ENERGY:
            if (pCharacter->_MP < pCharacter->_MaxMP)
            {
                pCharacter->_MP += pItem->ubValue;
                if (pCharacter->_MP > pCharacter->_MaxMP)
                    pCharacter->_MP = pCharacter->_MaxMP;
                // Remove consumable item
                if (pItem->ubFlags & ITEM_FLAG_CONSUMABLE)
                {
                    inventoryRemoveItem(pInventory, ubItemIndex, 1);
                }
                return 1;
            }
            break;
            
        case ITEM_USAGE_UNLOCK:
            // Keys are handled separately when interacting with doors
            return 1;
            
        case ITEM_USAGE_SCRIPT:
            // Script usage handled by event system
            return 1;
            
        default:
            break;
    }
    
    return 0;
}

UBYTE inventoryEquipItem(tInventory* pInventory, UBYTE ubItemIndex, UBYTE ubSlot)
{
    if (!pInventory || ubSlot >= 4)
        return 0;
    
    tItem* pItem = getItem(ubItemIndex);
    if (!pItem || !(pItem->ubFlags & ITEM_FLAG_EQUIPPABLE))
        return 0;
    
    // Check if item is in inventory
    if (!inventoryHasItem(pInventory, ubItemIndex))
        return 0;
    
    // Unequip current item in slot if any
    if (pInventory->pEquippedItem[ubSlot] != 255)
    {
        inventoryUnequipItem(pInventory, ubSlot);
    }
    
    // Equip new item
    pInventory->pEquippedItem[ubSlot] = ubItemIndex;
    return 1;
}

UBYTE inventoryUnequipItem(tInventory* pInventory, UBYTE ubSlot)
{
    if (!pInventory || ubSlot >= 4)
        return 0;
    
    if (pInventory->pEquippedItem[ubSlot] == 255)
        return 0; // Nothing equipped
    
    pInventory->pEquippedItem[ubSlot] = 255;
    return 1;
}
