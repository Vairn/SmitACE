#pragma once
#include "Maze.h"
#include "wallset.h"
#include "character.h"
#include "script.h"
#include <ace/managers/state.h>

extern tMaze *g_pCurrentMaze;
extern tWallset *g_pCurrentWallset;
extern tCharacterParty *g_pCurrentParty;

typedef struct _tGameState
{
    char *m_pszMazeName;
    char *m_pszWallsetName;
    UBYTE m_bIsMazeLoaded;
    UBYTE m_bIsWallsetLoaded;

    UBYTE m_bGlobalFlags[256];
    UBYTE m_bLocalFlags[256];
    
} tGameState;

extern tGameState *g_pGameState;

UBYTE LoadGameState(const char* fileName);
UBYTE SaveGameState(const char* fileName);
void FreeGameState();

UBYTE InitNewGame();
UBYTE LoadLevel(BYTE level);