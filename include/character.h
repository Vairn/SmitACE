#pragma once

#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/joy.h>
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/managers/ptplayer.h>
#include <ace/utils/palette.h>
#include <fade.h>

#ifndef AMIGA
#include "amiTypes.h"

#endif


typedef struct _character
{
    UBYTE _partyPos;
    UBYTE _Level;
    UWORD _Experience;
    UWORD _HP;
    UWORD _MaxHP;
    UWORD _MP;
    UWORD _MaxMP;
    UBYTE _Strength;
    UBYTE _Agility;
    UBYTE _Vitality;
    UBYTE _Intelligence;
    UBYTE _Luck;
    UBYTE _Attack;
    UBYTE _Defense;
    UBYTE _MagicDefense;
    UBYTE _Hands[4]; // Max of 4 hands
    UBYTE _Armor;
    UBYTE _Shield;
    UBYTE _Helmet;
    UBYTE _Accessory[3];
    UBYTE _Feet;
    UBYTE _StatusEffect;
    UBYTE _StatusEffectTime;
    UBYTE _ElementalResistance[8];
    UBYTE _ElementalWeakness[8];
    UBYTE _SpellResistance[8];
    UBYTE _SpellWeakness[8];
    UBYTE _SpellImmunity[8];
    UBYTE _Spells[8]; // bitmask of spells
    UBYTE _Class;
    UBYTE _Race;
    char _Name[16];
    UBYTE _Sprite;
} tCharacter;

typedef struct _characterList
{
    UBYTE _numCharacters;
    tCharacter** _characters; // dynamic list of Characters
} tCharacterList;

typedef struct _characterParty
{
    UBYTE _numCharacters;
    tCharacter** _characters; // dynamic list of Characters

    UBYTE _numActiveCharacters;
    UBYTE _PartyX;
    UBYTE _PartyY;
    UBYTE _PartyFacing;
    
} tCharacterParty;

tCharacter* characterCreate();
void characterDestroy(tCharacter* character);
tCharacterList* characterListCreate();
void characterListDestroy(tCharacterList* characterList);
tCharacterParty* characterPartyCreate();
void characterPartyDestroy(tCharacterParty* characterParty);
void characterPartyAdd(tCharacterParty* characterParty, tCharacter* character);
void characterPartyRemove(tCharacterParty* characterParty, UBYTE partyPos);
void characterPartySwap(tCharacterParty* characterParty, UBYTE partyPos1, UBYTE partyPos2);
