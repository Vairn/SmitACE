# SmitACE status and to-do

Source review of the game and `tools/smite_editor`, 6 October 2026. This replaces the checklist in `TODO.md`, which still described an empty party and unused combat.

The PC editor stays the existing C++ ImGui app (`tools/smite_editor`, “Smite Authoring Tool”). It already speaks the same binary formats the Amiga game loads. The work is to finish that tool, then close the gameplay gaps that make authored data invisible or broken in play.

## What a run is today

New Game loads `data/game.smt`, then the listed maze, wallset, and optional `.lvl`. Level 0 with an empty maze path is the built-in 32×32 demo plus `data/factory2/factory2.wll`.

You can walk and turn (keyboard and the on-screen pad), open and close doors, unlock a door when the pack holds the key, read messages, and hear `data/suspense.mod`. Battery drops one per step. At zero the No Battery screen returns to the title. `EVENT_WIN` reaches the win screen. F9 pauses. Wall buttons, door buttons, and pressure plates work when a `.lvl` places them.

One default hero is created (level 1, 30 HP, attack 6, defense 3). Monsters from `.lvl` or `EVENT_ADDMONSTER` wander, chase, and flee, drawn as colored rectangles. Sharing a cell with an aggressive monster trades hits every frame.

## Editor: what C++ already does

`tools/smite_editor` is Dear ImGui + GLFW + OpenGL. File formats live in `src/formats.cpp` and match the game loaders.

| Panel | Load / save | What you can actually edit |
| --- | --- | --- |
| Project | `game.smt` | Start level, level list, paths for items, monsters, palette, maze, wallset, `.lvl` |
| Items | `items.dat` | Add a row. On screen: type, flags, name. The file struct also has subtype, modifier, value, weight, icon, usage, key id, equip slot |
| Monsters | `monsters.dat` | Add a row. On screen: level, HP, attack, defense, XP, name. Aggro, flee, and the 8 drop slots are in the struct and absent from the form |
| Maze | any `.maze` path | Click cells to cycle types 0–5. Add an event and set x, y, type. Payload is shown as a byte count. Floor layer, color layer, and the string table have no UI |
| Level entities | any `.lvl` path | Counts, plus buttons that append an empty wall button or monster spawn. Door buttons, locks, plates, and ground items cannot be added or edited |
| Wallset preview | `.wll` + `_0.pln` | Shows one decoded planar page. Reload always uses `factory2/factory2.wll`, not the path in the manifest |
| Dungeon atlas | PNG / JSON | Separate wall-art capture and packer. It does not write `.wll` tile records |

Reload on startup looks for `level01.maze` and `level00.lvl` beside the data root, even when `game.smt` names other files. The File menu can reload and quit. There is no save-all and no prompt when a buffer is dirty.

## To-do: finish the C++ editor

Do this before adding a second editor. Python is not part of the plan.

### 1. Make every loaded field editable

- [ ] Items form: subtype, flags as named checkboxes (`ITEM_FLAG_*` in `include/item.h`), modifier type and amount, value, weight, icon, usage type, key id, equip slot. Delete row. Show the record index the game uses.
- [ ] Monsters form: aggro range, flee percent, eight drop item indices and chances. Delete row. Show the type id (the record index).
- [ ] Level entities: a table per section (wall button, door button, lock, plate, ground item, monster) with add, delete, and every field from `docs/formats/level_entities.md`.
- [ ] Maze events: opcode name from `include/script.h`, plus a payload editor that matches `docs/formats/script_events.md` (message id, door x/y, item index and quantity, flag, monster type and position, and so on).
- [ ] Maze string table: edit the strings `EVENT_SHOWMESSAGE` indexes.
- [ ] Floor and column layers (`_mazeFloor`, `_mazeCol`), which the file already stores and the click grid ignores.
- [ ] On Reload, open the maze, wallset, and `.lvl` named by level 0 in `game.smt`, and fall back to the current hardcoded names when those paths are empty.

### 2. Author on the map

- [ ] Replace the button-per-cell grid with a painted map: left button sets a selected cell type, right button samples. Color cells (floor, wall, door, locked, open, event).
- [ ] Click a cell to select it, then edit the events and `.lvl` records that sit on that cell.
- [ ] Place and drag monster spawns, ground items, plates, locks, and buttons on that map.
- [ ] Resize width and height without wiping existing cells (the current width/height inputs rebuild the grid as walls).

### 3. Save behavior

- [ ] One Save all that writes `game.smt`, `items.dat`, `monsters.dat`, the open maze, and the open `.lvl`.
- [ ] Show load and save errors in the Project panel. Several save buttons currently ignore the error string.
- [ ] Mark the window dirty when a field changes, and ask before Reload or Quit.

### 4. Later, still in this tool

- [ ] Opcode dropdowns limited to events the game actually runs (see the script section below), so new content does not depend on dead opcodes.
- [ ] Wallset tile list from `WallsetFile::tiles` (type, location, screen rect). The atlas window can stay the art generator.
- [ ] A first-person preview is optional and large. Skip it until the map editor can produce a maze the game already knows how to run.

## To-do: gameplay the editor is aiming at

Ordered so a maze you save from the editor does something visible in the game.

### Bugs

- [ ] Melee in `gameProcessMeleeCombat` (`src/game/game.c`) runs every frame. Put it on the same kind of cooldown as monster movement. When the hero’s HP hits 0, go to the game-over screen. Battery 0 already does that.
- [ ] Standing on a battery charger (`updateStandingOnEventTrigger`) recharges every frame. Tick it with the charger’s own ~1 second timer in `updateBatteryChargers`.
- [ ] `SaveGameState("data/save.dat")` has no caller. Add a key (and a pause-menu action once the menu gadget works). Title Load Game should say so when the file is missing or the version is wrong.
- [ ] Extend the save (currently version 2) with hero HP, XP, and the four equipped slots. Loading a level still rebuilds doors, monsters, and ground items from the maze and `.lvl`.
- [ ] Rebuild the minimap when doors change. `drawFullScreenMap` builds `g_pMazeBitmap` once.
- [ ] The four “unlock” events in `mazeCreateDemoData` are `EVENT_CLOSEDOOR` (type 10) and they replace those cells with walkable triggers. Either point them at a real open-door opcode or delete them.

### So authored entities are playable

- [ ] Pick up ground items when the party steps on them (`groundItemRemoveAt` exists and is never called). Give a one-line message.
- [ ] Inventory: left click uses a consumable (heal and energy already work in `inventoryUseItem`). Right click equips into `ubEquipSlot` via `inventoryEquipItem`, which currently has no caller. Draw the item name in the slot. Icons can wait.
- [ ] Combat message in the text field: who hit whom, for how much, and when a monster dies. Loot from `monsterDropLoot` should say what entered the pack.
- [ ] `EVENT_DAMAGE` currently subtracts from battery. Decide whether traps hurt the battery or the hero’s HP, and make the opcode match that.
- [ ] A way to leave the level. `LoadLevel` works. Nothing calls it after startup. A script opcode (or an exit cell) that takes a level id is enough for the editor to target.
- [ ] Wire the playfield menu region (`GAME_UI_GADGET_MENU` in `game_ui.c`). It has no click handler. Pause or save belongs there.

### Script opcodes to treat as real

These run today and the editor should offer them by name:

Set/clear wall, floor, and color. Show message. Open and close door. Teleport. Give and take item. Set and clear flag. Add and remove monster. Add XP. Damage. Turn. Battery charger. If / else / endif. Goto, gosub, return. Win.

If conditions that evaluate: local and global flags, party position, facing, class, race, and AND/OR.

### Leave unwired until a feature exists

Do not expose these as normal authoring choices until the game implements them:

- [ ] `EVENT_SOUND` and `EVENT_ENCOUNTER` only log an id.
- [ ] `EVENT_ADDITEM`, `EVENT_REMOVEITEM`, `EVENT_LAUNCHER`, `EVENT_IDENTIFYITEMS`, and `EVENT_DOOR_ANIM_1`–`6` hit the default handler.
- [ ] If-operands with no code: dice, party visible, trigger flag, pointer item, wall side, monsters on a cell, items on a cell, wall number.
- [ ] Code locks: `doorLockTryUnlockWithCode` has no input UI. Script-type locks always refuse.
- [ ] Sound effects. Music already plays.
- [ ] Level-up from XP. The number is stored and never read.
- [ ] Spells, resistances, and a party larger than the one default hero.
- [ ] Joystick movement in the dungeon. Fire already skips logo, win, game over, and pause.
- [ ] Item icons. Monster pictures are a separate plan, below.
- [ ] Corner-map update is listed under bugs. Full-screen map already works.

## How to draw real monsters

`drawMonsterPlaceholder` in `src/game/Renderer.c` paints a colored rectangle on the floor tile of each visible slot. The slot is already the right anchor. The picture should be a masked blit, the same kind of blit walls and doors use (`blitUnsafeCopyMask`).

Hardware sprites do not fit. The mouse already owns sprite channels, a sprite is 16 pixels wide, and a creature one step away is much wider than that. The ACE bob manager’s save-and-restore path is also the wrong tool: the dungeon view is redrawn as a whole whenever `g_ubRedrawRequire` is set. A monster is just another cookie-cut blit into that view.

The blitter does not scale. The 18 view slots are only four distances, so each monster type needs four pre-drawn sizes, not eighteen pictures:

| Slot `yDelta` | Where it is | Frame |
| --- | --- | --- |
| 0 | On your cell, or the cell beside you | Largest |
| -1 | One step ahead (left, center, right) | Large |
| -2 | Two steps ahead | Medium |
| -3 | Three steps ahead | Small |

`xDelta` only moves the picture. Feet sit on the bottom center of that slot’s floor tile, which is the same point the rectangle uses now (`floor->_screen` plus the tile size). The body is allowed to rise above the floor quad. Clip to the viewport.

Draw each creature at the end of its slot, while slots still run from far to near. Today every wall is drawn first and every monster second, which is fine for a tiny rectangle. A tall picture drawn that way will cover a nearer wall. Far-to-near, slot by slot, lets the nearer wall paint over the feet and legs that stick out.

Palette for the first version: the viewport already forces bitplane 5, so wall art lands in colors 32–63. Draw the creature in that same 32-color wallset palette. No new bitplanes. When a level changes wallset and the creature colors go wrong, move creatures to a shared bank at colors 96–127 (screen is already 8 bitplanes; 96–255 are unused). That second step means the blit must set bitplanes 5 and 6 on the masked pixels.

Files: one front-view image per type is enough to start. The C++ editor scales it to the four depths (it already has OpenGL and stb) and writes a planar bitmap plus a mask per depth, named by the monster’s record index in `monsters.dat`. The game loader picks the frame from `yDelta`. Facing and a second animation frame can wait. `tMonster` has no facing field, and dead monsters are already skipped.

- [ ] Replace the rectangle with the four-depth masked blit, feet on the floor tile, drawn inside the far-to-near slot loop.
- [ ] Add the four frames to the monster record in the C++ editor, generated from one imported image.
- [ ] Keep the rectangle when a type has no frames, so old data still runs.

## Decisions still open

These change the editor’s forms. Current code is the default if you want to keep moving:

1. Combat stays same-cell and automatic, with a cooldown and messages, until an encounter screen exists.
2. The party stays one hero.
3. `EVENT_ENCOUNTER` stays unused.
4. Level changes are a new script opcode, not a world map.
5. Winning stays `EVENT_WIN`.
6. A save keeps position, battery, pack, flags, hero HP, hero XP, and equipped slots. The maze itself is not saved; it reloads from the level files.
