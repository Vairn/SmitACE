#pragma once

#include <ace/types.h>

#ifndef AMIGA
#include "amiTypes.h"
#endif

struct _tGameState;
typedef struct _tGameState tGameState;

#define LEVEL_ENTITIES_MAGIC "LVLE"
#define LEVEL_ENTITIES_VERSION 1
/** In .lvl wall/door button records: use this gfxIndex to pick first matching tile in wallset. */
#define LEVEL_ENT_GFX_AUTO_WALL_BTN 255
#define LEVEL_ENT_GFX_AUTO_DOOR_BTN 254

/** Load level bundle (.lvl): wall buttons, door buttons, door locks, pressure plates, ground items, monster spawns. */
UBYTE levelEntitiesLoad(tGameState *pState, const char *szPath);
