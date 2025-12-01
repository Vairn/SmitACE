#include "doorlock.h"
#include "inventory.h"
#include "item.h"
#include <ace/managers/memory.h>
#include <ace/managers/log.h>

tDoorLock* doorLockCreate(UBYTE doorX, UBYTE doorY, UBYTE type, UBYTE keyId)
{
    tDoorLock* pLock = (tDoorLock*)memAllocFastClear(sizeof(tDoorLock));
    if (pLock)
    {
        pLock->_doorX = doorX;
        pLock->_doorY = doorY;
        pLock->_state = DOORLOCK_STATE_LOCKED;
        pLock->_type = type;
        pLock->_keyId = keyId;
        pLock->_code = 0;
        pLock->_next = NULL;
    }
    return pLock;
}

void doorLockDestroy(tDoorLock* pLock)
{
    if (pLock)
    {
        memFree(pLock, sizeof(tDoorLock));
    }
}

void doorLockListCreate(tDoorLockList* pList)
{
    if (pList)
    {
        pList->_numLocks = 0;
        pList->_locks = NULL;
    }
}

void doorLockListDestroy(tDoorLockList* pList)
{
    if (pList)
    {
        tDoorLock* pLock = pList->_locks;
        while (pLock)
        {
            tDoorLock* pNext = pLock->_next;
            doorLockDestroy(pLock);
            pLock = pNext;
        }
        pList->_locks = NULL;
        pList->_numLocks = 0;
    }
}

void doorLockAdd(tDoorLockList* pList, tDoorLock* pLock)
{
    if (!pList || !pLock)
        return;
    
    pLock->_next = pList->_locks;
    pList->_locks = pLock;
    pList->_numLocks++;
}

void doorLockRemove(tDoorLockList* pList, tDoorLock* pLock)
{
    if (!pList || !pLock)
        return;
    
    if (pList->_locks == pLock)
    {
        pList->_locks = pLock->_next;
        pList->_numLocks--;
        doorLockDestroy(pLock);
        return;
    }
    
    tDoorLock* pCurrent = pList->_locks;
    while (pCurrent && pCurrent->_next != pLock)
    {
        pCurrent = pCurrent->_next;
    }
    
    if (pCurrent)
    {
        pCurrent->_next = pLock->_next;
        pList->_numLocks--;
        doorLockDestroy(pLock);
    }
}

UBYTE doorLockTryUnlock(tDoorLock* pLock, tInventory* pInventory, UBYTE ubKeyId)
{
    if (!pLock || !pInventory)
        return 0;
    
    if (pLock->_state != DOORLOCK_STATE_LOCKED)
        return 0; // Already unlocked or broken
    
    switch (pLock->_type)
    {
        case DOORLOCK_TYPE_KEY:
            // Check if inventory has the required key
            if (inventoryHasItemByKeyId(pInventory, pLock->_keyId))
            {
                doorLockUnlock(pLock);
                return 1;
            }
            break;
            
        case DOORLOCK_TYPE_MASTER_KEY:
            // Check if inventory has master key (keyId = 0)
            if (inventoryHasItemByKeyId(pInventory, 0))
            {
                doorLockUnlock(pLock);
                return 1;
            }
            break;
            
        case DOORLOCK_TYPE_CODE:
            // Code locks handled separately
            return 0;
            
        case DOORLOCK_TYPE_SCRIPT:
            // Script locks handled by script system
            return 0;
    }
    
    return 0;
}

UBYTE doorLockTryUnlockWithCode(tDoorLock* pLock, UBYTE ubCode)
{
    if (!pLock)
        return 0;
    
    if (pLock->_type != DOORLOCK_TYPE_CODE)
        return 0;
    
    if (pLock->_state != DOORLOCK_STATE_LOCKED)
        return 0;
    
    if (pLock->_code == ubCode)
    {
        doorLockUnlock(pLock);
        return 1;
    }
    
    return 0;
}

void doorLockLock(tDoorLock* pLock)
{
    if (pLock && pLock->_state != DOORLOCK_STATE_BROKEN)
    {
        pLock->_state = DOORLOCK_STATE_LOCKED;
    }
}

void doorLockUnlock(tDoorLock* pLock)
{
    if (pLock)
    {
        pLock->_state = DOORLOCK_STATE_UNLOCKED;
    }
}

tDoorLock* doorLockFindAt(tDoorLockList* pList, UBYTE x, UBYTE y)
{
    if (!pList)
        return NULL;
    
    tDoorLock* pLock = pList->_locks;
    while (pLock)
    {
        if (pLock->_doorX == x && pLock->_doorY == y)
        {
            return pLock;
        }
        pLock = pLock->_next;
    }
    return NULL;
}

