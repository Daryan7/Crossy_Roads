#include "FloorRow.h"
#include "Scene.h"
#include "Pi.h"
using namespace glm;

const vector<vector<string>> models = { 
	{ "wood_3_1", "wood_3_2", "wood_3_3", "wood_3_4", "wood_3_5" },
	{ "wood_4_0", "wood_4_1", "wood_4_2", "wood_4_3", "wood_4_4" },
	{ "wood_5_0", "wood_5_1", "wood_5_2", "wood_5_3", "wood_5_4" },
	{ "wood_6_0", "wood_6_1", "wood_6_2", "wood_6_3", "wood_6_4" },
	{ "wood_7_0", "wood_7_1", "wood_7_2", "wood_7_3", "wood_7_4" } };

const vector<string> enemyMeshesStrings = {
	"pirate", "pirate_2"
};

vector<uint> indices;

void FloorRow::initIds(const Assets& assets) {
	FloorRow::assets = &assets;

	enemyMeshes.resize(enemyMeshesStrings.size());
	for (uint i = 0; i < enemyMeshesStrings.size(); ++i) {
		enemyMeshes[i] = assets.getMeshId(enemyMeshesStrings[i]);
	}

	cubeMesh = assets.getCubeMesh();
	indices.resize(models.size());
	for (uint i = 0; i < indices.size(); ++i) {
		indices[i] = i;
	}

	floorTextures.resize(models.size());
	for (uint i = 0; i < models.size(); ++i) {
		floorTextures[i].resize(models[i].size());
		for (uint j = 0; j < models[i].size(); ++j) {
			floorTextures[i][j] = assets.getTextureId(models[i][j]);
		}
	}
	planeWood = assets.getTextureId("wood_plane");
	water = assets.getTextureId("water_plane");
	crocodile = assets.getMeshId("crocodile-01");
}

void FloorRow::setParameters(vec2 tileSize, uint cols, vec3 lightDir) {
	FloorRow::tileSize = tileSize;
	FloorRow::cols = cols;
	FloorRow::lightDir = lightDir;
}

vec2 FloorRow::getPos() const {
	return pos;
}

inline float generateSpeed() {
	const static float maxSpeed = 0.3f;
	const static float minSpeed = 0.05f;
	return (((float)rand() / RAND_MAX)*(maxSpeed - minSpeed) + minSpeed)*(-1 + (rand()%2)*2);
}

void FloorRow::setPos(vec2 position) {
	pos = position;
}

void FloorRow::initRoad(Biome biome, vector<uint>* adjacentRow) {
	this->biome = biome;
	safeZone = false;
	switch (biome) {
		case Ship:
			initShipRoad(*adjacentRow);
			break;
		case Sea:
			initSea();
			break;
	}
}

bool applyConstraints(uint meshIndex, uint numAdjacents, vector<uint>& adjacentRow, uint position) {
	if (position + numAdjacents < adjacentRow.size() && adjacentRow[position + numAdjacents - 1] != adjacentRow[position + numAdjacents])
		return false;
	if (meshIndex == adjacentRow[position])
		return false;
	return true;
}

inline int between(int min, int max) {
	float num = ((float)rand() / RAND_MAX)*(max - min) + min;
	return (int)round(num);
}

pair<uint,uint> generateRandomTextureIndex(uint i, uint prevMeshIndex, vector<uint>& adjacentRow) {
	uint textureIndex;
	uint numAdjacentTiles;
	vector<uint> indicesCopy = indices;
	while (indicesCopy.size() > 0) {
		uint ind = rand() % indicesCopy.size();
		textureIndex = indicesCopy[ind];
		indicesCopy.erase(indicesCopy.begin() + ind);
		if (prevMeshIndex == textureIndex)
			continue;
		for (numAdjacentTiles = models[textureIndex].size(); numAdjacentTiles >= 4; --numAdjacentTiles) {
			if (applyConstraints(textureIndex, numAdjacentTiles, adjacentRow, i)) {
				indicesCopy.clear();
				break;
			}
		}
	}
	return make_pair(textureIndex, numAdjacentTiles);
}

void FloorRow::initSafeZone(vector<CellProperties>& map) {
	biome = Ship;
	safeZone = true;
	enemies.clear();
	speeds.clear();
	rowObjects = map;
	floorTiles.resize(1);
	rowHeight = 0.2f;

	static float realTileSize = tileSize.x / cols;
	float offsetX = pos.x - (realTileSize*(cols / 2) - (1 - cols % 2)*realTileSize / 2);

	static vec3 boundingBox = cubeMesh->getbbSize();
	static vec3 bbcenter = cubeMesh->getbbCenter();
	static float height = cubeMesh->getHeight();
	static vec3 floorTileSize = vec3(tileSize.x, 0.2f, tileSize.y) / boundingBox;

	TexturedObject& tile = floorTiles[0];
	tile.texture = planeWood;
	tile.setMesh(cubeMesh);
	tile.setScale(floorTileSize);
	tile.setCenter(vec3(bbcenter.x, bbcenter.y + height / 2.f, bbcenter.z));
	tile.setPos(vec3(pos.x, rowHeight, pos.y));

	furniture.clear();
	for (uint i = 0; i < cols; ++i) {
		if (map[i].height == 0 || map[i].mesh == INVALID)
			continue;
		furniture.push_back(ShadowedObject());
		ShadowedObject& object = furniture[furniture.size()-1];
		IdMesh meshId = map[i].mesh;
		float height = map[i].height;

		const Mesh* mesh = assets->getMesh(meshId);

		vec3 boundingBox = mesh->getbbSize();
		vec3 bbCenter = mesh->getbbCenter();
		vec3 objectSize = vec3(realTileSize*map[i].cols, height, tileSize.y*map[i].rows) / boundingBox;

		object.setMesh(meshId, mesh);
		object.setScale(objectSize);
		object.setCenterToBaseCenter();
		float posX = offsetX + i*realTileSize;
		object.setPos(vec3(posX + (realTileSize*(map[i].cols/2.f)) - realTileSize/2, rowHeight, pos.y - tileSize.y*(map[i].cols / 2.f) + tileSize.y/2));
		object.setPlane(vec4(0, 1, 0, -rowHeight), lightDir);
	}
}

float FloorRow::getHeight() const {
	return rowHeight;
}

void FloorRow::initShipRoad(vector<uint>& adjacentRow) {
	rowObjects.clear();
	furniture.clear();
	enemies.resize(2);
	speeds.resize(enemies.size());
	floorTiles.resize(cols);
	rowHeight = 0;

	static float realTileSize = tileSize.x / cols;
	static vec3 boundingBox = cubeMesh->getbbSize();
	static vec3 bbcenter = cubeMesh->getbbCenter();
	static float height = cubeMesh->getHeight();
	vec3 floorTileSize = vec3(realTileSize, 0.1f, tileSize.y) / boundingBox;
	float offsetX = pos.x - (realTileSize*(cols / 2) - (1 - cols % 2)*realTileSize / 2);

	for (uint i = 0; i < enemies.size(); ++i) {
		ShadowedObject& enemy = enemies[i];
		enemy.setMesh(enemyMeshes[i], assets->getMesh(enemyMeshes[i]));
		enemy.setRotationY(PI);
		enemy.setScale(vec3(0.1f));
		enemy.setCenterToBaseCenter();
		enemy.setPlane(vec4(0, 1, 0, -rowHeight), lightDir);
		speeds[i] = generateSpeed();
		float startPoint;
		if (speeds[i] >= 0)
			startPoint = -tileSize.x / 2 + ((float)rand() / RAND_MAX) * tileSize.x;
		else
			startPoint = tileSize.x / 2 - ((float)rand() / RAND_MAX) * tileSize.x;
		enemy.setPos(vec3(startPoint, rowHeight, pos.y));
	}

	uint textureIndex = models.size();
	IdTex texture = INVALID;
	uint numAdjacentTiles = 0;
	uint counter = numAdjacentTiles;
	vector<uint> indicesAux;
	for (uint i = 0; i < floorTiles.size(); ++i) {
		TexturedObject& tile = floorTiles[i];
		if (counter == numAdjacentTiles) {
			pair<uint, uint> ret = generateRandomTextureIndex(i, textureIndex, adjacentRow);
			textureIndex = ret.first;
			numAdjacentTiles = ret.second;
			counter = 0;
		}
		texture = floorTextures[textureIndex][counter];
		adjacentRow[i] = textureIndex;
		tile.texture = texture;
		tile.setMesh(cubeMesh);
		tile.setScale(floorTileSize);
		tile.setRotationY(PI / 2);
		tile.setCenter(vec3(bbcenter.x, bbcenter.y + height / 2.f, bbcenter.z));
		tile.setPos(vec3(offsetX + i*realTileSize, rowHeight, pos.y));
		++counter;
	}
}

void FloorRow::initSea() {
	furniture.clear();
	enemies.resize(1);
	speeds.resize(enemies.size());
	rowObjects.clear();
	floorTiles.resize(1);
	rowHeight = -5;

	static float realTileSize = tileSize.x / cols;
	float offsetX = pos.x - (realTileSize*(cols / 2) - (1 - cols % 2)*realTileSize / 2);

	static vec3 boundingBox = cubeMesh->getbbSize();
	static vec3 bbcenter = cubeMesh->getbbCenter();
	static float height = cubeMesh->getHeight();
	static vec3 floorTileSize = vec3(tileSize.x, 0.2f, tileSize.y) / boundingBox;

	TexturedObject& tile = floorTiles[0];
	tile.texture = water;
	tile.setMesh(cubeMesh);
	tile.setScale(floorTileSize);
	tile.setCenter(vec3(bbcenter.x, bbcenter.y + height / 2.f, bbcenter.z));
	tile.setPos(vec3(pos.x, rowHeight, pos.y));

	for (uint i = 0; i < enemies.size(); ++i) {
		ShadowedObject& enemy = enemies[i];
		enemy.setMesh(crocodile, assets->getMesh(crocodile));
		enemy.setScale(vec3(0.1f));
		enemy.setCenterToBaseCenter();
		enemy.setPlane(vec4(0, 1, 0, -rowHeight), lightDir);
		speeds[i] = generateSpeed();
		float startPoint;
		if (speeds[i] >= 0) {
			enemy.setRotationY(0);
			startPoint = -tileSize.x / 2 + ((float)rand() / RAND_MAX) * tileSize.x;
		}
		else {
			enemy.setRotationY(PI);
			startPoint = tileSize.x / 2 - ((float)rand() / RAND_MAX) * tileSize.x;
		}
		enemy.setPos(vec3(startPoint, rowHeight, pos.y));
	}
}

void FloorRow::update(int deltaTime) {
	for (uint i = 0; i < enemies.size(); ++i) {
		Object& object = enemies[i];
		object.move(speeds[i], 0, 0);
		float x = object.getPos().x;
		if (x > tileSize.x / 2 || x < -tileSize.x/2) {
			speeds[i] = generateSpeed();
			float startPoint;
			if (speeds[i] >= 0) {
				if (biome == Sea)
					object.setRotationY(0);
				startPoint = -tileSize.x / 2;
			}
			else {
				if (biome == Sea)
					object.setRotationY(PI);
				startPoint = tileSize.x / 2;
			}
			object.setPos(vec3(startPoint, rowHeight, pos.y));
		}
	}
}

void FloorRow::groupDrawableObjects(vector<vector<Object*>>& objects, vector<vector<TexturedObject*>>& texturedObjects, const FrustumG& frustum) {
	for (uint i = 0; i < enemies.size(); ++i) {
		if (enemies[i].isInsideViewFrustrum(frustum))
			objects[enemies[i].meshId].push_back(&enemies[i]);
	}
	for (uint i = 0; i < floorTiles.size(); ++i) {
		if (floorTiles[i].isInsideViewFrustrum(frustum))
			texturedObjects[floorTiles[i].texture].push_back(&floorTiles[i]);
	}
	for (uint i = 0; i < furniture.size(); ++i) {
		if (furniture[i].isInsideViewFrustrum(frustum))
			objects[furniture[i].meshId].push_back(&furniture[i]);
	}
}

vector<ShadowedObject>* FloorRow::getEnemies() {
	return &enemies;
}

vector<CellProperties>* FloorRow::getRowObjects() {
	return &rowObjects;
}

bool FloorRow::isSafeZone() const {
	return safeZone;
}
Biome FloorRow::getBiome() const {
	return biome;
}
FloorRow::FloorRow()
{
}


FloorRow::~FloorRow()
{
}

vec2 FloorRow::tileSize;
uint FloorRow::cols;
vec3 FloorRow::lightDir;
IdMesh FloorRow::crocodile;
const Mesh* FloorRow::cubeMesh;
vector<vector<IdTex>> FloorRow::floorTextures;
vector<IdMesh> FloorRow::enemyMeshes;
IdTex FloorRow::planeWood;
IdTex FloorRow::water;
const Assets* FloorRow::assets;