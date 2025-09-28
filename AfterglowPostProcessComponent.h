#pragma once
#include "AfterglowComponent.h"

class AfterglowPostProcessComponent : public AfterglowComponent<AfterglowPostProcessComponent > {
public:
	void setPostProcessMaterial(const std::string& materialName);
	const std::string& postProcessMaterialName() const;

private:
	std::string _materialName;
};

