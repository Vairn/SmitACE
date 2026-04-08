#include "game_manifest.h"
#include <ace/utils/disk_file.h>
#include <ace/utils/file.h>
#include <ace/managers/log.h>
#include <string.h>

static tGameManifest s_manifest;
static UBYTE s_manifestReady = 0;

void gameManifestSetDefaults(tGameManifest *p)
{
	if (!p)
		return;
	memset(p, 0, sizeof(tGameManifest));
	p->formatVersion = GAME_MANIFEST_VERSION;
	p->startLevel = 0;
	p->levelCount = 1;
	strncpy(p->itemsPath, "data/items.dat", GAME_MANIFEST_PATH_MAX - 1);
	strncpy(p->monstersPath, "data/monsters.dat", GAME_MANIFEST_PATH_MAX - 1);
	strncpy(p->uiPalettePath, "data/playfield.plt", GAME_MANIFEST_PATH_MAX - 1);
	/* Level 0: built-in demo maze when mazePath[0]=='\0' */
	p->levels[0].mazePath[0] = '\0';
	strncpy(p->levels[0].wallsetPath, "data/factory2/factory2.wll", GAME_MANIFEST_PATH_MAX - 1);
	strncpy(p->levels[0].entitiesPath, "data/level00.lvl", GAME_MANIFEST_PATH_MAX - 1);
}

UBYTE gameManifestLoad(tGameManifest *p, const char *szPath)
{
	if (!p)
		return 0;
	gameManifestSetDefaults(p);
	tFile *f = diskFileOpen(szPath, DISK_FILE_MODE_READ, 1);
	if (!f) {
		logWrite("gameManifest: '%s' not found, using defaults\n", szPath);
		return 0;
	}
	char magic[4];
	fileRead(f, magic, 4);
	if (magic[0] != 'S' || magic[1] != 'M' || magic[2] != 'T' || magic[3] != 'E') {
		fileClose(f);
		logWrite("gameManifest: bad magic\n");
		return 0;
	}
	UBYTE ver = 0;
	fileRead(f, &ver, 1);
	if (ver != 1) {
		fileClose(f);
		logWrite("gameManifest: unsupported version %u\n", (unsigned)ver);
		return 0;
	}
	fileRead(f, &p->startLevel, 1);
	fileRead(f, &p->levelCount, 1);
	if (p->levelCount == 0 || p->levelCount > GAME_MANIFEST_MAX_LEVELS) {
		fileClose(f);
		gameManifestSetDefaults(p);
		return 0;
	}
	fileRead(f, p->itemsPath, GAME_MANIFEST_PATH_MAX);
	p->itemsPath[GAME_MANIFEST_PATH_MAX - 1] = '\0';
	fileRead(f, p->monstersPath, GAME_MANIFEST_PATH_MAX);
	p->monstersPath[GAME_MANIFEST_PATH_MAX - 1] = '\0';
	fileRead(f, p->uiPalettePath, GAME_MANIFEST_PATH_MAX);
	p->uiPalettePath[GAME_MANIFEST_PATH_MAX - 1] = '\0';
	for (UBYTE i = 0; i < p->levelCount; i++) {
		fileRead(f, p->levels[i].mazePath, GAME_MANIFEST_PATH_MAX);
		p->levels[i].mazePath[GAME_MANIFEST_PATH_MAX - 1] = '\0';
		fileRead(f, p->levels[i].wallsetPath, GAME_MANIFEST_PATH_MAX);
		p->levels[i].wallsetPath[GAME_MANIFEST_PATH_MAX - 1] = '\0';
		fileRead(f, p->levels[i].entitiesPath, GAME_MANIFEST_PATH_MAX);
		p->levels[i].entitiesPath[GAME_MANIFEST_PATH_MAX - 1] = '\0';
	}
	fileClose(f);
	p->formatVersion = ver;
	logWrite("gameManifest: loaded %u levels from %s\n", (unsigned)p->levelCount, szPath);
	return 1;
}

const tGameManifest *gameManifestGet(void)
{
	if (!s_manifestReady) {
		gameManifestSetDefaults(&s_manifest);
		s_manifestReady = 1;
	}
	return &s_manifest;
}

void gameManifestEnsureLoaded(const char *szPath)
{
	const char *path = szPath ? szPath : "data/game.smt";
	gameManifestLoad(&s_manifest, path);
	s_manifestReady = 1;
}
