#pragma once

#include "character.h"
#include "Maze.h"

// Forward declaration
struct _tInventory;
typedef struct _tInventory tInventory;

// Monster constants
#define MAX_MONSTERS 64

// Monster types
#define MONSTER_TYPE_NORMAL 0
#define MONSTER_TYPE_BOSS 1
#define MONSTER_TYPE_MINIBOSS 2

// Monster states
#define MONSTER_STATE_IDLE 0
#define MONSTER_STATE_AGGRESSIVE 1
#define MONSTER_STATE_FLEEING 2
#define MONSTER_STATE_DEAD 3

typedef struct _monster
{
    tCharacter _base;  // Base character stats
    UBYTE _monsterType;
    UBYTE _state;
    UBYTE _aggroRange;  // How far the monster can detect the player
    UBYTE _fleeThreshold;  // HP percentage at which monster will flee
    UBYTE _dropTable[8];  // Items that can be dropped
    UBYTE _dropChance[8]; // Chance for each item to drop
    UWORD _experienceValue; // XP given when defeated
    UBYTE _partyPosX;
    UBYTE _partyPosY;
} tMonster;

typedef struct _monsterList
{
    UBYTE _numMonsters;
    tMonster** _monsters;  // Dynamic list of monsters
} tMonsterList;

// Monster creation and management
tMonster* monsterCreate(UBYTE monsterType);
void monsterDestroy(tMonster* monster);
tMonsterList* monsterListCreate();
void monsterListDestroy(tMonsterList* monsterList);

// Monster behavior
void monsterUpdate(tMonster* monster, tCharacterParty* party);
void monsterAttack(tMonster* monster, tCharacter* target);
void monsterDropLoot(tMonster* monster, tInventory* pInventory);

// Monster placement
void monsterPlaceInMaze(tMaze* maze, tMonster* monster, UBYTE x, UBYTE y);
void monsterRemoveFromMaze(tMaze* maze, tMonster* monster); 