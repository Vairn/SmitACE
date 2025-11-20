#pragma once
#include "Maze.h"
#include "wallset.h"
#include "character.h"
#include "script.h"
#include "monster.h"

#include <ace/managers/state.h>

// Define a simple stack for Gosub
#define SCRIPT_RETURN_STACK_SIZE 10
typedef struct {
    UWORD _stack[SCRIPT_RETURN_STACK_SIZE];
    UBYTE _top;
    BOOL _conditionMet; // Tracks if the most recent IF condition was true
    BOOL _skippingBlock; // Tracks if we are skipping lines due to a false IF or a true IF followed by ELSE
    UWORD _scriptProgramCounter; // Program counter for script execution
} tScriptState;


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
    tMonsterList *m_pMonsterList;  // List of monsters in current level
    tScriptState _scriptState;
} tGameState;

extern tGameState *g_pGameState;

UBYTE LoadGameState(const char* fileName);
UBYTE SaveGameState(const char* fileName);
void FreeGameState();

UBYTE InitNewGame();
UBYTE LoadLevel(BYTE level);

UBYTE mazeMove(tMaze* pMaze, tCharacterParty* pParty, UBYTE direction);
