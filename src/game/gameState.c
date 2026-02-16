#include "GameState.h"
#include "Renderer.h"
#include "inventory.h"
#include "wallbutton.h"
#include "doorbutton.h"
#include "doorlock.h"
#include "item.h"
#include "maze.h"
#include "wallset.h"
#include <ace/managers/memory.h>
#include <ace/utils/file.h>
#include <ace/utils/disk_file.h>

tGameState *g_pGameState = NULL;
UBYTE g_ubGameStateLoadedFromFile = 0;
UBYTE g_ubRequestWin = 0;

#define SAVE_VERSION 1

UBYTE LoadGameState(const char* fileName)
{
    tFile* pFile = diskFileOpen(fileName, DISK_FILE_MODE_READ, 1);
    if (!pFile) return 0;
    UBYTE ver = 0;
    fileRead(pFile, &ver, 1);
    if (ver != SAVE_VERSION) { fileClose(pFile); return 0; }
    UBYTE levelId = 0;
    UBYTE partyX = 0, partyY = 0, partyFacing = 0, battery = 0;
    fileRead(pFile, &levelId, 1);
    fileRead(pFile, &partyX, 1);
    fileRead(pFile, &partyY, 1);
    fileRead(pFile, &partyFacing, 1);
    fileRead(pFile, &battery, 1);
    UBYTE invCount = 0;
    fileRead(pFile, &invCount, 1);
    if (invCount > INVENTORY_MAX_ITEMS) invCount = INVENTORY_MAX_ITEMS;
    g_pGameState = (tGameState*)memAllocFastClear(sizeof(tGameState));
    g_pGameState->m_pCurrentParty = characterPartyCreate();
    g_pGameState->m_pCurrentParty->_PartyX = partyX;
    g_pGameState->m_pCurrentParty->_PartyY = partyY;
    g_pGameState->m_pCurrentParty->_PartyFacing = partyFacing;
    g_pGameState->m_pCurrentParty->_BatteryLevel = battery;
    g_pGameState->m_pMonsterList = monsterListCreate();
    g_pGameState->m_pInventory = inventoryCreate();
    wallButtonListCreate(&g_pGameState->m_wallButtons);
    doorButtonListCreate(&g_pGameState->m_doorButtons);
    doorLockListCreate(&g_pGameState->m_doorLocks);
    loadItems("data/items.dat");
    for (UBYTE i = 0; i < invCount; i++) {
        UBYTE itemIdx = 0, qty = 0;
        fileRead(pFile, &itemIdx, 1);
        fileRead(pFile, &qty, 1);
        inventoryAddItem(g_pGameState->m_pInventory, itemIdx, qty);
    }
    fileRead(pFile, g_pGameState->m_bGlobalFlags, 256);
    fileClose(pFile);
    if (!LoadLevel((BYTE)levelId)) {
        FreeGameState();
        return 0;
    }
    if (levelId == 0)
        g_pGameState->m_pCurrentWallset = wallsetLoad("data/factory2/factory2.wll");
    g_ubGameStateLoadedFromFile = 1;
    return 1;
}

UBYTE SaveGameState(const char* fileName)
{
    if (!g_pGameState || !g_pGameState->m_pCurrentParty || !g_pGameState->m_pInventory)
        return 0;
    tFile* pFile = diskFileOpen(fileName, DISK_FILE_MODE_WRITE, 1);
    if (!pFile) return 0;
    UBYTE ver = SAVE_VERSION;
    UBYTE levelId = 0;
    fileWrite(pFile, &ver, 1);
    fileWrite(pFile, &levelId, 1);
    fileWrite(pFile, &g_pGameState->m_pCurrentParty->_PartyX, 1);
    fileWrite(pFile, &g_pGameState->m_pCurrentParty->_PartyY, 1);
    fileWrite(pFile, &g_pGameState->m_pCurrentParty->_PartyFacing, 1);
    fileWrite(pFile, &g_pGameState->m_pCurrentParty->_BatteryLevel, 1);
    UBYTE invCount = g_pGameState->m_pInventory->ubItemCount;
    if (invCount > INVENTORY_MAX_ITEMS) invCount = INVENTORY_MAX_ITEMS;
    fileWrite(pFile, &invCount, 1);
    for (UBYTE i = 0; i < invCount; i++) {
        fileWrite(pFile, &g_pGameState->m_pInventory->pItems[i], 1);
        fileWrite(pFile, &g_pGameState->m_pInventory->pItemQuantities[i], 1);
    }
    fileWrite(pFile, g_pGameState->m_bGlobalFlags, 256);
    fileClose(pFile);
    return 1;
}
void FreeGameState()
{
    if (!g_pGameState) return;
    if (g_pGameState->m_pCurrentMaze)
    {
        mazeDelete(g_pGameState->m_pCurrentMaze);
        g_pGameState->m_pCurrentMaze = NULL;
    }
    if (g_pGameState->m_pCurrentWallset)
    {
        wallsetDestroy(g_pGameState->m_pCurrentWallset);
        g_pGameState->m_pCurrentWallset = NULL;
    }
    if (g_pGameState->m_pCurrentParty)
    {
        characterPartyDestroy(g_pGameState->m_pCurrentParty);
        g_pGameState->m_pCurrentParty = NULL;
    }
    if (g_pGameState->m_pMonsterList)
    {
        monsterListDestroy(g_pGameState->m_pMonsterList);
        g_pGameState->m_pMonsterList = NULL;
    }
    if (g_pGameState->m_pInventory)
    {
        inventoryDestroy(g_pGameState->m_pInventory);
        g_pGameState->m_pInventory = NULL;
    }
    wallButtonListDestroy(&g_pGameState->m_wallButtons);
    doorButtonListDestroy(&g_pGameState->m_doorButtons);
    doorLockListDestroy(&g_pGameState->m_doorLocks);
    memFree(g_pGameState, sizeof(tGameState));
    g_pGameState = NULL;
}

UBYTE InitNewGame()
{
    preFillFacing(g_mazeDr);
    preFillPosDataNoFacing(g_mazePos);
    g_pGameState = (tGameState*)memAllocFastClear(sizeof(tGameState));
    g_pGameState->m_pCurrentParty = characterPartyCreate();
    g_pGameState->m_pCurrentParty->_BatteryLevel = 100;
    g_pGameState->m_pMonsterList = monsterListCreate();
    g_pGameState->m_pInventory = inventoryCreate();
    
    // Initialize button and lock lists
    wallButtonListCreate(&g_pGameState->m_wallButtons);
    doorButtonListCreate(&g_pGameState->m_doorButtons);
    doorLockListCreate(&g_pGameState->m_doorLocks);
    
    // Load items from data file (or use fallback items if file missing)
    loadItems("data/items.dat");
    
    return 1;
}

UBYTE LoadLevel(BYTE level)
{
    if (!g_pGameState) return 0;
    if (g_pGameState->m_pCurrentMaze) {
        mazeDelete(g_pGameState->m_pCurrentMaze);
        g_pGameState->m_pCurrentMaze = NULL;
    }
    if (g_pGameState->m_pCurrentWallset) {
        wallsetDestroy(g_pGameState->m_pCurrentWallset);
        g_pGameState->m_pCurrentWallset = NULL;
    }
    if (level == 0) {
        g_pGameState->m_pCurrentMaze = mazeCreateDemoData();
        return g_pGameState->m_pCurrentMaze ? 1 : 0;
    }
    {
        char path[64];
        path[0] = 'd'; path[1] = 'a'; path[2] = 't'; path[3] = 'a'; path[4] = '/';
        path[5] = 'l'; path[6] = 'e'; path[7] = 'v'; path[8] = 'e'; path[9] = 'l';
        path[10] = '0' + (level / 10); path[11] = '0' + (level % 10);
        path[12] = '.'; path[13] = 'm'; path[14] = 'a'; path[15] = 'z'; path[16] = 'e'; path[17] = '\0';
        g_pGameState->m_pCurrentMaze = mazeLoad(path);
        if (g_pGameState->m_pCurrentMaze)
            g_pGameState->m_pCurrentWallset = wallsetLoad("data/factory2/factory2.wll");
        return g_pGameState->m_pCurrentMaze ? 1 : 0;
    }
}

UBYTE mazeMove(tMaze* pMaze, tCharacterParty* pParty, UBYTE direction)
{
    UBYTE x = pParty->_PartyX;
    UBYTE y = pParty->_PartyY;

    switch (direction)
    {
    case 0:
        y--;
        break;
    case 1:
        x++;
        break;
    case 2:
        y++;
        break;
    case 3:
        x--;
        break;
    default:
        break;
    }

    if (x >= pMaze->_width || y >= pMaze->_height)
    {
        // Failed to Move
        return 0;
    }

    UWORD mazeOffset = (y)*pMaze->_width + (x);
    UBYTE cellType = pMaze->_mazeData[mazeOffset];
    
    // Handle different cell types
    switch (cellType)
    {
    case MAZE_FLOOR:
    case MAZE_DOOR_OPEN:
        // Allow movement through floors and open doors
        pParty->_PartyX = x;
        pParty->_PartyY = y;
        pParty->_BatteryLevel--;
        if (pParty->_BatteryLevel == 0)
        {
            // Battery Dead
            return 2;
        }
        // Moved
        return 1;
    case MAZE_WALL:
    case MAZE_DOOR:
    case MAZE_DOOR_LOCKED:
        // Can't move through walls or closed doors
        return 0;
    case MAZE_EVENT_TRIGGER:
        // Handle event trigger
        pParty->_PartyX = x;
        pParty->_PartyY = y;
        pParty->_BatteryLevel--;
        if (pParty->_BatteryLevel == 0)
        {
            // Battery Dead
            return 2;
        }
        return 3;  // Event triggered
    default:
        return 0;
    }
}
