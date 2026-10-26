#include "character.h"
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <string.h>

tCharacter* characterCreate()
{
    tCharacter* character = (tCharacter*)memAllocFastClear(sizeof(tCharacter));
    return character;
}

void characterDestroy(tCharacter* character)
{
    memFree(character, sizeof(tCharacter));
}

tCharacterList* characterListCreate()
{
    tCharacterList* characterList = (tCharacterList*)memAllocFastClear(sizeof(tCharacterList));
    return characterList;
}

void characterListDestroy(tCharacterList* characterList)
{
    memFree(characterList, sizeof(tCharacterList));
}

tCharacterParty* characterPartyCreate()
{
    tCharacterParty* characterParty = (tCharacterParty*)memAllocFastClear(sizeof(tCharacterParty));
    if (characterParty) {
        characterParty->_characters = (tCharacter**)memAllocFastClear(sizeof(tCharacter*) * CHARACTER_PARTY_MAX);
        if (!characterParty->_characters) {
            memFree(characterParty, sizeof(tCharacterParty));
            return NULL;
        }
    }
    return characterParty;
}

void characterPartyDestroy(tCharacterParty* characterParty)
{
    if (!characterParty)
        return;
    if (characterParty->_characters) {
        for (UBYTE i = 0; i < characterParty->_numCharacters; i++) {
            if (characterParty->_characters[i])
                characterDestroy(characterParty->_characters[i]);
        }
        memFree(characterParty->_characters, sizeof(tCharacter*) * CHARACTER_PARTY_MAX);
    }
    memFree(characterParty, sizeof(tCharacterParty));
}

void characterPartyAdd(tCharacterParty* characterParty, tCharacter* character)
{
    if (!characterParty || !characterParty->_characters || characterParty->_numCharacters >= CHARACTER_PARTY_MAX)
        return;
    characterParty->_characters[characterParty->_numCharacters] = character;
    characterParty->_numCharacters++;
}

void characterPartyRemove(tCharacterParty* characterParty, UBYTE partyPos)
{
    if (!characterParty || !characterParty->_characters)
        return;
    if (partyPos >= characterParty->_numCharacters)
        return;
    characterDestroy(characterParty->_characters[partyPos]);
    /* Compact the array so the party never has holes in the middle. */
    for (UBYTE i = partyPos; i + 1 < characterParty->_numCharacters; i++)
        characterParty->_characters[i] = characterParty->_characters[i + 1];
    characterParty->_numCharacters--;
    characterParty->_characters[characterParty->_numCharacters] = NULL;
}

void characterPartySwap(tCharacterParty* characterParty, UBYTE partyPos1, UBYTE partyPos2)
{
    tCharacter* temp = characterParty->_characters[partyPos1];
    characterParty->_characters[partyPos1] = characterParty->_characters[partyPos2];
    characterParty->_characters[partyPos2] = temp;
}

void characterPartyEnsureDefaultHero(tCharacterParty* characterParty)
{
    if (!characterParty || !characterParty->_characters || characterParty->_numCharacters > 0)
        return;
    tCharacter* hero = characterCreate();
    if (!hero)
        return;
    hero->_Level = 1;
    hero->_MaxHP = 30;
    hero->_HP = 30;
    hero->_Attack = 6;
    hero->_Defense = 3;
    strncpy(hero->_Name, "Hero", sizeof(hero->_Name) - 1);
    hero->_Name[sizeof(hero->_Name) - 1] = '\0';
    characterPartyAdd(characterParty, hero);
}

UBYTE characterPartyGetAliveCount(tCharacterParty* characterParty)
{
    UBYTE ubAlive = 0;
    if (!characterParty || !characterParty->_characters)
        return 0;
    for (UBYTE i = 0; i < characterParty->_numCharacters; i++) {
        tCharacter* pChar = characterParty->_characters[i];
        if (pChar && pChar->_HP > 0)
            ubAlive++;
    }
    return ubAlive;
}

tCharacter* characterPartyGetAlive(tCharacterParty* characterParty)
{
    if (!characterParty || !characterParty->_characters)
        return NULL;
    for (UBYTE i = 0; i < characterParty->_numCharacters; i++) {
        tCharacter* pChar = characterParty->_characters[i];
        if (pChar && pChar->_HP > 0)
            return pChar;
    }
    return NULL;
}

UBYTE characterPartyAllDead(tCharacterParty* characterParty)
{
    if (!characterParty || !characterParty->_characters || characterParty->_numCharacters == 0)
        return 0;
    return characterPartyGetAliveCount(characterParty) == 0;
}

/**
 * Classic RPG curve, kept small enough for UWORD.
 * L1->2 = 100, L2->3 = 300, L3->4 = 600, L4->5 = 1000, then 65535 cap.
 */
UWORD characterXpToNextLevel(UBYTE ubLevel)
{
    ULONG ulLevel = ubLevel ? ubLevel : 1;
    ULONG ulNeed = 50ul * ulLevel * ulLevel + 50ul * ulLevel;
    if (ulNeed > 65535ul)
        ulNeed = 65535ul;
    return (UWORD)ulNeed;
}

UBYTE characterAwardXp(tCharacter* character, UWORD uwAmount)
{
    if (!character)
        return 0;

    ULONG ulXp = (ULONG)character->_Experience + uwAmount;
    if (ulXp > 65535ul)
        ulXp = 65535ul;
    character->_Experience = (UWORD)ulXp;

    UBYTE ubLevelsGained = 0;
    /* Level 0 means "never set up" - start the curve at 1 without growing. */
    if (character->_Level == 0)
        character->_Level = 1;

    while (character->_Experience >= characterXpToNextLevel(character->_Level)) {
        character->_Experience -= characterXpToNextLevel(character->_Level);
        character->_Level++;
        /* Grow a little each level; keep the numbers readable in the HUD. */
        character->_MaxHP += 5;
        character->_HP = character->_MaxHP;
        if (character->_Attack < 255)
            character->_Attack++;
        if (character->_Defense < 255)
            character->_Defense++;
        ubLevelsGained++;
        if (ubLevelsGained >= 20)
            break;
    }
    return ubLevelsGained;
}
