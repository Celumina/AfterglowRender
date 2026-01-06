#pragma once
#include <memory>
#include "AfterglowComponent.h"

class AfterglowShapeMeshResource;

class AfterglowPostProcessComponent : public AfterglowComponent<AfterglowPostProcessComponent> {
public:
	AfterglowPostProcessComponent();
	~AfterglowPostProcessComponent();

	AfterglowPostProcessComponent(AfterglowPostProcessComponent&&) noexcept;
	AfterglowPostProcessComponent& operator=(AfterglowPostProcessComponent&&) noexcept;

	AfterglowPostProcessComponent(const AfterglowPostProcessComponent& other);
	AfterglowPostProcessComponent& operator=(const AfterglowPostProcessComponent& other);

	void setPostProcessMaterial(const std::string& materialName);
	const std::string& postProcessMaterialName() const;

	inline std::unique_ptr<AfterglowShapeMeshResource>& shapeResource() noexcept { return _shapeResource; }
	inline const std::unique_ptr<AfterglowShapeMeshResource>& shapeResource() const noexcept { return _shapeResource; }

private:
	std::string _materialName;
	std::unique_ptr<AfterglowShapeMeshResource> _shapeResource;
};

INR_CLASS(AfterglowPostProcessComponent) {
	INR_BASE_CLASSES<AfterglowComponent<AfterglowPostProcessComponent>>;
	INR_FUNCS(
		INR_FUNC(setPostProcessMaterial), 
		INR_FUNC(postProcessMaterialName), 
		INR_OVERLOADED_FUNC(std::unique_ptr<AfterglowShapeMeshResource>& (AfterglowPostProcessComponent::*)(), shapeResource),
		INR_OVERLOADED_FUNC(const std::unique_ptr<AfterglowShapeMeshResource>& (AfterglowPostProcessComponent::*)() const, shapeResource)
	);
};