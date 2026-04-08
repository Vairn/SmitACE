#pragma once

#include "atlas_pack.hpp"

#include <string>
#include <vector>

struct DungeonGlSettings {
	int viewportW = 320, viewportH = 256;
	int dungeonDepth = 5, dungeonWidth = 4;
	float cameraOffsetX = 0.5f, cameraOffsetY = 0.005f, cameraOffsetZ = 0.026f;
	float cameraTiltRad = -0.15f;
	float cameraFOV = 50.f;
	float ceilingOffsetY = -0.225f;
	float tileWidth = 0.f;
	float cameraShiftZ = -0.115f;
	int tilePadding = 2;
	bool useFog = false;
	float fogStart = 0.f, fogEnd = 3.5f;
	float fogColor[3] = {0, 0, 0};
	bool textureFiltering = false;
};

enum class DungeonLayerType : int { Walls = 0, Floor = 1, Ceiling = 2, Decal = 3, Object = 4 };

struct DungeonGlLayer {
	bool on = true;
	int stableIndex = 0;
	int id = 1;
	std::string name;
	DungeonLayerType type = DungeonLayerType::Walls;
	std::string textureName;
	float scaleX = 1.f, scaleY = 1.f;
	float offsetY = 0.f;

	struct TileOut {
		std::string type;
		bool flipped = false;
		int tileX = 0, tileZ = 0;
		int screenX = 0, screenY = 0;
		int coordX = 0, coordY = 0, coordW = 0, coordH = 0;
		int fullWidth = 0;
	};
	std::vector<TileOut> tilesOut;
	void clearTiles() { tilesOut.clear(); }
};

struct DungeonGlTexture {
	std::string name;
	unsigned texId = 0;
	int w = 0, h = 0;
};

bool dungeonGlGenerate(const DungeonGlSettings &st,
	std::vector<DungeonGlLayer> &layers,
	const std::vector<DungeonGlTexture> &texLib,
	AtlasPacker &packer,
	int gutterPad,
	std::string &jsonMinified,
	std::string &err);

void dungeonGlClearDebugLog();
const std::string &dungeonGlGetDebugLog();

void dungeonGlShutdown();
