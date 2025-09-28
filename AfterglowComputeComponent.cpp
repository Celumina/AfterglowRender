#include "AfterglowComputeComponent.h"

void AfterglowComputeComponent::setComputeMaterial(const std::string materialName) {
	_materialName = materialName;
}

const std::string& AfterglowComputeComponent::computeMaterialName() const {
	return _materialName;
}

