#include "CubeMesh.h"
using namespace glm;
#include "Scene.h"
void CubeMesh::render() const {
	Scene::sceneTriangles += totalTriangles;
	Scene::sceneDrawCalls += 1;
	glDrawElements(renderMode, nVertices, GL_UNSIGNED_INT, (void*)0);
}

void CubeMesh::init() {

	static const float vertices[] = {
		-1.0f, -1.0f, -1.f,
		 1.0f, -1.0f, -1.f,
		-1.0f,  1.0f, -1.f,
		 1.0f,  1.0f, -1.f,

		-1.0f, -1.0f, 1.f,
		 1.0f, -1.0f, 1.f,
		-1.0f,  1.0f, 1.f,
		 1.0f,  1.0f, 1.f,
		 
		-1.0f, -1.f, -1.0f,
		 1.0f, -1.f, -1.0f,
		-1.0f, -1.f,  1.0f,
		 1.0f, -1.f,  1.0f,

		-1.0f, 1.f, -1.0f,
		 1.0f, 1.f, -1.0f,
		-1.0f, 1.f,  1.0f,
		 1.0f, 1.f,  1.0f,

		-1.f, -1.0f, -1.0f,
		-1.f,  1.0f, -1.0f,
		-1.f, -1.0f,  1.0f,
		-1.f,  1.0f,  1.0f,

		 1.f, -1.0f, -1.0f,
		 1.f,  1.0f, -1.0f,
		 1.f, -1.0f,  1.0f,
		 1.f,  1.0f,  1.0f,
	};
	static const float normals[] = {
		0,0,-1,
		0,0,-1,
		0,0,-1,
		0,0,-1,

		0,0,1,
		0,0,1,
		0,0,1,
		0,0,1,

		0,-1,0,
		0,-1,0,
		0,-1,0,
		0,-1,0,

		0,1,0,
		0,1,0,
		0,1,0,
		0,1,0,

		-1,0,0,
		-1,0,0,
		-1,0,0,
		-1,0,0,

		1,0,0,
		1,0,0,
		1,0,0,
		1,0,0
	};

	//0-0.5 0-0.33 cara y
	//0.5-1 0-0.33 cara -y
	//0-0.5 0.33-0.66 cara -z
	//0.5-1 0-33-0.66 cara z
	//0-0.5 0.66-1 cara -x
	//0.5-1 0.66-1 cara x

	/*static const float st = 0.1;
	static const float sX = 0.5f;
	static const float sY = 1.f / 3;

	static const float x0 = 0;
	static const float x1 = sX;
	static const float y0 = 0;
	static const float y1 = sY;*/

	static const float texCoords[] = {
		0,0,
		1,0,
		0,1,
		1,1,

		0,0,
		1,0,
		0,1,
		1,1,

		0,0,
		1,0,
		0,1,
		1,1,

		0,0,
		1,0,
		0,1,
		1,1,

		0,0,
		1,0,
		0,1,
		1,1,

		0,0,
		1,0,
		0,1,
		1,1

	};
	
	static const GLuint indices[] = {
		0,1,2,
		1,2,3,

		4,5,6,
		5,6,7,

		8,9,10,
		9,10,11,

		12,13,14,
		13,14,15,

		16,17,18,
		17,18,19,

		20,21,22,
		21,22,23
	};
	bbox[0] = vec3(-1);
	bbox[1] = vec3(1);
	generateAllbbPoints();
	height = 2;
	center = vec3(0);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBOvert);
	glBindBuffer(GL_ARRAY_BUFFER, VBOvert);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &VBOnorm);
	glBindBuffer(GL_ARRAY_BUFFER, VBOnorm);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);

	glGenBuffers(1, &VBOtex);
	glBindBuffer(GL_ARRAY_BUFFER, VBOtex);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);

	glGenBuffers(1, &VBOind);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOind);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	nVertices = sizeof(indices)/sizeof(GLuint);

	totalTriangles = 12;
}

CubeMesh::CubeMesh()
{
}

CubeMesh::~CubeMesh()
{
}
