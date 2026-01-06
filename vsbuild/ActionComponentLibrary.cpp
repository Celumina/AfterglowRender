#include "ActionComponentLibrary.h"

#include <random>
#include <limits>

#include "AfterglowTransformComponent.h"
#include "DebugUtilities.h"
#include "AfterglowInput.h"

#include "AfterglowCameraComponent.h"
#include "AfterglowStaticMeshComponent.h"

void acl::EntityRotator::update() {
	auto& transform = *entity().component<AfterglowTransformComponent>();
	float deltaTime = static_cast<float>(sysUtils().deltaTimeSec());
	auto deltaRotation = glm::angleAxis(_angularSpeed * deltaTime, glm::vec3(0.0f, 0.0f, 1.0f));
	transform.setRotation(glm::normalize(deltaRotation * transform.rotation()));
}

void acl::SimpleController::awake() {
	if (sysUtils().mainCamera()) {
		_fov = sysUtils().mainCamera()->fovY();
	}
}

void acl::SimpleController::update() {
	auto& input = sysUtils().input();
	auto& transform = *entity().component<AfterglowTransformComponent>();
	glm::vec2 deltaCursorPos = input.cursorPosition() - _cursorPosLastUpdate;
	float deltaTime = float(sysUtils().deltaTimeSec());

	AfterglowDirection forward = -transform.globalDirection();
	AfterglowDirection right = glm::cross(forward, cardinal::Up);
	
	AfterglowTranslation targetMoveVelocity{};

	// Update movement.
	if (input.pressed(AfterglowInput::Key::W)) {
		targetMoveVelocity += forward * _movementSpeed;
	}
	if (input.pressed(AfterglowInput::Key::S)) {
		targetMoveVelocity -= forward * _movementSpeed;
	}
	if (input.pressed(AfterglowInput::Key::A)) {
		targetMoveVelocity -= right * _movementSpeed;
	}
	if (input.pressed(AfterglowInput::Key::D)) {
		targetMoveVelocity += right * _movementSpeed;
	}
	if (input.pressed(AfterglowInput::Key::Q)) {
		targetMoveVelocity += cardinal::Down * _movementSpeed;
	}
	if (input.pressed(AfterglowInput::Key::E)) {
		targetMoveVelocity += cardinal::Up * _movementSpeed;
	}

	if (input.pressed(AfterglowInput::Key::LeftShift)) {
		targetMoveVelocity *= 8.0f;
	}

	if (input.pressDown(AfterglowInput::Key::Minus, AfterglowInput::Modifier::Control)
		&& _movementSpeed > _movementSpeedMin) {
		_movementSpeed *= 0.5;
	}
	if (input.pressDown(AfterglowInput::Key::Equal, AfterglowInput::AfterglowInput::Modifier::Control)
		&& _movementSpeed < _movementSpeedMax) {
		_movementSpeed *= 2.0;
	}

	// Update zooming.
	if (input.pressed(AfterglowInput::Key::Equal)) {
		_currentZoomingSpeed = (_fov > _fovMin)
			? util::Lerp(_currentZoomingSpeed, -_zoomingSpeed * deltaTime, 0.5f * deltaTime) : 0.0f;
	}
	else if (input.pressed(AfterglowInput::Key::Minus)) {
		_currentZoomingSpeed = (_fov < _fovMax)
			? util::Lerp(_currentZoomingSpeed, _zoomingSpeed * deltaTime, 0.5f * deltaTime) : 0.0f;
	}
	else {
		_currentZoomingSpeed = util::Lerp(_currentZoomingSpeed, 0.0f, 4.0f * deltaTime);
	}

	if (_currentZoomingSpeed != 0.0) {
		_fov = std::clamp(_fov + _currentZoomingSpeed, _fovMin, _fovMax);
		if (sysUtils().mainCamera()) {
			sysUtils().mainCamera()->setFovY(_fov);
		}
	}

	// Apply movement.
	_currentMoveVelocity = util::Lerp(_currentMoveVelocity, targetMoveVelocity, std::clamp(4.0f * deltaTime, 0.0f, 1.0f));
	transform.setGlobalTranslation(transform.globalTranslation() + _currentMoveVelocity * deltaTime);

	// Apply rotation.
	if (input.pressed(AfterglowInput::MouseButton::Left) || _fpvMode) {
		// Here had use delta curosor pos ,  so don't require to multiply deltaTime any more.
		// TODO: relative deltaCursorPos for different window size.
		float zoomedRotationSpeed = -_rotationSpeed * std::min(_fov, float(45_deg));
		transform.setGlobalEuler(transform.globalEuler() + AfterglowEuler{ deltaCursorPos.y * zoomedRotationSpeed, 0.0f, deltaCursorPos.x * zoomedRotationSpeed });
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

	//if (input.wheelOffset() != glm::dvec2(0.0)) {
	//	DEBUG_INFO("Move!!!!!!!!!!");
	//}
}

void acl::InteractiveTest::awake() {
	debugViewMaterialName = sysUtils().registerMaterialAsset("Assets/Shared/Materials/DebugView.mat");
}

void acl::InteractiveTest::update() {
	auto& input = sysUtils().input();
	// Create Entity Test
	if (input.pressDown(AfterglowInput::Key::F, AfterglowInput::Modifier::Control)) {
		DEBUG_COST_BEGIN("Create Boxes");
		static std::random_device randomDevice;
		static std::default_random_engine randomEngine(randomDevice());
		static std::uniform_real_distribution<float> randomRange(-10.0, 10.0);

		for (uint32_t index = 0; index < 32; ++index) {
			AfterglowTranslation randOffset = { randomRange(randomEngine), randomRange(randomEngine),  randomRange(randomEngine) };
			randOffset.z += 10.0f;
			auto& box = sysUtils().createEntity(
				"Box",
				{ util::TypeIndex<AfterglowStaticMeshComponent>(), util::TypeIndex<acl::EntityRotator>() }, 
				entity()
			);
			// std::cout << &box << "\n\n";
			auto& boxTransform = box.get<AfterglowTransformComponent>();
			boxTransform.setGlobalTranslation(AfterglowTranslation{ -2.0f, -2.0f, 1.0f } + randOffset);
			boxTransform.setGlobalScaling({ 0.4f, 0.4f, 0.4f });
			auto& boxMesh = box.get<AfterglowStaticMeshComponent>();
			// TODO: Load exist model from memory.
			boxMesh.setModel("Assets/Shared/Models/Box.fbx");
		}
		DEBUG_COST_END;
	}

	if (input.pressDown(AfterglowInput::Key::G, AfterglowInput::Modifier::Control)) {
		sysUtils().unregisterMaterialAsset(
			"Assets/Shared/Materials/Standard.mat"
		);
		sysUtils().unregisterMaterialInstanceAsset(
			"Assets/Characters/BattleMage/MaterialInstances/BattleMageWeapon.mati"
		);
	}
	
	if (input.pressDown(AfterglowInput::Key::H, AfterglowInput::Modifier::Control | AfterglowInput::Modifier::Shift)) {
		sysUtils().unregisterMaterialAsset(
			"Assets/Shared/Materials/BoidInstancing.mat"
		);
		auto* boids = sysUtils().findEntity("Boids");
		if (boids) {
			sysUtils().destroyEntity(*boids);
		}
	}

	if (input.pressed(AfterglowInput::Key::T, AfterglowInput::Modifier::Control)) {
		auto* grasses = sysUtils().findEntity("GrassInstances");
		if (grasses) {
			auto& grassesTransform = grasses->get<AfterglowTransformComponent>();
			grassesTransform.setGlobalTranslation(util::Lerp(
				grassesTransform.globalTranslation(),
				sysUtils().mainCamera()->entity().get<AfterglowTransformComponent>().globalTranslation(),
				static_cast<float>(sysUtils().deltaTimeSec())
			));
		}
	}

	if (input.pressDown(AfterglowInput::Key::D, AfterglowInput::Modifier::Control)) {
		AfterglowTranslation translation{};
		AfterglowQuaternion rotation{};
		auto* camera = sysUtils().mainCamera();
		if (camera) {
			auto& cameraTransform = camera->entity().get<AfterglowTransformComponent>();
			// Here use cardinal::Down due to the camera space use z-axis to control the depth.
			translation = cameraTransform.globalTranslation() + (cameraTransform.globalRotation() * cardinal::Down) * AfterglowTranslation{5.0, 5.0, 5.0};
			rotation = cameraTransform.globalRotation();
		}
		auto& debugBox = sysUtils().createEntity("DebugBox", util::TypeIndex<AfterglowStaticMeshComponent>(), entity());
		auto& debugBoxTransform = debugBox.get<AfterglowTransformComponent>();
		debugBoxTransform.setGlobalTranslation(translation);
		debugBoxTransform.setGlobalRotation(rotation);
		debugBoxTransform.setGlobalScaling({1.0f, 1.0f, 1.0f});
		
		auto& debugBoxMesh = debugBox.get<AfterglowStaticMeshComponent>();
		debugBoxMesh.setMaterial(debugViewMaterialName);
		debugBoxMesh.setModel("Assets/Shared/Models/DebugBox.fbx");
	}
}
