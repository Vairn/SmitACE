#include "wallset.h"
#include <ace/utils/file.h>
#include <ace/managers/memory.h>


tWallset* wallsetLoad(const char* fileName)
{
    tFile* pFile = fileOpen(fileName, "rb");
    if (pFile)
    {
        UWORD totalTilesetCount = 0;
        UBYTE paletteSize = 0;
        UBYTE* palette = 0;
        tWallGfx** tileset = 0;
        fileRead(pFile, &paletteSize, 1);
        palette = (UBYTE*)memAllocFastClear(paletteSize * 3);
        fileRead(pFile, palette, paletteSize * 3);
        

        fileRead(pFile, &totalTilesetCount, 2);
        UBYTE tilesetCount = 0;
        fileRead(pFile, &tilesetCount, 1);
        UBYTE wallsetCount =0;
        fileRead(pFile, &wallsetCount, 1);
        tileset = (tWallGfx**)memAllocFastClear(sizeof(tWallGfx*) * totalTilesetCount);
        for (int i = 0; i < totalTilesetCount; i++)
        {
            WORD location[2] = { 0 };
            WORD screen[2] = { 0 };
            UWORD width = 0;
            UWORD height = 0;
            UBYTE type = 0;
            tBitMap* gfx = 0;
            fileRead(pFile, &type, 1);
            fileRead(pFile, location, 4);
            fileRead(pFile, screen, 4);
            fileRead(pFile, &width, 2);
            fileRead(pFile, &height, 2);
            
            tileset[i] = (tWallGfx*)memAllocFastClear(sizeof(tWallGfx));
            tileset[i]->_location[0] = location[0];
            tileset[i]->_location[1] = location[1];
            tileset[i]->_screen[0] = screen[0];
            tileset[i]->_screen[1] = screen[1];
            tileset[i]->_width = width;
            tileset[i]->_height = height;
            tileset[i]->_type = type;

            //tileset[i]->_gfx = gfx;
        }
    }
    return 0;
}