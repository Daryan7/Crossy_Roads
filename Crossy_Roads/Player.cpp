#include "Player.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Game.h"
#include "Pi.h"
using namespace glm;

#define FRAMES_PER_SECOND 60
#define JUMP_DURATION 11

void Player::firstInit() {
}

void Player::groupDrawableObjects(std::vector<std::vector<Object*>>& objects, std::vector<std::vector<TexturedObject*>>& texturedObjects, const FrustumG& frustum) {
	objects[playerObject.meshId].push_back(&playerObject);
}

void Player::jump() {
}

void Player::init(const Assets& assets, vec3 lightDir, vec3 offset, float jumpDistance, Floor &floor) {
	GameObject::init();
	this->lightDir = lightDir;
	this->floor = &floor;
	this->jumpDistance = jumpDistance;
	IdMesh pirateId = assets.getMeshId("pirate_2");
	playerObject.setMesh(pirateId, assets.getMesh(pirateId));
	playerObject.setScale(vec3(0.1f));
	playerObject.setCenterToBaseCenter();
	playerObject.setPos(vec3(0));
	playerObject.setPlane(vec4(0, 1, 0, 0), lightDir);
	wPressed = aPressed = sPressed = dPressed = bPressed = false;
	currentOrientation = FRONT;
	directionVector = vec3(0, 0, 1.f);
	inMovement = false;
	gravity = -0.1f;
	verticalSpeed = getJumpingSpeed(0,0,JUMP_DURATION);
	speed = this->jumpDistance / float(JUMP_DURATION);
	testJump = 0;
	currentFrame = 0;
	currentRowIndex = floor.getRows()/2 - floor.getRowOffset();
	currentColIndex = floor.getCols()/2 - floor.getColOffset();
	upsideDown = false;
}

PlayerReturn Player::update(int deltaTime) {
	PlayerReturn ret = PlayerReturn::NOTHING;
	if (!upsideDown && collides()) {
		playerObject.setRotationZ(PI);
		upsideDown = true;
		return ret;
	}
	if (inMovement) {
		if (!keepMoving()) {
			currentFrame = 0;
			inMovement = false;
		}
	}
	else {
		if (Game::instance().getKey('w')) {
			uint nextRow = (currentRowIndex + 1) % floor->getRows();
			if (!wPressed && !collidesWithEnv(nextRow,currentColIndex)) {
				ret = PlayerReturn::MOVE_FRONT;
				wPressed = true;
				performRotation('w');
				currentOrientation = FRONT;
				setDirectionVector();
				inMovement = true;
				uint previousRowIndex = currentRowIndex;
				currentRowIndex = nextRow;
				float prevHeight = floor->getFloorRow(previousRowIndex)->getHeight();
				float currentHeight = floor->getFloorRow(currentRowIndex)->getHeight();
				verticalSpeed = getJumpingSpeed(prevHeight, currentHeight, JUMP_DURATION);
			}
		}
		else
			wPressed = false;
		if (Game::instance().getKey('a')) {
			uint nextCol = currentColIndex == 28 ? 28 : currentColIndex + 1;
			if (!aPressed && !collidesWithEnv(currentRowIndex, currentColIndex + 1)) {
				ret = PlayerReturn::MOVE_LEFT;
				aPressed = true;
				performRotation('a');
				currentOrientation = LEFT;
				setDirectionVector();
				inMovement = true;
				currentColIndex += 1;
			}
		}
		else
			aPressed = false;
		if (Game::instance().getKey('d')) {
			uint nextCol = currentColIndex == 0 ? 0 : currentRowIndex - 1;
			if (!dPressed && !collidesWithEnv(currentRowIndex, currentColIndex - 1)) {
				ret = PlayerReturn::MOVE_RIGHT;
				dPressed = true;
				performRotation('d');
				currentOrientation = RIGHT;
				setDirectionVector();
				inMovement = true;
				currentColIndex -= 1;
			}
		}
		else
			dPressed = false;
		if (Game::instance().getKey('s')) {
			uint nextRow = currentRowIndex == 0 ? floor->getRows() - 1 : currentRowIndex - 1;
			if (!sPressed && !collidesWithEnv(nextRow, currentColIndex)) {
				ret = PlayerReturn::MOVE_BACK;
				sPressed = true;
				performRotation('s');
				currentOrientation = BACK;
				setDirectionVector();
				inMovement = true;
				uint previousRowIndex = currentRowIndex;
				currentRowIndex = nextRow;
				float prevHeight = floor->getFloorRow(previousRowIndex)->getHeight();
				float currentHeight = floor->getFloorRow(currentRowIndex)->getHeight();
				verticalSpeed = getJumpingSpeed(prevHeight, currentHeight, JUMP_DURATION);
			}
		}
		else
			sPressed = false;
		//THIS IS THE DEBUG KEY AND WILL BE DELETED BEFORE DELIVERING
		//USE IT TO TEST STUFF ON THE PLAYER
		if (Game::instance().getKey('b')) {
			if (!bPressed) {
				bPressed = true;
				playerObject.rotateZ(PI);
				upsideDown = false;
			}
		}
		else
			bPressed = false;
	}
	return ret;
}

vec3 Player::getPos() const {
	return playerObject.getPos();
}

void Player::setDirectionVector() {
	switch (currentOrientation) {
	case FRONT:
		directionVector = vec3(0, 0, 1);
		break;
	case LEFT:
		directionVector = vec3(1, 0, 0);
		break;
	case BACK:
		directionVector = vec3(0, 0, -1);
		break;
	case RIGHT:
		directionVector = vec3(-1, 0, 0);
		break;
	}
}

void Player::performRotation(char key) {
	switch (key) {
	case 'w':
		playerObject.setRotationY(0);
		break;
	case 'a':
		playerObject.setRotationY(PI/2);
		break;
	case 'd':
		playerObject.setRotationY(-PI / 2);
		break;
	case 's':
		playerObject.setRotationY(PI);
		break;
	}
}

bool Player::keepMoving() {
	float auxHeight = floor->getFloorRow(currentRowIndex)->getHeight();
	currentFrame++;
	bool returnValue = true;
	playerObject.setPlane(vec4(0, 1, 0, -auxHeight), lightDir);

	currentVerticalSpeed = verticalSpeed + gravity*currentFrame;
	if (currentFrame == JUMP_DURATION) {
		currentVerticalSpeed =  -(playerObject.getPos().y - auxHeight);
		returnValue = false;
	}
	vec3 horizontalMove = directionVector*speed;
	vec3 verticalMove = vec3(0, currentVerticalSpeed, 0);
	playerObject.move(horizontalMove + verticalMove);
	return returnValue;
}

float Player::getJumpingSpeed(float y0, float y, uint frames) {
	return (y-y0)/frames -0.5f*gravity*frames;
}

bool Player::collides() {
	bool collision = false;
	FloorRow* currentRow = floor->getFloorRow(currentRowIndex);
	vector<ShadowedObject>* rowEnemies = currentRow->getEnemies();
	for (uint i = 0; i < rowEnemies->size() && !collision; ++i) {
		collision = playerObject.collidesWith((*rowEnemies)[i]);
	}
	return collision;
}

bool Player::collidesWithEnv(uint row, uint col) {
	FloorRow* rowToCheck = floor->getFloorRow(row);
	if (!rowToCheck->isSafeZone()) return false;
	vector<CellProperties>* rowObjects = rowToCheck->getRowObjects();
	return (*rowObjects)[col].height != 0;
}

Player::Player(){
}

Player::~Player()
{
}
