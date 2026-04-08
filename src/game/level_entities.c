#include "level_entities.h"
#include "GameState.h"
#include "wallbutton.h"
#include "doorbutton.h"
#include "doorlock.h"
#include "ground_item.h"
#include "pressure_plate.h"
#include "monster.h"
#include "wallset.h"
#include <ace/utils/disk_file.h>
#include <ace/utils/file.h>
#include <ace/managers/log.h>
#include <string.h>

#define WALL_BTN_EVENT_MAX 32

static UBYTE levelEntResolveWallGfx(tWallset *ws, UBYTE gfxIndex)
{
	if (!ws || gfxIndex != LEVEL_ENT_GFX_AUTO_WALL_BTN)
		return gfxIndex;
	for (UWORD i = 0; i < ws->_tilesetCount; i++) {
		if (ws->_tileset[i]->_type == WALL_GFX_WALL_BUTTON)
			return (UBYTE)i;
	}
	return 0;
}

static UBYTE levelEntResolveDoorGfx(tWallset *ws, UBYTE gfxIndex)
{
	if (!ws || gfxIndex != LEVEL_ENT_GFX_AUTO_DOOR_BTN)
		return gfxIndex;
	for (UWORD i = 0; i < ws->_tilesetCount; i++) {
		if (ws->_tileset[i]->_type == WALL_GFX_DOOR_BUTTON)
			return (UBYTE)i;
	}
	return 0;
}

UBYTE levelEntitiesLoad(tGameState *pState, const char *szPath)
{
	if (!pState || !szPath || !szPath[0])
		return 1;
	tFile *f = diskFileOpen(szPath, DISK_FILE_MODE_READ, 1);
	if (!f) {
		logWrite("levelEntities: '%s' not found (optional)\n", szPath);
		return 1;
	}
	char magic[4];
	fileRead(f, magic, 4);
	if (magic[0] != 'L' || magic[1] != 'V' || magic[2] != 'L' || magic[3] != 'E') {
		fileClose(f);
		logWrite("levelEntities: bad magic in %s\n", szPath);
		return 0;
	}
	UBYTE ver = 0;
	fileRead(f, &ver, 1);
	if (ver != LEVEL_ENTITIES_VERSION) {
		fileClose(f);
		logWrite("levelEntities: bad version %u\n", (unsigned)ver);
		return 0;
	}

	UBYTE nWall = 0;
	fileRead(f, &nWall, 1);
	for (UBYTE i = 0; i < nWall; i++) {
		UBYTE x, y, wallSide, type, gfxIndex, eventType, esz;
		fileRead(f, &x, 1);
		fileRead(f, &y, 1);
		fileRead(f, &wallSide, 1);
		fileRead(f, &type, 1);
		fileRead(f, &gfxIndex, 1);
		fileRead(f, &eventType, 1);
		fileRead(f, &esz, 1);
		UBYTE buf[WALL_BTN_EVENT_MAX];
		if (esz > WALL_BTN_EVENT_MAX) {
			for (UBYTE k = 0; k < esz; k++) {
				UBYTE b;
				fileRead(f, &b, 1);
			}
			esz = 0;
		} else 		if (esz > 0) {
			fileRead(f, buf, esz);
		}
		gfxIndex = levelEntResolveWallGfx(pState->m_pCurrentWallset, gfxIndex);
		tWallButton *wb = wallButtonCreateWithPayload(x, y, wallSide, type, gfxIndex, eventType, esz, esz ? buf : NULL);
		if (wb)
			wallButtonAdd(&pState->m_wallButtons, wb);
	}

	UBYTE nDoor = 0;
	fileRead(f, &nDoor, 1);
	for (UBYTE i = 0; i < nDoor; i++) {
		UBYTE x, y, wallSide, type, gfxIndex, tx, ty;
		fileRead(f, &x, 1);
		fileRead(f, &y, 1);
		fileRead(f, &wallSide, 1);
		fileRead(f, &type, 1);
		fileRead(f, &gfxIndex, 1);
		fileRead(f, &tx, 1);
		fileRead(f, &ty, 1);
		gfxIndex = levelEntResolveDoorGfx(pState->m_pCurrentWallset, gfxIndex);
		tDoorButton *db = doorButtonCreate(x, y, wallSide, type, gfxIndex, tx, ty);
		if (db)
			doorButtonAdd(&pState->m_doorButtons, db);
	}

	UBYTE nLock = 0;
	fileRead(f, &nLock, 1);
	for (UBYTE i = 0; i < nLock; i++) {
		UBYTE doorX, doorY, type, keyId, code;
		fileRead(f, &doorX, 1);
		fileRead(f, &doorY, 1);
		fileRead(f, &type, 1);
		fileRead(f, &keyId, 1);
		fileRead(f, &code, 1);
		tDoorLock *lk = doorLockCreate(doorX, doorY, type, keyId);
		if (lk) {
			lk->_code = code;
			doorLockAdd(&pState->m_doorLocks, lk);
		}
	}

	UBYTE nPlate = 0;
	fileRead(f, &nPlate, 1);
	for (UBYTE i = 0; i < nPlate; i++) {
		UBYTE x, y, eventType, dataSize;
		fileRead(f, &x, 1);
		fileRead(f, &y, 1);
		fileRead(f, &eventType, 1);
		fileRead(f, &dataSize, 1);
		UBYTE pdata[PRESSURE_PLATE_DATA_MAX];
		if (dataSize > PRESSURE_PLATE_DATA_MAX) {
			for (UBYTE k = 0; k < dataSize; k++) {
				UBYTE b;
				fileRead(f, &b, 1);
			}
			continue;
		}
		if (dataSize > 0)
			fileRead(f, pdata, dataSize);
		pressurePlateAdd(&pState->m_pressurePlates, x, y, eventType, dataSize, pdata);
	}

	UBYTE nGround = 0;
	fileRead(f, &nGround, 1);
	for (UBYTE i = 0; i < nGround; i++) {
		UBYTE x, y, itemIdx, qty;
		fileRead(f, &x, 1);
		fileRead(f, &y, 1);
		fileRead(f, &itemIdx, 1);
		fileRead(f, &qty, 1);
		groundItemAdd(&pState->m_groundItems, x, y, itemIdx, qty);
	}

	UBYTE nMon = 0;
	fileRead(f, &nMon, 1);
	logWrite("levelEntities: spawning %u monster(s)\n", (unsigned)nMon);
	for (UBYTE i = 0; i < nMon; i++) {
		UBYTE typeId, mx, my;
		fileRead(f, &typeId, 1);
		fileRead(f, &mx, 1);
		fileRead(f, &my, 1);
		if (pState->m_pMonsterList && pState->m_pMonsterList->_numMonsters < MAX_MONSTERS) {
			tMonster *m = monsterCreate(typeId);
			if (m) {
				monsterPlaceInMaze(pState->m_pCurrentMaze, m, mx, my);
				pState->m_pMonsterList->_monsters[pState->m_pMonsterList->_numMonsters++] = m;
				logWrite("levelEntities: placed type=%u at maze (%u,%u) idx=%u\n", (unsigned)typeId,
					(unsigned)mx, (unsigned)my,
					(unsigned)(pState->m_pMonsterList->_numMonsters - 1));
			} else {
				logWrite("levelEntities: monsterCreate failed type=%u at (%u,%u)\n",
					(unsigned)typeId, (unsigned)mx, (unsigned)my);
			}
		} else {
			logWrite("levelEntities: skip monster type=%u at (%u,%u) (list full or null)\n",
				(unsigned)typeId, (unsigned)mx, (unsigned)my);
		}
	}

	fileClose(f);
	logWrite("levelEntities: loaded %s\n", szPath);
	return 1;
}
