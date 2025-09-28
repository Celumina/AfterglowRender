#include "AfterglowPostProcessComponent.h"

void AfterglowPostProcessComponent::setPostProcessMaterial(const std::string& materialName) {
	_materialName = materialName;
}

const std::string& AfterglowPostProcessComponent::postProcessMaterialName() const {
	return _materialName;
}
