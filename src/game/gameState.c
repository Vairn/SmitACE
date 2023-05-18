#include "gameState.h"

tMaze *g_pCurrentMaze = NULL;
tWallset *g_pCurrentWallset = NULL;
tCharacterParty *g_pCurrentParty = NULL;
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
    if (g_pCurrentMaze)
    {
        mazeDelete(g_pCurrentMaze);
        g_pCurrentMaze = NULL;
    }
    if (g_pCurrentWallset)
    {
        wallsetDelete(g_pCurrentWallset);
        g_pCurrentWallset = NULL;
    }
    if (g_pCurrentParty)
    {
        partyDelete(g_pCurrentParty);
        g_pCurrentParty = NULL;
    }
    free(g_pGameState);
    g_pGameState = NULL;
    
}

UBYTE InitNewGame()
{
    // Failed to Init New Game
    return 0;
}


UBYTE LoadLevel(BYTE level)
{
    // Failed to Load Level
    return 0;
}

