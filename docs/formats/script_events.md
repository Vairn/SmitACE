# Maze script / event opcodes (`EVENT_*`)

Authoritative payload layouts for maze events consumed by `handleEvent()` / `executeScript()` in [`src/misc/script.c`](../../src/misc/script.c). Cell coordinates in payloads are **maze cell indices** (0 … width−1 / height−1). Multi-byte integers in **event data** are usually **little-endian** unless noted (Amiga CPU endian does not apply to arbitrary byte streams—keep payloads consistent with existing files).

| Opcode | Value | Payload |
|--------|------|---------|
| `EVENT_SETWALL` | 0 | 1 byte: new cell type for `_mazeData` at event cell |
| `EVENT_SETFLOOR` | 1 | 1 byte: floor value |
| `EVENT_SETCOL` | 2 | 1 byte: column value |
| `EVENT_CLEARWALL` | 3 | *(none)* |
| `EVENT_CLEARFLOOR` | 4 | *(none)* |
| `EVENT_CLEARCOL` | 5 | *(none)* |
| `EVENT_SETWALLCOL` | 6 | ≥2 bytes: wall type, col |
| `EVENT_CLEARWALLCOL` | 7 | *(none)* |
| `EVENT_SHOWMESSAGE` | 8 | ≥1 byte: maze string index; if ≥2 bytes, `id = data[0] \| (data[1]<<8)` |
| `EVENT_OPENDOOR` | 9 | 0 bytes: open at event cell; or ≥2 bytes: door `x`, `y` |
| `EVENT_CLOSEDOOR` | 10 | *(none)* — closes at event cell |
| `EVENT_DOOR_ANIM_1` … `EVENT_DOOR_ANIM_6` | 11–16 | *(see script.c if wired)* |
| `EVENT_TELEPORT` | 17 | ≥2 bytes: `targetX`, `targetY` |
| `EVENT_GIVEITEM` | 18 | ≥1 byte: item index; optional byte 2: quantity (default 1) |
| `EVENT_TAKEITEM` | 19 | same as give |
| `EVENT_SETFLAG` | 20 | ≥2 bytes: `flagType` (0 local, 1 global), `flagIndex`; optional byte 3: value (default 1) |
| `EVENT_CLEARFLAG` | 21 | ≥2 bytes: `flagType`, `flagIndex` |
| `EVENT_ADDMONSTER` | 22 | ≥3 bytes: `monsterTypeId`, `x`, `y` (uses [`monsters.dat`](monsters_dat.md) / built-in table) |
| `EVENT_REMOVEMONSTER` | 23 | ≥2 bytes: `x`, `y` |
| `EVENT_ADDITEM` / `EVENT_REMOVEITEM` | 24–25 | *(if implemented—match give/take)* |
| `EVENT_ADDXP` | 26 | ≥1 byte: XP amount LE (`data[0] \| (data[1]<<8)`) |
| `EVENT_DAMAGE` | 27 | ≥1 byte: damage to battery |
| `EVENT_LAUNCHER` | 28 | *(launcher-specific)* |
| `EVENT_TURN` | 29 | ≥2 bytes: `direction` (0=left, non-zero=right), `count` |
| `EVENT_IDENTIFYITEMS` | 30 | *(optional payload)* |
| `EVENT_ENCOUNTER` | 31 | ≥1 byte: encounter id |
| `EVENT_SOUND` | 32 | ≥1 byte: sound id |
| `EVENT_WIN` | 33 | *(none)* |
| `EVENT_IF` | 128 | condition subprogram (nested opcodes; see `evaluateCondition`) |
| `EVENT_ELSE` | 129 | *(none)* |
| `EVENT_ENDIF` | 130 | *(none)* |
| `EVENT_GOTO` | 131 | ≥1 byte: target **event ordinal** in maze list |
| `EVENT_GOSUB` | 132 | ≥1 byte: target ordinal |
| `EVENT_RETURN` | 133 | *(none)* |
| `EVENT_BATTERY_CHARGER` | 221 | ≥1 byte: charges remaining (decremented in place) |

Condition primitives (`EVENT_EQUAL`, `EVENT_LEVEL_FLAG`, …) are **not** meant to be executed as standalone `handleEvent` actions; they appear inside `EVENT_IF` data. See `evaluateCondition()` in `script.c`.

Also see constants in [`include/script.h`](../../include/script.h).
