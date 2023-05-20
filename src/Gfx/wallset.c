#include "wallset.h"
#include <ace/utils/file.h>
#include <ace/managers/memory.h>
#include <ace/utils/bitmap.h>

char* replace_extension(const char* filename, const char* new_extension) {
    const char* dot_position = strrchr(filename, '.');
    if (dot_position == NULL) {
        // No extension found, return a copy of the original filename
        char* new_filename = memAllocFast(strlen(filename) + strlen(new_extension) + 1);
        strcpy(new_filename, filename);
        return new_filename;
    }

    // Calculate the position of the dot
    size_t dot_index = dot_position - filename;

    // Calculate the length of the new filename
    size_t new_filename_length = dot_index + strlen(new_extension);

    // Allocate memory for the new filename
    char* new_filename = memAllocFast(new_filename_length + 1);

    // Copy the original filename up to the dot position
    strncpy(new_filename, filename, dot_index);
    new_filename[dot_index] = '\0';

    // Append the new extension
    strcat(new_filename, new_extension);

    return new_filename;
}

tWallset *wallsetLoad(const char *fileName)
{
    tFile *pFile = fileOpen(fileName, "rb");
    if (pFile)
    {
        UWORD totalTilesetCount = 0;
        UBYTE paletteSize = 0;
        UBYTE *palette = 0;
        tWallGfx **tileset = 0;
        UBYTE header[3];
        fileRead(pFile, header, 3);

        fileRead(pFile, &paletteSize, 1);
        palette = (UBYTE *)memAllocFastClear(paletteSize * 3);
        // fileRead(pFile, palette, paletteSize * 3);
        for (UBYTE p = 0; p < paletteSize; p++)
        {
            UBYTE r;
            UBYTE g;
            UBYTE b;
            fileRead(pFile, &r, 1);
            fileRead(pFile, &g, 1);
            fileRead(pFile, &b, 1);

            palette[p * 3] = r;
            palette[p * 3 + 1] = g;
            palette[p * 3 + 2] = b;
        }

        fileRead(pFile, &totalTilesetCount, 2);
        UBYTE tilesetCount = 0;
        fileRead(pFile, &tilesetCount, 1);

        tileset = (tWallGfx **)memAllocFastClear(sizeof(tWallGfx *) * totalTilesetCount);
        // for (int i = 0; i < totalTilesetCount; i++)
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
                tBitMap *gfx = 0;
                tBitMap *mask = 0;
                fileRead(pFile, &type, 1);
                fileRead(pFile, location, 2);
                fileRead(pFile, screen, 4);
                fileRead(pFile, &x, 2);
                fileRead(pFile, &y, 2);

                fileRead(pFile, &width, 2);
                fileRead(pFile, &height, 2);

                /*UBYTE depth;
                UWORD rowWordSize;
                UWORD heightInBytes;
                fileRead(pFile, &depth, 1);
                fileRead(pFile, &rowWordSize, 2);
                fileRead(pFile, &heightInBytes, 2);
                // WinUAE debug overlay test
                debug_clear();
                //char* debugText = "This is a WinUAE debug overlay";
                debug_filled_rect(100, 200 * 2, (400/totalTilesetCount)*i, 220 * 2, 0x0000ff00); // 0x00RRGGBB
                //debug_rect(90, 190 * 2, 400, 220 * 2, 0x000000ff);         // 0x00RRGGBB    
                
                UWORD uwSrcWidth = rowWordSize * 16;
                gfx = bitmapCreate(uwSrcWidth, height, depth, BMF_INTERLEAVED | BMF_CLEAR);
                // auto bitmapMem = memAllocChipClear(rowWordSize * heightInBytes * depth);
                // bitmapLoadFromMem(gfx, bitmapMem, 0, 0);
                // ULONG ulCurByte = 0;
                UWORD uwWidth = bitmapGetByteWidth(gfx);
                auto s = (((uwSrcWidth) + 7) >> 3);
                for (UWORD y = 0; y != heightInBytes; ++y)
                {
                    UWORD yb = ((y)*gfx->Depth);
                    for (UBYTE ubPlane = 0; ubPlane != gfx->Depth; ++ubPlane)
                    {

                        // memcpy(&pBitMap->Planes[0][rowWordSize*((( y)*pBitMap->Depth)+ubPlane)+(0)],&pData[ulCurByte],((uwSrcWidth+7)>>3));
                        ULONG p = uwWidth * (yb + ubPlane);
                
                        fileRead(pFile, &gfx->Planes[0][p], (s));
                        // ulCurByte+=((rowWordSize+7)>>3);
                    }
                }

                s = (((rowWordSize * 16) + 7) >> 3);
                mask = bitmapCreate(uwSrcWidth, height, depth, BMF_INTERLEAVED | BMF_CLEAR);
                uwWidth = bitmapGetByteWidth(mask);
                for (UWORD y = 0; y != heightInBytes; ++y)
                {
                    UWORD yb = ((y)*mask->Depth);
                    for (UBYTE ubPlane = 0; ubPlane != mask->Depth; ++ubPlane)
                    {

                        ULONG p = uwWidth * (yb + ubPlane);
                        

                        fileRead(pFile, &mask->Planes[0][p], (s));
                        // fileRead(pFile, &mask->Planes[0][uwWidth * (((y)*mask->Depth) + ubPlane) + (0)], ((uwWidth + 7) >> 3));
                    }
                }
                // FreeMem(bitmapMem, rowWordSize * heightInBytes * depth);
                */
                tileset[i] = (tWallGfx *)memAllocFastClear(sizeof(tWallGfx));
                tileset[i]->_location[0] = location[0];
                tileset[i]->_location[1] = location[1];
                tileset[i]->_screen[0] = screen[0];
                tileset[i]->_screen[1] = screen[1];
                tileset[i]->_x = x;
                tileset[i]->_y = y;
                tileset[i]->_width = width;
                tileset[i]->_height = height;
                tileset[i]->_type = type;
                tileset[i]->_gfx = bitmapCreate(width,height,6,BMF_INTERLEAVED | BMF_CLEAR);
                tileset[i]->_mask = bitmapCreate(width,height,6,BMF_INTERLEAVED | BMF_CLEAR);
                i++;
            }
            // tileset[i]->_gfx = gfx;
        }
        // Tile sets loaded, now allocate the wallset GFX, and Mask.
        tWallset *pWallset = (tWallset *)memAllocFastClear(sizeof(tWallset));
        pWallset->_paletteSize = paletteSize;
        pWallset->_palette = palette;
        pWallset->_tilesetCount = totalTilesetCount;
        pWallset->_tileset = tileset;
        char* bitmapFile = replace_extension(fileName, ".pln");
        pWallset->_gfx = bitmapCreateFromFile(bitmapFile,0);
        char* maskFile = replace_extension(fileName, ".msk");
        pWallset->_mask = bitmapCreateFromFile(maskFile,0);

        memFree(bitmapFile,strlen(bitmapFile)+1);
        memFree(maskFile,strlen(maskFile)+1);
        return pWallset;
    }
    return 0;
}
