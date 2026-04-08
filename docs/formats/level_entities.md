# Level entities `.lvl` (magic `LVLE`, version 1)

Binary layout after magic + version:

| Section | Count byte | Records |
|---------|------------|---------|
| Wall buttons | `n` | `n` × (x, y, wallSide, type, gfxIndex, eventType, eventDataSize, eventData…) |
| Door buttons | `n` | `n` × (x, y, wallSide, type, gfxIndex, targetDoorX, targetDoorY) |
| Door locks | `n` | `n` × (doorX, doorY, type, keyId, code) |
| Pressure plates | `n` | `n` × (x, y, eventType, dataSize, data[0..dataSize-1], max 8) |
| Ground items | `n` | `n` × (x, y, itemIdx, qty) |
| Monster spawns | `n` | `n` × (monsterTypeId, x, y) |

`gfxIndex` **255** / **254** resolve to the first wallset tile of type `WALL_GFX_WALL_BUTTON` / `WALL_GFX_DOOR_BUTTON` (see [`level_entities.h`](../../include/level_entities.h)).

Loader: [`levelEntitiesLoad`](../../src/game/level_entities.c).
