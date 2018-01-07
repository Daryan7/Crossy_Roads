#include "Player.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Game.h"
#include "Utils.h"
using namespace glm;

#define FRAMES_PER_SECOND 60
#define JUMP_DURATION 11

void Player::firstInit() {
	soundManager = Game::instance().getSoundManager();
	jumpSound = soundManager->loadSound("sounds/Effect_jump.wav", FMOD_DEFAULT);
	waterSplashSound = soundManager->loadSound("sounds/Effect_water_splash.wav", FMOD_CREATESTREAM);
}

void Player::groupDrawableObjects(const FrustumG& frustum, RenderVectors& renderVectors) {
	if (playerObject.isInsideViewFrustum(frustum)) {
		outOfTheScene = false;
		if (state == PlayerState::DeadByEnemy) {
			renderVectors.texturedObjects[textureObject.texture].push_back(&textureObject);
		}
		else {
			renderVectors.objects[playerObject.meshId].push_back(&playerObject);
			renderVectors.shadowObjects[playerObject.meshId].push_back(&playerObject);
		}
	}
	else {
		renderVectors.shadowObjects[playerObject.meshId].push_back(&playerObject);
		outOfTheScene = true;
	}
}

void Player::init(const Assets& assets, vec3 lightDir, vec3 offset, float jumpDistance, Floor& floor, WaterParticleSystem* particleSystem) {
	GameObject::init();
	this->lightDir = lightDir;
	this->floor = &floor;
	this->particleSystem = particleSystem;
	this->jumpDistance = jumpDistance;
	this->assets = &assets;
	playerModels = assets.getFreeGroup(FreeMeshGroup::Player);

	currentPosScore = score = 0;

	currentRowIndex = floor.getRows() / 2 - floor.getRowOffset();
	currentColIndex = floor.getCols() / 2 - floor.getColOffset();
	vec3 rowHeight = floor.getFloorRow(currentRowIndex)->getNextPos(currentColIndex).first;

	IdMesh playerModelId = (*playerModels)[Game::instance().charSelected];
	playerObject.setMesh(playerModelId, assets.getMesh(playerModelId));
	playerObject.setScale(vec3(assets.getScale(playerModelId)));
	playerObject.setCenterToBaseCenter();
	playerObject.setPos(vec3(0,rowHeight.y,0));
	playerObject.setPlane(vec4(0, 1, 0, -rowHeight.y), lightDir);

	wPressed = aPressed = sPressed = dPressed = false;
	currentOrientation = FRONT;
	directionVector = vec3(0, 0, 1.f);
	inMovement = false;
	gravity = -0.1f;
	verticalSpeed = getJumpingSpeed(0,0,JUMP_DURATION);
	speed = this->jumpDistance / float(JUMP_DURATION);
	testJump = 0;
	currentFrame = 0;
	outOfTheScene = false;
	state = PlayerState::Alive;

	const Mesh* cubeMesh = assets.getCubeMesh();
	vec3 bbSize = cubeMesh->getbbSize();
	textureObject.setMesh(cubeMesh);
	textureObject.texture = assets.getTextureId("char_unlocked_6");
	textureObject.setScale(vec3(3, 0.1f, 2)/bbSize);
	textureObject.setRotationY(PI / 2);

	nextPos.y = playerObject.getY();
	godModePressed = false;
	godMode = false;
}

PlayerReturn Player::update(int deltaTime) {
	IdMesh playerModelId = (*playerModels)[Game::instance().charSelected];
	playerObject.setMesh(playerModelId, assets->getMesh(playerModelId));
	playerObject.setScale(vec3(assets->getScale(playerModelId)));
	playerObject.setCenterToBaseCenter();

	if (Game::instance().getKey('g')) {
		godModePressed = true;
	}
	else if (godModePressed) {
		godMode = !godMode;
		godModePressed = false;
	}

	if (outOfTheScene && !godMode) {
		platformSpeed = 0;
		state = PlayerState::DeadByOut;
		return PlayerReturn::DEAD;
	}
	currentColIndex = FloorRow::worldToCol(playerObject.getPos().x);
	PlayerReturn ret = PlayerReturn::NOTHING;
	if (collides() && !godMode) {
		particleSystem->trigger(playerObject.getPos(), 30, vec4(1, 0, 0, 0));
		textureObject.setPos(vec3(nextPos.x, nextPos.y+0.001f, nextPos.z));
		state = PlayerState::DeadByEnemy;
		return PlayerReturn::DEAD;
	}
	if (inMovement) {
		if (!keepMoving()) {
			currentFrame = 0;
			inMovement = false;
			currentColIndex = FloorRow::worldToCol(playerObject.getPos().x);
			FloorRow* floorRow = floor->getFloorRow(currentRowIndex);
			if (floorRow->getRowHeight() == playerObject.getY() && floorRow->isTheFloorLava() && !godMode) {
				particleSystem->trigger(playerObject.getPos(), 17, vec4(0,0.4f,0.86f,0));
				soundManager->playSound(waterSplashSound);
				state = PlayerState::DeadByLava;
				return PlayerReturn::DEAD;
			}
		}
	}
	else  {
		playerObject.moveX(platformSpeed);

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
				pair<vec3, float> currentPos = floor->getFloorRow(currentRowIndex)->getNextPos(currentColIndex);
				nextPos = currentPos.first;
				platformSpeed = currentPos.second;
				calculateSpeeds();
				soundManager->playSound(jumpSound);
				currentPosScore += 1;
				score = max(currentPosScore, score);
			}
			aPressed = dPressed = sPressed = false;
		}
		else if (Game::instance().getKey('a')) {
			if (!aPressed && !collidesWithEnv(currentRowIndex, currentColIndex+1)) {
				ret = PlayerReturn::MOVE_LEFT;
				aPressed = true;
				performRotation('a');
				currentOrientation = LEFT;
				setDirectionVector();
				inMovement = true;
				currentColIndex += 1;
				pair<vec3, float> currentPos = floor->getFloorRow(currentRowIndex)->getNextPos(currentColIndex);
				nextPos = currentPos.first;
				platformSpeed = currentPos.second;
				calculateSpeeds();
				soundManager->playSound(jumpSound);
			}
			wPressed = dPressed = sPressed = false;
		}
		else if (Game::instance().getKey('d')) {
			if (!dPressed && !collidesWithEnv(currentRowIndex, currentColIndex - 1)) {
				ret = PlayerReturn::MOVE_RIGHT;
				dPressed = true;
				performRotation('d');
				currentOrientation = RIGHT;
				setDirectionVector();
				inMovement = true;
				currentColIndex -= 1;
				pair<vec3, float> currentPos = floor->getFloorRow(currentRowIndex)->getNextPos(currentColIndex);
				nextPos = currentPos.first;
				platformSpeed = currentPos.second;
				calculateSpeeds();
				soundManager->playSound(jumpSound);
			}
			wPressed = aPressed = sPressed = false;
		}
		else if (Game::instance().getKey('s')) {
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
				pair<vec3, float> currentPos = floor->getFloorRow(currentRowIndex)->getNextPos(currentColIndex);
				nextPos = currentPos.first;
				platformSpeed = currentPos.second;
				calculateSpeeds();
				soundManager->playSound(jumpSound);
				currentPosScore -= 1;
			}
			wPressed = dPressed = aPressed = false;
		}
		else
			wPressed = aPressed = dPressed = sPressed = false;
	}
	return ret;
}

vec3 Player::getPos() const {
	return playerObject.getPos();
}

float Player::getHeight() const {
	return nextPos.y;
}

const Object * Player::getObject() const
{
	return &playerObject;
}

void Player::calculateSpeeds() {
	vec3 playerPos = playerObject.getPos();
	verticalSpeed = getJumpingSpeed(playerPos.y, nextPos.y, JUMP_DURATION);
	speeds.x = (nextPos.x - playerPos.x) / JUMP_DURATION;
	speeds.z = (nextPos.z - playerPos.z) / JUMP_DURATION;
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
	currentFrame++;
	bool returnValue;

	if (currentFrame == JUMP_DURATION) {
		playerObject.setPos(nextPos);
		returnValue = false;
	}
	else {
		returnValue = true;
		speeds.y = verticalSpeed + gravity*currentFrame;
		playerObject.move(speeds);
	}
	return returnValue;
}

float Player::getJumpingSpeed(float y0, float y, uint frames) {
	return (y-y0)/frames -0.5f*gravity*frames;
}

bool Player::collides() {
	bool collision = false;
	FloorRow* currentRow = floor->getFloorRow(currentRowIndex);
	vector<Jumper>* rowEnemies = currentRow->getEnemies();
	for (uint i = 0; i < rowEnemies->size() && !collision; ++i) {
		collision = playerObject.collidesWith((*rowEnemies)[i]);
	}
	return collision;
}

bool Player::collidesWithEnv(uint row, uint col) {
	FloorRow* rowToCheck = floor->getFloorRow(row);
	if (!rowToCheck->isSafeZone() || rowToCheck->getBiome() == Sea)
		return false;
	vector<CellProperties>* rowObjects = rowToCheck->getRowObjects();
	return (*rowObjects)[col].collision;
}

int Player::getScore() {
	return score;
}

Player::Player(){
}

Player::~Player()
{
}
