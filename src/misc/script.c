#include "script.h"
#include "gameState.h"
#include <ace/managers/memory.h>

// Script execution result types
typedef enum {
    SCRIPT_RESULT_CONTINUE,
    SCRIPT_RESULT_GOTO,
    SCRIPT_RESULT_GOSUB,
    SCRIPT_RESULT_RETURN,
    SCRIPT_RESULT_END,
    SCRIPT_RESULT_ERROR
} tScriptResult;

typedef struct {
    tScriptResult result;
    UWORD targetIndex;
    tScriptError error;
} tScriptExecutionResult;

// Stack frame for nested constructs
typedef struct {
    UBYTE type;  // IF=1, GOSUB=2
    UWORD address;
    BOOL conditionMet;
} tStackFrame;

// Forward declarations
tScriptExecutionResult executeEvent(tMaze *pMaze, tMazeEvent *pEvent);
BOOL evaluateCondition(tMaze *pMaze, UBYTE *conditionData, UBYTE dataSize);
BOOL validateEventData(tMazeEvent *pEvent, tMaze *pMaze);
void pushStackFrame(UBYTE type, UWORD address, BOOL conditionMet);
tStackFrame popStackFrame();
BOOL isStackEmpty();

// Condition evaluation function
BOOL evaluateCondition(tMaze *pMaze, UBYTE *conditionData, UBYTE dataSize)
{
    if (!conditionData || dataSize < 3) {
        logWrite("Invalid condition data\n");
        return FALSE;
    }
    
    UBYTE conditionType = conditionData[0];
    UBYTE operandType = conditionData[1];
    UBYTE operandValue = conditionData[2];
    BOOL result = FALSE;
    
    switch (conditionType) {
        case EVENT_EQUAL:
            switch (operandType) {
                case EVENT_LEVEL_FLAG:
                    if (operandValue < 256) {
                        result = (g_pGameState->m_bLocalFlags[operandValue] == conditionData[3]);
                    }
                    break;
                case EVENT_GLOBAL_FLAG:
                    if (operandValue < 256) {
                        result = (g_pGameState->m_bGlobalFlags[operandValue] == conditionData[3]);
                    }
                    break;
                case EVENT_PARTY_ON_POS:
                    result = (g_pGameState->m_pCurrentParty->_PartyX == operandValue && 
                             g_pGameState->m_pCurrentParty->_PartyY == conditionData[3]);
                    break;
                case EVENT_PARTY_DIRECTION:
                    result = (g_pGameState->m_pCurrentParty->_PartyFacing == operandValue);
                    break;
            }
            break;
            
        case EVENT_GREATER:
            switch (operandType) {
                case EVENT_LEVEL_FLAG:
                    if (operandValue < 256) {
                        result = (g_pGameState->m_bLocalFlags[operandValue] > conditionData[3]);
                    }
                    break;
                case EVENT_GLOBAL_FLAG:
                    if (operandValue < 256) {
                        result = (g_pGameState->m_bGlobalFlags[operandValue] > conditionData[3]);
                    }
                    break;
            }
            break;
            
        case EVENT_LESS:
            switch (operandType) {
                case EVENT_LEVEL_FLAG:
                    if (operandValue < 256) {
                        result = (g_pGameState->m_bLocalFlags[operandValue] < conditionData[3]);
                    }
                    break;
                case EVENT_GLOBAL_FLAG:
                    if (operandValue < 256) {
                        result = (g_pGameState->m_bGlobalFlags[operandValue] < conditionData[3]);
                    }
                    break;
            }
            break;
            
        case EVENT_NOT_EQUAL:
            switch (operandType) {
                case EVENT_LEVEL_FLAG:
                    if (operandValue < 256) {
                        result = (g_pGameState->m_bLocalFlags[operandValue] != conditionData[3]);
                    }
                    break;
                case EVENT_GLOBAL_FLAG:
                    if (operandValue < 256) {
                        result = (g_pGameState->m_bGlobalFlags[operandValue] != conditionData[3]);
                    }
                    break;
            }
            break;
    }
    
    // Handle logical operators if more data available
    if (dataSize > 4) {
        UBYTE logicalOp = conditionData[4];
        BOOL nextResult = evaluateCondition(pMaze, &conditionData[5], dataSize - 5);
        
        switch (logicalOp) {
            case EVENT_AND:
                result = result && nextResult;
                break;
            case EVENT_OR:
                result = result || nextResult;
                break;
        }
    }
    
    
    return result;
}

// Stack management functions
void pushStackFrame(UBYTE type, UWORD address, BOOL conditionMet)
{
    if (g_pGameState->_scriptState._top < SCRIPT_RETURN_STACK_SIZE) {
        tStackFrame frame;
        frame.type = type;
        frame.address = address;
        frame.conditionMet = conditionMet;
        
        // Store as packed data in the stack
        g_pGameState->_scriptState._stack[g_pGameState->_scriptState._top] = 
            (type << 14) | (conditionMet ? 0x2000 : 0) | (address & 0x1FFF);
        g_pGameState->_scriptState._top++;
    }
}

tStackFrame popStackFrame()
{
    tStackFrame frame = {0, 0, FALSE};
    if (g_pGameState->_scriptState._top > 0) {
        g_pGameState->_scriptState._top--;
        UWORD packed = g_pGameState->_scriptState._stack[g_pGameState->_scriptState._top];
        frame.type = (packed >> 14) & 0x3;
        frame.conditionMet = (packed & 0x2000) != 0;
        frame.address = packed & 0x1FFF;
    }
    return frame;
}

BOOL isStackEmpty()
{
    return g_pGameState->_scriptState._top == 0;
}

// Event data validation
BOOL validateEventData(tMazeEvent *pEvent, tMaze *pMaze)
{
    switch (pEvent->_eventType) {
        case EVENT_TELEPORT:
        case EVENT_ADDMONSTER:
            return pEvent->_eventDataSize >= 2;
        case EVENT_OPENDOOR:
            // Can work with size 0/1 (uses event position) or size >= 2 (uses data)
            return TRUE;
        case EVENT_SETWALL:
        case EVENT_SETFLOOR:
        case EVENT_SETCOL:
        case EVENT_SETFLAG:
        case EVENT_CLEARFLAG:
            return pEvent->_eventDataSize >= 1;
        case EVENT_GOTO:
        case EVENT_GOSUB:
            return pEvent->_eventDataSize >= 1 && pEvent->_eventData[0] < pMaze->_eventCount;
        default:
            return TRUE; // Assume valid for unspecified events
    }
}

// Main event execution function
tScriptExecutionResult executeEvent(tMaze *pMaze, tMazeEvent *pEvent)
{
    tScriptExecutionResult result = {SCRIPT_RESULT_CONTINUE, 0, SCRIPT_ERROR_NONE};
    
    // Validate event data
    if (!validateEventData(pEvent, pMaze)) {
        logWrite("Invalid event data for event type %d\n", pEvent->_eventType);
        result.result = SCRIPT_RESULT_ERROR;
        result.error = SCRIPT_ERROR_INVALID_EVENT;
        return result;
    }
    
    logWrite("Executing event type %d at (%d,%d) with data size %d\n", 
        pEvent->_eventType, pEvent->_x, pEvent->_y, pEvent->_eventDataSize);
    
    switch (pEvent->_eventType)
    {
    case EVENT_SETWALL:
        pMaze->_mazeData[pEvent->_x + pEvent->_y * pMaze->_width] = pEvent->_eventData[0];
        break;
        
    case EVENT_SETFLOOR:
        pMaze->_mazeFloor[pEvent->_x + pEvent->_y * pMaze->_width] = pEvent->_eventData[0];
        break;
        
    case EVENT_SETCOL:
        pMaze->_mazeCol[pEvent->_x + pEvent->_y * pMaze->_width] = pEvent->_eventData[0];
        break;
        
    case EVENT_CLEARWALL:
        pMaze->_mazeData[pEvent->_x + pEvent->_y * pMaze->_width] = 0;
        break;
        
    case EVENT_CLEARFLOOR:
        pMaze->_mazeFloor[pEvent->_x + pEvent->_y * pMaze->_width] = 0;
        break;
        
    case EVENT_CLEARCOL:
        pMaze->_mazeCol[pEvent->_x + pEvent->_y * pMaze->_width] = 0;
        break;
        
    case EVENT_SETWALLCOL:
        if (pEvent->_eventDataSize >= 2) {
            pMaze->_mazeData[pEvent->_x + pEvent->_y * pMaze->_width] = pEvent->_eventData[0];
            pMaze->_mazeCol[pEvent->_x + pEvent->_y * pMaze->_width] = pEvent->_eventData[1];
        }
        break;
        
    case EVENT_CLEARWALLCOL:
        pMaze->_mazeData[pEvent->_x + pEvent->_y * pMaze->_width] = 0;
        pMaze->_mazeCol[pEvent->_x + pEvent->_y * pMaze->_width] = 0;
        break;
        
    case EVENT_SHOWMESSAGE:
        if (pEvent->_eventDataSize > 0) {
            // Assuming eventData contains message ID or text pointer
            logWrite("Showing message ID: %d\n", pEvent->_eventData[0]);
            // TODO: Implement actual message display system
        }
        break;
        
    case EVENT_OPENDOOR:
        if (pEvent->_eventDataSize >= 2) {
            UBYTE doorX = pEvent->_eventData[0];
            UBYTE doorY = pEvent->_eventData[1];
            logWrite("Opening door at (%d,%d)\n", doorX, doorY);
            tDoorAnim* anim = doorAnimCreate(doorX, doorY, DOOR_ANIM_OPENING);
            doorAnimAdd(pMaze, anim);
            pMaze->_mazeData[doorX + doorY * pMaze->_width] = MAZE_DOOR_OPEN;
        } else {
            logWrite("Opening door at event position (%d,%d)\n", pEvent->_x, pEvent->_y);
            tDoorAnim* anim = doorAnimCreate(pEvent->_x, pEvent->_y, DOOR_ANIM_OPENING);
            doorAnimAdd(pMaze, anim);
            pMaze->_mazeData[pEvent->_x + pEvent->_y * pMaze->_width] = MAZE_DOOR_OPEN;
        }
        break;
        
    case EVENT_CLOSEDOOR:
        {
            tDoorAnim* anim = doorAnimCreate(pEvent->_x, pEvent->_y, DOOR_ANIM_CLOSING);
            doorAnimAdd(pMaze, anim);
            pMaze->_mazeData[pEvent->_x + pEvent->_y * pMaze->_width] = MAZE_DOOR;
        }
        break;
        
    case EVENT_BATTERY_CHARGER:
        if (pEvent->_eventDataSize > 0 && pEvent->_eventData[0] > 0) {
            g_pGameState->m_pCurrentParty->_BatteryLevel++;
            pEvent->_eventData[0]--;
            logWrite("Battery recharged to %d, charger has %d units left\n", 
                g_pGameState->m_pCurrentParty->_BatteryLevel, pEvent->_eventData[0]);
        } else {
            logWrite("Battery charger at (%d,%d) is depleted\n", pEvent->_x, pEvent->_y);
        }
        break;
        
    case EVENT_TELEPORT:
        if (pEvent->_eventDataSize >= 2) {
            UBYTE targetX = pEvent->_eventData[0];
            UBYTE targetY = pEvent->_eventData[1];
            logWrite("Teleporting party to (%d,%d)\n", targetX, targetY);
            
            if (targetX < pMaze->_width && targetY < pMaze->_height) {
                UBYTE targetCell = pMaze->_mazeData[targetX + targetY * pMaze->_width];
                if (targetCell == MAZE_FLOOR || targetCell == MAZE_DOOR_OPEN) {
                    g_pGameState->m_pCurrentParty->_PartyX = targetX;
                    g_pGameState->m_pCurrentParty->_PartyY = targetY;
                } else {
                    logWrite("Cannot teleport to invalid position (%d,%d)\n", targetX, targetY);
                }
            } else {
                logWrite("Invalid teleport coordinates (%d,%d)\n", targetX, targetY);
            }
        }
        break;
        
    case EVENT_GIVEITEM:
        if (pEvent->_eventDataSize >= 2) {
            UBYTE itemType = pEvent->_eventData[0];
            UBYTE quantity = pEvent->_eventDataSize > 1 ? pEvent->_eventData[1] : 1;
            logWrite("Giving item type %d, quantity %d to party\n", itemType, quantity);
            // TODO: Implement inventory system integration
        }
        break;
        
    case EVENT_TAKEITEM:
        if (pEvent->_eventDataSize >= 2) {
            UBYTE itemType = pEvent->_eventData[0];
            UBYTE quantity = pEvent->_eventDataSize > 1 ? pEvent->_eventData[1] : 1;
            logWrite("Taking item type %d, quantity %d from party\n", itemType, quantity);
            // TODO: Implement inventory system integration
        }
        break;
        
    case EVENT_SETFLAG:
        if (pEvent->_eventDataSize >= 2) {
            UBYTE flagType = pEvent->_eventData[0]; // 0=local, 1=global
            UBYTE flagIndex = pEvent->_eventData[1];
            UBYTE flagValue = pEvent->_eventDataSize > 2 ? pEvent->_eventData[2] : 1;
            
            if (flagIndex < 256) {
                if (flagType == 0) {
                    g_pGameState->m_bLocalFlags[flagIndex] = flagValue;
                    logWrite("Set local flag %d to %d\n", flagIndex, flagValue);
                } else {
                    g_pGameState->m_bGlobalFlags[flagIndex] = flagValue;
                    logWrite("Set global flag %d to %d\n", flagIndex, flagValue);
                }
            }
        }
        break;
        
    case EVENT_CLEARFLAG:
        if (pEvent->_eventDataSize >= 2) {
            UBYTE flagType = pEvent->_eventData[0]; // 0=local, 1=global
            UBYTE flagIndex = pEvent->_eventData[1];
            
            if (flagIndex < 256) {
                if (flagType == 0) {
                    g_pGameState->m_bLocalFlags[flagIndex] = 0;
                    logWrite("Cleared local flag %d\n", flagIndex);
                } else {
                    g_pGameState->m_bGlobalFlags[flagIndex] = 0;
                    logWrite("Cleared global flag %d\n", flagIndex);
                }
            }
        }
        break;
        
    case EVENT_ADDMONSTER:
        if (pEvent->_eventDataSize >= 3) {
            UBYTE monsterType = pEvent->_eventData[0];
            UBYTE x = pEvent->_eventData[1];
            UBYTE y = pEvent->_eventData[2];
            
            tMonster* monster = monsterCreate(monsterType);
            if (monster) {
                monsterPlaceInMaze(pMaze, monster, x, y);
                if (g_pGameState->m_pMonsterList->_numMonsters < MAX_MONSTERS) {
                    g_pGameState->m_pMonsterList->_monsters[g_pGameState->m_pMonsterList->_numMonsters++] = monster;
                    logWrite("Added monster type %d at (%d,%d)\n", monsterType, x, y);
                } else {
                    logWrite("Monster list full, cannot add monster\n");
                    monsterDestroy(monster);
                }
            }
        }
        break;
        
    case EVENT_REMOVEMONSTER:
        if (pEvent->_eventDataSize >= 2) {
            UBYTE x = pEvent->_eventData[0];
            UBYTE y = pEvent->_eventData[1];
            
            for (UBYTE i = 0; i < g_pGameState->m_pMonsterList->_numMonsters; i++) {
                tMonster* monster = g_pGameState->m_pMonsterList->_monsters[i];
                if (monster->_partyPosX == x && monster->_partyPosY == y) {
                    monsterRemoveFromMaze(pMaze, monster);
                    monsterDestroy(monster);
                    
                    for (UBYTE j = i; j < g_pGameState->m_pMonsterList->_numMonsters - 1; j++) {
                        g_pGameState->m_pMonsterList->_monsters[j] = g_pGameState->m_pMonsterList->_monsters[j + 1];
                    }
                    g_pGameState->m_pMonsterList->_numMonsters--;
                    logWrite("Removed monster at (%d,%d)\n", x, y);
                    break;
                }
            }
        }
        break;
        
    case EVENT_ADDXP:
        if (pEvent->_eventDataSize >= 1) {
            UWORD xpAmount = pEvent->_eventData[0];
            if (pEvent->_eventDataSize >= 2) {
                xpAmount |= (pEvent->_eventData[1] << 8); // Support 16-bit XP values
            }
            logWrite("Adding %d XP to party\n", xpAmount);
            // TODO: Implement XP system integration
        }
        break;
        
    case EVENT_DAMAGE:
        if (pEvent->_eventDataSize >= 1) {
            UBYTE damageAmount = pEvent->_eventData[0];
            logWrite("Dealing %d damage to party\n", damageAmount);
            // TODO: Implement damage system integration
        }
        break;
        
    case EVENT_TURN:
        if (pEvent->_eventDataSize >= 2) {
            UBYTE direction = pEvent->_eventData[0];
            UBYTE count = pEvent->_eventData[1];
            logWrite("Turning party %s %d times\n", direction ? "right" : "left", count);
            
            for (UBYTE i = 0; i < count; i++) {
                if (direction) {
                    g_pGameState->m_pCurrentParty->_PartyFacing = (g_pGameState->m_pCurrentParty->_PartyFacing + 1) % 4;
                } else {
                    g_pGameState->m_pCurrentParty->_PartyFacing = (g_pGameState->m_pCurrentParty->_PartyFacing + 3) % 4;
                }
            }
        }
        break;
        
    case EVENT_IF:
        {
            BOOL conditionResult = evaluateCondition(pMaze, pEvent->_eventData, pEvent->_eventDataSize);
            g_pGameState->_scriptState._conditionMet = conditionResult;
            
            if (!conditionResult) {
                g_pGameState->_scriptState._skippingBlock = TRUE;
            }
            
            pushStackFrame(1, g_pGameState->_scriptState._scriptProgramCounter, conditionResult);
            logWrite("IF condition evaluated to %s\n", conditionResult ? "TRUE" : "FALSE");
        }
        break;
        
    case EVENT_ELSE:
        if (!isStackEmpty()) {
            tStackFrame frame = popStackFrame();
            if (frame.type == 1) { // IF frame
                g_pGameState->_scriptState._skippingBlock = frame.conditionMet;
                pushStackFrame(1, frame.address, frame.conditionMet);
            }
        }
        break;
        
    case EVENT_ENDIF:
        if (!isStackEmpty()) {
            tStackFrame frame = popStackFrame();
            if (frame.type == 1) { // IF frame
                g_pGameState->_scriptState._skippingBlock = FALSE;
                g_pGameState->_scriptState._conditionMet = FALSE;
            }
        }
        break;
        
    case EVENT_GOTO:
        if (pEvent->_eventDataSize > 0) {
            result.result = SCRIPT_RESULT_GOTO;
            result.targetIndex = pEvent->_eventData[0];
            logWrite("GOTO to index %d\n", result.targetIndex);
        }
        break;
        
    case EVENT_GOSUB:
        if (pEvent->_eventDataSize > 0) {
            pushStackFrame(2, g_pGameState->_scriptState._scriptProgramCounter + 1, FALSE);
            result.result = SCRIPT_RESULT_GOSUB;
            result.targetIndex = pEvent->_eventData[0];
            logWrite("GOSUB to index %d\n", result.targetIndex);
        }
        break;
        
    case EVENT_RETURN:
        if (!isStackEmpty()) {
            tStackFrame frame = popStackFrame();
            if (frame.type == 2) { // GOSUB frame
                result.result = SCRIPT_RESULT_RETURN;
                result.targetIndex = frame.address;
                logWrite("RETURN to index %d\n", result.targetIndex);
            }
        } else {
            logWrite("RETURN stack underflow!\n");
            result.result = SCRIPT_RESULT_ERROR;
            result.error = SCRIPT_ERROR_STACK_UNDERFLOW;
        }
        break;
        
    case EVENT_SOUND:
        if (pEvent->_eventDataSize >= 1) {
            UBYTE soundId = pEvent->_eventData[0];
            logWrite("Playing sound ID: %d\n", soundId);
            // TODO: Implement sound system integration
        }
        break;
        
    case EVENT_ENCOUNTER:
        if (pEvent->_eventDataSize >= 1) {
            UBYTE encounterId = pEvent->_eventData[0];
            logWrite("Starting encounter ID: %d\n", encounterId);
            // TODO: Implement encounter system integration
        }
        break;
        
    // Condition events - these should not be executed directly
    case EVENT_OR:
    case EVENT_AND:
    case EVENT_GREATER:
    case EVENT_LESS:
    case EVENT_NOT_EQUAL:
    case EVENT_EQUAL:
    case EVENT_LEVEL_FLAG:
    case EVENT_GLOBAL_FLAG:
    case EVENT_PARTY_ON_POS:
    case EVENT_PARTY_DIRECTION:
    case EVENT_HAS_CLASS:
    case EVENT_HAS_RACE:
        logWrite("Warning: Condition event %d executed directly\n", pEvent->_eventType);
        break;
        
    default:
        logWrite("Unhandled event type: %d\n", pEvent->_eventType);
        break;
    }
    
    return result;
}

// Main script execution function
void executeScript(tMaze *pMaze, UWORD startIndex)
{
    if (!pMaze || !pMaze->_events || pMaze->_eventCount == 0) {
        logWrite("No script events to execute.\n");
        return;
    }
    
    // Initialize script state
    g_pGameState->_scriptState._scriptProgramCounter = startIndex;
    g_pGameState->_scriptState._top = 0;
    g_pGameState->_scriptState._conditionMet = FALSE;
    g_pGameState->_scriptState._skippingBlock = FALSE;
    
    logWrite("Starting script execution from index %d with %d total events.\n", startIndex, pMaze->_eventCount);
    
    while (g_pGameState->_scriptState._scriptProgramCounter < pMaze->_eventCount) {
        tMazeEvent* currentEvent = &pMaze->_events[g_pGameState->_scriptState._scriptProgramCounter];
        
        // Skip events if we're in a skipped conditional block
        if (g_pGameState->_scriptState._skippingBlock) {
            if (currentEvent->_eventType != EVENT_ELSE && currentEvent->_eventType != EVENT_ENDIF) {
                logWrite("Skipping event type %d at index %d\n", 
                    currentEvent->_eventType, g_pGameState->_scriptState._scriptProgramCounter);
                g_pGameState->_scriptState._scriptProgramCounter++;
                continue;
            }
        }
        
        tScriptExecutionResult execResult = executeEvent(pMaze, currentEvent);
        
        switch (execResult.result) {
            case SCRIPT_RESULT_CONTINUE:
                g_pGameState->_scriptState._scriptProgramCounter++;
                break;
                
            case SCRIPT_RESULT_GOTO:
            case SCRIPT_RESULT_GOSUB:
                if (execResult.targetIndex < pMaze->_eventCount) {
                    g_pGameState->_scriptState._scriptProgramCounter = execResult.targetIndex;
                } else {
                    logWrite("Invalid jump target: %d\n", execResult.targetIndex);
                    g_pGameState->_scriptState._scriptProgramCounter++;
                }
                break;
                
            case SCRIPT_RESULT_RETURN:
                g_pGameState->_scriptState._scriptProgramCounter = execResult.targetIndex;
                break;
                
            case SCRIPT_RESULT_END:
                logWrite("Script execution ended.\n");
                return;
                
            case SCRIPT_RESULT_ERROR:
                logWrite("Script execution error: %d\n", execResult.error);
                return;
        }
    }
    
    logWrite("Script execution completed.\n");
}

// Legacy handleEvent function for compatibility
void handleEvent(tMaze *pMaze, tMazeEvent *pEvent)
{
    executeEvent(pMaze, pEvent);
}

void createEventTrigger(tMaze* pMaze, UBYTE x, UBYTE y, UBYTE eventType, UBYTE eventDataSize, UBYTE* eventData)
{
    logWrite("Creating event trigger at (%d,%d) type %d with data size %d\n", 
        x, y, eventType, eventDataSize);
    
    // Set the cell as an event trigger
    pMaze->_mazeData[x + y * pMaze->_width] = MAZE_EVENT_TRIGGER;
    
    // Create and add the event
    tMazeEvent* pEvent = mazeEventCreate(x, y, eventType, eventDataSize, eventData);
    if (pEvent) {
        logWrite("Event created successfully\n");
        mazeAppendEvent(pMaze, pEvent);
        logWrite("Event appended to maze, total events: %d\n", pMaze->_eventCount);
    } else {
        logWrite("Failed to create event\n");
    }
}

// Door animation functions
tDoorAnim* doorAnimCreate(BYTE x, BYTE y, UBYTE state)
{
    tDoorAnim* anim = (tDoorAnim*)memAllocFast(sizeof(tDoorAnim));
    if (anim) {
        anim->x = x;
        anim->y = y;
        anim->state = state;
        anim->frame = 0;  // Start at first frame
        anim->timer = 0;
        anim->next = NULL;
    }
    return anim;
}

void doorAnimAdd(tMaze* maze, tDoorAnim* anim)
{
    if (!anim) return;
    
    // Add to front of list
    anim->next = maze->_doorAnims;
    maze->_doorAnims = anim;
}

void doorAnimRemove(tMaze* maze, tDoorAnim* anim)
{
    if (!anim || !maze->_doorAnims) return;
    
    if (maze->_doorAnims == anim) {
        maze->_doorAnims = anim->next;
        memFree(anim, sizeof(tDoorAnim));
        return;
    }
    
    tDoorAnim* current = maze->_doorAnims;
    while (current->next) {
        if (current->next == anim) {
            current->next = anim->next;
            memFree(anim, sizeof(tDoorAnim));
            return;
        }
        current = current->next;
    }
}

tDoorAnim* doorAnimFind(tMaze* maze, BYTE x, BYTE y)
{
    tDoorAnim* current = maze->_doorAnims;
    while (current) {
        if (current->x == x && current->y == y) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void doorAnimUpdate(tMaze* maze)
{
    tDoorAnim* current = maze->_doorAnims;
    while (current) {
        tDoorAnim* next = current->next;
        
        // Update timer
        if (++current->timer >= DOOR_ANIM_FRAME_TIME) {
            current->timer = 0;
            
            // Update frame
            if (++current->frame >= DOOR_ANIM_FRAMES) {
                // Animation complete, remove it
                doorAnimRemove(maze, current);
            }
        }
        
        current = next;
    }
}

// Example usage:
// To create a button that opens a door:
// UBYTE doorX = 5, doorY = 5;
// UBYTE buttonX = 3, buttonY = 3;
// UBYTE eventData[2] = {doorX, doorY};
// createEventTrigger(pMaze, buttonX, buttonY, EVENT_OPENDOOR, 2, eventData);