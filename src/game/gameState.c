#include "gameState.h"
#include "Renderer.h"
#include <ace/managers/memory.h>
tGameState *g_pGameState= NULL;

UBYTE LoadGameState(const char* fileName)
{
    // Failed to Load Game State
    return 0;
}
UBYTE SaveGameState(const char* fileName)
{
    // failed to Save Game State
    return 0;
}
void FreeGameState()
{   
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
    memFree(g_pGameState, sizeof(tGameState));
    g_pGameState = NULL;
}

UBYTE InitNewGame()
{
    preFillFacing(g_mazeDr);
    preFillPosDataNoFacing(g_mazePos);
    g_pGameState = (tGameState*)memAllocFastClear(sizeof(tGameState));
    g_pGameState->m_pCurrentParty = characterPartyCreate();
    g_pGameState->m_pCurrentParty->_BatteryLevel = 1000;
    g_pGameState->m_pMonsterList = monsterListCreate();
    // Failed to Init New Game
    return 0;
}

UBYTE LoadLevel(BYTE level)
{
    // Failed to Load Level
    return 0;
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

    if (x < 0 || x >= pMaze->_width || y < 0 || y >= pMaze->_height)
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
