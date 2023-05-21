#pragma once
#include "Maze.h"
#include "wallset.h"
#include "character.h"
#include "script.h"

#include <ace/managers/state.h>



typedef struct _tGameState
{
    char *m_pszMazeName;
    char *m_pszWallsetName;
    UBYTE m_bIsMazeLoaded;
    UBYTE m_bIsWallsetLoaded;

    UBYTE m_bGlobalFlags[256];
    UBYTE m_bLocalFlags[256];
    tMaze *m_pCurrentMaze;
    tWallset *m_pCurrentWallset;
    tCharacterParty *m_pCurrentParty;    
} tGameState;

extern tGameState *g_pGameState;

UBYTE LoadGameState(const char* fileName);
UBYTE SaveGameState(const char* fileName);
void FreeGameState();

UBYTE InitNewGame();
UBYTE LoadLevel(BYTE level);