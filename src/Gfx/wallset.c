#include "wallset.h"
#include <ace/utils/file.h>
#include <ace/managers/memory.h>
#include <ace/utils/bitmap.h>
#include "gfx_util.h"
#include <ace/utils/disk_file.h>
#include <string.h>

static UWORD readU16Be(tFile *pFile)
{
	UBYTE b[2];
	fileRead(pFile, b, 2);
	return ((UWORD)b[0] << 8) | (UWORD)b[1];
}

static WORD readI16Be(tFile *pFile)
{
	return (WORD)readU16Be(pFile);
}

static void writeU16Be(tFile *pFile, UWORD v)
{
	UBYTE b[2];
	b[0] = (UBYTE)(v >> 8);
	b[1] = (UBYTE)v;
	fileWrite(pFile, b, 2);
}

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
		fileRead(pFile, palette, paletteSize * 3);

		totalTilesetCount = readU16Be(pFile);
		UBYTE tilesetCount = 0;
		fileRead(pFile, &tilesetCount, 1);

		tWallGfx *tilesetData = (tWallGfx *)memAllocFastClear(sizeof(tWallGfx) * totalTilesetCount);
		tileset = (tWallGfx **)memAllocFastClear(sizeof(tWallGfx *) * totalTilesetCount);
		UBYTE *tilesPerGroup = NULL;
		if (tilesetCount > 0)
			tilesPerGroup = (UBYTE *)memAllocFastClear(tilesetCount);

		for (int i = 0; i < totalTilesetCount; i++) {
			tileset[i] = &tilesetData[i];
		}

		int i = 0;
		for (int ts = 0; ts < tilesetCount; ts++)
		{
			UBYTE wallsetCount = 0;
			fileRead(pFile, &wallsetCount, 1);
			if (tilesPerGroup)
				tilesPerGroup[ts] = wallsetCount;
			for (int ws = 0; ws < wallsetCount; ws++)
			{
				BYTE location[2] = {0};
				WORD screen[2] = {0};
				UWORD width = 0;
				UWORD height = 0;
				UWORD x = 0;
				UWORD y = 0;
				UBYTE type = 0;
				UBYTE setIndex = 0;

				fileRead(pFile, &type, 1);
				fileRead(pFile, &setIndex, 1);
				fileRead(pFile, location, 2);
				screen[0] = readI16Be(pFile);
				screen[1] = readI16Be(pFile);
				x = readU16Be(pFile);
				y = readU16Be(pFile);
				width = readU16Be(pFile);
				height = readU16Be(pFile);

				tileset[i]->_location[0] = location[0];
				tileset[i]->_location[1] = location[1];
				tileset[i]->_screen[0] = screen[0];
				tileset[i]->_screen[1] = screen[1];
				tileset[i]->_x = x;
				tileset[i]->_y = y;
				tileset[i]->_width = width;
				tileset[i]->_height = height;
				tileset[i]->_type = type;
				tileset[i]->_setIndex = setIndex;

				i++;
			}
		}

		tWallset *pWallset = (tWallset *)memAllocFastClear(sizeof(tWallset));
		pWallset->_paletteSize = paletteSize;
		pWallset->_palette = palette;
		pWallset->_tilesetCount = totalTilesetCount;
		pWallset->_gfxCount = tilesetCount;
		pWallset->_tileset = tileset;
		pWallset->_gfx = (tBitMap**)memAllocFastClear(sizeof(tBitMap*)*tilesetCount);
		pWallset->_mask = (tBitMap**)memAllocFastClear(sizeof(tBitMap*)*tilesetCount);
		pWallset->_tilesPerGroup = tilesPerGroup;
		memcpy(pWallset->_header, header, 3);

		const char* lastDot = fileName;
		for(const char* p = fileName; *p; p++) {
			if(*p == '.') lastDot = p;
		}

		for (int ts=0; ts<tilesetCount; ts++) {
			char gfxPath[256];
			char maskPath[256];
			int baseLen = lastDot - fileName;

			memcpy(gfxPath, fileName, baseLen);
			memcpy(maskPath, fileName, baseLen);

			gfxPath[baseLen] = '_';
			maskPath[baseLen] = '_';

			char numStr[8];
			int numLen = 0;
			int num = ts;
			do {
				numStr[numLen++] = '0' + (num % 10);
				num /= 10;
			} while(num > 0);

			for(int j = 0; j < numLen/2; j++) {
				char temp = numStr[j];
				numStr[j] = numStr[numLen-1-j];
				numStr[numLen-1-j] = temp;
			}

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

void wallsetSave(tWallset *pWallset, const char *fileName)
{
	if (!pWallset || !fileName || !pWallset->_tilesPerGroup)
		return;
	systemUse();
	tFile *pFile = diskFileOpen(fileName, DISK_FILE_MODE_WRITE, 1);
	if (!pFile) {
		systemUnuse();
		return;
	}

	fileWrite(pFile, pWallset->_header, 3);
	fileWrite(pFile, &pWallset->_paletteSize, 1);
	fileWrite(pFile, pWallset->_palette, pWallset->_paletteSize * 3);

	writeU16Be(pFile, pWallset->_tilesetCount);
	{
		UBYTE tcount = (UBYTE)pWallset->_gfxCount;
		fileWrite(pFile, &tcount, 1);
	}

	int i = 0;
	for (UWORD ts = 0; ts < pWallset->_gfxCount; ts++) {
		UBYTE wcount = pWallset->_tilesPerGroup[ts];
		fileWrite(pFile, &wcount, 1);
		for (UBYTE ws = 0; ws < wcount; ws++, i++) {
			tWallGfx *g = pWallset->_tileset[i];
			BYTE loc[2];
			fileWrite(pFile, &g->_type, 1);
			fileWrite(pFile, &g->_setIndex, 1);
			loc[0] = (BYTE)g->_location[0];
			loc[1] = (BYTE)g->_location[1];
			fileWrite(pFile, loc, 2);
			writeU16Be(pFile, (UWORD)(WORD)g->_screen[0]);
			writeU16Be(pFile, (UWORD)(WORD)g->_screen[1]);
			writeU16Be(pFile, g->_x);
			writeU16Be(pFile, g->_y);
			writeU16Be(pFile, g->_width);
			writeU16Be(pFile, g->_height);
		}
	}

	const char* lastDot = fileName;
	for (const char* p = fileName; *p; p++) {
		if (*p == '.') lastDot = p;
	}

	for (UWORD ts = 0; ts < pWallset->_gfxCount; ts++) {
		char gfxPath[256];
		char maskPath[256];
		int baseLen = lastDot - fileName;
		memcpy(gfxPath, fileName, baseLen);
		memcpy(maskPath, fileName, baseLen);
		gfxPath[baseLen] = '_';
		maskPath[baseLen] = '_';
		char numStr[8];
		int numLen = 0;
		int num = (int)ts;
		do {
			numStr[numLen++] = '0' + (num % 10);
			num /= 10;
		} while (num > 0);
		for (int j = 0; j < numLen/2; j++) {
			char temp = numStr[j];
			numStr[j] = numStr[numLen-1-j];
			numStr[numLen-1-j] = temp;
		}
		memcpy(gfxPath + baseLen + 1, numStr, numLen);
		memcpy(maskPath + baseLen + 1, numStr, numLen);
		memcpy(gfxPath + baseLen + 1 + numLen, ".pln", 4);
		memcpy(maskPath + baseLen + 1 + numLen, ".msk", 4);
		gfxPath[baseLen + 1 + numLen + 4] = '\0';
		maskPath[baseLen + 1 + numLen + 4] = '\0';
		if (pWallset->_gfx[ts])
			bitmapSave(pWallset->_gfx[ts], gfxPath);
		if (pWallset->_mask[ts])
			bitmapSave(pWallset->_mask[ts], maskPath);
	}

	fileClose(pFile);
	systemUnuse();
}

void wallsetDestroy(tWallset* pWallset)
{
	if (pWallset == NULL)
		return;

	for (UWORD ts=0; ts<pWallset->_gfxCount; ts++) {
		bitmapDestroy(pWallset->_gfx[ts]);
		bitmapDestroy(pWallset->_mask[ts]);
	}
	memFree(pWallset->_gfx,sizeof(tBitMap*)*pWallset->_gfxCount);
	memFree(pWallset->_mask,sizeof(tBitMap*)*pWallset->_gfxCount);

	if (pWallset->_tilesetCount && pWallset->_tileset && pWallset->_tileset[0])
		memFree(pWallset->_tileset[0], sizeof(tWallGfx) * pWallset->_tilesetCount);
	memFree(pWallset->_tileset, sizeof(tWallGfx *) * pWallset->_tilesetCount);
	if (pWallset->_tilesPerGroup)
		memFree(pWallset->_tilesPerGroup, pWallset->_gfxCount);
	memFree(pWallset->_palette,pWallset->_paletteSize*3);
	memFree(pWallset,sizeof(tWallset));
}
