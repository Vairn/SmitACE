#pragma once

#include <ace/types.h>

#ifndef AMIGA
#include "amiTypes.h"
#endif

#define GAME_MANIFEST_MAGIC "SMTE"
#define GAME_MANIFEST_VERSION 1
#define GAME_MANIFEST_PATH_MAX 64
#define GAME_MANIFEST_MAX_LEVELS 64

typedef struct tGameLevelEntry {
	char mazePath[GAME_MANIFEST_PATH_MAX];
	char wallsetPath[GAME_MANIFEST_PATH_MAX];
	char entitiesPath[GAME_MANIFEST_PATH_MAX];
} tGameLevelEntry;

typedef struct tGameManifest {
	UBYTE formatVersion;
	UBYTE startLevel;
	UBYTE levelCount;
	char itemsPath[GAME_MANIFEST_PATH_MAX];
	char monstersPath[GAME_MANIFEST_PATH_MAX];
	char uiPalettePath[GAME_MANIFEST_PATH_MAX];
	tGameLevelEntry levels[GAME_MANIFEST_MAX_LEVELS];
} tGameManifest;

/** Default paths when data/game.smt is missing or invalid. */
void gameManifestSetDefaults(tGameManifest *p);

/** Load manifest from disk; on failure applies defaults and returns 0. */
UBYTE gameManifestLoad(tGameManifest *p, const char *szPath);

const tGameManifest *gameManifestGet(void);

/** Call once at startup (InitNewGame / savegame load). Safe to call again to reload. */
void gameManifestEnsureLoaded(const char *szPath);
