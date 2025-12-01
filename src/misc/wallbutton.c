#include "wallbutton.h"
#include "wallset.h"
#include "script.h"
#include <ace/managers/memory.h>
#include <ace/managers/blit.h>
#include <ace/managers/log.h>

#ifndef SOFFX
#define SOFFX 5
#endif

tWallButton* wallButtonCreate(UBYTE x, UBYTE y, UBYTE wallSide, UBYTE type, UBYTE gfxIndex)
{
    tWallButton* pButton = (tWallButton*)memAllocFastClear(sizeof(tWallButton));
    if (pButton)
    {
        pButton->_x = x;
        pButton->_y = y;
        pButton->_wallSide = wallSide;
        pButton->_state = WALLBUTTON_STATE_OFF;
        pButton->_type = type;
        pButton->_gfxIndex = gfxIndex;
        pButton->_eventType = 0;
        pButton->_pEventData = NULL;
        pButton->_eventDataSize = 0;
        pButton->_next = NULL;
    }
    return pButton;
}

void wallButtonDestroy(tWallButton* pButton)
{
    if (pButton)
    {
        if (pButton->_pEventData)
        {
            memFree(pButton->_pEventData, pButton->_eventDataSize);
        }
        memFree(pButton, sizeof(tWallButton));
    }
}

void wallButtonListCreate(tWallButtonList* pList)
{
    if (pList)
    {
        pList->_numButtons = 0;
        pList->_buttons = NULL;
    }
}

void wallButtonListDestroy(tWallButtonList* pList)
{
    if (pList)
    {
        tWallButton* pButton = pList->_buttons;
        while (pButton)
        {
            tWallButton* pNext = pButton->_next;
            wallButtonDestroy(pButton);
            pButton = pNext;
        }
        pList->_buttons = NULL;
        pList->_numButtons = 0;
    }
}

void wallButtonAdd(tWallButtonList* pList, tWallButton* pButton)
{
    if (!pList || !pButton)
        return;
    
    pButton->_next = pList->_buttons;
    pList->_buttons = pButton;
    pList->_numButtons++;
}

void wallButtonRemove(tWallButtonList* pList, tWallButton* pButton)
{
    if (!pList || !pButton)
        return;
    
    if (pList->_buttons == pButton)
    {
        pList->_buttons = pButton->_next;
        pList->_numButtons--;
        wallButtonDestroy(pButton);
        return;
    }
    
    tWallButton* pCurrent = pList->_buttons;
    while (pCurrent && pCurrent->_next != pButton)
    {
        pCurrent = pCurrent->_next;
    }
    
    if (pCurrent)
    {
        pCurrent->_next = pButton->_next;
        pList->_numButtons--;
        wallButtonDestroy(pButton);
    }
}

UBYTE wallButtonClick(tWallButton* pButton, tMaze* pMaze)
{
    if (!pButton || !pMaze)
        return 0;
    
    switch (pButton->_type)
    {
        case WALLBUTTON_TYPE_TOGGLE:
            if (pButton->_state == WALLBUTTON_STATE_OFF)
            {
                pButton->_state = WALLBUTTON_STATE_ON;
            }
            else
            {
                pButton->_state = WALLBUTTON_STATE_OFF;
            }
            break;
            
        case WALLBUTTON_TYPE_MOMENTARY:
            pButton->_state = WALLBUTTON_STATE_PRESSED;
            break;
            
        case WALLBUTTON_TYPE_TRIGGER:
            if (pButton->_state == WALLBUTTON_STATE_OFF)
            {
                pButton->_state = WALLBUTTON_STATE_ON;
            }
            break;
    }
    
    // Trigger event if configured
    if (pButton->_eventType > 0)
    {
        tMazeEvent* pEvent = mazeEventCreate(pButton->_x, pButton->_y, pButton->_eventType, pButton->_eventDataSize, pButton->_pEventData);
        if (pEvent)
        {
            handleEvent(pMaze, pEvent);
            mazeRemoveEvent(pMaze, pEvent);
        }
    }
    
    return 1;
}

tWallButton* wallButtonFindAt(tWallButtonList* pList, UBYTE x, UBYTE y, UBYTE wallSide)
{
    if (!pList)
        return NULL;
    
    tWallButton* pButton = pList->_buttons;
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

void wallButtonRender(tWallButton* pButton, tWallset* pWallset, tBitMap* pBuffer)
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
            if (pButton->_state == WALLBUTTON_STATE_ON || pButton->_state == WALLBUTTON_STATE_PRESSED)
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

