#include "GameState.h"
#include "character.h"
#include "Renderer.h"
#include "inventory.h"
#include "wallbutton.h"
#include "doorbutton.h"
#include "doorlock.h"
#include "item.h"
#include "maze.h"
#include "wallset.h"
#include "ground_item.h"
#include "pressure_plate.h"
#include "game_manifest.h"
#include "level_entities.h"
#include "monster.h"
#include <ace/managers/memory.h>
#include <ace/utils/file.h>
#include <ace/utils/disk_file.h>

tGameState *g_pGameState = NULL;
UBYTE g_ubGameStateLoadedFromFile = 0;
UBYTE g_ubRequestWin = 0;
UBYTE g_ubRequestLevel = 0;
UBYTE g_ubRequestLevelId = 0;

#define SAVE_VERSION 3
#define SAVE_VERSION_LEGACY 2
#define SAVE_VERSION_LEGACY_1 1

/** Every hero record is this wide, including empty slots, so offsets never shift. */
#define SAVE_CHAR_RECORD_SIZE 28

/** Little-endian helpers: saves stay byte-identical between Amiga and host. */
static void fileWriteWord(tFile* pFile, UWORD uwValue)
{
    UBYTE a[2];
    a[0] = (UBYTE)(uwValue & 0xFF);
    a[1] = (UBYTE)(uwValue >> 8);
    fileWriteBytes(pFile, a, 2);
}

static UWORD fileReadWord(tFile* pFile)
{
    UBYTE a[2] = {0, 0};
    fileReadBytes(pFile, a, 2);
    return (UWORD)(a[0] | ((UWORD)a[1] << 8));
}

/** Write one hero record. Fixed-width so older saves stay readable. */
static void saveCharacter(tFile* pFile, const tCharacter* pChar)
{
    UBYTE ubAlive = 1;
    fileWriteBytes(pFile, &ubAlive, 1);
    fileWriteBytes(pFile, &pChar->_Level, 1);
    fileWriteWord(pFile, pChar->_Experience);
    fileWriteWord(pFile, pChar->_HP);
    fileWriteWord(pFile, pChar->_MaxHP);
    fileWriteBytes(pFile, &pChar->_Attack, 1);
    fileWriteBytes(pFile, &pChar->_Defense, 1);
    fileWriteBytes(pFile, &pChar->_Class, 1);
    fileWriteBytes(pFile, &pChar->_Race, 1);
    fileWriteBytes(pFile, (UBYTE*)pChar->_Name, 16);
}

/**
 * Read one hero record. Always consumes SAVE_CHAR_RECORD_SIZE bytes so the
 * next record starts where the writer put it.
 * @return 0 when the stream ended early.
 */
static UBYTE loadCharacter(tFile* pFile, tCharacter** ppOut)
{
    UBYTE ubAlive = 0;
    *ppOut = NULL;
    if (fileReadBytes(pFile, &ubAlive, 1) != 1)
        return 0;
    if (!ubAlive) {
        UBYTE aSkip[SAVE_CHAR_RECORD_SIZE - 1];
        return fileReadBytes(pFile, aSkip, SAVE_CHAR_RECORD_SIZE - 1) ? 1 : 0;
    }
    tCharacter* pChar = characterCreate();
    if (!pChar)
        return 0;
    fileReadBytes(pFile, &pChar->_Level, 1);
    pChar->_Experience = fileReadWord(pFile);
    pChar->_HP = fileReadWord(pFile);
    pChar->_MaxHP = fileReadWord(pFile);
    fileReadBytes(pFile, &pChar->_Attack, 1);
    fileReadBytes(pFile, &pChar->_Defense, 1);
    fileReadBytes(pFile, &pChar->_Class, 1);
    fileReadBytes(pFile, &pChar->_Race, 1);
    fileReadBytes(pFile, (UBYTE*)pChar->_Name, 16);
    pChar->_Name[15] = '\0';
    *ppOut = pChar;
    return 1;
}

UBYTE LoadGameState(const char* fileName)
{
    tFile* pFile = diskFileOpen(fileName, DISK_FILE_MODE_READ, 1);
    if (!pFile) return 0;
    UBYTE ver = 0;
    fileReadBytes(pFile, &ver, 1);
    if (ver != SAVE_VERSION && ver != SAVE_VERSION_LEGACY && ver != SAVE_VERSION_LEGACY_1) {
        fileClose(pFile);
        return 0;
    }
    UBYTE levelId = 0;
    UBYTE partyX = 0, partyY = 0, partyFacing = 0, battery = 0;
    fileReadBytes(pFile, &levelId, 1);
    fileReadBytes(pFile, &partyX, 1);
    fileReadBytes(pFile, &partyY, 1);
    fileReadBytes(pFile, &partyFacing, 1);
    fileReadBytes(pFile, &battery, 1);
    UBYTE invCount = 0;
    fileReadBytes(pFile, &invCount, 1);
    if (invCount > INVENTORY_MAX_ITEMS) invCount = INVENTORY_MAX_ITEMS;
    g_pGameState = (tGameState*)memAllocFastClear(sizeof(tGameState));
    g_pGameState->m_pCurrentParty = characterPartyCreate();
    if (!g_pGameState->m_pCurrentParty) {
        fileClose(pFile);
        memFree(g_pGameState, sizeof(tGameState));
        g_pGameState = NULL;
        return 0;
    }
    g_pGameState->m_pCurrentParty->_PartyX = partyX;
    g_pGameState->m_pCurrentParty->_PartyY = partyY;
    g_pGameState->m_pCurrentParty->_PartyFacing = partyFacing;
    g_pGameState->m_pCurrentParty->_BatteryLevel = battery;
    g_pGameState->m_pMonsterList = monsterListCreate();
    g_pGameState->m_pInventory = inventoryCreate();
    wallButtonListCreate(&g_pGameState->m_wallButtons);
    doorButtonListCreate(&g_pGameState->m_doorButtons);
    doorLockListCreate(&g_pGameState->m_doorLocks);
    gameManifestEnsureLoaded("data/game.smt");
    loadItems((char *)gameManifestGet()->itemsPath);
    monsterTableLoad((char *)gameManifestGet()->monstersPath);
    for (UBYTE i = 0; i < invCount; i++) {
        UBYTE itemIdx = 0, qty = 0;
        fileReadBytes(pFile, &itemIdx, 1);
        fileReadBytes(pFile, &qty, 1);
        inventoryAddItem(g_pGameState->m_pInventory, itemIdx, qty);
    }
    fileReadBytes(pFile, g_pGameState->m_bGlobalFlags, 256);
    /* Local flags appeared in save v2; keep reading them for v2 and v3. */
    if (ver >= SAVE_VERSION_LEGACY)
        fileReadBytes(pFile, g_pGameState->m_bLocalFlags, 256);

    if (ver >= SAVE_VERSION) {
        /* Party roster appended in v3 so v1/v2 saves still load. */
        UBYTE ubPartyCount = 0;
        if (fileReadBytes(pFile, &ubPartyCount, 1) == 1) {
            if (ubPartyCount > CHARACTER_PARTY_MAX)
                ubPartyCount = CHARACTER_PARTY_MAX;
            /* Every slot is written, living or empty, to keep the layout fixed. */
            for (UBYTE i = 0; i < CHARACTER_PARTY_MAX; i++) {
                tCharacter* pChar = NULL;
                if (!loadCharacter(pFile, &pChar))
                    break;
                if (pChar)
                    characterPartyAdd(g_pGameState->m_pCurrentParty, pChar);
            }
        }
    }
    fileClose(pFile);

    characterPartyEnsureDefaultHero(g_pGameState->m_pCurrentParty);

    if (!LoadLevel((BYTE)levelId)) {
        FreeGameState();
        return 0;
    }
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
    UBYTE levelId = g_pGameState->m_ubCurrentLevel;
    fileWriteBytes(pFile, &ver, 1);
    fileWriteBytes(pFile, &levelId, 1);
    fileWriteBytes(pFile, &g_pGameState->m_pCurrentParty->_PartyX, 1);
    fileWriteBytes(pFile, &g_pGameState->m_pCurrentParty->_PartyY, 1);
    fileWriteBytes(pFile, &g_pGameState->m_pCurrentParty->_PartyFacing, 1);
    fileWriteBytes(pFile, &g_pGameState->m_pCurrentParty->_BatteryLevel, 1);
    UBYTE invCount = g_pGameState->m_pInventory->ubItemCount;
    if (invCount > INVENTORY_MAX_ITEMS) invCount = INVENTORY_MAX_ITEMS;
    fileWriteBytes(pFile, &invCount, 1);
    for (UBYTE i = 0; i < invCount; i++) {
        fileWriteBytes(pFile, &g_pGameState->m_pInventory->pItems[i], 1);
        fileWriteBytes(pFile, &g_pGameState->m_pInventory->pItemQuantities[i], 1);
    }
    fileWriteBytes(pFile, g_pGameState->m_bGlobalFlags, 256);
    fileWriteBytes(pFile, g_pGameState->m_bLocalFlags, 256);

    /* v3: party roster so levelled heroes survive a reload. */
    UBYTE ubPartyCount = g_pGameState->m_pCurrentParty->_numCharacters;
    if (ubPartyCount > CHARACTER_PARTY_MAX)
        ubPartyCount = CHARACTER_PARTY_MAX;
    fileWriteBytes(pFile, &ubPartyCount, 1);
    for (UBYTE i = 0; i < CHARACTER_PARTY_MAX; i++) {
        if (i < g_pGameState->m_pCurrentParty->_numCharacters &&
            g_pGameState->m_pCurrentParty->_characters[i]) {
            saveCharacter(pFile, g_pGameState->m_pCurrentParty->_characters[i]);
        } else {
            /* Empty record: same SAVE_CHAR_RECORD_SIZE bytes so offsets stay fixed. */
            UBYTE ubZero = 0;
            for (UBYTE p = 0; p < SAVE_CHAR_RECORD_SIZE; p++)
                fileWriteBytes(pFile, &ubZero, 1);
        }
    }
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
    if (!g_pGameState->m_pCurrentParty) {
        memFree(g_pGameState, sizeof(tGameState));
        g_pGameState = NULL;
        return 0;
    }
    g_pGameState->m_pCurrentParty->_BatteryLevel = 100;
    g_pGameState->m_pMonsterList = monsterListCreate();
    g_pGameState->m_pInventory = inventoryCreate();
    
    // Initialize button and lock lists
    wallButtonListCreate(&g_pGameState->m_wallButtons);
    doorButtonListCreate(&g_pGameState->m_doorButtons);
    doorLockListCreate(&g_pGameState->m_doorLocks);
    
    gameManifestEnsureLoaded("data/game.smt");
    loadItems((char *)gameManifestGet()->itemsPath);
    monsterTableLoad((char *)gameManifestGet()->monstersPath);

    characterPartyEnsureDefaultHero(g_pGameState->m_pCurrentParty);

    return 1;
}

UBYTE LoadLevel(BYTE level)
{
    if (!g_pGameState) return 0;
    const tGameManifest *man = gameManifestGet();
    /* Monsters belong to the floor that spawned them. A level swap used to
     * keep the old list, so anything still alive followed the party down. */
    if (g_pGameState->m_pMonsterList) {
        monsterListDestroy(g_pGameState->m_pMonsterList);
        g_pGameState->m_pMonsterList = NULL;
    }
    g_pGameState->m_pMonsterList = monsterListCreate();
    if (!g_pGameState->m_pMonsterList)
        return 0;
    groundItemListClear(&g_pGameState->m_groundItems);
    pressurePlateListClear(&g_pGameState->m_pressurePlates);
    wallButtonListDestroy(&g_pGameState->m_wallButtons);
    doorButtonListDestroy(&g_pGameState->m_doorButtons);
    doorLockListDestroy(&g_pGameState->m_doorLocks);
    wallButtonListCreate(&g_pGameState->m_wallButtons);
    doorButtonListCreate(&g_pGameState->m_doorButtons);
    doorLockListCreate(&g_pGameState->m_doorLocks);
    if (g_pGameState->m_pCurrentMaze) {
        mazeDelete(g_pGameState->m_pCurrentMaze);
        g_pGameState->m_pCurrentMaze = NULL;
    }
    if (g_pGameState->m_pCurrentWallset) {
        wallsetDestroy(g_pGameState->m_pCurrentWallset);
        g_pGameState->m_pCurrentWallset = NULL;
    }

    UBYTE ul = (UBYTE)level;
    if (ul < man->levelCount) {
        const tGameLevelEntry *e = &man->levels[ul];
        if (ul == 0 && e->mazePath[0] == '\0')
            g_pGameState->m_pCurrentMaze = mazeCreateDemoData();
        else
            g_pGameState->m_pCurrentMaze = mazeLoad(e->mazePath);
        if (!g_pGameState->m_pCurrentMaze)
            return 0;
        const char *wsPath = e->wallsetPath[0] ? e->wallsetPath : "data/factory2/factory2.wll";
        g_pGameState->m_pCurrentWallset = wallsetLoad(wsPath);
        if (!g_pGameState->m_pCurrentWallset) {
            mazeDelete(g_pGameState->m_pCurrentMaze);
            g_pGameState->m_pCurrentMaze = NULL;
            return 0;
        }
        if (e->entitiesPath[0])
            levelEntitiesLoad(g_pGameState, e->entitiesPath);
        g_pGameState->m_ubCurrentLevel = ul;
        return 1;
    }

    {
        char path[64];
        path[0] = 'd'; path[1] = 'a'; path[2] = 't'; path[3] = 'a'; path[4] = '/';
        path[5] = 'l'; path[6] = 'e'; path[7] = 'v'; path[8] = 'e'; path[9] = 'l';
        path[10] = '0' + (level / 10); path[11] = '0' + (level % 10);
        path[12] = '.'; path[13] = 'm'; path[14] = 'a'; path[15] = 'z'; path[16] = 'e'; path[17] = '\0';
        g_pGameState->m_pCurrentMaze = mazeLoad(path);
        if (g_pGameState->m_pCurrentMaze) {
            g_pGameState->m_pCurrentWallset = wallsetLoad("data/factory2/factory2.wll");
            if (!g_pGameState->m_pCurrentWallset) {
                mazeDelete(g_pGameState->m_pCurrentMaze);
                g_pGameState->m_pCurrentMaze = NULL;
                return 0;
            }
            g_pGameState->m_ubCurrentLevel = ul;
        }
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
