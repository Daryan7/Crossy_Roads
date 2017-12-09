#include "Camera.h"
#include "Game.h"
#include <GL\freeglut.h>
#include "Pi.h"
using namespace glm;

const float playerOffset = 5;

void Camera::init(vec3 lightDir, const Player* player) {
	this->player = player;
	cam.VRP = player->getPos() + vec3(0, 0, playerOffset);
	cam.d = 24;
	cam.phi = 0;
	cam.psi = 9.6f;
	cam.theta = -5.4f;
	cam.ra = float(CAMERA_WIDTH) / float(CAMERA_HEIGHT);
	cam.setFOV(45.f / 180.f * PI);
	cam.zNear = 0.1f;
	cam.zFar = 100.f;
	cam.updatePM();

	this->lightDir = lightDir*25.f;

	lightCam.VRP = cam.VRP;
	lightCam.OBS = cam.VRP + lightDir;
	lightCam.UP = vec3(0, 1, 0);
	lightCam.left = -22.5f;
	lightCam.right = 11.6;
	lightCam.bottom = -16.5;
	lightCam.top = 11.6f;
	lightCam.zNear = 0;
	lightCam.zFar = 100;
	lightCam.updatePM();

	updateVM();
}

void Camera::resize(int w, int h) {
	cam.resize(h, w);
	cam.updatePM();
}

inline void Camera::cameraControl() {
	if (Game::instance().getSpecialKey(GLUT_KEY_LEFT)) {
		cam.psi += 0.1f;
		updateVM();
	}
	else if (Game::instance().getSpecialKey(GLUT_KEY_RIGHT)) {
		cam.psi -= 0.1f;
		updateVM();
	}
	else if (Game::instance().getSpecialKey(GLUT_KEY_UP)) {
		cam.d -= 0.1f;
		updateVM();
	}
	else if (Game::instance().getSpecialKey(GLUT_KEY_DOWN)) {
		cam.d += 0.1f;
		updateVM();
	}
	else if (Game::instance().getKey('r')) {
		cam.theta += 0.1f;
		updateVM();
	}
	else if (Game::instance().getKey('f')) {
		cam.theta -= 0.1f;
		updateVM();
	}
}

void Camera::update(int deltaTime) {
	cameraControl();
	vec3 playerPos = player->getPos();
	playerPos.z += playerOffset;

	vec2 vel(0);

	if (playerPos.z-cam.VRP.z > 0.5f)
		vel.y = (playerPos.z - cam.VRP.z)*0.03f;
	else
		vel.y = 0.01f;
	if (playerPos.x > 5.f)
		vel.x = (5.f - cam.VRP.x)*0.03f;
	else if (playerPos.x < -5.f)
		vel.x = (-5.f - cam.VRP.x)*0.03f;
	else
		vel.x = (playerPos.x - cam.VRP.x)*0.03f;

	cam.VRP.x += vel.x;
	cam.VRP.z += vel.y;
	lightCam.VRP = cam.VRP;
	lightCam.OBS = cam.VRP + lightDir;
	updateVM();
}

void Camera::updateVM() {
	cam.updateVM();
	lightCam.updateVM();
	VP = (*cam.getFullProjectionMatrix())*(*cam.getFullViewMatrix());
	VPLight = (*lightCam.getFullProjectionMatrix())*(*lightCam.getFullViewMatrix());
	frustum.setFrustum(&VP[0][0]);
}

const glm::mat4 * Camera::getProjectionMatrix() const {
	return cam.getFullProjectionMatrix();
}

const glm::mat4 * Camera::getViewMatrix() const {
	return cam.getFullViewMatrix();
}

const glm::mat4* Camera::getVPLightMatrix() const {
	return &VPLight;
}

const mat4* Camera::getVPMatrix() const {
	return &VP;
}

glm::vec3 Camera::getPos() const {
	return cam.VRP;
}

void Camera::renderFrustum() const {
	frustum.drawPlanes();
	//frustum.drawNormals();
}

const FrustumG & Camera::getFrustum() const {
	return frustum;
}

Camera::Camera()
{
}

Camera::~Camera()
{
}
