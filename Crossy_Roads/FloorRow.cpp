#include "FloorRow.h"
#include "Scene.h"
#include "Utils.h"
#include "ShipBiome.h"
#include "SeaBiome.h"
using namespace glm;

uint FloorRow::worldToCol(float x) {
	return uint((x - offset) / fp.realTileSize);
}

void FloorRow::initResources(const Assets & assets, const Player* player) {
	res.init(assets, player);
}

void FloorRow::putWall() {
	wall.init(*res.assets, fp, vec3(pos.x, rowHeight, pos.y), 5, 3, 3);
}

void FloorRow::firstInit() {
	furniture.reserve(fp.cols);
}

void FloorRow::initAttributes(BiomeType biome, bool safeZone, float rowHeight) {
	this->biome = biome;
	this->safeZone = safeZone;
	this->rowHeight = rowHeight;
}

void FloorRow::initRoad(BiomeType type, vector<uint>& adjacentRow, const vector<CellProperties>& map, const FloorRow& prevRow) {
	this->map = map;
	biome = type;
	safeZone = false;
	if (prevRow.safeZone) rowHeight = prevRow.rowHeight - 0.2f;
	else rowHeight = prevRow.rowHeight;

	for (uint i = 0; i < furniture.size(); ++i) {
		delete furniture[i];
	}
	furniture.clear();
	wall.clear();
	switch (type) {
	case Ship:
		initShipRoad(adjacentRow);
		break;
	case Sea:
		initSeaRoad(prevRow);
		break;
	case Island:
		initIslandRoad();
		break;
	}
}

void FloorRow::initSafeZone(BiomeType type, const vector<CellProperties>& map, const FloorRow& prevRow) {
	this->map = map;
	biome = type;
	safeZone = true;
	for (uint i = 0; i < furniture.size(); ++i) {
		delete furniture[i];
	}
	furniture.clear();
	wall.clear();
	switch (biome) {
	case Ship:
		initShipSafeZone(prevRow);
		break;
	case Island:
		initIslandSafeZone(prevRow);
		break;
	}
}

void FloorRow::setParameters(const FloorParams & floorParams) {
	fp = floorParams;
	offset = -fp.tileSize.x / 2 + fp.colOffset*fp.realTileSize;
}

pair<vec3, float> FloorRow::getNextPos(uint col) {
	float offsetX = pos.x - (fp.realTileSize*(fp.cols / 2) - (1 - fp.cols % 2)*fp.realTileSize / 2);
	vec3 myHeight = vec3(offsetX + col*fp.realTileSize, rowHeight, pos.y);
	float platformSpeed = 0;
	for (uint i = 0; i < platforms.size(); ++i) {
		uint index = worldToCol(platforms[i].getPos().x);
		if (index == col || index == col + 1 || index == col - 1) {
			myHeight.y = platforms[i].getPos().y + platforms[i].getHeight();
			myHeight.x = platforms[i].getPos().x + speeds[i] * 11;
			if (index == col + 1) myHeight.x -= fp.realTileSize;
			else if (index == col - 1) myHeight.x += fp.realTileSize;
			platformSpeed = speeds[i];
			break;
		}
	}
	if (map.size() > 0 && map[col].canJump && map[col].height > 0) {
		if (rowHeight + map[col].verticalOffset + map[col].height > myHeight.y) {
			myHeight.y = rowHeight + map[col].verticalOffset + map[col].height;
			myHeight.x = offsetX + col*fp.realTileSize;
			myHeight.z = pos.y;
			platformSpeed = 0;
		}
	}
	return make_pair(myHeight, platformSpeed);
}

float FloorRow::getRowHeight() const {
	return rowHeight;
}

void FloorRow::update(int deltaTime) {
	uint j = 0;
	for (uint i = 0; i < furniture.size(); ++i) {
		furniture[i]->update(deltaTime);
		if (furniture[i]->isAlive()) {
			furniture[j] = furniture[i];
			++j;
		}
		else {
			delete furniture[i];
		}
	}
	furniture.resize(j);
	for (AnimTexObject& animatedTexure : animatedFloorTiles) {
		animatedTexure.update(deltaTime);
	}
	bool someZero = false;
	for (uint i = 0; i < enemies.size(); ++i) {
		Jumper& object = enemies[i];
		object.update();
		if (object.horizontalSpeed == 0 && enemies[i - 1].horizontalSpeed != 0) {
			someZero = true;
			if (frameCounter == frameLimit) {
				object.horizontalSpeed = enemies[0].horizontalSpeed;
				uint minTime = uint(fp.realTileSize / abs(enemies[0].horizontalSpeed)) + 1;
				uint maxTime = uint((fp.tileSize.x - fp.realTileSize) / abs(enemies[0].horizontalSpeed));
				frameLimit = between(minTime + 10, int(maxTime*0.4f));
				frameCounter = 0;
			}
			else
				++frameCounter;
		}
		float x = object.getPos().x;
		if (x > fp.upperLimit || x < fp.lowerLimit) {
			float startPoint;
			if (object.horizontalSpeed >= 0) {
				object.setRotationY(PI/2);
				startPoint = fp.lowerLimit;
			}
			else {
				object.setRotationY(-PI/2);
				startPoint = fp.upperLimit;
			}
			object.setPos(vec3(startPoint, rowHeight, pos.y));
		}
	}
	if (!someZero && rand() % 512 == 0) {
		for (uint i = 0; i < enemies.size(); ++i) {
			enemies[i].jump();
		}
	}
	
	for (uint i = 0; i < platforms.size(); ++i) {
		AnimMeshObject& object = platforms[i];
		object.update(deltaTime);
		object.move(speeds[i], 0, 0);
		if (speeds[i] == 0 && speeds[i-1] != 0) {
			if (frameCounter == frameLimit) {
				speeds[i] = speeds[0]; 
				uint minTime = uint(3 * fp.realTileSize / abs(speeds[0])) + 1;
				uint maxTime = uint((fp.tileSize.x - 3 * fp.realTileSize) / abs(speeds[0]));
				frameLimit = between(minTime + 10, int(maxTime*0.3f));
				frameCounter = 0;
			}
			else
				++frameCounter;
		}
		float x = object.getPos().x;
		if (x > fp.upperLimit || x < fp.lowerLimit) {
			float startPoint;
			if (speeds[0] >= 0) {
				object.setRotationY(0);
				startPoint = fp.lowerLimit;
			}
			else {
				object.setRotationY(PI);
				startPoint = fp.upperLimit;
			}
			vec3 prevPos = object.getPos();
			object.setPos(vec3(startPoint, prevPos.y, pos.y));
		}
	}
}

void FloorRow::groupDrawableObjects(const FrustumG& frustum, RenderVectors& renderVectors) {
	wall.groupDrawableObjects(frustum, renderVectors);
	for (uint i = 0; i < enemies.size(); ++i) {
		if (enemies[i].isInsideViewFrustum(frustum))
			renderVectors.objects[enemies[i].meshId].push_back(&enemies[i]);
		renderVectors.shadowObjects[enemies[i].meshId].push_back(&enemies[i]);
	}
	for (uint i = 0; i < floorTiles.size(); ++i) {
		if (floorTiles[i].isInsideViewFrustum(frustum)) {
			renderVectors.texturedObjects[floorTiles[i].texture].push_back(&floorTiles[i]);
		}
	}
	for (uint i = 0; i < animatedFloorTiles.size(); ++i) {
		if (animatedFloorTiles[i].isInsideViewFrustum(frustum)) {
			renderVectors.texturedObjects[animatedFloorTiles[i].getTexture()].push_back(&animatedFloorTiles[i]);
		}
	}
	for (uint i = 0; i < furniture.size(); ++i) {
		if (furniture[i]->isInsideViewFrustum(frustum)) {
			renderVectors.objects[furniture[i]->meshId].push_back(furniture[i]);
		}
		renderVectors.shadowObjects[furniture[i]->meshId].push_back(furniture[i]);
	}
	for (uint i = 0; i < platforms.size(); ++i) {
		if (platforms[i].isInsideViewFrustum(frustum)) {
			renderVectors.objects[platforms[i].getMesh()].push_back(&platforms[i]);
		}
		renderVectors.shadowObjects[platforms[i].getMesh()].push_back(&platforms[i]);
	}
}

vector<Jumper>* FloorRow::getEnemies() {
	return &enemies;
}

vector<CellProperties>* FloorRow::getRowObjects() {
	return &map;
}

vector<AnimMeshObject>* FloorRow::getPlatforms()
{
	return &platforms;
}

bool FloorRow::isSafeZone() const {
	return safeZone;
}
bool FloorRow::isTheFloorLava() const {
	return theFloorIsLava;
}
BiomeType FloorRow::getBiome() const {
	return biome;
}

FloorRow::FloorRow() {
}


FloorRow::~FloorRow()
{
}

float FloorRow::offset;
FloorParams FloorRow::fp;
FloorResources FloorRow::res;