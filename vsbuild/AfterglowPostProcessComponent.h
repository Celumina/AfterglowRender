#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "AfterglowActionComponent.h"

class AfterglowPassSetBase;
class AfterglowBloomPassSet;

class AfterglowPostProcessComponent : public AfterglowActionComponent<AfterglowPostProcessComponent> {
public:
	// @param index: 0 to n, from high resolution to low.
	void setBloomIntensity(uint32_t downSamplingIndex, glm::vec4 intensity);

	void enableBloom() noexcept;
	void disableBloom() noexcept;

	void awake();
	void onEnable();
	void onDisable();
	void onRenderBegin();

private:
	/**
	* @brief Set fragment stage scalar of the post process material instance.
	* @note To modify a amount of scalars, set scalars from sysUtils directly are better for performance.
	*/ 
	inline void setMaterialInstanceScalar(const std::string& name, float value);

	std::string _postProcessMaterialName;
	AfterglowBloomPassSet* _bloomPassSet;
};

INR_CLASS(AfterglowPostProcessComponent) {
	INR_BASE_CLASSES<AfterglowActionComponent<AfterglowPostProcessComponent>>;
	INR_FUNCS(
		INR_FUNC(setBloomIntensity), 
		INR_FUNC(enableBloom), 
		INR_FUNC(disableBloom)
	);
};