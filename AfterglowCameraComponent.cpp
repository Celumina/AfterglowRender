#include "AfterglowCameraComponent.h"
#include "AfterglowTransformComponent.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include "AfterglowUtilities.h"

AfterglowCameraComponent::AfterglowCameraComponent() :
	_aspectRatio(1.0f), 
	_near(0.1f), 
	_far(100'000.0f), 
	_fovY(static_cast<float>(45_deg)), 
	_view(), 
	_perspective(), 
	_invView(), 
	_invPerspective() {
}

float AfterglowCameraComponent::aspectRatio() const noexcept {
	return _aspectRatio;
}

float AfterglowCameraComponent::near() const noexcept {
	return _near;
}

float AfterglowCameraComponent::far() const noexcept {
	return _far;
}

float AfterglowCameraComponent::fovY() const noexcept {
	return _fovY;
}

void AfterglowCameraComponent::setAspectRatio(float aspectRatio) noexcept {
	_aspectRatio = aspectRatio;
}

void AfterglowCameraComponent::setNear(float near) noexcept {
	if (near < _far) {
		_near = near;
	}
}

void AfterglowCameraComponent::setFar(float far) noexcept {
	if (far > _near) {
		_far = far;
	}
}

void AfterglowCameraComponent::setFovY(float rad) noexcept {
	_fovY = rad;
}

void AfterglowCameraComponent::setGlobalTarget(const AfterglowTranslation& globalTarget) noexcept {
	auto& transform = entity().get<AfterglowTransformComponent>();
	auto direction = glm::normalize(globalTarget - transform.globalTranslation());
	transform.setGlobalRotation(glm::quatLookAt(direction, cardinal::Up));
}

void AfterglowCameraComponent::setTarget(const AfterglowTranslation& target) noexcept {
	auto& transform = entity().get<AfterglowTransformComponent>();
	auto direction = glm::normalize(target - transform.translation());
	transform.setRotation(glm::quatLookAt(direction, cardinal::Up));
}

const glm::mat4& AfterglowCameraComponent::view() const noexcept {
	return _view;
}

const glm::mat4& AfterglowCameraComponent::perspective() const noexcept {
	return _perspective;
}

const glm::mat4& AfterglowCameraComponent::invView() const noexcept {
	return _invView;
}

const glm::mat4& AfterglowCameraComponent::invPerspective() const noexcept {
	return _invPerspective;
}

void AfterglowCameraComponent::updateMatrices() noexcept {
	// View
	auto& transform = entity().get<AfterglowTransformComponent>();
	_view = glm::inverse(glm::translate(glm::mat4(1.0f), transform.globalTranslation()) * glm::mat4(transform.globalRotation()));

	// Perspective
	_perspective = glm::perspective(_fovY, _aspectRatio, _near, _far);
	_perspective[1][1] *= -1;  // Reverse Y

	// InvView
	_invView = glm::inverse(_view);

	// InvPerspective
	_invPerspective = glm::inverse(_perspective);
}

