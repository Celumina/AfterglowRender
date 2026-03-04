#include "ActionComponentLibrary.h"

#include <random>
#include <limits>

#include "AfterglowTransformComponent.h"
#include "DebugUtilities.h"
#include "AfterglowInput.h"
#include "AfterglowMaterialInstance.h"
#include "AfterglowCameraComponent.h"
#include "AfterglowStaticMeshComponent.h"
#include "AfterglowComputeComponent.h"

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

	AfterglowDirection cameraForward = -transform.globalViewDirection();
	AfterglowDirection horizontalForward { cameraForward.x, cameraForward.y, 0.0f };
	// @note: Normalize to avoid length issues (critical if forward was nearly vertical)
	if (glm::length(horizontalForward) > 1.0e-4f) {
		horizontalForward = glm::normalize(horizontalForward);
	}
	//else {
	//	// Fallback: if camera is straight up/down, use world forward (e.g., -Z)
	//	horizontalForward = cardinal::Forward;
	//}

	// Calculate camera right using the STABILIZED horizontal forward vector
	AfterglowDirection cameraRight = glm::normalize(glm::cross(horizontalForward, cardinal::Up));
	//AfterglowDirection cameraRight = glm::normalize(glm::cross(cameraForward, cardinal::Up));
	
	AfterglowTranslation targetMoveVelocity{};

	// Update movement.
	if (input.pressed(input::Key::W)) {
		targetMoveVelocity += cameraForward * _movementSpeed;
	}
	if (input.pressed(input::Key::S)) {
		targetMoveVelocity -= cameraForward * _movementSpeed;
	}
	if (input.pressed(input::Key::A)) {
		targetMoveVelocity -= cameraRight * _movementSpeed;
	}
	if (input.pressed(input::Key::D)) {
		targetMoveVelocity += cameraRight * _movementSpeed;
	}
	if (input.pressed(input::Key::Q)) {
		targetMoveVelocity += cardinal::Down * _movementSpeed;
	}
	if (input.pressed(input::Key::E)) {
		targetMoveVelocity += cardinal::Up * _movementSpeed;
	}

	if (input.pressed(input::Key::LeftShift)) {
		targetMoveVelocity *= 8.0f;
	}

	if (input.pressDown(input::Key::Minus, input::Modifier::Control)
		&& _movementSpeed > _movementSpeedMin) {
		_movementSpeed *= 0.5;
	}
	if (input.pressDown(input::Key::Equal, input::Modifier::Control)
		&& _movementSpeed < _movementSpeedMax) {
		_movementSpeed *= 2.0;
	}

	// Update zooming.
	if (input.pressed(input::Key::Equal)) {
		_currentZoomingSpeed = (_fov > _fovMin)
			? util::Lerp(_currentZoomingSpeed, -_zoomingSpeed * deltaTime, 0.5f * deltaTime) : 0.0f;
	}
	else if (input.pressed(input::Key::Minus)) {
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
	_currentMoveVelocity = util::Lerp(
		_currentMoveVelocity, 
		targetMoveVelocity, 
		std::clamp(_movementAcceleration * deltaTime, 0.0f, 1.0f)
	);
	transform.setGlobalTranslation(transform.globalTranslation() + _currentMoveVelocity * deltaTime);

	glm::vec2 targetRotationVelocity{ 0.0f, 0.0f };
	if (input.pressed(input::MouseButton::Left) || _fpvMode) {
		// FOV relavant rotation.
		// @note: Here we divide deltaTime first to solve this issue:
		//		"Very fast when the culling happend and sysThread is high framerate.
		// We apply deltaTime for velocity at the final setGlobalEuler(..)
		targetRotationVelocity = (deltaCursorPos / deltaTime) * (-_rotationSpeed * std::min(_fov, float(45_deg)));
	}
	_currentRotationVelocity = util::Lerp(
		_currentRotationVelocity, 
		targetRotationVelocity, 
		std::clamp(_rotationAcceleration * deltaTime, 0.0f, 1.0f)
	);
	// Apply rotation.
	auto currentEuler = transform.globalEuler() + AfterglowEuler{ _currentRotationVelocity.y, 0.0f, _currentRotationVelocity.x } *deltaTime;
	currentEuler.x = glm::clamp(currentEuler.x, 1.0_degf, 179.0_degf);
	transform.setGlobalEuler(currentEuler);

	if (input.pressDown(input::Key::Tab)) {
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
	if (input.pressDown(input::Key::F, input::Modifier::Control)) {
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

	if (input.pressDown(input::Key::G, input::Modifier::Control)) {
		sysUtils().unregisterMaterialAsset(
			"Assets/Shared/Materials/ArcToon.mat"
		);
		sysUtils().unregisterMaterialInstanceAsset(
			"Assets/Characters/BattleMage/Materials/BattleMageWeapon.mati"
		);
	}
	
	// Multiple modifiers and destroy entity test
	auto destoryModifier = input::Modifier::Control | input::Modifier::Shift | input::Modifier::Alt;
	destoryModifier &= input::Modifier::Shift | input::Modifier::Control;
	if (input.pressDown(input::Key::H, destoryModifier)) {
		sysUtils().unregisterMaterialAsset(
			"Assets/Shared/Materials/BoidInstancing.mat"
		);
		auto* boids = sysUtils().findEntity("Boids");
		if (boids) {
			sysUtils().destroyEntity(*boids);
		}
	}

	// @deprecated
	//if (input.pressed(input::Key::T, input::Modifier::Control)) {
	//	auto* grasses = sysUtils().findEntity("GrassInstances");
	//	if (grasses) {
	//		auto& grassesTransform = grasses->get<AfterglowTransformComponent>();
	//		grassesTransform.setGlobalTranslation(util::Lerp(
	//			grassesTransform.globalTranslation(),
	//			sysUtils().mainCamera()->entity().get<AfterglowTransformComponent>().globalTranslation(),
	//			static_cast<float>(sysUtils().deltaTimeSec())
	//		));
	//	}
	//}

	if (input.pressDown(input::Key::D, input::Modifier::Control)) {
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

void acl::MaterialObjectStateParamUpdater::bindMaterial(const std::string& name) {
	if (name != _materialName) {
		_materialInstanceNames.clear();
		_materialName = name;
		DEBUG_CLASS_INFO(std::format("Binding Material was changed, new material name:{}", _materialName));
	}
	initializeMaterialParams();
}

void acl::MaterialObjectStateParamUpdater::bindMaterialInstance(const std::string& name) {
	_materialInstanceNames.push_back(name);
}

void acl::MaterialObjectStateParamUpdater::update() {
	auto& transformComponent = entity().get<AfterglowTransformComponent>();
	for (auto& name : _materialInstanceNames) {
		auto* materialInstance = sysUtils().materialInstance(name);
		if (!materialInstance) {
			DEBUG_CLASS_WARNING(std::format("Material instance not found: {}", name));
			continue;
		}
		materialInstance->setVector(shader::Stage::Shared, "objectForward", AfterglowMaterial::Vector(transformComponent.globalForward(), 0.0f));
		materialInstance->setVector(shader::Stage::Shared, "objectRight", AfterglowMaterial::Vector(transformComponent.globalRight(), 0.0f));
		materialInstance->setVector(shader::Stage::Shared, "objectUp", AfterglowMaterial::Vector(transformComponent.globalUp(), 0.0f));
		// TODO: Something happend if apply UniformParams only?
		sysUtils().submitMaterialInstanceUniformParams(name);
		//sysUtils().submitMaterialInstance(name);
	}
}

void acl::MaterialObjectStateParamUpdater::initializeMaterialParams() {
	auto* material = sysUtils().material(_materialName);
	if (!material) {
		DEBUG_CLASS_ERROR("Material not found, make sure material was created before binding.");
		return;
	}
	// We don't need to write these declarations to asset file manually, thanks to material shader reload asset from memory.
	material->setVector(shader::Stage::Shared, "objectForward", {});
	material->setVector(shader::Stage::Shared, "objectRight", {});
	material->setVector(shader::Stage::Shared, "objectUp", {});
	sysUtils().submitMaterial(_materialName);
}

void acl::GreedySnakeSpawner::awake() {
	_greedySnakeMaterialName = sysUtils().registerMaterialAsset("Assets/Shared/Materials/GreedySnake.mat");
}

void acl::GreedySnakeSpawner::update() {
	auto& input = sysUtils().input();
	if (input.pressDown(input::Key::Num1, input::Modifier::Shift)) {
		auto& greedySnakeEntity = sysUtils().createEntity(
			"GreedySnake", 
			{ util::TypeIndex<AfterglowComputeComponent>(), util::TypeIndex<AfterglowStaticMeshComponent>() },
			entity()
		);
		auto* camera = sysUtils().mainCamera();
		if (camera) {
			auto& cameraTransform = camera->entity().get<AfterglowTransformComponent>();
			auto& greedySnakeTransform = greedySnakeEntity.get<AfterglowTransformComponent>();
			greedySnakeTransform.setGlobalTranslation(cameraTransform.globalTranslation() + AfterglowTranslation{ 0.0f, 0.0f, -2.0f });
			greedySnakeTransform.setGlobalEuler({ 0.0_deg, 0.0_deg, cameraTransform.globalEuler().z });
		}
		greedySnakeEntity.get<AfterglowComputeComponent>().setComputeMaterial(_greedySnakeMaterialName);
		auto& greedySnakeBoxMesh = greedySnakeEntity.get<AfterglowStaticMeshComponent>();
		greedySnakeBoxMesh.setModel("Assets/Shared/Models/Box.fbx");
		greedySnakeBoxMesh.setMaterial(_greedySnakeMaterialName);
	}

	if (input.pressDown(input::Key::Escape)) {
		setMaterialInputParam(static_cast<float>(InputFlag::Reset));
	}
	else if (input.pressDown(input::Key::Up)) {
		setMaterialInputParam(static_cast<float>(InputFlag::Up));
	}
	else if (input.pressDown(input::Key::Down)) {
		setMaterialInputParam(static_cast<float>(InputFlag::Down));
	}
	else if (input.pressDown(input::Key::Right)) {
		setMaterialInputParam(static_cast<float>(InputFlag::Right));
	}
	else if (input.pressDown(input::Key::Left)) {
		setMaterialInputParam(static_cast<float>(InputFlag::Left));
	}
}

void acl::GreedySnakeSpawner::setMaterialInputParam(float value) const {
	auto* matInst = sysUtils().materialInstance(_greedySnakeMaterialName);
	if (matInst) {
		matInst->setScalar(shader::Stage::Compute, "input", value);
		sysUtils().submitMaterialInstance(_greedySnakeMaterialName);
	}
}
