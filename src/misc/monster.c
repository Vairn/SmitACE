#include "monster.h"
#include "inventory.h"
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>
#include <ace/utils/disk_file.h>
#include <ace/utils/file.h>
#include <string.h>

#define MONSTER_DEF_MAX 64
/** Game ticks between monster move attempts (one cell when a move succeeds). */
#define MONSTER_MOVE_PERIOD 10

typedef struct {
	UBYTE ubLevel;
	UWORD uwMaxHP;
	UBYTE ubAttack;
	UBYTE ubDefense;
	UWORD uwExperience;
	UBYTE ubAggroRange;
	UBYTE ubFleeThreshold;
	UBYTE dropTable[8];
	UBYTE dropChance[8];
} tMonsterDef;

static tMonsterDef s_defs[MONSTER_DEF_MAX];
static UBYTE s_defCount;

// Simple random number generator (LCG)
static ULONG s_randSeed = 1;
static UBYTE simpleRand(void)
{
	s_randSeed = (s_randSeed * 1103515245 + 12345) & 0x7fffffff;
	return (UBYTE)(s_randSeed % 100);
}

static UBYTE simpleRandByte(void)
{
	s_randSeed = (s_randSeed * 1103515245 + 12345) & 0x7fffffff;
	return (UBYTE)s_randSeed;
}

static UBYTE monsterManhattan(UBYTE ax, UBYTE ay, UBYTE bx, UBYTE by)
{
	WORD dx = (WORD)ax - (WORD)bx;
	WORD dy = (WORD)ay - (WORD)by;
	if (dx < 0)
		dx = (WORD)-dx;
	if (dy < 0)
		dy = (WORD)-dy;
	return (UBYTE)(dx + dy);
}

static UBYTE monsterCellWalkable(const tMaze *maze, UBYTE x, UBYTE y)
{
	if (!maze || x >= maze->_width || y >= maze->_height)
		return 0;
	UBYTE c = maze->_mazeData[(UWORD)y * maze->_width + x];
	switch (c) {
	case MAZE_FLOOR:
	case MAZE_DOOR_OPEN:
	case MAZE_EVENT_TRIGGER:
		return 1;
	default:
		return 0;
	}
}

static UBYTE monsterCellBlockedByOther(const tMonsterList *list, UBYTE x, UBYTE y,
	const tMonster *self)
{
	if (!list)
		return 0;
	for (UBYTE i = 0; i < list->_numMonsters; ++i) {
		tMonster *o = list->_monsters[i];
		if (!o || o == self || o->_state == MONSTER_STATE_DEAD)
			continue;
		if (o->_partyPosX == x && o->_partyPosY == y)
			return 1;
	}
	return 0;
}

/** One step in map space; party tile is allowed (for melee). */
static UBYTE monsterTryStepDir(tMaze *maze, tMonster *self, const tMonsterList *list,
	BYTE sdx, BYTE sdy)
{
	WORD nx = (WORD)self->_partyPosX + (WORD)sdx;
	WORD ny = (WORD)self->_partyPosY + (WORD)sdy;
	if (nx < 0 || ny < 0)
		return 0;
	if (nx >= (WORD)maze->_width || ny >= (WORD)maze->_height)
		return 0;
	UBYTE ux = (UBYTE)nx;
	UBYTE uy = (UBYTE)ny;
	if (!monsterCellWalkable(maze, ux, uy))
		return 0;
	if (monsterCellBlockedByOther(list, ux, uy, self))
		return 0;
	self->_partyPosX = ux;
	self->_partyPosY = uy;
	return 1;
}

static void monsterShuffle4(UBYTE *order)
{
	for (UBYTE i = 3; i > 0; --i) {
		UBYTE j = (UBYTE)(simpleRandByte() % (i + 1));
		UBYTE t = order[i];
		order[i] = order[j];
		order[j] = t;
	}
}

static void monsterWander(tMaze *maze, tMonster *self, const tMonsterList *list)
{
	static const BYTE s_dx[4] = {0, 1, 0, -1};
	static const BYTE s_dy[4] = {-1, 0, 1, 0};
	UBYTE order[4] = {0, 1, 2, 3};
	monsterShuffle4(order);
	for (UBYTE k = 0; k < 4; ++k) {
		UBYTE oi = order[k];
		if (monsterTryStepDir(maze, self, list, s_dx[oi], s_dy[oi]))
			return;
	}
}

static void monsterMoveChase(tMaze *maze, tMonster *self, const tMonsterList *list,
	tCharacterParty *party)
{
	WORD mdx = (WORD)party->_PartyX - (WORD)self->_partyPosX;
	WORD mdy = (WORD)party->_PartyY - (WORD)self->_partyPosY;
	if (mdx == 0 && mdy == 0)
		return;

	WORD adx = mdx < 0 ? (WORD)-mdx : mdx;
	WORD ady = mdy < 0 ? (WORD)-mdy : mdy;
	if (adx >= ady) {
		if (mdx != 0 && monsterTryStepDir(maze, self, list, (BYTE)(mdx > 0 ? 1 : -1), 0))
			return;
		if (mdy != 0 && monsterTryStepDir(maze, self, list, 0, (BYTE)(mdy > 0 ? 1 : -1)))
			return;
	} else {
		if (mdy != 0 && monsterTryStepDir(maze, self, list, 0, (BYTE)(mdy > 0 ? 1 : -1)))
			return;
		if (mdx != 0 && monsterTryStepDir(maze, self, list, (BYTE)(mdx > 0 ? 1 : -1), 0))
			return;
	}

	UBYTE start = monsterManhattan(self->_partyPosX, self->_partyPosY,
		party->_PartyX, party->_PartyY);
	static const BYTE s_dx[4] = {0, 1, 0, -1};
	static const BYTE s_dy[4] = {-1, 0, 1, 0};
	UBYTE order[4] = {0, 1, 2, 3};
	monsterShuffle4(order);
	for (UBYTE k = 0; k < 4; ++k) {
		UBYTE oi = order[k];
		UBYTE sx = self->_partyPosX;
		UBYTE sy = self->_partyPosY;
		if (!monsterTryStepDir(maze, self, list, s_dx[oi], s_dy[oi]))
			continue;
		if (monsterManhattan(self->_partyPosX, self->_partyPosY,
				party->_PartyX, party->_PartyY) < start)
			return;
		self->_partyPosX = sx;
		self->_partyPosY = sy;
	}
	monsterWander(maze, self, list);
}

static void monsterMoveFlee(tMaze *maze, tMonster *self, const tMonsterList *list,
	tCharacterParty *party)
{
	UBYTE start = monsterManhattan(self->_partyPosX, self->_partyPosY,
		party->_PartyX, party->_PartyY);
	static const BYTE s_dx[4] = {0, 1, 0, -1};
	static const BYTE s_dy[4] = {-1, 0, 1, 0};
	UBYTE order[4] = {0, 1, 2, 3};
	monsterShuffle4(order);
	for (UBYTE k = 0; k < 4; ++k) {
		UBYTE oi = order[k];
		UBYTE sx = self->_partyPosX;
		UBYTE sy = self->_partyPosY;
		if (!monsterTryStepDir(maze, self, list, s_dx[oi], s_dy[oi]))
			continue;
		if (monsterManhattan(self->_partyPosX, self->_partyPosY,
				party->_PartyX, party->_PartyY) > start)
			return;
		self->_partyPosX = sx;
		self->_partyPosY = sy;
	}
	monsterWander(maze, self, list);
}

static UWORD readU16Be(tFile *f)
{
	UBYTE b[2];
	fileRead(f, b, 2);
	return ((UWORD)b[0] << 8) | (UWORD)b[1];
}

static void monsterApplyDef(tMonster *m, UBYTE typeId, const tMonsterDef *d)
{
	m->_monsterType = typeId;
	m->_state = MONSTER_STATE_IDLE;
	m->_base._Level = d->ubLevel;
	m->_base._MaxHP = d->uwMaxHP;
	m->_base._HP = d->uwMaxHP;
	m->_base._Attack = d->ubAttack;
	m->_base._Defense = d->ubDefense;
	m->_experienceValue = d->uwExperience;
	m->_aggroRange = d->ubAggroRange;
	m->_fleeThreshold = d->ubFleeThreshold;
	for (int i = 0; i < 8; i++) {
		m->_dropTable[i] = d->dropTable[i];
		m->_dropChance[i] = d->dropChance[i];
	}
}

static void monsterTableSetBuiltins(void)
{
	s_defCount = 3;
	memset(s_defs, 0, sizeof(s_defs));
	s_defs[0].ubLevel = 1;
	s_defs[0].uwMaxHP = 20;
	s_defs[0].ubAttack = 5;
	s_defs[0].ubDefense = 3;
	s_defs[0].uwExperience = 10;
	s_defs[0].ubAggroRange = 5;
	s_defs[0].ubFleeThreshold = 20;

	s_defs[1].ubLevel = 5;
	s_defs[1].uwMaxHP = 100;
	s_defs[1].ubAttack = 15;
	s_defs[1].ubDefense = 12;
	s_defs[1].uwExperience = 200;
	s_defs[1].ubAggroRange = 5;
	s_defs[1].ubFleeThreshold = 20;

	s_defs[2].ubLevel = 3;
	s_defs[2].uwMaxHP = 50;
	s_defs[2].ubAttack = 10;
	s_defs[2].ubDefense = 8;
	s_defs[2].uwExperience = 50;
	s_defs[2].ubAggroRange = 5;
	s_defs[2].ubFleeThreshold = 20;
}

void monsterTableClear(void)
{
	s_defCount = 0;
	memset(s_defs, 0, sizeof(s_defs));
}

void monsterTableLoad(const char *szPath)
{
	monsterTableClear();
	if (!szPath || !szPath[0]) {
		monsterTableSetBuiltins();
		return;
	}
	tFile *f = diskFileOpen(szPath, DISK_FILE_MODE_READ, 1);
	if (!f) {
		logWrite("monsters: '%s' not found, using built-in table\n", szPath);
		monsterTableSetBuiltins();
		return;
	}
	char magic[4];
	fileRead(f, magic, 4);
	if (magic[0] != 'M' || magic[1] != 'O' || magic[2] != 'N' || magic[3] != 'S') {
		fileClose(f);
		monsterTableSetBuiltins();
		return;
	}
	UBYTE ver = 0;
	fileRead(f, &ver, 1);
	if (ver != 1) {
		fileClose(f);
		monsterTableSetBuiltins();
		return;
	}
	UBYTE count = 0;
	fileRead(f, &count, 1);
	if (count == 0 || count > MONSTER_DEF_MAX) {
		fileClose(f);
		monsterTableSetBuiltins();
		return;
	}
	for (UBYTE i = 0; i < count; i++) {
		tMonsterDef *d = &s_defs[i];
		fileRead(f, &d->ubLevel, 1);
		d->uwMaxHP = readU16Be(f);
		d->ubAttack = 0;
		d->ubDefense = 0;
		fileRead(f, &d->ubAttack, 1);
		fileRead(f, &d->ubDefense, 1);
		d->uwExperience = readU16Be(f);
		fileRead(f, &d->ubAggroRange, 1);
		fileRead(f, &d->ubFleeThreshold, 1);
		fileRead(f, d->dropTable, 8);
		fileRead(f, d->dropChance, 8);
		UBYTE nlen = 0;
		fileRead(f, &nlen, 1);
		for (UBYTE k = 0; k < nlen; k++) {
			UBYTE b;
			fileRead(f, &b, 1);
		}
	}
	s_defCount = count;
	fileClose(f);
	logWrite("monsters: loaded %u from %s\n", (unsigned)s_defCount, szPath);
}

static void monsterCreateLegacySwitch(tMonster *monster, UBYTE monsterType)
{
	monster->_monsterType = monsterType;
	monster->_state = MONSTER_STATE_IDLE;
	monster->_aggroRange = 5;
	monster->_fleeThreshold = 20;
	memset(monster->_dropTable, 0, 8);
	memset(monster->_dropChance, 0, 8);
	switch (monsterType)
	{
	case MONSTER_TYPE_NORMAL:
		monster->_base._Level = 1;
		monster->_base._MaxHP = 20;
		monster->_base._HP = monster->_base._MaxHP;
		monster->_base._Attack = 5;
		monster->_base._Defense = 3;
		monster->_experienceValue = 10;
		break;
	case MONSTER_TYPE_MINIBOSS:
		monster->_base._Level = 3;
		monster->_base._MaxHP = 50;
		monster->_base._HP = monster->_base._MaxHP;
		monster->_base._Attack = 10;
		monster->_base._Defense = 8;
		monster->_experienceValue = 50;
		break;
	case MONSTER_TYPE_BOSS:
		monster->_base._Level = 5;
		monster->_base._MaxHP = 100;
		monster->_base._HP = monster->_base._MaxHP;
		monster->_base._Attack = 15;
		monster->_base._Defense = 12;
		monster->_experienceValue = 200;
		break;
	default:
		monster->_base._Level = 1;
		monster->_base._MaxHP = 20;
		monster->_base._HP = 20;
		monster->_base._Attack = 5;
		monster->_base._Defense = 3;
		monster->_experienceValue = 10;
		break;
	}
}

tMonster* monsterCreate(UBYTE monsterType)
{
	tMonster* monster = (tMonster*)memAllocFastClear(sizeof(tMonster));
	if (!monster)
		return NULL;
	if (s_defCount > 0 && monsterType < s_defCount) {
		monsterApplyDef(monster, monsterType, &s_defs[monsterType]);
		monster->_moveCooldown = (UBYTE)(1 + (simpleRandByte() % 24));
		logWrite("monsterCreate: type=%u (table) hp=%u/%u\n", (unsigned)monsterType,
			(unsigned)monster->_base._HP, (unsigned)monster->_base._MaxHP);
		return monster;
	}
	monsterCreateLegacySwitch(monster, monsterType);
	monster->_moveCooldown = (UBYTE)(1 + (simpleRandByte() % 24));
	logWrite("monsterCreate: type=%u (legacy) hp=%u/%u\n", (unsigned)monsterType,
		(unsigned)monster->_base._HP, (unsigned)monster->_base._MaxHP);
	return monster;
}

void monsterDestroy(tMonster* monster)
{
    if (monster)
    {
        memFree(monster, sizeof(tMonster));
    }
}

tMonsterList* monsterListCreate()
{
    tMonsterList* list = (tMonsterList*)memAllocFastClear(sizeof(tMonsterList));
    if (list)
    {
        list->_monsters = (tMonster**)memAllocFastClear(sizeof(tMonster*) * MAX_MONSTERS);
    }
    return list;
}

void monsterListDestroy(tMonsterList* list)
{
    if (list)
    {
        for (UBYTE i = 0; i < list->_numMonsters; i++)
        {
            monsterDestroy(list->_monsters[i]);
        }
        memFree(list->_monsters, sizeof(tMonster*) * MAX_MONSTERS);
        memFree(list, sizeof(tMonsterList));
    }
}

void monsterUpdate(tMonster *monster, tMaze *maze, tCharacterParty *party, tMonsterList *allMonsters)
{
	if (!monster || monster->_state == MONSTER_STATE_DEAD)
		return;
	if (!party)
		return;

	if (monster->_moveCooldown > 0)
		monster->_moveCooldown--;

	WORD dx = (WORD)monster->_partyPosX - (WORD)party->_PartyX;
	WORD dy = (WORD)monster->_partyPosY - (WORD)party->_PartyY;
	WORD adx = dx < 0 ? (WORD)(-dx) : dx;
	WORD ady = dy < 0 ? (WORD)(-dy) : dy;
	UBYTE distance = (UBYTE)(adx + ady);

	if (monster->_state == MONSTER_STATE_IDLE) {
		if (distance <= monster->_aggroRange)
			monster->_state = MONSTER_STATE_AGGRESSIVE;
	} else if (monster->_state == MONSTER_STATE_AGGRESSIVE) {
		UBYTE hpPercent = (monster->_base._HP * 100) / monster->_base._MaxHP;
		if (hpPercent <= monster->_fleeThreshold)
			monster->_state = MONSTER_STATE_FLEEING;
		else if (distance > monster->_aggroRange)
			monster->_state = MONSTER_STATE_IDLE;
	} else if (monster->_state == MONSTER_STATE_FLEEING) {
		if (distance > monster->_aggroRange)
			monster->_state = MONSTER_STATE_IDLE;
	}

	if (maze && allMonsters && monster->_moveCooldown == 0) {
		monster->_moveCooldown = MONSTER_MOVE_PERIOD;
		switch (monster->_state) {
		case MONSTER_STATE_IDLE:
			if (distance > monster->_aggroRange)
				monsterWander(maze, monster, allMonsters);
			break;
		case MONSTER_STATE_AGGRESSIVE:
			monsterMoveChase(maze, monster, allMonsters, party);
			break;
		case MONSTER_STATE_FLEEING:
			monsterMoveFlee(maze, monster, allMonsters, party);
			break;
		default:
			break;
		}
	}

	logWrite(
		"monster tick: type=%u pos=(%u,%u) party=(%u,%u) dist=%u state=%u hp=%u/%u aggro=%u\n",
		(unsigned)monster->_monsterType, (unsigned)monster->_partyPosX,
		(unsigned)monster->_partyPosY, (unsigned)party->_PartyX, (unsigned)party->_PartyY,
		(unsigned)distance, (unsigned)monster->_state, (unsigned)monster->_base._HP,
		(unsigned)monster->_base._MaxHP, (unsigned)monster->_aggroRange);
}

void monsterAttack(tMonster* monster, tCharacter* target)
{
    if (!monster || !target || monster->_state == MONSTER_STATE_DEAD)
        return;

    // Simple attack calculation
    UWORD damage = monster->_base._Attack;
    if (damage > target->_Defense)
    {
        damage -= target->_Defense;
    }
    else
    {
        damage = 1;  // Minimum 1 damage
    }

    // Apply damage
    if (target->_HP > damage)
    {
        target->_HP -= damage;
    }
    else
    {
        target->_HP = 0;
        // Handle character death
    }
}

void monsterTakeDamageFromCharacter(tMonster* monster, tCharacter* attacker)
{
    if (!monster || !attacker || monster->_state == MONSTER_STATE_DEAD)
        return;

    UWORD damage = attacker->_Attack;
    if (damage > monster->_base._Defense)
        damage -= monster->_base._Defense;
    else
        damage = 1;

    if (monster->_base._HP > damage)
        monster->_base._HP -= damage;
    else
        monster->_base._HP = 0;
}

void monsterDropLoot(tMonster* monster, tInventory* pInventory)
{
    if (!monster || monster->_state != MONSTER_STATE_DEAD || !pInventory)
        return;

    // Roll for each possible drop
    for (UBYTE i = 0; i < 8; i++)
    {
        if (monster->_dropTable[i] != 0)  // If there's an item in this slot
        {
            UBYTE roll = simpleRand();  // Roll 0-99
            if (roll < monster->_dropChance[i])
            {
                // Item dropped! Add to inventory
                inventoryAddItem(pInventory, monster->_dropTable[i], 1);
            }
        }
    }
}

void monsterPlaceInMaze(tMaze* maze, tMonster* monster, UBYTE x, UBYTE y)
{
    if (!maze || !monster)
        return;

    // Set monster position
    monster->_partyPosX = x;
    monster->_partyPosY = y;
}

void monsterRemoveFromMaze(tMaze* maze, tMonster* monster)
{
    if (!maze || !monster)
        return;

    // Clear monster position
    monster->_partyPosX = 0;
    monster->_partyPosY = 0;
} 