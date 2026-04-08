# `monsters.dat` (version 1)

| Field | Size | Description |
|-------|------|-------------|
| Magic | 4 | `MONS` |
| Version | 1 | `1` |
| Count | 1 | 1–64 |
| Records | * | `count` times |

Per record:

| Field | Size | Endian |
|-------|------|--------|
| Level | 1 | — |
| MaxHP | 2 | **big-endian** |
| Attack | 1 | — |
| Defense | 1 | — |
| Experience | 2 | **big-endian** |
| Aggro range | 1 | — |
| Flee threshold % | 1 | — |
| Drop table | 8 | item indices |
| Drop chances | 1 | 0–99 per slot |
| Name length | 1 | — |
| Name | *n* | ASCII (not NUL-terminated) |

Monster id used in scripts and `EVENT_ADDMONSTER` is the **0-based record index**.
