#include "AfterglowTransformComponent.h"

// #include <glm/gtx/euler_angles.hpp>

AfterglowTranslation AfterglowTransformComponent::translation() const noexcept {
	return _transform.translation;
}

AfterglowQuaternion AfterglowTransformComponent::rotation() const noexcept {
	return _transform.rotation;
}

AfterglowScaling AfterglowTransformComponent::scaling() const noexcept {
	return _transform.scaling;
}

AfterglowTransform AfterglowTransformComponent::transform() const noexcept {
	return _transform;
}

AfterglowEuler AfterglowTransformComponent::euler() const noexcept {
	return glm::eulerAngles(_transform.rotation);
}

AfterglowDirection AfterglowTransformComponent::direction() const noexcept {
	return _transform.rotation * glm::vec4{ 0.0f, cardinal::Forward };
}

void AfterglowTransformComponent::setTranslation(const AfterglowTranslation& translation)  noexcept {
	_transformChanged = true;
	_transform.translation = translation;
}

void AfterglowTransformComponent::setRotation(const AfterglowQuaternion& rotation)  noexcept {
	_transformChanged = true;
	_transform.rotation = rotation;
}

void AfterglowTransformComponent::setScaling(const AfterglowScaling& scaling)  noexcept {
	_transformChanged = true;
	_transform.scaling = scaling;
}

void AfterglowTransformComponent::setTransform(const AfterglowTransform& transform)  noexcept {
	_transformChanged = true;
	_transform = transform;
}

void AfterglowTransformComponent::setEuler(const AfterglowEuler& euler)  noexcept {
	_transformChanged = true;
	_transform.rotation = glm::qua(euler);
}

glm::mat4 AfterglowTransformComponent::transformMatrix() const  noexcept {
	// Translation <- Rotation <- Scaling
	glm::mat4 mat = glm::translate(glm::mat4(1.0f), translation());
	mat *= glm::mat4_cast(rotation());
	mat = glm::scale(mat, scaling());
	return mat;
}

bool AfterglowTransformComponent::transformChanged() const noexcept {
	if (entity().parent()) {
		auto& parentTransformComponent = entity().parent()->get<AfterglowTransformComponent>();
		return _transformChanged || parentTransformComponent.transformChanged();
	}
	return _transformChanged;
}

void AfterglowTransformComponent::updateGlobalTransform() const noexcept {
	if (!transformChanged()) {
		return;
	}
	_globalTransform = _transform;
	if (entity().parent()) {
		auto& parentTransformComponent = entity().parent()->get<AfterglowTransformComponent>();
		
		_globalTransform.translation = 
			parentTransformComponent.globalRotation()
			* (_globalTransform.translation * parentTransformComponent.globalScaling())
			+ parentTransformComponent.globalTranslation();

		_globalTransform.rotation = parentTransformComponent.globalRotation() * _globalTransform.rotation;
		_globalTransform.scaling *= parentTransformComponent.globalScaling();
	}
	_transformChanged = false;
}

AfterglowTranslation AfterglowTransformComponent::globalTranslation() const noexcept {
	updateGlobalTransform();
	return _globalTransform.translation;
}

AfterglowQuaternion AfterglowTransformComponent::globalRotation() const noexcept {
	updateGlobalTransform();
	return _globalTransform.rotation;
}

AfterglowScaling AfterglowTransformComponent::globalScaling() const noexcept {
	updateGlobalTransform();
	return _globalTransform.scaling;
}

AfterglowTransform AfterglowTransformComponent::globalTransform() const noexcept {
	updateGlobalTransform();
	return _globalTransform;
}

AfterglowEuler AfterglowTransformComponent::globalEuler() const noexcept {
	return glm::eulerAngles(globalRotation());
}

AfterglowDirection AfterglowTransformComponent::globalDirection() const noexcept {
	// Rotate foward vector by quat.
	// vec4 vq = {0, v};
	// vq' = q * vq * inverse(q).
	// But in glm, just q * vq, inverse(q) is not required. 
	return globalRotation() * glm::vec4{0.0f, cardinal::Forward};
}

// #include "DebugUtilities.h"
void AfterglowTransformComponent::setGlobalTranslation(const AfterglowTranslation& globalTranslation) noexcept {
	updateGlobalTransform();
	_globalTransform.translation = globalTranslation;
	if (entity().parent()) {
		auto& parentTransformComponent = entity().parent()->get<AfterglowTransformComponent>();
		setTranslation(globalTranslation - parentTransformComponent.globalTranslation());
	}
	else {
		// setTranslation(AfterglowTransform::defaultTranslation());
		setTranslation(globalTranslation);
	}
}

void AfterglowTransformComponent::setGlobalRotation(const AfterglowQuaternion& globalRotation) noexcept {
	updateGlobalTransform();
	_globalTransform.rotation = globalRotation;
	if (entity().parent()) { 
		auto& parentTransformComponent = entity().parent()->get<AfterglowTransformComponent>();
		setRotation(glm::conjugate(parentTransformComponent.globalRotation()) * globalRotation);
	}
	else {
		// setRotation(AfterglowTransform::defaultRotation());
		setRotation(globalRotation);
	}
}

void AfterglowTransformComponent::setGlobalScaling(const AfterglowScaling& globalScaling) noexcept {
	updateGlobalTransform();
	_globalTransform.scaling = globalScaling;
	if (entity().parent()) {
		auto& parentTransformComponent = entity().parent()->get<AfterglowTransformComponent>();
		setScaling(globalScaling / parentTransformComponent.globalScaling());
	}
	else {
		// setScaling(AfterglowTransform::defaultScaling());
		setScaling(globalScaling);
	}
}

void AfterglowTransformComponent::setGlobalTransform(const AfterglowTransform& globalTransform) noexcept {
	updateGlobalTransform();
	_globalTransform = globalTransform;
	if (entity().parent()) {
		auto& parentTransformComponent = entity().parent()->get<AfterglowTransformComponent>();
		setTranslation(globalTransform.translation - parentTransformComponent.globalTranslation());
		setRotation(glm::conjugate(parentTransformComponent.globalRotation()) * globalTransform.rotation);
		setScaling(globalTransform.scaling / parentTransformComponent.globalScaling());
	}
	else {
		setTransform(globalTransform);
	}
}

void AfterglowTransformComponent::setGlobalEuler(const AfterglowEuler& globalEuler) {
	setGlobalRotation(glm::qua(globalEuler));
}

glm::mat4 AfterglowTransformComponent::globalTransformMatrix() const noexcept {
	// Translation <- Rotation <- Scaling
	glm::mat4 mat = glm::translate(glm::mat4(1.0f), globalTranslation());
	mat *= glm::mat4_cast(globalRotation());
	mat = glm::scale(mat, globalScaling());
	return mat;
}
