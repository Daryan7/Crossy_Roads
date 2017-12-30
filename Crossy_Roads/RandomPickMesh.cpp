#include "RandomPickMesh.h"
using namespace glm;

void RandomPickMesh::setMeshes(IdMesh firstMesh, float * probabilities, uint size) {
	this->firstMesh = firstMesh;
	this->probabilities = probabilities;
}

IdMesh RandomPickMesh::getMesh() const {
	float pick = rand() / (float)(RAND_MAX+1);
	float prob = 0;
	for (uint i = 0; i < size; ++i) {
		if (pick >= prob && pick < prob + probabilities[i]) {
			return firstMesh + i;
		}
		prob += probabilities[i];
	}
	return INVALID;
}

RandomPickMesh::RandomPickMesh() : probabilities(NULL)
{
}

RandomPickMesh::~RandomPickMesh() {
	if (probabilities != NULL) {
		delete probabilities;
	}
}