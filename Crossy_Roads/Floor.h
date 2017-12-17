#pragma once
#include "QuadMesh.h"
#include <vector>
#include "ShadowedObject.h"
#include "ImportedMesh.h"
#include "FloorRow.h"
#include "Assets.h"
#include "ObjectContainer.h"
#include "FrustumG.h"

struct MeshConfig {
	glm::uint rows, cols;
	float height;
	IdMesh mesh;
};

class Floor : public GameObject, public ObjectContainer {
	glm::uint rows, cols;
	glm::uint lastRow;
	IdMesh plankMesh, railMesh;
	float firstPos;
	glm::uint colOffset;
	glm::uint rowOffset;
	glm::uint counter, biomeCounter;
	glm::uint length, biomeLength;
	std::vector<MeshConfig> furniture;
	std::vector<FloorRow> floorRows;
	std::vector<std::vector<CellProperties>> map;
	std::vector<glm::uint> textureIndex;
	FloorType type;
	Biome biome;
	glm::vec2 tileSize;
private:
	void updateFloorRow(FloorRow& floorRow);
	void updateMap(bool lastRow, glm::uint size);
public:
	void addLevel();
	void firstInit() override;
	void init(glm::vec3 lightDir, const Assets& assets);
	void update(int deltaTime);
	void groupDrawableObjects(
		std::vector<std::vector<Object*>>& objects,
		std::vector<std::vector<TexturedObject*>>& texturedObjects,
		const FrustumG& frustum
	) override;
	FloorRow* getFloorRow(glm::uint index);
	glm::uint getRows() const;
	glm::uint getCols() const;
	glm::uint getColOffset() const;
	glm::uint getRowOffset() const;
	glm::vec2 getTileSize() const;
	Floor();
	~Floor();
};

