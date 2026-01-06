#pragma once
#include <atomic>
#include "AfterglowComponent.h"
#include "AfterglowTransform.h"

class AfterglowTransformComponent : public AfterglowComponent<AfterglowTransformComponent> {
public:
	AfterglowTransformComponent() = default;
	~AfterglowTransformComponent() = default;

	AfterglowTransformComponent(const AfterglowTransformComponent& other);
	AfterglowTransformComponent& operator=(const AfterglowTransformComponent& other);

	AfterglowTransformComponent(AfterglowTransformComponent&& rval) noexcept;
	AfterglowTransformComponent& operator=(AfterglowTransformComponent&& rval) noexcept;

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

	inline glm::mat4 globalTransformMatrix() const noexcept { return _globalMat; }
	inline glm::mat4 globalInvTransTransformMatrix() const noexcept { return _globalInvTransMat; }
	
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

	void updateGlobalMatrices() const noexcept;

private:
	void updateGlobalTransform() const noexcept;


	AfterglowTransform _transform {};

	// If transform changed, recalculate global transform when globalTransformMatrix() was called.
	mutable bool _transformChanged = true;
	mutable AfterglowTransform _globalTransform {};

	// Cache global mat to improve renderer performance.
	mutable std::atomic<glm::mat4> _globalMat;
	mutable std::atomic<glm::mat4> _globalInvTransMat;
};

INR_CLASS(AfterglowTransformComponent) {
	INR_BASE_CLASSES<AfterglowComponent<AfterglowTransformComponent>>;
	INR_FUNCS(
		INR_FUNC(globalTranslation),
		INR_FUNC(globalRotation),
		INR_FUNC(globalScaling),
		INR_FUNC(globalTransform),
		INR_FUNC(globalEuler),
		INR_FUNC(globalDirection),
		//INR_FUNC(globalForward),
		//INR_FUNC(globalRight),
		INR_FUNC(setGlobalTranslation),
		INR_FUNC(setGlobalRotation),
		INR_FUNC(setGlobalScaling),
		INR_FUNC(setGlobalTransform),
		INR_FUNC(setGlobalEuler),
		INR_FUNC(globalTransformMatrix),
		INR_FUNC(translation),
		INR_FUNC(rotation),
		INR_FUNC(scaling),
		INR_FUNC(transform),
		INR_FUNC(euler),
		INR_FUNC(direction),
		INR_FUNC(setTranslation),
		INR_FUNC(setRotation),
		INR_FUNC(setScaling),
		INR_FUNC(setTransform),
		INR_FUNC(setEuler),
		INR_FUNC(transformMatrix),
		INR_FUNC(transformChanged), 
		INR_FUNC(updateGlobalMatrices)
	);
};