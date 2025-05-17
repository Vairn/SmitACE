#include "script.h"
#include "gameState.h"

void handleEvent(tMaze *pMaze, tMazeEvent *pEvent)
{
    // Debug output
    logWrite("Handling event type %d at (%d,%d) with data size %d\n", 
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
        pMaze->_mazeData[pEvent->_x + pEvent->_y * pMaze->_width] = pEvent->_eventData[0];
        pMaze->_mazeCol[pEvent->_x + pEvent->_y * pMaze->_width] = pEvent->_eventData[1];
        break;
    case EVENT_CLEARWALLCOL:
        pMaze->_mazeData[pEvent->_x + pEvent->_y * pMaze->_width] = 0;
        pMaze->_mazeCol[pEvent->_x + pEvent->_y * pMaze->_width] = 0;
        break;
    case EVENT_SHOWMESSAGE:
        
        break;
    case EVENT_OPENDOOR:
        // If event data contains coordinates, use those instead of event position
        if (pEvent->_eventDataSize >= 2) {
            UBYTE doorX = pEvent->_eventData[0];
            UBYTE doorY = pEvent->_eventData[1];
            logWrite("Opening door at (%d,%d)\n", doorX, doorY);
            pMaze->_mazeData[doorX + doorY * pMaze->_width] = MAZE_DOOR_OPEN;
        } else {
            logWrite("Opening door at event position (%d,%d)\n", pEvent->_x, pEvent->_y);
            pMaze->_mazeData[pEvent->_x + pEvent->_y * pMaze->_width] = MAZE_DOOR_OPEN;
        }
        break;
    case EVENT_CLOSEDOOR:
        pMaze->_mazeData[pEvent->_x + pEvent->_y * pMaze->_width] = MAZE_DOOR;
        break;
    case EVENT_BATTERY_CHARGER:
        // Check if charger has power left
        if (pEvent->_eventDataSize > 0 && pEvent->_eventData[0] > 0) {
            // Recharge battery by 1 unit
            g_pGameState->m_pCurrentParty->_BatteryLevel++;
            // Decrease charger power by 1
            pEvent->_eventData[0]--;
            logWrite("Battery recharged to %d, charger has %d units left\n", 
                g_pGameState->m_pCurrentParty->_BatteryLevel, pEvent->_eventData[0]);
        } else {
            logWrite("Battery charger at (%d,%d) is depleted\n", pEvent->_x, pEvent->_y);
        }
        break;
    case EVENT_TELEPORT:

        break;
    case EVENT_GIVEITEM:

        break;
    case EVENT_TAKEITEM:

        break;
    case EVENT_SETFLAG:

        break;
    case EVENT_CLEARFLAG:

        break;
    case EVENT_ADDMONSTER:

        break;
    case EVENT_REMOVEMONSTER:

        break;
    case EVENT_ADDITEM:

        break;
    case EVENT_REMOVEITEM:

        break;
    case EVENT_ADDXP:

        break;
    case EVENT_DAMAGE:

        break;
    case EVENT_LAUNCHER:

        break;
    case EVENT_TURN:

        break;
    case EVENT_IDENTIFYITEMS:

        break;
    case EVENT_ENCOUNTER:

        break;
    case EVENT_SOUND:

        break;
    case EVENT_IF:

        break;
    case EVENT_ELSE:

        break;
    case EVENT_ENDIF:

        break;
    case EVENT_GOTO:

        break;
    case EVENT_GOSUB:

        break;
    case EVENT_RETURN:

        break;
    case EVENT_PARTY_VISIBLE:

        break;
    case EVENT_ROLL_DICE:

        break;
    case EVENT_HAS_CLASS:

        break;
    case EVENT_HAS_RACE:

        break;
    case EVENT_TRIGGER_FLAG:

        break;
    case EVENT_POINTER_ITEM:

        break;
    case EVENT_WALL_SIDE:

        break;
    case EVENT_PARTY_DIRECTION:

        break;
    case EVENT_ELSE_GOTO:

        break;
    case EVENT_LEVEL_FLAG:

        break;
    case EVENT_GLOBAL_FLAG:

        break;
    case EVENT_PARTY_ON_POS:

        break;
    case EVENT_MONSTERS_ON_POS:

        break;
    case EVENT_ITEMS_ON_POS:

        break;
    case EVENT_WALL_NUMBER:

        break;
    case EVENT_OR:

        break;
    case EVENT_AND:

        break;
    case EVENT_GREATER:

        break;
    case EVENT_LESS:

        break;
    case EVENT_NOT_EQUAL:

        break;
    case EVENT_EQUAL:

        break;
    default:
        break;
    }
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

// Example usage:
// To create a button that opens a door:
// UBYTE doorX = 5, doorY = 5;
// UBYTE buttonX = 3, buttonY = 3;
// UBYTE eventData[2] = {doorX, doorY};
// createEventTrigger(pMaze, buttonX, buttonY, EVENT_OPENDOOR, 2, eventData);