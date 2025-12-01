#pragma once

#include <ace/types.h>

// Item flags (bitmask) - flexible properties
#define ITEM_FLAG_EQUIPPABLE 0x01    // Can be equipped
#define ITEM_FLAG_CONSUMABLE 0x02    // Single use item
#define ITEM_FLAG_STACKABLE 0x04     // Can stack in inventory
#define ITEM_FLAG_KEY 0x08           // Is a key
#define ITEM_FLAG_UNIQUE 0x10         // Unique item (can only have one)
#define ITEM_FLAG_CURSED 0x20        // Cursed item (negative effects)
#define ITEM_FLAG_ACTIVATABLE 0x40   // Can be activated/used
#define ITEM_FLAG_AUTOMATIC 0x80     // Auto-uses when conditions met

// Item usage types - what happens when used
#define ITEM_USAGE_NONE 0
#define ITEM_USAGE_HEAL 1            // Restores HP
#define ITEM_USAGE_ENERGY 2          // Restores MP/Energy
#define ITEM_USAGE_BUFF 3            // Applies buff
#define ITEM_USAGE_UNLOCK 4          // Unlocks doors/containers
#define ITEM_USAGE_ATTACK 5          // Used as weapon
#define ITEM_USAGE_SCRIPT 6          // Triggers script event

// Item modifier types - what stat the modifier affects
#define ITEM_MOD_NONE 0
#define ITEM_MOD_ATTACK 1
#define ITEM_MOD_DEFENSE 2
#define ITEM_MOD_AGILITY 3
#define ITEM_MOD_INTELLIGENCE 4
#define ITEM_MOD_VITALITY 5
#define ITEM_MOD_LUCK 6

typedef struct _tItem
{
    UBYTE ubType;        // Generic type ID (defined in data, not hardcoded)
    UBYTE ubSubType;     // Sub-type ID
    UBYTE ubFlags;       // Item flags (bitmask)
    UBYTE ubModifierType; // What stat this modifies (ITEM_MOD_*)
    UBYTE ubModifier;    // Modifier value (attack/defense etc.)
    UBYTE ubValue;       // Value for consumables (HP/Energy restore), key ID for keys
    UBYTE ubWeight;      // Weight (not used in smite atm)
    UBYTE ubIcon;        // Icon/sprite index
    UBYTE ubUsageType;   // How the item is used (ITEM_USAGE_*)
    UBYTE ubKeyId;       // For keys: which lock this opens (0 = master key)
    UBYTE ubEquipSlot;   // Which slot this equips to (0-3, or 255 for none)
    UBYTE _pad;          // Padding for 68020+ alignment
    char* pszName;       // Item name (pointer to string)
} tItem;

extern tItem* s_vecItems;
extern UBYTE s_ubItemCount;

void loadItems(const char* filename);
void itemSystemInit(void);
void itemSystemDestroy(void);

tItem* getItem(UBYTE ubIndex);
UBYTE findItemByName(const char* pszName);
UBYTE findItemByKeyId(UBYTE ubKeyId);
