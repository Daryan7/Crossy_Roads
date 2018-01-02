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

const RandomMeshConfig * Assets::getRandomMesh(const string & name) const {
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

const std::vector<MeshConfigConstructor*>* Assets::getDecoration() const {
	return decorationGroup;
}

MeshBehavior Assets::getBehavior(IdMesh mesh) const {
	return behaviours[mesh];
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

inline MeshBehavior behaviorString2enum(const string& behaviorString) {
	if (behaviorString == "stalker")
		return MeshBehavior::Stalker;
	else if (behaviorString == "None")
		return MeshBehavior::None;
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
	behaviours = new MeshBehavior[nImportedMeshes];

	string name, type;
	uint i = 0;
	vector<string> idMeshNames(nImportedMeshes);
	for (const Value& meshProperties : models.GetArray()) {
		uint firstId = i;
		const Value& namesV = meshProperties["names"];
		for (const Value& nameValue : namesV.GetArray()) {
			name.assign(nameValue.GetString());
			meshes[i].loadFromFile("models/" + name);
			meshIds[name] = i;
			idMeshNames[i] = name;
			++i;
		}
		uint nMeshes = namesV.Size();
		type.assign(meshProperties["type"].GetString());
		if (meshProperties.HasMember("behaviour")) {
			behaviours[firstId] = behaviorString2enum(meshProperties["behaviour"].GetString());
		}
		else
			behaviours[firstId] = MeshBehavior::None;

		if (type == "decoration") {
			const Value& biomeV = meshProperties["biomes"];
			const Value& size = meshProperties["size"];
			MeshConfigConstructor* constructor;
			if (nMeshes > 1) {
				RandomMeshConfig* randomMeshConfig = new RandomMeshConfig();
				randomMeshConfig->rows = size[0].GetUint();
				randomMeshConfig->cols = size[1].GetUint();

				const Value& heightsV = meshProperties["height"];
				float* heights = new float[nMeshes];
				if (heightsV.IsArray()) {
					for (uint i = 0; i < nMeshes; ++i)
						heights[i] = heightsV[i].GetFloat();
				}
				else {
					float height = heightsV.GetFloat();
					for (uint i = 0; i < nMeshes; ++i)
						heights[i] = height;
				}

				bool* empty = new bool[nMeshes];
				if (meshProperties.HasMember("floor empty")) {
					const Value& emptiesV = meshProperties["floor empty"];
					for (uint i = 0; i < nMeshes; ++i)
						empty[i] = emptiesV[i].GetBool();
				}
				else
					memset(empty, false, nMeshes);

				bool* collisionMap = new bool[randomMeshConfig->rows*randomMeshConfig->cols];
				if (meshProperties.HasMember("collision map")) {
					const Value& collisionMapV = meshProperties["collision map"];
					for (uint i = 0; i < collisionMapV.Size(); ++i) {
						collisionMap[i] = collisionMapV[i].GetBool();
					}
				}
				else
					memset(collisionMap, true, randomMeshConfig->rows*randomMeshConfig->cols);
				
				const Value& probabilitiesV = meshProperties["probabilities"];
				float* probabilities = new float[nMeshes];
				if (probabilitiesV.IsArray()) {
					for (uint i = 0; i < nMeshes; ++i)
						probabilities[i] = probabilitiesV[i].GetFloat();
				}
				else {
					float probability = 1.f / nMeshes;
					for (uint i = 0; i < nMeshes; ++i)
						probabilities[i] = probability;
				}
				randomMeshConfig->setProbabilities(probabilities, nMeshes);
				randomMeshConfig->firstMesh = firstId;
				randomMeshConfig->heights = heights;
				randomMeshConfig->empty = empty;
				randomMeshConfig->collisionMap = collisionMap;
				randomMeshConfig->canJump = false;
				constructor = randomMeshConfig;
			}
			else {
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

				bool* collisionMap = new bool[aux.rows*aux.cols];
				if (meshProperties.HasMember("collision map")) {
					const Value& collisionMapV = meshProperties["collision map"];
					for (uint i = 0; i < collisionMapV.Size(); ++i) {
						collisionMap[i] = collisionMapV[i].GetBool();
					}
				}
				else
					memset(collisionMap, true, aux.rows*aux.cols);
				aux.collisionMap = collisionMap;
				aux.canJump = false;
				constructor = new BasicMeshConfig();
				((BasicMeshConfig*)constructor)->meshConfig = aux;
			}
			for (const Value& biomeName : biomeV.GetArray()) {
				decorationGroup[biomeString2enum(biomeName.GetString())].push_back(constructor);
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
			map<string, RandomMeshConfig>::iterator it = randomGroup.insert(make_pair(groupName, RandomMeshConfig())).first;
			RandomMeshConfig& randomMesh = it->second;
			const Value& size = meshProperties["size"];
			randomMesh.rows = size[0].GetUint();
			randomMesh.cols = size[1].GetUint();
			const Value& heightsV = meshProperties["height"];
			float* heights = new float[heightsV.Size()];
			bool* empty = new bool[nMeshes];
			if (meshProperties.HasMember("floor empty")) {
				const Value& emptiesV = meshProperties["floor empty"];
				for (uint i = 0; i < nMeshes; ++i) {
					empty[i] = emptiesV[i].GetBool();
				}
			}
			else
				memset(empty, false, nMeshes);
			const Value& probabilitiesV = meshProperties["probabilities"];
			float* probabilities = new float[probabilitiesV.Size()];
			for (uint i = 0; i < heightsV.Size(); ++i) {
				heights[i] = heightsV[i].GetFloat();
				probabilities[i] = probabilitiesV[i].GetFloat();
			}

			bool* collisionMap = new bool[randomMesh.rows*randomMesh.cols];
			if (meshProperties.HasMember("collision map")) {
				const Value& collisionMapV = meshProperties["collision map"];
				for (uint i = 0; i < collisionMapV.Size(); ++i) {
					collisionMap[i] = collisionMapV[i].GetBool();
				}
			}
			else
				memset(collisionMap, true, randomMesh.rows*randomMesh.cols);
			randomMesh.collisionMap = collisionMap;
			randomMesh.setProbabilities(probabilities, probabilitiesV.Size());
			randomMesh.firstMesh = firstId;
			randomMesh.heights = heights;
			randomMesh.empty = empty;
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
	string extension, fullName;
	i = 0;
	vector<string> idTexNames(nTextures);
	vector<string> idTexFullNames(nTextures);
	for (const Value& texture : texturesJ.GetArray()) {
		bool linear = texture["type"].GetString() == "linear";
		const Value& namesV = texture["names"];
		if (namesV.Size() > 1) {
			map<string, pair<IdTex, uint>>::iterator it = animatedTextureGroup.insert(make_pair(texture["group name"].GetString(), pair<IdTex, uint>())).first;
			pair<IdTex, uint>& animTextures = it->second;
			animTextures.first = i;
			animTextures.second = namesV.Size();
		}
		bool hasExtension = texture.HasMember("extension");
		extension.assign(hasExtension ? texture["extension"].GetString() : "png");
		for (const Value& texName : namesV.GetArray()) {
			name.assign(texName.GetString());
			fullName.assign("images/" + name + "." + extension);
			if (linear) {
				textures[i].loadFromFile(fullName, TEXTURE_PIXEL_FORMAT_RGBA, true);
			}
			else {
				textures[i].loadFromFile(fullName, TEXTURE_PIXEL_FORMAT_RGBA, false);
				textures[i].magFilter = GL_NEAREST;
				textures[i].minFilter = GL_NEAREST;
			}
			textures[i].applyParams();
			textureIds[name] = i;
			idTexNames[i] = name;
			idTexFullNames[i] = fullName;
			++i;
		}
	}

	ofstream output("binaryAssets.notxt", ofstream::binary);
	uint size = idMeshNames.size();
	output.write((const char*)&size, sizeof(uint));
	for (uint i = 0; i < idMeshNames.size(); ++i) {
		uint length = idMeshNames[i].length();
		output.write((const char*)&length, sizeof(uint));
		output.write(idMeshNames[i].c_str(), length);
	}
	output.write((const char*)behaviours, sizeof(MeshBehavior)*size);

	size = idTexNames.size();
	output.write((const char*)&size, sizeof(uint));
	for (uint i = 0; i < idTexNames.size(); ++i) {
		uint length = idTexNames[i].length();
		output.write((const char*)&length, sizeof(uint));
		output.write(idTexNames[i].c_str(), length);

		string fullName = idTexFullNames[i];
		length = fullName.length();
		output.write((const char*)&length, sizeof(uint));
		output.write(fullName.c_str(), length);

		output.write((const char*)&textures[i].wrapT, sizeof(GLint));
		output.write((const char*)&textures[i].wrapS, sizeof(GLint));
		output.write((const char*)&textures[i].magFilter, sizeof(GLint));
		output.write((const char*)&textures[i].minFilter, sizeof(GLint));
		bool mipmap = textures[i].hasMipmap();
		output.write((const char*)&mipmap, sizeof(bool));
	}

	for (uint i = 0; i < nBiomes; ++i) {
		for (uint j = 0; j < nGroups; ++j) {
			uint size = groups[i][j].size();
			output.write((const char*)&size, sizeof(uint));
			if (size == 0)
				continue;
			output.write((const char*)&groups[i][j][0], sizeof(IdMesh)*size);
		}
	}

	for (uint i = 0; i < nBiomes; ++i) {
		vector<MeshConfigConstructor*>& decorG = decorationGroup[i];
		uint size = decorG.size();
		output.write((const char*)&size, sizeof(uint));
		for (uint j = 0; j < decorG.size(); ++j) {
			MeshConfigConstructor* constructor = decorG[j];
			constructor->store(output);
		}
	}

	size = randomGroup.size();
	output.write((const char*)&size, sizeof(uint));
	for (pair<const string, RandomMeshConfig>& p : randomGroup) {
		uint length = p.first.length();
		output.write((const char*)&length, sizeof(uint));
		output.write(p.first.c_str(), length);
		p.second.store(output);
	}

	size = animatedTextureGroup.size();
	output.write((const char*)&size, sizeof(uint));
	for (pair <const string, pair<IdTex, uint>>& p : animatedTextureGroup) {
		uint length = p.first.length();
		output.write((const char*)&length, sizeof(uint));
		output.write(p.first.c_str(), length);
		output.write((const char*)&p.second, sizeof(pair<IdTex, uint>));
	}

	size = animatedMeshGroup.size();
	output.write((const char*)&size, sizeof(uint));
	for (pair <const string, pair<IdTex, uint>>& p : animatedMeshGroup) {
		uint length = p.first.length();
		output.write((const char*)&length, sizeof(uint));
		output.write(p.first.c_str(), length);
		output.write((const char*)&p.second, sizeof(pair<IdTex, uint>));
	}

	output.close();
}

void Assets::loadAssets(const string & binaryPath) {
	cubeMesh.init();
	ifstream input(binaryPath, ifstream::binary);

	input.read((char*)&nImportedMeshes, sizeof(uint));
	meshes = new ImportedMesh[nImportedMeshes];
	for (uint i = 0; i < nImportedMeshes; ++i) {
		uint length;
		input.read((char*)&length, sizeof(uint));
		char* c_str = new char[length];
		input.read(c_str, length);
		string name(c_str, length);
		delete c_str;
		meshes[i].loadFromBinary("models/" + name);
		meshIds[name] = i;
	}
	behaviours = new MeshBehavior[nImportedMeshes];
	input.read((char*)behaviours, sizeof(MeshBehavior)*nImportedMeshes);

	input.read((char*)&nTextures, sizeof(uint));
	textures = new Texture[nTextures];
	for (uint i = 0; i < nTextures; ++i) {
		uint length;
		input.read((char*)&length, sizeof(uint));
		char* c_str = new char[length];
		input.read(c_str, length);
		string name(c_str, length);
		delete c_str;

		input.read((char*)&length, sizeof(uint));
		c_str = new char[length];
		input.read(c_str, length);
		string fullName(c_str, length);
		delete c_str;

		input.read((char*)&textures[i].wrapT, sizeof(GLint));
		input.read((char*)&textures[i].wrapS, sizeof(GLint));
		input.read((char*)&textures[i].magFilter, sizeof(GLint));
		input.read((char*)&textures[i].minFilter, sizeof(GLint));
		bool mipmap;
		input.read((char*)&mipmap, sizeof(bool));

		textures[i].loadFromFile(fullName, TEXTURE_PIXEL_FORMAT_RGBA, mipmap);
		textures[i].applyParams();
		textureIds[name] = i;
	}

	for (uint i = 0; i < nBiomes; ++i) {
		for (uint j = 0; j < nGroups; ++j) {
			uint size;
			input.read((char*)&size, sizeof(uint));
			if (size == 0)
				continue;
			groups[i][j].resize(size);
			input.read((char*)&groups[i][j][0], sizeof(IdMesh)*size);
		}
	}

	for (uint i = 0; i < nBiomes; ++i) {
		uint size;
		input.read((char*)&size, sizeof(uint));
		decorationGroup[i].resize(size);
		for (uint j = 0; j < size; ++j) {
			MeshConfigConstructor* constructor;
			MeshConfigConstructorType type;
			input.read((char*)&type, sizeof(MeshConfigConstructorType));
			if (type == MeshConfigConstructorType::Basic) {
				constructor = new BasicMeshConfig();
			}
			else {
				constructor = new RandomMeshConfig();
			}
			constructor->load(input);
			decorationGroup[i][j] = constructor;
		}
	}

	uint size;
	input.read((char*)&size, sizeof(uint));
	for (uint i = 0; i < size; ++i) {
		uint length;
		input.read((char*)&length, sizeof(uint));
		char* c_str = new char[length];
		input.read(c_str, length);
		string groupName(c_str, length);
		delete c_str;

		MeshConfigConstructorType type;
		input.read((char*)&type, sizeof(MeshConfigConstructorType));

		map<string, RandomMeshConfig>::iterator it = randomGroup.insert(make_pair(groupName, RandomMeshConfig())).first;
		RandomMeshConfig& randomMesh = it->second;
		randomMesh.load(input);
	}

	input.read((char*)&size, sizeof(uint));
	for (uint i = 0; i < size; ++i) {
		uint length;
		input.read((char*)&length, sizeof(uint));
		char* c_str = new char[length];
		input.read(c_str, length);
		string name(c_str, length);
		delete c_str;
		pair<IdTex, uint> p;
		input.read((char*)&p, sizeof(pair<IdTex, uint>));
		animatedTextureGroup.insert(make_pair(name, p));
	}

	input.read((char*)&size, sizeof(uint));
	for (uint i = 0; i < size; ++i) {
		uint length;
		input.read((char*)&length, sizeof(uint));
		char* c_str = new char[length];
		input.read(c_str, length);
		string name(c_str, length);
		delete c_str;
		pair<IdTex, uint> p;
		input.read((char*)&p, sizeof(pair<IdTex, uint>));
		animatedMeshGroup.insert(make_pair(name, p));
	}

	input.close();
}

Assets::Assets() : meshes(NULL), textures(NULL) {
}


Assets::~Assets() {
	if (meshes != NULL) {
		delete meshes;
		delete textures;
	}
}
