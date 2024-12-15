#include <ace/utils/bitmap.h>
#include <ace/utils/file.h>

#include "uigfx.h"
#include "gfx_util.h"


tUigfxSprite* s_uiSprites;
tBitMap* s_pBitmap = NULL;

void LoadUIGraphics(const char* sFilename)
{
    systemUse();
    // load the bitmap.
    char* bitmapFile = replace_extension(sFilename, ".bm");
    s_pBitmap = bitmapCreateFromPath(bitmapFile,FALSE);
    char* maskFile = replace_extension(sFilename, ".msk");
           
    memFree(bitmapFile,strlen(bitmapFile)+1);
    memFree(maskFile,strlen(maskFile)+1);
    systemUnuse();
}

void DrawUIGfx(tBitMap* pDest, tUigfxSprite* pSprite, UWORD uwX, UWORD uwY)
{

}

void DestroyUIGraphics(void)
{
    if (s_pBitmap != NULL)
        bitmapDestroy(s_pBitmap);
}