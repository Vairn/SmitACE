#ifndef _TEXT_RENDER_H_
#define _TEXT_RENDER_H_

#include <ace/types.h>
#include <ace/utils/font.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Text justification options
 */
typedef enum {
    TEXT_JUSTIFY_LEFT,
    TEXT_JUSTIFY_RIGHT,
    TEXT_JUSTIFY_CENTER
} eTextJustify;

/**
 * @brief Text renderer structure
 */
typedef struct _tTextRenderer {
    tFont *pFont;              ///< Font to use for rendering
    UWORD *pGlyphCache;         ///< Cached glyph widths (256 entries)
    char *pScratchArea;         ///< Scratch buffer for line processing
    UWORD uwScratchSize;        ///< Size of scratch buffer
} tTextRenderer;

/**
 * @brief Creates a text renderer from a font
 * @param pFont Pointer to font to use
 * @return Pointer to text renderer, or NULL on failure
 */
tTextRenderer *textRendererCreate(tFont *pFont);

/**
 * @brief Destroys a text renderer
 * @param pRenderer Pointer to text renderer to destroy
 */
void textRendererDestroy(tTextRenderer *pRenderer);

/**
 * @brief Renders text with line breaking and justification
 * @param pRenderer Text renderer instance
 * @param szText Text to render
 * @param uwMaxWidth Maximum width in pixels (0 = no limit)
 * @param eJustify Justification mode
 * @return Pointer to text bitmap, or NULL on failure
 */
tTextBitMap *textRendererCreateText(
    tTextRenderer *pRenderer,
    const char *szText,
    UWORD uwMaxWidth,
    eTextJustify eJustify
);

/**
 * @brief Color segment for multi-color text
 * @note Increased to 64 to support all 32 text palette colors and more
 */
#define MAX_COLOR_SEGMENTS 32
typedef struct {
    tTextBitMap *pBitmap;  ///< Text bitmap for this segment
    UBYTE ubColor;          ///< Color index for this segment
    UWORD uwX;              ///< X position (calculated during layout)
    UWORD uwY;              ///< Y position (calculated during layout)
} tColorSegment;

/**
 * @brief Multi-color text result structure
 */
typedef struct {
    tColorSegment aSegments[MAX_COLOR_SEGMENTS];  ///< Array of color segments
    UBYTE ubSegmentCount;                        ///< Number of segments
    UWORD uwTotalWidth;                          ///< Total width of all segments
    UWORD uwTotalHeight;                         ///< Total height (after wrapping)
} tMultiColorText;

/**
 * @brief Creates multi-color text from text with color markup {c:N}
 * @param pRenderer Text renderer instance
 * @param szText Text with optional color markup (e.g., "{c:8}white{c:9}red text")
 * @param ubDefaultColor Default color if no markup found (0-31, maps to text palette colors 64-95)
 * @param uwMaxWidth Maximum width before wrapping to new line (0 = no wrapping)
 * @return Pointer to multi-color text structure, or NULL on failure
 * @note Caller must call textRendererDestroyMultiColorText() to free
 * @note Color values 0-31 map to text palette colors 64-95 (bitplane 6 offset)
 *       All 32 colors from the text palette are available via {c:0} through {c:31}
 */
tMultiColorText *textRendererCreateMultiColorText(
    tTextRenderer *pRenderer,
    const char *szText,
    UBYTE ubDefaultColor,
    UWORD uwMaxWidth
);

/**
 * @brief Destroys multi-color text and frees all resources
 * @param pMultiColorText Pointer to multi-color text to destroy
 */
void textRendererDestroyMultiColorText(tMultiColorText *pMultiColorText);

#ifdef __cplusplus
}
#endif

#endif // _TEXT_RENDER_H_

