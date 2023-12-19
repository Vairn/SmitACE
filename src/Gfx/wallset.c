#include "wallset.h"
#include <ace/utils/file.h>
#include <ace/managers/memory.h>
#include <ace/utils/bitmap.h>

char* addPostfixToFile(char* filename, const char* postfix, int number) {
    //char new_filename[100];
     char* new_filename = memAllocFast(strlen(filename) + strlen(postfix) + 5);
       
    char number_str[20];
    char* dot = strchr(filename, '.');

    if (dot != NULL) {
        size_t index = dot - filename;
        strncpy(new_filename, filename, index);
        new_filename[index] = '\0';

        sprintf(number_str, "%d", number); // Convert number to string
        strcat(new_filename, postfix);
        strcat(new_filename, number_str);
        strcat(new_filename, dot);
    } else {
        strcpy(new_filename, filename);

        sprintf(number_str, "%d", number); // Convert number to string
        strcat(new_filename, postfix);
        strcat(new_filename, number_str);
    }

    return new_filename;
}


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
        systemUse();
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
                UBYTE setIndex = 0;
               
                fileRead(pFile, &type, 1);
                fileRead(pFile, &setIndex,1);
                fileRead(pFile, location, 2);
                fileRead(pFile, screen, 4);
                fileRead(pFile, &x, 2);
                fileRead(pFile, &y, 2);

                fileRead(pFile, &width, 2);
                fileRead(pFile, &height, 2);

               
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
                tileset[i]->_setIndex=setIndex;
                
                i++;
            }
            // tileset[i]->_gfx = gfx;
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
        for (int ts=0; ts<tilesetCount; ts++)
        {
            char* bitmapFile = replace_extension(fileName, ".pln");
            bitmapFile = addPostfixToFile(bitmapFile, "_", ts);
            pWallset->_gfx[ts] = bitmapCreateFromFile(bitmapFile,0);
            char* maskFile = replace_extension(fileName, ".msk");
            maskFile = addPostfixToFile(maskFile, "_", ts);
            pWallset->_mask[ts] = bitmapCreateFromFile(maskFile,0);
        /*    if (pWallset->_gfx[ts]==0)
            {
                pWallset->_gfx[ts] = bitmapCreate(20,20,1,BMF_CLEAR);
            }
            if (pWallset->_mask[ts]==0)
            {
                pWallset->_mask[ts] = bitmapCreate(20,20,1,BMF_CLEAR);
            }*/
            memFree(bitmapFile,strlen(bitmapFile)+1);
            memFree(maskFile,strlen(maskFile)+1);
       
        }
        // char* bitmapFile = replace_extension(fwwileName, ".pln");
        // pWallset->_gfx = bitmapCreateFromFile(bitmapFile,0);
        // char* maskFile = replace_extension(fileName, ".msk");
        // pWallset->_mask = bitmapCreateFromFile(maskFile,0);
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