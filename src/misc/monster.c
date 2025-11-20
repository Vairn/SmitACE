#include "monster.h"
#include <ace/managers/memory.h>
#include <ace/managers/system.h>

tMonster* monsterCreate(UBYTE monsterType)
{
    tMonster* monster = (tMonster*)memAllocFastClear(sizeof(tMonster));
    if (monster)
    {
        monster->_monsterType = monsterType;
        monster->_state = MONSTER_STATE_IDLE;
        monster->_aggroRange = 5;  // Default aggro range
        monster->_fleeThreshold = 20;  // Default flee at 20% HP
        
        // Initialize base character stats based on monster type
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
        }
    }
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
        list->_monsters = (tMonster**)memAllocFastClear(sizeof(tMonster*) * 32);  // Max 32 monsters
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
        memFree(list->_monsters, sizeof(tMonster*) * 32);
        memFree(list, sizeof(tMonsterList));
    }
}

void monsterUpdate(tMonster* monster, tCharacterParty* party)
{
    if (!monster || monster->_state == MONSTER_STATE_DEAD)
        return;

    // Calculate distance to party
    UBYTE dx = (monster->_partyPosX - party->_PartyX);
    UBYTE dy = (monster->_partyPosY - party->_PartyY);
    UBYTE distance = dx + dy;

    // Update monster state based on distance and HP
    if (monster->_state == MONSTER_STATE_IDLE)
    {
        if (distance <= monster->_aggroRange)
        {
            monster->_state = MONSTER_STATE_AGGRESSIVE;
        }
    }
    else if (monster->_state == MONSTER_STATE_AGGRESSIVE)
    {
        // Check if monster should flee
        UBYTE hpPercent = (monster->_base._HP * 100) / monster->_base._MaxHP;
        if (hpPercent <= monster->_fleeThreshold)
        {
            monster->_state = MONSTER_STATE_FLEEING;
        }
        else if (distance > monster->_aggroRange)
        {
            monster->_state = MONSTER_STATE_IDLE;
        }
    }
    else if (monster->_state == MONSTER_STATE_FLEEING)
    {
        if (distance > monster->_aggroRange)
        {
            monster->_state = MONSTER_STATE_IDLE;
        }
    }
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

void monsterDropLoot(tMonster* monster)
{
    if (!monster || monster->_state != MONSTER_STATE_DEAD)
        return;

    // Roll for each possible drop
    for (UBYTE i = 0; i < 8; i++)
    {
        if (monster->_dropTable[i] != 0)  // If there's an item in this slot
        {
            UBYTE roll = 1;//rand() % 100;  // Roll 0-99
            if (roll < monster->_dropChance[i])
            {
                // Item dropped! Handle item creation here
                // TODO: Implement item creation system
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