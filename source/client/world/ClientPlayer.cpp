/*
 * =====================================================================================
 *
 *  OpenMiner
 *
 *  Copyright (C) 2018-2020 Unarelith, Quentin Bazin <openminer@unarelith.net>
 *  Copyright (C) 2019-2020 the OpenMiner contributors (see CONTRIBUTORS.md)
 *
 *  This file is part of OpenMiner.
 *
 *  OpenMiner is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  OpenMiner is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenMiner; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * =====================================================================================
 */
#include <glm/gtc/matrix_transform.hpp>

#include <gk/core/input/GamePad.hpp>
#include <gk/core/Mouse.hpp>

#include "ClientPlayer.hpp"
#include "ClientWorld.hpp"
#include "GameConfig.hpp"
#include "GameKey.hpp"
#include "Registry.hpp"

ClientPlayer *ClientPlayer::s_instance = nullptr;

ClientPlayer::ClientPlayer(gk::Camera &camera) : m_camera(camera) {
	m_cameraLocalPos = gk::Vector3f{0.f, 0.f, 1.625f};

	updateCamera();

	m_camera.setDPosition(m_x + m_cameraLocalPos.x, m_y + m_cameraLocalPos.y, m_z + m_cameraLocalPos.z);
}

void ClientPlayer::turnH(float angle) {
	m_viewAngleH = fmodf(m_viewAngleH + angle, 360.f);
	if (m_viewAngleH >= 180.f) m_viewAngleH -= 360.f;
	if (m_viewAngleH < -180.f) m_viewAngleH += 360.f;

	updateCamera();
}

void ClientPlayer::turnViewV(float angle) {
	m_viewAngleV = std::max(std::min(m_viewAngleV + angle, 90.f), -90.f);

	updateCamera();
}

void ClientPlayer::updateCamera() {
	float ch = cosf(m_viewAngleH * RADIANS_PER_DEGREES);
	float sh = sinf(m_viewAngleH * RADIANS_PER_DEGREES);
	float cv = cosf(m_viewAngleV * RADIANS_PER_DEGREES);
	float sv = sinf(m_viewAngleV * RADIANS_PER_DEGREES);
	float cr = cosf(m_viewAngleRoll * RADIANS_PER_DEGREES);
	float sr = sinf(m_viewAngleRoll * RADIANS_PER_DEGREES);

	m_forwardDir = gk::Vector3f{ch * cv, sh * cv, sv};
	m_camera.setDirection(m_forwardDir);
	m_camera.setUpVector(gk::Vector3f{sh * sr - ch * sv * cr, -ch * sr - sh * sv * cr, cv * cr});
}

void ClientPlayer::move(float direction) {
	direction += m_viewAngleH;
	inertiaBuffer.x += 0.001 * cosf(direction * RADIANS_PER_DEGREES);
	inertiaBuffer.y += 0.001 * sinf(direction * RADIANS_PER_DEGREES);

	glm::dvec2 test = glm::dvec2(inertiaBuffer.x, inertiaBuffer.y);
	double testSpeed = glm::length(test);

	//limit speed
	if (testSpeed > maxSpeed){

		test = glm::normalize(test);
    	test *= (maxSpeed + testSpeed) / 20;

		//smooth transition
		inertiaBuffer.x -= test.x;
		inertiaBuffer.y -= test.y;
	}
}

void ClientPlayer::applyFriction(){
	inertiaBuffer.x /= 1.05;
	inertiaBuffer.y /= 1.05;
}

void ClientPlayer::applyVelocity(){
	m_velocity.x = inertiaBuffer.x;
	m_velocity.y = inertiaBuffer.y;
}



void ClientPlayer::processInputs() {


	if(gk::GamePad::isKeyPressed(GameKey::Jump) && !m_isJumping) {
		m_isJumping = true;
		m_velocity.z = m_jumpSpeed;
	}

	if (gk::GamePad::isKeyPressed(GameKey::Sprint)) {
		maxSpeed = 0.05;
	}
	//sneak overrides sprint
	else if(gk::GamePad::isKeyPressed(GameKey::Sneak)) {
		maxSpeed = 0.01;
	} else {
		maxSpeed = 0.03;
	}

	bool hasPressed = false;

	if(gk::GamePad::isKeyPressed(GameKey::Forward)){
		move(0.0f);
		hasPressed = true;
	}

	if(gk::GamePad::isKeyPressed(GameKey::Back)){
		move(180.0f);
		hasPressed = true;
	}

	if(gk::GamePad::isKeyPressed(GameKey::Left)){
		move(90.0f);
		hasPressed = true;
	}

	if(gk::GamePad::isKeyPressed(GameKey::Right)){
		move(-90.0f);
		hasPressed = true;
	}

	applyVelocity();

	if (!hasPressed){
		applyFriction();		
	}
}

void ClientPlayer::updatePosition(const ClientWorld &world) {

	ClientChunk *chunk = (ClientChunk *)world.getChunkAtBlockPos(m_x, m_y, m_z);
	if (chunk && chunk->isInitialized()) {
		if (!Config::isFlyModeEnabled) {
			m_velocity.z -= chunk->dimension().gravity() * 0.001f;

			m_isJumping = true;

			if (m_velocity.z < -m_jumpSpeed) // Limit max vertical speed to jump speed
				m_velocity.z = -m_jumpSpeed;
		}
	}
	else if (!Config::isNoClipEnabled) {
		// Block player until the chunk loads, unless "no clip" mode is enabled
		m_velocity = glm::vec3{0.f, 0.f, 0.f};
	}

	if (!Config::isNoClipEnabled)
		checkCollisions(world);

	if (!Config::isFlyModeEnabled && m_velocity.z != 0.f) {
		m_velocity.x *= 0.75f;
		m_velocity.y *= 0.75f;
	}

	setPosition(m_x + m_velocity.x, m_y + m_velocity.y, m_z + m_velocity.z);

	m_velocity.x = 0.f;
	m_velocity.y = 0.f;

	if (Config::isFlyModeEnabled)
		m_velocity.z = 0.f;

	// Checking to block at camera position to enable specific effects
	const BlockState *blockState = world.getBlockState(m_camera.getDPosition().x, m_camera.getDPosition().y, m_camera.getDPosition().z);
	if (blockState && blockState->fogDepth() != 0) {
		GameConfig::currentScreenEffect = 1;
		GameConfig::fogDepth = blockState->fogDepth();
		GameConfig::fogColor = blockState->fogColor();
	}
	else {
		GameConfig::currentScreenEffect = 0;
	}
}

void ClientPlayer::setPosition(double x, double y, double z) {
	m_x = x;
	m_y = y;
	m_z = z;

	Player::setPosition(x, y, z);

	gk::Vector3f camPos = m_cameraLocalPos;
	m_camera.setDPosition(m_x + camPos.x, m_y + camPos.y, m_z + camPos.z);
}

void ClientPlayer::checkCollisions(const ClientWorld &world) {
	gk::Vector3d corner{m_x + m_hitbox.x, m_y + m_hitbox.y, m_z + m_hitbox.z};

	constexpr int numPointsPerEdge = 3;
	constexpr int lastPoint = numPointsPerEdge - 1;

	for (int z = 0; z <= lastPoint; z++) {
		for (int y = 0; y <= lastPoint; y++) {
			for (int x = 0; x <= lastPoint; x++) {
				if (x == 0 || x == lastPoint || y == 0 || y == lastPoint || z == 0 || z == lastPoint)
					testPoint(world, corner.x + m_hitbox.sizeX * ((double)x / lastPoint),
					          corner.y + m_hitbox.sizeY * ((double)y / lastPoint),
					          corner.z + m_hitbox.sizeZ * ((double)z / lastPoint), m_velocity);
			}
		}
	}
}

bool passable(const ClientWorld &world, double x, double y, double z) {
	const BlockState *blockState = world.getBlockState(floor(x), floor(y), floor(z));
	return !blockState || !blockState->block().id() || !blockState->isCollidable();
}

void ClientPlayer::testPoint(const ClientWorld &world, double x, double y, double z, glm::vec3 &vel) {
	if(!passable(world, x + vel.x, y, z)) vel.x = 0.f;
	if(!passable(world, x, y + vel.y, z)) vel.y = 0.f;
	if(!passable(world, x, y, z + vel.z)) {
		if(vel.z < 0.f && m_isJumping) m_isJumping = false;
		vel.z = 0.f;
	}
}

