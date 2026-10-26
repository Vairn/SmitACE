#pragma once

#include "formats.hpp"
#include <functional>

/** Load the maze and entities named by this level. An empty maze path loads the game's built-in demo. */
bool loadLevelFromManifest(const GameManifest &man, int level, MazeFile &maze, LvlEntities &lvl, std::string &err);

/**
 * Write the maze to the path in game.smt.
 * If that path is empty, fill in data/levelNN.maze and save game.smt so the game loads the file.
 */
bool saveMazeToManifest(GameManifest &man, int level, const MazeFile &maze, const std::string &root, std::string &err);

/** Write entities the same way. An empty path becomes data/levelNN.lvl and is stored in game.smt. */
bool saveEntitiesToManifest(GameManifest &man, int level, const LvlEntities &lvl, const std::string &root, std::string &err);

/** One decoded wallset plane. index matches a tile's setIndex. */
struct DungeonSheet {
	unsigned tex = 0;
	int w = 0;
	int h = 0;
	int index = 0;
};

void drawMazeEditor(MazeFile &maze, LvlEntities &lvl,
	const std::vector<MonsterRow> &monsters,
	const std::vector<ItemRow> &items,
	const WallsetFile &wallset,
	const std::vector<DungeonSheet> &sheets,
	GameManifest &man, int levelIndex,
	const std::string &root,
	const std::function<void()> &reloadLevel,
	char *err, int errBytes);
