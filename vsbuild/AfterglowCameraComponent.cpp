#include "AfterglowCameraComponent.h"
#include "AfterglowTransformComponent.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include "AfterglowUtilities.h"
#include "RenderConfigurations.h"

AfterglowCameraComponent::AfterglowCameraComponent() :
	_aspectRatio(1.0f), 
	_near(0.1f), 
	_far(1'000.0f), 
	_fovY(static_cast<float>(45_deg)) {
}

AfterglowCameraComponent::AfterglowCameraComponent(const AfterglowCameraComponent& other) : 
	AfterglowComponent(other), 
	_aspectRatio(other._aspectRatio), 
	_near(other._near), 
	_far(other._far), 
	_fovY(other._fovY), 
	_view(other._view.load()), 
	_perspective(other._perspective.load()), 
	_invView(other._invView.load()), 
	_invPerspective(other._invPerspective.load()) {

}

AfterglowCameraComponent& AfterglowCameraComponent::operator=(const AfterglowCameraComponent& other) {
	AfterglowComponent::operator=(other);
	_aspectRatio = other._aspectRatio;
	_near = other._near;
	_far = other._far;
	_fovY = other._fovY;
	_view = other._view.load();
	_perspective = other._perspective.load();
	_invView = other._invView.load();
	_invPerspective = other._invPerspective.load();
	return *this;
}

AfterglowCameraComponent::AfterglowCameraComponent(AfterglowCameraComponent&& rval) noexcept : 
	AfterglowComponent(rval),
	_aspectRatio(rval._aspectRatio),
	_near(rval._near),
	_far(rval._far),
	_fovY(rval._fovY),
	_view(rval._view.load()),
	_perspective(rval._perspective.load()),
	_invView(rval._invView.load()),
	_invPerspective(rval._invPerspective.load()) {
}

AfterglowCameraComponent& AfterglowCameraComponent::operator=(AfterglowCameraComponent&& rval) noexcept {
	AfterglowComponent::operator=(rval);
	_aspectRatio = rval._aspectRatio;
	_near = rval._near;
	_far = rval._far;
	_fovY = rval._fovY;
	_view = rval._view.load();
	_perspective = rval._perspective.load();
	_invView = rval._invView.load();
	_invPerspective = rval._invPerspective.load();
	return *this;
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
	// _view = glm::inverse(glm::translate(glm::mat4(1.0f), transform.globalTranslation()) * glm::mat4(transform.globalRotation()));
	_view = glm::transpose(transform.globalInvTransTransformMatrix());

	// Perspective
	glm::mat4 perspectMat{};
	if constexpr (cfg::reverseDepth) {
		perspectMat = reversedDepthPerspective();
	}
	else {
		perspectMat = glm::perspective(_fovY, _aspectRatio, _near, _far);
	}	
	perspectMat[1][1] *= -1;  // Reverse Y
	_perspective = perspectMat;

	// InvView
	_invView = glm::inverse(_view.load());

	// InvPerspective
	_invPerspective = glm::inverse(_perspective.load());
}

inline glm::mat4 AfterglowCameraComponent::reversedDepthPerspective() const noexcept {
	const float tanHalfFovy = tan(_fovY / 2.0f);
	glm::mat4 result(0);
	result[0][0] = 1.0f / (_aspectRatio * tanHalfFovy);
	result[1][1] = 1.0f / (tanHalfFovy);
	result[2][2] = (_near + _far) / (_near - _far);
	result[2][3] = -1.0f;
	result[3][2] = (2.0f * _near * _far) / (_near - _far);
	return result;
}

