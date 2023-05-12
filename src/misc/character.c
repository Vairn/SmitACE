#include "character.h"
#include <ace/managers/memory.h>
#include <ace/managers/system.h>

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
    return characterParty;
}

void characterPartyDestroy(tCharacterParty* characterParty)
{
    memFree(characterParty, sizeof(tCharacterParty));
}

void characterPartyAdd(tCharacterParty* characterParty, tCharacter* character)
{
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

