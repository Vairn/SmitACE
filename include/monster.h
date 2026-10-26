#pragma once

#include "character.h"
#include "maze.h"
#include <ace/utils/bitmap.h>

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
    /** 0 north, 1 east, 2 south, 3 west. Set when the monster steps. */
    UBYTE _facing;
    /** Seconds until next move attempt (world runs ~1 Hz; staggered at spawn). */
    UBYTE _moveCooldown;
} tMonster;

typedef struct _monsterList
{
    UBYTE _numMonsters;
    tMonster** _monsters;  // Dynamic list of monsters
} tMonsterList;

// Monster creation and management
/** Load monster stat table from data/monsters.dat (or path from game manifest). Safe to call repeatedly. */
void monsterTableLoad(const char *szPath);
void monsterTableClear(void);

/** Display name from monsters.dat, or "monster" when the type has none. */
const char *monsterNameForType(UBYTE typeId);

/**
 * One baked pose at one view distance.
 * Order: Front, Back, Left1, Left2, Right1, Right2, Attack1, Attack2.
 * Right is the mirror of Left, written at import. Depth 0 is yDelta 0, then -1, -2, -3.
 * Either pointer may be NULL when that picture was not loaded.
 */
void monsterGfxGet(UBYTE typeId, UBYTE frame, UBYTE depth, tBitMap **ppGfx, tBitMap **ppMask);
/** How many baked poses were loaded. 0 if this type has no pictures. */
UBYTE monsterGfxFrameCount(UBYTE typeId);
/**
 * Front, back, left, or right from where the party stands, or Attack1/Attack2 while attacking.
 * Facing is 0 north, 1 east, 2 south, 3 west.
 */
UBYTE monsterGfxPick(UBYTE typeId, UBYTE attacking, UBYTE monFacing, UBYTE partyFacing,
	UBYTE monX, UBYTE monY, UBYTE partyX, UBYTE partyY);
/** Step the shared pose clock. Call from the game loop. */
void monsterGfxAdvance(void);

tMonster* monsterCreate(UBYTE monsterType);
void monsterDestroy(tMonster* monster);
tMonsterList* monsterListCreate();
void monsterListDestroy(tMonsterList* monsterList);

// Monster behavior
/** @return non-zero if monster changed map cell this tick (caller may need to refresh the view). */
UBYTE monsterUpdate(tMonster* monster, tMaze* maze, tCharacterParty* party, tMonsterList* allMonsters);

/**
 * Monster strikes the target. Applies damage and leaves _HP at zero on death.
 * @return damage dealt (always at least 1).
 */
UWORD monsterAttack(tMonster* monster, tCharacter* target);

/**
 * Melee strike from party member (after monster's attack).
 * @return damage dealt (always at least 1).
 */
UWORD monsterTakeDamageFromCharacter(tMonster* monster, tCharacter* attacker);
void monsterDropLoot(tMonster* monster, tInventory* pInventory);

// Monster placement
void monsterPlaceInMaze(tMaze* maze, tMonster* monster, UBYTE x, UBYTE y);
void monsterRemoveFromMaze(tMaze* maze, tMonster* monster); 