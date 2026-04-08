# `game.smt` (version 1)

| Field | Size | Description |
|-------|------|-------------|
| Magic | 4 | `SMTE` |
| Version | 1 | `1` |
| Start level | 1 | Passed to `LoadLevel()` on new game |
| Level count | 1 | 1–64 |
| Items path | 64 | NUL-padded ASCII (`data/items.dat`) |
| Monsters path | 64 | `data/monsters.dat` |
| UI palette path | 64 | e.g. `data/playfield.plt` |
| Levels | `count` × 3×64 | For each: maze path, wallset path, entities (`.lvl`) path |

**Level 0 maze path empty** → engine uses built-in `mazeCreateDemoData()` (same as legacy demo).

Paths are relative to the game working directory (typically `data/` next to the executable).
