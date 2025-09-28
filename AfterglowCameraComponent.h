#pragma once
#include "AfterglowComponent.h"
#include "AfterglowTransform.h"

#include <glm/glm.hpp>

// TODO: Camera rotation transform apply to target.
class AfterglowCameraComponent : public AfterglowComponent<AfterglowCameraComponent> {
public:
	AfterglowCameraComponent();

	float aspectRatio() const;
	float near() const;
	float far() const;
	float fovY() const;

	void setAspectRatio(float aspectRatio);
	void setNear(float near);
	void setFar(float far);
	void setFovY(float rad);

	void setGlobalTarget(const AfterglowTranslation& globalTarget);
	void setTarget(const AfterglowTranslation& target);

	glm::mat4 view() const;
	glm::mat4 perspective() const;

private:
	float  _aspectRatio;
	float _near;
	float _far;
	float _fovY;
};

