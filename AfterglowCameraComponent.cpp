#include "AfterglowCameraComponent.h"
#include "AfterglowTransformComponent.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include "AfterglowUtilities.h"

AfterglowCameraComponent::AfterglowCameraComponent() :
	_aspectRatio(1.0f), 
	_near(0.1f), 
	_far(100'000.0f), 
	_fovY(static_cast<float>(45_deg)) {
}

float AfterglowCameraComponent::aspectRatio() const {
	return _aspectRatio;
}

float AfterglowCameraComponent::near() const {
	return _near;
}

float AfterglowCameraComponent::far() const {
	return _far;
}

float AfterglowCameraComponent::fovY() const {
	return _fovY;
}

void AfterglowCameraComponent::setAspectRatio(float aspectRatio) {
	_aspectRatio = aspectRatio;
}

void AfterglowCameraComponent::setNear(float near) {
	if (near < _far) {
		_near = near;
	}
}

void AfterglowCameraComponent::setFar(float far) {
	if (far > _near) {
		_far = far;
	}
}

void AfterglowCameraComponent::setFovY(float rad) {
	_fovY = rad;
}

void AfterglowCameraComponent::setGlobalTarget(const AfterglowTranslation& globalTarget) {
	auto& transform = entity().get<AfterglowTransformComponent>();
	auto direction = glm::normalize(globalTarget - transform.globalTranslation());
	transform.setGlobalRotation(glm::quatLookAt(direction, cardinal::Up));
}

void AfterglowCameraComponent::setTarget(const AfterglowTranslation& target) {
	auto& transform = entity().get<AfterglowTransformComponent>();
	auto direction = glm::normalize(target - transform.translation());
	transform.setRotation(glm::quatLookAt(direction, cardinal::Up));
}

glm::mat4 AfterglowCameraComponent::view() const {
	auto& transform = entity().get<AfterglowTransformComponent>();
	return glm::inverse(glm::translate(glm::mat4(1.0f), transform.globalTranslation()) * glm::mat4(transform.globalRotation()));
}

glm::mat4 AfterglowCameraComponent::perspective() const {
	auto mat = glm::perspective(_fovY, _aspectRatio, _near, _far);
	mat[1][1] *= -1;
	return mat;
}

