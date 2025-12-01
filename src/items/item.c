#include "item.h"
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>
#include <string.h>

tItem* s_vecItems = NULL;
UBYTE s_ubItemCount = 0;

void itemSystemInit(void)
{
    // Initialize item system - items will be loaded from data file
    s_vecItems = NULL;
    s_ubItemCount = 0;
}

void itemSystemDestroy(void)
{
    if (s_vecItems)
    {
        // Free item names if allocated
        for (UBYTE i = 0; i < s_ubItemCount; i++)
        {
            if (s_vecItems[i].pszName)
            {
                memFree(s_vecItems[i].pszName, strlen(s_vecItems[i].pszName) + 1);
            }
        }
        memFree(s_vecItems, sizeof(tItem) * s_ubItemCount);
        s_vecItems = NULL;
        s_ubItemCount = 0;
    }
}

void loadItems(const char* filename)
{
    // TODO: Load items from data file
    // For now, create a few example items
    itemSystemInit();
    
    // Allocate space for items (example: 32 items)
    s_ubItemCount = 32;
    s_vecItems = (tItem*)memAllocFastClear(sizeof(tItem) * s_ubItemCount);
    
    if (!s_vecItems)
    {
        logWrite("ERROR: Failed to allocate item array\n");
        return;
    }
    
    // Example items will be created here or loaded from file
    // This is a placeholder for the actual file loading
}

tItem* getItem(UBYTE ubIndex)
{
    if (ubIndex >= s_ubItemCount || !s_vecItems)
        return NULL;
    return &s_vecItems[ubIndex];
}

UBYTE findItemByName(const char* pszName)
{
    if (!s_vecItems || !pszName)
        return 255; // Invalid index
    
    for (UBYTE i = 0; i < s_ubItemCount; i++)
    {
        if (s_vecItems[i].pszName && strcmp(s_vecItems[i].pszName, pszName) == 0)
        {
            return i;
        }
    }
    return 255; // Not found
}

UBYTE findItemByKeyId(UBYTE ubKeyId)
{
    if (!s_vecItems)
        return 255; // Invalid index
    
    for (UBYTE i = 0; i < s_ubItemCount; i++)
    {
        if ((s_vecItems[i].ubFlags & ITEM_FLAG_KEY) && s_vecItems[i].ubKeyId == ubKeyId)
        {
            return i;
        }
    }
    return 255; // Not found
}