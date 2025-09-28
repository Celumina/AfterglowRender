#include "ActionComponentLibrary.h"

#include <random>
#include <limits>

#include "AfterglowTransformComponent.h"
#include "DebugUtilities.h"
#include "LocalClock.h"

#include "AfterglowStaticMeshComponent.h"

void acl::EntityRotator::update() {
	auto& transform = *entity().component<AfterglowTransformComponent>();
	float angularSpeed = 1.0f;
	float deltaTime = static_cast<float>(sysUtils().clock().deltaTimeSec());
	auto deltaRotation = glm::angleAxis(angularSpeed * deltaTime, glm::vec3(0.0f, 0.0f, 1.0f));
	transform.setRotation(glm::normalize(deltaRotation * transform.rotation()));
}


void acl::SimpleController::update() {
	auto& input = sysUtils().input();
	auto& transform = *entity().component<AfterglowTransformComponent>();
	glm::vec2 deltaCursorPos = input.cursorPosition() - _cursorPosLastUpdate;
	float deltaTime = static_cast<float>(sysUtils().clock().deltaTimeSec());

	float moveSpeed = 200.0f ;
	float rotateSpeed = 0.002f;

	AfterglowDirection forward = -transform.globalDirection();
	AfterglowDirection right = glm::cross(forward, cardinal::Up);
	
	AfterglowTranslation targetMoveVelocity{};
	// TODO: apply to target spped for smoothly move.
	if (input.pressed(AfterglowInput::Key::W)) {
		targetMoveVelocity += forward * moveSpeed;
	}
	if (input.pressed(AfterglowInput::Key::S)) {
		targetMoveVelocity -= forward * moveSpeed;
	}
	if (input.pressed(AfterglowInput::Key::A)) {
		targetMoveVelocity -= right * moveSpeed;
	}
	if (input.pressed(AfterglowInput::Key::D)) {
		targetMoveVelocity += right * moveSpeed;
	}
	if (input.pressed(AfterglowInput::Key::Q)) {
		targetMoveVelocity += cardinal::Down * moveSpeed;
	}
	if (input.pressed(AfterglowInput::Key::E)) {
		targetMoveVelocity += cardinal::Up * moveSpeed;
	}

	if (input.pressed(AfterglowInput::Key::LeftShift)) {
		targetMoveVelocity *= 8.0f;
	}

	_currentMoveVelocity = util::Lerp(_currentMoveVelocity, targetMoveVelocity, std::clamp(8.0f * deltaTime, 0.0f, 1.0f));
	transform.setGlobalTranslation(transform.globalTranslation() + _currentMoveVelocity * deltaTime);

	if (input.pressed(AfterglowInput::MouseButton::Left) || _fpvMode) {
		// Here had use delta curosor pos ,  so don't require to multiply deltaTime any more.
		// TODO: relative deltaCursorPos for different window size.
		transform.setGlobalEuler(transform.globalEuler() + AfterglowEuler{ deltaCursorPos.y * -rotateSpeed , 0.0f, deltaCursorPos.x * -rotateSpeed });
	}

	// Create Entity Test
	if (input.pressDown(AfterglowInput::Key::F)) {
		DEBUG_COST_INFO_BEGIN("Create Boxes");
		static std::random_device randomDevice;  
		static std::default_random_engine randomEngine(randomDevice());
		static std::uniform_int_distribution<int> randomRange(-1000, 1000);

		for (uint32_t index = 0; index < 32; ++index) {
			AfterglowTranslation randOffset = { randomRange(randomEngine), randomRange(randomEngine),  randomRange(randomEngine) };
			randOffset.z += 1000.0f;
			auto& box = sysUtils().createEntity(
				"Box", 
				{ util::TypeIndex<AfterglowStaticMeshComponent>(), util::TypeIndex<acl::EntityRotator>()}
			);
			// std::cout << &box << "\n\n";
			auto& boxTransform = box.get<AfterglowTransformComponent>();
			boxTransform.setGlobalTranslation(AfterglowTranslation{ -200, -200, 100 } + randOffset);
			boxTransform.setGlobalScaling({ 40, 40, 40 });
			auto& boxMesh = box.get<AfterglowStaticMeshComponent>();
			// TODO: Load exist model from memory.
			boxMesh.setModel("Assets/Shared/Models/Box.fbx");
		}
		DEBUG_COST_INFO_END;
	}

	if (input.pressDown(AfterglowInput::Key::G)) {
		sysUtils().unregisterMaterialInstanceAsset(
			"Assets/Characters/BattleMage/MaterialInstances/BattleMageWeapon.mati"
		);
	}

	if (input.pressDown(AfterglowInput::Key::Tab)) {
		_fpvMode = !_fpvMode;
		if (_fpvMode) {
			sysUtils().lockCursor();
		}
		else {
			sysUtils().unlockCursor();
		}
	}

	// DEBUG_INFO(std::to_string(input.cursorEntered()));
	_cursorPosLastUpdate = input.cursorPosition();
}


void acl::ParticleSpawner::awake() {
	std::default_random_engine randomEngine(time(nullptr));
	std::uniform_real_distribution<float> randomDistribution(0.0f, 1.0f);

	_particles.resize(512);

	for (auto& particle : _particles) {
		float radius = 0.25f * sqrt(randomDistribution(randomEngine));
		float theta = randomDistribution(randomEngine) * 2.0 * constant::pi;
		particle.position.x = radius * cos(theta) * 100.0f;
		particle.position.y = radius * sin(theta) * 100.0f;
		particle.position.z = radius * 100.0f;

		particle.velocity = glm::normalize(particle.position) * 0.00025f;
		particle.color = glm::vec4(
			randomDistribution(randomEngine),
			randomDistribution(randomEngine),
			randomDistribution(randomEngine),
			1.0f
		);
	}

}

void acl::ParticleSpawner::update() {
}
