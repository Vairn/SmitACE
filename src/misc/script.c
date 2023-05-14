#include "script.h"
void handleEvent(tMaze *pMaze, tMazeEvent *pEvent)
{
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
        
        break;
    case EVENT_CLOSEDOOR:

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