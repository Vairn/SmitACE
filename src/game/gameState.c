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
    FreeMem(g_pGameState, sizeof(tGameState));
    g_pGameState = NULL;
    
}

UBYTE InitNewGame()
{
    preFillFacing(g_mazeDr);
    preFillPosDataNoFacing(g_mazePos);
    g_pGameState = (tGameState*)memAllocFastClear(sizeof(tGameState));
    g_pGameState->m_pCurrentParty = characterPartyCreate();

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

    pParty->_PartyX =x;
    pParty->_PartyY =y;
    // Failed to Move
    return 0;
}
