#include "wallset.h"
#include <ace/utils/file.h>
#include <ace/managers/memory.h>
#include <ace/utils/bitmap.h>
#include "gfx_util.h"
#include <ace/utils/disk_file.h>

tWallset *wallsetLoad(const char *fileName)
{
    tFile *pFile = diskFileOpen(fileName, DISK_FILE_MODE_READ, 1);
    if (pFile)
    {
        systemUse();
        UWORD totalTilesetCount = 0;
        UBYTE paletteSize = 0;
        UBYTE *palette = 0;
        tWallGfx **tileset = 0;
        UBYTE header[3];
        fileRead(pFile, header, 3);

        fileRead(pFile, &paletteSize, 1);
        palette = (UBYTE *)memAllocFastClear(paletteSize * 3);
        // Read palette data in bulk
        fileRead(pFile, palette, paletteSize * 3);

        fileRead(pFile, &totalTilesetCount, 2);
        UBYTE tilesetCount = 0;
        fileRead(pFile, &tilesetCount, 1);

        // Allocate all tileset structures in one block
        tWallGfx *tilesetData = (tWallGfx *)memAllocFastClear(sizeof(tWallGfx) * totalTilesetCount);
        tileset = (tWallGfx **)memAllocFastClear(sizeof(tWallGfx *) * totalTilesetCount);
        
        // Set up the pointers to the pre-allocated structures
        for (int i = 0; i < totalTilesetCount; i++) {
            tileset[i] = &tilesetData[i];
        }

        int i = 0;
        for (int ts = 0; ts < tilesetCount; ts++)
        {
            UBYTE wallsetCount = 0;
            fileRead(pFile, &wallsetCount, 1);
            for (int ws = 0; ws < wallsetCount; ws++)
            {
                BYTE location[2] = {0};
                WORD screen[2] = {0};
                UWORD width = 0;
                UWORD height = 0;
                UWORD x=0;
                UWORD y=0;
                UBYTE type = 0;
                UBYTE setIndex = 0;
               
                fileRead(pFile, &type, 1);
                fileRead(pFile, &setIndex,1);
                fileRead(pFile, location, 2);
                fileRead(pFile, screen, 4);
                fileRead(pFile, &x, 2);
                fileRead(pFile, &y, 2);

                fileRead(pFile, &width, 2);
                fileRead(pFile, &height, 2);

                tileset[i]->_location[0] = location[0];
                tileset[i]->_location[1] = location[1];
                tileset[i]->_screen[0] = screen[0];
                tileset[i]->_screen[1] = screen[1];
                tileset[i]->_x = x;
                tileset[i]->_y = y;
                tileset[i]->_width = width;
                tileset[i]->_height = height;
                tileset[i]->_type = type;
                tileset[i]->_setIndex=setIndex;
                
                i++;
            }
        }
        // Tile sets loaded, now allocate the wallset GFX, and Mask.
        tWallset *pWallset = (tWallset *)memAllocFastClear(sizeof(tWallset));
        pWallset->_paletteSize = paletteSize;
        pWallset->_palette = palette;
        pWallset->_tilesetCount = totalTilesetCount;
        pWallset->_gfxCount = tilesetCount;
        pWallset->_tileset = tileset;
        pWallset->_gfx = (tBitMap**)memAllocFastClear(sizeof(tBitMap*)*tilesetCount);
        pWallset->_mask = (tBitMap**)memAllocFastClear(sizeof(tBitMap*)*tilesetCount);

        // Find the last dot in the filename
        const char* lastDot = fileName;
        for(const char* p = fileName; *p; p++) {
            if(*p == '.') lastDot = p;
        }
        
        // Load all bitmaps
        for (int ts=0; ts<tilesetCount; ts++) {
            // Build filenames directly without extra allocations
            char gfxPath[256];
            char maskPath[256];
            int baseLen = lastDot - fileName;
            
            // Copy base name
            memcpy(gfxPath, fileName, baseLen);
            memcpy(maskPath, fileName, baseLen);
            
            // Add suffix and extension
            gfxPath[baseLen] = '_';
            maskPath[baseLen] = '_';
            
            // Convert number to string
            char numStr[8];
            int numLen = 0;
            int num = ts;
            do {
                numStr[numLen++] = '0' + (num % 10);
                num /= 10;
            } while(num > 0);
            
            // Reverse the number string
            for(int i = 0; i < numLen/2; i++) {
                char temp = numStr[i];
                numStr[i] = numStr[numLen-1-i];
                numStr[numLen-1-i] = temp;
            }
            
            // Add number and extension
            memcpy(gfxPath + baseLen + 1, numStr, numLen);
            memcpy(maskPath + baseLen + 1, numStr, numLen);
            memcpy(gfxPath + baseLen + 1 + numLen, ".pln", 4);
            memcpy(maskPath + baseLen + 1 + numLen, ".msk", 4);
            gfxPath[baseLen + 1 + numLen + 4] = '\0';
            maskPath[baseLen + 1 + numLen + 4] = '\0';
            
            pWallset->_gfx[ts] = bitmapCreateFromPath(gfxPath, 0);
            pWallset->_mask[ts] = bitmapCreateFromPath(maskPath, 0);
        }
        
        systemUnuse();
        return pWallset;
    }
    return 0;
}

void wallsetDestroy(tWallset* pWallset)
{
    if (pWallset == NULL)
        return;
   
    for (int ts=0; ts<pWallset->_gfxCount; ts++) {
        bitmapDestroy(pWallset->_gfx[ts]);
        bitmapDestroy(pWallset->_mask[ts]);
    }
    for (int ts=0; ts<pWallset->_tilesetCount; ts++)
    {
        memFree(pWallset->_tileset[ts],sizeof(tWallGfx));
    }
    memFree(pWallset->_gfx,sizeof(tBitMap*)*pWallset->_gfxCount);
    memFree(pWallset->_mask,sizeof(tBitMap*)*pWallset->_gfxCount);
    memFree(pWallset->_tileset,sizeof(tWallGfx*)*pWallset->_tilesetCount);
    memFree(pWallset->_palette,pWallset->_paletteSize*3);
    memFree(pWallset,sizeof(tWallset));

}