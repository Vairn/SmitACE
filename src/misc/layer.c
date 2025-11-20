#include "layer.h"

#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/managers/mouse.h>

#include "mouse_pointer.h"
#include "screen.h"

#define REGION_HASH_SIZE 32
#define REGION_HASH_MASK (REGION_HASH_SIZE - 1)

enum eRegionState
{
    REGION_IDLE,
    REGION_HOVERED,
    REGION_PRESSED
};

typedef struct _RegionInternal
{
    RegionId id;  // 2 bytes (UWORD)
    Region region;  // Size depends on Region structure
    enum eRegionState state;  // 4 bytes (enum, int-sized on 68000/68020)
    // If Region ends at odd address, state would be misaligned, but Region should be properly aligned
    // After state (4 bytes), pointer needs 4-byte alignment - state is already 4-byte aligned
    struct _RegionInternal *pNext;
} RegionInternal;

typedef struct _Layer
{
    tUwRect bounds;
    UWORD uwRegionCount;
    RegionId nextRegionId;
    RegionInternal *pFirstRegion;
    RegionInternal *pLastRegion;
    RegionInternal *pRegionHash[REGION_HASH_SIZE];  // Simple hash table for region lookups
    UBYTE ubIsEnabled;
    UBYTE ubUpdateOutsideBounds;
    UBYTE ubMousePointerUpdateEnabled;
    UBYTE ubBoundsDirty;
} Layer;

tUwRect calculateLayerBounds(Layer * pLayer);

Layer* layerCreate(void)
{
    logBlockBegin("layerCreate");
    Layer *pLayer = memAllocFastClear(sizeof(Layer));
    logBlockEnd("layerCreate");

    return pLayer;
}

void layerUpdate(Layer* pLayer)
{
    if (!pLayer)
    {
        logWrite("layerUpdate: layer cannot be null");
        return;
    }

    if (
        !pLayer->ubIsEnabled ||
        (!pLayer->ubUpdateOutsideBounds && !mouseInRect(MOUSE_PORT_1, pLayer->bounds)))
    {
        return;
    }

    // Cache all mouse state at the start
    UBYTE ubMousePressedLeft = mouseCheck(MOUSE_PORT_1, MOUSE_LMB);
    UBYTE ubMousePressedRight = mouseCheck(MOUSE_PORT_1, MOUSE_RMB);
    UBYTE ubMousePressed = ubMousePressedLeft || ubMousePressedRight;
    mouse_pointer_t mousePointerId = MOUSE_POINTER;
    
    // Cache mouse position
    UWORD uwMouseX = mouseGetX(MOUSE_PORT_1);
    UWORD uwMouseY = mouseGetY(MOUSE_PORT_1);

    // Update bounds if dirty
    if (pLayer->ubBoundsDirty)
    {
        pLayer->bounds = calculateLayerBounds(pLayer);
        pLayer->ubBoundsDirty = 0;
    }

    RegionInternal *pCurrent = pLayer->pFirstRegion;
    while (pCurrent)
    {
        // Fast mouse position check using cached coordinates
        UBYTE ubOverRegion = (
            uwMouseX >= pCurrent->region.bounds.uwX &&
            uwMouseX < (pCurrent->region.bounds.uwX + pCurrent->region.bounds.uwWidth) &&
            uwMouseY >= pCurrent->region.bounds.uwY &&
            uwMouseY < (pCurrent->region.bounds.uwY + pCurrent->region.bounds.uwHeight)
        );

        if (ubOverRegion)
        {
            mousePointerId = pCurrent->region.pointer;
        }

        // Optimized state machine with combined conditions
        switch (pCurrent->state)
        {
            case REGION_IDLE:
                if (ubOverRegion)
                {
                    pCurrent->state = ubMousePressed ? REGION_PRESSED : REGION_HOVERED;
                    if (ubMousePressed)
                    {
                        SAFE_CB_CALL(pCurrent->region.cbOnPressed, &pCurrent->region, ubMousePressedLeft, ubMousePressedRight);
                    }
                    else
                    {
                        SAFE_CB_CALL(pCurrent->region.cbOnHovered, &pCurrent->region);
                    }
                }
                else
                {
                    SAFE_CB_CALL(pCurrent->region.cbOnIdle, &pCurrent->region);
                }
                break;

            case REGION_HOVERED:
                if (!ubOverRegion)
                {
                    pCurrent->state = REGION_IDLE;
                    SAFE_CB_CALL(pCurrent->region.cbOnUnhovered, &pCurrent->region);
                }
                else if (ubMousePressed)
                {
                    pCurrent->state = REGION_PRESSED;
                    SAFE_CB_CALL(pCurrent->region.cbOnPressed, &pCurrent->region, ubMousePressedLeft, ubMousePressedRight);
                }
                break;

            case REGION_PRESSED:
                if (!ubOverRegion)
                {
                    pCurrent->state = REGION_IDLE;
                    SAFE_CB_CALL(pCurrent->region.cbOnUnhovered, &pCurrent->region);
                }
                else if (!ubMousePressed)
                {
                    pCurrent->state = REGION_HOVERED;
                    SAFE_CB_CALL(pCurrent->region.cbOnReleased, &pCurrent->region, ubMousePressedLeft, ubMousePressedRight);
                }
                break;

            default:
                logWrite("layerUpdate: Unknown region state %d", pCurrent->state);
                break;
        }

        pCurrent = pCurrent->pNext;
    }

    if (pLayer->ubMousePointerUpdateEnabled)
    {
        mouse_pointer_switch(mousePointerId);
    }
}

void layerDestroy(Layer* pLayer)
{
    logBlockBegin("layerDestroy");
    if (pLayer)
    {
        RegionInternal *pCurrent = pLayer->pFirstRegion;
        while (pCurrent)
        {
            RegionInternal *pNext = pCurrent->pNext;
            memFree(pCurrent, sizeof(RegionInternal));

            pCurrent = pNext;
        }

        memFree(pLayer, sizeof(Layer));
    }

    logBlockEnd("layerDestroy");
}

void layerSetEnable(Layer *pLayer, UBYTE ubIsEnabled)
{
    if (!pLayer)
    {
        logWrite("layerSetEnable: layer cannot be null");
    }

    pLayer->ubIsEnabled = ubIsEnabled;
}

void layerSetUpdateOutsideBounds(Layer *pLayer, UBYTE ubUpdateOutsideBounds)
{
    if (!pLayer)
    {
        logWrite("layerSetUpdateOutsideBounds: layer cannot be null");
    }

    pLayer->ubUpdateOutsideBounds = ubUpdateOutsideBounds;
}

RegionId layerAddRegion(Layer *pLayer, Region *pRegion)
{
    logBlockBegin("layerAddRegion");
    if (!pLayer)
    {
        logWrite("layerAddRegion: layer cannot be null");
        return INVALID_REGION;
    }

    if (!pRegion)
    {
        logWrite("layerAddRegion: region cannot be null");
        return INVALID_REGION;
    }

    RegionInternal *pNewRegion = memAllocFastClear(sizeof(RegionInternal));
    if (!pNewRegion)
    {
        logWrite("layerAddRegion: Unable to allocate memory for region");
        return INVALID_REGION;
    }

    pNewRegion->id = pLayer->nextRegionId;
    pLayer->nextRegionId++;

    memcpy(&pNewRegion->region, pRegion, sizeof(Region));
    pNewRegion->region.bounds.uwY;//+= g_mainScreen->uwOffset;
    pNewRegion->pNext = 0;

    // Add to hash table
    UWORD hashIndex = pNewRegion->id & REGION_HASH_MASK;
    pNewRegion->pNext = pLayer->pRegionHash[hashIndex];
    pLayer->pRegionHash[hashIndex] = pNewRegion;

    if (!pLayer->pFirstRegion)
    {
        pLayer->pFirstRegion = pNewRegion;
    }
    else
    {
        pLayer->pLastRegion->pNext = pNewRegion;
    }
    pLayer->pLastRegion = pNewRegion;
    pLayer->ubBoundsDirty = 1;

    logBlockEnd("layerAddRegion");

    return pNewRegion->id;
}

const Region *layerGetRegion(Layer *pLayer, RegionId id)
{
    if (!pLayer)
    {
        logWrite("layerGetRegion: layer cannot be null");
        return 0;
    }

    // Use hash table for lookup
    UWORD hashIndex = id & REGION_HASH_MASK;
    RegionInternal *pCurrent = pLayer->pRegionHash[hashIndex];
    while (pCurrent && pCurrent->id != id)
    {
        pCurrent = pCurrent->pNext;
    }

    return (pCurrent) ? &pCurrent->region : 0;
}

void layerRemoveRegion(Layer *pLayer, RegionId id)
{
    logBlockBegin("layerRemoveRegion");
    if (!pLayer)
    {
        logWrite("layerRemoveRegion: layer cannot be null");
        return;
    }

    // Remove from hash table
    UWORD hashIndex = id & REGION_HASH_MASK;
    RegionInternal *pPrev = NULL;
    RegionInternal *pCurrent = pLayer->pRegionHash[hashIndex];
    
    while(pCurrent && pCurrent->id != id)
    {
        pPrev = pCurrent;
        pCurrent = pCurrent->pNext;
    }

    if (pCurrent == 0)
    {
        logWrite("layerRemoveRegion: no region with id %d", id);
        return;
    }

    // Update hash table
    if (pPrev)
    {
        pPrev->pNext = pCurrent->pNext;
    }
    else
    {
        pLayer->pRegionHash[hashIndex] = pCurrent->pNext;
    }

    // Update first/last pointers if needed
    if (pCurrent == pLayer->pFirstRegion)
    {
        pLayer->pFirstRegion = pCurrent->pNext;
    }
    if (pCurrent == pLayer->pLastRegion)
    {
        pLayer->pLastRegion = pPrev;
    }

    memFree(pCurrent, sizeof(RegionInternal));
    pLayer->ubBoundsDirty = 1;

    logBlockEnd("layerRemoveRegion");
}

void layerEnablePointerUpdate(Layer *pLayer, UBYTE ubEnable)
{
    if (!pLayer)
    {
        logWrite("layerEnablePointerUpdate: layer cannot be null");
        return;
    }

    pLayer->ubMousePointerUpdateEnabled = ubEnable;
}

/*
 * Internal function.
 * Assumptions:
 *   - layer is not null
 *   - layer has at least one region
 */
tUwRect calculateLayerBounds(Layer *pLayer)
{
    tUwRect *pBounds = &pLayer->pFirstRegion->region.bounds;
    UWORD minX = pBounds->uwX;
    UWORD minY = pBounds->uwY;
    UWORD maxX = minX + pBounds->uwWidth;
    UWORD maxY = minY + pBounds->uwHeight;

    // Skip first region since we already processed it
    RegionInternal *pCurrent = pLayer->pFirstRegion->pNext;
    while (pCurrent)
    {
        pBounds = &pCurrent->region.bounds;
        minX = MIN(minX, pBounds->uwX);
        minY = MIN(minY, pBounds->uwY);
        maxX = MAX(maxX, pBounds->uwX + pBounds->uwWidth);
        maxY = MAX(maxY, pBounds->uwY + pBounds->uwHeight);
        pCurrent = pCurrent->pNext;
    }

    return (tUwRect) {
        .uwX = minX, .uwY = minY,
        .uwWidth = maxX - minX, .uwHeight = maxY - minY
    };
}

