#include "doorbutton.h"
#include "wallset.h"
#include "script.h"
#include <ace/managers/memory.h>
#include <ace/managers/blit.h>
#include <ace/managers/log.h>

#ifndef SOFFX
#define SOFFX 5
#endif

tDoorButton* doorButtonCreate(UBYTE x, UBYTE y, UBYTE wallSide, UBYTE type, UBYTE gfxIndex, UBYTE targetDoorX, UBYTE targetDoorY)
{
    tDoorButton* pButton = (tDoorButton*)memAllocFastClear(sizeof(tDoorButton));
    if (pButton)
    {
        pButton->_x = x;
        pButton->_y = y;
        pButton->_wallSide = wallSide;
        pButton->_state = DOORBUTTON_STATE_OFF;
        pButton->_type = type;
        pButton->_gfxIndex = gfxIndex;
        pButton->_targetDoorX = targetDoorX;
        pButton->_targetDoorY = targetDoorY;
        pButton->_next = NULL;
    }
    return pButton;
}

void doorButtonDestroy(tDoorButton* pButton)
{
    if (pButton)
    {
        memFree(pButton, sizeof(tDoorButton));
    }
}

void doorButtonListCreate(tDoorButtonList* pList)
{
    if (pList)
    {
        pList->_numButtons = 0;
        pList->_buttons = NULL;
    }
}

void doorButtonListDestroy(tDoorButtonList* pList)
{
    if (pList)
    {
        tDoorButton* pButton = pList->_buttons;
        while (pButton)
        {
            tDoorButton* pNext = pButton->_next;
            doorButtonDestroy(pButton);
            pButton = pNext;
        }
        pList->_buttons = NULL;
        pList->_numButtons = 0;
    }
}

void doorButtonAdd(tDoorButtonList* pList, tDoorButton* pButton)
{
    if (!pList || !pButton)
        return;
    
    pButton->_next = pList->_buttons;
    pList->_buttons = pButton;
    pList->_numButtons++;
}

void doorButtonRemove(tDoorButtonList* pList, tDoorButton* pButton)
{
    if (!pList || !pButton)
        return;
    
    if (pList->_buttons == pButton)
    {
        pList->_buttons = pButton->_next;
        pList->_numButtons--;
        doorButtonDestroy(pButton);
        return;
    }
    
    tDoorButton* pCurrent = pList->_buttons;
    while (pCurrent && pCurrent->_next != pButton)
    {
        pCurrent = pCurrent->_next;
    }
    
    if (pCurrent)
    {
        pCurrent->_next = pButton->_next;
        pList->_numButtons--;
        doorButtonDestroy(pButton);
    }
}

UBYTE doorButtonClick(tDoorButton* pButton, tMaze* pMaze)
{
    if (!pButton || !pMaze)
        return 0;
    
    UBYTE doorCell = mazeGetCell(pMaze, pButton->_targetDoorX, pButton->_targetDoorY);
    
    switch (pButton->_type)
    {
        case DOORBUTTON_TYPE_TOGGLE:
            if (doorCell == MAZE_DOOR)
            {
                // Open door
                UBYTE eventData[1] = {0};
                tMazeEvent* pEvent = mazeEventCreate(pButton->_targetDoorX, pButton->_targetDoorY, EVENT_OPENDOOR, 0, eventData);
                handleEvent(pMaze, pEvent);
                mazeRemoveEvent(pMaze, pEvent);
                pButton->_state = DOORBUTTON_STATE_ON;
            }
            else if (doorCell == MAZE_DOOR_OPEN)
            {
                // Close door
                UBYTE eventData[1] = {0};
                tMazeEvent* pEvent = mazeEventCreate(pButton->_targetDoorX, pButton->_targetDoorY, EVENT_CLOSEDOOR, 0, eventData);
                handleEvent(pMaze, pEvent);
                mazeRemoveEvent(pMaze, pEvent);
                pButton->_state = DOORBUTTON_STATE_OFF;
            }
            break;
            
        case DOORBUTTON_TYPE_OPEN_ONLY:
            if (doorCell == MAZE_DOOR || doorCell == MAZE_DOOR_LOCKED)
            {
                UBYTE eventData[1] = {0};
                tMazeEvent* pEvent = mazeEventCreate(pButton->_targetDoorX, pButton->_targetDoorY, EVENT_OPENDOOR, 0, eventData);
                handleEvent(pMaze, pEvent);
                mazeRemoveEvent(pMaze, pEvent);
                pButton->_state = DOORBUTTON_STATE_ON;
            }
            break;
            
        case DOORBUTTON_TYPE_CLOSE_ONLY:
            if (doorCell == MAZE_DOOR_OPEN)
            {
                UBYTE eventData[1] = {0};
                tMazeEvent* pEvent = mazeEventCreate(pButton->_targetDoorX, pButton->_targetDoorY, EVENT_CLOSEDOOR, 0, eventData);
                handleEvent(pMaze, pEvent);
                mazeRemoveEvent(pMaze, pEvent);
                pButton->_state = DOORBUTTON_STATE_ON;
            }
            break;
            
        case DOORBUTTON_TYPE_TRIGGER:
            if (doorCell == MAZE_DOOR || doorCell == MAZE_DOOR_LOCKED)
            {
                UBYTE eventData[1] = {0};
                tMazeEvent* pEvent = mazeEventCreate(pButton->_targetDoorX, pButton->_targetDoorY, EVENT_OPENDOOR, 0, eventData);
                handleEvent(pMaze, pEvent);
                mazeRemoveEvent(pMaze, pEvent);
                pButton->_state = DOORBUTTON_STATE_ON;
            }
            break;
    }
    
    return 1;
}

tDoorButton* doorButtonFindAt(tDoorButtonList* pList, UBYTE x, UBYTE y, UBYTE wallSide)
{
    if (!pList)
        return NULL;
    
    tDoorButton* pButton = pList->_buttons;
    while (pButton)
    {
        if (pButton->_x == x && pButton->_y == y && pButton->_wallSide == wallSide)
        {
            return pButton;
        }
        pButton = pButton->_next;
    }
    return NULL;
}

void doorButtonRender(tDoorButton* pButton, tWallset* pWallset, tBitMap* pBuffer)
{
    if (!pButton || !pWallset || !pBuffer)
        return;
    
    // Find the button graphics in the wallset
    if (pButton->_gfxIndex < pWallset->_tilesetCount)
    {
        tWallGfx* pGfx = pWallset->_tileset[pButton->_gfxIndex];
        if (pGfx && pGfx->_setIndex < pWallset->_gfxCount)
        {
            // Calculate screen position based on button state
            UWORD frameOffset = 0;
            if (pButton->_state == DOORBUTTON_STATE_ON || pButton->_state == DOORBUTTON_STATE_PRESSED)
            {
                frameOffset = pGfx->_height; // Use second frame for pressed state
            }
            
            blitUnsafeCopyMask(
                pWallset->_gfx[pGfx->_setIndex],
                pGfx->_x, pGfx->_y + frameOffset,
                pBuffer,
                pGfx->_screen[0] + SOFFX, pGfx->_screen[1] + SOFFX,
                pGfx->_width, pGfx->_height,
                (UBYTE *)pWallset->_mask[pGfx->_setIndex]->Planes[0]
            );
        }
    }
}

