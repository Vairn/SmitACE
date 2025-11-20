# Script Engine Examples

This document provides practical examples of how to use the new script execution engine in your Amiga game.

## Basic Usage

### 1. Simple Door Trigger

```c
// Create a pressure plate that opens a door
UBYTE doorCoords[] = {10, 5}; // Door at position (10,5)
createEventTrigger(pMaze, 8, 5, EVENT_OPENDOOR, 2, doorCoords);
```

### 2. Flag-Based Door

```c
// Door that only opens if player has a key (flag 1 set)
tMaze* maze = getCurrentMaze();
UBYTE eventData[10];
int dataIndex = 0;

// Event 0: IF flag 1 == 1 (has key)
eventData[dataIndex++] = EVENT_EQUAL;
eventData[dataIndex++] = EVENT_LEVEL_FLAG;
eventData[dataIndex++] = 1; // flag index
eventData[dataIndex++] = 1; // value to compare
addEvent(maze, EVENT_IF, 0, 0, dataIndex, eventData);

// Event 1: Open door
UBYTE doorCoords[] = {10, 5};
addEvent(maze, EVENT_OPENDOOR, 0, 0, 2, doorCoords);

// Event 2: Show message
UBYTE msgData[] = {MSG_DOOR_OPENED};
addEvent(maze, EVENT_SHOWMESSAGE, 0, 0, 1, msgData);

// Event 3: ELSE
addEvent(maze, EVENT_ELSE, 0, 0, 0, NULL);

// Event 4: Show "need key" message
UBYTE needKeyMsg[] = {MSG_NEED_KEY};
addEvent(maze, EVENT_SHOWMESSAGE, 0, 0, 1, needKeyMsg);

// Event 5: ENDIF
addEvent(maze, EVENT_ENDIF, 0, 0, 0, NULL);
```

### 3. Complex Puzzle Switch

```c
// Switch that requires multiple conditions:
// - Party must be facing north
// - Must have completed quest (global flag 5 > 0)
// - Must have magic item (local flag 10 == 1)

UBYTE conditionData[] = {
    EVENT_EQUAL, EVENT_PARTY_DIRECTION, 0,        // facing north
    EVENT_AND,                                     // AND
    EVENT_GREATER, EVENT_GLOBAL_FLAG, 5, 0,       // quest completed
    EVENT_AND,                                     // AND  
    EVENT_EQUAL, EVENT_LEVEL_FLAG, 10, 1          // has magic item
};
addEvent(maze, EVENT_IF, 0, 0, 11, conditionData);

// Success actions
UBYTE secretDoor[] = {15, 20};
addEvent(maze, EVENT_OPENDOOR, 0, 0, 2, secretDoor);

UBYTE successMsg[] = {MSG_SECRET_REVEALED};
addEvent(maze, EVENT_SHOWMESSAGE, 0, 0, 1, successMsg);

// Set completion flag
UBYTE completionFlag[] = {0, 20, 1}; // local flag 20 = 1
addEvent(maze, EVENT_SETFLAG, 0, 0, 3, completionFlag);

addEvent(maze, EVENT_ENDIF, 0, 0, 0, NULL);
```

### 4. Teleporter with Subroutine

```c
// Main teleporter script
// Event 0: Show teleport message
UBYTE teleMsg[] = {MSG_TELEPORTING};
addEvent(maze, EVENT_SHOWMESSAGE, 0, 0, 1, teleMsg);

// Event 1: Call teleport effect subroutine
UBYTE gosubTarget[] = {10}; // Jump to event 10
addEvent(maze, EVENT_GOSUB, 0, 0, 1, gosubTarget);

// Event 2: Teleport player
UBYTE teleCoords[] = {50, 50};
addEvent(maze, EVENT_TELEPORT, 0, 0, 2, teleCoords);

// Event 3: Show arrival message
UBYTE arrivalMsg[] = {MSG_TELEPORT_COMPLETE};
addEvent(maze, EVENT_SHOWMESSAGE, 0, 0, 1, arrivalMsg);

// Event 4: End main script
UBYTE endScript[] = {99}; // Jump past subroutine
addEvent(maze, EVENT_GOTO, 0, 0, 1, endScript);

// Events 10-15: Teleport effect subroutine
// Event 10: Play sound effect
UBYTE teleSound[] = {SFX_TELEPORT};
addEvent(maze, EVENT_SOUND, 0, 0, 1, teleSound);

// Event 11: Screen flash effect (custom event)
// Event 12: Particle effects (custom event)
// Event 13: Second sound
UBYTE teleSound2[] = {SFX_TELEPORT_END};
addEvent(maze, EVENT_SOUND, 0, 0, 1, teleSound2);

// Event 14: Return from subroutine
addEvent(maze, EVENT_RETURN, 0, 0, 0, NULL);

// Event 99: Script end point
```

### 5. Monster Spawner

```c
// Spawns monsters based on party level and position
// Event 0: Check if party is in spawn zone
UBYTE spawnZoneCheck[] = {
    EVENT_GREATER, EVENT_PARTY_ON_POS, 20, 19,    // X > 20
    EVENT_AND,
    EVENT_LESS, EVENT_PARTY_ON_POS, 30, 31        // X < 30, Y < 31
};
addEvent(maze, EVENT_IF, 0, 0, 8, spawnZoneCheck);

// Event 1: Check if not already spawned
UBYTE notSpawnedCheck[] = {EVENT_EQUAL, EVENT_LEVEL_FLAG, 50, 0};
addEvent(maze, EVENT_IF, 0, 0, 4, notSpawnedCheck);

// Event 2: Spawn monster
UBYTE monsterData[] = {MONSTER_TYPE_NORMAL, 25, 25}; // type, x, y
addEvent(maze, EVENT_ADDMONSTER, 0, 0, 3, monsterData);

// Event 3: Set spawned flag
UBYTE spawnedFlag[] = {0, 50, 1}; // local flag 50 = 1
addEvent(maze, EVENT_SETFLAG, 0, 0, 3, spawnedFlag);

// Event 4: Show spawn message
UBYTE spawnMsg[] = {MSG_MONSTER_APPEARS};
addEvent(maze, EVENT_SHOWMESSAGE, 0, 0, 1, spawnMsg);

// Event 5: ENDIF (not spawned check)
addEvent(maze, EVENT_ENDIF, 0, 0, 0, NULL);

// Event 6: ENDIF (spawn zone check)
addEvent(maze, EVENT_ENDIF, 0, 0, 0, NULL);
```

### 6. Treasure Chest with Trap

```c
// Event 0: Check if chest is already opened
UBYTE chestOpenCheck[] = {EVENT_EQUAL, EVENT_LEVEL_FLAG, 30, 1};
addEvent(maze, EVENT_IF, 0, 0, 4, chestOpenCheck);

// Event 1: Already opened message
UBYTE emptyMsg[] = {MSG_CHEST_EMPTY};
addEvent(maze, EVENT_SHOWMESSAGE, 0, 0, 1, emptyMsg);

// Event 2: GOTO end
UBYTE gotoEnd[] = {20};
addEvent(maze, EVENT_GOTO, 0, 0, 1, gotoEnd);

// Event 3: ENDIF
addEvent(maze, EVENT_ENDIF, 0, 0, 0, NULL);

// Event 4: Check for trap detection skill
UBYTE trapCheck[] = {EVENT_GREATER, EVENT_GLOBAL_FLAG, 15, 5}; // skill > 5
addEvent(maze, EVENT_IF, 0, 0, 4, trapCheck);

// Event 5: Trap detected message
UBYTE trapMsg[] = {MSG_TRAP_DETECTED};
addEvent(maze, EVENT_SHOWMESSAGE, 0, 0, 1, trapMsg);

// Event 6: ELSE (trap not detected)
addEvent(maze, EVENT_ELSE, 0, 0, 0, NULL);

// Event 7: Spring trap - damage player
UBYTE trapDamage[] = {15}; // 15 damage
addEvent(maze, EVENT_DAMAGE, 0, 0, 1, trapDamage);

// Event 8: Trap damage message
UBYTE damageMsg[] = {MSG_TRAP_DAMAGE};
addEvent(maze, EVENT_SHOWMESSAGE, 0, 0, 1, damageMsg);

// Event 9: ENDIF (trap check)
addEvent(maze, EVENT_ENDIF, 0, 0, 0, NULL);

// Event 10: Give treasure
UBYTE treasure[] = {ITEM_GOLD_COINS, 50}; // 50 gold coins
addEvent(maze, EVENT_GIVEITEM, 0, 0, 2, treasure);

// Event 11: Give XP
UBYTE xpReward[] = {100, 0}; // 100 XP
addEvent(maze, EVENT_ADDXP, 0, 0, 2, xpReward);

// Event 12: Set chest opened flag
UBYTE openedFlag[] = {0, 30, 1}; // local flag 30 = 1
addEvent(maze, EVENT_SETFLAG, 0, 0, 3, openedFlag);

// Event 13: Success message
UBYTE successMsg[] = {MSG_TREASURE_FOUND};
addEvent(maze, EVENT_SHOWMESSAGE, 0, 0, 1, successMsg);

// Event 20: End point
```

## Advanced Techniques

### Nested Conditions

```c
// IF player has key AND (is wizard OR has magic item)
UBYTE complexCondition[] = {
    EVENT_EQUAL, EVENT_LEVEL_FLAG, 1, 1,          // has key
    EVENT_AND,
    EVENT_EQUAL, EVENT_GLOBAL_FLAG, 10, 1,        // is wizard
    EVENT_OR,
    EVENT_EQUAL, EVENT_LEVEL_FLAG, 5, 1           // has magic item
};
```

### State Machines

Use flags to create complex state machines:

```c
// State 0: Initial
// State 1: First switch activated  
// State 2: Second switch activated
// State 3: Puzzle complete

// Check current state and advance accordingly
UBYTE stateCheck[] = {EVENT_EQUAL, EVENT_LEVEL_FLAG, 100, 0}; // state flag
addEvent(maze, EVENT_IF, 0, 0, 4, stateCheck);
// Handle state 0 logic...
addEvent(maze, EVENT_ENDIF, 0, 0, 0, NULL);

UBYTE stateCheck2[] = {EVENT_EQUAL, EVENT_LEVEL_FLAG, 100, 1};
addEvent(maze, EVENT_IF, 0, 0, 4, stateCheck2);
// Handle state 1 logic...
addEvent(maze, EVENT_ENDIF, 0, 0, 0, NULL);
```

### Script Debugging

Enable detailed logging by setting debug flags:

```c
// Set debug flag to enable verbose logging
UBYTE debugFlag[] = {1, 255, 1}; // global flag 255 = debug mode
addEvent(maze, EVENT_SETFLAG, 0, 0, 3, debugFlag);
```

## Best Practices

1. **Use meaningful flag numbers**: Document what each flag represents
2. **Test edge cases**: Always test with invalid conditions
3. **Avoid infinite loops**: Be careful with GOTO statements
4. **Use subroutines**: Break complex scripts into reusable parts
5. **Error handling**: Always provide fallback behavior
6. **Performance**: Minimize complex condition chains in frequently executed scripts

## Flag Management

### Recommended Flag Usage:
- **0-99**: Level-specific temporary flags
- **100-199**: Level-specific persistent flags  
- **200-255**: Global game state flags

### Global Flags (examples):
- **200**: Game difficulty level
- **201**: Player class
- **202**: Main quest progress
- **203**: Tutorial completed
- **255**: Debug mode

### Local Flags (examples):
- **0-9**: Temporary puzzle states
- **10-19**: Door/switch states
- **20-29**: Monster spawn flags
- **30-39**: Treasure/item flags
- **40-49**: NPC interaction states 