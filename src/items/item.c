#include "item.h"
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>
#include <ace/utils/file.h>
#include <ace/utils/disk_file.h>
#include <ace/utils/bitmap.h>
#include <string.h>
#include <stdio.h>

#define ITEM_GFX_MAX 16
#define ITEM_GFX_DEPTHS 4

static tBitMap *s_floorGfx[ITEM_GFX_MAX][ITEM_GFX_DEPTHS];
static tBitMap *s_floorMsk[ITEM_GFX_MAX][ITEM_GFX_DEPTHS];
static tBitMap *s_hudGfx[ITEM_GFX_MAX];
static tBitMap *s_hudMsk[ITEM_GFX_MAX];

static void itemGfxFreeBitmaps(void)
{
    for (UBYTE i = 0; i < ITEM_GFX_MAX; i++) {
        for (UBYTE d = 0; d < ITEM_GFX_DEPTHS; d++) {
            if (s_floorGfx[i][d]) {
                bitmapDestroy(s_floorGfx[i][d]);
                s_floorGfx[i][d] = NULL;
            }
            if (s_floorMsk[i][d]) {
                bitmapDestroy(s_floorMsk[i][d]);
                s_floorMsk[i][d] = NULL;
            }
        }
        if (s_hudGfx[i]) {
            bitmapDestroy(s_hudGfx[i]);
            s_hudGfx[i] = NULL;
        }
        if (s_hudMsk[i]) {
            bitmapDestroy(s_hudMsk[i]);
            s_hudMsk[i] = NULL;
        }
    }
}

static UBYTE itemGfxLoadPair(const char *bmPath, const char *mskPath, tBitMap **ppGfx, tBitMap **ppMsk)
{
    tBitMap *gfx;
    tBitMap *msk;
    if (!diskFileExists(bmPath) || !diskFileExists(mskPath))
        return 0;
    gfx = bitmapCreateFromPath(bmPath, 0);
    msk = bitmapCreateFromPath(mskPath, 0);
    if (gfx && msk && msk->Planes[0]) {
        *ppGfx = gfx;
        *ppMsk = msk;
        return 1;
    }
    if (gfx)
        bitmapDestroy(gfx);
    if (msk)
        bitmapDestroy(msk);
    return 0;
}

void itemGfxDestroy(void)
{
    itemGfxFreeBitmaps();
}

void itemGfxLoad(void)
{
    UBYTE n;
    itemGfxFreeBitmaps();
    n = s_ubItemCount;
    if (n > ITEM_GFX_MAX)
        n = ITEM_GFX_MAX;
    for (UBYTE i = 0; i < n; i++) {
        for (UBYTE d = 0; d < ITEM_GFX_DEPTHS; d++) {
            char bm[40];
            char msk[40];
            sprintf(bm, "data/items/%u_d%u.bm", (unsigned)i, (unsigned)d);
            sprintf(msk, "data/items/%u_d%u.msk", (unsigned)i, (unsigned)d);
            itemGfxLoadPair(bm, msk, &s_floorGfx[i][d], &s_floorMsk[i][d]);
        }
        {
            char bm[40];
            char msk[40];
            sprintf(bm, "data/items/%u_ui.bm", (unsigned)i);
            sprintf(msk, "data/items/%u_ui.msk", (unsigned)i);
            if (itemGfxLoadPair(bm, msk, &s_hudGfx[i], &s_hudMsk[i]))
                logWrite("items: icon %u\n", (unsigned)i);
        }
    }
}

UBYTE itemGfxFloor(UBYTE itemIdx, UBYTE depth, tBitMap **ppGfx, tBitMap **ppMask)
{
    if (ppGfx)
        *ppGfx = NULL;
    if (ppMask)
        *ppMask = NULL;
    if (itemIdx >= ITEM_GFX_MAX || depth >= ITEM_GFX_DEPTHS)
        return 0;
    if (!s_floorGfx[itemIdx][depth] || !s_floorMsk[itemIdx][depth])
        return 0;
    if (ppGfx)
        *ppGfx = s_floorGfx[itemIdx][depth];
    if (ppMask)
        *ppMask = s_floorMsk[itemIdx][depth];
    return 1;
}

UBYTE itemGfxHud(UBYTE itemIdx, tBitMap **ppGfx, tBitMap **ppMask)
{
    if (ppGfx)
        *ppGfx = NULL;
    if (ppMask)
        *ppMask = NULL;
    if (itemIdx >= ITEM_GFX_MAX || !s_hudGfx[itemIdx] || !s_hudMsk[itemIdx])
        return 0;
    if (ppGfx)
        *ppGfx = s_hudGfx[itemIdx];
    if (ppMask)
        *ppMask = s_hudMsk[itemIdx];
    return 1;
}

tItem* s_vecItems = NULL;
UBYTE s_ubItemCount = 0;

void itemSystemInit(void)
{
    itemGfxDestroy();
    s_vecItems = NULL;
    s_ubItemCount = 0;
}

void itemSystemDestroy(void)
{
    itemGfxDestroy();
    if (s_vecItems)
    {
        // Free item names if allocated
        for (UBYTE i = 0; i < s_ubItemCount; i++)
        {
            if (s_vecItems[i].pszName)
            {
                memFree(s_vecItems[i].pszName, strlen(s_vecItems[i].pszName) + 1);
            }
        }
        memFree(s_vecItems, sizeof(tItem) * s_ubItemCount);
        s_vecItems = NULL;
        s_ubItemCount = 0;
    }
}

/* Item file format: UBYTE count; then per item: 11 bytes (type,subType,flags,modType,mod,value,weight,icon,usageType,keyId,equipSlot), UBYTE nameLen, nameLen bytes */
static void createFallbackItems(void)
{
    itemSystemInit();
    s_ubItemCount = 2;
    s_vecItems = (tItem*)memAllocFastClear(sizeof(tItem) * s_ubItemCount);
    if (!s_vecItems) { s_ubItemCount = 0; return; }
    /* Item 0: Key */
    s_vecItems[0].ubType = 0;
    s_vecItems[0].ubSubType = 0;
    s_vecItems[0].ubFlags = ITEM_FLAG_KEY;
    s_vecItems[0].ubUsageType = ITEM_USAGE_UNLOCK;
    s_vecItems[0].ubKeyId = 1;
    s_vecItems[0].ubEquipSlot = 255;
    s_vecItems[0].pszName = (char*)memAllocFastClear(4);
    if (s_vecItems[0].pszName) { s_vecItems[0].pszName[0] = 'K'; s_vecItems[0].pszName[1] = 'e'; s_vecItems[0].pszName[2] = 'y'; }
    /* Item 1: Potion */
    s_vecItems[1].ubType = 1;
    s_vecItems[1].ubSubType = 0;
    s_vecItems[1].ubFlags = ITEM_FLAG_CONSUMABLE | ITEM_FLAG_ACTIVATABLE | ITEM_FLAG_STACKABLE;
    s_vecItems[1].ubUsageType = ITEM_USAGE_HEAL;
    s_vecItems[1].ubValue = 20;
    s_vecItems[1].ubEquipSlot = 255;
    s_vecItems[1].pszName = (char*)memAllocFastClear(7);
    if (s_vecItems[1].pszName) { s_vecItems[1].pszName[0] = 'P'; s_vecItems[1].pszName[1] = 'o'; s_vecItems[1].pszName[2] = 't'; s_vecItems[1].pszName[3] = 'i'; s_vecItems[1].pszName[4] = 'o'; s_vecItems[1].pszName[5] = 'n'; }
    itemGfxLoad();
}

void loadItems(const char* filename)
{
    itemSystemInit();
    tFile* pFile = diskFileOpen(filename, DISK_FILE_MODE_READ, 1);
    if (!pFile) {
        logWrite("Items: file not found, using fallback items\n");
        createFallbackItems();
        return;
    }
    UBYTE count = 0;
    fileReadBytes(pFile, &count, 1);
    if (count == 0 || count > 128) {
        fileClose(pFile);
        createFallbackItems();
        return;
    }
    s_vecItems = (tItem*)memAllocFastClear(sizeof(tItem) * count);
    if (!s_vecItems) {
        fileClose(pFile);
        s_ubItemCount = 0;
        return;
    }
    s_ubItemCount = count;
    for (UBYTE i = 0; i < count; i++) {
        tItem* p = &s_vecItems[i];
        fileReadBytes(pFile, &p->ubType, 1);
        fileReadBytes(pFile, &p->ubSubType, 1);
        fileReadBytes(pFile, &p->ubFlags, 1);
        fileReadBytes(pFile, &p->ubModifierType, 1);
        fileReadBytes(pFile, &p->ubModifier, 1);
        fileReadBytes(pFile, &p->ubValue, 1);
        fileReadBytes(pFile, &p->ubWeight, 1);
        fileReadBytes(pFile, &p->ubIcon, 1);
        fileReadBytes(pFile, &p->ubUsageType, 1);
        fileReadBytes(pFile, &p->ubKeyId, 1);
        fileReadBytes(pFile, &p->ubEquipSlot, 1);
        UBYTE nameLen = 0;
        fileReadBytes(pFile, &nameLen, 1);
        p->pszName = NULL;
        if (nameLen > 0 && nameLen < 64) {
            p->pszName = (char*)memAllocFastClear(nameLen + 1);
            if (p->pszName) {
                fileReadBytes(pFile, (UBYTE*)p->pszName, nameLen);
                p->pszName[nameLen] = '\0';
            } else {
                for (UBYTE k = 0; k < nameLen; k++) { UBYTE b; fileReadBytes(pFile, &b, 1); }
            }
        } else if (nameLen > 0) {
            for (UBYTE k = 0; k < nameLen; k++) { UBYTE b; fileReadBytes(pFile, &b, 1); }
        }
    }
    fileClose(pFile);
    logWrite("Items: loaded %d items from %s\n", (int)s_ubItemCount, filename);
    itemGfxLoad();
}

void saveItems(const char* filename)
{
    if (!filename || !s_vecItems || s_ubItemCount == 0)
        return;
    tFile* pFile = diskFileOpen(filename, DISK_FILE_MODE_WRITE, 1);
    if (!pFile) {
        logWrite("Items: could not write %s\n", filename);
        return;
    }
    fileWriteBytes(pFile, &s_ubItemCount, 1);
    for (UBYTE i = 0; i < s_ubItemCount; i++) {
        tItem* p = &s_vecItems[i];
        fileWriteBytes(pFile, &p->ubType, 1);
        fileWriteBytes(pFile, &p->ubSubType, 1);
        fileWriteBytes(pFile, &p->ubFlags, 1);
        fileWriteBytes(pFile, &p->ubModifierType, 1);
        fileWriteBytes(pFile, &p->ubModifier, 1);
        fileWriteBytes(pFile, &p->ubValue, 1);
        fileWriteBytes(pFile, &p->ubWeight, 1);
        fileWriteBytes(pFile, &p->ubIcon, 1);
        fileWriteBytes(pFile, &p->ubUsageType, 1);
        fileWriteBytes(pFile, &p->ubKeyId, 1);
        fileWriteBytes(pFile, &p->ubEquipSlot, 1);
        UBYTE nameLen = 0;
        if (p->pszName)
            nameLen = (UBYTE)strlen(p->pszName);
        if (nameLen > 63)
            nameLen = 63;
        fileWriteBytes(pFile, &nameLen, 1);
        if (nameLen > 0)
            fileWriteBytes(pFile, (const UBYTE*)p->pszName, nameLen);
    }
    fileClose(pFile);
    logWrite("Items: saved %d items to %s\n", (int)s_ubItemCount, filename);
}

tItem* getItem(UBYTE ubIndex)
{
    if (ubIndex >= s_ubItemCount || !s_vecItems)
        return NULL;
    return &s_vecItems[ubIndex];
}

UBYTE findItemByName(const char* pszName)
{
    if (!s_vecItems || !pszName)
        return 255; // Invalid index
    
    for (UBYTE i = 0; i < s_ubItemCount; i++)
    {
        if (s_vecItems[i].pszName && strcmp(s_vecItems[i].pszName, pszName) == 0)
        {
            return i;
        }
    }
    return 255; // Not found
}

UBYTE findItemByKeyId(UBYTE ubKeyId)
{
    if (!s_vecItems)
        return 255; // Invalid index
    
    for (UBYTE i = 0; i < s_ubItemCount; i++)
    {
        if ((s_vecItems[i].ubFlags & ITEM_FLAG_KEY) && s_vecItems[i].ubKeyId == ubKeyId)
        {
            return i;
        }
    }
    return 255; // Not found
}