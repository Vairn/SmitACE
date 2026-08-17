#include "wallset.h"
#include <ace/utils/file.h>
#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/utils/bitmap.h>
#include "gfx_util.h"
#include <ace/utils/disk_file.h>
#include <string.h>

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
		fileReadBytes(pFile, header, 3);

		fileReadBytes(pFile, &paletteSize, 1);
		palette = (UBYTE *)memAllocFastClear(paletteSize * 3);
		fileReadBytes(pFile, palette, paletteSize * 3);

		fileReadWords(pFile, &totalTilesetCount, 1);
		UBYTE tilesetCount = 0;
		fileReadBytes(pFile, &tilesetCount, 1);

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
			fileReadBytes(pFile, &wallsetCount, 1);
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

				fileReadBytes(pFile, &type, 1);
				fileReadBytes(pFile, &setIndex, 1);
				fileReadBytes(pFile, (UBYTE*)location, 2);
				fileReadWords(pFile, (UWORD*)&screen[0], 1);
				fileReadWords(pFile, (UWORD*)&screen[1], 1);
				fileReadWords(pFile, &x, 1);
				fileReadWords(pFile, &y, 1);
				fileReadWords(pFile, &width, 1);
				fileReadWords(pFile, &height, 1);

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
			if (!pWallset->_gfx[ts] || !pWallset->_mask[ts]) {
				logWrite(
					"ERR: wallset missing gfx/mask: '%s' / '%s'\n",
					gfxPath, maskPath
				);
			}
		}

		/* If set 0 files are absent, borrow first loaded set so blits are never null. */
		if (tilesetCount > 0) {
			if (!pWallset->_gfx[0] || !pWallset->_mask[0]) {
				for (UWORD t = 1; t < (UWORD)tilesetCount; t++) {
					if (pWallset->_gfx[t] && pWallset->_mask[t]) {
						pWallset->_gfx[0] = pWallset->_gfx[t];
						pWallset->_mask[0] = pWallset->_mask[t];
						logWrite(
							"WARN: wallset: tileset 0 missing; using set %u gfx/mask as fallback\n", t
						);
						break;
					}
				}
			}
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

	fileWriteBytes(pFile, pWallset->_header, 3);
	fileWriteBytes(pFile, &pWallset->_paletteSize, 1);
	fileWriteBytes(pFile, pWallset->_palette, pWallset->_paletteSize * 3);

	fileWriteWords(pFile, &pWallset->_tilesetCount, 1);
	{
		UBYTE tcount = (UBYTE)pWallset->_gfxCount;
		fileWriteBytes(pFile, &tcount, 1);
	}

	int i = 0;
	for (UWORD ts = 0; ts < pWallset->_gfxCount; ts++) {
		UBYTE wcount = pWallset->_tilesPerGroup[ts];
		fileWriteBytes(pFile, &wcount, 1);
		for (UBYTE ws = 0; ws < wcount; ws++, i++) {
			tWallGfx *g = pWallset->_tileset[i];
			BYTE loc[2];
			fileWriteBytes(pFile, &g->_type, 1);
			fileWriteBytes(pFile, &g->_setIndex, 1);
			loc[0] = (BYTE)g->_location[0];
			loc[1] = (BYTE)g->_location[1];
			fileWriteBytes(pFile, (const UBYTE*)loc, 2);
			UWORD uwScreen0 = (UWORD)(WORD)g->_screen[0];
			UWORD uwScreen1 = (UWORD)(WORD)g->_screen[1];
			fileWriteWords(pFile, &uwScreen0, 1);
			fileWriteWords(pFile, &uwScreen1, 1);
			fileWriteWords(pFile, &g->_x, 1);
			fileWriteWords(pFile, &g->_y, 1);
			fileWriteWords(pFile, &g->_width, 1);
			fileWriteWords(pFile, &g->_height, 1);
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

static void wallsetDestroyBitmapListUnique(tBitMap** pp, UWORD count)
{
	for (UWORD ts = 0; ts < count; ts++) {
		tBitMap *b = pp[ts];
		if (!b)
			continue;
		UBYTE dup = 0;
		for (UWORD u = 0; u < ts; u++) {
			if (pp[u] == b) {
				dup = 1;
				break;
			}
		}
		if (!dup)
			bitmapDestroy(b);
	}
}

void wallsetDestroy(tWallset* pWallset)
{
	if (pWallset == NULL)
		return;

	wallsetDestroyBitmapListUnique(pWallset->_gfx, pWallset->_gfxCount);
	wallsetDestroyBitmapListUnique(pWallset->_mask, pWallset->_gfxCount);
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
