#include "Floor.h"
#include <cmath>
#include <glm\glm.hpp>
using namespace glm;

Floor::Floor()
{
}

Floor::~Floor()
{
}

void Floor::firstInit() {
	colOffset = 1;
	rowOffset = 4;
	tileSize = vec2(54, 2);
	rows = 22;
	cols = (uint)tileSize.x/(uint)tileSize.y;
	floorRows.resize(rows);
}

inline int between(int min, int max) {
	float num = ((float)rand() / RAND_MAX)*(max - min) + min;
	return (int)round(num);
}

inline void updateSafeZoneMap(uint size, uint cols, vector<MeshConfig>& furniture, vector<vector<CellProperties>>& map, ivec2 restriction) {
	CellProperties aux;
	aux.height = 0;
	map.resize(size, vector<CellProperties>(cols, aux));
	vector<ivec2> indices;
	indices.reserve(size*cols);

	uint objects = between(size, size * 2);

	for (uint i = 0; i < objects; ++i) {

		MeshConfig& meshConfig = furniture[rand() % furniture.size()];

		for (int i = 0; i < (int)size - (int)meshConfig.rows + 1; ++i) {
			bool rowConflict = i <= restriction.x && i + (int)meshConfig.rows >= restriction.x;
			for (int j = 0; j < (int)cols - (int)meshConfig.cols + 1; ++j) {
				bool colConflict = j <= restriction.y && j + (int)meshConfig.cols >= restriction.y;
				bool conflict = rowConflict && colConflict;
				for (uint i1 = 0; i1 < meshConfig.rows && !conflict; ++i1) {
					for (uint j1 = 0; j1 < meshConfig.cols && !conflict; ++j1) {
						conflict = map[i + i1][j + j1].height != 0;
					}
				}
				if (!conflict)
					indices.push_back(ivec2(i, j));
			}
		}
		if (indices.size() == 0)
			continue;

		uint index = rand() % indices.size();
		ivec2 pos = indices[index];

		for (uint i = 0; i < meshConfig.rows; ++i) {
			for (uint j = 0; j < meshConfig.cols; ++j) {
				CellProperties cell;
				cell.mesh = INVALID;
				cell.height = meshConfig.height;
				map[pos.x + i][pos.y + j] = cell;
			}
		}
		map[pos.x][pos.y].mesh = meshConfig.mesh;
		map[pos.x][pos.y].rows = meshConfig.rows;
		map[pos.x][pos.y].cols = meshConfig.cols;
		indices.clear();
	}
}

const uint plankLength = 3;

void Floor::updateMap(bool lastRow, uint size) {
	CellProperties aux;
	aux.height = 0;

	uint prevSize = map.size();
	map.resize(size, vector<CellProperties>(cols, aux));
	for (uint i = 0; i < std::min(prevSize, size); ++i) {
		for (uint j = 0; j < cols; ++j) {
			map[i][j] = aux;
		}
	}

	uint start;

	if (lastRow) {
		aux.height = 1.5f;
		aux.mesh = railMesh;
		aux.cols = 1;
		aux.rows = 1;
		for (uint j = 0; j < cols; ++j) {
			map[plankLength - 1][j] = aux;
		}

		uint plankPos = between(cols / 2 - 3, cols / 2 + 3);
		aux.height = 0.1f;
		aux.mesh = INVALID;
		aux.rows = plankLength;
		aux.cols = 1;
		for (uint i = 0; i < plankLength; ++i) {
			map[i][plankPos] = aux;
		}
		map[0][plankPos].mesh = plankMesh;
		start = plankLength - 1;
	}
	else
		start = 0;

	vector<ivec2> indices;
	indices.reserve(size*cols);

	uint objects = between(size, size*2);

	for (uint i = 0; i < objects; ++i) {

		MeshConfig& meshConfig = furniture[rand() % furniture.size()];

		for (int i = 0; i < (int)size-(int)meshConfig.rows+1; ++i) {
			for (int j = 0; j < (int)cols-(int)meshConfig.cols+1; ++j) {
				bool conflict = false;
				for (uint i1 = 0; i1 < meshConfig.rows && !conflict; ++i1) {
					for (uint j1 = 0; j1 < meshConfig.cols && !conflict; ++j1) {
						conflict = map[i + i1][j + j1].height != 0;
					}
				}
				if (!conflict)
					indices.push_back(ivec2(i, j));
			}
		}
		if (indices.size() == 0)
			continue;

		uint index = rand() % indices.size();
		ivec2 pos = indices[index];

		for (uint i = 0; i < meshConfig.rows; ++i) {
			for (uint j = 0; j < meshConfig.cols; ++j) {
				CellProperties cell;
				cell.mesh = INVALID;
				cell.height = meshConfig.height;
				map[pos.x+i][pos.y+j] = cell;
			}
		}
		map[pos.x][pos.y].mesh = meshConfig.mesh;
		map[pos.x][pos.y].rows = meshConfig.rows;
		map[pos.x][pos.y].cols = meshConfig.cols;
		indices.clear();
	}
}

void Floor::updateFloorRow(FloorRow& floorRow) {
	if (biomeCounter == biomeLength) {
		switch (biome) {
		case Ship:
			biome = Sea;
			break;
		case Sea:
			type = Road;
			counter = length = 0;
			biome = Ship;
			break;
		}
		biomeLength = between(15, 20);
		biomeCounter = 0;
	}
	switch (biome) {
	case Sea:
		if (map.size() > 0) {
			floorRow.initRoad(Sea, NULL, &map[map.size() - 1]);
			map.pop_back();
		}
		else {
			floorRow.initRoad(Sea);
		}
		break;
	case Ship:
		bool transition = false;
		if (counter == length) {
			switch (type) {
			case Safe:
				length = between(3, 10);
				if (length >= biomeLength - biomeCounter) {
					length = biomeLength - biomeCounter-1;
				}
				type = Road;
				transition = true;
				break;
			case Road:
				length = between(1, 4);
				bool lastRow;
				if (length >= biomeLength - biomeCounter) {
					length = biomeLength - biomeCounter + (plankLength - 1);
					lastRow = true;
				}
				else if (length + 1 == biomeLength - biomeCounter) {
					lastRow = false;
					++length;
				}
				else
					lastRow = false;
				updateMap(lastRow, length);
				type = Safe;
				break;
			}
			counter = 0;
		}
		switch (type) {
		case Safe:
			floorRow.initSafeZone(map[map.size() - 1]);
			map.pop_back();
			break;
		case Road:
			if (transition) {
				for (uint i = 0; i < cols; ++i)
					textureIndex[i] = 999;
			}
			floorRow.initRoad(Ship, &textureIndex);
			break;
		}
		++counter;
		break;
	}
	++biomeCounter;
}

void Floor::init(vec3 lightDir, const Assets& assets) {
	GameObject::init();

	furniture.resize(3);

	MeshConfig configAux;
	configAux.rows = 1;
	configAux.cols = 2;
	configAux.height = 2;
	configAux.mesh = assets.getMeshId("cannon");
	furniture[0] = configAux;

	configAux.mesh = assets.getMeshId("cannon");
	furniture[1] = configAux;

	configAux.rows = 3;
	configAux.cols = 3;
	configAux.height = 0.5f;
	configAux.mesh = assets.getMeshId("cubierta");
	furniture[2] = configAux;

	plankMesh = assets.getMeshId("plank");
	railMesh = assets.getMeshId("railing_parrot");

	FloorRow::initIds(assets);
	FloorRow::setParameters(tileSize, cols, lightDir, colOffset);

	float realTileSize = tileSize.x / cols;

	float offsetZ = -tileSize.y*(rows/2);
	textureIndex.resize(cols, 999);

	biome = Ship;
	biomeLength = between(15, 20);
	biomeCounter = 0;

	type = Safe;
	length = rows/2 - rowOffset + between(2, 5);
	counter = 0;

	ivec2 playerIni;
	playerIni.x = length - 1 - (rows / 2 - rowOffset);
	playerIni.y = cols / 2 - colOffset;

	updateSafeZoneMap(length, cols, furniture, map, playerIni);
	for (uint i = 0; i < rows; ++i) {
		floorRows[i].setPos(vec2(colOffset*realTileSize, rowOffset*tileSize.y + offsetZ + i*tileSize.y));
		updateFloorRow(floorRows[i]);
	}
	lastRow = 0;
	firstPos = floorRows[floorRows.size()-1].getPos().y;
}

void Floor::addLevel() {
	firstPos += tileSize.y;
	vec2 lastPos = floorRows[lastRow].getPos();
	floorRows[lastRow].setPos(vec2(lastPos.x, firstPos));
	updateFloorRow(floorRows[lastRow]);

	lastRow = (lastRow + 1) % rows;
}

void Floor::update(int deltaTime) {
	for (FloorRow& row : floorRows) {
		row.update(deltaTime);
	}
}

void Floor::groupDrawableObjects(std::vector<vector<Object*>>& objects, vector<vector<TexturedObject*>>& texturedObjects, const FrustumG& frustum) {
	for (uint i = 0; i < rows; ++i) {
		floorRows[i].groupDrawableObjects(objects, texturedObjects, frustum);
	}
}

vec2 Floor::getTileSize() const {
	return tileSize;
}

uint Floor::getColOffset() const {
	return colOffset;
}

glm::uint Floor::getRowOffset() const
{
	return rowOffset;
}

uint Floor::getRows() const {
	return rows;
}
glm::uint Floor::getCols() const {
	return cols;
}
FloorRow* Floor::getFloorRow(uint index) {
	return &floorRows[index];
}