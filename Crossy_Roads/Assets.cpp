#include "Assets.h"
#include <document.h>
#include <error/en.h>
#include <fstream>
#include <iostream>
using namespace std;
using namespace glm;
using namespace rapidjson;
typedef map<string, int>::const_iterator Iterator;

const CubeMesh * Assets::getCubeMesh() const {
	return &cubeMesh;
}

int Assets::getMeshId(const string & name) const {
	Iterator it = meshIds.find(name);
	if (it == meshIds.end()) {
		int a = 3;
	}
	return it == meshIds.end() ? INVALID : it->second;
}

int Assets::getTextureId(const string & name) const {
	Iterator it = textureIds.find(name);
	if (it == textureIds.end()) {
		int a = 3;
	}
	return it == textureIds.end() ? INVALID : it->second;
}

const ImportedMesh * Assets::getMesh(IdMesh id) const {
	return &meshes[id];
}

const Texture* Assets::getTexture(IdTex id) const {
	return &textures[id];
}

const Texture* Assets::getTexture(const string& name) const {
	return &textures[textureIds.find(name)->second];
}

const RandomPickMesh * Assets::getRandomMesh(const string & name) const {
	return &randomGroup.find(name)->second;
}

pair<IdTex, uint> Assets::getAnimatedTexture(const string & name) const {
	return animatedTextureGroup.find(name)->second;
}

pair<IdMesh, uint> Assets::getAnimatedMesh(const string & name) const {
	return animatedMeshGroup.find(name)->second;
}

const std::vector<IdMesh>* Assets::getGroups() const {
	return &groups[0][0];
}

const std::vector<MeshConfig>* Assets::getDecoration() const {
	return decorationGroup;
}

uint Assets::getNumMeshes() const {
	return nImportedMeshes;
}

uint Assets::getNumTextures() const {
	return nTextures;
}

inline BiomeType biomeString2enum(const string& biomeString) {
	if (biomeString == "sea")
		return Sea;
	else if (biomeString == "ship")
		return Ship;
	else if (biomeString == "island")
		return Island;
	int a = 3;
}

void Assets::loadAssets(const string& modelPath, const string& texturePath) {
	ifstream jsonFile(modelPath);
	string s((istreambuf_iterator<char>(jsonFile)), istreambuf_iterator<char>());
	jsonFile.close();

	Document document;
	document.Parse(s.c_str());
	s.clear();
	if (document.HasParseError()) {
		fprintf(stderr, "\nError(offset %u): %s\n",
			(unsigned)document.GetErrorOffset(),
			GetParseError_En(document.GetParseError()));
	}

	const Value& models = document["models"];
	nImportedMeshes = 0;

	for (const Value& meshProperties : models.GetArray()) {
		nImportedMeshes += meshProperties["names"].Size();
	}

	for (uint i = 0; i < nBiomes; ++i) {
		for (uint j = 0; j < nGroups; ++j) {
			groups[i][j].reserve(nImportedMeshes);
		}
		decorationGroup[i].reserve(nImportedMeshes);
	}

	cubeMesh.init();
	meshes = new ImportedMesh[nImportedMeshes];

	string name, type;
	uint i = 0;
	for (const Value& meshProperties : models.GetArray()) {
		uint firstId = i;
		const Value& namesV = meshProperties["names"];
		for (const Value& nameValue : meshProperties["names"].GetArray()) {
			name.assign(nameValue.GetString());
			meshes[i].loadFromFile("models/" + name + ".obj");
			meshIds[name] = i;
			++i;
		}

		type.assign(meshProperties["type"].GetString());

		if (type == "decoration") {
			const Value& biomeV = meshProperties["biomes"];
			const Value& size = meshProperties["size"];
			MeshConfig aux;
			if (meshProperties.HasMember("floor empty")) {
				aux.floorEmpty = meshProperties["floor empty"].GetBool();
			}
			else
				aux.floorEmpty = false;
			aux.rows = size[0].GetUint();
			aux.cols = size[1].GetUint();
			aux.height = meshProperties["height"].GetFloat();
			aux.mesh = firstId;
			for (const Value& biomeName : biomeV.GetArray()) {
				decorationGroup[biomeString2enum(biomeName.GetString())].push_back(aux);
			}
		}
		else if (type == "enemy") {
			const Value& biomeV = meshProperties["biomes"];
			for (const Value& biomeName : biomeV.GetArray()) {
				groups[biomeString2enum(biomeName.GetString())][Enemy].push_back(firstId);
			}
		}
		else if (type == "platform") {
			const Value& biomeV = meshProperties["biomes"];
			if (namesV.Size() > 1) {
				map<string, pair<IdMesh, uint>>::iterator it = animatedMeshGroup.insert(make_pair(meshProperties["group name"].GetString(), pair<IdMesh, uint>())).first;
				pair<IdMesh, uint>& animMeshes = it->second;
				animMeshes.first = firstId;
				animMeshes.second = namesV.Size();
			}
			for (const Value& biomeName : biomeV.GetArray()) {
				groups[biomeString2enum(biomeName.GetString())][Platform].push_back(firstId);
			}
		}
		else if (type == "random") {
			string groupName = meshProperties["group name"].GetString();
			const Value& probabilitiesV = meshProperties["probabilities"];
			float* probabilities = new float[probabilitiesV.Size()];
			uint j = 0;
			for (const Value& prob : probabilitiesV.GetArray()) {
				float probNum = prob.GetFloat();
				probabilities[j] = probNum;
				++j;
			}
			map<string, RandomPickMesh>::iterator it = randomGroup.insert(make_pair(groupName, RandomPickMesh())).first;
			RandomPickMesh& randomMesh = it->second;
			randomMesh.setMeshes(firstId, probabilities, probabilitiesV.Size());
		}
		else if (type != "unique") {
			int a = 3;
		}
	}

	for (uint i = 0; i < nBiomes; ++i) {
		for (uint j = 0; j < nGroups; ++j) {
			groups[i][j].shrink_to_fit();
		}
		decorationGroup[i].shrink_to_fit();
	}

	jsonFile.open(texturePath);
	s.assign((istreambuf_iterator<char>(jsonFile)), istreambuf_iterator<char>());
	jsonFile.close();

	document.Parse(s.c_str());
	s.clear();

	const Value& texturesJ = document["textures"];
	nTextures = 0;

	for (const Value& texture : texturesJ.GetArray()) {
		nTextures += texture["names"].Size();
	}

	textures = new Texture[nTextures];
	i = 0;
	for (const Value& texture : texturesJ.GetArray()) {
		bool linear = texture["type"].GetString() == "linear";
		const Value& namesV = texture["names"];
		if (namesV.Size() > 1) {
			map<string, pair<IdTex, uint>>::iterator it = animatedTextureGroup.insert(make_pair(texture["group name"].GetString(), pair<IdTex, uint>())).first;
			pair<IdTex, uint>& animTextures = it->second;
			animTextures.first = i;
			animTextures.second = namesV.Size();
		}
		for (const Value& texName : namesV.GetArray()) {
			name.assign(texName.GetString());
			if (linear) {
				textures[i].loadFromFile("images/" + name + ".png", TEXTURE_PIXEL_FORMAT_RGBA, true);
			}
			else {
				textures[i].loadFromFile("images/" + name + ".png", TEXTURE_PIXEL_FORMAT_RGBA, false);
				textures[i].setMagFilter(GL_NEAREST);
				textures[i].setMinFilter(GL_NEAREST);
			}
			textures[i].applyParams();
			textureIds[name] = i;
			++i;
		}
	}
}

Assets::Assets() : meshes(NULL), textures(NULL) {
}


Assets::~Assets() {
	if (meshes != NULL) {
		delete meshes;
		delete textures;
	}
}
