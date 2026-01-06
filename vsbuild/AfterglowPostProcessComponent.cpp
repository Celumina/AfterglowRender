#include "AfterglowPostProcessComponent.h"
#include "AfterglowShapeMeshResource.h"

AfterglowPostProcessComponent::AfterglowPostProcessComponent() = default;
AfterglowPostProcessComponent::~AfterglowPostProcessComponent() = default;

AfterglowPostProcessComponent::AfterglowPostProcessComponent(AfterglowPostProcessComponent&&) noexcept = default;
AfterglowPostProcessComponent& AfterglowPostProcessComponent::operator=(AfterglowPostProcessComponent&&) noexcept = default;

AfterglowPostProcessComponent::AfterglowPostProcessComponent(const AfterglowPostProcessComponent& other) : 
	_materialName(other._materialName), _shapeResource(nullptr) {
}

AfterglowPostProcessComponent& AfterglowPostProcessComponent::operator=(const AfterglowPostProcessComponent& other) {
	_materialName = other._materialName;
	_shapeResource = nullptr;
	return *this;
}

void AfterglowPostProcessComponent::setPostProcessMaterial(const std::string& materialName) {
	_materialName = materialName;
}

const std::string& AfterglowPostProcessComponent::postProcessMaterialName() const {
	return _materialName;
}

//void AfterglowPostProcessComponent::awake() {
//	AfterglowComputeComponent& computeComponent = reinterpret_cast<AfterglowComputeComponent&>(
//		sysUtils().addComponent(entity(), util::TypeIndex<AfterglowComputeComponent>())
//	);
//	computeComponent.setComputeMaterial(_materialName);
//}
