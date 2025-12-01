#pragma once

#include "maze.h"
#include <ace/types.h>

// Forward declaration
struct _tInventory;
typedef struct _tInventory tInventory;

// Lock states
#define DOORLOCK_STATE_UNLOCKED 0
#define DOORLOCK_STATE_LOCKED 1
#define DOORLOCK_STATE_BROKEN 2

// Lock types
#define DOORLOCK_TYPE_KEY 0          // Requires specific key
#define DOORLOCK_TYPE_MASTER_KEY 1   // Requires master key (keyId = 0)
#define DOORLOCK_TYPE_CODE 2         // Requires code/combination
#define DOORLOCK_TYPE_SCRIPT 3       // Requires script condition

typedef struct _doorLock
{
    UBYTE _doorX;           // X position of locked door
    UBYTE _doorY;            // Y position of locked door
    UBYTE _state;            // Current lock state
    UBYTE _type;             // Lock type
    UBYTE _keyId;            // Key ID required (0 = master key, or specific key)
    UBYTE _code;             // Code for code locks (0-255)
    UBYTE _pad[2];          // Padding for 68020+ alignment
    struct _doorLock* _next;
} tDoorLock;

typedef struct _doorLockList
{
    UBYTE _numLocks;
    tDoorLock* _locks;
} tDoorLockList;

// Door lock management
tDoorLock* doorLockCreate(UBYTE doorX, UBYTE doorY, UBYTE type, UBYTE keyId);
void doorLockDestroy(tDoorLock* pLock);
void doorLockListCreate(tDoorLockList* pList);
void doorLockListDestroy(tDoorLockList* pList);
void doorLockAdd(tDoorLockList* pList, tDoorLock* pLock);
void doorLockRemove(tDoorLockList* pList, tDoorLock* pLock);

// Door lock interaction
UBYTE doorLockTryUnlock(tDoorLock* pLock, tInventory* pInventory, UBYTE ubKeyId);
UBYTE doorLockTryUnlockWithCode(tDoorLock* pLock, UBYTE ubCode);
void doorLockLock(tDoorLock* pLock);
void doorLockUnlock(tDoorLock* pLock);
tDoorLock* doorLockFindAt(tDoorLockList* pList, UBYTE x, UBYTE y);

