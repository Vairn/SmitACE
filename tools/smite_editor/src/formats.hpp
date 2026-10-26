#pragma once

#include <cstdint>
#include <string>
#include <vector>

bool readRawFile(const std::string &path, std::vector<unsigned char> &out, std::string &err);

std::uint16_t readBe16(const unsigned char *p);
void writeBe16(std::vector<unsigned char> &out, std::uint16_t v);

struct GameManifest {
	int version = 1;
	int startLevel = 0;
	int levelCount = 0;
	std::string itemsPath;
	std::string monstersPath;
	std::string uiPalettePath;
	struct Level {
		std::string mazePath;
		std::string wallsetPath;
		std::string entitiesPath;
	};
	std::vector<Level> levels;
};

bool loadGameManifest(const std::string &path, GameManifest &out, std::string &err);
bool saveGameManifest(const std::string &path, const GameManifest &m, std::string &err);

struct ItemRow {
	unsigned char type, subType, flags, modType, mod, value, weight, icon, usage, keyId, equipSlot;
	std::string name;
};

bool loadItemsDat(const std::string &path, std::vector<ItemRow> &out, std::string &err);
bool saveItemsDat(const std::string &path, const std::vector<ItemRow> &items, std::string &err);

struct MonsterRow {
	unsigned char level = 1;
	std::uint16_t maxHp = 20;
	unsigned char attack = 5, defense = 3;
	std::uint16_t experience = 10;
	unsigned char aggro = 5, flee = 20;
	unsigned char dropTable[8]{};
	unsigned char dropChance[8]{};
	std::string name;
};

bool loadMonstersDat(const std::string &path, std::vector<MonsterRow> &out, std::string &err);
bool saveMonstersDat(const std::string &path, const std::vector<MonsterRow> &mons, std::string &err);

struct MazeEvent {
	int x = 0, y = 0;
	int type = 0;
	std::vector<unsigned char> data;
};

struct MazeFile {
	int width = 0, height = 0;
	std::vector<unsigned char> cells, cols, floors;
	std::vector<MazeEvent> events;
	std::vector<std::vector<unsigned char>> strings;
};

bool loadMaze(const std::string &path, MazeFile &out, std::string &err);
bool saveMaze(const std::string &path, const MazeFile &m, std::string &err);

/** The maze the game builds when a level's maze path is empty. */
void loadBuiltinDemoMaze(MazeFile &out);

struct LvlEntities {
	int version = 1;
	struct WB { int x,y,wallSide,type,gfx,evt,esz; std::vector<unsigned char> ed; };
	struct DB { int x,y,wallSide,type,gfx,tx,ty; };
	struct Lk { int doorX,doorY,type,keyId,code; };
	struct Pl { int x,y,evt,dsz; unsigned char d[8]; };
	struct GI { int x,y,item,qty; };
	struct MS { int type,x,y; };
	std::vector<WB> wallBtns;
	std::vector<DB> doorBtns;
	std::vector<Lk> locks;
	std::vector<Pl> plates;
	std::vector<GI> ground;
	std::vector<MS> monsters;
};

bool loadLvl(const std::string &path, LvlEntities &out, std::string &err);
bool saveLvl(const std::string &path, const LvlEntities &e, std::string &err);

struct WallsetFile {
	unsigned char header[3]{};
	std::vector<unsigned char> palette;
	int totalTiles = 0;
	int gfxCount = 0;
	struct Tile {
		unsigned char type = 0, setIndex = 0;
		char loc[2]{};
		std::int16_t screen[2]{};
		std::uint16_t x = 0, y = 0, w = 0, h = 0;
	};
	std::vector<Tile> tiles;
	std::vector<int> tilesPerGroup;
};

bool loadWallsetMain(const std::string &path, WallsetFile &out, std::string &err);

/** Decode ACE bitmap (.pln/.bm): non-interleaved, version 0. Returns RGBA 8bpp. */
bool decodeAcePlanar(const std::vector<unsigned char> &fileData, const unsigned char *palRgb, int palColors,
	std::vector<unsigned char> &rgbaOut, int &outW, int &outH, std::string &err);

struct MonsterFrameImage {
	int w = 0;
	int h = 0;
	std::vector<unsigned char> rgba;
};

/**
 * Slice one PNG into 6 poses: Front, Back, Left1, Left2, Attack1, Attack2.
 * Left1 and Left2 are mirrored into Right1 and Right2.
 * Baked order is Front, Back, Left1, Left2, Right1, Right2, Attack1, Attack2.
 * Each pose is written at four view distances as
 * {outDir}/{typeIndex}_f{frame}_d{depth}.bm and .msk. Mask bit 1 is opaque.
 * outFrames may be null. On success it receives the near-distance preview of each pose.
 */
bool exportMonsterFrames(const std::string &pngPath, const std::string &outDir, int typeIndex,
	const WallsetFile &wallset, std::string &err, std::vector<MonsterFrameImage> *outFrames);

/** Load one written frame pair for the editor preview. Missing files return false. */
bool loadMonsterFramePreview(const std::string &bmPath, const std::string &mskPath,
	const unsigned char *palRgb, int palColors, MonsterFrameImage &out, std::string &err);
