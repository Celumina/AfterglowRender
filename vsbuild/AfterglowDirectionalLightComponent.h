#pragma once

#include <glm/glm.hpp>

#include "AfterglowLightComponent.h"
class AfterglowDirectionalLightComponent : public AfterglowLightComponent<AfterglowDirectionalLightComponent> {
public:
	// @brief: Modifiy light direction in transformComponent.
};

INR_CLASS(AfterglowDirectionalLightComponent) {
	INR_BASE_CLASSES<AfterglowLightComponent<AfterglowDirectionalLightComponent>>;
};