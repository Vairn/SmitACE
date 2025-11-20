#include "game.h"

#include <ace/managers/mouse.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/blit.h>
#include <ace/utils/sprite.h>
#include <ace/managers/bob.h>

#include "gameState.h"
#include "screen.h"
#include "Renderer.h"
#include "mouse_pointer.h"
#include "maze.h"

#include "game_ui.h"
#include "game_ui_regions.h"
#include "script.h"
#include "text_render.h"
#include <string.h>
ULONG seed = 1;
#define SOFFX 5
UBYTE s_lastMoveResult = 0;
ULONG rand()
{
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return seed;
}

void srand(ULONG new_seed)
{
    seed = new_seed;
}

tScreen *pScreen = NULL;
tWallset *pWallset = NULL;
tBitMap *pBackground = NULL;

static ULONG s_ulSampleSize;
UWORD *g_pModSamples;
tPtplayerMod *pMod;

UBYTE g_ubGameActive = 0;
UBYTE g_ubRedrawRequire = 0;
tBob g_pBob;
tBitMap *temp = NULL;

// Track which movement button is currently pressed (for repeat-while-held)
static UBYTE s_ubPressedMovementButton = 0;
static UBYTE s_ubPressedTurnButton = 0;

// Text renderer variables
static tTextRenderer *s_pTextRenderer = NULL;
static tFont *s_pTestFont = NULL;
static UBYTE s_ubTextRendererInitialized = 0;

// Battery hover display and message system
static UBYTE s_ubBatteryHovered = 0;
static UBYTE s_ubLastBatteryLevel = 255;  // Track last battery level to update text when it changes

// Message types
typedef enum {
    MESSAGE_TYPE_SMALL,  // Small text at bottom text field
    MESSAGE_TYPE_VIEWPORT // Block text in middle of 3D viewport
} eMessageType;

// Message system - tracks last 2 messages
#define MAX_MESSAGES 2
static char s_szMessages[MAX_MESSAGES][128];
static tTextBitMap *s_pMessageBitmaps[MAX_MESSAGES];
static tMultiColorText *s_pMessageMultiColorTexts[MAX_MESSAGES];  // Multi-color support for small messages
static eMessageType s_eMessageTypes[MAX_MESSAGES];
static UBYTE s_ubMessageCount = 0;

// Viewport message (only one at a time)
static tTextBitMap *s_pViewportMessageBitmap = NULL;
static char s_szViewportMessage[256];
static UBYTE s_ubViewportMessageActive = 0;
static UBYTE s_ubViewportMessageColor = 7;  // Default color (white)

// Multi-color text for viewport messages (using types from text_render.h)
static tMultiColorText *s_pViewportMultiColorText = NULL;

/**
 * @brief Clear the viewport message
 */
static void clearViewportMessage(void) {
    if (s_pViewportMultiColorText) {
        textRendererDestroyMultiColorText(s_pViewportMultiColorText);
        s_pViewportMultiColorText = NULL;
    }
    if (s_pViewportMessageBitmap) {
        systemUse();
        fontDestroyTextBitMap(s_pViewportMessageBitmap);
        systemUnuse();
        s_pViewportMessageBitmap = NULL;
    }
    s_ubViewportMessageActive = 0;
}

/**
 * @brief Add a message to the message queue (keeps last 2 messages)
 * @param szMessage Message text
 * @param eType Message type (MESSAGE_TYPE_SMALL or MESSAGE_TYPE_VIEWPORT)
 * @param ubColor Color for viewport messages (ignored for small messages)
 */
static void addMessage(const char *szMessage, eMessageType eType, UBYTE ubColor) {
    // For small messages, color is ignored (determined at draw time)
    if (!s_ubTextRendererInitialized || !s_pTextRenderer) return;
    
    if (eType == MESSAGE_TYPE_VIEWPORT) {
        // Clean up old viewport message
        if (s_pViewportMessageBitmap) {
            systemUse();
            fontDestroyTextBitMap(s_pViewportMessageBitmap);
            systemUnuse();
            s_pViewportMessageBitmap = NULL;
        }
        
        // Clean up old multi-color text
        if (s_pViewportMultiColorText) {
            textRendererDestroyMultiColorText(s_pViewportMultiColorText);
            s_pViewportMultiColorText = NULL;
        }
        
        strncpy(s_szViewportMessage, szMessage, 255);
        s_szViewportMessage[255] = '\0';
        
        // Create multi-color text (handles both single-color and multi-color)
        // Max width 220px for wrapping
        systemUse();
        s_pViewportMultiColorText = textRendererCreateMultiColorText(s_pTextRenderer, s_szViewportMessage, ubColor, 220);
        
        // If multi-color text creation failed or has no segments, fall back to single bitmap
        if (!s_pViewportMultiColorText || s_pViewportMultiColorText->ubSegmentCount == 0) {
            if (s_pViewportMultiColorText) {
                textRendererDestroyMultiColorText(s_pViewportMultiColorText);
                s_pViewportMultiColorText = NULL;
            }
            // Fallback to single-color bitmap if multi-color creation failed
            s_pViewportMessageBitmap = textRendererCreateText(s_pTextRenderer, s_szViewportMessage, 230, TEXT_JUSTIFY_CENTER);
            s_ubViewportMessageColor = ubColor;
        }
        systemUnuse();
        s_ubViewportMessageActive = 1;
        return;
    }
    
    // Small messages go to bottom text field
    // Shift messages down (oldest first)
    if (s_ubMessageCount >= MAX_MESSAGES) {
        // Destroy oldest message bitmap
        if (s_pMessageBitmaps[0]) {
            systemUse();
            fontDestroyTextBitMap(s_pMessageBitmaps[0]);
            systemUnuse();
        }
        // Destroy oldest multi-color text
        if (s_pMessageMultiColorTexts[0]) {
            textRendererDestroyMultiColorText(s_pMessageMultiColorTexts[0]);
        }
        // Move messages up
        for (UBYTE i = 0; i < MAX_MESSAGES - 1; i++) {
            s_pMessageBitmaps[i] = s_pMessageBitmaps[i + 1];
            s_pMessageMultiColorTexts[i] = s_pMessageMultiColorTexts[i + 1];
            s_eMessageTypes[i] = s_eMessageTypes[i + 1];
            strcpy(s_szMessages[i], s_szMessages[i + 1]);
        }
        s_ubMessageCount = MAX_MESSAGES - 1;
    }
    
    // Add new message
    strncpy(s_szMessages[s_ubMessageCount], szMessage, 127);
    s_szMessages[s_ubMessageCount][127] = '\0';
    s_eMessageTypes[s_ubMessageCount] = eType;
    
    // Check if message has color markup
    UBYTE ubHasColorMarkup = 0;
    UWORD uwMsgLen = strlen(szMessage);
    for (UWORD i = 0; i < uwMsgLen - 3; i++) {
        if (szMessage[i] == '{' && szMessage[i+1] == 'c' && szMessage[i+2] == ':') {
            ubHasColorMarkup = 1;
            break;
        }
    }
    
    // Create text bitmap or multi-color text for new message
    systemUse();
    if (ubHasColorMarkup) {
        // Use multi-color text for messages with color markup
        s_pMessageMultiColorTexts[s_ubMessageCount] = textRendererCreateMultiColorText(s_pTextRenderer, s_szMessages[s_ubMessageCount], 1, 240);
        s_pMessageBitmaps[s_ubMessageCount] = NULL;
    } else {
        // Use single-color bitmap
        s_pMessageBitmaps[s_ubMessageCount] = textRendererCreateText(s_pTextRenderer, s_szMessages[s_ubMessageCount], 240, TEXT_JUSTIFY_LEFT);
        s_pMessageMultiColorTexts[s_ubMessageCount] = NULL;
    }
    systemUnuse();
    
    s_ubMessageCount++;
}

void handleEquipmentClicked(WORD slotID)
{
}

void handleEquipmentUsed(WORD slotID)
{
}

void handleInventoryClicked(WORD slotID)
{
}

void handleInventoryScrolled(WORD direction, UBYTE page)
{
}

void handleMapClicked(void)
{
}

void TurnRight()
{
    g_pGameState->m_pCurrentParty->_PartyFacing++;
    g_pGameState->m_pCurrentParty->_PartyFacing %= 4;
    g_ubRedrawRequire = 2;
}

void TurnLeft()
{
    g_pGameState->m_pCurrentParty->_PartyFacing--;
    g_pGameState->m_pCurrentParty->_PartyFacing %= 4;
    g_ubRedrawRequire = 2;
}

static void handleEventTrigger(void)
{
    if (s_lastMoveResult == 3) {
        tMazeEvent* pEvent = mazeFindEventAtPosition(g_pGameState->m_pCurrentMaze, 
            g_pGameState->m_pCurrentParty->_PartyX, g_pGameState->m_pCurrentParty->_PartyY);
        if (pEvent) {
            handleEvent(g_pGameState->m_pCurrentMaze, pEvent);
        }
    }
}

static void updateStandingOnEventTrigger(void)
{
    // Check if player is standing on an event trigger cell
    UBYTE cellType = mazeGetCell(g_pGameState->m_pCurrentMaze, 
        g_pGameState->m_pCurrentParty->_PartyX, 
        g_pGameState->m_pCurrentParty->_PartyY);
    
    if (cellType == MAZE_EVENT_TRIGGER) {
        tMazeEvent* pEvent = mazeFindEventAtPosition(g_pGameState->m_pCurrentMaze, 
            g_pGameState->m_pCurrentParty->_PartyX, 
            g_pGameState->m_pCurrentParty->_PartyY);
        if (pEvent && pEvent->_eventType == EVENT_BATTERY_CHARGER) {
            // Only charge if charger has charge left
            handleEvent(g_pGameState->m_pCurrentMaze, pEvent);
        }
    }
}

void MoveRight()
{
    // Clear viewport message when moving
    clearViewportMessage();
    
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing + 1;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    handleEventTrigger();
    g_ubRedrawRequire = 2;
}

void MoveLeft()
{
    // Clear viewport message when moving
    clearViewportMessage();
    
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing + 3;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    handleEventTrigger();
    g_ubRedrawRequire = 2;
}

void MoveBackwards()
{
    // Clear viewport message when moving
    clearViewportMessage();
    
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing + 2;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    handleEventTrigger();
    g_ubRedrawRequire = 2;
}

void MoveForwards()
{
    // Clear viewport message when moving
    clearViewportMessage();
    
    UBYTE modFace = g_pGameState->m_pCurrentParty->_PartyFacing;
    modFace %= 4;
    s_lastMoveResult = mazeMove(g_pGameState->m_pCurrentMaze, g_pGameState->m_pCurrentParty, modFace);
    handleEventTrigger();
    g_ubRedrawRequire = 2;
}

void cbGameOnUnhovered(Region *pRegion)
{
    UBYTE id = ((UBYTE)(ULONG)pRegion->context);
    if (id == GAME_UI_GADGET_BATTERY)
    {
        s_ubBatteryHovered = 0;
    }
}

void cbGameOnHovered(Region *pRegion)
{
    UBYTE id = ((UBYTE)(ULONG)pRegion->context);
    if (id == GAME_UI_GADGET_BATTERY)
    {
        s_ubBatteryHovered = 1;
    }
}

void handleDoorClick(UBYTE x, UBYTE y)
{
    tMaze* pMaze = g_pGameState->m_pCurrentMaze;
    UBYTE cellType = mazeGetCell(pMaze, x, y);
    
    switch (cellType)
    {
    case MAZE_DOOR:
        // Create door event to open door (uses event position, no data needed)
        {
            UBYTE eventData[1] = {0};
            tMazeEvent* pEvent = mazeEventCreate(x, y, EVENT_OPENDOOR, 0, eventData);
            handleEvent(pMaze, pEvent);
            mazeRemoveEvent(pMaze, pEvent);
            g_ubRedrawRequire = 2;
        }
        break;
        
    case MAZE_DOOR_OPEN:
        // Create door event to close door (uses event position, no data needed)
        {
            UBYTE eventData[1] = {0};
            tMazeEvent* pEvent = mazeEventCreate(x, y, EVENT_CLOSEDOOR, 0, eventData);
            handleEvent(pMaze, pEvent);
            mazeRemoveEvent(pMaze, pEvent);
            g_ubRedrawRequire = 2;
        }
        break;
        
    case MAZE_DOOR_LOCKED:
        // Check if player has key (you can implement key checking logic here)
        // For now, just show a message that the door is locked
        // You can add a message event here
        break;
    }
}

void cbGameOnPressed(Region *pRegion, UBYTE ubLeft, UBYTE ubRight)
{
    UBYTE id = ((UBYTE)(ULONG)pRegion->context);
    switch (id)
    {
    case GAME_UI_GADGET_FORWARD:
        if (ubLeft) {
            s_ubPressedMovementButton = GAME_UI_GADGET_FORWARD;
            MoveForwards();
        }
        break;
    case GAME_UI_GADGET_BACKWARD:
        if (ubLeft) {
            s_ubPressedMovementButton = GAME_UI_GADGET_BACKWARD;
            MoveBackwards();
        }
        break;
    case GAME_UI_GADGET_LEFT:
        if (ubLeft) {
            s_ubPressedMovementButton = GAME_UI_GADGET_LEFT;
            MoveLeft();
        }
        break;
    case GAME_UI_GADGET_RIGHT:
        if (ubLeft) {
            s_ubPressedMovementButton = GAME_UI_GADGET_RIGHT;
            MoveRight();
        }
        break;
    case GAME_UI_GADGET_TURN_LEFT:
        if (ubLeft) {
            s_ubPressedTurnButton = GAME_UI_GADGET_TURN_LEFT;
            TurnLeft();
        }
        break;
    case GAME_UI_GADGET_TURN_RIGHT:
        if (ubLeft) {
            s_ubPressedTurnButton = GAME_UI_GADGET_TURN_RIGHT;
            TurnRight();
        }
        break;
    case GAME_UI_GADGET_EQUIPMENT_1:
    case GAME_UI_GADGET_EQUIPMENT_2:
    case GAME_UI_GADGET_EQUIPMENT_3:
    case GAME_UI_GADGET_EQUIPMENT_4:
        if (ubLeft)
            handleEquipmentClicked(id - GAME_UI_GADGET_EQUIPMENT_1);
        if (ubRight)
            handleEquipmentUsed(id - GAME_UI_GADGET_EQUIPMENT_1);
        break;
    case GAME_UI_GADGET_INV_SLOT_1:
    case GAME_UI_GADGET_INV_SLOT_2:
    case GAME_UI_GADGET_INV_SLOT_3:
    case GAME_UI_GADGET_INV_SLOT_4:
    case GAME_UI_GADGET_INV_SLOT_5:
    case GAME_UI_GADGET_INV_SLOT_6:
        if (ubLeft)
            handleInventoryClicked(id - GAME_UI_GADGET_INV_SLOT_1);
        break;
    case GAME_UI_GADGET_INV_DOWN:
    case GAME_UI_GADGET_INV_UP:
        handleInventoryScrolled(id - GAME_UI_GADGET_INV_UP, ubRight);
        break;
    case GAME_UI_GADGET_MAP:
        handleMapClicked();
        break;
    case VIEWPORT_UI_GADGET_DOOR:
        if (ubLeft)
        {
            // Get the door position based on player's position and facing
            UBYTE doorX = g_pGameState->m_pCurrentParty->_PartyX;
            UBYTE doorY = g_pGameState->m_pCurrentParty->_PartyY;
            switch (g_pGameState->m_pCurrentParty->_PartyFacing)
            {
            case 0: // North
                doorY--;
                break;
            case 1: // East
                doorX++;
                break;
            case 2: // South
                doorY++;
                break;
            case 3: // West
                doorX--;
                break;
            }
            handleDoorClick(doorX, doorY);
        }
        break;
    default:
        break;
    }
}

void cbGameOnReleased(Region *pRegion, UBYTE ubLeft, UBYTE ubRight)
{
    UBYTE id = ((UBYTE)(ULONG)pRegion->context);
    // Clear pressed movement button if this region was released
    if (id == s_ubPressedMovementButton) {
        s_ubPressedMovementButton = 0;
    }
    // Clear pressed turn button if this region was released
    if (id == s_ubPressedTurnButton) {
        s_ubPressedTurnButton = 0;
    }
}

static void fadeCompleteNoBat(void)
{
    ptplayerEnableMusic(0);
    ptplayerModDestroy(pMod);

    stateChange(g_pStateMachineGame, &g_sStateGameOver);
}

static void fadeInComplete(void)
{
    g_ubGameActive = 1;
}
static void gameGsCreate(void)
{
    systemUse();
    InitNewGame();
    pScreen = ScreenGetActive();

    g_pGameState->m_pCurrentMaze = mazeCreateDemoData();
    g_pGameState->m_pCurrentParty->_PartyX = 2;
    g_pGameState->m_pCurrentParty->_PartyY = 2;
    g_pGameState->m_pCurrentParty->_PartyFacing = 0;

    pWallset = wallsetLoad("data/factory2/factory2.wll");
    g_pGameState->m_pCurrentWallset = pWallset;

    for (int p = 0; p < 256; p++)
    {
        ULONG *pPalRef = (ULONG *)pScreen->_pFade->pPaletteRef;
        pPalRef[p] = pWallset->_palette[3 * p] << 16 | pWallset->_palette[(3 * p) + 1] << 8 | pWallset->_palette[(3 * p + 2)];
    }
    
    pMod = ptplayerModCreateFromPath("data/suspense.mod");
    ptplayerLoadMod(pMod, NULL, 0);
    ptplayerSetMasterVolume(64);
    ptplayerEnableMusic(1);
    
    // Initialize text renderer (if font file exists)
    s_pTestFont = fontCreateFromPath("data/font.fnt");
    if (s_pTestFont) {
        s_pTextRenderer = textRendererCreate(s_pTestFont);
        if (s_pTextRenderer) {
            s_ubTextRendererInitialized = 1;
        }
    }
    
    tBitMap *pPlayfield = bitmapCreateFromPath("data/playfield.bm", 0);
    blitCopyAligned(pPlayfield, 0, 0, pScreen->_pBfr->pBack, 0, 0, 320, 256);
    // ScreenUpdate();
    blitCopyAligned(pPlayfield, 0, 0, pScreen->_pBfr->pFront, 0, 0, 320, 256);
    bitmapDestroy(pPlayfield);
    // do an initial render to both front and back.
    drawView(g_pGameState, pScreen->_pBfr->pBack);
    drawView(g_pGameState, pScreen->_pBfr->pFront);
    
    
    systemUnuse();

    gameUIInit(cbGameOnHovered, cbGameOnUnhovered, cbGameOnPressed, cbGameOnReleased);
    Layer *pLayer = gameUIGetLayer();
    layerEnablePointerUpdate(pLayer, 1);
    // viewUpdateCLUT(pScreen->_pView);
    ScreenUpdate();
    ScreenFadeFromBlack(NULL, 7, fadeInComplete); // 7 is the speed of the fade
}

static void gameGsLoop(void)
{
    if (g_ubGameActive)
    {
        gameUIUpdate();

        if (g_pGameState->m_pCurrentParty->_BatteryLevel <= 0)
        {
            ScreenFadeToBlack(NULL, 7, fadeCompleteNoBat);
            g_ubGameActive = 0;
            g_ubRedrawRequire = 0;
        }

        // Update door animations
        UBYTE animationsCompleted = doorAnimUpdate(g_pGameState->m_pCurrentMaze);
        
        // If there are active door animations or one just completed, we need to redraw
        if (g_pGameState->m_pCurrentMaze->_doorAnims != NULL || animationsCompleted) {
            g_ubRedrawRequire = 2;
        }
        
        // Update battery chargers (slowly recharge over time)
        updateBatteryChargers(g_pGameState->m_pCurrentMaze);
        
        // Check if standing on event triggers (e.g., battery chargers)
        updateStandingOnEventTrigger();

        // Update monsters
        for (UBYTE i = 0; i < g_pGameState->m_pMonsterList->_numMonsters; i++) {
            tMonster* monster = g_pGameState->m_pMonsterList->_monsters[i];
            if (monster && monster->_state != MONSTER_STATE_DEAD) {
                // Update monster state
                monsterUpdate(monster, g_pGameState->m_pCurrentParty);

                // Check for combat
                if (monster->_state == MONSTER_STATE_AGGRESSIVE) {
                    // If monster is in same cell as party, initiate combat
                    if (monster->_partyPosX == g_pGameState->m_pCurrentParty->_PartyX &&
                        monster->_partyPosY == g_pGameState->m_pCurrentParty->_PartyY) {
                        // Monster attacks first character in party
                        if (g_pGameState->m_pCurrentParty->_numCharacters > 0) {
                            monsterAttack(monster, g_pGameState->m_pCurrentParty->_characters[0]);
                            
                            // Check if monster was defeated
                            if (monster->_base._HP == 0) {
                                monster->_state = MONSTER_STATE_DEAD;
                                monsterDropLoot(monster);
                                // Give experience to party
                                for (UBYTE j = 0; j < g_pGameState->m_pCurrentParty->_numCharacters; j++) {
                                    g_pGameState->m_pCurrentParty->_characters[j]->_Experience += monster->_experienceValue;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (g_ubRedrawRequire)
        {
            drawView(g_pGameState, pScreen->_pBfr->pBack);
            g_ubRedrawRequire--;
        }

        gameUpdateBattery(g_pGameState->m_pCurrentParty->_BatteryLevel);
        
        // Update battery percentage message when hovered
        static UBYTE s_ubLastHoverState = 0;
        if (s_ubBatteryHovered && s_ubTextRendererInitialized && s_pTextRenderer) {
            UBYTE ubBatteryLevel = g_pGameState->m_pCurrentParty->_BatteryLevel;
            
            // Add message when first hovered or if battery level changed
            if (!s_ubLastHoverState || ubBatteryLevel != s_ubLastBatteryLevel) {
                s_ubLastHoverState = 1;
                // Create battery percentage message
                char szBatteryText[32];
                // Format: "Battery: XX%" or "Battery: XXX%" for 100%
                szBatteryText[0] = 'B';
                szBatteryText[1] = 'a';
                szBatteryText[2] = 't';
                szBatteryText[3] = 't';
                szBatteryText[4] = 'e';
                szBatteryText[5] = 'r';
                szBatteryText[6] = 'y';
                szBatteryText[7] = ':';
                szBatteryText[8] = ' ';
                
                if (ubBatteryLevel == 100) {
                    // Handle 100% case
                    szBatteryText[9] = '1';
                    szBatteryText[10] = '0';
                    szBatteryText[11] = '0';
                    szBatteryText[12] = '%';
                    szBatteryText[13] = '\0';
                } else {
                    // Handle 0-99%
                    UBYTE tens = ubBatteryLevel / 10;
                    UBYTE ones = ubBatteryLevel % 10;
                    szBatteryText[9] = '0' + tens;
                    szBatteryText[10] = '0' + ones;
                    szBatteryText[11] = '%';
                    szBatteryText[12] = '\0';
                }
                
                addMessage(szBatteryText, MESSAGE_TYPE_SMALL, 1);
                s_ubLastBatteryLevel = ubBatteryLevel;
            }
        } else {
            // Reset hover state tracking when not hovered
            s_ubLastHoverState = 0;
        }
        
        // Clear text field area before drawing (y=234, height=21, width=246, x=2)
        blitRect(pScreen->_pBfr->pBack, 2, 234, 246, 21, 0);
        
        // Draw messages in text field at bottom (y=234, height=21, width=246)
        // Only show small messages, latest at bottom
        if (s_ubTextRendererInitialized && s_ubMessageCount > 0) {
            UWORD uwTextFieldY = 234;
            UWORD uwFontHeight = s_pTestFont->uwHeight;
            UWORD uwTextX = 4;  // Small margin from left edge
            UWORD uwTextFieldBottom = uwTextFieldY + 21;  // Bottom of text field (234 + 21 = 255)
            
            // Collect small message indices (oldest to newest order)
            UBYTE ubSmallMessageIndices[MAX_MESSAGES];
            UBYTE ubSmallMessageCount = 0;
            
            // Collect all small messages in order
            for (UBYTE i = 0; i < s_ubMessageCount && ubSmallMessageCount < MAX_MESSAGES; i++) {
                if (s_eMessageTypes[i] == MESSAGE_TYPE_SMALL) {
                    ubSmallMessageIndices[ubSmallMessageCount] = i;
                    ubSmallMessageCount++;
                }
            }
            
            // Draw the last 2 small messages (newest at bottom)
            // ubSmallMessageIndices[0] is oldest, ubSmallMessageIndices[ubSmallMessageCount-1] is newest
            // We want to draw the last 2, so start from the end
            UBYTE ubStartIdx = (ubSmallMessageCount > MAX_MESSAGES) ? (ubSmallMessageCount - MAX_MESSAGES) : 0;
            UBYTE ubDrawCount = (ubSmallMessageCount > MAX_MESSAGES) ? MAX_MESSAGES : ubSmallMessageCount;
            
            for (UBYTE drawIdx = 0; drawIdx < ubDrawCount; drawIdx++) {
                UBYTE msgIdx = ubSmallMessageIndices[ubStartIdx + drawIdx];
                
                // Check if this is a multi-color message
                if (s_pMessageMultiColorTexts[msgIdx] && s_pMessageMultiColorTexts[msgIdx]->ubSegmentCount > 0) {
                    // Draw multi-color message
                    UBYTE linesFromBottom = ubDrawCount - drawIdx;
                    UWORD uwY = uwTextFieldBottom - (uwFontHeight * linesFromBottom);
                    
                    // Draw each segment
                    for (UBYTE i = 0; i < s_pMessageMultiColorTexts[msgIdx]->ubSegmentCount; i++) {
                        if (s_pMessageMultiColorTexts[msgIdx]->aSegments[i].pBitmap) {
                            UWORD uwSegX = uwTextX + s_pMessageMultiColorTexts[msgIdx]->aSegments[i].uwX;
                            UWORD uwSegY = uwY + s_pMessageMultiColorTexts[msgIdx]->aSegments[i].uwY;
                            
                            if (uwSegY >= uwTextFieldY && uwSegY + uwFontHeight <= uwTextFieldBottom) {
                                fontDrawTextBitMap(pScreen->_pBfr->pBack, 
                                    s_pMessageMultiColorTexts[msgIdx]->aSegments[i].pBitmap, 
                                    uwSegX, uwSegY, 
                                    s_pMessageMultiColorTexts[msgIdx]->aSegments[i].ubColor, 
                                    FONT_COOKIE);
                            }
                        }
                    }
                } else if (s_pMessageBitmaps[msgIdx]) {
                    // Draw single-color message
                    // Determine color
                    UBYTE ubColor = 1;  // Default color
                    if (s_ubBatteryHovered && s_ubLastBatteryLevel != 255) {
                        UBYTE ubBatteryLevel = s_ubLastBatteryLevel;
                        if (ubBatteryLevel < 25) {
                            ubColor = 4;  // Red when low
                        } else if (ubBatteryLevel < 50) {
                            ubColor = 3;  // Yellow when medium
                        } else {
                            ubColor = 2;  // Green when high
                        }
                    }
                    
                    // Position: newest at bottom, oldest above
                    // drawIdx=0 is oldest of the 2, drawIdx=ubDrawCount-1 is newest
                    // Calculate Y from bottom: newest is 1 line from bottom, oldest is 2 lines from bottom
                    UBYTE linesFromBottom = ubDrawCount - drawIdx;  // 1=newest (bottom), 2=oldest (above)
                    UWORD uwY = uwTextFieldBottom - (uwFontHeight * linesFromBottom);
                    
                    // Ensure message fits within text field bounds
                    if (uwY >= uwTextFieldY && uwY + uwFontHeight <= uwTextFieldBottom) {
                        // Draw the message
                        fontDrawTextBitMap(pScreen->_pBfr->pBack, s_pMessageBitmaps[msgIdx], uwTextX, uwY, ubColor, FONT_COOKIE);
                    }
                }
            }
        }
        
        // Draw viewport message in center of 3D viewport (if active)
        // Viewport is at (1, 28) with size 248x160, but drawing area is (5, 5) with size 240x180
        if (s_ubTextRendererInitialized && s_ubViewportMessageActive) {
            UWORD uwViewportCenterX = 5 + (240 / 2);  // Center of viewport
            UWORD uwViewportCenterY = 5 + (180 / 2);  // Center of viewport
            
            if (s_pViewportMultiColorText && s_pViewportMultiColorText->ubSegmentCount > 0) {
                // Multi-color message: use pre-calculated dimensions from text renderer
                UWORD uwTotalWidth = s_pViewportMultiColorText->uwTotalWidth;
                UWORD uwTotalHeight = s_pViewportMultiColorText->uwTotalHeight;
                
                // Limit width to maximum of 220 pixels
                if (uwTotalWidth > 220) {
                    uwTotalWidth = 220;
                }
                
                // Add padding around text for background
                UWORD uwPadding = 8;
                UWORD uwBgWidth = uwTotalWidth + (uwPadding * 2);
                UWORD uwBgHeight = uwTotalHeight + (uwPadding * 2);
                
                // Center the background
                UWORD uwBgX = uwViewportCenterX - (uwBgWidth / 2);
                UWORD uwBgY = uwViewportCenterY - (uwBgHeight / 2);
                
                // Calculate centering offset for text (center text within background)
                UWORD uwTextOffsetX = (uwBgWidth - uwTotalWidth) / 2;
                
                // Draw background rectangle (color 0 = black/dark background)
                blitRect(pScreen->_pBfr->pBack, uwBgX, uwBgY, uwBgWidth, uwBgHeight, 0);
                
                // Draw each segment with its color
                for (UBYTE i = 0; i < s_pViewportMultiColorText->ubSegmentCount; i++) {
                    if (s_pViewportMultiColorText->aSegments[i].pBitmap) {
                        UWORD uwTextX = uwBgX + uwTextOffsetX + s_pViewportMultiColorText->aSegments[i].uwX;
                        UWORD uwTextY = uwBgY + uwPadding + s_pViewportMultiColorText->aSegments[i].uwY;
                        
                        // Only draw if segment is within the 220px width limit
                        if (s_pViewportMultiColorText->aSegments[i].uwX + s_pViewportMultiColorText->aSegments[i].pBitmap->uwActualWidth <= 220) {
                            fontDrawTextBitMap(pScreen->_pBfr->pBack, 
                                s_pViewportMultiColorText->aSegments[i].pBitmap, 
                                uwTextX, uwTextY, 
                                s_pViewportMultiColorText->aSegments[i].ubColor, 
                                FONT_COOKIE);
                        }
                    }
                }
            } else if (s_pViewportMessageBitmap) {
                // Single color message
                UWORD uwTextWidth = s_pViewportMessageBitmap->uwActualWidth;
                UWORD uwTextHeight = s_pViewportMessageBitmap->uwActualHeight;
                
                // Add padding around text for background
                UWORD uwPadding = 8;
                UWORD uwBgWidth = uwTextWidth + (uwPadding * 2);
                UWORD uwBgHeight = uwTextHeight + (uwPadding * 2);
                
                // Center the background
                UWORD uwBgX = uwViewportCenterX - (uwBgWidth / 2);
                UWORD uwBgY = uwViewportCenterY - (uwBgHeight / 2);
                
                // Center the text on the background (text is already centered within bitmap)
                UWORD uwTextX = uwBgX + uwPadding;
                UWORD uwTextY = uwBgY + uwPadding;
                
                // Draw background rectangle (color 0 = black/dark background)
                blitRect(pScreen->_pBfr->pBack, uwBgX, uwBgY, uwBgWidth, uwBgHeight, 0);
                
                // Draw text on top of background with stored color
                fontDrawTextBitMap(pScreen->_pBfr->pBack, s_pViewportMessageBitmap, uwTextX, uwTextY, s_ubViewportMessageColor, FONT_COOKIE);
            }
        }
        
        // Process mouse multiple times per frame for better responsiveness
        mouseProcess();
        mouseProcess();
        
        // Check if a movement button is still pressed (repeat-while-held, like EOB/DM)
        if (s_ubPressedMovementButton != 0 && mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
            UWORD uwMouseX = mouseGetX(MOUSE_PORT_1);
            UWORD uwMouseY = mouseGetY(MOUSE_PORT_1);
            UBYTE ubOverButton = 0;
            
            // Check if mouse is still over the pressed button (using known button coordinates)
            switch (s_ubPressedMovementButton) {
            case GAME_UI_GADGET_FORWARD:
                ubOverButton = (uwMouseX >= 25 && uwMouseX < 43 && uwMouseY >= 192 && uwMouseY < 210);
                break;
            case GAME_UI_GADGET_BACKWARD:
                ubOverButton = (uwMouseX >= 25 && uwMouseX < 43 && uwMouseY >= 212 && uwMouseY < 230);
                break;
            case GAME_UI_GADGET_LEFT:
                ubOverButton = (uwMouseX >= 5 && uwMouseX < 23 && uwMouseY >= 212 && uwMouseY < 230);
                break;
            case GAME_UI_GADGET_RIGHT:
                ubOverButton = (uwMouseX >= 45 && uwMouseX < 63 && uwMouseY >= 212 && uwMouseY < 230);
                break;
            }
            
            if (ubOverButton) {
                // Button still pressed, trigger movement each frame
                switch (s_ubPressedMovementButton) {
                case GAME_UI_GADGET_FORWARD:
                    MoveForwards();
                    break;
                case GAME_UI_GADGET_BACKWARD:
                    MoveBackwards();
                    break;
                case GAME_UI_GADGET_LEFT:
                    MoveLeft();
                    break;
                case GAME_UI_GADGET_RIGHT:
                    MoveRight();
                    break;
                }
            } else {
                // Mouse moved away from button, clear pressed state
                s_ubPressedMovementButton = 0;
            }
        }
        
        // Check if a turn button is still pressed (repeat-while-held)
        if (s_ubPressedTurnButton != 0 && mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
            UWORD uwMouseX = mouseGetX(MOUSE_PORT_1);
            UWORD uwMouseY = mouseGetY(MOUSE_PORT_1);
            UBYTE ubOverButton = 0;
            
            // Check if mouse is still over the pressed button
            switch (s_ubPressedTurnButton) {
            case GAME_UI_GADGET_TURN_LEFT:
                ubOverButton = (uwMouseX >= 5 && uwMouseX < 23 && uwMouseY >= 192 && uwMouseY < 210);
                break;
            case GAME_UI_GADGET_TURN_RIGHT:
                ubOverButton = (uwMouseX >= 45 && uwMouseX < 63 && uwMouseY >= 192 && uwMouseY < 210);
                break;
            }
            
            if (ubOverButton) {
                // Button still pressed, trigger turn each frame
                switch (s_ubPressedTurnButton) {
                case GAME_UI_GADGET_TURN_LEFT:
                    TurnLeft();
                    break;
                case GAME_UI_GADGET_TURN_RIGHT:
                    TurnRight();
                    break;
                }
            } else {
                // Mouse moved away from button, clear pressed state
                s_ubPressedTurnButton = 0;
            }
        }
        
        if (keyCheck(KEY_ESCAPE))
            gameExit();

        if (keyCheck(KEY_W) || keyCheck(KEY_UP))
        {
            MoveForwards();
        }
        if (keyCheck(KEY_S) || keyCheck(KEY_DOWN))
        {
            MoveBackwards();
        }
        if (keyCheck(KEY_A) || keyCheck(KEY_LEFT))
        {
            MoveLeft();
        }
        if (keyCheck(KEY_D) || keyCheck(KEY_RIGHT))
        {
            MoveRight();
        }

        if (keyCheck(KEY_Q) || keyCheck(KEY_HELP))
        {
            TurnLeft();
        }
        if (keyCheck(KEY_E) || keyCheck(KEY_DEL))
        {
            TurnRight();
        }
        
        // Test events for message system
        static UBYTE s_ubF1Pressed = 0;
        static UBYTE s_ubF2Pressed = 0;
        
        // F1 - Test small message (appears in bottom text field)
        if (keyCheck(KEY_F1)) {
            if (!s_ubF1Pressed) {
                s_ubF1Pressed = 1;
                if (s_ubTextRendererInitialized) {
                    addMessage("Test small message: F1 pressed!", MESSAGE_TYPE_SMALL, 1);
                }
            }
        } else {
            s_ubF1Pressed = 0;
        }
        
        // F2 - Test viewport message (appears in center of 3D viewport)
        if (keyCheck(KEY_F2)) {
            if (!s_ubF2Pressed) {
                s_ubF2Pressed = 1;
                if (s_ubTextRendererInitialized) {
                    addMessage("This is a viewport message!\nIt appears in the center\nof the 3D viewport.\nPress F2 again to change it.", MESSAGE_TYPE_VIEWPORT, 7);
                }
            }
        } else {
            s_ubF2Pressed = 0;
        }
        
        // F3 - Test multi-color viewport message
        static UBYTE s_ubF3Pressed = 0;
        if (keyCheck(KEY_F3)) {
            if (!s_ubF3Pressed) {
                s_ubF3Pressed = 1;
                if (s_ubTextRendererInitialized) {
                    // Example with color markup: {c:2}green text{c:7} white text{c:4} red text
                    addMessage("{c:2}MULTI{c:7}-{c:4}COLOR{c:7} MESSAGE!\n\n{c:2}Green{c:7} word, {c:3}Yellow{c:7} word, {c:4}Red{c:7} word!", MESSAGE_TYPE_VIEWPORT, 7);
                }
            }
        } else {
            s_ubF3Pressed = 0;
        }
        
        // F4 - Test rainbow message (all 16 colors)
        static UBYTE s_ubF4Pressed = 0;
        if (keyCheck(KEY_F4)) {
            if (!s_ubF4Pressed) {
                s_ubF4Pressed = 1;
                if (s_ubTextRendererInitialized) {
                    // Rainbow message showing all 16 colors
                    addMessage("{c:0}0 {c:1}1 {c:2}2 {c:3}3 {c:4}4 {c:5}5 {c:6}6 {c:7}7\n{c:8}8 {c:9}9 {c:10}10 {c:11}11 {c:12}12 {c:13}13 {c:14}14 {c:15}15", MESSAGE_TYPE_VIEWPORT, 7);
                }
            }
        } else {
            s_ubF4Pressed = 0;
        }
        
        // F5 - Test multi-color damage message
        static UBYTE s_ubF5Pressed = 0;
        if (keyCheck(KEY_F5)) {
            if (!s_ubF5Pressed) {
                s_ubF5Pressed = 1;
                if (s_ubTextRendererInitialized) {
                    // Multi-color damage message: "bob deals 13 damage"
                    addMessage("{c:7}bob {c:1}deals {c:4}13 {c:1}damage", MESSAGE_TYPE_VIEWPORT, 7);
                }
            }
        } else {
            s_ubF5Pressed = 0;
        }
    }

    ScreenUpdate();
}

static void gameGsDestroy(void)
{
    systemUse();
    
    // Clean up message bitmaps
    for (UBYTE i = 0; i < MAX_MESSAGES; i++) {
        if (s_pMessageBitmaps[i]) {
            fontDestroyTextBitMap(s_pMessageBitmaps[i]);
            s_pMessageBitmaps[i] = NULL;
        }
        if (s_pMessageMultiColorTexts[i]) {
            textRendererDestroyMultiColorText(s_pMessageMultiColorTexts[i]);
            s_pMessageMultiColorTexts[i] = NULL;
        }
    }
    s_ubMessageCount = 0;
    // Initialize multi-color text arrays
    for (UBYTE i = 0; i < MAX_MESSAGES; i++) {
        s_pMessageMultiColorTexts[i] = NULL;
    }
    
    // Clean up viewport message
    clearViewportMessage();
    systemUnuse();
    
    if (s_pTextRenderer) {
        textRendererDestroy(s_pTextRenderer);
        s_pTextRenderer = NULL;
    }
    if (s_pTestFont) {
        fontDestroy(s_pTestFont);
        s_pTestFont = NULL;
    }
    s_ubTextRendererInitialized = 0;
    
    gameUIDestroy();
    FreeGameState();
    systemUnuse();
}
tState g_sStateGame = {
    .cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy};

tState g_statePaused = {
    .cbCreate = NULL, .cbLoop = NULL, .cbDestroy = NULL};
