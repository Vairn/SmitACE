# SmitACE – TODO and unfinished work

This file tracks unfinished code, known bugs, and gameplay gaps found during a full codebase review. Items are grouped by area.

---

## Bugs (fix soon)

- [x] **Game Over → wrong state** (fixed) (`src/game/gameOver.c`): `fadeCompleteGameOver()` transitions to `g_sStateWin` (victory screen). It should go to `g_sStateTitle` (or a dedicated game-over loop that then returns to title) so the player sees “you died” and can start again, not the win screen.

---

## Code TODOs (explicit in source)

### Game (`src/game/game.c`)
- [x] **Door locked feedback** (lines ~361, ~406): When the player tries to open a locked door without the key, show a message “Door is locked” (e.g. via `addMessage(..., MESSAGE_TYPE_SMALL, ...)` or a viewport message) instead of doing nothing.

### Script (`src/misc/script.c`)
- [x] **EVENT_SHOWMESSAGE** (line ~253): Implement actual message display. Currently only `logWrite("Showing message ID: %d", ...)`. Need to resolve message ID (e.g. from maze string table or a global message table) and call the game’s message API (e.g. `addMessage` or equivalent) so script-driven messages appear in the UI.
- [x] **EVENT_GIVEITEM** (line ~321): Wire to `inventoryAddItem(g_pGameState->m_pInventory, itemType, quantity)` (and ensure items are loaded and item indices valid).
- [x] **EVENT_TAKEITEM** (line ~330): Wire to `inventoryRemoveItem(g_pGameState->m_pInventory, itemType, quantity)`.
- [x] **EVENT_ADDXP** (line ~418): Apply XP to party (e.g. add to character(s) and handle level-up if you have that system).
- [x] **EVENT_DAMAGE** (line ~426): Apply damage to party (e.g. current character or whole party, and handle death/game over if needed).
- [x] **EVENT_SOUND** (line ~516): Play sound by ID (integrate with ptplayer or a sound effects API).
- [ ] **EVENT_ENCOUNTER** (line ~524): Start encounter by ID. Not implemented; monster/combat system is not used yet (see “Monsters / combat” below).

### Items (`src/items/item.c`)
- [x] **loadItems()** (lines ~37, ~52): Replace placeholder with real data loading (file format TBD). Currently allocates 32 empty slots and never fills them. `loadItems()` is also **never called** (e.g. not from `InitNewGame()` or level load), so `s_ubItemCount` stays 0 and inventory add/remove by item index will not work until this is done.

### Character (`include/character.h`)
- [ ] **Comment** (line ~71): “todo remove after doing proper character stuff for the engine” – `_BatteryLevel` on `tCharacterParty` is used everywhere; either formalize it as part of “character stuff” or document why it lives on the party.

---

## Monsters / combat – not implemented

**Nothing in the monster system is actually used.** The code in `src/misc/monster.c` (create/destroy, update, attack, drop loot, place/remove in maze) and the monster/combat loop in `src/game/game.c` (monsterUpdate, monsterAttack, XP/loot on kill) is **unused or placeholder** – not hooked into real gameplay. Treat it as reference or future scaffolding; monsters and combat still need to be designed and implemented when you add them.

---

## Unimplemented / stub systems

### Save / load (`src/game/gameState.c`)
- [x] **LoadGameState()**: Always returns 0 (failure). No file read or state restore.
- [x] **SaveGameState()**: Always returns 0 (failure). No file write or state snapshot.
- [x] **LoadLevel()**: Always returns 0 (failure). Level loading is not implemented; game uses `mazeCreateDemoData()` only.

### Loading state (`src/game/loading.c`)
- [x] **g_sStateLoading** (minimal stub): Create/Loop/Destroy implemented; Create starts timer, Loop transitions to game after delay. Full level loading will be added in Phase 5 when `LoadLevel()` exists.

### Title screen (`src/game/title.c`)
- [x] **Load Game** (case 1 in `cbOnPressed`): No-op. Should call `LoadGameState(...)`, and on success transition to game state (and optionally loading state first). New Game and Quit work.

### Game flow (`src/smite.c`)
- [x] **Initial state**: Entry point is now `g_sStateTitle`. New Game goes to game; Logo/Intro flow available if entry is changed to `g_sStateLogo`.

### UI handlers (`src/game/game.c`)
- [ ] **handleEquipmentClicked()** / **handleEquipmentUsed()**: Empty; equipment slots do nothing.
- [ ] **handleInventoryClicked()** / **handleInventoryScrolled()**: Empty; inventory UI is not functional.
- [ ] **handleMapClicked()**: Empty; map button does nothing.

---

## Script condition events not implemented

In `evaluateCondition()` in `src/misc/script.c`, these condition types are **not** handled (they fall through to the default “Condition event executed directly” path):

- [x] **EVENT_HAS_CLASS** (202): Check party/character class.
- [x] **EVENT_HAS_RACE** (203): Check party/character race.

Other condition operands (LEVEL_FLAG, GLOBAL_FLAG, PARTY_ON_POS, PARTY_DIRECTION, etc.) are implemented.

---

## Gameplay gaps (design + implementation)

### Party and characters
- **Party is always empty**: `InitNewGame()` creates a `tCharacterParty` but never calls `characterPartyAdd()`. So `_numCharacters` is 0. (The monster/combat code in `game.c` that references “first character” is unused; see “Monsters / combat” above.)
- **Character stats**: Full stat blocks exist (HP, MP, level, equipment, spells, etc.) but there is no character creation, level-up, or equipment UI. No combat or other system currently uses them.

### Combat & encounters
- **Monsters / combat**: Not implemented. The code in `monster.c` and the combat loop in `game.c` is not used. When you add monsters, you will need: monster placement (level data or script), combat mode or auto-attack flow, and any UI (combat screen, messages, etc.).
- **EVENT_ENCOUNTER**: Script event is unimplemented. Design and implement when you add an encounter/combat system.

### Items and inventory
- **Items not loaded**: Until `loadItems()` is implemented and called, item indices in scripts (give/take/add/remove) are invalid and inventory will not show real items.
- **Inventory/equipment UI**: Slots and scroll are wired to empty handlers; no way to use or equip items from the UI yet. Door keys are checked via `inventoryHasItemByKeyId()` and work for locked doors.

### Progression and win condition
- **XP**: Script EVENT_ADDXP is not wired; no level-up logic. (Monster XP-on-kill code exists but is unused; monster system is not implemented.)
- **Damage**: Script EVENT_DAMAGE is not wired; no script-driven damage (e.g. traps).
- **Win condition**: There is a Win state and assets, but no clear trigger (e.g. script “win level” or “reached exit”). Define how the player reaches `g_sStateWin` (e.g. script event, special tile, or “all objectives done”).

### Level and content
- **Single level**: Only `mazeCreateDemoData()` is used; no multi-level or “change level” flow. `LoadLevel()` and the loading state need to be implemented and hooked to script EVENT_CHANGE_LEVEL (or equivalent) if you want multiple levels.
- **Message content**: Script SHOWMESSAGE needs a source of text (string table per level, or global message file keyed by ID). Maze has `_strings`; you may need a way to index by message ID and pass that text to the message system.

### Audio
- **Music**: ProTracker mod is loaded and played in game.
- **Sound effects**: EVENT_SOUND in scripts is not implemented; no SFX API used by the game yet.

---

## Questions for you (gameplay)

1. **Start flow**: Do you want the game to always start at the **Title** screen (New Game / Load Game / Quit), or keep starting directly in **Game** for now?
2. **Party size**: Will the player have a fixed party (e.g. one character) or multiple? That determines how much of the character/party UI and combat logic you need (e.g. party order, target selection).
3. **Combat**: Monsters aren’t implemented yet. When you add them: dedicated combat mode (separate screen, turn-based) or “monster on same tile = auto attack” with minimal feedback?
4. **Encounters**: Should EVENT_ENCOUNTER start a specific combat (predefined group of monsters) or something else (e.g. minigame, dialogue)?
5. **Levels**: How do you want level progression? (e.g. one maze per level, script-triggered “next level,” or a world map.) That will drive `LoadLevel()` and the loading state.
6. **Win condition**: How should the player reach the Win screen? (e.g. script event “win”, exit tile, boss death, or collect N items.)
7. **Save/Load**: Should save files live on disk (path?), and what should be persisted? (e.g. party, inventory, current level, global flags.)
8. **Items**: Do you already have (or plan) an item data format (e.g. JSON, binary, or ACE resource)? That will determine how to implement `loadItems()`.

If you answer these, the TODOs can be refined (e.g. “implement EVENT_ENCOUNTER as start combat screen with encounter table”) and broken into smaller implementation tasks.

---

## Summary checklist

| Area              | Status |
|-------------------|--------|
| Door locked msg   | Done |
| Script: message   | Done (maze string table + gameDisplayMessage) |
| Script: give/take item | Done (inventoryAddItem/RemoveItem) |
| Script: XP/Damage | Done |
| Script: Sound     | Placeholder (log only) |
| Script: Encounter | TODO, needs encounter/combat design |
| Script: HAS_CLASS / HAS_RACE | Done |
| Items load        | Done; loadItems() from InitNewGame, fallback if no file |
| Save/Load game    | Done |
| Load level        | Done (level 0 = demo, 1+ = file) |
| Loading state     | Minimal stub |
| Title “Load Game” | Done |
| Logo/Intro/Title  | Start at Title |
| Equipment/Inv/Map handlers | Minimal (scroll, show names) |
| Party has 0 characters | No characters added |
| Monsters / combat | Not implemented (existing code unused) |
| Game Over → Win   | Fixed: goes to Title |
| Win condition     | EVENT_WIN in script triggers win state |
