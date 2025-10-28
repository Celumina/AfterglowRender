#pragma once
#include "AfterglowComponent.h"
#include "AfterglowTransform.h"

#include <glm/glm.hpp>

// TODO: Camera rotation transform apply to target.
class AfterglowCameraComponent : public AfterglowComponent<AfterglowCameraComponent> {
public:
	AfterglowCameraComponent();

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
	float  _aspectRatio;
	float _near;
	float _far;
	float _fovY;

	// Precalculating value, to avoid calculated by every entity.
	glm::mat4 _view;
	glm::mat4 _perspective;
	glm::mat4 _invView;
	glm::mat4 _invPerspective;
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