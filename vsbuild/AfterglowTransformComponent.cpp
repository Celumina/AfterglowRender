#include "AfterglowTransformComponent.h"


AfterglowTransformComponent::AfterglowTransformComponent(const AfterglowTransformComponent& other) :
	AfterglowComponent(other),
	_transform(other._transform),
	_transformChanged(other._transformChanged),
	_globalTransform(other._globalTransform),
	_globalMat(other._globalMat.load()),
	_globalInvTransMat(other._globalInvTransMat.load()) {
}

AfterglowTransformComponent& AfterglowTransformComponent::operator=(const AfterglowTransformComponent& other) {
	AfterglowComponent::operator=(other);
	_transform = other._transform;
	_transformChanged = other._transformChanged;
	_globalTransform = other._globalTransform;
	_globalMat = other._globalMat.load();
	_globalInvTransMat = other._globalInvTransMat.load();
	return *this;
}

AfterglowTransformComponent::AfterglowTransformComponent(AfterglowTransformComponent&& rval) noexcept :
	AfterglowComponent(rval),
	_transform(std::move(rval._transform)),
	_transformChanged(std::move(rval._transformChanged)),
	_globalTransform(std::move(rval._globalTransform)),
	_globalMat(rval._globalMat.load()),
	_globalInvTransMat(rval._globalInvTransMat.load()) {
}

AfterglowTransformComponent& AfterglowTransformComponent::operator=(AfterglowTransformComponent&& rval) noexcept {
	AfterglowComponent::operator=(rval);
	_transform = std::move(rval._transform);
	_transformChanged = std::move(rval._transformChanged);
	_globalTransform = std::move(rval._globalTransform);
	_globalMat = rval._globalMat.load();
	_globalInvTransMat = rval._globalInvTransMat.load();
	return *this;
}

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

void AfterglowTransformComponent::updateGlobalMatrices() const noexcept {
	// Transform
	// Translation <- Rotation <- Scaling
	glm::mat4 mat = glm::translate(glm::mat4(1.0f), globalTranslation());
	mat *= glm::mat4_cast(globalRotation());
	mat = glm::scale(mat, globalScaling());
	_globalMat = mat;

	// Invesed transposed transform
	_globalInvTransMat = glm::transpose(glm::inverse(mat));
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

AfterglowDirection AfterglowTransformComponent::globalViewDirection() const noexcept {
	// Rotate foward vector by quat.
	// vec4 vq = {0, v};
	// vq' = q * vq * inverse(q).
	// But in glm, just q * vq, inverse(q) is not required. 
	return glm::normalize(globalRotation() * cardinal::Up); 
}

AfterglowDirection AfterglowTransformComponent::globalForward() const noexcept {
	return glm::normalize(globalRotation() * cardinal::Forward);
}

AfterglowDirection AfterglowTransformComponent::globalRight() const noexcept {
	return glm::normalize(globalRotation() * cardinal::Right);
}

AfterglowDirection AfterglowTransformComponent::globalUp() const noexcept {
	return glm::normalize(globalRotation() * cardinal::Up);
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
