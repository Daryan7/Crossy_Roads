#include "FloorRow.h"
#include "Scene.h"
#include "Pi.h"
using namespace glm;

//const string models[] = { "models/floor_2.obj","models/floor_3.obj","models/floor_4.obj",
//	"models/floor_5.obj"};

//const string models[] = { "images/wood_1.png", "images/wood_2.png", "images/road.png",
//	"images/white.png" };

const vector<vector<string>> models = { 
	//{ "images/wood_1.png" }, 
	//{ "images/wood_2.png" },
	//{ "images/road.png" }, 
	{ "images/wood_3_1.png", "images/wood_3_2.png", "images/wood_3_3.png", "images/wood_3_4.png", "images/wood_3_5.png" },
	{ "images/wood_4_0.png", "images/wood_4_1.png", "images/wood_4_2.png", "images/wood_4_3.png", "images/wood_4_4.png" },
	{ "images/wood_5_0.png", "images/wood_5_1.png", "images/wood_5_2.png", "images/wood_5_3.png", "images/wood_5_4.png" },
	{ "images/wood_6_0.png", "images/wood_6_1.png", "images/wood_6_2.png", "images/wood_6_3.png", "images/wood_6_4.png" },
	{ "images/wood_7_0.png", "images/wood_7_1.png", "images/wood_7_2.png", "images/wood_7_3.png", "images/wood_7_4.png" } };

const vector<string> enemyMeshesStrings = {
	"models/pirate.obj", "models/pirate_2.obj"
};
//const uint nModels = sizeof(models) / sizeof(string);

vector<uint> indices;

void FloorRow::initMeshes() {
	enemyMeshes.resize(enemyMeshesStrings.size());
	for (int i = 0; i < enemyMeshesStrings.size(); ++i) {
		enemyMeshes[i].loadFromFile(enemyMeshesStrings[i]);
	}
	//pirateMesh.loadFromFile("models/pirate.obj");
	//pirateMesh2.loadFromFile("models/pirate2.obj");
	cubeMesh.init();
	indices.resize(models.size());
	for (uint i = 0; i < indices.size(); ++i) {
		indices[i] = i;
	}

	floorTextures.resize(models.size());
	for (uint i = 0; i < models.size(); ++i) {
		floorTextures[i].resize(models[i].size());
		for (uint j = 0; j < models[i].size(); ++j) {
			floorTextures[i][j].loadFromFile(models[i][j], TEXTURE_PIXEL_FORMAT_RGBA);
			floorTextures[i][j].setMagFilter(GL_NEAREST);
			floorTextures[i][j].setMinFilter(GL_NEAREST);
		}
	}
	planeWood.loadFromFile("images/wood_plane.png", TEXTURE_PIXEL_FORMAT_RGB);
	planeWood.setMagFilter(GL_NEAREST);
	planeWood.setMinFilter(GL_NEAREST);
}

void FloorRow::setParameters(vec2 tileSize, uint cols, vec3 lightDir) {
	FloorRow::tileSize = tileSize;
	FloorRow::cols = cols;
	FloorRow::lightDir = lightDir;
}

vec2 FloorRow::getPos() const {
	return pos;
}

float generateSpeed() {
	const static float maxSpeed = 0.3f;
	const static float minSpeed = 0.05f;
	return (((float)rand() / RAND_MAX)*(maxSpeed - minSpeed) + minSpeed)*(-1 + (rand()%2)*2);
}

void FloorRow::moveToPosition(vec2 position) {
	static float realTileSize = tileSize.x / cols;
	float offsetX = pos.x - (realTileSize*(cols / 2) - (1 - cols % 2)*realTileSize / 2);
	for (uint i = 0; i < floorTiles.size(); ++i) {
		Object& tile = floorTiles[i];
		vec3 tilePos = tile.getPos();
		tile.setPos(vec3(tilePos.x, tilePos.y, position.y));
	}
	for (uint i = 0; i < enemies.size(); ++i) {
		speeds[i] = generateSpeed();
		float startPoint;
		if (speeds[i] >= 0)
			startPoint = -tileSize.x / 2 + ((float)rand() / RAND_MAX) * tileSize.x;
		else
			startPoint = tileSize.x / 2 - ((float)rand() / RAND_MAX) * tileSize.x;
		enemies[i].setPos(vec3(startPoint, 0, position.y));
	}
	pos = position;
}

void FloorRow::setPos(vec2 position) {
	pos = position;
}

bool applyConstraints(uint meshIndex, uint numAdjacents, vector<uint>& adjacentRow, uint position) {
	if (position + numAdjacents < adjacentRow.size() && adjacentRow[position + numAdjacents - 1] != adjacentRow[position + numAdjacents])
		return false;
	if (meshIndex == adjacentRow[position])
		return false;
	return true;
}

inline uint minim(uint a, uint b) {
	return a < b ? a : b;
}

inline int between(int min, int max) {
	return ((float)rand() / RAND_MAX)*(max - min) + min;
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

void FloorRow::initSafeZone(vector<Mesh*>& meshes) {
	enemies.clear();
	speeds.clear();
	floorTiles.resize(cols);
	rowHeight = 0.2f;

	static float realTileSize = tileSize.x / cols;
	float offsetX = pos.x - (realTileSize*(cols / 2) - (1 - cols % 2)*realTileSize / 2);

	for (uint i = 0; i < floorTiles.size(); ++i) {
		vec3 boundingBox = meshes[i]->getbbSize();
		vec3 bbcenter = meshes[i]->getbbCenter();
		float height = meshes[i]->getHeight();
		vec3 floorTileSize = vec3(realTileSize, 0.2f, tileSize.y) / boundingBox;

		TexturedObject& tile = floorTiles[i];
		tile.name = "floor tile " + to_string(i);
		tile.setTexture(&planeWood);
		tile.setMesh(meshes[i]);
		tile.setScale(floorTileSize);
		//tile.rotateY(PI / 2);
		tile.setCenter(vec3(bbcenter.x, bbcenter.y + height / 2.f, bbcenter.z));
		tile.setPos(vec3(offsetX + i*realTileSize, rowHeight, pos.y));
	}
}
float FloorRow::getHeight() const {
	return rowHeight;
}

void FloorRow::initRoad(vector<uint>& adjacentRow) {
	enemies.resize(2);
	speeds.resize(enemies.size());
	floorTiles.resize(cols);
	rowHeight = 0.0f;

	static float realTileSize = tileSize.x / cols;
	static vec3 boundingBox = cubeMesh.getbbSize();
	static vec3 bbcenter = cubeMesh.getbbCenter();
	static float height = cubeMesh.getHeight();
	vec3 floorTileSize = vec3(realTileSize, 0.1f, tileSize.y) / boundingBox;
	float offsetX = pos.x - (realTileSize*(cols / 2) - (1 - cols % 2)*realTileSize / 2);

	for (uint i = 0; i < enemies.size(); ++i) {
		ShadowedObject& enemy = enemies[i];
		enemy.setMesh(&enemyMeshes[i]);
		enemy.name = "enemy " + to_string(i);
		enemy.rotateY(PI);
		enemy.setScale(vec3(0.1f));
		enemy.setCenterToBaseCenter();
		enemy.setPlane(vec4(0, 1, 0, 0), lightDir);
		speeds[i] = generateSpeed();
		float startPoint;
		if (speeds[i] >= 0)
			startPoint = -tileSize.x / 2 + ((float)rand() / RAND_MAX) * tileSize.x;
		else
			startPoint = tileSize.x / 2 - ((float)rand() / RAND_MAX) * tileSize.x;
		enemy.setPos(vec3(startPoint, 0, pos.y));
	}

	uint textureIndex = models.size();
	Texture* texture = NULL;
	uint numAdjacentTiles = 0;
	uint counter = numAdjacentTiles;
	vector<uint> indicesAux;
	for (uint i = 0; i < floorTiles.size(); ++i) {
		TexturedObject& tile = floorTiles[i];
		tile.name = "floor tile " + to_string(i);
		if (counter == numAdjacentTiles) {
			pair<uint, uint> ret = generateRandomTextureIndex(i, textureIndex, adjacentRow);
			textureIndex = ret.first;
			numAdjacentTiles = ret.second;
			counter = 0;
		}
		texture = &floorTextures[textureIndex][counter];
		adjacentRow[i] = textureIndex;
		tile.setTexture(texture);
		tile.setMesh(&cubeMesh);
		tile.setScale(floorTileSize);
		tile.rotateY(PI / 2);
		tile.setCenter(vec3(bbcenter.x, bbcenter.y + height / 2.f, bbcenter.z));
		tile.setPos(vec3(offsetX + i*realTileSize, 0, pos.y));
		++counter;
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
			if (speeds[i] >= 0)
				startPoint = -tileSize.x / 2;
			else
				startPoint = tileSize.x / 2;
			object.setPos(vec3(startPoint, 0, pos.y));
		}
	}
}

void FloorRow::renderSimpleObjects(ShaderProgram & program) {
	for (Object& object : floorTiles) {
		Scene::sceneTriangles += object.getTriangles();
		Scene::sceneDrawCalls += 1;
		object.render(program);
	}
}

void FloorRow::renderLightObjects(ShaderProgram & program) {
	for (Object& object : enemies) {
		Scene::sceneTriangles += object.getTriangles();
		Scene::sceneDrawCalls += 1;
		object.render(program);
	}
}

void FloorRow::renderShadows(ShaderProgram & program) {
	for (ShadowedObject& object : enemies) {
		Scene::sceneTriangles += object.getTriangles();
		Scene::sceneDrawCalls += 1;
		object.renderShadow(program);
	}
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
ImportedMesh FloorRow::pirateMesh;
CubeMesh FloorRow::cubeMesh;
vector<vector<Texture>> FloorRow::floorTextures;
vector<ImportedMesh> FloorRow::enemyMeshes;
Texture FloorRow::planeWood;