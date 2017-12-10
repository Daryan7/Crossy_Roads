#pragma once
#include "Object.h"

class ShadowedObject : public Object {
	glm::mat4 shadowProjection;
public:
	glm::mat4 getShadowModel();
	void setPlane(glm::vec4 plane, glm::vec3 direction);
	ShadowedObject();
	~ShadowedObject();
};

