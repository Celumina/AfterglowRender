#pragma once
#include "AfterglowComponent.h"
#include "AfterglowTransform.h"

class AfterglowTransformComponent : public AfterglowComponent<AfterglowTransformComponent> {
public:
	// Global transform (WorldSpace)
	// TODO: Also apply parent transform.
	AfterglowTranslation globalTranslation() const noexcept;
	AfterglowQuaternion globalRotation() const noexcept;
	AfterglowScaling globalScaling() const noexcept;
	AfterglowTransform globalTransform() const noexcept;

	AfterglowEuler globalEuler() const noexcept;
	AfterglowDirection globalDirection() const noexcept;
	AfterglowDirection globalForward() const noexcept;
	AfterglowDirection globalRight() const noexcept;


	void setGlobalTranslation(const AfterglowTranslation& globalTranslation) noexcept;
	void setGlobalRotation(const AfterglowQuaternion& globalRotation) noexcept;
	void setGlobalScaling(const AfterglowScaling& globalScaling) noexcept;
	void setGlobalTransform(const AfterglowTransform& globalTransform) noexcept;

	void setGlobalEuler(const AfterglowEuler& globalEuler);

	glm::mat4 globalTransformMatrix() const noexcept;
	
	// Relative transform (LocalSpace)
	AfterglowTranslation translation() const noexcept;
	AfterglowQuaternion rotation() const noexcept;
	AfterglowScaling scaling() const noexcept;
	AfterglowTransform transform() const noexcept;

	AfterglowEuler euler() const noexcept;

	AfterglowDirection direction() const noexcept;

	void setTranslation(const AfterglowTranslation& translation) noexcept;
	void setRotation(const AfterglowQuaternion& rotation) noexcept;
	void setScaling(const AfterglowScaling& scaling) noexcept;
	void setTransform(const AfterglowTransform& transform) noexcept;

	void setEuler(const AfterglowEuler& euler) noexcept;

	glm::mat4 transformMatrix() const noexcept;

	// Recursive function, considering parent's transform changement.
	bool transformChanged() const noexcept;

private:
	void updateGlobalTransform() const noexcept;

	AfterglowTransform _transform {};

	// If transform changed, recalculate global transform when globalTransformMatrix() was called.
	mutable bool _transformChanged = true;
	mutable AfterglowTransform _globalTransform {};
};

