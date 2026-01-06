#pragma once


#include <atomic>
#include <glm/glm.hpp>

#include "AfterglowComponent.h"
#include "AfterglowTransform.h"

// TODO: Camera rotation transform apply to target.
class AfterglowCameraComponent : public AfterglowComponent<AfterglowCameraComponent> {
public:
	AfterglowCameraComponent();
	~AfterglowCameraComponent() = default;

	AfterglowCameraComponent(const AfterglowCameraComponent& other);
	AfterglowCameraComponent& operator=(const AfterglowCameraComponent& other);

	AfterglowCameraComponent(AfterglowCameraComponent&& rval) noexcept;
	AfterglowCameraComponent& operator=(AfterglowCameraComponent&& rval) noexcept;

	float aspectRatio() const noexcept;
	float near() const noexcept;
	float far() const noexcept;
	float fovY() const noexcept;

	void setAspectRatio(float aspectRatio) noexcept;
	void setNear(float near) noexcept;
	void setFar(float far) noexcept;
	void setFovY(float rad) noexcept;

	void setGlobalTarget(const AfterglowTranslation& globalTarget) noexcept;
	void setTarget(const AfterglowTranslation& target) noexcept;

	const glm::mat4& view() const noexcept;
	const glm::mat4& perspective() const noexcept;
	const glm::mat4& invView() const noexcept;
	const glm::mat4& invPerspective() const noexcept;

	void updateMatrices() noexcept;

private:
	inline glm::mat4 reversedDepthPerspective() const noexcept;

	float  _aspectRatio;
	float _near;
	float _far;
	float _fovY;

	// Precalculating value, to avoid calculated by every entity.
	mutable std::atomic<glm::mat4> _view;
	mutable std::atomic<glm::mat4> _perspective;
	mutable std::atomic<glm::mat4> _invView;
	mutable std::atomic<glm::mat4> _invPerspective;
};


INR_CLASS(AfterglowCameraComponent) {
	INR_BASE_CLASSES<AfterglowComponent<AfterglowCameraComponent>>;
	INR_FUNCS(
		INR_FUNC(aspectRatio),
		INR_FUNC(near),
		INR_FUNC(far),
		INR_FUNC(fovY),
		INR_FUNC(setAspectRatio),
		INR_FUNC(setNear),
		INR_FUNC(setFar),
		INR_FUNC(setFovY),
		INR_FUNC(setGlobalTarget),
		INR_FUNC(setTarget),
		INR_FUNC(view),
		INR_FUNC(perspective),
		INR_FUNC(invView),
		INR_FUNC(invPerspective),
		INR_FUNC(updateMatrices)
	);
};