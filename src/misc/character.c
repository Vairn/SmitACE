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
    characterDestroy(characterParty->_characters[partyPos]);
    characterParty->_characters[partyPos] = NULL;
    characterParty->_numCharacters--;
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
