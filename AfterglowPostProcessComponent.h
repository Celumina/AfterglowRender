#pragma once
#include "AfterglowComponent.h"

class AfterglowPostProcessComponent : public AfterglowComponent<AfterglowPostProcessComponent> {
public:
	void setPostProcessMaterial(const std::string& materialName);
	const std::string& postProcessMaterialName() const;

private:
	std::string _materialName;
};

INR_CLASS(AfterglowPostProcessComponent) {
	INR_BASE_CLASSES<AfterglowComponent<AfterglowPostProcessComponent>>;
	INR_FUNCS(
		INR_FUNC(setPostProcessMaterial), 
		INR_FUNC(postProcessMaterialName)
	);
};