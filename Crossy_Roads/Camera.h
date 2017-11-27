#pragma once
#include "Camera\eulerperspcamera.h"
#include "Camera\obsorthocamera.h"
#include "Player.h"

class Camera {
private:
	const Player* player;
	glm::vec3 lightDir;
	EulerPerspCamera cam;
	OBSOrthoCamera uiCam, lightCam;
	bool cameraMode;
private:
	void cameraControl();
public:
	void init(glm::vec3 lightDir, const Player* player);
	void resize(int w, int h);
	void update(int deltaTime);
	void updateVM();
	const glm::mat4* getProjectionMatrix() const;
	const glm::mat4* getViewMatrix() const;
	glm::mat4 getVPLightMatrix() const;
	glm::mat4 getVPMatrix() const;
	glm::vec3 getPos() const;
	void setPos(glm::vec3 pos);
	Camera();
	~Camera();
};