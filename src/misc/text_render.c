#include "text_render.h"
#include <ace/managers/system.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/utils/font.h>
#include <string.h>

#define INITIAL_SCRATCH_CAPACITY 256
#define MAX_LINES 64

/**
 * @brief Line data structure for internal use
 */
typedef struct {
    UWORD uwStart;
    UWORD uwEnd;
} tLineData;

/**
 * @brief Breaks text into lines based on max width
 * @param pRenderer Text renderer instance
 * @param szText Text to break
 * @param pStartIndex Pointer to start index (updated on return)
 * @param uwMaxWidth Maximum width in pixels
 * @param pOutData Output line data
 * @return 1 if line found, 0 if end of text
 */
static UBYTE breakTextIntoLines(
    tTextRenderer *pRenderer,
    const char *szText,
    UWORD *pStartIndex,
    UWORD uwMaxWidth,
    tLineData *pOutData
) {
    UWORD uwEndOfLine = 0;
    UWORD uwLineWidth = 0;
    UWORD uwOffset = 1;
    UWORD uwLastSpacePos = 0;
    UWORD uwTextLength = (UWORD)strlen(szText);

    if (uwTextLength == 0 || *pStartIndex >= uwTextLength) {
        return 0;
    }

    // Early exit for newline-only cases
    if (szText[*pStartIndex] == '\n') {
        pOutData->uwStart = *pStartIndex;
        pOutData->uwEnd = *pStartIndex;
        (*pStartIndex)++;
        return (*pStartIndex <= uwTextLength);
    }

    // Process characters
    for (UWORD idx = *pStartIndex; idx <= uwTextLength; ++idx) {
        char c = (idx < uwTextLength) ? szText[idx] : '\0';

        if (c == ' ') {
            uwLastSpacePos = idx;
            uwEndOfLine = idx;
        }
        else if (c == '\n' || c == '\0') {
            uwEndOfLine = idx;
            break;
        }

        if (c >= ' ') {
            UBYTE ubGlyphWidth = pRenderer->pGlyphCache[(UBYTE)c];
            uwLineWidth += ubGlyphWidth + 1;  // +1 for spacing

            if (uwMaxWidth > 0 && uwLineWidth > uwMaxWidth) {
                // Prefer breaking at last space, otherwise break at current position
                uwEndOfLine = (uwLastSpacePos > *pStartIndex) ? uwLastSpacePos : idx;
                uwOffset = (uwEndOfLine == uwLastSpacePos) ? 1 : 0;
                break;
            }
        }
    }

    if (uwEndOfLine == 0) {
        uwEndOfLine = uwTextLength;
    }

    pOutData->uwStart = *pStartIndex;
    pOutData->uwEnd = uwEndOfLine;
    *pStartIndex = uwEndOfLine + uwOffset;
    return 1;
}

/**
 * @brief Rounds up to nearest multiple of 16
 */
static UWORD roundUp16(UWORD uwValue) {
    return (uwValue + 15) & ~15;
}

tTextRenderer *textRendererCreate(tFont *pFont) {
    if (!pFont) {
        logWrite("Text Renderer: Invalid font pointer.\n");
        return NULL;
    }

    systemUse();
    tTextRenderer *pRenderer = (tTextRenderer *)memAllocFast(sizeof(tTextRenderer));
    if (!pRenderer) {
        systemUnuse();
        return NULL;
    }

    pRenderer->pFont = pFont;
    pRenderer->pGlyphCache = (UWORD *)memAllocFast(sizeof(UWORD) * 256);
    if (!pRenderer->pGlyphCache) {
        memFree(pRenderer, sizeof(tTextRenderer));
        systemUnuse();
        return NULL;
    }

    // Cache glyph widths
    for (UBYTE glyph = 0; glyph < 255; ++glyph) {
        pRenderer->pGlyphCache[glyph] = fontGlyphWidth(pFont, (char)glyph);
    }
    pRenderer->pGlyphCache[255] = fontGlyphWidth(pFont, (char)255);

    // Allocate scratch area
    pRenderer->uwScratchSize = INITIAL_SCRATCH_CAPACITY;
    pRenderer->pScratchArea = (char *)memAllocFast(pRenderer->uwScratchSize);
    if (!pRenderer->pScratchArea) {
        memFree(pRenderer->pGlyphCache, sizeof(UWORD) * 256);
        memFree(pRenderer, sizeof(tTextRenderer));
        systemUnuse();
        return NULL;
    }

    systemUnuse();
    return pRenderer;
}

void textRendererDestroy(tTextRenderer *pRenderer) {
    if (!pRenderer) {
        return;
    }

    systemUse();
    if (pRenderer->pScratchArea) {
        memFree(pRenderer->pScratchArea, pRenderer->uwScratchSize);
    }
    if (pRenderer->pGlyphCache) {
        memFree(pRenderer->pGlyphCache, sizeof(UWORD) * 256);
    }
    memFree(pRenderer, sizeof(tTextRenderer));
    systemUnuse();
}

tTextBitMap *textRendererCreateText(
    tTextRenderer *pRenderer,
    const char *szText,
    UWORD uwMaxWidth,
    eTextJustify eJustify
) {
    if (!pRenderer || !szText) {
        logWrite("ERROR: Invalid renderer or text pointer.\n");
        return NULL;
    }

    if (!pRenderer->pFont) {
        logWrite("ERROR: Font not initialized.\n");
        return NULL;
    }

    UWORD uwTextLength = (UWORD)strlen(szText);
    if (uwTextLength == 0) {
        logWrite("ERROR: Empty text string.\n");
        return NULL;
    }

    systemUse();

    // Break text into lines
    tLineData lines[MAX_LINES];
    UWORD uwLineCount = 0;
    UWORD uwStartIndex = 0;
    tLineData line;

    while (breakTextIntoLines(pRenderer, szText, &uwStartIndex, uwMaxWidth, &line) && uwLineCount < MAX_LINES) {
        lines[uwLineCount++] = line;
    }

    if (uwLineCount == 0) {
        systemUnuse();
        return NULL;
    }

    // Calculate result dimensions
    UWORD uwHeight = pRenderer->pFont->uwHeight * uwLineCount;
    UWORD uwResultWidth = (uwMaxWidth > 0) ? uwMaxWidth : 320;  // Default to 320 if no max width

    // Create result bitmap
    tTextBitMap *pResult = fontCreateTextBitMap(roundUp16(uwResultWidth), roundUp16(uwHeight));
    if (!pResult) {
        systemUnuse();
        return NULL;
    }

    pResult->uwActualWidth = uwResultWidth;
    pResult->uwActualHeight = uwHeight;

    // Create temporary line bitmap
    tTextBitMap *pLineBitmap = fontCreateTextBitMap(320, roundUp16(pRenderer->pFont->uwHeight));
    if (!pLineBitmap) {
        fontDestroyTextBitMap(pResult);
        systemUnuse();
        return NULL;
    }

    // Render each line
    for (UWORD idx = 0; idx < uwLineCount; ++idx) {
        line = lines[idx];
        UWORD uwLineLength = line.uwEnd - line.uwStart;
        
        if (uwLineLength == 0) {
            continue;  // Skip empty lines
        }

        // Resize scratch area if needed
        if (pRenderer->uwScratchSize < uwLineLength + 1) {
            memFree(pRenderer->pScratchArea, pRenderer->uwScratchSize);
            pRenderer->uwScratchSize = uwLineLength + 1;
            pRenderer->pScratchArea = (char *)memAllocFast(pRenderer->uwScratchSize);
            if (!pRenderer->pScratchArea) {
                fontDestroyTextBitMap(pLineBitmap);
                fontDestroyTextBitMap(pResult);
                systemUnuse();
                return NULL;
            }
        }

        // Copy line text to scratch area
        memcpy(pRenderer->pScratchArea, szText + line.uwStart, uwLineLength);
        pRenderer->pScratchArea[uwLineLength] = '\0';

        // Fill line bitmap
        fontFillTextBitMap(pRenderer->pFont, pLineBitmap, pRenderer->pScratchArea);

        // Calculate X position based on justification
        UWORD uwX = 0;
        switch (eJustify) {
            case TEXT_JUSTIFY_RIGHT:
                uwX = uwResultWidth - pLineBitmap->uwActualWidth;
                break;
            case TEXT_JUSTIFY_CENTER:
                uwX = (uwResultWidth - pLineBitmap->uwActualWidth) >> 1;
                break;
            case TEXT_JUSTIFY_LEFT:
            default:
                uwX = 0;
                break;
        }

        // Draw line to result bitmap
        fontDrawTextBitMap(
            pResult->pBitMap, pLineBitmap, uwX, idx * pRenderer->pFont->uwHeight, 1, 0
        );
    }

    // Clean up line bitmap
    fontDestroyTextBitMap(pLineBitmap);

    systemUnuse();
    return pResult;
}

/**
 * @brief Creates multi-color text from text with color markup {c:N}
 */
tMultiColorText *textRendererCreateMultiColorText(
    tTextRenderer *pRenderer,
    const char *szText,
    UBYTE ubDefaultColor,
    UWORD uwMaxWidth
) {
    if (!pRenderer || !szText || !pRenderer->pFont) {
        return NULL;
    }
    
    systemUse();
    
    // Allocate multi-color text structure
    tMultiColorText *pResult = (tMultiColorText *)memAllocFast(sizeof(tMultiColorText));
    if (!pResult) {
        systemUnuse();
        return NULL;
    }
    
    // Initialize
    pResult->ubSegmentCount = 0;
    pResult->uwTotalWidth = 0;
    pResult->uwTotalHeight = 0;
    for (UBYTE i = 0; i < MAX_COLOR_SEGMENTS; i++) {
        pResult->aSegments[i].pBitmap = NULL;
        pResult->aSegments[i].ubColor = 0;
        pResult->aSegments[i].uwX = 0;
        pResult->aSegments[i].uwY = 0;
    }
    
    // Copy text (preserve newlines for line breaks)
    char szWorkingText[256];
    UWORD uwTextLen = strlen(szText);
    if (uwTextLen >= 256) uwTextLen = 255;
    memcpy(szWorkingText, szText, uwTextLen);
    szWorkingText[uwTextLen] = '\0';
    
    // Check if message contains color markup {c:N}
    UBYTE ubHasColorMarkup = 0;
    for (UWORD i = 0; i < uwTextLen - 3; i++) {
        if (szWorkingText[i] == '{' && szWorkingText[i+1] == 'c' && szWorkingText[i+2] == ':') {
            ubHasColorMarkup = 1;
            break;
        }
    }
    
    if (!ubHasColorMarkup) {
        // No color markup - create single segment with default color (centered)
        tTextBitMap *pBitmap = textRendererCreateText(pRenderer, szWorkingText, uwMaxWidth, TEXT_JUSTIFY_CENTER);
        if (pBitmap) {
            pResult->aSegments[0].pBitmap = pBitmap;
            pResult->aSegments[0].ubColor = ubDefaultColor;
            pResult->aSegments[0].uwX = 0;
            pResult->aSegments[0].uwY = 0;
            pResult->ubSegmentCount = 1;
            pResult->uwTotalWidth = pBitmap->uwActualWidth;
            pResult->uwTotalHeight = pBitmap->uwActualHeight;
        } else {
            memFree(pResult, sizeof(tMultiColorText));
            systemUnuse();
            return NULL;
        }
    } else {
        // Parse color markup and create segments
        UBYTE ubCurrentColor = ubDefaultColor;
        char szSegment[256];
        UWORD uwSegmentStart = 0;
        
        for (UWORD i = 0; i < uwTextLen && pResult->ubSegmentCount < MAX_COLOR_SEGMENTS; i++) {
            if (szWorkingText[i] == '{' && i + 3 < uwTextLen && 
                szWorkingText[i+1] == 'c' && szWorkingText[i+2] == ':') {
                // Found color code {c:N}
                UBYTE ubNewColor = 0;
                UWORD uwColorEnd = i + 3;
                while (uwColorEnd < uwTextLen && szWorkingText[uwColorEnd] >= '0' && 
                       szWorkingText[uwColorEnd] <= '9') {
                    ubNewColor = ubNewColor * 10 + (szWorkingText[uwColorEnd] - '0');
                    uwColorEnd++;
                }
                if (uwColorEnd < uwTextLen && szWorkingText[uwColorEnd] == '}') {
                    // Save segment before color change (may contain newlines)
                    if (i > uwSegmentStart) {
                        UWORD uwSegStart = uwSegmentStart;
                        UWORD uwSegEnd = i;
                        
                        // Process segment, splitting at newlines
                        while (uwSegStart < uwSegEnd && pResult->ubSegmentCount < MAX_COLOR_SEGMENTS) {
                            // Find next newline or end of segment
                            UWORD uwNewlinePos = uwSegEnd;
                            for (UWORD j = uwSegStart; j < uwSegEnd; j++) {
                                if (szWorkingText[j] == '\n' || szWorkingText[j] == '\r') {
                                    uwNewlinePos = j;
                                    break;
                                }
                            }
                            
                            UWORD uwSegLen = uwNewlinePos - uwSegStart;
                            if (uwSegLen > 0 && uwSegLen < 255) {
                                memcpy(szSegment, szWorkingText + uwSegStart, uwSegLen);
                                szSegment[uwSegLen] = '\0';
                                
                                // Skip empty or whitespace-only segments
                                UBYTE ubIsEmpty = 1;
                                for (UWORD j = 0; j < uwSegLen; j++) {
                                    if (szSegment[j] != ' ' && szSegment[j] != '\t') {
                                        ubIsEmpty = 0;
                                        break;
                                    }
                                }
                                
                                if (!ubIsEmpty) {
                                    // Create single-line bitmap
                                    tUwCoordYX sBounds = fontMeasureText(pRenderer->pFont, szSegment);
                                    if (sBounds.uwY > pRenderer->pFont->uwHeight) {
                                        sBounds.uwY = pRenderer->pFont->uwHeight;
                                    }
                                    UWORD uwSegWidth = roundUp16(sBounds.uwX);
                                    UWORD uwSegHeight = pRenderer->pFont->uwHeight;
                                    
                                    tTextBitMap *pSegBitmap = fontCreateTextBitMap(uwSegWidth, uwSegHeight);
                                    if (pSegBitmap) {
                                        fontFillTextBitMap(pRenderer->pFont, pSegBitmap, szSegment);
                                        pSegBitmap->uwActualWidth = sBounds.uwX;
                                        pSegBitmap->uwActualHeight = uwSegHeight;
                                        
                                        pResult->aSegments[pResult->ubSegmentCount].pBitmap = pSegBitmap;
                                        pResult->aSegments[pResult->ubSegmentCount].ubColor = ubCurrentColor;
                                        // Mark segment: if this was created after a newline (not the first part of segment), 
                                        // set X to 0xFFFF to indicate it should start on a new line
                                        if (uwSegStart > uwSegmentStart) {
                                            pResult->aSegments[pResult->ubSegmentCount].uwX = 0xFFFF;  // Marker for newline
                                        } else {
                                            pResult->aSegments[pResult->ubSegmentCount].uwX = 0;
                                        }
                                        pResult->aSegments[pResult->ubSegmentCount].uwY = 0;
                                        pResult->ubSegmentCount++;
                                    }
                                }
                            }
                            
                            // Move past newline (if found)
                            if (uwNewlinePos < uwSegEnd) {
                                uwSegStart = uwNewlinePos + 1;
                                // Skip \r\n pairs
                                if (uwSegStart < uwSegEnd && 
                                    ((szWorkingText[uwNewlinePos] == '\r' && szWorkingText[uwSegStart] == '\n') ||
                                     (szWorkingText[uwNewlinePos] == '\n' && szWorkingText[uwSegStart] == '\r'))) {
                                    uwSegStart++;
                                }
                            } else {
                                break;  // No more newlines in this segment
                            }
                        }
                    }
                    ubCurrentColor = ubNewColor;
                    uwSegmentStart = uwColorEnd + 1;
                    i = uwColorEnd;
                }
            }
        }
        
        // Add final segment (may contain newlines)
        if (uwSegmentStart < uwTextLen && pResult->ubSegmentCount < MAX_COLOR_SEGMENTS) {
            UWORD uwSegStart = uwSegmentStart;
            UWORD uwSegEnd = uwTextLen;
            
            // Process segment, splitting at newlines
            while (uwSegStart < uwSegEnd && pResult->ubSegmentCount < MAX_COLOR_SEGMENTS) {
                // Find next newline or end of segment
                UWORD uwNewlinePos = uwSegEnd;
                for (UWORD j = uwSegStart; j < uwSegEnd; j++) {
                    if (szWorkingText[j] == '\n' || szWorkingText[j] == '\r') {
                        uwNewlinePos = j;
                        break;
                    }
                }
                
                UWORD uwSegLen = uwNewlinePos - uwSegStart;
                if (uwSegLen > 0 && uwSegLen < 255) {
                    memcpy(szSegment, szWorkingText + uwSegStart, uwSegLen);
                    szSegment[uwSegLen] = '\0';
                    
                    // Skip empty or whitespace-only segments
                    UBYTE ubIsEmpty = 1;
                    for (UWORD j = 0; j < uwSegLen; j++) {
                        if (szSegment[j] != ' ' && szSegment[j] != '\t') {
                            ubIsEmpty = 0;
                            break;
                        }
                    }
                    
                    if (!ubIsEmpty) {
                        // Create single-line bitmap
                        tUwCoordYX sBounds = fontMeasureText(pRenderer->pFont, szSegment);
                        if (sBounds.uwY > pRenderer->pFont->uwHeight) {
                            sBounds.uwY = pRenderer->pFont->uwHeight;
                        }
                        UWORD uwSegWidth = roundUp16(sBounds.uwX);
                        UWORD uwSegHeight = pRenderer->pFont->uwHeight;
                        
                        tTextBitMap *pSegBitmap = fontCreateTextBitMap(uwSegWidth, uwSegHeight);
                        if (pSegBitmap) {
                            fontFillTextBitMap(pRenderer->pFont, pSegBitmap, szSegment);
                            pSegBitmap->uwActualWidth = sBounds.uwX;
                            pSegBitmap->uwActualHeight = uwSegHeight;
                            
                            pResult->aSegments[pResult->ubSegmentCount].pBitmap = pSegBitmap;
                            pResult->aSegments[pResult->ubSegmentCount].ubColor = ubCurrentColor;
                            // Mark segment: if this was created after a newline, set X to 0xFFFF
                            if (uwSegStart > uwSegmentStart) {
                                pResult->aSegments[pResult->ubSegmentCount].uwX = 0xFFFF;  // Marker for newline
                            } else {
                                pResult->aSegments[pResult->ubSegmentCount].uwX = 0;
                            }
                            pResult->aSegments[pResult->ubSegmentCount].uwY = 0;
                            pResult->ubSegmentCount++;
                        }
                    }
                }
                
                // Move past newline (if found)
                if (uwNewlinePos < uwSegEnd) {
                    uwSegStart = uwNewlinePos + 1;
                    // Skip \r\n pairs
                    if (uwSegStart < uwSegEnd && 
                        ((szWorkingText[uwNewlinePos] == '\r' && szWorkingText[uwSegStart] == '\n') ||
                         (szWorkingText[uwNewlinePos] == '\n' && szWorkingText[uwSegStart] == '\r'))) {
                        uwSegStart++;
                    }
                } else {
                    break;  // No more newlines in this segment
                }
            }
        }
        
        // Calculate layout (positions and dimensions)
        if (pResult->ubSegmentCount > 0) {
            UWORD uwCurrentX = 0;
            UWORD uwCurrentY = 0;
            UWORD uwLineWidth = 0;
            UWORD uwFontHeight = pRenderer->pFont->uwHeight;
            
            for (UBYTE i = 0; i < pResult->ubSegmentCount; i++) {
                if (pResult->aSegments[i].pBitmap) {
                    UWORD uwSegWidth = pResult->aSegments[i].pBitmap->uwActualWidth;
                    UWORD uwSegHeight = pResult->aSegments[i].pBitmap->uwActualHeight;
                    
                    // Force single line height
                    if (uwSegHeight > uwFontHeight) {
                        pResult->aSegments[i].pBitmap->uwActualHeight = uwFontHeight;
                        uwSegHeight = uwFontHeight;
                    }
                    
                    // Check if this segment should start on a new line (marked with 0xFFFF)
                    if (pResult->aSegments[i].uwX == 0xFFFF) {
                        uwCurrentX = 0;
                        uwCurrentY += uwFontHeight;
                    }
                    
                    // Wrap to new line if exceeds max width
                    if (uwMaxWidth > 0 && uwCurrentX + uwSegWidth > uwMaxWidth && uwCurrentX > 0) {
                        uwCurrentX = 0;
                        uwCurrentY += uwFontHeight;
                    }
                    
                    // Position segment
                    pResult->aSegments[i].uwX = uwCurrentX;
                    pResult->aSegments[i].uwY = uwCurrentY;
                    uwCurrentX += uwSegWidth + 1;  // +1 for spacing
                    
                    // Track maximum line width
                    if (uwCurrentX > uwLineWidth) {
                        uwLineWidth = uwCurrentX;
                    }
                }
            }
            
            pResult->uwTotalWidth = (uwLineWidth > 0) ? (uwLineWidth - 1) : 0;  // Remove last spacing
            pResult->uwTotalHeight = uwCurrentY + uwFontHeight;
        }
    }
    
    // If no segments created, cleanup and return NULL
    if (pResult->ubSegmentCount == 0) {
        textRendererDestroyMultiColorText(pResult);
        systemUnuse();
        return NULL;
    }
    
    systemUnuse();
    return pResult;
}

/**
 * @brief Destroys multi-color text and frees all resources
 */
void textRendererDestroyMultiColorText(tMultiColorText *pMultiColorText) {
    if (!pMultiColorText) {
        return;
    }
    
    systemUse();
    for (UBYTE i = 0; i < pMultiColorText->ubSegmentCount; i++) {
        if (pMultiColorText->aSegments[i].pBitmap) {
            fontDestroyTextBitMap(pMultiColorText->aSegments[i].pBitmap);
            pMultiColorText->aSegments[i].pBitmap = NULL;
        }
    }
    memFree(pMultiColorText, sizeof(tMultiColorText));
    systemUnuse();
}

